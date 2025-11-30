#ifndef MUSICCACHE_H
#define MUSICCACHE_H

#include <QObject>
#include <QCache>
#include <QHash>
#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <memory>

// Forward declarations
struct MusicItem;

/**
 * @brief Intelligent caching system for music metadata
 * 
 * The MusicCache provides high-performance caching for frequently accessed
 * music metadata with intelligent cache invalidation strategies, memory
 * usage monitoring, and automatic cleanup. It significantly improves
 * application performance by reducing database queries for commonly
 * accessed music data.
 * 
 * Features:
 * - LRU (Least Recently Used) cache eviction
 * - Intelligent cache invalidation strategies
 * - Memory usage monitoring and limits
 * - Cache hit/miss statistics
 * - Automatic cache warming for popular items
 * - Thread-safe operations
 * - Cache persistence across application restarts
 * - Smart prefetching based on access patterns
 * 
 * @example
 * @code
 * MusicCache* cache = new MusicCache(this);
 * cache->setMaxMemoryUsage(50 * 1024 * 1024); // 50MB limit
 * cache->initialize();
 * 
 * // Cache a music item
 * MusicItem music = getMusicFromDatabase(123);
 * cache->put(123, music);
 * 
 * // Retrieve from cache
 * if (auto cachedMusic = cache->get(123)) {
 *     // Use cached data - much faster than database query
 *     processMusic(*cachedMusic);
 * }
 * 
 * // Cache search results
 * QList<MusicItem> results = searchDatabase("rock");
 * cache->putSearchResults("genre:rock", results);
 * @endcode
 * 
 * @since XFB 2.0
 */
class MusicCache : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Cache entry metadata
     */
    struct CacheEntry {
        QVariant data;              // Cached data
        QDateTime createdAt;        // When entry was created
        QDateTime lastAccessed;     // When entry was last accessed
        QDateTime expiresAt;        // When entry expires (optional)
        int accessCount = 0;        // Number of times accessed
        qint64 size = 0;           // Size in bytes
        QString source;             // Source of the data (e.g., "database", "file")
        bool isPinned = false;      // Pinned entries are not evicted
        QHash<QString, QVariant> metadata; // Additional metadata
    };

    /**
     * @brief Cache statistics
     */
    struct CacheStatistics {
        qint64 totalHits = 0;           // Total cache hits
        qint64 totalMisses = 0;         // Total cache misses
        qint64 totalEvictions = 0;      // Total evictions
        qint64 currentEntries = 0;      // Current number of entries
        qint64 currentMemoryUsage = 0;  // Current memory usage in bytes
        qint64 maxMemoryUsage = 0;      // Maximum allowed memory usage
        double hitRatio = 0.0;          // Hit ratio (0.0 - 1.0)
        QDateTime lastCleanup;          // Last cleanup time
        QDateTime lastWarmup;           // Last warmup time
        
        // Per-category statistics
        QHash<QString, qint64> categoryHits;
        QHash<QString, qint64> categoryMisses;
        QHash<QString, qint64> categorySize;
    };

    /**
     * @brief Cache invalidation strategy
     */
    enum InvalidationStrategy {
        TimeBasedExpiration,    // Expire after fixed time
        AccessBasedExpiration,  // Expire after period of no access
        VersionBasedExpiration, // Expire when version changes
        ManualInvalidation,     // Only manual invalidation
        SmartInvalidation       // Combination of strategies
    };

    /**
     * @brief Cache warming strategy
     */
    enum WarmupStrategy {
        NoWarmup,              // No automatic warmup
        PopularItemsWarmup,    // Warm up most popular items
        RecentItemsWarmup,     // Warm up recently accessed items
        PredictiveWarmup,      // Predictive warmup based on patterns
        FullWarmup             // Warm up entire dataset (use carefully)
    };

    explicit MusicCache(QObject* parent = nullptr);
    ~MusicCache() override;

    /**
     * @brief Initialize the cache system
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the cache and cleanup resources
     */
    void shutdown();

    /**
     * @brief Set maximum memory usage for the cache
     * @param maxBytes Maximum memory usage in bytes
     */
    void setMaxMemoryUsage(qint64 maxBytes);

    /**
     * @brief Get maximum memory usage
     * @return Maximum memory usage in bytes
     */
    qint64 maxMemoryUsage() const;

    /**
     * @brief Get current memory usage
     * @return Current memory usage in bytes
     */
    qint64 currentMemoryUsage() const;

    /**
     * @brief Set default expiration time for cache entries
     * @param seconds Expiration time in seconds
     */
    void setDefaultExpirationTime(int seconds);

    /**
     * @brief Get default expiration time
     * @return Expiration time in seconds
     */
    int defaultExpirationTime() const;

    /**
     * @brief Set cache invalidation strategy
     * @param strategy Invalidation strategy to use
     */
    void setInvalidationStrategy(InvalidationStrategy strategy);

    /**
     * @brief Get current invalidation strategy
     * @return Current invalidation strategy
     */
    InvalidationStrategy invalidationStrategy() const;

    /**
     * @brief Set cache warmup strategy
     * @param strategy Warmup strategy to use
     */
    void setWarmupStrategy(WarmupStrategy strategy);

    /**
     * @brief Get current warmup strategy
     * @return Current warmup strategy
     */
    WarmupStrategy warmupStrategy() const;

    // Music item caching
    /**
     * @brief Cache a music item
     * @param musicId Music item ID
     * @param music Music item to cache
     * @param category Optional category for organization
     * @param expirationSeconds Optional custom expiration time
     */
    void put(int musicId, const MusicItem& music, const QString& category = "music", int expirationSeconds = -1);

    /**
     * @brief Get a cached music item
     * @param musicId Music item ID
     * @param category Optional category to search in
     * @return Cached music item or nullptr if not found
     */
    std::unique_ptr<MusicItem> get(int musicId, const QString& category = "music");

    /**
     * @brief Check if a music item is cached
     * @param musicId Music item ID
     * @param category Optional category to search in
     * @return true if item is cached
     */
    bool contains(int musicId, const QString& category = "music");

    /**
     * @brief Remove a music item from cache
     * @param musicId Music item ID
     * @param category Optional category to remove from
     * @return true if item was removed
     */
    bool remove(int musicId, const QString& category = "music");

    // Search results caching
    /**
     * @brief Cache search results
     * @param searchKey Search key (e.g., "artist:Beatles", "genre:rock")
     * @param results List of music items
     * @param expirationSeconds Optional custom expiration time
     */
    void putSearchResults(const QString& searchKey, const QList<MusicItem>& results, int expirationSeconds = -1);

    /**
     * @brief Get cached search results
     * @param searchKey Search key
     * @return Cached search results or empty list if not found
     */
    QList<MusicItem> getSearchResults(const QString& searchKey);

    /**
     * @brief Check if search results are cached
     * @param searchKey Search key
     * @return true if results are cached
     */
    bool containsSearchResults(const QString& searchKey);

    /**
     * @brief Remove search results from cache
     * @param searchKey Search key
     * @return true if results were removed
     */
    bool removeSearchResults(const QString& searchKey);

    // Metadata caching
    /**
     * @brief Cache arbitrary metadata
     * @param key Cache key
     * @param data Data to cache
     * @param category Optional category
     * @param expirationSeconds Optional custom expiration time
     */
    void putMetadata(const QString& key, const QVariant& data, const QString& category = "metadata", int expirationSeconds = -1);

    /**
     * @brief Get cached metadata
     * @param key Cache key
     * @param category Optional category
     * @return Cached data or invalid QVariant if not found
     */
    QVariant getMetadata(const QString& key, const QString& category = "metadata");

    /**
     * @brief Check if metadata is cached
     * @param key Cache key
     * @param category Optional category
     * @return true if metadata is cached
     */
    bool containsMetadata(const QString& key, const QString& category = "metadata");

    /**
     * @brief Remove metadata from cache
     * @param key Cache key
     * @param category Optional category
     * @return true if metadata was removed
     */
    bool removeMetadata(const QString& key, const QString& category = "metadata");

    // Cache management
    /**
     * @brief Clear all cache entries
     */
    void clear();

    /**
     * @brief Clear cache entries in a specific category
     * @param category Category to clear
     */
    void clearCategory(const QString& category);

    /**
     * @brief Invalidate cache entries based on current strategy
     */
    void invalidateExpired();

    /**
     * @brief Force cleanup of cache to free memory
     * @param targetMemoryUsage Target memory usage after cleanup
     */
    void cleanup(qint64 targetMemoryUsage = -1);

    /**
     * @brief Warm up cache with popular/recent items
     */
    void warmupCache();

    /**
     * @brief Pin an entry to prevent eviction
     * @param key Cache key
     * @param category Category
     */
    void pinEntry(const QString& key, const QString& category = "music");

    /**
     * @brief Unpin an entry to allow eviction
     * @param key Cache key
     * @param category Category
     */
    void unpinEntry(const QString& key, const QString& category = "music");

    /**
     * @brief Get cache statistics
     * @return Current cache statistics
     */
    CacheStatistics getStatistics() const;

    /**
     * @brief Get cache entry information
     * @param key Cache key
     * @param category Category
     * @return Cache entry information or nullptr if not found
     */
    std::unique_ptr<CacheEntry> getEntryInfo(const QString& key, const QString& category = "music");

    /**
     * @brief Get all cache keys in a category
     * @param category Category to list
     * @return List of cache keys
     */
    QStringList getKeys(const QString& category = QString());

    /**
     * @brief Export cache statistics to JSON
     * @return JSON string with cache statistics
     */
    QString exportStatistics();

    /**
     * @brief Save cache to persistent storage
     * @param filePath Path to save cache data
     * @return true if successful
     */
    bool saveToFile(const QString& filePath);

    /**
     * @brief Load cache from persistent storage
     * @param filePath Path to load cache data from
     * @return true if successful
     */
    bool loadFromFile(const QString& filePath);

    /**
     * @brief Enable/disable automatic cache persistence
     * @param enabled true to enable automatic persistence
     * @param filePath Path for automatic saves
     */
    void setAutoPersistence(bool enabled, const QString& filePath = QString());

signals:
    /**
     * @brief Emitted when cache hit occurs
     * @param key Cache key
     * @param category Category
     */
    void cacheHit(const QString& key, const QString& category);

    /**
     * @brief Emitted when cache miss occurs
     * @param key Cache key
     * @param category Category
     */
    void cacheMiss(const QString& key, const QString& category);

    /**
     * @brief Emitted when cache entry is evicted
     * @param key Cache key
     * @param category Category
     * @param reason Eviction reason
     */
    void entryEvicted(const QString& key, const QString& category, const QString& reason);

    /**
     * @brief Emitted when memory usage exceeds threshold
     * @param currentUsage Current memory usage
     * @param maxUsage Maximum allowed usage
     */
    void memoryThresholdExceeded(qint64 currentUsage, qint64 maxUsage);

    /**
     * @brief Emitted when cache cleanup completes
     * @param entriesRemoved Number of entries removed
     * @param memoryFreed Memory freed in bytes
     */
    void cleanupCompleted(int entriesRemoved, qint64 memoryFreed);

    /**
     * @brief Emitted when cache warmup completes
     * @param entriesWarmed Number of entries warmed up
     */
    void warmupCompleted(int entriesWarmed);

    /**
     * @brief Emitted when cache statistics are updated
     * @param statistics Current statistics
     */
    void statisticsUpdated(const CacheStatistics& statistics);

private slots:
    /**
     * @brief Perform periodic cache maintenance
     */
    void performMaintenance();

    /**
     * @brief Save cache to persistent storage (auto-persistence)
     */
    void autoPersist();

private:
    /**
     * @brief Generate cache key for music item
     * @param musicId Music item ID
     * @param category Category
     * @return Cache key
     */
    QString generateMusicKey(int musicId, const QString& category);

    /**
     * @brief Generate cache key for search results
     * @param searchKey Search key
     * @return Cache key
     */
    QString generateSearchKey(const QString& searchKey);

    /**
     * @brief Generate cache key for metadata
     * @param key Metadata key
     * @param category Category
     * @return Cache key
     */
    QString generateMetadataKey(const QString& key, const QString& category);

    /**
     * @brief Calculate size of data in bytes
     * @param data Data to calculate size for
     * @return Size in bytes
     */
    qint64 calculateDataSize(const QVariant& data);

    /**
     * @brief Check if entry has expired
     * @param entry Cache entry to check
     * @return true if expired
     */
    bool isExpired(const CacheEntry& entry);

    /**
     * @brief Evict least recently used entries
     * @param targetSize Target cache size after eviction
     * @return Number of entries evicted
     */
    int evictLRU(qint64 targetSize);

    /**
     * @brief Update cache statistics
     */
    void updateStatistics();

    /**
     * @brief Log cache operation
     * @param operation Operation name
     * @param key Cache key
     * @param success Operation success
     * @param details Additional details
     */
    void logCacheOperation(const QString& operation, const QString& key, bool success, const QString& details = QString());

    // Cache storage
    mutable QMutex m_cacheMutex;
    QHash<QString, CacheEntry> m_cache;
    
    // Configuration
    qint64 m_maxMemoryUsage;
    int m_defaultExpirationTime;
    InvalidationStrategy m_invalidationStrategy;
    WarmupStrategy m_warmupStrategy;
    
    // Statistics
    mutable QMutex m_statsMutex;
    CacheStatistics m_statistics;
    
    // Maintenance
    QTimer* m_maintenanceTimer;
    QTimer* m_persistenceTimer;
    bool m_autoPersistenceEnabled;
    QString m_persistenceFilePath;
    
    // Constants
    static constexpr qint64 DEFAULT_MAX_MEMORY = 100 * 1024 * 1024; // 100MB
    static constexpr int DEFAULT_EXPIRATION_TIME = 3600; // 1 hour
    static constexpr int MAINTENANCE_INTERVAL_MS = 300000; // 5 minutes
    static constexpr int PERSISTENCE_INTERVAL_MS = 600000; // 10 minutes
    static constexpr double MEMORY_CLEANUP_THRESHOLD = 0.9; // 90% of max memory
    static constexpr double MEMORY_CLEANUP_TARGET = 0.7; // 70% of max memory
};

#endif // MUSICCACHE_H