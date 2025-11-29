#ifndef TESTLOGGER_H
#define TESTLOGGER_H

#include <QObject>
#include <QTest>
#include <QTemporaryDir>
#include <memory>

class Logger;

/**
 * @brief Unit tests for Logger class
 * 
 * Tests the file-based logging system including:
 * - Log file creation and writing
 * - File rotation based on size
 * - Log level filtering
 * - Thread safety
 * - File cleanup and maintenance
 */
class TestLogger : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testInitializationWithParameters();
    void testInitializationFailure();

    // Logging tests
    void testWriteLog();
    void testLogLevels();
    void testLogFormatting();
    void testLogCategories();

    // File management tests
    void testLogFileCreation();
    void testLogFileRotation();
    void testLogFileCleanup();
    void testMaxFileLimit();

    // Configuration tests
    void testSetMinLevel();
    void testGetCurrentLogFile();
    void testGetLogDirectory();
    void testIsEnabled();

    // Utility tests
    void testLevelToString();
    void testFlush();

    // Thread safety tests
    void testConcurrentLogging();

    // Error scenarios
    void testInvalidDirectory();
    void testDiskFull(); // Simulated
    void testPermissionDenied(); // Simulated

    // Maintenance tests
    void testPeriodicMaintenance();
    void testRotateIfNeeded();
    void testCleanupOldFiles();

private:
    void writeLogsToFillFile(Logger* logger, int count = 1000);
    void verifyLogFileExists(const QString& logDir);
    void verifyLogFileContent(const QString& logFile, const QString& expectedContent);
    int countLogFiles(const QString& logDir);
    QString getLogFileContent(const QString& logFile);

    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_testLogDir;
};

#endif // TESTLOGGER_H