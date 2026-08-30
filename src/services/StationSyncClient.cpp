#include "StationSyncClient.h"

#include "MobileSyncServer.h"
#include "../secretstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>

#include <utility>

namespace {

/** Where XFB's own settings live (the Options dialog writes this file). */
QString configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/**
 * XFB opens its library on a *named* connection, so asking for the default one
 * finds nothing and every write silently goes nowhere.
 */
QSqlDatabase libraryDatabase()
{
    QSqlDatabase named = QSqlDatabase::database(QStringLiteral("xfb_connection"), false);
    if (named.isValid() && named.isOpen())
        return named;
    return QSqlDatabase::database();
}

/** Partly-fetched files are parked under this suffix until they are whole. */
const QString kPartSuffix = QStringLiteral(".xfbpart");

/** Tables whose rows point at a media file the backup has to hold a copy of. */
const QStringList kMediaTables = {
    QStringLiteral("musics"), QStringLiteral("jingles"),
    QStringLiteral("pub"), QStringLiteral("programs"),
};

/** Tables that are pure station configuration — no files, just rows. */
const QStringList kConfigTables = {
    QStringLiteral("genres1"), QStringLiteral("genres2"),
    QStringLiteral("hourgenre"), QStringLiteral("hourprograms"),
    QStringLiteral("scheduler"),
};

QString humanBytes(qint64 bytes)
{
    if (bytes >= 1024LL * 1024 * 1024)
        return QStringLiteral("%1 GB").arg(bytes / double(1024LL * 1024 * 1024), 0, 'f', 1);
    if (bytes >= 1024 * 1024)
        return QStringLiteral("%1 MB").arg(bytes / double(1024 * 1024), 0, 'f', 1);
    if (bytes >= 1024)
        return QStringLiteral("%1 kB").arg(bytes / 1024);
    return QStringLiteral("%1 B").arg(bytes);
}

/**
 * A relative path that arrived over the network decides where a file is
 * written, so it is checked rather than trusted: no absolute paths, no drive
 * letters, no climbing out with "..", nothing empty.
 */
bool safeRelativePath(const QString &relative)
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

} // namespace

StationSyncClient::StationSyncClient(QObject *parent)
    : QObject(parent)
{
    m_net = new QNetworkAccessManager(this);
    m_autoTimer = new QTimer(this);
    m_autoTimer->setSingleShot(false);
    connect(m_autoTimer, &QTimer::timeout, this, &StationSyncClient::sync);

    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setSingleShot(false);
    connect(m_monitorTimer, &QTimer::timeout, this, &StationSyncClient::pollHeartbeat);

    load();

    if (m_autoSyncMinutes > 0)
        m_autoTimer->start(m_autoSyncMinutes * 60 * 1000);
    restartMonitor();
}

StationSyncClient::~StationSyncClient() = default;

QStringList StationSyncClient::categories()
{
    return kMediaTables;
}

// ------------------------------------------------------------------ settings

void StationSyncClient::load()
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("StationSync"));
    m_host = settings.value(QStringLiteral("PeerHost")).toString();
    m_port = quint16(settings.value(QStringLiteral("PeerPort"),
                                    MobileSyncServer::defaultPort()).toUInt());
    m_token = SecretStore::open(settings.value(QStringLiteral("Token")).toString());
    m_peerName = settings.value(QStringLiteral("PeerName")).toString();
    m_autoSyncMinutes = settings.value(QStringLiteral("AutoSyncMinutes"), 0).toInt();
    m_syncOnStart = settings.value(QStringLiteral("SyncOnStart"), false).toBool();
    m_monitorSeconds = settings.value(QStringLiteral("MonitorSeconds"), 0).toInt();
    m_darkAfterSeconds = settings.value(QStringLiteral("DarkAfterSeconds"), 90).toInt();
    if (m_monitorSeconds > 0)
        m_monitorSeconds = qBound(5, m_monitorSeconds, 3600);
    m_darkAfterSeconds = qBound(15, m_darkAfterSeconds, 86400);
    m_lastSync = settings.value(QStringLiteral("LastSync")).toDateTime();
    m_lastResult = settings.value(QStringLiteral("LastResult")).toString();
    for (const QString &category : kMediaTables) {
        const QString value =
            settings.value(QStringLiteral("Root/") + category).toString();
        if (!value.isEmpty())
            m_roots.insert(category, value);
    }
    settings.endGroup();
}

void StationSyncClient::save()
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("StationSync"));
    settings.setValue(QStringLiteral("PeerHost"), m_host);
    settings.setValue(QStringLiteral("PeerPort"), m_port);
    settings.setValue(QStringLiteral("Token"), SecretStore::seal(m_token));
    settings.setValue(QStringLiteral("PeerName"), m_peerName);
    settings.setValue(QStringLiteral("AutoSyncMinutes"), m_autoSyncMinutes);
    settings.setValue(QStringLiteral("SyncOnStart"), m_syncOnStart);
    settings.setValue(QStringLiteral("MonitorSeconds"), m_monitorSeconds);
    settings.setValue(QStringLiteral("DarkAfterSeconds"), m_darkAfterSeconds);
    settings.setValue(QStringLiteral("LastSync"), m_lastSync);
    settings.setValue(QStringLiteral("LastResult"), m_lastResult);
    for (auto it = m_roots.constBegin(); it != m_roots.constEnd(); ++it)
        settings.setValue(QStringLiteral("Root/") + it.key(), it.value());
    settings.endGroup();
    settings.sync();
    // The token lives in here now.
    SecretStore::restrictFile(configPath());
}

void StationSyncClient::setPeer(const QString &host, quint16 port)
{
    m_host = host.trimmed();
    m_port = port ? port : MobileSyncServer::defaultPort();
    save();
    restartMonitor();
}

void StationSyncClient::forgetPeer()
{
    m_token.clear();
    m_peerName.clear();
    save();
    restartMonitor();
}

void StationSyncClient::setAutoSyncMinutes(int minutes)
{
    m_autoSyncMinutes = qMax(0, minutes);
    save();
    if (m_autoSyncMinutes > 0)
        m_autoTimer->start(m_autoSyncMinutes * 60 * 1000);
    else
        m_autoTimer->stop();
}

void StationSyncClient::setSyncOnStart(bool on)
{
    m_syncOnStart = on;
    save();
}

// -------------------------------------------------------- watching the studio

void StationSyncClient::setMonitorSeconds(int seconds)
{
    m_monitorSeconds = seconds > 0 ? qBound(5, seconds, 3600) : 0;
    save();
    restartMonitor();
}

void StationSyncClient::setDarkAfterSeconds(int seconds)
{
    m_darkAfterSeconds = qBound(15, seconds, 86400);
    save();
}

void StationSyncClient::restartMonitor()
{
    if (m_monitorSeconds > 0 && paired() && !m_host.isEmpty()) {
        // Start the clock now rather than at zero: a backup that has just
        // been launched has not yet failed to hear anything.
        if (!m_lastGoodHeartbeat.isValid())
            m_lastGoodHeartbeat = QDateTime::currentDateTime();
        m_monitorTimer->start(m_monitorSeconds * 1000);
        pollHeartbeat();
    } else {
        m_monitorTimer->stop();
        if (m_studioDark) {
            m_studioDark = false;
            emit studioCameBack(tr("Studio monitoring turned off."));
        }
        m_studioState.clear();
    }
}

void StationSyncClient::pollHeartbeat()
{
    if (!paired() || m_host.isEmpty())
        return;
    if (m_heartbeatReply)   // the previous poll is still out; do not stack them
        return;

    QNetworkReply *reply = get(QStringLiteral("/api/station/heartbeat"));
    m_heartbeatReply = reply;

    // A studio that has crashed hard answers nothing at all, so a poll that
    // never returns must not be the thing that keeps the alarm quiet.
    QTimer::singleShot(qMin(10, qMax(3, m_monitorSeconds)) * 1000, reply,
                       [reply]() { if (reply->isRunning()) reply->abort(); });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (m_heartbeatReply == reply)
            m_heartbeatReply = nullptr;

        if (reply->error() != QNetworkReply::NoError) {
            applyHeartbeat(false, QJsonObject(), reply->errorString());
            return;
        }
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        if (!document.isObject()) {
            applyHeartbeat(false, QJsonObject(),
                           tr("the station answered something unreadable"));
            return;
        }
        applyHeartbeat(true, document.object(), QString());
    });
}

void StationSyncClient::applyHeartbeat(bool reachable, const QJsonObject &state,
                                       const QString &error)
{
    const QDateTime now = QDateTime::currentDateTime();

    QString reason;
    bool healthy = false;

    if (!reachable) {
        m_studioState = tr("unreachable");
        reason = tr("the studio machine is not answering (%1)").arg(error);
    } else {
        emit heartbeat(state);
        const QString word = state.value(QStringLiteral("state")).toString();
        m_studioState = word;
        // "fallback" is the studio's own watchdog holding it up: the station
        // is on air, so this is not darkness — but it is worth seeing, which
        // is what the incident fields in the heartbeat are for.
        healthy = (word == QLatin1String("playing")
                   || word == QLatin1String("fallback"));
        if (!healthy) {
            reason = tr("the studio reports \"%1\"")
                         .arg(word.isEmpty() ? tr("nothing") : word);
        }
    }

    if (healthy) {
        m_lastGoodHeartbeat = now;
        if (m_studioDark) {
            m_studioDark = false;
            emit studioCameBack(tr("The studio is making sound again (%1).")
                                    .arg(m_studioState));
        }
        return;
    }

    if (!m_lastGoodHeartbeat.isValid())
        m_lastGoodHeartbeat = now;

    const qint64 darkFor = m_lastGoodHeartbeat.secsTo(now);
    if (!m_studioDark && darkFor >= m_darkAfterSeconds) {
        m_studioDark = true;
        emit studioWentDark(tr("No sound from %1 for %2 seconds — %3.")
                                .arg(m_peerName.isEmpty() ? m_host : m_peerName)
                                .arg(darkFor)
                                .arg(reason));
    }
}

QString StationSyncClient::localRoot(const QString &category) const
{
    const QString chosen = m_roots.value(category);
    if (!chosen.isEmpty())
        return chosen;

    // Saved playlists go where the rest of XFB already looks for them, so a
    // backup that takes over opens the same list from the same menu.
    if (category == QLatin1String("playlists")) {
        QSettings native;
        const QString configured =
            native.value(QStringLiteral("MobileSync/PlaylistsPath")).toString();
        if (!configured.trimmed().isEmpty())
            return configured.trimmed();
        QSettings xfb(configPath(), QSettings::IniFormat);
        const QString savePath = xfb.value(QStringLiteral("SavePath")).toString().trimmed();
        if (!savePath.isEmpty())
            return QDir(savePath).filePath(QStringLiteral("playlists"));
        const QString docs =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        return QDir(docs).filePath(QStringLiteral("XFB/playlists"));
    }

    // A backup normally keeps its media where this XFB already keeps its own,
    // so the operator's existing folders are used when they are configured.
    QSettings settings(configPath(), QSettings::IniFormat);
    QString configured;
    if (category == QLatin1String("musics"))
        configured = settings.value(QStringLiteral("MusicPath")).toString();
    else if (category == QLatin1String("jingles"))
        configured = settings.value(QStringLiteral("JinglePath")).toString();
    else if (category == QLatin1String("programs"))
        configured = settings.value(QStringLiteral("ProgramsPath")).toString();
    if (!configured.trimmed().isEmpty())
        return configured.trimmed();

    // Ads have never had a folder of their own, and neither has an XFB that
    // was set up as a backup and nothing else, so there is a default.
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("station-mirror/") + category);
}

void StationSyncClient::setLocalRoot(const QString &category, const QString &path)
{
    if (path.trimmed().isEmpty())
        m_roots.remove(category);
    else
        m_roots.insert(category, path.trimmed());
    save();
}

// ------------------------------------------------------------------- pairing

QNetworkReply *StationSyncClient::get(const QString &path, qint64 resumeFrom)
{
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(m_host);
    url.setPort(m_port);
    const int question = path.indexOf(QLatin1Char('?'));
    if (question >= 0) {
        url.setPath(path.left(question));
        url.setQuery(path.mid(question + 1));
    } else {
        url.setPath(path);
    }

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    if (resumeFrom > 0)
        request.setRawHeader("Range", "bytes=" + QByteArray::number(resumeFrom) + "-");
    return m_net->get(request);
}

void StationSyncClient::pairWith(const QString &host, quint16 port, const QString &code)
{
    const QString cleanHost = host.trimmed();
    if (cleanHost.isEmpty()) {
        emit pairingFailed(tr("Enter the address of the station to mirror."));
        return;
    }

    m_host = cleanHost;
    m_port = port ? port : MobileSyncServer::defaultPort();

    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(m_host);
    url.setPort(m_port);
    url.setPath(QStringLiteral("/api/pair"));

    QJsonObject body;
    body.insert(QStringLiteral("code"), code.trimmed());
    body.insert(QStringLiteral("device"),
                tr("XFB backup on %1").arg(QSysInfo::machineHostName()));
    // The station decides the role from the window its operator opened; this
    // only says what we came for, so the other end can say no clearly.
    body.insert(QStringLiteral("role"), QStringLiteral("station"));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    QNetworkReply *reply = m_net->post(request,
                                       QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray payload = reply->readAll();
        const QJsonObject answer = QJsonDocument::fromJson(payload).object();

        if (reply->error() != QNetworkReply::NoError) {
            const QString message = answer.value(QStringLiteral("error")).toString();
            emit pairingFailed(message.isEmpty() ? reply->errorString() : message);
            return;
        }
        const QString token = answer.value(QStringLiteral("token")).toString();
        if (token.isEmpty()) {
            emit pairingFailed(tr("The station did not send a token back."));
            return;
        }
        if (answer.value(QStringLiteral("role")).toString() != QLatin1String("station")) {
            // The station's operator opened the phone pairing window instead;
            // that token would be refused by every endpoint we need.
            emit pairingFailed(tr("That code was for pairing a phone. On the main "
                                  "station, open Station Backup and start pairing "
                                  "from there instead."));
            return;
        }

        m_token = token;
        m_peerName = m_host;
        save();
        restartMonitor();   // there is now a station worth listening to
        emit pairingSucceeded(m_peerName);
    });
}

// ---------------------------------------------------------------------- sync

void StationSyncClient::sync()
{
    if (m_busy)
        return;
    if (m_host.isEmpty() || m_token.isEmpty()) {
        emit failed(tr("This XFB is not paired with a station yet."));
        return;
    }

    m_cancelled = false;
    m_manifest = QJsonObject();
    m_pending.clear();
    m_pendingPlaylists.clear();
    m_bytesTotal = m_bytesDone = 0;
    m_filesFetched = m_filesFailed = m_filesAlreadyHere = 0;
    m_playlistsFetched = m_rowsWritten = m_rowsPending = 0;

    setBusy(true);
    setStage(Stage::Connecting, tr("Asking %1 what it holds...").arg(m_host));

    QNetworkReply *reply = get(QStringLiteral("/api/station/manifest"));
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        if (reply->error() != QNetworkReply::NoError) {
            const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();
            const QString message = answer.value(QStringLiteral("error")).toString();
            abortSync(message.isEmpty() ? reply->errorString() : message);
            return;
        }
        onManifest(reply->readAll());
    });
}

void StationSyncClient::cancel()
{
    if (!m_busy)
        return;
    m_cancelled = true;
    if (m_currentReply)
        m_currentReply->abort();
}

void StationSyncClient::onManifest(const QByteArray &body)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        abortSync(tr("The station sent something this XFB could not read."));
        return;
    }

    m_manifest = document.object();
    const QString name = m_manifest.value(QStringLiteral("station")).toString();
    if (!name.isEmpty() && name != m_peerName) {
        m_peerName = name;
        save();
    }

    planFiles(m_manifest);

    const QJsonArray playlists = m_manifest.value(QStringLiteral("playlists")).toArray();
    const QString playlistDir = localRoot(QStringLiteral("playlists"));
    for (const QJsonValue &value : playlists) {
        const QJsonObject entry = value.toObject();
        const QString playlistName = entry.value(QStringLiteral("name")).toString();
        if (playlistName.isEmpty() || !safeRelativePath(playlistName + QStringLiteral(".xml")))
            continue;
        const QFileInfo local(QDir(playlistDir).filePath(playlistName + QStringLiteral(".xml")));
        if (local.exists()
            && local.size() == entry.value(QStringLiteral("bytes")).toInteger()
            && local.lastModified().toSecsSinceEpoch()
                   >= entry.value(QStringLiteral("mtime")).toInteger()) {
            continue;   // already have this one, unchanged
        }
        m_pendingPlaylists << playlistName;
    }

    if (m_pending.isEmpty() && m_pendingPlaylists.isEmpty()) {
        setStage(Stage::Catalogue, tr("Everything is already here; checking the catalogue."));
        applyCatalogue();
        applySettings();
        completeSync();
        return;
    }

    setStage(Stage::Files, tr("%1 to fetch in %2 files")
                               .arg(humanBytes(m_bytesTotal))
                               .arg(m_pending.size()));
    startNextFile();
}

void StationSyncClient::planFiles(const QJsonObject &manifest)
{
    const QJsonObject tables = manifest.value(QStringLiteral("tables")).toObject();

    for (const QString &category : kMediaTables) {
        const QJsonArray rows = tables.value(category).toArray();
        const QString root = localRoot(category);

        for (const QJsonValue &value : rows) {
            const QJsonObject file = value.toObject().value(QStringLiteral("file")).toObject();
            if (file.isEmpty())
                continue;

            FileJob job;
            job.id = file.value(QStringLiteral("id")).toString();
            job.relative = file.value(QStringLiteral("rel")).toString();
            job.bytes = file.value(QStringLiteral("bytes")).toInteger();
            job.modified = file.value(QStringLiteral("mtime")).toInteger();
            job.category = category;
            if (job.id.isEmpty() || !safeRelativePath(job.relative))
                continue;

            job.target = QDir(root).filePath(job.relative);

            const QFileInfo local(job.target);
            if (local.exists() && local.size() == job.bytes && job.bytes > 0) {
                ++m_filesAlreadyHere;
                continue;   // same size: treat it as the same file
            }

            m_bytesTotal += job.bytes;
            m_pending.enqueue(job);
        }
    }
}

void StationSyncClient::startNextFile()
{
    if (m_cancelled) {
        abortSync(tr("Sync stopped."));
        return;
    }
    if (m_pending.isEmpty()) {
        setStage(Stage::Playlists, tr("Fetching playlists..."));
        startNextPlaylist();
        return;
    }

    const FileJob job = m_pending.dequeue();
    QDir().mkpath(QFileInfo(job.target).absolutePath());

    const QString partPath = job.target + kPartSuffix;
    qint64 have = QFileInfo(partPath).size();
    if (have < 0 || have >= job.bytes)
        have = 0;   // a stale or over-long fragment is worth less than the time

    auto *partial = new QFile(partPath, this);
    if (!partial->open(have > 0 ? QIODevice::Append : QIODevice::WriteOnly)) {
        ++m_filesFailed;
        delete partial;
        startNextFile();
        return;
    }
    if (have == 0)
        partial->resize(0);

    QNetworkReply *reply = get(QStringLiteral("/api/station/file?id=") + job.id, have);
    m_currentReply = reply;
    m_bytesDone += have;

    // Whether the station honoured the resume is only knowable once the
    // headers are in: a 200 means it is sending the whole file, so whatever
    // was already on disk has to go rather than be prepended to it.
    auto restarted = QSharedPointer<bool>::create(false);
    connect(reply, &QNetworkReply::readyRead, this,
            [this, reply, partial, restarted, have]() {
        if (!*restarted) {
            *restarted = true;
            const int status =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (have > 0 && status == 200) {
                partial->resize(0);
                partial->seek(0);
                m_bytesDone -= have;
            }
        }
        const QByteArray chunk = reply->readAll();
        partial->write(chunk);
        m_bytesDone += chunk.size();
        reportProgress();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, partial, job]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        partial->close();

        const bool ok = reply->error() == QNetworkReply::NoError;
        if (ok && partial->size() >= job.bytes) {
            QFile::remove(job.target);
            if (!partial->rename(job.target))
                ++m_filesFailed;
            else
                ++m_filesFetched;
        } else if (!m_cancelled) {
            // The fragment stays put: the next run picks up where this left off.
            ++m_filesFailed;
        }
        partial->deleteLater();
        startNextFile();
    });
}

void StationSyncClient::startNextPlaylist()
{
    if (m_cancelled) {
        abortSync(tr("Sync stopped."));
        return;
    }
    if (m_pendingPlaylists.isEmpty()) {
        setStage(Stage::Catalogue, tr("Writing the catalogue..."));
        applyCatalogue();
        applySettings();
        completeSync();
        return;
    }

    const QString name = m_pendingPlaylists.takeFirst();
    const QString dir = localRoot(QStringLiteral("playlists"));
    QDir().mkpath(dir);
    const QString target = QDir(dir).filePath(name + QStringLiteral(".xml"));

    QNetworkReply *reply =
        get(QStringLiteral("/api/station/playlist?name=")
            + QString::fromLatin1(QUrl::toPercentEncoding(name)));
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, target]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(target);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                ++m_playlistsFetched;
            }
        }
        reportProgress();
        startNextPlaylist();
    });
}

// ------------------------------------------------------------ applying it all


// A safety copy of the live database, taken the way WAL requires.
//
// A plain file copy is no longer a whole backup: in WAL mode the newest rows
// sit in the -wal file until a checkpoint folds them in, so the .db on its own
// can be missing exactly the work this copy exists to protect. VACUUM INTO
// writes a complete, consistent database in one step — but it refuses to run
// while any statement is open on the connection, and the library view keeps
// one open for as long as its window is, so it is given a connection of its
// own. If that fails for any reason, a file copy is still better than nothing.
static bool backupDatabaseTo(const QString &dbPath, const QString &backupPath)
{
    QFile::remove(backupPath);

    const QString connectionName =
        QStringLiteral("backup_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool copied = false;
    {
        QSqlDatabase backupDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        backupDb.setDatabaseName(dbPath);
        backupDb.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=15000"));
        if (backupDb.open()) {
            // VACUUM takes no bound parameters, so the path goes in as a
            // quoted literal.
            QString target = backupPath;
            target.replace(QLatin1Char('\''), QLatin1String("''"));
            QSqlQuery vacuum(backupDb);
            copied = vacuum.exec(QStringLiteral("VACUUM INTO '%1'").arg(target));
            if (!copied) {
                qWarning() << "Could not VACUUM the database into" << backupPath
                           << "-" << vacuum.lastError().text();
            }
            backupDb.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);

    if (!copied) {
        QFile::remove(backupPath);
        copied = QFile::copy(dbPath, backupPath);
        if (copied) {
            qWarning() << "Fell back to a plain file copy for" << backupPath
                       << "- it may not carry writes still held in the -wal file.";
        }
    }
    return copied;
}

void StationSyncClient::applyCatalogue()
{
    const QJsonObject tables = m_manifest.value(QStringLiteral("tables")).toObject();
    if (tables.isEmpty())
        return;

    QSqlDatabase db = libraryDatabase();
    if (!db.isValid() || !db.isOpen()) {
        m_lastResult = tr("The library database is not open; the catalogue was not written.");
        return;
    }

    // A mirror replaces what it finds, so the database as it stands is put
    // somewhere safe first. The app's own rotating daily backups are not
    // enough on their own here: a few syncs would roll straight through them.
    const QString dbPath = db.databaseName();
    if (!dbPath.isEmpty() && QFile::exists(dbPath)) {
        const QString backupDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + QStringLiteral("/backups");
        QDir().mkpath(backupDir);
        backupDatabaseTo(dbPath, QDir(backupDir).filePath(
            QStringLiteral("before-station-sync.db")));
    }

    db.transaction();

    auto columnsOf = [&db](const QString &table) {
        QStringList names;
        const QSqlRecord record = db.record(table);
        for (int i = 0; i < record.count(); ++i)
            names << record.fieldName(i);
        return names;
    };

    // Column names arrive over the network, so they are matched against the
    // columns this database actually has before they get anywhere near a
    // statement. Anything unknown is dropped rather than trusted.
    auto writeTable = [&](const QString &table, const QJsonArray &rows,
                          const QString &category) {
        const QStringList known = columnsOf(table);
        if (known.isEmpty())
            return;

        QSqlQuery clear(db);
        if (!clear.exec(QStringLiteral("DELETE FROM \"%1\"").arg(table)))
            return;

        const QString root = localRoot(category);
        for (const QJsonValue &value : rows) {
            const QJsonObject row = value.toObject();

            QStringList columns;
            QVariantList values;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                if (it.key() == QLatin1String("file"))
                    continue;
                if (!known.contains(it.key(), Qt::CaseInsensitive))
                    continue;
                columns << it.key();
                values << (it.value().isNull() ? QVariant()
                                               : QVariant(it.value().toVariant()));
            }

            if (!category.isEmpty()) {
                const QJsonObject file = row.value(QStringLiteral("file")).toObject();
                const QString relative = file.value(QStringLiteral("rel")).toString();
                if (!safeRelativePath(relative))
                    continue;
                const QString localPath = QDir(root).filePath(relative);
                if (!QFile::exists(localPath)) {
                    // The file has not arrived yet. Leaving the row out keeps
                    // the backup honest: what it lists, it can play.
                    ++m_rowsPending;
                    continue;
                }
                columns << QStringLiteral("path");
                values << localPath;
            }

            if (columns.isEmpty())
                continue;

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
                ++m_rowsWritten;
        }
    };

    for (const QString &table : kMediaTables) {
        if (!tables.contains(table))
            continue;   // a manifest that says nothing about a table leaves it alone
        writeTable(table, tables.value(table).toArray(), table);
    }
    for (const QString &table : kConfigTables) {
        if (!tables.contains(table))
            continue;
        writeTable(table, tables.value(table).toArray(), QString());
    }

    if (!db.commit()) {
        db.rollback();
        m_lastResult = tr("The catalogue could not be written: %1")
                           .arg(db.lastError().text());
    }
}

void StationSyncClient::applySettings()
{
    const QJsonObject incoming = m_manifest.value(QStringLiteral("settings")).toObject();
    if (incoming.isEmpty())
        return;

    // The station says what it runs with; this end decides what it is willing
    // to take. Anything outside the shared list is ignored, so a station
    // cannot reach across and repoint this machine's folders or credentials.
    const QStringList allowed = MobileSyncServer::mirroredSettingKeys();

    QSettings settings(configPath(), QSettings::IniFormat);
    for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
        const QString key = it.key();
        const bool isFx = key.startsWith(QLatin1String("Fx/"))
                          && !key.contains(QLatin1String(".."));
        if (!isFx && !allowed.contains(key))
            continue;
        settings.setValue(key, it.value().toString());
    }
    settings.sync();
}

void StationSyncClient::completeSync()
{
    m_lastSync = QDateTime::currentDateTime();

    QStringList parts;
    if (m_filesFetched > 0)
        parts << tr("%n file(s) copied", nullptr, m_filesFetched);
    if (m_filesAlreadyHere > 0)
        parts << tr("%n already here", nullptr, m_filesAlreadyHere);
    if (m_playlistsFetched > 0)
        parts << tr("%n playlist(s)", nullptr, m_playlistsFetched);
    if (m_rowsWritten > 0)
        parts << tr("%n catalogue row(s)", nullptr, m_rowsWritten);
    if (m_rowsPending > 0)
        parts << tr("%n entry/entries still waiting for its file", nullptr, m_rowsPending);
    if (m_filesFailed > 0)
        parts << tr("%n file(s) failed — run the sync again to pick them up",
                    nullptr, m_filesFailed);
    if (parts.isEmpty())
        parts << tr("nothing had changed");

    m_lastResult = parts.join(QStringLiteral(", "));
    save();

    setStage(Stage::Done, m_lastResult);
    setBusy(false);
    emit finished(m_lastResult);
}

void StationSyncClient::abortSync(const QString &reason)
{
    m_lastResult = reason;
    save();
    setStage(Stage::Idle, reason);
    setBusy(false);
    emit failed(reason);
}

// ------------------------------------------------------------------ progress

void StationSyncClient::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged(busy);
}

void StationSyncClient::setStage(Stage stage, const QString &description)
{
    m_stage = stage;
    emit stageChanged(stage, description);
    emit progress(m_bytesTotal > 0
                      ? int(qMin<qint64>(100, m_bytesDone * 100 / m_bytesTotal))
                      : -1,
                  description);
}

void StationSyncClient::reportProgress(const QString &message)
{
    const int percent = m_bytesTotal > 0
                            ? int(qMin<qint64>(100, m_bytesDone * 100 / m_bytesTotal))
                            : -1;
    emit progress(percent, message.isEmpty()
                               ? tr("%1 of %2").arg(humanBytes(m_bytesDone),
                                                    humanBytes(m_bytesTotal))
                               : message);
}
