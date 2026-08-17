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
#include <QXmlStreamReader>

namespace {

/** How long a pairing code stays valid. */
constexpr int kPairingWindowSeconds = 180;

/** Wrong codes tolerated before the window slams shut. */
constexpr int kMaxPairingAttempts = 5;

/** Requests larger than this are refused outright (no legitimate one is big). */
constexpr int kMaxRequestBytes = 64 * 1024;

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
    case 413: return QStringLiteral("Payload Too Large");
    case 416: return QStringLiteral("Range Not Satisfiable");
    case 429: return QStringLiteral("Too Many Requests");
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

QString MobileSyncServer::beginPairing()
{
    // Six digits, zero-padded, from the system CSPRNG.
    const quint32 value = QRandomGenerator::system()->bounded(1000000);
    m_pairingCode = QStringLiteral("%1").arg(value, 6, 10, QLatin1Char('0'));
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

    if (it->size() > kMaxRequestBytes) {
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

    const QString device = authenticate(request);
    if (device.isEmpty()) {
        sendError(socket, 401, tr("Not paired with this XFB."));
        return;
    }

    if (request.method != "GET") {
        sendError(socket, 405, tr("Only GET is supported here."));
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
    } else {
        sendError(socket, 404, tr("No such endpoint."));
    }
}

QString MobileSyncServer::authenticate(const Request &request)
{
    QByteArray header = request.headers.value("authorization");
    if (!header.startsWith("Bearer "))
        return QString();

    const QString token = QString::fromUtf8(header.mid(7)).trimmed();
    auto it = m_tokens.find(token);
    if (it == m_tokens.end())
        return QString();

    it->lastSeen = QDateTime::currentDateTime();
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
    if (deviceName.isEmpty())
        deviceName = tr("Android device");
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
    m_tokens.insert(token, device);
    saveTokens();

    // One code, one device.
    endPairing();

    QJsonObject object;
    object.insert(QStringLiteral("token"), token);
    object.insert(QStringLiteral("protocol"), protocolVersion());
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
    }

    QString body;
    if (matches) {
        const QString deepLink = pairingDeepLink(host).toHtmlEscaped();
        body = QStringLiteral(
            "<p class=\"lead\">Open this in the XFB app to finish pairing.</p>"
            "<p class=\"code\">%1</p>"
            "<p><a class=\"button\" href=\"%2\">Open in XFB</a></p>"
            "<p class=\"note\">Nothing happens when you tap that? The XFB app "
            "is not installed on this phone yet &mdash; install it below, then "
            "scan the code again, or type the six digits into the app by hand.</p>"
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
        "</style></head><body><h1>Pair with XFB</h1>%1</body></html>")
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

// ----------------------------------------------------------- track index

QString MobileSyncServer::idForPath(const QString &path)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256)
            .toHex().left(32));
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
