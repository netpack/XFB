#include "MobileSyncServer.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QSettings>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QSqlIndex>
#include <QSqlRecord>
#include <QXmlStreamReader>

#include <functional>
#include <utility>

namespace {

/** How long a pairing code stays valid. */
constexpr int kPairingWindowSeconds = 180;

/** Wrong codes tolerated before the window slams shut. */
constexpr int kMaxPairingAttempts = 5;

/** Requests larger than this are refused outright (no legitimate one is big). */
constexpr int kMaxRequestBytes = 64 * 1024;

/**
 * Except the one kind that is: a production computer publishing a jingle has
 * to get the file here somehow. It arrives in chunks rather than in one piece,
 * so this bounds a chunk, not a track, and a machine that has not authenticated
 * still cannot make this server hold more than one chunk.
 */
constexpr int kMaxUploadBytes = 8 * 1024 * 1024;

/** Uploads are parked under this suffix until the last chunk has landed. */
const QString kPartSuffix = QStringLiteral(".xfbpart");

/** The track index is cheap to build but not free; reuse it for a minute. */
constexpr int kIndexTtlSeconds = 60;

constexpr qint64 kChunkBytes = 64 * 1024;
constexpr qint64 kHighWaterBytes = 512 * 1024;

QString reasonPhrase(int status)
{
    switch (status) {
    case 200: return QStringLiteral("OK");
    case 206: return QStringLiteral("Partial Content");
    case 400: return QStringLiteral("Bad Request");
    case 401: return QStringLiteral("Unauthorized");
    case 403: return QStringLiteral("Forbidden");
    case 404: return QStringLiteral("Not Found");
    case 405: return QStringLiteral("Method Not Allowed");
    case 409: return QStringLiteral("Conflict");
    case 413: return QStringLiteral("Payload Too Large");
    case 416: return QStringLiteral("Range Not Satisfiable");
    case 429: return QStringLiteral("Too Many Requests");
    case 500: return QStringLiteral("Internal Server Error");
    case 503: return QStringLiteral("Service Unavailable");
    default:  return QStringLiteral("Error");
    }
}

/**
 * XFB opens its library on a *named* connection ("xfb_connection"), so asking
 * for the default one finds nothing and every library query comes back empty.
 */
QSqlDatabase libraryDatabase()
{
    QSqlDatabase named = QSqlDatabase::database(QStringLiteral("xfb_connection"), false);
    if (named.isValid() && named.isOpen())
        return named;
    return QSqlDatabase::database();
}

} // namespace

MobileSyncServer::MobileSyncServer(QObject *parent)
    : QObject(parent)
{
    m_pairingTimer = new QTimer(this);
    m_pairingTimer->setSingleShot(true);
    connect(m_pairingTimer, &QTimer::timeout, this, &MobileSyncServer::endPairing);

    loadTokens();
    loadSyncSet();

    QSettings settings;
    m_playlistsDir = settings.value(QStringLiteral("MobileSync/PlaylistsPath")).toString();
    m_startedAt = QDateTime::currentDateTime();
}

MobileSyncServer::~MobileSyncServer()
{
    stop();
}

// ---------------------------------------------------------------- lifecycle

bool MobileSyncServer::isListening() const
{
    return m_server && m_server->isListening();
}

bool MobileSyncServer::start(quint16 port)
{
    if (isListening())
        return true;

    if (port == 0) {
        QSettings settings;
        port = quint16(settings.value(QStringLiteral("MobileSync/Port"),
                                      defaultPort()).toUInt());
        if (port == 0)
            port = defaultPort();
    }

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection,
                this, &MobileSyncServer::onNewConnection);
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        const QString message = tr("Could not listen on port %1: %2")
                                    .arg(port).arg(m_server->errorString());
        emit errorOccurred(message);
        return false;
    }

    m_port = m_server->serverPort();
    emit started(m_port);
    return true;
}

void MobileSyncServer::stop()
{
    endPairing();

    if (m_server && m_server->isListening()) {
        m_server->close();
        const auto sockets = m_buffers.keys();
        for (QTcpSocket *socket : sockets)
            socket->abort();
        m_buffers.clear();
        m_port = 0;
        emit stopped();
    }
}

QStringList MobileSyncServer::listenAddresses() const
{
    QStringList addresses;
    const QStringList hosts = hostAddresses();
    addresses.reserve(hosts.size());
    for (const QString &host : hosts)
        addresses << QStringLiteral("http://%1:%2").arg(host).arg(m_port);
    return addresses;
}

QStringList MobileSyncServer::hostAddresses() const
{
    QStringList addresses;
    if (!isListening())
        return addresses;

    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp))
            continue;
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            addresses << ip.toString();
        }
    }
    return addresses;
}

QString MobileSyncServer::pairingUri(const QString &host) const
{
    if (m_pairingCode.isEmpty() || host.isEmpty())
        return QString();

    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(host);
    url.setPort(m_port);
    url.setPath(QStringLiteral("/pair"));

    // The host and port are already in the URL itself; only the code and the
    // protocol version need carrying.
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("code"), m_pairingCode);
    query.addQueryItem(QStringLiteral("v"), QString::number(protocolVersion()));
    url.setQuery(query);

    return url.toString(QUrl::FullyEncoded);
}

QString MobileSyncServer::pairingDeepLink(const QString &host) const
{
    if (m_pairingCode.isEmpty() || host.isEmpty())
        return QString();

    QUrl url;
    url.setScheme(QStringLiteral("xfb"));
    url.setHost(QStringLiteral("pair"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("host"), host);
    query.addQueryItem(QStringLiteral("port"), QString::number(m_port));
    query.addQueryItem(QStringLiteral("code"), m_pairingCode);
    query.addQueryItem(QStringLiteral("v"), QString::number(protocolVersion()));
    url.setQuery(query);

    return url.toString(QUrl::FullyEncoded);
}

void MobileSyncServer::setLivePlaylistProvider(std::function<QVector<Track>()> provider)
{
    m_livePlaylistProvider = std::move(provider);
}

QString MobileSyncServer::playlistsDirectory() const
{
    if (!m_playlistsDir.isEmpty())
        return m_playlistsDir;

    // Saved playlists have never had a settings key of their own; the sensible
    // default is next to wherever the operator keeps the rest of their data.
    QSettings settings;
    const QString savePath = settings.value(QStringLiteral("SavePath")).toString();
    if (!savePath.isEmpty())
        return QDir(savePath).filePath(QStringLiteral("playlists"));

    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QDir(docs).filePath(QStringLiteral("XFB/playlists"));
}

void MobileSyncServer::setPlaylistsDirectory(const QString &dir)
{
    m_playlistsDir = dir;
    QSettings settings;
    settings.setValue(QStringLiteral("MobileSync/PlaylistsPath"), dir);
    m_indexBuiltAt = QDateTime();
}

// ------------------------------------------------------------------ pairing

QString MobileSyncServer::beginPairing(PeerRole role)
{
    // Six digits, zero-padded, from the system CSPRNG.
    const quint32 value = QRandomGenerator::system()->bounded(1000000);
    m_pairingCode = QStringLiteral("%1").arg(value, 6, 10, QLatin1Char('0'));
    m_pairingRole = role;
    m_pairingAttempts = 0;
    m_pairingExpiry = QDateTime::currentDateTime().addSecs(kPairingWindowSeconds);
    m_pairingTimer->start(kPairingWindowSeconds * 1000);
    emit pairingWindowChanged();
    return m_pairingCode;
}

void MobileSyncServer::endPairing()
{
    if (m_pairingCode.isEmpty())
        return;
    m_pairingCode.clear();
    m_pairingExpiry = QDateTime();
    m_pairingTimer->stop();
    emit pairingWindowChanged();
}

int MobileSyncServer::pairingSecondsRemaining() const
{
    if (m_pairingCode.isEmpty() || !m_pairingExpiry.isValid())
        return 0;
    const qint64 secs = QDateTime::currentDateTime().secsTo(m_pairingExpiry);
    return secs > 0 ? int(secs) : 0;
}

QVector<MobileSyncServer::PairedDevice> MobileSyncServer::pairedDevices() const
{
    QVector<PairedDevice> devices;
    devices.reserve(m_tokens.size());
    for (auto it = m_tokens.constBegin(); it != m_tokens.constEnd(); ++it)
        devices << it.value();
    return devices;
}

void MobileSyncServer::revokeDevice(const QString &tokenId)
{
    for (auto it = m_tokens.begin(); it != m_tokens.end(); ++it) {
        if (it.value().tokenId == tokenId) {
            m_tokens.erase(it);
            saveTokens();
            return;
        }
    }
}

void MobileSyncServer::revokeAllDevices()
{
    m_tokens.clear();
    saveTokens();
}

void MobileSyncServer::loadTokens()
{
    m_tokens.clear();
    QSettings settings;
    const int count = settings.beginReadArray(QStringLiteral("MobileSync/Devices"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        const QString token = settings.value(QStringLiteral("token")).toString();
        if (token.isEmpty())
            continue;
        PairedDevice device;
        device.name     = settings.value(QStringLiteral("name")).toString();
        device.tokenId  = token.left(8);
        device.pairedAt = settings.value(QStringLiteral("pairedAt")).toDateTime();
        device.lastSeen = settings.value(QStringLiteral("lastSeen")).toDateTime();
        device.role = roleFromName(settings.value(QStringLiteral("role")).toString());
        m_tokens.insert(token, device);
    }
    settings.endArray();
}

void MobileSyncServer::saveTokens() const
{
    QSettings settings;
    settings.beginWriteArray(QStringLiteral("MobileSync/Devices"), m_tokens.size());
    int i = 0;
    for (auto it = m_tokens.constBegin(); it != m_tokens.constEnd(); ++it, ++i) {
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("token"), it.key());
        settings.setValue(QStringLiteral("name"), it.value().name);
        settings.setValue(QStringLiteral("pairedAt"), it.value().pairedAt);
        settings.setValue(QStringLiteral("lastSeen"), it.value().lastSeen);
        settings.setValue(QStringLiteral("role"), roleName(it.value().role));
    }
    settings.endArray();
}

// --------------------------------------------------------------- connections

void MobileSyncServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        m_buffers.insert(socket, QByteArray());

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            onReadyRead(socket);
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_buffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void MobileSyncServer::onReadyRead(QTcpSocket *socket)
{
    auto it = m_buffers.find(socket);
    if (it == m_buffers.end())
        return;

    it->append(socket->readAll());

    // Only one kind of request is allowed to be big, and it is recognisable
    // from its request line, which arrives first. Everything else — including
    // anything that never sends a request line at all — stays on the tight
    // bound it has always had.
    const bool upload = it->startsWith("POST /api/production/file");
    if (it->size() > (upload ? kMaxUploadBytes : kMaxRequestBytes)) {
        sendError(socket, 413, tr("Request too large."));
        return;
    }

    const int headerEnd = it->indexOf("\r\n\r\n");
    if (headerEnd < 0)
        return; // headers still arriving

    // A declared body may still be in flight; waiting for it here keeps
    // parseRequest's only failure mode "malformed".
    int contentLength = 0;
    const QList<QByteArray> headLines = it->left(headerEnd).split('\n');
    for (const QByteArray &line : headLines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.toLower().startsWith("content-length:")) {
            contentLength = trimmed.mid(15).trimmed().toInt();
            break;
        }
    }
    if (it->size() < headerEnd + 4 + contentLength)
        return;

    Request request;
    if (!parseRequest(*it, &request)) {
        sendError(socket, 400, tr("Malformed request."));
        return;
    }

    // Everything needed is in hand; stop buffering this connection.
    m_buffers[socket].clear();
    route(socket, request);
}

bool MobileSyncServer::parseRequest(const QByteArray &raw, Request *out) const
{
    const int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd < 0)
        return false;

    const QByteArray head = raw.left(headerEnd);
    const QList<QByteArray> lines = head.split('\n');
    if (lines.isEmpty())
        return false;

    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2)
        return false;

    out->method = requestLine.at(0).toUpper();

    const QUrl url = QUrl::fromEncoded(requestLine.at(1));
    out->path = url.path();

    const QString queryString = url.query();
    const QStringList pairs = queryString.split(QLatin1Char('&'), Qt::SkipEmptyParts);
    for (const QString &pair : pairs) {
        const int equals = pair.indexOf(QLatin1Char('='));
        if (equals <= 0)
            continue;
        out->query.insert(QUrl::fromPercentEncoding(pair.left(equals).toUtf8()),
                          QUrl::fromPercentEncoding(pair.mid(equals + 1).toUtf8()));
    }

    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const int colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        out->headers.insert(line.left(colon).toLower(),
                            line.mid(colon + 1).trimmed());
    }

    const int contentLength = out->headers.value("content-length").toInt();
    if (contentLength > 0) {
        const QByteArray body = raw.mid(headerEnd + 4);
        if (body.size() < contentLength)
            return false; // wait for the rest
        out->body = body.left(contentLength);
    }

    return true;
}

void MobileSyncServer::route(QTcpSocket *socket, const Request &request)
{
    // Unauthenticated: discovery and pairing only.
    if (request.path == QLatin1String("/api/hello")) {
        handleHello(socket);
        return;
    }
    if (request.path == QLatin1String("/api/pair")) {
        if (request.method != "POST") {
            sendError(socket, 405, tr("Pairing requires POST."));
            return;
        }
        handlePair(socket, request);
        return;
    }
    if (request.path == QLatin1String("/pair")) {
        handlePairPage(socket, request);
        return;
    }
    // The installer and its version are open on purpose: a phone with no app
    // yet has no token, and getting the app is the first thing it needs.
    if (request.path == QLatin1String("/api/app")) {
        handleAppInfo(socket);
        return;
    }
    if (request.path == QLatin1String("/app.apk")) {
        handleAppDownload(socket, request);
        return;
    }

    PeerRole role = PeerRole::Mobile;
    const QString device = authenticate(request, &role);
    if (device.isEmpty()) {
        sendError(socket, 401, tr("Not paired with this XFB."));
        return;
    }

    // Publishing is the one thing a peer may do that changes this station, so
    // it is also the only thing that may arrive as anything but a GET.
    const bool production = request.path.startsWith(QLatin1String("/api/production/"));
    if (request.method != "GET" && !(production && request.method == "POST")) {
        sendError(socket, 405, tr("Only GET is supported here."));
        return;
    }

    // What a production computer publishes: files first, then the catalogue
    // rows that point at them, then any playlist it built. Locked to the
    // production role — a backup station reads this station, it does not
    // write to it, and a phone does neither.
    if (production) {
        if (role != PeerRole::Production) {
            sendError(socket, 403,
                      tr("This device is not paired as a production computer."));
            return;
        }
        if (request.path == QLatin1String("/api/production/hello")) {
            handleProductionHello(socket);
        } else if (request.path == QLatin1String("/api/production/have")) {
            handleProductionHave(socket, request);
        } else if (request.path == QLatin1String("/api/production/file")) {
            handleProductionFile(socket, request);
        } else if (request.path == QLatin1String("/api/production/rows")) {
            handleProductionRows(socket, request, device);
        } else if (request.path == QLatin1String("/api/production/playlist")) {
            handleProductionPlaylist(socket, request);
        } else {
            sendError(socket, 404, tr("No such endpoint."));
        }
        return;
    }

    // Everything a standby station needs to become this one. A production
    // computer reads the very same thing — it cannot prepare next week's
    // programme without the catalogue it will be added to. A paired phone
    // gets 403 here, not the catalogue.
    if (request.path.startsWith(QLatin1String("/api/station/"))) {
        if (role != PeerRole::Station && role != PeerRole::Production) {
            sendError(socket, 403, tr("This device is not paired as a backup station."));
            return;
        }
        if (request.path == QLatin1String("/api/station/heartbeat")) {
            // Polled every few seconds by a machine standing by, so it is
            // deliberately not announced through deviceActivity(): the
            // operator's activity list would be nothing else.
            handleStationHeartbeat(socket);
        } else if (request.path == QLatin1String("/api/station/manifest")) {
            emit deviceActivity(device, tr("read the station manifest"));
            handleStationManifest(socket);
        } else if (request.path == QLatin1String("/api/station/file")) {
            handleStationFile(socket, request);
        } else if (request.path == QLatin1String("/api/station/playlist")) {
            handleStationPlaylist(socket, request);
        } else {
            sendError(socket, 404, tr("No such endpoint."));
        }
        return;
    }

    if (request.path == QLatin1String("/api/library")) {
        emit deviceActivity(device, tr("browsed the library"));
        handleLibrary(socket);
    } else if (request.path == QLatin1String("/api/playlists")) {
        emit deviceActivity(device, tr("listed playlists"));
        handlePlaylists(socket);
    } else if (request.path == QLatin1String("/api/playlist")) {
        emit deviceActivity(device, tr("opened a playlist"));
        handlePlaylist(socket, request);
    } else if (request.path == QLatin1String("/api/track")) {
        handleTrack(socket, request);
    } else if (request.path == QLatin1String("/api/incidents")) {
        // Every paired role may read this. The server cannot call a phone, so
        // this is where "the station went dark" is left for one to find.
        handleIncidents(socket, request);
    } else {
        sendError(socket, 404, tr("No such endpoint."));
    }
}

QString MobileSyncServer::authenticate(const Request &request, PeerRole *role)
{
    QByteArray header = request.headers.value("authorization");
    if (!header.startsWith("Bearer "))
        return QString();

    const QString token = QString::fromUtf8(header.mid(7)).trimmed();
    auto it = m_tokens.find(token);
    if (it == m_tokens.end())
        return QString();

    it->lastSeen = QDateTime::currentDateTime();
    if (role)
        *role = it->role;
    return it->name;
}

// ----------------------------------------------------------------- endpoints

void MobileSyncServer::handleHello(QTcpSocket *socket)
{
    QJsonObject object;
    object.insert(QStringLiteral("application"), QStringLiteral("XFB"));
    object.insert(QStringLiteral("protocol"), protocolVersion());
    object.insert(QStringLiteral("name"), QSysInfo::machineHostName());
    object.insert(QStringLiteral("pairing"), pairingOpen());
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handlePair(QTcpSocket *socket, const Request &request)
{
    if (!pairingOpen()) {
        sendError(socket, 403, tr("XFB is not accepting new devices right now."));
        return;
    }

    const QJsonObject body = QJsonDocument::fromJson(request.body).object();
    const QString code = body.value(QStringLiteral("code")).toString().trimmed();
    QString deviceName = body.value(QStringLiteral("device")).toString().trimmed();
    if (deviceName.isEmpty()) {
        switch (m_pairingRole) {
        case PeerRole::Station:    deviceName = tr("Backup station"); break;
        case PeerRole::Production: deviceName = tr("Production computer"); break;
        case PeerRole::Mobile:     deviceName = tr("Android device"); break;
        }
    }
    deviceName = deviceName.left(64);

    if (code != m_pairingCode) {
        if (++m_pairingAttempts >= kMaxPairingAttempts) {
            endPairing();
            sendError(socket, 429, tr("Too many wrong codes; pairing closed."));
            return;
        }
        sendError(socket, 403, tr("That code does not match."));
        return;
    }

    const QByteArray raw = QByteArray::number(QRandomGenerator::system()->generate64(), 16)
                           + QByteArray::number(QRandomGenerator::system()->generate64(), 16);
    const QString token = QString::fromLatin1(
        QCryptographicHash::hash(raw, QCryptographicHash::Sha256).toHex());

    PairedDevice device;
    device.name     = deviceName;
    device.tokenId  = token.left(8);
    device.pairedAt = QDateTime::currentDateTime();
    device.lastSeen = device.pairedAt;
    // The role comes from the window the operator opened, never from what the
    // caller asked for — otherwise a paired phone could request the lot.
    device.role     = m_pairingRole;
    m_tokens.insert(token, device);
    saveTokens();

    // One code, one device.
    endPairing();

    QJsonObject object;
    object.insert(QStringLiteral("token"), token);
    object.insert(QStringLiteral("protocol"), protocolVersion());
    object.insert(QStringLiteral("role"), roleName(device.role));
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));

    emit devicePaired(deviceName);
}

void MobileSyncServer::handlePairPage(QTcpSocket *socket, const Request &request)
{
    // The address the phone actually reached us on, taken from the connection
    // rather than the Host header so a spoofed header cannot steer the link.
    QString host = socket->localAddress().toString();
    if (host.startsWith(QLatin1String("::ffff:")))
        host = host.mid(7);
    if (host.isEmpty() || host == QLatin1String("::1"))
        host = hostAddresses().value(0);

    // Only ever a run of six digits reaches the page, so there is nothing to
    // inject through it. Anything else is treated as no code at all.
    QString code = request.query.value(QStringLiteral("code"));
    bool digitsOnly = code.size() == 6;
    for (const QChar character : code)
        if (!character.isDigit())
            digitsOnly = false;
    if (!digitsOnly)
        code.clear();

    const bool matches = !code.isEmpty() && pairingOpen() && code == m_pairingCode;

    // Offer the installer when one is sitting next to XFB, so a phone with no
    // app can get one without being sent somewhere else to find it.
    const QString apk = companionApkPath();
    QString install;
    if (!apk.isEmpty()) {
        const QString version = companionVersionName();
        install = QStringLiteral(
            "<p><a class=\"button secondary\" href=\"/app.apk\">Download the XFB app%1</a></p>"
            "<p class=\"note\">Your phone will ask whether to allow installing "
            "an app from this browser; that prompt is Android checking you meant "
            "to, because this came from your own computer rather than the "
            "Play Store.</p>")
            .arg(version.isEmpty() ? QString()
                                   : QStringLiteral(" (%1)").arg(version.toHtmlEscaped()));
    } else {
        // Say so rather than showing nothing. Without this the page above still
        // reads "install it below" while there is no "below", which looks like
        // the page failed to load rather than like this XFB having no copy of
        // the app to hand out.
        install = QStringLiteral(
            "<p class=\"note\">This XFB has no copy of the phone app to hand "
            "out, so there is nothing to download here. Put "
            "<code>xfb-companion.apk</code> next to XFB on the computer &mdash; "
            "the folder is named under Options &rsaquo; Sync to Phone &mdash; "
            "and this page will offer it.</p>");
    }

    QString body;
    if (matches) {
        const QString deepLink = pairingDeepLink(host).toHtmlEscaped();
        body = QStringLiteral(
            "<p class=\"lead\">Open this in the XFB app to finish pairing.</p>"
            "<p class=\"code\">%1</p>"
            "<p><a class=\"button\" href=\"%2\">Open in XFB</a></p>"
            "<p class=\"note\">Nothing happens when you tap that? The XFB app "
            "is not installed on this phone yet &mdash; install it, then scan "
            "the code again, or type the six digits into the app by hand.</p>"
            "%3")
            .arg(code, deepLink, install);
    } else if (pairingOpen()) {
        body = QStringLiteral(
            "<p class=\"lead\">This pairing code has expired.</p>"
            "<p class=\"note\">Ask for a fresh code in XFB, under "
            "Options &rsaquo; Sync to Phone, and scan it again.</p>%1").arg(install);
    } else {
        body = QStringLiteral(
            "<p class=\"lead\">XFB is not accepting new phones right now.</p>"
            "<p class=\"note\">In XFB, open Options &rsaquo; Sync to Phone and "
            "choose &ldquo;Show a pairing code&rdquo;, then scan it again.</p>%1").arg(install);
    }

    const QString page = QStringLiteral(
        "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Pair with XFB</title><style>"
        ":root{color-scheme:light dark}"
        "body{margin:0;padding:2.5rem 1.5rem;font:16px/1.6 -apple-system,"
        "BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;max-width:26rem;"
        "margin-inline:auto;background:#fff;color:#16181c}"
        "@media(prefers-color-scheme:dark){body{background:#101215;color:#e8ecef}}"
        "h1{font-size:1.4rem;margin:0 0 1.5rem;letter-spacing:-.02em}"
        ".lead{font-size:1.05rem;margin:0 0 1rem}"
        ".code{font:600 2.4rem/1 ui-monospace,SFMono-Regular,Menlo,monospace;"
        "letter-spacing:.16em;margin:1.5rem 0}"
        ".button{display:inline-block;background:#7c7cba;color:#000;"
        "text-decoration:none;padding:.8rem 1.5rem;border-radius:4px;"
        "font-weight:600}"
        ".button.secondary{background:transparent;color:#7c7cba;"
        "border:1px solid #7c7cba}"
        ".note{color:#6b7280;font-size:.92rem;margin-top:1.5rem}"
        "@media(prefers-color-scheme:dark){.note{color:#a6b0b9}"
        ".button{background:#7c7cba;color:#000}"
        ".button.secondary{background:transparent;color:#9a9ad0;border-color:#9a9ad0}}"
        "</style></head><body><h1>Pair with XFB</h1>%1"
        // The phone is standing in front of the decision, so this is where the
        // transport gets described. Plain HTTP on the local network: the code
        // keeps strangers out, it does not encrypt anything.
        "<p class=\"note\">XFB serves your library over this local network "
        "without encryption. Anyone else on the same Wi-Fi can see which "
        "tracks your phone downloads.</p>"
        "</body></html>")
        .arg(body);

    const QByteArray encoded = page.toUtf8();
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(encoded.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "Connection: close\r\n\r\n";
    response += encoded;

    socket->write(response);
    socket->disconnectFromHost();
}

void MobileSyncServer::handleLibrary(QTcpSocket *socket)
{
    rebuildIndexIfStale();

    QJsonArray array;
    const QVector<Track> tracks = libraryTracks();
    for (const Track &track : tracks) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), track.id);
        object.insert(QStringLiteral("artist"), track.artist);
        object.insert(QStringLiteral("song"), track.song);
        object.insert(QStringLiteral("duration"), track.duration);
        object.insert(QStringLiteral("bytes"), track.bytes);
        array.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("tracks"), array);
    sendJson(socket, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handlePlaylists(QTcpSocket *socket)
{
    QJsonArray array;

    // Whatever the operator marked in the music list, offered first because it
    // is the thing they just asked for.
    if (!m_syncSet.isEmpty()) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), syncSetName());
        object.insert(QStringLiteral("title"), tr("Marked for this phone"));
        object.insert(QStringLiteral("live"), false);
        object.insert(QStringLiteral("tracks"), m_syncSet.size());
        array.append(object);
    }

    if (m_livePlaylistProvider) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), QStringLiteral("__live__"));
        object.insert(QStringLiteral("title"), tr("Playlist on air now"));
        object.insert(QStringLiteral("live"), true);
        object.insert(QStringLiteral("tracks"), m_livePlaylistProvider().size());
        array.append(object);
    }

    QDir dir(playlistsDirectory());
    const QFileInfoList files = dir.entryInfoList(QStringList() << QStringLiteral("*.xml"),
                                                 QDir::Files, QDir::Name);
    for (const QFileInfo &info : files) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), info.completeBaseName());
        object.insert(QStringLiteral("title"), info.completeBaseName());
        object.insert(QStringLiteral("live"), false);
        array.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("playlists"), array);
    sendJson(socket, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handlePlaylist(QTcpSocket *socket, const Request &request)
{
    const QString name = request.query.value(QStringLiteral("name"));
    if (name.isEmpty()) {
        sendError(socket, 400, tr("Which playlist?"));
        return;
    }

    rebuildIndexIfStale();

    QVector<Track> tracks;
    if (name == syncSetName()) {
        tracks = syncSetTracks();
        if (tracks.isEmpty()) {
            sendError(socket, 404, tr("Nothing is marked for this phone."));
            return;
        }
        for (const Track &track : tracks)
            m_index.insert(track.id, track.path);
    } else if (name == QLatin1String("__live__")) {
        if (!m_livePlaylistProvider) {
            sendError(socket, 404, tr("No playlist is loaded."));
            return;
        }
        tracks = m_livePlaylistProvider();
        // The live playlist can name tracks the index has not seen yet.
        for (const Track &track : tracks)
            m_index.insert(track.id, track.path);
    } else {
        tracks = playlistTracks(name);
        if (tracks.isEmpty()) {
            sendError(socket, 404, tr("No such playlist."));
            return;
        }
    }

    QJsonArray array;
    for (const Track &track : tracks) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), track.id);
        object.insert(QStringLiteral("artist"), track.artist);
        object.insert(QStringLiteral("song"), track.song);
        object.insert(QStringLiteral("duration"), track.duration);
        object.insert(QStringLiteral("bytes"), track.bytes);
        if (track.overlapMs > 0)
            object.insert(QStringLiteral("overlapMs"), track.overlapMs);
        if (!track.volumeEnvelope.isEmpty())
            object.insert(QStringLiteral("volumeEnvelope"), track.volumeEnvelope);
        array.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("name"), name);
    root.insert(QStringLiteral("tracks"), array);
    sendJson(socket, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleTrack(QTcpSocket *socket, const Request &request)
{
    const QString id = request.query.value(QStringLiteral("id"));
    if (id.isEmpty()) {
        sendError(socket, 400, tr("Which track?"));
        return;
    }

    const QString path = pathForId(id);
    if (path.isEmpty()) {
        // Unknown id — the allow-list is the whole access control story here,
        // so this is also what a traversal attempt looks like.
        sendError(socket, 404, tr("No such track."));
        return;
    }

    sendFile(socket, path, request);
}

// --------------------------------------------------------------- incidents

void MobileSyncServer::postIncident(const QString &kind, const QString &reason,
                                    const QString &detail)
{
    Incident incident;
    incident.when = QDateTime::currentDateTime();
    incident.kind = kind;
    incident.reason = reason;
    incident.detail = detail;
    m_incidents.append(incident);
    while (m_incidents.size() > kMaxIncidents)
        m_incidents.removeFirst();
}

void MobileSyncServer::resolveIncident(const QString &kind, const QString &detail)
{
    for (int i = m_incidents.size() - 1; i >= 0; --i) {
        if (m_incidents[i].kind != kind || m_incidents[i].resolvedAt.isValid())
            continue;
        m_incidents[i].resolvedAt = QDateTime::currentDateTime();
        if (!detail.isEmpty())
            m_incidents[i].detail += QStringLiteral(" — ") + detail;
        return;
    }
}

void MobileSyncServer::setStationStateProvider(std::function<QJsonObject()> provider)
{
    m_stationStateProvider = std::move(provider);
}

// Any paired device may read this: a phone that cannot be told the station is
// dark is not much of a companion, and an incident says nothing about the
// library it would not already be allowed to see.
void MobileSyncServer::handleIncidents(QTcpSocket *socket, const Request &request)
{
    bool ok = false;
    const int since = request.query.value(QStringLiteral("since")).toInt(&ok);

    QJsonArray array;
    for (const Incident &incident : m_incidents) {
        if (ok && since > 0 && incident.when.toSecsSinceEpoch() <= since)
            continue;
        QJsonObject object;
        object.insert(QStringLiteral("at"),
                      incident.when.toString(Qt::ISODate));
        object.insert(QStringLiteral("epoch"), incident.when.toSecsSinceEpoch());
        object.insert(QStringLiteral("kind"), incident.kind);
        object.insert(QStringLiteral("reason"), incident.reason);
        object.insert(QStringLiteral("detail"), incident.detail);
        object.insert(QStringLiteral("open"), !incident.resolvedAt.isValid());
        if (incident.resolvedAt.isValid()) {
            object.insert(QStringLiteral("resolvedAt"),
                          incident.resolvedAt.toString(Qt::ISODate));
        }
        array.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("station"), QSysInfo::machineHostName());
    root.insert(QStringLiteral("now"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    root.insert(QStringLiteral("incidents"), array);
    sendJson(socket, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

// The backup's whole view of whether this station is still making sound. Kept
// deliberately small and free of file access: a machine standing by asks for
// it every few seconds, forever.
void MobileSyncServer::handleStationHeartbeat(QTcpSocket *socket)
{
    QJsonObject root = m_stationStateProvider ? m_stationStateProvider()
                                              : QJsonObject();

    if (!root.contains(QStringLiteral("state"))) {
        // No provider installed (a headless or half-built XFB): say so
        // honestly rather than claiming the station is fine.
        root.insert(QStringLiteral("state"), QStringLiteral("unknown"));
    }
    root.insert(QStringLiteral("station"), QSysInfo::machineHostName());
    root.insert(QStringLiteral("protocol"), protocolVersion());
    root.insert(QStringLiteral("now"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (m_startedAt.isValid()) {
        root.insert(QStringLiteral("serverUptime"),
                    m_startedAt.secsTo(QDateTime::currentDateTime()));
    }

    int openIncidents = 0;
    for (const Incident &incident : m_incidents) {
        if (!incident.resolvedAt.isValid())
            ++openIncidents;
    }
    root.insert(QStringLiteral("openIncidents"), openIncidents);
    if (!m_incidents.isEmpty()) {
        root.insert(QStringLiteral("lastIncident"), m_incidents.last().detail);
        root.insert(QStringLiteral("lastIncidentAt"),
                    m_incidents.last().when.toString(Qt::ISODate));
    }

    sendJson(socket, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

// ----------------------------------------------------------- track index

QString MobileSyncServer::idForPath(const QString &path)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256)
            .toHex().left(32));
}

QString MobileSyncServer::roleName(PeerRole role)
{
    switch (role) {
    case PeerRole::Station:    return QStringLiteral("station");
    case PeerRole::Production: return QStringLiteral("production");
    case PeerRole::Mobile:     break;
    }
    return QStringLiteral("mobile");
}

MobileSyncServer::PeerRole MobileSyncServer::roleFromName(const QString &name)
{
    if (name == QLatin1String("station"))
        return PeerRole::Station;
    if (name == QLatin1String("production"))
        return PeerRole::Production;
    // Anything unrecognised — an older XFB, a garbled settings file, a caller
    // trying its luck — gets the role that can do the least.
    return PeerRole::Mobile;
}

QStringList MobileSyncServer::mediaCategories()
{
    return {
        QStringLiteral("musics"), QStringLiteral("jingles"),
        QStringLiteral("pub"), QStringLiteral("programs"),
    };
}

bool MobileSyncServer::isSafeRelativePath(const QString &relative)
{
    if (relative.isEmpty() || relative.size() > 512)
        return false;
    if (relative.startsWith(QLatin1Char('/')) || relative.startsWith(QLatin1Char('\\')))
        return false;
    if (relative.contains(QLatin1Char(':')))
        return false;
    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return false;
    for (const QString &part : parts) {
        if (part == QLatin1String("..") || part == QLatin1String("."))
            return false;
        if (part.contains(QLatin1Char('\\')))
            return false;
    }
    return true;
}

void MobileSyncServer::rebuildIndexIfStale()
{
    if (m_indexBuiltAt.isValid()
        && m_indexBuiltAt.secsTo(QDateTime::currentDateTime()) < kIndexTtlSeconds) {
        return;
    }

    m_index.clear();

    const QVector<Track> library = libraryTracks();
    for (const Track &track : library)
        m_index.insert(track.id, track.path);

    QDir dir(playlistsDirectory());
    const QFileInfoList files = dir.entryInfoList(QStringList() << QStringLiteral("*.xml"),
                                                  QDir::Files);
    for (const QFileInfo &info : files) {
        const QVector<Track> tracks = playlistTracks(info.completeBaseName());
        for (const Track &track : tracks)
            m_index.insert(track.id, track.path);
    }

    if (m_livePlaylistProvider) {
        const QVector<Track> tracks = m_livePlaylistProvider();
        for (const Track &track : tracks)
            m_index.insert(track.id, track.path);
    }

    // Marked tracks may not belong to any playlist, so they need adding on
    // their own account or /api/track would refuse them.
    for (const QString &path : m_syncSet)
        m_index.insert(idForPath(path), path);

    m_indexBuiltAt = QDateTime::currentDateTime();
}

QString MobileSyncServer::pathForId(const QString &id)
{
    rebuildIndexIfStale();
    const QString path = m_index.value(id);
    if (path.isEmpty())
        return QString();
    // The index can outlive the file it points at.
    return QFile::exists(path) ? path : QString();
}

QVector<MobileSyncServer::Track> MobileSyncServer::libraryTracks()
{
    QVector<Track> tracks;

    QSqlDatabase db = libraryDatabase();
    if (!db.isValid() || !db.isOpen())
        return tracks;

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT artist, song, path, time FROM musics")))
        return tracks;

    while (query.next()) {
        Track track;
        track.artist   = query.value(0).toString();
        track.song     = query.value(1).toString();
        track.path     = query.value(2).toString();
        track.duration = query.value(3).toString();
        if (track.path.isEmpty())
            continue;
        track.id    = idForPath(track.path);
        track.bytes = QFileInfo(track.path).size();
        tracks << track;
    }

    return tracks;
}

// ------------------------------------------------------ the companion app

QString MobileSyncServer::companionApkPath() const
{
    QSettings settings;
    const QString configured = settings.value(QStringLiteral("MobileSync/ApkPath")).toString();
    if (!configured.isEmpty())
        return QFile::exists(configured) ? configured : QString();

    // Wherever the packaging happens to have put it. Same shape as the
    // server-scripts lookup: user data first, then the installed locations.
    QStringList candidates;
    const QString name = QStringLiteral("xfb-companion.apk");

    candidates << QDir(QCoreApplication::applicationDirPath()).filePath(name);
#ifdef Q_OS_MACOS
    // Inside the bundle: MacOS/../Resources
    candidates << QDir(QCoreApplication::applicationDirPath())
                      .filePath(QStringLiteral("../Resources/") + name);
#endif
    for (const QString &dir : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation))
        candidates << QDir(dir).filePath(name);
    candidates << QStringLiteral("/usr/share/xfb/") + name;

    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isFile())
            return info.absoluteFilePath();
    }
    return QString();
}

QString MobileSyncServer::companionDropDirectory() const
{
    const QStringList locations =
        QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    return locations.isEmpty() ? QString() : locations.first();
}

/** Reads the sidecar written next to the APK, if there is one. */
static QJsonObject companionSidecar(const QString &apkPath)
{
    if (apkPath.isEmpty())
        return QJsonObject();
    QFileInfo info(apkPath);
    QFile file(QDir(info.absolutePath()).filePath(QStringLiteral("xfb-companion.json")));
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();
    return QJsonDocument::fromJson(file.readAll()).object();
}

int MobileSyncServer::companionVersionCode() const
{
    return companionSidecar(companionApkPath())
        .value(QStringLiteral("versionCode")).toInt(0);
}

QString MobileSyncServer::companionVersionName() const
{
    return companionSidecar(companionApkPath())
        .value(QStringLiteral("versionName")).toString();
}

void MobileSyncServer::handleAppInfo(QTcpSocket *socket)
{
    const QString apk = companionApkPath();

    QJsonObject object;
    object.insert(QStringLiteral("available"), !apk.isEmpty());
    if (!apk.isEmpty()) {
        object.insert(QStringLiteral("bytes"), QFileInfo(apk).size());
        object.insert(QStringLiteral("url"), QStringLiteral("/app.apk"));
        const int code = companionVersionCode();
        if (code > 0)
            object.insert(QStringLiteral("versionCode"), code);
        const QString name = companionVersionName();
        if (!name.isEmpty())
            object.insert(QStringLiteral("versionName"), name);
    }
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleAppDownload(QTcpSocket *socket, const Request &request)
{
    const QString apk = companionApkPath();
    if (apk.isEmpty()) {
        sendError(socket, 404, tr("No companion app is available from this XFB."));
        return;
    }
    sendFile(socket, apk, request);
}

// ------------------------------------------------------- marked-for-phone

int MobileSyncServer::addToSyncSet(const QStringList &paths)
{
    int added = 0;
    for (const QString &path : paths) {
        if (path.isEmpty() || m_syncSet.contains(path))
            continue;
        m_syncSet << path;
        ++added;
    }
    if (added > 0) {
        saveSyncSet();
        m_indexBuiltAt = QDateTime();   // the new paths must become requestable
        emit syncSetChanged(m_syncSet.size());
    }
    return added;
}

void MobileSyncServer::clearSyncSet()
{
    if (m_syncSet.isEmpty())
        return;
    m_syncSet.clear();
    saveSyncSet();
    m_indexBuiltAt = QDateTime();
    emit syncSetChanged(0);
}

void MobileSyncServer::loadSyncSet()
{
    QSettings settings;
    m_syncSet = settings.value(QStringLiteral("MobileSync/SyncSet")).toStringList();
}

void MobileSyncServer::saveSyncSet() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("MobileSync/SyncSet"), m_syncSet);
}

QVector<MobileSyncServer::Track> MobileSyncServer::syncSetTracks()
{
    QVector<Track> tracks;
    if (m_syncSet.isEmpty())
        return tracks;

    QSqlDatabase db = libraryDatabase();
    QSqlQuery lookup(db);
    const bool haveDb = db.isValid() && db.isOpen()
        && lookup.prepare(QStringLiteral(
               "SELECT artist, song, time FROM musics WHERE path = :path"));

    for (const QString &path : m_syncSet) {
        const QFileInfo info(path);
        if (!info.exists())
            continue;   // deleted since it was marked

        Track track;
        track.path  = path;
        track.id    = idForPath(path);
        track.bytes = info.size();
        track.song  = info.completeBaseName();

        if (haveDb) {
            lookup.bindValue(QStringLiteral(":path"), path);
            if (lookup.exec() && lookup.next()) {
                const QString artist = lookup.value(0).toString();
                const QString song   = lookup.value(1).toString();
                if (!artist.isEmpty()) track.artist = artist;
                if (!song.isEmpty())   track.song   = song;
                track.duration = lookup.value(2).toString();
            }
        }

        tracks << track;
    }

    return tracks;
}

QVector<MobileSyncServer::Track> MobileSyncServer::playlistTracks(const QString &name)
{
    QVector<Track> tracks;

    // The name is a base name chosen from our own listing; rebuilding the path
    // from the directory plus the base name keeps a crafted name harmless.
    const QString fileName = QDir(playlistsDirectory())
                                 .filePath(QFileInfo(name).fileName() + QStringLiteral(".xml"));

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return tracks;

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QLatin1String("track"))
            continue;

        const QXmlStreamAttributes attributes = xml.attributes();
        Track track;
        track.overlapMs = attributes.value(QLatin1String("overlap")).toLongLong();
        track.volumeEnvelope = attributes.value(QLatin1String("volenv")).toString();
        track.path = xml.readElementText().trimmed();
        if (track.path.isEmpty())
            continue;

        const QFileInfo info(track.path);
        track.id    = idForPath(track.path);
        track.bytes = info.size();
        track.song  = info.completeBaseName();
        tracks << track;
    }

    return tracks;
}

// ----------------------------------------------------------------- responses


// ------------------------------------------- mirroring onto a backup station
//
// A radio station cannot wait for a rebuild when the studio machine dies. The
// answer here is a second XFB on the same network that holds a copy of
// everything — the catalogue, the jingles, the ads, the programs, the schedule
// and the saved playlists — and is one launch away from going on air. This
// side of that is the source station: it describes what it holds and hands
// over whatever the backup asks for. Nothing is ever pushed, so the backup can
// be behind a firewall, switched off for a week, or moved to another building
// without anything here having to know.

QString MobileSyncServer::categoryRoot(const QString &category)
{
    // The media folders live in xfb.conf (the Options dialog writes them
    // there), not in the native settings this class keeps its own keys in.
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + QStringLiteral("/xfb.conf"), QSettings::IniFormat);
    if (category == QLatin1String("musics"))
        return settings.value(QStringLiteral("MusicPath")).toString();
    if (category == QLatin1String("jingles"))
        return settings.value(QStringLiteral("JinglePath")).toString();
    if (category == QLatin1String("programs"))
        return settings.value(QStringLiteral("ProgramsPath")).toString();
    if (category == QLatin1String("pub"))
        return settings.value(QStringLiteral("StationSync/PubPath")).toString();
    return QString();
}

MobileSyncServer::MirrorFile MobileSyncServer::describeFile(const QString &path,
                                                            const QString &category)
{
    MirrorFile file;
    file.path = path;
    file.id = idForPath(path);

    const QFileInfo info(path);
    if (info.exists()) {
        file.bytes = info.size();
        file.modified = info.lastModified().toSecsSinceEpoch();
    }

    // The backup keeps its own media folders, so what travels is the position
    // of the file *inside* its category, never the source machine's paths.
    const QString root = QDir::cleanPath(categoryRoot(category));
    const QString clean = QDir::cleanPath(path);
    if (!root.isEmpty() && clean.startsWith(root + QLatin1Char('/'))) {
        file.relative = clean.mid(root.size() + 1);
    } else {
        // Files the operator added from somewhere else entirely still have to
        // land somewhere predictable and collision-free. Everything from one
        // source folder stays together in one folder on the backup.
        const QString folderKey = QString::fromLatin1(
            QCryptographicHash::hash(info.absolutePath().toUtf8(),
                                     QCryptographicHash::Sha1).toHex().left(8));
        file.relative = QStringLiteral("_elsewhere/") + folderKey
                        + QLatin1Char('/') + info.fileName();
    }
    return file;
}

QStringList MobileSyncServer::mirroredSettingKeys()
{
    // Everything machine-specific is left out on purpose: folders, ports,
    // credentials and window geometry all belong to the machine XFB runs on.
    return {
        QStringLiteral("Normalize_Soft"),
        QStringLiteral("Disable_Seek_Bar"),
        QStringLiteral("Disable_Volume"),
        QStringLiteral("AutoAutoMix"),
        QStringLiteral("AutoMixThresholdPercent"),
        QStringLiteral("AutoModeMatchBpm"),
        QStringLiteral("AutoModeBpmTolerance"),
        QStringLiteral("MaxOverlapSeconds"),
        QStringLiteral("PlaylistWaveView"),
        QStringLiteral("HideProgressInWaveView"),
        QStringLiteral("MusicFormat"),
        QStringLiteral("MusicKeepVideo"),
        QStringLiteral("MusicEmbedThumbnail"),
        QStringLiteral("MusicEmbedMetadata"),
        QStringLiteral("ComHour"),
        QStringLiteral("Theme"),
        QStringLiteral("AccentColor"),
        QStringLiteral("Language"),
        QStringLiteral("ShowFxTab"),
        QStringLiteral("ShowPadsTab"),
        QStringLiteral("ShowLevelMeter"),
        QStringLiteral("LevelMeterPlacement"),
    };
}

namespace {

/**
 * Reads a whole table into JSON, column names and all, so the shape of the
 * manifest follows the database rather than a hand-written list that would
 * quietly drop a column the next migration adds.
 *
 * A `path` column never travels as itself: the backup has its own folders, so
 * the row carries an opaque file id and a position inside its category
 * instead, and the absolute path stays on this machine.
 */
QJsonArray dumpTable(QSqlDatabase &db, const QString &table, const QString &category,
                     QHash<QString, QString> *index,
                     const std::function<MobileSyncServer::MirrorFile(const QString &,
                                                                      const QString &)> &describe)
{
    QJsonArray rows;
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT * FROM \"%1\"").arg(table)))
        return rows;

    while (query.next()) {
        const QSqlRecord record = query.record();
        QJsonObject row;
        QString path;
        for (int column = 0; column < record.count(); ++column) {
            const QString name = record.fieldName(column);
            const QVariant value = query.value(column);
            if (name.compare(QLatin1String("path"), Qt::CaseInsensitive) == 0) {
                path = value.toString();
                continue;
            }
            if (value.isNull()) {
                row.insert(name, QJsonValue::Null);
            } else if (value.typeId() == QMetaType::Int
                       || value.typeId() == QMetaType::LongLong) {
                row.insert(name, value.toLongLong());
            } else if (value.typeId() == QMetaType::Double) {
                row.insert(name, value.toDouble());
            } else {
                row.insert(name, value.toString());
            }
        }

        if (!category.isEmpty()) {
            if (path.isEmpty())
                continue;   // a media row with no file is of no use to a backup
            const MobileSyncServer::MirrorFile file = describe(path, category);
            QJsonObject fileObject;
            fileObject.insert(QStringLiteral("id"), file.id);
            fileObject.insert(QStringLiteral("rel"), file.relative);
            fileObject.insert(QStringLiteral("bytes"), file.bytes);
            fileObject.insert(QStringLiteral("mtime"), file.modified);
            row.insert(QStringLiteral("file"), fileObject);
            if (index)
                index->insert(file.id, file.path);
        }

        rows.append(row);
    }
    return rows;
}

} // namespace

QJsonObject MobileSyncServer::buildStationManifest()
{
    QJsonObject manifest;
    manifest.insert(QStringLiteral("protocol"), protocolVersion());
    manifest.insert(QStringLiteral("station"), QSysInfo::machineHostName());
    manifest.insert(QStringLiteral("generatedAt"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    m_stationIndex.clear();

    QSqlDatabase db = libraryDatabase();
    QJsonObject tables;
    if (db.isValid() && db.isOpen()) {
        auto describe = [](const QString &path, const QString &category) {
            return describeFile(path, category);
        };
        // Tables whose rows point at a file the backup has to fetch...
        const struct { const char *table; const char *category; } media[] = {
            {"musics",   "musics"},
            {"jingles",  "jingles"},
            {"pub",      "pub"},
            {"programs", "programs"},
        };
        for (const auto &entry : media) {
            tables.insert(QLatin1String(entry.table),
                          dumpTable(db, QLatin1String(entry.table),
                                    QLatin1String(entry.category),
                                    &m_stationIndex, describe));
        }
        // ...and the ones that are pure station configuration.
        for (const char *table : {"genres1", "genres2", "hourgenre",
                                  "hourprograms", "scheduler"}) {
            tables.insert(QLatin1String(table),
                          dumpTable(db, QLatin1String(table), QString(),
                                    nullptr, describe));
        }
    }
    manifest.insert(QStringLiteral("tables"), tables);
    m_stationIndexBuiltAt = QDateTime::currentDateTime();

    // Saved playlists travel as their own XML, so a backup that takes over
    // mid-show can load the very playlist that was on air.
    QJsonArray playlists;
    QDir dir(playlistsDirectory());
    const QFileInfoList files = dir.entryInfoList(QStringList()
                                                      << QStringLiteral("*.xml"),
                                                  QDir::Files);
    for (const QFileInfo &info : files) {
        QJsonObject entry;
        entry.insert(QStringLiteral("name"), info.completeBaseName());
        entry.insert(QStringLiteral("bytes"), info.size());
        entry.insert(QStringLiteral("mtime"), info.lastModified().toSecsSinceEpoch());
        playlists.append(entry);
    }
    manifest.insert(QStringLiteral("playlists"), playlists);

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + QStringLiteral("/xfb.conf"), QSettings::IniFormat);
    QJsonObject mirrored;
    const QStringList wanted = mirroredSettingKeys();
    for (const QString &key : wanted) {
        const QVariant value = settings.value(key);
        if (value.isValid())
            mirrored.insert(key, value.toString());
    }
    // The FX chain is a group rather than a fixed list of keys.
    settings.beginGroup(QStringLiteral("Fx"));
    const QStringList fxKeys = settings.allKeys();
    for (const QString &key : fxKeys)
        mirrored.insert(QStringLiteral("Fx/") + key, settings.value(key).toString());
    settings.endGroup();
    manifest.insert(QStringLiteral("settings"), mirrored);

    return manifest;
}

void MobileSyncServer::rebuildStationIndexIfStale()
{
    if (m_stationIndexBuiltAt.isValid()
        && m_stationIndexBuiltAt.secsTo(QDateTime::currentDateTime()) < kIndexTtlSeconds) {
        return;
    }
    buildStationManifest();   // building it is what fills the index
}

QString MobileSyncServer::stationPathForId(const QString &id)
{
    rebuildStationIndexIfStale();
    const QString path = m_stationIndex.value(id);
    if (path.isEmpty())
        return QString();
    return QFile::exists(path) ? path : QString();
}

void MobileSyncServer::handleStationManifest(QTcpSocket *socket)
{
    const QJsonObject manifest = buildStationManifest();
    sendJson(socket, QJsonDocument(manifest).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleStationFile(QTcpSocket *socket, const Request &request)
{
    const QString id = request.query.value(QStringLiteral("id"));
    const QString path = stationPathForId(id);
    if (path.isEmpty()) {
        sendError(socket, 404, tr("This station holds no such file."));
        return;
    }
    sendFile(socket, path, request);
}

void MobileSyncServer::handleStationPlaylist(QTcpSocket *socket, const Request &request)
{
    const QString name = request.query.value(QStringLiteral("name"));
    // The name addresses a file in one known folder and nothing else: anything
    // that could climb out of it is not a playlist name.
    if (name.isEmpty() || name.contains(QLatin1Char('/'))
        || name.contains(QLatin1Char('\\')) || name.contains(QLatin1String(".."))) {
        sendError(socket, 400, tr("That is not a playlist name."));
        return;
    }
    const QString path = QDir(playlistsDirectory()).filePath(name + QStringLiteral(".xml"));
    if (!QFile::exists(path)) {
        sendError(socket, 404, tr("No playlist by that name."));
        return;
    }
    sendFile(socket, path, request);
}

void MobileSyncServer::sendJson(QTcpSocket *socket, const QByteArray &json, int status)
{
    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(status) + ' '
                + reasonPhrase(status).toUtf8() + "\r\n";
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "Connection: close\r\n\r\n";
    response += json;

    socket->write(response);
    socket->disconnectFromHost();
}

void MobileSyncServer::sendError(QTcpSocket *socket, int status, const QString &message)
{
    QJsonObject object;
    object.insert(QStringLiteral("error"), message);
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact), status);
}

void MobileSyncServer::sendFile(QTcpSocket *socket, const QString &path, const Request &request)
{
    auto *file = new QFile(path, socket); // dies with the connection
    if (!file->open(QIODevice::ReadOnly)) {
        sendError(socket, 404, tr("That track could not be opened."));
        return;
    }

    const qint64 total = file->size();
    qint64 start = 0;
    qint64 end = total > 0 ? total - 1 : 0;
    bool partial = false;

    // Range support is what lets the phone resume an interrupted download.
    const QByteArray range = request.headers.value("range");
    if (range.startsWith("bytes=")) {
        const QByteArray spec = range.mid(6).trimmed();
        const int dash = spec.indexOf('-');
        if (dash >= 0) {
            bool startOk = false;
            bool endOk = false;
            const qint64 rangeStart = spec.left(dash).toLongLong(&startOk);
            const qint64 rangeEnd = spec.mid(dash + 1).toLongLong(&endOk);

            if (startOk)
                start = rangeStart;
            if (endOk && rangeEnd >= start)
                end = qMin(rangeEnd, total - 1);

            if (start >= total) {
                QByteArray response;
                response += "HTTP/1.1 416 " + reasonPhrase(416).toUtf8() + "\r\n";
                response += "Content-Range: bytes */" + QByteArray::number(total) + "\r\n";
                response += "Content-Length: 0\r\n";
                response += "Connection: close\r\n\r\n";
                socket->write(response);
                socket->disconnectFromHost();
                return;
            }
            partial = true;
        }
    }

    const qint64 length = (total == 0) ? 0 : (end - start + 1);
    file->seek(start);

    const QString contentType =
        QMimeDatabase().mimeTypeForFile(path).name();

    QByteArray header;
    header += partial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: " + contentType.toUtf8() + "\r\n";
    header += "Content-Length: " + QByteArray::number(length) + "\r\n";
    header += "Accept-Ranges: bytes\r\n";
    if (partial) {
        header += "Content-Range: bytes " + QByteArray::number(start) + '-'
                  + QByteArray::number(end) + '/' + QByteArray::number(total) + "\r\n";
    }
    header += "Connection: close\r\n\r\n";
    socket->write(header);

    // Audio files are far too big to hand to write() in one go, so the body is
    // pumped out behind bytesWritten with a bounded amount in flight.
    auto remaining = QSharedPointer<qint64>::create(length);
    auto pump = [socket, file, remaining]() {
        while (*remaining > 0 && socket->bytesToWrite() < kHighWaterBytes) {
            const QByteArray data = file->read(qMin(kChunkBytes, *remaining));
            if (data.isEmpty()) {
                *remaining = 0;
                break;
            }
            socket->write(data);
            *remaining -= data.size();
        }
        if (*remaining <= 0)
            socket->disconnectFromHost();
    };

    connect(socket, &QTcpSocket::bytesWritten, socket, [pump](qint64) { pump(); });
    pump();
}

// -------------------------------------------- taking work from a production PC
//
// The other half of the same idea as the backup station, pointed the other way.
// A station of any size prepares more than it plays: next week's music is
// tagged, this month's ads are cut, a jingle is re-recorded — and none of that
// wants doing on the machine that is on air, where a stray click is heard by
// everybody. So a second XFB does the preparing, reads this one's catalogue to
// know what is already there, and publishes what it made back here.
//
// Everything below is a write, which is why it is fenced off behind a role of
// its own: only a machine the operator deliberately paired from the Production
// Computers dialog can reach any of it. What arrives is still not trusted —
// the category has to be one this station keeps, the relative path has to be
// one that cannot climb out of its folder, the table has to be one of the
// station's own, and every column name is matched against the schema before it
// is allowed anywhere near a statement.

QString MobileSyncServer::ensureCategoryRoot(const QString &category)
{
    QString root = categoryRoot(category).trimmed();
    if (!root.isEmpty()) {
        QDir().mkpath(root);
        return root;
    }

    // Ads have never had a folder of their own, and a station whose operator
    // has not set one cannot be told "no" here — the ad has already been made.
    // So one is chosen, created, and written down where the rest of XFB looks,
    // which also keeps the manifest's relative paths agreeing with reality.
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    root = QDir(base).filePath(QStringLiteral("media/") + category);
    QDir().mkpath(root);

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + QStringLiteral("/xfb.conf"), QSettings::IniFormat);
    if (category == QLatin1String("musics"))
        settings.setValue(QStringLiteral("MusicPath"), root);
    else if (category == QLatin1String("jingles"))
        settings.setValue(QStringLiteral("JinglePath"), root);
    else if (category == QLatin1String("programs"))
        settings.setValue(QStringLiteral("ProgramsPath"), root);
    else if (category == QLatin1String("pub"))
        settings.setValue(QStringLiteral("StationSync/PubPath"), root);
    settings.sync();

    return root;
}

bool MobileSyncServer::allowsMediaDeletion()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + QStringLiteral("/xfb.conf"), QSettings::IniFormat);
    return settings.value(QStringLiteral("ProductionSync/AllowMediaDeletion"), false).toBool();
}

void MobileSyncServer::setAllowMediaDeletion(bool allow)
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + QStringLiteral("/xfb.conf"), QSettings::IniFormat);
    settings.setValue(QStringLiteral("ProductionSync/AllowMediaDeletion"), allow);
    settings.sync();
}

QString MobileSyncServer::incomingPath(const QString &category, const QString &relative)
{
    if (!mediaCategories().contains(category) || !isSafeRelativePath(relative))
        return QString();
    const QString root = ensureCategoryRoot(category);
    if (root.isEmpty())
        return QString();
    const QString target = QDir::cleanPath(QDir(root).filePath(relative));
    // cleanPath resolves what is left of any trickery; the result still has to
    // sit inside the folder it was meant for.
    if (!target.startsWith(QDir::cleanPath(root) + QLatin1Char('/')))
        return QString();
    return target;
}

void MobileSyncServer::handleProductionHello(QTcpSocket *socket)
{
    QJsonObject object;
    object.insert(QStringLiteral("application"), QStringLiteral("XFB"));
    object.insert(QStringLiteral("protocol"), protocolVersion());
    object.insert(QStringLiteral("station"), QSysInfo::machineHostName());

    // Which categories this station can actually take delivery of, so the
    // production end can say so plainly instead of failing file by file.
    QJsonArray categories;
    for (const QString &category : mediaCategories())
        categories.append(category);
    object.insert(QStringLiteral("categories"), categories);

    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleProductionHave(QTcpSocket *socket, const Request &request)
{
    const QString category = request.query.value(QStringLiteral("category"));
    const QString relative = request.query.value(QStringLiteral("rel"));
    const QString target = incomingPath(category, relative);
    if (target.isEmpty()) {
        sendError(socket, 400, tr("That is not a file this station can hold."));
        return;
    }

    QJsonObject object;
    const QFileInfo whole(target);
    object.insert(QStringLiteral("bytes"), whole.exists() ? whole.size() : 0);
    // What of a previous attempt is still on disk decides where the next one
    // starts, so an upload interrupted at 90% costs 10%, not the lot.
    const QFileInfo part(target + kPartSuffix);
    object.insert(QStringLiteral("partial"), part.exists() ? part.size() : 0);
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleProductionFile(QTcpSocket *socket, const Request &request)
{
    const QString category = request.query.value(QStringLiteral("category"));
    const QString relative = request.query.value(QStringLiteral("rel"));
    const QString target = incomingPath(category, relative);
    if (target.isEmpty()) {
        sendError(socket, 400, tr("That is not a file this station can hold."));
        return;
    }

    bool offsetOk = false;
    bool totalOk = false;
    const qint64 offset = request.query.value(QStringLiteral("offset")).toLongLong(&offsetOk);
    const qint64 total  = request.query.value(QStringLiteral("total")).toLongLong(&totalOk);
    if (!offsetOk || !totalOk || offset < 0 || total <= 0 || offset > total) {
        sendError(socket, 400, tr("The upload did not say where it was up to."));
        return;
    }

    const QString partPath = target + kPartSuffix;
    QDir().mkpath(QFileInfo(target).absolutePath());

    // The offset the sender believes it is at has to match what is here, or
    // two attempts at the same file would interleave into a corrupt one. A
    // mismatch is answered with the truth rather than an error, so the sender
    // can simply carry on from the right place.
    const qint64 have = QFileInfo(partPath).exists() ? QFileInfo(partPath).size() : 0;
    if (offset != have) {
        QJsonObject object;
        object.insert(QStringLiteral("received"), have);
        object.insert(QStringLiteral("complete"), false);
        sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact), 409);
        return;
    }

    QFile part(partPath);
    if (!part.open(offset > 0 ? QIODevice::Append : QIODevice::WriteOnly)) {
        sendError(socket, 500, tr("This station could not write that file."));
        return;
    }
    if (offset == 0)
        part.resize(0);
    const qint64 written = part.write(request.body);
    part.close();
    if (written != request.body.size()) {
        sendError(socket, 500, tr("This station ran out of room for that file."));
        return;
    }

    const qint64 received = offset + written;
    const bool complete = received >= total;
    if (complete) {
        QFile::remove(target);
        if (!QFile::rename(partPath, target)) {
            sendError(socket, 500, tr("This station could not store that file."));
            return;
        }
        // The allow-lists are built from what the database says; a file that
        // has just arrived is not in them yet.
        m_stationIndexBuiltAt = QDateTime();
        m_indexBuiltAt = QDateTime();
    }

    QJsonObject object;
    object.insert(QStringLiteral("received"), received);
    object.insert(QStringLiteral("complete"), complete);
    if (complete)
        object.insert(QStringLiteral("id"), idForPath(target));
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleProductionRows(QTcpSocket *socket, const Request &request,
                                            const QString &device)
{
    const QJsonObject body = QJsonDocument::fromJson(request.body).object();
    const QString table = body.value(QStringLiteral("table")).toString();
    const QString mode  = body.value(QStringLiteral("mode")).toString();
    const QJsonArray rows = body.value(QStringLiteral("rows")).toArray();

    // A table name cannot be bound as a parameter, so it is never taken from
    // the request: it is matched against the list of tables this station has
    // agreed to accept, and the matching entry is what reaches the statement.
    const bool media = mediaCategories().contains(table);
    const QStringList configTables = {
        QStringLiteral("genres1"), QStringLiteral("genres2"),
        QStringLiteral("hourgenre"), QStringLiteral("hourprograms"),
        QStringLiteral("scheduler"),
    };
    if (!media && !configTables.contains(table)) {
        sendError(socket, 400, tr("This station does not take entries for \"%1\".").arg(table));
        return;
    }

    QSqlDatabase db = libraryDatabase();
    if (!db.isValid() || !db.isOpen()) {
        sendError(socket, 503, tr("This station's library is not open."));
        return;
    }

    QStringList known;
    const QSqlRecord record = db.record(table);
    for (int i = 0; i < record.count(); ++i)
        known << record.fieldName(i);
    if (known.isEmpty()) {
        sendError(socket, 400, tr("This station has no \"%1\" table.").arg(table));
        return;
    }

    // Two XFBs number their own rows, so an ad prepared on the production
    // machine may carry an id this station already gave to something else.
    // Where that happens the id is left off and SQLite assigns one here: the
    // entry belongs to this station now, and its numbering is this station's
    // business. Media rows are identified by their file, never by their id.
    QString keyColumn;
    const QSqlIndex primary = db.primaryIndex(table);
    if (primary.count() == 1)
        keyColumn = primary.fieldName(0);

    int written = 0;
    int skipped = 0;
    int removed = 0;
    int filesRemoved = 0;
    QJsonArray missing;

    db.transaction();

    // An ad campaign that ended, a jingle withdrawn, a track pulled: what the
    // production machine took out has to come out here too, or the station
    // keeps playing it. The entry always goes; whether its audio goes with it
    // is the station operator's decision and nobody else's, so it is read
    // here rather than taken from the request.
    const bool deleteFiles = media && allowsMediaDeletion();
    for (const QJsonValue &value : body.value(QStringLiteral("remove")).toArray()) {
        if (!media)
            break;   // configuration tables travel whole; there is nothing to remove
        const QString path = incomingPath(table, value.toString());
        if (path.isEmpty())
            continue;
        QSqlQuery drop(db);
        drop.prepare(QStringLiteral("DELETE FROM \"%1\" WHERE path = ?").arg(table));
        drop.addBindValue(path);
        if (!drop.exec())
            continue;
        removed += drop.numRowsAffected() > 0 ? drop.numRowsAffected() : 0;

        if (!deleteFiles)
            continue;
        // Only ever a file inside one of this station's own media folders —
        // incomingPath() has already established that — and only one nothing
        // else still plays. A jingle used as an ad as well keeps its file.
        bool stillListed = false;
        for (const QString &other : mediaCategories()) {
            QSqlQuery used(db);
            used.prepare(QStringLiteral("SELECT 1 FROM \"%1\" WHERE path = ?").arg(other));
            used.addBindValue(path);
            if (used.exec() && used.next()) {
                stillListed = true;
                break;
            }
        }
        if (!stillListed && QFile::remove(path))
            ++filesRemoved;
    }

    // The schedule is a shape, not a set of rows: an hour that was cleared on
    // the production machine has to end up cleared here too, and there is no
    // row left to carry that news. So those tables travel whole.
    if (!media && mode == QLatin1String("replace")) {
        QSqlQuery clear(db);
        if (!clear.exec(QStringLiteral("DELETE FROM \"%1\"").arg(table))) {
            db.rollback();
            sendError(socket, 500, tr("This station could not clear \"%1\".").arg(table));
            return;
        }
    }

    for (const QJsonValue &value : rows) {
        const QJsonObject row = value.toObject();

        QString path;
        if (media) {
            const QJsonObject file = row.value(QStringLiteral("file")).toObject();
            path = incomingPath(table, file.value(QStringLiteral("rel")).toString());
            if (path.isEmpty() || !QFile::exists(path)) {
                // The row is refused rather than written: an entry whose file
                // never arrived is an entry Auto Mode would play as silence.
                missing.append(file.value(QStringLiteral("rel")).toString());
                ++skipped;
                continue;
            }
        }

        QStringList columns;
        QVariantList values;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (it.key() == QLatin1String("file"))
                continue;
            if (it.key().compare(QLatin1String("path"), Qt::CaseInsensitive) == 0)
                continue;   // the path here is this station's, never the sender's
            if (!known.contains(it.key(), Qt::CaseInsensitive))
                continue;
            columns << it.key();
            values << (it.value().isNull() ? QVariant() : QVariant(it.value().toVariant()));
        }

        if (media) {
            // Publishing the same track twice must update it, not double it.
            QSqlQuery replace(db);
            replace.prepare(QStringLiteral("DELETE FROM \"%1\" WHERE path = ?").arg(table));
            replace.addBindValue(path);
            replace.exec();

            int keyAt = -1;
            for (int i = 0; !keyColumn.isEmpty() && i < columns.size(); ++i) {
                if (columns.at(i).compare(keyColumn, Qt::CaseInsensitive) == 0) {
                    keyAt = i;
                    break;
                }
            }
            if (keyAt >= 0) {
                QSqlQuery clash(db);
                clash.prepare(QStringLiteral("SELECT 1 FROM \"%1\" WHERE \"%2\" = ?")
                                  .arg(table, keyColumn));
                clash.addBindValue(values.at(keyAt));
                if (clash.exec() && clash.next()) {
                    columns.removeAt(keyAt);
                    values.removeAt(keyAt);
                }
            }

            columns << QStringLiteral("path");
            values << path;
        }

        if (columns.isEmpty()) {
            ++skipped;
            continue;
        }

        QStringList quoted;
        QStringList placeholders;
        for (const QString &column : std::as_const(columns)) {
            quoted << QLatin1Char('"') + column + QLatin1Char('"');
            placeholders << QStringLiteral("?");
        }

        QSqlQuery insert(db);
        insert.prepare(QStringLiteral("INSERT INTO \"%1\" (%2) VALUES (%3)")
                           .arg(table, quoted.join(QStringLiteral(", ")),
                                placeholders.join(QStringLiteral(", "))));
        for (const QVariant &bound : std::as_const(values))
            insert.addBindValue(bound);
        if (insert.exec())
            ++written;
        else
            ++skipped;
    }

    if (!db.commit()) {
        db.rollback();
        sendError(socket, 500, tr("This station could not write those entries."));
        return;
    }

    if (written > 0 || removed > 0) {
        m_stationIndexBuiltAt = QDateTime();
        m_indexBuiltAt = QDateTime();
        QString summary;
        if (written > 0 && removed > 0) {
            summary = tr("%n change(s) to %1", nullptr, written + removed).arg(table);
        } else if (written > 0) {
            summary = tr("%n entry/entries in %1", nullptr, written).arg(table);
        } else {
            summary = tr("the removal of %n entry/entries from %1", nullptr, removed).arg(table);
        }
        emit deviceActivity(device, tr("published %1").arg(summary));
        emit catalogueChangedByPeer(device, summary);
    }

    QJsonObject object;
    object.insert(QStringLiteral("written"), written);
    object.insert(QStringLiteral("skipped"), skipped);
    object.insert(QStringLiteral("removed"), removed);
    object.insert(QStringLiteral("filesRemoved"), filesRemoved);
    object.insert(QStringLiteral("missing"), missing);
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void MobileSyncServer::handleProductionPlaylist(QTcpSocket *socket, const Request &request)
{
    const QString name = request.query.value(QStringLiteral("name")).trimmed();
    // A playlist name becomes a file name here, so it may be a name and
    // nothing else — no folders, no climbing, no surprises.
    if (name.isEmpty() || name.size() > 128
        || name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))
        || name.contains(QLatin1Char(':')) || name.startsWith(QLatin1Char('.'))) {
        sendError(socket, 400, tr("That is not a name this station can save."));
        return;
    }
    if (request.body.isEmpty()) {
        sendError(socket, 400, tr("The playlist arrived empty."));
        return;
    }

    const QString dir = playlistsDirectory();
    QDir().mkpath(dir);
    QFile file(QDir(dir).filePath(name + QStringLiteral(".xml")));
    if (!file.open(QIODevice::WriteOnly)) {
        sendError(socket, 500, tr("This station could not save that playlist."));
        return;
    }
    file.write(request.body);
    file.close();

    QJsonObject object;
    object.insert(QStringLiteral("saved"), name);
    sendJson(socket, QJsonDocument(object).toJson(QJsonDocument::Compact));
}
