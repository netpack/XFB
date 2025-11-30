#ifndef ACCESSIBILITYPERFORMANCEMONITOR_H
#define ACCESSIBILITYPERFORMANCEMONITOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QHash>
#include <QVariant>
#include <QDateTime>

/**
 * @brief Performance monitoring system for accessibility operations
 * 
 * Monitors and optimizes performance of accessibility-related operations
 * including announcement latency, memory usage, and focus change performance.
 * Provides metrics and automatic optimization suggestions.
 */
class AccessibilityPerformanceMonitor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Performance metric types
     */
    enum MetricType {
        AnnouncementLatency,    ///< Time from announcement request to delivery
        FocusChangeLatency,     ///< Time for focus change processing
        MemoryUsage,           ///< Memory used by accessibility metadata
        InterfaceCreationTime, ///< Time to create accessible interfaces
        LiveRegionUpdateTime,  ///< Time to update live regions
        WidgetEnhancementTime  ///< Time to enhance widgets with accessibility
    };

    /**
     * @brief Performance alert levels
     */
    enum AlertLevel {
        Normal,     ///< Performance within acceptable limits
        Warning,    ///< Performance degraded but functional
        Critical    ///< Performance severely impacted
    };

    /**
     * @brief Performance statistics structure
     */
    struct PerformanceStats {
        double averageLatency;
        double maxLatency;
        double minLatency;
        int sampleCount;
        qint64 totalMemoryUsage;
        double memoryGrowthRate;
        QDateTime lastUpdated;
    };

    explicit AccessibilityPerformanceMonitor(QObject *parent = nullptr);
    ~AccessibilityPerformanceMonitor();

    /**
     * @brief Initialize performance monitoring
     */
    void initialize();

    /**
     * @brief Start monitoring a specific metric type
     * @param type The metric type to monitor
     * @param enabled Whether to enable monitoring for this metric
     */
    void setMonitoringEnabled(MetricType type, bool enabled);

    /**
     * @brief Check if monitoring is enabled for a metric type
     * @param type The metric type to check
     * @return True if monitoring is enabled
     */
    bool isMonitoringEnabled(MetricType type) const;

    /**
     * @brief Start measuring an operation
     * @param type The type of operation being measured
     * @param operationId Unique identifier for this operation
     */
    void startMeasurement(MetricType type, const QString& operationId = QString());

    /**
     * @brief End measuring an operation
     * @param type The type of operation being measured
     * @param operationId Unique identifier for this operation
     */
    void endMeasurement(MetricType type, const QString& operationId = QString());

    /**
     * @brief Record a memory usage measurement
     * @param bytes Number of bytes used
     * @param component Component name for tracking
     */
    void recordMemoryUsage(qint64 bytes, const QString& component = QString());

    /**
     * @brief Get performance statistics for a metric type
     * @param type The metric type to get stats for
     * @return Performance statistics
     */
    PerformanceStats getStats(MetricType type) const;

    /**
     * @brief Get current alert level for overall performance
     * @return Current alert level
     */
    AlertLevel getCurrentAlertLevel() const;

    /**
     * @brief Get performance recommendations
     * @return List of performance improvement recommendations
     */
    QStringList getPerformanceRecommendations() const;

    /**
     * @brief Reset all performance statistics
     */
    void resetStats();

    /**
     * @brief Enable or disable automatic optimization
     * @param enabled Whether to enable automatic optimization
     */
    void setAutoOptimizationEnabled(bool enabled);

    /**
     * @brief Check if automatic optimization is enabled
     * @return True if auto-optimization is enabled
     */
    bool isAutoOptimizationEnabled() const;

public slots:
    /**
     * @brief Optimize accessibility performance based on current metrics
     */
    void optimizePerformance();

    /**
     * @brief Handle announcement latency measurement
     * @param latencyMs Latency in milliseconds
     */
    void onAnnouncementLatency(double latencyMs);

    /**
     * @brief Handle focus change latency measurement
     * @param latencyMs Latency in milliseconds
     */
    void onFocusChangeLatency(double latencyMs);

    /**
     * @brief Handle memory usage update
     * @param bytes Current memory usage in bytes
     */
    void onMemoryUsageUpdate(qint64 bytes);

signals:
    /**
     * @brief Emitted when performance issue is detected
     * @param level Alert level
     * @param message Description of the issue
     * @param metricType The metric type that triggered the alert
     */
    void performanceIssueDetected(AlertLevel level, const QString& message, MetricType metricType);

    /**
     * @brief Emitted when performance statistics are updated
     * @param type The metric type that was updated
     * @param stats Updated statistics
     */
    void statsUpdated(MetricType type, const PerformanceStats& stats);

    /**
     * @brief Emitted when optimization is performed
     * @param optimizations List of optimizations applied
     */
    void optimizationPerformed(const QStringList& optimizations);

private slots:
    void performPeriodicCheck();
    void cleanupOldMeasurements();

private:
    /**
     * @brief Structure for tracking individual measurements
     */
    struct Measurement {
        QElapsedTimer timer;
        QString operationId;
        QDateTime startTime;
        bool active;
    };

    /**
     * @brief Structure for storing metric data
     */
    struct MetricData {
        QQueue<double> samples;
        PerformanceStats stats;
        bool enabled;
        QHash<QString, Measurement> activeMeasurements;
    };

    // Helper methods
    void updateStats(MetricType type, double value);
    void checkPerformanceThresholds(MetricType type, double value);
    AlertLevel calculateAlertLevel(MetricType type, const PerformanceStats& stats) const;
    void applyOptimizations(const QStringList& optimizations);
    double getThreshold(MetricType type, const QString& thresholdType) const;
    void initializeThresholds();
    void recordSample(MetricType type, double value);
    void pruneOldSamples(MetricType type);

    // Member variables
    QHash<MetricType, MetricData> m_metrics;
    QHash<QString, qint64> m_memoryUsageByComponent;
    QTimer* m_periodicTimer;
    QTimer* m_cleanupTimer;
    mutable QMutex m_dataMutex;
    
    bool m_autoOptimizationEnabled;
    AlertLevel m_currentAlertLevel;
    QDateTime m_lastOptimization;
    
    // Performance thresholds
    QHash<MetricType, QHash<QString, double>> m_thresholds;
    
    // Configuration
    static const int MAX_SAMPLES = 1000;
    static const int PERIODIC_CHECK_INTERVAL = 5000; // 5 seconds
    static const int CLEANUP_INTERVAL = 60000; // 1 minute
    static const int SAMPLE_RETENTION_HOURS = 24;
};

Q_DECLARE_METATYPE(AccessibilityPerformanceMonitor::MetricType)
Q_DECLARE_METATYPE(AccessibilityPerformanceMonitor::AlertLevel)
Q_DECLARE_METATYPE(AccessibilityPerformanceMonitor::PerformanceStats)

#endif // ACCESSIBILITYPERFORMANCEMONITOR_H