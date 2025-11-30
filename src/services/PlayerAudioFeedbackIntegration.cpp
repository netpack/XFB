#include "PlayerAudioFeedbackIntegration.h"
#include "AudioFeedbackService.h"
#include "ServiceContainer.h"
#include "../player.h"
#include "../controllers/PlayerUIController.h"
#include <QMediaPlayer>
#include <QTimer>
#include <QUrl>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QTime>

PlayerAudioFeedbackIntegration::PlayerAudioFeedbackIntegration(player* playerWindow, QObject* parent)
    : QObject(parent)
    , m_playerWindow(playerWindow)
    , m_playerUIController(nullptr)
    , m_audioFeedbackService(nullptr)
    , m_initialized(false)
    , m_trackEndingCountdownActive(false)
    , m_currentPlaybackState(QMediaPlayer::StoppedState)
    , m_currentPosition(0)
    , m_currentDuration(0)
    , m_currentVolume(50)
    , m_timeAnnouncementTimer(nullptr)
    , m_trackEndingTimer(nullptr)
{
}

PlayerAudioFeedbackIntegration::~PlayerAudioFeedbackIntegration()
{
    if (m_initialized) {
        shutdown();
    }
}

bool PlayerAudioFeedbackIntegration::initialize()
{
    qDebug() << "PlayerAudioFeedbackIntegration: Initializing player audio feedback integration";
    
    if (!m_playerWindow) {
        qWarning() << "PlayerAudioFeedbackIntegration: Player window is null";
        return false;
    }
    
    try {
        // Get audio feedback service
        m_audioFeedbackService = ServiceContainer::instance()->resolve<AudioFeedbackService>();
        if (!m_audioFeedbackService) {
            qWarning() << "PlayerAudioFeedbackIntegration: AudioFeedbackService not available";
            return false;
        }
        
        // Create timers
        m_timeAnnouncementTimer = new QTimer(this);
        m_timeAnnouncementTimer->setInterval(TIME_ANNOUNCEMENT_INTERVAL_MS);
        m_timeAnnouncementTimer->setSingleShot(false);
        connect(m_timeAnnouncementTimer, &QTimer::timeout,
                this, &PlayerAudioFeedbackIntegration::onTimeAnnouncementTimer);
        
        m_trackEndingTimer = new QTimer(this);
        m_trackEndingTimer->setSingleShot(false);
        connect(m_trackEndingTimer, &QTimer::timeout,
                this, &PlayerAudioFeedbackIntegration::onTrackEndingCountdown);
        
        // Connect to player signals
        connectPlayerSignals();
        
        m_initialized = true;
        qDebug() << "PlayerAudioFeedbackIntegration: Initialization completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "PlayerAudioFeedbackIntegration: Exception during initialization:" << e.what();
        return false;
    }
}

void PlayerAudioFeedbackIntegration::shutdown()
{
    qDebug() << "PlayerAudioFeedbackIntegration: Shutting down player audio feedback integration";
    
    if (m_initialized) {
        disconnectSignals();
        
        if (m_timeAnnouncementTimer) {
            m_timeAnnouncementTimer->stop();
        }
        
        if (m_trackEndingTimer) {
            m_trackEndingTimer->stop();
        }
        
        m_initialized = false;
    }
}

void PlayerAudioFeedbackIntegration::connectPlayerSignals()
{
    // Note: The actual player class doesn't seem to have QMediaPlayer signals exposed
    // We'll need to connect to the UI slots that handle these events
    
    // Connect to button click events through the player's slots
    // These connections will be made when the actual player events occur
    
    qDebug() << "PlayerAudioFeedbackIntegration: Connected to player signals";
}

void PlayerAudioFeedbackIntegration::connectUIControllerSignals()
{
    if (m_playerUIController) {
        // Connect to UI controller signals for feedback
        qDebug() << "PlayerAudioFeedbackIntegration: Connected to UI controller signals";
    }
}

void PlayerAudioFeedbackIntegration::disconnectSignals()
{
    // Disconnect all signal connections
    disconnect(this);
}

void PlayerAudioFeedbackIntegration::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == m_currentPlaybackState) {
        return; // No change
    }
    
    Q_UNUSED(m_currentPlaybackState); // Store previous state for potential future use
    m_currentPlaybackState = state;
    
    QString stateString = formatPlaybackState(state);
    QString trackInfo;
    
    // Get current track info if playing
    if (state == QMediaPlayer::PlayingState && !m_currentMedia.isEmpty()) {
        trackInfo = getTrackInfoFromUrl(m_currentMedia);
    }
    
    // Announce the state change
    m_audioFeedbackService->announcePlaybackStateChange(stateString, trackInfo);
    
    // Handle timer management based on state
    if (state == QMediaPlayer::PlayingState) {
        // Start time announcement timer if not already running
        if (!m_timeAnnouncementTimer->isActive()) {
            m_timeAnnouncementTimer->start();
        }
        
        // Setup track ending countdown if we have duration info
        if (m_currentDuration > 0) {
            setupTrackEndingCountdown();
        }
    } else {
        // Stop timers when not playing
        m_timeAnnouncementTimer->stop();
        stopTrackEndingCountdown();
    }
}

void PlayerAudioFeedbackIntegration::onMediaChanged(const QUrl& media)
{
    if (media == m_currentMedia) {
        return; // No change
    }
    
    m_currentMedia = media;
    
    // Get track information
    m_currentTrackTitle = getTrackTitleFromDatabase(media.toLocalFile());
    m_currentArtist = getArtistFromDatabase(media.toLocalFile());
    
    // Announce track change
    QString duration;
    if (m_currentDuration > 0) {
        duration = formatDuration(m_currentDuration);
    }
    
    m_audioFeedbackService->announceTrackChange(m_currentTrackTitle, m_currentArtist, duration);
    
    // Reset countdown state for new track
    stopTrackEndingCountdown();
    if (m_currentPlaybackState == QMediaPlayer::PlayingState && m_currentDuration > 0) {
        setupTrackEndingCountdown();
    }
}

void PlayerAudioFeedbackIntegration::onVolumeChanged(int volume)
{
    if (volume == m_currentVolume) {
        return; // No change
    }
    
    m_currentVolume = volume;
    
    // Announce volume change
    QString message = QString("Volume %1%").arg(volume);
    m_audioFeedbackService->announceStatusChange("Player", message);
}

void PlayerAudioFeedbackIntegration::onPositionChanged(qint64 position)
{
    m_currentPosition = position;
    
    // Check if we need to start countdown announcements
    if (m_currentDuration > 0 && isTrackApproachingEnd(position, m_currentDuration)) {
        if (!m_trackEndingCountdownActive) {
            m_trackEndingCountdownActive = true;
            m_trackEndingTimer->setInterval(COUNTDOWN_ANNOUNCEMENT_INTERVAL * 1000);
            m_trackEndingTimer->start();
        }
    }
}

void PlayerAudioFeedbackIntegration::onDurationChanged(qint64 duration)
{
    m_currentDuration = duration;
    
    // Setup countdown if playing and we now have duration
    if (m_currentPlaybackState == QMediaPlayer::PlayingState && duration > 0) {
        setupTrackEndingCountdown();
    }
}

void PlayerAudioFeedbackIntegration::onPlaylistModified(const QString& action, int itemCount, const QString& playlistName)
{
    m_audioFeedbackService->announcePlaylistModification(action, itemCount, playlistName);
}

void PlayerAudioFeedbackIntegration::onPlaylistSaved(const QString& playlistName, bool success)
{
    m_audioFeedbackService->announceSaveOperation("playlist", playlistName, success);
}

void PlayerAudioFeedbackIntegration::onPlaylistLoaded(const QString& playlistName, bool success)
{
    m_audioFeedbackService->announceLoadOperation("playlist", playlistName, success);
}

void PlayerAudioFeedbackIntegration::onMusicImported(int importedCount, bool success)
{
    if (success) {
        QString message = QString("Successfully imported %1 tracks").arg(importedCount);
        m_audioFeedbackService->announceDatabaseOperation("import", message, true);
    } else {
        m_audioFeedbackService->announceError("Failed to import music files", "Music Import");
    }
}

void PlayerAudioFeedbackIntegration::onPlayerError(QMediaPlayer::Error error, const QString& errorString)
{
    QString context;
    switch (error) {
        case QMediaPlayer::NoError:
            return; // No error to announce
        case QMediaPlayer::ResourceError:
            context = "Media Resource";
            break;
        case QMediaPlayer::FormatError:
            context = "Media Format";
            break;
        case QMediaPlayer::NetworkError:
            context = "Network";
            break;
        case QMediaPlayer::AccessDeniedError:
            context = "Access Denied";
            break;
        default:
            context = "Player";
            break;
    }
    
    m_audioFeedbackService->announceError(errorString, context);
}

void PlayerAudioFeedbackIntegration::onButtonClicked(const QString& buttonName, const QString& action)
{
    m_audioFeedbackService->announceButtonClick(buttonName, action);
}

void PlayerAudioFeedbackIntegration::onMenuSelected(const QString& menuPath, const QString& description)
{
    m_audioFeedbackService->announceMenuSelection(menuPath, description);
}

void PlayerAudioFeedbackIntegration::announceCurrentTrack()
{
    if (m_currentTrackTitle.isEmpty()) {
        m_audioFeedbackService->queueAnnouncement("No track currently loaded", AudioFeedbackService::Priority::Normal);
        return;
    }
    
    QString duration;
    if (m_currentDuration > 0) {
        duration = formatDuration(m_currentDuration);
    }
    
    m_audioFeedbackService->announceTrackChange(m_currentTrackTitle, m_currentArtist, duration);
}

void PlayerAudioFeedbackIntegration::announceCurrentTime()
{
    if (m_currentDuration <= 0) {
        m_audioFeedbackService->queueAnnouncement("No track loaded", AudioFeedbackService::Priority::Normal);
        return;
    }
    
    QString currentTime = formatDuration(m_currentPosition);
    QString totalTime = formatDuration(m_currentDuration);
    QString message = QString("Current time: %1 of %2").arg(currentTime, totalTime);
    
    m_audioFeedbackService->queueAnnouncement(message, AudioFeedbackService::Priority::Normal);
}

void PlayerAudioFeedbackIntegration::announceRemainingTime()
{
    if (m_currentDuration <= 0) {
        m_audioFeedbackService->queueAnnouncement("No track loaded", AudioFeedbackService::Priority::Normal);
        return;
    }
    
    qint64 remaining = m_currentDuration - m_currentPosition;
    QString remainingTime = formatDuration(remaining);
    QString message = QString("Time remaining: %1").arg(remainingTime);
    
    m_audioFeedbackService->queueAnnouncement(message, AudioFeedbackService::Priority::Normal);
}

void PlayerAudioFeedbackIntegration::announcePlaylistInfo()
{
    // This would need to be implemented based on the actual playlist structure
    // For now, provide a basic announcement
    m_audioFeedbackService->queueAnnouncement("Playlist information requested", AudioFeedbackService::Priority::Normal);
}

void PlayerAudioFeedbackIntegration::onTimeAnnouncementTimer()
{
    // Periodic time announcements (every 30 seconds during playback)
    if (m_currentPlaybackState == QMediaPlayer::PlayingState && m_currentDuration > 0) {
        announceCurrentTime();
    }
}

void PlayerAudioFeedbackIntegration::onTrackEndingCountdown()
{
    if (!m_trackEndingCountdownActive || m_currentDuration <= 0) {
        return;
    }
    
    int remainingSeconds = getRemainingSeconds(m_currentPosition, m_currentDuration);
    
    if (remainingSeconds <= 0) {
        stopTrackEndingCountdown();
        return;
    }
    
    // Announce countdown at specific intervals
    if (remainingSeconds == 30 || remainingSeconds == 20 || remainingSeconds == 10 || remainingSeconds <= 5) {
        QString message = QString("%1 seconds remaining").arg(remainingSeconds);
        m_audioFeedbackService->queueAnnouncement(message, AudioFeedbackService::Priority::High);
    }
}

QString PlayerAudioFeedbackIntegration::getTrackInfoFromUrl(const QUrl& url)
{
    QString title = getTrackTitleFromDatabase(url.toLocalFile());
    QString artist = getArtistFromDatabase(url.toLocalFile());
    
    if (!title.isEmpty()) {
        if (!artist.isEmpty()) {
            return QString("%1 by %2").arg(title, artist);
        }
        return title;
    }
    
    // Fallback to filename
    QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.baseName();
}

QString PlayerAudioFeedbackIntegration::getTrackTitleFromDatabase(const QString& filePath)
{
    if (!m_playerWindow || filePath.isEmpty()) {
        return QString();
    }
    
    QSqlQuery query(m_playerWindow->adb);
    query.prepare("SELECT title FROM music WHERE file_path = ?");
    query.addBindValue(filePath);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    
    // Fallback to filename without extension
    QFileInfo fileInfo(filePath);
    return fileInfo.baseName();
}

QString PlayerAudioFeedbackIntegration::getArtistFromDatabase(const QString& filePath)
{
    if (!m_playerWindow || filePath.isEmpty()) {
        return QString();
    }
    
    QSqlQuery query(m_playerWindow->adb);
    query.prepare("SELECT artist FROM music WHERE file_path = ?");
    query.addBindValue(filePath);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    
    return QString();
}

QString PlayerAudioFeedbackIntegration::formatDuration(qint64 milliseconds)
{
    if (milliseconds <= 0) {
        return "0:00";
    }
    
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    
    if (minutes >= 60) {
        int hours = minutes / 60;
        minutes = minutes % 60;
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    
    return QString("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QChar('0'));
}

QString PlayerAudioFeedbackIntegration::formatPlaybackState(QMediaPlayer::PlaybackState state)
{
    switch (state) {
        case QMediaPlayer::PlayingState:
            return "playing";
        case QMediaPlayer::PausedState:
            return "paused";
        case QMediaPlayer::StoppedState:
            return "stopped";
        default:
            return "unknown";
    }
}

bool PlayerAudioFeedbackIntegration::isTrackApproachingEnd(qint64 position, qint64 duration)
{
    if (duration <= 0) {
        return false;
    }
    
    qint64 remaining = duration - position;
    return remaining <= (TRACK_ENDING_COUNTDOWN_SECONDS * 1000);
}

int PlayerAudioFeedbackIntegration::getRemainingSeconds(qint64 position, qint64 duration)
{
    if (duration <= 0) {
        return 0;
    }
    
    qint64 remaining = duration - position;
    return static_cast<int>(remaining / 1000);
}

void PlayerAudioFeedbackIntegration::setupTrackEndingCountdown()
{
    if (m_currentDuration <= 0) {
        return;
    }
    
    // Only setup countdown for tracks longer than the countdown period
    if (m_currentDuration > (TRACK_ENDING_COUNTDOWN_SECONDS * 1000)) {
        m_trackEndingCountdownActive = false; // Will be activated when position reaches countdown zone
    }
}

void PlayerAudioFeedbackIntegration::stopTrackEndingCountdown()
{
    m_trackEndingCountdownActive = false;
    if (m_trackEndingTimer) {
        m_trackEndingTimer->stop();
    }
}