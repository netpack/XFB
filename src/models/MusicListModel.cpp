#include "MusicListModel.h"
#include "../repositories/MusicRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QtConcurrent>
#include <QThread>
#include <algorithm>

// Static member definitions
const QStringList MusicListModel::s_columnHeaders = {
    QObject::tr("ID"),
    QObject::tr("Title"),
    QObject::tr("Artist"),
    QObject::tr("Album"),
    QObject::tr("Genre"),
    QObject::tr("Duration"),
    QObject::tr("Path"),
    QObject::tr("Date Added"),
    QObject::tr("Play Count"),
    QObject::tr("Last Played")
};

const QStringList MusicListModel::s_sqlColumnNames = {
    "id",
    "song",
    "artist",
    "album",
    "genre1",
    "duration",
    "path",
    "date_added",
    "play_count",
    "last_played"
};

MusicListModel::MusicListModel(MusicRepository* repository, QObject* parent)
    : QAbstractTableModel(parent)
    , m_repository(repository)
    , m_itemCache(1000) // Default cache size
    , m_totalCount(0)
    , m_loadedCount(0)
    , m_batchSize(100)
    , m_loadingState(Idle)
    , m_loadingWatcher(new QFutureWatcher<QList<MusicItem>>(this))
    , m_cacheCleanupTimer(new QTimer(this))
    , m_preloadTimer(new QTimer(this))
    , m_sortColumn(ColumnTitle)
    , m_sortOrder(Qt::AscendingOrder)
    , m_maxCacheSize(1000)
    , m_preloadRadius(50)
    , m_cacheCleanupInterval(30000) // 30 seconds
{
    // Setup cache
    m_itemCache.setMaxCost(m_maxCacheSize);
    
    // Setup timers
    m_cacheCleanupTimer->setInterval(m_cacheCleanupInterval);
    m_cacheCleanupTimer->setSingleShot(false);
    connect(m_cacheCleanupTimer, &QTimer::timeout, this, &MusicListModel::onCacheCleanupTimer);
    m_cacheCleanupTimer->start();
    
    m_preloadTimer->setInterval(100); // 100ms delay for preloading
    m_preloadTimer->setSingleShot(true);
    connect(m_preloadTimer, &QTimer::timeout, this, &MusicListModel::onPreloadTimer);
    
    // Setup async loading
    connect(m_loadingWatcher, &QFutureWatcher<QList<MusicItem>>::finished,
            this, &MusicListModel::onDataLoadingFinished);
    
    // Connect to repository signals if available
    if (m_repository) {
        connect(m_repository, &MusicRepository::musicAdded,
                this, &MusicListModel::onMusicItemAdded);
        connect(m_repository, &MusicRepository::musicUpdated,
                this, &MusicListModel::onMusicItemUpdated);
        connect(m_repository, &MusicRepository::musicDeleted,
                this, &MusicListModel::onMusicItemRemoved);
        
        // Initialize total count
        updateTotalCount();
    }
    
    qDebug() << "MusicListModel: Initialized with batch size" << m_batchSize 
             << "and cache size" << m_maxCacheSize;
}

MusicListModel::~MusicListModel()
{
    // Cancel any ongoing loading
    if (m_loadingWatcher && m_loadingWatcher->isRunning()) {
        m_loadingWatcher->cancel();
        m_loadingWatcher->waitForFinished();
    }
}

int MusicListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_totalCount;
}

int MusicListModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant MusicListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_totalCount || index.column() >= ColumnCount) {
        return QVariant();
    }
    
    // Check if we need to load more data
    if (!isItemCached(index.row())) {
        // Trigger loading for this range
        const_cast<MusicListModel*>(this)->loadDataBatch(
            (index.row() / m_batchSize) * m_batchSize, 
            m_batchSize
        );
        
        // Return loading indicator for now
        if (role == Qt::DisplayRole) {
            return tr("Loading...");
        } else if (role == Qt::ForegroundRole) {
            return QColor(Qt::gray);
        }
        return QVariant();
    }
    
    auto item = getCachedItem(index.row());
    if (!item) {
        return QVariant();
    }
    
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return getDisplayData(*item, index.column());
        
    case Qt::ToolTipRole:
        return getTooltipData(*item, index.column());
        
    case Qt::TextAlignmentRole:
        return getAlignmentData(index.column());
        
    case Qt::UserRole:
        return QVariant::fromValue(*item);
        
    case Qt::UserRole + 1: // Music ID role
        return item->id;
        
    default:
        return QVariant();
    }
}

QVariant MusicListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || section >= ColumnCount) {
        return QVariant();
    }
    
    switch (role) {
    case Qt::DisplayRole:
        return s_columnHeaders.at(section);
        
    case Qt::ToolTipRole:
        return getHeaderTooltip(section);
        
    default:
        return QVariant();
    }
}

bool MusicListModel::canFetchMore(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_loadedCount < m_totalCount && m_loadingState != Loading;
}

void MusicListModel::fetchMore(const QModelIndex& parent)
{
    Q_UNUSED(parent)
    
    if (m_loadingState == Loading || m_loadedCount >= m_totalCount) {
        return;
    }
    
    // Load next batch
    loadDataBatch(m_loadedCount, m_batchSize);
}

void MusicListModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= ColumnCount) {
        return;
    }
    
    if (m_sortColumn == column && m_sortOrder == order) {
        return; // No change needed
    }
    
    emit layoutAboutToBeChanged();
    
    m_sortColumn = column;
    m_sortOrder = order;
    
    // Clear cache and reload data with new sorting
    m_itemCache.clear();
    m_loadedRanges.clear();
    m_loadedCount = 0;
    
    emit layoutChanged();
    
    // Trigger reload of visible data
    loadDataBatch(0, m_batchSize);
    
    qDebug() << "MusicListModel: Sorted by column" << column << "order" << order;
}

void MusicListModel::setBatchSize(int batchSize)
{
    if (batchSize > 0 && batchSize != m_batchSize) {
        m_batchSize = batchSize;
        qDebug() << "MusicListModel: Batch size set to" << batchSize;
    }
}

int MusicListModel::batchSize() const
{
    return m_batchSize;
}

void MusicListModel::setCacheSize(int maxItems)
{
    if (maxItems > 0 && maxItems != m_maxCacheSize) {
        m_maxCacheSize = maxItems;
        m_itemCache.setMaxCost(maxItems);
        qDebug() << "MusicListModel: Cache size set to" << maxItems;
    }
}

int MusicListModel::cacheSize() const
{
    return m_maxCacheSize;
}

void MusicListModel::setSearchFilter(const QString& searchText, const QList<int>& columns)
{
    if (m_searchText == searchText && m_searchColumns == columns) {
        return; // No change
    }
    
    beginResetModel();
    
    m_searchText = searchText;
    m_searchColumns = columns;
    
    // Clear cache and reload
    m_itemCache.clear();
    m_loadedRanges.clear();
    m_loadedCount = 0;
    
    updateTotalCount();
    
    endResetModel();
    
    // Load first batch with new filter
    if (m_totalCount > 0) {
        loadDataBatch(0, m_batchSize);
    }
    
    emit filterResultsChanged(m_totalCount);
    
    qDebug() << "MusicListModel: Search filter set to" << searchText 
             << "in columns" << columns << "- found" << m_totalCount << "results";
}

void MusicListModel::setGenreFilter(const QString& genre)
{
    if (m_genreFilter == genre) {
        return; // No change
    }
    
    beginResetModel();
    
    m_genreFilter = genre;
    
    // Clear cache and reload
    m_itemCache.clear();
    m_loadedRanges.clear();
    m_loadedCount = 0;
    
    updateTotalCount();
    
    endResetModel();
    
    // Load first batch with new filter
    if (m_totalCount > 0) {
        loadDataBatch(0, m_batchSize);
    }
    
    emit filterResultsChanged(m_totalCount);
    
    qDebug() << "MusicListModel: Genre filter set to" << genre 
             << "- found" << m_totalCount << "results";
}

void MusicListModel::clearFilters()
{
    if (m_searchText.isEmpty() && m_genreFilter.isEmpty()) {
        return; // No filters to clear
    }
    
    beginResetModel();
    
    m_searchText.clear();
    m_searchColumns.clear();
    m_genreFilter.clear();
    
    // Clear cache and reload
    m_itemCache.clear();
    m_loadedRanges.clear();
    m_loadedCount = 0;
    
    updateTotalCount();
    
    endResetModel();
    
    // Load first batch
    if (m_totalCount > 0) {
        loadDataBatch(0, m_batchSize);
    }
    
    emit filterResultsChanged(m_totalCount);
    
    qDebug() << "MusicListModel: Filters cleared - total count" << m_totalCount;
}

void MusicListModel::refresh()
{
    beginResetModel();
    
    // Clear all cached data
    m_itemCache.clear();
    m_loadedRanges.clear();
    m_loadedCount = 0;
    
    updateTotalCount();
    
    endResetModel();
    
    // Reload first batch
    if (m_totalCount > 0) {
        loadDataBatch(0, m_batchSize);
    }
    
    qDebug() << "MusicListModel: Refreshed - total count" << m_totalCount;
}

int MusicListModel::totalItemCount() const
{
    return m_totalCount;
}

int MusicListModel::loadedItemCount() const
{
    return m_loadedCount;
}

MusicListModel::LoadingState MusicListModel::loadingState() const
{
    return m_loadingState;
}

std::shared_ptr<MusicItem> MusicListModel::getMusicItem(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() >= m_totalCount) {
        return nullptr;
    }
    
    return getCachedItem(index.row());
}

std::shared_ptr<MusicItem> MusicListModel::getMusicItemById(int musicId) const
{
    // QCache doesn't support iteration, so we'll query the repository directly
    // This is a simplified implementation - in a real scenario, you might want
    // to maintain a separate index for ID-based lookups
    if (m_repository) {
        // Query repository for the specific music item by ID
        // This would need to be implemented in MusicRepository
        // For now, return nullptr to avoid blocking the UI
    }
    return nullptr;
}

void MusicListModel::preloadAround(const QModelIndex& index, int radius)
{
    if (!index.isValid() || radius <= 0) {
        return;
    }
    
    int centerRow = index.row();
    int startRow = std::max(0, centerRow - radius);
    int endRow = std::min(m_totalCount - 1, centerRow + radius);
    
    // Check which ranges need loading
    for (int row = startRow; row <= endRow; row += m_batchSize) {
        int batchStart = (row / m_batchSize) * m_batchSize;
        int batchEnd = std::min(batchStart + m_batchSize - 1, m_totalCount - 1);
        
        if (!isRangeLoaded(batchStart, batchEnd)) {
            // Schedule this batch for loading
            QTimer::singleShot(0, this, [this, batchStart]() {
                loadDataBatch(batchStart, m_batchSize);
            });
        }
    }
}

QString MusicListModel::getColumnName(Column column)
{
    if (column >= 0 && column < s_columnHeaders.size()) {
        return s_columnHeaders.at(column);
    }
    return QString();
}

int MusicListModel::getColumnIndex(const QString& name)
{
    return s_columnHeaders.indexOf(name);
}

void MusicListModel::onRepositoryDataChanged()
{
    refresh();
}

void MusicListModel::onMusicItemAdded(const MusicItem& musicItem)
{
    // For now, just refresh the model
    // In a more sophisticated implementation, we could insert the item
    // at the correct position based on current sorting
    refresh();
}

void MusicListModel::onMusicItemUpdated(const MusicItem& musicItem)
{
    // QCache doesn't support iteration, so we'll need to refresh the model
    // In a real implementation, you'd maintain a separate index for ID-based lookups
    // For now, just refresh the entire model
    // Clear cache and reload data
    m_itemCache.clear();
    m_loadedRanges.clear();
    beginResetModel();
    endResetModel();
}

void MusicListModel::onMusicItemRemoved(int musicId)
{
    // QCache doesn't support iteration, so we'll clear the cache and refresh
    // In a real implementation, you'd maintain a separate index for ID-based lookups
    m_itemCache.clear();
    
    // Refresh to get accurate count
    refresh();
}

void MusicListModel::onDataLoadingFinished()
{
    if (!m_loadingWatcher->isFinished()) {
        return;
    }
    
    try {
        QList<MusicItem> items = m_loadingWatcher->result();
        
        // Cache the loaded items
        int startIndex = m_loadedCount;
        for (int i = 0; i < items.size(); ++i) {
            auto item = std::make_shared<MusicItem>(items.at(i));
            cacheItem(startIndex + i, item);
        }
        
        // Update loaded count
        int oldLoadedCount = m_loadedCount;
        m_loadedCount += items.size();
        
        // Mark range as loaded
        markRangeLoaded(startIndex, startIndex + items.size() - 1);
        
        // Emit data changed for the loaded range
        if (items.size() > 0) {
            QModelIndex topLeft = index(startIndex, 0);
            QModelIndex bottomRight = index(startIndex + items.size() - 1, ColumnCount - 1);
            emit dataChanged(topLeft, bottomRight);
        }
        
        m_loadingState = Idle;
        emit loadingStateChanged(m_loadingState);
        emit dataLoaded(items.size());
        
        qDebug() << "MusicListModel: Loaded" << items.size() << "items, total loaded:" << m_loadedCount;
        
    } catch (const std::exception& e) {
        m_loadingState = Error;
        emit loadingStateChanged(m_loadingState);
        emit loadingError(QString("Failed to load data: %1").arg(e.what()));
        
        qWarning() << "MusicListModel: Loading error:" << e.what();
    }
}

void MusicListModel::onCacheCleanupTimer()
{
    cleanupCache();
}

void MusicListModel::onPreloadTimer()
{
    // This could implement intelligent preloading based on scroll patterns
    // For now, it's just a placeholder
}

void MusicListModel::loadDataBatch(int offset, int count)
{
    if (m_loadingState == Loading || !m_repository) {
        return;
    }
    
    // Check if this range is already loaded
    if (isRangeLoaded(offset, offset + count - 1)) {
        return;
    }
    
    m_loadingState = Loading;
    emit loadingStateChanged(m_loadingState);
    
    // Load data asynchronously
    loadDataAsync(offset, count);
}

void MusicListModel::loadDataAsync(int offset, int count)
{
    // Create the query in the main thread
    QString query = buildQuery();
    query += QString(" LIMIT %1 OFFSET %2").arg(count).arg(offset);
    
    // Run the query in a separate thread
    QFuture<QList<MusicItem>> future = QtConcurrent::run([this, query]() -> QList<MusicItem> {
        if (!m_repository) {
            return QList<MusicItem>();
        }
        
        // This would need to be implemented in MusicRepository
        // For now, return empty list
        return QList<MusicItem>();
    });
    
    m_loadingWatcher->setFuture(future);
}

void MusicListModel::updateTotalCount()
{
    if (!m_repository) {
        m_totalCount = 0;
        return;
    }
    
    // Build count query
    QString countQuery = "SELECT COUNT(*) FROM musics";
    QString whereClause = buildWhereClause();
    if (!whereClause.isEmpty()) {
        countQuery += " WHERE " + whereClause;
    }
    
    // Execute count query (this should be done asynchronously in production)
    // For now, we'll use a placeholder value
    int oldCount = m_totalCount;
    // m_totalCount = m_repository->getCount(countQuery);
    m_totalCount = 1000; // Placeholder
    
    if (oldCount != m_totalCount) {
        emit totalCountChanged(m_totalCount);
    }
}

QString MusicListModel::buildQuery() const
{
    QString query = "SELECT " + s_sqlColumnNames.join(", ") + " FROM musics";
    
    QString whereClause = buildWhereClause();
    if (!whereClause.isEmpty()) {
        query += " WHERE " + whereClause;
    }
    
    QString orderByClause = buildOrderByClause();
    if (!orderByClause.isEmpty()) {
        query += " ORDER BY " + orderByClause;
    }
    
    return query;
}

QString MusicListModel::buildWhereClause() const
{
    QStringList conditions;
    
    // Search filter
    if (!m_searchText.isEmpty()) {
        QStringList searchConditions;
        QList<int> columnsToSearch = m_searchColumns.isEmpty() ? 
            QList<int>{ColumnTitle, ColumnArtist, ColumnAlbum} : m_searchColumns;
        
        for (int column : columnsToSearch) {
            QString columnName = getSqlColumnName(column);
            if (!columnName.isEmpty()) {
                searchConditions << QString("%1 LIKE '%%2%'").arg(columnName, m_searchText);
            }
        }
        
        if (!searchConditions.isEmpty()) {
            conditions << "(" + searchConditions.join(" OR ") + ")";
        }
    }
    
    // Genre filter
    if (!m_genreFilter.isEmpty()) {
        conditions << QString("genre1 = '%1'").arg(m_genreFilter);
    }
    
    return conditions.join(" AND ");
}

QString MusicListModel::buildOrderByClause() const
{
    QString columnName = getSqlColumnName(m_sortColumn);
    if (columnName.isEmpty()) {
        return QString();
    }
    
    QString orderDirection = (m_sortOrder == Qt::AscendingOrder) ? "ASC" : "DESC";
    return QString("%1 %2").arg(columnName, orderDirection);
}

QString MusicListModel::getSqlColumnName(int column) const
{
    if (column >= 0 && column < s_sqlColumnNames.size()) {
        return s_sqlColumnNames.at(column);
    }
    return QString();
}

void MusicListModel::cacheItem(int index, std::shared_ptr<MusicItem> item)
{
    if (item) {
        m_itemCache.insert(index, new MusicItem(*item));
    }
}

std::shared_ptr<MusicItem> MusicListModel::getCachedItem(int index) const
{
    MusicItem* item = m_itemCache.object(index);
    if (item) {
        return std::make_shared<MusicItem>(*item);
    }
    return nullptr;
}

bool MusicListModel::isItemCached(int index) const
{
    return m_itemCache.contains(index);
}

void MusicListModel::cleanupCache()
{
    // The QCache will automatically remove least recently used items
    // when it exceeds the maximum cost. This method could implement
    // additional cleanup logic if needed.
    
    qDebug() << "MusicListModel: Cache cleanup - current size:" << m_itemCache.size();
}

QString MusicListModel::formatDuration(qint64 durationMs) const
{
    if (durationMs <= 0) {
        return "0:00";
    }
    
    int totalSeconds = static_cast<int>(durationMs / 1000);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

QString MusicListModel::formatDate(const QDateTime& dateTime) const
{
    if (!dateTime.isValid()) {
        return QString();
    }
    
    return dateTime.toString("yyyy-MM-dd hh:mm");
}

bool MusicListModel::isRangeLoaded(int startIndex, int endIndex) const
{
    // Simplified check - in practice this would be more sophisticated
    for (int i = startIndex; i <= endIndex; ++i) {
        if (!isItemCached(i)) {
            return false;
        }
    }
    return true;
}

void MusicListModel::markRangeLoaded(int startIndex, int endIndex)
{
    // Mark the range as loaded in our tracking set
    for (int i = startIndex; i <= endIndex; ++i) {
        m_loadedRanges.insert(i);
    }
}

QVariant MusicListModel::getDisplayData(const MusicItem& item, int column) const
{
    switch (column) {
    case ColumnId:
        return item.id;
    case ColumnTitle:
        return item.song;
    case ColumnArtist:
        return item.artist;
    case ColumnAlbum:
        return QString(); // Album not available in current MusicItem structure
    case ColumnGenre:
        return item.genre1;
    case ColumnDuration:
        return item.time; // Use time field instead of duration
    case ColumnPath:
        return item.path;
    case ColumnDateAdded:
        return QString(); // Date added not available in current MusicItem structure
    case ColumnPlayCount:
        return item.playedTimes;
    case ColumnLastPlayed:
        return item.lastPlayed; // lastPlayed is already a QString
    default:
        return QVariant();
    }
}

QVariant MusicListModel::getTooltipData(const MusicItem& item, int column) const
{
    switch (column) {
    case ColumnPath:
        return item.path;
    case ColumnTitle:
        return QString("%1\nArtist: %2\nGenre: %3").arg(item.song, item.artist, item.genre1);
    default:
        return getDisplayData(item, column);
    }
}

QVariant MusicListModel::getAlignmentData(int column) const
{
    switch (column) {
    case ColumnId:
    case ColumnDuration:
    case ColumnPlayCount:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    case ColumnDateAdded:
    case ColumnLastPlayed:
        return Qt::AlignCenter;
    default:
        return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    }
}

QVariant MusicListModel::getHeaderTooltip(int section) const
{
    switch (section) {
    case ColumnId:
        return tr("Unique identifier for the music item");
    case ColumnTitle:
        return tr("Song title");
    case ColumnArtist:
        return tr("Artist name");
    case ColumnAlbum:
        return tr("Album name");
    case ColumnGenre:
        return tr("Music genre");
    case ColumnDuration:
        return tr("Song duration (mm:ss)");
    case ColumnPath:
        return tr("File path on disk");
    case ColumnDateAdded:
        return tr("Date when the song was added to the library");
    case ColumnPlayCount:
        return tr("Number of times the song has been played");
    case ColumnLastPlayed:
        return tr("Date and time when the song was last played");
    default:
        return QString();
    }
}

// MusicFilterProxyModel implementation

MusicFilterProxyModel::MusicFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_minPlayCount(0)
    , m_minDuration(0)
    , m_maxDuration(0)
{
    setDynamicSortFilter(true);
}

void MusicFilterProxyModel::setMinPlayCountFilter(int minPlayCount)
{
    if (m_minPlayCount != minPlayCount) {
        m_minPlayCount = minPlayCount;
        invalidateFilter();
    }
}

void MusicFilterProxyModel::setDateRangeFilter(const QDate& startDate, const QDate& endDate)
{
    if (m_startDate != startDate || m_endDate != endDate) {
        m_startDate = startDate;
        m_endDate = endDate;
        invalidateFilter();
    }
}

void MusicFilterProxyModel::setDurationRangeFilter(int minDuration, int maxDuration)
{
    if (m_minDuration != minDuration || m_maxDuration != maxDuration) {
        m_minDuration = minDuration;
        m_maxDuration = maxDuration;
        invalidateFilter();
    }
}

bool MusicFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }
    
    // Get the music item
    QVariant itemVariant = sourceModel()->data(index, Qt::UserRole);
    if (!itemVariant.canConvert<MusicItem>()) {
        return true; // Accept if we can't get the item (might be loading)
    }
    
    MusicItem item = itemVariant.value<MusicItem>();
    
    // Apply play count filter
    if (m_minPlayCount > 0 && item.playedTimes < m_minPlayCount) {
        return false;
    }
    
    // Date filtering not available with current MusicItem structure
    // Duration filtering not available with current MusicItem structure
    int durationSeconds = 0; // Placeholder since duration is not available as integer
    if (m_minDuration > 0 && durationSeconds < m_minDuration) {
        return false;
    }
    if (m_maxDuration > 0 && durationSeconds > m_maxDuration) {
        return false;
    }
    
    return true;
}