#include "ProductionSyncClient.h"

#include "MobileSyncServer.h"
#include "../secretstore.h"

#include <QCryptographicHash>
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
#include <QSqlIndex>
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

/** Tables whose rows point at a media file. */
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

/** What each media table is called in the operator's language. */
QString tableTitle(const QString &table)
{
    if (table == QLatin1String("musics"))
        return ProductionSyncClient::tr("music");
    if (table == QLatin1String("jingles"))
        return ProductionSyncClient::tr("jingles");
    if (table == QLatin1String("pub"))
        return ProductionSyncClient::tr("ads");
    if (table == QLatin1String("programs"))
        return ProductionSyncClient::tr("programs");
    if (table == QLatin1String("hourgenre"))
        return ProductionSyncClient::tr("the hour schedule");
    if (table == QLatin1String("hourprograms"))
        return ProductionSyncClient::tr("the programme schedule");
    if (table == QLatin1String("scheduler"))
        return ProductionSyncClient::tr("the scheduler");
    if (table == QLatin1String("genres1") || table == QLatin1String("genres2"))
        return ProductionSyncClient::tr("the genre lists");
    return table;
}

/** A chunk of an upload. Big enough to be quick, small enough to resume. */
constexpr qint64 kUploadChunkBytes = 1024 * 1024;

/** How many rows travel in one request. */
constexpr int kRowBatch = 200;

/** How many of a category's files a shared-folder check opens. */
constexpr int kShareCheckSample = 20;

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

/** The table's own key column, where it has exactly one. */
QString keyColumnOf(QSqlDatabase &db, const QString &table)
{
    const QSqlIndex primary = db.primaryIndex(table);
    return primary.count() == 1 ? primary.fieldName(0) : QString();
}

/** The columns a table actually has here, so nothing else reaches a statement. */
QStringList columnsOf(QSqlDatabase &db, const QString &table)
{
    QStringList names;
    const QSqlRecord record = db.record(table);
    for (int i = 0; i < record.count(); ++i)
        names << record.fieldName(i);
    return names;
}

} // namespace

ProductionSyncClient::ProductionSyncClient(QObject *parent)
    : QObject(parent)
{
    m_net = new QNetworkAccessManager(this);
    m_autoTimer = new QTimer(this);
    m_autoTimer->setSingleShot(false);
    // An unattended run fetches what the station has; whether it also sends
    // this machine's work up is the operator's call, because publishing is the
    // half that changes what goes on air.
    connect(m_autoTimer, &QTimer::timeout, this, [this]() {
        if (m_publishAutomatically)
            sync();
        else
            pull();
    });

    load();
    loadBaseline();

    if (m_autoSyncMinutes > 0)
        m_autoTimer->start(m_autoSyncMinutes * 60 * 1000);
}

ProductionSyncClient::~ProductionSyncClient() = default;

// ------------------------------------------------------------------ settings

void ProductionSyncClient::load()
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("ProductionSync"));
    m_host = settings.value(QStringLiteral("PeerHost")).toString();
    m_port = quint16(settings.value(QStringLiteral("PeerPort"),
                                    MobileSyncServer::defaultPort()).toUInt());
    m_token = SecretStore::open(settings.value(QStringLiteral("Token")).toString());
    m_peerName = settings.value(QStringLiteral("PeerName")).toString();
    m_autoSyncMinutes = settings.value(QStringLiteral("AutoSyncMinutes"), 0).toInt();
    m_syncOnStart = settings.value(QStringLiteral("SyncOnStart"), false).toBool();
    m_publishAutomatically =
        settings.value(QStringLiteral("PublishAutomatically"), true).toBool();
    // Shared unless this machine was set up before there was a choice: an
    // existing production computer already holds a copy of the library and
    // has a baseline built around it, and quietly switching it to the share
    // would strand all of that.
    const QVariant storage = settings.value(QStringLiteral("MediaStorage"));
    m_mediaStorage = storage.toString().compare(QLatin1String("local"), Qt::CaseInsensitive) == 0
                         ? MediaStorage::LocalCopy
                         : MediaStorage::Shared;
    m_needsPull = settings.value(QStringLiteral("NeedsPull"), false).toBool();
    m_lastSync = settings.value(QStringLiteral("LastSync")).toDateTime();
    m_lastResult = settings.value(QStringLiteral("LastResult")).toString();
    for (const QString &category : kMediaTables) {
        const QString value = settings.value(QStringLiteral("Root/") + category).toString();
        if (!value.isEmpty())
            m_roots.insert(category, value);
        const QString there =
            settings.value(QStringLiteral("StationRoot/") + category).toString();
        if (!there.isEmpty())
            m_stationRoots.insert(category, there);
    }
    settings.endGroup();
}

void ProductionSyncClient::save()
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("ProductionSync"));
    settings.setValue(QStringLiteral("PeerHost"), m_host);
    settings.setValue(QStringLiteral("PeerPort"), m_port);
    settings.setValue(QStringLiteral("Token"), SecretStore::seal(m_token));
    settings.setValue(QStringLiteral("PeerName"), m_peerName);
    settings.setValue(QStringLiteral("AutoSyncMinutes"), m_autoSyncMinutes);
    settings.setValue(QStringLiteral("SyncOnStart"), m_syncOnStart);
    settings.setValue(QStringLiteral("PublishAutomatically"), m_publishAutomatically);
    settings.setValue(QStringLiteral("NeedsPull"), m_needsPull);
    settings.setValue(QStringLiteral("LastSync"), m_lastSync);
    settings.setValue(QStringLiteral("LastResult"), m_lastResult);
    settings.setValue(QStringLiteral("MediaStorage"),
                      m_mediaStorage == MediaStorage::LocalCopy
                          ? QStringLiteral("local")
                          : QStringLiteral("shared"));
    for (auto it = m_roots.constBegin(); it != m_roots.constEnd(); ++it)
        settings.setValue(QStringLiteral("Root/") + it.key(), it.value());
    for (auto it = m_stationRoots.constBegin(); it != m_stationRoots.constEnd(); ++it)
        settings.setValue(QStringLiteral("StationRoot/") + it.key(), it.value());
    settings.endGroup();
    settings.sync();
    // The token lives in here now.
    SecretStore::restrictFile(configPath());
}

// ------------------------------------------------------- what was last agreed

// The baseline is the only thing standing between "this is new work" and "this
// is what the station sent me last time", so it is kept next to the database
// rather than in the settings file: it is data, it can run to thousands of
// lines, and it has no business being edited by hand.
static QString baselinePath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("production-baseline.json"));
}

void ProductionSyncClient::loadBaseline()
{
    m_baselineRows.clear();
    m_baselineTables.clear();
    m_baselinePlaylists.clear();

    QFile file(baselinePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    // A baseline taken against another station says nothing about this one,
    // and treating it as if it did would silently withhold work from the new
    // station. Better to start from nothing and publish a little too much.
    if (root.value(QStringLiteral("peer")).toString() != m_host)
        return;

    const QJsonObject media = root.value(QStringLiteral("media")).toObject();
    for (auto table = media.constBegin(); table != media.constEnd(); ++table) {
        const QJsonObject rows = table.value().toObject();
        QHash<QString, QString> hashes;
        for (auto row = rows.constBegin(); row != rows.constEnd(); ++row)
            hashes.insert(row.key(), row.value().toString());
        m_baselineRows.insert(table.key(), hashes);
    }

    const QJsonObject tables = root.value(QStringLiteral("config")).toObject();
    for (auto it = tables.constBegin(); it != tables.constEnd(); ++it)
        m_baselineTables.insert(it.key(), it.value().toString());

    const QJsonObject playlists = root.value(QStringLiteral("playlists")).toObject();
    for (auto it = playlists.constBegin(); it != playlists.constEnd(); ++it)
        m_baselinePlaylists.insert(it.key(), it.value().toString());
}

void ProductionSyncClient::saveBaseline()
{
    QJsonObject root;
    root.insert(QStringLiteral("peer"), m_host);

    QJsonObject media;
    for (auto table = m_baselineRows.constBegin(); table != m_baselineRows.constEnd(); ++table) {
        QJsonObject rows;
        for (auto row = table.value().constBegin(); row != table.value().constEnd(); ++row)
            rows.insert(row.key(), row.value());
        media.insert(table.key(), rows);
    }
    root.insert(QStringLiteral("media"), media);

    QJsonObject tables;
    for (auto it = m_baselineTables.constBegin(); it != m_baselineTables.constEnd(); ++it)
        tables.insert(it.key(), it.value());
    root.insert(QStringLiteral("config"), tables);

    QJsonObject playlists;
    for (auto it = m_baselinePlaylists.constBegin(); it != m_baselinePlaylists.constEnd(); ++it)
        playlists.insert(it.key(), it.value());
    root.insert(QStringLiteral("playlists"), playlists);

    QFile file(baselinePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.close();
}

// -------------------------------------------------------------------- the peer

void ProductionSyncClient::setPeer(const QString &host, quint16 port)
{
    m_host = host.trimmed();
    m_port = port ? port : MobileSyncServer::defaultPort();
    save();
}

void ProductionSyncClient::forgetPeer()
{
    m_token.clear();
    m_peerName.clear();
    save();
}

void ProductionSyncClient::setAutoSyncMinutes(int minutes)
{
    m_autoSyncMinutes = qMax(0, minutes);
    save();
    if (m_autoSyncMinutes > 0)
        m_autoTimer->start(m_autoSyncMinutes * 60 * 1000);
    else
        m_autoTimer->stop();
}

void ProductionSyncClient::setSyncOnStart(bool on)
{
    m_syncOnStart = on;
    save();
}

void ProductionSyncClient::setMediaStorage(MediaStorage storage)
{
    if (m_mediaStorage == storage)
        return;
    m_mediaStorage = storage;
    save();

    // The two arrangements disagree about where every file is, so what the two
    // ends last agreed on under the old one is worth nothing under the new.
    // Dropping it makes the next run look at the station afresh; the merge
    // rules then keep whatever this machine holds that the station does not.
    m_baselineRows.clear();
    m_baselineTables.clear();
    m_baselinePlaylists.clear();
    saveBaseline();
    m_needsPull = true;
    save();

    emit mediaStorageChanged(m_mediaStorage);
    refreshPendingChanges();
}

void ProductionSyncClient::setPublishAutomatically(bool on)
{
    m_publishAutomatically = on;
    save();
}

QString ProductionSyncClient::localRoot(const QString &category) const
{
    const QString chosen = m_roots.value(category);
    if (!chosen.isEmpty())
        return chosen;

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

    // A production machine keeps its media where this XFB already keeps its
    // own: the operator adds tracks here with the ordinary Add Music dialogs,
    // and those write to these very folders.
    QSettings settings(configPath(), QSettings::IniFormat);
    QString configured;
    if (category == QLatin1String("musics"))
        configured = settings.value(QStringLiteral("MusicPath")).toString();
    else if (category == QLatin1String("jingles"))
        configured = settings.value(QStringLiteral("JinglePath")).toString();
    else if (category == QLatin1String("programs"))
        configured = settings.value(QStringLiteral("ProgramsPath")).toString();
    else if (category == QLatin1String("pub"))
        configured = settings.value(QStringLiteral("StationSync/PubPath")).toString();
    if (!configured.trimmed().isEmpty())
        return configured.trimmed();

    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("media/") + category);
}

void ProductionSyncClient::setLocalRoot(const QString &category, const QString &path)
{
    if (path.trimmed().isEmpty())
        m_roots.remove(category);
    else
        m_roots.insert(category, path.trimmed());
    save();
}

QString ProductionSyncClient::relativeFor(const QString &path, const QString &category) const
{
    const QString root = QDir::cleanPath(localRoot(category));
    const QString clean = QDir::cleanPath(path);
    if (!root.isEmpty() && clean.startsWith(root + QLatin1Char('/')))
        return clean.mid(root.size() + 1);

    // A file the operator added from a memory stick still has to land
    // somewhere predictable on the station, and everything that came from one
    // folder should stay together once it gets there. This is the same shape
    // MobileSyncServer::describeFile() uses, so a file that travels down and
    // back up again keeps the same name on both machines.
    const QFileInfo info(clean);
    const QString folderKey = QString::fromLatin1(
        QCryptographicHash::hash(info.absolutePath().toUtf8(),
                                 QCryptographicHash::Sha1).toHex().left(8));
    return QStringLiteral("_elsewhere/") + folderKey + QLatin1Char('/') + info.fileName();
}

QString ProductionSyncClient::hashText(const QString &text)
{
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(),
                                                       QCryptographicHash::Sha1).toHex());
}

QString ProductionSyncClient::hashRow(const QJsonObject &row, const QString &ignoreColumn)
{
    // Sorted, so two rows with the same columns in a different order are the
    // same row; JSON objects do not promise an order and SQLite does not
    // either. The path is left out on purpose: it is different on every
    // machine and says nothing about whether the entry itself has changed.
    QStringList keys;
    for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
        if (it.key() == QLatin1String("file")
            || it.key().compare(QLatin1String("path"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        if (!ignoreColumn.isEmpty()
            && it.key().compare(ignoreColumn, Qt::CaseInsensitive) == 0) {
            continue;
        }
        keys << it.key();
    }
    keys.sort();

    QString canonical;
    for (const QString &key : std::as_const(keys)) {
        const QJsonValue value = row.value(key);
        canonical += key + QLatin1Char('=');
        canonical += value.isNull() ? QStringLiteral("\x01") : value.toVariant().toString();
        canonical += QLatin1Char('\n');
    }
    return hashText(canonical);
}

// ------------------------------------------------------------------- requests

QNetworkReply *ProductionSyncClient::get(const QString &path, qint64 resumeFrom)
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

QNetworkReply *ProductionSyncClient::post(const QString &path, const QByteArray &body,
                                          const QByteArray &contentType)
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
    request.setRawHeader("Content-Type", contentType);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    return m_net->post(request, body);
}

// -------------------------------------------------------------------- pairing

void ProductionSyncClient::pairWith(const QString &host, quint16 port, const QString &code)
{
    const QString cleanHost = host.trimmed();
    if (cleanHost.isEmpty()) {
        emit pairingFailed(tr("Enter the address of the station on air."));
        return;
    }

    const bool newStation = (cleanHost != m_host);
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
                tr("XFB production on %1").arg(QSysInfo::machineHostName()));
    // The station decides the role from the window its operator opened; this
    // only says what we came for, so the other end can say no clearly.
    body.insert(QStringLiteral("role"), QStringLiteral("production"));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_net->post(request,
                                       QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, newStation]() {
        reply->deleteLater();
        const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();

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
        if (answer.value(QStringLiteral("role")).toString() != QLatin1String("production")) {
            // The station's operator opened one of the other pairing windows;
            // that token would be refused by every endpoint that matters here.
            emit pairingFailed(tr("That code was for pairing something else. On the "
                                  "station, open Production Computers and start "
                                  "pairing from there instead."));
            return;
        }

        m_token = token;
        m_peerName = m_host;
        save();
        if (newStation) {
            // Nothing has been agreed with this station yet.
            loadBaseline();
            refreshPendingChanges();
        }
        emit pairingSucceeded(m_peerName);
    });
}

// ------------------------------------------------------- copying the station down

void ProductionSyncClient::pull()
{
    if (m_busy)
        return;
    if (m_host.isEmpty() || m_token.isEmpty()) {
        // Whatever asked for this run is not going to get one, so the standing
        // intention to publish afterwards goes with it — otherwise the next
        // plain fetch would publish without being asked to.
        m_publishAfterPull = false;
        emit failed(tr("This XFB is not paired with a station yet."));
        return;
    }

    m_cancelled = false;
    m_manifest = QJsonObject();
    m_pending.clear();
    m_pendingPlaylists.clear();
    m_bytesTotal = m_bytesDone = 0;
    m_filesFetched = m_filesFailed = m_filesAlreadyHere = 0;
    m_playlistsFetched = m_rowsWritten = m_rowsKept = 0;
    m_filesPublished = m_rowsPublished = m_publishFailures = m_playlistsPublished = 0;
    m_rowsRemoved = m_filesDeletedThere = 0;

    m_filesMissingOnShare = m_filesCopiedToShare = 0;

    setBusy(true);

    // An unmounted share and a station that has deleted its library look the
    // same from here — every file missing, every row skipped — so the one that
    // is this machine's own fault is caught before the station is troubled.
    if (sharesMedia()) {
        QString reason;
        if (!sharedRootsUsable(&reason)) {
            abortRun(reason);
            return;
        }
    }

    setStage(Stage::Connecting, tr("Asking %1 what it is playing...").arg(m_host));

    QNetworkReply *reply = get(QStringLiteral("/api/station/manifest"));
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        if (reply->error() != QNetworkReply::NoError) {
            const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();
            const QString message = answer.value(QStringLiteral("error")).toString();
            abortRun(message.isEmpty() ? reply->errorString() : message);
            return;
        }
        onManifest(reply->readAll());
    });
}

void ProductionSyncClient::sync()
{
    if (m_busy)
        return;
    m_publishAfterPull = true;
    pull();
}

void ProductionSyncClient::cancel()
{
    if (!m_busy)
        return;
    m_cancelled = true;
    if (m_currentReply)
        m_currentReply->abort();
}

void ProductionSyncClient::onManifest(const QByteArray &body)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        abortRun(tr("The station sent something this XFB could not read."));
        return;
    }

    m_manifest = document.object();
    rememberStationRoots(m_manifest.value(QStringLiteral("roots")).toObject());
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
        if (playlistName.isEmpty()
            || !MobileSyncServer::isSafeRelativePath(playlistName + QStringLiteral(".xml"))) {
            continue;
        }
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
        mergeCatalogue();
        applySettings();
        afterPull();
        return;
    }

    setStage(Stage::Files, tr("%1 to fetch in %2 files")
                               .arg(humanBytes(m_bytesTotal))
                               .arg(m_pending.size()));
    startNextFile();
}

void ProductionSyncClient::planFiles(const QJsonObject &manifest)
{
    const QJsonObject tables = manifest.value(QStringLiteral("tables")).toObject();

    // On shared storage there is nothing to plan. The station's roots and this
    // machine's roots are the same folders, so every file the manifest names
    // is already open-able here at exactly the path the merge will write. All
    // this pass does is look, so that a share which turns out to hold half of
    // what the station claims is reported as that rather than as a catalogue
    // with holes in it.
    if (sharesMedia()) {
        for (const QString &category : kMediaTables) {
            const QJsonArray rows = tables.value(category).toArray();
            const QString root = localRoot(category);
            for (const QJsonValue &value : rows) {
                const QJsonObject file =
                    value.toObject().value(QStringLiteral("file")).toObject();
                if (file.isEmpty())
                    continue;
                const QString rel = file.value(QStringLiteral("rel")).toString();
                if (!MobileSyncServer::isSafeRelativePath(rel))
                    continue;
                const qint64 bytes = file.value(QStringLiteral("bytes")).toInteger();
                const QFileInfo here(QDir(root).filePath(rel));
                if (here.exists() && (bytes <= 0 || here.size() == bytes))
                    ++m_filesAlreadyHere;
                else
                    ++m_filesMissingOnShare;
            }
        }
        return;
    }

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
            job.category = category;
            if (job.id.isEmpty() || !MobileSyncServer::isSafeRelativePath(job.relative))
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

bool ProductionSyncClient::sharedRootsUsable(QString *reason) const
{
    for (const QString &category : kMediaTables) {
        const QString root = localRoot(category).trimmed();
        if (root.isEmpty()) {
            if (reason) {
                *reason = tr("This XFB has no folder set for %1. On shared "
                             "storage every media folder has to point at the "
                             "station's own — set them in Options.")
                              .arg(tableTitle(category));
            }
            return false;
        }
        if (!QDir(root).exists()) {
            if (reason) {
                *reason = tr("The folder for %1 (%2) is not there. If it is on "
                             "the station, check that the share is mounted.")
                              .arg(tableTitle(category), root);
            }
            return false;
        }
    }
    return true;
}

void ProductionSyncClient::rememberStationRoots(const QJsonObject &roots)
{
    if (roots.isEmpty())
        return;
    bool changed = false;
    for (const QString &category : kMediaTables) {
        const QString value = roots.value(category).toString();
        if (value.isEmpty() || m_stationRoots.value(category) == value)
            continue;
        m_stationRoots.insert(category, value);
        changed = true;
    }
    if (changed)
        save();
}

bool ProductionSyncClient::isUnderRoot(const QString &path, const QString &category) const
{
    const QString root = QDir::cleanPath(localRoot(category));
    if (root.isEmpty())
        return false;
    const QString clean = QDir::cleanPath(path);
    return clean == root || clean.startsWith(root + QLatin1Char('/'));
}

void ProductionSyncClient::startNextFile()
{
    if (m_cancelled) {
        abortRun(tr("Sync stopped."));
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

void ProductionSyncClient::startNextPlaylist()
{
    if (m_cancelled) {
        abortRun(tr("Sync stopped."));
        return;
    }
    if (m_pendingPlaylists.isEmpty()) {
        setStage(Stage::Catalogue, tr("Merging the station's catalogue..."));
        mergeCatalogue();
        applySettings();
        afterPull();
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, target, name]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray contents = reply->readAll();
            QFile file(target);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(contents);
                file.close();
                ++m_playlistsFetched;
                // This copy came from the station, so it is not something to
                // publish back at it on the next run.
                m_baselinePlaylists.insert(name,
                                           hashText(QString::fromUtf8(contents)));
            }
        }
        reportProgress();
        startNextPlaylist();
    });
}

namespace {

/** Every row of a table as JSON, plus the path it points at (media only). */
QVector<QPair<QString, QJsonObject>> readRows(QSqlDatabase &db, const QString &table)
{
    QVector<QPair<QString, QJsonObject>> rows;
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
            if (value.isNull())
                row.insert(name, QJsonValue::Null);
            else if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong)
                row.insert(name, value.toLongLong());
            else if (value.typeId() == QMetaType::Double)
                row.insert(name, value.toDouble());
            else
                row.insert(name, value.toString());
        }
        rows.append(qMakePair(path, row));
    }
    return rows;
}

} // namespace

QString ProductionSyncClient::hashRows(const QVector<QJsonObject> &rows,
                                      const QString &ignoreColumn)
{
    QStringList hashes;
    hashes.reserve(rows.size());
    for (const QJsonObject &row : rows)
        hashes << hashRow(row, ignoreColumn);
    hashes.sort();
    return hashText(hashes.join(QLatin1Char('\n')));
}


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

void ProductionSyncClient::mergeCatalogue()
{
    const QJsonObject tables = m_manifest.value(QStringLiteral("tables")).toObject();
    if (tables.isEmpty())
        return;

    QSqlDatabase db = libraryDatabase();
    if (!db.isValid() || !db.isOpen()) {
        m_lastResult = tr("The library database is not open; the catalogue was not merged.");
        return;
    }

    // Merging rewrites rows the operator may have spent an afternoon on, so
    // the database as it stands goes somewhere safe first. XFB's own rotating
    // backups are no help here: a few syncs would roll straight through them.
    const QString dbPath = db.databaseName();
    if (!dbPath.isEmpty() && QFile::exists(dbPath)) {
        const QString backupDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + QStringLiteral("/backups");
        QDir().mkpath(backupDir);
        backupDatabaseTo(dbPath, QDir(backupDir).filePath(
            QStringLiteral("before-production-sync.db")));
    }

    db.transaction();

    for (const QString &table : kMediaTables) {
        if (!tables.contains(table))
            continue;   // a manifest silent about a table leaves it alone

        const QStringList known = columnsOf(db, table);
        if (known.isEmpty())
            continue;

        // Both machines number their own rows, so an entry made here may
        // already hold the id the station is sending for a different one.
        // Where they collide the incoming id is dropped and this database
        // assigns its own: a media entry is identified by its file, and the
        // numbering is each machine's own business — which is also why the
        // number is left out of the fingerprints below.
        const QString keyColumn = keyColumnOf(db, table);

        // What this machine holds right now, keyed the way the station keys
        // it, so the two can be compared entry by entry.
        QHash<QString, QString> localHashes;   // rel -> fingerprint
        QHash<QString, QString> localPaths;    // rel -> path here
        const QVector<QPair<QString, QJsonObject>> local = readRows(db, table);
        for (const auto &entry : local) {
            if (entry.first.isEmpty())
                continue;
            const QString rel = relativeFor(entry.first, table);
            localHashes.insert(rel, hashRow(entry.second, keyColumn));
            localPaths.insert(rel, entry.first);
        }

        const QHash<QString, QString> agreed = m_baselineRows.value(table);
        QHash<QString, QString> nowAgreed;
        const QString root = localRoot(table);

        const QJsonArray rows = tables.value(table).toArray();
        for (const QJsonValue &value : rows) {
            const QJsonObject row = value.toObject();
            const QJsonObject file = row.value(QStringLiteral("file")).toObject();
            const QString rel = file.value(QStringLiteral("rel")).toString();
            if (!MobileSyncServer::isSafeRelativePath(rel))
                continue;

            const QString localPath = QDir(root).filePath(rel);
            if (!QFile::exists(localPath)) {
                // Its file has not arrived. Writing the row anyway would give
                // this machine an entry that plays as silence.
                continue;
            }

            const QString stationHash = hashRow(row, keyColumn);
            const QString agreedHash = agreed.value(rel);

            // The one case where this machine wins: the entry was changed
            // here and not there, which is simply work that has not been
            // published yet. Anything else, the station on air is the truth.
            // Taking an entry out counts as changing it — an ad withdrawn here
            // must not be quietly put back by the next fetch, or it can never
            // be withdrawn at all.
            const bool knownHere = localHashes.contains(rel);
            const bool changedHere = knownHere && localHashes.value(rel) != agreedHash;
            const bool deletedHere = !knownHere && agreed.contains(rel);
            const bool changedThere = stationHash != agreedHash;
            if ((changedHere || deletedHere) && !changedThere) {
                ++m_rowsKept;
                // Still on the agreed list: that is what tells publishing the
                // station has an entry here no longer wants.
                nowAgreed.insert(rel, agreedHash);
                continue;
            }

            QStringList columns;
            QVariantList values;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                if (it.key() == QLatin1String("file"))
                    continue;
                if (!known.contains(it.key(), Qt::CaseInsensitive))
                    continue;   // a column this database does not have
                columns << it.key();
                values << (it.value().isNull() ? QVariant() : QVariant(it.value().toVariant()));
            }
            if (columns.isEmpty())
                continue;

            // The same entry arriving twice must update, never double up.
            QSqlQuery remove(db);
            remove.prepare(QStringLiteral("DELETE FROM \"%1\" WHERE path = ?").arg(table));
            remove.addBindValue(localPaths.value(rel, localPath));
            remove.exec();

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
            values << localPath;

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
            if (insert.exec()) {
                ++m_rowsWritten;
                nowAgreed.insert(rel, stationHash);
            }
        }

        // Entries this machine has that the station never sent are exactly the
        // work waiting to be published, so they are left where they are and
        // kept out of what the two ends have agreed on.
        m_baselineRows.insert(table, nowAgreed);
    }

    // Configuration tables are a shape rather than a set of rows — an hour
    // that was cleared has no row left to say so — so they travel whole and
    // are taken whole, unless this machine is the one holding a change.
    for (const QString &table : kConfigTables) {
        if (!tables.contains(table))
            continue;
        const QStringList known = columnsOf(db, table);
        if (known.isEmpty())
            continue;

        const QString keyColumn = keyColumnOf(db, table);

        QVector<QJsonObject> localRows;
        for (const auto &entry : readRows(db, table))
            localRows << entry.second;
        const QString localHash = hashRows(localRows, keyColumn);

        QVector<QJsonObject> stationRows;
        const QJsonArray rows = tables.value(table).toArray();
        for (const QJsonValue &value : rows)
            stationRows << value.toObject();
        const QString stationHash = hashRows(stationRows, keyColumn);

        const QString agreedHash = m_baselineTables.value(table);
        if (localHash != agreedHash && stationHash == agreedHash)
            continue;   // changed here only: publishing will carry it up
        if (localHash == stationHash) {
            m_baselineTables.insert(table, stationHash);
            continue;   // already identical; nothing to write
        }

        QSqlQuery clear(db);
        if (!clear.exec(QStringLiteral("DELETE FROM \"%1\"").arg(table)))
            continue;

        for (const QJsonObject &row : std::as_const(stationRows)) {
            QStringList columns;
            QVariantList values;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                if (!known.contains(it.key(), Qt::CaseInsensitive))
                    continue;
                columns << it.key();
                values << (it.value().isNull() ? QVariant() : QVariant(it.value().toVariant()));
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
        m_baselineTables.insert(table, stationHash);
    }

    if (!db.commit()) {
        db.rollback();
        m_lastResult = tr("The catalogue could not be merged: %1").arg(db.lastError().text());
    }
    saveBaseline();
}

void ProductionSyncClient::applySettings()
{
    const QJsonObject incoming = m_manifest.value(QStringLiteral("settings")).toObject();
    if (incoming.isEmpty())
        return;

    // The station says what it runs with; this end decides what it is willing
    // to take. Anything outside the shared list is machine-specific — a
    // folder, a port, a credential — and taking it would point this machine at
    // the station's own surroundings.
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

void ProductionSyncClient::afterPull()
{
    if (m_cancelled) {
        abortRun(tr("Sync stopped."));
        return;
    }
    // The merge has just written the baseline from what the station holds, so
    // whatever is left over here really is this machine's own work.
    if (m_needsPull) {
        m_needsPull = false;
        save();
    }
    if (m_publishAfterPull) {
        m_publishAfterPull = false;
        startPublish();
        return;
    }
    scanForChanges();   // so the dialog can say what is still waiting to go up
    completeRun();
}

// -------------------------------------------------- sending the work back up

void ProductionSyncClient::scanForChanges()
{
    m_uploads.clear();
    m_readyRows.clear();
    m_rowBatches.clear();
    m_changedTables.clear();
    m_changedPlaylists.clear();
    m_pendingConfigTables.clear();
    m_removals.clear();
    m_pendingSummary.clear();

    QSqlDatabase db = libraryDatabase();
    if (!db.isValid() || !db.isOpen()) {
        emit pendingChangesChanged();
        return;
    }

    for (const QString &table : kMediaTables) {
        const QHash<QString, QString> agreed = m_baselineRows.value(table);
        const QString keyColumn = keyColumnOf(db, table);
        int changed = 0;
        QStringList here;

        for (const auto &entry : readRows(db, table)) {
            const QString path = entry.first;
            if (path.isEmpty())
                continue;
            const QFileInfo info(path);
            if (!info.exists())
                continue;   // an entry whose file is gone is nothing to publish

            const QString rel = relativeFor(path, table);
            here << rel;
            const QString hash = hashRow(entry.second, keyColumn);
            if (agreed.value(rel) == hash)
                continue;   // the station already has this exact entry

            PendingRow pending;
            pending.table = table;
            pending.relative = rel;
            pending.path = path;
            pending.bytes = info.size();
            pending.row = entry.second;
            pending.hash = hash;
            m_uploads.enqueue(pending);
            ++changed;
        }

        // Anything the station was told about that is not here any more was
        // taken out on purpose, and the station is still playing it.
        QStringList gone;
        for (auto it = agreed.constBegin(); it != agreed.constEnd(); ++it) {
            if (!here.contains(it.key()))
                gone << it.key();
        }
        if (!gone.isEmpty())
            m_removals.insert(table, gone);

        if (changed > 0) {
            m_pendingSummary << tr("%n new or changed entry/entries in %1", nullptr, changed)
                                    .arg(tableTitle(table));
        }
        if (!gone.isEmpty()) {
            m_pendingSummary << tr("%n entry/entries taken out of %1", nullptr, gone.size())
                                    .arg(tableTitle(table));
        }
    }

    for (const QString &table : kConfigTables) {
        QVector<QJsonObject> rows;
        for (const auto &entry : readRows(db, table))
            rows << entry.second;
        const QString hash = hashRows(rows, keyColumnOf(db, table));
        // A table nobody has ever synced is not "changed" until there is
        // something in it; a station has no use for an empty hour grid it
        // never asked for.
        if (hash == m_baselineTables.value(table))
            continue;
        if (rows.isEmpty() && !m_baselineTables.contains(table))
            continue;
        m_changedTables.insert(table, hash);
        m_pendingConfigTables << table;
        m_pendingSummary << tr("changes to %1").arg(tableTitle(table));
    }

    const QDir playlistDir(localRoot(QStringLiteral("playlists")));
    const QFileInfoList playlists =
        playlistDir.entryInfoList(QStringList() << QStringLiteral("*.xml"), QDir::Files);
    for (const QFileInfo &info : playlists) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString hash = hashText(QString::fromUtf8(file.readAll()));
        file.close();
        if (m_baselinePlaylists.value(info.completeBaseName()) == hash)
            continue;
        m_changedPlaylists << info.completeBaseName();
    }
    if (!m_changedPlaylists.isEmpty()) {
        m_pendingSummary << tr("%n playlist(s)", nullptr, m_changedPlaylists.size());
    }

    emit pendingChangesChanged();
}

void ProductionSyncClient::checkSharedStorage()
{
    if (m_host.isEmpty() || m_token.isEmpty()) {
        emit sharedStorageChecked(false, tr("This XFB is not paired with a station yet."));
        return;
    }
    QString reason;
    if (!sharedRootsUsable(&reason)) {
        emit sharedStorageChecked(false, reason);
        return;
    }

    QNetworkReply *hello = get(QStringLiteral("/api/production/hello"));
    connect(hello, &QNetworkReply::finished, this, [this, hello]() {
        hello->deleteLater();
        if (hello->error() == QNetworkReply::NoError) {
            rememberStationRoots(QJsonDocument::fromJson(hello->readAll())
                                     .object()
                                     .value(QStringLiteral("roots"))
                                     .toObject());
        }

        QNetworkReply *reply = get(QStringLiteral("/api/station/manifest"));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                emit sharedStorageChecked(
                    false, tr("Could not ask the station what it holds: %1")
                               .arg(reply->errorString()));
                return;
            }

            const QJsonObject tables = QJsonDocument::fromJson(reply->readAll())
                                           .object()
                                           .value(QStringLiteral("tables"))
                                           .toObject();
            int looked = 0;
            int found = 0;
            QStringList empties;
            QString firstMiss;

            for (const QString &category : kMediaTables) {
                const QJsonArray rows = tables.value(category).toArray();
                if (rows.isEmpty())
                    continue;
                const QString root = localRoot(category);
                // A sample rather than the lot: this runs while the operator
                // waits, and a station with forty thousand tracks would have
                // them staring at a spinner to learn what twenty files can
                // tell them just as well.
                int seen = 0;
                for (const QJsonValue &value : rows) {
                    if (seen >= kShareCheckSample)
                        break;
                    const QJsonObject file =
                        value.toObject().value(QStringLiteral("file")).toObject();
                    const QString rel = file.value(QStringLiteral("rel")).toString();
                    if (rel.isEmpty() || !MobileSyncServer::isSafeRelativePath(rel))
                        continue;
                    ++seen;
                    ++looked;
                    const qint64 bytes = file.value(QStringLiteral("bytes")).toInteger();
                    const QFileInfo here(QDir(root).filePath(rel));
                    if (here.exists() && (bytes <= 0 || here.size() == bytes)) {
                        ++found;
                    } else if (firstMiss.isEmpty()) {
                        firstMiss = here.absoluteFilePath();
                    }
                }
                if (seen > 0 && found == 0)
                    empties << tableTitle(category);
            }

            if (looked == 0) {
                emit sharedStorageChecked(
                    false, tr("The station has no media to check against yet, so "
                              "there is nothing here to prove the folders are "
                              "shared. Try again once it has some."));
                return;
            }
            if (found == looked) {
                emit sharedStorageChecked(
                    true, tr("Checked %n file(s) the station named and found "
                             "every one of them on this machine's folders. "
                             "These are the same folders.", nullptr, looked));
                return;
            }
            if (found == 0) {
                emit sharedStorageChecked(
                    false, tr("None of the %n file(s) the station named is on "
                              "this machine's folders — the first one it looked "
                              "for was %1. These are not the same folders: "
                              "either the share is not mounted or the media "
                              "folders in Options point somewhere else.",
                              nullptr, looked)
                               .arg(firstMiss));
                return;
            }
            QString detail = tr("Found %1 of the %n file(s) the station named. "
                                "The folders are shared but not complete — the "
                                "first one missing was %2.", nullptr, looked)
                                 .arg(found)
                                 .arg(firstMiss);
            if (!empties.isEmpty()) {
                detail += QLatin1Char(' ')
                          + tr("Nothing at all was found for %1.")
                                .arg(empties.join(QStringLiteral(", ")));
            }
            emit sharedStorageChecked(false, detail);
        });
    });
}

void ProductionSyncClient::refreshPendingChanges()
{
    if (m_busy)
        return;   // a run in progress is already keeping this up to date
    scanForChanges();
    // Nothing is being sent, so what was queued for sending is not wanted.
    m_uploads.clear();
}

void ProductionSyncClient::publish()
{
    if (m_busy)
        return;
    if (m_host.isEmpty() || m_token.isEmpty()) {
        emit failed(tr("This XFB is not paired with a station yet."));
        return;
    }
    if (m_needsPull) {
        emit failed(tr("Fetch from the station first. Where the media is kept "
                       "has changed, so this XFB has to see what the station "
                       "holds before it can tell that apart from new work."));
        return;
    }
    m_cancelled = false;
    m_publishAfterPull = false;
    m_filesFetched = m_filesFailed = m_filesAlreadyHere = 0;
    m_playlistsFetched = m_rowsWritten = m_rowsKept = 0;
    m_filesPublished = m_rowsPublished = m_publishFailures = m_playlistsPublished = 0;
    m_rowsRemoved = m_filesDeletedThere = 0;
    m_filesMissingOnShare = m_filesCopiedToShare = 0;
    setBusy(true);

    if (sharesMedia()) {
        QString reason;
        if (!sharedRootsUsable(&reason)) {
            abortRun(reason);
            return;
        }
    }

    startPublish();
}

void ProductionSyncClient::startPublish()
{
    setStage(Stage::Scanning, tr("Looking for work to publish..."));
    scanForChanges();

    if (m_uploads.isEmpty() && m_pendingConfigTables.isEmpty()
        && m_changedPlaylists.isEmpty() && m_removals.isEmpty()) {
        completeRun();
        return;
    }

    m_bytesTotal = 0;
    m_bytesDone = 0;
    for (const PendingRow &row : std::as_const(m_uploads))
        m_bytesTotal += row.bytes;

    if (sharesMedia()) {
        setStage(Stage::Uploading, tr("Putting the work on the station's folders..."));
    } else {
        setStage(Stage::Uploading, tr("Sending %1 to %2...")
                                       .arg(humanBytes(m_bytesTotal), m_peerName.isEmpty()
                                                                          ? m_host : m_peerName));
    }
    startNextUpload();
}

QString ProductionSyncClient::shareTargetFor(const QString &source,
                                            const QString &category) const
{
    const QDir root(localRoot(category));
    const QFileInfo info(source);
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty()
                               ? QString()
                               : QLatin1Char('.') + info.suffix();

    // Straight into the category folder, under its own name: this is a file
    // the station is about to be told to play, and burying it under a hashed
    // folder because of where it happened to come from would make a mess of a
    // music library somebody else has to live in.
    QString candidate = root.filePath(base + suffix);
    for (int n = 2; n < 1000; ++n) {
        const QFileInfo there(candidate);
        if (!there.exists())
            return candidate;
        if (there.size() == info.size())
            return candidate;   // same name, same size: the same file already
        candidate = root.filePath(QStringLiteral("%1 (%2)%3").arg(base)
                                      .arg(n).arg(suffix));
    }
    return candidate;
}

bool ProductionSyncClient::repointRow(const QString &table, const QString &from,
                                      const QString &to)
{
    QSqlDatabase db = libraryDatabase();
    if (!db.isValid() || !db.isOpen())
        return false;
    QSqlQuery query(db);
    query.prepare(QStringLiteral("UPDATE \"%1\" SET path = :to WHERE path = :from")
                      .arg(table));
    query.bindValue(QStringLiteral(":to"), to);
    query.bindValue(QStringLiteral(":from"), from);
    if (!query.exec()) {
        qWarning() << "Could not repoint" << from << "at the shared copy:"
                   << query.lastError().text();
        return false;
    }
    return true;
}

bool ProductionSyncClient::placeOnShare(PendingRow &row)
{
    // The ordinary case by a long way: the operator has been editing entries
    // that came from the station in the first place, and their files never
    // left the share.
    if (isUnderRoot(row.path, row.table)) {
        row.relative = relativeFor(row.path, row.table);
        return true;
    }

    const QString target = shareTargetFor(row.path, row.table);
    const QFileInfo existing(target);
    if (!(existing.exists() && existing.size() == QFileInfo(row.path).size())) {
        QDir().mkpath(QFileInfo(target).absolutePath());
        // A half-written copy is worse than none: the station would be told to
        // play a truncated file. So it lands beside its name and is only given
        // that name once all of it is there.
        const QString part = target + kPartSuffix;
        QFile::remove(part);
        if (!QFile::copy(row.path, part)) {
            qWarning() << "Could not copy" << row.path << "to the shared folder" << part;
            return false;
        }
        QFile::remove(target);
        if (!QFile::rename(part, target)) {
            QFile::remove(part);
            qWarning() << "Could not put" << part << "in place as" << target;
            return false;
        }
        ++m_filesCopiedToShare;
    }

    // From here on this machine plays the shared copy too. Without this the
    // next scan would find the original again and offer to copy it a second
    // time, and this desk would go on playing a file nobody else can hear.
    repointRow(row.table, row.path, target);
    row.path = target;
    row.relative = relativeFor(target, row.table);
    return true;
}

void ProductionSyncClient::startNextUpload()
{
    if (m_cancelled) {
        abortRun(tr("Publishing stopped."));
        return;
    }
    if (m_uploads.isEmpty()) {
        // Group what made it into requests: a station with a thousand new
        // tracks should not mean a thousand round trips.
        QHash<QString, QVector<PendingRow>> byTable;
        for (const PendingRow &row : std::as_const(m_readyRows))
            byTable[row.table].append(row);
        for (auto it = byTable.constBegin(); it != byTable.constEnd(); ++it) {
            QVector<PendingRow> batch;
            for (const PendingRow &row : it.value()) {
                batch.append(row);
                if (batch.size() >= kRowBatch) {
                    m_rowBatches.enqueue(qMakePair(it.key(), batch));
                    batch.clear();
                }
            }
            if (!batch.isEmpty())
                m_rowBatches.enqueue(qMakePair(it.key(), batch));
        }
        setStage(Stage::Publishing, tr("Adding the entries to the station..."));
        sendNextRowBatch();
        return;
    }

    m_currentUpload = m_uploads.dequeue();
    m_uploadOffset = 0;
    m_uploadConflicts = 0;

    // Shared storage: there is nowhere to send it to. The file is either
    // already on the station's own disk — in which case uploading it would be
    // this machine reading a file and writing it over itself — or it is a
    // local one that belongs on the share, and a copy over the mount is both
    // quicker than HTTP and the only way the path in the entry can be right.
    if (sharesMedia()) {
        if (placeOnShare(m_currentUpload)) {
            m_bytesDone += m_currentUpload.bytes;
            m_readyRows.append(m_currentUpload);
        } else {
            ++m_publishFailures;
        }
        reportProgress();
        // Straight on rather than through the event loop would recurse once
        // per entry, and a thousand new tracks is a thousand frames of stack.
        QTimer::singleShot(0, this, &ProductionSyncClient::startNextUpload);
        return;
    }

    // Ask before sending: a track that travelled down from this very station
    // is already there, and a previous attempt may have got most of the way.
    QNetworkReply *reply =
        get(QStringLiteral("/api/production/have?category=") + m_currentUpload.table
            + QStringLiteral("&rel=")
            + QString::fromLatin1(QUrl::toPercentEncoding(m_currentUpload.relative)));
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_currentReply = nullptr;

        if (reply->error() != QNetworkReply::NoError) {
            ++m_publishFailures;
            startNextUpload();
            return;
        }

        const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();
        const qint64 there = answer.value(QStringLiteral("bytes")).toInteger();
        if (there == m_currentUpload.bytes && there > 0) {
            // The file is already up there; only its entry needs sending.
            m_bytesDone += m_currentUpload.bytes;
            m_readyRows.append(m_currentUpload);
            reportProgress();
            startNextUpload();
            return;
        }
        const qint64 partial = answer.value(QStringLiteral("partial")).toInteger();
        m_uploadOffset = (partial > 0 && partial < m_currentUpload.bytes) ? partial : 0;
        m_bytesDone += m_uploadOffset;
        uploadChunk();
    });
}

void ProductionSyncClient::uploadChunk()
{
    if (m_cancelled) {
        abortRun(tr("Publishing stopped."));
        return;
    }

    QFile file(m_currentUpload.path);
    if (!file.open(QIODevice::ReadOnly)) {
        ++m_publishFailures;
        startNextUpload();
        return;
    }
    file.seek(m_uploadOffset);
    const QByteArray chunk = file.read(kUploadChunkBytes);
    file.close();

    if (chunk.isEmpty()) {
        // The file shrank under us, or the offset is past its end.
        ++m_publishFailures;
        startNextUpload();
        return;
    }

    // Built by concatenation rather than with arg(): the encoded relative path
    // contains sequences like "%2F", and a later arg() would happily take that
    // "%2" for its own placeholder and eat the rest of the query.
    const QString path =
        QStringLiteral("/api/production/file?category=") + m_currentUpload.table
        + QStringLiteral("&rel=")
        + QString::fromLatin1(QUrl::toPercentEncoding(m_currentUpload.relative))
        + QStringLiteral("&offset=") + QString::number(m_uploadOffset)
        + QStringLiteral("&total=") + QString::number(m_currentUpload.bytes);

    QNetworkReply *reply = post(path, chunk, "application/octet-stream");
    m_currentReply = reply;
    const qint64 sent = chunk.size();
    connect(reply, &QNetworkReply::finished, this, [this, reply, sent]() {
        reply->deleteLater();
        m_currentReply = nullptr;

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();

        if (status == 409) {
            // The station is further along (or further behind) than this end
            // thought; carry on from where it actually is.
            const qint64 received = answer.value(QStringLiteral("received")).toInteger();
            if (++m_uploadConflicts > 3 || received >= m_currentUpload.bytes) {
                ++m_publishFailures;
                startNextUpload();
                return;
            }
            m_bytesDone += received - m_uploadOffset;
            m_uploadOffset = received;
            uploadChunk();
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            // Whatever arrived stays on the station as a fragment, so the next
            // run picks this file up where it stopped rather than at the start.
            ++m_publishFailures;
            startNextUpload();
            return;
        }

        m_bytesDone += sent;
        m_uploadOffset = answer.value(QStringLiteral("received")).toInteger();
        reportProgress();

        if (answer.value(QStringLiteral("complete")).toBool()) {
            ++m_filesPublished;
            m_readyRows.append(m_currentUpload);
            startNextUpload();
            return;
        }
        uploadChunk();
    });
}

void ProductionSyncClient::sendNextRowBatch()
{
    if (m_cancelled) {
        abortRun(tr("Publishing stopped."));
        return;
    }

    if (m_rowBatches.isEmpty()) {
        // Then what was taken out here. It goes after the additions so that a
        // track moved from one folder to another is added under its new name
        // before it is dropped under its old one, never the other way round.
        if (!m_removals.isEmpty()) {
            const QString table = m_removals.constBegin().key();
            const QStringList gone = m_removals.take(table);

            QJsonArray remove;
            for (const QString &rel : gone)
                remove.append(rel);

            QJsonObject body;
            body.insert(QStringLiteral("table"), table);
            body.insert(QStringLiteral("mode"), QStringLiteral("upsert"));
            body.insert(QStringLiteral("rows"), QJsonArray());
            body.insert(QStringLiteral("remove"), remove);

            QNetworkReply *reply = post(QStringLiteral("/api/production/rows"),
                                        QJsonDocument(body).toJson(QJsonDocument::Compact),
                                        "application/json");
            m_currentReply = reply;
            connect(reply, &QNetworkReply::finished, this, [this, reply, table, gone]() {
                reply->deleteLater();
                m_currentReply = nullptr;
                const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();
                if (reply->error() == QNetworkReply::NoError) {
                    m_rowsRemoved += answer.value(QStringLiteral("removed")).toInt();
                    // Whether the audio went with the entry was the station's
                    // decision, so it is reported rather than assumed.
                    m_filesDeletedThere +=
                        answer.value(QStringLiteral("filesRemoved")).toInt();
                    // The station no longer has them, so neither does the
                    // record of what the two ends agree on.
                    QHash<QString, QString> agreed = m_baselineRows.value(table);
                    for (const QString &rel : gone)
                        agreed.remove(rel);
                    m_baselineRows.insert(table, agreed);
                } else {
                    ++m_publishFailures;
                }
                sendNextRowBatch();
            });
            return;
        }

        // Then the schedule, the genre lists and anything else that travels
        // whole rather than row by row.
        if (!m_pendingConfigTables.isEmpty()) {
            const QString table = m_pendingConfigTables.takeFirst();
            QSqlDatabase db = libraryDatabase();
            QJsonArray rows;
            QVector<QJsonObject> plain;
            if (db.isValid() && db.isOpen()) {
                for (const auto &entry : readRows(db, table)) {
                    rows.append(entry.second);
                    plain << entry.second;
                }
            }

            QJsonObject body;
            body.insert(QStringLiteral("table"), table);
            body.insert(QStringLiteral("mode"), QStringLiteral("replace"));
            body.insert(QStringLiteral("rows"), rows);

            const QString hash = hashRows(plain, keyColumnOf(db, table));
            QNetworkReply *reply = post(QStringLiteral("/api/production/rows"),
                                        QJsonDocument(body).toJson(QJsonDocument::Compact),
                                        "application/json");
            m_currentReply = reply;
            connect(reply, &QNetworkReply::finished, this, [this, reply, table, hash]() {
                reply->deleteLater();
                m_currentReply = nullptr;
                const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();
                if (reply->error() == QNetworkReply::NoError) {
                    m_rowsPublished += answer.value(QStringLiteral("written")).toInt();
                    m_baselineTables.insert(table, hash);
                } else {
                    ++m_publishFailures;
                }
                sendNextRowBatch();
            });
            return;
        }

        setStage(Stage::Playlists, tr("Sending playlists..."));
        startNextPlaylistUpload();
        return;
    }

    const QPair<QString, QVector<PendingRow>> batch = m_rowBatches.dequeue();

    QJsonArray rows;
    for (const PendingRow &pending : batch.second) {
        QJsonObject row = pending.row;
        QJsonObject file;
        file.insert(QStringLiteral("rel"), pending.relative);
        row.insert(QStringLiteral("file"), file);
        rows.append(row);
    }

    QJsonObject body;
    body.insert(QStringLiteral("table"), batch.first);
    body.insert(QStringLiteral("mode"), QStringLiteral("upsert"));
    body.insert(QStringLiteral("rows"), rows);

    QNetworkReply *reply = post(QStringLiteral("/api/production/rows"),
                                QJsonDocument(body).toJson(QJsonDocument::Compact),
                                "application/json");
    m_currentReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, batch]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        const QJsonObject answer = QJsonDocument::fromJson(reply->readAll()).object();

        if (reply->error() != QNetworkReply::NoError) {
            ++m_publishFailures;
            sendNextRowBatch();
            return;
        }

        m_rowsPublished += answer.value(QStringLiteral("written")).toInt();

        // Whatever the station could not take stays out of the agreement, so
        // the next run offers it again rather than assuming it landed.
        QStringList missing;
        for (const QJsonValue &value : answer.value(QStringLiteral("missing")).toArray())
            missing << value.toString();

        QHash<QString, QString> agreed = m_baselineRows.value(batch.first);
        for (const PendingRow &pending : batch.second) {
            if (missing.contains(pending.relative))
                continue;
            agreed.insert(pending.relative, pending.hash);
        }
        m_baselineRows.insert(batch.first, agreed);

        sendNextRowBatch();
    });
}

void ProductionSyncClient::startNextPlaylistUpload()
{
    if (m_cancelled) {
        abortRun(tr("Publishing stopped."));
        return;
    }
    if (m_changedPlaylists.isEmpty()) {
        finishPublish();
        return;
    }

    const QString name = m_changedPlaylists.takeFirst();
    const QString path = QDir(localRoot(QStringLiteral("playlists")))
                             .filePath(name + QStringLiteral(".xml"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        startNextPlaylistUpload();
        return;
    }
    const QByteArray contents = file.readAll();
    file.close();

    QNetworkReply *reply =
        post(QStringLiteral("/api/production/playlist?name=")
                 + QString::fromLatin1(QUrl::toPercentEncoding(name)),
             contents, "application/xml");
    m_currentReply = reply;
    const QString hash = hashText(QString::fromUtf8(contents));
    connect(reply, &QNetworkReply::finished, this, [this, reply, name, hash]() {
        reply->deleteLater();
        m_currentReply = nullptr;
        if (reply->error() == QNetworkReply::NoError) {
            ++m_playlistsPublished;
            m_baselinePlaylists.insert(name, hash);
        } else {
            ++m_publishFailures;
        }
        startNextPlaylistUpload();
    });
}

void ProductionSyncClient::finishPublish()
{
    saveBaseline();
    scanForChanges();   // what, if anything, is still waiting
    m_uploads.clear();
    completeRun();
}

// ------------------------------------------------------------------ finishing

void ProductionSyncClient::completeRun()
{
    m_lastSync = QDateTime::currentDateTime();

    QStringList parts;
    if (m_filesFetched > 0)
        parts << tr("%n file(s) copied down", nullptr, m_filesFetched);
    if (sharesMedia() && m_filesAlreadyHere > 0) {
        parts << tr("%n file(s) read straight off the station's folders",
                    nullptr, m_filesAlreadyHere);
    }
    if (m_filesCopiedToShare > 0) {
        parts << tr("%n file(s) copied onto the station's folders",
                    nullptr, m_filesCopiedToShare);
    }
    if (m_filesMissingOnShare > 0) {
        parts << tr("%n entry/entries the station lists but the shared folders "
                    "do not hold", nullptr, m_filesMissingOnShare);
    }
    if (m_rowsWritten > 0)
        parts << tr("%n catalogue row(s) from the station", nullptr, m_rowsWritten);
    if (m_rowsKept > 0)
        parts << tr("%n unpublished change(s) kept", nullptr, m_rowsKept);
    if (m_playlistsFetched > 0)
        parts << tr("%n playlist(s) copied down", nullptr, m_playlistsFetched);
    if (m_filesPublished > 0)
        parts << tr("%n file(s) published", nullptr, m_filesPublished);
    if (m_rowsPublished > 0)
        parts << tr("%n entry/entries published", nullptr, m_rowsPublished);
    if (m_rowsRemoved > 0)
        parts << tr("%n entry/entries taken off the station", nullptr, m_rowsRemoved);
    if (m_filesDeletedThere > 0)
        parts << tr("%n file(s) deleted there too", nullptr, m_filesDeletedThere);
    if (m_playlistsPublished > 0)
        parts << tr("%n playlist(s) published", nullptr, m_playlistsPublished);
    if (m_filesFailed > 0)
        parts << tr("%n file(s) failed — run it again to pick them up",
                    nullptr, m_filesFailed);
    if (m_publishFailures > 0)
        parts << tr("%n item(s) could not be published — run it again",
                    nullptr, m_publishFailures);
    if (parts.isEmpty())
        parts << tr("nothing had changed on either side");

    m_lastResult = parts.join(QStringLiteral(", "));
    save();
    saveBaseline();

    setStage(Stage::Done, m_lastResult);
    setBusy(false);
    emit finished(m_lastResult);
}

void ProductionSyncClient::abortRun(const QString &reason)
{
    m_publishAfterPull = false;
    m_lastResult = reason;
    save();
    saveBaseline();
    setStage(Stage::Idle, reason);
    setBusy(false);
    emit failed(reason);
}

// ------------------------------------------------------------------ progress

void ProductionSyncClient::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged(busy);
}

void ProductionSyncClient::setStage(Stage stage, const QString &description)
{
    m_stage = stage;
    emit stageChanged(stage, description);
    emit progress(m_bytesTotal > 0
                      ? int(qMin<qint64>(100, m_bytesDone * 100 / m_bytesTotal))
                      : -1,
                  description);
}

void ProductionSyncClient::reportProgress(const QString &message)
{
    const int percent = m_bytesTotal > 0
                            ? int(qMin<qint64>(100, m_bytesDone * 100 / m_bytesTotal))
                            : -1;
    emit progress(percent, message.isEmpty()
                               ? tr("%1 of %2").arg(humanBytes(m_bytesDone),
                                                    humanBytes(m_bytesTotal))
                               : message);
}
