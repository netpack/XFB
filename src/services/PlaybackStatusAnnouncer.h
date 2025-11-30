#ifndef PLAYBACKSTATUSANNOUNCER_H
#define PLAYBACKSTATUSANNOUNCER_H

#include "BaseService.h"
#include <QObject>
#include <QTimer>
#include <QtMultimedia/QMediaPlayer>
#include <QKeySequence>
#include <QShortcut>

class LiveRegionManager;
class AccessibilityManager;
class player;

/**
 * @brief Service for announcing playback status and time information
 * 
 * The PlaybackStatusAnnouncer integrates with the LiveRegionManager to provide
 * real-time announcements of playback status, track changes, and time updates.
 * It supports keyboard shortcuts for on-demand time announcements and countdown
 * functionality for live broadcasting scenarios.
 * 
 * This service monitors the player state and provides appropriate announcements
 * based on the current accessibility verbosity level and user preferences.
 * 
 * @example
 * @code
 * auto* announcer = ServiceContainer::instance()->resolve<PlaybackStatusAnnouncer>();
 * announcer->connectToPlayer(playerInstance);
 * announcer->setTimeAnnouncementShortcut(QKeySequence("Ctrl+T"));
 * @endcode
 * 
 * @see LiveRegionManager, AccessibilityManager, BaseService
 * @since XFB 2.0
 */
class PlaybackStatusAnnouncer : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Configuration for playback announcements
     */
    struct AnnouncementConfig {
        bool trackChangeEnabled = true;             ///< Announce track changes
        bool playbackStateEnabled = true;           ///< Announce play/pause/stop
        bool timeUpdatesEnabled = false;            ///< Automatic time updates (usually disabled)
        bool countdownEnabled = true;               ///< Countdown announcements for broadcasting
        int countdownThresholdSeconds = 30;         ///< Threshold for countdown announcements
        int timeUpdateIntervalSeconds = 30;         ///< Interval for automatic time updates
        bool includeArtistInAnnouncements = true;   ///< Include artist name in track announcements
        bool includeDurationInAnnouncements = true; ///< Include duration in track announcements
        bool announceRemainingTime = true;          ///< Include remaining time in time announcements
    };

    explicit PlaybackStatusAnnouncer(QObject* parent = nullptr);
    ~PlaybackStatusAnnouncer() override;

    /**
     * @brief Connect to a player instance for monitoring
     * @param playerInstance The player instance to monitor
     * @return true if connection was successful
     */
    bool connectToPlayer(player* playerInstance);

    /**
     * @brief Disconnect from the current player instance
     */
    void disconnectFromPlayer();

    /**
     * @brief Set keyboard shortcut for time announcements
     * @param shortcut The keyboard shortcut to use
     * @return true if shortcut was set successfully
     */
    bool setTimeAnnouncementShortcut(const QKeySequence& shortcut);

    /**
     * @brief Get the current time announcement shortcut
     * @return Current keyboard shortcut
     */
    QKeySequence getTimeAnnouncementShortcut() const { return m_timeAnnouncementShortcut; }

    /**
     * @brief Manually announce current playback time
     * @param includeRemaining Whether to include remaining time
     */
    void announceCurrentTime(bool includeRemaining = true);

    /**
     * @brief Start countdown announcements for live broadcasting
     * @param targetTime The target time for the countdown
     * @param eventDescription Optional description of the event
     */
    void startCountdown(const QDateTime& targetTime, const QString& eventDescription = QString());

    /**
     * @brief Stop countdown announcements
     */
    void stopCountdown();

    /**
     * @brief Check if countdown is currently active
     * @return true if countdown is active
     */
    bool isCountdownActive() const { return m_countdownActive; }

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
     * @brief Enable or disable track change announcements
     * @param enabled true to enable, false to disable
     */
    void setTrackChangeAnnouncementsEnabled(bool enabled);

    /**
     * @brief Enable or disable playback state announcements
     * @param enabled true to enable, false to disable
     */
    void setPlaybackStateAnnouncementsEnabled(bool enabled);

    /**
     * @brief Enable or disable automatic time updates
     * @param enabled true to enable, false to disable
     */
    void setTimeUpdatesEnabled(bool enabled);

    /**
     * @brief Get information about the currently playing track
     * @return Track information string
     */
    QString getCurrentTrackInfo() const;

    /**
     * @brief Get current playback time information
     * @param includeRemaining Whether to include remaining time
     * @return Time information string
     */
    QString getCurrentTimeInfo(bool includeRemaining = true) const;

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
     * @brief Emitted when a track change is announced
     * @param trackInfo Information about the new track
     */
    void trackChangeAnnounced(const QString& trackInfo);

    /**
     * @brief Emitted when playback state is announced
     * @param state The new playback state
     */
    void playbackStateAnnounced(const QString& state);

    /**
     * @brief Emitted when time information is announced
     * @param timeInfo The time information
     */
    void timeInfoAnnounced(const QString& timeInfo);

    /**
     * @brief Emitted when countdown reaches zero
     * @param eventDescription Description of the event
     */
    void countdownCompleted(const QString& eventDescription);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Handle player state changes
     * @param state New player state
     */
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);

    /**
     * @brief Handle media changes
     * @param media New media content
     */
    void onMediaChanged(const QUrl& media);

    /**
     * @brief Handle position changes
     * @param position New position in milliseconds
     */
    void onPositionChanged(qint64 position);

    /**
     * @brief Handle duration changes
     * @param duration New duration in milliseconds
     */
    void onDurationChanged(qint64 duration);

    /**
     * @brief Handle automatic time updates
     */
    void onTimeUpdateTimer();

    /**
     * @brief Handle countdown timer
     */
    void onCountdownTimer();

    /**
     * @brief Handle time announcement shortcut activation
     */
    void onTimeAnnouncementShortcutActivated();

private:
    /**
     * @brief Initialize playback monitoring
     */
    void initializePlaybackMonitoring();

    /**
     * @brief Shutdown playback monitoring
     */
    void shutdownPlaybackMonitoring();

    /**
     * @brief Setup keyboard shortcuts
     */
    void setupKeyboardShortcuts();

    /**
     * @brief Cleanup keyboard shortcuts
     */
    void cleanupKeyboardShortcuts();

    /**
     * @brief Get track information from file path
     * @param filePath Path to the media file
     * @return Track information structure
     */
    struct TrackInfo {
        QString title;
        QString artist;
        QString album;
        QString duration;
        QString filePath;
    };
    TrackInfo getTrackInfoFromPath(const QString& filePath) const;

    /**
     * @brief Format track information for announcement
     * @param trackInfo Track information to format
     * @param verbosityLevel Current verbosity level
     * @return Formatted announcement string
     */
    QString formatTrackAnnouncement(const TrackInfo& trackInfo, int verbosityLevel) const;

    /**
     * @brief Format playback state for announcement
     * @param state Playback state
     * @param trackInfo Optional current track info
     * @return Formatted announcement string
     */
    QString formatPlaybackStateAnnouncement(QMediaPlayer::PlaybackState state, const TrackInfo& trackInfo = TrackInfo()) const;

    /**
     * @brief Format time information for announcement
     * @param currentMs Current position in milliseconds
     * @param totalMs Total duration in milliseconds
     * @param includeRemaining Whether to include remaining time
     * @param verbosityLevel Current verbosity level
     * @return Formatted time announcement string
     */
    QString formatTimeAnnouncement(qint64 currentMs, qint64 totalMs, bool includeRemaining, int verbosityLevel) const;

    /**
     * @brief Convert milliseconds to readable time string
     * @param milliseconds Time in milliseconds
     * @return Formatted time string (e.g., "3:45", "1:23:45")
     */
    QString formatTime(qint64 milliseconds) const;

    /**
     * @brief Calculate remaining time for countdown
     * @return Seconds remaining until target time
     */
    int getCountdownSecondsRemaining() const;

    /**
     * @brief Load configuration from settings
     */
    void loadConfiguration();

    /**
     * @brief Save configuration to settings
     */
    void saveConfiguration();

    /**
     * @brief Install global keyboard shortcut
     * @param shortcut The shortcut to install
     * @return true if installation was successful
     */
    bool installGlobalShortcut(const QKeySequence& shortcut);

    /**
     * @brief Uninstall global keyboard shortcut
     */
    void uninstallGlobalShortcut();

    // Member variables
    AnnouncementConfig m_config;
    
    // Service dependencies
    LiveRegionManager* m_liveRegionManager;
    AccessibilityManager* m_accessibilityManager;
    
    // Player monitoring
    player* m_playerInstance;
    QMediaPlayer* m_mediaPlayer;
    
    // Current state tracking
    QMediaPlayer::PlaybackState m_currentState;
    QUrl m_currentMedia;
    qint64 m_currentPosition;
    qint64 m_currentDuration;
    TrackInfo m_currentTrackInfo;
    int m_currentVerbosityLevel;
    
    // Timers
    QTimer* m_timeUpdateTimer;
    QTimer* m_countdownTimer;
    
    // Countdown functionality
    bool m_countdownActive;
    QDateTime m_countdownTargetTime;
    QString m_countdownEventDescription;
    int m_lastAnnouncedCountdown;
    
    // Keyboard shortcuts
    QKeySequence m_timeAnnouncementShortcut;
    QShortcut* m_globalTimeShortcut;
    
    // State management
    bool m_isInitialized;
    bool m_playerConnected;
    qint64 m_lastTimeAnnouncement;
    
    // Constants
    static constexpr int DEFAULT_TIME_UPDATE_INTERVAL_SECONDS = 30;
    static constexpr int DEFAULT_COUNTDOWN_THRESHOLD_SECONDS = 30;
    static constexpr int COUNTDOWN_TIMER_INTERVAL_MS = 1000;
    static constexpr int MIN_TIME_ANNOUNCEMENT_INTERVAL_MS = 1000;
};

#endif // PLAYBACKSTATUSANNOUNCER_H