#include "MusicCache.h"
#include "../repositories/MusicRepository.h"
#include "ErrorHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDataStream>
#include <QBuffer>
#include <algorithm>

MusicCache::MusicCache(QObject* parent)
    : QObject(parent)
    , m_maxMemoryUsage(DEFAULT_MAX_MEMORY)
    , m_defaultExpirationTime(DEFAULT_EXPIRATION_TIME)
    , m_invalidationStrategy(SmartInvalidation)
    , m_warmupStrategy(PopularItemsWarmup)
    , m_maintenanceTimer(new QTimer(this))
    , m_persistenceTimer(new QTimer(this))
    , m_autoPersistenceEnabled(false)
{
    // Initialize statistics
    m_statistics = CacheStatistics{};
    m_statistics.maxMemoryUsage = m_maxMemoryUsage;
    
    // Setup maintenance timer
    m_maintenanceTimer->setSingleShot(false);
    m_maintenanceTimer->setInterval(MAINTENANCE_INTERVAL_MS);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &MusicCache::performMaintenance);
    
    // Setup persistence timer
    m_persistenceTimer->setSingleShot(false);
    m_persistenceTimer->setInterval(PERSISTENCE_INTERVAL_MS);
    connect(m_persistenceTimer, &QTimer::timeout, this, &MusicCache::autoPersist);
}

MusicCache::~MusicCache()
{
    shutdown();
}

bool MusicCache::initialize()
{
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Initializing music cache");
    
    // Start maintenance timer
    m_maintenanceTimer->start();
    
    // Load cache from persistent storage if enabled
    if (m_autoPersistenceEnabled && !m_persistenceFilePath.isEmpty()) {
        if (QFile::exists(m_persistenceFilePath)) {
            loadFromFile(m_persistenceFilePath);
        }
        m_persistenceTimer->start();
    }
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Music cache initialized (max memory: %1 MB)")
                 .arg(m_maxMemoryUsage / (1024 * 1024)));
    
    return true;
}

void MusicCache::shutdown()
{
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Shutting down music cache");
    
    // Stop timers
    m_maintenanceTimer->stop();
    m_persistenceTimer->stop();
    
    // Save cache if auto-persistence is enabled
    if (m_autoPersistenceEnabled && !m_persistenceFilePath.isEmpty()) {
        saveToFile(m_persistenceFilePath);
    }
    
    // Clear cache
    QMutexLocker locker(&m_cacheMutex);
    m_cache.clear();
    
    // Reset statistics
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics = CacheStatistics{};
    m_statistics.maxMemoryUsage = m_maxMemoryUsage;
}

void MusicCache::setMaxMemoryUsage(qint64 maxBytes)
{
    QMutexLocker locker(&m_statsMutex);
    m_maxMemoryUsage = maxBytes;
    m_statistics.maxMemoryUsage = maxBytes;
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Max memory usage set to %1 MB").arg(maxBytes / (1024 * 1024)));
    
    // Trigger cleanup if current usage exceeds new limit
    if (m_statistics.currentMemoryUsage > maxBytes) {
        locker.unlock();
        cleanup(static_cast<qint64>(maxBytes * MEMORY_CLEANUP_TARGET));
    }
}

qint64 MusicCache::maxMemoryUsage() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_maxMemoryUsage;
}

qint64 MusicCache::currentMemoryUsage() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_statistics.currentMemoryUsage;
}

void MusicCache::setDefaultExpirationTime(int seconds)
{
    m_defaultExpirationTime = seconds;
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Default expiration time set to %1 seconds").arg(seconds));
}

int MusicCache::defaultExpirationTime() const
{
    return m_defaultExpirationTime;
}

void MusicCache::setInvalidationStrategy(InvalidationStrategy strategy)
{
    m_invalidationStrategy = strategy;
    
    QString strategyName;
    switch (strategy) {
        case TimeBasedExpiration: strategyName = "TimeBasedExpiration"; break;
        case AccessBasedExpiration: strategyName = "AccessBasedExpiration"; break;
        case VersionBasedExpiration: strategyName = "VersionBasedExpiration"; break;
        case ManualInvalidation: strategyName = "ManualInvalidation"; break;
        case SmartInvalidation: strategyName = "SmartInvalidation"; break;
    }
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Invalidation strategy set to %1").arg(strategyName));
}

MusicCache::InvalidationStrategy MusicCache::invalidationStrategy() const
{
    return m_invalidationStrategy;
}

void MusicCache::setWarmupStrategy(WarmupStrategy strategy)
{
    m_warmupStrategy = strategy;
    
    QString strategyName;
    switch (strategy) {
        case NoWarmup: strategyName = "NoWarmup"; break;
        case PopularItemsWarmup: strategyName = "PopularItemsWarmup"; break;
        case RecentItemsWarmup: strategyName = "RecentItemsWarmup"; break;
        case PredictiveWarmup: strategyName = "PredictiveWarmup"; break;
        case FullWarmup: strategyName = "FullWarmup"; break;
    }
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Warmup strategy set to %1").arg(strategyName));
}

MusicCache::WarmupStrategy MusicCache::warmupStrategy() const
{
    return m_warmupStrategy;
}

void MusicCache::put(int musicId, const MusicItem& music, const QString& category, int expirationSeconds)
{
    QString key = generateMusicKey(musicId, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    // Create cache entry
    CacheEntry entry;
    entry.data = QVariant::fromValue(music);
    entry.createdAt = QDateTime::currentDateTime();
    entry.lastAccessed = entry.createdAt;
    entry.accessCount = 0;
    entry.source = "manual";
    entry.size = calculateDataSize(entry.data);
    
    // Set expiration
    if (expirationSeconds > 0) {
        entry.expiresAt = entry.createdAt.addSecs(expirationSeconds);
    } else if (m_defaultExpirationTime > 0) {
        entry.expiresAt = entry.createdAt.addSecs(m_defaultExpirationTime);
    }
    
    // Add metadata
    entry.metadata["musicId"] = musicId;
    entry.metadata["category"] = category;
    
    // Check if we need to evict entries to make room
    qint64 newMemoryUsage = m_statistics.currentMemoryUsage + entry.size;
    if (newMemoryUsage > m_maxMemoryUsage) {
        locker.unlock();
        cleanup(static_cast<qint64>(m_maxMemoryUsage * MEMORY_CLEANUP_TARGET));
        locker.relock();
    }
    
    // Store entry
    bool isUpdate = m_cache.contains(key);
    if (isUpdate) {
        // Update existing entry size in statistics
        qint64 oldSize = m_cache[key].size;
        m_statistics.currentMemoryUsage -= oldSize;
    }
    
    m_cache[key] = entry;
    m_statistics.currentMemoryUsage += entry.size;
    m_statistics.currentEntries = m_cache.size();
    
    // Update category statistics
    m_statistics.categorySize[category] += entry.size;
    
    logCacheOperation("put", key, true, QString("Music ID: %1, Size: %2 bytes").arg(musicId).arg(entry.size));
}

std::unique_ptr<MusicItem> MusicCache::get(int musicId, const QString& category)
{
    QString key = generateMusicKey(musicId, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        // Cache miss
        m_statistics.totalMisses++;
        m_statistics.categoryMisses[category]++;
        
        locker.unlock();
        emit cacheMiss(key, category);
        logCacheOperation("get", key, false, "Cache miss");
        return nullptr;
    }
    
    CacheEntry& entry = m_cache[key];
    
    // Check if entry has expired
    if (isExpired(entry)) {
        m_cache.remove(key);
        m_statistics.currentMemoryUsage -= entry.size;
        m_statistics.currentEntries = m_cache.size();
        m_statistics.totalMisses++;
        m_statistics.categoryMisses[category]++;
        
        locker.unlock();
        emit cacheMiss(key, category);
        logCacheOperation("get", key, false, "Entry expired");
        return nullptr;
    }
    
    // Update access information
    entry.lastAccessed = QDateTime::currentDateTime();
    entry.accessCount++;
    
    // Cache hit
    m_statistics.totalHits++;
    m_statistics.categoryHits[category]++;
    
    // Calculate hit ratio
    qint64 totalAccesses = m_statistics.totalHits + m_statistics.totalMisses;
    if (totalAccesses > 0) {
        m_statistics.hitRatio = static_cast<double>(m_statistics.totalHits) / totalAccesses;
    }
    
    auto result = std::make_unique<MusicItem>(entry.data.value<MusicItem>());
    
    locker.unlock();
    emit cacheHit(key, category);
    logCacheOperation("get", key, true, QString("Access count: %1").arg(entry.accessCount));
    
    return result;
}

bool MusicCache::contains(int musicId, const QString& category)
{
    QString key = generateMusicKey(musicId, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    // Check if entry has expired
    const CacheEntry& entry = m_cache[key];
    if (isExpired(entry)) {
        // Remove expired entry
        m_cache.remove(key);
        m_statistics.currentMemoryUsage -= entry.size;
        m_statistics.currentEntries = m_cache.size();
        return false;
    }
    
    return true;
}

bool MusicCache::remove(int musicId, const QString& category)
{
    QString key = generateMusicKey(musicId, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    const CacheEntry& entry = m_cache[key];
    m_statistics.currentMemoryUsage -= entry.size;
    m_statistics.categorySize[category] -= entry.size;
    
    m_cache.remove(key);
    m_statistics.currentEntries = m_cache.size();
    
    logCacheOperation("remove", key, true, QString("Music ID: %1").arg(musicId));
    return true;
}

void MusicCache::putSearchResults(const QString& searchKey, const QList<MusicItem>& results, int expirationSeconds)
{
    QString key = generateSearchKey(searchKey);
    
    QMutexLocker locker(&m_cacheMutex);
    
    // Create cache entry
    CacheEntry entry;
    entry.data = QVariant::fromValue(results);
    entry.createdAt = QDateTime::currentDateTime();
    entry.lastAccessed = entry.createdAt;
    entry.accessCount = 0;
    entry.source = "search";
    entry.size = calculateDataSize(entry.data);
    
    // Set expiration (search results typically expire faster)
    int searchExpirationTime = expirationSeconds > 0 ? expirationSeconds : (m_defaultExpirationTime / 2);
    if (searchExpirationTime > 0) {
        entry.expiresAt = entry.createdAt.addSecs(searchExpirationTime);
    }
    
    // Add metadata
    entry.metadata["searchKey"] = searchKey;
    entry.metadata["resultCount"] = results.size();
    entry.metadata["category"] = "search";
    
    // Check memory usage
    qint64 newMemoryUsage = m_statistics.currentMemoryUsage + entry.size;
    if (newMemoryUsage > m_maxMemoryUsage) {
        locker.unlock();
        cleanup(static_cast<qint64>(m_maxMemoryUsage * MEMORY_CLEANUP_TARGET));
        locker.relock();
    }
    
    // Store entry
    bool isUpdate = m_cache.contains(key);
    if (isUpdate) {
        qint64 oldSize = m_cache[key].size;
        m_statistics.currentMemoryUsage -= oldSize;
    }
    
    m_cache[key] = entry;
    m_statistics.currentMemoryUsage += entry.size;
    m_statistics.currentEntries = m_cache.size();
    m_statistics.categorySize["search"] += entry.size;
    
    logCacheOperation("putSearchResults", key, true, 
                     QString("Search: %1, Results: %2, Size: %3 bytes")
                     .arg(searchKey).arg(results.size()).arg(entry.size));
}

QList<MusicItem> MusicCache::getSearchResults(const QString& searchKey)
{
    QString key = generateSearchKey(searchKey);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        m_statistics.totalMisses++;
        m_statistics.categoryMisses["search"]++;
        
        locker.unlock();
        emit cacheMiss(key, "search");
        logCacheOperation("getSearchResults", key, false, "Cache miss");
        return QList<MusicItem>();
    }
    
    CacheEntry& entry = m_cache[key];
    
    // Check if entry has expired
    if (isExpired(entry)) {
        m_cache.remove(key);
        m_statistics.currentMemoryUsage -= entry.size;
        m_statistics.currentEntries = m_cache.size();
        m_statistics.totalMisses++;
        m_statistics.categoryMisses["search"]++;
        
        locker.unlock();
        emit cacheMiss(key, "search");
        logCacheOperation("getSearchResults", key, false, "Entry expired");
        return QList<MusicItem>();
    }
    
    // Update access information
    entry.lastAccessed = QDateTime::currentDateTime();
    entry.accessCount++;
    
    // Cache hit
    m_statistics.totalHits++;
    m_statistics.categoryHits["search"]++;
    
    // Calculate hit ratio
    qint64 totalAccesses = m_statistics.totalHits + m_statistics.totalMisses;
    if (totalAccesses > 0) {
        m_statistics.hitRatio = static_cast<double>(m_statistics.totalHits) / totalAccesses;
    }
    
    QList<MusicItem> results = entry.data.value<QList<MusicItem>>();
    
    locker.unlock();
    emit cacheHit(key, "search");
    logCacheOperation("getSearchResults", key, true, 
                     QString("Search: %1, Results: %2").arg(searchKey).arg(results.size()));
    
    return results;
}

bool MusicCache::containsSearchResults(const QString& searchKey)
{
    QString key = generateSearchKey(searchKey);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    const CacheEntry& entry = m_cache[key];
    if (isExpired(entry)) {
        m_cache.remove(key);
        m_statistics.currentMemoryUsage -= entry.size;
        m_statistics.currentEntries = m_cache.size();
        return false;
    }
    
    return true;
}

bool MusicCache::removeSearchResults(const QString& searchKey)
{
    QString key = generateSearchKey(searchKey);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    const CacheEntry& entry = m_cache[key];
    m_statistics.currentMemoryUsage -= entry.size;
    m_statistics.categorySize["search"] -= entry.size;
    
    m_cache.remove(key);
    m_statistics.currentEntries = m_cache.size();
    
    logCacheOperation("removeSearchResults", key, true, QString("Search: %1").arg(searchKey));
    return true;
}

void MusicCache::putMetadata(const QString& key, const QVariant& data, const QString& category, int expirationSeconds)
{
    QString cacheKey = generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    // Create cache entry
    CacheEntry entry;
    entry.data = data;
    entry.createdAt = QDateTime::currentDateTime();
    entry.lastAccessed = entry.createdAt;
    entry.accessCount = 0;
    entry.source = "metadata";
    entry.size = calculateDataSize(data);
    
    // Set expiration
    if (expirationSeconds > 0) {
        entry.expiresAt = entry.createdAt.addSecs(expirationSeconds);
    } else if (m_defaultExpirationTime > 0) {
        entry.expiresAt = entry.createdAt.addSecs(m_defaultExpirationTime);
    }
    
    // Add metadata
    entry.metadata["originalKey"] = key;
    entry.metadata["category"] = category;
    
    // Check memory usage
    qint64 newMemoryUsage = m_statistics.currentMemoryUsage + entry.size;
    if (newMemoryUsage > m_maxMemoryUsage) {
        locker.unlock();
        cleanup(static_cast<qint64>(m_maxMemoryUsage * MEMORY_CLEANUP_TARGET));
        locker.relock();
    }
    
    // Store entry
    bool isUpdate = m_cache.contains(cacheKey);
    if (isUpdate) {
        qint64 oldSize = m_cache[cacheKey].size;
        m_statistics.currentMemoryUsage -= oldSize;
    }
    
    m_cache[cacheKey] = entry;
    m_statistics.currentMemoryUsage += entry.size;
    m_statistics.currentEntries = m_cache.size();
    m_statistics.categorySize[category] += entry.size;
    
    logCacheOperation("putMetadata", cacheKey, true, 
                     QString("Key: %1, Category: %2, Size: %3 bytes")
                     .arg(key).arg(category).arg(entry.size));
}

QVariant MusicCache::getMetadata(const QString& key, const QString& category)
{
    QString cacheKey = generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(cacheKey)) {
        m_statistics.totalMisses++;
        m_statistics.categoryMisses[category]++;
        
        locker.unlock();
        emit cacheMiss(cacheKey, category);
        logCacheOperation("getMetadata", cacheKey, false, "Cache miss");
        return QVariant();
    }
    
    CacheEntry& entry = m_cache[cacheKey];
    
    // Check if entry has expired
    if (isExpired(entry)) {
        m_cache.remove(cacheKey);
        m_statistics.currentMemoryUsage -= entry.size;
        m_statistics.currentEntries = m_cache.size();
        m_statistics.totalMisses++;
        m_statistics.categoryMisses[category]++;
        
        locker.unlock();
        emit cacheMiss(cacheKey, category);
        logCacheOperation("getMetadata", cacheKey, false, "Entry expired");
        return QVariant();
    }
    
    // Update access information
    entry.lastAccessed = QDateTime::currentDateTime();
    entry.accessCount++;
    
    // Cache hit
    m_statistics.totalHits++;
    m_statistics.categoryHits[category]++;
    
    // Calculate hit ratio
    qint64 totalAccesses = m_statistics.totalHits + m_statistics.totalMisses;
    if (totalAccesses > 0) {
        m_statistics.hitRatio = static_cast<double>(m_statistics.totalHits) / totalAccesses;
    }
    
    QVariant result = entry.data;
    
    locker.unlock();
    emit cacheHit(cacheKey, category);
    logCacheOperation("getMetadata", cacheKey, true, QString("Key: %1, Category: %2").arg(key).arg(category));
    
    return result;
}

bool MusicCache::containsMetadata(const QString& key, const QString& category)
{
    QString cacheKey = generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(cacheKey)) {
        return false;
    }
    
    const CacheEntry& entry = m_cache[cacheKey];
    if (isExpired(entry)) {
        m_cache.remove(cacheKey);
        m_statistics.currentMemoryUsage -= entry.size;
        m_statistics.currentEntries = m_cache.size();
        return false;
    }
    
    return true;
}

bool MusicCache::removeMetadata(const QString& key, const QString& category)
{
    QString cacheKey = generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(cacheKey)) {
        return false;
    }
    
    const CacheEntry& entry = m_cache[cacheKey];
    m_statistics.currentMemoryUsage -= entry.size;
    m_statistics.categorySize[category] -= entry.size;
    
    m_cache.remove(cacheKey);
    m_statistics.currentEntries = m_cache.size();
    
    logCacheOperation("removeMetadata", cacheKey, true, QString("Key: %1, Category: %2").arg(key).arg(category));
    return true;
}

void MusicCache::clear()
{
    QMutexLocker locker(&m_cacheMutex);
    
    int entriesRemoved = m_cache.size();
    qint64 memoryFreed = m_statistics.currentMemoryUsage;
    
    m_cache.clear();
    
    // Reset statistics
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.currentEntries = 0;
    m_statistics.currentMemoryUsage = 0;
    m_statistics.categorySize.clear();
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Cache cleared: %1 entries, %2 MB freed")
                 .arg(entriesRemoved).arg(memoryFreed / (1024 * 1024)));
    
    emit cleanupCompleted(entriesRemoved, memoryFreed);
}

void MusicCache::clearCategory(const QString& category)
{
    QMutexLocker locker(&m_cacheMutex);
    
    int entriesRemoved = 0;
    qint64 memoryFreed = 0;
    
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        const CacheEntry& entry = it.value();
        QString entryCategory = entry.metadata.value("category", "unknown").toString();
        
        if (entryCategory == category) {
            memoryFreed += entry.size;
            entriesRemoved++;
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update statistics
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.currentEntries = m_cache.size();
    m_statistics.currentMemoryUsage -= memoryFreed;
    m_statistics.categorySize[category] = 0;
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Category '%1' cleared: %2 entries, %3 MB freed")
                 .arg(category).arg(entriesRemoved).arg(memoryFreed / (1024 * 1024)));
    
    emit cleanupCompleted(entriesRemoved, memoryFreed);
}

void MusicCache::invalidateExpired()
{
    QMutexLocker locker(&m_cacheMutex);
    
    int entriesRemoved = 0;
    qint64 memoryFreed = 0;
    
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        const CacheEntry& entry = it.value();
        
        if (isExpired(entry)) {
            QString category = entry.metadata.value("category", "unknown").toString();
            memoryFreed += entry.size;
            entriesRemoved++;
            
            emit entryEvicted(it.key(), category, "expired");
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update statistics
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.currentEntries = m_cache.size();
    m_statistics.currentMemoryUsage -= memoryFreed;
    m_statistics.totalEvictions += entriesRemoved;
    
    if (entriesRemoved > 0) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Expired entries invalidated: %1 entries, %2 MB freed")
                     .arg(entriesRemoved).arg(memoryFreed / (1024 * 1024)));
    }
}

void MusicCache::cleanup(qint64 targetMemoryUsage)
{
    QMutexLocker locker(&m_cacheMutex);
    
    if (targetMemoryUsage < 0) {
        targetMemoryUsage = static_cast<qint64>(m_maxMemoryUsage * MEMORY_CLEANUP_TARGET);
    }
    
    if (m_statistics.currentMemoryUsage <= targetMemoryUsage) {
        return; // No cleanup needed
    }
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Starting cleanup: current %1 MB, target %2 MB")
                 .arg(m_statistics.currentMemoryUsage / (1024 * 1024))
                 .arg(targetMemoryUsage / (1024 * 1024)));
    
    int entriesRemoved = 0;
    qint64 memoryFreed = 0;
    
    // First, remove expired entries
    auto it = m_cache.begin();
    while (it != m_cache.end() && m_statistics.currentMemoryUsage > targetMemoryUsage) {
        const CacheEntry& entry = it.value();
        
        if (isExpired(entry)) {
            QString category = entry.metadata.value("category", "unknown").toString();
            memoryFreed += entry.size;
            m_statistics.currentMemoryUsage -= entry.size;
            entriesRemoved++;
            
            emit entryEvicted(it.key(), category, "expired");
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
    
    // If still over target, use LRU eviction
    if (m_statistics.currentMemoryUsage > targetMemoryUsage) {
        int lruEvicted = evictLRU(targetMemoryUsage);
        entriesRemoved += lruEvicted;
    }
    
    // Update statistics
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.currentEntries = m_cache.size();
    m_statistics.totalEvictions += entriesRemoved;
    m_statistics.lastCleanup = QDateTime::currentDateTime();
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Cleanup completed: %1 entries removed, %2 MB freed, current usage: %3 MB")
                 .arg(entriesRemoved)
                 .arg(memoryFreed / (1024 * 1024))
                 .arg(m_statistics.currentMemoryUsage / (1024 * 1024)));
    
    emit cleanupCompleted(entriesRemoved, memoryFreed);
    
    // Check if we're still over the memory threshold
    if (m_statistics.currentMemoryUsage > m_maxMemoryUsage * MEMORY_CLEANUP_THRESHOLD) {
        emit memoryThresholdExceeded(m_statistics.currentMemoryUsage, m_maxMemoryUsage);
    }
}

void MusicCache::warmupCache()
{
    if (m_warmupStrategy == NoWarmup) {
        return;
    }
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Starting cache warmup");
    
    int entriesWarmed = 0;
    
    // This is a simplified warmup implementation
    // In a real application, you would query the database for popular/recent items
    // and pre-load them into the cache
    
    switch (m_warmupStrategy) {
        case PopularItemsWarmup:
            // Would query database for most played items
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Popular items warmup not implemented - requires database access");
            break;
            
        case RecentItemsWarmup:
            // Would query database for recently accessed items
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Recent items warmup not implemented - requires database access");
            break;
            
        case PredictiveWarmup:
            // Would use machine learning or heuristics to predict likely accessed items
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Predictive warmup not implemented - requires ML model");
            break;
            
        case FullWarmup:
            // Would load entire dataset (use with caution)
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Full warmup not implemented - requires database access");
            break;
            
        case NoWarmup:
        default:
            break;
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.lastWarmup = QDateTime::currentDateTime();
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Cache warmup completed: %1 entries warmed").arg(entriesWarmed));
    emit warmupCompleted(entriesWarmed);
}

void MusicCache::pinEntry(const QString& key, const QString& category)
{
    QString cacheKey = (category == "music") ? generateMusicKey(key.toInt(), category) : 
                       (category == "search") ? generateSearchKey(key) :
                       generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (m_cache.contains(cacheKey)) {
        m_cache[cacheKey].isPinned = true;
        logCacheOperation("pinEntry", cacheKey, true, QString("Category: %1").arg(category));
    }
}

void MusicCache::unpinEntry(const QString& key, const QString& category)
{
    QString cacheKey = (category == "music") ? generateMusicKey(key.toInt(), category) : 
                       (category == "search") ? generateSearchKey(key) :
                       generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (m_cache.contains(cacheKey)) {
        m_cache[cacheKey].isPinned = false;
        logCacheOperation("unpinEntry", cacheKey, true, QString("Category: %1").arg(category));
    }
}

MusicCache::CacheStatistics MusicCache::getStatistics() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_statistics;
}

std::unique_ptr<MusicCache::CacheEntry> MusicCache::getEntryInfo(const QString& key, const QString& category)
{
    QString cacheKey = (category == "music") ? generateMusicKey(key.toInt(), category) : 
                       (category == "search") ? generateSearchKey(key) :
                       generateMetadataKey(key, category);
    
    QMutexLocker locker(&m_cacheMutex);
    
    if (!m_cache.contains(cacheKey)) {
        return nullptr;
    }
    
    return std::make_unique<CacheEntry>(m_cache[cacheKey]);
}

QStringList MusicCache::getKeys(const QString& category)
{
    QMutexLocker locker(&m_cacheMutex);
    
    QStringList keys;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheEntry& entry = it.value();
        QString entryCategory = entry.metadata.value("category", "unknown").toString();
        
        if (category.isEmpty() || entryCategory == category) {
            keys.append(it.key());
        }
    }
    
    return keys;
}

QString MusicCache::exportStatistics()
{
    QMutexLocker locker(&m_statsMutex);
    
    QJsonObject root;
    
    // Basic statistics
    root["totalHits"] = static_cast<qint64>(m_statistics.totalHits);
    root["totalMisses"] = static_cast<qint64>(m_statistics.totalMisses);
    root["totalEvictions"] = static_cast<qint64>(m_statistics.totalEvictions);
    root["currentEntries"] = static_cast<qint64>(m_statistics.currentEntries);
    root["currentMemoryUsage"] = static_cast<qint64>(m_statistics.currentMemoryUsage);
    root["maxMemoryUsage"] = static_cast<qint64>(m_statistics.maxMemoryUsage);
    root["hitRatio"] = m_statistics.hitRatio;
    
    if (m_statistics.lastCleanup.isValid()) {
        root["lastCleanup"] = m_statistics.lastCleanup.toString(Qt::ISODate);
    }
    if (m_statistics.lastWarmup.isValid()) {
        root["lastWarmup"] = m_statistics.lastWarmup.toString(Qt::ISODate);
    }
    
    // Category statistics
    QJsonObject categoryHits;
    for (auto it = m_statistics.categoryHits.begin(); it != m_statistics.categoryHits.end(); ++it) {
        categoryHits[it.key()] = static_cast<qint64>(it.value());
    }
    root["categoryHits"] = categoryHits;
    
    QJsonObject categoryMisses;
    for (auto it = m_statistics.categoryMisses.begin(); it != m_statistics.categoryMisses.end(); ++it) {
        categoryMisses[it.key()] = static_cast<qint64>(it.value());
    }
    root["categoryMisses"] = categoryMisses;
    
    QJsonObject categorySize;
    for (auto it = m_statistics.categorySize.begin(); it != m_statistics.categorySize.end(); ++it) {
        categorySize[it.key()] = static_cast<qint64>(it.value());
    }
    root["categorySize"] = categorySize;
    
    QJsonDocument doc(root);
    return doc.toJson();
}

bool MusicCache::saveToFile(const QString& filePath)
{
    QMutexLocker locker(&m_cacheMutex);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Error, "MusicCache", QString("Failed to open cache file for writing: %1").arg(filePath));
        return false;
    }
    
    QJsonObject root;
    QJsonArray entries;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheEntry& entry = it.value();
        
        QJsonObject entryObj;
        entryObj["key"] = it.key();
        entryObj["createdAt"] = entry.createdAt.toString(Qt::ISODate);
        entryObj["lastAccessed"] = entry.lastAccessed.toString(Qt::ISODate);
        if (entry.expiresAt.isValid()) {
            entryObj["expiresAt"] = entry.expiresAt.toString(Qt::ISODate);
        }
        entryObj["accessCount"] = entry.accessCount;
        entryObj["size"] = static_cast<qint64>(entry.size);
        entryObj["source"] = entry.source;
        entryObj["isPinned"] = entry.isPinned;
        
        // Serialize data (simplified - in practice you'd need proper serialization)
        QJsonObject dataObj;
        dataObj["type"] = entry.data.typeName();
        // Note: Full data serialization would require custom serialization for MusicItem
        
        entryObj["data"] = dataObj;
        
        // Metadata
        QJsonObject metadataObj;
        for (auto metaIt = entry.metadata.begin(); metaIt != entry.metadata.end(); ++metaIt) {
            metadataObj[metaIt.key()] = QJsonValue::fromVariant(metaIt.value());
        }
        entryObj["metadata"] = metadataObj;
        
        entries.append(entryObj);
    }
    
    root["entries"] = entries;
    root["version"] = "1.0";
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(root);
    file.write(doc.toJson());
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Cache saved to file: %1 (%2 entries)")
                 .arg(filePath).arg(m_cache.size()));
    
    return true;
}

bool MusicCache::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "MusicCache", QString("Failed to open cache file for reading: %1").arg(filePath));
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Error, "MusicCache", QString("Failed to parse cache file: %1").arg(error.errorString()));
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray entries = root["entries"].toArray();
    
    QMutexLocker locker(&m_cacheMutex);
    
    int loadedEntries = 0;
    
    for (const QJsonValue& value : entries) {
        QJsonObject entryObj = value.toObject();
        
        QString key = entryObj["key"].toString();
        
        CacheEntry entry;
        entry.createdAt = QDateTime::fromString(entryObj["createdAt"].toString(), Qt::ISODate);
        entry.lastAccessed = QDateTime::fromString(entryObj["lastAccessed"].toString(), Qt::ISODate);
        
        if (entryObj.contains("expiresAt")) {
            entry.expiresAt = QDateTime::fromString(entryObj["expiresAt"].toString(), Qt::ISODate);
        }
        
        entry.accessCount = entryObj["accessCount"].toInt();
        entry.size = entryObj["size"].toVariant().toLongLong();
        entry.source = entryObj["source"].toString();
        entry.isPinned = entryObj["isPinned"].toBool();
        
        // Load metadata
        QJsonObject metadataObj = entryObj["metadata"].toObject();
        for (auto it = metadataObj.begin(); it != metadataObj.end(); ++it) {
            entry.metadata[it.key()] = it.value().toVariant();
        }
        
        // Skip expired entries
        if (isExpired(entry)) {
            continue;
        }
        
        // Note: Data loading is simplified here
        // In practice, you'd need proper deserialization for MusicItem and other types
        
        m_cache[key] = entry;
        m_statistics.currentMemoryUsage += entry.size;
        loadedEntries++;
    }
    
    m_statistics.currentEntries = m_cache.size();
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Cache loaded from file: %1 (%2 entries loaded)")
                 .arg(filePath).arg(loadedEntries));
    
    return true;
}

void MusicCache::setAutoPersistence(bool enabled, const QString& filePath)
{
    m_autoPersistenceEnabled = enabled;
    
    if (enabled) {
        if (filePath.isEmpty()) {
            // Use default cache directory
            QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
            QDir().mkpath(cacheDir);
            m_persistenceFilePath = QDir(cacheDir).filePath("music_cache.json");
        } else {
            m_persistenceFilePath = filePath;
        }
        
        m_persistenceTimer->start();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Auto-persistence enabled: %1").arg(m_persistenceFilePath));
    } else {
        m_persistenceTimer->stop();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", "Auto-persistence disabled");
    }
}

void MusicCache::performMaintenance()
{
    // Invalidate expired entries
    invalidateExpired();
    
    // Check memory usage and cleanup if necessary
    if (m_statistics.currentMemoryUsage > m_maxMemoryUsage * MEMORY_CLEANUP_THRESHOLD) {
        cleanup();
    }
    
    // Update statistics
    updateStatistics();
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", QString("Maintenance completed: %1 entries, %2 MB used")
                  .arg(m_statistics.currentEntries)
                  .arg(m_statistics.currentMemoryUsage / (1024 * 1024)));
}

void MusicCache::autoPersist()
{
    if (m_autoPersistenceEnabled && !m_persistenceFilePath.isEmpty()) {
        saveToFile(m_persistenceFilePath);
    }
}

QString MusicCache::generateMusicKey(int musicId, const QString& category)
{
    return QString("music:%1:%2").arg(category).arg(musicId);
}

QString MusicCache::generateSearchKey(const QString& searchKey)
{
    // Create a hash of the search key to handle long/complex search strings
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(searchKey.toUtf8());
    return QString("search:%1").arg(QString(hash.result().toHex()));
}

QString MusicCache::generateMetadataKey(const QString& key, const QString& category)
{
    return QString("meta:%1:%2").arg(category).arg(key);
}

qint64 MusicCache::calculateDataSize(const QVariant& data)
{
    // Simplified size calculation
    // In practice, you'd need more accurate size calculation for complex types
    
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream stream(&buffer);
    stream << data;
    
    return buffer.size();
}

bool MusicCache::isExpired(const CacheEntry& entry)
{
    if (!entry.expiresAt.isValid()) {
        return false; // No expiration set
    }
    
    QDateTime now = QDateTime::currentDateTime();
    
    switch (m_invalidationStrategy) {
        case TimeBasedExpiration:
            return now > entry.expiresAt;
            
        case AccessBasedExpiration:
            // Expire if not accessed for expiration time
            return entry.lastAccessed.secsTo(now) > m_defaultExpirationTime;
            
        case VersionBasedExpiration:
            // Would need version tracking - simplified here
            return now > entry.expiresAt;
            
        case ManualInvalidation:
            return false; // Never expire automatically
            
        case SmartInvalidation:
            // Combination of time-based and access-based
            return (now > entry.expiresAt) || 
                   (entry.lastAccessed.secsTo(now) > m_defaultExpirationTime * 2);
    }
    
    return false;
}

int MusicCache::evictLRU(qint64 targetSize)
{
    // Create list of entries sorted by last access time (oldest first)
    QList<QPair<QString, QDateTime>> entries;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheEntry& entry = it.value();
        
        // Don't evict pinned entries
        if (entry.isPinned) {
            continue;
        }
        
        entries.append(qMakePair(it.key(), entry.lastAccessed));
    }
    
    // Sort by last accessed time (oldest first)
    std::sort(entries.begin(), entries.end(), 
              [](const QPair<QString, QDateTime>& a, const QPair<QString, QDateTime>& b) {
                  return a.second < b.second;
              });
    
    int evicted = 0;
    
    // Evict oldest entries until we reach target size
    for (const auto& pair : entries) {
        if (m_statistics.currentMemoryUsage <= targetSize) {
            break;
        }
        
        const QString& key = pair.first;
        if (m_cache.contains(key)) {
            const CacheEntry& entry = m_cache[key];
            QString category = entry.metadata.value("category", "unknown").toString();
            
            m_statistics.currentMemoryUsage -= entry.size;
            m_cache.remove(key);
            evicted++;
            
            emit entryEvicted(key, category, "LRU");
        }
    }
    
    return evicted;
}

void MusicCache::updateStatistics()
{
    QMutexLocker locker(&m_statsMutex);
    
    // Update current entries count
    m_statistics.currentEntries = m_cache.size();
    
    // Emit statistics update
    emit statisticsUpdated(m_statistics);
}

void MusicCache::logCacheOperation(const QString& operation, const QString& key, bool success, const QString& details)
{
    QString message = QString("Operation: %1, Key: %2, Success: %3")
                     .arg(operation)
                     .arg(key.left(50)) // Truncate long keys
                     .arg(success ? "true" : "false");
    
    if (!details.isEmpty()) {
        message += QString(", Details: %1").arg(details);
    }
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "MusicCache", message);
}

// Register MusicItem with Qt's meta-type system for QVariant storage
Q_DECLARE_METATYPE(MusicItem)
Q_DECLARE_METATYPE(QList<MusicItem>)