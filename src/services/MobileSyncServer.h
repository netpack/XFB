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

#include <QJsonObject>

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

    /**
     * What a paired peer is allowed to ask for.
     *
     * A phone gets the library and playlists it needs to play a set offline.
     * A second XFB standing by as the station's backup needs rather more —
     * the whole catalogue, the jingles, the ads, the programs and the
     * schedule — so that role is granted separately and only from the Station
     * Backup dialog, never from the phone pairing window.
     *
     * A production computer reads the same catalogue as a backup does, and is
     * additionally allowed to *write*: it is the machine where next week's
     * music, jingles and ads are prepared, and what it publishes has to reach
     * the station on air or the work stays on the wrong machine. That is the
     * one role that can change anything here, which is why it too is granted
     * only from its own dialog.
     */
    enum class PeerRole {
        Mobile,      ///< the companion app on a phone
        Station,     ///< another XFB mirroring this one as a backup
        Production,  ///< another XFB used to prepare what this one broadcasts
    };

    struct PairedDevice {
        QString name;
        QString tokenId;    ///< first 8 chars of the token, for display/revoke
        QDateTime pairedAt;
        QDateTime lastSeen;
        PeerRole role = PeerRole::Mobile;
    };

    /**
     * Something that went wrong on air, held for a paired device to collect.
     *
     * This server is pull-only by construction — it cannot open a connection
     * to a phone, and inventing a push service (a cloud relay, a registered
     * FCM sender, an account) would drag half a product into an application
     * that currently needs no account at all. So an incident is *published*
     * here and the companion app picks it up the next time it looks, which
     * for a phone on the studio wifi is a matter of seconds.
     */
    struct Incident {
        QDateTime when;
        QString   kind;      ///< "deadair"
        QString   reason;    ///< "silence" | "stall" | "stopped"
        QString   detail;    ///< the human sentence
        QDateTime resolvedAt;///< invalid while it is still going on
    };

    /** One file the backup station has to end up holding a copy of. */
    struct MirrorFile {
        QString id;
        QString path;
        QString relative;   ///< where to put it under the backup's own root
        qint64  bytes = 0;
        qint64  modified = 0;   ///< seconds since the epoch
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

    /**
     * Opens a pairing window and returns the six-digit code to show.
     *
     * The role is fixed by whoever opened the window, not asked for by the
     * device that turns up: a phone cannot talk its way into a station token.
     */
    QString beginPairing(PeerRole role = PeerRole::Mobile);
    /** The role the open pairing window will grant. */
    PeerRole pairingRole() const { return m_pairingRole; }
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

    /** The word a role travels and is stored as. */
    static QString roleName(PeerRole role);
    /** The role that word means; anything unknown is the least privileged one. */
    static PeerRole roleFromName(const QString &name);

    /** The media categories a station holds, in manifest order. */
    static QStringList mediaCategories();

    /**
     * Whether a relative path that arrived over the network may be used to
     * name a file on this machine: no absolute paths, no drive letters, no
     * climbing out with "..", nothing empty.
     */
    static bool isSafeRelativePath(const QString &relative);

    /**
     * Whether a production computer withdrawing an entry may also delete the
     * audio file it pointed at.
     *
     * Off unless the operator of the station on air turns it on, and it is
     * their switch alone: the machine that would do the deleting does not get
     * to decide. With it off, a withdrawal removes the catalogue entry and
     * leaves the file on disk, which is recoverable; with it on, an ad
     * campaign that ends takes its audio with it and the station's disk does
     * not fill up with material nobody will play again.
     */
    static bool allowsMediaDeletion();
    static void setAllowMediaDeletion(bool allow);

    /**
     * The settings a backup station copies from the station it mirrors.
     *
     * Public because both ends need it: this side to decide what to put in the
     * manifest, and the backup to decide what it is willing to accept out of
     * one. A key that is not on this list is machine-specific — a folder, a
     * port, a credential, a window position — and copying it would point the
     * backup at the very machine it exists to replace.
     */
    static QStringList mirroredSettingKeys();

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

    /**
     * Where to put xfb-companion.apk so this XFB will hand it out.
     *
     * companionApkPath() searches several places, but only this one is both
     * writable and stable across upgrades, so it is the one worth telling the
     * operator about. The directory is not created here.
     */
    QString companionDropDirectory() const;

    // --- incidents and the heartbeat ---------------------------------------

    /**
     * Publishes an incident on GET /api/incidents, where any paired device
     * can see it. Returns nothing and costs nothing when nobody is listening:
     * the point is that the record is already there when somebody looks.
     */
    void postIncident(const QString &kind, const QString &reason,
                      const QString &detail);

    /** Closes the newest open incident of @p kind. */
    void resolveIncident(const QString &kind, const QString &detail);

    QVector<Incident> incidents() const { return m_incidents; }

    /**
     * Where GET /api/station/heartbeat gets its answer.
     *
     * Only the player knows what is on air, and the server has no business
     * reaching into it, so the player installs a provider here — the same
     * shape as setLivePlaylistProvider() above.
     */
    void setStationStateProvider(std::function<QJsonObject()> provider);

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

    /**
     * A production computer has written something into this station's library.
     *
     * The player listens so the tables on screen show what arrived; Auto Mode
     * itself needs no telling, because every pick is a fresh query.
     */
    void catalogueChangedByPeer(const QString &deviceName, const QString &summary);

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
    void handleIncidents(QTcpSocket *socket, const Request &request);
    /** The backup's view of whether this station is still making sound. */
    void handleStationHeartbeat(QTcpSocket *socket);

    // production computers (another XFB that prepares what this one plays)
    void handleProductionHello(QTcpSocket *socket);
    void handleProductionHave(QTcpSocket *socket, const Request &request);
    void handleProductionFile(QTcpSocket *socket, const Request &request);
    void handleProductionRows(QTcpSocket *socket, const Request &request,
                              const QString &device);
    void handleProductionPlaylist(QTcpSocket *socket, const Request &request);
    /** The folder incoming files of a category land in, created if need be. */
    static QString ensureCategoryRoot(const QString &category);
    /** Where @p relative lands under that folder, empty when it is not safe. */
    static QString incomingPath(const QString &category, const QString &relative);

    // station mirroring (a second XFB kept ready to take over)
    void handleStationManifest(QTcpSocket *socket);
    void handleStationFile(QTcpSocket *socket, const Request &request);
    void handleStationPlaylist(QTcpSocket *socket, const Request &request);
    QJsonObject buildStationManifest();
    void rebuildStationIndexIfStale();
    QString stationPathForId(const QString &id);
    static QString categoryRoot(const QString &category);
    static MirrorFile describeFile(const QString &path, const QString &category);

    // responses
    void sendJson(QTcpSocket *socket, const QByteArray &json, int status = 200);
    void sendError(QTcpSocket *socket, int status, const QString &message);
    void sendFile(QTcpSocket *socket, const QString &path, const Request &request);

    // auth
    /** Returns the device name, empty when rejected; fills in its role. */
    QString authenticate(const Request &request, PeerRole *role = nullptr);
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
    PeerRole m_pairingRole = PeerRole::Mobile;
    QTimer *m_pairingTimer = nullptr;
    QDateTime m_pairingExpiry;
    int m_pairingAttempts = 0;

    // token -> device
    QHash<QString, PairedDevice> m_tokens;

    // id -> absolute path, rebuilt lazily
    QHash<QString, QString> m_index;
    QDateTime m_indexBuiltAt;

    /// The same, over everything a backup station may fetch (music, jingles,
    /// ads and programs), kept apart so a phone's allow-list stays narrow.
    QHash<QString, QString> m_stationIndex;
    QDateTime m_stationIndexBuiltAt;

    std::function<QVector<Track>()> m_livePlaylistProvider;
    std::function<QJsonObject()> m_stationStateProvider;
    QString m_playlistsDir;

    /// Recent incidents, newest last, capped: a station that fails all night
    /// must not fill memory with its own bad news.
    QVector<Incident> m_incidents;
    static constexpr int kMaxIncidents = 50;
    /// When this server object was made, for the heartbeat's uptime.
    QDateTime m_startedAt;

    /// Absolute paths marked on the desktop, in the order they were added.
    QStringList m_syncSet;
};

#endif // MOBILESYNCSERVER_H
