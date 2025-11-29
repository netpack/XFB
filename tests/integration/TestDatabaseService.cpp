#include "TestDatabaseService.h"
#include "../../src/services/DatabaseService.h"
#include <QTest>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QDebug>

void TestDatabaseService::initTestCase()
{
    // Ensure temporary directory is valid
    QVERIFY(m_tempDir.isValid());
    
    // Set up test database path
    m_testDatabasePath = m_tempDir.path() + "/test_database.db";
    
    qDebug() << "Test database path:" << m_testDatabasePath;
}

void TestDatabaseService::cleanupTestCase()
{
    // Cleanup is automatic with QTemporaryDir
}

void TestDatabaseService::init()
{
    // Create fresh DatabaseService for each test
    m_databaseService = std::make_unique<DatabaseService>();
    m_databaseService->setDatabasePath(m_testDatabasePath);
    
    // Remove any existing test database
    if (QFile::exists(m_testDatabasePath)) {
        QFile::remove(m_testDatabasePath);
    }
}

void TestDatabaseService::cleanup()
{
    // Shutdown service
    if (m_databaseService && m_databaseService->isRunning()) {
        m_databaseService->shutdown();
    }
    m_databaseService.reset();
    
    // Clean up test database
    if (QFile::exists(m_testDatabasePath)) {
        QFile::remove(m_testDatabasePath);
    }
}

void TestDatabaseService::testInitialization()
{
    // Test successful initialization
    QVERIFY(m_databaseService->initialize());
    QVERIFY(m_databaseService->isRunning());
    QCOMPARE(m_databaseService->databasePath(), m_testDatabasePath);
    
    // Verify database file was created
    QVERIFY(QFile::exists(m_testDatabasePath));
    
    // Test that we can't initialize twice
    QVERIFY(m_databaseService->initialize()); // Should return true but not reinitialize
}

void TestDatabaseService::testDatabasePathSetting()
{
    QString newPath = m_tempDir.path() + "/another_test.db";
    
    // Should be able to set path before initialization
    m_databaseService->setDatabasePath(newPath);
    QCOMPARE(m_databaseService->databasePath(), newPath);
    
    // Initialize with new path
    QVERIFY(m_databaseService->initialize());
    QVERIFY(QFile::exists(newPath));
    
    // Should not be able to change path after initialization
    QString anotherPath = m_tempDir.path() + "/third_test.db";
    m_databaseService->setDatabasePath(anotherPath);
    QCOMPARE(m_databaseService->databasePath(), newPath); // Should remain unchanged
}

void TestDatabaseService::testConnectionCreation()
{
    QVERIFY(m_databaseService->initialize());
    
    // Test creating queries
    QSqlQuery query1 = m_databaseService->createQuery();
    QVERIFY(query1.isValid());
    
    QSqlQuery query2 = m_databaseService->createQuery();
    QVERIFY(query2.isValid());
    
    // Queries should use different connections or be properly managed
    QVERIFY(query1.exec("SELECT 1"));
    QVERIFY(query2.exec("SELECT 2"));
}

void TestDatabaseService::testQueryExecution()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    // Test simple query execution
    QVERIFY(m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES (?, ?)", 
                                           QVariantList() << "test1" << 100));
    
    // Test query with no bind values
    QVERIFY(m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('test2', 200)"));
    
    // Test invalid query
    QVERIFY(!m_databaseService->executeQuery("INVALID SQL STATEMENT"));
}

void TestDatabaseService::testSelectQueries()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    // Test simple select
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT * FROM test_table ORDER BY id");
    QVERIFY(!results.isEmpty());
    QCOMPARE(results.size(), 3);
    
    // Verify first record
    QCOMPARE(results[0]["name"].toString(), QString("test1"));
    QCOMPARE(results[0]["value"].toInt(), 100);
    
    // Test select with bind values
    results = m_databaseService->executeSelect("SELECT * FROM test_table WHERE value > ?", 
                                              QVariantList() << 150);
    QCOMPARE(results.size(), 2);
    
    // Test select with no results
    results = m_databaseService->executeSelect("SELECT * FROM test_table WHERE value > 1000");
    QVERIFY(results.isEmpty());
}

void TestDatabaseService::testSimpleTransaction()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    QSignalSpy transactionStartedSpy(m_databaseService.get(), &DatabaseService::transactionStarted);
    QSignalSpy transactionCommittedSpy(m_databaseService.get(), &DatabaseService::transactionCommitted);
    
    // Test successful transaction
    bool success = m_databaseService->executeTransaction([this]() {
        return m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('tx_test', 500)");
    });
    
    QVERIFY(success);
    QCOMPARE(transactionStartedSpy.count(), 1);
    QCOMPARE(transactionCommittedSpy.count(), 1);
    
    // Verify data was committed
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT * FROM test_table WHERE name = 'tx_test'");
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0]["value"].toInt(), 500);
}

void TestDatabaseService::testTransactionRollback()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    QSignalSpy transactionRolledBackSpy(m_databaseService.get(), &DatabaseService::transactionRolledBack);
    
    // Count records before transaction
    QList<QVariantMap> beforeResults = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    int recordCountBefore = beforeResults[0]["count"].toInt();
    
    // Test transaction that should rollback
    bool success = m_databaseService->executeTransaction([this]() {
        // Insert valid record
        if (!m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('rollback_test', 600)")) {
            return false;
        }
        
        // Execute invalid query to force rollback
        return m_databaseService->executeQuery("INVALID SQL THAT WILL FAIL");
    });
    
    QVERIFY(!success);
    QCOMPARE(transactionRolledBackSpy.count(), 1);
    
    // Verify rollback - record count should be unchanged
    QList<QVariantMap> afterResults = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    int recordCountAfter = afterResults[0]["count"].toInt();
    QCOMPARE(recordCountAfter, recordCountBefore);
    
    // Verify the test record was not inserted
    QList<QVariantMap> testResults = m_databaseService->executeSelect("SELECT * FROM test_table WHERE name = 'rollback_test'");
    QVERIFY(testResults.isEmpty());
}

void TestDatabaseService::testNestedTransactions()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    // SQLite doesn't support true nested transactions, but we should handle it gracefully
    bool success = m_databaseService->executeTransaction([this]() {
        // Insert first record
        if (!m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('outer', 100)")) {
            return false;
        }
        
        // Attempt nested transaction (should work as a single transaction)
        return m_databaseService->executeTransaction([this]() {
            return m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('inner', 200)");
        });
    });
    
    QVERIFY(success);
    
    // Verify both records were inserted
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    QCOMPARE(results[0]["count"].toInt(), 2);
}

void TestDatabaseService::testTransactionWithException()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    QSignalSpy transactionRolledBackSpy(m_databaseService.get(), &DatabaseService::transactionRolledBack);
    
    // Test transaction that throws exception
    bool success = m_databaseService->executeTransaction([this]() -> bool {
        m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('exception_test', 700)");
        throw std::runtime_error("Test exception");
        return true; // Never reached
    });
    
    QVERIFY(!success);
    QCOMPARE(transactionRolledBackSpy.count(), 1);
    
    // Verify rollback occurred
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT * FROM test_table WHERE name = 'exception_test'");
    QVERIFY(results.isEmpty());
}

void TestDatabaseService::testConnectionPooling()
{
    QVERIFY(m_databaseService->initialize());
    
    // Test default max connections
    QCOMPARE(m_databaseService->maxConnections(), 5);
    
    // Test setting max connections
    m_databaseService->setMaxConnections(3);
    QCOMPARE(m_databaseService->maxConnections(), 3);
    
    // Test connection info
    QList<DatabaseService::ConnectionInfo> connections = m_databaseService->getConnectionInfo();
    QVERIFY(connections.size() <= 3);
    
    // Create multiple queries to test pooling
    QList<QSqlQuery> queries;
    for (int i = 0; i < 5; ++i) {
        QSqlQuery query = m_databaseService->createQuery();
        if (query.isValid()) {
            queries.append(query);
        }
    }
    
    // Should have created connections up to the limit
    connections = m_databaseService->getConnectionInfo();
    QVERIFY(connections.size() <= 3);
}

void TestDatabaseService::testMaxConnectionsLimit()
{
    m_databaseService->setMaxConnections(2);
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    // Create queries up to the limit
    QSqlQuery query1 = m_databaseService->createQuery();
    QSqlQuery query2 = m_databaseService->createQuery();
    
    QVERIFY(query1.isValid());
    QVERIFY(query2.isValid());
    
    // Both should be able to execute
    QVERIFY(query1.exec("SELECT 1"));
    QVERIFY(query2.exec("SELECT 2"));
}

void TestDatabaseService::testConnectionReuse()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    // Get initial connection count
    QList<DatabaseService::ConnectionInfo> initialConnections = m_databaseService->getConnectionInfo();
    int initialCount = initialConnections.size();
    
    // Create and use multiple queries
    for (int i = 0; i < 10; ++i) {
        QSqlQuery query = m_databaseService->createQuery();
        QVERIFY(query.exec("SELECT 1"));
    }
    
    // Connection count should not have grown significantly due to reuse
    QList<DatabaseService::ConnectionInfo> finalConnections = m_databaseService->getConnectionInfo();
    QVERIFY(finalConnections.size() <= initialCount + m_databaseService->maxConnections());
}

void TestDatabaseService::testConnectionCleanup()
{
    QVERIFY(m_databaseService->initialize());
    
    // This test is difficult to verify automatically since cleanup happens on a timer
    // We'll just verify that the cleanup mechanism exists
    QList<DatabaseService::ConnectionInfo> connections = m_databaseService->getConnectionInfo();
    
    // Create some queries to populate the pool
    for (int i = 0; i < 3; ++i) {
        QSqlQuery query = m_databaseService->createQuery();
        query.exec("SELECT 1");
    }
    
    // Verify connections were created
    connections = m_databaseService->getConnectionInfo();
    QVERIFY(connections.size() > 0);
    
    // Note: Actual cleanup testing would require waiting for the timer or manually triggering cleanup
}

void TestDatabaseService::testDatabaseBackup()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    QString backupPath = m_tempDir.path() + "/backup.db";
    
    QSignalSpy backupCompletedSpy(m_databaseService.get(), &DatabaseService::backupCompleted);
    
    // Test successful backup
    QVERIFY(m_databaseService->backup(backupPath));
    QCOMPARE(backupCompletedSpy.count(), 1);
    
    QList<QVariant> arguments = backupCompletedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool()); // success
    QCOMPARE(arguments.at(1).toString(), backupPath);
    
    // Verify backup file exists and has content
    QVERIFY(QFile::exists(backupPath));
    QFileInfo backupInfo(backupPath);
    QVERIFY(backupInfo.size() > 0);
}

void TestDatabaseService::testDatabaseRestore()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    // Create backup
    QString backupPath = m_tempDir.path() + "/restore_backup.db";
    QVERIFY(m_databaseService->backup(backupPath));
    
    // Modify original database
    QVERIFY(m_databaseService->executeQuery("DELETE FROM test_table"));
    
    // Verify data is gone
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    QCOMPARE(results[0]["count"].toInt(), 0);
    
    QSignalSpy restoreCompletedSpy(m_databaseService.get(), &DatabaseService::restoreCompleted);
    
    // Test restore
    QVERIFY(m_databaseService->restore(backupPath));
    QCOMPARE(restoreCompletedSpy.count(), 1);
    
    QList<QVariant> arguments = restoreCompletedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool()); // success
    QCOMPARE(arguments.at(1).toString(), backupPath);
    
    // Verify data is restored
    results = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    QCOMPARE(results[0]["count"].toInt(), 3);
}

void TestDatabaseService::testBackupWithNonExistentSource()
{
    // Don't initialize - no database file should exist
    QString backupPath = m_tempDir.path() + "/nonexistent_backup.db";
    
    QSignalSpy backupCompletedSpy(m_databaseService.get(), &DatabaseService::backupCompleted);
    
    // Should fail
    QVERIFY(!m_databaseService->backup(backupPath));
    QCOMPARE(backupCompletedSpy.count(), 1);
    
    QList<QVariant> arguments = backupCompletedSpy.takeFirst();
    QVERIFY(!arguments.at(0).toBool()); // failure
}

void TestDatabaseService::testRestoreWithInvalidBackup()
{
    QVERIFY(m_databaseService->initialize());
    
    QString invalidBackupPath = m_tempDir.path() + "/invalid_backup.db";
    
    QSignalSpy restoreCompletedSpy(m_databaseService.get(), &DatabaseService::restoreCompleted);
    
    // Should fail - backup file doesn't exist
    QVERIFY(!m_databaseService->restore(invalidBackupPath));
    QCOMPARE(restoreCompletedSpy.count(), 1);
    
    QList<QVariant> arguments = restoreCompletedSpy.takeFirst();
    QVERIFY(!arguments.at(0).toBool()); // failure
}

void TestDatabaseService::testDatabaseOptimization()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    QSignalSpy optimizationCompletedSpy(m_databaseService.get(), &DatabaseService::optimizationCompleted);
    
    // Test optimization
    QVERIFY(m_databaseService->optimizeDatabase());
    QCOMPARE(optimizationCompletedSpy.count(), 1);
    
    QList<QVariant> arguments = optimizationCompletedSpy.takeFirst();
    QVERIFY(arguments.at(0).toBool()); // success
    
    // Verify optimization timestamp is updated
    QVariantMap stats = m_databaseService->getDatabaseStats();
    QVERIFY(stats.contains("last_optimization"));
    QVERIFY(stats["last_optimization"].toDateTime().isValid());
}

void TestDatabaseService::testIntegrityCheck()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    // Test integrity check on good database
    QVERIFY(m_databaseService->checkIntegrity());
}

void TestDatabaseService::testDatabaseStats()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    // Execute some queries to generate stats
    m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('stats_test', 123)");
    m_databaseService->executeQuery("INVALID QUERY"); // This should fail
    
    QVariantMap stats = m_databaseService->getDatabaseStats();
    
    // Verify stats structure
    QVERIFY(stats.contains("total_queries"));
    QVERIFY(stats.contains("failed_queries"));
    QVERIFY(stats.contains("total_transactions"));
    QVERIFY(stats.contains("failed_transactions"));
    QVERIFY(stats.contains("max_connections"));
    QVERIFY(stats.contains("active_connections"));
    
    // Verify some values
    QVERIFY(stats["total_queries"].toInt() > 0);
    QVERIFY(stats["failed_queries"].toInt() > 0);
    QCOMPARE(stats["max_connections"].toInt(), m_databaseService->maxConnections());
}

void TestDatabaseService::testInvalidDatabasePath()
{
    // Set invalid path
    m_databaseService->setDatabasePath("/invalid/path/that/does/not/exist/database.db");
    
    // Should fail to initialize
    QVERIFY(!m_databaseService->initialize());
    QVERIFY(!m_databaseService->isRunning());
}

void TestDatabaseService::testCorruptedDatabase()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    // Shutdown service
    m_databaseService->shutdown();
    
    // Corrupt the database file
    corruptDatabase();
    
    // Try to initialize again
    m_databaseService = std::make_unique<DatabaseService>();
    m_databaseService->setDatabasePath(m_testDatabasePath);
    
    // May or may not initialize depending on corruption level
    // But should handle gracefully without crashing
    bool initialized = m_databaseService->initialize();
    
    if (initialized) {
        // If it initialized, integrity check should fail
        QVERIFY(!m_databaseService->checkIntegrity());
    }
}

void TestDatabaseService::testDiskSpaceError()
{
    // This test is difficult to simulate reliably across platforms
    // We'll skip it for now but it would involve filling up disk space
    QSKIP("Disk space error simulation not implemented");
}

void TestDatabaseService::testConcurrentAccess()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    const int threadCount = 5;
    const int operationsPerThread = 20;
    
    QMutex resultMutex;
    QList<bool> results;
    
    // Create worker threads
    QList<QThread*> threads;
    
    for (int i = 0; i < threadCount; ++i) {
        QThread* thread = QThread::create([this, i, operationsPerThread, &resultMutex, &results]() {
            bool threadSuccess = true;
            
            for (int j = 0; j < operationsPerThread; ++j) {
                QString name = QString("thread_%1_op_%2").arg(i).arg(j);
                int value = i * 1000 + j;
                
                bool success = m_databaseService->executeTransaction([this, name, value]() {
                    return m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES (?, ?)",
                                                          QVariantList() << name << value);
                });
                
                if (!success) {
                    threadSuccess = false;
                    break;
                }
            }
            
            QMutexLocker locker(&resultMutex);
            results.append(threadSuccess);
        });
        
        threads.append(thread);
    }
    
    // Start all threads
    for (QThread* thread : threads) {
        thread->start();
    }
    
    // Wait for all threads to complete
    for (QThread* thread : threads) {
        QVERIFY(thread->wait(10000)); // 10 second timeout
        delete thread;
    }
    
    // Check results
    QCOMPARE(results.size(), threadCount);
    for (bool result : results) {
        QVERIFY(result);
    }
    
    // Verify all records were inserted
    QList<QVariantMap> finalResults = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    QCOMPARE(finalResults[0]["count"].toInt(), threadCount * operationsPerThread);
}

void TestDatabaseService::testLargeTransactionPerformance()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    QElapsedTimer timer;
    timer.start();
    
    // Insert many records in a single transaction
    bool success = m_databaseService->executeTransaction([this]() {
        for (int i = 0; i < TEST_RECORD_COUNT; ++i) {
            QString name = QString("perf_test_%1").arg(i);
            if (!m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES (?, ?)",
                                                QVariantList() << name << i)) {
                return false;
            }
        }
        return true;
    });
    
    qint64 elapsed = timer.elapsed();
    
    QVERIFY(success);
    qDebug() << "Large transaction (" << TEST_RECORD_COUNT << "records) took:" << elapsed << "ms";
    
    // Verify all records were inserted
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    QCOMPARE(results[0]["count"].toInt(), TEST_RECORD_COUNT);
    
    // Performance should be reasonable (less than 10 seconds for 1000 records)
    QVERIFY(elapsed < 10000);
}

void TestDatabaseService::testConcurrentQueryPerformance()
{
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    insertTestData();
    
    const int queryCount = 100;
    QElapsedTimer timer;
    timer.start();
    
    QMutex resultMutex;
    QList<bool> results;
    
    // Execute many concurrent queries
    QList<QThread*> threads;
    
    for (int i = 0; i < CONCURRENT_THREAD_COUNT; ++i) {
        QThread* thread = QThread::create([this, queryCount, &resultMutex, &results]() {
            bool threadSuccess = true;
            
            for (int j = 0; j < queryCount / CONCURRENT_THREAD_COUNT; ++j) {
                QList<QVariantMap> queryResults = m_databaseService->executeSelect("SELECT * FROM test_table WHERE value > ?",
                                                                                  QVariantList() << (j % 300));
                if (queryResults.isEmpty() && j % 300 < 200) {
                    // Should have results for values < 200
                    threadSuccess = false;
                    break;
                }
            }
            
            QMutexLocker locker(&resultMutex);
            results.append(threadSuccess);
        });
        
        threads.append(thread);
    }
    
    // Start all threads
    for (QThread* thread : threads) {
        thread->start();
    }
    
    // Wait for completion
    for (QThread* thread : threads) {
        QVERIFY(thread->wait(10000));
        delete thread;
    }
    
    qint64 elapsed = timer.elapsed();
    
    // Check results
    for (bool result : results) {
        QVERIFY(result);
    }
    
    qDebug() << "Concurrent queries (" << queryCount << "queries," << CONCURRENT_THREAD_COUNT << "threads) took:" << elapsed << "ms";
    
    // Performance should be reasonable
    QVERIFY(elapsed < 5000); // Less than 5 seconds
}

void TestDatabaseService::testConnectionPoolPerformance()
{
    m_databaseService->setMaxConnections(3);
    QVERIFY(m_databaseService->initialize());
    
    createTestTables();
    
    QElapsedTimer timer;
    timer.start();
    
    // Create many queries to test connection reuse
    for (int i = 0; i < 100; ++i) {
        QSqlQuery query = m_databaseService->createQuery();
        QVERIFY(query.exec("SELECT 1"));
    }
    
    qint64 elapsed = timer.elapsed();
    
    qDebug() << "Connection pool performance (100 queries, 3 max connections) took:" << elapsed << "ms";
    
    // Should be fast due to connection reuse
    QVERIFY(elapsed < 1000); // Less than 1 second
    
    // Verify connection count stayed within limits
    QList<DatabaseService::ConnectionInfo> connections = m_databaseService->getConnectionInfo();
    QVERIFY(connections.size() <= 3);
}

void TestDatabaseService::createTestTables()
{
    QVERIFY(m_databaseService->executeQuery(
        "CREATE TABLE IF NOT EXISTS test_table ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "value INTEGER DEFAULT 0, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    ));
}

void TestDatabaseService::insertTestData()
{
    QVERIFY(m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('test1', 100)"));
    QVERIFY(m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('test2', 200)"));
    QVERIFY(m_databaseService->executeQuery("INSERT INTO test_table (name, value) VALUES ('test3', 300)"));
}

void TestDatabaseService::verifyTestData()
{
    QList<QVariantMap> results = m_databaseService->executeSelect("SELECT COUNT(*) as count FROM test_table");
    QCOMPARE(results[0]["count"].toInt(), 3);
}

void TestDatabaseService::corruptDatabase()
{
    // Simple corruption - write random data to the beginning of the file
    QFile dbFile(m_testDatabasePath);
    if (dbFile.open(QIODevice::WriteOnly)) {
        dbFile.write("CORRUPTED_DATABASE_FILE");
        dbFile.close();
    }
}

QTEST_MAIN(TestDatabaseService)
#include "TestDatabaseService.moc"