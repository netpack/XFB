#include "TestGenreRepository.h"
#include "../../../src/repositories/GenreRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSignalSpy>

void TestGenreRepository::initTestCase()
{
    // Ensure we have a valid temporary directory
    QVERIFY(m_tempDir.isValid());
    
    // Set up database path
    m_databasePath = m_tempDir.path() + "/test_genre.db";
}

void TestGenreRepository::cleanupTestCase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
    QSqlDatabase::removeDatabase("test_genre_connection");
}

void TestGenreRepository::init()
{
    // Set up fresh database for each test
    setupTestDatabase();
    
    // Create repository instance
    m_repository = std::make_unique<GenreRepository>(m_database, this);
}

void TestGenreRepository::cleanup()
{
    // Clean up repository
    m_repository.reset();
    
    // Close database connection
    if (m_database.isOpen()) {
        m_database.close();
    }
}

void TestGenreRepository::setupTestDatabase()
{
    // Remove existing connection if any
    if (QSqlDatabase::contains("test_genre_connection")) {
        QSqlDatabase::removeDatabase("test_genre_connection");
    }
    
    // Create new database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE", "test_genre_connection");
    m_database.setDatabaseName(m_databasePath);
    
    QVERIFY(m_database.open());
    
    // Create the genre tables with the same schema as the real database
    QSqlQuery query(m_database);
    
    QString createGenres1Query = R"(
        CREATE TABLE IF NOT EXISTS "genres1" (
            "id" INTEGER PRIMARY KEY AUTOINCREMENT,
            "name" TEXT
        )
    )";
    QVERIFY2(query.exec(createGenres1Query), qPrintable(query.lastError().text()));
    
    QString createGenres2Query = R"(
        CREATE TABLE IF NOT EXISTS "genres2" (
            "id" INTEGER PRIMARY KEY AUTOINCREMENT,
            "name" TEXT
        )
    )";
    QVERIFY2(query.exec(createGenres2Query), qPrintable(query.lastError().text()));
    
    // Create musics table for statistics testing
    QString createMusicsQuery = R"(
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
    QVERIFY2(query.exec(createMusicsQuery), qPrintable(query.lastError().text()));
}

GenreItem TestGenreRepository::createValidGenreItem(const QString& name, int genreType)
{
    GenreItem genre;
    genre.name = name;
    genre.genreType = genreType;
    return genre;
}

void TestGenreRepository::insertTestData()
{
    // Insert some test data directly into database
    QSqlQuery query(m_database);
    
    // Insert into genres1
    query.prepare("INSERT INTO genres1 (name) VALUES (?)");
    QStringList genres1 = {"Rock", "Pop", "Jazz"};
    for (const QString& genre : genres1) {
        query.addBindValue(genre);
        QVERIFY(query.exec());
    }
    
    // Insert into genres2
    query.prepare("INSERT INTO genres2 (name) VALUES (?)");
    QStringList genres2 = {"Alternative", "Classic", "Modern"};
    for (const QString& genre : genres2) {
        query.addBindValue(genre);
        QVERIFY(query.exec());
    }
    
    // Insert some music data for statistics
    query.prepare("INSERT INTO musics (artist, song, genre1, genre2) VALUES (?, ?, ?, ?)");
    query.addBindValue("Artist 1");
    query.addBindValue("Song 1");
    query.addBindValue("Rock");
    query.addBindValue("Alternative");
    QVERIFY(query.exec());
    
    query.addBindValue("Artist 2");
    query.addBindValue("Song 2");
    query.addBindValue("Rock");
    query.addBindValue("Classic");
    QVERIFY(query.exec());
    
    query.addBindValue("Artist 3");
    query.addBindValue("Song 3");
    query.addBindValue("Pop");
    query.addBindValue("Modern");
    QVERIFY(query.exec());
}

void TestGenreRepository::testAddGenre()
{
    GenreItem genre = createValidGenreItem("Test Rock", 1);
    
    QSignalSpy spy(m_repository.get(), &GenreRepository::genreAdded);
    
    bool result = m_repository->addGenre(genre);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the genre was added to database
    GenreItem retrieved = m_repository->getGenreById(1, 1);
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.name, genre.name);
    QCOMPARE(retrieved.genreType, genre.genreType);
}

void TestGenreRepository::testAddGenreToGenres2()
{
    GenreItem genre = createValidGenreItem("Test Alternative", 2);
    
    QSignalSpy spy(m_repository.get(), &GenreRepository::genreAdded);
    
    bool result = m_repository->addGenre(genre);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the genre was added to genres2 table
    GenreItem retrieved = m_repository->getGenreById(1, 2);
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.name, genre.name);
    QCOMPARE(retrieved.genreType, 2);
}

void TestGenreRepository::testAddGenreWithInvalidData()
{
    GenreItem genre; // Empty genre item
    
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    bool result = m_repository->addGenre(genre);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestGenreRepository::testAddGenreDuplicate()
{
    GenreItem genre1 = createValidGenreItem("Duplicate Genre", 1);
    GenreItem genre2 = createValidGenreItem("Duplicate Genre", 1); // Same name and type
    
    // Add first genre
    QVERIFY(m_repository->addGenre(genre1));
    
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    // Try to add duplicate
    bool result = m_repository->addGenre(genre2);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestGenreRepository::testUpdateGenre()
{
    // First add a genre
    GenreItem genre = createValidGenreItem("Original Name", 1);
    QVERIFY(m_repository->addGenre(genre));
    
    // Get the added genre with ID
    GenreItem retrieved = m_repository->getGenreById(1, 1);
    QVERIFY(retrieved.isValid());
    
    // Update the genre
    retrieved.name = "Updated Name";
    
    QSignalSpy spy(m_repository.get(), &GenreRepository::genreUpdated);
    
    bool result = m_repository->updateGenre(retrieved);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the update
    GenreItem updated = m_repository->getGenreById(1, 1);
    QCOMPARE(updated.name, QString("Updated Name"));
}

void TestGenreRepository::testUpdateGenreInvalidId()
{
    GenreItem genre = createValidGenreItem("Test Genre", 1);
    genre.id = 999; // Non-existent ID
    
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    bool result = m_repository->updateGenre(genre);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestGenreRepository::testDeleteGenre()
{
    // First add a genre
    GenreItem genre = createValidGenreItem("To Delete", 1);
    QVERIFY(m_repository->addGenre(genre));
    
    QSignalSpy spy(m_repository.get(), &GenreRepository::genreDeleted);
    
    bool result = m_repository->deleteGenre(1, 1);
    
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    
    // Verify the genre is deleted
    GenreItem retrieved = m_repository->getGenreById(1, 1);
    QVERIFY(!retrieved.isValid());
}

void TestGenreRepository::testDeleteGenreInvalidId()
{
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    bool result = m_repository->deleteGenre(999, 1);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestGenreRepository::testGetGenreById()
{
    insertTestData();
    
    GenreItem genre = m_repository->getGenreById(1, 1);
    
    QVERIFY(genre.isValid());
    QCOMPARE(genre.id, 1);
    QCOMPARE(genre.name, QString("Rock"));
    QCOMPARE(genre.genreType, 1);
}

void TestGenreRepository::testGetGenreByIdNotFound()
{
    GenreItem genre = m_repository->getGenreById(999, 1);
    
    QVERIFY(!genre.isValid());
    QCOMPARE(genre.id, -1);
}

void TestGenreRepository::testGetAllGenres()
{
    insertTestData();
    
    QList<GenreItem> allGenres = m_repository->getAllGenres(0); // Both tables
    
    QCOMPARE(allGenres.size(), 6); // 3 from genres1 + 3 from genres2
    
    // Check that we have genres from both tables
    bool hasGenres1 = false, hasGenres2 = false;
    for (const GenreItem& genre : allGenres) {
        if (genre.genreType == 1) hasGenres1 = true;
        if (genre.genreType == 2) hasGenres2 = true;
    }
    QVERIFY(hasGenres1);
    QVERIFY(hasGenres2);
}

void TestGenreRepository::testGetAllGenresFromGenres1()
{
    insertTestData();
    
    QList<GenreItem> genres1 = m_repository->getAllGenres(1);
    
    QCOMPARE(genres1.size(), 3);
    for (const GenreItem& genre : genres1) {
        QCOMPARE(genre.genreType, 1);
    }
}

void TestGenreRepository::testGetAllGenresFromGenres2()
{
    insertTestData();
    
    QList<GenreItem> genres2 = m_repository->getAllGenres(2);
    
    QCOMPARE(genres2.size(), 3);
    for (const GenreItem& genre : genres2) {
        QCOMPARE(genre.genreType, 2);
    }
}

void TestGenreRepository::testSearchGenres()
{
    insertTestData();
    
    GenreRepository::SearchCriteria criteria;
    criteria.name = "Rock";
    criteria.genreType = 1;
    
    QList<GenreItem> results = m_repository->searchGenres(criteria);
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].name, QString("Rock"));
    QCOMPARE(results[0].genreType, 1);
}

void TestGenreRepository::testSearchGenresExactMatch()
{
    insertTestData();
    
    GenreRepository::SearchCriteria criteria;
    criteria.name = "Rock";
    criteria.exactMatch = true;
    criteria.genreType = 0; // Both tables
    
    QList<GenreItem> results = m_repository->searchGenres(criteria);
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].name, QString("Rock"));
}

void TestGenreRepository::testGetGenresByName()
{
    insertTestData();
    
    QList<GenreItem> results = m_repository->getGenresByName("Clas", 0); // Partial match
    
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].name, QString("Classic"));
}

void TestGenreRepository::testAddGenreBatch()
{
    QList<GenreItem> genreList;
    
    genreList.append(createValidGenreItem("Batch Genre 1", 1));
    genreList.append(createValidGenreItem("Batch Genre 2", 1));
    genreList.append(createValidGenreItem("Batch Genre 3", 2));
    
    int result = m_repository->addGenreBatch(genreList);
    
    QCOMPARE(result, 3);
    
    // Verify all items were added
    QList<GenreItem> allGenres = m_repository->getAllGenres(0);
    QCOMPARE(allGenres.size(), 3);
}

void TestGenreRepository::testAddGenreBatchWithInvalidItems()
{
    QList<GenreItem> genreList;
    
    // Add valid item
    genreList.append(createValidGenreItem("Valid Genre", 1));
    
    // Add invalid item (empty name)
    GenreItem invalid;
    invalid.genreType = 1;
    genreList.append(invalid);
    
    // Add another valid item
    genreList.append(createValidGenreItem("Valid Genre 2", 2));
    
    int result = m_repository->addGenreBatch(genreList);
    
    // Should add only the valid items
    QCOMPARE(result, 2);
}

void TestGenreRepository::testAddGenreBatchWithDuplicates()
{
    QList<GenreItem> genreList;
    
    // Add same name twice to same table
    genreList.append(createValidGenreItem("Duplicate", 1));
    genreList.append(createValidGenreItem("Duplicate", 1));
    
    int result = m_repository->addGenreBatch(genreList);
    
    // Should add only the first item
    QCOMPARE(result, 1);
}

void TestGenreRepository::testGenreExists()
{
    insertTestData();
    
    bool exists = m_repository->genreExists("Rock", 1);
    QVERIFY(exists);
    
    bool notExists = m_repository->genreExists("NonExistent", 1);
    QVERIFY(!notExists);
}

void TestGenreRepository::testGetStatistics()
{
    insertTestData();
    
    GenreRepository::GenreStats stats = m_repository->getStatistics();
    
    QCOMPARE(stats.totalGenres1, 3);
    QCOMPARE(stats.totalGenres2, 3);
    QCOMPARE(stats.totalGenres, 6);
    QCOMPARE(stats.mostUsedGenre1, QString("Rock")); // Used twice in test data
}

void TestGenreRepository::testGetAllGenreNames()
{
    insertTestData();
    
    QStringList names = m_repository->getAllGenreNames(0); // Both tables
    
    QCOMPARE(names.size(), 6);
    QVERIFY(names.contains("Rock"));
    QVERIFY(names.contains("Pop"));
    QVERIFY(names.contains("Jazz"));
    QVERIFY(names.contains("Alternative"));
    QVERIFY(names.contains("Classic"));
    QVERIFY(names.contains("Modern"));
}

void TestGenreRepository::testValidateGenreItem()
{
    GenreItem validGenre = createValidGenreItem("Valid Genre", 1);
    
    QString error = m_repository->validateGenreItem(validGenre);
    QVERIFY(error.isEmpty());
    
    // Test invalid genre
    GenreItem invalidGenre;
    invalidGenre.genreType = 1; // Valid type but empty name
    
    error = m_repository->validateGenreItem(invalidGenre);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("Genre name is required"));
}

void TestGenreRepository::testInvalidGenreType()
{
    GenreItem genre = createValidGenreItem("Test Genre", 3); // Invalid type
    
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    bool result = m_repository->addGenre(genre);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestGenreRepository::testDatabaseConnectionError()
{
    // Close the database to simulate connection error
    m_database.close();
    
    GenreItem genre = createValidGenreItem("Test Genre", 1);
    
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    bool result = m_repository->addGenre(genre);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestGenreRepository::testSignalEmission()
{
    QSignalSpy addedSpy(m_repository.get(), &GenreRepository::genreAdded);
    QSignalSpy updatedSpy(m_repository.get(), &GenreRepository::genreUpdated);
    QSignalSpy deletedSpy(m_repository.get(), &GenreRepository::genreDeleted);
    QSignalSpy errorSpy(m_repository.get(), &GenreRepository::operationError);
    
    // Test add signal
    GenreItem genre = createValidGenreItem("Test Genre", 1);
    m_repository->addGenre(genre);
    QCOMPARE(addedSpy.count(), 1);
    
    // Test update signal
    GenreItem retrieved = m_repository->getGenreById(1, 1);
    retrieved.name = "Updated Genre";
    m_repository->updateGenre(retrieved);
    QCOMPARE(updatedSpy.count(), 1);
    
    // Test delete signal
    m_repository->deleteGenre(1, 1);
    QCOMPARE(deletedSpy.count(), 1);
    
    // Test error signal
    GenreItem invalid;
    m_repository->addGenre(invalid);
    QCOMPARE(errorSpy.count(), 1);
}

QTEST_MAIN(TestGenreRepository)
#include "TestGenreRepository.moc"