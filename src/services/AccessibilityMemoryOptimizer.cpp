#include "AccessibilityMemoryOptimizer.h"
#include <QWidget>
#include <QAccessibleInterface>
#include <QAccessible>
#include <QApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QDateTime>

AccessibilityMemoryOptimizer::AccessibilityMemoryOptimizer(QObject *parent)
    : QObject(parent)
    , m_strategy(Balanced)
    , m_interfaceCache(100) // Default cache size
    , m_cleanupTimer(new QTimer(this))
    , m_memoryCheckTimer(new QTimer(this))
    , m_currentMemoryUsage(0)
    , m_memoryLimit(50 * 1024 * 1024) // 50MB default limit
    , m_lastCleanupUsage(0)
    , m_cleanupInterval(30000) // 30 seconds
    , m_cacheSize(100)
    , m_accessThreshold(5)
    , m_staleThreshold(24) // 24 hours
{
    connect(m_cleanupTimer, &QTimer::timeout, this, &AccessibilityMemoryOptimizer::performPeriodicCleanup);
    connect(m_memoryCheckTimer, &QTimer::timeout, this, &AccessibilityMemoryOptimizer::checkMemoryUsage);
    
    // Set up cache cost function
    m_interfaceCache.setMaxCost(m_cacheSize);
}

AccessibilityMemoryOptimizer::~AccessibilityMemoryOptimizer()
{
    clearCache();
}

void AccessibilityMemoryOptimizer::initialize()
{
    qDebug() << "Initializing AccessibilityMemoryOptimizer";
    
    // Apply initial optimization strategy
    applyOptimizationStrategy();
    
    // Start timers
    m_cleanupTimer->start(m_cleanupInterval);
    m_memoryCheckTimer->start(5000); // Check every 5 seconds
    
    qDebug() << "AccessibilityMemoryOptimizer initialized with strategy:" << m_strategy;
}

void AccessibilityMemoryOptimizer::setOptimizationStrategy(OptimizationStrategy strategy)
{
    if (m_strategy != strategy) {
        m_strategy = strategy;
        applyOptimizationStrategy();
        
        qDebug() << "Optimization strategy changed to:" << strategy;
    }
}

AccessibilityMemoryOptimizer::OptimizationStrategy AccessibilityMemoryOptimizer::getOptimizationStrategy() const
{
    return m_strategy;
}

void AccessibilityMemoryOptimizer::registerWidget(QWidget* widget, qint64 metadataSize)
{
    if (!widget) return;
    
    QMutexLocker locker(&m_dataMutex);
    
    WidgetMemoryInfo info;
    info.widget = widget;
    info.metadataSize = metadataSize;
    info.lastAccessed = QDateTime::currentDateTime();
    info.accessCount = 1;
    
    m_trackedWidgets[widget] = info;
    m_currentMemoryUsage += metadataSize;
    
    updateMemoryUsage();
}

void AccessibilityMemoryOptimizer::unregisterWidget(QWidget* widget)
{
    if (!widget) return;
    
    QMutexLocker locker(&m_dataMutex);
    
    if (m_trackedWidgets.contains(widget)) {
        qint64 size = m_trackedWidgets[widget].metadataSize;
        m_trackedWidgets.remove(widget);
        m_currentMemoryUsage -= size;
        
        // Remove from cache as well
        m_interfaceCache.remove(widget);
        
        updateMemoryUsage();
    }
}

void AccessibilityMemoryOptimizer::cacheInterface(QWidget* widget, QAccessibleInterface* interface)
{
    if (!widget || !interface) return;
    
    QMutexLocker locker(&m_dataMutex);
    
    // Calculate interface size
    qint64 interfaceSize = calculateInterfaceSize(interface);
    
    // Update widget access info
    if (m_trackedWidgets.contains(widget)) {
        WidgetMemoryInfo& info = m_trackedWidgets[widget];
        info.lastAccessed = QDateTime::currentDateTime();
        info.accessCount++;
    }
    
    // Cache the interface
    m_interfaceCache.insert(widget, interface);
    
    m_currentMemoryUsage += interfaceSize;
    updateMemoryUsage();
}

QAccessibleInterface* AccessibilityMemoryOptimizer::getCachedInterface(QWidget* widget)
{
    if (!widget) return nullptr;
    
    QMutexLocker locker(&m_dataMutex);
    
    QAccessibleInterface* interface = m_interfaceCache.value(widget);
    
    if (interface && m_trackedWidgets.contains(widget)) {
        // Update access info
        WidgetMemoryInfo& info = m_trackedWidgets[widget];
        info.lastAccessed = QDateTime::currentDateTime();
        info.accessCount++;
    }
    
    return interface;
}

qint64 AccessibilityMemoryOptimizer::getCurrentMemoryUsage() const
{
    return m_currentMemoryUsage;
}

QHash<QString, qint64> AccessibilityMemoryOptimizer::getMemoryUsageByCategory() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QHash<QString, qint64> usage;
    
    qint64 widgetMetadata = 0;
    qint64 cachedInterfaces = 0;
    
    for (const WidgetMemoryInfo& info : m_trackedWidgets) {
        widgetMetadata += info.metadataSize;
    }
    
    // Estimate cached interface memory
    cachedInterfaces = m_interfaceCache.totalCost() * 1024; // Convert from KB to bytes
    
    usage["Widget Metadata"] = widgetMetadata;
    usage["Cached Interfaces"] = cachedInterfaces;
    usage["Other"] = m_currentMemoryUsage - widgetMetadata - cachedInterfaces;
    
    return usage;
}

void AccessibilityMemoryOptimizer::performCleanup(bool force)
{
    QMutexLocker locker(&m_dataMutex);
    
    qint64 initialUsage = m_currentMemoryUsage;
    
    // Clean up unused interfaces
    cleanupUnusedInterfaces();
    
    // Clean up stale widgets
    cleanupStaleWidgets();
    
    // Force cleanup if memory limit exceeded or forced
    if (force || m_currentMemoryUsage > m_memoryLimit) {
        // More aggressive cleanup
        m_interfaceCache.clear();
        
        // Remove widgets that haven't been accessed recently
        auto it = m_trackedWidgets.begin();
        while (it != m_trackedWidgets.end()) {
            const WidgetMemoryInfo& info = it.value();
            QDateTime cutoff = QDateTime::currentDateTime().addSecs(-3600); // 1 hour
            
            if (info.lastAccessed < cutoff || info.widget.isNull()) {
                m_currentMemoryUsage -= info.metadataSize;
                it = m_trackedWidgets.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    qint64 freedBytes = initialUsage - m_currentMemoryUsage;
    if (freedBytes > 0) {
        emit cleanupPerformed(freedBytes);
        qDebug() << "Accessibility memory cleanup freed" << freedBytes << "bytes";
    }
    
    updateMemoryUsage();
}

void AccessibilityMemoryOptimizer::setMemoryLimit(qint64 bytes)
{
    m_memoryLimit = bytes;
    
    // Trigger cleanup if current usage exceeds new limit
    if (m_currentMemoryUsage > m_memoryLimit) {
        QTimer::singleShot(0, this, [this]() { performCleanup(true); });
    }
}

qint64 AccessibilityMemoryOptimizer::getMemoryLimit() const
{
    return m_memoryLimit;
}

void AccessibilityMemoryOptimizer::optimizeMemory()
{
    qDebug() << "Optimizing accessibility memory usage";
    
    switch (m_strategy) {
    case Conservative:
        // Only clean up obviously stale data
        cleanupStaleWidgets();
        break;
        
    case Balanced:
        // Standard cleanup
        performCleanup(false);
        break;
        
    case Aggressive:
        // Force full cleanup
        performCleanup(true);
        break;
    }
}

void AccessibilityMemoryOptimizer::onLowMemoryWarning()
{
    qDebug() << "Low memory warning received, performing aggressive cleanup";
    
    // Temporarily switch to aggressive strategy
    OptimizationStrategy originalStrategy = m_strategy;
    setOptimizationStrategy(Aggressive);
    
    performCleanup(true);
    
    // Restore original strategy
    setOptimizationStrategy(originalStrategy);
}

void AccessibilityMemoryOptimizer::clearCache()
{
    QMutexLocker locker(&m_dataMutex);
    
    qint64 initialUsage = m_currentMemoryUsage;
    
    m_interfaceCache.clear();
    m_trackedWidgets.clear();
    m_currentMemoryUsage = 0;
    
    qint64 freedBytes = initialUsage;
    if (freedBytes > 0) {
        emit cleanupPerformed(freedBytes);
    }
    
    updateMemoryUsage();
}

void AccessibilityMemoryOptimizer::performPeriodicCleanup()
{
    // Only perform cleanup if memory usage has grown significantly
    qint64 growthThreshold = m_lastCleanupUsage * 1.2; // 20% growth
    
    if (m_currentMemoryUsage > growthThreshold || m_currentMemoryUsage > m_memoryLimit * 0.8) {
        optimizeMemory();
        m_lastCleanupUsage = m_currentMemoryUsage;
    }
}

void AccessibilityMemoryOptimizer::checkMemoryUsage()
{
    if (m_currentMemoryUsage > m_memoryLimit) {
        emit memoryLimitExceeded(m_currentMemoryUsage, m_memoryLimit);
        
        // Trigger cleanup
        QTimer::singleShot(0, this, [this]() { performCleanup(true); });
    }
}

void AccessibilityMemoryOptimizer::updateMemoryUsage()
{
    emit memoryUsageChanged(m_currentMemoryUsage);
}

void AccessibilityMemoryOptimizer::cleanupUnusedInterfaces()
{
    // Remove interfaces for widgets that no longer exist
    QList<QWidget*> keysToRemove;
    
    for (auto it = m_trackedWidgets.begin(); it != m_trackedWidgets.end(); ++it) {
        if (it.value().widget.isNull()) {
            keysToRemove.append(it.key());
        }
    }
    
    for (QWidget* key : keysToRemove) {
        qint64 size = m_trackedWidgets[key].metadataSize;
        m_trackedWidgets.remove(key);
        m_interfaceCache.remove(key);
        m_currentMemoryUsage -= size;
    }
}

void AccessibilityMemoryOptimizer::cleanupStaleWidgets()
{
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-m_staleThreshold * 3600);
    
    auto it = m_trackedWidgets.begin();
    while (it != m_trackedWidgets.end()) {
        const WidgetMemoryInfo& info = it.value();
        
        if (shouldCleanupWidget(info)) {
            m_currentMemoryUsage -= info.metadataSize;
            m_interfaceCache.remove(it.key());
            it = m_trackedWidgets.erase(it);
        } else {
            ++it;
        }
    }
}

qint64 AccessibilityMemoryOptimizer::calculateInterfaceSize(QAccessibleInterface* interface) const
{
    if (!interface) return 0;
    
    // Estimate interface size based on text content and complexity
    qint64 size = sizeof(QAccessibleInterface);
    
    // Add estimated size for text content
    QString name = interface->text(QAccessible::Name);
    QString description = interface->text(QAccessible::Description);
    QString value = interface->text(QAccessible::Value);
    
    size += name.length() * sizeof(QChar);
    size += description.length() * sizeof(QChar);
    size += value.length() * sizeof(QChar);
    
    // Add estimated size for child interfaces
    int childCount = interface->childCount();
    size += childCount * sizeof(void*); // Rough estimate for child pointers
    
    return size;
}

bool AccessibilityMemoryOptimizer::shouldCleanupWidget(const WidgetMemoryInfo& info) const
{
    // Widget no longer exists
    if (info.widget.isNull()) {
        return true;
    }
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-m_staleThreshold * 3600);
    
    switch (m_strategy) {
    case Conservative:
        // Only clean up very old, rarely accessed widgets
        return info.lastAccessed < cutoff && info.accessCount < 2;
        
    case Balanced:
        // Clean up widgets not accessed recently or with low access count
        return info.lastAccessed < cutoff || 
               (info.accessCount < m_accessThreshold && 
                info.lastAccessed < QDateTime::currentDateTime().addSecs(-3600));
        
    case Aggressive:
        // Clean up any widget not accessed in the last hour
        return info.lastAccessed < QDateTime::currentDateTime().addSecs(-3600);
    }
    
    return false;
}

void AccessibilityMemoryOptimizer::applyOptimizationStrategy()
{
    switch (m_strategy) {
    case Conservative:
        m_cleanupInterval = 60000; // 1 minute
        m_cacheSize = 200;
        m_accessThreshold = 2;
        m_staleThreshold = 48; // 48 hours
        break;
        
    case Balanced:
        m_cleanupInterval = 30000; // 30 seconds
        m_cacheSize = 100;
        m_accessThreshold = 5;
        m_staleThreshold = 24; // 24 hours
        break;
        
    case Aggressive:
        m_cleanupInterval = 15000; // 15 seconds
        m_cacheSize = 50;
        m_accessThreshold = 10;
        m_staleThreshold = 12; // 12 hours
        break;
    }
    
    // Update cache size
    m_interfaceCache.setMaxCost(m_cacheSize);
    
    // Update timer interval
    if (m_cleanupTimer->isActive()) {
        m_cleanupTimer->setInterval(m_cleanupInterval);
    }
}