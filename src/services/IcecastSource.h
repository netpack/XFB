#ifndef ICECASTSOURCE_H
#define ICECASTSOURCE_H

#include <QByteArray>
#include <QObject>
#include <QString>

class QTcpSocket;
class QTimer;

/**
 * @brief An Icecast 2 source client, spoken directly over QTcpSocket.
 *
 * Hand-rolled for the same reason MobileSyncServer is: the Debian package is
 * built against Qt 6.4 (bookworm), which has no usable QHttpServer, and the
 * source protocol is barely HTTP anyway — a request line, a block of headers,
 * then an audio body that never ends.
 *
 * Two dialects are supported, because which one a server accepts depends on
 * its vintage:
 *
 *  - `SOURCE /mount ICE/1.0` — the classic form, understood by every Icecast
 *    2.x and by most of the Shoutcast-compatible servers people actually run.
 *  - `PUT /mount HTTP/1.1` with `Expect: 100-continue` — the form Icecast
 *    2.4.1 and later prefer, and the only one some hardened proxies pass.
 *
 * Everything about being on air unattended is in here: the connection is
 * re-established with an exponential backoff, audio produced while the
 * handshake is in flight is buffered (briefly, and with a cap), audio that
 * cannot be pushed to a wedged server is dropped rather than queued, and
 * every drop says *why* in words an operator can act on — "wrong password"
 * and "the server went away" need very different responses at 3 a.m.
 */
class IcecastSource : public QObject
{
    Q_OBJECT

public:
    struct Config {
        QString host = QStringLiteral("localhost");
        quint16 port = 8000;
        QString mount = QStringLiteral("/stream");
        QString user = QStringLiteral("source");
        QString password;
        /** Use `PUT` instead of `SOURCE` (Icecast 2.4.1+). */
        bool usePut = false;

        // Announced to the server, and from there to the listener and to
        // any directory the station is listed in.
        QString contentType = QStringLiteral("audio/mpeg");
        QString name;
        QString genre;
        QString description;
        QString url;
        bool isPublic = false;

        // Only used to build the ice-audio-info header.
        int bitrateKbps = 128;
        int sampleRate = 48000;
        int channels = 2;
    };

    enum class State {
        Disconnected,
        Connecting,   ///< socket opening, or handshake in flight
        Connected,    ///< the server accepted the source; audio is flowing
    };

    explicit IcecastSource(QObject *parent = nullptr);
    ~IcecastSource() override;

    void setConfig(const Config &config);
    Config config() const { return m_config; }

    State state() const { return m_state; }
    bool isConnected() const { return m_state == State::Connected; }
    /** Bytes handed to the socket since the current connection came up. */
    qint64 bytesSent() const { return m_bytesSent; }
    /** Why the last connection ended; empty while it is up. */
    QString lastError() const { return m_lastError; }

public slots:
    /** Open the connection and keep it open until disconnectFromServer(). */
    void connectToServer();
    void disconnectFromServer();
    /** A chunk of encoded stream data for the body. */
    void writeAudio(const QByteArray &data);
    /**
     * Push the on-air track to the server's metadata endpoint.
     *
     * Icecast carries titles out of band rather than in the audio, so this
     * is a separate one-shot request. The value is remembered and pushed
     * again after a reconnect, so a listener who tunes in later still sees
     * what is playing.
     */
    void setMetadata(const QString &song);

signals:
    void stateChanged(State state);
    void connected();
    /** Always carries a reason; that is the whole point of it. */
    void disconnected(const QString &reason);
    void logMessage(const QString &message);

private:
    void openSocket();
    void sendRequest();
    void readHandshake();
    void handleHandshakeResponse();
    void fail(const QString &reason, bool retry);
    void setState(State state);
    void scheduleReconnect();
    void pushMetadata();
    QByteArray authorizationHeader() const;
    QString normalisedMount() const;

    /** Audio kept while the handshake is in flight (~1 s at 128 kbit/s). */
    static constexpr qint64 kMaxHandshakeBuffer = 64 * 1024;
    /** Unwritten bytes on a live socket past which audio is dropped. */
    static constexpr qint64 kMaxPendingBytes = 512 * 1024;
    static constexpr int kHandshakeTimeoutMs = 15000;
    static constexpr int kFirstReconnectDelayMs = 2000;
    static constexpr int kMaxReconnectDelayMs = 60000;

    Config m_config;
    QTcpSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_handshakeTimer = nullptr;

    State m_state = State::Disconnected;
    bool m_wantConnected = false;
    bool m_handshakeDone = false;
    QByteArray m_responseBuffer;
    QByteArray m_pending;        // audio produced during the handshake
    qint64 m_bytesSent = 0;
    qint64 m_droppedBytes = 0;
    bool m_dropping = false;
    int m_reconnectDelayMs = kFirstReconnectDelayMs;
    QString m_lastError;
    QString m_song;              // last metadata pushed, replayed on reconnect
};

#endif // ICECASTSOURCE_H
