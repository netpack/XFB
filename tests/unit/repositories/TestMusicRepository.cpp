#include "TestMusicRepository.h"
#include "../../../src/repositories/MusicRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

void TestMusicRepository::initTestCase()
{
    // Ensure we have a valid temporary directory
    QVERIFY(m_tempDir.isValid());
    
    // Set up database path
    m_databasePath = m_tempDir.path() + "/test_music.db";
    
    // Create test music files
    createTestMusicFiles();
}

void TestMusicRepository::cleanupTestCase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
    QSqlDatabase::removeDatabase("test_music_connection");
}

void TestMusicRepository::init()
{
    // Set up fresh database for each test
    setupTestDatabase();
    
    // Create repository instance
    m_repository = std::make_unique<MusicRepository>(m_database, this);
}

void TestMusicRepository::cleanup()
{
    // Clean up repository
    m_repository.reset();
    
    // Close database connection
    if (m_database.isOpen()) {
        m_database.close();
    }
}

void TestMusicRepository::setupTestDatabase()
{
    // Remove existing connection if any
    if (QSqlDatabase::contains("test_music_connection")) {
        QSqlDatabase::removeDatabase("test_music_connection");
    }
    
    // Create new database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE", "test_music_connection");
    m_database.setDatabaseName(m_databasePath);
    
    QVERIFY(m_database.open());
    
    // Create the musics table with the same schema as the real database
    QSqlQuery query(m_database);
    QString createTableQuery = R"(
        CREATE TABLE musics (
            "id" INTEGER PRIMARY KEY AUTOINCREMENT,
            "artist" VARCHAR(50) NOT NULL,
            "song" VARCHAR(25) NOT NULL,
            "genre1" VARCHAR(25) NOT NULL,
            "genre2" VARCHAR(25),
            "country" VARCHAR(25),
            "published_date" VARCHAR(25),
            "path" TEXT,
            "time" TEXT,
            "played_times" INTEGER DEFAULT 0,
            "last_played" TEXT
        )
    )";
    
    QVERIFY2(query.exec(createTableQuery), qPrintable(query.lastError().text()));
}

void TestMusicRepository::createTestMusicFiles()
{
    // Create some test audio files
    QStringList extensions = {"mp3", "wav", "flac", "ogg"};
    
    for (int i = 0; i < 4; ++i) {
        QString fileName = QString("test_song_%1.%2").arg(i + 1).arg(extensions[i]);
        QString filePath = m_tempDir.path() + "/" + fileName;
        
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        
        // Write some dummy content
        QTextStream stream(&file);
        stream << "Dummy audio file content for testing";
        file.close();
        
        m_testMusicFiles.append(filePath);
    }
    
    // Create a subdirectory with more test files
    QDir subDir(m_tempDir.path() + "/subdir");
    QVERIFY(subDir.mkpath("."));
    
    for (int i = 0; i < 2; ++i) {
        QString fileName = QString("sub_song_%1.mp3").arg(i + 1);
        QString filePath = subDir.absoluteFilePath(fileName);
        
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        
        QTextStream stream(&file);
        stream << "Dummy audio file content in subdirectory";
        file.close();
        
        m_testMusicFiles.append(filePath);
    }
}

MusicItem TestMusicRepository::createValidMusicItem(const QString& artist, const QString& song, const QString& path)
{
    MusicItem music;
    music.artist = artist;
    music.song = song;
    music.genre1 = "Rock";
    music.genre2 = "Alternative";
    music.country = "USA";
    music.publishedDate = "2023";
    music.path = path.isEmpty() ? m_testMusicFiles.first() : path;
    music.time = "3:45";
    music.playedTimes = 0;
    music.lastPlayed = "";
    
    return music;
}

void TestMusicRepository::insertTestData()
{
    // Insert some test data directly into database
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO musics (artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    // Test data
    QList<QVariantList> testData = {
        {"Artist A", "Song 1", "Rock", "Alternative", "USA", "2020", "/path/song1.mp3", "3:30", 5, "2023-01-01"},
        {"Artist B", "Song 2", "Pop", "", "UK", "2021", "/path/song2.mp3", "4:15", 10, "2023-01-02"},
        {"Artist A", "Song 3", "Rock", "Classic", "USA", "2019", "/path/song3.mp3", "2:45", 2, "2023-01-03"}
    };
    
    for (const auto& data : testData) {
        for (const auto& value : data) {
            query.addBindValue(value);
        }
        QVERIFY(query.exec());
    }
}

void TestMusicRepository::testAddMusic()
{
    MusicItem music = createValidMusicItem();
    
    QSignalSpy spy(m_repository.get(), &MusicRepository::musicAdded);
    
    bool result = m_repository->addMusic(music);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the music was added to database
    MusicItem retrieved = m_repository->getMusicById(1);
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.artist, music.artist);
    QCOMPARE(retrieved.song, music.song);
    QCOMPARE(retrieved.path, music.path);
}

void TestMusicRepository::testAddMusicWithInvalidData()
{
    MusicItem music; // Empty music item
    
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    bool result = m_repository->addMusic(music);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestMusicRepository::testAddMusicDuplicatePath()
{
    MusicItem music1 = createValidMusicItem("Artist 1", "Song 1");
    MusicItem music2 = createValidMusicItem("Artist 2", "Song 2", music1.path); // Same path
    
    // Add first music item
    QVERIFY(m_repository->addMusic(music1));
    
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    // Try to add second with same path
    bool result = m_repository->addMusic(music2);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestMusicRepository::testUpdateMusic()
{
    // First add a music item
    MusicItem music = createValidMusicItem();
    QVERIFY(m_repository->addMusic(music));
    
    // Get the added item with ID
    MusicItem retrieved = m_repository->getMusicById(1);
    QVERIFY(retrieved.isValid());
    
    // Update the item
    retrieved.artist = "Updated Artist";
    retrieved.song = "Updated Song";
    
    QSignalSpy spy(m_repository.get(), &MusicRepository::musicUpdated);
    
    bool result = m_repository->updateMusic(retrieved);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the update
    MusicItem updated = m_repository->getMusicById(1);
    QCOMPARE(updated.artist, QString("Updated Artist"));
    QCOMPARE(updated.song, QString("Updated Song"));
}

void TestMusicRepository::testUpdateMusicInvalidId()
{
    MusicItem music = createValidMusicItem();
    music.id = 999; // Non-existent ID
    
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    bool result = m_repository->updateMusic(music);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestMusicRepository::testDeleteMusic()
{
    // First add a music item
    MusicItem music = createValidMusicItem();
    QVERIFY(m_repository->addMusic(music));
    
    QSignalSpy spy(m_repository.get(), &MusicRepository::musicDeleted);
    
    bool result = m_repository->deleteMusic(1);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the item is deleted
    MusicItem retrieved = m_repository->getMusicById(1);
    QVERIFY(!retrieved.isValid());
}

void TestMusicRepository::testDeleteMusicInvalidId()
{
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    bool result = m_repository->deleteMusic(999);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestMusicRepository::testGetMusicById()
{
    insertTestData();
    
    MusicItem music = m_repository->getMusicById(1);
    
    QVERIFY(music.isValid());
    QCOMPARE(music.id, 1);
    QCOMPARE(music.artist, QString("Artist A"));
    QCOMPARE(music.song, QString("Song 1"));
}

void TestMusicRepository::testGetMusicByIdNotFound()
{
    MusicItem music = m_repository->getMusicById(999);
    
    QVERIFY(!music.isValid());
    QCOMPARE(music.id, -1);
}

void TestMusicRepository::testGetAllMusic()
{
    insertTestData();
    
    QList<MusicItem> allMusic = m_repository->getAllMusic();
    
    QCOMPARE(allMusic.size(), 3);
    
    // Should be ordered by artist, song
    QCOMPARE(allMusic[0].artist, QString("Artist A"));
    QCOMPARE(allMusic[0].song, QString("Song 1"));
    QCOMPARE(allMusic[1].artist, QString("Artist A"));
    QCOMPARE(allMusic[1].song, QString("Song 3"));
    QCOMPARE(allMusic[2].artist, QString("Artist B"));
    QCOMPARE(allMusic[2].song, QString("Song 2"));
}

void TestMusicRepository::testGetAllMusicWithLimitAndOffset()
{
    insertTestData();
    
    QList<MusicItem> limitedMusic = m_repository->getAllMusic(2, 1);
    
    QCOMPARE(limitedMusic.size(), 2);
    // Should skip first item and return next 2
    QCOMPARE(limitedMusic[0].song, QString("Song 3"));
    QCOMPARE(limitedMusic[1].song, QString("Song 2"));
}

void TestMusicRepository::testSearchMusic()
{
    insertTestData();
    
    MusicRepository::SearchCriteria criteria;
    criteria.artist = "Artist A";
    
    QList<MusicItem> results = m_repository->searchMusic(criteria);
    
    QCOMPARE(results.size(), 2);
    QCOMPARE(results[0].artist, QString("Artist A"));
    QCOMPARE(results[1].artist, QString("Artist A"));
}

void TestMusicRepository::testSearchMusicWithCriteria()
{
    insertTestData();
    
    MusicRepository::SearchCriteria criteria;
    criteria.genre1 = "Rock";
    criteria.country = "USA";
    criteria.limit = 1;
    
    QList<MusicItem> results = m_repository->searchMusic(criteria);
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].genre1, QString("Rock"));
    QCOMPARE(results[0].country, QString("USA"));
}

void TestMusicRepository::testGetMusicByGenre()
{
    insertTestData();
    
    QList<MusicItem> rockMusic = m_repository->getMusicByGenre("Rock");
    
    QCOMPARE(rockMusic.size(), 2);
    for (const auto& music : rockMusic) {
        QCOMPARE(music.genre1, QString("Rock"));
    }
}

void TestMusicRepository::testGetMusicByGenreWithGenre2()
{
    insertTestData();
    
    QList<MusicItem> alternativeMusic = m_repository->getMusicByGenre("Alternative", true);
    
    QCOMPARE(alternativeMusic.size(), 1);
    QCOMPARE(alternativeMusic[0].genre2, QString("Alternative"));
}

void TestMusicRepository::testGetMusicByArtist()
{
    insertTestData();
    
    QList<MusicItem> artistAMusic = m_repository->getMusicByArtist("Artist A");
    
    QCOMPARE(artistAMusic.size(), 2);
    for (const auto& music : artistAMusic) {
        QCOMPARE(music.artist, QString("Artist A"));
    }
}

void TestMusicRepository::testAddMusicBatch()
{
    QList<MusicItem> musicList;
    
    for (int i = 0; i < 3; ++i) {
        MusicItem music = createValidMusicItem(
            QString("Batch Artist %1").arg(i + 1),
            QString("Batch Song %1").arg(i + 1),
            m_testMusicFiles[i]
        );
        musicList.append(music);
    }
    
    int result = m_repository->addMusicBatch(musicList);
    
    QCOMPARE(result, 3);
    
    // Verify all items were added
    QList<MusicItem> allMusic = m_repository->getAllMusic();
    QCOMPARE(allMusic.size(), 3);
}

void TestMusicRepository::testAddMusicBatchWithInvalidItems()
{
    QList<MusicItem> musicList;
    
    // Add valid item
    musicList.append(createValidMusicItem("Valid Artist", "Valid Song", m_testMusicFiles[0]));
    
    // Add invalid item (empty artist)
    MusicItem invalid;
    invalid.song = "Invalid Song";
    invalid.path = m_testMusicFiles[1];
    musicList.append(invalid);
    
    // Add another valid item
    musicList.append(createValidMusicItem("Valid Artist 2", "Valid Song 2", m_testMusicFiles[2]));
    
    int result = m_repository->addMusicBatch(musicList);
    
    // Should add only the valid items
    QCOMPARE(result, 2);
}

void TestMusicRepository::testAddMusicBatchWithDuplicates()
{
    QList<MusicItem> musicList;
    
    // Add same path twice
    musicList.append(createValidMusicItem("Artist 1", "Song 1", m_testMusicFiles[0]));
    musicList.append(createValidMusicItem("Artist 2", "Song 2", m_testMusicFiles[0])); // Duplicate path
    
    int result = m_repository->addMusicBatch(musicList);
    
    // Should add only the first item
    QCOMPARE(result, 1);
}

void TestMusicRepository::testImportFromDirectory()
{
    int result = m_repository->importFromDirectory(m_tempDir.path(), false);
    
    // Should import the 4 files in the root directory
    QCOMPARE(result, 4);
    
    QList<MusicItem> allMusic = m_repository->getAllMusic();
    QCOMPARE(allMusic.size(), 4);
}

void TestMusicRepository::testImportFromDirectoryRecursive()
{
    int result = m_repository->importFromDirectory(m_tempDir.path(), true);
    
    // Should import all 6 files (4 in root + 2 in subdirectory)
    QCOMPARE(result, 6);
    
    QList<MusicItem> allMusic = m_repository->getAllMusic();
    QCOMPARE(allMusic.size(), 6);
}

void TestMusicRepository::testImportFromDirectoryNonExistent()
{
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    int result = m_repository->importFromDirectory("/non/existent/path");
    
    QCOMPARE(result, 0);
    QCOMPARE(errorSpy.count(), 1);
}

void TestMusicRepository::testIncrementPlayCount()
{
    insertTestData();
    
    // Get initial play count
    MusicItem music = m_repository->getMusicById(1);
    int initialPlayCount = music.playedTimes;
    
    bool result = m_repository->incrementPlayCount(1);
    
    QVERIFY(result);
    
    // Verify play count was incremented
    MusicItem updated = m_repository->getMusicById(1);
    QCOMPARE(updated.playedTimes, initialPlayCount + 1);
    QVERIFY(!updated.lastPlayed.isEmpty());
}

void TestMusicRepository::testGetStatistics()
{
    insertTestData();
    
    MusicRepository::MusicStats stats = m_repository->getStatistics();
    
    QCOMPARE(stats.totalTracks, 3);
    QCOMPARE(stats.totalArtists, 2); // Artist A and Artist B
    QCOMPARE(stats.totalGenres, 2); // Rock and Pop
    QCOMPARE(stats.totalPlays, 17); // 5 + 10 + 2
    QCOMPARE(stats.mostPlayedTrackId, 2); // Song 2 with 10 plays
}

void TestMusicRepository::testGetAllArtists()
{
    insertTestData();
    
    QStringList artists = m_repository->getAllArtists();
    
    QCOMPARE(artists.size(), 2);
    QVERIFY(artists.contains("Artist A"));
    QVERIFY(artists.contains("Artist B"));
    
    // Should be sorted
    QCOMPARE(artists[0], QString("Artist A"));
    QCOMPARE(artists[1], QString("Artist B"));
}

void TestMusicRepository::testGetAllGenres()
{
    insertTestData();
    
    QStringList genres = m_repository->getAllGenres(false);
    
    QCOMPARE(genres.size(), 2);
    QVERIFY(genres.contains("Rock"));
    QVERIFY(genres.contains("Pop"));
}

void TestMusicRepository::testGetAllGenresWithGenre2()
{
    insertTestData();
    
    QStringList genres = m_repository->getAllGenres(true);
    
    // Should include genres from both genre1 and genre2 fields
    QVERIFY(genres.size() >= 3); // At least Rock, Pop, Alternative, Classic
    QVERIFY(genres.contains("Rock"));
    QVERIFY(genres.contains("Pop"));
    QVERIFY(genres.contains("Alternative"));
    QVERIFY(genres.contains("Classic"));
}

void TestMusicRepository::testPathExists()
{
    insertTestData();
    
    bool exists = m_repository->pathExists("/path/song1.mp3");
    QVERIFY(exists);
    
    bool notExists = m_repository->pathExists("/path/nonexistent.mp3");
    QVERIFY(!notExists);
}

void TestMusicRepository::testValidateMusicItem()
{
    MusicItem validMusic = createValidMusicItem();
    
    QString error = m_repository->validateMusicItem(validMusic);
    QVERIFY(error.isEmpty());
}

void TestMusicRepository::testValidateMusicItemInvalidData()
{
    MusicItem invalidMusic;
    // Empty artist, song, and path
    
    QString error = m_repository->validateMusicItem(invalidMusic);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("Artist name is required"));
}

void TestMusicRepository::testDatabaseConnectionError()
{
    // Close the database to simulate connection error
    m_database.close();
    
    MusicItem music = createValidMusicItem();
    
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    bool result = m_repository->addMusic(music);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestMusicRepository::testTransactionRollback()
{
    // This test is more complex and would require mocking database failures
    // For now, we'll test the basic batch operation which uses transactions
    
    QList<MusicItem> musicList;
    
    // Add a valid item
    musicList.append(createValidMusicItem("Artist 1", "Song 1", m_testMusicFiles[0]));
    
    // Add an item with non-existent file path to cause validation failure
    MusicItem invalidPath = createValidMusicItem("Artist 2", "Song 2", "/non/existent/path.mp3");
    musicList.append(invalidPath);
    
    int result = m_repository->addMusicBatch(musicList);
    
    // Should add only the valid item
    QCOMPARE(result, 1);
}

void TestMusicRepository::testSignalEmission()
{
    QSignalSpy addedSpy(m_repository.get(), &MusicRepository::musicAdded);
    QSignalSpy updatedSpy(m_repository.get(), &MusicRepository::musicUpdated);
    QSignalSpy deletedSpy(m_repository.get(), &MusicRepository::musicDeleted);
    QSignalSpy errorSpy(m_repository.get(), &MusicRepository::operationError);
    
    // Test add signal
    MusicItem music = createValidMusicItem();
    m_repository->addMusic(music);
    QCOMPARE(addedSpy.count(), 1);
    
    // Test update signal
    MusicItem retrieved = m_repository->getMusicById(1);
    retrieved.artist = "Updated Artist";
    m_repository->updateMusic(retrieved);
    QCOMPARE(updatedSpy.count(), 1);
    
    // Test delete signal
    m_repository->deleteMusic(1);
    QCOMPARE(deletedSpy.count(), 1);
    
    // Test error signal
    MusicItem invalid;
    m_repository->addMusic(invalid);
    QCOMPARE(errorSpy.count(), 1);
}

QTEST_MAIN(TestMusicRepository)
#include "TestMusicRepository.moc"