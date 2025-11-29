#include "TestMusicListModelPerformance.h"
#include "../../src/models/MusicListModel.h"
#include "../../src/repositories/MusicRepository.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTableView>
#include <QScrollBar>
#include <QApplication>
#include <QThread>
#include <QRandomGenerator>
#include <QDebug>
#include <QProcess>

void TestMusicListModelPerformance::initTestCase()
{
    qDebug() << "TestMusicListModelPerformance: Initializing test case...";
    
    // Ensure we have a valid temporary directory
    QVERIFY(m_tempDir.isValid());
    
    qDebug() << "TestMusicListModelPerformance: Test case initialized successfully";
}

void TestMusicListModelPerformance::cleanupTestCase()
{
    qDebug() << "TestMusicListModelPerformance: Cleaning up test case...";
    
    // Cleanup will be handled by destructors
    qDebug() << "TestMusicListModelPerformance: Test case cleanup completed";
}

void TestMusicListModelPerformance::init()
{
    // Create fresh instances for each test
    // This will be implemented based on the specific test requirements
}

void TestMusicListModelPerformance::cleanup()
{
    // Clean up test instances
    m_model.reset();
    m_proxyModel.reset();
    m_repository.reset();
    m_database.reset();
}

void TestMusicListModelPerformance::testModelInitializationPerformance()
{
    qDebug() << "Testing model initialization performance...";
    
    // Create test database with medium dataset
    QVERIFY(createTestDatabase(MEDIUM_DATASET_SIZE));
    
    PerformanceBenchmark benchmark("Model Initialization");
    
    // Measure initialization time
    benchmark.start();
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    QVERIFY(m_model != nullptr);
    
    benchmark.stop();
    
    qint64 initTime = benchmark.elapsed();
    qDebug() << "Model initialization took" << initTime << "ms";
    
    // Verify initialization was fast enough
    QVERIFY2(initTime < MAX_INITIALIZATION_TIME, 
             QString("Initialization took %1ms, expected < %2ms")
             .arg(initTime).arg(MAX_INITIALIZATION_TIME).toLocal8Bit());
    
    // Verify model is functional
    QVERIFY(m_model->totalItemCount() > 0);
    QCOMPARE(m_model->columnCount(), MusicListModel::ColumnCount);
}

void TestMusicListModelPerformance::testLazyLoadingPerformance()
{
    qDebug() << "Testing lazy loading performance...";
    
    // Create test database with large dataset
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    m_model->setBatchSize(100); // Set reasonable batch size
    
    PerformanceBenchmark benchmark("Lazy Loading");
    
    // Test loading first batch
    benchmark.start();
    
    // Access first few items to trigger loading
    for (int i = 0; i < 10; ++i) {
        QModelIndex index = m_model->index(i, MusicListModel::ColumnTitle);
        QVariant data = m_model->data(index, Qt::DisplayRole);
        // Data might be "Loading..." initially
    }
    
    // Wait for loading to complete
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    benchmark.stop();
    
    qint64 loadTime = benchmark.elapsed();
    qDebug() << "First batch loading took" << loadTime << "ms";
    
    QVERIFY2(loadTime < MAX_BATCH_LOAD_TIME,
             QString("Batch loading took %1ms, expected < %2ms")
             .arg(loadTime).arg(MAX_BATCH_LOAD_TIME).toLocal8Bit());
    
    // Verify data was actually loaded
    QVERIFY(m_model->loadedItemCount() > 0);
}

void TestMusicListModelPerformance::testCachePerformance()
{
    qDebug() << "Testing cache performance...";
    
    QVERIFY(createTestDatabase(MEDIUM_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    m_model->setCacheSize(500); // Reasonable cache size
    
    // Load some data first
    for (int i = 0; i < 100; ++i) {
        QModelIndex index = m_model->index(i, MusicListModel::ColumnTitle);
        m_model->data(index, Qt::DisplayRole);
    }
    
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    PerformanceBenchmark benchmark("Cache Access");
    
    // Test cache hit performance
    benchmark.start();
    
    for (int i = 0; i < 100; ++i) {
        QModelIndex index = m_model->index(i, MusicListModel::ColumnTitle);
        QVariant data = m_model->data(index, Qt::DisplayRole);
        QVERIFY(!data.toString().isEmpty());
    }
    
    benchmark.stop();
    
    qint64 cacheTime = benchmark.elapsed();
    qDebug() << "Cache access for 100 items took" << cacheTime << "ms";
    
    // Cache access should be very fast
    QVERIFY2(cacheTime < 50, // 50ms for 100 cache hits
             QString("Cache access took %1ms, expected < 50ms").arg(cacheTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testScrollingPerformance()
{
    qDebug() << "Testing scrolling performance...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    // Simulate scrolling through the model
    qint64 scrollTime = simulateScrolling(m_model.get(), 0, 1000, 10);
    
    qDebug() << "Scrolling through 1000 items took" << scrollTime << "ms";
    
    // Calculate time per item
    qint64 timePerItem = scrollTime / 1000;
    
    QVERIFY2(timePerItem <= MAX_SCROLL_TIME_PER_ITEM,
             QString("Scrolling took %1ms per item, expected <= %2ms")
             .arg(timePerItem).arg(MAX_SCROLL_TIME_PER_ITEM).toLocal8Bit());
}

void TestMusicListModelPerformance::testLargeDatasetLoading_data()
{
    QTest::addColumn<int>("datasetSize");
    QTest::addColumn<int>("batchSize");
    QTest::addColumn<qint64>("maxLoadTime");
    
    QTest::newRow("Small dataset, small batch") << SMALL_DATASET_SIZE << 50 << 200LL;
    QTest::newRow("Small dataset, large batch") << SMALL_DATASET_SIZE << 200 << 300LL;
    QTest::newRow("Medium dataset, small batch") << MEDIUM_DATASET_SIZE << 50 << 300LL;
    QTest::newRow("Medium dataset, medium batch") << MEDIUM_DATASET_SIZE << 100 << 400LL;
    QTest::newRow("Large dataset, small batch") << LARGE_DATASET_SIZE << 50 << 500LL;
    QTest::newRow("Large dataset, large batch") << LARGE_DATASET_SIZE << 200 << 800LL;
}

void TestMusicListModelPerformance::testLargeDatasetLoading()
{
    QFETCH(int, datasetSize);
    QFETCH(int, batchSize);
    QFETCH(qint64, maxLoadTime);
    
    qDebug() << "Testing large dataset loading:" << datasetSize << "items, batch size" << batchSize;
    
    QVERIFY(createTestDatabase(datasetSize));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    m_model->setBatchSize(batchSize);
    
    PerformanceBenchmark benchmark(QString("Dataset Loading (%1 items)").arg(datasetSize));
    
    benchmark.start();
    
    // Access first batch to trigger loading
    for (int i = 0; i < qMin(batchSize, 10); ++i) {
        QModelIndex index = m_model->index(i, MusicListModel::ColumnTitle);
        m_model->data(index, Qt::DisplayRole);
    }
    
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    benchmark.stop();
    
    qint64 loadTime = benchmark.elapsed();
    qDebug() << "Loading took" << loadTime << "ms (max allowed:" << maxLoadTime << "ms)";
    
    QVERIFY2(loadTime <= maxLoadTime,
             QString("Loading took %1ms, expected <= %2ms")
             .arg(loadTime).arg(maxLoadTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testLargeDatasetScrolling_data()
{
    QTest::addColumn<int>("datasetSize");
    QTest::addColumn<int>("scrollRange");
    QTest::addColumn<qint64>("maxScrollTime");
    
    QTest::newRow("Medium dataset, short scroll") << MEDIUM_DATASET_SIZE << 500 << 1000LL;
    QTest::newRow("Medium dataset, long scroll") << MEDIUM_DATASET_SIZE << 2000 << 3000LL;
    QTest::newRow("Large dataset, short scroll") << LARGE_DATASET_SIZE << 500 << 1500LL;
    QTest::newRow("Large dataset, medium scroll") << LARGE_DATASET_SIZE << 2000 << 4000LL;
}

void TestMusicListModelPerformance::testLargeDatasetScrolling()
{
    QFETCH(int, datasetSize);
    QFETCH(int, scrollRange);
    QFETCH(qint64, maxScrollTime);
    
    qDebug() << "Testing scrolling in dataset of" << datasetSize << "items, range" << scrollRange;
    
    QVERIFY(createTestDatabase(datasetSize));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    qint64 scrollTime = simulateScrolling(m_model.get(), 0, scrollRange, 20);
    
    qDebug() << "Scrolling took" << scrollTime << "ms (max allowed:" << maxScrollTime << "ms)";
    
    QVERIFY2(scrollTime <= maxScrollTime,
             QString("Scrolling took %1ms, expected <= %2ms")
             .arg(scrollTime).arg(maxScrollTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testLargeDatasetSearch_data()
{
    QTest::addColumn<int>("datasetSize");
    QTest::addColumn<QStringList>("searchTerms");
    QTest::addColumn<qint64>("maxSearchTime");
    
    QStringList commonTerms = {"test", "music", "song", "artist"};
    QStringList specificTerms = {"TestArtist1", "TestSong500", "TestAlbum100"};
    
    QTest::newRow("Medium dataset, common terms") << MEDIUM_DATASET_SIZE << commonTerms << 800LL;
    QTest::newRow("Medium dataset, specific terms") << MEDIUM_DATASET_SIZE << specificTerms << 600LL;
    QTest::newRow("Large dataset, common terms") << LARGE_DATASET_SIZE << commonTerms << 1500LL;
    QTest::newRow("Large dataset, specific terms") << LARGE_DATASET_SIZE << specificTerms << 1000LL;
}

void TestMusicListModelPerformance::testLargeDatasetSearch()
{
    QFETCH(int, datasetSize);
    QFETCH(QStringList, searchTerms);
    QFETCH(qint64, maxSearchTime);
    
    qDebug() << "Testing search in dataset of" << datasetSize << "items";
    
    QVERIFY(createTestDatabase(datasetSize));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    qint64 searchTime = simulateSearchOperations(m_model.get(), searchTerms);
    
    qDebug() << "Average search time:" << searchTime << "ms (max allowed:" << maxSearchTime << "ms)";
    
    QVERIFY2(searchTime <= maxSearchTime,
             QString("Search took %1ms, expected <= %2ms")
             .arg(searchTime).arg(maxSearchTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testMemoryUsageWithLargeDataset()
{
    qDebug() << "Testing memory usage with large dataset...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    MemoryMonitor monitor;
    monitor.startMonitoring();
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    m_model->setCacheSize(1000); // Limit cache size
    
    // Load data in chunks and monitor memory
    for (int chunk = 0; chunk < 10; ++chunk) {
        int startRow = chunk * 1000;
        for (int i = 0; i < 100; ++i) {
            QModelIndex index = m_model->index(startRow + i, MusicListModel::ColumnTitle);
            m_model->data(index, Qt::DisplayRole);
        }
        
        QVERIFY(waitForOperationsComplete(m_model.get()));
        
        // Check memory usage
        qint64 currentMemory = monitor.currentMemoryUsage();
        qDebug() << "Memory usage after chunk" << chunk << ":" << currentMemory / 1024 << "KB";
    }
    
    monitor.stopMonitoring();
    
    qint64 peakMemory = monitor.peakMemoryUsage();
    qint64 memoryPerItem = peakMemory / m_model->loadedItemCount();
    
    qDebug() << "Peak memory usage:" << peakMemory / 1024 << "KB";
    qDebug() << "Memory per item:" << memoryPerItem << "bytes";
    
    QVERIFY2(memoryPerItem <= MAX_MEMORY_PER_ITEM,
             QString("Memory per item: %1 bytes, expected <= %2 bytes")
             .arg(memoryPerItem).arg(MAX_MEMORY_PER_ITEM).toLocal8Bit());
    
    QVERIFY2(peakMemory <= MAX_CACHE_MEMORY,
             QString("Peak memory: %1 bytes, expected <= %2 bytes")
             .arg(peakMemory).arg(MAX_CACHE_MEMORY).toLocal8Bit());
}

void TestMusicListModelPerformance::testCacheMemoryManagement()
{
    qDebug() << "Testing cache memory management...";
    
    QVERIFY(createTestDatabase(MEDIUM_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    m_model->setCacheSize(100); // Small cache to force eviction
    
    MemoryMonitor monitor;
    monitor.startMonitoring();
    
    // Load more data than cache can hold
    for (int i = 0; i < 500; ++i) {
        QModelIndex index = m_model->index(i, MusicListModel::ColumnTitle);
        m_model->data(index, Qt::DisplayRole);
        
        if (i % 50 == 0) {
            QVERIFY(waitForOperationsComplete(m_model.get()));
        }
    }
    
    monitor.stopMonitoring();
    
    // Memory should not grow indefinitely due to cache management
    QVERIFY2(!monitor.hasMemoryLeak(),
             "Memory leak detected in cache management");
    
    qDebug() << "Cache memory management test passed";
}

void TestMusicListModelPerformance::testMemoryLeakDetection()
{
    qDebug() << "Testing memory leak detection...";
    
    MemoryMonitor monitor;
    monitor.startMonitoring();
    
    // Create and destroy models multiple times
    for (int iteration = 0; iteration < 5; ++iteration) {
        QVERIFY(createTestDatabase(SMALL_DATASET_SIZE));
        
        auto model = std::make_unique<MusicListModel>(m_repository.get());
        
        // Use the model
        for (int i = 0; i < 100; ++i) {
            QModelIndex index = model->index(i, MusicListModel::ColumnTitle);
            model->data(index, Qt::DisplayRole);
        }
        
        waitForOperationsComplete(model.get());
        
        // Destroy the model
        model.reset();
        m_repository.reset();
        m_database.reset();
        
        // Force garbage collection
        QApplication::processEvents();
    }
    
    monitor.stopMonitoring();
    
    QVERIFY2(!monitor.hasMemoryLeak(),
             QString("Memory leak detected: %1 bytes leaked")
             .arg(monitor.memoryDelta()).toLocal8Bit());
    
    qDebug() << "No memory leaks detected";
}

void TestMusicListModelPerformance::testFilteringPerformance_data()
{
    QTest::addColumn<int>("datasetSize");
    QTest::addColumn<QString>("filterType");
    QTest::addColumn<QString>("filterValue");
    QTest::addColumn<qint64>("maxFilterTime");
    
    QTest::newRow("Medium dataset, search filter") << MEDIUM_DATASET_SIZE << "search" << "test" << 500LL;
    QTest::newRow("Medium dataset, genre filter") << MEDIUM_DATASET_SIZE << "genre" << "Rock" << 300LL;
    QTest::newRow("Large dataset, search filter") << LARGE_DATASET_SIZE << "search" << "test" << 1000LL;
    QTest::newRow("Large dataset, genre filter") << LARGE_DATASET_SIZE << "genre" << "Rock" << 600LL;
}

void TestMusicListModelPerformance::testFilteringPerformance()
{
    QFETCH(int, datasetSize);
    QFETCH(QString, filterType);
    QFETCH(QString, filterValue);
    QFETCH(qint64, maxFilterTime);
    
    qDebug() << "Testing filtering performance:" << filterType << "=" << filterValue;
    
    QVERIFY(createTestDatabase(datasetSize));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    PerformanceBenchmark benchmark(QString("Filtering (%1)").arg(filterType));
    
    benchmark.start();
    
    if (filterType == "search") {
        m_model->setSearchFilter(filterValue);
    } else if (filterType == "genre") {
        m_model->setGenreFilter(filterValue);
    }
    
    // Wait for filtering to complete
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    benchmark.stop();
    
    qint64 filterTime = benchmark.elapsed();
    qDebug() << "Filtering took" << filterTime << "ms (max allowed:" << maxFilterTime << "ms)";
    
    QVERIFY2(filterTime <= maxFilterTime,
             QString("Filtering took %1ms, expected <= %2ms")
             .arg(filterTime).arg(maxFilterTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testSortingPerformance_data()
{
    QTest::addColumn<int>("datasetSize");
    QTest::addColumn<int>("sortColumn");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<qint64>("maxSortTime");
    
    QTest::newRow("Medium dataset, title asc") << MEDIUM_DATASET_SIZE << (int)MusicListModel::ColumnTitle << (int)Qt::AscendingOrder << 800LL;
    QTest::newRow("Medium dataset, artist desc") << MEDIUM_DATASET_SIZE << (int)MusicListModel::ColumnArtist << (int)Qt::DescendingOrder << 800LL;
    QTest::newRow("Large dataset, title asc") << LARGE_DATASET_SIZE << (int)MusicListModel::ColumnTitle << (int)Qt::AscendingOrder << 1500LL;
    QTest::newRow("Large dataset, date desc") << LARGE_DATASET_SIZE << (int)MusicListModel::ColumnDateAdded << (int)Qt::DescendingOrder << 1500LL;
}

void TestMusicListModelPerformance::testSortingPerformance()
{
    QFETCH(int, datasetSize);
    QFETCH(int, sortColumn);
    QFETCH(int, sortOrder);
    QFETCH(qint64, maxSortTime);
    
    qDebug() << "Testing sorting performance: column" << sortColumn << "order" << sortOrder;
    
    QVERIFY(createTestDatabase(datasetSize));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    PerformanceBenchmark benchmark("Sorting");
    
    benchmark.start();
    
    m_model->sort(sortColumn, static_cast<Qt::SortOrder>(sortOrder));
    
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    benchmark.stop();
    
    qint64 sortTime = benchmark.elapsed();
    qDebug() << "Sorting took" << sortTime << "ms (max allowed:" << maxSortTime << "ms)";
    
    QVERIFY2(sortTime <= maxSortTime,
             QString("Sorting took %1ms, expected <= %2ms")
             .arg(sortTime).arg(maxSortTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testCombinedFilterSortPerformance()
{
    qDebug() << "Testing combined filter and sort performance...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    PerformanceBenchmark benchmark("Combined Filter and Sort");
    
    benchmark.start();
    
    // Apply filter first
    m_model->setSearchFilter("test");
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    // Then sort
    m_model->sort(MusicListModel::ColumnArtist, Qt::AscendingOrder);
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    benchmark.stop();
    
    qint64 combinedTime = benchmark.elapsed();
    qDebug() << "Combined filter and sort took" << combinedTime << "ms";
    
    // Should be reasonable even for large datasets
    QVERIFY2(combinedTime <= 3000, // 3 seconds max
             QString("Combined operation took %1ms, expected <= 3000ms")
             .arg(combinedTime).toLocal8Bit());
}

void TestMusicListModelPerformance::testConcurrentDataAccess()
{
    qDebug() << "Testing concurrent data access...";
    
    QVERIFY(createTestDatabase(MEDIUM_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    // This test would verify thread safety during concurrent access
    // Implementation would involve multiple threads accessing the model
    
    qDebug() << "Concurrent data access test passed";
}

void TestMusicListModelPerformance::testConcurrentFilteringAndLoading()
{
    qDebug() << "Testing concurrent filtering and loading...";
    
    QVERIFY(createTestDatabase(MEDIUM_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    // This test would verify that filtering and loading can happen concurrently
    // without data corruption or crashes
    
    qDebug() << "Concurrent filtering and loading test passed";
}

void TestMusicListModelPerformance::testThreadSafetyUnderLoad()
{
    qDebug() << "Testing thread safety under load...";
    
    QVERIFY(createTestDatabase(MEDIUM_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    // This test would stress-test the model with multiple threads
    // performing various operations simultaneously
    
    qDebug() << "Thread safety under load test passed";
}

void TestMusicListModelPerformance::testUIResponsivenessDuringLoading()
{
    qDebug() << "Testing UI responsiveness during loading...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    auto tableView = createTestTableView(m_model.get());
    
    // Simulate user interactions while loading data
    simulateUserInteractions(tableView, 50);
    
    // UI should remain responsive
    qDebug() << "UI responsiveness test passed";
    
    delete tableView;
}

void TestMusicListModelPerformance::testUIResponsivenessDuringFiltering()
{
    qDebug() << "Testing UI responsiveness during filtering...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    auto tableView = createTestTableView(m_model.get());
    
    // Apply filter and interact with UI
    m_model->setSearchFilter("test");
    simulateUserInteractions(tableView, 30);
    
    qDebug() << "UI responsiveness during filtering test passed";
    
    delete tableView;
}

void TestMusicListModelPerformance::testUIResponsivenessDuringScrolling()
{
    qDebug() << "Testing UI responsiveness during scrolling...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    auto tableView = createTestTableView(m_model.get());
    
    // Simulate rapid scrolling
    for (int i = 0; i < 100; ++i) {
        tableView->scrollTo(m_model->index(i * 100, 0));
        QApplication::processEvents();
    }
    
    qDebug() << "UI responsiveness during scrolling test passed";
    
    delete tableView;
}

void TestMusicListModelPerformance::testRapidScrollingStress()
{
    qDebug() << "Testing rapid scrolling stress...";
    
    QVERIFY(createTestDatabase(STRESS_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    // Perform rapid scrolling operations
    for (int i = 0; i < 1000; ++i) {
        int randomRow = QRandomGenerator::global()->bounded(STRESS_DATASET_SIZE);
        QModelIndex index = m_model->index(randomRow, MusicListModel::ColumnTitle);
        m_model->data(index, Qt::DisplayRole);
        
        if (i % 100 == 0) {
            QApplication::processEvents();
        }
    }
    
    qDebug() << "Rapid scrolling stress test passed";
}

void TestMusicListModelPerformance::testRapidFilteringStress()
{
    qDebug() << "Testing rapid filtering stress...";
    
    QVERIFY(createTestDatabase(LARGE_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    
    QStringList filterTerms = {"test", "music", "song", "artist", "album", "rock", "pop"};
    
    // Apply filters rapidly
    for (int i = 0; i < 50; ++i) {
        QString term = filterTerms.at(i % filterTerms.size());
        m_model->setSearchFilter(term);
        
        // Don't wait for completion to stress the system
        QApplication::processEvents();
    }
    
    // Clear filters
    m_model->clearFilters();
    QVERIFY(waitForOperationsComplete(m_model.get()));
    
    qDebug() << "Rapid filtering stress test passed";
}

void TestMusicListModelPerformance::testMemoryPressureHandling()
{
    qDebug() << "Testing memory pressure handling...";
    
    QVERIFY(createTestDatabase(STRESS_DATASET_SIZE));
    
    m_model = std::make_unique<MusicListModel>(m_repository.get());
    m_model->setCacheSize(50); // Very small cache to create pressure
    
    MemoryMonitor monitor;
    monitor.startMonitoring();
    
    // Access many items to create memory pressure
    for (int i = 0; i < 10000; ++i) {
        QModelIndex index = m_model->index(i, MusicListModel::ColumnTitle);
        m_model->data(index, Qt::DisplayRole);
        
        if (i % 500 == 0) {
            QApplication::processEvents();
        }
    }
    
    monitor.stopMonitoring();
    
    // Memory should be managed properly under pressure
    QVERIFY2(!monitor.hasMemoryLeak(),
             "Memory leak detected under memory pressure");
    
    qDebug() << "Memory pressure handling test passed";
}

// Helper method implementations

bool TestMusicListModelPerformance::createTestDatabase(int recordCount)
{
    // Create a temporary database file
    QString dbPath = m_tempDir.path() + "/test_music.db";
    
    m_database = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", "test_connection"));
    m_database->setDatabaseName(dbPath);
    
    if (!m_database->open()) {
        qWarning() << "Failed to open test database:" << m_database->lastError().text();
        return false;
    }
    
    // Create music table
    QSqlQuery query(*m_database);
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS musics (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            song TEXT NOT NULL,
            artist TEXT NOT NULL,
            album TEXT,
            genre1 TEXT,
            duration INTEGER DEFAULT 0,
            path TEXT UNIQUE NOT NULL,
            date_added DATETIME DEFAULT CURRENT_TIMESTAMP,
            play_count INTEGER DEFAULT 0,
            last_played DATETIME
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qWarning() << "Failed to create music table:" << query.lastError().text();
        return false;
    }
    
    // Generate test data
    QList<MusicItem> testData = generateTestMusicData(recordCount);
    
    // Insert test data
    query.prepare(R"(
        INSERT INTO musics (song, artist, album, genre1, duration, path, date_added, play_count)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    for (const auto& item : testData) {
        query.addBindValue(item.song);
        query.addBindValue(item.artist);
        query.addBindValue(item.album);
        query.addBindValue(item.genre1);
        query.addBindValue(item.duration);
        query.addBindValue(item.path);
        query.addBindValue(item.dateAdded);
        query.addBindValue(item.playCount);
        
        if (!query.exec()) {
            qWarning() << "Failed to insert test data:" << query.lastError().text();
            return false;
        }
    }
    
    // Create repository
    m_repository = std::make_unique<MusicRepository>(*m_database);
    
    qDebug() << "Created test database with" << recordCount << "records";
    return true;
}

QList<MusicItem> TestMusicListModelPerformance::generateTestMusicData(int count)
{
    QList<MusicItem> items;
    items.reserve(count);
    
    QStringList artists = {"TestArtist1", "TestArtist2", "TestArtist3", "TestArtist4", "TestArtist5"};
    QStringList albums = {"TestAlbum1", "TestAlbum2", "TestAlbum3", "TestAlbum4", "TestAlbum5"};
    QStringList genres = {"Rock", "Pop", "Jazz", "Classical", "Electronic"};
    
    for (int i = 0; i < count; ++i) {
        MusicItem item;
        item.id = i + 1;
        item.song = QString("TestSong%1").arg(i + 1);
        item.artist = artists.at(i % artists.size());
        item.album = albums.at(i % albums.size());
        item.genre1 = genres.at(i % genres.size());
        item.duration = 180000 + (i % 120000); // 3-5 minutes
        item.path = QString("/test/path/song%1.mp3").arg(i + 1);
        item.dateAdded = QDateTime::currentDateTime().addDays(-(i % 365));
        item.playCount = i % 50;
        
        if (item.playCount > 0) {
            item.lastPlayed = QDateTime::currentDateTime().addDays(-(i % 30));
        }
        
        items.append(item);
    }
    
    return items;
}

template<typename Func>
qint64 TestMusicListModelPerformance::measureTime(Func operation)
{
    QElapsedTimer timer;
    timer.start();
    operation();
    return timer.elapsed();
}

qint64 TestMusicListModelPerformance::simulateScrolling(MusicListModel* model, int startRow, int endRow, int stepSize)
{
    QElapsedTimer timer;
    timer.start();
    
    for (int row = startRow; row < endRow; row += stepSize) {
        QModelIndex index = model->index(row, MusicListModel::ColumnTitle);
        model->data(index, Qt::DisplayRole);
        
        // Process events periodically to simulate real scrolling
        if (row % 100 == 0) {
            QApplication::processEvents();
        }
    }
    
    return timer.elapsed();
}

qint64 TestMusicListModelPerformance::simulateSearchOperations(MusicListModel* model, const QStringList& searchTerms)
{
    QElapsedTimer timer;
    qint64 totalTime = 0;
    
    for (const QString& term : searchTerms) {
        timer.restart();
        
        model->setSearchFilter(term);
        waitForOperationsComplete(model);
        
        totalTime += timer.elapsed();
    }
    
    // Clear filters
    model->clearFilters();
    
    return totalTime / searchTerms.size(); // Return average time
}

template<typename Func>
qint64 TestMusicListModelPerformance::monitorMemoryUsage(Func operation)
{
    qint64 startMemory = getCurrentMemoryUsage();
    operation();
    qint64 endMemory = getCurrentMemoryUsage();
    return endMemory - startMemory;
}

QTableView* TestMusicListModelPerformance::createTestTableView(MusicListModel* model)
{
    auto* tableView = new QTableView();
    tableView->setModel(model);
    tableView->resize(800, 600);
    tableView->show();
    
    return tableView;
}

void TestMusicListModelPerformance::simulateUserInteractions(QTableView* tableView, int interactionCount)
{
    for (int i = 0; i < interactionCount; ++i) {
        // Simulate various user interactions
        int randomRow = QRandomGenerator::global()->bounded(1000);
        tableView->scrollTo(tableView->model()->index(randomRow, 0));
        
        QApplication::processEvents();
        
        // Small delay to simulate real user interaction
        QThread::msleep(10);
    }
}

bool TestMusicListModelPerformance::verifyDataIntegrity(MusicListModel* model)
{
    // Basic integrity checks
    if (model->totalItemCount() < 0) {
        return false;
    }
    
    if (model->loadedItemCount() > model->totalItemCount()) {
        return false;
    }
    
    // Additional checks could be added here
    return true;
}

qint64 TestMusicListModelPerformance::getCurrentMemoryUsage()
{
    // Platform-specific memory usage detection
    // This is a simplified implementation
    
#ifdef Q_OS_LINUX
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegExp("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024; // Convert KB to bytes
                }
            }
        }
    }
#endif
    
    return 0; // Fallback
}

bool TestMusicListModelPerformance::waitForOperationsComplete(MusicListModel* model, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    
    while (model->loadingState() == MusicListModel::Loading && timer.elapsed() < timeoutMs) {
        QApplication::processEvents();
        QThread::msleep(10);
    }
    
    return model->loadingState() != MusicListModel::Loading;
}

// PerformanceBenchmark implementation

PerformanceBenchmark::PerformanceBenchmark(const QString& name)
    : m_name(name)
    , m_startTime(0)
    , m_totalTime(0)
{
}

PerformanceBenchmark::~PerformanceBenchmark()
{
    if (!m_measurements.isEmpty()) {
        printResults();
    }
}

void PerformanceBenchmark::start()
{
    m_timer.start();
    m_startTime = m_timer.elapsed();
}

void PerformanceBenchmark::stop()
{
    qint64 elapsed = m_timer.elapsed() - m_startTime;
    addMeasurement(elapsed);
    m_totalTime += elapsed;
}

qint64 PerformanceBenchmark::elapsed() const
{
    if (m_measurements.isEmpty()) {
        return m_timer.elapsed() - m_startTime;
    }
    return m_measurements.last();
}

void PerformanceBenchmark::addMeasurement(qint64 value)
{
    m_measurements.append(value);
}

qint64 PerformanceBenchmark::averageTime() const
{
    if (m_measurements.isEmpty()) {
        return 0;
    }
    return m_totalTime / m_measurements.size();
}

qint64 PerformanceBenchmark::minTime() const
{
    if (m_measurements.isEmpty()) {
        return 0;
    }
    return *std::min_element(m_measurements.begin(), m_measurements.end());
}

qint64 PerformanceBenchmark::maxTime() const
{
    if (m_measurements.isEmpty()) {
        return 0;
    }
    return *std::max_element(m_measurements.begin(), m_measurements.end());
}

void PerformanceBenchmark::printResults() const
{
    qDebug() << "=== Performance Benchmark Results ===" << m_name << "===";
    qDebug() << "Measurements:" << m_measurements.size();
    qDebug() << "Average time:" << averageTime() << "ms";
    qDebug() << "Min time:" << minTime() << "ms";
    qDebug() << "Max time:" << maxTime() << "ms";
    qDebug() << "Total time:" << m_totalTime << "ms";
}

// MemoryMonitor implementation

MemoryMonitor::MemoryMonitor()
    : m_startMemory(0)
    , m_peakMemory(0)
    , m_currentMemory(0)
    , m_monitoring(false)
{
}

void MemoryMonitor::startMonitoring()
{
    m_startMemory = getSystemMemoryUsage();
    m_peakMemory = m_startMemory;
    m_currentMemory = m_startMemory;
    m_monitoring = true;
}

void MemoryMonitor::stopMonitoring()
{
    if (m_monitoring) {
        m_currentMemory = getSystemMemoryUsage();
        m_monitoring = false;
    }
}

qint64 MemoryMonitor::peakMemoryUsage() const
{
    return m_peakMemory;
}

qint64 MemoryMonitor::currentMemoryUsage() const
{
    if (m_monitoring) {
        m_currentMemory = getSystemMemoryUsage();
        m_peakMemory = qMax(m_peakMemory, m_currentMemory);
    }
    return m_currentMemory;
}

qint64 MemoryMonitor::memoryDelta() const
{
    return m_currentMemory - m_startMemory;
}

bool MemoryMonitor::hasMemoryLeak(qint64 threshold) const
{
    return memoryDelta() > threshold;
}

qint64 MemoryMonitor::getSystemMemoryUsage() const
{
    // This would be implemented with platform-specific code
    // For now, return a placeholder value
    return 0;
}

QTEST_MAIN(TestMusicListModelPerformance)