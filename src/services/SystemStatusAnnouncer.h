#ifndef SYSTEMSTATUSANNOUNCER_H
#define SYSTEMSTATUSANNOUNCER_H

#include "BaseService.h"
#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>

class LiveRegionManager;
class AccessibilityManager;
class AudioFeedbackService;

/**
 * @brief Service for announcing system status changes and critical alerts
 * 
 * The SystemStatusAnnouncer handles announcements for recording status, streaming
 * status, database operations, and critical system alerts. It integrates with the
 * LiveRegionManager to provide real-time updates and implements priority-based
 * announcement queuing with critical alert interruption capabilities.
 * 
 * This service monitors various system components and provides appropriate
 * announcements based on the current accessibility settings and user preferences.
 * 
 * @example
 * @code
 * auto* announcer = ServiceContainer::instance()->resolve<SystemStatusAnnouncer>();
 * announcer->announceRecordingStatus("started", "Program Recording");
 * announcer->announceStreamingStatus("connected", "Live Stream");
 * announcer->announceCriticalAlert("Database connection lost", "Database Error");
 * @endcode
 * 
 * @see LiveRegionManager, AccessibilityManager, BaseService
 * @since XFB 2.0
 */
class SystemStatusAnnouncer : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Types of system components that can have status updates
     */
    enum class ComponentType {
        Recorder,           ///< Audio recording system
        Streamer,           ///< Live streaming system
        Database,           ///< Database operations
        FileSystem,         ///< File system operations
        Network,            ///< Network connectivity
        AudioDevice,        ///< Audio device status
        Scheduler,          ///< Program scheduler
        FTPClient,          ///< FTP operations
        ExternalProcess,    ///< External process monitoring
        SystemResource      ///< System resource monitoring
    };

    /**
     * @brief Status types for system components
     */
    enum class StatusType {
        Started,            ///< Component started/activated
        Stopped,            ///< Component stopped/deactivated
        Connected,          ///< Connection established
        Disconnected,       ///< Connection lost
        InProgress,         ///< Operation in progress
        Completed,          ///< Operation completed successfully
        Failed,             ///< Operation failed
        Warning,            ///< Warning condition
        Error,              ///< Error condition
        Critical,           ///< Critical error requiring immediate attention
        Recovering,         ///< System attempting recovery
        Recovered           ///< Recovery completed
    };

    /**
     * @brief Priority levels for system announcements
     */
    enum class Priority {
        Low,                ///< Background status updates
        Normal,             ///< Standard status changes
        High,               ///< Important status changes
        Critical,           ///< Critical alerts that interrupt other announcements
        Emergency           ///< Emergency alerts with highest priority
    };

    /**
     * @brief Configuration for system status announcements
     */
    struct AnnouncementConfig {
        bool enabled = true;                        ///< Whether system announcements are enabled
        bool recordingStatusEnabled = true;         ///< Announce recording status changes
        bool streamingStatusEnabled = true;         ///< Announce streaming status changes
        bool databaseProgressEnabled = true;        ///< Announce database operation progress
        bool networkStatusEnabled = true;           ///< Announce network status changes
        bool criticalAlertsEnabled = true;          ///< Announce critical alerts
        bool progressUpdatesEnabled = true;         ///< Announce progress for long operations
        int progressUpdateIntervalMs = 5000;        ///< Interval for progress updates
        int criticalAlertRepeatIntervalMs = 30000;  ///< Interval for repeating critical alerts
        bool allowCriticalInterruption = true;     ///< Allow critical alerts to interrupt
        int maxQueuedAnnouncements = 20;            ///< Maximum queued announcements
    };

    explicit SystemStatusAnnouncer(QObject* parent = nullptr);
    ~SystemStatusAnnouncer() override;

    /**
     * @brief Announce recording status change
     * @param status The new recording status (started, stopped, paused, etc.)
     * @param recordingName Optional name of the recording
     * @param additionalInfo Optional additional information
     */
    void announceRecordingStatus(const QString& status, const QString& recordingName = QString(), 
                                const QString& additionalInfo = QString());

    /**
     * @brief Announce streaming status change
     * @param status The new streaming status (connected, disconnected, etc.)
     * @param streamName Optional name of the stream
     * @param additionalInfo Optional additional information (bitrate, listeners, etc.)
     */
    void announceStreamingStatus(const QString& status, const QString& streamName = QString(), 
                                const QString& additionalInfo = QString());

    /**
     * @brief Announce database operation progress
     * @param operation The database operation (scan, backup, optimize, etc.)
     * @param progress Progress percentage (0-100)
     * @param itemsProcessed Optional number of items processed
     * @param totalItems Optional total number of items
     * @param estimatedTimeRemaining Optional estimated time remaining
     */
    void announceDatabaseProgress(const QString& operation, int progress, 
                                 int itemsProcessed = -1, int totalItems = -1,
                                 const QString& estimatedTimeRemaining = QString());

    /**
     * @brief Announce database operation completion
     * @param operation The completed operation
     * @param success Whether the operation was successful
     * @param resultMessage Optional result message
     */
    void announceDatabaseCompletion(const QString& operation, bool success, 
                                   const QString& resultMessage = QString());

    /**
     * @brief Announce network status change
     * @param component The network component (FTP, streaming server, etc.)
     * @param status The new network status
     * @param additionalInfo Optional additional information
     */
    void announceNetworkStatus(const QString& component, const QString& status, 
                              const QString& additionalInfo = QString());

    /**
     * @brief Announce file system operation
     * @param operation The file operation (copy, move, delete, etc.)
     * @param fileName Optional file name
     * @param status The operation status
     * @param additionalInfo Optional additional information
     */
    void announceFileSystemOperation(const QString& operation, const QString& fileName, 
                                    const QString& status, const QString& additionalInfo = QString());

    /**
     * @brief Announce audio device status change
     * @param deviceName The audio device name
     * @param status The new device status
     * @param deviceType Optional device type (input, output)
     */
    void announceAudioDeviceStatus(const QString& deviceName, const QString& status, 
                                  const QString& deviceType = QString());

    /**
     * @brief Announce scheduler event
     * @param eventType The type of scheduler event (program start, end, etc.)
     * @param programName Optional program name
     * @param scheduledTime Optional scheduled time
     */
    void announceSchedulerEvent(const QString& eventType, const QString& programName = QString(), 
                               const QString& scheduledTime = QString());

    /**
     * @brief Announce critical system alert
     * @param alertMessage The critical alert message
     * @param alertType Optional type of alert for categorization
     * @param requiresImmediateAction Whether this requires immediate user action
     */
    void announceCriticalAlert(const QString& alertMessage, const QString& alertType = QString(), 
                              bool requiresImmediateAction = true);

    /**
     * @brief Announce system resource status
     * @param resourceType The type of resource (memory, disk, CPU)
     * @param status The resource status
     * @param usagePercentage Optional usage percentage
     */
    void announceResourceStatus(const QString& resourceType, const QString& status, 
                               int usagePercentage = -1);

    /**
     * @brief Announce external process status
     * @param processName The name of the external process
     * @param status The process status
     * @param additionalInfo Optional additional information
     */
    void announceExternalProcessStatus(const QString& processName, const QString& status, 
                                      const QString& additionalInfo = QString());

    /**
     * @brief Announce general system status change
     * @param component The system component
     * @param status The new status
     * @param priority Priority level of the announcement
     * @param additionalInfo Optional additional information
     */
    void announceSystemStatus(const QString& component, const QString& status, 
                             Priority priority = Priority::Normal, const QString& additionalInfo = QString());

    /**
     * @brief Get the current announcement configuration
     * @return Current configuration
     */
    AnnouncementConfig getConfig() const { return m_config; }

    /**
     * @brief Set announcement configuration
     * @param config New configuration to apply
     */
    void setConfig(const AnnouncementConfig& config);

    /**
     * @brief Enable or disable system status announcements
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if system announcements are currently enabled
     * @return true if enabled
     */
    bool isEnabled() const { return m_config.enabled; }

    /**
     * @brief Enable or disable recording status announcements
     * @param enabled true to enable, false to disable
     */
    void setRecordingStatusEnabled(bool enabled);

    /**
     * @brief Enable or disable streaming status announcements
     * @param enabled true to enable, false to disable
     */
    void setStreamingStatusEnabled(bool enabled);

    /**
     * @brief Enable or disable database progress announcements
     * @param enabled true to enable, false to disable
     */
    void setDatabaseProgressEnabled(bool enabled);

    /**
     * @brief Enable or disable critical alert announcements
     * @param enabled true to enable, false to disable
     */
    void setCriticalAlertsEnabled(bool enabled);

    /**
     * @brief Clear all queued announcements
     * @param preserveCritical Whether to preserve critical announcements
     */
    void clearAnnouncementQueue(bool preserveCritical = true);

    /**
     * @brief Get the number of queued announcements
     * @return Number of announcements in queue
     */
    int getQueuedAnnouncementCount() const;

public slots:
    /**
     * @brief Handle accessibility state changes
     * @param enabled Whether accessibility is enabled
     */
    void onAccessibilityStateChanged(bool enabled);

    /**
     * @brief Handle verbosity level changes
     * @param level New verbosity level
     */
    void onVerbosityLevelChanged(int level);

signals:
    /**
     * @brief Emitted when a system status is announced
     * @param component The system component
     * @param status The status
     * @param priority Priority level
     */
    void systemStatusAnnounced(const QString& component, const QString& status, Priority priority);

    /**
     * @brief Emitted when a critical alert is announced
     * @param alertMessage The alert message
     * @param alertType The alert type
     */
    void criticalAlertAnnounced(const QString& alertMessage, const QString& alertType);

    /**
     * @brief Emitted when a database operation progress is announced
     * @param operation The operation
     * @param progress Progress percentage
     */
    void databaseProgressAnnounced(const QString& operation, int progress);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Process queued announcements
     */
    void processAnnouncementQueue();

    /**
     * @brief Handle progress update timer
     */
    void onProgressUpdateTimer();

    /**
     * @brief Handle critical alert repeat timer
     */
    void onCriticalAlertRepeatTimer();

private:
    /**
     * @brief Queued announcement item
     */
    struct QueuedAnnouncement {
        QString component;
        QString status;
        Priority priority;
        QString additionalInfo;
        ComponentType componentType;
        StatusType statusType;
        qint64 timestamp;
        bool isRepeating;
        int repeatCount;

        QueuedAnnouncement() 
            : priority(Priority::Normal)
            , componentType(ComponentType::SystemResource)
            , statusType(StatusType::InProgress)
            , timestamp(0)
            , isRepeating(false)
            , repeatCount(0) {}
    };

    /**
     * @brief Initialize announcement system
     */
    void initializeAnnouncementSystem();

    /**
     * @brief Shutdown announcement system
     */
    void shutdownAnnouncementSystem();

    /**
     * @brief Queue an announcement for processing
     * @param announcement The announcement to queue
     */
    void queueAnnouncement(const QueuedAnnouncement& announcement);

    /**
     * @brief Process a single announcement
     * @param announcement The announcement to process
     */
    void processAnnouncement(const QueuedAnnouncement& announcement);

    /**
     * @brief Format announcement message based on verbosity level
     * @param component The system component
     * @param status The status
     * @param additionalInfo Additional information
     * @param componentType Type of component
     * @param statusType Type of status
     * @return Formatted announcement message
     */
    QString formatAnnouncementMessage(const QString& component, const QString& status, 
                                     const QString& additionalInfo, ComponentType componentType, 
                                     StatusType statusType) const;

    /**
     * @brief Get priority for component and status type
     * @param componentType Type of component
     * @param statusType Type of status
     * @return Recommended priority level
     */
    Priority getPriorityForStatus(ComponentType componentType, StatusType statusType) const;

    /**
     * @brief Convert status string to StatusType enum
     * @param status Status string
     * @return Corresponding StatusType
     */
    StatusType parseStatusType(const QString& status) const;

    /**
     * @brief Check if announcement should be queued or processed immediately
     * @param priority Priority level
     * @return true if should be processed immediately
     */
    bool shouldProcessImmediately(Priority priority) const;

    /**
     * @brief Handle queue overflow by removing low-priority items
     * @return Number of items removed
     */
    int handleQueueOverflow();

    /**
     * @brief Load configuration from settings
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to settings
     */
    void saveConfiguration();

    /**
     * @brief Send announcement to live region manager
     * @param message The message to announce
     * @param priority Priority level
     * @param componentType Type of component
     */
    void sendToLiveRegionManager(const QString& message, Priority priority, ComponentType componentType);

    // Member variables
    AnnouncementConfig m_config;
    
    // Service dependencies
    LiveRegionManager* m_liveRegionManager;
    AccessibilityManager* m_accessibilityManager;
    AudioFeedbackService* m_audioFeedbackService;
    
    // Announcement queue management
    QQueue<QueuedAnnouncement> m_announcementQueue;
    QTimer* m_queueProcessingTimer;
    QTimer* m_progressUpdateTimer;
    QTimer* m_criticalAlertRepeatTimer;
    QMutex m_queueMutex;
    
    // State tracking
    bool m_isProcessingQueue;
    int m_currentVerbosityLevel;
    QueuedAnnouncement m_lastCriticalAlert;
    bool m_hasPendingCriticalAlert;
    
    // Statistics
    int m_announcementsProcessed;
    int m_criticalAlertsAnnounced;
    qint64 m_lastProgressUpdate;
    
    // Constants
    static constexpr int QUEUE_PROCESSING_INTERVAL_MS = 500;
    static constexpr int DEFAULT_PROGRESS_UPDATE_INTERVAL_MS = 5000;
    static constexpr int DEFAULT_CRITICAL_ALERT_REPEAT_INTERVAL_MS = 30000;
    static constexpr int MAX_QUEUED_ANNOUNCEMENTS = 20;
    static constexpr int MAX_CRITICAL_ALERT_REPEATS = 3;
};

Q_DECLARE_METATYPE(SystemStatusAnnouncer::ComponentType)
Q_DECLARE_METATYPE(SystemStatusAnnouncer::StatusType)
Q_DECLARE_METATYPE(SystemStatusAnnouncer::Priority)

#endif // SYSTEMSTATUSANNOUNCER_H