#ifndef AUDIOFEEDBACKSERVICE_H
#define AUDIOFEEDBACKSERVICE_H

#include "BaseService.h"
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QAccessible>

class AccessibilityManager;

/**
 * @brief Service for providing audio feedback and announcements to screen readers
 * 
 * The AudioFeedbackService handles immediate action confirmation, queued announcements,
 * and priority management for accessibility feedback. It integrates with Qt's accessibility
 * framework to provide announcements through ORCA and other assistive technologies.
 * 
 * This service manages announcement queuing with priority levels, ensuring critical
 * messages are delivered promptly while maintaining a smooth user experience.
 * 
 * @example
 * @code
 * auto* audioService = ServiceContainer::instance()->resolve<AudioFeedbackService>();
 * audioService->announceButtonClick("Save", "Configuration saved successfully");
 * audioService->announceError("Failed to save file: Permission denied");
 * @endcode
 * 
 * @see AccessibilityManager, BaseService
 * @since XFB 2.0
 */
class AudioFeedbackService : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Priority levels for audio announcements
     */
    enum class Priority {
        Low = 0,        ///< Background information, can be skipped
        Normal = 1,     ///< Standard user action feedback
        High = 2,       ///< Important notifications
        Critical = 3    ///< Emergency alerts that interrupt other announcements
    };

    /**
     * @brief Types of audio feedback
     */
    enum class FeedbackType {
        ButtonClick,        ///< Button activation feedback
        MenuSelection,      ///< Menu item selection feedback
        SaveOperation,      ///< File/data save confirmation
        LoadOperation,      ///< File/data load confirmation
        PlaybackChange,     ///< Media playback state change
        TrackChange,        ///< Current track change
        PlaylistModified,   ///< Playlist modification feedback
        DatabaseOperation,  ///< Database operation feedback
        ErrorMessage,       ///< Error notification
        ProgressUpdate,     ///< Long operation progress
        CriticalAlert,      ///< Critical system alert
        StatusChange        ///< General status change
    };

    /**
     * @brief Configuration for announcement behavior
     */
    struct AnnouncementConfig {
        bool enabled = true;                    ///< Whether announcements are enabled
        int maxQueueSize = 50;                  ///< Maximum number of queued announcements
        int processingIntervalMs = 100;         ///< Interval between announcement processing
        int criticalInterruptDelayMs = 50;      ///< Delay before critical announcements interrupt
        bool allowInterruption = true;         ///< Whether announcements can be interrupted
        int maxAnnouncementLengthChars = 500;   ///< Maximum length of a single announcement
    };

    explicit AudioFeedbackService(QObject* parent = nullptr);
    ~AudioFeedbackService() override;

    /**
     * @brief Announce button click with optional confirmation message
     * @param buttonText The text/label of the clicked button
     * @param confirmationMessage Optional confirmation message for the action
     */
    void announceButtonClick(const QString& buttonText, const QString& confirmationMessage = QString());

    /**
     * @brief Announce menu selection
     * @param menuPath The path to the selected menu item (e.g., "File > Open")
     * @param description Optional description of the menu action
     */
    void announceMenuSelection(const QString& menuPath, const QString& description = QString());

    /**
     * @brief Announce save operation completion
     * @param itemType The type of item saved (e.g., "playlist", "configuration")
     * @param itemName The name of the saved item
     * @param success Whether the save operation was successful
     */
    void announceSaveOperation(const QString& itemType, const QString& itemName, bool success = true);

    /**
     * @brief Announce load operation completion
     * @param itemType The type of item loaded
     * @param itemName The name of the loaded item
     * @param success Whether the load operation was successful
     */
    void announceLoadOperation(const QString& itemType, const QString& itemName, bool success = true);

    /**
     * @brief Announce playback state change
     * @param newState The new playback state (e.g., "playing", "paused", "stopped")
     * @param trackInfo Optional current track information
     */
    void announcePlaybackStateChange(const QString& newState, const QString& trackInfo = QString());

    /**
     * @brief Announce track change
     * @param trackTitle The title of the new track
     * @param artist Optional artist name
     * @param duration Optional track duration
     */
    void announceTrackChange(const QString& trackTitle, const QString& artist = QString(), const QString& duration = QString());

    /**
     * @brief Announce playlist modification
     * @param action The action performed (e.g., "added", "removed", "moved")
     * @param itemCount Number of items affected
     * @param playlistName Name of the affected playlist
     */
    void announcePlaylistModification(const QString& action, int itemCount, const QString& playlistName);

    /**
     * @brief Announce database operation
     * @param operation The database operation (e.g., "scan", "update", "backup")
     * @param status Current status or completion message
     * @param isComplete Whether the operation is complete
     */
    void announceDatabaseOperation(const QString& operation, const QString& status, bool isComplete = false);

    /**
     * @brief Announce error message
     * @param errorMessage The error message to announce
     * @param context Optional context where the error occurred
     */
    void announceError(const QString& errorMessage, const QString& context = QString());

    /**
     * @brief Announce progress update for long-running operations
     * @param operation The operation in progress
     * @param percentage Progress percentage (0-100)
     * @param estimatedTimeRemaining Optional estimated time remaining
     */
    void announceProgressUpdate(const QString& operation, int percentage, const QString& estimatedTimeRemaining = QString());

    /**
     * @brief Announce critical alert that should interrupt other announcements
     * @param alertMessage The critical alert message
     * @param requiresImmediateAttention Whether this requires immediate user attention
     */
    void announceCriticalAlert(const QString& alertMessage, bool requiresImmediateAttention = true);

    /**
     * @brief Announce general status change
     * @param component The component whose status changed
     * @param newStatus The new status
     * @param additionalInfo Optional additional information
     */
    void announceStatusChange(const QString& component, const QString& newStatus, const QString& additionalInfo = QString());

    /**
     * @brief Queue a custom announcement with specified priority
     * @param message The message to announce
     * @param priority Priority level of the announcement
     * @param feedbackType Type of feedback for categorization
     */
    void queueAnnouncement(const QString& message, Priority priority = Priority::Normal, FeedbackType feedbackType = FeedbackType::StatusChange);

    /**
     * @brief Clear all queued announcements
     * @param preserveCritical Whether to preserve critical announcements
     */
    void clearAnnouncementQueue(bool preserveCritical = true);

    /**
     * @brief Get the current announcement configuration
     * @return Current announcement configuration
     */
    AnnouncementConfig getConfig() const { return m_config; }

    /**
     * @brief Set announcement configuration
     * @param config New configuration to apply
     */
    void setConfig(const AnnouncementConfig& config);

    /**
     * @brief Check if announcements are currently enabled
     * @return true if announcements are enabled
     */
    bool isEnabled() const { return m_config.enabled; }

    /**
     * @brief Enable or disable announcements
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Get the current queue size
     * @return Number of announcements currently queued
     */
    int getQueueSize() const;

    /**
     * @brief Check if the service is currently processing announcements
     * @return true if processing announcements
     */
    bool isProcessing() const { return m_isProcessing; }

public slots:
    /**
     * @brief Handle accessibility manager verbosity level changes
     * @param level New verbosity level
     */
    void onVerbosityLevelChanged(int level);

signals:
    /**
     * @brief Emitted when an announcement is about to be delivered
     * @param message The announcement message
     * @param priority Priority level
     * @param feedbackType Type of feedback
     */
    void announcementDelivered(const QString& message, Priority priority, FeedbackType feedbackType);

    /**
     * @brief Emitted when the announcement queue becomes full
     * @param droppedCount Number of announcements dropped
     */
    void queueOverflow(int droppedCount);

    /**
     * @brief Emitted when a critical announcement interrupts the queue
     * @param message The critical message
     */
    void criticalAnnouncementInterrupted(const QString& message);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Process the next announcement in the queue
     */
    void processNextAnnouncement();

    /**
     * @brief Handle accessibility manager state changes
     * @param enabled Whether accessibility is enabled
     */
    void onAccessibilityStateChanged(bool enabled);

private:
    /**
     * @brief Announcement item for queue management
     */
    struct AnnouncementItem {
        QString message;
        Priority priority;
        FeedbackType feedbackType;
        qint64 timestamp;
        bool isProcessed;

        AnnouncementItem() : priority(Priority::Normal), feedbackType(FeedbackType::StatusChange), timestamp(0), isProcessed(false) {}
        
        AnnouncementItem(const QString& msg, Priority prio, FeedbackType type)
            : message(msg), priority(prio), feedbackType(type), timestamp(QDateTime::currentMSecsSinceEpoch()), isProcessed(false) {}

        bool operator<(const AnnouncementItem& other) const {
            // Higher priority first, then older timestamp
            if (priority != other.priority) {
                return static_cast<int>(priority) < static_cast<int>(other.priority);
            }
            return timestamp > other.timestamp;
        }
    };

    /**
     * @brief Initialize the announcement processing system
     */
    void initializeAnnouncementSystem();

    /**
     * @brief Shutdown the announcement processing system
     */
    void shutdownAnnouncementSystem();

    /**
     * @brief Add an announcement to the queue with priority handling
     * @param item The announcement item to add
     */
    void addToQueue(const AnnouncementItem& item);

    /**
     * @brief Process a single announcement item
     * @param item The announcement item to process
     */
    void processAnnouncementItem(const AnnouncementItem& item);

    /**
     * @brief Send announcement to the accessibility framework
     * @param message The message to send
     * @param priority Priority level for delivery
     */
    void sendToAccessibilityFramework(const QString& message, Priority priority);

    /**
     * @brief Format message based on current verbosity level
     * @param message The raw message
     * @param feedbackType Type of feedback for context
     * @return Formatted message appropriate for current verbosity
     */
    QString formatMessageForVerbosity(const QString& message, FeedbackType feedbackType) const;

    /**
     * @brief Handle queue overflow by removing low-priority items
     * @return Number of items removed
     */
    int handleQueueOverflow();

    /**
     * @brief Check if an announcement should interrupt current processing
     * @param priority Priority of the new announcement
     * @return true if it should interrupt
     */
    bool shouldInterruptForPriority(Priority priority) const;

    /**
     * @brief Load configuration from settings
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to settings
     */
    void saveConfiguration();

    // Member variables
    AnnouncementConfig m_config;
    QQueue<AnnouncementItem> m_announcementQueue;
    QTimer* m_processingTimer;
    QMutex m_queueMutex;
    bool m_isProcessing;
    bool m_isInitialized;
    AccessibilityManager* m_accessibilityManager;
    
    // Statistics and monitoring
    qint64 m_lastAnnouncementTime;
    int m_announcementsProcessed;
    int m_announcementsDropped;
    
    // Verbosity level cache
    int m_currentVerbosityLevel;
    
    // Constants
    static constexpr int DEFAULT_PROCESSING_INTERVAL_MS = 100;
    static constexpr int DEFAULT_MAX_QUEUE_SIZE = 50;
    static constexpr int CRITICAL_INTERRUPT_DELAY_MS = 50;
    static constexpr int MAX_ANNOUNCEMENT_LENGTH = 500;
};

Q_DECLARE_METATYPE(AudioFeedbackService::Priority)
Q_DECLARE_METATYPE(AudioFeedbackService::FeedbackType)

#endif // AUDIOFEEDBACKSERVICE_H