#include "PlaybackStatusAnnouncer.h"
#include "LiveRegionManager.h"
#include "AccessibilityManager.h"
#include "ServiceContainer.h"
#include "../player.h"
#include <QApplication>
#include <QShortcut>
#include <QDateTime>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>

PlaybackStatusAnnouncer::PlaybackStatusAnnouncer(QObject* parent)
    : BaseService(parent)
    , m_liveRegionManager(nullptr)
    , m_accessibilityManager(nullptr)
    , m_playerInstance(nullptr)
    , m_mediaPlayer(nullptr)
    , m_currentState(QMediaPlayer::StoppedState)
    , m_currentPosition(0)
    , m_currentDuration(0)
    , m_currentVerbosityLevel(1)
    , m_timeUpdateTimer(nullptr)
    , m_countdownTimer(nullptr)
    , m_countdownActive(false)
    , m_lastAnnouncedCountdown(-1)
    , m_timeAnnouncementShortcut(QKeySequence("Ctrl+T"))
    , m_globalTimeShortcut(nullptr)
    , m_isInitialized(false)
    , m_playerConnected(false)
    , m_lastTimeAnnouncement(0)
{
    // Initialize default configuration
    m_config.trackChangeEnabled = true;
    m_config.playbackStateEnabled = true;
    m_config.timeUpdatesEnabled = false; // Usually disabled to avoid spam
    m_config.countdownEnabled = true;
    m_config.countdownThresholdSeconds = DEFAULT_COUNTDOWN_THRESHOLD_SECONDS;
    m_config.timeUpdateIntervalSeconds = DEFAULT_TIME_UPDATE_INTERVAL_SECONDS;
    m_config.includeArtistInAnnouncements = true;
    m_config.includeDurationInAnnouncements = true;
    m_config.announceRemainingTime = true;
}

PlaybackStatusAnnouncer::~PlaybackStatusAnnouncer()
{
    shutdown();
}

bool PlaybackStatusAnnouncer::doInitialize()
{
    qDebug() << "PlaybackStatusAnnouncer: Initializing playback status announcements";
    
    try {
        // Get service dependencies
        auto* serviceContainer = ServiceContainer::instance();
        m_liveRegionManager = serviceContainer->resolve<LiveRegionManager>();
        m_accessibilityManager = serviceContainer->resolve<AccessibilityManager>();
        
        if (!m_liveRegionManager) {
            qWarning() << "PlaybackStatusAnnouncer: LiveRegionManager not available";
            return false;
        }
        
        if (!m_accessibilityManager) {
            qWarning() << "PlaybackStatusAnnouncer: AccessibilityManager not available";
            return false;
        }
        
        // Initialize playback monitoring
        initializePlaybackMonitoring();
        
        // Connect to accessibility manager signals
        connect(m_accessibilityManager, &AccessibilityManager::accessibilityStateChanged,
                this, &PlaybackStatusAnnouncer::onAccessibilityStateChanged);
        connect(m_accessibilityManager, &AccessibilityManager::verbosityLevelChanged,
                this, [this](AccessibilityManager::VerbosityLevel level) {
                    onVerbosityLevelChanged(static_cast<int>(level));
                });
        
        // Setup keyboard shortcuts
        setupKeyboardShortcuts();
        
        // Load configuration
        loadConfiguration();
        
        m_isInitialized = true;
        
        qDebug() << "PlaybackStatusAnnouncer: Initialization completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "PlaybackStatusAnnouncer: Exception during initialization:" << e.what();
        return false;
    }
}

void PlaybackStatusAnnouncer::doShutdown()
{
    qDebug() << "PlaybackStatusAnnouncer: Shutting down playback status announcements";
    
    // Save configuration
    saveConfiguration();
    
    // Disconnect from player
    disconnectFromPlayer();
    
    // Cleanup keyboard shortcuts
    cleanupKeyboardShortcuts();
    
    // Shutdown playback monitoring
    shutdownPlaybackMonitoring();
    
    // Reset service dependencies
    m_liveRegionManager = nullptr;
    m_accessibilityManager = nullptr;
    
    m_isInitialized = false;
    
    qDebug() << "PlaybackStatusAnnouncer: Shutdown completed";
}

QString PlaybackStatusAnnouncer::getServiceName() const
{
    return "PlaybackStatusAnnouncer";
}

void PlaybackStatusAnnouncer::initializePlaybackMonitoring()
{
    // Create time update timer
    m_timeUpdateTimer = new QTimer(this);
    m_timeUpdateTimer->setInterval(m_config.timeUpdateIntervalSeconds * 1000);
    m_timeUpdateTimer->setSingleShot(false);
    connect(m_timeUpdateTimer, &QTimer::timeout, this, &PlaybackStatusAnnouncer::onTimeUpdateTimer);
    
    // Create countdown timer
    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(COUNTDOWN_TIMER_INTERVAL_MS);
    m_countdownTimer->setSingleShot(false);
    connect(m_countdownTimer, &QTimer::timeout, this, &PlaybackStatusAnnouncer::onCountdownTimer);
}

void PlaybackStatusAnnouncer::shutdownPlaybackMonitoring()
{
    // Stop and cleanup timers
    if (m_timeUpdateTimer) {
        m_timeUpdateTimer->stop();
        m_timeUpdateTimer->deleteLater();
        m_timeUpdateTimer = nullptr;
    }
    
    if (m_countdownTimer) {
        m_countdownTimer->stop();
        m_countdownTimer->deleteLater();
        m_countdownTimer = nullptr;
    }
    
    // Stop countdown
    stopCountdown();
}

bool PlaybackStatusAnnouncer::connectToPlayer(player* playerInstance)
{
    if (!playerInstance) {
        qWarning() << "PlaybackStatusAnnouncer: Cannot connect to null player instance";
        return false;
    }
    
    // Disconnect from previous player if any
    disconnectFromPlayer();
    
    m_playerInstance = playerInstance;
    
    // Get the media player from the player instance
    // Note: This assumes the player class has a way to access the QMediaPlayer
    // We'll need to add a getter method to the player class
    
    qDebug() << "PlaybackStatusAnnouncer: Connected to player instance";
    m_playerConnected = true;
    
    return true;
}

void PlaybackStatusAnnouncer::disconnectFromPlayer()
{
    if (m_mediaPlayer) {
        // Disconnect all signals
        disconnect(m_mediaPlayer, nullptr, this, nullptr);
        m_mediaPlayer = nullptr;
    }
    
    m_playerInstance = nullptr;
    m_playerConnected = false;
    
    // Reset current state
    m_currentState = QMediaPlayer::StoppedState;
    m_currentMedia = QUrl();
    m_currentPosition = 0;
    m_currentDuration = 0;
    m_currentTrackInfo = TrackInfo();
    
    qDebug() << "PlaybackStatusAnnouncer: Disconnected from player";
}

bool PlaybackStatusAnnouncer::setTimeAnnouncementShortcut(const QKeySequence& shortcut)
{
    if (m_timeAnnouncementShortcut == shortcut) {
        return true;
    }
    
    // Uninstall old shortcut
    uninstallGlobalShortcut();
    
    m_timeAnnouncementShortcut = shortcut;
    
    // Install new shortcut
    bool success = installGlobalShortcut(shortcut);
    
    if (success) {
        qDebug() << "PlaybackStatusAnnouncer: Time announcement shortcut set to" << shortcut.toString();
        saveConfiguration();
    } else {
        qWarning() << "PlaybackStatusAnnouncer: Failed to set time announcement shortcut" << shortcut.toString();
    }
    
    return success;
}

void PlaybackStatusAnnouncer::announceCurrentTime(bool includeRemaining)
{
    if (!m_isInitialized || !m_liveRegionManager) {
        return;
    }
    
    // Throttle manual time announcements
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastTimeAnnouncement < MIN_TIME_ANNOUNCEMENT_INTERVAL_MS) {
        return;
    }
    
    m_lastTimeAnnouncement = currentTime;
    
    QString timeInfo = getCurrentTimeInfo(includeRemaining);
    
    if (!timeInfo.isEmpty()) {
        m_liveRegionManager->announceTimeUpdate(
            formatTime(m_currentPosition),
            formatTime(m_currentDuration),
            includeRemaining ? formatTime(m_currentDuration - m_currentPosition) : QString()
        );
        
        emit timeInfoAnnounced(timeInfo);
    }
}

void PlaybackStatusAnnouncer::startCountdown(const QDateTime& targetTime, const QString& eventDescription)
{
    if (!m_config.countdownEnabled) {
        return;
    }
    
    m_countdownTargetTime = targetTime;
    m_countdownEventDescription = eventDescription;
    m_countdownActive = true;
    m_lastAnnouncedCountdown = -1;
    
    if (m_countdownTimer) {
        m_countdownTimer->start();
    }
    
    qDebug() << "PlaybackStatusAnnouncer: Started countdown to" << targetTime.toString() 
             << "for event:" << eventDescription;
}

void PlaybackStatusAnnouncer::stopCountdown()
{
    if (m_countdownTimer) {
        m_countdownTimer->stop();
    }
    
    m_countdownActive = false;
    m_lastAnnouncedCountdown = -1;
    
    qDebug() << "PlaybackStatusAnnouncer: Stopped countdown";
}

// getConfig() method is already defined inline in the header file

void PlaybackStatusAnnouncer::setConfig(const AnnouncementConfig& config)
{
    m_config = config;
    
    // Update timer intervals
    if (m_timeUpdateTimer) {
        m_timeUpdateTimer->setInterval(config.timeUpdateIntervalSeconds * 1000);
        
        if (config.timeUpdatesEnabled && m_playerConnected) {
            if (!m_timeUpdateTimer->isActive()) {
                m_timeUpdateTimer->start();
            }
        } else {
            m_timeUpdateTimer->stop();
        }
    }
    
    saveConfiguration();
}

void PlaybackStatusAnnouncer::setTrackChangeAnnouncementsEnabled(bool enabled)
{
    if (m_config.trackChangeEnabled != enabled) {
        m_config.trackChangeEnabled = enabled;
        saveConfiguration();
    }
}

void PlaybackStatusAnnouncer::setPlaybackStateAnnouncementsEnabled(bool enabled)
{
    if (m_config.playbackStateEnabled != enabled) {
        m_config.playbackStateEnabled = enabled;
        saveConfiguration();
    }
}

void PlaybackStatusAnnouncer::setTimeUpdatesEnabled(bool enabled)
{
    if (m_config.timeUpdatesEnabled != enabled) {
        m_config.timeUpdatesEnabled = enabled;
        
        if (m_timeUpdateTimer) {
            if (enabled && m_playerConnected) {
                m_timeUpdateTimer->start();
            } else {
                m_timeUpdateTimer->stop();
            }
        }
        
        saveConfiguration();
    }
}

QString PlaybackStatusAnnouncer::getCurrentTrackInfo() const
{
    return formatTrackAnnouncement(m_currentTrackInfo, m_currentVerbosityLevel);
}

QString PlaybackStatusAnnouncer::getCurrentTimeInfo(bool includeRemaining) const
{
    return formatTimeAnnouncement(m_currentPosition, m_currentDuration, includeRemaining, m_currentVerbosityLevel);
}

void PlaybackStatusAnnouncer::onAccessibilityStateChanged(bool enabled)
{
    qDebug() << "PlaybackStatusAnnouncer: Accessibility state changed to" << enabled;
    
    if (!enabled) {
        // Stop timers when accessibility is disabled
        if (m_timeUpdateTimer) {
            m_timeUpdateTimer->stop();
        }
        stopCountdown();
    } else {
        // Restart timers if needed
        if (m_config.timeUpdatesEnabled && m_playerConnected && m_timeUpdateTimer) {
            m_timeUpdateTimer->start();
        }
    }
}

void PlaybackStatusAnnouncer::onVerbosityLevelChanged(int level)
{
    m_currentVerbosityLevel = level;
    qDebug() << "PlaybackStatusAnnouncer: Verbosity level changed to" << level;
}

void PlaybackStatusAnnouncer::onPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    if (m_currentState == state) {
        return;
    }
    
    Q_UNUSED(m_currentState); // Store previous state for potential future use
    m_currentState = state;
    
    if (m_config.playbackStateEnabled && m_liveRegionManager) {
        QString announcement = formatPlaybackStateAnnouncement(state, m_currentTrackInfo);
        
        // Determine the appropriate playback state string
        QString stateString;
        switch (state) {
            case QMediaPlayer::PlayingState:
                stateString = "playing";
                break;
            case QMediaPlayer::PausedState:
                stateString = "paused";
                break;
            case QMediaPlayer::StoppedState:
                stateString = "stopped";
                break;
        }
        
        m_liveRegionManager->announcePlaybackChange(
            m_currentTrackInfo.title,
            m_currentTrackInfo.artist,
            m_currentTrackInfo.duration,
            stateString
        );
        
        emit playbackStateAnnounced(announcement);
    }
    
    // Start/stop time updates based on playback state
    if (m_timeUpdateTimer) {
        if (state == QMediaPlayer::PlayingState && m_config.timeUpdatesEnabled) {
            m_timeUpdateTimer->start();
        } else {
            m_timeUpdateTimer->stop();
        }
    }
}

void PlaybackStatusAnnouncer::onMediaChanged(const QUrl& media)
{
    if (m_currentMedia == media) {
        return;
    }
    
    m_currentMedia = media;
    
    // Get track information
    TrackInfo newTrackInfo = getTrackInfoFromPath(media.toLocalFile());
    
    // Check if track actually changed (not just URL format)
    bool trackChanged = (newTrackInfo.title != m_currentTrackInfo.title || 
                        newTrackInfo.artist != m_currentTrackInfo.artist);
    
    m_currentTrackInfo = newTrackInfo;
    
    if (trackChanged && m_config.trackChangeEnabled && m_liveRegionManager) {
        m_liveRegionManager->announcePlaybackChange(
            m_currentTrackInfo.title,
            m_currentTrackInfo.artist,
            m_currentTrackInfo.duration,
            m_currentState == QMediaPlayer::PlayingState ? "playing" : ""
        );
        
        QString announcement = formatTrackAnnouncement(m_currentTrackInfo, m_currentVerbosityLevel);
        emit trackChangeAnnounced(announcement);
    }
}

void PlaybackStatusAnnouncer::onPositionChanged(qint64 position)
{
    m_currentPosition = position;
    
    // No automatic announcements for position changes to avoid spam
    // Time announcements are handled by timer or manual requests
}

void PlaybackStatusAnnouncer::onDurationChanged(qint64 duration)
{
    m_currentDuration = duration;
    
    // Update track info with duration if not already set
    if (m_currentTrackInfo.duration.isEmpty() && duration > 0) {
        m_currentTrackInfo.duration = formatTime(duration);
    }
}

void PlaybackStatusAnnouncer::onTimeUpdateTimer()
{
    if (m_currentState == QMediaPlayer::PlayingState && m_config.timeUpdatesEnabled) {
        announceCurrentTime(m_config.announceRemainingTime);
    }
}

void PlaybackStatusAnnouncer::onCountdownTimer()
{
    if (!m_countdownActive) {
        return;
    }
    
    int secondsRemaining = getCountdownSecondsRemaining();
    
    if (secondsRemaining <= 0) {
        // Countdown completed
        if (m_liveRegionManager) {
            m_liveRegionManager->announceCountdown(0, m_countdownEventDescription);
        }
        
        emit countdownCompleted(m_countdownEventDescription);
        stopCountdown();
        return;
    }
    
    // Announce countdown at specific intervals
    bool shouldAnnounce = false;
    
    if (secondsRemaining <= m_config.countdownThresholdSeconds) {
        // Announce every second for the last 10 seconds
        if (secondsRemaining <= 10) {
            shouldAnnounce = true;
        }
        // Announce at 30, 20, 15 seconds
        else if (secondsRemaining == 30 || secondsRemaining == 20 || secondsRemaining == 15) {
            shouldAnnounce = true;
        }
        // Announce every 10 seconds for longer countdowns
        else if (secondsRemaining % 10 == 0) {
            shouldAnnounce = true;
        }
    }
    
    if (shouldAnnounce && secondsRemaining != m_lastAnnouncedCountdown) {
        if (m_liveRegionManager) {
            m_liveRegionManager->announceCountdown(secondsRemaining, m_countdownEventDescription);
        }
        
        m_lastAnnouncedCountdown = secondsRemaining;
    }
}

void PlaybackStatusAnnouncer::onTimeAnnouncementShortcutActivated()
{
    announceCurrentTime(true);
}

void PlaybackStatusAnnouncer::setupKeyboardShortcuts()
{
    installGlobalShortcut(m_timeAnnouncementShortcut);
}

void PlaybackStatusAnnouncer::cleanupKeyboardShortcuts()
{
    uninstallGlobalShortcut();
}

PlaybackStatusAnnouncer::TrackInfo PlaybackStatusAnnouncer::getTrackInfoFromPath(const QString& filePath) const
{
    TrackInfo info;
    info.filePath = filePath;
    
    if (filePath.isEmpty()) {
        return info;
    }
    
    QFileInfo fileInfo(filePath);
    
    // Try to get information from database first
    if (m_playerInstance && m_playerInstance->adb.isOpen()) {
        QSqlQuery query(m_playerInstance->adb);
        query.prepare("SELECT title, artist, album, duration FROM music WHERE path = ?");
        query.addBindValue(filePath);
        
        if (query.exec() && query.next()) {
            info.title = query.value("title").toString();
            info.artist = query.value("artist").toString();
            info.album = query.value("album").toString();
            info.duration = query.value("duration").toString();
        }
    }
    
    // Fallback to filename if no database info
    if (info.title.isEmpty()) {
        info.title = fileInfo.completeBaseName();
        
        // Try to parse artist and title from filename (Artist - Title format)
        QStringList parts = info.title.split(" - ");
        if (parts.size() >= 2) {
            info.artist = parts[0].trimmed();
            info.title = parts.mid(1).join(" - ").trimmed();
        }
    }
    
    return info;
}

QString PlaybackStatusAnnouncer::formatTrackAnnouncement(const TrackInfo& trackInfo, int verbosityLevel) const
{
    if (trackInfo.title.isEmpty()) {
        return "No track information available";
    }
    
    QString announcement;
    
    switch (verbosityLevel) {
        case 0: // Terse
            announcement = trackInfo.title;
            break;
            
        case 1: // Normal
            announcement = trackInfo.title;
            if (!trackInfo.artist.isEmpty() && m_config.includeArtistInAnnouncements) {
                announcement += " by " + trackInfo.artist;
            }
            break;
            
        case 2: // Verbose
            announcement = trackInfo.title;
            if (!trackInfo.artist.isEmpty() && m_config.includeArtistInAnnouncements) {
                announcement += " by " + trackInfo.artist;
            }
            if (!trackInfo.album.isEmpty()) {
                announcement += " from " + trackInfo.album;
            }
            if (!trackInfo.duration.isEmpty() && m_config.includeDurationInAnnouncements) {
                announcement += " (" + trackInfo.duration + ")";
            }
            break;
    }
    
    return announcement;
}

QString PlaybackStatusAnnouncer::formatPlaybackStateAnnouncement(QMediaPlayer::PlaybackState state, const TrackInfo& trackInfo) const
{
    QString stateString;
    
    switch (state) {
        case QMediaPlayer::PlayingState:
            stateString = "Playing";
            break;
        case QMediaPlayer::PausedState:
            stateString = "Paused";
            break;
        case QMediaPlayer::StoppedState:
            stateString = "Stopped";
            break;
    }
    
    if (state == QMediaPlayer::PlayingState && !trackInfo.title.isEmpty()) {
        return stateString + ": " + formatTrackAnnouncement(trackInfo, m_currentVerbosityLevel);
    }
    
    return stateString;
}

QString PlaybackStatusAnnouncer::formatTimeAnnouncement(qint64 currentMs, qint64 totalMs, bool includeRemaining, int verbosityLevel) const
{
    if (currentMs < 0 || totalMs <= 0) {
        return "Time information not available";
    }
    
    QString announcement;
    QString currentTime = formatTime(currentMs);
    QString totalTime = formatTime(totalMs);
    
    switch (verbosityLevel) {
        case 0: // Terse
            announcement = currentTime;
            break;
            
        case 1: // Normal
            announcement = currentTime + " of " + totalTime;
            break;
            
        case 2: // Verbose
            announcement = "Current time: " + currentTime + " of " + totalTime;
            if (includeRemaining && m_config.announceRemainingTime) {
                QString remainingTime = formatTime(totalMs - currentMs);
                announcement += ", " + remainingTime + " remaining";
            }
            break;
    }
    
    return announcement;
}

QString PlaybackStatusAnnouncer::formatTime(qint64 milliseconds) const
{
    if (milliseconds < 0) {
        return "0:00";
    }
    
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'));
    }
}

int PlaybackStatusAnnouncer::getCountdownSecondsRemaining() const
{
    if (!m_countdownActive) {
        return -1;
    }
    
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 msRemaining = currentTime.msecsTo(m_countdownTargetTime);
    
    return static_cast<int>(msRemaining / 1000);
}

void PlaybackStatusAnnouncer::loadConfiguration()
{
    QSettings settings;
    settings.beginGroup("PlaybackStatusAnnouncer");
    
    m_config.trackChangeEnabled = settings.value("trackChangeEnabled", true).toBool();
    m_config.playbackStateEnabled = settings.value("playbackStateEnabled", true).toBool();
    m_config.timeUpdatesEnabled = settings.value("timeUpdatesEnabled", false).toBool();
    m_config.countdownEnabled = settings.value("countdownEnabled", true).toBool();
    m_config.countdownThresholdSeconds = settings.value("countdownThresholdSeconds", DEFAULT_COUNTDOWN_THRESHOLD_SECONDS).toInt();
    m_config.timeUpdateIntervalSeconds = settings.value("timeUpdateIntervalSeconds", DEFAULT_TIME_UPDATE_INTERVAL_SECONDS).toInt();
    m_config.includeArtistInAnnouncements = settings.value("includeArtistInAnnouncements", true).toBool();
    m_config.includeDurationInAnnouncements = settings.value("includeDurationInAnnouncements", true).toBool();
    m_config.announceRemainingTime = settings.value("announceRemainingTime", true).toBool();
    
    QString shortcutString = settings.value("timeAnnouncementShortcut", "Ctrl+T").toString();
    m_timeAnnouncementShortcut = QKeySequence(shortcutString);
    
    settings.endGroup();
}

void PlaybackStatusAnnouncer::saveConfiguration()
{
    QSettings settings;
    settings.beginGroup("PlaybackStatusAnnouncer");
    
    settings.setValue("trackChangeEnabled", m_config.trackChangeEnabled);
    settings.setValue("playbackStateEnabled", m_config.playbackStateEnabled);
    settings.setValue("timeUpdatesEnabled", m_config.timeUpdatesEnabled);
    settings.setValue("countdownEnabled", m_config.countdownEnabled);
    settings.setValue("countdownThresholdSeconds", m_config.countdownThresholdSeconds);
    settings.setValue("timeUpdateIntervalSeconds", m_config.timeUpdateIntervalSeconds);
    settings.setValue("includeArtistInAnnouncements", m_config.includeArtistInAnnouncements);
    settings.setValue("includeDurationInAnnouncements", m_config.includeDurationInAnnouncements);
    settings.setValue("announceRemainingTime", m_config.announceRemainingTime);
    settings.setValue("timeAnnouncementShortcut", m_timeAnnouncementShortcut.toString());
    
    settings.endGroup();
}

bool PlaybackStatusAnnouncer::installGlobalShortcut(const QKeySequence& shortcut)
{
    if (shortcut.isEmpty()) {
        return false;
    }
    
    // Uninstall existing shortcut first
    uninstallGlobalShortcut();
    
    try {
        // Create application-level shortcut
        m_globalTimeShortcut = new QShortcut(shortcut, QApplication::activeWindow());
        connect(m_globalTimeShortcut, &QShortcut::activated,
                this, &PlaybackStatusAnnouncer::onTimeAnnouncementShortcutActivated);
        
        qDebug() << "PlaybackStatusAnnouncer: Installed global shortcut" << shortcut.toString();
        return true;
        
    } catch (const std::exception& e) {
        qWarning() << "PlaybackStatusAnnouncer: Failed to install global shortcut:" << e.what();
        return false;
    }
}

void PlaybackStatusAnnouncer::uninstallGlobalShortcut()
{
    if (m_globalTimeShortcut) {
        m_globalTimeShortcut->deleteLater();
        m_globalTimeShortcut = nullptr;
    }
}