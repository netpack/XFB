#ifndef TESTDATABASEMIGRATOR_H
#define TESTDATABASEMIGRATOR_H

#include <QObject>
#include <QTest>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <memory>

class DatabaseMigrator;
struct Migration;

/**
 * @brief Unit tests for DatabaseMigrator class
 * 
 * This test class provides comprehensive testing for all DatabaseMigrator
 * functionality including migration execution, rollbacks, version tracking,
 * validation, and error handling.
 */
class TestDatabaseMigrator : public QObject
{
    Q_OBJECT

private slots:
    // Test framework methods
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Initialization and setup
    void testInitialize();
    void testCreateMigrationsTable();

    // Migration management
    void testAddMigration();
    void testAddMigrationWithInvalidData();
    void testAddMigrationDuplicateVersion();
    void testAddMigrationInvalidSql();

    // Migration execution
    void testMigrate();
    void testMigrateEmpty();
    void testMigrateTo();
    void testMigrateToInvalidVersion();

    // Rollback functionality
    void testRollback();
    void testRollbackTo();
    void testRollbackWithoutDownSql();

    // Version management
    void testGetCurrentVersion();
    void testGetLatestVersion();
    void testCompareVersions();

    // Migration queries
    void testGetMigrations();
    void testGetPendingMigrations();
    void testGetAppliedMigrations();
    void testGetMigrationByVersion();
    void testMigrationExists();
    void testIsMigrationApplied();

    // Statistics and utilities
    void testGetStatistics();
    void testValidateMigrationSql();

    // Backup and restore
    void testCreateBackup();
    void testRestoreFromBackup();

    // System management
    void testReset();

    // Error handling
    void testDatabaseConnectionError();
    void testTransactionRollback();

    // Signal testing
    void testSignalEmission();

private:
    void setupTestDatabase();
    Migration createValidMigration(const QString& version = "001", 
                                  const QString& name = "Test Migration",
                                  const QString& upSql = "CREATE TABLE test_table (id INTEGER);",
                                  const QString& downSql = "DROP TABLE test_table;");
    void insertTestMigrations();

    std::unique_ptr<DatabaseMigrator> m_migrator;
    QSqlDatabase m_database;
    QTemporaryDir m_tempDir;
    QString m_databasePath;
};

#endif // TESTDATABASEMIGRATOR_H