#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "test_utils.h"

class TestDatabase : public XFBTestBase
{
    Q_OBJECT

private slots:
    void testDatabaseConnection();
    void testMusicTableOperations();
    void testGenreTableOperations();
    void testPlaylistTableOperations();

private:
    QSqlDatabase getTestDatabase();
};

QSqlDatabase TestDatabase::getTestDatabase()
{
    return QSqlDatabase::database(m_testDbConnection);
}

void TestDatabase::testDatabaseConnection()
{
    QSqlDatabase db = getTestDatabase();
    QVERIFY(db.isOpen());
    QVERIFY(db.isValid());
}

void TestDatabase::testMusicTableOperations()
{
    QSqlDatabase db = getTestDatabase();
    QSqlQuery query(db);
    
    // Test INSERT
    query.prepare("INSERT INTO music (title, artist, album, genre, file_path, duration) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue("Test Song");
    query.addBindValue("Test Artist");
    query.addBindValue("Test Album");
    query.addBindValue("Rock");
    query.addBindValue("/test/path/song.mp3");
    query.addBindValue(180000); // 3 minutes in milliseconds
    
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    
    // Test SELECT
    QVERIFY(query.exec("SELECT COUNT(*) FROM music"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    
    // Test SELECT with specific data
    query.prepare("SELECT title, artist FROM music WHERE file_path = ?");
    query.addBindValue("/test/path/song.mp3");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value("title").toString(), QString("Test Song"));
    QCOMPARE(query.value("artist").toString(), QString("Test Artist"));
    
    // Test UPDATE
    query.prepare("UPDATE music SET play_count = ? WHERE file_path = ?");
    query.addBindValue(5);
    query.addBindValue("/test/path/song.mp3");
    QVERIFY(query.exec());
    
    // Verify UPDATE
    query.prepare("SELECT play_count FROM music WHERE file_path = ?");
    query.addBindValue("/test/path/song.mp3");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 5);
    
    // Test DELETE
    query.prepare("DELETE FROM music WHERE file_path = ?");
    query.addBindValue("/test/path/song.mp3");
    QVERIFY(query.exec());
    
    // Verify DELETE
    QVERIFY(query.exec("SELECT COUNT(*) FROM music"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 0);
}

void TestDatabase::testGenreTableOperations()
{
    QSqlDatabase db = getTestDatabase();
    QSqlQuery query(db);
    
    // Test INSERT
    query.prepare("INSERT INTO genres (name) VALUES (?)");
    query.addBindValue("Rock");
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    
    // Test duplicate prevention (should fail)
    query.prepare("INSERT INTO genres (name) VALUES (?)");
    query.addBindValue("Rock");
    QVERIFY(!query.exec()); // Should fail due to UNIQUE constraint
    
    // Test SELECT
    QVERIFY(query.exec("SELECT COUNT(*) FROM genres"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
}

void TestDatabase::testPlaylistTableOperations()
{
    QSqlDatabase db = getTestDatabase();
    QSqlQuery query(db);
    
    // Test INSERT
    query.prepare("INSERT INTO playlists (name) VALUES (?)");
    query.addBindValue("My Test Playlist");
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    
    // Test SELECT
    query.prepare("SELECT name FROM playlists WHERE name = ?");
    query.addBindValue("My Test Playlist");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    QCOMPARE(query.value("name").toString(), QString("My Test Playlist"));
}

QTEST_MAIN(TestDatabase)
#include "test_database.moc"