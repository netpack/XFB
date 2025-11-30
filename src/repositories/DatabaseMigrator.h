#ifndef DATABASEMIGRATOR_H
#define DATABASEMIGRATOR_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>
#include <QDateTime>
#include <QMutex>
#include <QStringList>
#include <functional>
#include <memory>

/**
 * @brief Data model representing a database migration
 * 
 * This structure represents a database migration with its metadata
 * and execution information.
 */
struct Migration {
    int id = -1;
    QString version;        // Migration version (e.g., "001", "002", etc.)
    QString name;           // Human-readable migration name
    QString description;    // Migration description
    QString upSql;          // SQL to apply the migration
    QString downSql;        // SQL to rollback the migration
    QDateTime appliedAt;    // When the migration was applied
    bool isApplied = false; // Whether the migration has been applied
    
    /**
     * @brief Check if the migration has valid required fields
     * @return true if migration is valid
     */
    bool isValid() const {
        return !version.isEmpty() && !name.isEmpty() && !upSql.isEmpty();
    }
    
    /**
     * @brief Convert to QVariantMap for database operations
     * @return QVariantMap representation
     */
    QVariantMap toVariantMap() const {
        QVariantMap map;
        if (id != -1) map["id"] = id;
        map["version"] = version;
        map["name"] = name;
        map["description"] = description;
        map["up_sql"] = upSql;
        map["down_sql"] = downSql;
        map["applied_at"] = appliedAt;
        map["is_applied"] = isApplied;
        return map;
    }
    
    /**
     * @brief Create Migration from QVariantMap
     * @param map QVariantMap from database query
     * @return Migration instance
     */
    static Migration fromVariantMap(const QVariantMap& map) {
        Migration migration;
        migration.id = map.value("id", -1).toInt();
        migration.version = map.value("version").toString();
        migration.name = map.value("name").toString();
        migration.description = map.value("description").toString();
        migration.upSql = map.value("up_sql").toString();
        migration.downSql = map.value("down_sql").toString();
        migration.appliedAt = map.value("applied_at").toDateTime();
        migration.isApplied = map.value("is_applied", false).toBool();
        return migration;
    }
};

/**
 * @brief Database migration system for managing schema updates
 * 
 * The DatabaseMigrator provides a comprehensive system for managing
 * database schema changes over time. It supports forward migrations,
 * rollbacks, version tracking, and safe migration execution.
 * 
 * Features:
 * - Version-based migration system
 * - Forward and backward migration support
 * - Transaction-based migration execution
 * - Migration history tracking
 * - Rollback capabilities
 * - Validation and error handling
 * - Thread-safe operations
 * 
 * @example
 * @code
 * DatabaseMigrator* migrator = new DatabaseMigrator(database, this);
 * 
 * // Initialize migration system
 * migrator->initialize();
 * 
 * // Add a migration
 * Migration migration;
 * migration.version = "001";
 * migration.name = "Add indexes to music table";
 * migration.upSql = "CREATE INDEX idx_music_artist ON musics(artist);";
 * migration.downSql = "DROP INDEX idx_music_artist;";
 * migrator->addMigration(migration);
 * 
 * // Run pending migrations
 * migrator->migrate();
 * @endcode
 * 
 * @since XFB 2.0
 */
class DatabaseMigrator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Migration execution result
     */
    struct MigrationResult {
        bool success = false;
        QString error;
        QStringList appliedMigrations;
        QStringList failedMigrations;
        int totalMigrations = 0;
        int appliedCount = 0;
    };

    /**
     * @brief Migration statistics
     */
    struct MigrationStats {
        int totalMigrations = 0;
        int appliedMigrations = 0;
        int pendingMigrations = 0;
        QString currentVersion;
        QString latestVersion;
        QDateTime lastMigration;
    };

    explicit DatabaseMigrator(QSqlDatabase& database, QObject* parent = nullptr);
    ~DatabaseMigrator() override;

    /**
     * @brief Initialize the migration system
     * Creates the migrations table if it doesn't exist
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Add a migration to the system
     * @param migration Migration to add
     * @return true if successful, false otherwise
     */
    bool addMigration(const Migration& migration);

    /**
     * @brief Run all pending migrations
     * @return MigrationResult with execution details
     */
    MigrationResult migrate();

    /**
     * @brief Run migrations up to a specific version
     * @param targetVersion Target version to migrate to
     * @return MigrationResult with execution details
     */
    MigrationResult migrateTo(const QString& targetVersion);

    /**
     * @brief Rollback to a specific version
     * @param targetVersion Target version to rollback to
     * @return MigrationResult with execution details
     */
    MigrationResult rollbackTo(const QString& targetVersion);

    /**
     * @brief Rollback the last N migrations
     * @param count Number of migrations to rollback
     * @return MigrationResult with execution details
     */
    MigrationResult rollback(int count = 1);

    /**
     * @brief Get the current database version
     * @return Current version string, empty if no migrations applied
     */
    QString getCurrentVersion();

    /**
     * @brief Get the latest available migration version
     * @return Latest version string, empty if no migrations available
     */
    QString getLatestVersion();

    /**
     * @brief Get all migrations
     * @param appliedOnly If true, return only applied migrations
     * @return List of Migration objects
     */
    QList<Migration> getMigrations(bool appliedOnly = false);

    /**
     * @brief Get pending migrations
     * @return List of pending Migration objects
     */
    QList<Migration> getPendingMigrations();

    /**
     * @brief Get applied migrations
     * @return List of applied Migration objects
     */
    QList<Migration> getAppliedMigrations();

    /**
     * @brief Get migration by version
     * @param version Migration version
     * @return Migration object if found, invalid Migration otherwise
     */
    Migration getMigrationByVersion(const QString& version);

    /**
     * @brief Check if a migration version exists
     * @param version Migration version to check
     * @return true if migration exists, false otherwise
     */
    bool migrationExists(const QString& version);

    /**
     * @brief Check if a migration has been applied
     * @param version Migration version to check
     * @return true if migration is applied, false otherwise
     */
    bool isMigrationApplied(const QString& version);

    /**
     * @brief Get migration statistics
     * @return MigrationStats structure with migration information
     */
    MigrationStats getStatistics();

    /**
     * @brief Validate migration SQL
     * @param sql SQL to validate
     * @return Error message if invalid, empty string if valid
     */
    QString validateMigrationSql(const QString& sql);

    /**
     * @brief Create a backup of the database before migration
     * @param backupPath Path where backup should be saved
     * @return true if backup was successful, false otherwise
     */
    bool createBackup(const QString& backupPath);

    /**
     * @brief Restore database from backup
     * @param backupPath Path to the backup file
     * @return true if restore was successful, false otherwise
     */
    bool restoreFromBackup(const QString& backupPath);

    /**
     * @brief Reset migration system (removes all migration records)
     * WARNING: This will mark all migrations as not applied
     * @return true if successful, false otherwise
     */
    bool reset();

signals:
    /**
     * @brief Emitted when a migration starts
     * @param version Migration version
     * @param name Migration name
     */
    void migrationStarted(const QString& version, const QString& name);

    /**
     * @brief Emitted when a migration completes successfully
     * @param version Migration version
     * @param name Migration name
     */
    void migrationCompleted(const QString& version, const QString& name);

    /**
     * @brief Emitted when a migration fails
     * @param version Migration version
     * @param name Migration name
     * @param error Error message
     */
    void migrationFailed(const QString& version, const QString& name, const QString& error);

    /**
     * @brief Emitted when a rollback starts
     * @param version Migration version being rolled back
     * @param name Migration name
     */
    void rollbackStarted(const QString& version, const QString& name);

    /**
     * @brief Emitted when a rollback completes successfully
     * @param version Migration version
     * @param name Migration name
     */
    void rollbackCompleted(const QString& version, const QString& name);

    /**
     * @brief Emitted when a rollback fails
     * @param version Migration version
     * @param name Migration name
     * @param error Error message
     */
    void rollbackFailed(const QString& version, const QString& name, const QString& error);

    /**
     * @brief Emitted when migration batch starts
     * @param totalMigrations Total number of migrations to process
     */
    void migrationBatchStarted(int totalMigrations);

    /**
     * @brief Emitted when migration batch completes
     * @param result Migration result
     */
    void migrationBatchCompleted(const MigrationResult& result);

    /**
     * @brief Emitted when an operation error occurs
     * @param operation Name of the operation that failed
     * @param error Error message
     */
    void operationError(const QString& operation, const QString& error);

private:
    /**
     * @brief Create the migrations table
     * @return true if successful, false otherwise
     */
    bool createMigrationsTable();

    /**
     * @brief Execute a single migration
     * @param migration Migration to execute
     * @param isRollback true if this is a rollback operation
     * @return true if successful, false otherwise
     */
    bool executeMigration(const Migration& migration, bool isRollback = false);

    /**
     * @brief Mark migration as applied
     * @param migration Migration to mark as applied
     * @return true if successful, false otherwise
     */
    bool markMigrationAsApplied(const Migration& migration);

    /**
     * @brief Mark migration as not applied (for rollbacks)
     * @param migration Migration to mark as not applied
     * @return true if successful, false otherwise
     */
    bool markMigrationAsNotApplied(const Migration& migration);

    /**
     * @brief Compare migration versions
     * @param version1 First version
     * @param version2 Second version
     * @return -1 if version1 < version2, 0 if equal, 1 if version1 > version2
     */
    int compareVersions(const QString& version1, const QString& version2);

    /**
     * @brief Sort migrations by version
     * @param migrations List of migrations to sort
     * @param ascending true for ascending order, false for descending
     * @return Sorted list of migrations
     */
    QList<Migration> sortMigrationsByVersion(const QList<Migration>& migrations, bool ascending = true);

    /**
     * @brief Log migration operation error
     * @param operation Operation name
     * @param error Error message
     * @param query SQL query (optional)
     */
    void logError(const QString& operation, const QString& error, const QString& query = QString());

    /**
     * @brief Execute a prepared query with error handling
     * @param query Prepared QSqlQuery
     * @param operation Operation name for error logging
     * @return true if successful, false otherwise
     */
    bool executeQuery(QSqlQuery& query, const QString& operation);

    QSqlDatabase& m_database;
    mutable QMutex m_mutex;
    bool m_initialized = false;
    
    // Migration storage
    QList<Migration> m_migrations;
    
    // Statistics cache
    mutable QMutex m_statsMutex;
    mutable MigrationStats m_cachedStats;
    mutable QDateTime m_statsLastUpdated;
    static constexpr int STATS_CACHE_DURATION_MS = 60000; // 1 minute
    
    // Migration table name
    static constexpr const char* MIGRATIONS_TABLE = "schema_migrations";
};

#endif // DATABASEMIGRATOR_H