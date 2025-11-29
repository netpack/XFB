#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QString>

/**
 * @brief Utility class for common test operations
 */
class TestUtils
{
public:
    /**
     * @brief Create a temporary test database
     * @param connectionName Name for the database connection
     * @return True if database was created successfully
     */
    static bool createTestDatabase(const QString& connectionName = "test_db");
    
    /**
     * @brief Clean up test database
     * @param connectionName Name of the database connection to clean up
     */
    static void cleanupTestDatabase(const QString& connectionName = "test_db");
    
    /**
     * @brief Create a temporary directory for test files
     * @return QTemporaryDir object (caller owns)
     */
    static QTemporaryDir* createTestDirectory();
    
    /**
     * @brief Create a test audio file for testing
     * @param filePath Path where to create the test file
     * @param durationMs Duration in milliseconds
     * @return True if file was created successfully
     */
    static bool createTestAudioFile(const QString& filePath, int durationMs = 5000);
    
    /**
     * @brief Wait for a condition to be true with timeout
     * @param condition Lambda function returning bool
     * @param timeoutMs Timeout in milliseconds
     * @return True if condition became true within timeout
     */
    template<typename Func>
    static bool waitForCondition(Func condition, int timeoutMs = 5000) {
        QElapsedTimer timer;
        timer.start();
        
        while (!condition() && timer.elapsed() < timeoutMs) {
            QTest::qWait(10);
            QCoreApplication::processEvents();
        }
        
        return condition();
    }
};

/**
 * @brief Base class for XFB tests with common setup
 */
class XFBTestBase : public QObject
{
    Q_OBJECT
    
protected slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();
    virtual void init();
    virtual void cleanup();
    
protected:
    QTemporaryDir* m_tempDir = nullptr;
    QString m_testDbConnection = "xfb_test";
};

#endif // TEST_UTILS_H