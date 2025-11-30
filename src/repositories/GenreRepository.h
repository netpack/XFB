#ifndef GENREREPOSITORY_H
#define GENREREPOSITORY_H

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
 * @brief Data model representing a genre item
 * 
 * This structure represents a music genre as stored in the database.
 * The system supports two genre tables (genres1 and genres2) for
 * different categorization purposes.
 */
struct GenreItem {
    int id = -1;
    QString name;
    int genreType = 1; // 1 for genres1, 2 for genres2
    
    /**
     * @brief Check if the genre item has valid required fields
     * @return true if item is valid
     */
    bool isValid() const {
        return !name.isEmpty() && (genreType == 1 || genreType == 2);
    }
    
    /**
     * @brief Convert to QVariantMap for database operations
     * @return QVariantMap representation
     */
    QVariantMap toVariantMap() const {
        QVariantMap map;
        if (id != -1) map["id"] = id;
        map["name"] = name;
        return map;
    }
    
    /**
     * @brief Create GenreItem from QVariantMap
     * @param map QVariantMap from database query
     * @param genreType Genre type (1 or 2)
     * @return GenreItem instance
     */
    static GenreItem fromVariantMap(const QVariantMap& map, int genreType = 1) {
        GenreItem item;
        item.id = map.value("id", -1).toInt();
        item.name = map.value("name").toString();
        item.genreType = genreType;
        return item;
    }
};

/**
 * @brief Repository for managing genre data with secure database operations
 * 
 * The GenreRepository provides a secure data access layer for genre-related
 * database operations. It manages both genres1 and genres2 tables and uses
 * prepared statements exclusively to prevent SQL injection attacks.
 * 
 * Features:
 * - All operations use prepared statements for security
 * - Supports both genres1 and genres2 tables
 * - Comprehensive error handling and logging
 * - Thread-safe operations
 * - Input validation and sanitization
 * - Duplicate prevention
 * 
 * @example
 * @code
 * GenreRepository* repo = new GenreRepository(database, this);
 * 
 * // Add a genre to genres1 table
 * GenreItem genre;
 * genre.name = "Jazz";
 * genre.genreType = 1;
 * bool success = repo->addGenre(genre);
 * 
 * // Get all genres from both tables
 * QList<GenreItem> allGenres = repo->getAllGenres();
 * @endcode
 * 
 * @since XFB 2.0
 */
class GenreRepository : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Search criteria for genre queries
     */
    struct SearchCriteria {
        QString name;       // Genre name to search for
        int genreType = 0;  // 0 for both, 1 for genres1, 2 for genres2
        int limit = -1;     // -1 for no limit
        int offset = 0;
        bool exactMatch = false; // true for exact match, false for partial match
    };

    /**
     * @brief Statistics about genre collection
     */
    struct GenreStats {
        int totalGenres1 = 0;
        int totalGenres2 = 0;
        int totalGenres = 0;
        QString mostUsedGenre1;
        QString mostUsedGenre2;
    };

    explicit GenreRepository(QSqlDatabase& database, QObject* parent = nullptr);
    ~GenreRepository() override;

    /**
     * @brief Add a new genre to the appropriate table
     * @param genre GenreItem to add
     * @return true if successful, false otherwise
     */
    bool addGenre(const GenreItem& genre);

    /**
     * @brief Update an existing genre
     * @param genre GenreItem with updated data (must have valid id and genreType)
     * @return true if successful, false otherwise
     */
    bool updateGenre(const GenreItem& genre);

    /**
     * @brief Delete a genre by ID and type
     * @param genreId ID of the genre item to delete
     * @param genreType Genre type (1 or 2)
     * @return true if successful, false otherwise
     */
    bool deleteGenre(int genreId, int genreType);

    /**
     * @brief Get a genre item by ID and type
     * @param genreId ID of the genre item
     * @param genreType Genre type (1 or 2)
     * @return GenreItem if found, invalid GenreItem otherwise
     */
    GenreItem getGenreById(int genreId, int genreType);

    /**
     * @brief Get all genres from specified table(s)
     * @param genreType 0 for both tables, 1 for genres1, 2 for genres2
     * @return List of GenreItem objects
     */
    QList<GenreItem> getAllGenres(int genreType = 0);

    /**
     * @brief Search genres based on criteria
     * @param criteria Search criteria
     * @return List of matching GenreItem objects
     */
    QList<GenreItem> searchGenres(const SearchCriteria& criteria);

    /**
     * @brief Get genres by name (partial match)
     * @param name Genre name to search for
     * @param genreType 0 for both tables, 1 for genres1, 2 for genres2
     * @return List of matching GenreItem objects
     */
    QList<GenreItem> getGenresByName(const QString& name, int genreType = 0);

    /**
     * @brief Add multiple genres in a single transaction
     * @param genreList List of GenreItem objects to add
     * @return Number of successfully added items
     */
    int addGenreBatch(const QList<GenreItem>& genreList);

    /**
     * @brief Check if a genre name already exists in the specified table
     * @param name Genre name to check
     * @param genreType Genre type (1 or 2)
     * @return true if name exists, false otherwise
     */
    bool genreExists(const QString& name, int genreType);

    /**
     * @brief Get genre collection statistics
     * @return GenreStats structure with collection information
     */
    GenreStats getStatistics();

    /**
     * @brief Get list of all unique genre names
     * @param genreType 0 for both tables, 1 for genres1, 2 for genres2
     * @return List of genre names
     */
    QStringList getAllGenreNames(int genreType = 0);

    /**
     * @brief Validate genre item data
     * @param genre GenreItem to validate
     * @return Error message if invalid, empty string if valid
     */
    QString validateGenreItem(const GenreItem& genre);

    /**
     * @brief Get the table name for the specified genre type
     * @param genreType Genre type (1 or 2)
     * @return Table name or empty string if invalid type
     */
    static QString getTableName(int genreType);

signals:
    /**
     * @brief Emitted when a genre is successfully added
     * @param genre The added GenreItem
     */
    void genreAdded(const GenreItem& genre);

    /**
     * @brief Emitted when a genre is successfully updated
     * @param genre The updated GenreItem
     */
    void genreUpdated(const GenreItem& genre);

    /**
     * @brief Emitted when a genre is successfully deleted
     * @param genreId ID of the deleted genre
     * @param genreType Type of the deleted genre
     */
    void genreDeleted(int genreId, int genreType);

    /**
     * @brief Emitted when an operation error occurs
     * @param operation Name of the operation that failed
     * @param error Error message
     */
    void operationError(const QString& operation, const QString& error);

private:
    /**
     * @brief Sanitize genre name for database storage
     * @param name Genre name to sanitize
     * @return Sanitized name
     */
    QString sanitizeName(const QString& name);

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
    
    // Statistics cache
    mutable QMutex m_statsMutex;
    mutable GenreStats m_cachedStats;
    mutable QDateTime m_statsLastUpdated;
    static constexpr int STATS_CACHE_DURATION_MS = 60000; // 1 minute
};

#endif // GENREREPOSITORY_H