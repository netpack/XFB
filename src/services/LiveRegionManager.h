#ifndef LIVEREGIONMANAGER_H
#define LIVEREGIONMANAGER_H

#include "BaseService.h"
#include <QObject>
#include <QWidget>
#include <QAccessible>
#include <QHash>
#include <QTimer>
#include <QQueue>
#include <QMutex>

class AccessibilityManager;
class AudioFeedbackService;

/**
 * @brief Manager for live regions and real-time content updates in accessibility
 * 
 * The LiveRegionManager handles dynamic content announcements through Qt's accessibility
 * framework, managing live regions for real-time updates like playback status, time
 * announcements, and system notifications. It provides throttling and optimization
 * to prevent announcement flooding while ensuring critical updates are delivered promptly.
 * 
 * This service integrates with ORCA screen reader through QAccessible::LiveRegion
 * functionality and coordinates with AudioFeedbackService for announcement delivery.
 * 
 * @example
 * @code
 * auto* liveManager = ServiceContainer::instance()->resolve<LiveRegionManager>();
 * liveManager->createLiveRegion(statusWidget, QAccessible::LiveRegion::Polite);
 * liveManager->updateLiveRegion(statusWidget, "Now playing: Track Title");
 * liveManager->announcePlaybackChange("Track Title", "Artist Name", "3:45");
 * @endcode
 * 
 * @see AccessibilityManager, AudioFeedbackService, BaseService
 * @since XFB 2.0
 */
class LiveRegionManager : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Types of live region content updates
     */
    enum class UpdateType {
        PlaybackStatus,     ///< Current playback state and track info
        TimeUpdate,         ///< Current playback time and position
        SystemStatus,       ///< System status changes (recording, streaming)
        DatabaseProgress,   ///< Database operation progress
        QueueUpdate,        ///< Playlist/queue modifications
        CriticalAlert,      ///< Critical system alerts
        CountdownTimer,     ///< Live broadcasting countdown
        GeneralStatus       ///< General status updates
    };

    /**
     * @brief Configuration for live region behavior
     */
    struct LiveRegionConfig {
        bool enabled = true;                        ///< Whether live regions are enabled
        int updateThrottleMs = 1000;               ///< Minimum time between updates for same region
        int timeUpdateIntervalMs = 5000;           ///< Interval for time-based announcements
        int countdownThresholdSeconds = 30;        ///< Threshold for countdown announcements
        bool allowTimeUpdatesOnDemand = true;     ///< Allow manual time update requests
        int maxPendingUpdates = 20;                ///< Maximum pending updates per region
        bool prioritizeCriticalUpdates = true;     ///< Whether critical updates bypass throttling
    };

    /**
     * @brief Live region types (Qt6 compatibility)
     */
    enum class LiveRegionType {
        None = 0,
        Polite = 1,
        Assertive = 2
    };

    /**
     * @brief Information about a registered live region
     */
    struct LiveRegionInfo {
        QWidget* widget;
        LiveRegionType type;
        UpdateType updateType;
        QString lastContent;
        qint64 lastUpdateTime;
        int pendingUpdates;
        bool isActive;

        LiveRegionInfo() 
            : widget(nullptr)
            , type(LiveRegionType::None)
            , updateType(UpdateType::GeneralStatus)
            , lastUpdateTime(0)
            , pendingUpdates(0)
            , isActive(true) {}
    };

    explicit LiveRegionManager(QObject* parent = nullptr);
    ~LiveRegionManager() override;

    /**
     * @brief Create and register a live region for a widget
     * @param widget The widget to make into a live region
     * @param type The type of live region (Polite, Assertive, Off)
     * @param updateType The type of content updates this region will handle
     * @return true if the live region was created successfully
     */
    bool createLiveRegion(QWidget* widget, LiveRegionType type, UpdateType updateType = UpdateType::GeneralStatus);

    /**
     * @brief Remove a live region registration
     * @param widget The widget to remove from live region management
     */
    void removeLiveRegion(QWidget* widget);

    /**
     * @brief Update the content of a live region
     * @param widget The widget whose live region to update
     * @param content The new content to announce
     * @param forceUpdate Whether to bypass throttling for this update
     */
    void updateLiveRegion(QWidget* widget, const QString& content, bool forceUpdate = false);

    /**
     * @brief Announce playback status change with track information
     * @param trackTitle The title of the current track
     * @param artist Optional artist name
     * @param duration Optional track duration
     * @param playbackState Current playback state (playing, paused, stopped)
     */
    void announcePlaybackChange(const QString& trackTitle, const QString& artist = QString(), 
                               const QString& duration = QString(), const QString& playbackState = QString());

    /**
     * @brief Announce current playback time
     * @param currentTime Current playback position
     * @param totalTime Total track duration
     * @param remainingTime Optional remaining time
     */
    void announceTimeUpdate(const QString& currentTime, const QString& totalTime = QString(), 
                           const QString& remainingTime = QString());

    /**
     * @brief Announce system status change
     * @param component The system component (e.g., "recorder", "streamer")
     * @param status The new status
     * @param additionalInfo Optional additional information
     */
    void announceSystemStatus(const QString& component, const QString& status, const QString& additionalInfo = QString());

    /**
     * @brief Announce database operation progress
     * @param operation The operation being performed
     * @param progress Progress percentage (0-100)
     * @param itemsProcessed Optional number of items processed
     * @param totalItems Optional total number of items
     */
    void announceDatabaseProgress(const QString& operation, int progress, 
                                 int itemsProcessed = -1, int totalItems = -1);

    /**
     * @brief Announce queue or playlist update
     * @param queueName Name of the queue/playlist
     * @param action Action performed (added, removed, moved, cleared)
     * @param itemCount Number of items affected
     * @param totalItems Optional total items in queue
     */
    void announceQueueUpdate(const QString& queueName, const QString& action, 
                            int itemCount, int totalItems = -1);

    /**
     * @brief Announce critical alert that requires immediate attention
     * @param alertMessage The critical alert message
     * @param alertType Optional type of alert for categorization
     */
    void announceCriticalAlert(const QString& alertMessage, const QString& alertType = QString());

    /**
     * @brief Announce countdown for live broadcasting
     * @param secondsRemaining Seconds remaining until broadcast
     * @param eventDescription Optional description of the upcoming event
     */
    void announceCountdown(int secondsRemaining, const QString& eventDescription = QString());

    /**
     * @brief Request immediate time announcement (typically triggered by keyboard shortcut)
     * @param includeRemaining Whether to include remaining time information
     */
    void requestTimeAnnouncement(bool includeRemaining = true);

    /**
     * @brief Get the current configuration
     * @return Current live region configuration
     */
    LiveRegionConfig getConfig() const { return m_config; }

    /**
     * @brief Set live region configuration
     * @param config New configuration to apply
     */
    void setConfig(const LiveRegionConfig& config);

    /**
     * @brief Check if live regions are currently enabled
     * @return true if live regions are enabled
     */
    bool isEnabled() const { return m_config.enabled; }

    /**
     * @brief Enable or disable live regions
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Get information about all registered live regions
     * @return List of live region information
     */
    QList<LiveRegionInfo> getLiveRegions() const;

    /**
     * @brief Get the number of registered live regions
     * @return Number of active live regions
     */
    int getLiveRegionCount() const;

    /**
     * @brief Clear all pending updates for a specific region
     * @param widget The widget whose pending updates to clear
     */
    void clearPendingUpdates(QWidget* widget);

    /**
     * @brief Clear all pending updates for all regions
     */
    void clearAllPendingUpdates();

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
     * @brief Emitted when a live region is updated
     * @param widget The widget whose live region was updated
     * @param content The new content
     * @param updateType Type of update
     */
    void liveRegionUpdated(QWidget* widget, const QString& content, UpdateType updateType);

    /**
     * @brief Emitted when a live region update is throttled
     * @param widget The widget whose update was throttled
     * @param content The content that was throttled
     */
    void updateThrottled(QWidget* widget, const QString& content);

    /**
     * @brief Emitted when a critical announcement is made
     * @param message The critical message
     */
    void criticalAnnouncementMade(const QString& message);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Process pending live region updates
     */
    void processPendingUpdates();

    /**
     * @brief Handle widget destruction
     * @param obj The destroyed widget
     */
    void onWidgetDestroyed(QObject* obj);

    /**
     * @brief Handle automatic time updates
     */
    void onTimeUpdateTimer();

private:
    /**
     * @brief Pending update item for throttling management
     */
    struct PendingUpdate {
        QWidget* widget;
        QString content;
        UpdateType updateType;
        qint64 timestamp;
        bool isCritical;

        PendingUpdate() : widget(nullptr), updateType(UpdateType::GeneralStatus), timestamp(0), isCritical(false) {}
        
        PendingUpdate(QWidget* w, const QString& c, UpdateType t, bool critical = false)
            : widget(w), content(c), updateType(t), timestamp(QDateTime::currentMSecsSinceEpoch()), isCritical(critical) {}
    };

    /**
     * @brief Initialize live region management system
     */
    void initializeLiveRegionSystem();

    /**
     * @brief Shutdown live region management system
     */
    void shutdownLiveRegionSystem();

    /**
     * @brief Check if an update should be throttled
     * @param widget The widget to check
     * @param updateType Type of update
     * @param isCritical Whether this is a critical update
     * @return true if the update should be throttled
     */
    bool shouldThrottleUpdate(QWidget* widget, UpdateType updateType, bool isCritical) const;

    /**
     * @brief Add an update to the pending queue
     * @param update The update to add
     */
    void addPendingUpdate(const PendingUpdate& update);

    /**
     * @brief Process a single live region update
     * @param widget The widget to update
     * @param content The content to announce
     * @param updateType Type of update
     */
    void processLiveRegionUpdate(QWidget* widget, const QString& content, UpdateType updateType);

    /**
     * @brief Send update to Qt accessibility framework
     * @param widget The widget whose live region to update
     * @param content The content to announce
     */
    void sendToAccessibilityFramework(QWidget* widget, const QString& content);

    /**
     * @brief Format content based on update type and verbosity
     * @param content The raw content
     * @param updateType Type of update
     * @return Formatted content appropriate for announcement
     */
    QString formatContentForAnnouncement(const QString& content, UpdateType updateType) const;

    /**
     * @brief Get the appropriate live region type for an update type
     * @param updateType The type of update
     * @return Recommended live region type
     */
    LiveRegionType getRecommendedLiveRegionType(UpdateType updateType) const;

    /**
     * @brief Load configuration from settings
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to settings
     */
    void saveConfiguration();

    /**
     * @brief Clean up expired pending updates
     */
    void cleanupExpiredUpdates();

    // Member variables
    LiveRegionConfig m_config;
    QHash<QWidget*, LiveRegionInfo> m_liveRegions;
    QQueue<PendingUpdate> m_pendingUpdates;
    QTimer* m_updateTimer;
    QTimer* m_timeUpdateTimer;
    QMutex m_updateMutex;
    
    // Service dependencies
    AccessibilityManager* m_accessibilityManager;
    AudioFeedbackService* m_audioFeedbackService;
    
    // State tracking
    bool m_isProcessingUpdates;
    int m_currentVerbosityLevel;
    QString m_lastPlaybackInfo;
    QString m_lastTimeInfo;
    
    // Statistics
    int m_updatesProcessed;
    int m_updatesThrottled;
    qint64 m_lastTimeAnnouncementRequest;
    
    // Constants
    static constexpr int DEFAULT_UPDATE_THROTTLE_MS = 1000;
    static constexpr int DEFAULT_TIME_UPDATE_INTERVAL_MS = 5000;
    static constexpr int DEFAULT_COUNTDOWN_THRESHOLD_SECONDS = 30;
    static constexpr int MAX_PENDING_UPDATES_PER_REGION = 20;
    static constexpr int UPDATE_PROCESSING_INTERVAL_MS = 200;
    static constexpr int EXPIRED_UPDATE_CLEANUP_MS = 10000;
};

Q_DECLARE_METATYPE(LiveRegionManager::UpdateType)

#endif // LIVEREGIONMANAGER_H