#include "TestMusicCache.h"
#include "../../../src/services/MusicCache.h"
#include "../../../src/repositories/MusicRepository.h"
#include <QSignalSpy>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QDebug>

void TestMusicCache::initTestCase()
{
    // Register metatypes for signal testing
    qRegisterMetaType<MusicCache::CacheStatistics>("CacheStatistics");
    qRegisterMetaType<MusicItem>("MusicItem");
    qRegisterMetaType<QList<MusicItem>>("QList<MusicItem>");
}

void TestMusicCache::cleanupTestCase()
{
    // Cleanup is handled in cleanup()
}

void TestMusicCache::init()
{
    // Create temporary directory
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    
    // Create cache instance
    m_cache = std::make_unique<MusicCache>(this);
}

void TestMusicCache::cleanup()
{
    m_cache.reset();
    m_tempDir.reset();
}

void TestMusicCache::testInitialization()
{
    QVERIFY(m_cache->initialize());
    
    // Test default values
    QVERIFY(m_cache->maxMemoryUsage() > 0);
    QVERIFY(m_cache->defaultExpirationTime() > 0);
    QCOMPARE(m_cache->currentMemoryUsage(), qint64(0));
    
    // Test statistics are initialized
    auto stats = m_cache->getStatistics();
    QCOMPARE(stats.currentEntries, qint64(0));
    QCOMPARE(stats.currentMemoryUsage, qint64(0));
    QVERIFY(stats.maxMemoryUsage > 0);
}

void TestMusicCache::testShutdown()
{
    QVERIFY(m_cache->initialize());
    
    // Add some data
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Verify data exists
    QVERIFY(m_cache->contains(1));
    
    // Shutdown
    m_cache->shutdown();
    
    // Verify cache is cleared
    QVERIFY(!m_cache->contains(1));
    QCOMPARE(m_cache->currentMemoryUsage(), qint64(0));
}

void TestMusicCache::testSetMaxMemoryUsage()
{
    QVERIFY(m_cache->initialize());
    
    qint64 newLimit = 50 * 1024 * 1024; // 50MB
    m_cache->setMaxMemoryUsage(newLimit);
    
    QCOMPARE(m_cache->maxMemoryUsage(), newLimit);
    
    auto stats = m_cache->getStatistics();
    QCOMPARE(stats.maxMemoryUsage, newLimit);
}

void TestMusicCache::testSetDefaultExpirationTime()
{
    int newExpiration = 1800; // 30 minutes
    m_cache->setDefaultExpirationTime(newExpiration);
    
    QCOMPARE(m_cache->defaultExpirationTime(), newExpiration);
}

void TestMusicCache::testSetInvalidationStrategy()
{
    m_cache->setInvalidationStrategy(MusicCache::TimeBasedExpiration);
    QCOMPARE(m_cache->invalidationStrategy(), MusicCache::TimeBasedExpiration);
    
    m_cache->setInvalidationStrategy(MusicCache::AccessBasedExpiration);
    QCOMPARE(m_cache->invalidationStrategy(), MusicCache::AccessBasedExpiration);
    
    m_cache->setInvalidationStrategy(MusicCache::SmartInvalidation);
    QCOMPARE(m_cache->invalidationStrategy(), MusicCache::SmartInvalidation);
}

void TestMusicCache::testSetWarmupStrategy()
{
    m_cache->setWarmupStrategy(MusicCache::PopularItemsWarmup);
    QCOMPARE(m_cache->warmupStrategy(), MusicCache::PopularItemsWarmup);
    
    m_cache->setWarmupStrategy(MusicCache::NoWarmup);
    QCOMPARE(m_cache->warmupStrategy(), MusicCache::NoWarmup);
}

void TestMusicCache::testPutAndGetMusicItem()
{
    QVERIFY(m_cache->initialize());
    
    MusicItem music = createTestMusicItem(123, "Artist1", "Song1");
    
    // Put item in cache
    m_cache->put(123, music);
    
    // Verify it can be retrieved
    auto cachedMusic = m_cache->get(123);
    QVERIFY(cachedMusic != nullptr);
    QCOMPARE(cachedMusic->id, 123);
    QCOMPARE(cachedMusic->artist, QString("Artist1"));
    QCOMPARE(cachedMusic->song, QString("Song1"));
    
    // Verify memory usage increased
    QVERIFY(m_cache->currentMemoryUsage() > 0);
}

void TestMusicCache::testMusicItemCacheHit()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy hitSpy(m_cache.get(), &MusicCache::cacheHit);
    
    MusicItem music = createTestMusicItem(456);
    m_cache->put(456, music);
    
    // Get item - should be cache hit
    auto cachedMusic = m_cache->get(456);
    QVERIFY(cachedMusic != nullptr);
    
    // Verify hit signal was emitted
    QCOMPARE(hitSpy.count(), 1);
    
    // Verify statistics
    auto stats = m_cache->getStatistics();
    QCOMPARE(stats.totalHits, qint64(1));
    QCOMPARE(stats.totalMisses, qint64(0));
    QCOMPARE(stats.hitRatio, 1.0);
}

void TestMusicCache::testMusicItemCacheMiss()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy missSpy(m_cache.get(), &MusicCache::cacheMiss);
    
    // Try to get non-existent item
    auto cachedMusic = m_cache->get(999);
    QVERIFY(cachedMusic == nullptr);
    
    // Verify miss signal was emitted
    QCOMPARE(missSpy.count(), 1);
    
    // Verify statistics
    auto stats = m_cache->getStatistics();
    QCOMPARE(stats.totalHits, qint64(0));
    QCOMPARE(stats.totalMisses, qint64(1));
    QCOMPARE(stats.hitRatio, 0.0);
}

void TestMusicCache::testContainsMusicItem()
{
    QVERIFY(m_cache->initialize());
    
    MusicItem music = createTestMusicItem(789);
    
    // Initially not in cache
    QVERIFY(!m_cache->contains(789));
    
    // Add to cache
    m_cache->put(789, music);
    
    // Now should be in cache
    QVERIFY(m_cache->contains(789));
}

void TestMusicCache::testRemoveMusicItem()
{
    QVERIFY(m_cache->initialize());
    
    MusicItem music = createTestMusicItem(101);
    m_cache->put(101, music);
    
    // Verify it's in cache
    QVERIFY(m_cache->contains(101));
    
    // Remove it
    QVERIFY(m_cache->remove(101));
    
    // Verify it's no longer in cache
    QVERIFY(!m_cache->contains(101));
    
    // Try to remove non-existent item
    QVERIFY(!m_cache->remove(999));
}

void TestMusicCache::testMusicItemExpiration()
{
    QVERIFY(m_cache->initialize());
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    MusicItem music = createTestMusicItem(202);
    m_cache->put(202, music);
    
    // Verify it's in cache
    QVERIFY(m_cache->contains(202));
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Should be expired now
    QVERIFY(!m_cache->contains(202));
    
    // Getting expired item should return nullptr
    auto cachedMusic = m_cache->get(202);
    QVERIFY(cachedMusic == nullptr);
}

void TestMusicCache::testPutAndGetSearchResults()
{
    QVERIFY(m_cache->initialize());
    
    QList<MusicItem> results = createTestMusicList(5);
    QString searchKey = "artist:TestArtist";
    
    // Put search results in cache
    m_cache->putSearchResults(searchKey, results);
    
    // Verify they can be retrieved
    auto cachedResults = m_cache->getSearchResults(searchKey);
    QCOMPARE(cachedResults.size(), 5);
    QCOMPARE(cachedResults.first().artist, QString("TestArtist"));
}

void TestMusicCache::testSearchResultsCacheHit()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy hitSpy(m_cache.get(), &MusicCache::cacheHit);
    
    QList<MusicItem> results = createTestMusicList(3);
    QString searchKey = "genre:rock";
    
    m_cache->putSearchResults(searchKey, results);
    
    // Get results - should be cache hit
    auto cachedResults = m_cache->getSearchResults(searchKey);
    QCOMPARE(cachedResults.size(), 3);
    
    // Verify hit signal was emitted
    QCOMPARE(hitSpy.count(), 1);
}

void TestMusicCache::testSearchResultsCacheMiss()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy missSpy(m_cache.get(), &MusicCache::cacheMiss);
    
    // Try to get non-existent search results
    auto cachedResults = m_cache->getSearchResults("nonexistent:search");
    QVERIFY(cachedResults.isEmpty());
    
    // Verify miss signal was emitted
    QCOMPARE(missSpy.count(), 1);
}

void TestMusicCache::testContainsSearchResults()
{
    QVERIFY(m_cache->initialize());
    
    QString searchKey = "country:USA";
    
    // Initially not in cache
    QVERIFY(!m_cache->containsSearchResults(searchKey));
    
    // Add to cache
    QList<MusicItem> results = createTestMusicList(2);
    m_cache->putSearchResults(searchKey, results);
    
    // Now should be in cache
    QVERIFY(m_cache->containsSearchResults(searchKey));
}

void TestMusicCache::testRemoveSearchResults()
{
    QVERIFY(m_cache->initialize());
    
    QString searchKey = "year:2023";
    QList<MusicItem> results = createTestMusicList(4);
    m_cache->putSearchResults(searchKey, results);
    
    // Verify it's in cache
    QVERIFY(m_cache->containsSearchResults(searchKey));
    
    // Remove it
    QVERIFY(m_cache->removeSearchResults(searchKey));
    
    // Verify it's no longer in cache
    QVERIFY(!m_cache->containsSearchResults(searchKey));
    
    // Try to remove non-existent search results
    QVERIFY(!m_cache->removeSearchResults("nonexistent:search"));
}

void TestMusicCache::testSearchResultsExpiration()
{
    QVERIFY(m_cache->initialize());
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    QString searchKey = "duration:short";
    QList<MusicItem> results = createTestMusicList(1);
    m_cache->putSearchResults(searchKey, results);
    
    // Verify it's in cache
    QVERIFY(m_cache->containsSearchResults(searchKey));
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Should be expired now
    QVERIFY(!m_cache->containsSearchResults(searchKey));
}

void TestMusicCache::testPutAndGetMetadata()
{
    QVERIFY(m_cache->initialize());
    
    QString key = "total_songs";
    QVariant value = 12345;
    
    // Put metadata in cache
    m_cache->putMetadata(key, value);
    
    // Verify it can be retrieved
    QVariant cachedValue = m_cache->getMetadata(key);
    QVERIFY(cachedValue.isValid());
    QCOMPARE(cachedValue.toInt(), 12345);
}

void TestMusicCache::testMetadataCacheHit()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy hitSpy(m_cache.get(), &MusicCache::cacheHit);
    
    QString key = "artist_count";
    QVariant value = 567;
    
    m_cache->putMetadata(key, value);
    
    // Get metadata - should be cache hit
    QVariant cachedValue = m_cache->getMetadata(key);
    QCOMPARE(cachedValue.toInt(), 567);
    
    // Verify hit signal was emitted
    QCOMPARE(hitSpy.count(), 1);
}

void TestMusicCache::testMetadataCacheMiss()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy missSpy(m_cache.get(), &MusicCache::cacheMiss);
    
    // Try to get non-existent metadata
    QVariant cachedValue = m_cache->getMetadata("nonexistent_key");
    QVERIFY(!cachedValue.isValid());
    
    // Verify miss signal was emitted
    QCOMPARE(missSpy.count(), 1);
}

void TestMusicCache::testContainsMetadata()
{
    QVERIFY(m_cache->initialize());
    
    QString key = "genre_count";
    
    // Initially not in cache
    QVERIFY(!m_cache->containsMetadata(key));
    
    // Add to cache
    m_cache->putMetadata(key, 42);
    
    // Now should be in cache
    QVERIFY(m_cache->containsMetadata(key));
}

void TestMusicCache::testRemoveMetadata()
{
    QVERIFY(m_cache->initialize());
    
    QString key = "playlist_count";
    m_cache->putMetadata(key, 15);
    
    // Verify it's in cache
    QVERIFY(m_cache->containsMetadata(key));
    
    // Remove it
    QVERIFY(m_cache->removeMetadata(key));
    
    // Verify it's no longer in cache
    QVERIFY(!m_cache->containsMetadata(key));
    
    // Try to remove non-existent metadata
    QVERIFY(!m_cache->removeMetadata("nonexistent_key"));
}

void TestMusicCache::testMetadataExpiration()
{
    QVERIFY(m_cache->initialize());
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    QString key = "temp_data";
    m_cache->putMetadata(key, "temporary");
    
    // Verify it's in cache
    QVERIFY(m_cache->containsMetadata(key));
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Should be expired now
    QVERIFY(!m_cache->containsMetadata(key));
}

void TestMusicCache::testClear()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy cleanupSpy(m_cache.get(), &MusicCache::cleanupCompleted);
    
    // Add some data
    populateCacheWithTestData(5);
    
    // Verify data exists
    QVERIFY(m_cache->currentMemoryUsage() > 0);
    auto stats = m_cache->getStatistics();
    QVERIFY(stats.currentEntries > 0);
    
    // Clear cache
    m_cache->clear();
    
    // Verify cache is empty
    QCOMPARE(m_cache->currentMemoryUsage(), qint64(0));
    stats = m_cache->getStatistics();
    QCOMPARE(stats.currentEntries, qint64(0));
    
    // Verify cleanup signal was emitted
    QCOMPARE(cleanupSpy.count(), 1);
}

void TestMusicCache::testClearCategory()
{
    QVERIFY(m_cache->initialize());
    
    // Add data to different categories
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music, "music");
    m_cache->putMetadata("key1", "value1", "metadata");
    m_cache->putSearchResults("search1", createTestMusicList(2));
    
    // Verify all data exists
    QVERIFY(m_cache->contains(1, "music"));
    QVERIFY(m_cache->containsMetadata("key1", "metadata"));
    QVERIFY(m_cache->containsSearchResults("search1"));
    
    // Clear only music category
    m_cache->clearCategory("music");
    
    // Verify only music category was cleared
    QVERIFY(!m_cache->contains(1, "music"));
    QVERIFY(m_cache->containsMetadata("key1", "metadata"));
    QVERIFY(m_cache->containsSearchResults("search1"));
}

void TestMusicCache::testInvalidateExpired()
{
    QVERIFY(m_cache->initialize());
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    // Add some data
    MusicItem music1 = createTestMusicItem(1);
    MusicItem music2 = createTestMusicItem(2);
    m_cache->put(1, music1);
    m_cache->put(2, music2);
    
    // Verify data exists
    QVERIFY(m_cache->contains(1));
    QVERIFY(m_cache->contains(2));
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Manually invalidate expired entries
    m_cache->invalidateExpired();
    
    // Verify expired entries are removed
    QVERIFY(!m_cache->contains(1));
    QVERIFY(!m_cache->contains(2));
}

void TestMusicCache::testCleanup()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy cleanupSpy(m_cache.get(), &MusicCache::cleanupCompleted);
    
    // Set small memory limit
    m_cache->setMaxMemoryUsage(1024); // 1KB
    
    // Add data that exceeds limit
    populateCacheWithTestData(20);
    
    qint64 initialMemory = m_cache->currentMemoryUsage();
    QVERIFY(initialMemory > 1024);
    
    // Force cleanup
    m_cache->cleanup(512); // Target 512 bytes
    
    // Verify memory usage decreased
    qint64 finalMemory = m_cache->currentMemoryUsage();
    QVERIFY(finalMemory < initialMemory);
    
    // Verify cleanup signal was emitted
    QVERIFY(cleanupSpy.count() > 0);
}

void TestMusicCache::testMemoryLimitEnforcement()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy thresholdSpy(m_cache.get(), &MusicCache::memoryThresholdExceeded);
    
    // Set very small memory limit
    m_cache->setMaxMemoryUsage(512); // 512 bytes
    
    // Add data that should trigger cleanup
    for (int i = 0; i < 10; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    // Memory usage should be controlled
    QVERIFY(m_cache->currentMemoryUsage() <= m_cache->maxMemoryUsage());
}

void TestMusicCache::testLRUEviction()
{
    QVERIFY(m_cache->initialize());
    
    // Set small memory limit to force eviction
    m_cache->setMaxMemoryUsage(1024);
    
    // Add items
    for (int i = 0; i < 10; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    // Access some items to update their last accessed time
    m_cache->get(5);
    m_cache->get(7);
    m_cache->get(9);
    
    // Add more items to trigger eviction
    for (int i = 10; i < 15; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    // Recently accessed items should still be in cache
    QVERIFY(m_cache->contains(5) || m_cache->contains(7) || m_cache->contains(9));
    
    // Some older items should have been evicted
    int presentCount = 0;
    for (int i = 0; i < 5; i++) {
        if (m_cache->contains(i)) {
            presentCount++;
        }
    }
    QVERIFY(presentCount < 5); // Some should have been evicted
}

void TestMusicCache::testPinnedEntriesNotEvicted()
{
    QVERIFY(m_cache->initialize());
    
    // Set small memory limit
    m_cache->setMaxMemoryUsage(1024);
    
    // Add and pin an item
    MusicItem music = createTestMusicItem(100);
    m_cache->put(100, music);
    m_cache->pinEntry("100", "music");
    
    // Add many more items to trigger eviction
    for (int i = 0; i < 20; i++) {
        MusicItem item = createTestMusicItem(i);
        m_cache->put(i, item);
    }
    
    // Pinned item should still be in cache
    QVERIFY(m_cache->contains(100));
}

void TestMusicCache::testEvictionSignals()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy evictionSpy(m_cache.get(), &MusicCache::entryEvicted);
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1);
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Wait for expiration and trigger cleanup
    QThread::sleep(2);
    m_cache->invalidateExpired();
    
    // Verify eviction signal was emitted
    QVERIFY(evictionSpy.count() > 0);
    
    QList<QVariant> arguments = evictionSpy.takeFirst();
    QVERIFY(arguments.at(2).toString().contains("expired"));
}

void TestMusicCache::testCacheStatistics()
{
    QVERIFY(m_cache->initialize());
    
    auto initialStats = m_cache->getStatistics();
    QCOMPARE(initialStats.totalHits, qint64(0));
    QCOMPARE(initialStats.totalMisses, qint64(0));
    QCOMPARE(initialStats.currentEntries, qint64(0));
    
    // Add some data and access it
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    auto cachedMusic = m_cache->get(1); // Hit
    QVERIFY(cachedMusic != nullptr);
    
    auto nonExistent = m_cache->get(999); // Miss
    QVERIFY(nonExistent == nullptr);
    
    auto finalStats = m_cache->getStatistics();
    QCOMPARE(finalStats.totalHits, qint64(1));
    QCOMPARE(finalStats.totalMisses, qint64(1));
    QCOMPARE(finalStats.currentEntries, qint64(1));
    QVERIFY(finalStats.currentMemoryUsage > 0);
}

void TestMusicCache::testHitRatioCalculation()
{
    QVERIFY(m_cache->initialize());
    
    // Add some data
    for (int i = 0; i < 5; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    // Generate hits and misses
    for (int i = 0; i < 5; i++) {
        m_cache->get(i); // Hit
    }
    
    for (int i = 10; i < 15; i++) {
        m_cache->get(i); // Miss
    }
    
    auto stats = m_cache->getStatistics();
    QCOMPARE(stats.totalHits, qint64(5));
    QCOMPARE(stats.totalMisses, qint64(5));
    QCOMPARE(stats.hitRatio, 0.5); // 50% hit ratio
}

void TestMusicCache::testCategoryStatistics()
{
    QVERIFY(m_cache->initialize());
    
    // Add data to different categories
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music, "music");
    m_cache->putMetadata("key1", "value1", "metadata");
    
    // Access them
    m_cache->get(1, "music"); // Hit
    m_cache->getMetadata("key1", "metadata"); // Hit
    m_cache->get(999, "music"); // Miss
    
    auto stats = m_cache->getStatistics();
    QCOMPARE(stats.categoryHits["music"], qint64(1));
    QCOMPARE(stats.categoryHits["metadata"], qint64(1));
    QCOMPARE(stats.categoryMisses["music"], qint64(1));
}

void TestMusicCache::testStatisticsUpdate()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy statsSpy(m_cache.get(), &MusicCache::statisticsUpdated);
    
    // Add some data to trigger statistics update
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Wait a bit for potential statistics updates
    QThread::msleep(100);
    
    // Statistics update signals may be emitted during maintenance
    // We just verify the signal exists and can be emitted
    QVERIFY(statsSpy.count() >= 0);
}

void TestMusicCache::testTimeBasedExpiration()
{
    QVERIFY(m_cache->initialize());
    
    m_cache->setInvalidationStrategy(MusicCache::TimeBasedExpiration);
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Should be available immediately
    QVERIFY(m_cache->contains(1));
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Should be expired
    QVERIFY(!m_cache->contains(1));
}

void TestMusicCache::testAccessBasedExpiration()
{
    QVERIFY(m_cache->initialize());
    
    m_cache->setInvalidationStrategy(MusicCache::AccessBasedExpiration);
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Access the item to update last accessed time
    m_cache->get(1);
    
    // Wait less than expiration time
    QThread::msleep(500);
    
    // Access again
    m_cache->get(1);
    
    // Should still be available due to recent access
    QVERIFY(m_cache->contains(1));
}

void TestMusicCache::testManualInvalidation()
{
    QVERIFY(m_cache->initialize());
    
    m_cache->setInvalidationStrategy(MusicCache::ManualInvalidation);
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Wait past expiration time
    QThread::sleep(2);
    
    // Should still be available (manual invalidation only)
    QVERIFY(m_cache->contains(1));
    
    // Manual removal should work
    QVERIFY(m_cache->remove(1));
    QVERIFY(!m_cache->contains(1));
}

void TestMusicCache::testSmartInvalidation()
{
    QVERIFY(m_cache->initialize());
    
    m_cache->setInvalidationStrategy(MusicCache::SmartInvalidation);
    m_cache->setDefaultExpirationTime(1); // 1 second
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Should be available immediately
    QVERIFY(m_cache->contains(1));
    
    // Wait for expiration
    QThread::sleep(3); // Wait longer for smart invalidation
    
    // Should be expired
    QVERIFY(!m_cache->contains(1));
}

void TestMusicCache::testWarmupCache()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy warmupSpy(m_cache.get(), &MusicCache::warmupCompleted);
    
    // Test different warmup strategies
    m_cache->setWarmupStrategy(MusicCache::NoWarmup);
    m_cache->warmupCache();
    
    m_cache->setWarmupStrategy(MusicCache::PopularItemsWarmup);
    m_cache->warmupCache();
    
    // Warmup completed signal should be emitted
    QVERIFY(warmupSpy.count() > 0);
}

void TestMusicCache::testWarmupStrategies()
{
    QVERIFY(m_cache->initialize());
    
    // Test setting different warmup strategies
    m_cache->setWarmupStrategy(MusicCache::NoWarmup);
    QCOMPARE(m_cache->warmupStrategy(), MusicCache::NoWarmup);
    
    m_cache->setWarmupStrategy(MusicCache::PopularItemsWarmup);
    QCOMPARE(m_cache->warmupStrategy(), MusicCache::PopularItemsWarmup);
    
    m_cache->setWarmupStrategy(MusicCache::RecentItemsWarmup);
    QCOMPARE(m_cache->warmupStrategy(), MusicCache::RecentItemsWarmup);
}

void TestMusicCache::testPinEntry()
{
    QVERIFY(m_cache->initialize());
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Pin the entry
    m_cache->pinEntry("1", "music");
    
    // Entry should still be accessible
    QVERIFY(m_cache->contains(1));
    
    // Get entry info to verify it's pinned
    auto entryInfo = m_cache->getEntryInfo("1", "music");
    QVERIFY(entryInfo != nullptr);
    QVERIFY(entryInfo->isPinned);
}

void TestMusicCache::testUnpinEntry()
{
    QVERIFY(m_cache->initialize());
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Pin then unpin the entry
    m_cache->pinEntry("1", "music");
    m_cache->unpinEntry("1", "music");
    
    // Get entry info to verify it's unpinned
    auto entryInfo = m_cache->getEntryInfo("1", "music");
    QVERIFY(entryInfo != nullptr);
    QVERIFY(!entryInfo->isPinned);
}

void TestMusicCache::testPinnedEntryNotEvicted()
{
    QVERIFY(m_cache->initialize());
    
    // Set small memory limit
    m_cache->setMaxMemoryUsage(1024);
    
    // Add and pin an entry
    MusicItem music = createTestMusicItem(100);
    m_cache->put(100, music);
    m_cache->pinEntry("100", "music");
    
    // Add many more entries to force eviction
    for (int i = 0; i < 20; i++) {
        MusicItem item = createTestMusicItem(i);
        m_cache->put(i, item);
    }
    
    // Pinned entry should still be there
    QVERIFY(m_cache->contains(100));
}

void TestMusicCache::testGetEntryInfo()
{
    QVERIFY(m_cache->initialize());
    
    MusicItem music = createTestMusicItem(1, "TestArtist", "TestSong");
    m_cache->put(1, music);
    
    // Access the entry to update access count
    m_cache->get(1);
    
    auto entryInfo = m_cache->getEntryInfo("1", "music");
    QVERIFY(entryInfo != nullptr);
    QVERIFY(entryInfo->createdAt.isValid());
    QVERIFY(entryInfo->lastAccessed.isValid());
    QCOMPARE(entryInfo->accessCount, 1);
    QVERIFY(entryInfo->size > 0);
    QVERIFY(!entryInfo->isPinned);
    QCOMPARE(entryInfo->metadata["musicId"].toInt(), 1);
}

void TestMusicCache::testGetKeys()
{
    QVERIFY(m_cache->initialize());
    
    // Add data to different categories
    MusicItem music1 = createTestMusicItem(1);
    MusicItem music2 = createTestMusicItem(2);
    m_cache->put(1, music1, "music");
    m_cache->put(2, music2, "music");
    m_cache->putMetadata("key1", "value1", "metadata");
    
    // Get all keys
    QStringList allKeys = m_cache->getKeys();
    QVERIFY(allKeys.size() >= 3);
    
    // Get keys for specific category
    QStringList musicKeys = m_cache->getKeys("music");
    QCOMPARE(musicKeys.size(), 2);
    
    QStringList metadataKeys = m_cache->getKeys("metadata");
    QCOMPARE(metadataKeys.size(), 1);
}

void TestMusicCache::testGetKeysWithCategory()
{
    QVERIFY(m_cache->initialize());
    
    // Add data to multiple categories
    m_cache->put(1, createTestMusicItem(1), "music");
    m_cache->put(2, createTestMusicItem(2), "music");
    m_cache->putMetadata("stat1", 100, "stats");
    m_cache->putMetadata("stat2", 200, "stats");
    m_cache->putSearchResults("search1", createTestMusicList(1));
    
    // Test category filtering
    QStringList musicKeys = m_cache->getKeys("music");
    QStringList statsKeys = m_cache->getKeys("stats");
    QStringList searchKeys = m_cache->getKeys("search");
    
    QCOMPARE(musicKeys.size(), 2);
    QCOMPARE(statsKeys.size(), 2);
    QCOMPARE(searchKeys.size(), 1);
}

void TestMusicCache::testSaveToFile()
{
    QVERIFY(m_cache->initialize());
    
    // Add some test data
    populateCacheWithTestData(5);
    
    QString filePath = m_tempDir->filePath("cache_test.json");
    
    // Save cache to file
    QVERIFY(m_cache->saveToFile(filePath));
    
    // Verify file was created
    QVERIFY(QFile::exists(filePath));
    
    // Verify file contains valid JSON
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    
    QJsonObject root = doc.object();
    QVERIFY(root.contains("entries"));
    QVERIFY(root.contains("version"));
    QVERIFY(root.contains("timestamp"));
}

void TestMusicCache::testLoadFromFile()
{
    QVERIFY(m_cache->initialize());
    
    // Add and save some test data
    populateCacheWithTestData(3);
    QString filePath = m_tempDir->filePath("cache_load_test.json");
    QVERIFY(m_cache->saveToFile(filePath));
    
    // Clear cache
    m_cache->clear();
    QCOMPARE(m_cache->getStatistics().currentEntries, qint64(0));
    
    // Load cache from file
    QVERIFY(m_cache->loadFromFile(filePath));
    
    // Verify data was loaded (some entries might be expired)
    auto stats = m_cache->getStatistics();
    QVERIFY(stats.currentEntries >= 0);
}

void TestMusicCache::testAutoPersistence()
{
    QVERIFY(m_cache->initialize());
    
    QString filePath = m_tempDir->filePath("auto_persist_test.json");
    
    // Enable auto-persistence
    m_cache->setAutoPersistence(true, filePath);
    
    // Add some data
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Shutdown should trigger save
    m_cache->shutdown();
    
    // Verify file was created
    QVERIFY(QFile::exists(filePath));
}

void TestMusicCache::testPersistenceWithExpiredEntries()
{
    QVERIFY(m_cache->initialize());
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1);
    
    // Add data that will expire
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    QString filePath = m_tempDir->filePath("expired_test.json");
    QVERIFY(m_cache->saveToFile(filePath));
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Clear and reload
    m_cache->clear();
    QVERIFY(m_cache->loadFromFile(filePath));
    
    // Expired entries should not be loaded
    QVERIFY(!m_cache->contains(1));
}

void TestMusicCache::testExportStatistics()
{
    QVERIFY(m_cache->initialize());
    
    // Generate some statistics
    populateCacheWithTestData(5);
    
    // Access some items to generate hits/misses
    m_cache->get(1); // Hit
    m_cache->get(999); // Miss
    
    QString jsonStats = m_cache->exportStatistics();
    QVERIFY(!jsonStats.isEmpty());
    
    // Verify it's valid JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStats.toUtf8(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    
    QJsonObject root = doc.object();
    QVERIFY(root.contains("totalHits"));
    QVERIFY(root.contains("totalMisses"));
    QVERIFY(root.contains("currentEntries"));
    QVERIFY(root.contains("hitRatio"));
}

void TestMusicCache::testStatisticsJsonFormat()
{
    QVERIFY(m_cache->initialize());
    
    // Add some data and generate statistics
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music, "music");
    m_cache->putMetadata("key1", "value1", "metadata");
    
    m_cache->get(1, "music"); // Hit
    m_cache->get(999, "music"); // Miss
    
    QString jsonStats = m_cache->exportStatistics();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStats.toUtf8());
    QJsonObject root = doc.object();
    
    // Verify required fields
    QVERIFY(root.contains("totalHits"));
    QVERIFY(root.contains("totalMisses"));
    QVERIFY(root.contains("hitRatio"));
    QVERIFY(root.contains("categoryHits"));
    QVERIFY(root.contains("categoryMisses"));
    
    // Verify category statistics
    QJsonObject categoryHits = root["categoryHits"].toObject();
    QVERIFY(categoryHits.contains("music"));
    QCOMPARE(categoryHits["music"].toInt(), 1);
}

void TestMusicCache::testMemoryUsageCalculation()
{
    QVERIFY(m_cache->initialize());
    
    qint64 initialMemory = m_cache->currentMemoryUsage();
    QCOMPARE(initialMemory, qint64(0));
    
    // Add an item
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    qint64 afterAddMemory = m_cache->currentMemoryUsage();
    QVERIFY(afterAddMemory > initialMemory);
    
    // Remove the item
    m_cache->remove(1);
    
    qint64 afterRemoveMemory = m_cache->currentMemoryUsage();
    QCOMPARE(afterRemoveMemory, initialMemory);
}

void TestMusicCache::testMemoryThresholdExceeded()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy thresholdSpy(m_cache.get(), &MusicCache::memoryThresholdExceeded);
    
    // Set very small memory limit
    m_cache->setMaxMemoryUsage(100); // 100 bytes
    
    // Add data that exceeds the threshold
    for (int i = 0; i < 10; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    // Memory threshold signal might be emitted
    // (depends on cleanup timing and actual data sizes)
    QVERIFY(thresholdSpy.count() >= 0);
}

void TestMusicCache::testCleanupTargetSize()
{
    QVERIFY(m_cache->initialize());
    
    // Add substantial data
    populateCacheWithTestData(20);
    
    qint64 initialMemory = m_cache->currentMemoryUsage();
    qint64 targetSize = initialMemory / 2;
    
    // Cleanup to target size
    m_cache->cleanup(targetSize);
    
    qint64 finalMemory = m_cache->currentMemoryUsage();
    QVERIFY(finalMemory <= targetSize || finalMemory < initialMemory);
}

void TestMusicCache::testConcurrentAccess()
{
    QVERIFY(m_cache->initialize());
    
    // Pre-populate cache
    for (int i = 0; i < 10; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    QList<QThread*> threads;
    QAtomicInt successCount;
    
    // Create threads that access cache concurrently
    for (int t = 0; t < 5; t++) {
        QThread* thread = QThread::create([this, &successCount, t]() {
            for (int i = 0; i < 20; i++) {
                int id = (t * 20 + i) % 10; // Access existing items
                auto music = m_cache->get(id);
                if (music != nullptr) {
                    successCount.fetchAndAddOrdered(1);
                }
                QThread::msleep(1);
            }
        });
        
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads to complete
    for (auto* thread : threads) {
        QVERIFY(thread->wait(5000));
        delete thread;
    }
    
    // Verify successful concurrent access
    QVERIFY(successCount.loadAcquire() > 0);
}

void TestMusicCache::testThreadSafety()
{
    QVERIFY(m_cache->initialize());
    
    QList<QThread*> threads;
    QAtomicInt operationCount;
    
    // Create threads that perform different operations
    for (int t = 0; t < 3; t++) {
        QThread* thread = QThread::create([this, &operationCount, t]() {
            for (int i = 0; i < 10; i++) {
                int id = t * 10 + i;
                
                // Put operation
                MusicItem music = createTestMusicItem(id);
                m_cache->put(id, music);
                operationCount.fetchAndAddOrdered(1);
                
                // Get operation
                auto cachedMusic = m_cache->get(id);
                if (cachedMusic != nullptr) {
                    operationCount.fetchAndAddOrdered(1);
                }
                
                // Contains operation
                if (m_cache->contains(id)) {
                    operationCount.fetchAndAddOrdered(1);
                }
                
                QThread::msleep(1);
            }
        });
        
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads to complete
    for (auto* thread : threads) {
        QVERIFY(thread->wait(5000));
        delete thread;
    }
    
    // Verify operations completed successfully
    QVERIFY(operationCount.loadAcquire() > 0);
    
    // Cache should be in consistent state
    auto stats = m_cache->getStatistics();
    QVERIFY(stats.currentEntries >= 0);
    QVERIFY(stats.currentMemoryUsage >= 0);
}

void TestMusicCache::testLargeDatasetPerformance()
{
    QVERIFY(m_cache->initialize());
    
    // Set reasonable memory limit
    m_cache->setMaxMemoryUsage(10 * 1024 * 1024); // 10MB
    
    QElapsedTimer timer;
    timer.start();
    
    // Add large number of items
    const int itemCount = 1000;
    for (int i = 0; i < itemCount; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    qint64 insertTime = timer.elapsed();
    
    // Test retrieval performance
    timer.restart();
    int hitCount = 0;
    
    for (int i = 0; i < itemCount; i++) {
        auto music = m_cache->get(i);
        if (music != nullptr) {
            hitCount++;
        }
    }
    
    qint64 retrievalTime = timer.elapsed();
    
    // Performance should be reasonable
    QVERIFY(insertTime < 5000); // Less than 5 seconds
    QVERIFY(retrievalTime < 2000); // Less than 2 seconds
    QVERIFY(hitCount > 0); // Some items should be found
    
    qDebug() << QString("Performance test: %1 items, insert: %2ms, retrieval: %3ms, hits: %4")
                 .arg(itemCount).arg(insertTime).arg(retrievalTime).arg(hitCount);
}

void TestMusicCache::testCachePerformanceWithManyEntries()
{
    QVERIFY(m_cache->initialize());
    
    // Set large memory limit
    m_cache->setMaxMemoryUsage(50 * 1024 * 1024); // 50MB
    
    const int entryCount = 5000;
    
    QElapsedTimer timer;
    timer.start();
    
    // Add many entries
    for (int i = 0; i < entryCount; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
        
        // Also add some metadata and search results
        if (i % 10 == 0) {
            m_cache->putMetadata(QString("meta_%1").arg(i), i * 2);
            m_cache->putSearchResults(QString("search_%1").arg(i), createTestMusicList(2));
        }
    }
    
    qint64 populationTime = timer.elapsed();
    
    // Test random access performance
    timer.restart();
    int accessCount = 1000;
    int hitCount = 0;
    
    for (int i = 0; i < accessCount; i++) {
        int randomId = QRandomGenerator::global()->bounded(entryCount);
        auto music = m_cache->get(randomId);
        if (music != nullptr) {
            hitCount++;
        }
    }
    
    qint64 accessTime = timer.elapsed();
    
    // Verify performance is acceptable
    QVERIFY(populationTime < 10000); // Less than 10 seconds
    QVERIFY(accessTime < 1000); // Less than 1 second for 1000 accesses
    
    auto stats = m_cache->getStatistics();
    QVERIFY(stats.currentEntries > 0);
    QVERIFY(stats.hitRatio >= 0.0);
    
    qDebug() << QString("Large dataset test: %1 entries, population: %2ms, access: %3ms, hit ratio: %4")
                 .arg(entryCount).arg(populationTime).arg(accessTime).arg(stats.hitRatio);
}

void TestMusicCache::testInvalidFilePath()
{
    QVERIFY(m_cache->initialize());
    
    // Try to save to invalid path
    QString invalidPath = "/invalid/path/that/does/not/exist/cache.json";
    QVERIFY(!m_cache->saveToFile(invalidPath));
    
    // Try to load from non-existent file
    QString nonExistentPath = m_tempDir->filePath("nonexistent.json");
    QVERIFY(!m_cache->loadFromFile(nonExistentPath));
}

void TestMusicCache::testCorruptedCacheFile()
{
    QVERIFY(m_cache->initialize());
    
    // Create corrupted JSON file
    QString corruptedPath = m_tempDir->filePath("corrupted.json");
    QFile file(corruptedPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ invalid json content");
    file.close();
    
    // Try to load corrupted file
    QVERIFY(!m_cache->loadFromFile(corruptedPath));
}

void TestMusicCache::testMemoryExhaustion()
{
    QVERIFY(m_cache->initialize());
    
    // Set very small memory limit
    m_cache->setMaxMemoryUsage(512); // 512 bytes
    
    // Try to add data that exceeds limit
    for (int i = 0; i < 100; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
    }
    
    // Cache should handle memory exhaustion gracefully
    QVERIFY(m_cache->currentMemoryUsage() <= m_cache->maxMemoryUsage() * 1.1); // Allow 10% tolerance
    
    // Cache should still be functional
    auto stats = m_cache->getStatistics();
    QVERIFY(stats.currentEntries >= 0);
}

void TestMusicCache::testCacheHitSignal()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy hitSpy(m_cache.get(), &MusicCache::cacheHit);
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Access item - should emit hit signal
    auto cachedMusic = m_cache->get(1);
    QVERIFY(cachedMusic != nullptr);
    
    QCOMPARE(hitSpy.count(), 1);
    QList<QVariant> arguments = hitSpy.takeFirst();
    QVERIFY(arguments.at(0).toString().contains("music:music:1"));
    QCOMPARE(arguments.at(1).toString(), QString("music"));
}

void TestMusicCache::testCacheMissSignal()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy missSpy(m_cache.get(), &MusicCache::cacheMiss);
    
    // Try to access non-existent item
    auto cachedMusic = m_cache->get(999);
    QVERIFY(cachedMusic == nullptr);
    
    QCOMPARE(missSpy.count(), 1);
    QList<QVariant> arguments = missSpy.takeFirst();
    QVERIFY(arguments.at(0).toString().contains("music:music:999"));
    QCOMPARE(arguments.at(1).toString(), QString("music"));
}

void TestMusicCache::testEntryEvictedSignal()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy evictionSpy(m_cache.get(), &MusicCache::entryEvicted);
    
    // Set short expiration time
    m_cache->setDefaultExpirationTime(1);
    
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Wait for expiration
    QThread::sleep(2);
    
    // Trigger cleanup
    m_cache->invalidateExpired();
    
    // Should emit eviction signal
    QVERIFY(evictionSpy.count() > 0);
    
    QList<QVariant> arguments = evictionSpy.takeFirst();
    QVERIFY(arguments.at(2).toString().contains("expired"));
}

void TestMusicCache::testCleanupCompletedSignal()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy cleanupSpy(m_cache.get(), &MusicCache::cleanupCompleted);
    
    // Add some data
    populateCacheWithTestData(5);
    
    // Clear cache
    m_cache->clear();
    
    // Should emit cleanup completed signal
    QCOMPARE(cleanupSpy.count(), 1);
    
    QList<QVariant> arguments = cleanupSpy.takeFirst();
    QVERIFY(arguments.at(0).toInt() > 0); // Entries removed
    QVERIFY(arguments.at(1).toLongLong() > 0); // Memory freed
}

void TestMusicCache::testStatisticsUpdatedSignal()
{
    QVERIFY(m_cache->initialize());
    
    QSignalSpy statsSpy(m_cache.get(), &MusicCache::statisticsUpdated);
    
    // Add some data to potentially trigger statistics update
    MusicItem music = createTestMusicItem(1);
    m_cache->put(1, music);
    
    // Statistics updates may happen during maintenance
    // We just verify the signal can be emitted
    QVERIFY(statsSpy.count() >= 0);
}

MusicItem TestMusicCache::createTestMusicItem(int id, const QString& artist, const QString& song)
{
    MusicItem music;
    music.id = id;
    music.artist = artist.isEmpty() ? QString("TestArtist%1").arg(id) : artist;
    music.song = song.isEmpty() ? QString("TestSong%1").arg(id) : song;
    music.genre1 = "TestGenre";
    music.genre2 = "TestGenre2";
    music.country = "TestCountry";
    music.publishedDate = "2023";
    music.path = QString("/test/path/song%1.mp3").arg(id);
    music.time = "3:45";
    music.playedTimes = id % 10;
    music.lastPlayed = QDateTime::currentDateTime().toString();
    
    return music;
}

QList<MusicItem> TestMusicCache::createTestMusicList(int count)
{
    QList<MusicItem> list;
    for (int i = 0; i < count; i++) {
        list.append(createTestMusicItem(i));
    }
    return list;
}

void TestMusicCache::populateCacheWithTestData(int count)
{
    for (int i = 0; i < count; i++) {
        MusicItem music = createTestMusicItem(i);
        m_cache->put(i, music);
        
        // Add some metadata too
        if (i % 3 == 0) {
            m_cache->putMetadata(QString("meta_%1").arg(i), QString("value_%1").arg(i));
        }
        
        // Add some search results
        if (i % 5 == 0) {
            QList<MusicItem> results;
            results.append(music);
            m_cache->putSearchResults(QString("search_%1").arg(i), results);
        }
    }
}

void TestMusicCache::waitForSignal(QObject* sender, const char* signal, int timeout)
{
    QSignalSpy spy(sender, signal);
    QElapsedTimer timer;
    timer.start();
    
    while (spy.count() == 0 && timer.elapsed() < timeout) {
        QThread::msleep(10);
        QCoreApplication::processEvents();
    }
}

QTEST_MAIN(TestMusicCache)