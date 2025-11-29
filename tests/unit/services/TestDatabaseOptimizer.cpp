#include "TestDatabaseOptimizer.h"
#include "../../../src/services/DatabaseOptimizer.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSignalSpy>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>

void TestDatabaseOptimizer::initTestCase()
{
    // Register metatypes for signal testing
    qRegisterMetaType<DatabaseOptimizer::PerformanceMetrics>("PerformanceMetrics");
}

void TestDatabaseOptimizer::cleanupTestCase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
    
    if (!m_connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

void TestDatabaseOptimizer::init()
{
    // Create temporary directory
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    
    // Create test database
    createTestDatabase();
    
    // Create optimizer
    m_optimizer = std::make_unique<DatabaseOptimizer>(m_database, this);
}

void TestDatabaseOptimizer::cleanup()
{
    m_optimizer.reset();
    
    if (m_database.isOpen()) {
        m_database.close();
    }
    
    if (!m_connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
    }
    
    m_tempDir.reset();
}

void TestDatabaseOptimizer::testInitialization()
{
    QVERIFY(m_optimizer->initialize());
    
    // Test that metrics are available after initialization
    auto metrics = m_optimizer->getPerformanceMetrics();
    QVERIFY(metrics.pageSize > 0);
    QVERIFY(metrics.pageCount >= 0);
}

void TestDatabaseOptimizer::testInitializationWithClosedDatabase()
{
    m_database.close();
    QVERIFY(!m_optimizer->initialize());
}

void TestDatabaseOptimizer::testCreateOptimalIndexes()
{
    QVERIFY(m_optimizer->initialize());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::optimizationCompleted);
    
    bool result = m_optimizer->createOptimalIndexes();
    QVERIFY(result);
    
    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("createOptimalIndexes"));
    QCOMPARE(arguments.at(1).toBool(), true);
    
    // Verify indexes were created
    auto indexes = m_optimizer->getIndexInformation();
    QVERIFY(indexes.size() > 0);
    
    // Check for specific indexes
    bool foundArtistIndex = false;
    bool foundPathIndex = false;
    
    for (const auto& index : indexes) {
        if (index.name == "idx_music_artist") {
            foundArtistIndex = true;
            QCOMPARE(index.tableName, QString("music"));
            QVERIFY(index.columns.contains("artist"));
        }
        if (index.name == "idx_music_path") {
            foundPathIndex = true;
            QCOMPARE(index.tableName, QString("music"));
            QVERIFY(index.columns.contains("path"));
            QVERIFY(index.isUnique);
        }
    }
    
    QVERIFY(foundArtistIndex);
    QVERIFY(foundPathIndex);
}

void TestDatabaseOptimizer::testCreateOptimalIndexesWithExistingIndexes()
{
    QVERIFY(m_optimizer->initialize());
    
    // Create indexes first time
    QVERIFY(m_optimizer->createOptimalIndexes());
    int firstCount = m_optimizer->getIndexInformation().size();
    
    // Create indexes second time - should not create duplicates
    QVERIFY(m_optimizer->createOptimalIndexes());
    int secondCount = m_optimizer->getIndexInformation().size();
    
    QCOMPARE(firstCount, secondCount);
}

void TestDatabaseOptimizer::testDropUnusedIndexes()
{
    QVERIFY(m_optimizer->initialize());
    
    // Create some indexes first
    QVERIFY(m_optimizer->createOptimalIndexes());
    
    // Create a test unused index
    QSqlQuery query(m_database);
    QVERIFY(query.exec("CREATE INDEX test_unused_index ON music (country)"));
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::optimizationCompleted);
    
    int droppedCount = m_optimizer->dropUnusedIndexes();
    
    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("dropUnusedIndexes"));
    QCOMPARE(arguments.at(1).toBool(), true);
    
    // Note: The actual number depends on implementation details
    // We just verify the function completes successfully
    QVERIFY(droppedCount >= 0);
}

void TestDatabaseOptimizer::testIndexExists()
{
    QVERIFY(m_optimizer->initialize());
    
    // Test non-existent index
    QSqlQuery query(m_database);
    QVERIFY(!query.exec("SELECT name FROM sqlite_master WHERE type='index' AND name='nonexistent_index'") || !query.next());
    
    // Create an index
    QVERIFY(query.exec("CREATE INDEX test_index ON music (artist)"));
    
    // Test existing index
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='index' AND name='test_index'"));
    QVERIFY(query.next());
}

void TestDatabaseOptimizer::testCreateSingleIndex()
{
    QVERIFY(m_optimizer->initialize());
    
    // This tests the private createIndex method indirectly through createOptimalIndexes
    QVERIFY(m_optimizer->createOptimalIndexes());
    
    // Verify specific index was created
    QSqlQuery query(m_database);
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='index' AND name='idx_music_artist'"));
    QVERIFY(query.next());
}

void TestDatabaseOptimizer::testDropSingleIndex()
{
    QVERIFY(m_optimizer->initialize());
    
    // Create an index first
    QSqlQuery query(m_database);
    QVERIFY(query.exec("CREATE INDEX test_drop_index ON music (song)"));
    
    // Verify it exists
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='index' AND name='test_drop_index'"));
    QVERIFY(query.next());
    
    // Drop it
    QVERIFY(query.exec("DROP INDEX test_drop_index"));
    
    // Verify it's gone
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='index' AND name='test_drop_index'"));
    QVERIFY(!query.next());
}

void TestDatabaseOptimizer::testAnalyzeDatabase()
{
    QVERIFY(m_optimizer->initialize());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::optimizationCompleted);
    
    bool result = m_optimizer->analyzeDatabase();
    QVERIFY(result);
    
    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("analyzeDatabase"));
    QCOMPARE(arguments.at(1).toBool(), true);
    
    // Verify metrics were updated
    auto metrics = m_optimizer->getPerformanceMetrics();
    QVERIFY(metrics.lastAnalyze.isValid());
}

void TestDatabaseOptimizer::testVacuumDatabase()
{
    QVERIFY(m_optimizer->initialize());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::optimizationCompleted);
    
    bool result = m_optimizer->vacuumDatabase();
    QVERIFY(result);
    
    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("vacuumDatabase"));
    QCOMPARE(arguments.at(1).toBool(), true);
    
    // Verify metrics were updated
    auto metrics = m_optimizer->getPerformanceMetrics();
    QVERIFY(metrics.lastVacuum.isValid());
}

void TestDatabaseOptimizer::testReindexDatabase()
{
    QVERIFY(m_optimizer->initialize());
    
    // Create some indexes first
    QVERIFY(m_optimizer->createOptimalIndexes());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::optimizationCompleted);
    
    bool result = m_optimizer->reindexDatabase();
    QVERIFY(result);
    
    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("reindexDatabase"));
    QCOMPARE(arguments.at(1).toBool(), true);
    
    // Verify metrics were updated
    auto metrics = m_optimizer->getPerformanceMetrics();
    QVERIFY(metrics.lastReindex.isValid());
}

void TestDatabaseOptimizer::testStartStopQueryMonitoring()
{
    QVERIFY(m_optimizer->initialize());
    
    // Test starting monitoring
    m_optimizer->startQueryMonitoring(50);
    
    // Test stopping monitoring
    m_optimizer->stopQueryMonitoring();
    
    // No exceptions should be thrown
    QVERIFY(true);
}

void TestDatabaseOptimizer::testRecordQueryExecution()
{
    QVERIFY(m_optimizer->initialize());
    
    // Start monitoring
    m_optimizer->startQueryMonitoring(100);
    
    // Record some query executions
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE artist = 'Test'", 50);
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE artist = 'Test'", 75);
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE song = 'Song'", 150);
    
    // Get statistics
    auto stats = m_optimizer->getQueryStatistics();
    QVERIFY(stats.size() >= 2);
    
    // Find the first query stats
    bool foundFirstQuery = false;
    for (const auto& stat : stats) {
        if (stat.queryPattern.contains("ARTIST")) {
            foundFirstQuery = true;
            QCOMPARE(stat.executionCount, 2);
            QCOMPARE(stat.totalExecutionTime, qint64(125));
            QCOMPARE(stat.averageExecutionTime, qint64(62));
            QCOMPARE(stat.minExecutionTime, qint64(50));
            QCOMPARE(stat.maxExecutionTime, qint64(75));
            QVERIFY(!stat.isSlowQuery);
            break;
        }
    }
    QVERIFY(foundFirstQuery);
}

void TestDatabaseOptimizer::testSlowQueryDetection()
{
    QVERIFY(m_optimizer->initialize());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::slowQueryDetected);
    
    // Start monitoring with low threshold
    m_optimizer->startQueryMonitoring(50);
    
    // Record a slow query
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE artist = 'Slow'", 100);
    
    // Verify signal was emitted
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(1).toLongLong(), qint64(100));
    
    // Get slow queries
    auto slowQueries = m_optimizer->getSlowQueries();
    QCOMPARE(slowQueries.size(), 1);
    QVERIFY(slowQueries.first().isSlowQuery);
    QCOMPARE(slowQueries.first().averageExecutionTime, qint64(100));
}

void TestDatabaseOptimizer::testQueryStatistics()
{
    QVERIFY(m_optimizer->initialize());
    
    // Start monitoring
    m_optimizer->startQueryMonitoring();
    
    // Record various queries
    for (int i = 0; i < 10; i++) {
        m_optimizer->recordQueryExecution(QString("SELECT * FROM music WHERE id = %1").arg(i), 10 + i);
    }
    
    // Get statistics with limit
    auto stats = m_optimizer->getQueryStatistics(5);
    QVERIFY(stats.size() <= 5);
    
    // Get all statistics
    auto allStats = m_optimizer->getQueryStatistics();
    QCOMPARE(allStats.size(), 10);
    
    // Verify sorting (by total execution time, descending)
    if (allStats.size() > 1) {
        QVERIFY(allStats.first().totalExecutionTime >= allStats.last().totalExecutionTime);
    }
}

void TestDatabaseOptimizer::testClearQueryStatistics()
{
    QVERIFY(m_optimizer->initialize());
    
    // Start monitoring and record some queries
    m_optimizer->startQueryMonitoring();
    m_optimizer->recordQueryExecution("SELECT * FROM music", 50);
    
    // Verify statistics exist
    auto stats = m_optimizer->getQueryStatistics();
    QVERIFY(stats.size() > 0);
    
    // Clear statistics
    m_optimizer->clearQueryStatistics();
    
    // Verify statistics are cleared
    stats = m_optimizer->getQueryStatistics();
    QCOMPARE(stats.size(), 0);
}

void TestDatabaseOptimizer::testGetPerformanceMetrics()
{
    QVERIFY(m_optimizer->initialize());
    
    auto metrics = m_optimizer->getPerformanceMetrics();
    
    // Verify basic metrics are populated
    QVERIFY(metrics.pageSize > 0);
    QVERIFY(metrics.pageCount >= 0);
    QVERIFY(metrics.databaseSize >= 0);
    QVERIFY(metrics.indexCount >= 0);
    QVERIFY(metrics.cacheHitRatio >= 0.0 && metrics.cacheHitRatio <= 1.0);
}

void TestDatabaseOptimizer::testUpdateMetrics()
{
    QVERIFY(m_optimizer->initialize());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::metricsUpdated);
    
    // Get initial metrics
    auto initialMetrics = m_optimizer->getPerformanceMetrics();
    
    // Add some data to change metrics
    QSqlQuery query(m_database);
    QVERIFY(query.exec("INSERT INTO music (artist, song, path) VALUES ('Test', 'Song', '/test/path')"));
    
    // Force metrics update by waiting for cache to expire and requesting again
    QThread::msleep(100);
    auto updatedMetrics = m_optimizer->getPerformanceMetrics();
    
    // Metrics should be valid
    QVERIFY(updatedMetrics.pageSize > 0);
    QVERIFY(updatedMetrics.databaseSize >= initialMetrics.databaseSize);
}

void TestDatabaseOptimizer::testMetricsCache()
{
    QVERIFY(m_optimizer->initialize());
    
    // Get metrics twice quickly - should use cache
    auto metrics1 = m_optimizer->getPerformanceMetrics();
    auto metrics2 = m_optimizer->getPerformanceMetrics();
    
    // Should be identical (from cache)
    QCOMPARE(metrics1.pageSize, metrics2.pageSize);
    QCOMPARE(metrics1.pageCount, metrics2.pageCount);
    QCOMPARE(metrics1.databaseSize, metrics2.databaseSize);
}

void TestDatabaseOptimizer::testGetOptimizationRecommendations()
{
    QVERIFY(m_optimizer->initialize());
    
    QSignalSpy spy(m_optimizer.get(), &DatabaseOptimizer::recommendationsAvailable);
    
    auto recommendations = m_optimizer->getOptimizationRecommendations();
    
    // Should have some recommendations for a new database
    QVERIFY(recommendations.size() > 0);
    
    // Verify recommendations are sorted by priority
    if (recommendations.size() > 1) {
        QVERIFY(recommendations.first().priority >= recommendations.last().priority);
    }
    
    // Check for common recommendations
    bool foundAnalyze = false;
    for (const auto& rec : recommendations) {
        if (rec.type == DatabaseOptimizer::OptimizationRecommendation::UpdateStatistics) {
            foundAnalyze = true;
            QVERIFY(!rec.description.isEmpty());
            QVERIFY(!rec.sqlCommand.isEmpty());
            QVERIFY(rec.priority > 0);
            break;
        }
    }
    QVERIFY(foundAnalyze);
    
    // Verify signal was emitted if recommendations were found
    if (!recommendations.isEmpty()) {
        QCOMPARE(spy.count(), 1);
    }
}

void TestDatabaseOptimizer::testApplyRecommendation()
{
    QVERIFY(m_optimizer->initialize());
    
    // Create a recommendation
    DatabaseOptimizer::OptimizationRecommendation rec;
    rec.type = DatabaseOptimizer::OptimizationRecommendation::UpdateStatistics;
    rec.description = "Test recommendation";
    rec.sqlCommand = "ANALYZE";
    rec.priority = 5;
    
    bool result = m_optimizer->applyRecommendation(rec);
    QVERIFY(result);
    
    // Verify the operation was performed
    auto metrics = m_optimizer->getPerformanceMetrics();
    QVERIFY(metrics.lastAnalyze.isValid());
}

void TestDatabaseOptimizer::testRecommendationPriority()
{
    QVERIFY(m_optimizer->initialize());
    
    auto recommendations = m_optimizer->getOptimizationRecommendations();
    
    // Verify recommendations are sorted by priority (descending)
    for (int i = 1; i < recommendations.size(); i++) {
        QVERIFY(recommendations[i-1].priority >= recommendations[i].priority);
    }
}

void TestDatabaseOptimizer::testAutoOptimizationEnabled()
{
    QVERIFY(m_optimizer->initialize());
    
    // Test initial state
    QVERIFY(!m_optimizer->isAutoOptimizationEnabled());
    
    // Enable auto optimization
    m_optimizer->setAutoOptimizationEnabled(true);
    QVERIFY(m_optimizer->isAutoOptimizationEnabled());
    
    // Disable auto optimization
    m_optimizer->setAutoOptimizationEnabled(false);
    QVERIFY(!m_optimizer->isAutoOptimizationEnabled());
}

void TestDatabaseOptimizer::testAutoOptimizationInterval()
{
    QVERIFY(m_optimizer->initialize());
    
    // Test default interval
    int defaultInterval = m_optimizer->optimizationInterval();
    QVERIFY(defaultInterval > 0);
    
    // Set custom interval
    m_optimizer->setOptimizationInterval(5000);
    QCOMPARE(m_optimizer->optimizationInterval(), 5000);
}

void TestDatabaseOptimizer::testPerformAutoOptimization()
{
    QVERIFY(m_optimizer->initialize());
    
    // Enable auto optimization
    m_optimizer->setAutoOptimizationEnabled(true);
    
    // This tests the private performAutoOptimization method indirectly
    // by enabling auto optimization and verifying it doesn't crash
    QThread::msleep(100);
    
    QVERIFY(true); // If we get here, no crash occurred
}

void TestDatabaseOptimizer::testNormalizeQuery()
{
    QVERIFY(m_optimizer->initialize());
    
    // This tests the private normalizeQuery method indirectly
    // by recording queries and checking the patterns
    m_optimizer->startQueryMonitoring();
    
    // Record similar queries that should normalize to the same pattern
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE id = 1", 10);
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE id = 2", 15);
    m_optimizer->recordQueryExecution("SELECT * FROM music WHERE id = 999", 20);
    
    auto stats = m_optimizer->getQueryStatistics();
    
    // Should have only one pattern for all three queries
    QCOMPARE(stats.size(), 1);
    QCOMPARE(stats.first().executionCount, 3);
}

void TestDatabaseOptimizer::testGetTableNames()
{
    QVERIFY(m_optimizer->initialize());
    
    // This tests the private getTableNames method indirectly
    // by creating indexes which uses this method
    QVERIFY(m_optimizer->createOptimalIndexes());
    
    // If indexes were created successfully, getTableNames worked
    auto indexes = m_optimizer->getIndexInformation();
    QVERIFY(indexes.size() > 0);
}

void TestDatabaseOptimizer::testGetColumnNames()
{
    QVERIFY(m_optimizer->initialize());
    
    // This tests the private getColumnNames method indirectly
    // by creating indexes which uses this method
    QVERIFY(m_optimizer->createOptimalIndexes());
    
    // Verify indexes have correct column information
    auto indexes = m_optimizer->getIndexInformation();
    bool foundIndexWithColumns = false;
    
    for (const auto& index : indexes) {
        if (!index.columns.isEmpty()) {
            foundIndexWithColumns = true;
            break;
        }
    }
    
    QVERIFY(foundIndexWithColumns);
}

void TestDatabaseOptimizer::testExportPerformanceData()
{
    QVERIFY(m_optimizer->initialize());
    
    // Record some data
    m_optimizer->startQueryMonitoring();
    m_optimizer->recordQueryExecution("SELECT * FROM music", 50);
    m_optimizer->createOptimalIndexes();
    
    QString jsonData = m_optimizer->exportPerformanceData();
    QVERIFY(!jsonData.isEmpty());
    
    // Verify it's valid JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    
    QJsonObject root = doc.object();
    QVERIFY(root.contains("metrics"));
    QVERIFY(root.contains("queryStatistics"));
    QVERIFY(root.contains("indexes"));
    
    // Verify metrics section
    QJsonObject metrics = root["metrics"].toObject();
    QVERIFY(metrics.contains("totalQueries"));
    QVERIFY(metrics.contains("databaseSize"));
    QVERIFY(metrics.contains("pageSize"));
}

void TestDatabaseOptimizer::testErrorHandling()
{
    QVERIFY(m_optimizer->initialize());
    
    // Test applying invalid recommendation
    DatabaseOptimizer::OptimizationRecommendation invalidRec;
    invalidRec.type = DatabaseOptimizer::OptimizationRecommendation::CreateIndex;
    invalidRec.sqlCommand = "INVALID SQL COMMAND";
    
    bool result = m_optimizer->applyRecommendation(invalidRec);
    QVERIFY(!result);
    
    // Test should not crash on invalid operations
    QVERIFY(true);
}

void TestDatabaseOptimizer::testInvalidDatabase()
{
    // Close the database
    m_database.close();
    
    // Try to initialize with closed database
    QVERIFY(!m_optimizer->initialize());
    
    // Operations should fail gracefully
    QVERIFY(!m_optimizer->createOptimalIndexes());
    QVERIFY(!m_optimizer->analyzeDatabase());
    QVERIFY(!m_optimizer->vacuumDatabase());
}

void TestDatabaseOptimizer::testLargeDatasetPerformance()
{
    QVERIFY(m_optimizer->initialize());
    
    // Insert a larger dataset
    QSqlQuery query(m_database);
    QVERIFY(query.exec("BEGIN TRANSACTION"));
    
    for (int i = 0; i < 1000; i++) {
        query.prepare("INSERT INTO music (artist, song, path) VALUES (?, ?, ?)");
        query.addBindValue(QString("Artist%1").arg(i % 100));
        query.addBindValue(QString("Song%1").arg(i));
        query.addBindValue(QString("/path/to/song%1.mp3").arg(i));
        QVERIFY(query.exec());
    }
    
    QVERIFY(query.exec("COMMIT"));
    
    // Test optimization with larger dataset
    QElapsedTimer timer;
    timer.start();
    
    QVERIFY(m_optimizer->createOptimalIndexes());
    
    qint64 elapsed = timer.elapsed();
    
    // Should complete within reasonable time (10 seconds)
    QVERIFY(elapsed < 10000);
    
    // Verify indexes were created
    auto indexes = m_optimizer->getIndexInformation();
    QVERIFY(indexes.size() > 0);
}

void TestDatabaseOptimizer::testConcurrentAccess()
{
    QVERIFY(m_optimizer->initialize());
    
    // Start query monitoring
    m_optimizer->startQueryMonitoring();
    
    // Simulate concurrent query recording
    QList<QThread*> threads;
    
    for (int i = 0; i < 5; i++) {
        QThread* thread = QThread::create([this, i]() {
            for (int j = 0; j < 10; j++) {
                m_optimizer->recordQueryExecution(
                    QString("SELECT * FROM music WHERE artist = 'Thread%1Query%2'").arg(i).arg(j), 
                    10 + j
                );
                QThread::msleep(1);
            }
        });
        
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads to complete
    for (auto* thread : threads) {
        QVERIFY(thread->wait(5000));
        delete thread;
    }
    
    // Verify statistics were recorded correctly
    auto stats = m_optimizer->getQueryStatistics();
    QVERIFY(stats.size() > 0);
    
    int totalExecutions = 0;
    for (const auto& stat : stats) {
        totalExecutions += stat.executionCount;
    }
    
    QCOMPARE(totalExecutions, 50); // 5 threads * 10 queries each
}

void TestDatabaseOptimizer::createTestDatabase()
{
    // Create unique connection name
    m_connectionName = QString("test_db_optimizer_%1").arg(QDateTime::currentMSecsSinceEpoch());
    
    // Create database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_database.setDatabaseName(m_tempDir->filePath("test.db"));
    
    QVERIFY(m_database.open());
    
    // Create test schema
    QSqlQuery query(m_database);
    
    // Create music table
    QVERIFY(query.exec(R"(
        CREATE TABLE music (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            artist TEXT NOT NULL,
            song TEXT NOT NULL,
            genre1 TEXT,
            genre2 TEXT,
            country TEXT,
            published_date TEXT,
            path TEXT UNIQUE NOT NULL,
            time TEXT,
            played_times INTEGER DEFAULT 0,
            last_played TEXT
        )
    )"));
    
    // Create playlist table
    QVERIFY(query.exec(R"(
        CREATE TABLE playlist (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            created_date TEXT,
            description TEXT
        )
    )"));
    
    // Create genre table
    QVERIFY(query.exec(R"(
        CREATE TABLE genre (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL
        )
    )"));
    
    populateTestData();
}

void TestDatabaseOptimizer::populateTestData()
{
    QSqlQuery query(m_database);
    
    // Insert test music data
    QStringList artists = {"Artist1", "Artist2", "Artist3", "Artist4", "Artist5"};
    QStringList genres = {"Rock", "Pop", "Jazz", "Classical", "Electronic"};
    QStringList countries = {"USA", "UK", "Canada", "Germany", "France"};
    
    for (int i = 0; i < 50; i++) {
        query.prepare(R"(
            INSERT INTO music (artist, song, genre1, genre2, country, path, played_times) 
            VALUES (?, ?, ?, ?, ?, ?, ?)
        )");
        query.addBindValue(artists[i % artists.size()]);
        query.addBindValue(QString("Song%1").arg(i + 1));
        query.addBindValue(genres[i % genres.size()]);
        query.addBindValue(genres[(i + 1) % genres.size()]);
        query.addBindValue(countries[i % countries.size()]);
        query.addBindValue(QString("/path/to/song%1.mp3").arg(i + 1));
        query.addBindValue(i % 10); // Played times
        QVERIFY(query.exec());
    }
    
    // Insert test playlist data
    for (int i = 0; i < 10; i++) {
        query.prepare("INSERT INTO playlist (name, created_date) VALUES (?, ?)");
        query.addBindValue(QString("Playlist%1").arg(i + 1));
        query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
        QVERIFY(query.exec());
    }
    
    // Insert test genre data
    for (const QString& genre : genres) {
        query.prepare("INSERT INTO genre (name) VALUES (?)");
        query.addBindValue(genre);
        QVERIFY(query.exec());
    }
}

void TestDatabaseOptimizer::createTestIndexes()
{
    QSqlQuery query(m_database);
    
    // Create some test indexes
    QVERIFY(query.exec("CREATE INDEX test_artist_idx ON music (artist)"));
    QVERIFY(query.exec("CREATE INDEX test_genre_idx ON music (genre1)"));
    QVERIFY(query.exec("CREATE UNIQUE INDEX test_path_idx ON music (path)"));
}

QTEST_MAIN(TestDatabaseOptimizer)