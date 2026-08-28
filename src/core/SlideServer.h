#ifndef SANCTIFYLIVE_CORE_SLIDESERVER_H_
#define SANCTIFYLIVE_CORE_SLIDESERVER_H_

#include <QDateTime>
#include <QHash>
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
// Background images (see Slide::backgroundImagePath) are sent to clients
// as a downsampled, JPEG-encoded, base64 "image" field on the slide frame
// -- see PROTOCOL.md. Encoded images are cached by (path, last-modified)
// so re-broadcasting the same live slide (e.g. a ping-triggered resend,
// or a second client connecting) never re-decodes/re-encodes the file.
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

    // Forces every connected client's screen on and to the front, even
    // over its lock screen. On-demand only (see OperatorWindow's "Wake
    // Display" status bar button) -- the server never sends this itself.
    void wakeAll();

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
    QByteArray buildWakeFrame() const;
    QString encodedImageFor(const QString &path) const;

    ScheduleModel *m_model;
    QTcpServer *m_server;
    QSet<QTcpSocket *> m_clients;
    QTimer *m_pingTimer;

    // One-entry cache of the last background image we base64-encoded,
    // keyed by file path + last-modified time so an edited-and-reimported
    // file (same path, new content) is correctly re-encoded. mutable
    // because encoding happens lazily from const frame-building methods.
    mutable QString m_cachedImagePath;
    mutable QDateTime m_cachedImageModified;
    mutable QString m_cachedImageBase64;
};

#endif // SANCTIFYLIVE_CORE_SLIDESERVER_H_