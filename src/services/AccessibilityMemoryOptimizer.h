#ifndef ACCESSIBILITYMEMORYOPTIMIZER_H
#define ACCESSIBILITYMEMORYOPTIMIZER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QWeakPointer>
#include <QMutex>
#include <QCache>
#include <QDateTime>

class QWidget;
class QAccessibleInterface;

/**
 * @brief Memory optimization system for accessibility metadata
 * 
 * Manages memory usage of accessibility-related data including
 * cached interfaces, metadata, and temporary objects.
 */
class AccessibilityMemoryOptimizer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Memory optimization strategies
     */
    enum OptimizationStrategy {
        Conservative,   ///< Minimal optimization, preserve most data
        Balanced,      ///< Balance between performance and memory usage
        Aggressive     ///< Maximum memory savings, may impact performance
    };

    explicit AccessibilityMemoryOptimizer(QObject *parent = nullptr);
    ~AccessibilityMemoryOptimizer();

    /**
     * @brief Initialize the memory optimizer
     */
    void initialize();

    /**
     * @brief Set optimization strategy
     * @param strategy The optimization strategy to use
     */
    void setOptimizationStrategy(OptimizationStrategy strategy);

    /**
     * @brief Get current optimization strategy
     * @return Current optimization strategy
     */
    OptimizationStrategy getOptimizationStrategy() const;

    /**
     * @brief Register widget for memory tracking
     * @param widget Widget to track
     * @param metadataSize Estimated size of accessibility metadata
     */
    void registerWidget(QWidget* widget, qint64 metadataSize = 0);

    /**
     * @brief Unregister widget from memory tracking
     * @param widget Widget to unregister
     */
    void unregisterWidget(QWidget* widget);

    /**
     * @brief Cache accessibility interface
     * @param widget Widget associated with interface
     * @param interface Accessible interface to cache
     */
    void cacheInterface(QWidget* widget, QAccessibleInterface* interface);

    /**
     * @brief Get cached accessibility interface
     * @param widget Widget to get interface for
     * @return Cached interface or nullptr if not found
     */
    QAccessibleInterface* getCachedInterface(QWidget* widget);

    /**
     * @brief Get current memory usage
     * @return Current memory usage in bytes
     */
    qint64 getCurrentMemoryUsage() const;

    /**
     * @brief Get memory usage by category
     * @return Hash of category names to memory usage
     */
    QHash<QString, qint64> getMemoryUsageByCategory() const;

    /**
     * @brief Perform memory cleanup
     * @param force Whether to force cleanup regardless of strategy
     */
    void performCleanup(bool force = false);

    /**
     * @brief Set memory usage limit
     * @param bytes Maximum memory usage in bytes
     */
    void setMemoryLimit(qint64 bytes);

    /**
     * @brief Get memory usage limit
     * @return Memory usage limit in bytes
     */
    qint64 getMemoryLimit() const;

public slots:
    /**
     * @brief Optimize memory usage based on current strategy
     */
    void optimizeMemory();

    /**
     * @brief Handle low memory warning
     */
    void onLowMemoryWarning();

    /**
     * @brief Clear all cached data
     */
    void clearCache();

signals:
    /**
     * @brief Emitted when memory usage changes significantly
     * @param usage Current memory usage in bytes
     */
    void memoryUsageChanged(qint64 usage);

    /**
     * @brief Emitted when memory limit is exceeded
     * @param usage Current usage
     * @param limit Memory limit
     */
    void memoryLimitExceeded(qint64 usage, qint64 limit);

    /**
     * @brief Emitted when cleanup is performed
     * @param freedBytes Number of bytes freed
     */
    void cleanupPerformed(qint64 freedBytes);

private slots:
    void performPeriodicCleanup();
    void checkMemoryUsage();

private:
    /**
     * @brief Structure for tracking widget memory usage
     */
    struct WidgetMemoryInfo {
        QWeakPointer<QWidget> widget;
        qint64 metadataSize;
        QDateTime lastAccessed;
        int accessCount;
    };

    // Helper methods
    void updateMemoryUsage();
    void cleanupUnusedInterfaces();
    void cleanupStaleWidgets();
    qint64 calculateInterfaceSize(QAccessibleInterface* interface) const;
    bool shouldCleanupWidget(const WidgetMemoryInfo& info) const;
    void applyOptimizationStrategy();

    // Member variables
    OptimizationStrategy m_strategy;
    QHash<QWidget*, WidgetMemoryInfo> m_trackedWidgets;
    QHash<QWidget*, QAccessibleInterface*> m_interfaceCache;
    QTimer* m_cleanupTimer;
    QTimer* m_memoryCheckTimer;
    mutable QMutex m_dataMutex;
    
    qint64 m_currentMemoryUsage;
    qint64 m_memoryLimit;
    qint64 m_lastCleanupUsage;
    
    // Configuration based on strategy
    int m_cleanupInterval;
    int m_cacheSize;
    int m_accessThreshold;
    int m_staleThreshold; // Hours
};

#endif // ACCESSIBILITYMEMORYOPTIMIZER_H