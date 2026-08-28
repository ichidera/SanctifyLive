#ifndef SANCTIFYLIVE_CORE_SLIDESERVER_H_
#define SANCTIFYLIVE_CORE_SLIDESERVER_H_

#include <QObject>
#include <QSet>
#include <QTcpServer>

class QTcpSocket;
class QTimer;
class ScheduleModel;

// SlideServer is the desktop side of the Android "stage display" link.
// It listens on a TCP port and pushes newline-delimited JSON frames to
// every connected client whenever the live content changes -- see
// src/android/PROTOCOL.md for the wire format. This is push-only: the
// server never expects anything back from a client.
//
// One SlideServer per ScheduleModel is expected (created and owned by
// OperatorWindow). It intentionally has no knowledge of Qt widgets --
// it only touches ScheduleModel, so it could equally serve a future
// second Android client, a web remote, or anything else that speaks
// this protocol.
//
// KNOWN LIMITATION: the protocol only carries text + solid bg/fg colors.
// A live slide with a real background image (see Slide::backgroundImagePath)
// currently reaches Android clients as just its fallback color with no
// image -- image support would need a new frame type and is out of
// scope until the Android app can decode/cache images over the wire.
class SlideServer : public QObject
{
    Q_OBJECT

public:
    static constexpr quint16 kDefaultPort = 55432;

    explicit SlideServer(ScheduleModel *model, QObject *parent = nullptr);

    bool start(quint16 port = kDefaultPort);
    void stop();

    bool isListening() const;
    quint16 port() const;
    int clientCount() const;

signals:
    // Fired whenever a client connects or disconnects, so the UI can
    // show an accurate "N devices connected" indicator.
    void clientCountChanged(int count);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onLiveContentChanged();
    void sendPing();

private:
    void pushToAll(const QByteArray &frame);
    QByteArray currentFrame() const;
    QByteArray buildSlideFrame() const;
    QByteArray buildBlackoutFrame() const;

    ScheduleModel *m_model;
    QTcpServer *m_server;
    QSet<QTcpSocket *> m_clients;
    QTimer *m_pingTimer;
};

#endif // SANCTIFYLIVE_CORE_SLIDESERVER_H_