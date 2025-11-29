#include "TestPlaylistRepository.h"
#include "../../../src/repositories/PlaylistRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSignalSpy>

void TestPlaylistRepository::initTestCase()
{
    // Ensure we have a valid temporary directory
    QVERIFY(m_tempDir.isValid());
    
    // Set up database path
    m_databasePath = m_tempDir.path() + "/test_playlist.db";
}

void TestPlaylistRepository::cleanupTestCase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
    QSqlDatabase::removeDatabase("test_playlist_connection");
}

void TestPlaylistRepository::init()
{
    // Set up fresh database for each test
    setupTestDatabase();
    
    // Create repository instance
    m_repository = std::make_unique<PlaylistRepository>(m_database, this);
}

void TestPlaylistRepository::cleanup()
{
    // Clean up repository
    m_repository.reset();
    
    // Close database connection
    if (m_database.isOpen()) {
        m_database.close();
    }
}

void TestPlaylistRepository::setupTestDatabase()
{
    // Remove existing connection if any
    if (QSqlDatabase::contains("test_playlist_connection")) {
        QSqlDatabase::removeDatabase("test_playlist_connection");
    }
    
    // Create new database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE", "test_playlist_connection");
    m_database.setDatabaseName(m_databasePath);
    
    QVERIFY(m_database.open());
    
    // Create the programs table with the same schema as the real database
    QSqlQuery query(m_database);
    QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS "programs" (
            "id" INTEGER PRIMARY KEY AUTOINCREMENT,
            "name" TEXT,
            "path" TEXT
        )
    )";
    
    QVERIFY2(query.exec(createTableQuery), qPrintable(query.lastError().text()));
}

PlaylistItem TestPlaylistRepository::createValidPlaylistItem(const QString& name, const QString& path)
{
    PlaylistItem playlist;
    playlist.name = name;
    playlist.path = path.isEmpty() ? "/test/path/playlist.xml" : path;
    playlist.description = "Test playlist description";
    return playlist;
}

void TestPlaylistRepository::insertTestData()
{
    // Insert some test data directly into database
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO programs (name, path) VALUES (?, ?)");
    
    // Test data
    QList<QPair<QString, QString>> testData = {
        {"Morning Show", "/programs/morning.xml"},
        {"Evening News", "/programs/evening.xml"},
        {"Music Hour", "/programs/music.xml"}
    };
    
    for (const auto& data : testData) {
        query.addBindValue(data.first);
        query.addBindValue(data.second);
        QVERIFY(query.exec());
    }
}

void TestPlaylistRepository::testAddPlaylist()
{
    PlaylistItem playlist = createValidPlaylistItem();
    
    QSignalSpy spy(m_repository.get(), &PlaylistRepository::playlistAdded);
    
    bool result = m_repository->addPlaylist(playlist);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the playlist was added to database
    PlaylistItem retrieved = m_repository->getPlaylistById(1);
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.name, playlist.name);
    QCOMPARE(retrieved.path, playlist.path);
}

void TestPlaylistRepository::testAddPlaylistWithInvalidData()
{
    PlaylistItem playlist; // Empty playlist item
    
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    bool result = m_repository->addPlaylist(playlist);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testAddPlaylistDuplicateName()
{
    PlaylistItem playlist1 = createValidPlaylistItem("Duplicate Name", "/path1.xml");
    PlaylistItem playlist2 = createValidPlaylistItem("Duplicate Name", "/path2.xml"); // Same name
    
    // Add first playlist
    QVERIFY(m_repository->addPlaylist(playlist1));
    
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    // Try to add second with same name
    bool result = m_repository->addPlaylist(playlist2);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testAddPlaylistDuplicatePath()
{
    PlaylistItem playlist1 = createValidPlaylistItem("Name 1", "/duplicate/path.xml");
    PlaylistItem playlist2 = createValidPlaylistItem("Name 2", "/duplicate/path.xml"); // Same path
    
    // Add first playlist
    QVERIFY(m_repository->addPlaylist(playlist1));
    
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    // Try to add second with same path
    bool result = m_repository->addPlaylist(playlist2);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testUpdatePlaylist()
{
    // First add a playlist
    PlaylistItem playlist = createValidPlaylistItem();
    QVERIFY(m_repository->addPlaylist(playlist));
    
    // Get the added playlist with ID
    PlaylistItem retrieved = m_repository->getPlaylistById(1);
    QVERIFY(retrieved.isValid());
    
    // Update the playlist
    retrieved.name = "Updated Playlist";
    retrieved.path = "/updated/path.xml";
    
    QSignalSpy spy(m_repository.get(), &PlaylistRepository::playlistUpdated);
    
    bool result = m_repository->updatePlaylist(retrieved);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the update
    PlaylistItem updated = m_repository->getPlaylistById(1);
    QCOMPARE(updated.name, QString("Updated Playlist"));
    QCOMPARE(updated.path, QString("/updated/path.xml"));
}

void TestPlaylistRepository::testUpdatePlaylistInvalidId()
{
    PlaylistItem playlist = createValidPlaylistItem();
    playlist.id = 999; // Non-existent ID
    
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    bool result = m_repository->updatePlaylist(playlist);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testDeletePlaylist()
{
    // First add a playlist
    PlaylistItem playlist = createValidPlaylistItem();
    QVERIFY(m_repository->addPlaylist(playlist));
    
    QSignalSpy spy(m_repository.get(), &PlaylistRepository::playlistDeleted);
    
    bool result = m_repository->deletePlaylist(1);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the playlist is deleted
    PlaylistItem retrieved = m_repository->getPlaylistById(1);
    QVERIFY(!retrieved.isValid());
}

void TestPlaylistRepository::testDeletePlaylistInvalidId()
{
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    bool result = m_repository->deletePlaylist(999);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testGetPlaylistById()
{
    insertTestData();
    
    PlaylistItem playlist = m_repository->getPlaylistById(1);
    
    QVERIFY(playlist.isValid());
    QCOMPARE(playlist.id, 1);
    QCOMPARE(playlist.name, QString("Morning Show"));
    QCOMPARE(playlist.path, QString("/programs/morning.xml"));
}

void TestPlaylistRepository::testGetPlaylistByIdNotFound()
{
    PlaylistItem playlist = m_repository->getPlaylistById(999);
    
    QVERIFY(!playlist.isValid());
    QCOMPARE(playlist.id, -1);
}

void TestPlaylistRepository::testGetAllPlaylists()
{
    insertTestData();
    
    QList<PlaylistItem> allPlaylists = m_repository->getAllPlaylists();
    
    QCOMPARE(allPlaylists.size(), 3);
    
    // Should be ordered by name
    QCOMPARE(allPlaylists[0].name, QString("Evening News"));
    QCOMPARE(allPlaylists[1].name, QString("Morning Show"));
    QCOMPARE(allPlaylists[2].name, QString("Music Hour"));
}

void TestPlaylistRepository::testGetAllPlaylistsWithLimitAndOffset()
{
    insertTestData();
    
    QList<PlaylistItem> limitedPlaylists = m_repository->getAllPlaylists(2, 1);
    
    QCOMPARE(limitedPlaylists.size(), 2);
    // Should skip first item and return next 2
    QCOMPARE(limitedPlaylists[0].name, QString("Morning Show"));
    QCOMPARE(limitedPlaylists[1].name, QString("Music Hour"));
}

void TestPlaylistRepository::testSearchPlaylists()
{
    insertTestData();
    
    PlaylistRepository::SearchCriteria criteria;
    criteria.name = "Morning";
    
    QList<PlaylistItem> results = m_repository->searchPlaylists(criteria);
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].name, QString("Morning Show"));
}

void TestPlaylistRepository::testSearchPlaylistsExactMatch()
{
    insertTestData();
    
    PlaylistRepository::SearchCriteria criteria;
    criteria.name = "Morning Show";
    criteria.exactMatch = true;
    
    QList<PlaylistItem> results = m_repository->searchPlaylists(criteria);
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].name, QString("Morning Show"));
    
    // Test exact match that should not match
    criteria.name = "Morning";
    results = m_repository->searchPlaylists(criteria);
    QCOMPARE(results.size(), 0);
}

void TestPlaylistRepository::testGetPlaylistsByName()
{
    insertTestData();
    
    QList<PlaylistItem> results = m_repository->getPlaylistsByName("Music");
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].name, QString("Music Hour"));
}

void TestPlaylistRepository::testAddPlaylistBatch()
{
    QList<PlaylistItem> playlistList;
    
    playlistList.append(createValidPlaylistItem("Batch Playlist 1", "/batch1.xml"));
    playlistList.append(createValidPlaylistItem("Batch Playlist 2", "/batch2.xml"));
    playlistList.append(createValidPlaylistItem("Batch Playlist 3", "/batch3.xml"));
    
    int result = m_repository->addPlaylistBatch(playlistList);
    
    QCOMPARE(result, 3);
    
    // Verify all items were added
    QList<PlaylistItem> allPlaylists = m_repository->getAllPlaylists();
    QCOMPARE(allPlaylists.size(), 3);
}

void TestPlaylistRepository::testAddPlaylistBatchWithInvalidItems()
{
    QList<PlaylistItem> playlistList;
    
    // Add valid item
    playlistList.append(createValidPlaylistItem("Valid Playlist", "/valid.xml"));
    
    // Add invalid item (empty name)
    PlaylistItem invalid;
    invalid.path = "/invalid.xml";
    playlistList.append(invalid);
    
    // Add another valid item
    playlistList.append(createValidPlaylistItem("Valid Playlist 2", "/valid2.xml"));
    
    int result = m_repository->addPlaylistBatch(playlistList);
    
    // Should add only the valid items
    QCOMPARE(result, 2);
}

void TestPlaylistRepository::testAddPlaylistBatchWithDuplicates()
{
    QList<PlaylistItem> playlistList;
    
    // Add same name twice
    playlistList.append(createValidPlaylistItem("Duplicate", "/path1.xml"));
    playlistList.append(createValidPlaylistItem("Duplicate", "/path2.xml"));
    
    int result = m_repository->addPlaylistBatch(playlistList);
    
    // Should add only the first item
    QCOMPARE(result, 1);
}

void TestPlaylistRepository::testPlaylistExists()
{
    insertTestData();
    
    bool exists = m_repository->playlistExists("Morning Show");
    QVERIFY(exists);
    
    bool notExists = m_repository->playlistExists("NonExistent");
    QVERIFY(!notExists);
}

void TestPlaylistRepository::testPathExists()
{
    insertTestData();
    
    bool exists = m_repository->pathExists("/programs/morning.xml");
    QVERIFY(exists);
    
    bool notExists = m_repository->pathExists("/nonexistent/path.xml");
    QVERIFY(!notExists);
}

void TestPlaylistRepository::testGetStatistics()
{
    insertTestData();
    
    PlaylistRepository::PlaylistStats stats = m_repository->getStatistics();
    
    QCOMPARE(stats.totalPlaylists, 3);
    // Other stats are future extensions and will be 0 or empty
    QCOMPARE(stats.totalTracks, 0);
}

void TestPlaylistRepository::testGetAllPlaylistNames()
{
    insertTestData();
    
    QStringList names = m_repository->getAllPlaylistNames();
    
    QCOMPARE(names.size(), 3);
    QVERIFY(names.contains("Morning Show"));
    QVERIFY(names.contains("Evening News"));
    QVERIFY(names.contains("Music Hour"));
}

void TestPlaylistRepository::testValidatePlaylistItem()
{
    PlaylistItem validPlaylist = createValidPlaylistItem();
    
    QString error = m_repository->validatePlaylistItem(validPlaylist);
    QVERIFY(error.isEmpty());
    
    // Test invalid playlist
    PlaylistItem invalidPlaylist;
    // Empty name
    
    error = m_repository->validatePlaylistItem(invalidPlaylist);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("Playlist name is required"));
}

void TestPlaylistRepository::testAddTrackToPlaylist()
{
    // This is a placeholder for future functionality
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    bool result = m_repository->addTrackToPlaylist(1, 1, 0);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
    
    // Check that the error message indicates feature not implemented
    QList<QVariant> arguments = errorSpy.takeFirst();
    QString errorMessage = arguments.at(1).toString();
    QVERIFY(errorMessage.contains("not yet implemented"));
}

void TestPlaylistRepository::testRemoveTrackFromPlaylist()
{
    // This is a placeholder for future functionality
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    bool result = m_repository->removeTrackFromPlaylist(1, 1);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testGetPlaylistTracks()
{
    // This is a placeholder for future functionality
    QList<PlaylistTrack> tracks = m_repository->getPlaylistTracks(1);
    
    QVERIFY(tracks.isEmpty());
}

void TestPlaylistRepository::testDatabaseConnectionError()
{
    // Close the database to simulate connection error
    m_database.close();
    
    PlaylistItem playlist = createValidPlaylistItem();
    
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    bool result = m_repository->addPlaylist(playlist);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestPlaylistRepository::testSignalEmission()
{
    QSignalSpy addedSpy(m_repository.get(), &PlaylistRepository::playlistAdded);
    QSignalSpy updatedSpy(m_repository.get(), &PlaylistRepository::playlistUpdated);
    QSignalSpy deletedSpy(m_repository.get(), &PlaylistRepository::playlistDeleted);
    QSignalSpy errorSpy(m_repository.get(), &PlaylistRepository::operationError);
    
    // Test add signal
    PlaylistItem playlist = createValidPlaylistItem();
    m_repository->addPlaylist(playlist);
    QCOMPARE(addedSpy.count(), 1);
    
    // Test update signal
    PlaylistItem retrieved = m_repository->getPlaylistById(1);
    retrieved.name = "Updated Playlist";
    m_repository->updatePlaylist(retrieved);
    QCOMPARE(updatedSpy.count(), 1);
    
    // Test delete signal
    m_repository->deletePlaylist(1);
    QCOMPARE(deletedSpy.count(), 1);
    
    // Test error signal
    PlaylistItem invalid;
    m_repository->addPlaylist(invalid);
    QCOMPARE(errorSpy.count(), 1);
}

QTEST_MAIN(TestPlaylistRepository)
#include "TestPlaylistRepository.moc"