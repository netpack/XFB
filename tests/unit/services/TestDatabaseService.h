#ifndef TESTDATABASESERVICE_UNIT_H
#define TESTDATABASESERVICE_UNIT_H

#include <QObject>
#include <QTest>
#include <QTemporaryDir>
#include <memory>

class DatabaseService;

/**
 * @brief Unit tests for DatabaseService
 * 
 * These tests focus on individual methods and components of DatabaseService
 * without requiring full database integration.
 */
class TestDatabaseServiceUnit : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor and basic property tests
    void testConstructor();
    void testDatabasePathProperty();
    void testMaxConnectionsProperty();
    void testServiceName();

    // State management tests
    void testInitialState();
    void testStateTransitions();
    void testMultipleInitialization();
    void testShutdownFromDifferentStates();

    // Error handling tests
    void testInvalidPathHandling();
    void testMissingDriverHandling();
    void testConnectionFailureHandling();

    // Configuration tests
    void testDefaultConfiguration();
    void testConfigurationValidation();

private:
    std::unique_ptr<DatabaseService> m_databaseService;
    QTemporaryDir m_tempDir;
    QString m_testDatabasePath;
};

#endif // TESTDATABASESERVICE_UNIT_H