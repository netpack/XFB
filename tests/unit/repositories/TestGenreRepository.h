#ifndef TESTGENREREPOSITORY_H
#define TESTGENREREPOSITORY_H

#include <QObject>
#include <QTest>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <memory>

class GenreRepository;
struct GenreItem;

/**
 * @brief Unit tests for GenreRepository class
 * 
 * This test class provides comprehensive testing for all GenreRepository
 * functionality including CRUD operations, batch operations, search,
 * validation, and error handling for both genres1 and genres2 tables.
 */
class TestGenreRepository : public QObject
{
    Q_OBJECT

private slots:
    // Test framework methods
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic CRUD operations
    void testAddGenre();
    void testAddGenreToGenres2();
    void testAddGenreWithInvalidData();
    void testAddGenreDuplicate();
    void testUpdateGenre();
    void testUpdateGenreInvalidId();
    void testDeleteGenre();
    void testDeleteGenreInvalidId();
    void testGetGenreById();
    void testGetGenreByIdNotFound();

    // Query operations
    void testGetAllGenres();
    void testGetAllGenresFromGenres1();
    void testGetAllGenresFromGenres2();
    void testSearchGenres();
    void testSearchGenresExactMatch();
    void testGetGenresByName();

    // Batch operations
    void testAddGenreBatch();
    void testAddGenreBatchWithInvalidItems();
    void testAddGenreBatchWithDuplicates();

    // Utility methods
    void testGenreExists();
    void testGetStatistics();
    void testGetAllGenreNames();
    void testValidateGenreItem();

    // Error handling
    void testInvalidGenreType();
    void testDatabaseConnectionError();

    // Signal testing
    void testSignalEmission();

private:
    void setupTestDatabase();
    GenreItem createValidGenreItem(const QString& name = "Test Genre", int genreType = 1);
    void insertTestData();

    std::unique_ptr<GenreRepository> m_repository;
    QSqlDatabase m_database;
    QTemporaryDir m_tempDir;
    QString m_databasePath;
};

#endif // TESTGENREREPOSITORY_H