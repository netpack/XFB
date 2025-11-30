#ifndef PLAYERAUDIOFEEDBACKINTEGRATION_H
#define PLAYERAUDIOFEEDBACKINTEGRATION_H

#include <QObject>
#include <QtMultimedia/QMediaPlayer>
#include <QTimer>

class AudioFeedbackService;
class player;
class PlayerUIController;

/**
 * @brief Integration class for player audio feedback
 * 
 * This class connects player operations to the AudioFeedbackService to provide
 * accessibility announcements for playback state changes, track changes, and
 * playlist modifications. It monitors player events and translates them into
 * appropriate audio feedback for screen readers.
 * 
 * @example
 * @code
 * auto* integration = new PlayerAudioFeedbackIntegration(playerWindow, this);
 * integration->initialize();
 * @endcode
 * 
 * @see AudioFeedbackService, player, PlayerUIController
 * @since XFB 2.0
 */
class PlayerAudioFeedbackIntegration : public QObject
{
    Q_OBJECT

public:
    explicit PlayerAudioFeedbackIntegration(player* playerWindow, QObject* parent = nullptr);
    ~PlayerAudioFeedbackIntegration();

    /**
     * @brief Initialize the audio feedback integration
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the integration
     */
    void shutdown();

    /**
     * @brief Check if the integration is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

public slots:
    /**
     * @brief Handle playback state changes
     * @param state New playback state
     */
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

    /**
     * @brief Handle media changes (track changes)
     * @param media New media URL
     */
    void onMediaChanged(const QUrl& media);

    /**
     * @brief Handle volume changes
     * @param volume New volume level (0-100)
     */
    void onVolumeChanged(int volume);

    /**
     * @brief Handle position changes for time announcements
     * @param position Current position in milliseconds
     */
    void onPositionChanged(qint64 position);

    /**
     * @brief Handle duration changes
     * @param duration Total duration in milliseconds
     */
    void onDurationChanged(qint64 duration);

    /**
     * @brief Handle playlist modifications
     * @param action The action performed (added, removed, moved, cleared)
     * @param itemCount Number of items affected
     * @param playlistName Name of the playlist (if applicable)
     */
    void onPlaylistModified(const QString& action, int itemCount, const QString& playlistName = QString());

    /**
     * @brief Handle playlist save operations
     * @param playlistName Name of the saved playlist
     * @param success Whether the save was successful
     */
    void onPlaylistSaved(const QString& playlistName, bool success);

    /**
     * @brief Handle playlist load operations
     * @param playlistName Name of the loaded playlist
     * @param success Whether the load was successful
     */
    void onPlaylistLoaded(const QString& playlistName, bool success);

    /**
     * @brief Handle music import operations
     * @param importedCount Number of tracks imported
     * @param success Whether the import was successful
     */
    void onMusicImported(int importedCount, bool success);

    /**
     * @brief Handle player errors
     * @param error Error type
     * @param errorString Error description
     */
    void onPlayerError(QMediaPlayer::Error error, const QString& errorString);

    /**
     * @brief Handle button clicks with audio feedback
     * @param buttonName Name of the clicked button
     * @param action Optional action description
     */
    void onButtonClicked(const QString& buttonName, const QString& action = QString());

    /**
     * @brief Handle menu selections
     * @param menuPath Path to the selected menu item
     * @param description Optional description
     */
    void onMenuSelected(const QString& menuPath, const QString& description = QString());

    /**
     * @brief Announce current track information on request
     */
    void announceCurrentTrack();

    /**
     * @brief Announce current playback time on request
     */
    void announceCurrentTime();

    /**
     * @brief Announce remaining time on request
     */
    void announceRemainingTime();

    /**
     * @brief Announce playlist information
     */
    void announcePlaylistInfo();

private slots:
    /**
     * @brief Handle periodic time announcements
     */
    void onTimeAnnouncementTimer();

    /**
     * @brief Handle track ending countdown
     */
    void onTrackEndingCountdown();

private:
    /**
     * @brief Connect to player signals
     */
    void connectPlayerSignals();

    /**
     * @brief Connect to UI controller signals
     */
    void connectUIControllerSignals();

    /**
     * @brief Disconnect all signals
     */
    void disconnectSignals();

    /**
     * @brief Get track information from URL
     * @param url Media URL
     * @return Formatted track information
     */
    QString getTrackInfoFromUrl(const QUrl& url);

    /**
     * @brief Get track title from database
     * @param filePath File path of the track
     * @return Track title, or filename if not found in database
     */
    QString getTrackTitleFromDatabase(const QString& filePath);

    /**
     * @brief Get artist name from database
     * @param filePath File path of the track
     * @return Artist name, or empty string if not found
     */
    QString getArtistFromDatabase(const QString& filePath);

    /**
     * @brief Format duration for announcement
     * @param milliseconds Duration in milliseconds
     * @return Formatted duration string
     */
    QString formatDuration(qint64 milliseconds);

    /**
     * @brief Format playback state for announcement
     * @param state Playback state
     * @return Human-readable state string
     */
    QString formatPlaybackState(QMediaPlayer::PlaybackState state);

    /**
     * @brief Check if track is approaching end (for countdown announcements)
     * @param position Current position
     * @param duration Total duration
     * @return true if track is approaching end
     */
    bool isTrackApproachingEnd(qint64 position, qint64 duration);

    /**
     * @brief Get remaining time in seconds
     * @param position Current position
     * @param duration Total duration
     * @return Remaining time in seconds
     */
    int getRemainingSeconds(qint64 position, qint64 duration);

    /**
     * @brief Setup countdown timer for track ending
     */
    void setupTrackEndingCountdown();

    /**
     * @brief Stop countdown timer
     */
    void stopTrackEndingCountdown();

private:
    player* m_playerWindow;
    PlayerUIController* m_playerUIController;
    AudioFeedbackService* m_audioFeedbackService;
    
    bool m_initialized;
    bool m_trackEndingCountdownActive;
    
    // Current state tracking
    QMediaPlayer::PlaybackState m_currentPlaybackState;
    QUrl m_currentMedia;
    qint64 m_currentPosition;
    qint64 m_currentDuration;
    int m_currentVolume;
    QString m_currentTrackTitle;
    QString m_currentArtist;
    
    // Timers for announcements
    QTimer* m_timeAnnouncementTimer;
    QTimer* m_trackEndingTimer;
    
    // Configuration
    static constexpr int TIME_ANNOUNCEMENT_INTERVAL_MS = 30000; // 30 seconds
    static constexpr int TRACK_ENDING_COUNTDOWN_SECONDS = 30;   // 30 seconds before end
    static constexpr int COUNTDOWN_ANNOUNCEMENT_INTERVAL = 10;   // Every 10 seconds during countdown
};

#endif // PLAYERAUDIOFEEDBACKINTEGRATION_H