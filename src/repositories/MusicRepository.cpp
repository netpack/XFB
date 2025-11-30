#include "MusicRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QFileInfo>
#include <QDirIterator>
#include <QMimeType>
#include <QDateTime>
#include <QMutexLocker>
#include <QCoreApplication>

MusicRepository::MusicRepository(QSqlDatabase& database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
    // Initialize mime database for file type detection
    // Cache will be initialized on first use
}

MusicRepository::~MusicRepository()
{
    // Prepared queries will be automatically cleaned up
}

bool MusicRepository::addMusic(const MusicItem& music)
{
    QMutexLocker locker(&m_mutex);
    
    // Validate input
    QString validationError = validateMusicItem(music);
    if (!validationError.isEmpty()) {
        logError("addMusic", validationError);
        emit operationError("addMusic", validationError);
        return false;
    }
    
    // Check if path already exists (inline to avoid recursive mutex lock)
    {
        QSqlQuery checkQuery(m_database);
        checkQuery.prepare("SELECT COUNT(*) as count FROM musics WHERE path = ?");
        checkQuery.addBindValue(sanitizePath(music.path));
        
        if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
            QString error = QString("Music file already exists in database: %1").arg(music.path);
            logError("addMusic", error);
            emit operationError("addMusic", error);
            return false;
        }
    }
    
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO musics (artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(music.artist);
    query.addBindValue(music.song);
    query.addBindValue(music.genre1);
    query.addBindValue(music.genre2);
    query.addBindValue(music.country);
    query.addBindValue(music.publishedDate);
    query.addBindValue(sanitizePath(music.path));
    query.addBindValue(music.time);
    query.addBindValue(music.playedTimes);
    query.addBindValue(music.lastPlayed);
    
    if (!executeQuery(query, "addMusic")) {
        return false;
    }
    
    // Create a copy with the new ID for the signal
    MusicItem addedMusic = music;
    addedMusic.id = query.lastInsertId().toInt();
    
    emit musicAdded(addedMusic);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

bool MusicRepository::updateMusic(const MusicItem& music)
{
    QMutexLocker locker(&m_mutex);
    
    if (music.id <= 0) {
        QString error = "Invalid music ID for update operation";
        logError("updateMusic", error);
        emit operationError("updateMusic", error);
        return false;
    }
    
    // Validate input
    QString validationError = validateMusicItem(music);
    if (!validationError.isEmpty()) {
        logError("updateMusic", validationError);
        emit operationError("updateMusic", validationError);
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare("UPDATE musics SET artist = ?, song = ?, genre1 = ?, genre2 = ?, "
                  "country = ?, published_date = ?, path = ?, time = ?, "
                  "played_times = ?, last_played = ? WHERE id = ?");
    
    query.addBindValue(music.artist);
    query.addBindValue(music.song);
    query.addBindValue(music.genre1);
    query.addBindValue(music.genre2);
    query.addBindValue(music.country);
    query.addBindValue(music.publishedDate);
    query.addBindValue(sanitizePath(music.path));
    query.addBindValue(music.time);
    query.addBindValue(music.playedTimes);
    query.addBindValue(music.lastPlayed);
    query.addBindValue(music.id);
    
    if (!executeQuery(query, "updateMusic")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No music item found with ID: %1").arg(music.id);
        logError("updateMusic", error);
        emit operationError("updateMusic", error);
        return false;
    }
    
    emit musicUpdated(music);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

bool MusicRepository::deleteMusic(int musicId)
{
    QMutexLocker locker(&m_mutex);
    
    if (musicId <= 0) {
        QString error = "Invalid music ID for delete operation";
        logError("deleteMusic", error);
        emit operationError("deleteMusic", error);
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM musics WHERE id = ?");
    query.addBindValue(musicId);
    
    if (!executeQuery(query, "deleteMusic")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No music item found with ID: %1").arg(musicId);
        logError("deleteMusic", error);
        emit operationError("deleteMusic", error);
        return false;
    }
    
    emit musicDeleted(musicId);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

MusicItem MusicRepository::getMusicById(int musicId)
{
    QMutexLocker locker(&m_mutex);
    
    if (musicId <= 0) {
        return MusicItem(); // Invalid item
    }
    
    QSqlQuery query(m_database);
    query.prepare("SELECT id, artist, song, genre1, genre2, country, published_date, "
                  "path, time, played_times, last_played FROM musics WHERE id = ?");
    query.addBindValue(musicId);
    
    if (!executeQuery(query, "getMusicById")) {
        return MusicItem();
    }
    
    if (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["artist"] = query.value("artist");
        map["song"] = query.value("song");
        map["genre1"] = query.value("genre1");
        map["genre2"] = query.value("genre2");
        map["country"] = query.value("country");
        map["published_date"] = query.value("published_date");
        map["path"] = query.value("path");
        map["time"] = query.value("time");
        map["played_times"] = query.value("played_times");
        map["last_played"] = query.value("last_played");
        
        return MusicItem::fromVariantMap(map);
    }
    
    return MusicItem(); // Not found
}

QList<MusicItem> MusicRepository::getAllMusic(int limit, int offset)
{
    QMutexLocker locker(&m_mutex);
    
    QString queryString = "SELECT id, artist, song, genre1, genre2, country, published_date, "
                         "path, time, played_times, last_played FROM musics ORDER BY artist, song";
    
    if (limit > 0) {
        queryString += QString(" LIMIT %1").arg(limit);
        if (offset > 0) {
            queryString += QString(" OFFSET %1").arg(offset);
        }
    }
    
    QSqlQuery query(m_database);
    query.prepare(queryString);
    
    QList<MusicItem> results;
    
    if (!executeQuery(query, "getAllMusic")) {
        return results;
    }
    
    while (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["artist"] = query.value("artist");
        map["song"] = query.value("song");
        map["genre1"] = query.value("genre1");
        map["genre2"] = query.value("genre2");
        map["country"] = query.value("country");
        map["published_date"] = query.value("published_date");
        map["path"] = query.value("path");
        map["time"] = query.value("time");
        map["played_times"] = query.value("played_times");
        map["last_played"] = query.value("last_played");
        
        results.append(MusicItem::fromVariantMap(map));
    }
    
    return results;
}

QList<MusicItem> MusicRepository::searchMusic(const SearchCriteria& criteria)
{
    QMutexLocker locker(&m_mutex);
    
    QString queryString = "SELECT id, artist, song, genre1, genre2, country, published_date, "
                         "path, time, played_times, last_played FROM musics WHERE 1=1";
    
    QVariantList bindValues;
    
    // Build WHERE clause based on criteria
    if (!criteria.artist.isEmpty()) {
        queryString += " AND artist LIKE ?";
        bindValues.append(QString("%%1%").arg(criteria.artist));
    }
    
    if (!criteria.song.isEmpty()) {
        queryString += " AND song LIKE ?";
        bindValues.append(QString("%%1%").arg(criteria.song));
    }
    
    if (!criteria.genre1.isEmpty()) {
        queryString += " AND genre1 LIKE ?";
        bindValues.append(QString("%%1%").arg(criteria.genre1));
    }
    
    if (!criteria.genre2.isEmpty()) {
        queryString += " AND genre2 LIKE ?";
        bindValues.append(QString("%%1%").arg(criteria.genre2));
    }
    
    if (!criteria.country.isEmpty()) {
        queryString += " AND country LIKE ?";
        bindValues.append(QString("%%1%").arg(criteria.country));
    }
    
    if (!criteria.searchText.isEmpty()) {
        queryString += " AND (artist LIKE ? OR song LIKE ? OR genre1 LIKE ? OR genre2 LIKE ?)";
        QString searchPattern = QString("%%1%").arg(criteria.searchText);
        bindValues.append(searchPattern);
        bindValues.append(searchPattern);
        bindValues.append(searchPattern);
        bindValues.append(searchPattern);
    }
    
    // Add ORDER BY clause
    queryString += QString(" ORDER BY %1").arg(criteria.orderBy);
    if (!criteria.ascending) {
        queryString += " DESC";
    }
    
    // Add LIMIT and OFFSET
    if (criteria.limit > 0) {
        queryString += QString(" LIMIT %1").arg(criteria.limit);
        if (criteria.offset > 0) {
            queryString += QString(" OFFSET %1").arg(criteria.offset);
        }
    }
    
    QSqlQuery query(m_database);
    query.prepare(queryString);
    
    // Bind all values
    for (const QVariant& value : bindValues) {
        query.addBindValue(value);
    }
    
    QList<MusicItem> results;
    
    if (!executeQuery(query, "searchMusic")) {
        return results;
    }
    
    while (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["artist"] = query.value("artist");
        map["song"] = query.value("song");
        map["genre1"] = query.value("genre1");
        map["genre2"] = query.value("genre2");
        map["country"] = query.value("country");
        map["published_date"] = query.value("published_date");
        map["path"] = query.value("path");
        map["time"] = query.value("time");
        map["played_times"] = query.value("played_times");
        map["last_played"] = query.value("last_played");
        
        results.append(MusicItem::fromVariantMap(map));
    }
    
    return results;
}

QList<MusicItem> MusicRepository::getMusicByGenre(const QString& genre, bool useGenre2)
{
    SearchCriteria criteria;
    criteria.genre1 = genre;
    if (useGenre2) {
        // We'll need to modify the search to include genre2
        // For now, let's implement a specific query
        QMutexLocker locker(&m_mutex);
        
        QSqlQuery query(m_database);
        query.prepare("SELECT id, artist, song, genre1, genre2, country, published_date, "
                      "path, time, played_times, last_played FROM musics "
                      "WHERE genre1 LIKE ? OR genre2 LIKE ? ORDER BY artist, song");
        
        QString searchPattern = QString("%%1%").arg(genre);
        query.addBindValue(searchPattern);
        query.addBindValue(searchPattern);
        
        QList<MusicItem> results;
        
        if (!executeQuery(query, "getMusicByGenre")) {
            return results;
        }
        
        while (query.next()) {
            QVariantMap map;
            map["id"] = query.value("id");
            map["artist"] = query.value("artist");
            map["song"] = query.value("song");
            map["genre1"] = query.value("genre1");
            map["genre2"] = query.value("genre2");
            map["country"] = query.value("country");
            map["published_date"] = query.value("published_date");
            map["path"] = query.value("path");
            map["time"] = query.value("time");
            map["played_times"] = query.value("played_times");
            map["last_played"] = query.value("last_played");
            
            results.append(MusicItem::fromVariantMap(map));
        }
        
        return results;
    }
    
    return searchMusic(criteria);
}

QList<MusicItem> MusicRepository::getMusicByArtist(const QString& artist)
{
    SearchCriteria criteria;
    criteria.artist = artist;
    return searchMusic(criteria);
}

int MusicRepository::addMusicBatch(const QList<MusicItem>& musicList)
{
    if (musicList.isEmpty()) {
        return 0;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Start transaction
    if (!m_database.transaction()) {
        logError("addMusicBatch", "Failed to start transaction");
        emit operationError("addMusicBatch", "Failed to start transaction");
        return 0;
    }
    
    int successCount = 0;
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO musics (artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    for (const MusicItem& music : musicList) {
        // Validate each item
        QString validationError = validateMusicItem(music);
        if (!validationError.isEmpty()) {
            qDebug() << "Skipping invalid music item:" << validationError;
            continue;
        }
        
        // Check if path already exists (inline to avoid recursive mutex lock)
        {
            QSqlQuery checkQuery(m_database);
            checkQuery.prepare("SELECT COUNT(*) as count FROM musics WHERE path = ?");
            checkQuery.addBindValue(sanitizePath(music.path));
            
            if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
                qDebug() << "Skipping duplicate path:" << music.path;
                continue;
            }
        }
        
        query.addBindValue(music.artist);
        query.addBindValue(music.song);
        query.addBindValue(music.genre1);
        query.addBindValue(music.genre2);
        query.addBindValue(music.country);
        query.addBindValue(music.publishedDate);
        query.addBindValue(sanitizePath(music.path));
        query.addBindValue(music.time);
        query.addBindValue(music.playedTimes);
        query.addBindValue(music.lastPlayed);
        
        if (query.exec()) {
            successCount++;
            
            // Emit signal for each added item
            MusicItem addedMusic = music;
            addedMusic.id = query.lastInsertId().toInt();
            emit musicAdded(addedMusic);
        } else {
            logError("addMusicBatch", QString("Failed to add music item: %1 - %2")
                    .arg(music.artist, music.song), query.lastError().text());
        }
    }
    
    // Commit transaction
    if (!m_database.commit()) {
        m_database.rollback();
        logError("addMusicBatch", "Failed to commit transaction");
        emit operationError("addMusicBatch", "Failed to commit transaction");
        return 0;
    }
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return successCount;
}

int MusicRepository::importFromDirectory(const QString& directoryPath, bool recursive, const QStringList& supportedExtensions)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        QString error = QString("Directory does not exist: %1").arg(directoryPath);
        logError("importFromDirectory", error);
        emit operationError("importFromDirectory", error);
        return 0;
    }
    
    QStringList extensions = supportedExtensions.isEmpty() ? getDefaultSupportedExtensions() : supportedExtensions;
    
    // Get all audio files
    QStringList audioFiles;
    QDirIterator::IteratorFlags flags = recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(directoryPath, flags);
    
    while (it.hasNext()) {
        QString filePath = it.next();
        if (it.fileInfo().isFile() && isSupportedAudioFile(filePath, extensions)) {
            audioFiles.append(filePath);
        }
    }
    
    if (audioFiles.isEmpty()) {
        emit importCompleted(0, 0);
        return 0;
    }
    
    // Process files in batches
    QList<MusicItem> musicBatch;
    int processed = 0;
    int imported = 0;
    const int batchSize = 50; // Process in batches of 50
    
    for (const QString& filePath : audioFiles) {
        emit importProgress(processed, audioFiles.size(), filePath);
        
        // Skip if already exists (check without additional mutex lock)
        {
            QSqlQuery checkQuery(m_database);
            checkQuery.prepare("SELECT COUNT(*) as count FROM musics WHERE path = ?");
            checkQuery.addBindValue(sanitizePath(filePath));
            
            if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
                processed++;
                continue;
            }
        }
        
        // Extract metadata
        MusicItem music = extractMetadata(filePath);
        if (music.isValid()) {
            musicBatch.append(music);
        }
        
        processed++;
        
        // Process batch when full or at end
        if (musicBatch.size() >= batchSize || processed == audioFiles.size()) {
            if (!musicBatch.isEmpty()) {
                imported += addMusicBatch(musicBatch);
                musicBatch.clear();
            }
        }
        
        // Allow event processing
        QCoreApplication::processEvents();
    }
    
    emit importCompleted(imported, audioFiles.size());
    return imported;
}

bool MusicRepository::incrementPlayCount(int musicId)
{
    QMutexLocker locker(&m_mutex);
    
    if (musicId <= 0) {
        QString error = "Invalid music ID for play count increment";
        logError("incrementPlayCount", error);
        emit operationError("incrementPlayCount", error);
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare("UPDATE musics SET played_times = played_times + 1, last_played = ? WHERE id = ?");
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(musicId);
    
    if (!executeQuery(query, "incrementPlayCount")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No music item found with ID: %1").arg(musicId);
        logError("incrementPlayCount", error);
        emit operationError("incrementPlayCount", error);
        return false;
    }
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

MusicRepository::MusicStats MusicRepository::getStatistics()
{
    QMutexLocker statsLocker(&m_statsMutex);
    
    // Check if cached stats are still valid
    if (m_statsLastUpdated.isValid() && 
        m_statsLastUpdated.msecsTo(QDateTime::currentDateTime()) < STATS_CACHE_DURATION_MS) {
        return m_cachedStats;
    }
    
    statsLocker.unlock();
    QMutexLocker locker(&m_mutex);
    
    MusicStats stats;
    
    // Get total tracks
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) as total FROM musics");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalTracks = query.value("total").toInt();
    }
    
    // Get total artists
    query.prepare("SELECT COUNT(DISTINCT artist) as total FROM musics");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalArtists = query.value("total").toInt();
    }
    
    // Get total genres
    query.prepare("SELECT COUNT(DISTINCT genre1) as total FROM musics WHERE genre1 IS NOT NULL AND genre1 != ''");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalGenres = query.value("total").toInt();
    }
    
    // Get most played track
    query.prepare("SELECT id, song, artist, played_times FROM musics WHERE played_times = (SELECT MAX(played_times) FROM musics) LIMIT 1");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.mostPlayedTrackId = query.value("id").toInt();
        stats.mostPlayedTrackTitle = QString("%1 - %2").arg(query.value("artist").toString(), query.value("song").toString());
    }
    
    // Get total plays
    query.prepare("SELECT SUM(played_times) as total FROM musics");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalPlays = query.value("total").toInt();
    }
    
    locker.unlock();
    
    // Cache the results
    statsLocker.relock();
    m_cachedStats = stats;
    m_statsLastUpdated = QDateTime::currentDateTime();
    
    return stats;
}

QStringList MusicRepository::getAllArtists()
{
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT artist FROM musics WHERE artist IS NOT NULL AND artist != '' ORDER BY artist");
    
    QStringList artists;
    
    if (!executeQuery(query, "getAllArtists")) {
        return artists;
    }
    
    while (query.next()) {
        artists.append(query.value("artist").toString());
    }
    
    return artists;
}

QStringList MusicRepository::getAllGenres(bool includeGenre2)
{
    QMutexLocker locker(&m_mutex);
    
    QStringList genres;
    
    // Get genres from genre1
    QSqlQuery query(m_database);
    query.prepare("SELECT DISTINCT genre1 FROM musics WHERE genre1 IS NOT NULL AND genre1 != '' ORDER BY genre1");
    
    if (executeQuery(query, "getAllGenres")) {
        while (query.next()) {
            QString genre = query.value("genre1").toString();
            if (!genres.contains(genre)) {
                genres.append(genre);
            }
        }
    }
    
    // Get genres from genre2 if requested
    if (includeGenre2) {
        query.prepare("SELECT DISTINCT genre2 FROM musics WHERE genre2 IS NOT NULL AND genre2 != '' ORDER BY genre2");
        
        if (executeQuery(query, "getAllGenres")) {
            while (query.next()) {
                QString genre = query.value("genre2").toString();
                if (!genres.contains(genre)) {
                    genres.append(genre);
                }
            }
        }
    }
    
    genres.sort();
    return genres;
}

bool MusicRepository::pathExists(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) as count FROM musics WHERE path = ?");
    query.addBindValue(sanitizePath(filePath));
    
    if (query.exec() && query.next()) {
        return query.value("count").toInt() > 0;
    }
    
    if (query.lastError().isValid()) {
        logError("pathExists", query.lastError().text(), query.lastQuery());
    }
    
    return false;
}

QString MusicRepository::validateMusicItem(const MusicItem& music)
{
    if (music.artist.isEmpty()) {
        return "Artist name is required";
    }
    
    if (music.song.isEmpty()) {
        return "Song title is required";
    }
    
    if (music.path.isEmpty()) {
        return "File path is required";
    }
    
    // Validate file path
    QFileInfo fileInfo(music.path);
    if (!fileInfo.exists()) {
        return QString("File does not exist: %1").arg(music.path);
    }
    
    if (!fileInfo.isFile()) {
        return QString("Path is not a file: %1").arg(music.path);
    }
    
    // Check if it's a supported audio file (only check extension for validation)
    QFileInfo pathInfo(music.path);
    if (!getDefaultSupportedExtensions().contains(pathInfo.suffix().toLower(), Qt::CaseInsensitive)) {
        return QString("Unsupported audio file format: %1").arg(music.path);
    }
    
    return QString(); // Valid
}

MusicItem MusicRepository::extractMetadata(const QString& filePath)
{
    MusicItem music;
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return music;
    }
    
    music.path = filePath;
    
    // Extract basic information from filename if no metadata available
    QString baseName = fileInfo.baseName();
    
    // Try to parse "Artist - Song" format
    if (baseName.contains(" - ")) {
        QStringList parts = baseName.split(" - ", Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            music.artist = parts[0].trimmed();
            music.song = parts[1].trimmed();
        } else {
            music.artist = "Unknown Artist";
            music.song = baseName;
        }
    } else {
        music.artist = "Unknown Artist";
        music.song = baseName;
    }
    
    // Set default values
    music.genre1 = "Unknown";
    music.playedTimes = 0;
    music.lastPlayed = "";
    
    // TODO: In a real implementation, you would use a library like TagLib
    // to extract actual metadata from audio files. For now, we'll use basic file info.
    
    return music;
}

bool MusicRepository::isSupportedAudioFile(const QString& filePath, const QStringList& supportedExtensions)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    return supportedExtensions.contains(extension, Qt::CaseInsensitive);
}

QStringList MusicRepository::getDefaultSupportedExtensions()
{
    return QStringList() << "mp3" << "wav" << "flac" << "ogg" << "m4a" << "aac" << "wma";
}

QString MusicRepository::sanitizePath(const QString& path)
{
    // Convert to native separators and clean up the path
    return QDir::cleanPath(QDir::toNativeSeparators(path));
}

void MusicRepository::logError(const QString& operation, const QString& error, const QString& query)
{
    QString logMessage = QString("MusicRepository::%1 - %2").arg(operation, error);
    if (!query.isEmpty()) {
        logMessage += QString(" (Query: %1)").arg(query);
    }
    
    qWarning() << logMessage;
}

bool MusicRepository::executeQuery(QSqlQuery& query, const QString& operation)
{
    if (!query.exec()) {
        QString error = QString("SQL Error: %1").arg(query.lastError().text());
        logError(operation, error, query.lastQuery());
        emit operationError(operation, error);
        return false;
    }
    
    return true;
}