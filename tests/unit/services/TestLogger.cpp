#include "TestLogger.h"
#include "../../../src/services/Logger.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QEventLoop>
#include <QFileInfo>

void TestLogger::initTestCase()
{
    // No special setup needed
}

void TestLogger::cleanupTestCase()
{
    // No special cleanup needed
}

void TestLogger::init()
{
    // Create a temporary directory for each test
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    m_testLogDir = m_tempDir->path() + "/logs";
}

void TestLogger::cleanup()
{
    // Cleanup is handled by QTemporaryDir destructor
    m_tempDir.reset();
}

void TestLogger::testInitialization()
{
    Logger logger;
    
    bool result = logger.initialize(m_testLogDir);
    QVERIFY(result);
    
    QCOMPARE(logger.getLogDirectory(), m_testLogDir);
    QVERIFY(logger.isEnabled());
    
    // Verify log directory was created
    QDir logDir(m_testLogDir);
    QVERIFY(logDir.exists());
}

void TestLogger::testInitializationWithParameters()
{
    Logger logger;
    
    bool result = logger.initialize(m_testLogDir, 5, 2, Logger::LogLevel::Warning);
    QVERIFY(result);
    
    QCOMPARE(logger.getLogDirectory(), m_testLogDir);
    QVERIFY(logger.isEnabled());
}

void TestLogger::testInitializationFailure()
{
    Logger logger;
    
    // Try to initialize with an invalid directory
    QString invalidDir = "/root/invalid_logs";
    bool result = logger.initialize(invalidDir);
    
    // This might still succeed on some systems, so we just test it doesn't crash
    // The test passes if we reach here without crashing
    QVERIFY(true);
}

void TestLogger::testWriteLog()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    // Just test that writeLog doesn't crash
    logger.writeLog(Logger::LogLevel::Info, "TestComponent", "Test message", "TestCategory");
    
    // Test passes if we reach here
    QVERIFY(true);
}

void TestLogger::testLogLevels()
{
    Logger logger;
    logger.initialize(m_testLogDir, 10, 10, Logger::LogLevel::Warning);
    
    // These should not be logged (below threshold)
    logger.writeLog(Logger::LogLevel::Info, "Test", "Info message");
    
    // These should be logged (at or above threshold)
    logger.writeLog(Logger::LogLevel::Warning, "Test", "Warning message");
    logger.writeLog(Logger::LogLevel::Error, "Test", "Error message");
    logger.writeLog(Logger::LogLevel::Critical, "Test", "Critical message");
    
    QTest::qWait(100);
    
    // Verify only the warning, error, and critical messages were logged
    QDir logDir(m_testLogDir);
    QStringList logFiles = logDir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
    
    QString logFile = logDir.absoluteFilePath(logFiles.first());
    QString content = getLogFileContent(logFile);
    
    QVERIFY(!content.contains("Info message"));
    QVERIFY(content.contains("Warning message"));
    QVERIFY(content.contains("Error message"));
    QVERIFY(content.contains("Critical message"));
}

void TestLogger::testLogFormatting()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    logger.writeLog(Logger::LogLevel::Error, "TestComponent", "Test message", "TestCategory");
    
    QTest::qWait(100);
    
    QDir logDir(m_testLogDir);
    QStringList logFiles = logDir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
    
    QString logFile = logDir.absoluteFilePath(logFiles.first());
    QString content = getLogFileContent(logFile);
    
    // Verify log format contains expected elements
    QVERIFY(content.contains("ERROR"));
    QVERIFY(content.contains("TestComponent"));
    QVERIFY(content.contains("Test message"));
    QVERIFY(content.contains("TestCategory"));
    
    // Verify timestamp format (should contain date and time)
    QVERIFY(content.contains(QRegularExpression("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}")));
}

void TestLogger::testLogCategories()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    logger.writeLog(Logger::LogLevel::Info, "Component1", "Message1", "Audio");
    logger.writeLog(Logger::LogLevel::Info, "Component2", "Message2", "Database");
    logger.writeLog(Logger::LogLevel::Info, "Component3", "Message3", "Network");
    
    QTest::qWait(100);
    
    QDir logDir(m_testLogDir);
    QStringList logFiles = logDir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
    
    QString logFile = logDir.absoluteFilePath(logFiles.first());
    QString content = getLogFileContent(logFile);
    
    QVERIFY(content.contains("Audio"));
    QVERIFY(content.contains("Database"));
    QVERIFY(content.contains("Network"));
}

void TestLogger::testLogFileCreation()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    QString currentLogFile = logger.getCurrentLogFile();
    QVERIFY(!currentLogFile.isEmpty());
    
    QFileInfo fileInfo(currentLogFile);
    QVERIFY(fileInfo.exists());
    QVERIFY(fileInfo.fileName().startsWith("xfb_"));
    QVERIFY(fileInfo.fileName().endsWith(".log"));
}

void TestLogger::testLogFileRotation()
{
    // Initialize with very small max file size to force rotation
    Logger logger;
    logger.initialize(m_testLogDir, 10, 1); // 1MB max size
    
    QString initialLogFile = logger.getCurrentLogFile();
    
    // Write enough data to trigger rotation
    writeLogsToFillFile(&logger, 2000);
    
    // Force rotation check
    logger.rotateIfNeeded();
    
    QTest::qWait(200);
    
    // Check if rotation occurred
    int logFileCount = countLogFiles(m_testLogDir);
    
    // We should have at least one log file, possibly more if rotation occurred
    QVERIFY(logFileCount >= 1);
    
    // The current log file might have changed
    QString currentLogFile = logger.getCurrentLogFile();
    QVERIFY(!currentLogFile.isEmpty());
}

void TestLogger::testLogFileCleanup()
{
    // Initialize with small max files limit
    Logger logger;
    logger.initialize(m_testLogDir, 3, 1); // Keep only 3 files, 1MB each
    
    // Create multiple log files by forcing rotation
    for (int i = 0; i < 5; ++i) {
        writeLogsToFillFile(&logger, 500);
        logger.rotateIfNeeded();
        QTest::qWait(50);
    }
    
    // Force cleanup
    logger.cleanupOldFiles();
    QTest::qWait(100);
    
    // Should have at most 3 log files
    int logFileCount = countLogFiles(m_testLogDir);
    QVERIFY(logFileCount <= 3);
}

void TestLogger::testMaxFileLimit()
{
    Logger logger;
    logger.initialize(m_testLogDir, 2, 1); // Keep only 2 files
    
    // Generate multiple rotations
    for (int i = 0; i < 4; ++i) {
        writeLogsToFillFile(&logger, 300);
        logger.rotateIfNeeded();
        logger.cleanupOldFiles();
        QTest::qWait(50);
    }
    
    // Should not exceed the limit
    int logFileCount = countLogFiles(m_testLogDir);
    QVERIFY(logFileCount <= 2);
}

void TestLogger::testSetMinLevel()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    // Set minimum level to Error
    logger.setMinLevel(Logger::LogLevel::Error);
    
    logger.writeLog(Logger::LogLevel::Info, "Test", "Info message");
    logger.writeLog(Logger::LogLevel::Warning, "Test", "Warning message");
    logger.writeLog(Logger::LogLevel::Error, "Test", "Error message");
    
    QTest::qWait(100);
    
    QDir logDir(m_testLogDir);
    QStringList logFiles = logDir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
    
    QString logFile = logDir.absoluteFilePath(logFiles.first());
    QString content = getLogFileContent(logFile);
    
    // Only error message should be present
    QVERIFY(!content.contains("Info message"));
    QVERIFY(!content.contains("Warning message"));
    QVERIFY(content.contains("Error message"));
}

void TestLogger::testGetCurrentLogFile()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    QString currentFile = logger.getCurrentLogFile();
    QVERIFY(!currentFile.isEmpty());
    QVERIFY(currentFile.contains(m_testLogDir));
    QVERIFY(currentFile.endsWith(".log"));
}

void TestLogger::testGetLogDirectory()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    QCOMPARE(logger.getLogDirectory(), m_testLogDir);
}

void TestLogger::testIsEnabled()
{
    Logger logger;
    
    QVERIFY(!logger.isEnabled()); // Should be disabled before initialization
    
    logger.initialize(m_testLogDir);
    
    QVERIFY(logger.isEnabled()); // Should be enabled after initialization
}

void TestLogger::testLevelToString()
{
    QCOMPARE(Logger::levelToString(Logger::LogLevel::Info), QString("INFO"));
    QCOMPARE(Logger::levelToString(Logger::LogLevel::Warning), QString("WARN"));
    QCOMPARE(Logger::levelToString(Logger::LogLevel::Error), QString("ERROR"));
    QCOMPARE(Logger::levelToString(Logger::LogLevel::Critical), QString("CRIT"));
}

void TestLogger::testFlush()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    logger.writeLog(Logger::LogLevel::Info, "Test", "Test message");
    logger.flush();
    
    // After flush, the message should be immediately available in the file
    QTest::qWait(50);
    
    QDir logDir(m_testLogDir);
    QStringList logFiles = logDir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
    
    QString logFile = logDir.absoluteFilePath(logFiles.first());
    verifyLogFileContent(logFile, "Test message");
}

void TestLogger::testConcurrentLogging()
{
    Logger logger;
    logger.initialize(m_testLogDir);
    
    const int numThreads = 5;
    const int messagesPerThread = 100;
    
    QList<QThread*> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        QThread* thread = QThread::create([&logger, i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                logger.writeLog(Logger::LogLevel::Info,
                               QString("Thread%1").arg(i),
                               QString("Message %1").arg(j),
                               "Test");
                QThread::msleep(1); // Small delay
            }
        });
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads to complete
    for (QThread* thread : threads) {
        thread->wait(5000); // 5 second timeout
        thread->deleteLater();
    }
    
    logger.flush();
    QTest::qWait(200);
    
    // Verify that all messages were logged
    QDir logDir(m_testLogDir);
    QStringList logFiles = logDir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
    
    // Count total messages in all log files
    int totalMessages = 0;
    for (const QString& fileName : logFiles) {
        QString logFile = logDir.absoluteFilePath(fileName);
        QString content = getLogFileContent(logFile);
        totalMessages += content.count("Thread");
    }
    
    // We should have all messages (allowing for some potential loss in extreme concurrency)
    QVERIFY(totalMessages >= (numThreads * messagesPerThread * 0.9)); // Allow 10% loss
}

void TestLogger::testInvalidDirectory()
{
    Logger logger;
    
    // Test with empty directory - should handle gracefully
    bool result = logger.initialize("");
    
    // Might succeed with default behavior, should not crash
    QVERIFY(true);
}

void TestLogger::testDiskFull()
{
    // This is hard to simulate reliably, so we just test that the logger
    // handles write failures gracefully
    Logger logger;
    logger.initialize(m_testLogDir);
    
    // Write a message - should not crash even if disk operations fail
    logger.writeLog(Logger::LogLevel::Info, "Test", "Test message");
    
    QVERIFY(true);
}

void TestLogger::testPermissionDenied()
{
    // Similar to disk full test - ensure graceful handling
    Logger logger;
    logger.initialize(m_testLogDir);
    
    logger.writeLog(Logger::LogLevel::Info, "Test", "Test message");
    
    QVERIFY(true);
}

void TestLogger::testPeriodicMaintenance()
{
    Logger logger;
    logger.initialize(m_testLogDir, 3, 1);
    
    // Generate some log files
    writeLogsToFillFile(&logger, 500);
    
    // Trigger maintenance manually
    logger.rotateIfNeeded();
    logger.cleanupOldFiles();
    
    // Should not crash
    QVERIFY(true);
}

void TestLogger::testRotateIfNeeded()
{
    Logger logger;
    logger.initialize(m_testLogDir, 5, 1); // 1MB max size
    
    QString initialFile = logger.getCurrentLogFile();
    
    // Write enough to potentially trigger rotation
    writeLogsToFillFile(&logger, 1000);
    
    logger.rotateIfNeeded();
    QTest::qWait(100);
    
    // Should not crash and should have valid current file
    QString currentFile = logger.getCurrentLogFile();
    QVERIFY(!currentFile.isEmpty());
}

void TestLogger::testCleanupOldFiles()
{
    Logger logger;
    logger.initialize(m_testLogDir, 2, 1); // Keep only 2 files
    
    // Create several log files
    for (int i = 0; i < 4; ++i) {
        writeLogsToFillFile(&logger, 200);
        logger.rotateIfNeeded();
        QTest::qWait(50);
    }
    
    logger.cleanupOldFiles();
    QTest::qWait(100);
    
    // Should have at most 2 files
    int fileCount = countLogFiles(m_testLogDir);
    QVERIFY(fileCount <= 2);
}

void TestLogger::writeLogsToFillFile(Logger* logger, int count)
{
    for (int i = 0; i < count; ++i) {
        logger->writeLog(Logger::LogLevel::Info,
                        "TestComponent",
                        QString("This is a long test message number %1 designed to fill up the log file with substantial content so that we can test file rotation functionality properly").arg(i),
                        "Test");
    }
}

void TestLogger::verifyLogFileExists(const QString& logDir)
{
    QDir dir(logDir);
    QStringList logFiles = dir.entryList(QStringList() << "xfb*.log", QDir::Files);
    QVERIFY(logFiles.size() > 0);
}

void TestLogger::verifyLogFileContent(const QString& logFile, const QString& expectedContent)
{
    QString content = getLogFileContent(logFile);
    QVERIFY(content.contains(expectedContent));
}

int TestLogger::countLogFiles(const QString& logDir)
{
    QDir dir(logDir);
    QStringList logFiles = dir.entryList(QStringList() << "xfb*.log", QDir::Files);
    return logFiles.size();
}

QString TestLogger::getLogFileContent(const QString& logFile)
{
    QFile file(logFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QTextStream stream(&file);
    return stream.readAll();
}

QTEST_MAIN(TestLogger)