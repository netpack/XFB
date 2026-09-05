#include "LibraryWatcher.h"

#include "../audioformats.h"
#include "../mediaduration.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTimer>
#include <QtDebug>

#include <utility>

namespace {

constexpr const char *kGroup = "WatchedFolders";
constexpr const char *kArray = "Folders";

// The database every other library write goes through.
QSqlDatabase libraryDb()
{
    return QSqlDatabase::database(QStringLiteral("xfb_connection"));
}

// The category a music file inherits from where it sits: the first folder
// below the watched one. Same rule as the folder importer, so a library filed
// "Rock/Nirvana/song.mp3" is filed the same way whether it is taken on by hand
// or noticed here. A file sitting loose in the watched folder has no category.
QString folderCategory(const QDir &root, const QString &filePath)
{
    const QString relative = root.relativeFilePath(filePath);
    if (relative.startsWith(QLatin1String("..")))
        return QString();   // reached through a link that leaves the tree

    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return QString();

    return parts.first().trimmed();
}

} // namespace

LibraryWatcher::LibraryWatcher(QObject *parent)
    : QObject(parent)
{
    m_config = loadConfig();

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &LibraryWatcher::onDirectoryChanged);

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &LibraryWatcher::onPollTimeout);

    // A copy in progress fires directoryChanged over and over; this is what
    // turns that burst into one pass, and what comes back for a file that was
    // still growing when we looked.
    m_settleTimer = new QTimer(this);
    m_settleTimer->setSingleShot(true);
    connect(m_settleTimer, &QTimer::timeout, this, &LibraryWatcher::onPollTimeout);

    if (m_config.enabled) {
        rearmWatcher();
        restartTimer();
        // Not at once: startup has a database to open and a window to draw,
        // and the folders have waited this long already.
        QTimer::singleShot(5000, this, &LibraryWatcher::onPollTimeout);
    }
}

// ------------------------------------------------------------------ settings

QString LibraryWatcher::configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

LibraryWatcher::Config LibraryWatcher::loadConfig()
{
    Config config;
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    config.enabled = settings.value(QStringLiteral("Enabled"), false).toBool();
    config.pollSeconds = settings.value(QStringLiteral("PollSeconds"), 60).toInt();
    config.quietSeconds = settings.value(QStringLiteral("QuietSeconds"), 15).toInt();
    config.batchLimit = settings.value(QStringLiteral("BatchLimit"), 100).toInt();

    const int count = settings.beginReadArray(QLatin1String(kArray));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        Folder folder;
        folder.path = settings.value(QStringLiteral("Path")).toString();
        folder.destination =
            destinationFromKey(settings.value(QStringLiteral("Destination")).toString());
        folder.recursive = settings.value(QStringLiteral("Recursive"), true).toBool();
        folder.folderGenres = settings.value(QStringLiteral("FolderGenres"), true).toBool();
        folder.genre = settings.value(QStringLiteral("Genre")).toString();
        folder.enabled = settings.value(QStringLiteral("Enabled"), true).toBool();
        if (!folder.path.trimmed().isEmpty())
            config.folders << folder;
    }
    settings.endArray();
    settings.endGroup();

    // A poll of zero seconds is a busy loop with a database at the end of it.
    config.pollSeconds = qBound(10, config.pollSeconds, 3600);
    config.quietSeconds = qBound(2, config.quietSeconds, 600);
    config.batchLimit = qBound(1, config.batchLimit, 5000);
    return config;
}

void LibraryWatcher::saveConfig(const Config &config)
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    settings.setValue(QStringLiteral("Enabled"), config.enabled);
    settings.setValue(QStringLiteral("PollSeconds"), config.pollSeconds);
    settings.setValue(QStringLiteral("QuietSeconds"), config.quietSeconds);
    settings.setValue(QStringLiteral("BatchLimit"), config.batchLimit);

    // remove() first: beginWriteArray leaves the tail of a longer previous
    // array behind, so deleting the last folder would not stick.
    settings.remove(QLatin1String(kArray));
    settings.beginWriteArray(QLatin1String(kArray), config.folders.size());
    for (int i = 0; i < config.folders.size(); ++i) {
        const Folder &folder = config.folders.at(i);
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("Path"), folder.path);
        settings.setValue(QStringLiteral("Destination"), destinationKey(folder.destination));
        settings.setValue(QStringLiteral("Recursive"), folder.recursive);
        settings.setValue(QStringLiteral("FolderGenres"), folder.folderGenres);
        settings.setValue(QStringLiteral("Genre"), folder.genre);
        settings.setValue(QStringLiteral("Enabled"), folder.enabled);
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();
}

void LibraryWatcher::setConfig(const Config &config)
{
    const bool wasEnabled = m_config.enabled;
    m_config = config;
    m_config.pollSeconds = qBound(10, m_config.pollSeconds, 3600);
    m_config.quietSeconds = qBound(2, m_config.quietSeconds, 600);
    m_config.batchLimit = qBound(1, m_config.batchLimit, 5000);
    saveConfig(m_config);

    // Folders that were edited may point somewhere else now, so nothing that
    // was noticed under the old settings is still worth remembering.
    m_seen.clear();
    m_refused.clear();

    rearmWatcher();
    restartTimer();

    if (m_config.enabled) {
        if (!wasEnabled)
            emit logMessage(tr("Watching %n folder(s) for new material",
                               nullptr, m_config.folders.size()));
        // Whatever is sitting in a folder that was just added should not have
        // to wait for the first poll.
        QTimer::singleShot(0, this, &LibraryWatcher::onPollTimeout);
    }

    emit stateChanged();
}

// ------------------------------------------------------------- destinations

QString LibraryWatcher::destinationLabel(Destination destination)
{
    switch (destination) {
    case Destination::Music:     return tr("Music");
    case Destination::Jingles:   return tr("Jingles");
    case Destination::Publicity: return tr("Publicities");
    case Destination::Programs:  return tr("Programs");
    }
    return tr("Music");
}

QString LibraryWatcher::destinationKey(Destination destination)
{
    switch (destination) {
    case Destination::Music:     return QStringLiteral("music");
    case Destination::Jingles:   return QStringLiteral("jingles");
    case Destination::Publicity: return QStringLiteral("pub");
    case Destination::Programs:  return QStringLiteral("programs");
    }
    return QStringLiteral("music");
}

LibraryWatcher::Destination LibraryWatcher::destinationFromKey(const QString &key)
{
    const QString k = key.trimmed().toLower();
    if (k == QLatin1String("jingles"))  return Destination::Jingles;
    if (k == QLatin1String("pub"))      return Destination::Publicity;
    if (k == QLatin1String("programs")) return Destination::Programs;
    return Destination::Music;
}

QString LibraryWatcher::destinationTable(Destination destination)
{
    switch (destination) {
    case Destination::Music:     return QStringLiteral("musics");
    case Destination::Jingles:   return QStringLiteral("jingles");
    case Destination::Publicity: return QStringLiteral("pub");
    case Destination::Programs:  return QStringLiteral("programs");
    }
    return QStringLiteral("musics");
}

// ----------------------------------------------------------------- watching

void LibraryWatcher::rearmWatcher()
{
    const QStringList watched = m_watcher->directories();
    if (!watched.isEmpty())
        m_watcher->removePaths(watched);

    if (!m_config.enabled)
        return;

    // Only the folders themselves, never the tree below them: a music library
    // is tens of thousands of directories and every one of them costs a file
    // descriptor. The poll is what covers the depths.
    QStringList paths;
    for (const Folder &folder : std::as_const(m_config.folders)) {
        if (!folder.enabled)
            continue;
        const QString path = folder.path.trimmed();
        if (!path.isEmpty() && QFileInfo(path).isDir() && !paths.contains(path))
            paths << path;
    }
    if (!paths.isEmpty())
        m_watcher->addPaths(paths);
}

void LibraryWatcher::restartTimer()
{
    m_pollTimer->stop();
    if (!m_config.enabled)
        return;
    m_pollTimer->setInterval(m_config.pollSeconds * 1000);
    m_pollTimer->start();
}

void LibraryWatcher::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    if (!m_config.enabled)
        return;
    // Wait out the copy that caused this rather than reading a growing file.
    m_settleTimer->start(m_config.quietSeconds * 1000);
}

void LibraryWatcher::onPollTimeout()
{
    if (m_config.enabled)
        scanNow();
}

// ----------------------------------------------------------------- scanning

int LibraryWatcher::scanNow()
{
    // A pass that takes longer than the poll interval must not be started
    // again on top of itself, and neither the settle timer nor "Scan now"
    // knows what the other is doing.
    if (m_scanning)
        return 0;
    m_scanning = true;

    // Genre spellings, read once per pass rather than once per file.
    m_knownGenres.clear();
    {
        QSqlQuery genreQuery(libraryDb());
        genreQuery.prepare(QStringLiteral("select name from genres1"));
        if (genreQuery.exec()) {
            while (genreQuery.next()) {
                const QString name = genreQuery.value(0).toString();
                m_knownGenres.insert(name.toLower(), name);
            }
        }
    }

    int budget = m_config.batchLimit;
    int added = 0;
    QStringList report;

    for (const Folder &folder : std::as_const(m_config.folders)) {
        if (!folder.enabled)
            continue;
        if (budget <= 0)
            break;

        const int taken = scanFolder(folder, budget, &report);
        added += taken;
        budget -= taken;
    }

    m_lastScan = QDateTime::currentDateTime();
    m_scanning = false;

    if (added > 0) {
        m_lastSummary = tr("%n file(s) added: %1", nullptr, added)
                            .arg(report.join(QStringLiteral(", ")));
        emit imported(added, m_lastSummary);
        emit logMessage(m_lastSummary);
        // More than one batch's worth landed at once; come back for the rest
        // instead of waiting out a full poll interval.
        if (budget <= 0)
            m_settleTimer->start(2000);
    } else {
        m_lastSummary = tr("Nothing new");
    }

    emit stateChanged();
    return added;
}

int LibraryWatcher::scanFolder(const Folder &folder, int budget, QStringList *report)
{
    const QString root = folder.path.trimmed();
    if (root.isEmpty())
        return 0;

    if (!QFileInfo(root).isDir()) {
        // A share that is not mounted this morning is the normal case, not a
        // fault worth a dialog. Said once per pass, in the log.
        qWarning() << "Watched folder is not there:" << root;
        return 0;
    }

    const QStringList found = AudioFormats::findAudioFiles(root, folder.recursive);

    int added = 0;
    bool holdback = false;

    for (const QString &path : found) {
        if (added >= budget) {
            holdback = true;
            break;
        }
        if (m_refused.contains(path))
            continue;
        if (alreadyInLibrary(path, folder.destination)) {
            // Known: forget the bookkeeping so a folder of 20,000 tracks does
            // not carry 20,000 entries around for the rest of the session.
            m_seen.remove(path);
            continue;
        }
        if (!hasSettled(path)) {
            holdback = true;
            continue;
        }

        const QString genre = folder.destination == Destination::Music
                                  ? genreForFile(path, folder)
                                  : QString();
        QString error;
        if (importFile(path, folder, genre, &error)) {
            ++added;
            m_seen.remove(path);
        } else {
            // Remembered so the same broken file is not retried, and its error
            // not logged, every single pass.
            m_refused.insert(path, error);
            qWarning() << "Watched folder could not add" << path << ":" << error;
            emit logMessage(tr("Could not add %1: %2")
                                .arg(QFileInfo(path).fileName(), error));
        }
    }

    if (added > 0 && report)
        *report << tr("%1 to %2").arg(added).arg(destinationLabel(folder.destination));

    // Something is still arriving (or was left over): look again sooner than
    // the poll interval would.
    if (holdback && !m_settleTimer->isActive())
        m_settleTimer->start(m_config.quietSeconds * 1000);

    return added;
}

bool LibraryWatcher::hasSettled(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isReadable())
        return false;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 size = info.size();
    const qint64 modTime = info.lastModified().toMSecsSinceEpoch();

    const Seen previous = m_seen.value(path);
    if (previous.size != size || previous.modTime != modTime) {
        // First sight, or it has changed since the last one: start the clock.
        m_seen.insert(path, Seen{size, modTime, nowMs});
        return false;
    }

    // Unchanged since we last looked. Both clocks have to have run out: the
    // file's own timestamp covers a copy that is still being written, and the
    // time since we first saw it covers a copy that preserved the original
    // timestamp (cp -p, rsync -t), where the file's age says nothing at all.
    const qint64 quietMs = qint64(m_config.quietSeconds) * 1000;
    return (nowMs - previous.firstSeenMs) >= quietMs
           && (nowMs - modTime) >= quietMs;
}

bool LibraryWatcher::alreadyInLibrary(const QString &path, Destination destination) const
{
    QSqlQuery query(libraryDb());
    query.prepare(QStringLiteral("select 1 from %1 where path = :path limit 1")
                      .arg(destinationTable(destination)));
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec()) {
        // Better to skip a file than to add a second copy of it.
        qWarning() << "Watched folder duplicate check failed:" << query.lastError().text();
        return true;
    }
    return query.next();
}

QString LibraryWatcher::genreForFile(const QString &path, const Folder &folder)
{
    QString genre = folder.genre.trimmed();

    if (folder.recursive && folder.folderGenres) {
        const QString category = folderCategory(QDir(folder.path), path);
        if (!category.isEmpty()) {
            const QString key = category.toLower();
            if (m_knownGenres.contains(key)) {
                // Keep the spelling the genre list already uses.
                return m_knownGenres.value(key);
            }
            // A category nobody has typed in yet: file under it, and put it in
            // the genre list so it can be picked and filtered like any other.
            QSqlQuery insertGenre(libraryDb());
            insertGenre.prepare(
                QStringLiteral("insert into genres1 (name) select :n where not exists "
                               "(select 1 from genres1 where name = :n collate nocase)"));
            insertGenre.bindValue(QStringLiteral(":n"), category);
            if (!insertGenre.exec()) {
                // The track still gets the folder's name; only the genre list
                // misses out.
                qWarning() << "Could not add genre" << category << ":"
                           << insertGenre.lastError().text();
            }
            m_knownGenres.insert(key, category);
            return category;
        }
    }

    if (!genre.isEmpty())
        return genre;

    // Nothing configured (a hand-edited xfb.conf): anything is better than the
    // empty string, which musics.genre1 refuses.
    if (!m_knownGenres.isEmpty())
        return m_knownGenres.constBegin().value();
    return tr("Unknown");
}

bool LibraryWatcher::importFile(const QString &path, const Folder &folder,
                                const QString &genre, QString *error)
{
    const QFileInfo info(path);
    QSqlQuery sql(libraryDb());

    if (folder.destination == Destination::Music) {
        QString artist;
        QString song;
        AudioFormats::splitArtistAndSong(info.fileName(), &artist, &song);
        if (song.isEmpty())
            song = QStringLiteral("-");

        // Columns named, never positional: the table has grown (bpm, loudness,
        // intro times) and a bare VALUES(...) has to fill every one of them.
        sql.prepare(QStringLiteral(
            "insert into musics (artist,song,genre1,genre2,country,published_date,"
            "path,time,played_times,last_played) "
            "values(:artist,:song,:g1,:g2,:country,:pub_date,:file,:time,:played,:last)"));
        sql.bindValue(QStringLiteral(":artist"), artist);
        sql.bindValue(QStringLiteral(":song"), song);
        sql.bindValue(QStringLiteral(":g1"), genre);
        sql.bindValue(QStringLiteral(":g2"), QString());
        sql.bindValue(QStringLiteral(":country"), QStringLiteral("Other country / language"));
        sql.bindValue(QStringLiteral(":pub_date"),
                      QDate::currentDate().toString(QStringLiteral("yyyy/MM/dd")));
        sql.bindValue(QStringLiteral(":file"), path);
        sql.bindValue(QStringLiteral(":time"), MediaDuration::forFile(path));
        sql.bindValue(QStringLiteral(":played"), 0);
        sql.bindValue(QStringLiteral(":last"), QString());
    } else {
        // programs elsewhere in XFB carry the file name with its extension —
        // the scheduler matches on it — while jingles and adverts are named
        // the way the operator would type them.
        const QString name = folder.destination == Destination::Programs
                                 ? info.fileName()
                                 : info.baseName();
        sql.prepare(QStringLiteral("insert into %1 (name,path) values(:name,:path)")
                        .arg(destinationTable(folder.destination)));
        sql.bindValue(QStringLiteral(":name"), name);
        sql.bindValue(QStringLiteral(":path"), path);
    }

    if (sql.exec())
        return true;

    if (error)
        *error = sql.lastError().text();
    return false;
}

int LibraryWatcher::pendingCount(const Folder &folder)
{
    const QString root = folder.path.trimmed();
    if (root.isEmpty() || !QFileInfo(root).isDir())
        return -1;   // nothing to say: the folder is not there

    const QStringList found = AudioFormats::findAudioFiles(root, folder.recursive);
    const QString table = destinationTable(folder.destination);

    int pending = 0;
    QSqlQuery query(libraryDb());
    query.prepare(QStringLiteral("select 1 from %1 where path = :path limit 1").arg(table));
    for (const QString &path : found) {
        query.bindValue(QStringLiteral(":path"), path);
        if (query.exec() && !query.next())
            ++pending;
    }
    return pending;
}
