#include "GenreRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>

GenreRepository::GenreRepository(QSqlDatabase& database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

GenreRepository::~GenreRepository()
{
}

bool GenreRepository::addGenre(const GenreItem& genre)
{
    QMutexLocker locker(&m_mutex);
    
    // Validate input
    QString validationError = validateGenreItem(genre);
    if (!validationError.isEmpty()) {
        logError("addGenre", validationError);
        emit operationError("addGenre", validationError);
        return false;
    }
    
    // Check if genre already exists (inline to avoid recursive mutex lock)
    {
        QString tableName = getTableName(genre.genreType);
        if (!tableName.isEmpty()) {
            QSqlQuery checkQuery(m_database);
            QString checkQueryString = QString("SELECT COUNT(*) as count FROM %1 WHERE name = ?").arg(tableName);
            checkQuery.prepare(checkQueryString);
            checkQuery.addBindValue(sanitizeName(genre.name));
            
            if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
                QString error = QString("Genre already exists in %1: %2")
                               .arg(tableName, genre.name);
                logError("addGenre", error);
                emit operationError("addGenre", error);
                return false;
            }
        }
    }
    
    QString tableName = getTableName(genre.genreType);
    if (tableName.isEmpty()) {
        QString error = QString("Invalid genre type: %1").arg(genre.genreType);
        logError("addGenre", error);
        emit operationError("addGenre", error);
        return false;
    }
    
    QSqlQuery query(m_database);
    QString queryString = QString("INSERT INTO %1 (name) VALUES (?)").arg(tableName);
    query.prepare(queryString);
    query.addBindValue(sanitizeName(genre.name));
    
    if (!executeQuery(query, "addGenre")) {
        return false;
    }
    
    // Create a copy with the new ID for the signal
    GenreItem addedGenre = genre;
    addedGenre.id = query.lastInsertId().toInt();
    
    emit genreAdded(addedGenre);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

bool GenreRepository::updateGenre(const GenreItem& genre)
{
    QMutexLocker locker(&m_mutex);
    
    if (genre.id <= 0) {
        QString error = "Invalid genre ID for update operation";
        logError("updateGenre", error);
        emit operationError("updateGenre", error);
        return false;
    }
    
    // Validate input
    QString validationError = validateGenreItem(genre);
    if (!validationError.isEmpty()) {
        logError("updateGenre", validationError);
        emit operationError("updateGenre", validationError);
        return false;
    }
    
    QString tableName = getTableName(genre.genreType);
    if (tableName.isEmpty()) {
        QString error = QString("Invalid genre type: %1").arg(genre.genreType);
        logError("updateGenre", error);
        emit operationError("updateGenre", error);
        return false;
    }
    
    QSqlQuery query(m_database);
    QString queryString = QString("UPDATE %1 SET name = ? WHERE id = ?").arg(tableName);
    query.prepare(queryString);
    query.addBindValue(sanitizeName(genre.name));
    query.addBindValue(genre.id);
    
    if (!executeQuery(query, "updateGenre")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No genre found with ID: %1 in %2")
                       .arg(genre.id).arg(tableName);
        logError("updateGenre", error);
        emit operationError("updateGenre", error);
        return false;
    }
    
    emit genreUpdated(genre);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

bool GenreRepository::deleteGenre(int genreId, int genreType)
{
    QMutexLocker locker(&m_mutex);
    
    if (genreId <= 0) {
        QString error = "Invalid genre ID for delete operation";
        logError("deleteGenre", error);
        emit operationError("deleteGenre", error);
        return false;
    }
    
    QString tableName = getTableName(genreType);
    if (tableName.isEmpty()) {
        QString error = QString("Invalid genre type: %1").arg(genreType);
        logError("deleteGenre", error);
        emit operationError("deleteGenre", error);
        return false;
    }
    
    QSqlQuery query(m_database);
    QString queryString = QString("DELETE FROM %1 WHERE id = ?").arg(tableName);
    query.prepare(queryString);
    query.addBindValue(genreId);
    
    if (!executeQuery(query, "deleteGenre")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        QString error = QString("No genre found with ID: %1 in %2")
                       .arg(genreId).arg(tableName);
        logError("deleteGenre", error);
        emit operationError("deleteGenre", error);
        return false;
    }
    
    emit genreDeleted(genreId, genreType);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

GenreItem GenreRepository::getGenreById(int genreId, int genreType)
{
    QMutexLocker locker(&m_mutex);
    
    if (genreId <= 0) {
        return GenreItem(); // Invalid item
    }
    
    QString tableName = getTableName(genreType);
    if (tableName.isEmpty()) {
        return GenreItem(); // Invalid item
    }
    
    QSqlQuery query(m_database);
    QString queryString = QString("SELECT id, name FROM %1 WHERE id = ?").arg(tableName);
    query.prepare(queryString);
    query.addBindValue(genreId);
    
    if (!executeQuery(query, "getGenreById")) {
        return GenreItem();
    }
    
    if (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["name"] = query.value("name");
        
        return GenreItem::fromVariantMap(map, genreType);
    }
    
    return GenreItem(); // Not found
}

QList<GenreItem> GenreRepository::getAllGenres(int genreType)
{
    QMutexLocker locker(&m_mutex);
    
    QList<GenreItem> results;
    
    if (genreType == 0 || genreType == 1) {
        // Get from genres1
        QSqlQuery query(m_database);
        query.prepare("SELECT id, name FROM genres1 ORDER BY name");
        
        if (executeQuery(query, "getAllGenres")) {
            while (query.next()) {
                QVariantMap map;
                map["id"] = query.value("id");
                map["name"] = query.value("name");
                
                results.append(GenreItem::fromVariantMap(map, 1));
            }
        }
    }
    
    if (genreType == 0 || genreType == 2) {
        // Get from genres2
        QSqlQuery query(m_database);
        query.prepare("SELECT id, name FROM genres2 ORDER BY name");
        
        if (executeQuery(query, "getAllGenres")) {
            while (query.next()) {
                QVariantMap map;
                map["id"] = query.value("id");
                map["name"] = query.value("name");
                
                results.append(GenreItem::fromVariantMap(map, 2));
            }
        }
    }
    
    return results;
}

QList<GenreItem> GenreRepository::searchGenres(const SearchCriteria& criteria)
{
    QMutexLocker locker(&m_mutex);
    
    QList<GenreItem> results;
    
    // Determine which tables to search
    QList<int> tablesToSearch;
    if (criteria.genreType == 0) {
        tablesToSearch << 1 << 2;
    } else if (criteria.genreType == 1 || criteria.genreType == 2) {
        tablesToSearch << criteria.genreType;
    }
    
    for (int tableType : tablesToSearch) {
        QString tableName = getTableName(tableType);
        if (tableName.isEmpty()) continue;
        
        QString queryString = QString("SELECT id, name FROM %1").arg(tableName);
        QVariantList bindValues;
        
        // Build WHERE clause
        if (!criteria.name.isEmpty()) {
            if (criteria.exactMatch) {
                queryString += " WHERE name = ?";
                bindValues.append(criteria.name);
            } else {
                queryString += " WHERE name LIKE ?";
                bindValues.append(QString("%%1%").arg(criteria.name));
            }
        }
        
        // Add ORDER BY
        queryString += " ORDER BY name";
        
        // Add LIMIT and OFFSET
        if (criteria.limit > 0) {
            queryString += QString(" LIMIT %1").arg(criteria.limit);
            if (criteria.offset > 0) {
                queryString += QString(" OFFSET %1").arg(criteria.offset);
            }
        }
        
        QSqlQuery query(m_database);
        query.prepare(queryString);
        
        // Bind values
        for (const QVariant& value : bindValues) {
            query.addBindValue(value);
        }
        
        if (executeQuery(query, "searchGenres")) {
            while (query.next()) {
                QVariantMap map;
                map["id"] = query.value("id");
                map["name"] = query.value("name");
                
                results.append(GenreItem::fromVariantMap(map, tableType));
            }
        }
    }
    
    return results;
}

QList<GenreItem> GenreRepository::getGenresByName(const QString& name, int genreType)
{
    SearchCriteria criteria;
    criteria.name = name;
    criteria.genreType = genreType;
    criteria.exactMatch = false;
    
    return searchGenres(criteria);
}

int GenreRepository::addGenreBatch(const QList<GenreItem>& genreList)
{
    if (genreList.isEmpty()) {
        return 0;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // Start transaction
    if (!m_database.transaction()) {
        logError("addGenreBatch", "Failed to start transaction");
        emit operationError("addGenreBatch", "Failed to start transaction");
        return 0;
    }
    
    int successCount = 0;
    
    // Group genres by type for batch processing
    QHash<int, QList<GenreItem>> genresByType;
    for (const GenreItem& genre : genreList) {
        genresByType[genre.genreType].append(genre);
    }
    
    // Process each type separately
    for (auto it = genresByType.begin(); it != genresByType.end(); ++it) {
        int genreType = it.key();
        const QList<GenreItem>& genres = it.value();
        
        QString tableName = getTableName(genreType);
        if (tableName.isEmpty()) {
            qDebug() << "Skipping invalid genre type:" << genreType;
            continue;
        }
        
        QSqlQuery query(m_database);
        QString queryString = QString("INSERT INTO %1 (name) VALUES (?)").arg(tableName);
        query.prepare(queryString);
        
        for (const GenreItem& genre : genres) {
            // Validate each item
            QString validationError = validateGenreItem(genre);
            if (!validationError.isEmpty()) {
                qDebug() << "Skipping invalid genre item:" << validationError;
                continue;
            }
            
            // Check if genre already exists (inline to avoid recursive mutex lock)
            {
                QSqlQuery checkQuery(m_database);
                QString checkQueryString = QString("SELECT COUNT(*) as count FROM %1 WHERE name = ?").arg(tableName);
                checkQuery.prepare(checkQueryString);
                checkQuery.addBindValue(sanitizeName(genre.name));
                
                if (checkQuery.exec() && checkQuery.next() && checkQuery.value("count").toInt() > 0) {
                    qDebug() << "Skipping duplicate genre:" << genre.name;
                    continue;
                }
            }
            
            query.addBindValue(sanitizeName(genre.name));
            
            if (query.exec()) {
                successCount++;
                
                // Emit signal for each added item
                GenreItem addedGenre = genre;
                addedGenre.id = query.lastInsertId().toInt();
                emit genreAdded(addedGenre);
            } else {
                logError("addGenreBatch", QString("Failed to add genre: %1")
                        .arg(genre.name), query.lastError().text());
            }
        }
    }
    
    // Commit transaction
    if (!m_database.commit()) {
        m_database.rollback();
        logError("addGenreBatch", "Failed to commit transaction");
        emit operationError("addGenreBatch", "Failed to commit transaction");
        return 0;
    }
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return successCount;
}

bool GenreRepository::genreExists(const QString& name, int genreType)
{
    QMutexLocker locker(&m_mutex);
    
    QString tableName = getTableName(genreType);
    if (tableName.isEmpty()) {
        return false;
    }
    
    QSqlQuery query(m_database);
    QString queryString = QString("SELECT COUNT(*) as count FROM %1 WHERE name = ?").arg(tableName);
    query.prepare(queryString);
    query.addBindValue(sanitizeName(name));
    
    if (query.exec() && query.next()) {
        return query.value("count").toInt() > 0;
    }
    
    if (query.lastError().isValid()) {
        logError("genreExists", query.lastError().text(), query.lastQuery());
    }
    
    return false;
}

GenreRepository::GenreStats GenreRepository::getStatistics()
{
    QMutexLocker statsLocker(&m_statsMutex);
    
    // Check if cached stats are still valid
    if (m_statsLastUpdated.isValid() && 
        m_statsLastUpdated.msecsTo(QDateTime::currentDateTime()) < STATS_CACHE_DURATION_MS) {
        return m_cachedStats;
    }
    
    statsLocker.unlock();
    QMutexLocker locker(&m_mutex);
    
    GenreStats stats;
    
    // Get total genres1
    QSqlQuery query(m_database);
    query.prepare("SELECT COUNT(*) as total FROM genres1");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalGenres1 = query.value("total").toInt();
    }
    
    // Get total genres2
    query.prepare("SELECT COUNT(*) as total FROM genres2");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.totalGenres2 = query.value("total").toInt();
    }
    
    stats.totalGenres = stats.totalGenres1 + stats.totalGenres2;
    
    // Get most used genre1 (based on usage in musics table)
    query.prepare("SELECT genre1, COUNT(*) as usage_count FROM musics "
                  "WHERE genre1 IS NOT NULL AND genre1 != '' "
                  "GROUP BY genre1 ORDER BY usage_count DESC LIMIT 1");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.mostUsedGenre1 = query.value("genre1").toString();
    }
    
    // Get most used genre2 (based on usage in musics table)
    query.prepare("SELECT genre2, COUNT(*) as usage_count FROM musics "
                  "WHERE genre2 IS NOT NULL AND genre2 != '' "
                  "GROUP BY genre2 ORDER BY usage_count DESC LIMIT 1");
    if (executeQuery(query, "getStatistics") && query.next()) {
        stats.mostUsedGenre2 = query.value("genre2").toString();
    }
    
    locker.unlock();
    
    // Cache the results
    statsLocker.relock();
    m_cachedStats = stats;
    m_statsLastUpdated = QDateTime::currentDateTime();
    
    return stats;
}

QStringList GenreRepository::getAllGenreNames(int genreType)
{
    QMutexLocker locker(&m_mutex);
    
    QStringList names;
    
    if (genreType == 0 || genreType == 1) {
        // Get from genres1
        QSqlQuery query(m_database);
        query.prepare("SELECT name FROM genres1 ORDER BY name");
        
        if (executeQuery(query, "getAllGenreNames")) {
            while (query.next()) {
                QString name = query.value("name").toString();
                if (!names.contains(name)) {
                    names.append(name);
                }
            }
        }
    }
    
    if (genreType == 0 || genreType == 2) {
        // Get from genres2
        QSqlQuery query(m_database);
        query.prepare("SELECT name FROM genres2 ORDER BY name");
        
        if (executeQuery(query, "getAllGenreNames")) {
            while (query.next()) {
                QString name = query.value("name").toString();
                if (!names.contains(name)) {
                    names.append(name);
                }
            }
        }
    }
    
    names.sort();
    return names;
}

QString GenreRepository::validateGenreItem(const GenreItem& genre)
{
    if (genre.name.isEmpty()) {
        return "Genre name is required";
    }
    
    if (genre.name.length() > 100) {
        return "Genre name is too long (maximum 100 characters)";
    }
    
    if (genre.genreType != 1 && genre.genreType != 2) {
        return QString("Invalid genre type: %1 (must be 1 or 2)").arg(genre.genreType);
    }
    
    return QString(); // Valid
}

QString GenreRepository::getTableName(int genreType)
{
    switch (genreType) {
        case 1: return "genres1";
        case 2: return "genres2";
        default: return QString();
    }
}

QString GenreRepository::sanitizeName(const QString& name)
{
    return name.trimmed();
}

void GenreRepository::logError(const QString& operation, const QString& error, const QString& query)
{
    QString logMessage = QString("GenreRepository::%1 - %2").arg(operation, error);
    if (!query.isEmpty()) {
        logMessage += QString(" (Query: %1)").arg(query);
    }
    
    qWarning() << logMessage;
}

bool GenreRepository::executeQuery(QSqlQuery& query, const QString& operation)
{
    if (!query.exec()) {
        QString error = QString("SQL Error: %1").arg(query.lastError().text());
        logError(operation, error, query.lastQuery());
        emit operationError(operation, error);
        return false;
    }
    
    return true;
}