#include "PlaylistRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QDir>
#include <QFileInfo>

PlaylistRepository::PlaylistRepository(QSqlDatabase& database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

PlaylistRepository::~PlaylistRepository()
{
}

bool PlaylistRepository::addPlaylist(const PlaylistItem& playlist)
{
    QMutexLocker locker(&m_mutex);
    
    // Validate input
    QString validationError = validatePlaylistItem(playlist);
    if (!validationError.isEmpty()) {
        logError("addPlaylist", validationError);
        emit operationError("addPlaylist", validationError);
        return false;
    }
    
    // Check if playlist name already exists (inline to avoid recursive mutex lock)
    {
        QSqlQuery checkQuery(m_database);
        checkQuery.prepare("SELECT COUNT(*) as count FROM programs WHERE name = ?");
        checkQuery.addBindValue(sanitizeName(playlist.name));
        
        if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
            QString error = QString("Playlist already exists: %1").arg(playlist.name);
            logError("addPlaylist", error);
            emit operationError("addPlaylist", error);
            return false;
        }
    }
    
    // Check if path already exists (if path is provided)
    if (!playlist.path.isEmpty()) {
        QSqlQuery checkQuery(m_database);
        checkQuery.prepare("SELECT COUNT(*) as count FROM programs WHERE path = ?");
        checkQuery.addBindValue(sanitizePath(playlist.path));
        
        if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
            QString error = QString("Playlist path already exists: %1").arg(playlist.path);
            logError("addPlaylist", error);
            emit operationError("addPlaylist", error);
            return false;
        }
    }
    
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO programs (name, path) VALUES (?, ?)");
    query.addBindValue(sanitizeName(playlist.name));
    query.addBindValue(sanitizePath(playlist.path));
    
    if (!executeQuery(query, "addPlaylist")) {
        return false;
    }
    
    // Create a copy with the new ID for the signal
    PlaylistItem addedPlaylist = playlist;
    addedPlaylist.id = query.lastInsertId().toInt();
    
    emit playlistAdded(addedPlaylist);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

bool PlaylistRepository::updatePlaylist(const PlaylistItem& playlist)
{
    QMutexLocker locker(&m_mutex);
    
    if (playlist.id <= 0) {
        QString error = "Invalid playlist ID for update operation";
        logError("updatePlaylist", error);
        emit operationError("updatePlaylist", error);
        return false;
    }
    
    // Validate input
    QString validationError = validatePlaylistItem(playlist);
    if (!validationError.isEmpty()) {
        logError("updatePlaylist", validationError);
        emit operationError("updatePlaylist", validationError);
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare("UPDATE programs SET name = ?, path = ? WHERE id = ?");
    query.addBindValue(sanitizeName(playlist.name));
    query.addBindValue(sanitizePath(playlist.path));
    query.addBindValue(playlist.id);
    
    if (!executeQuery(query, "updatePlaylist")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No playlist found with ID: %1").arg(playlist.id);
        logError("updatePlaylist", error);
        emit operationError("updatePlaylist", error);
        return false;
    }
    
    emit playlistUpdated(playlist);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

bool PlaylistRepository::deletePlaylist(int playlistId)
{
    QMutexLocker locker(&m_mutex);
    
    if (playlistId <= 0) {
        QString error = "Invalid playlist ID for delete operation";
        logError("deletePlaylist", error);
        emit operationError("deletePlaylist", error);
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM programs WHERE id = ?");
    query.addBindValue(playlistId);
    
    if (!executeQuery(query, "deletePlaylist")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No playlist found with ID: %1").arg(playlistId);
        logError("deletePlaylist", error);
        emit operationError("deletePlaylist", error);
        return false;
    }
    
    emit playlistDeleted(playlistId);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

PlaylistItem PlaylistRepository::getPlaylistById(int playlistId)
{
    QMutexLocker locker(&m_mutex);
    
    if (playlistId <= 0) {
        return PlaylistItem(); // Invalid item
    }
    
    QSqlQuery query(m_database);
    query.prepare("SELECT id, name, path FROM programs WHERE id = ?");
    query.addBindValue(playlistId);
    
    if (!executeQuery(query, "getPlaylistById")) {
        return PlaylistItem();
    }
    
    if (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["name"] = query.value("name");
        map["path"] = query.value("path");
        
        return PlaylistItem::fromVariantMap(map);
    }
    
    return PlaylistItem(); // Not found
}

QList<PlaylistItem> PlaylistRepository::getAllPlaylists(int limit, int offset)
{
    QMutexLocker locker(&m_mutex);
    
    QString queryString = "SELECT id, name, path FROM programs ORDER BY name";
    
    if (limit > 0) {
        queryString += QString(" LIMIT %1").arg(limit);
        if (offset > 0) {
            queryString += QString(" OFFSET %1").arg(offset);
        }
    }
    
    QSqlQuery query(m_database);
    query.prepare(queryString);
    
    QList<PlaylistItem> results;
    
    if (!executeQuery(query, "getAllPlaylists")) {
        return results;
    }
    
    while (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["name"] = query.value("name");
        map["path"] = query.value("path");
        
        results.append(PlaylistItem::fromVariantMap(map));
    }
    
    return results;
}

QList<PlaylistItem> PlaylistRepository::searchPlaylists(const SearchCriteria& criteria)
{
    QMutexLocker locker(&m_mutex);
    
    QString queryString = "SELECT id, name, path FROM programs WHERE 1=1";
    QVariantList bindValues;
    
    // Build WHERE clause based on criteria
    if (!criteria.name.isEmpty()) {
        if (criteria.exactMatch) {
            queryString += " AND name = ?";
            bindValues.append(criteria.name);
        } else {
            queryString += " AND name LIKE ?";
            bindValues.append(QString("%%1%").arg(criteria.name));
        }
    }
    
    if (!criteria.path.isEmpty()) {
        if (criteria.exactMatch) {
            queryString += " AND path = ?";
            bindValues.append(criteria.path);
        } else {
            queryString += " AND path LIKE ?";
            bindValues.append(QString("%%1%").arg(criteria.path));
        }
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
    
    QList<PlaylistItem> results;
    
    if (!executeQuery(query, "searchPlaylists")) {
        return results;
    }
    
    while (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["name"] = query.value("name");
        map["path"] = query.value("path");
        
        results.append(PlaylistItem::fromVariantMap(map));
    }
    
    return results;
}

QList<PlaylistItem> PlaylistRepository::getPlaylistsByName(const QString& name)
{
    SearchCriteria criteria;
    criteria.name = name;
    criteria.exactMatch = false;
    
    return searchPlaylists(criteria);
}

int PlaylistRepository::addPlaylistBatch(const QList<PlaylistItem>& playlistList)
{
    if (playlistList.isEmpty()) {
        return 0;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Start transaction
    if (!m_database.transaction()) {
        logError("addPlaylistBatch", "Failed to start transaction");
        emit operationError("addPlaylistBatch", "Failed to start transaction");
        return 0;
    }
    
    int successCount = 0;
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO programs (name, path) VALUES (?, ?)");
    
    for (const PlaylistItem& playlist : playlistList) {
        // Validate each item
        QString validationError = validatePlaylistItem(playlist);
        if (!validationError.isEmpty()) {
            qDebug() << "Skipping invalid playlist item:" << validationError;
            continue;
        }
        
        // Check if name already exists (inline to avoid recursive mutex lock)
        {
            QSqlQuery checkQuery(m_database);
            checkQuery.prepare("SELECT COUNT(*) as count FROM programs WHERE name = ?");
            checkQuery.addBindValue(sanitizeName(playlist.name));
            
            if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
                qDebug() << "Skipping duplicate playlist name:" << playlist.name;
                continue;
            }
        }
        
        // Check if path already exists (if path is provided)
        if (!playlist.path.isEmpty()) {
            QSqlQuery checkQuery(m_database);
            checkQuery.prepare("SELECT COUNT(*) as count FROM programs WHERE path = ?");
            checkQuery.addBindValue(sanitizePath(playlist.path));
            
            if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
                qDebug() << "Skipping duplicate playlist path:" << playlist.path;
                continue;
            }
        }
        
        query.addBindValue(sanitizeName(playlist.name));
        query.addBindValue(sanitizePath(playlist.path));
        
        if (query.exec()) {
            successCount++;
            
            // Emit signal for each added item
            PlaylistItem addedPlaylist = playlist;
            addedPlaylist.id = query.lastInsertId().toInt();
            emit playlistAdded(addedPlaylist);
        } else {
            logError("addPlaylistBatch", QString("Failed to add playlist: %1")
                    .arg(playlist.name), query.lastError().text());
        }
    }
    
    // Commit transaction
    if (!m_database.commit()) {
        m_database.rollback();
        logError("addPlaylistBatch", "Failed to commit transaction");
        emit operationError("addPlaylistBatch", "Failed to commit transaction");
        return 0;
    }
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return successCount;
}

bool PlaylistRepository::playlistExists(const QString& name)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) as count FROM programs WHERE name = ?");
    query.addBindValue(sanitizeName(name));
    
    if (query.exec() && query.next()) {
        return query.value("count").toInt() > 0;
    }
    
    if (query.lastError().isValid()) {
        logError("playlistExists", query.lastError().text(), query.lastQuery());
    }
    
    return false;
}

bool PlaylistRepository::pathExists(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) as count FROM programs WHERE path = ?");
    query.addBindValue(sanitizePath(path));
    
    if (query.exec() && query.next()) {
        return query.value("count").toInt() > 0;
    }
    
    if (query.lastError().isValid()) {
        logError("pathExists", query.lastError().text(), query.lastQuery());
    }
    
    return false;
}

PlaylistRepository::PlaylistStats PlaylistRepository::getStatistics()
{
    QMutexLocker statsLocker(&m_statsMutex);
    
    // Check if cached stats are still valid
    if (m_statsLastUpdated.isValid() && 
        m_statsLastUpdated.msecsTo(QDateTime::currentDateTime()) < STATS_CACHE_DURATION_MS) {
        return m_cachedStats;
    }
    
    statsLocker.unlock();
    QMutexLocker locker(&m_mutex);
    
    PlaylistStats stats;
    
    // Get total playlists
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) as total FROM programs");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalPlaylists = query.value("total").toInt();
    }
    
    // Future extensions can add more statistics here
    // For now, we'll set some default values
    stats.totalTracks = 0;
    stats.lastCreated = QDateTime();
    stats.lastUpdated = QDateTime();
    
    locker.unlock();
    
    // Cache the results
    statsLocker.relock();
    m_cachedStats = stats;
    m_statsLastUpdated = QDateTime::currentDateTime();
    
    return stats;
}

QStringList PlaylistRepository::getAllPlaylistNames()
{
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM programs WHERE name IS NOT NULL AND name != '' ORDER BY name");
    
    QStringList names;
    
    if (!executeQuery(query, "getAllPlaylistNames")) {
        return names;
    }
    
    while (query.next()) {
        names.append(query.value("name").toString());
    }
    
    return names;
}

QString PlaylistRepository::validatePlaylistItem(const PlaylistItem& playlist)
{
    if (playlist.name.isEmpty()) {
        return "Playlist name is required";
    }
    
    if (playlist.name.length() > 255) {
        return "Playlist name is too long (maximum 255 characters)";
    }
    
    // Validate path if provided
    if (!playlist.path.isEmpty()) {
        if (playlist.path.length() > 500) {
            return "Playlist path is too long (maximum 500 characters)";
        }
        
        // Basic path validation - check if it contains invalid characters
        QString invalidChars = "<>:\"|?*";
        for (const QChar& ch : invalidChars) {
            if (playlist.path.contains(ch)) {
                return QString("Playlist path contains invalid character: %1").arg(ch);
            }
        }
    }
    
    return QString(); // Valid
}

// Future playlist-track functionality
bool PlaylistRepository::addTrackToPlaylist(int playlistId, int musicId, int position)
{
    Q_UNUSED(playlistId)
    Q_UNUSED(musicId)
    Q_UNUSED(position)
    
    // This is a placeholder for future functionality
    // Would require creating a playlist_tracks table
    logError("addTrackToPlaylist", "Feature not yet implemented");
    emit operationError("addTrackToPlaylist", "Feature not yet implemented");
    return false;
}

bool PlaylistRepository::removeTrackFromPlaylist(int playlistId, int musicId)
{
    Q_UNUSED(playlistId)
    Q_UNUSED(musicId)
    
    // This is a placeholder for future functionality
    logError("removeTrackFromPlaylist", "Feature not yet implemented");
    emit operationError("removeTrackFromPlaylist", "Feature not yet implemented");
    return false;
}

QList<PlaylistTrack> PlaylistRepository::getPlaylistTracks(int playlistId)
{
    Q_UNUSED(playlistId)
    
    // This is a placeholder for future functionality
    return QList<PlaylistTrack>();
}

bool PlaylistRepository::initializePlaylistTables()
{
    // The programs table already exists, so we don't need to create it
    // Future playlist-track functionality would create additional tables here
    m_tablesInitialized = true;
    return true;
}

QString PlaylistRepository::sanitizeName(const QString& name)
{
    return name.trimmed();
}

QString PlaylistRepository::sanitizePath(const QString& path)
{
    if (path.isEmpty()) {
        return path;
    }
    
    // Convert to native separators and clean up the path
    return QDir::cleanPath(QDir::toNativeSeparators(path));
}

void PlaylistRepository::logError(const QString& operation, const QString& error, const QString& query)
{
    QString logMessage = QString("PlaylistRepository::%1 - %2").arg(operation, error);
    if (!query.isEmpty()) {
        logMessage += QString(" (Query: %1)").arg(query);
    }
    
    qWarning() << logMessage;
}

bool PlaylistRepository::executeQuery(QSqlQuery& query, const QString& operation)
{
    if (!query.exec()) {
        QString error = QString("SQL Error: %1").arg(query.lastError().text());
        logError(operation, error, query.lastQuery());
        emit operationError(operation, error);
        return false;
    }
    
    return true;
}