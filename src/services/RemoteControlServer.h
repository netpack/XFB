#ifndef REMOTECONTROLSERVER_H
#define REMOTECONTROLSERVER_H

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

class QTcpServer;
class QTcpSocket;
class QTimer;

/**
 * @brief Lets another program drive XFB over the network: a Stream Deck on
 *        the presenter's desk, a home-automation panel in the corridor, the
 *        station's own scripts, a web page somebody writes for the newsroom.
 *
 * A small JSON API over HTTP/1.1 — status, the running order, a library
 * search, the transport, Auto Mode, the volume, recording and the stream.
 * Hand-rolled on QTcpServer for the same reason MobileSyncServer is: the
 * Debian package builds against Qt 6.4, where QHttpServer is not packaged.
 *
 * It is a separate server on its own port rather than another role on the
 * phone sync server, because what it grants is of a different kind. A paired
 * phone or backup can *read* the station; a key here can take it off air. So
 * it has its own switch, its own port, its own keys and its own window, and
 * turning the phone sync on can never quietly open this.
 *
 * ### Security, by construction
 *
 *  - **Off by default.** Nothing listens until the operator switches it on in
 *    Options ▸ Remote Control, and the choice is remembered.
 *  - **Every route but /api/v1/hello needs a key**, sent as
 *    `Authorization: Bearer <key>`. There is no key until the operator makes
 *    one, so switching the server on alone grants nothing to anybody.
 *  - **Keys are stored as SHA-256 digests**, never as themselves: a copy of
 *    xfb.conf does not hand over a working key. A key is shown once, when it
 *    is made, and cannot be recovered afterwards — only revoked and replaced.
 *  - **Two scopes.** A *read* key may look (status, running order, library,
 *    events); only a *control* key may change anything. A wall display gets
 *    the first kind and cannot be turned into a way to stop the station.
 *  - **Guessing is expensive.** Keys carry 240 random bits, comparisons do not
 *    short-circuit, and an address that presents ten bad keys is refused
 *    outright for five minutes.
 *  - **Nothing is addressed by path.** A track is added to the running order
 *    by its library reference ("music:123"), resolved against the database;
 *    a path that XFB does not already list cannot be named at all.
 *  - **No CORS.** A web page on another origin cannot read an answer or send
 *    the Authorization header; OPTIONS is refused, so there is no preflight
 *    to pass.
 *  - **Bounded.** Small requests only, a cap on connections and on event
 *    subscribers, and idle connections are closed.
 *
 * What it is not: encrypted. Like the phone sync, this is plain HTTP, meant
 * for the station's own network. The window says so, and offers "this
 * computer only" for anyone who would rather put a TLS proxy or an SSH tunnel
 * in front of it.
 *
 * ### Who does what
 *
 * The server knows HTTP, keys and routes, and nothing about playback. Every
 * command is handed to a single handler the player installs, so the server
 * never reaches into the main window, and the player never parses a request.
 */
class RemoteControlServer : public QObject
{
    Q_OBJECT

public:
    /** What a key is allowed to do. */
    enum class Scope {
        Read,     ///< look, never touch
        Control,  ///< everything the API offers
    };

    /** One key, as the window lists it. The key itself is never kept. */
    struct ApiKey {
        QString    id;       ///< first characters of the key, for display/revoke
        QString    name;     ///< what the operator called it ("Studio Stream Deck")
        QByteArray digest;   ///< SHA-256 of the key, hex
        Scope      scope = Scope::Read;
        QDateTime  created;
        QDateTime  lastUsed;
    };

    /** The answer to one command: an HTTP status and a JSON body. */
    struct Reply {
        int status = 200;
        QJsonObject body;
        /** Set instead of @a body when the answer is a file rather than JSON
         *  — the on-air cover, which the control page shows. */
        QByteArray binary;
        QByteArray contentType;

        static Reply ok(const QJsonObject &body = QJsonObject());
        static Reply error(int status, const QString &message);
        static Reply file(const QByteArray &bytes, const QByteArray &contentType);
    };

    /**
     * Carries out one command.
     *
     * @param command  a dotted name from the route table ("transport.play",
     *                 "playlist.add", "status" ...)
     * @param args     the JSON body of a POST, or the query of a GET, as an
     *                 object; already known to be an object
     */
    using CommandHandler =
        std::function<Reply(const QString &command, const QJsonObject &args)>;

    explicit RemoteControlServer(QObject *parent = nullptr);
    ~RemoteControlServer() override;

    /** The port XFB listens on unless the operator picks another. */
    static quint16 defaultPort() { return 8643; }
    /** Bumped when the JSON contract changes incompatibly. */
    static int apiVersion() { return 1; }

    // --- the operator's settings (RemoteControl/ in xfb.conf) ---------------

    /** Whether XFB starts the server on launch. */
    static bool enabledSetting();
    static void setEnabledSetting(bool enabled);
    static quint16 portSetting();
    static void setPortSetting(quint16 port);
    /** True: listen on 127.0.0.1 only. False: on every interface. */
    static bool localOnlySetting();
    static void setLocalOnlySetting(bool localOnly);
    /**
     * Whether the control page is served at "/" as well as the API.
     *
     * On by default, because a remote control only a programmer can use is
     * not much of a remote control. Off leaves the API exactly as it was: the
     * page's address then answers 404 like any other unknown path.
     */
    static bool serveWebAppSetting();
    static void setServeWebAppSetting(bool serve);

    // --- running --------------------------------------------------------

    bool isListening() const;
    quint16 port() const { return m_port; }
    bool isLocalOnly() const { return m_localOnly; }
    /** "http://192.168.1.20:8643" for each address a client could use. */
    QStringList listenAddresses() const;

    void setCommandHandler(CommandHandler handler);

    // --- keys -----------------------------------------------------------

    QVector<ApiKey> keys() const { return m_keys; }
    /**
     * Makes a key and returns it — the only time it exists in full. The
     * digest is saved at once; the key itself is up to the caller to show.
     */
    QString createKey(const QString &name, Scope scope);
    void revokeKey(const QString &id);

    static QString scopeName(Scope scope);
    /** Anything unknown reads as the lesser scope. */
    static Scope scopeFromName(const QString &name);

    /**
     * The route table, public so the window and the manual can list it and a
     * test can walk it: method, path, command, and the scope it needs.
     */
    struct Route {
        QByteArray method;
        QString    path;
        QString    command;
        Scope      scope;
    };
    static const QVector<Route> &routes();

public slots:
    /** Listens with the saved port and address. False after errorOccurred(). */
    bool start();
    void stop();

    /**
     * Tells event subscribers something changed now, rather than at the next
     * poll. The player calls this when the operator acts at the desk, too.
     */
    void notifyStateChanged();

signals:
    void started(quint16 port);
    void stopped();
    void errorOccurred(const QString &message);
    /** A key did something that changes the station. For the activity list. */
    void commandExecuted(const QString &keyName, const QString &command, int status);
    /** An address was locked out for presenting bad keys. */
    void addressBlocked(const QString &address);

private:
    struct Request {
        QByteArray method;
        QString path;
        QHash<QString, QString> query;
        QHash<QByteArray, QByteArray> headers;
        QByteArray body;
    };

    struct Connection {
        QByteArray buffer;
        qint64     lastActivityMs = 0;
        bool       streaming = false;  ///< an open /events subscription
    };

    struct Failures {
        int       count = 0;
        QDateTime windowStart;
        QDateTime blockedUntil;
    };

    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    /** -1 malformed, 0 need more bytes, 1 complete. Fills @p error on -1. */
    int parseRequest(const QByteArray &raw, Request *out, int *errorStatus) const;
    void route(QTcpSocket *socket, const Request &request);

    void handleHello(QTcpSocket *socket);
    /** The control page itself, at "/" — no key, because this is where one
     *  is typed in. It reveals no more than /api/v1/hello does. */
    void handleWebApp(QTcpSocket *socket);
    /** Who this key is and what it may do, so the page can hide what it
     *  cannot use rather than offering buttons that answer 403. */
    void handleWhoAmI(QTcpSocket *socket, const ApiKey &key);
    void handleEvents(QTcpSocket *socket);
    void pushEvents();

    /** The key presented, or nullptr. Constant-time over every stored key. */
    ApiKey *authenticate(const Request &request);
    bool isBlocked(const QString &address) const;
    void noteFailure(const QString &address);

    void sendJson(QTcpSocket *socket, int status, const QJsonObject &body);
    void sendBinary(QTcpSocket *socket, const QByteArray &bytes, const QByteArray &contentType);
    /** The control page, under a content policy that permits its own inline
     *  style and script, its own origin, and nothing else anywhere. */
    void sendHtml(QTcpSocket *socket, const QString &page);
    void sendError(QTcpSocket *socket, int status, const QString &message);
    void closeIdleConnections();

    void loadKeys();
    void saveKeys() const;

    static QString peerAddress(QTcpSocket *socket);
    static QByteArray digestFor(const QString &key);

    QTcpServer *m_server = nullptr;
    quint16 m_port = 0;
    bool m_localOnly = false;
    CommandHandler m_handler;

    QHash<QTcpSocket *, Connection> m_connections;
    QHash<QString, Failures> m_failures;
    QVector<ApiKey> m_keys;

    QTimer *m_housekeeping = nullptr;   ///< idle sweep + persisting lastUsed
    QTimer *m_eventTimer = nullptr;     ///< runs only while somebody subscribes
    QByteArray m_lastEvent;
    int m_ticksSinceEvent = 0;
    bool m_keysDirty = false;
};

#endif // REMOTECONTROLSERVER_H
