#include "SlideServer.h"

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

    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";
}

QByteArray SlideServer::buildBlackoutFrame() const
{
    return QJsonDocument(QJsonObject{{"type", QStringLiteral("blackout")}})
               .toJson(QJsonDocument::Compact) + "\n";
}