#ifndef TESTMUSICCACHE_H
#define TESTMUSICCACHE_H

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <memory>

class MusicCache;
struct MusicItem;

/**
 * @brief Unit tests for MusicCache class
 * 
 * Tests intelligent caching functionality including cache operations,
 * invalidation strategies, memory management, and performance optimization.
 */
class TestMusicCache : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Initialization tests
    void testInitialization();
    void testShutdown();

    // Configuration tests
    void testSetMaxMemoryUsage();
    void testSetDefaultExpirationTime();
    void testSetInvalidationStrategy();
    void testSetWarmupStrategy();

    // Music item caching tests
    void testPutAndGetMusicItem();
    void testMusicItemCacheHit();
    void testMusicItemCacheMiss();
    void testContainsMusicItem();
    void testRemoveMusicItem();
    void testMusicItemExpiration();

    // Search results caching tests
    void testPutAndGetSearchResults();
    void testSearchResultsCacheHit();
    void testSearchResultsCacheMiss();
    void testContainsSearchResults();
    void testRemoveSearchResults();
    void testSearchResultsExpiration();

    // Metadata caching tests
    void testPutAndGetMetadata();
    void testMetadataCacheHit();
    void testMetadataCacheMiss();
    void testContainsMetadata();
    void testRemoveMetadata();
    void testMetadataExpiration();

    // Cache management tests
    void testClear();
    void testClearCategory();
    void testInvalidateExpired();
    void testCleanup();
    void testMemoryLimitEnforcement();

    // LRU eviction tests
    void testLRUEviction();
    void testPinnedEntriesNotEvicted();
    void testEvictionSignals();

    // Statistics tests
    void testCacheStatistics();
    void testHitRatioCalculation();
    void testCategoryStatistics();
    void testStatisticsUpdate();

    // Invalidation strategy tests
    void testTimeBasedExpiration();
    void testAccessBasedExpiration();
    void testManualInvalidation();
    void testSmartInvalidation();

    // Cache warming tests
    void testWarmupCache();
    void testWarmupStrategies();

    // Pin/unpin tests
    void testPinEntry();
    void testUnpinEntry();
    void testPinnedEntryNotEvicted();

    // Entry information tests
    void testGetEntryInfo();
    void testGetKeys();
    void testGetKeysWithCategory();

    // Persistence tests
    void testSaveToFile();
    void testLoadFromFile();
    void testAutoPersistence();
    void testPersistenceWithExpiredEntries();

    // Export/import tests
    void testExportStatistics();
    void testStatisticsJsonFormat();

    // Memory management tests
    void testMemoryUsageCalculation();
    void testMemoryThresholdExceeded();
    void testCleanupTargetSize();

    // Concurrency tests
    void testConcurrentAccess();
    void testThreadSafety();

    // Performance tests
    void testLargeDatasetPerformance();
    void testCachePerformanceWithManyEntries();

    // Error handling tests
    void testInvalidFilePath();
    void testCorruptedCacheFile();
    void testMemoryExhaustion();

    // Signal tests
    void testCacheHitSignal();
    void testCacheMissSignal();
    void testEntryEvictedSignal();
    void testCleanupCompletedSignal();
    void testStatisticsUpdatedSignal();

private:
    MusicItem createTestMusicItem(int id, const QString& artist = "TestArtist", const QString& song = "TestSong");
    QList<MusicItem> createTestMusicList(int count);
    void populateCacheWithTestData(int count = 10);
    void waitForSignal(QObject* sender, const char* signal, int timeout = 1000);
    
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<MusicCache> m_cache;
};

#endif // TESTMUSICCACHE_H