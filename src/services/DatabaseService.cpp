#include "DatabaseService.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>
#include <QVariant>

DatabaseService::DatabaseService(QObject* parent)
    : BaseService(parent)
    , m_driverName("QSQLITE")
    , m_maxConnections(DEFAULT_MAX_CONNECTIONS)
    , m_connectionCounter(0)
    , m_cleanupTimer(new QTimer(this))
    , m_totalQueries(0)
    , m_failedQueries(0)
    , m_totalTransactions(0)
    , m_failedTransactions(0)
{
    // Set up cleanup timer
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL_MS);
    m_cleanupTimer->setSingleShot(false);
    connect(m_cleanupTimer, &QTimer::timeout, this, &DatabaseService::onConnectionCleanupTimer);
    
    // Set default database path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appDir = configDir + "/Netpack - Online Solutions/XFB/config";
    m_databasePath = appDir + "/adb.db";
}

DatabaseService::~DatabaseService()
{
    // Shutdown is called in BaseService destructor
}

bool DatabaseService::doInitialize()
{
    logDebug("Initializing database service...");
    
    // Validate database path
    if (m_databasePath.isEmpty()) {
        setError("Database path is not set");
        return false;
    }
    
    // Ensure database directory exists
    QFileInfo dbFileInfo(m_databasePath);
    QDir dbDir = dbFileInfo.absoluteDir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            setError(QString("Failed to create database directory: %1").arg(dbDir.absolutePath()));
            return false;
        }
        logDebug(QString("Created database directory: %1").arg(dbDir.absolutePath()));
    }
    
    // Check if SQLite driver is available
    if (!QSqlDatabase::isDriverAvailable(m_driverName)) {
        setError(QString("Database driver not available: %1").arg(m_driverName));
        return false;
    }
    
    // Create initial connection to test database
    auto testConnection = createConnection();
    if (!testConnection || !testConnection->database.isValid()) {
        setError("Failed to create initial database connection");
        return false;
    }
    
    // Test opening the database
    if (!testConnection->database.open()) {
        QSqlError error = testConnection->database.lastError();
        setError(QString("Failed to open database: %1").arg(error.text()));
        logSqlError(error, "", "Initial connection test");
        return false;
    }
    
    // Add test connection to pool before schema initialization
    m_connectionPool.push_back(std::move(testConnection));
    
    // Create a default database connection for simple queries
    QSqlDatabase defaultDb = QSqlDatabase::addDatabase(m_driverName);
    defaultDb.setDatabaseName(m_databasePath);
    defaultDb.setConnectOptions("QSQLITE_BUSY_TIMEOUT=30000");
    
    if (!defaultDb.open()) {
        QSqlError error = defaultDb.lastError();
        setError(QString("Failed to open default database connection: %1").arg(error.text()));
        logSqlError(error, "", "Default connection");
        return false;
    }
    
    // Initialize schema if needed
    if (!initializeSchema()) {
        setError("Failed to initialize database schema");
        return false;
    }
    
    // Start cleanup timer
    m_cleanupTimer->start();
    
    logDebug(QString("Database service initialized successfully with path: %1").arg(m_databasePath));
    return true;
}

void DatabaseService::doShutdown()
{
    logDebug("Shutting down database service...");
    
    // Stop cleanup timer
    m_cleanupTimer->stop();
    
    // Close default connection
    QSqlDatabase defaultDb = QSqlDatabase::database();
    if (defaultDb.isValid() && defaultDb.isOpen()) {
        defaultDb.close();
    }
    
    // Close all connections
    QMutexLocker locker(&m_poolMutex);
    for (auto& connection : m_connectionPool) {
        if (connection->database.isOpen()) {
            connection->database.close();
        }
    }
    m_connectionPool.clear();
    
    // Remove all database connections
    QStringList connectionNames = QSqlDatabase::connectionNames();
    for (const QString& name : connectionNames) {
        if (name.startsWith("DatabaseService_")) {
            QSqlDatabase::removeDatabase(name);
        }
    }
    
    // Remove default connection
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    
    logDebug("Database service shutdown completed");
}

void DatabaseService::setDatabasePath(const QString& databasePath)
{
    if (isRunning()) {
        logWarning("Cannot change database path while service is running");
        return;
    }
    
    m_databasePath = databasePath;
    logDebug(QString("Database path set to: %1").arg(databasePath));
}

QString DatabaseService::databasePath() const
{
    return m_databasePath;
}

QSqlQuery DatabaseService::createQuery()
{
    // Use the default database connection for simple queries
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        logError("Default database connection is not available");
        return QSqlQuery();
    }
    
    return QSqlQuery(db);
}

bool DatabaseService::executeTransaction(TransactionFunction transaction)
{
    if (!transaction) {
        logError("Transaction function is null");
        return false;
    }
    
    PooledConnection* connection = getConnection();
    if (!connection) {
        logError("Failed to get database connection for transaction");
        return false;
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_totalTransactions++;
    statsLocker.unlock();
    
    emit transactionStarted();
    
    bool success = false;
    QString errorMessage;
    
    if (!connection->database.transaction()) {
        errorMessage = QString("Failed to start transaction: %1").arg(connection->database.lastError().text());
        logSqlError(connection->database.lastError(), "", "Transaction start");
    } else {
        try {
            success = transaction();
            
            if (success) {
                if (!connection->database.commit()) {
                    errorMessage = QString("Failed to commit transaction: %1").arg(connection->database.lastError().text());
                    logSqlError(connection->database.lastError(), "", "Transaction commit");
                    success = false;
                } else {
                    emit transactionCommitted();
                    logDebug("Transaction committed successfully");
                }
            } else {
                errorMessage = "Transaction function returned false";
            }
        } catch (const std::exception& e) {
            errorMessage = QString("Exception in transaction: %1").arg(e.what());
            success = false;
        } catch (...) {
            errorMessage = "Unknown exception in transaction";
            success = false;
        }
        
        if (!success) {
            if (!connection->database.rollback()) {
                QString rollbackError = connection->database.lastError().text();
                logError(QString("Failed to rollback transaction: %1").arg(rollbackError));
                errorMessage += QString(" (Rollback also failed: %1)").arg(rollbackError);
            } else {
                emit transactionRolledBack(errorMessage);
                logDebug(QString("Transaction rolled back: %1").arg(errorMessage));
            }
        }
    }
    
    returnConnection(connection);
    
    if (!success) {
        QMutexLocker statsLocker(&m_statsMutex);
        m_failedTransactions++;
        statsLocker.unlock();
        
        emit databaseError(errorMessage);
        logError(QString("Transaction failed: %1").arg(errorMessage));
    }
    
    return success;
}

bool DatabaseService::executeQuery(const QString& queryString, const QVariantList& bindValues)
{
    QSqlQuery query = createQuery();
    if (!query.isValid()) {
        logError("Failed to create valid query for execution");
        return false;
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_totalQueries++;
    statsLocker.unlock();
    
    if (!query.prepare(queryString)) {
        QMutexLocker statsLocker(&m_statsMutex);
        m_failedQueries++;
        statsLocker.unlock();
        
        logSqlError(query.lastError(), queryString, "Query preparation");
        emit databaseError(query.lastError().text(), queryString);
        return false;
    }
    
    // Bind values if provided
    for (const QVariant& value : bindValues) {
        query.addBindValue(value);
    }
    
    if (!query.exec()) {
        QMutexLocker statsLocker(&m_statsMutex);
        m_failedQueries++;
        statsLocker.unlock();
        
        logSqlError(query.lastError(), queryString, "Query execution");
        emit databaseError(query.lastError().text(), queryString);
        return false;
    }
    
    return true;
}

QList<QVariantMap> DatabaseService::executeSelect(const QString& queryString, const QVariantList& bindValues)
{
    QList<QVariantMap> results;
    
    QSqlQuery query = createQuery();
    if (!query.isValid()) {
        return results;
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_totalQueries++;
    statsLocker.unlock();
    
    if (!query.prepare(queryString)) {
        QMutexLocker statsLocker(&m_statsMutex);
        m_failedQueries++;
        statsLocker.unlock();
        
        logSqlError(query.lastError(), queryString, "SELECT query preparation");
        emit databaseError(query.lastError().text(), queryString);
        return results;
    }
    
    // Bind values if provided
    for (const QVariant& value : bindValues) {
        query.addBindValue(value);
    }
    
    if (!query.exec()) {
        QMutexLocker statsLocker(&m_statsMutex);
        m_failedQueries++;
        statsLocker.unlock();
        
        logSqlError(query.lastError(), queryString, "SELECT query execution");
        emit databaseError(query.lastError().text(), queryString);
        return results;
    }
    
    // Process results
    QSqlRecord record = query.record();
    int fieldCount = record.count();
    
    while (query.next()) {
        QVariantMap row;
        for (int i = 0; i < fieldCount; ++i) {
            QString fieldName = record.fieldName(i);
            QVariant value = query.value(i);
            row[fieldName] = value;
        }
        results.append(row);
    }
    
    logDebug(QString("SELECT query returned %1 rows").arg(results.size()));
    return results;
}

bool DatabaseService::backup(const QString& backupPath)
{
    logDebug(QString("Starting database backup to: %1").arg(backupPath));
    
    if (!QFile::exists(m_databasePath)) {
        QString error = QString("Source database file does not exist: %1").arg(m_databasePath);
        logError(error);
        emit backupCompleted(false, backupPath);
        return false;
    }
    
    // Ensure backup directory exists
    QFileInfo backupInfo(backupPath);
    QDir backupDir = backupInfo.absoluteDir();
    if (!backupDir.exists()) {
        if (!backupDir.mkpath(".")) {
            QString error = QString("Failed to create backup directory: %1").arg(backupDir.absolutePath());
            logError(error);
            emit backupCompleted(false, backupPath);
            return false;
        }
    }
    
    // Remove existing backup file
    if (QFile::exists(backupPath)) {
        if (!QFile::remove(backupPath)) {
            QString error = QString("Failed to remove existing backup file: %1").arg(backupPath);
            logError(error);
            emit backupCompleted(false, backupPath);
            return false;
        }
    }
    
    // Copy database file
    if (!QFile::copy(m_databasePath, backupPath)) {
        QString error = QString("Failed to copy database file from %1 to %2").arg(m_databasePath, backupPath);
        logError(error);
        emit backupCompleted(false, backupPath);
        return false;
    }
    
    logDebug(QString("Database backup completed successfully: %1").arg(backupPath));
    emit backupCompleted(true, backupPath);
    return true;
}

bool DatabaseService::restore(const QString& backupPath)
{
    logDebug(QString("Starting database restore from: %1").arg(backupPath));
    
    if (!QFile::exists(backupPath)) {
        QString error = QString("Backup file does not exist: %1").arg(backupPath);
        logError(error);
        emit restoreCompleted(false, backupPath);
        return false;
    }
    
    // Close all connections before restore
    {
        QMutexLocker locker(&m_poolMutex);
        for (auto& connection : m_connectionPool) {
            if (connection->database.isOpen()) {
                connection->database.close();
            }
        }
    }
    
    // Backup current database
    QString currentBackup = m_databasePath + ".restore_backup";
    if (QFile::exists(m_databasePath)) {
        if (QFile::exists(currentBackup)) {
            QFile::remove(currentBackup);
        }
        if (!QFile::copy(m_databasePath, currentBackup)) {
            logWarning(QString("Failed to create restore backup: %1").arg(currentBackup));
        }
    }
    
    // Remove current database
    if (QFile::exists(m_databasePath)) {
        if (!QFile::remove(m_databasePath)) {
            QString error = QString("Failed to remove current database: %1").arg(m_databasePath);
            logError(error);
            emit restoreCompleted(false, backupPath);
            return false;
        }
    }
    
    // Copy backup to database location
    if (!QFile::copy(backupPath, m_databasePath)) {
        QString error = QString("Failed to restore database from %1 to %2").arg(backupPath, m_databasePath);
        logError(error);
        
        // Try to restore from backup
        if (QFile::exists(currentBackup)) {
            QFile::copy(currentBackup, m_databasePath);
        }
        
        emit restoreCompleted(false, backupPath);
        return false;
    }
    
    // Test restored database
    auto testConnection = createConnection();
    if (!testConnection || !testConnection->database.open()) {
        QString error = "Restored database failed validation";
        logError(error);
        
        // Restore from backup
        if (QFile::exists(currentBackup)) {
            QFile::remove(m_databasePath);
            QFile::copy(currentBackup, m_databasePath);
        }
        
        emit restoreCompleted(false, backupPath);
        return false;
    }
    
    // Return test connection to pool
    m_connectionPool.push_back(std::move(testConnection));
    
    // Clean up restore backup
    if (QFile::exists(currentBackup)) {
        QFile::remove(currentBackup);
    }
    
    logDebug(QString("Database restore completed successfully from: %1").arg(backupPath));
    emit restoreCompleted(true, backupPath);
    return true;
}

bool DatabaseService::optimizeDatabase()
{
    logDebug("Starting database optimization...");
    
    bool success = executeTransaction([this]() {
        QSqlQuery query = createQuery();
        
        // VACUUM to reclaim space and defragment
        if (!query.exec("VACUUM")) {
            logSqlError(query.lastError(), "VACUUM", "Database optimization");
            return false;
        }
        
        // ANALYZE to update query planner statistics
        if (!query.exec("ANALYZE")) {
            logSqlError(query.lastError(), "ANALYZE", "Database optimization");
            return false;
        }
        
        return true;
    });
    
    if (success) {
        QMutexLocker locker(&m_statsMutex);
        m_lastOptimization = QDateTime::currentDateTime();
        locker.unlock();
        
        logDebug("Database optimization completed successfully");
    } else {
        logError("Database optimization failed");
    }
    
    emit optimizationCompleted(success);
    return success;
}

bool DatabaseService::checkIntegrity()
{
    logDebug("Checking database integrity...");
    
    QList<QVariantMap> results = executeSelect("PRAGMA integrity_check");
    
    if (results.isEmpty()) {
        logError("Failed to check database integrity");
        return false;
    }
    
    // Check results
    for (const QVariantMap& row : results) {
        QString result = row.value("integrity_check").toString();
        if (result != "ok") {
            logError(QString("Database integrity check failed: %1").arg(result));
            return false;
        }
    }
    
    logDebug("Database integrity check passed");
    return true;
}

QVariantMap DatabaseService::getDatabaseStats()
{
    QVariantMap stats;
    
    QMutexLocker locker(&m_statsMutex);
    stats["total_queries"] = m_totalQueries;
    stats["failed_queries"] = m_failedQueries;
    stats["total_transactions"] = m_totalTransactions;
    stats["failed_transactions"] = m_failedTransactions;
    stats["last_optimization"] = m_lastOptimization;
    locker.unlock();
    
    // Get database file size
    QFileInfo dbInfo(m_databasePath);
    if (dbInfo.exists()) {
        stats["database_size"] = dbInfo.size();
        stats["last_modified"] = dbInfo.lastModified();
    }
    
    // Get connection pool stats
    QMutexLocker poolLocker(&m_poolMutex);
    stats["max_connections"] = m_maxConnections;
    stats["active_connections"] = static_cast<int>(m_connectionPool.size());
    
    int inUseCount = 0;
    for (const auto& connection : m_connectionPool) {
        if (connection->inUse) {
            inUseCount++;
        }
    }
    stats["connections_in_use"] = inUseCount;
    
    return stats;
}

QList<DatabaseService::ConnectionInfo> DatabaseService::getConnectionInfo() const
{
    QList<ConnectionInfo> info;
    
    QMutexLocker locker(&m_poolMutex);
    for (const auto& connection : m_connectionPool) {
        ConnectionInfo connInfo;
        connInfo.connectionName = connection->connectionName;
        connInfo.databasePath = m_databasePath;
        connInfo.driverName = m_driverName;
        connInfo.isValid = connection->database.isValid() && connection->database.isOpen();
        connInfo.lastUsed = connection->lastUsed;
        connInfo.useCount = connection->useCount;
        info.append(connInfo);
    }
    
    return info;
}

void DatabaseService::setMaxConnections(int maxConnections)
{
    if (maxConnections < 1) {
        logWarning("Maximum connections must be at least 1");
        return;
    }
    
    QMutexLocker locker(&m_poolMutex);
    m_maxConnections = maxConnections;
    
    // Clean up excess connections if needed
    while (static_cast<int>(m_connectionPool.size()) > maxConnections) {
        auto it = m_connectionPool.end() - 1;
        if (!(*it)->inUse) {
            if ((*it)->database.isOpen()) {
                (*it)->database.close();
            }
            m_connectionPool.erase(it);
        } else {
            break; // Can't remove connections that are in use
        }
    }
    
    logDebug(QString("Maximum connections set to: %1").arg(maxConnections));
}

int DatabaseService::maxConnections() const
{
    QMutexLocker locker(&m_poolMutex);
    return m_maxConnections;
}

DatabaseService::PooledConnection* DatabaseService::getConnection()
{
    QMutexLocker locker(&m_poolMutex);
    
    // Look for available connection
    for (auto& connection : m_connectionPool) {
        if (!connection->inUse && connection->database.isValid() && connection->database.isOpen()) {
            connection->inUse = true;
            connection->lastUsed = QDateTime::currentDateTime();
            connection->useCount++;
            return connection.get();
        }
    }
    
    // Create new connection if under limit
    if (static_cast<int>(m_connectionPool.size()) < m_maxConnections) {
        auto newConnection = createConnection();
        if (newConnection && newConnection->database.isValid() && newConnection->database.open()) {
            newConnection->inUse = true;
            newConnection->lastUsed = QDateTime::currentDateTime();
            newConnection->useCount++;
            
            PooledConnection* ptr = newConnection.get();
            m_connectionPool.push_back(std::move(newConnection));
            return ptr;
        } else {
            logError("Failed to create new database connection");
        }
    }
    
    // Wait for connection to become available
    if (m_connectionAvailable.wait(&m_poolMutex, CONNECTION_TIMEOUT_MS)) {
        // Try again after wait
        for (auto& connection : m_connectionPool) {
            if (!connection->inUse && connection->database.isValid() && connection->database.isOpen()) {
                connection->inUse = true;
                connection->lastUsed = QDateTime::currentDateTime();
                connection->useCount++;
                return connection.get();
            }
        }
    }
    
    logError("Failed to get database connection (timeout or no connections available)");
    return nullptr;
}

void DatabaseService::returnConnection(PooledConnection* connection)
{
    if (!connection) {
        return;
    }
    
    QMutexLocker locker(&m_poolMutex);
    connection->inUse = false;
    connection->lastUsed = QDateTime::currentDateTime();
    
    // Notify waiting threads
    m_connectionAvailable.wakeOne();
}

std::unique_ptr<DatabaseService::PooledConnection> DatabaseService::createConnection()
{
    QString connectionName = QString("DatabaseService_%1_%2")
                           .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                           .arg(++m_connectionCounter);
    
    auto connection = std::make_unique<PooledConnection>();
    connection->connectionName = connectionName;
    connection->database = QSqlDatabase::addDatabase(m_driverName, connectionName);
    connection->database.setDatabaseName(m_databasePath);
    connection->lastUsed = QDateTime::currentDateTime();
    
    // Set SQLite-specific options for better performance and reliability
    connection->database.setConnectOptions("QSQLITE_BUSY_TIMEOUT=30000");
    
    logDebug(QString("Created database connection: %1").arg(connectionName));
    return connection;
}

bool DatabaseService::initializeSchema()
{
    logDebug("Initializing database schema...");
    
    // Use the test connection that was already created and validated
    if (m_connectionPool.empty()) {
        logError("No database connections available for schema initialization");
        return false;
    }
    
    PooledConnection* connection = m_connectionPool[0].get();
    if (!connection || !connection->database.isValid() || !connection->database.isOpen()) {
        logError("Invalid database connection for schema initialization");
        return false;
    }
    
    QSqlQuery query(connection->database);
    
    // Check if database has any tables
    if (!query.exec("SELECT name FROM sqlite_master WHERE type='table'")) {
        logSqlError(query.lastError(), "SELECT name FROM sqlite_master WHERE type='table'", "Schema check");
        return false;
    }
    
    bool hasExistingTables = query.next();
    
    if (!hasExistingTables) {
        logDebug("Creating initial database schema...");
        
        // Create basic tables - this is a minimal schema for now
        // The actual schema will be created by other services/repositories
        
        if (!query.exec("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY, applied_at DATETIME DEFAULT CURRENT_TIMESTAMP)")) {
            logSqlError(query.lastError(), "CREATE TABLE schema_version", "Schema creation");
            return false;
        }
        
        if (!query.exec("INSERT OR IGNORE INTO schema_version (version) VALUES (1)")) {
            logSqlError(query.lastError(), "INSERT INTO schema_version", "Schema initialization");
            return false;
        }
        
        logDebug("Initial database schema created");
    } else {
        logDebug("Database schema already exists");
    }
    
    return true;
}

void DatabaseService::cleanupConnections()
{
    QMutexLocker locker(&m_poolMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-300); // 5 minutes ago
    
    auto it = m_connectionPool.begin();
    while (it != m_connectionPool.end()) {
        if (!(*it)->inUse && (*it)->lastUsed < cutoff) {
            logDebug(QString("Cleaning up unused connection: %1").arg((*it)->connectionName));
            if ((*it)->database.isOpen()) {
                (*it)->database.close();
            }
            it = m_connectionPool.erase(it);
        } else {
            ++it;
        }
    }
}

void DatabaseService::onConnectionCleanupTimer()
{
    cleanupConnections();
}

void DatabaseService::logSqlError(const QSqlError& error, const QString& query, const QString& operation)
{
    QString errorMsg = QString("[%1] SQL Error: %2").arg(operation, error.text());
    if (!query.isEmpty()) {
        errorMsg += QString(" | Query: %1").arg(query);
    }
    
    logError(errorMsg);
    
    // Attempt recovery for certain types of errors
    if (isRecoverableError(error)) {
        attemptRecovery(error);
    }
}

bool DatabaseService::isRecoverableError(const QSqlError& error)
{
    // Check for recoverable SQLite errors
    QString errorText = error.text().toLower();
    
    return errorText.contains("database is locked") ||
           errorText.contains("database is busy") ||
           errorText.contains("disk i/o error");
}

bool DatabaseService::attemptRecovery(const QSqlError& error)
{
    QString errorText = error.text().toLower();
    
    if (errorText.contains("database is locked") || errorText.contains("database is busy")) {
        logDebug("Attempting recovery from database lock/busy error");
        
        // Close and recreate connections
        QMutexLocker locker(&m_poolMutex);
        for (auto& connection : m_connectionPool) {
            if (connection->database.isOpen()) {
                connection->database.close();
            }
        }
        
        // Wait a bit and try to reopen
        QThread::msleep(100);
        
        for (auto& connection : m_connectionPool) {
            if (!connection->database.open()) {
                logError(QString("Failed to reopen connection after recovery attempt: %1")
                        .arg(connection->database.lastError().text()));
                return false;
            }
        }
        
        logDebug("Database connection recovery completed");
        return true;
    }
    
    return false;
}