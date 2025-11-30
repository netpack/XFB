#include "PlayerKeyboardNavigationEnhancer.h"
#include "KeyboardNavigationController.h"
#include "AccessibilityManager.h"
#include "../player.h"
#include "Logger.h"
#include <QApplication>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QTime>
#include <QDebug>

PlayerKeyboardNavigationEnhancer::PlayerKeyboardNavigationEnhancer(player* playerWindow,
                                                                   KeyboardNavigationController* navigationController,
                                                                   AccessibilityManager* accessibilityManager,
                                                                   QObject* parent)
    : QObject(parent)
    , m_playerWindow(playerWindow)
    , m_navigationController(navigationController)
    , m_accessibilityManager(accessibilityManager)
    , m_initialized(false)
    , m_isPlaying(false)
    , m_isMuted(false)
    , m_currentVolume(75)
    , m_previousVolume(75)
    , m_currentPosition(0)
    , m_currentDuration(0)
    , m_playButton(nullptr)
    , m_stopButton(nullptr)
    , m_nextButton(nullptr)
    , m_volumeSlider(nullptr)
    , m_progressSlider(nullptr)
    , m_musicView(nullptr)
    , m_playlistView(nullptr)
{
    if (!m_playerWindow) {
        logError("PlayerKeyboardNavigationEnhancer: Player window is null");
        return;
    }
    
    if (!m_navigationController) {
        logError("PlayerKeyboardNavigationEnhancer: KeyboardNavigationController is null");
        return;
    }
    
    if (!m_accessibilityManager) {
        logError("PlayerKeyboardNavigationEnhancer: AccessibilityManager is null");
        return;
    }
    
    logDebug("PlayerKeyboardNavigationEnhancer created");
}

PlayerKeyboardNavigationEnhancer::~PlayerKeyboardNavigationEnhancer()
{
    shutdown();
    logDebug("PlayerKeyboardNavigationEnhancer destroyed");
}

bool PlayerKeyboardNavigationEnhancer::initialize()
{
    if (m_initialized) {
        logWarning("PlayerKeyboardNavigationEnhancer already initialized");
        return true;
    }
    
    logDebug("Initializing PlayerKeyboardNavigationEnhancer");
    
    // Cache widget references
    m_playButton = getPlayButton();
    m_stopButton = getStopButton();
    m_nextButton = getNextButton();
    m_volumeSlider = getVolumeSlider();
    m_progressSlider = getProgressSlider();
    m_musicView = getMusicView();
    m_playlistView = getPlaylistView();
    
    // Setup keyboard shortcuts
    setupPlayerShortcuts();
    
    // Setup widget navigation
    setupPlayerWidgetNavigation();
    
    // Setup custom tab order
    setupPlayerTabOrder();
    
    // Setup playlist navigation
    setupPlaylistNavigation();
    
    // Connect to navigation controller signals
    connect(m_navigationController, &KeyboardNavigationController::shortcutTriggered,
            this, &PlayerKeyboardNavigationEnhancer::onShortcutTriggered);
    
    // Connect to player signals for state tracking
    if (m_playerWindow) {
        // Note: These connections assume the player class has these signals
        // In a real implementation, you'd connect to the actual player signals
        
        // Connect volume slider if available
        if (m_volumeSlider) {
            connect(m_volumeSlider, &QSlider::valueChanged,
                    this, &PlayerKeyboardNavigationEnhancer::onVolumeChanged);
        }
        
        // Connect play button if available
        if (m_playButton) {
            connect(m_playButton, &QPushButton::clicked,
                    this, &PlayerKeyboardNavigationEnhancer::onPlaybackStateChanged);
        }
        
        // Connect stop button if available
        if (m_stopButton) {
            connect(m_stopButton, &QPushButton::clicked,
                    this, &PlayerKeyboardNavigationEnhancer::onPlaybackStateChanged);
        }
    }
    
    m_initialized = true;
    logDebug("PlayerKeyboardNavigationEnhancer initialized successfully");
    return true;
}

void PlayerKeyboardNavigationEnhancer::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    logDebug("Shutting down PlayerKeyboardNavigationEnhancer");
    
    // Disconnect signals
    if (m_navigationController) {
        disconnect(m_navigationController, &KeyboardNavigationController::shortcutTriggered,
                  this, &PlayerKeyboardNavigationEnhancer::onShortcutTriggered);
    }
    
    // Clear widget references
    m_playButton = nullptr;
    m_stopButton = nullptr;
    m_nextButton = nullptr;
    m_volumeSlider = nullptr;
    m_progressSlider = nullptr;
    m_musicView = nullptr;
    m_playlistView = nullptr;
    
    m_initialized = false;
    logDebug("PlayerKeyboardNavigationEnhancer shutdown complete");
}

void PlayerKeyboardNavigationEnhancer::onPlayPauseShortcut()
{
    if (!m_playButton) {
        return;
    }
    
    // Simulate button click
    m_playButton->click();
    
    // Announce action
    QString action = m_isPlaying ? "Paused" : "Playing";
    announcePlaybackAction(action);
    
    m_isPlaying = !m_isPlaying;
}

void PlayerKeyboardNavigationEnhancer::onStopShortcut()
{
    if (!m_stopButton) {
        return;
    }
    
    // Simulate button click
    m_stopButton->click();
    
    // Announce action
    announcePlaybackAction("Stopped");
    
    m_isPlaying = false;
}

void PlayerKeyboardNavigationEnhancer::onNextTrackShortcut()
{
    if (!m_nextButton) {
        return;
    }
    
    // Simulate button click
    m_nextButton->click();
    
    // Announce action
    announcePlaybackAction("Next track");
}

void PlayerKeyboardNavigationEnhancer::onPreviousTrackShortcut()
{
    // For previous track, we might need to implement custom logic
    // since there might not be a dedicated previous button
    announcePlaybackAction("Previous track");
}

void PlayerKeyboardNavigationEnhancer::onVolumeUpShortcut()
{
    if (!m_volumeSlider) {
        return;
    }
    
    int currentValue = m_volumeSlider->value();
    int newValue = qMin(currentValue + 5, m_volumeSlider->maximum());
    m_volumeSlider->setValue(newValue);
    
    m_currentVolume = newValue;
    
    // Announce new volume
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(getCurrentVolumeString(), 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onVolumeDownShortcut()
{
    if (!m_volumeSlider) {
        return;
    }
    
    int currentValue = m_volumeSlider->value();
    int newValue = qMax(currentValue - 5, m_volumeSlider->minimum());
    m_volumeSlider->setValue(newValue);
    
    m_currentVolume = newValue;
    
    // Announce new volume
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(getCurrentVolumeString(), 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onMuteShortcut()
{
    if (!m_volumeSlider) {
        return;
    }
    
    if (m_isMuted) {
        // Unmute - restore previous volume
        m_volumeSlider->setValue(m_previousVolume);
        m_currentVolume = m_previousVolume;
        m_isMuted = false;
        
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Unmuted", AccessibilityManager::Priority::Normal);
        }
    } else {
        // Mute - save current volume and set to 0
        m_previousVolume = m_volumeSlider->value();
        m_volumeSlider->setValue(0);
        m_currentVolume = 0;
        m_isMuted = true;
        
        if (m_accessibilityManager) {
            m_accessibilityManager->announceMessage("Muted", AccessibilityManager::Priority::Normal);
        }
    }
}

void PlayerKeyboardNavigationEnhancer::onVolumeUpFineShortcut()
{
    if (!m_volumeSlider) {
        return;
    }
    
    int currentValue = m_volumeSlider->value();
    int newValue = qMin(currentValue + 1, m_volumeSlider->maximum());
    m_volumeSlider->setValue(newValue);
    
    m_currentVolume = newValue;
    
    // Announce new volume
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(getCurrentVolumeString(), 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onVolumeDownFineShortcut()
{
    if (!m_volumeSlider) {
        return;
    }
    
    int currentValue = m_volumeSlider->value();
    int newValue = qMax(currentValue - 1, m_volumeSlider->minimum());
    m_volumeSlider->setValue(newValue);
    
    m_currentVolume = newValue;
    
    // Announce new volume
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage(getCurrentVolumeString(), 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onResetVolumeShortcut()
{
    if (!m_volumeSlider) {
        return;
    }
    
    m_volumeSlider->setValue(100);
    m_currentVolume = 100;
    m_isMuted = false;
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage("Volume reset to 100 percent", 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onFastForwardShortcut()
{
    if (!m_progressSlider) {
        return;
    }
    
    // Seek forward 10 seconds (10000 milliseconds)
    qint64 newPosition = m_currentPosition + 10000;
    if (newPosition > m_currentDuration) {
        newPosition = m_currentDuration;
    }
    
    // Update position (this would typically call the media player's seek function)
    m_currentPosition = newPosition;
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage("Seeking forward 10 seconds", 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onRewindShortcut()
{
    if (!m_progressSlider) {
        return;
    }
    
    // Seek backward 10 seconds (10000 milliseconds)
    qint64 newPosition = m_currentPosition - 10000;
    if (newPosition < 0) {
        newPosition = 0;
    }
    
    // Update position (this would typically call the media player's seek function)
    m_currentPosition = newPosition;
    
    if (m_accessibilityManager) {
        m_accessibilityManager->announceMessage("Seeking backward 10 seconds", 
                                               AccessibilityManager::Priority::Normal);
    }
}

void PlayerKeyboardNavigationEnhancer::onCurrentTimeShortcut()
{
    if (m_accessibilityManager) {
        QString timeInfo = getCurrentTimeString();
        if (!timeInfo.isEmpty()) {
            m_accessibilityManager->announceMessage(timeInfo, AccessibilityManager::Priority::Normal);
        } else {
            m_accessibilityManager->announceMessage("No track playing", AccessibilityManager::Priority::Normal);
        }
    }
}

void PlayerKeyboardNavigationEnhancer::onRemainingTimeShortcut()
{
    if (m_accessibilityManager) {
        if (m_currentDuration > 0 && m_currentPosition >= 0) {
            qint64 remainingTime = m_currentDuration - m_currentPosition;
            QString remainingTimeStr = formatTime(remainingTime);
            QString announcement = QString("Time remaining: %1").arg(remainingTimeStr);
            m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
        } else {
            m_accessibilityManager->announceMessage("No track playing", AccessibilityManager::Priority::Normal);
        }
    }
}

void PlayerKeyboardNavigationEnhancer::onTotalDurationShortcut()
{
    if (m_accessibilityManager) {
        if (m_currentDuration > 0) {
            QString durationStr = formatTime(m_currentDuration);
            QString announcement = QString("Total duration: %1").arg(durationStr);
            m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
        } else {
            m_accessibilityManager->announceMessage("No track playing", AccessibilityManager::Priority::Normal);
        }
    }
}

void PlayerKeyboardNavigationEnhancer::onTrackInfoShortcut()
{
    if (m_accessibilityManager) {
        QString trackInfo = getCurrentTrackInfo();
        if (!trackInfo.isEmpty() && trackInfo != "No track selected") {
            // Add additional track information if available
            QString fullInfo = trackInfo;
            if (m_currentDuration > 0) {
                QString durationStr = formatTime(m_currentDuration);
                fullInfo += QString(", Duration: %1").arg(durationStr);
            }
            m_accessibilityManager->announceMessage(fullInfo, AccessibilityManager::Priority::Normal);
        } else {
            m_accessibilityManager->announceMessage("No track selected", AccessibilityManager::Priority::Normal);
        }
    }
}

void PlayerKeyboardNavigationEnhancer::onShortcutTriggered(const QString& action, const QKeySequence& sequence)
{
    Q_UNUSED(sequence)
    
    // Handle player-specific shortcuts
    if (action == "play_pause") {
        onPlayPauseShortcut();
    } else if (action == "stop") {
        onStopShortcut();
    } else if (action == "next_track" || action == "next_track_legacy") {
        onNextTrackShortcut();
    } else if (action == "previous_track" || action == "previous_track_legacy") {
        onPreviousTrackShortcut();
    } else if (action == "volume_up" || action == "volume_up_legacy") {
        onVolumeUpShortcut();
    } else if (action == "volume_down" || action == "volume_down_legacy") {
        onVolumeDownShortcut();
    } else if (action == "volume_up_fine") {
        onVolumeUpFineShortcut();
    } else if (action == "volume_down_fine") {
        onVolumeDownFineShortcut();
    } else if (action == "mute" || action == "mute_legacy") {
        onMuteShortcut();
    } else if (action == "reset_volume") {
        onResetVolumeShortcut();
    } else if (action == "fast_forward") {
        onFastForwardShortcut();
    } else if (action == "rewind") {
        onRewindShortcut();
    } else if (action == "current_time") {
        onCurrentTimeShortcut();
    } else if (action == "remaining_time") {
        onRemainingTimeShortcut();
    } else if (action == "total_duration") {
        onTotalDurationShortcut();
    } else if (action == "track_info") {
        onTrackInfoShortcut();
    }
}

void PlayerKeyboardNavigationEnhancer::onPlaybackStateChanged()
{
    // This would be called when playback state changes
    // Update internal state and announce if needed
    logDebug("Playback state changed");
}

void PlayerKeyboardNavigationEnhancer::onVolumeChanged(int volume)
{
    m_currentVolume = volume;
    
    // Announce volume change if accessibility is enabled
    if (m_accessibilityManager && m_accessibilityManager->isAccessibilityEnabled()) {
        QString volumeString = getCurrentVolumeString();
        m_accessibilityManager->announceMessage(volumeString, AccessibilityManager::Priority::Low);
    }
}

void PlayerKeyboardNavigationEnhancer::onPositionChanged(qint64 position)
{
    m_currentPosition = position;
}

void PlayerKeyboardNavigationEnhancer::onDurationChanged(qint64 duration)
{
    m_currentDuration = duration;
}

void PlayerKeyboardNavigationEnhancer::setupPlayerShortcuts()
{
    if (!m_navigationController) {
        return;
    }
    
    // Register player-specific shortcuts (these override the default ones with player context)
    m_navigationController->registerKeyboardShortcut(
        "play_pause", QKeySequence(Qt::Key_Space), 
        "Play or pause current track", "Player", true, m_playButton);
    
    m_navigationController->registerKeyboardShortcut(
        "stop", QKeySequence(Qt::CTRL | Qt::Key_Space), 
        "Stop playback", "Player", true, m_stopButton);
    
    // Primary shortcuts (documented)
    m_navigationController->registerKeyboardShortcut(
        "next_track", QKeySequence(Qt::CTRL | Qt::Key_Right), 
        "Play next track", "Player", true, m_nextButton);
    
    m_navigationController->registerKeyboardShortcut(
        "previous_track", QKeySequence(Qt::CTRL | Qt::Key_Left), 
        "Play previous track", "Player", true);
    
    // Volume control (documented)
    m_navigationController->registerKeyboardShortcut(
        "volume_up", QKeySequence(Qt::CTRL | Qt::Key_Up), 
        "Increase volume by 5%", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "volume_down", QKeySequence(Qt::CTRL | Qt::Key_Down), 
        "Decrease volume by 5%", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "volume_up_fine", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up), 
        "Increase volume by 1%", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "volume_down_fine", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down), 
        "Decrease volume by 1%", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "mute", QKeySequence(Qt::CTRL | Qt::Key_M), 
        "Mute or unmute audio", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "reset_volume", QKeySequence(Qt::CTRL | Qt::Key_0), 
        "Reset volume to 100%", "Player", true, m_volumeSlider);
    
    // Seeking controls
    m_navigationController->registerKeyboardShortcut(
        "fast_forward", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Right), 
        "Seek forward 10 seconds", "Player", true, m_progressSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "rewind", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Left), 
        "Seek backward 10 seconds", "Player", true, m_progressSlider);
    
    // Time information
    m_navigationController->registerKeyboardShortcut(
        "current_time", QKeySequence(Qt::CTRL | Qt::Key_T), 
        "Announce current playback time", "Player", true);
    
    m_navigationController->registerKeyboardShortcut(
        "remaining_time", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), 
        "Announce remaining time", "Player", true);
    
    m_navigationController->registerKeyboardShortcut(
        "total_duration", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T), 
        "Announce total duration", "Player", true);
    
    m_navigationController->registerKeyboardShortcut(
        "track_info", QKeySequence(Qt::CTRL | Qt::Key_D), 
        "Announce track details", "Player", true);
    
    // Legacy shortcuts for compatibility
    m_navigationController->registerKeyboardShortcut(
        "next_track_legacy", QKeySequence(Qt::Key_N), 
        "Play next track (legacy)", "Player", true, m_nextButton);
    
    m_navigationController->registerKeyboardShortcut(
        "previous_track_legacy", QKeySequence(Qt::Key_P), 
        "Play previous track (legacy)", "Player", true);
    
    m_navigationController->registerKeyboardShortcut(
        "volume_up_legacy", QKeySequence(Qt::Key_Plus), 
        "Increase volume (legacy)", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "volume_down_legacy", QKeySequence(Qt::Key_Minus), 
        "Decrease volume (legacy)", "Player", true, m_volumeSlider);
    
    m_navigationController->registerKeyboardShortcut(
        "mute_legacy", QKeySequence(Qt::Key_M), 
        "Mute or unmute audio (legacy)", "Player", true, m_volumeSlider);
    
    logDebug("Player keyboard shortcuts registered");
}

void PlayerKeyboardNavigationEnhancer::setupPlayerWidgetNavigation()
{
    if (!m_navigationController) {
        return;
    }
    
    // Register player control widgets for enhanced navigation
    if (m_playButton) {
        m_navigationController->registerNavigationWidget(
            m_playButton, 
            KeyboardNavigationController::NavigationContext::Player,
            "Play button",
            "Press Space to play or pause"
        );
    }
    
    if (m_stopButton) {
        m_navigationController->registerNavigationWidget(
            m_stopButton, 
            KeyboardNavigationController::NavigationContext::Player,
            "Stop button",
            "Press S to stop playback"
        );
    }
    
    if (m_nextButton) {
        m_navigationController->registerNavigationWidget(
            m_nextButton, 
            KeyboardNavigationController::NavigationContext::Player,
            "Next track button",
            "Press N for next track"
        );
    }
    
    if (m_volumeSlider) {
        m_navigationController->registerNavigationWidget(
            m_volumeSlider, 
            KeyboardNavigationController::NavigationContext::Player,
            "Volume slider",
            "Use Plus and Minus keys to adjust volume, M to mute"
        );
    }
    
    if (m_progressSlider) {
        m_navigationController->registerNavigationWidget(
            m_progressSlider, 
            KeyboardNavigationController::NavigationContext::Player,
            "Progress slider",
            "Shows current playback position"
        );
    }
    
    logDebug("Player widget navigation registered");
}

void PlayerKeyboardNavigationEnhancer::setupPlayerTabOrder()
{
    if (!m_navigationController) {
        return;
    }
    
    // Set logical tab order for player controls
    if (m_playButton && m_stopButton) {
        m_navigationController->setCustomTabOrder(m_playButton, m_stopButton);
    }
    
    if (m_stopButton && m_nextButton) {
        m_navigationController->setCustomTabOrder(m_stopButton, m_nextButton);
    }
    
    if (m_nextButton && m_volumeSlider) {
        m_navigationController->setCustomTabOrder(m_nextButton, m_volumeSlider);
    }
    
    if (m_volumeSlider && m_progressSlider) {
        m_navigationController->setCustomTabOrder(m_volumeSlider, m_progressSlider);
    }
    
    logDebug("Player tab order configured");
}

void PlayerKeyboardNavigationEnhancer::setupPlaylistNavigation()
{
    if (!m_navigationController) {
        return;
    }
    
    // Setup music library grid navigation
    if (m_musicView) {
        m_navigationController->registerNavigationWidget(
            m_musicView, 
            KeyboardNavigationController::NavigationContext::Database,
            "Music library",
            "Use arrow keys to navigate, Enter to add to playlist"
        );
        
        m_navigationController->setArrowKeyNavigationEnabled(m_musicView, true);
    }
    
    // Setup playlist navigation
    if (m_playlistView) {
        m_navigationController->registerNavigationWidget(
            m_playlistView, 
            KeyboardNavigationController::NavigationContext::Playlist,
            "Playlist",
            "Use arrow keys to navigate, Delete to remove items"
        );
        
        m_navigationController->setArrowKeyNavigationEnabled(m_playlistView, true);
    }
    
    logDebug("Playlist navigation configured");
}

QString PlayerKeyboardNavigationEnhancer::getCurrentTimeString() const
{
    if (m_currentDuration == 0) {
        return QString();
    }
    
    QString currentTime = formatTime(m_currentPosition);
    QString totalTime = formatTime(m_currentDuration);
    
    return QString("Current time: %1 of %2").arg(currentTime, totalTime);
}

QString PlayerKeyboardNavigationEnhancer::getCurrentVolumeString() const
{
    return QString("Volume %1 percent").arg(m_currentVolume);
}

QString PlayerKeyboardNavigationEnhancer::getCurrentTrackInfo() const
{
    if (m_currentTrack.isEmpty()) {
        return "No track selected";
    }
    
    return QString("Current track: %1").arg(m_currentTrack);
}

QString PlayerKeyboardNavigationEnhancer::formatTime(qint64 milliseconds) const
{
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    
    if (minutes > 0) {
        return QString("%1 minutes %2 seconds").arg(minutes).arg(seconds);
    } else {
        return QString("%1 seconds").arg(seconds);
    }
}

void PlayerKeyboardNavigationEnhancer::announcePlaybackAction(const QString& action)
{
    if (m_accessibilityManager) {
        QString announcement = action;
        
        // Add track info for verbose mode
        if (m_accessibilityManager->verbosityLevel() == AccessibilityManager::VerbosityLevel::Verbose) {
            QString trackInfo = getCurrentTrackInfo();
            if (!trackInfo.isEmpty() && trackInfo != "No track selected") {
                announcement += ", " + trackInfo;
            }
        }
        
        m_accessibilityManager->announceMessage(announcement, AccessibilityManager::Priority::Normal);
    }
}

QPushButton* PlayerKeyboardNavigationEnhancer::getPlayButton() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    return m_playerWindow->findChild<QPushButton*>("btPlay");
}

QPushButton* PlayerKeyboardNavigationEnhancer::getStopButton() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    return m_playerWindow->findChild<QPushButton*>("btStop");
}

QPushButton* PlayerKeyboardNavigationEnhancer::getNextButton() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    return m_playerWindow->findChild<QPushButton*>("btPlayNext");
}

QSlider* PlayerKeyboardNavigationEnhancer::getVolumeSlider() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    return m_playerWindow->findChild<QSlider*>("sliderVolume");
}

QSlider* PlayerKeyboardNavigationEnhancer::getProgressSlider() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    return m_playerWindow->findChild<QSlider*>("sliderProgress");
}

QTableView* PlayerKeyboardNavigationEnhancer::getMusicView() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    return m_playerWindow->findChild<QTableView*>("musicView");
}

QListWidget* PlayerKeyboardNavigationEnhancer::getPlaylistView() const
{
    if (!m_playerWindow) {
        return nullptr;
    }
    
    // Assuming there's a playlist widget - this might need adjustment based on actual UI
    return m_playerWindow->findChild<QListWidget*>("playlistView");
}