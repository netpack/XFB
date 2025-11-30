#ifndef PLAYLISTREPOSITORY_H
#define PLAYLISTREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>
#include <QDateTime>
#include <QMutex>
#include <memory>

/**
 * @brief Data model representing a playlist item
 * 
 * This structure represents a playlist/program as stored in the database.
 * Currently maps to the 'programs' table but designed to be extensible
 * for future playlist functionality.
 */
struct PlaylistItem {
    int id = -1;
    QString name;
    QString path;           // File path for program files
    QString description;    // Future extension
    QDateTime createdAt;    // Future extension
    QDateTime updatedAt;    // Future extension
    int trackCount = 0;     // Future extension
    QString duration;       // Future extension
    
    /**
     * @brief Check if the playlist item has valid required fields
     * @return true if item is valid
     */
    bool isValid() const {
        return !name.isEmpty();
    }
    
    /**
     * @brief Convert to QVariantMap for database operations
     * @return QVariantMap representation
     */
    QVariantMap toVariantMap() const {
        QVariantMap map;
        if (id != -1) map["id"] = id;
        map["name"] = name;
        map["path"] = path;
        map["description"] = description;
        map["created_at"] = createdAt;
        map["updated_at"] = updatedAt;
        map["track_count"] = trackCount;
        map["duration"] = duration;
        return map;
    }
    
    /**
     * @brief Create PlaylistItem from QVariantMap
     * @param map QVariantMap from database query
     * @return PlaylistItem instance
     */
    static PlaylistItem fromVariantMap(const QVariantMap& map) {
        PlaylistItem item;
        item.id = map.value("id", -1).toInt();
        item.name = map.value("name").toString();
        item.path = map.value("path").toString();
        item.description = map.value("description").toString();
        item.createdAt = map.value("created_at").toDateTime();
        item.updatedAt = map.value("updated_at").toDateTime();
        item.trackCount = map.value("track_count", 0).toInt();
        item.duration = map.value("duration").toString();
        return item;
    }
};

/**
 * @brief Data model representing a playlist track relationship
 * 
 * This structure represents the relationship between playlists and music tracks.
 * Designed for future playlist-track functionality.
 */
struct PlaylistTrack {
    int id = -1;
    int playlistId = -1;
    int musicId = -1;
    int position = 0;       // Track position in playlist
    QDateTime addedAt;
    
    bool isValid() const {
        return playlistId > 0 && musicId > 0;
    }
};

/**
 * @brief Repository for managing playlist data with secure database operations
 * 
 * The PlaylistRepository provides a secure data access layer for playlist-related
 * database operations. It currently works with the existing 'programs' table
 * but is designed to be extensible for future playlist functionality.
 * 
 * Features:
 * - All operations use prepared statements for security
 * - Comprehensive error handling and logging
 * - Thread-safe operations
 * - Input validation and sanitization
 * - Support for future playlist-track relationships
 * - Extensible design for enhanced playlist features
 * 
 * @example
 * @code
 * PlaylistRepository* repo = new PlaylistRepository(database, this);
 * 
 * // Add a playlist/program
 * PlaylistItem playlist;
 * playlist.name = "Morning Show";
 * playlist.path = "/path/to/program.xml";
 * bool success = repo->addPlaylist(playlist);
 * 
 * // Get all playlists
 * QList<PlaylistItem> allPlaylists = repo->getAllPlaylists();
 * @endcode
 * 
 * @since XFB 2.0
 */
class PlaylistRepository : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Search criteria for playlist queries
     */
    struct SearchCriteria {
        QString name;           // Playlist name to search for
        QString path;           // Path to search for
        QString description;    // Description to search for
        int limit = -1;         // -1 for no limit
        int offset = 0;
        QString orderBy = "name"; // Default ordering
        bool ascending = true;
        bool exactMatch = false; // true for exact match, false for partial match
    };

    /**
     * @brief Statistics about playlist collection
     */
    struct PlaylistStats {
        int totalPlaylists = 0;
        int totalTracks = 0;        // Future extension
        QString longestPlaylist;    // Future extension
        QString mostUsedPlaylist;   // Future extension
        QDateTime lastCreated;
        QDateTime lastUpdated;
    };

    explicit PlaylistRepository(QSqlDatabase& database, QObject* parent = nullptr);
    ~PlaylistRepository() override;

    /**
     * @brief Add a new playlist to the database
     * @param playlist PlaylistItem to add
     * @return true if successful, false otherwise
     */
    bool addPlaylist(const PlaylistItem& playlist);

    /**
     * @brief Update an existing playlist
     * @param playlist PlaylistItem with updated data (must have valid id)
     * @return true if successful, false otherwise
     */
    bool updatePlaylist(const PlaylistItem& playlist);

    /**
     * @brief Delete a playlist by ID
     * @param playlistId ID of the playlist to delete
     * @return true if successful, false otherwise
     */
    bool deletePlaylist(int playlistId);

    /**
     * @brief Get a playlist by ID
     * @param playlistId ID of the playlist
     * @return PlaylistItem if found, invalid PlaylistItem otherwise
     */
    PlaylistItem getPlaylistById(int playlistId);

    /**
     * @brief Get all playlists
     * @param limit Maximum number of items to return (-1 for no limit)
     * @param offset Number of items to skip
     * @return List of PlaylistItem objects
     */
    QList<PlaylistItem> getAllPlaylists(int limit = -1, int offset = 0);

    /**
     * @brief Search playlists based on criteria
     * @param criteria Search criteria
     * @return List of matching PlaylistItem objects
     */
    QList<PlaylistItem> searchPlaylists(const SearchCriteria& criteria);

    /**
     * @brief Get playlists by name (partial match)
     * @param name Playlist name to search for
     * @return List of matching PlaylistItem objects
     */
    QList<PlaylistItem> getPlaylistsByName(const QString& name);

    /**
     * @brief Add multiple playlists in a single transaction
     * @param playlistList List of PlaylistItem objects to add
     * @return Number of successfully added items
     */
    int addPlaylistBatch(const QList<PlaylistItem>& playlistList);

    /**
     * @brief Check if a playlist name already exists
     * @param name Playlist name to check
     * @return true if name exists, false otherwise
     */
    bool playlistExists(const QString& name);

    /**
     * @brief Check if a playlist path already exists
     * @param path Playlist path to check
     * @return true if path exists, false otherwise
     */
    bool pathExists(const QString& path);

    /**
     * @brief Get playlist collection statistics
     * @return PlaylistStats structure with collection information
     */
    PlaylistStats getStatistics();

    /**
     * @brief Get list of all playlist names
     * @return List of playlist names
     */
    QStringList getAllPlaylistNames();

    /**
     * @brief Validate playlist item data
     * @param playlist PlaylistItem to validate
     * @return Error message if invalid, empty string if valid
     */
    QString validatePlaylistItem(const PlaylistItem& playlist);

    // Future playlist-track functionality
    /**
     * @brief Add a track to a playlist (future functionality)
     * @param playlistId ID of the playlist
     * @param musicId ID of the music track
     * @param position Position in playlist (-1 for end)
     * @return true if successful, false otherwise
     */
    bool addTrackToPlaylist(int playlistId, int musicId, int position = -1);

    /**
     * @brief Remove a track from a playlist (future functionality)
     * @param playlistId ID of the playlist
     * @param musicId ID of the music track
     * @return true if successful, false otherwise
     */
    bool removeTrackFromPlaylist(int playlistId, int musicId);

    /**
     * @brief Get tracks in a playlist (future functionality)
     * @param playlistId ID of the playlist
     * @return List of PlaylistTrack objects
     */
    QList<PlaylistTrack> getPlaylistTracks(int playlistId);

signals:
    /**
     * @brief Emitted when a playlist is successfully added
     * @param playlist The added PlaylistItem
     */
    void playlistAdded(const PlaylistItem& playlist);

    /**
     * @brief Emitted when a playlist is successfully updated
     * @param playlist The updated PlaylistItem
     */
    void playlistUpdated(const PlaylistItem& playlist);

    /**
     * @brief Emitted when a playlist is successfully deleted
     * @param playlistId ID of the deleted playlist
     */
    void playlistDeleted(int playlistId);

    /**
     * @brief Emitted when a track is added to a playlist
     * @param playlistId ID of the playlist
     * @param musicId ID of the music track
     */
    void trackAddedToPlaylist(int playlistId, int musicId);

    /**
     * @brief Emitted when a track is removed from a playlist
     * @param playlistId ID of the playlist
     * @param musicId ID of the music track
     */
    void trackRemovedFromPlaylist(int playlistId, int musicId);

    /**
     * @brief Emitted when an operation error occurs
     * @param operation Name of the operation that failed
     * @param error Error message
     */
    void operationError(const QString& operation, const QString& error);

private:
    /**
     * @brief Initialize playlist-related tables if they don't exist
     * @return true if successful, false otherwise
     */
    bool initializePlaylistTables();

    /**
     * @brief Sanitize playlist name for database storage
     * @param name Playlist name to sanitize
     * @return Sanitized name
     */
    QString sanitizeName(const QString& name);

    /**
     * @brief Sanitize file path for database storage
     * @param path File path to sanitize
     * @return Sanitized path
     */
    QString sanitizePath(const QString& path);

    /**
     * @brief Log repository operation error
     * @param operation Operation name
     * @param error Error message
     * @param query SQL query (optional)
     */
    void logError(const QString& operation, const QString& error, const QString& query = QString());

    /**
     * @brief Execute a prepared query with error handling
     * @param query Prepared QSqlQuery
     * @param operation Operation name for error logging
     * @return true if successful, false otherwise
     */
    bool executeQuery(QSqlQuery& query, const QString& operation);

    QSqlDatabase& m_database;
    mutable QMutex m_mutex;
    bool m_tablesInitialized = false;
    
    // Statistics cache
    mutable QMutex m_statsMutex;
    mutable PlaylistStats m_cachedStats;
    mutable QDateTime m_statsLastUpdated;
    static constexpr int STATS_CACHE_DURATION_MS = 60000; // 1 minute
};

#endif // PLAYLISTREPOSITORY_H