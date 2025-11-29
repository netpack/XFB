#ifndef TESTMUSICREPOSITORY_H
#define TESTMUSICREPOSITORY_H

#include <QObject>
#include <QTest>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <memory>

class MusicRepository;
struct MusicItem;

/**
 * @brief Unit tests for MusicRepository class
 * 
 * This test class provides comprehensive testing for all MusicRepository
 * functionality including CRUD operations, batch operations, search,
 * validation, and error handling.
 */
class TestMusicRepository : public QObject
{
    Q_OBJECT

private slots:
    // Test framework methods
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic CRUD operations
    void testAddMusic();
    void testAddMusicWithInvalidData();
    void testAddMusicDuplicatePath();
    void testUpdateMusic();
    void testUpdateMusicInvalidId();
    void testDeleteMusic();
    void testDeleteMusicInvalidId();
    void testGetMusicById();
    void testGetMusicByIdNotFound();

    // Query operations
    void testGetAllMusic();
    void testGetAllMusicWithLimitAndOffset();
    void testSearchMusic();
    void testSearchMusicWithCriteria();
    void testGetMusicByGenre();
    void testGetMusicByGenreWithGenre2();
    void testGetMusicByArtist();

    // Batch operations
    void testAddMusicBatch();
    void testAddMusicBatchWithInvalidItems();
    void testAddMusicBatchWithDuplicates();
    void testImportFromDirectory();
    void testImportFromDirectoryRecursive();
    void testImportFromDirectoryNonExistent();

    // Statistics and metadata
    void testIncrementPlayCount();
    void testGetStatistics();
    void testGetAllArtists();
    void testGetAllGenres();
    void testGetAllGenresWithGenre2();

    // Validation and utility methods
    void testPathExists();
    void testValidateMusicItem();
    void testValidateMusicItemInvalidData();

    // Error handling
    void testDatabaseConnectionError();
    void testTransactionRollback();

    // Signal testing
    void testSignalEmission();

private:
    void setupTestDatabase();
    void createTestMusicFiles();
    MusicItem createValidMusicItem(const QString& artist = "Test Artist", 
                                  const QString& song = "Test Song",
                                  const QString& path = "");
    void insertTestData();

    std::unique_ptr<MusicRepository> m_repository;
    QSqlDatabase m_database;
    QTemporaryDir m_tempDir;
    QStringList m_testMusicFiles;
    QString m_databasePath;
};

#endif // TESTMUSICREPOSITORY_H