#ifndef BACKGROUNDOPERATIONFEEDBACK_H
#define BACKGROUNDOPERATIONFEEDBACK_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QMutex>

class AudioFeedbackService;

/**
 * @brief Service for providing feedback on background operations
 * 
 * This class monitors long-running background operations and provides
 * accessibility announcements for progress updates, completion notifications,
 * and critical alerts. It integrates with the AudioFeedbackService to ensure
 * users are informed about background activities.
 * 
 * @example
 * @code
 * auto* feedback = new BackgroundOperationFeedback(this);
 * feedback->initialize();
 * 
 * // Start monitoring an operation
 * int operationId = feedback->startOperation("Database scan", "Scanning music library");
 * 
 * // Update progress
 * feedback->updateProgress(operationId, 50, "Scanned 500 of 1000 files");
 * 
 * // Complete operation
 * feedback->completeOperation(operationId, true, "Scan completed successfully");
 * @endcode
 * 
 * @see AudioFeedbackService
 * @since XFB 2.0
 */
class BackgroundOperationFeedback : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Priority levels for background operation announcements
     */
    enum class Priority {
        Low,        ///< Background information
        Normal,     ///< Standard operation updates
        High,       ///< Important notifications
        Critical    ///< Critical alerts requiring immediate attention
    };

    /**
     * @brief Types of background operations
     */
    enum class OperationType {
        DatabaseScan,       ///< Music library scanning
        DatabaseUpdate,     ///< Database updates
        DatabaseBackup,     ///< Database backup operations
        DatabaseRestore,    ///< Database restore operations
        FileImport,         ///< File import operations
        FileExport,         ///< File export operations
        AudioConversion,    ///< Audio format conversion
        AudioProcessing,    ///< Audio processing operations
        NetworkOperation,   ///< Network-related operations
        SystemMaintenance,  ///< System maintenance tasks
        UserOperation       ///< User-initiated operations
    };

    /**
     * @brief Configuration for background operation feedback
     */
    struct FeedbackConfig {
        bool enabled = true;                    ///< Whether feedback is enabled
        int progressUpdateIntervalMs = 5000;    ///< Interval between progress announcements
        int minProgressChangePercent = 10;      ///< Minimum progress change to announce
        bool announceStart = true;              ///< Whether to announce operation start
        bool announceProgress = true;           ///< Whether to announce progress updates
        bool announceCompletion = true;         ///< Whether to announce completion
        bool announceCriticalAlerts = true;     ///< Whether to announce critical alerts
        int maxConcurrentOperations = 10;       ///< Maximum concurrent operations to track
    };

    explicit BackgroundOperationFeedback(QObject* parent = nullptr);
    ~BackgroundOperationFeedback();

    /**
     * @brief Initialize the background operation feedback service
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the service
     */
    void shutdown();

    /**
     * @brief Start monitoring a background operation
     * @param operationName Name of the operation
     * @param description Initial description
     * @param type Type of operation
     * @param priority Priority level for announcements
     * @return Operation ID for tracking, or -1 if failed
     */
    int startOperation(const QString& operationName, 
                      const QString& description = QString(),
                      OperationType type = OperationType::UserOperation,
                      Priority priority = Priority::Normal);

    /**
     * @brief Update progress for an operation
     * @param operationId Operation ID returned by startOperation
     * @param percentage Progress percentage (0-100)
     * @param statusMessage Current status message
     * @param estimatedTimeRemaining Optional estimated time remaining
     */
    void updateProgress(int operationId, 
                       int percentage, 
                       const QString& statusMessage = QString(),
                       const QString& estimatedTimeRemaining = QString());

    /**
     * @brief Complete an operation
     * @param operationId Operation ID
     * @param success Whether the operation was successful
     * @param completionMessage Final completion message
     */
    void completeOperation(int operationId, 
                          bool success, 
                          const QString& completionMessage = QString());

    /**
     * @brief Cancel an operation
     * @param operationId Operation ID
     * @param reason Reason for cancellation
     */
    void cancelOperation(int operationId, const QString& reason = QString());

    /**
     * @brief Report a critical alert for an operation
     * @param operationId Operation ID (can be -1 for general alerts)
     * @param alertMessage Alert message
     * @param requiresImmediateAttention Whether this requires immediate user attention
     */
    void reportCriticalAlert(int operationId, 
                           const QString& alertMessage,
                           bool requiresImmediateAttention = true);

    /**
     * @brief Report an error for an operation
     * @param operationId Operation ID
     * @param errorMessage Error message
     * @param isFatal Whether this is a fatal error that stops the operation
     */
    void reportError(int operationId, 
                    const QString& errorMessage,
                    bool isFatal = false);

    /**
     * @brief Get the current configuration
     * @return Current feedback configuration
     */
    FeedbackConfig getConfig() const { return m_config; }

    /**
     * @brief Set feedback configuration
     * @param config New configuration
     */
    void setConfig(const FeedbackConfig& config);

    /**
     * @brief Check if feedback is enabled
     * @return true if enabled
     */
    bool isEnabled() const { return m_config.enabled; }

    /**
     * @brief Enable or disable feedback
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Get the number of active operations
     * @return Number of currently active operations
     */
    int getActiveOperationCount() const;

    /**
     * @brief Get information about an active operation
     * @param operationId Operation ID
     * @return Operation information, or empty string if not found
     */
    QString getOperationInfo(int operationId) const;

    /**
     * @brief Get list of all active operation IDs
     * @return List of active operation IDs
     */
    QList<int> getActiveOperationIds() const;

public slots:
    /**
     * @brief Handle database operation start
     * @param operation Operation name
     * @param description Operation description
     */
    void onDatabaseOperationStarted(const QString& operation, const QString& description);

    /**
     * @brief Handle database operation progress
     * @param operation Operation name
     * @param percentage Progress percentage
     * @param status Current status
     */
    void onDatabaseOperationProgress(const QString& operation, int percentage, const QString& status);

    /**
     * @brief Handle database operation completion
     * @param operation Operation name
     * @param success Whether successful
     * @param message Completion message
     */
    void onDatabaseOperationCompleted(const QString& operation, bool success, const QString& message);

    /**
     * @brief Handle file import/export operations
     * @param operation Operation type
     * @param fileCount Number of files processed
     * @param totalFiles Total number of files
     * @param currentFile Current file being processed
     */
    void onFileOperationProgress(const QString& operation, int fileCount, int totalFiles, const QString& currentFile);

signals:
    /**
     * @brief Emitted when an operation starts
     * @param operationId Operation ID
     * @param operationName Operation name
     * @param type Operation type
     */
    void operationStarted(int operationId, const QString& operationName, OperationType type);

    /**
     * @brief Emitted when operation progress is updated
     * @param operationId Operation ID
     * @param percentage Progress percentage
     * @param statusMessage Status message
     */
    void operationProgressUpdated(int operationId, int percentage, const QString& statusMessage);

    /**
     * @brief Emitted when an operation completes
     * @param operationId Operation ID
     * @param success Whether successful
     * @param completionMessage Completion message
     */
    void operationCompleted(int operationId, bool success, const QString& completionMessage);

    /**
     * @brief Emitted when a critical alert occurs
     * @param operationId Operation ID
     * @param alertMessage Alert message
     */
    void criticalAlertReported(int operationId, const QString& alertMessage);

private slots:
    /**
     * @brief Handle periodic progress announcements
     */
    void onProgressAnnouncementTimer();

private:
    /**
     * @brief Operation tracking information
     */
    struct OperationInfo {
        int id;
        QString name;
        QString description;
        OperationType type;
        Priority priority;
        int currentProgress;
        QString lastStatusMessage;
        QString estimatedTimeRemaining;
        qint64 startTime;
        qint64 lastProgressTime;
        qint64 lastAnnouncementTime;
        bool isActive;
        bool hasError;
        QString errorMessage;

        OperationInfo() 
            : id(-1), type(OperationType::UserOperation), priority(Priority::Normal)
            , currentProgress(0), startTime(0), lastProgressTime(0), lastAnnouncementTime(0)
            , isActive(false), hasError(false) {}
    };

    /**
     * @brief Initialize the feedback system
     */
    void initializeFeedbackSystem();

    /**
     * @brief Shutdown the feedback system
     */
    void shutdownFeedbackSystem();

    /**
     * @brief Generate next operation ID
     * @return New unique operation ID
     */
    int generateOperationId();

    /**
     * @brief Check if progress should be announced
     * @param operation Operation info
     * @param newProgress New progress percentage
     * @return true if should announce
     */
    bool shouldAnnounceProgress(const OperationInfo& operation, int newProgress) const;

    /**
     * @brief Format operation type for announcement
     * @param type Operation type
     * @return Human-readable type string
     */
    QString formatOperationType(OperationType type) const;

    /**
     * @brief Format priority for announcement
     * @param priority Priority level
     * @return AudioFeedbackService priority
     */
    int convertPriorityToAudioFeedback(Priority priority) const;

    /**
     * @brief Clean up completed operations
     */
    void cleanupCompletedOperations();

    /**
     * @brief Load configuration from settings
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to settings
     */
    void saveConfiguration();

private:
    AudioFeedbackService* m_audioFeedbackService;
    FeedbackConfig m_config;
    
    // Operation tracking
    QHash<int, OperationInfo> m_activeOperations;
    QMutex m_operationsMutex;
    int m_nextOperationId;
    
    // Timers
    QTimer* m_progressAnnouncementTimer;
    
    // State
    bool m_initialized;
    
    // Statistics
    int m_totalOperationsStarted;
    int m_totalOperationsCompleted;
    int m_totalOperationsFailed;
    
    // Constants
    static constexpr int DEFAULT_PROGRESS_INTERVAL_MS = 5000;
    static constexpr int MIN_PROGRESS_CHANGE_PERCENT = 10;
    static constexpr int MAX_CONCURRENT_OPERATIONS = 10;
    static constexpr int OPERATION_CLEANUP_INTERVAL_MS = 30000;
};

Q_DECLARE_METATYPE(BackgroundOperationFeedback::Priority)
Q_DECLARE_METATYPE(BackgroundOperationFeedback::OperationType)

#endif // BACKGROUNDOPERATIONFEEDBACK_H