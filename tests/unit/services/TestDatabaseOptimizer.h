#ifndef TESTDATABASEOPTIMIZER_H
#define TESTDATABASEOPTIMIZER_H

#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <memory>

class DatabaseOptimizer;

/**
 * @brief Unit tests for DatabaseOptimizer class
 * 
 * Tests database optimization functionality including index management,
 * query performance monitoring, and optimization recommendations.
 */
class TestDatabaseOptimizer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Initialization tests
    void testInitialization();
    void testInitializationWithClosedDatabase();

    // Index management tests
    void testCreateOptimalIndexes();
    void testCreateOptimalIndexesWithExistingIndexes();
    void testDropUnusedIndexes();
    void testIndexExists();
    void testCreateSingleIndex();
    void testDropSingleIndex();

    // Database optimization tests
    void testAnalyzeDatabase();
    void testVacuumDatabase();
    void testReindexDatabase();

    // Query monitoring tests
    void testStartStopQueryMonitoring();
    void testRecordQueryExecution();
    void testSlowQueryDetection();
    void testQueryStatistics();
    void testClearQueryStatistics();

    // Performance metrics tests
    void testGetPerformanceMetrics();
    void testUpdateMetrics();
    void testMetricsCache();

    // Optimization recommendations tests
    void testGetOptimizationRecommendations();
    void testApplyRecommendation();
    void testRecommendationPriority();

    // Auto optimization tests
    void testAutoOptimizationEnabled();
    void testAutoOptimizationInterval();
    void testPerformAutoOptimization();

    // Utility tests
    void testNormalizeQuery();
    void testGetTableNames();
    void testGetColumnNames();
    void testExportPerformanceData();

    // Error handling tests
    void testErrorHandling();
    void testInvalidDatabase();

    // Performance tests
    void testLargeDatasetPerformance();
    void testConcurrentAccess();

private:
    void createTestDatabase();
    void populateTestData();
    void createTestIndexes();
    
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QSqlDatabase m_database;
    std::unique_ptr<DatabaseOptimizer> m_optimizer;
    QString m_connectionName;
};

#endif // TESTDATABASEOPTIMIZER_H