#ifndef SANCTIFYLIVE_CORE_SLIDESERVER_H_
#define SANCTIFYLIVE_CORE_SLIDESERVER_H_

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QSize>
#include <QTcpServer>
#include <QVector>

class QTcpSocket;
class QTimer;
class ScheduleModel;
struct Slide;

// SlideServer is the desktop side of the Android "stage display" link.
// It listens on a TCP port and pushes newline-delimited JSON frames to
// every connected client whenever the live content changes -- see
// src/Android/PROTOCOL.md for the wire format.
//
// One SlideServer per ScheduleModel is expected (created and owned by
// OperatorWindow). It intentionally has no knowledge of Qt widgets --
// it only touches ScheduleModel, so it could equally serve a future
// second Android client, a web remote, or anything else that speaks
// this protocol.
//
// Two things worth knowing before touching this file:
//
// ROLES. Every client identifies itself right after connecting as
// either a "display" (a sanctuary/stage screen -- always mirrors
// ScheduleModel::liveSlide()) or a "phone" (an operator's handheld,
// which mirrors the same content UNLESS ScheduleModel has a phone
// override set, in which case it gets that instead). See PROTOCOL.md's
// "Roles" section. Until a client's hello line arrives it's treated as
// role Display with unknown resolution, matching the original
// (pre-role) behavior exactly, so an old/misbehaving client is never
// left stuck with no content.
//
// IMAGES. Background images (see Slide::backgroundImagePath) are sent
// to clients as a downsampled, JPEG-encoded, base64 "image" field on
// the slide frame. Decoding a large source photo can take real time,
// so it happens on a worker thread (QtConcurrent) rather than inline --
// the affected role is pushed the frame immediately with no image (the
// solid bg color shows right away), then pushed again with the image
// once encoding finishes. Encoded results are cached by (path,
// last-modified) in m_imageCache, keyed by path rather than a single
// slot, because the display and phone roles can independently be
// showing two different images at once.
class SlideServer : public QObject
{
    Q_OBJECT

public:
    static constexpr quint16 kDefaultPort = 55432;

    enum class Role { Display, Phone };

    // A snapshot of one connected client, for UI that wants to show the
    // operator what's plugged in (see SettingsWindow's "Displays" tab).
    struct ClientInfo
    {
        QString address;      // e.g. "192.168.1.42:51022"
        Role role = Role::Display;
        QSize resolution;     // invalid/empty until the client's hello reports one
    };

    explicit SlideServer(ScheduleModel *model, QObject *parent = nullptr);

    bool start(quint16 port = kDefaultPort);
    void stop();

    bool isListening() const;
    quint16 port() const;
    int clientCount() const;
    QVector<ClientInfo> clients() const;

    // Forces every connected client's screen on and to the front, even
    // over its lock screen. On-demand only (see OperatorWindow's "Wake
    // Display" status bar button) -- the server never sends this itself.
    void wakeAll();

signals:
    // Fired whenever a client connects or disconnects, so the UI can
    // show an accurate "N devices connected" indicator.
    void clientCountChanged(int count);
    // Fired whenever the connected-client list itself is worth
    // redrawing: on connect/disconnect, and whenever a hello updates a
    // client's role or resolution. Carries the same data clients()
    // would return, so listeners don't need to call back in.
    void clientsChanged(const QVector<ClientInfo> &clients);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientReadyRead();
    void onLiveContentChanged();
    void onPhoneContentChanged();
    void sendPing();

private:
    struct ClientState
    {
        Role role = Role::Display;
        QSize resolution;
        QByteArray recvBuffer;   // partial hello bytes until a newline arrives
        bool helloReceived = false;
    };

    struct CachedImage
    {
        QDateTime modified;
        QString base64;
    };

    // Result of the worker-thread encode job. Deliberately a plain,
    // thread-safe-to-copy value (no pointers back into this object) --
    // see encodeImageForPath()'s comment for why.
    struct EncodedImage
    {
        QString path;
        QDateTime modified;
        QString base64;   // empty means the decode/encode failed
    };

    void pushToAll(const QByteArray &frame);
    void pushToRole(Role role, const QByteArray &frame);
    void sendFrameTo(QTcpSocket *socket, const QByteArray &frame);

    QByteArray buildFrameForRole(Role role) const;
    QByteArray buildSlideFrame(const Slide *slide) const;
    QByteArray buildBlackoutFrame() const;
    QByteArray buildWakeFrame() const;

    void ensureImageEncoded(const QString &path);
    void refreshClientsShowingImage(const QString &path);
    static EncodedImage encodeImageForPath(QString path);

    ScheduleModel *m_model;
    QTcpServer *m_server;
    QHash<QTcpSocket *, ClientState> m_clientStates;
    QTimer *m_pingTimer;

    QHash<QString, CachedImage> m_imageCache;     // path -> last successful encode
    QSet<QString> m_pendingImagePaths;            // paths currently being encoded
};

Q_DECLARE_METATYPE(SlideServer::ClientInfo)

#endif // SANCTIFYLIVE_CORE_SLIDESERVER_H_
