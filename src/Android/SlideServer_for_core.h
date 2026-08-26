#pragma once
// ============================================================
// SlideServer.h  — drop into src/core/
// ============================================================
// Listens on TCP port 55432.  Every time the ScheduleModel fires
// liveContentChanged() or a blackout is toggled, it pushes a
// newline-delimited JSON frame to every connected Android client.
//
// Wire it up in OperatorWindow (or wherever you create ScheduleModel):
//
//   m_server = new SlideServer(m_model, this);
//   m_server->start();
//
// No other setup needed.  The Android app will connect automatically.
// ============================================================

#include <QObject>
#include <QTcpServer>
#include <QSet>

class QTcpSocket;
class ScheduleModel;

class SlideServer : public QObject
{
    Q_OBJECT

public:
    static constexpr quint16 DEFAULT_PORT = 55432;

    explicit SlideServer(ScheduleModel *model, QObject *parent = nullptr);

    bool start(quint16 port = DEFAULT_PORT);
    void stop();
    quint16 port() const;

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onLiveContentChanged();
    void sendPing();             // called by a 2-s QTimer

private:
    void pushToAll(const QByteArray &frame);
    QByteArray buildSlideFrame() const;
    QByteArray buildBlackoutFrame() const;

    ScheduleModel          *m_model;
    QTcpServer             *m_server;
    QSet<QTcpSocket *>      m_clients;
};

// ============================================================
// Implementation sketch (put in SlideServer.cpp):
//
//   #include "SlideServer.h"
//   #include "ScheduleModel.h"
//   #include <QTcpSocket>
//   #include <QJsonDocument>
//   #include <QJsonObject>
//   #include <QTimer>
//   #include <QColor>
//
//   SlideServer::SlideServer(ScheduleModel *model, QObject *parent)
//       : QObject(parent), m_model(model), m_server(new QTcpServer(this))
//   {
//       connect(m_server, &QTcpServer::newConnection, this, &SlideServer::onNewConnection);
//       connect(m_model, &ScheduleModel::liveContentChanged, this, &SlideServer::onLiveContentChanged);
//       connect(m_model, &ScheduleModel::scheduleChanged,    this, &SlideServer::onLiveContentChanged);
//
//       auto *pingTimer = new QTimer(this);
//       connect(pingTimer, &QTimer::timeout, this, &SlideServer::sendPing);
//       pingTimer->start(2000);
//   }
//
//   bool SlideServer::start(quint16 port) {
//       return m_server->listen(QHostAddress::Any, port);
//   }
//
//   void SlideServer::onNewConnection() {
//       while (m_server->hasPendingConnections()) {
//           QTcpSocket *sock = m_server->nextPendingConnection();
//           m_clients.insert(sock);
//           connect(sock, &QTcpSocket::disconnected, this, &SlideServer::onClientDisconnected);
//           // Immediately push current state to the new client
//           const QByteArray frame = m_model->isBlackout() || !m_model->liveSlide()
//               ? buildBlackoutFrame() : buildSlideFrame();
//           sock->write(frame);
//       }
//   }
//
//   void SlideServer::onClientDisconnected() {
//       QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
//       m_clients.remove(sock);
//       sock->deleteLater();
//   }
//
//   void SlideServer::onLiveContentChanged() {
//       const bool blackout = m_model->isBlackout() || !m_model->liveSlide();
//       pushToAll(blackout ? buildBlackoutFrame() : buildSlideFrame());
//   }
//
//   QByteArray SlideServer::buildSlideFrame() const {
//       const Slide *s = m_model->liveSlide();
//       QJsonObject obj;
//       obj["type"] = "slide";
//       obj["text"] = s->text;
//       obj["bg"]   = s->background.name();   // e.g. "#1a1a2e"
//       obj["fg"]   = "#ffffff";               // extend Slide struct if you add fg
//       return QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";
//   }
//
//   QByteArray SlideServer::buildBlackoutFrame() const {
//       return QJsonDocument(QJsonObject{ {"type","blackout"} })
//                  .toJson(QJsonDocument::Compact) + "\n";
//   }
//
//   void SlideServer::pushToAll(const QByteArray &frame) {
//       for (QTcpSocket *sock : m_clients) sock->write(frame);
//   }
//
//   void SlideServer::sendPing() {
//       if (m_clients.isEmpty()) return;
//       static const QByteArray ping =
//           QJsonDocument(QJsonObject{{"type","ping"}}).toJson(QJsonDocument::Compact) + "\n";
//       pushToAll(ping);
//   }
// ============================================================
