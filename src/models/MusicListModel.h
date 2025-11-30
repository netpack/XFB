#ifndef MUSICLISTMODEL_H
#define MUSICLISTMODEL_H

#include <QAbstractTableModel>
#include <QTimer>
#include <QCache>
#include <QFuture>
#include <QFutureWatcher>
#include <QSortFilterProxyModel>
#include <QDate>
#include <memory>

// Forward declarations
class MusicRepository;
struct MusicItem;
class QSqlDatabase;

/**
 * @brief High-performance model for large music collections with lazy loading
 * 
 * This model implements virtual scrolling and progressive loading to handle
 * large music collections efficiently. It loads data in batches as needed
 * and provides search and filtering capabilities.
 * 
 * Features:
 * - Lazy loading with configurable batch sizes
 * - Virtual scrolling for smooth performance
 * - Intelligent caching with memory management
 * - Asynchronous data loading
 * - Search and filtering support
 * - Sort support for all columns
 * 
 * @since XFB 2.0
 */
class MusicListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief Column definitions for the music table
     */
    enum Column {
        ColumnId = 0,
        ColumnTitle,
        ColumnArtist,
        ColumnAlbum,
        ColumnGenre,
        ColumnDuration,
        ColumnPath,
        ColumnDateAdded,
        ColumnPlayCount,
        ColumnLastPlayed,
        ColumnCount // Must be last
    };

    /**
     * @brief Loading states for the model
     */
    enum LoadingState {
        Idle,
        Loading,
        Error
    };

    explicit MusicListModel(MusicRepository* repository, QObject* parent = nullptr);
    ~MusicListModel();

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    /**
     * @brief Set the batch size for loading data
     * @param batchSize Number of items to load per batch
     */
    void setBatchSize(int batchSize);

    /**
     * @brief Get the current batch size
     * @return Current batch size
     */
    int batchSize() const;

    /**
     * @brief Set the cache size limit
     * @param maxItems Maximum number of items to cache
     */
    void setCacheSize(int maxItems);

    /**
     * @brief Get the current cache size limit
     * @return Maximum cache size
     */
    int cacheSize() const;

    /**
     * @brief Set search filter
     * @param searchText Text to search for
     * @param columns Columns to search in (empty = all columns)
     */
    void setSearchFilter(const QString& searchText, const QList<int>& columns = QList<int>());

    /**
     * @brief Set genre filter
     * @param genre Genre to filter by (empty = no filter)
     */
    void setGenreFilter(const QString& genre);

    /**
     * @brief Clear all filters
     */
    void clearFilters();

    /**
     * @brief Refresh the model data
     */
    void refresh();

    /**
     * @brief Get the total number of items (including unloaded)
     * @return Total item count
     */
    int totalItemCount() const;

    /**
     * @brief Get the number of loaded items
     * @return Loaded item count
     */
    int loadedItemCount() const;

    /**
     * @brief Get the current loading state
     * @return Current loading state
     */
    LoadingState loadingState() const;

    /**
     * @brief Get music item at the specified index
     * @param index Model index
     * @return Music item (may be null if not loaded)
     */
    std::shared_ptr<MusicItem> getMusicItem(const QModelIndex& index) const;

    /**
     * @brief Get music item by ID
     * @param musicId Music ID
     * @return Music item (may be null if not found/loaded)
     */
    std::shared_ptr<MusicItem> getMusicItemById(int musicId) const;

    /**
     * @brief Preload data around the specified index
     * @param index Center index for preloading
     * @param radius Number of items to preload in each direction
     */
    void preloadAround(const QModelIndex& index, int radius = 50);

    /**
     * @brief Get column name for the specified column
     * @param column Column index
     * @return Column name
     */
    static QString getColumnName(Column column);

    /**
     * @brief Get column index for the specified name
     * @param name Column name
     * @return Column index (-1 if not found)
     */
    static int getColumnIndex(const QString& name);

public slots:
    /**
     * @brief Handle repository data changes
     */
    void onRepositoryDataChanged();

    /**
     * @brief Handle music item addition
     * @param musicItem Added music item
     */
    void onMusicItemAdded(const MusicItem& musicItem);

    /**
     * @brief Handle music item update
     * @param musicItem Updated music item
     */
    void onMusicItemUpdated(const MusicItem& musicItem);

    /**
     * @brief Handle music item removal
     * @param musicId ID of removed music item
     */
    void onMusicItemRemoved(int musicId);

signals:
    /**
     * @brief Emitted when loading state changes
     * @param state New loading state
     */
    void loadingStateChanged(LoadingState state);

    /**
     * @brief Emitted when data loading is completed
     * @param itemsLoaded Number of items loaded in this batch
     */
    void dataLoaded(int itemsLoaded);

    /**
     * @brief Emitted when an error occurs during loading
     * @param error Error message
     */
    void loadingError(const QString& error);

    /**
     * @brief Emitted when the total item count changes
     * @param totalCount New total count
     */
    void totalCountChanged(int totalCount);

    /**
     * @brief Emitted when search/filter results change
     * @param resultCount Number of items matching current filters
     */
    void filterResultsChanged(int resultCount);

private slots:
    /**
     * @brief Handle completion of asynchronous data loading
     */
    void onDataLoadingFinished();

    /**
     * @brief Handle cache cleanup timer
     */
    void onCacheCleanupTimer();

    /**
     * @brief Handle preload timer
     */
    void onPreloadTimer();

private:
    /**
     * @brief Load data batch starting at the specified offset
     * @param offset Starting offset
     * @param count Number of items to load
     */
    void loadDataBatch(int offset, int count);

    /**
     * @brief Load data asynchronously
     * @param offset Starting offset
     * @param count Number of items to load
     */
    void loadDataAsync(int offset, int count);

    /**
     * @brief Update total count from repository
     */
    void updateTotalCount();

    /**
     * @brief Build SQL query with current filters and sorting
     * @return SQL query string
     */
    QString buildQuery() const;

    /**
     * @brief Build WHERE clause for current filters
     * @return WHERE clause string
     */
    QString buildWhereClause() const;

    /**
     * @brief Build ORDER BY clause for current sorting
     * @return ORDER BY clause string
     */
    QString buildOrderByClause() const;

    /**
     * @brief Get column name for SQL queries
     * @param column Column index
     * @return SQL column name
     */
    QString getSqlColumnName(int column) const;

    /**
     * @brief Cache music item
     * @param index Row index
     * @param item Music item to cache
     */
    void cacheItem(int index, std::shared_ptr<MusicItem> item);

    /**
     * @brief Get cached music item
     * @param index Row index
     * @return Cached music item (null if not cached)
     */
    std::shared_ptr<MusicItem> getCachedItem(int index) const;

    /**
     * @brief Check if item is cached
     * @param index Row index
     * @return true if item is cached
     */
    bool isItemCached(int index) const;

    /**
     * @brief Clean up old cache entries
     */
    void cleanupCache();

    /**
     * @brief Format duration for display
     * @param durationMs Duration in milliseconds
     * @return Formatted duration string
     */
    QString formatDuration(qint64 durationMs) const;

    /**
     * @brief Format date for display
     * @param dateTime Date/time to format
     * @return Formatted date string
     */
    QString formatDate(const QDateTime& dateTime) const;

    /**
     * @brief Check if the specified range is loaded
     * @param startIndex Start index
     * @param endIndex End index
     * @return true if range is fully loaded
     */
    bool isRangeLoaded(int startIndex, int endIndex) const;

    /**
     * @brief Mark range as loaded
     * @param startIndex Start index
     * @param endIndex End index
     */
    void markRangeLoaded(int startIndex, int endIndex);
    
    /**
     * @brief Get display data for a specific item and column
     */
    QVariant getDisplayData(const MusicItem& item, int column) const;
    
    /**
     * @brief Get tooltip data for a specific item and column
     */
    QVariant getTooltipData(const MusicItem& item, int column) const;
    
    /**
     * @brief Get alignment data for a specific column
     */
    QVariant getAlignmentData(int column) const;
    
    /**
     * @brief Get header tooltip for a specific section
     */
    QVariant getHeaderTooltip(int section) const;

private:
    MusicRepository* m_repository;
    
    // Data management
    mutable QCache<int, MusicItem> m_itemCache;
    QSet<int> m_loadedRanges; // Tracks which ranges have been loaded
    int m_totalCount;
    int m_loadedCount;
    int m_batchSize;
    
    // Loading state
    LoadingState m_loadingState;
    QFutureWatcher<QList<MusicItem>>* m_loadingWatcher;
    QTimer* m_cacheCleanupTimer;
    QTimer* m_preloadTimer;
    
    // Filtering and sorting
    QString m_searchText;
    QList<int> m_searchColumns;
    QString m_genreFilter;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
    
    // Performance settings
    int m_maxCacheSize;
    int m_preloadRadius;
    int m_cacheCleanupInterval;
    
    // Column headers
    static const QStringList s_columnHeaders;
    static const QStringList s_sqlColumnNames;
};

/**
 * @brief Proxy model for additional filtering and sorting
 * 
 * This proxy model provides additional filtering capabilities
 * on top of the base MusicListModel.
 */
class MusicFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit MusicFilterProxyModel(QObject* parent = nullptr);

    /**
     * @brief Set minimum play count filter
     * @param minPlayCount Minimum play count (0 = no filter)
     */
    void setMinPlayCountFilter(int minPlayCount);

    /**
     * @brief Set date range filter
     * @param startDate Start date (invalid = no filter)
     * @param endDate End date (invalid = no filter)
     */
    void setDateRangeFilter(const QDate& startDate, const QDate& endDate);

    /**
     * @brief Set duration range filter
     * @param minDuration Minimum duration in seconds (0 = no filter)
     * @param maxDuration Maximum duration in seconds (0 = no filter)
     */
    void setDurationRangeFilter(int minDuration, int maxDuration);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int m_minPlayCount;
    QDate m_startDate;
    QDate m_endDate;
    int m_minDuration;
    int m_maxDuration;
};

#endif // MUSICLISTMODEL_H