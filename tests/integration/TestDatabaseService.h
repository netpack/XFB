#ifndef TESTDATABASESERVICE_H
#define TESTDATABASESERVICE_H

#include <QObject>
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <memory>

class DatabaseService;

/**
 * @brief Integration tests for DatabaseService
 * 
 * These tests verify the DatabaseService functionality including:
 * - Connection management and pooling
 * - Transaction handling with rollback
 * - Backup and restore operations
 * - Database optimization and integrity checks
 * - Error handling and recovery
 * - Thread safety
 */
class TestDatabaseService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testDatabasePathSetting();
    void testConnectionCreation();
    void testQueryExecution();
    void testSelectQueries();

    // Transaction tests
    void testSimpleTransaction();
    void testTransactionRollback();
    void testNestedTransactions();
    void testTransactionWithException();

    // Connection pooling tests
    void testConnectionPooling();
    void testMaxConnectionsLimit();
    void testConnectionReuse();
    void testConnectionCleanup();

    // Backup and restore tests
    void testDatabaseBackup();
    void testDatabaseRestore();
    void testBackupWithNonExistentSource();
    void testRestoreWithInvalidBackup();

    // Optimization and maintenance tests
    void testDatabaseOptimization();
    void testIntegrityCheck();
    void testDatabaseStats();

    // Error handling tests
    void testInvalidDatabasePath();
    void testCorruptedDatabase();
    void testDiskSpaceError();
    void testConcurrentAccess();

    // Performance tests
    void testLargeTransactionPerformance();
    void testConcurrentQueryPerformance();
    void testConnectionPoolPerformance();

private:
    void createTestTables();
    void insertTestData();
    void verifyTestData();
    void corruptDatabase();
    
    std::unique_ptr<DatabaseService> m_databaseService;
    QTemporaryDir m_tempDir;
    QString m_testDatabasePath;
    
    // Test data
    static constexpr int TEST_RECORD_COUNT = 1000;
    static constexpr int CONCURRENT_THREAD_COUNT = 10;
};

#endif // TESTDATABASESERVICE_H