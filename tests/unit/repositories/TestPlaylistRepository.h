#ifndef TESTPLAYLISTREPOSITORY_H
#define TESTPLAYLISTREPOSITORY_H

#include <QObject>
#include <QTest>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <memory>

class PlaylistRepository;
struct PlaylistItem;

/**
 * @brief Unit tests for PlaylistRepository class
 * 
 * This test class provides comprehensive testing for all PlaylistRepository
 * functionality including CRUD operations, batch operations, search,
 * validation, and error handling.
 */
class TestPlaylistRepository : public QObject
{
    Q_OBJECT

private slots:
    // Test framework methods
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic CRUD operations
    void testAddPlaylist();
    void testAddPlaylistWithInvalidData();
    void testAddPlaylistDuplicateName();
    void testAddPlaylistDuplicatePath();
    void testUpdatePlaylist();
    void testUpdatePlaylistInvalidId();
    void testDeletePlaylist();
    void testDeletePlaylistInvalidId();
    void testGetPlaylistById();
    void testGetPlaylistByIdNotFound();

    // Query operations
    void testGetAllPlaylists();
    void testGetAllPlaylistsWithLimitAndOffset();
    void testSearchPlaylists();
    void testSearchPlaylistsExactMatch();
    void testGetPlaylistsByName();

    // Batch operations
    void testAddPlaylistBatch();
    void testAddPlaylistBatchWithInvalidItems();
    void testAddPlaylistBatchWithDuplicates();

    // Utility methods
    void testPlaylistExists();
    void testPathExists();
    void testGetStatistics();
    void testGetAllPlaylistNames();
    void testValidatePlaylistItem();

    // Future functionality placeholders
    void testAddTrackToPlaylist();
    void testRemoveTrackFromPlaylist();
    void testGetPlaylistTracks();

    // Error handling
    void testDatabaseConnectionError();

    // Signal testing
    void testSignalEmission();

private:
    void setupTestDatabase();
    PlaylistItem createValidPlaylistItem(const QString& name = "Test Playlist", 
                                        const QString& path = "");
    void insertTestData();

    std::unique_ptr<PlaylistRepository> m_repository;
    QSqlDatabase m_database;
    QTemporaryDir m_tempDir;
    QString m_databasePath;
};

#endif // TESTPLAYLISTREPOSITORY_H