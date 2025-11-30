#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "BaseService.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief Service for managing database connections, transactions, and operations
 * 
 * The DatabaseService provides a high-level interface for database operations
 * with connection pooling, transaction management, backup/restore functionality,
 * and proper error handling. It ensures thread-safe access to the database
 * and manages connection lifecycle automatically.
 * 
 * Features:
 * - Connection pooling for improved performance
 * - Transaction management with automatic rollback on failure
 * - Database backup and restore functionality
 * - Thread-safe operations
 * - Automatic connection recovery
 * - Query performance monitoring
 * 
 * @example
 * @code
 * DatabaseService* dbService = container->resolve<DatabaseService>();
 * 
 * // Execute a transaction
 * bool success = dbService->executeTransaction([&]() {
 *     QSqlQuery query = dbService->createQuery();
 *     query.prepare("INSERT INTO music (title, artist) VALUES (?, ?)");
 *     query.addBindValue("Song Title");
 *     query.addBindValue("Artist Name");
 *     return query.exec();
 * });
 * 
 * // Backup database
 * dbService->backup("/path/to/backup.db");
 * @endcode
 * 
 * @since XFB 2.0
 */
class DatabaseService : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Transaction function type
     */
    using TransactionFunction = std::function<bool()>;

    /**
     * @brief Database connection information
     */
    struct ConnectionInfo {
        QString connectionName;
        QString databasePath;
        QString driverName;
        bool isValid = false;
        QDateTime lastUsed;
        int useCount = 0;
    };

    explicit DatabaseService(QObject* parent = nullptr);
    ~DatabaseService() override;

    // BaseService interface
    QString getServiceName() const override { return "DatabaseService"; }

    /**
     * @brief Set the database file path
     * @param databasePath Path to the SQLite database file
     */
    void setDatabasePath(const QString& databasePath);

    /**
     * @brief Get the current database path
     * @return Database file path
     */
    QString databasePath() const;

    /**
     * @brief Create a new query object using a pooled connection
     * @return QSqlQuery object ready for use
     */
    QSqlQuery createQuery();

    /**
     * @brief Execute a transaction with automatic rollback on failure
     * @param transaction Function containing the transaction logic
     * @return true if transaction completed successfully
     */
    bool executeTransaction(TransactionFunction transaction);

    /**
     * @brief Execute a simple query with error handling
     * @param queryString SQL query string
     * @param bindValues Optional bind values for prepared statements
     * @return true if query executed successfully
     */
    bool executeQuery(const QString& queryString, const QVariantList& bindValues = QVariantList());

    /**
     * @brief Execute a SELECT query and return results
     * @param queryString SQL SELECT query
     * @param bindValues Optional bind values for prepared statements
     * @return List of result rows as QVariantMap
     */
    QList<QVariantMap> executeSelect(const QString& queryString, const QVariantList& bindValues = QVariantList());

    /**
     * @brief Backup the database to a file
     * @param backupPath Path where backup should be saved
     * @return true if backup was successful
     */
    bool backup(const QString& backupPath);

    /**
     * @brief Restore database from a backup file
     * @param backupPath Path to the backup file
     * @return true if restore was successful
     */
    bool restore(const QString& backupPath);

    /**
     * @brief Optimize database performance (VACUUM, ANALYZE)
     * @return true if optimization completed successfully
     */
    bool optimizeDatabase();

    /**
     * @brief Check database integrity
     * @return true if database integrity is OK
     */
    bool checkIntegrity();

    /**
     * @brief Get database statistics
     * @return Map containing database statistics
     */
    QVariantMap getDatabaseStats();

    /**
     * @brief Get connection pool information
     * @return List of connection information
     */
    QList<ConnectionInfo> getConnectionInfo() const;

    /**
     * @brief Set maximum number of connections in the pool
     * @param maxConnections Maximum connections (default: 5)
     */
    void setMaxConnections(int maxConnections);

    /**
     * @brief Get maximum number of connections
     * @return Maximum connections in pool
     */
    int maxConnections() const;

signals:
    /**
     * @brief Emitted when a database error occurs
     * @param error Error message
     * @param query SQL query that caused the error (if applicable)
     */
    void databaseError(const QString& error, const QString& query = QString());

    /**
     * @brief Emitted when backup operation completes
     * @param success true if backup was successful
     * @param backupPath Path to the backup file
     */
    void backupCompleted(bool success, const QString& backupPath);

    /**
     * @brief Emitted when restore operation completes
     * @param success true if restore was successful
     * @param backupPath Path to the backup file used
     */
    void restoreCompleted(bool success, const QString& backupPath);

    /**
     * @brief Emitted when database optimization completes
     * @param success true if optimization was successful
     */
    void optimizationCompleted(bool success);

    /**
     * @brief Emitted when a transaction is started
     */
    void transactionStarted();

    /**
     * @brief Emitted when a transaction is committed
     */
    void transactionCommitted();

    /**
     * @brief Emitted when a transaction is rolled back
     * @param reason Reason for rollback
     */
    void transactionRolledBack(const QString& reason);

protected:
    // BaseService interface
    bool doInitialize() override;
    void doShutdown() override;

private slots:
    void onConnectionCleanupTimer();

private:
    /**
     * @brief Connection pool entry
     */
    struct PooledConnection {
        QSqlDatabase database;
        QString connectionName;
        QDateTime lastUsed;
        bool inUse = false;
        int useCount = 0;
    };

    /**
     * @brief Get a connection from the pool
     * @return Pooled connection, or invalid connection if none available
     */
    PooledConnection* getConnection();

    /**
     * @brief Return a connection to the pool
     * @param connection Connection to return
     */
    void returnConnection(PooledConnection* connection);

    /**
     * @brief Create a new database connection
     * @return New pooled connection
     */
    std::unique_ptr<PooledConnection> createConnection();

    /**
     * @brief Initialize database schema if needed
     * @return true if schema initialization was successful
     */
    bool initializeSchema();

    /**
     * @brief Clean up unused connections
     */
    void cleanupConnections();

    /**
     * @brief Log SQL error with context
     * @param error SQL error
     * @param query Query that caused the error
     * @param operation Operation being performed
     */
    void logSqlError(const QSqlError& error, const QString& query, const QString& operation);

    /**
     * @brief Check if error is recoverable
     * @param error SQL error to check
     * @return true if error might be recoverable
     */
    bool isRecoverableError(const QSqlError& error);

    /**
     * @brief Attempt to recover from database error
     * @param error SQL error
     * @return true if recovery was attempted
     */
    bool attemptRecovery(const QSqlError& error);

    QString m_databasePath;
    QString m_driverName;
    int m_maxConnections;
    int m_connectionCounter;
    
    mutable QMutex m_poolMutex;
    std::vector<std::unique_ptr<PooledConnection>> m_connectionPool;
    QWaitCondition m_connectionAvailable;
    
    QTimer* m_cleanupTimer;
    
    // Statistics
    mutable QMutex m_statsMutex;
    int m_totalQueries;
    int m_failedQueries;
    int m_totalTransactions;
    int m_failedTransactions;
    QDateTime m_lastOptimization;
    
    static constexpr int DEFAULT_MAX_CONNECTIONS = 5;
    static constexpr int CONNECTION_TIMEOUT_MS = 30000; // 30 seconds
    static constexpr int CLEANUP_INTERVAL_MS = 60000;   // 1 minute
};

#endif // DATABASESERVICE_H