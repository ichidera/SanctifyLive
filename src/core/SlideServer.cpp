#include "SlideServer.h"

#include <QBuffer>
#include <QFileInfo>
#include <QFuture>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include "ScheduleModel.h"
#include "Slide.h"

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
    connect(m_model, &ScheduleModel::phoneContentChanged, this, &SlideServer::onPhoneContentChanged);

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

    for (QTcpSocket *socket : std::as_const(m_clientStates).keys()) {
        socket->disconnect(this);
        socket->disconnectFromHost();
        socket->deleteLater();
    }
    m_clientStates.clear();

    m_server->close();
    emit clientCountChanged(0);
    emit clientsChanged({});
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
    return m_clientStates.size();
}

QVector<SlideServer::ClientInfo> SlideServer::clients() const
{
    QVector<ClientInfo> result;
    result.reserve(m_clientStates.size());
    for (auto it = m_clientStates.constBegin(); it != m_clientStates.constEnd(); ++it) {
        ClientInfo info;
        info.address = it.key()->peerAddress().toString() + QStringLiteral(":")
                     + QString::number(it.key()->peerPort());
        info.role = it.value().role;
        info.resolution = it.value().resolution;
        result.append(info);
    }
    return result;
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

        m_clientStates.insert(socket, ClientState{});
        connect(socket, &QTcpSocket::disconnected, this, &SlideServer::onClientDisconnected);
        connect(socket, &QTcpSocket::readyRead, this, &SlideServer::onClientReadyRead);

        // Bring the new client up to date immediately rather than making
        // it wait for the next live change or ping. Its hello (role)
        // hasn't arrived yet, so this assumes Display -- the original,
        // pre-role behavior -- and gets corrected within onClientReadyRead
        // moments later if it's actually a phone.
        sendFrameTo(socket, buildFrameForRole(Role::Display));
    }

    emit clientCountChanged(m_clientStates.size());
    emit clientsChanged(clients());
}

void SlideServer::onClientDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    m_clientStates.remove(socket);
    socket->deleteLater();
    emit clientCountChanged(m_clientStates.size());
    emit clientsChanged(clients());
}

void SlideServer::onClientReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    auto it = m_clientStates.find(socket);
    if (it == m_clientStates.end())
        return;

    ClientState &state = it.value();
    if (state.helloReceived) {
        // The protocol has nothing else for a client to say (yet) --
        // drain and ignore rather than letting the socket's read buffer
        // grow unbounded if something unexpected gets sent.
        socket->readAll();
        return;
    }

    state.recvBuffer.append(socket->readAll());
    const int newlineIdx = state.recvBuffer.indexOf('\n');
    if (newlineIdx < 0)
        return; // hello hasn't fully arrived yet

    const QByteArray line = state.recvBuffer.left(newlineIdx);
    state.recvBuffer.clear(); // only the one hello line is ever expected
    state.helloReceived = true;

    const QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject())
        return; // malformed hello -- stays Role::Display with unknown resolution
    const QJsonObject obj = doc.object();
    if (obj.value(QStringLiteral("type")).toString() != QStringLiteral("hello"))
        return;

    const Role newRole = (obj.value(QStringLiteral("role")).toString() == QStringLiteral("phone"))
        ? Role::Phone : Role::Display;
    const int w = obj.value(QStringLiteral("w")).toInt();
    const int h = obj.value(QStringLiteral("h")).toInt();

    state.role = newRole;
    if (w > 0 && h > 0)
        state.resolution = QSize(w, h);

    emit clientsChanged(clients());

    // The client sat there for a moment (assumed Display) before this
    // hello told us what it actually is -- if it turned out to be a
    // phone, get it the right content now instead of waiting for the
    // next unrelated change.
    if (newRole == Role::Phone)
        sendFrameTo(socket, buildFrameForRole(Role::Phone));
}

void SlideServer::onLiveContentChanged()
{
    pushToRole(Role::Display, buildFrameForRole(Role::Display));
    if (const Slide *slide = m_model->liveSlide())
        ensureImageEncoded(slide->backgroundImagePath);
}

void SlideServer::onPhoneContentChanged()
{
    pushToRole(Role::Phone, buildFrameForRole(Role::Phone));
    if (const Slide *slide = m_model->phoneSlide())
        ensureImageEncoded(slide->backgroundImagePath);
}

void SlideServer::wakeAll()
{
    pushToAll(buildWakeFrame());
}

void SlideServer::sendPing()
{
    if (m_clientStates.isEmpty())
        return;
    pushToAll(QJsonDocument(QJsonObject{{"type", QStringLiteral("ping")}})
                  .toJson(QJsonDocument::Compact) + "\n");
}

void SlideServer::pushToAll(const QByteArray &frame)
{
    for (QTcpSocket *socket : std::as_const(m_clientStates).keys())
        sendFrameTo(socket, frame);
}

void SlideServer::pushToRole(Role role, const QByteArray &frame)
{
    for (auto it = m_clientStates.constBegin(); it != m_clientStates.constEnd(); ++it) {
        if (it.value().role == role)
            sendFrameTo(it.key(), frame);
    }
}

void SlideServer::sendFrameTo(QTcpSocket *socket, const QByteArray &frame)
{
    if (socket->state() == QAbstractSocket::ConnectedState)
        socket->write(frame);
}

QByteArray SlideServer::buildFrameForRole(Role role) const
{
    const Slide *slide = (role == Role::Phone) ? m_model->phoneSlide() : m_model->liveSlide();

    // Blackout applies identically to both roles today -- there's no
    // "blackout the sanctuary screen but keep the phone lit" concept
    // (nor an obvious need for one), so this falls straight through to
    // ScheduleModel's existing blackout flag via liveSlide()/phoneSlide()
    // both returning nullptr while blacked out.
    if (m_model->isBlackout() || !slide)
        return buildBlackoutFrame();
    return buildSlideFrame(slide);
}

QByteArray SlideServer::buildSlideFrame(const Slide *slide) const
{
    QJsonObject obj;
    obj["type"] = QStringLiteral("slide");
    obj["text"] = slide->text;
    obj["bg"] = slide->background.name();
    // Fixed for now -- Slide has no per-slide foreground color yet, and
    // OutputWindow itself always renders white text (see its paintEvent),
    // so this keeps the Android client visually consistent with it.
    obj["fg"] = QStringLiteral("#ffffff");

    if (!slide->backgroundImagePath.isEmpty()) {
        obj["fx"] = slide->backgroundFocus.x();
        obj["fy"] = slide->backgroundFocus.y();

        const auto cacheIt = m_imageCache.constFind(slide->backgroundImagePath);
        // Only trust the cache entry if it's still fresh (same
        // last-modified time) -- an edited-and-reimported file at the
        // same path should not keep serving its old picture.
        if (cacheIt != m_imageCache.constEnd()
            && cacheIt->modified == QFileInfo(slide->backgroundImagePath).lastModified()) {
            obj["image"] = cacheIt->base64;
        }
        // Else: leave "image" out entirely (rather than an empty string)
        // so the client's fallback to the solid `bg` color kicks in --
        // either the file failed to load, or (the common case) it's
        // still being encoded on the worker thread and a follow-up frame
        // with the image will arrive shortly. See ensureImageEncoded().
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

void SlideServer::ensureImageEncoded(const QString &path)
{
    if (path.isEmpty())
        return;

    const auto cacheIt = m_imageCache.constFind(path);
    if (cacheIt != m_imageCache.constEnd()
        && cacheIt->modified == QFileInfo(path).lastModified()) {
        return; // already cached and fresh -- buildSlideFrame() already has it
    }
    if (m_pendingImagePaths.contains(path))
        return; // an encode for this exact path is already in flight

    m_pendingImagePaths.insert(path);

    // .then(this, ...) hops the continuation back onto this object's
    // thread (the GUI thread) automatically, so everything after the
    // worker call below is safe to touch m_imageCache/m_model/sockets
    // from -- no manual queued-connection plumbing needed, and unlike a
    // single shared QFutureWatcher this supports the display and phone
    // roles independently encoding two different images at once.
    QtConcurrent::run(&SlideServer::encodeImageForPath, path)
        .then(this, [this](EncodedImage result) {
            m_pendingImagePaths.remove(result.path);
            if (result.base64.isEmpty())
                return; // decode/encode failed -- clients keep the solid fallback color
            m_imageCache[result.path] = CachedImage{result.modified, result.base64};
            refreshClientsShowingImage(result.path);
        });
}

void SlideServer::refreshClientsShowingImage(const QString &path)
{
    // Whichever role(s) are actually showing this exact image right now
    // get a fresh push with it included; a role showing something else
    // (or nothing) is left alone.
    if (const Slide *slide = m_model->liveSlide(); slide && slide->backgroundImagePath == path)
        pushToRole(Role::Display, buildFrameForRole(Role::Display));
    if (const Slide *slide = m_model->phoneSlide(); slide && slide->backgroundImagePath == path)
        pushToRole(Role::Phone, buildFrameForRole(Role::Phone));
}

SlideServer::EncodedImage SlideServer::encodeImageForPath(QString path)
{
    // Runs on a QtConcurrent worker thread -- must not touch `this`,
    // ScheduleModel, or any socket. Everything it needs comes in as a
    // by-value argument, and everything it produces goes out as a
    // by-value return, so there's nothing here that needs a mutex.
    EncodedImage result;
    result.path = path;
    result.modified = QFileInfo(path).lastModified();

    QImageReader reader(path);
    reader.setAutoTransform(true); // respect EXIF rotation from phone cameras

    const QSize original = reader.size();
    if (original.isValid()
        && (original.width() > kMaxImageDimension || original.height() > kMaxImageDimension)) {
        // Ask the decoder to downsample WHILE decoding (e.g. libjpeg's
        // DCT scaling) instead of decoding at full resolution and then
        // throwing most of it away with QImage::scaled() afterward --
        // this is the actual fix for a large photo taking a visibly long
        // time to reach the display: a 24MP camera photo decoded at full
        // size costs real, felt time even before any scaling happens.
        reader.setScaledSize(original.scaled(kMaxImageDimension, kMaxImageDimension,
                                              Qt::KeepAspectRatio));
    }

    const QImage image = reader.read();
    if (image.isNull())
        return result; // base64 stays empty -- caller treats as failure

    QByteArray jpegBytes;
    QBuffer buffer(&jpegBytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPG", kJpegQuality);

    result.base64 = QString::fromLatin1(jpegBytes.toBase64());
    return result;
}
