#include "SlideServer.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>

#include "ScheduleModel.h"

namespace {
// Sent periodically so a client (and the OS's TCP stack / any NAT in
// between) can tell the connection is still alive even during long
// stretches with no slide changes -- see PROTOCOL.md.
constexpr int kPingIntervalMs = 15000;

// Background images are downsampled before encoding so a single slide
// frame never balloons into megabytes of base64 text: it keeps the
// (already infrequent) image pushes fast over WiFi/USB and keeps the
// client's JSON-line read + JPEG decode cheap. 1280px is comfortably
// larger than any tablet/projector this app targets edge-to-edge.
constexpr int kMaxImageDimension = 1280;
constexpr int kJpegQuality = 80;
}

SlideServer::SlideServer(ScheduleModel *model, QObject *parent)
    : QObject(parent), m_model(model), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &SlideServer::onNewConnection);
    connect(m_model, &ScheduleModel::liveContentChanged, this, &SlideServer::onLiveContentChanged);

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(kPingIntervalMs);
    connect(m_pingTimer, &QTimer::timeout, this, &SlideServer::sendPing);
}

bool SlideServer::start(quint16 port)
{
    if (m_server->isListening())
        return true;

    if (!m_server->listen(QHostAddress::Any, port))
        return false;

    m_pingTimer->start();
    return true;
}

void SlideServer::stop()
{
    m_pingTimer->stop();

    for (QTcpSocket *socket : std::as_const(m_clients)) {
        socket->disconnect(this);
        socket->disconnectFromHost();
        socket->deleteLater();
    }
    m_clients.clear();

    m_server->close();
    emit clientCountChanged(0);
}

bool SlideServer::isListening() const
{
    return m_server->isListening();
}

quint16 SlideServer::port() const
{
    return m_server->serverPort();
}

int SlideServer::clientCount() const
{
    return m_clients.size();
}

void SlideServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();

        // Disable Nagle's algorithm. Without this, small frames (every
        // slide/blackout/wake push is well under a packet) can sit
        // buffered for tens of milliseconds waiting for more data before
        // the OS actually sends them, which shows up as a felt delay
        // between the operator acting and the Android display updating.
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

        m_clients.insert(socket);
        connect(socket, &QTcpSocket::disconnected, this, &SlideServer::onClientDisconnected);

        // Bring the new client up to date immediately rather than making
        // it wait for the next live change or ping.
        socket->write(currentFrame());
    }

    emit clientCountChanged(m_clients.size());
}

void SlideServer::onClientDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    m_clients.remove(socket);
    socket->deleteLater();
    emit clientCountChanged(m_clients.size());
}

void SlideServer::onLiveContentChanged()
{
    pushToAll(currentFrame());
}

void SlideServer::wakeAll()
{
    pushToAll(buildWakeFrame());
}

void SlideServer::sendPing()
{
    if (m_clients.isEmpty())
        return;
    pushToAll(QJsonDocument(QJsonObject{{"type", QStringLiteral("ping")}})
                  .toJson(QJsonDocument::Compact) + "\n");
}

void SlideServer::pushToAll(const QByteArray &frame)
{
    for (QTcpSocket *socket : std::as_const(m_clients)) {
        if (socket->state() == QAbstractSocket::ConnectedState)
            socket->write(frame);
    }
}

QByteArray SlideServer::currentFrame() const
{
    if (m_model->isBlackout() || !m_model->liveSlide())
        return buildBlackoutFrame();
    return buildSlideFrame();
}

QByteArray SlideServer::buildSlideFrame() const
{
    const Slide *slide = m_model->liveSlide();

    QJsonObject obj;
    obj["type"] = QStringLiteral("slide");
    obj["text"] = slide ? slide->text : QString();
    obj["bg"] = slide ? slide->background.name() : QStringLiteral("#000000");
    // Fixed for now -- Slide has no per-slide foreground color yet, and
    // OutputWindow itself always renders white text (see its paintEvent),
    // so this keeps the Android client visually consistent with it.
    obj["fg"] = QStringLiteral("#ffffff");

    if (slide && !slide->backgroundImagePath.isEmpty()) {
        const QString encoded = encodedImageFor(slide->backgroundImagePath);
        // Leave "image" out entirely on failure (missing/unreadable file)
        // rather than sending an empty string -- the client's fallback to
        // the solid `bg` color only kicks in when the field is absent.
        if (!encoded.isEmpty())
            obj["image"] = encoded;
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";
}

QByteArray SlideServer::buildBlackoutFrame() const
{
    return QJsonDocument(QJsonObject{{"type", QStringLiteral("blackout")}})
               .toJson(QJsonDocument::Compact) + "\n";
}

QByteArray SlideServer::buildWakeFrame() const
{
    return QJsonDocument(QJsonObject{{"type", QStringLiteral("wake")}})
               .toJson(QJsonDocument::Compact) + "\n";
}

QString SlideServer::encodedImageFor(const QString &path) const
{
    const QDateTime modified = QFileInfo(path).lastModified();

    if (path == m_cachedImagePath && modified == m_cachedImageModified)
        return m_cachedImageBase64;

    const QImage image(path);
    if (image.isNull())
        return QString(); // file missing/unreadable -- caller falls back to solid color

    const QImage scaled = (image.width() > kMaxImageDimension || image.height() > kMaxImageDimension)
        ? image.scaled(kMaxImageDimension, kMaxImageDimension, Qt::KeepAspectRatio,
                        Qt::SmoothTransformation)
        : image;

    QByteArray jpegBytes;
    QBuffer buffer(&jpegBytes);
    buffer.open(QIODevice::WriteOnly);
    scaled.save(&buffer, "JPG", kJpegQuality);

    m_cachedImagePath = path;
    m_cachedImageModified = modified;
    m_cachedImageBase64 = QString::fromLatin1(jpegBytes.toBase64());
    return m_cachedImageBase64;
}