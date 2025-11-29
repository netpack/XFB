#ifndef TESTERRORHANDLER_H
#define TESTERRORHANDLER_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <memory>

class ErrorHandler;

/**
 * @brief Unit tests for ErrorHandler class
 * 
 * Tests the centralized error handling and logging system including:
 * - Error handling with different severity levels
 * - Logging functionality with file rotation
 * - User dialog display
 * - Signal emission
 * - Thread safety
 */
class TestErrorHandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testSingletonInstance();
    void testInitialization();
    void testInitializationWithCustomDirectory();
    void testInitializationFailure();

    // Error handling tests
    void testHandleErrorInfo();
    void testHandleErrorWarning();
    void testHandleErrorError();
    void testHandleErrorCritical();
    void testHandleErrorWithDetails();
    void testHandleErrorWithCategory();

    // Logging tests
    void testLogMessage();
    void testLogLevels();
    void testLogCategories();
    void testLogFileCreation();
    void testLogFileRotation();

    // User dialog tests
    void testShowUserError();
    void testShowUserWarning();
    void testShowUserInfo();
    void testUserDialogsDisabled();

    // Signal emission tests
    void testErrorOccurredSignal();
    void testMessageLoggedSignal();

    // Configuration tests
    void testSetLogLevel();
    void testSetUserDialogsEnabled();
    void testGetLogDirectory();

    // Utility function tests
    void testSeverityToString();
    void testCategoryToString();

    // Thread safety tests
    void testThreadSafety();

    // Error scenarios
    void testInvalidLogDirectory();
    void testLogFilePermissionError();

private:
    void waitForSignals(int timeoutMs = 1000);
    QString createTestLogDirectory();
    void verifyLogFileExists(const QString& logDir);
    void verifyLogFileContent(const QString& logFile, const QString& expectedContent);

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_testLogDir;
};

#endif // TESTERRORHANDLER_H