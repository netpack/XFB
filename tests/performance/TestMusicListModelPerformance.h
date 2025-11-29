#ifndef TESTMUSICLISTMODELPERFORMANCE_H
#define TESTMUSICLISTMODELPERFORMANCE_H

#include <QObject>
#include <QTest>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <memory>

// Forward declarations
class MusicListModel;
class MusicFilterProxyModel;
class MusicRepository;
class QSqlDatabase;
class QTableView;

/**
 * @brief Performance tests for MusicListModel lazy loading
 * 
 * These tests verify that the lazy loading implementation performs
 * well with large datasets and provides responsive UI interaction.
 * 
 * @since XFB 2.0
 */
class TestMusicListModelPerformance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic performance tests
    void testModelInitializationPerformance();
    void testLazyLoadingPerformance();
    void testCachePerformance();
    void testScrollingPerformance();

    // Large dataset tests
    void testLargeDatasetLoading_data();
    void testLargeDatasetLoading();
    void testLargeDatasetScrolling_data();
    void testLargeDatasetScrolling();
    void testLargeDatasetSearch_data();
    void testLargeDatasetSearch();

    // Memory usage tests
    void testMemoryUsageWithLargeDataset();
    void testCacheMemoryManagement();
    void testMemoryLeakDetection();

    // Filtering and sorting performance
    void testFilteringPerformance_data();
    void testFilteringPerformance();
    void testSortingPerformance_data();
    void testSortingPerformance();
    void testCombinedFilterSortPerformance();

    // Concurrent access tests
    void testConcurrentDataAccess();
    void testConcurrentFilteringAndLoading();
    void testThreadSafetyUnderLoad();

    // UI responsiveness tests
    void testUIResponsivenessDuringLoading();
    void testUIResponsivenessDuringFiltering();
    void testUIResponsivenessDuringScrolling();

    // Stress tests
    void testRapidScrollingStress();
    void testRapidFilteringStress();
    void testMemoryPressureHandling();

private:
    /**
     * @brief Create a test database with specified number of records
     * @param recordCount Number of music records to create
     * @return true if database was created successfully
     */
    bool createTestDatabase(int recordCount);

    /**
     * @brief Generate test music data
     * @param count Number of records to generate
     * @return List of generated music items
     */
    QList<MusicItem> generateTestMusicData(int count);

    /**
     * @brief Measure time for a specific operation
     * @param operation Lambda function to measure
     * @return Elapsed time in milliseconds
     */
    template<typename Func>
    qint64 measureTime(Func operation);

    /**
     * @brief Simulate scrolling through the model
     * @param model Model to scroll through
     * @param startRow Starting row
     * @param endRow Ending row
     * @param stepSize Step size for scrolling
     * @return Time taken for scrolling
     */
    qint64 simulateScrolling(MusicListModel* model, int startRow, int endRow, int stepSize = 10);

    /**
     * @brief Simulate search operations
     * @param model Model to search in
     * @param searchTerms List of search terms to test
     * @return Average search time
     */
    qint64 simulateSearchOperations(MusicListModel* model, const QStringList& searchTerms);

    /**
     * @brief Monitor memory usage during operation
     * @param operation Operation to monitor
     * @return Peak memory usage in bytes
     */
    template<typename Func>
    qint64 monitorMemoryUsage(Func operation);

    /**
     * @brief Create a table view for UI testing
     * @param model Model to attach to the view
     * @return Pointer to the created table view
     */
    QTableView* createTestTableView(MusicListModel* model);

    /**
     * @brief Simulate user interactions with the table view
     * @param tableView Table view to interact with
     * @param interactionCount Number of interactions to simulate
     */
    void simulateUserInteractions(QTableView* tableView, int interactionCount);

    /**
     * @brief Verify data integrity after operations
     * @param model Model to verify
     * @return true if data integrity is maintained
     */
    bool verifyDataIntegrity(MusicListModel* model);

    /**
     * @brief Get current memory usage
     * @return Memory usage in bytes
     */
    qint64 getCurrentMemoryUsage();

    /**
     * @brief Wait for all pending operations to complete
     * @param model Model to wait for
     * @param timeoutMs Timeout in milliseconds
     * @return true if operations completed within timeout
     */
    bool waitForOperationsComplete(MusicListModel* model, int timeoutMs = 5000);

private:
    QTemporaryDir m_tempDir;
    std::unique_ptr<QSqlDatabase> m_database;
    std::unique_ptr<MusicRepository> m_repository;
    std::unique_ptr<MusicListModel> m_model;
    std::unique_ptr<MusicFilterProxyModel> m_proxyModel;
    
    // Performance thresholds (in milliseconds)
    static constexpr qint64 MAX_INITIALIZATION_TIME = 100;
    static constexpr qint64 MAX_BATCH_LOAD_TIME = 500;
    static constexpr qint64 MAX_SEARCH_TIME = 1000;
    static constexpr qint64 MAX_SORT_TIME = 2000;
    static constexpr qint64 MAX_SCROLL_TIME_PER_ITEM = 1;
    
    // Memory thresholds (in bytes)
    static constexpr qint64 MAX_MEMORY_PER_ITEM = 1024; // 1KB per item
    static constexpr qint64 MAX_CACHE_MEMORY = 100 * 1024 * 1024; // 100MB
    
    // Test data sizes
    static constexpr int SMALL_DATASET_SIZE = 1000;
    static constexpr int MEDIUM_DATASET_SIZE = 10000;
    static constexpr int LARGE_DATASET_SIZE = 100000;
    static constexpr int STRESS_DATASET_SIZE = 1000000;
};

/**
 * @brief Benchmark helper class for measuring performance
 */
class PerformanceBenchmark
{
public:
    explicit PerformanceBenchmark(const QString& name);
    ~PerformanceBenchmark();
    
    void start();
    void stop();
    qint64 elapsed() const;
    
    void addMeasurement(qint64 value);
    qint64 averageTime() const;
    qint64 minTime() const;
    qint64 maxTime() const;
    
    void printResults() const;

private:
    QString m_name;
    QElapsedTimer m_timer;
    QList<qint64> m_measurements;
    qint64 m_startTime;
    qint64 m_totalTime;
};

/**
 * @brief Memory usage monitor for detecting leaks
 */
class MemoryMonitor
{
public:
    MemoryMonitor();
    
    void startMonitoring();
    void stopMonitoring();
    
    qint64 peakMemoryUsage() const;
    qint64 currentMemoryUsage() const;
    qint64 memoryDelta() const;
    
    bool hasMemoryLeak(qint64 threshold = 1024 * 1024) const; // 1MB threshold

private:
    qint64 m_startMemory;
    qint64 m_peakMemory;
    qint64 m_currentMemory;
    bool m_monitoring;
    
    qint64 getSystemMemoryUsage() const;
};

#endif // TESTMUSICLISTMODELPERFORMANCE_H