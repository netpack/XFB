#include "test_utils.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

bool TestUtils::createTestDatabase(const QString& connectionName)
{
    // Remove existing connection if it exists
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    // Create in-memory SQLite database for testing
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(":memory:");
    
    if (!db.open()) {
        qWarning() << "Failed to create test database:" << db.lastError().text();
        return false;
    }
    
    // Create basic test tables (simplified schema for testing)
    QSqlQuery query(db);
    
    // Music table
    if (!query.exec("CREATE TABLE music ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "title TEXT NOT NULL, "
                   "artist TEXT NOT NULL, "
                   "album TEXT, "
                   "genre TEXT, "
                   "file_path TEXT UNIQUE NOT NULL, "
                   "duration INTEGER DEFAULT 0, "
                   "date_added DATETIME DEFAULT CURRENT_TIMESTAMP, "
                   "play_count INTEGER DEFAULT 0"
                   ")")) {
        qWarning() << "Failed to create music table:" << query.lastError().text();
        return false;
    }
    
    // Genres table
    if (!query.exec("CREATE TABLE genres ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL"
                   ")")) {
        qWarning() << "Failed to create genres table:" << query.lastError().text();
        return false;
    }
    
    // Playlists table
    if (!query.exec("CREATE TABLE playlists ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT NOT NULL, "
                   "created_date DATETIME DEFAULT CURRENT_TIMESTAMP"
                   ")")) {
        qWarning() << "Failed to create playlists table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

void TestUtils::cleanupTestDatabase(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::database(connectionName).close();
        QSqlDatabase::removeDatabase(connectionName);
    }
}

QTemporaryDir* TestUtils::createTestDirectory()
{
    auto* tempDir = new QTemporaryDir();
    if (!tempDir->isValid()) {
        qWarning() << "Failed to create temporary directory";
        delete tempDir;
        return nullptr;
    }
    return tempDir;
}

bool TestUtils::createTestAudioFile(const QString& filePath, int durationMs)
{
    Q_UNUSED(durationMs) // For now, just create a dummy file
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create test audio file:" << filePath;
        return false;
    }
    
    // Write minimal WAV header for a valid audio file
    // This is a very basic WAV file structure for testing
    QByteArray wavHeader;
    wavHeader.append("RIFF");
    wavHeader.append(QByteArray(4, 0)); // File size (placeholder)
    wavHeader.append("WAVE");
    wavHeader.append("fmt ");
    wavHeader.append(QByteArray::fromHex("10000000")); // Format chunk size
    wavHeader.append(QByteArray::fromHex("0100")); // Audio format (PCM)
    wavHeader.append(QByteArray::fromHex("0100")); // Number of channels
    wavHeader.append(QByteArray::fromHex("44AC0000")); // Sample rate (44100)
    wavHeader.append(QByteArray::fromHex("88580100")); // Byte rate
    wavHeader.append(QByteArray::fromHex("0200")); // Block align
    wavHeader.append(QByteArray::fromHex("1000")); // Bits per sample
    wavHeader.append("data");
    wavHeader.append(QByteArray(4, 0)); // Data size (placeholder)
    
    file.write(wavHeader);
    
    // Write some dummy audio data
    QByteArray dummyData(1024, 0);
    file.write(dummyData);
    
    file.close();
    return true;
}

// XFBTestBase implementation
void XFBTestBase::initTestCase()
{
    // Set up test environment
    QCoreApplication::setApplicationName("XFB_Test");
    QCoreApplication::setOrganizationName("Netpack_Test");
    
    // Create temporary directory
    m_tempDir = TestUtils::createTestDirectory();
    QVERIFY(m_tempDir != nullptr);
    
    // Create test database
    QVERIFY(TestUtils::createTestDatabase(m_testDbConnection));
}

void XFBTestBase::cleanupTestCase()
{
    // Clean up database
    TestUtils::cleanupTestDatabase(m_testDbConnection);
    
    // Clean up temporary directory
    delete m_tempDir;
    m_tempDir = nullptr;
}

void XFBTestBase::init()
{
    // Per-test initialization
}

void XFBTestBase::cleanup()
{
    // Per-test cleanup
}

// MOC file will be generated automatically