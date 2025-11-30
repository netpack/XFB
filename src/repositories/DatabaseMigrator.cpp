#include "DatabaseMigrator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>

DatabaseMigrator::DatabaseMigrator(QSqlDatabase& database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

DatabaseMigrator::~DatabaseMigrator()
{
}

bool DatabaseMigrator::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    if (!createMigrationsTable()) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool DatabaseMigrator::addMigration(const Migration& migration)
{
    QMutexLocker locker(&m_mutex);
    
    if (!migration.isValid()) {
        QString error = "Invalid migration: missing required fields";
        logError("addMigration", error);
        emit operationError("addMigration", error);
        return false;
    }
    
    // Check if migration version already exists (inline to avoid recursive mutex lock)
    for (const Migration& existingMigration : m_migrations) {
        if (existingMigration.version == migration.version) {
            QString error = QString("Migration version already exists: %1").arg(migration.version);
            logError("addMigration", error);
            emit operationError("addMigration", error);
            return false;
        }
    }
    
    // Validate SQL
    QString sqlError = validateMigrationSql(migration.upSql);
    if (!sqlError.isEmpty()) {
        logError("addMigration", QString("Invalid up SQL: %1").arg(sqlError));
        emit operationError("addMigration", QString("Invalid up SQL: %1").arg(sqlError));
        return false;
    }
    
    if (!migration.downSql.isEmpty()) {
        sqlError = validateMigrationSql(migration.downSql);
        if (!sqlError.isEmpty()) {
            logError("addMigration", QString("Invalid down SQL: %1").arg(sqlError));
            emit operationError("addMigration", QString("Invalid down SQL: %1").arg(sqlError));
            return false;
        }
    }
    
    // Add to in-memory list
    m_migrations.append(migration);
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    return true;
}

DatabaseMigrator::MigrationResult DatabaseMigrator::migrate()
{
    QMutexLocker locker(&m_mutex);
    
    MigrationResult result;
    
    if (!m_initialized && !initialize()) {
        result.error = "Migration system not initialized";
        return result;
    }
    
    // Get pending migrations inline to avoid recursive mutex lock
    QList<Migration> pendingMigrations;
    for (const Migration& migration : m_migrations) {
        QSqlQuery query(m_database);
        query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ? AND is_applied = 1").arg(MIGRATIONS_TABLE));
        query.addBindValue(migration.version);
        
        bool isApplied = false;
        if (query.exec() && query.next()) {
            isApplied = query.value("count").toInt() > 0;
        }
        
        if (!isApplied) {
            pendingMigrations.append(migration);
        }
    }
    if (pendingMigrations.isEmpty()) {
        result.success = true;
        result.totalMigrations = 0;
        result.appliedCount = 0;
        return result;
    }
    
    // Sort migrations by version
    pendingMigrations = sortMigrationsByVersion(pendingMigrations, true);
    
    result.totalMigrations = pendingMigrations.size();
    emit migrationBatchStarted(result.totalMigrations);
    
    // Start transaction
    if (!m_database.transaction()) {
        result.error = "Failed to start transaction";
        logError("migrate", result.error);
        emit operationError("migrate", result.error);
        return result;
    }
    
    bool allSuccessful = true;
    
    for (const Migration& migration : pendingMigrations) {
        emit migrationStarted(migration.version, migration.name);
        
        if (executeMigration(migration, false)) {
            if (markMigrationAsApplied(migration)) {
                result.appliedMigrations.append(migration.version);
                result.appliedCount++;
                emit migrationCompleted(migration.version, migration.name);
            } else {
                QString error = QString("Failed to mark migration as applied: %1").arg(migration.version);
                result.failedMigrations.append(migration.version);
                emit migrationFailed(migration.version, migration.name, error);
                allSuccessful = false;
                break;
            }
        } else {
            QString error = QString("Failed to execute migration: %1").arg(migration.version);
            result.failedMigrations.append(migration.version);
            emit migrationFailed(migration.version, migration.name, error);
            allSuccessful = false;
            break;
        }
    }
    
    if (allSuccessful) {
        if (m_database.commit()) {
            result.success = true;
        } else {
            result.error = "Failed to commit transaction";
            result.success = false;
            m_database.rollback();
        }
    } else {
        result.error = "One or more migrations failed";
        result.success = false;
        m_database.rollback();
    }
    
    // Invalidate stats cache
    QMutexLocker statsLocker(&m_statsMutex);
    m_statsLastUpdated = QDateTime();
    
    emit migrationBatchCompleted(result);
    return result;
}

DatabaseMigrator::MigrationResult DatabaseMigrator::migrateTo(const QString& targetVersion)
{
    QMutexLocker locker(&m_mutex);
    
    MigrationResult result;
    
    if (!m_initialized && !initialize()) {
        result.error = "Migration system not initialized";
        return result;
    }
    
    // Get pending migrations inline and filter up to target version
    QList<Migration> migrationsToApply;
    for (const Migration& migration : m_migrations) {
        if (compareVersions(migration.version, targetVersion) <= 0) {
            QSqlQuery query(m_database);
            query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ? AND is_applied = 1").arg(MIGRATIONS_TABLE));
            query.addBindValue(migration.version);
            
            bool isApplied = false;
            if (query.exec() && query.next()) {
                isApplied = query.value("count").toInt() > 0;
            }
            
            if (!isApplied) {
                migrationsToApply.append(migration);
            }
        }
    }
    
    if (migrationsToApply.isEmpty()) {
        result.success = true;
        result.totalMigrations = 0;
        result.appliedCount = 0;
        return result;
    }
    
    // Sort migrations by version
    migrationsToApply = sortMigrationsByVersion(migrationsToApply, true);
    
    result.totalMigrations = migrationsToApply.size();
    emit migrationBatchStarted(result.totalMigrations);
    
    // Execute migrations (similar to migrate() but with filtered list)
    if (!m_database.transaction()) {
        result.error = "Failed to start transaction";
        return result;
    }
    
    bool allSuccessful = true;
    
    for (const Migration& migration : migrationsToApply) {
        emit migrationStarted(migration.version, migration.name);
        
        if (executeMigration(migration, false)) {
            if (markMigrationAsApplied(migration)) {
                result.appliedMigrations.append(migration.version);
                result.appliedCount++;
                emit migrationCompleted(migration.version, migration.name);
            } else {
                allSuccessful = false;
                break;
            }
        } else {
            result.failedMigrations.append(migration.version);
            allSuccessful = false;
            break;
        }
    }
    
    if (allSuccessful && m_database.commit()) {
        result.success = true;
    } else {
        result.error = allSuccessful ? "Failed to commit transaction" : "One or more migrations failed";
        result.success = false;
        m_database.rollback();
    }
    
    emit migrationBatchCompleted(result);
    return result;
}

DatabaseMigrator::MigrationResult DatabaseMigrator::rollbackTo(const QString& targetVersion)
{
    QMutexLocker locker(&m_mutex);
    
    MigrationResult result;
    
    if (!m_initialized && !initialize()) {
        result.error = "Migration system not initialized";
        return result;
    }
    
    // Get applied migrations inline and filter those to rollback
    QList<Migration> migrationsToRollback;
    for (const Migration& migration : m_migrations) {
        if (compareVersions(migration.version, targetVersion) > 0) {
            QSqlQuery query(m_database);
            query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ? AND is_applied = 1").arg(MIGRATIONS_TABLE));
            query.addBindValue(migration.version);
            
            bool isApplied = false;
            if (query.exec() && query.next()) {
                isApplied = query.value("count").toInt() > 0;
            }
            
            if (isApplied) {
                migrationsToRollback.append(migration);
            }
        }
    }
    
    if (migrationsToRollback.isEmpty()) {
        result.success = true;
        result.totalMigrations = 0;
        result.appliedCount = 0;
        return result;
    }
    
    // Sort migrations by version in descending order for rollback
    migrationsToRollback = sortMigrationsByVersion(migrationsToRollback, false);
    
    result.totalMigrations = migrationsToRollback.size();
    
    if (!m_database.transaction()) {
        result.error = "Failed to start transaction";
        return result;
    }
    
    bool allSuccessful = true;
    
    for (const Migration& migration : migrationsToRollback) {
        emit rollbackStarted(migration.version, migration.name);
        
        if (executeMigration(migration, true)) {
            if (markMigrationAsNotApplied(migration)) {
                result.appliedMigrations.append(migration.version);
                result.appliedCount++;
                emit rollbackCompleted(migration.version, migration.name);
            } else {
                allSuccessful = false;
                break;
            }
        } else {
            result.failedMigrations.append(migration.version);
            emit rollbackFailed(migration.version, migration.name, "Failed to execute rollback");
            allSuccessful = false;
            break;
        }
    }
    
    if (allSuccessful && m_database.commit()) {
        result.success = true;
    } else {
        result.error = allSuccessful ? "Failed to commit transaction" : "One or more rollbacks failed";
        result.success = false;
        m_database.rollback();
    }
    
    return result;
}

DatabaseMigrator::MigrationResult DatabaseMigrator::rollback(int count)
{
    QMutexLocker locker(&m_mutex);
    
    MigrationResult result;
    
    if (!m_initialized && !initialize()) {
        result.error = "Migration system not initialized";
        return result;
    }
    
    // Get applied migrations inline
    QList<Migration> appliedMigrations;
    for (const Migration& migration : m_migrations) {
        QSqlQuery query(m_database);
        query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ? AND is_applied = 1").arg(MIGRATIONS_TABLE));
        query.addBindValue(migration.version);
        
        bool isApplied = false;
        if (query.exec() && query.next()) {
            isApplied = query.value("count").toInt() > 0;
        }
        
        if (isApplied) {
            appliedMigrations.append(migration);
        }
    }
    
    if (appliedMigrations.isEmpty()) {
        result.success = true;
        result.totalMigrations = 0;
        result.appliedCount = 0;
        return result;
    }
    
    // Sort by version descending and take the last N migrations
    appliedMigrations = sortMigrationsByVersion(appliedMigrations, false);
    
    QList<Migration> migrationsToRollback;
    for (int i = 0; i < qMin(count, appliedMigrations.size()); ++i) {
        migrationsToRollback.append(appliedMigrations[i]);
    }
    
    result.totalMigrations = migrationsToRollback.size();
    
    if (!m_database.transaction()) {
        result.error = "Failed to start transaction";
        return result;
    }
    
    bool allSuccessful = true;
    
    for (const Migration& migration : migrationsToRollback) {
        emit rollbackStarted(migration.version, migration.name);
        
        if (executeMigration(migration, true)) {
            if (markMigrationAsNotApplied(migration)) {
                result.appliedMigrations.append(migration.version);
                result.appliedCount++;
                emit rollbackCompleted(migration.version, migration.name);
            } else {
                allSuccessful = false;
                break;
            }
        } else {
            result.failedMigrations.append(migration.version);
            emit rollbackFailed(migration.version, migration.name, "Failed to execute rollback");
            allSuccessful = false;
            break;
        }
    }
    
    if (allSuccessful && m_database.commit()) {
        result.success = true;
    } else {
        result.error = allSuccessful ? "Failed to commit transaction" : "One or more rollbacks failed";
        result.success = false;
        m_database.rollback();
    }
    
    return result;
}

QString DatabaseMigrator::getCurrentVersion()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized && !initialize()) {
        return QString();
    }
    
    // Get the highest version directly from database
    QSqlQuery query(m_database);
    query.prepare(QString("SELECT version FROM %1 WHERE is_applied = 1 ORDER BY version DESC LIMIT 1").arg(MIGRATIONS_TABLE));
    
    if (query.exec() && query.next()) {
        return query.value("version").toString();
    }
    
    return QString();
}

QString DatabaseMigrator::getLatestVersion()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_migrations.isEmpty()) {
        return QString();
    }
    
    QList<Migration> sortedMigrations = sortMigrationsByVersion(m_migrations, false);
    return sortedMigrations.first().version;
}

QList<Migration> DatabaseMigrator::getMigrations(bool appliedOnly)
{
    QMutexLocker locker(&m_mutex);
    
    if (appliedOnly) {
        return getAppliedMigrations();
    }
    
    return m_migrations;
}

QList<Migration> DatabaseMigrator::getPendingMigrations()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized && !initialize()) {
        return QList<Migration>();
    }
    
    QList<Migration> pending;
    
    // Check applied migrations inline to avoid recursive mutex lock
    for (const Migration& migration : m_migrations) {
        QSqlQuery query(m_database);
        query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ? AND is_applied = 1").arg(MIGRATIONS_TABLE));
        query.addBindValue(migration.version);
        
        bool isApplied = false;
        if (query.exec() && query.next()) {
            isApplied = query.value("count").toInt() > 0;
        }
        
        if (!isApplied) {
            pending.append(migration);
        }
    }
    
    return pending;
}

QList<Migration> DatabaseMigrator::getAppliedMigrations()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized && !initialize()) {
        return QList<Migration>();
    }
    
    QList<Migration> applied;
    
    QSqlQuery query(m_database);
    query.prepare(QString("SELECT version, name, description, applied_at FROM %1 WHERE is_applied = 1 ORDER BY version").arg(MIGRATIONS_TABLE));
    
    if (query.exec()) {
        while (query.next()) {
            // Find the migration in our list to get the full details
            QString version = query.value("version").toString();
            for (const Migration& migration : m_migrations) {
                if (migration.version == version) {
                    Migration appliedMigration = migration;
                    appliedMigration.appliedAt = query.value("applied_at").toDateTime();
                    appliedMigration.isApplied = true;
                    applied.append(appliedMigration);
                    break;
                }
            }
        }
    } else {
        logError("getAppliedMigrations", query.lastError().text(), query.lastQuery());
    }
    
    return applied;
}

Migration DatabaseMigrator::getMigrationByVersion(const QString& version)
{
    QMutexLocker locker(&m_mutex);
    
    for (const Migration& migration : m_migrations) {
        if (migration.version == version) {
            return migration;
        }
    }
    
    return Migration(); // Invalid migration
}

bool DatabaseMigrator::migrationExists(const QString& version)
{
    QMutexLocker locker(&m_mutex);
    
    for (const Migration& migration : m_migrations) {
        if (migration.version == version) {
            return true;
        }
    }
    
    return false;
}

bool DatabaseMigrator::isMigrationApplied(const QString& version)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized && !initialize()) {
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ? AND is_applied = 1").arg(MIGRATIONS_TABLE));
    query.addBindValue(version);
    
    if (query.exec() && query.next()) {
        return query.value("count").toInt() > 0;
    }
    
    return false;
}

DatabaseMigrator::MigrationStats DatabaseMigrator::getStatistics()
{
    QMutexLocker statsLocker(&m_statsMutex);
    
    // Check if cached stats are still valid
    if (m_statsLastUpdated.isValid() && 
        m_statsLastUpdated.msecsTo(QDateTime::currentDateTime()) < STATS_CACHE_DURATION_MS) {
        return m_cachedStats;
    }
    
    statsLocker.unlock();
    QMutexLocker locker(&m_mutex);
    
    MigrationStats stats;
    
    stats.totalMigrations = m_migrations.size();
    
    QList<Migration> appliedMigrations = getAppliedMigrations();
    stats.appliedMigrations = appliedMigrations.size();
    stats.pendingMigrations = stats.totalMigrations - stats.appliedMigrations;
    
    stats.currentVersion = getCurrentVersion();
    stats.latestVersion = getLatestVersion();
    
    if (!appliedMigrations.isEmpty()) {
        // Sort by applied date and get the most recent
        std::sort(appliedMigrations.begin(), appliedMigrations.end(), 
                 [](const Migration& a, const Migration& b) {
                     return a.appliedAt > b.appliedAt;
                 });
        stats.lastMigration = appliedMigrations.first().appliedAt;
    }
    
    locker.unlock();
    
    // Cache the results
    statsLocker.relock();
    m_cachedStats = stats;
    m_statsLastUpdated = QDateTime::currentDateTime();
    
    return stats;
}

QString DatabaseMigrator::validateMigrationSql(const QString& sql)
{
    if (sql.trimmed().isEmpty()) {
        return "SQL cannot be empty";
    }
    
    // Basic SQL validation - check for dangerous operations
    QString upperSql = sql.toUpper();
    
    // Check for multiple statements (basic check)
    if (sql.count(';') > 1) {
        return "Multiple SQL statements not allowed in single migration";
    }
    
    // Check for some dangerous operations (this is basic validation)
    QStringList dangerousKeywords = {"DROP DATABASE", "DROP SCHEMA"};
    for (const QString& keyword : dangerousKeywords) {
        if (upperSql.contains(keyword)) {
            return QString("Dangerous operation not allowed: %1").arg(keyword);
        }
    }
    
    return QString(); // Valid
}

bool DatabaseMigrator::createBackup(const QString& backupPath)
{
    // This is a simplified backup - in a real implementation,
    // you would use database-specific backup tools
    QFile sourceFile(m_database.databaseName());
    
    if (!sourceFile.exists()) {
        logError("createBackup", "Source database file does not exist");
        return false;
    }
    
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    
    bool success = sourceFile.copy(backupPath);
    if (!success) {
        logError("createBackup", QString("Failed to copy database to %1").arg(backupPath));
    }
    
    return success;
}

bool DatabaseMigrator::restoreFromBackup(const QString& backupPath)
{
    QFile backupFile(backupPath);
    
    if (!backupFile.exists()) {
        logError("restoreFromBackup", "Backup file does not exist");
        return false;
    }
    
    QString dbPath = m_database.databaseName();
    
    // Close the database connection
    m_database.close();
    
    // Remove current database file
    if (QFile::exists(dbPath)) {
        QFile::remove(dbPath);
    }
    
    // Copy backup to database location
    bool success = backupFile.copy(dbPath);
    
    // Reopen the database
    m_database.open();
    
    if (!success) {
        logError("restoreFromBackup", QString("Failed to restore from backup %1").arg(backupPath));
    }
    
    return success;
}

bool DatabaseMigrator::reset()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized && !initialize()) {
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare(QString("DELETE FROM %1").arg(MIGRATIONS_TABLE));
    
    bool success = executeQuery(query, "reset");
    
    if (success) {
        // Invalidate stats cache
        QMutexLocker statsLocker(&m_statsMutex);
        m_statsLastUpdated = QDateTime();
    }
    
    return success;
}

bool DatabaseMigrator::createMigrationsTable()
{
    QSqlQuery query(m_database);
    
    QString createTableSql = QString(R"(
        CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            version TEXT NOT NULL UNIQUE,
            name TEXT NOT NULL,
            description TEXT,
            up_sql TEXT NOT NULL,
            down_sql TEXT,
            applied_at DATETIME,
            is_applied BOOLEAN DEFAULT 0
        )
    )").arg(MIGRATIONS_TABLE);
    
    if (!query.exec(createTableSql)) {
        logError("createMigrationsTable", query.lastError().text(), createTableSql);
        return false;
    }
    
    return true;
}

bool DatabaseMigrator::executeMigration(const Migration& migration, bool isRollback)
{
    QString sql = isRollback ? migration.downSql : migration.upSql;
    
    if (sql.isEmpty()) {
        if (isRollback) {
            logError("executeMigration", QString("No rollback SQL provided for migration %1").arg(migration.version));
            return false;
        }
        // Forward migration without SQL is considered successful (no-op)
        return true;
    }
    
    QSqlQuery query(m_database);
    
    if (!query.exec(sql)) {
        QString operation = isRollback ? "rollback" : "migration";
        logError("executeMigration", QString("Failed to execute %1 %2: %3")
                .arg(operation, migration.version, query.lastError().text()), sql);
        return false;
    }
    
    return true;
}

bool DatabaseMigrator::markMigrationAsApplied(const Migration& migration)
{
    QSqlQuery query(m_database);
    
    // First, check if the migration record exists
    query.prepare(QString("SELECT COUNT(*) as count FROM %1 WHERE version = ?").arg(MIGRATIONS_TABLE));
    query.addBindValue(migration.version);
    
    if (!executeQuery(query, "markMigrationAsApplied")) {
        return false;
    }
    
    bool exists = false;
    if (query.next()) {
        exists = query.value("count").toInt() > 0;
    }
    
    if (exists) {
        // Update existing record
        query.prepare(QString("UPDATE %1 SET is_applied = 1, applied_at = ? WHERE version = ?").arg(MIGRATIONS_TABLE));
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(migration.version);
    } else {
        // Insert new record
        query.prepare(QString("INSERT INTO %1 (version, name, description, up_sql, down_sql, applied_at, is_applied) VALUES (?, ?, ?, ?, ?, ?, 1)").arg(MIGRATIONS_TABLE));
        query.addBindValue(migration.version);
        query.addBindValue(migration.name);
        query.addBindValue(migration.description);
        query.addBindValue(migration.upSql);
        query.addBindValue(migration.downSql);
        query.addBindValue(QDateTime::currentDateTime());
    }
    
    return executeQuery(query, "markMigrationAsApplied");
}

bool DatabaseMigrator::markMigrationAsNotApplied(const Migration& migration)
{
    QSqlQuery query(m_database);
    query.prepare(QString("UPDATE %1 SET is_applied = 0, applied_at = NULL WHERE version = ?").arg(MIGRATIONS_TABLE));
    query.addBindValue(migration.version);
    
    return executeQuery(query, "markMigrationAsNotApplied");
}

int DatabaseMigrator::compareVersions(const QString& version1, const QString& version2)
{
    // Simple numeric comparison for versions like "001", "002", etc.
    // For more complex versioning schemes, this would need to be enhanced
    
    bool ok1, ok2;
    int v1 = version1.toInt(&ok1);
    int v2 = version2.toInt(&ok2);
    
    if (ok1 && ok2) {
        if (v1 < v2) return -1;
        if (v1 > v2) return 1;
        return 0;
    }
    
    // Fallback to string comparison
    return version1.compare(version2);
}

QList<Migration> DatabaseMigrator::sortMigrationsByVersion(const QList<Migration>& migrations, bool ascending)
{
    QList<Migration> sorted = migrations;
    
    std::sort(sorted.begin(), sorted.end(), [this, ascending](const Migration& a, const Migration& b) {
        int comparison = compareVersions(a.version, b.version);
        return ascending ? (comparison < 0) : (comparison > 0);
    });
    
    return sorted;
}

void DatabaseMigrator::logError(const QString& operation, const QString& error, const QString& query)
{
    QString logMessage = QString("DatabaseMigrator::%1 - %2").arg(operation, error);
    if (!query.isEmpty()) {
        logMessage += QString(" (Query: %1)").arg(query);
    }
    
    qWarning() << logMessage;
}

bool DatabaseMigrator::executeQuery(QSqlQuery& query, const QString& operation)
{
    if (!query.exec()) {
        QString error = QString("SQL Error: %1").arg(query.lastError().text());
        logError(operation, error, query.lastQuery());
        emit operationError(operation, error);
        return false;
    }
    
    return true;
}