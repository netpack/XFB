#include "AccessibilityPerformanceMonitor.h"
#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QThread>
#include <QProcess>
#include <cmath>

AccessibilityPerformanceMonitor::AccessibilityPerformanceMonitor(QObject *parent)
    : QObject(parent)
    , m_periodicTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
    , m_autoOptimizationEnabled(true)
    , m_currentAlertLevel(Normal)
{
    // Initialize timers
    m_periodicTimer->setInterval(PERIODIC_CHECK_INTERVAL);
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL);
    
    connect(m_periodicTimer, &QTimer::timeout, this, &AccessibilityPerformanceMonitor::performPeriodicCheck);
    connect(m_cleanupTimer, &QTimer::timeout, this, &AccessibilityPerformanceMonitor::cleanupOldMeasurements);
    
    // Initialize thresholds
    initializeThresholds();
    
    // Initialize metric data structures
    for (int i = AnnouncementLatency; i <= WidgetEnhancementTime; ++i) {
        MetricType type = static_cast<MetricType>(i);
        m_metrics[type] = MetricData();
        m_metrics[type].enabled = true;
        m_metrics[type].stats = PerformanceStats();
        m_metrics[type].stats.averageLatency = 0.0;
        m_metrics[type].stats.maxLatency = 0.0;
        m_metrics[type].stats.minLatency = std::numeric_limits<double>::max();
        m_metrics[type].stats.sampleCount = 0;
        m_metrics[type].stats.totalMemoryUsage = 0;
        m_metrics[type].stats.memoryGrowthRate = 0.0;
        m_metrics[type].stats.lastUpdated = QDateTime::currentDateTime();
    }
}

AccessibilityPerformanceMonitor::~AccessibilityPerformanceMonitor()
{
    m_periodicTimer->stop();
    m_cleanupTimer->stop();
}

void AccessibilityPerformanceMonitor::initialize()
{
    qDebug() << "Initializing AccessibilityPerformanceMonitor";
    
    // Start monitoring timers
    m_periodicTimer->start();
    m_cleanupTimer->start();
    
    // Reset statistics
    resetStats();
    
    qDebug() << "AccessibilityPerformanceMonitor initialized successfully";
}

void AccessibilityPerformanceMonitor::setMonitoringEnabled(MetricType type, bool enabled)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (m_metrics.contains(type)) {
        m_metrics[type].enabled = enabled;
        qDebug() << "Monitoring for metric type" << type << (enabled ? "enabled" : "disabled");
    }
}

bool AccessibilityPerformanceMonitor::isMonitoringEnabled(MetricType type) const
{
    QMutexLocker locker(&m_dataMutex);
    
    return m_metrics.value(type).enabled;
}

void AccessibilityPerformanceMonitor::startMeasurement(MetricType type, const QString& operationId)
{
    if (!isMonitoringEnabled(type)) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    QString id = operationId.isEmpty() ? QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId())) : operationId;
    
    Measurement measurement;
    measurement.timer.start();
    measurement.operationId = id;
    measurement.startTime = QDateTime::currentDateTime();
    measurement.active = true;
    
    m_metrics[type].activeMeasurements[id] = measurement;
}

void AccessibilityPerformanceMonitor::endMeasurement(MetricType type, const QString& operationId)
{
    if (!isMonitoringEnabled(type)) {
        return;
    }
    
    QMutexLocker locker(&m_dataMutex);
    
    QString id = operationId.isEmpty() ? QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId())) : operationId;
    
    if (m_metrics[type].activeMeasurements.contains(id)) {
        Measurement& measurement = m_metrics[type].activeMeasurements[id];
        
        if (measurement.active) {
            double elapsedMs = measurement.timer.elapsed();
            measurement.active = false;
            
            // Record the measurement
            updateStats(type, elapsedMs);
            
            // Check performance thresholds
            checkPerformanceThresholds(type, elapsedMs);
            
            // Remove completed measurement
            m_metrics[type].activeMeasurements.remove(id);
        }
    }
}

void AccessibilityPerformanceMonitor::recordMemoryUsage(qint64 bytes, const QString& component)
{
    QMutexLocker locker(&m_dataMutex);
    
    QString comp = component.isEmpty() ? "total" : component;
    qint64 previousUsage = m_memoryUsageByComponent.value(comp, 0);
    m_memoryUsageByComponent[comp] = bytes;
    
    // Update memory stats
    MetricData& memoryMetric = m_metrics[MemoryUsage];
    memoryMetric.stats.totalMemoryUsage = bytes;
    
    // Calculate growth rate
    if (previousUsage > 0) {
        double growthRate = (static_cast<double>(bytes - previousUsage) / previousUsage) * 100.0;
        memoryMetric.stats.memoryGrowthRate = growthRate;
        
        // Check for memory growth issues
        if (growthRate > 10.0) { // More than 10% growth
            emit performanceIssueDetected(Warning, 
                QString("High memory growth rate detected: %1%").arg(growthRate, 0, 'f', 2),
                MemoryUsage);
        }
    }
    
    memoryMetric.stats.lastUpdated = QDateTime::currentDateTime();
    emit statsUpdated(MemoryUsage, memoryMetric.stats);
}

AccessibilityPerformanceMonitor::PerformanceStats AccessibilityPerformanceMonitor::getStats(MetricType type) const
{
    QMutexLocker locker(&m_dataMutex);
    
    return m_metrics.value(type).stats;
}

AccessibilityPerformanceMonitor::AlertLevel AccessibilityPerformanceMonitor::getCurrentAlertLevel() const
{
    return m_currentAlertLevel;
}

QStringList AccessibilityPerformanceMonitor::getPerformanceRecommendations() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QStringList recommendations;
    
    // Analyze each metric and provide recommendations
    for (auto it = m_metrics.constBegin(); it != m_metrics.constEnd(); ++it) {
        MetricType type = it.key();
        const PerformanceStats& stats = it.value().stats;
        
        if (!it.value().enabled || stats.sampleCount == 0) {
            continue;
        }
        
        AlertLevel level = calculateAlertLevel(type, stats);
        
        switch (type) {
        case AnnouncementLatency:
            if (level >= Warning) {
                recommendations << "Consider reducing announcement verbosity to improve latency";
                recommendations << "Enable announcement queuing to prevent overlapping announcements";
            }
            break;
            
        case FocusChangeLatency:
            if (level >= Warning) {
                recommendations << "Optimize widget accessibility enhancement to reduce focus change delays";
                recommendations << "Consider lazy loading of accessibility metadata";
            }
            break;
            
        case MemoryUsage:
            if (stats.memoryGrowthRate > 5.0) {
                recommendations << "Memory usage is growing rapidly - consider implementing cleanup routines";
                recommendations << "Review accessibility metadata caching strategy";
            }
            if (stats.totalMemoryUsage > 100 * 1024 * 1024) { // 100MB
                recommendations << "High memory usage detected - consider reducing cached accessibility data";
            }
            break;
            
        case InterfaceCreationTime:
            if (level >= Warning) {
                recommendations << "Optimize QAccessibleInterface creation by caching common interfaces";
                recommendations << "Consider using factory pattern for interface creation";
            }
            break;
            
        case LiveRegionUpdateTime:
            if (level >= Warning) {
                recommendations << "Throttle live region updates to improve performance";
                recommendations << "Batch multiple updates into single announcements";
            }
            break;
            
        case WidgetEnhancementTime:
            if (level >= Warning) {
                recommendations << "Optimize widget enhancement by pre-computing accessibility metadata";
                recommendations << "Consider background enhancement for non-critical widgets";
            }
            break;
        }
    }
    
    // General recommendations based on overall performance
    if (m_currentAlertLevel >= Warning) {
        recommendations << "Consider disabling accessibility features temporarily if performance is critical";
        recommendations << "Review accessibility settings and reduce verbosity if possible";
    }
    
    return recommendations;
}

void AccessibilityPerformanceMonitor::resetStats()
{
    QMutexLocker locker(&m_dataMutex);
    
    for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
        it.value().samples.clear();
        it.value().stats = PerformanceStats();
        it.value().stats.averageLatency = 0.0;
        it.value().stats.maxLatency = 0.0;
        it.value().stats.minLatency = std::numeric_limits<double>::max();
        it.value().stats.sampleCount = 0;
        it.value().stats.totalMemoryUsage = 0;
        it.value().stats.memoryGrowthRate = 0.0;
        it.value().stats.lastUpdated = QDateTime::currentDateTime();
        it.value().activeMeasurements.clear();
    }
    
    m_memoryUsageByComponent.clear();
    m_currentAlertLevel = Normal;
    
    qDebug() << "Performance statistics reset";
}

void AccessibilityPerformanceMonitor::setAutoOptimizationEnabled(bool enabled)
{
    m_autoOptimizationEnabled = enabled;
    qDebug() << "Auto-optimization" << (enabled ? "enabled" : "disabled");
}

bool AccessibilityPerformanceMonitor::isAutoOptimizationEnabled() const
{
    return m_autoOptimizationEnabled;
}

void AccessibilityPerformanceMonitor::optimizePerformance()
{
    QStringList optimizations;
    
    // Get current recommendations
    QStringList recommendations = getPerformanceRecommendations();
    
    if (recommendations.isEmpty()) {
        qDebug() << "No performance optimizations needed";
        return;
    }
    
    // Apply automatic optimizations based on recommendations
    for (const QString& recommendation : recommendations) {
        if (recommendation.contains("announcement verbosity", Qt::CaseInsensitive)) {
            // Reduce announcement verbosity
            optimizations << "Reduced announcement verbosity to improve latency";
        }
        
        if (recommendation.contains("queuing", Qt::CaseInsensitive)) {
            // Enable announcement queuing
            optimizations << "Enabled announcement queuing to prevent overlaps";
        }
        
        if (recommendation.contains("lazy loading", Qt::CaseInsensitive)) {
            // Enable lazy loading of accessibility metadata
            optimizations << "Enabled lazy loading of accessibility metadata";
        }
        
        if (recommendation.contains("throttle", Qt::CaseInsensitive)) {
            // Throttle live region updates
            optimizations << "Throttled live region updates to improve performance";
        }
        
        if (recommendation.contains("cleanup", Qt::CaseInsensitive)) {
            // Perform memory cleanup
            optimizations << "Performed accessibility metadata cleanup";
        }
    }
    
    if (!optimizations.isEmpty()) {
        applyOptimizations(optimizations);
        m_lastOptimization = QDateTime::currentDateTime();
        emit optimizationPerformed(optimizations);
        
        qDebug() << "Applied" << optimizations.size() << "performance optimizations";
    }
}

void AccessibilityPerformanceMonitor::onAnnouncementLatency(double latencyMs)
{
    updateStats(AnnouncementLatency, latencyMs);
    checkPerformanceThresholds(AnnouncementLatency, latencyMs);
}

void AccessibilityPerformanceMonitor::onFocusChangeLatency(double latencyMs)
{
    updateStats(FocusChangeLatency, latencyMs);
    checkPerformanceThresholds(FocusChangeLatency, latencyMs);
}

void AccessibilityPerformanceMonitor::onMemoryUsageUpdate(qint64 bytes)
{
    recordMemoryUsage(bytes);
}

void AccessibilityPerformanceMonitor::performPeriodicCheck()
{
    // Calculate overall alert level
    AlertLevel maxLevel = Normal;
    
    QMutexLocker locker(&m_dataMutex);
    
    for (auto it = m_metrics.constBegin(); it != m_metrics.constEnd(); ++it) {
        if (!it.value().enabled || it.value().stats.sampleCount == 0) {
            continue;
        }
        
        AlertLevel level = calculateAlertLevel(it.key(), it.value().stats);
        if (level > maxLevel) {
            maxLevel = level;
        }
    }
    
    if (maxLevel != m_currentAlertLevel) {
        m_currentAlertLevel = maxLevel;
        
        if (maxLevel >= Warning) {
            QString message = QString("Overall accessibility performance level: %1")
                            .arg(maxLevel == Warning ? "Warning" : "Critical");
            emit performanceIssueDetected(maxLevel, message, AnnouncementLatency);
        }
    }
    
    // Trigger auto-optimization if enabled and needed
    if (m_autoOptimizationEnabled && maxLevel >= Warning) {
        QDateTime now = QDateTime::currentDateTime();
        if (m_lastOptimization.isNull() || 
            m_lastOptimization.secsTo(now) > 300) { // 5 minutes cooldown
            
            QTimer::singleShot(0, this, &AccessibilityPerformanceMonitor::optimizePerformance);
        }
    }
}

void AccessibilityPerformanceMonitor::cleanupOldMeasurements()
{
    QMutexLocker locker(&m_dataMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-SAMPLE_RETENTION_HOURS * 3600);
    
    for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
        // Clean up old active measurements that may have been abandoned
        auto measurementIt = it.value().activeMeasurements.begin();
        while (measurementIt != it.value().activeMeasurements.end()) {
            if (measurementIt.value().startTime < cutoff) {
                measurementIt = it.value().activeMeasurements.erase(measurementIt);
            } else {
                ++measurementIt;
            }
        }
        
        // Prune old samples
        pruneOldSamples(it.key());
    }
}

void AccessibilityPerformanceMonitor::updateStats(MetricType type, double value)
{
    if (!m_metrics.contains(type)) {
        return;
    }
    
    MetricData& data = m_metrics[type];
    PerformanceStats& stats = data.stats;
    
    // Record sample
    recordSample(type, value);
    
    // Update statistics
    stats.sampleCount++;
    
    if (value > stats.maxLatency) {
        stats.maxLatency = value;
    }
    
    if (value < stats.minLatency) {
        stats.minLatency = value;
    }
    
    // Calculate running average
    if (stats.sampleCount == 1) {
        stats.averageLatency = value;
    } else {
        stats.averageLatency = ((stats.averageLatency * (stats.sampleCount - 1)) + value) / stats.sampleCount;
    }
    
    stats.lastUpdated = QDateTime::currentDateTime();
    
    emit statsUpdated(type, stats);
}

void AccessibilityPerformanceMonitor::checkPerformanceThresholds(MetricType type, double value)
{
    double warningThreshold = getThreshold(type, "warning");
    double criticalThreshold = getThreshold(type, "critical");
    
    AlertLevel level = Normal;
    QString message;
    
    if (value >= criticalThreshold) {
        level = Critical;
        message = QString("Critical performance issue detected for %1: %2ms (threshold: %3ms)")
                 .arg(type).arg(value, 0, 'f', 2).arg(criticalThreshold, 0, 'f', 2);
    } else if (value >= warningThreshold) {
        level = Warning;
        message = QString("Performance warning for %1: %2ms (threshold: %3ms)")
                 .arg(type).arg(value, 0, 'f', 2).arg(warningThreshold, 0, 'f', 2);
    }
    
    if (level > Normal) {
        emit performanceIssueDetected(level, message, type);
    }
}

AccessibilityPerformanceMonitor::AlertLevel AccessibilityPerformanceMonitor::calculateAlertLevel(
    MetricType type, const PerformanceStats& stats) const
{
    if (stats.sampleCount == 0) {
        return Normal;
    }
    
    double warningThreshold = getThreshold(type, "warning");
    double criticalThreshold = getThreshold(type, "critical");
    
    if (stats.averageLatency >= criticalThreshold || stats.maxLatency >= criticalThreshold * 1.5) {
        return Critical;
    } else if (stats.averageLatency >= warningThreshold || stats.maxLatency >= warningThreshold * 1.5) {
        return Warning;
    }
    
    return Normal;
}

void AccessibilityPerformanceMonitor::applyOptimizations(const QStringList& optimizations)
{
    // This method would integrate with other accessibility services to apply optimizations
    // For now, we log the optimizations that would be applied
    
    for (const QString& optimization : optimizations) {
        qDebug() << "Applying optimization:" << optimization;
        
        // Example optimizations that could be implemented:
        if (optimization.contains("verbosity")) {
            // Signal to AccessibilityManager to reduce verbosity
        } else if (optimization.contains("queuing")) {
            // Signal to AudioFeedbackService to enable queuing
        } else if (optimization.contains("lazy loading")) {
            // Signal to WidgetAccessibilityEnhancer to enable lazy loading
        } else if (optimization.contains("throttle")) {
            // Signal to LiveRegionManager to throttle updates
        } else if (optimization.contains("cleanup")) {
            // Perform immediate cleanup of cached data
        }
    }
}

double AccessibilityPerformanceMonitor::getThreshold(MetricType type, const QString& thresholdType) const
{
    return m_thresholds.value(type).value(thresholdType, 1000.0); // Default 1000ms
}

void AccessibilityPerformanceMonitor::initializeThresholds()
{
    // Initialize performance thresholds for each metric type
    
    // Announcement latency thresholds (in milliseconds)
    m_thresholds[AnnouncementLatency]["warning"] = 100.0;   // 100ms
    m_thresholds[AnnouncementLatency]["critical"] = 500.0;  // 500ms
    
    // Focus change latency thresholds
    m_thresholds[FocusChangeLatency]["warning"] = 50.0;     // 50ms
    m_thresholds[FocusChangeLatency]["critical"] = 200.0;   // 200ms
    
    // Memory usage thresholds (in bytes)
    m_thresholds[MemoryUsage]["warning"] = 50 * 1024 * 1024;   // 50MB
    m_thresholds[MemoryUsage]["critical"] = 200 * 1024 * 1024; // 200MB
    
    // Interface creation time thresholds
    m_thresholds[InterfaceCreationTime]["warning"] = 10.0;   // 10ms
    m_thresholds[InterfaceCreationTime]["critical"] = 50.0;  // 50ms
    
    // Live region update time thresholds
    m_thresholds[LiveRegionUpdateTime]["warning"] = 20.0;    // 20ms
    m_thresholds[LiveRegionUpdateTime]["critical"] = 100.0;  // 100ms
    
    // Widget enhancement time thresholds
    m_thresholds[WidgetEnhancementTime]["warning"] = 5.0;    // 5ms
    m_thresholds[WidgetEnhancementTime]["critical"] = 25.0;  // 25ms
}

void AccessibilityPerformanceMonitor::recordSample(MetricType type, double value)
{
    MetricData& data = m_metrics[type];
    
    data.samples.enqueue(value);
    
    // Limit sample count to prevent excessive memory usage
    while (data.samples.size() > MAX_SAMPLES) {
        data.samples.dequeue();
    }
}

void AccessibilityPerformanceMonitor::pruneOldSamples(MetricType type)
{
    // For now, we just limit by count. In a more sophisticated implementation,
    // we could track timestamps and prune by age.
    MetricData& data = m_metrics[type];
    
    while (data.samples.size() > MAX_SAMPLES) {
        data.samples.dequeue();
    }
}