#ifndef DATABASEOPTIMIZER_H
#define DATABASEOPTIMIZER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QDateTime>
#include <QTimer>
#include <QMutex>
#include <QElapsedTimer>
#include <QHash>
#include <memory>

/**
 * @brief Service for database performance optimization and monitoring
 * 
 * The DatabaseOptimizer provides comprehensive database performance tuning
 * capabilities including index management, query performance monitoring,
 * and automatic optimization strategies. It helps maintain optimal database
 * performance as the music collection grows.
 * 
 * Features:
 * - Automatic index creation and management
 * - Query performance monitoring and analysis
 * - Database statistics collection
 * - Optimization recommendations
 * - Scheduled maintenance operations
 * - Performance regression detection
 * 
 * @example
 * @code
 * DatabaseOptimizer* optimizer = new DatabaseOptimizer(database, this);
 * optimizer->initialize();
 * 
 * // Create optimal indexes
 * optimizer->createOptimalIndexes();
 * 
 * // Monitor query performance
 * optimizer->startQueryMonitoring();
 * 
 * // Get performance recommendations
 * auto recommendations = optimizer->getOptimizationRecommendations();
 * @endcode
 * 
 * @since XFB 2.0
 */
class DatabaseOptimizer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Query performance statistics
     */
    struct QueryStats {
        QString queryPattern;           // Normalized query pattern
        int executionCount = 0;         // Number of times executed
        qint64 totalExecutionTime = 0;  // Total execution time in ms
        qint64 averageExecutionTime = 0; // Average execution time in ms
        qint64 minExecutionTime = 0;    // Minimum execution time in ms
        qint64 maxExecutionTime = 0;    // Maximum execution time in ms
        QDateTime firstSeen;            // When first seen
        QDateTime lastSeen;             // When last seen
        bool isSlowQuery = false;       // Flagged as slow query
    };

    /**
     * @brief Database index information
     */
    struct IndexInfo {
        QString name;
        QString tableName;
        QStringList columns;
        bool isUnique = false;
        bool isPartial = false;
        QString whereClause;
        qint64 size = 0;
        int useCount = 0;
        QDateTime lastUsed;
        bool isRecommended = false;
    };

    /**
     * @brief Optimization recommendation
     */
    struct OptimizationRecommendation {
        enum Type {
            CreateIndex,
            DropIndex,
            UpdateStatistics,
            Vacuum,
            Reindex,
            QueryOptimization
        };
        
        Type type;
        QString description;
        QString sqlCommand;
        int priority = 0; // Higher number = higher priority
        qint64 estimatedImpact = 0; // Estimated performance improvement in ms
    };

    /**
     * @brief Database performance metrics
     */
    struct PerformanceMetrics {
        qint64 totalQueries = 0;
        qint64 slowQueries = 0;
        qint64 averageQueryTime = 0;
        qint64 databaseSize = 0;
        int indexCount = 0;
        int unusedIndexCount = 0;
        QDateTime lastVacuum;
        QDateTime lastAnalyze;
        QDateTime lastReindex;
        double cacheHitRatio = 0.0;
        int pageSize = 0;
        int pageCount = 0;
    };

    explicit DatabaseOptimizer(QSqlDatabase& database, QObject* parent = nullptr);
    ~DatabaseOptimizer() override;

    /**
     * @brief Initialize the optimizer
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the optimizer and cleanup resources
     */
    void shutdown();

    /**
     * @brief Create optimal indexes for the current database schema
     * @return true if all indexes were created successfully
     */
    bool createOptimalIndexes();

    /**
     * @brief Drop unused or redundant indexes
     * @return Number of indexes dropped
     */
    int dropUnusedIndexes();

    /**
     * @brief Analyze database and update statistics
     * @return true if analysis completed successfully
     */
    bool analyzeDatabase();

    /**
     * @brief Vacuum database to reclaim space and optimize storage
     * @return true if vacuum completed successfully
     */
    bool vacuumDatabase();

    /**
     * @brief Reindex all database indexes
     * @return true if reindexing completed successfully
     */
    bool reindexDatabase();

    /**
     * @brief Start monitoring query performance
     * @param slowQueryThreshold Threshold in ms for flagging slow queries
     */
    void startQueryMonitoring(qint64 slowQueryThreshold = 100);

    /**
     * @brief Stop monitoring query performance
     */
    void stopQueryMonitoring();

    /**
     * @brief Record query execution for performance monitoring
     * @param query SQL query string
     * @param executionTime Execution time in milliseconds
     */
    void recordQueryExecution(const QString& query, qint64 executionTime);

    /**
     * @brief Get query performance statistics
     * @param limit Maximum number of results to return
     * @return List of query statistics ordered by total execution time
     */
    QList<QueryStats> getQueryStatistics(int limit = 50);

    /**
     * @brief Get slow queries that exceed the threshold
     * @return List of slow query statistics
     */
    QList<QueryStats> getSlowQueries();

    /**
     * @brief Get information about all database indexes
     * @return List of index information
     */
    QList<IndexInfo> getIndexInformation();

    /**
     * @brief Get optimization recommendations
     * @return List of recommendations ordered by priority
     */
    QList<OptimizationRecommendation> getOptimizationRecommendations();

    /**
     * @brief Get current performance metrics
     * @return Performance metrics structure
     */
    PerformanceMetrics getPerformanceMetrics();

    /**
     * @brief Apply an optimization recommendation
     * @param recommendation Recommendation to apply
     * @return true if applied successfully
     */
    bool applyRecommendation(const OptimizationRecommendation& recommendation);

    /**
     * @brief Set automatic optimization enabled/disabled
     * @param enabled true to enable automatic optimization
     */
    void setAutoOptimizationEnabled(bool enabled);

    /**
     * @brief Check if automatic optimization is enabled
     * @return true if automatic optimization is enabled
     */
    bool isAutoOptimizationEnabled() const;

    /**
     * @brief Set the interval for automatic optimization checks
     * @param intervalMs Interval in milliseconds
     */
    void setOptimizationInterval(int intervalMs);

    /**
     * @brief Get the current optimization interval
     * @return Interval in milliseconds
     */
    int optimizationInterval() const;

    /**
     * @brief Clear all query statistics
     */
    void clearQueryStatistics();

    /**
     * @brief Export performance data to JSON
     * @return JSON string containing performance data
     */
    QString exportPerformanceData();

signals:
    /**
     * @brief Emitted when a slow query is detected
     * @param query SQL query string
     * @param executionTime Execution time in milliseconds
     */
    void slowQueryDetected(const QString& query, qint64 executionTime);

    /**
     * @brief Emitted when optimization completes
     * @param operation Operation that completed
     * @param success true if successful
     * @param duration Duration in milliseconds
     */
    void optimizationCompleted(const QString& operation, bool success, qint64 duration);

    /**
     * @brief Emitted when new optimization recommendations are available
     * @param count Number of new recommendations
     */
    void recommendationsAvailable(int count);

    /**
     * @brief Emitted when performance metrics are updated
     * @param metrics Current performance metrics
     */
    void metricsUpdated(const PerformanceMetrics& metrics);

private slots:
    /**
     * @brief Perform automatic optimization checks
     */
    void performAutoOptimization();

    /**
     * @brief Update performance metrics
     */
    void updateMetrics();

private:
    /**
     * @brief Normalize query for pattern matching
     * @param query Raw SQL query
     * @return Normalized query pattern
     */
    QString normalizeQuery(const QString& query);

    /**
     * @brief Check if an index exists
     * @param indexName Name of the index
     * @return true if index exists
     */
    bool indexExists(const QString& indexName);

    /**
     * @brief Create a single index
     * @param indexName Name of the index
     * @param tableName Table name
     * @param columns List of columns
     * @param unique true for unique index
     * @param whereClause Optional WHERE clause for partial index
     * @return true if created successfully
     */
    bool createIndex(const QString& indexName, 
                    const QString& tableName,
                    const QStringList& columns,
                    bool unique = false,
                    const QString& whereClause = QString());

    /**
     * @brief Drop a single index
     * @param indexName Name of the index to drop
     * @return true if dropped successfully
     */
    bool dropIndex(const QString& indexName);

    /**
     * @brief Get table names in the database
     * @return List of table names
     */
    QStringList getTableNames();

    /**
     * @brief Get column names for a table
     * @param tableName Name of the table
     * @return List of column names
     */
    QStringList getColumnNames(const QString& tableName);

    /**
     * @brief Analyze query patterns to recommend indexes
     * @return List of recommended indexes
     */
    QList<IndexInfo> analyzeQueryPatterns();

    /**
     * @brief Check if index is being used
     * @param indexName Name of the index
     * @return true if index is being used
     */
    bool isIndexUsed(const QString& indexName);

    /**
     * @brief Get database page size
     * @return Page size in bytes
     */
    int getDatabasePageSize();

    /**
     * @brief Get database page count
     * @return Number of pages
     */
    int getDatabasePageCount();

    /**
     * @brief Calculate cache hit ratio
     * @return Cache hit ratio as percentage (0.0 - 1.0)
     */
    double calculateCacheHitRatio();

    /**
     * @brief Log optimization operation
     * @param operation Operation name
     * @param success true if successful
     * @param duration Duration in milliseconds
     * @param details Additional details
     */
    void logOptimization(const QString& operation, bool success, qint64 duration, const QString& details = QString());

    QSqlDatabase& m_database;
    
    // Query monitoring
    mutable QMutex m_statsMutex;
    QHash<QString, QueryStats> m_queryStats;
    qint64 m_slowQueryThreshold;
    bool m_monitoringEnabled;
    
    // Index management
    mutable QMutex m_indexMutex;
    QList<IndexInfo> m_indexCache;
    QDateTime m_indexCacheUpdated;
    
    // Auto optimization
    QTimer* m_optimizationTimer;
    bool m_autoOptimizationEnabled;
    int m_optimizationInterval;
    
    // Performance metrics
    mutable QMutex m_metricsMutex;
    PerformanceMetrics m_metrics;
    QDateTime m_metricsLastUpdated;
    
    // Constants
    static constexpr qint64 DEFAULT_SLOW_QUERY_THRESHOLD = 100; // 100ms
    static constexpr int DEFAULT_OPTIMIZATION_INTERVAL = 3600000; // 1 hour
    static constexpr int INDEX_CACHE_DURATION = 300000; // 5 minutes
    static constexpr int METRICS_CACHE_DURATION = 60000; // 1 minute
};

#endif // DATABASEOPTIMIZER_H