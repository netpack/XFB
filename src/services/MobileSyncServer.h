#ifndef MOBILESYNCSERVER_H
#define MOBILESYNCSERVER_H

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

class QTcpServer;
class QTcpSocket;
class QTimer;

/**
 * @brief Serves XFB's library and playlists to the XFB companion app over the
 *        local network, so a phone can take a set on the road and play it
 *        offline.
 *
 * This is deliberately a hand-rolled HTTP/1.1 server on top of QTcpServer
 * rather than QHttpServer: the Debian package is built against Qt 6.4
 * (bookworm), where QHttpServer is still a tech preview and is not packaged.
 * QTcpServer is available on every target XFB ships to.
 *
 * Security model — the server hands out audio files, so it is locked down by
 * construction rather than by validation:
 *
 *  - It is **off by default** and only listens once the operator starts it.
 *  - Tracks are addressed by an opaque id (a hash of the path), never by the
 *    path itself, and ids are resolved against an allow-list built from the
 *    music database and the served playlists. A path that XFB does not already
 *    know about cannot be requested, so there is no traversal surface at all.
 *  - Every endpoint except /api/hello and /api/pair requires a bearer token.
 *  - Tokens are only issued while a pairing window is open (the operator has
 *    the Mobile Sync dialog in front of them showing a six-digit code), and
 *    the window closes on the first success, on timeout, or after too many
 *    wrong codes.
 */
class MobileSyncServer : public QObject
{
    Q_OBJECT

public:
    /** One entry of the library or of a playlist, as sent to the phone. */
    struct Track {
        QString id;         ///< opaque, stable for a given path
        QString path;       ///< absolute path, never sent to the client
        QString artist;
        QString song;
        QString duration;   ///< "h:mm:ss", as stored in musics.time
        qint64  bytes = 0;
        qint64  overlapMs = 0;  ///< crossfade overlap carried from the wave view
        QString volumeEnvelope; ///< "ms:gain;..." volume line, may be empty
    };

    struct PairedDevice {
        QString name;
        QString tokenId;    ///< first 8 chars of the token, for display/revoke
        QDateTime pairedAt;
        QDateTime lastSeen;
    };

    explicit MobileSyncServer(QObject *parent = nullptr);
    ~MobileSyncServer() override;

    /** The port XFB listens on unless the operator overrides it. */
    static quint16 defaultPort() { return 8642; }

    /** Protocol version, bumped when the JSON contract changes. */
    static int protocolVersion() { return 1; }

    bool isListening() const;
    quint16 port() const { return m_port; }

    /** "http://192.168.1.20:8642" for every non-loopback IPv4 interface. */
    QStringList listenAddresses() const;

    /** Bare IPv4 addresses, in the same order, for building a pairing URI. */
    QStringList hostAddresses() const;

    /**
     * The URL encoded in the pairing QR. It is a plain http:// address served
     * by this very server, not a custom scheme: any camera app can open it,
     * whereas an "xfb://" payload is rejected outright by a phone with no app
     * registered for that scheme — which is every phone until the companion
     * app is installed.
     *
     * Empty unless a pairing window is open.
     */
    QString pairingUri(const QString &host) const;

    /**
     * The deep link the companion app itself claims. It is offered from the
     * landing page rather than encoded in the QR, so that scanning does
     * something useful whether or not the app is installed.
     */
    QString pairingDeepLink(const QString &host) const;

    /**
     * The playlist currently loaded in the main window is the one the operator
     * most likely wants on the phone, and only the player can read it, so the
     * player installs a provider here instead of the server reaching into it.
     */
    void setLivePlaylistProvider(std::function<QVector<Track>()> provider);

    /** Directory scanned for saved playlist XMLs. */
    QString playlistsDirectory() const;
    void setPlaylistsDirectory(const QString &dir);

    // --- pairing -----------------------------------------------------------

    /** Opens a pairing window and returns the six-digit code to show. */
    QString beginPairing();
    void endPairing();
    bool pairingOpen() const { return !m_pairingCode.isEmpty(); }
    /** The code currently on offer, empty when no window is open. */
    QString pairingCode() const { return m_pairingCode; }
    /** Seconds left in the pairing window, 0 when closed. */
    int pairingSecondsRemaining() const;

    QVector<PairedDevice> pairedDevices() const;
    void revokeDevice(const QString &tokenId);
    void revokeAllDevices();

    /** The opaque id a given file is addressed by. */
    static QString idForPath(const QString &path);

    // --- tracks marked on the desktop for the phone ----------------------

    /**
     * The server is pull-only — it cannot reach out to a phone — so "sync this
     * to my phone" means marking a track here and letting the phone collect it.
     * The marked set is offered to the phone as a playlist named by
     * syncSetName(), which is why it needs no new client support.
     */
    static QString syncSetName() { return QStringLiteral("__synced__"); }

    /**
     * The companion app's installer, when one is sitting next to XFB.
     *
     * The desktop is the phone's distribution channel: it is already the thing
     * the phone trusts and can reach, and the alternative is asking people to
     * sideload from somewhere else. Empty when no APK is present.
     */
    QString companionApkPath() const;

    /**
     * Version of that APK, read from a sidecar JSON written at build time
     * (xfb-companion.json: {"versionName": "0.2", "versionCode": 2}).
     * Returns 0 when unknown, which means "offer it, but do not call it an
     * update" rather than guessing.
     */
    int companionVersionCode() const;
    QString companionVersionName() const;

    QStringList syncSet() const { return m_syncSet; }
    /** Adds paths, ignoring duplicates. Returns how many were actually new. */
    int addToSyncSet(const QStringList &paths);
    void clearSyncSet();

public slots:
    /** Starts listening. Passing 0 uses the configured/default port. */
    bool start(quint16 port = 0);
    void stop();

signals:
    void started(quint16 port);
    void stopped();
    void errorOccurred(const QString &message);
    void devicePaired(const QString &deviceName);
    void deviceActivity(const QString &deviceName, const QString &description);
    void pairingWindowChanged();
    void syncSetChanged(int trackCount);

private:
    struct Request {
        QByteArray method;
        QString path;
        QHash<QString, QString> query;
        QHash<QByteArray, QByteArray> headers;
        QByteArray body;
    };

    // connection handling
    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    bool parseRequest(const QByteArray &raw, Request *out) const;
    void route(QTcpSocket *socket, const Request &request);

    // endpoints
    void handleHello(QTcpSocket *socket);
    void handlePair(QTcpSocket *socket, const Request &request);
    void handlePairPage(QTcpSocket *socket, const Request &request);
    void handleAppInfo(QTcpSocket *socket);
    void handleAppDownload(QTcpSocket *socket, const Request &request);
    void handleLibrary(QTcpSocket *socket);
    void handlePlaylists(QTcpSocket *socket);
    void handlePlaylist(QTcpSocket *socket, const Request &request);
    void handleTrack(QTcpSocket *socket, const Request &request);

    // responses
    void sendJson(QTcpSocket *socket, const QByteArray &json, int status = 200);
    void sendError(QTcpSocket *socket, int status, const QString &message);
    void sendFile(QTcpSocket *socket, const QString &path, const Request &request);

    // auth
    QString authenticate(const Request &request);   ///< returns device name, empty when rejected
    void loadTokens();
    void saveTokens() const;

    // track index / allow-list
    void rebuildIndexIfStale();
    QVector<Track> libraryTracks();
    QVector<Track> playlistTracks(const QString &name);
    QVector<Track> syncSetTracks();
    QString pathForId(const QString &id);

    void loadSyncSet();
    void saveSyncSet() const;

    QTcpServer *m_server = nullptr;
    quint16 m_port = 0;

    QHash<QTcpSocket *, QByteArray> m_buffers;

    // pairing
    QString m_pairingCode;
    QTimer *m_pairingTimer = nullptr;
    QDateTime m_pairingExpiry;
    int m_pairingAttempts = 0;

    // token -> device
    QHash<QString, PairedDevice> m_tokens;

    // id -> absolute path, rebuilt lazily
    QHash<QString, QString> m_index;
    QDateTime m_indexBuiltAt;

    std::function<QVector<Track>()> m_livePlaylistProvider;
    QString m_playlistsDir;

    /// Absolute paths marked on the desktop, in the order they were added.
    QStringList m_syncSet;
};

#endif // MOBILESYNCSERVER_H
