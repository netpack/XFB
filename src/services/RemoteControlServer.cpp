#include "RemoteControlServer.h"

#include "RemoteControlPage.h"
#include "RequestLine.h"

#include <QCryptographicHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkInterface>
#include <QPointer>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QSysInfo>
#include <QUrl>

#include <utility>

namespace {

/** Headers of one request. Nothing legitimate comes close. */
constexpr int kMaxHeaderBytes = 16 * 1024;
/** Body of one request: a command's arguments, never a file. */
constexpr int kMaxBodyBytes = 64 * 1024;
/** Open connections at once, of every kind. */
constexpr int kMaxConnections = 64;
/** Of those, how many may be event subscriptions. */
constexpr int kMaxSubscribers = 16;
/** A connection that has said nothing for this long is closed. */
constexpr qint64 kIdleTimeoutMs = 15000;

/** Bad keys one address may present inside the window below... */
constexpr int kMaxFailures = 10;
constexpr int kFailureWindowSeconds = 300;
/** ...before it is refused outright for this long. */
constexpr int kBlockSeconds = 300;
/** Addresses remembered before the stale ones are forgotten. */
constexpr int kFailureTableCap = 4096;

/** Random bytes behind a key: 240 bits, 40 characters of base64url. */
constexpr int kKeyBytes = 30;
const QString kKeyPrefix = QStringLiteral("xfb_");

/** Seconds between event polls, and polls between keep-alive comments. */
constexpr int kEventIntervalMs = 1000;
constexpr int kKeepAliveTicks = 15;

const QString kGroup = QStringLiteral("RemoteControl");

QString xfbConfigFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

QByteArray reasonPhrase(int status)
{
    switch (status) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 409: return "Conflict";
    case 413: return "Payload Too Large";
    case 415: return "Unsupported Media Type";
    case 422: return "Unprocessable Entity";
    case 429: return "Too Many Requests";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 503: return "Service Unavailable";
    default:  return "Status";
    }
}

/** Compares two digests without stopping at the first difference. */
bool sameDigest(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size())
        return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a.at(i) ^ b.at(i));
    return diff == 0;
}

} // namespace

// ------------------------------------------------------------------ replies

RemoteControlServer::Reply RemoteControlServer::Reply::ok(const QJsonObject &body)
{
    Reply reply;
    reply.status = 200;
    reply.body = body;
    if (!reply.body.contains(QStringLiteral("ok")))
        reply.body.insert(QStringLiteral("ok"), true);
    return reply;
}

RemoteControlServer::Reply RemoteControlServer::Reply::error(int status, const QString &message)
{
    Reply reply;
    reply.status = status;
    reply.body.insert(QStringLiteral("ok"), false);
    reply.body.insert(QStringLiteral("error"), message);
    return reply;
}

RemoteControlServer::Reply RemoteControlServer::Reply::file(const QByteArray &bytes,
                                                            const QByteArray &contentType)
{
    Reply reply;
    reply.status = 200;
    reply.binary = bytes;
    reply.contentType = contentType;
    return reply;
}

// ------------------------------------------------------------------- routes

const QVector<RemoteControlServer::Route> &RemoteControlServer::routes()
{
    static const QVector<Route> table = {
        {"GET",  QStringLiteral("/api/v1/status"),              QStringLiteral("status"),            Scope::Read},
        {"GET",  QStringLiteral("/api/v1/playlist"),            QStringLiteral("playlist"),          Scope::Read},
        {"GET",  QStringLiteral("/api/v1/library"),             QStringLiteral("library.search"),    Scope::Read},
        {"GET",  QStringLiteral("/api/v1/events"),              QStringLiteral("events"),            Scope::Read},
        {"GET",  QStringLiteral("/api/v1/whoami"),              QStringLiteral("whoami"),            Scope::Read},
        {"GET",  QStringLiteral("/api/v1/artwork"),             QStringLiteral("artwork"),           Scope::Read},

        {"POST", QStringLiteral("/api/v1/transport/play"),      QStringLiteral("transport.play"),     Scope::Control},
        {"POST", QStringLiteral("/api/v1/transport/pause"),     QStringLiteral("transport.pause"),    Scope::Control},
        {"POST", QStringLiteral("/api/v1/transport/resume"),    QStringLiteral("transport.resume"),   Scope::Control},
        {"POST", QStringLiteral("/api/v1/transport/stop"),      QStringLiteral("transport.stop"),     Scope::Control},
        {"POST", QStringLiteral("/api/v1/transport/stop-after"),QStringLiteral("transport.stopAfter"),Scope::Control},
        {"POST", QStringLiteral("/api/v1/transport/next"),      QStringLiteral("transport.next"),     Scope::Control},
        {"POST", QStringLiteral("/api/v1/transport/seek"),      QStringLiteral("transport.seek"),     Scope::Control},

        {"POST", QStringLiteral("/api/v1/volume"),              QStringLiteral("volume.set"),         Scope::Control},
        {"POST", QStringLiteral("/api/v1/automode"),            QStringLiteral("automode.set"),       Scope::Control},

        {"POST", QStringLiteral("/api/v1/playlist/add"),        QStringLiteral("playlist.add"),       Scope::Control},
        {"POST", QStringLiteral("/api/v1/playlist/remove"),     QStringLiteral("playlist.remove"),    Scope::Control},
        {"POST", QStringLiteral("/api/v1/playlist/move"),       QStringLiteral("playlist.move"),      Scope::Control},
        {"POST", QStringLiteral("/api/v1/playlist/clear"),      QStringLiteral("playlist.clear"),     Scope::Control},

        {"POST", QStringLiteral("/api/v1/recording/start"),     QStringLiteral("recording.start"),    Scope::Control},
        {"POST", QStringLiteral("/api/v1/recording/stop"),      QStringLiteral("recording.stop"),     Scope::Control},
        {"POST", QStringLiteral("/api/v1/stream/start"),        QStringLiteral("stream.start"),       Scope::Control},
        {"POST", QStringLiteral("/api/v1/stream/stop"),         QStringLiteral("stream.stop"),        Scope::Control},
    };
    return table;
}

QString RemoteControlServer::scopeName(Scope scope)
{
    return scope == Scope::Control ? QStringLiteral("control") : QStringLiteral("read");
}

RemoteControlServer::Scope RemoteControlServer::scopeFromName(const QString &name)
{
    return name == QLatin1String("control") ? Scope::Control : Scope::Read;
}

// ----------------------------------------------------------------- settings

bool RemoteControlServer::enabledSetting()
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    return settings.value(kGroup + QStringLiteral("/Enabled"), false).toBool();
}

void RemoteControlServer::setEnabledSetting(bool enabled)
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    settings.setValue(kGroup + QStringLiteral("/Enabled"), enabled);
}

quint16 RemoteControlServer::portSetting()
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    const uint port = settings.value(kGroup + QStringLiteral("/Port"), defaultPort()).toUInt();
    return (port == 0 || port > 65535) ? defaultPort() : quint16(port);
}

void RemoteControlServer::setPortSetting(quint16 port)
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    settings.setValue(kGroup + QStringLiteral("/Port"), port);
}

bool RemoteControlServer::localOnlySetting()
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    return settings.value(kGroup + QStringLiteral("/LocalOnly"), false).toBool();
}

void RemoteControlServer::setLocalOnlySetting(bool localOnly)
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    settings.setValue(kGroup + QStringLiteral("/LocalOnly"), localOnly);
}

bool RemoteControlServer::serveWebAppSetting()
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    return settings.value(kGroup + QStringLiteral("/WebApp"), true).toBool();
}

void RemoteControlServer::setServeWebAppSetting(bool serve)
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    settings.setValue(kGroup + QStringLiteral("/WebApp"), serve);
}

// ---------------------------------------------------------------- lifecycle

RemoteControlServer::RemoteControlServer(QObject *parent)
    : QObject(parent)
{
    loadKeys();

    m_housekeeping = new QTimer(this);
    m_housekeeping->setInterval(5000);
    connect(m_housekeeping, &QTimer::timeout, this, [this]() {
        closeIdleConnections();
        // lastUsed changes on every request; the file is written at most
        // once per sweep rather than once per poll of a wall display.
        if (m_keysDirty)
            saveKeys();
    });

    m_eventTimer = new QTimer(this);
    m_eventTimer->setInterval(kEventIntervalMs);
    connect(m_eventTimer, &QTimer::timeout, this, &RemoteControlServer::pushEvents);
}

RemoteControlServer::~RemoteControlServer()
{
    stop();
}

bool RemoteControlServer::isListening() const
{
    return m_server && m_server->isListening();
}

bool RemoteControlServer::start()
{
    if (isListening())
        return true;

    const quint16 port = portSetting();
    m_localOnly = localOnlySetting();

    if (!m_server) {
        m_server = new QTcpServer(this);
        m_server->setMaxPendingConnections(kMaxConnections);
        connect(m_server, &QTcpServer::newConnection,
                this, &RemoteControlServer::onNewConnection);
    }

    const QHostAddress address = m_localOnly ? QHostAddress(QHostAddress::LocalHost)
                                             : QHostAddress(QHostAddress::Any);
    if (!m_server->listen(address, port)) {
        emit errorOccurred(tr("Could not listen on port %1: %2")
                               .arg(port).arg(m_server->errorString()));
        return false;
    }

    m_port = m_server->serverPort();
    m_housekeeping->start();
    emit started(m_port);
    return true;
}

void RemoteControlServer::stop()
{
    m_eventTimer->stop();
    m_housekeeping->stop();
    if (m_keysDirty)
        saveKeys();

    if (m_server && m_server->isListening()) {
        m_server->close();
        const auto sockets = m_connections.keys();
        m_connections.clear();
        for (QTcpSocket *socket : sockets)
            socket->abort();
        m_port = 0;
        emit stopped();
    }
}

QStringList RemoteControlServer::listenAddresses() const
{
    QStringList addresses;
    if (!isListening())
        return addresses;

    if (m_localOnly) {
        addresses << QStringLiteral("http://127.0.0.1:%1").arg(m_port);
        return addresses;
    }

    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp)
            || iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                addresses << QStringLiteral("http://%1:%2").arg(entry.ip().toString()).arg(m_port);
        }
    }
    return addresses;
}

void RemoteControlServer::setCommandHandler(CommandHandler handler)
{
    m_handler = std::move(handler);
}

// --------------------------------------------------------------------- keys

QByteArray RemoteControlServer::digestFor(const QString &key)
{
    return QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex();
}

QString RemoteControlServer::createKey(const QString &name, Scope scope)
{
    QByteArray random(kKeyBytes, Qt::Uninitialized);
    QRandomGenerator::system()->generate(random.begin(), random.end());
    const QString key = kKeyPrefix + QString::fromLatin1(
        random.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));

    ApiKey entry;
    entry.id = key.mid(kKeyPrefix.size(), 8);
    entry.name = name.trimmed().isEmpty() ? tr("Unnamed key") : name.trimmed();
    entry.digest = digestFor(key);
    entry.scope = scope;
    entry.created = QDateTime::currentDateTime();
    m_keys.append(entry);
    saveKeys();
    return key;
}

void RemoteControlServer::revokeKey(const QString &id)
{
    for (int i = m_keys.size() - 1; i >= 0; --i) {
        if (m_keys.at(i).id == id)
            m_keys.removeAt(i);
    }
    saveKeys();
}

void RemoteControlServer::loadKeys()
{
    m_keys.clear();
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    const int count = settings.beginReadArray(kGroup + QStringLiteral("/Keys"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        ApiKey key;
        key.id = settings.value(QStringLiteral("id")).toString();
        key.name = settings.value(QStringLiteral("name")).toString();
        key.digest = settings.value(QStringLiteral("digest")).toByteArray();
        key.scope = scopeFromName(settings.value(QStringLiteral("scope")).toString());
        key.created = settings.value(QStringLiteral("created")).toDateTime();
        key.lastUsed = settings.value(QStringLiteral("lastUsed")).toDateTime();
        // A digest of the wrong length could never match; drop it rather
        // than keep a row in the window that looks like a working key.
        if (key.digest.size() == 64)
            m_keys.append(key);
    }
    settings.endArray();
}

void RemoteControlServer::saveKeys() const
{
    QSettings settings(xfbConfigFile(), QSettings::IniFormat);
    settings.remove(kGroup + QStringLiteral("/Keys"));
    settings.beginWriteArray(kGroup + QStringLiteral("/Keys"), m_keys.size());
    for (int i = 0; i < m_keys.size(); ++i) {
        const ApiKey &key = m_keys.at(i);
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("id"), key.id);
        settings.setValue(QStringLiteral("name"), key.name);
        settings.setValue(QStringLiteral("digest"), key.digest);
        settings.setValue(QStringLiteral("scope"), scopeName(key.scope));
        settings.setValue(QStringLiteral("created"), key.created);
        if (key.lastUsed.isValid())
            settings.setValue(QStringLiteral("lastUsed"), key.lastUsed);
    }
    settings.endArray();
    const_cast<RemoteControlServer *>(this)->m_keysDirty = false;
}

RemoteControlServer::ApiKey *RemoteControlServer::authenticate(const Request &request)
{
    const QByteArray header = request.headers.value("authorization");
    if (!header.startsWith("Bearer "))
        return nullptr;

    const QByteArray presented = digestFor(QString::fromUtf8(header.mid(7).trimmed()));

    // Every stored digest is compared, match or not, so the time taken says
    // nothing about which key came close.
    ApiKey *found = nullptr;
    for (ApiKey &key : m_keys) {
        if (sameDigest(presented, key.digest) && !found)
            found = &key;
    }
    return found;
}

// ------------------------------------------------------------ rate limiting

QString RemoteControlServer::peerAddress(QTcpSocket *socket)
{
    QHostAddress address = socket->peerAddress();
    // An IPv4 client on a dual-stack socket shows up as ::ffff:a.b.c.d;
    // keying on both spellings would halve the limit's bite.
    bool isV4 = false;
    const quint32 v4 = address.toIPv4Address(&isV4);
    if (isV4)
        address = QHostAddress(v4);
    return address.toString();
}

bool RemoteControlServer::isBlocked(const QString &address) const
{
    const auto it = m_failures.constFind(address);
    return it != m_failures.constEnd() && it->blockedUntil.isValid()
           && it->blockedUntil > QDateTime::currentDateTimeUtc();
}

void RemoteControlServer::noteFailure(const QString &address)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();

    if (m_failures.size() >= kFailureTableCap && !m_failures.contains(address)) {
        for (auto it = m_failures.begin(); it != m_failures.end();) {
            const bool stale = it->windowStart.secsTo(now) > kFailureWindowSeconds
                               && (!it->blockedUntil.isValid() || it->blockedUntil < now);
            it = stale ? m_failures.erase(it) : std::next(it);
        }
        if (m_failures.size() >= kFailureTableCap)
            return; // still full of live entries: the per-address cap already bites
    }

    Failures &entry = m_failures[address];
    if (!entry.windowStart.isValid() || entry.windowStart.secsTo(now) > kFailureWindowSeconds) {
        entry.windowStart = now;
        entry.count = 0;
    }
    if (++entry.count >= kMaxFailures) {
        entry.blockedUntil = now.addSecs(kBlockSeconds);
        entry.count = 0;
        entry.windowStart = now;
        emit addressBlocked(address);
    }
}

// -------------------------------------------------------------- connections

void RemoteControlServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (m_connections.size() >= kMaxConnections) {
            socket->abort();
            socket->deleteLater();
            continue;
        }

        Connection connection;
        connection.lastActivityMs = QDateTime::currentMSecsSinceEpoch();
        m_connections.insert(socket, connection);

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            onReadyRead(socket);
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_connections.remove(socket);
            socket->deleteLater();
            if (m_eventTimer->isActive()) {
                bool anyStreaming = false;
                for (const Connection &c : std::as_const(m_connections))
                    anyStreaming = anyStreaming || c.streaming;
                if (!anyStreaming)
                    m_eventTimer->stop();
            }
        });
    }
}

void RemoteControlServer::closeIdleConnections()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QTcpSocket *> idle;
    for (auto it = m_connections.cbegin(); it != m_connections.cend(); ++it) {
        if (!it->streaming && now - it->lastActivityMs > kIdleTimeoutMs)
            idle << it.key();
    }
    for (QTcpSocket *socket : idle)
        socket->abort();
}

void RemoteControlServer::onReadyRead(QTcpSocket *socket)
{
    auto it = m_connections.find(socket);
    if (it == m_connections.end())
        return;

    if (it->streaming) {
        // A subscriber has nothing more to say; whatever it sends is dropped
        // so it cannot grow a buffer here.
        socket->readAll();
        return;
    }

    it->buffer.append(socket->readAll());
    it->lastActivityMs = QDateTime::currentMSecsSinceEpoch();

    Request request;
    int errorStatus = 400;
    const int parsed = parseRequest(it->buffer, &request, &errorStatus);
    if (parsed == 0)
        return;

    // One request per connection: stop reading and answer.
    it->buffer.clear();
    if (parsed < 0) {
        const QString message = errorStatus == 413 ? tr("Request too large.")
                              : errorStatus == 501 ? tr("Chunked requests are not supported.")
                                                   : tr("Malformed request.");
        sendError(socket, errorStatus, message);
        return;
    }
    route(socket, request);
}

int RemoteControlServer::parseRequest(const QByteArray &raw, Request *out, int *errorStatus) const
{
    const int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        if (raw.size() > kMaxHeaderBytes) {
            *errorStatus = 413;
            return -1;
        }
        return 0;
    }
    if (headerEnd > kMaxHeaderBytes) {
        *errorStatus = 413;
        return -1;
    }

    const QList<QByteArray> lines = raw.left(headerEnd).split('\n');
    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() != 3 || !requestLine.at(2).startsWith("HTTP/1.")) {
        *errorStatus = 400;
        return -1;
    }

    out->method = requestLine.at(0);
    const QUrl url = QUrl::fromEncoded(requestLine.at(1), QUrl::StrictMode);
    if (!url.isValid() || !requestLine.at(1).startsWith('/')) {
        *errorStatus = 400;
        return -1;
    }
    out->path = url.path();
    // A trailing slash is the same route; anything else is not.
    while (out->path.size() > 1 && out->path.endsWith(QLatin1Char('/')))
        out->path.chop(1);

    const QStringList pairs = url.query(QUrl::FullyEncoded).split(QLatin1Char('&'), Qt::SkipEmptyParts);
    for (const QString &pair : pairs) {
        const int equals = pair.indexOf(QLatin1Char('='));
        const QString name = equals < 0 ? pair : pair.left(equals);
        const QString value = equals < 0 ? QString() : pair.mid(equals + 1);
        QByteArray decodedValue = value.toUtf8();
        decodedValue.replace('+', ' ');
        out->query.insert(QUrl::fromPercentEncoding(name.toUtf8()),
                          QUrl::fromPercentEncoding(decodedValue));
    }

    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const int colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        out->headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon + 1).trimmed());
    }

    if (out->headers.contains("transfer-encoding")) {
        *errorStatus = 501;
        return -1;
    }

    bool lengthOk = true;
    const QByteArray lengthHeader = out->headers.value("content-length", "0");
    const qint64 contentLength = lengthHeader.toLongLong(&lengthOk);
    if (!lengthOk || contentLength < 0) {
        *errorStatus = 400;
        return -1;
    }
    if (contentLength > kMaxBodyBytes) {
        *errorStatus = 413;
        return -1;
    }
    if (raw.size() < headerEnd + 4 + contentLength)
        return 0;

    out->body = raw.mid(headerEnd + 4, int(contentLength));
    return 1;
}

// -------------------------------------------------------------------- route

void RemoteControlServer::route(QTcpSocket *socket, const Request &request)
{
    const QString address = peerAddress(socket);
    if (isBlocked(address)) {
        sendError(socket, 429, tr("Too many bad keys from this address. Try again later."));
        return;
    }

    // The control page. Served before a key is looked at, because this is the
    // page on which one is typed; it carries no station data of its own.
    if (request.path == QLatin1String("/") || request.path == QLatin1String("/app")) {
        if (!serveWebAppSetting()) {
            sendError(socket, 404, tr("No such endpoint."));
            return;
        }
        if (request.method != "GET") {
            sendError(socket, 405, tr("Use GET."));
            return;
        }
        handleWebApp(socket);
        return;
    }

    if (request.path == QLatin1String("/api/v1/hello") || request.path == QLatin1String("/api/v1")) {
        if (request.method != "GET") {
            sendError(socket, 405, tr("Use GET."));
            return;
        }
        handleHello(socket);
        return;
    }

    const Route *match = nullptr;
    bool pathKnown = false;
    for (const Route &candidate : routes()) {
        if (candidate.path != request.path)
            continue;
        pathKnown = true;
        if (candidate.method == request.method) {
            match = &candidate;
            break;
        }
    }
    if (!pathKnown) {
        sendError(socket, 404, tr("No such endpoint."));
        return;
    }

    // Authenticate before saying anything about methods, so an unauthenticated
    // caller learns nothing beyond "this path exists", which the manual says.
    ApiKey *key = authenticate(request);
    if (!key) {
        if (request.headers.contains("authorization"))
            noteFailure(address);
        sendError(socket, 401, tr("A valid API key is required."));
        return;
    }

    if (!match) {
        sendError(socket, 405, tr("This endpoint does not accept %1.")
                                   .arg(QString::fromLatin1(request.method)));
        return;
    }
    if (match->scope == Scope::Control && key->scope != Scope::Control) {
        sendError(socket, 403, tr("This key may only read. Make a control key to change anything."));
        return;
    }

    key->lastUsed = QDateTime::currentDateTime();
    m_keysDirty = true;
    const QString keyName = key->name;  // the pointer may not survive the handler

    if (match->command == QLatin1String("events")) {
        handleEvents(socket);
        return;
    }
    if (match->command == QLatin1String("whoami")) {
        handleWhoAmI(socket, *key);
        return;
    }

    QJsonObject args;
    if (request.method == "GET") {
        for (auto it = request.query.cbegin(); it != request.query.cend(); ++it)
            args.insert(it.key(), it.value());
    } else if (!request.body.trimmed().isEmpty()) {
        const QByteArray type = request.headers.value("content-type").toLower();
        if (!type.startsWith("application/json")) {
            sendError(socket, 415, tr("Send the arguments as application/json."));
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(request.body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            sendError(socket, 400, tr("The body must be a JSON object."));
            return;
        }
        args = document.object();
    }

    if (!m_handler) {
        sendError(socket, 503, tr("XFB is not ready to take commands yet."));
        return;
    }

    // The handler runs player code, which can open a message box and with it
    // a nested event loop — in which this very client may hang up.
    QPointer<QTcpSocket> guard(socket);
    const Reply reply = m_handler(match->command, args);
    if (match->scope == Scope::Control) {
        emit commandExecuted(keyName, match->command, reply.status);
        notifyStateChanged();
    }
    if (!guard)
        return;
    if (!reply.binary.isEmpty() && reply.status == 200)
        sendBinary(guard, reply.binary, reply.contentType);
    else
        sendJson(guard, reply.status, reply.body);
}

void RemoteControlServer::handleHello(QTcpSocket *socket)
{
    // Enough for a client to know it found XFB and which contract it speaks,
    // and nothing about the station itself: this is the one open route.
    QJsonObject body;
    body.insert(QStringLiteral("ok"), true);
    body.insert(QStringLiteral("app"), QStringLiteral("XFB"));
    body.insert(QStringLiteral("api"), apiVersion());
    body.insert(QStringLiteral("auth"), QStringLiteral("bearer"));
    sendJson(socket, 200, body);
}

void RemoteControlServer::handleWebApp(QTcpSocket *socket)
{
    sendHtml(socket, RemoteControlPage::html());
}

void RemoteControlServer::handleWhoAmI(QTcpSocket *socket, const ApiKey &key)
{
    QString station = RequestLine::stationName();
    if (station.isEmpty())
        station = QSysInfo::machineHostName();

    QJsonObject body;
    body.insert(QStringLiteral("ok"), true);
    body.insert(QStringLiteral("name"), key.name);
    body.insert(QStringLiteral("scope"), scopeName(key.scope));
    body.insert(QStringLiteral("station"), station);
    body.insert(QStringLiteral("api"), apiVersion());
    sendJson(socket, 200, body);
}

// ------------------------------------------------------------------- events

void RemoteControlServer::handleEvents(QTcpSocket *socket)
{
    int subscribers = 0;
    for (const Connection &c : std::as_const(m_connections))
        subscribers += c.streaming ? 1 : 0;
    if (subscribers >= kMaxSubscribers) {
        sendError(socket, 503, tr("Too many event subscribers."));
        return;
    }

    auto it = m_connections.find(socket);
    if (it == m_connections.end())
        return;
    it->streaming = true;

    socket->write("HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/event-stream; charset=utf-8\r\n"
                  "Cache-Control: no-store\r\n"
                  "X-Content-Type-Options: nosniff\r\n"
                  "Connection: keep-alive\r\n"
                  "\r\n"
                  "retry: 3000\n\n");

    // The newcomer gets the state straight away rather than waiting for the
    // next change, which for a stopped station could be hours.
    if (m_handler) {
        const Reply reply = m_handler(QStringLiteral("status"), QJsonObject());
        const QByteArray json = QJsonDocument(reply.body).toJson(QJsonDocument::Compact);
        socket->write("event: status\ndata: " + json + "\n\n");
        // Everybody already subscribed has either had this state or is about
        // to be sent it; the next poll should not send it to the newcomer twice.
        if (!m_eventTimer->isActive())
            m_lastEvent = json;
    }

    if (!m_eventTimer->isActive())
        m_eventTimer->start();
}

void RemoteControlServer::notifyStateChanged()
{
    if (m_eventTimer->isActive())
        pushEvents();
}

void RemoteControlServer::pushEvents()
{
    QList<QTcpSocket *> subscribers;
    for (auto it = m_connections.cbegin(); it != m_connections.cend(); ++it) {
        if (it->streaming)
            subscribers << it.key();
    }
    if (subscribers.isEmpty()) {
        m_eventTimer->stop();
        return;
    }
    if (!m_handler)
        return;

    const Reply reply = m_handler(QStringLiteral("status"), QJsonObject());
    const QByteArray json = QJsonDocument(reply.body).toJson(QJsonDocument::Compact);

    QByteArray frame;
    if (json != m_lastEvent) {
        m_lastEvent = json;
        m_ticksSinceEvent = 0;
        frame = "event: status\ndata: " + json + "\n\n";
    } else if (++m_ticksSinceEvent >= kKeepAliveTicks) {
        // A comment line: ignored by EventSource, but it keeps proxies and
        // NAT tables from deciding the connection is dead.
        m_ticksSinceEvent = 0;
        frame = ": keep-alive\n\n";
    }
    if (frame.isEmpty())
        return;

    for (QTcpSocket *socket : std::as_const(subscribers)) {
        // A subscriber that stopped reading is not allowed to make this
        // process hold the whole broadcast day in memory for it.
        if (socket->bytesToWrite() > 256 * 1024) {
            socket->abort();
            continue;
        }
        socket->write(frame);
    }
}

// ---------------------------------------------------------------- responses

void RemoteControlServer::sendJson(QTcpSocket *socket, int status, const QJsonObject &body)
{
    const QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QByteArray response;
    response.reserve(json.size() + 256);
    response += "HTTP/1.1 " + QByteArray::number(status) + ' ' + reasonPhrase(status) + "\r\n";
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "X-Content-Type-Options: nosniff\r\n";
    if (status == 401)
        response += "WWW-Authenticate: Bearer realm=\"XFB\"\r\n";
    response += "Connection: close\r\n\r\n";
    response += json;

    if (auto it = m_connections.find(socket); it != m_connections.end())
        it->lastActivityMs = QDateTime::currentMSecsSinceEpoch();
    socket->write(response);
    socket->disconnectFromHost();
}

void RemoteControlServer::sendBinary(QTcpSocket *socket, const QByteArray &bytes,
                                     const QByteArray &contentType)
{
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + (contentType.isEmpty() ? QByteArray("application/octet-stream")
                                                          : contentType) + "\r\n";
    response += "Content-Length: " + QByteArray::number(bytes.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "X-Content-Type-Options: nosniff\r\n";
    response += "Connection: close\r\n\r\n";
    response += bytes;

    socket->write(response);
    socket->disconnectFromHost();
}

void RemoteControlServer::sendHtml(QTcpSocket *socket, const QString &page)
{
    const QByteArray encoded = page.toUtf8();

    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(encoded.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    // The page is entirely its own: inline style, inline script, and the only
    // thing it may talk to is the origin it came from. Saying so in a header
    // means the browser holds the line even if a later edit here forgets to.
    response += "Content-Security-Policy: default-src 'none'; "
                "img-src 'self' data: blob:; style-src 'unsafe-inline'; "
                "script-src 'unsafe-inline'; connect-src 'self'; "
                "form-action 'none'; base-uri 'none'; frame-ancestors 'none'\r\n";
    response += "X-Content-Type-Options: nosniff\r\n";
    response += "Referrer-Policy: no-referrer\r\n";
    response += "Connection: close\r\n\r\n";
    response += encoded;

    socket->write(response);
    socket->disconnectFromHost();
}

void RemoteControlServer::sendError(QTcpSocket *socket, int status, const QString &message)
{
    sendJson(socket, status, Reply::error(status, message).body);
}
