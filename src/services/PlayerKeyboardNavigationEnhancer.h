#ifndef PLAYERKEYBOARDNAVIGATIONENHANCER_H
#define PLAYERKEYBOARDNAVIGATIONENHANCER_H

#include <QObject>
#include <QWidget>
#include <QKeyEvent>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>

class KeyboardNavigationController;
class AccessibilityManager;
class player;

/**
 * @brief Enhances the main player interface with comprehensive keyboard navigation
 * 
 * This class provides specialized keyboard navigation enhancements for the XFB
 * player interface, including playback controls, playlist navigation, and
 * volume/seek controls. It integrates with the KeyboardNavigationController
 * to provide a seamless keyboard-only experience for visually impaired users.
 * 
 * Key features:
 * - Enhanced playback controls with keyboard accessibility
 * - Playlist navigation with arrow keys and shortcuts
 * - Volume and seek controls via keyboard
 * - Context-aware navigation announcements
 * - Integration with ORCA screen reader
 * 
 * @see KeyboardNavigationController, AccessibilityManager
 * @since XFB 2.0
 */
class PlayerKeyboardNavigationEnhancer : public QObject
{
    Q_OBJECT

public:
    explicit PlayerKeyboardNavigationEnhancer(player* playerWindow, 
                                             KeyboardNavigationController* navigationController,
                                             AccessibilityManager* accessibilityManager,
                                             QObject* parent = nullptr);
    ~PlayerKeyboardNavigationEnhancer() override;

    /**
     * @brief Initialize the player keyboard navigation enhancements
     * @return true if initialization was successful
     */
    bool initialize();

    /**
     * @brief Shutdown the player keyboard navigation enhancements
     */
    void shutdown();

public slots:
    /**
     * @brief Handle play/pause keyboard shortcut
     */
    void onPlayPauseShortcut();

    /**
     * @brief Handle stop keyboard shortcut
     */
    void onStopShortcut();

    /**
     * @brief Handle next track keyboard shortcut
     */
    void onNextTrackShortcut();

    /**
     * @brief Handle previous track keyboard shortcut
     */
    void onPreviousTrackShortcut();

    /**
     * @brief Handle volume up keyboard shortcut
     */
    void onVolumeUpShortcut();

    /**
     * @brief Handle volume down keyboard shortcut
     */
    void onVolumeDownShortcut();

    /**
     * @brief Handle mute/unmute keyboard shortcut
     */
    void onMuteShortcut();

    /**
     * @brief Handle fine volume up keyboard shortcut (1% increment)
     */
    void onVolumeUpFineShortcut();

    /**
     * @brief Handle fine volume down keyboard shortcut (1% decrement)
     */
    void onVolumeDownFineShortcut();

    /**
     * @brief Handle volume reset keyboard shortcut (set to 100%)
     */
    void onResetVolumeShortcut();

    /**
     * @brief Handle fast forward keyboard shortcut (seek +10 seconds)
     */
    void onFastForwardShortcut();

    /**
     * @brief Handle rewind keyboard shortcut (seek -10 seconds)
     */
    void onRewindShortcut();

    /**
     * @brief Handle current time announcement shortcut
     */
    void onCurrentTimeShortcut();

    /**
     * @brief Handle remaining time announcement shortcut
     */
    void onRemainingTimeShortcut();

    /**
     * @brief Handle total duration announcement shortcut
     */
    void onTotalDurationShortcut();

    /**
     * @brief Handle track information announcement shortcut
     */
    void onTrackInfoShortcut();

    /**
     * @brief Handle keyboard shortcut execution
     * @param action Action identifier
     * @param sequence Key sequence that triggered the action
     */
    void onShortcutTriggered(const QString& action, const QKeySequence& sequence);

private slots:
    /**
     * @brief Handle playback state changes for announcements
     */
    void onPlaybackStateChanged();

    /**
     * @brief Handle volume changes for announcements
     * @param volume New volume level
     */
    void onVolumeChanged(int volume);

    /**
     * @brief Handle position changes for time announcements
     * @param position Current playback position in milliseconds
     */
    void onPositionChanged(qint64 position);

    /**
     * @brief Handle duration changes
     * @param duration Total track duration in milliseconds
     */
    void onDurationChanged(qint64 duration);

private:
    /**
     * @brief Setup keyboard shortcuts for player controls
     */
    void setupPlayerShortcuts();

    /**
     * @brief Setup navigation for player widgets
     */
    void setupPlayerWidgetNavigation();

    /**
     * @brief Setup custom tab order for player interface
     */
    void setupPlayerTabOrder();

    /**
     * @brief Setup playlist navigation enhancements
     */
    void setupPlaylistNavigation();

    /**
     * @brief Get current playback time as formatted string
     * @return Formatted time string (e.g., "2 minutes 30 seconds")
     */
    QString getCurrentTimeString() const;

    /**
     * @brief Get current volume as formatted string
     * @return Formatted volume string (e.g., "Volume 75 percent")
     */
    QString getCurrentVolumeString() const;

    /**
     * @brief Get current track information
     * @return Formatted track information string
     */
    QString getCurrentTrackInfo() const;

    /**
     * @brief Format time in milliseconds to human-readable string
     * @param milliseconds Time in milliseconds
     * @return Formatted time string
     */
    QString formatTime(qint64 milliseconds) const;

    /**
     * @brief Announce playback control action
     * @param action Action that was performed
     */
    void announcePlaybackAction(const QString& action);

    /**
     * @brief Get player control widgets
     */
    QPushButton* getPlayButton() const;
    QPushButton* getStopButton() const;
    QPushButton* getNextButton() const;
    QSlider* getVolumeSlider() const;
    QSlider* getProgressSlider() const;
    QTableView* getMusicView() const;
    QListWidget* getPlaylistView() const;

    // Member variables
    player* m_playerWindow;
    KeyboardNavigationController* m_navigationController;
    AccessibilityManager* m_accessibilityManager;
    
    // State tracking
    bool m_initialized;
    bool m_isPlaying;
    bool m_isMuted;
    int m_currentVolume;
    int m_previousVolume;
    qint64 m_currentPosition;
    qint64 m_currentDuration;
    QString m_currentTrack;
    
    // Widget references (cached for performance)
    QPushButton* m_playButton;
    QPushButton* m_stopButton;
    QPushButton* m_nextButton;
    QSlider* m_volumeSlider;
    QSlider* m_progressSlider;
    QTableView* m_musicView;
    QListWidget* m_playlistView;
};

#endif // PLAYERKEYBOARDNAVIGATIONENHANCER_H