#include "TestDatabaseService.h"
#include "../../../src/services/DatabaseService.h"
#include <QTest>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

void TestDatabaseServiceUnit::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    m_testDatabasePath = m_tempDir.path() + "/unit_test_database.db";
}

void TestDatabaseServiceUnit::cleanupTestCase()
{
    // Cleanup handled by QTemporaryDir
}

void TestDatabaseServiceUnit::init()
{
    m_databaseService = std::make_unique<DatabaseService>();
    
    // Remove any existing test database
    if (QFile::exists(m_testDatabasePath)) {
        QFile::remove(m_testDatabasePath);
    }
}

void TestDatabaseServiceUnit::cleanup()
{
    if (m_databaseService && m_databaseService->isRunning()) {
        m_databaseService->shutdown();
    }
    m_databaseService.reset();
    
    if (QFile::exists(m_testDatabasePath)) {
        QFile::remove(m_testDatabasePath);
    }
}

void TestDatabaseServiceUnit::testConstructor()
{
    // Test that constructor creates service in uninitialized state
    QVERIFY(m_databaseService != nullptr);
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Uninitialized);
    QVERIFY(!m_databaseService->isRunning());
    
    // Test default values
    QCOMPARE(m_databaseService->maxConnections(), 5); // Default value
    QVERIFY(!m_databaseService->databasePath().isEmpty()); // Should have default path
}

void TestDatabaseServiceUnit::testDatabasePathProperty()
{
    // Test initial path (should be default)
    QString initialPath = m_databaseService->databasePath();
    QVERIFY(!initialPath.isEmpty());
    QVERIFY(initialPath.contains("adb.db")); // Default database name
    
    // Test setting custom path
    m_databaseService->setDatabasePath(m_testDatabasePath);
    QCOMPARE(m_databaseService->databasePath(), m_testDatabasePath);
    
    // Test setting empty path
    m_databaseService->setDatabasePath("");
    QCOMPARE(m_databaseService->databasePath(), QString(""));
    
    // Test setting path with special characters
    QString specialPath = m_tempDir.path() + "/test with spaces & symbols.db";
    m_databaseService->setDatabasePath(specialPath);
    QCOMPARE(m_databaseService->databasePath(), specialPath);
}

void TestDatabaseServiceUnit::testMaxConnectionsProperty()
{
    // Test default value
    QCOMPARE(m_databaseService->maxConnections(), 5);
    
    // Test setting valid values
    m_databaseService->setMaxConnections(1);
    QCOMPARE(m_databaseService->maxConnections(), 1);
    
    m_databaseService->setMaxConnections(10);
    QCOMPARE(m_databaseService->maxConnections(), 10);
    
    m_databaseService->setMaxConnections(100);
    QCOMPARE(m_databaseService->maxConnections(), 100);
    
    // Test setting invalid values (should be ignored or clamped)
    int validValue = m_databaseService->maxConnections();
    m_databaseService->setMaxConnections(0);
    QCOMPARE(m_databaseService->maxConnections(), validValue); // Should remain unchanged
    
    m_databaseService->setMaxConnections(-1);
    QCOMPARE(m_databaseService->maxConnections(), validValue); // Should remain unchanged
}

void TestDatabaseServiceUnit::testServiceName()
{
    QCOMPARE(m_databaseService->serviceName(), QString("DatabaseService"));
    QCOMPARE(m_databaseService->getServiceName(), QString("DatabaseService"));
}

void TestDatabaseServiceUnit::testInitialState()
{
    // Test initial state
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Uninitialized);
    QVERIFY(!m_databaseService->isRunning());
    
    // Test that operations fail in uninitialized state
    QSqlQuery query = m_databaseService->createQuery();
    QVERIFY(!query.isValid()); // Should be invalid when service not initialized
}

void TestDatabaseServiceUnit::testStateTransitions()
{
    QSignalSpy stateChangedSpy(m_databaseService.get(), &DatabaseService::stateChanged);
    
    // Set valid database path
    m_databaseService->setDatabasePath(m_testDatabasePath);
    
    // Test initialization
    QVERIFY(m_databaseService->initialize());
    
    // Should have transitioned through Initializing to Running
    QVERIFY(stateChangedSpy.count() >= 2);
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Running);
    QVERIFY(m_databaseService->isRunning());
    
    // Test shutdown
    stateChangedSpy.clear();
    m_databaseService->shutdown();
    
    // Should have transitioned through Stopping to Stopped
    QVERIFY(stateChangedSpy.count() >= 1);
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Stopped);
    QVERIFY(!m_databaseService->isRunning());
}

void TestDatabaseServiceUnit::testMultipleInitialization()
{
    m_databaseService->setDatabasePath(m_testDatabasePath);
    
    // First initialization should succeed
    QVERIFY(m_databaseService->initialize());
    QVERIFY(m_databaseService->isRunning());
    
    // Second initialization should return true but not change state
    IService::ServiceState stateBefore = m_databaseService->state();
    QVERIFY(m_databaseService->initialize());
    QCOMPARE(m_databaseService->state(), stateBefore);
    QVERIFY(m_databaseService->isRunning());
}

void TestDatabaseServiceUnit::testShutdownFromDifferentStates()
{
    m_databaseService->setDatabasePath(m_testDatabasePath);
    
    // Test shutdown from uninitialized state (should be safe)
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Uninitialized);
    m_databaseService->shutdown(); // Should not crash
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Uninitialized);
    
    // Initialize and test shutdown from running state
    QVERIFY(m_databaseService->initialize());
    QVERIFY(m_databaseService->isRunning());
    
    m_databaseService->shutdown();
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Stopped);
    
    // Test shutdown from stopped state (should be safe)
    m_databaseService->shutdown(); // Should not crash
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Stopped);
}

void TestDatabaseServiceUnit::testInvalidPathHandling()
{
    QSignalSpy errorSpy(m_databaseService.get(), &DatabaseService::serviceError);
    
    // Test with completely invalid path
    m_databaseService->setDatabasePath("/this/path/does/not/exist/and/cannot/be/created/database.db");
    
    // Should fail to initialize
    QVERIFY(!m_databaseService->initialize());
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Error);
    QVERIFY(errorSpy.count() > 0);
    
    // Test with path that has invalid characters (platform dependent)
    errorSpy.clear();
    m_databaseService = std::make_unique<DatabaseService>(); // Reset service
    
#ifdef Q_OS_WIN
    m_databaseService->setDatabasePath("C:\\invalid<>path\\database.db");
#else
    m_databaseService->setDatabasePath("/invalid\0path/database.db");
#endif
    
    // May or may not fail depending on platform, but should handle gracefully
    bool result = m_databaseService->initialize();
    if (!result) {
        QCOMPARE(m_databaseService->state(), IService::ServiceState::Error);
    }
}

void TestDatabaseServiceUnit::testMissingDriverHandling()
{
    // This test is difficult to implement reliably since we can't easily
    // remove the SQLite driver. We'll skip it for now.
    QSKIP("Missing driver test not implemented - requires driver manipulation");
}

void TestDatabaseServiceUnit::testConnectionFailureHandling()
{
    QSignalSpy errorSpy(m_databaseService.get(), &DatabaseService::serviceError);
    
    // Set path to a directory instead of a file (should cause connection failure)
    m_databaseService->setDatabasePath(m_tempDir.path()); // Directory, not file
    
    // Should fail to initialize
    QVERIFY(!m_databaseService->initialize());
    QCOMPARE(m_databaseService->state(), IService::ServiceState::Error);
    QVERIFY(errorSpy.count() > 0);
    
    // Verify error message contains useful information
    QList<QVariant> errorArgs = errorSpy.first();
    QString errorMessage = errorArgs.first().toString();
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(errorMessage.contains("database") || errorMessage.contains("connection"));
}

void TestDatabaseServiceUnit::testDefaultConfiguration()
{
    // Test that default configuration is reasonable
    QCOMPARE(m_databaseService->maxConnections(), 5);
    
    QString defaultPath = m_databaseService->databasePath();
    QVERIFY(!defaultPath.isEmpty());
    QVERIFY(defaultPath.endsWith("adb.db"));
    
    // Default path should be in a reasonable location
    QVERIFY(defaultPath.contains("XFB") || defaultPath.contains("config"));
}

void TestDatabaseServiceUnit::testConfigurationValidation()
{
    // Test that configuration changes are validated
    
    // Valid max connections
    m_databaseService->setMaxConnections(1);
    QCOMPARE(m_databaseService->maxConnections(), 1);
    
    m_databaseService->setMaxConnections(100);
    QCOMPARE(m_databaseService->maxConnections(), 100);
    
    // Invalid max connections should be rejected
    int validValue = m_databaseService->maxConnections();
    
    m_databaseService->setMaxConnections(0);
    QCOMPARE(m_databaseService->maxConnections(), validValue);
    
    m_databaseService->setMaxConnections(-5);
    QCOMPARE(m_databaseService->maxConnections(), validValue);
    
    // Path validation is harder to test comprehensively, but we can test basic cases
    QString validPath = m_testDatabasePath;
    m_databaseService->setDatabasePath(validPath);
    QCOMPARE(m_databaseService->databasePath(), validPath);
    
    // Empty path should be allowed (might use default)
    m_databaseService->setDatabasePath("");
    QCOMPARE(m_databaseService->databasePath(), QString(""));
}

QTEST_MAIN(TestDatabaseServiceUnit)
#include "TestDatabaseService.moc"