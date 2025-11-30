#ifndef MUSICREPOSITORY_H
#define MUSICREPOSITORY_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMutex>
#include <memory>

/**
 * @brief Data model representing a music item
 * 
 * This structure represents a music track with all its metadata
 * as stored in the database. It provides validation and serialization
 * capabilities for safe data handling.
 */
struct MusicItem {
    int id = -1;
    QString artist;
    QString song;
    QString genre1;
    QString genre2;
    QString country;
    QString publishedDate;
    QString path;
    QString time;
    int playedTimes = 0;
    QString lastPlayed;
    
    /**
     * @brief Check if the music item has valid required fields
     * @return true if item is valid
     */
    bool isValid() const {
        return !artist.isEmpty() && !song.isEmpty() && !path.isEmpty();
    }
    
    /**
     * @brief Convert to QVariantMap for database operations
     * @return QVariantMap representation
     */
    QVariantMap toVariantMap() const {
        QVariantMap map;
        if (id != -1) map["id"] = id;
        map["artist"] = artist;
        map["song"] = song;
        map["genre1"] = genre1;
        map["genre2"] = genre2;
        map["country"] = country;
        map["published_date"] = publishedDate;
        map["path"] = path;
        map["time"] = time;
        map["played_times"] = playedTimes;
        map["last_played"] = lastPlayed;
        return map;
    }
    
    /**
     * @brief Create MusicItem from QVariantMap
     * @param map QVariantMap from database query
     * @return MusicItem instance
     */
    static MusicItem fromVariantMap(const QVariantMap& map) {
        MusicItem item;
        item.id = map.value("id", -1).toInt();
        item.artist = map.value("artist").toString();
        item.song = map.value("song").toString();
        item.genre1 = map.value("genre1").toString();
        item.genre2 = map.value("genre2").toString();
        item.country = map.value("country").toString();
        item.publishedDate = map.value("published_date").toString();
        item.path = map.value("path").toString();
        item.time = map.value("time").toString();
        item.playedTimes = map.value("played_times", 0).toInt();
        item.lastPlayed = map.value("last_played").toString();
        return item;
    }
};

/**
 * @brief Repository for managing music data with secure database operations
 * 
 * The MusicRepository provides a secure data access layer for music-related
 * database operations. It uses prepared statements exclusively to prevent
 * SQL injection attacks and provides comprehensive error handling.
 * 
 * Features:
 * - All operations use prepared statements for security
 * - Comprehensive error handling and logging
 * - Batch import functionality for directory scanning
 * - Thread-safe operations
 * - Input validation and sanitization
 * - Support for audio file metadata extraction
 * 
 * @example
 * @code
 * MusicRepository* repo = new MusicRepository(database, this);
 * 
 * // Add a single music item
 * MusicItem music;
 * music.artist = "Artist Name";
 * music.song = "Song Title";
 * music.path = "/path/to/song.mp3";
 * bool success = repo->addMusic(music);
 * 
 * // Batch import from directory
 * int imported = repo->importFromDirectory("/path/to/music/folder");
 * @endcode
 * 
 * @since XFB 2.0
 */
class MusicRepository : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Search criteria for music queries
     */
    struct SearchCriteria {
        QString artist;
        QString song;
        QString genre1;
        QString genre2;
        QString country;
        QString searchText; // General text search across multiple fields
        int limit = -1;     // -1 for no limit
        int offset = 0;
        QString orderBy = "artist, song"; // Default ordering
        bool ascending = true;
    };

    /**
     * @brief Statistics about music collection
     */
    struct MusicStats {
        int totalTracks = 0;
        int totalArtists = 0;
        int totalGenres = 0;
        QString totalDuration;
        int mostPlayedTrackId = -1;
        QString mostPlayedTrackTitle;
        int totalPlays = 0;
    };

    explicit MusicRepository(QSqlDatabase& database, QObject* parent = nullptr);
    ~MusicRepository() override;

    /**
     * @brief Add a new music item to the database
     * @param music MusicItem to add
     * @return true if successful, false otherwise
     */
    bool addMusic(const MusicItem& music);

    /**
     * @brief Update an existing music item
     * @param music MusicItem with updated data (must have valid id)
     * @return true if successful, false otherwise
     */
    bool updateMusic(const MusicItem& music);

    /**
     * @brief Delete a music item by ID
     * @param musicId ID of the music item to delete
     * @return true if successful, false otherwise
     */
    bool deleteMusic(int musicId);

    /**
     * @brief Get a music item by ID
     * @param musicId ID of the music item
     * @return MusicItem if found, invalid MusicItem otherwise
     */
    MusicItem getMusicById(int musicId);

    /**
     * @brief Get all music items
     * @param limit Maximum number of items to return (-1 for no limit)
     * @param offset Number of items to skip
     * @return List of MusicItem objects
     */
    QList<MusicItem> getAllMusic(int limit = -1, int offset = 0);

    /**
     * @brief Search music items based on criteria
     * @param criteria Search criteria
     * @return List of matching MusicItem objects
     */
    QList<MusicItem> searchMusic(const SearchCriteria& criteria);

    /**
     * @brief Get music items by genre
     * @param genre Genre name to search for
     * @param useGenre2 If true, search in genre2 field as well
     * @return List of matching MusicItem objects
     */
    QList<MusicItem> getMusicByGenre(const QString& genre, bool useGenre2 = false);

    /**
     * @brief Get music items by artist
     * @param artist Artist name to search for
     * @return List of matching MusicItem objects
     */
    QList<MusicItem> getMusicByArtist(const QString& artist);

    /**
     * @brief Add multiple music items in a single transaction
     * @param musicList List of MusicItem objects to add
     * @return Number of successfully added items
     */
    int addMusicBatch(const QList<MusicItem>& musicList);

    /**
     * @brief Import music files from a directory
     * @param directoryPath Path to directory containing music files
     * @param recursive If true, scan subdirectories recursively
     * @param supportedExtensions List of supported file extensions (empty for default)
     * @return Number of successfully imported files
     */
    int importFromDirectory(const QString& directoryPath, 
                           bool recursive = true,
                           const QStringList& supportedExtensions = QStringList());

    /**
     * @brief Update play count and last played time for a music item
     * @param musicId ID of the music item
     * @return true if successful, false otherwise
     */
    bool incrementPlayCount(int musicId);

    /**
     * @brief Get music collection statistics
     * @return MusicStats structure with collection information
     */
    MusicStats getStatistics();

    /**
     * @brief Get list of all unique artists
     * @return List of artist names
     */
    QStringList getAllArtists();

    /**
     * @brief Get list of all unique genres
     * @param includeGenre2 If true, include genre2 field values
     * @return List of genre names
     */
    QStringList getAllGenres(bool includeGenre2 = true);

    /**
     * @brief Check if a music file path already exists in database
     * @param filePath Path to check
     * @return true if path exists, false otherwise
     */
    bool pathExists(const QString& filePath);

    /**
     * @brief Validate music item data
     * @param music MusicItem to validate
     * @return Error message if invalid, empty string if valid
     */
    QString validateMusicItem(const MusicItem& music);

signals:
    /**
     * @brief Emitted when a music item is successfully added
     * @param music The added MusicItem
     */
    void musicAdded(const MusicItem& music);

    /**
     * @brief Emitted when a music item is successfully updated
     * @param music The updated MusicItem
     */
    void musicUpdated(const MusicItem& music);

    /**
     * @brief Emitted when a music item is successfully deleted
     * @param musicId ID of the deleted music item
     */
    void musicDeleted(int musicId);

    /**
     * @brief Emitted when a batch import operation progresses
     * @param processed Number of files processed
     * @param total Total number of files to process
     * @param currentFile Current file being processed
     */
    void importProgress(int processed, int total, const QString& currentFile);

    /**
     * @brief Emitted when a batch import operation completes
     * @param imported Number of successfully imported files
     * @param total Total number of files processed
     */
    void importCompleted(int imported, int total);

    /**
     * @brief Emitted when an operation error occurs
     * @param operation Name of the operation that failed
     * @param error Error message
     */
    void operationError(const QString& operation, const QString& error);

private:
    /**
     * @brief Extract metadata from audio file
     * @param filePath Path to audio file
     * @return MusicItem with extracted metadata
     */
    MusicItem extractMetadata(const QString& filePath);

    /**
     * @brief Check if file is a supported audio format
     * @param filePath Path to file
     * @param supportedExtensions List of supported extensions
     * @return true if supported, false otherwise
     */
    bool isSupportedAudioFile(const QString& filePath, const QStringList& supportedExtensions);

    /**
     * @brief Get default supported audio file extensions
     * @return List of supported extensions
     */
    QStringList getDefaultSupportedExtensions();

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
    QMimeDatabase m_mimeDatabase;
    
    // Prepared statement cache
    mutable QHash<QString, std::unique_ptr<QSqlQuery>> m_preparedQueries;
    
    // Statistics cache
    mutable QMutex m_statsMutex;
    mutable MusicStats m_cachedStats;
    mutable QDateTime m_statsLastUpdated;
    static constexpr int STATS_CACHE_DURATION_MS = 60000; // 1 minute
};

#endif // MUSICREPOSITORY_H