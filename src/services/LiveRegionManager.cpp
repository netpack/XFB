#include "LiveRegionManager.h"
#include "AccessibilityManager.h"
#include "AudioFeedbackService.h"
#include "ServiceContainer.h"
#include <QApplication>
#include <QAccessibleEvent>
#include <QDateTime>
#include <QDebug>
#include <QSettings>

LiveRegionManager::LiveRegionManager(QObject* parent)
    : BaseService(parent)
    , m_updateTimer(nullptr)
    , m_timeUpdateTimer(nullptr)
    , m_accessibilityManager(nullptr)
    , m_audioFeedbackService(nullptr)
    , m_isProcessingUpdates(false)
    , m_currentVerbosityLevel(1)
    , m_updatesProcessed(0)
    , m_updatesThrottled(0)
    , m_lastTimeAnnouncementRequest(0)
{
    // Initialize default configuration
    m_config.enabled = true;
    m_config.updateThrottleMs = DEFAULT_UPDATE_THROTTLE_MS;
    m_config.timeUpdateIntervalMs = DEFAULT_TIME_UPDATE_INTERVAL_MS;
    m_config.countdownThresholdSeconds = DEFAULT_COUNTDOWN_THRESHOLD_SECONDS;
    m_config.allowTimeUpdatesOnDemand = true;
    m_config.maxPendingUpdates = MAX_PENDING_UPDATES_PER_REGION;
    m_config.prioritizeCriticalUpdates = true;
}

LiveRegionManager::~LiveRegionManager()
{
    shutdown();
}

bool LiveRegionManager::doInitialize()
{
    qDebug() << "LiveRegionManager: Initializing live region management system";
    
    try {
        // Get service dependencies
        auto* serviceContainer = ServiceContainer::instance();
        m_accessibilityManager = serviceContainer->resolve<AccessibilityManager>();
        m_audioFeedbackService = serviceContainer->resolve<AudioFeedbackService>();
        
        if (!m_accessibilityManager) {
            qWarning() << "LiveRegionManager: AccessibilityManager not available";
            return false;
        }
        
        if (!m_audioFeedbackService) {
            qWarning() << "LiveRegionManager: AudioFeedbackService not available";
            return false;
        }
        
        // Initialize live region system
        initializeLiveRegionSystem();
        
        // Connect to accessibility manager signals
        connect(m_accessibilityManager, &AccessibilityManager::accessibilityStateChanged,
                this, &LiveRegionManager::onAccessibilityStateChanged);
        connect(m_accessibilityManager, &AccessibilityManager::verbosityLevelChanged,
                this, [this](AccessibilityManager::VerbosityLevel level) {
                    onVerbosityLevelChanged(static_cast<int>(level));
                });
        
        // Load configuration
        loadConfiguration();
        
        qDebug() << "LiveRegionManager: Initialization completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "LiveRegionManager: Exception during initialization:" << e.what();
        return false;
    }
}

void LiveRegionManager::doShutdown()
{
    qDebug() << "LiveRegionManager: Shutting down live region management system";
    
    // Save configuration
    saveConfiguration();
    
    // Shutdown live region system
    shutdownLiveRegionSystem();
    
    // Clear all live regions
    m_liveRegions.clear();
    m_pendingUpdates.clear();
    
    // Reset service dependencies
    m_accessibilityManager = nullptr;
    m_audioFeedbackService = nullptr;
    
    qDebug() << "LiveRegionManager: Shutdown completed";
}

QString LiveRegionManager::getServiceName() const
{
    return "LiveRegionManager";
}

void LiveRegionManager::initializeLiveRegionSystem()
{
    // Create update processing timer
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(UPDATE_PROCESSING_INTERVAL_MS);
    m_updateTimer->setSingleShot(false);
    connect(m_updateTimer, &QTimer::timeout, this, &LiveRegionManager::processPendingUpdates);
    
    // Create time update timer
    m_timeUpdateTimer = new QTimer(this);
    m_timeUpdateTimer->setInterval(m_config.timeUpdateIntervalMs);
    m_timeUpdateTimer->setSingleShot(false);
    connect(m_timeUpdateTimer, &QTimer::timeout, this, &LiveRegionManager::onTimeUpdateTimer);
    
    // Start timers if enabled
    if (m_config.enabled) {
        m_updateTimer->start();
        if (m_config.timeUpdateIntervalMs > 0) {
            m_timeUpdateTimer->start();
        }
    }
}

void LiveRegionManager::shutdownLiveRegionSystem()
{
    // Stop and cleanup timers
    if (m_updateTimer) {
        m_updateTimer->stop();
        m_updateTimer->deleteLater();
        m_updateTimer = nullptr;
    }
    
    if (m_timeUpdateTimer) {
        m_timeUpdateTimer->stop();
        m_timeUpdateTimer->deleteLater();
        m_timeUpdateTimer = nullptr;
    }
    
    // Clear processing state
    m_isProcessingUpdates = false;
}

bool LiveRegionManager::createLiveRegion(QWidget* widget, LiveRegionType type, UpdateType updateType)
{
    if (!widget) {
        qWarning() << "LiveRegionManager: Cannot create live region for null widget";
        return false;
    }
    
    if (!m_config.enabled) {
        qDebug() << "LiveRegionManager: Live regions are disabled";
        return false;
    }
    
    QMutexLocker locker(&m_updateMutex);
    
    // Check if widget is already registered
    if (m_liveRegions.contains(widget)) {
        qDebug() << "LiveRegionManager: Widget already has live region, updating configuration";
        m_liveRegions[widget].type = type;
        m_liveRegions[widget].updateType = updateType;
        return true;
    }
    
    // Create live region info
    LiveRegionInfo info;
    info.widget = widget;
    info.type = type;
    info.updateType = updateType;
    info.lastUpdateTime = 0;
    info.pendingUpdates = 0;
    info.isActive = true;
    
    // Register with Qt accessibility framework
    if (QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(widget)) {
        // Set live region property
        widget->setProperty("accessibleLiveRegion", static_cast<int>(type));
        
        qDebug() << "LiveRegionManager: Created live region for widget" << widget->objectName() 
                 << "type:" << static_cast<int>(type) << "updateType:" << static_cast<int>(updateType);
    } else {
        qWarning() << "LiveRegionManager: Failed to get accessible interface for widget" << widget->objectName();
        return false;
    }
    
    // Store live region info
    m_liveRegions[widget] = info;
    
    // Connect to widget destruction
    connect(widget, &QObject::destroyed, this, &LiveRegionManager::onWidgetDestroyed);
    
    emit liveRegionUpdated(widget, QString(), updateType);
    return true;
}

void LiveRegionManager::removeLiveRegion(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    QMutexLocker locker(&m_updateMutex);
    
    if (m_liveRegions.contains(widget)) {
        qDebug() << "LiveRegionManager: Removing live region for widget" << widget->objectName();
        
        // Clear pending updates for this widget
        clearPendingUpdates(widget);
        
        // Remove from registry
        m_liveRegions.remove(widget);
        
        // Disconnect from widget
        disconnect(widget, &QObject::destroyed, this, &LiveRegionManager::onWidgetDestroyed);
        
        // Clear Qt accessibility property
        widget->setProperty("accessibleLiveRegion", QVariant());
    }
}

void LiveRegionManager::updateLiveRegion(QWidget* widget, const QString& content, bool forceUpdate)
{
    if (!widget || content.isEmpty()) {
        return;
    }
    
    if (!m_config.enabled) {
        return;
    }
    
    QMutexLocker locker(&m_updateMutex);
    
    // Check if widget is registered
    if (!m_liveRegions.contains(widget)) {
        qWarning() << "LiveRegionManager: Attempting to update unregistered live region for widget" << widget->objectName();
        return;
    }
    
    LiveRegionInfo& info = m_liveRegions[widget];
    
    // Check if content has changed
    if (info.lastContent == content && !forceUpdate) {
        return;
    }
    
    // Check throttling
    bool isCritical = (info.updateType == UpdateType::CriticalAlert);
    if (!forceUpdate && !isCritical && shouldThrottleUpdate(widget, info.updateType, false)) {
        // Add to pending updates
        PendingUpdate update(widget, content, info.updateType, isCritical);
        addPendingUpdate(update);
        return;
    }
    
    // Process update immediately
    processLiveRegionUpdate(widget, content, info.updateType);
}

void LiveRegionManager::announcePlaybackChange(const QString& trackTitle, const QString& artist, 
                                             const QString& duration, const QString& playbackState)
{
    if (trackTitle.isEmpty()) {
        return;
    }
    
    QString announcement;
    
    // Format based on verbosity level
    switch (m_currentVerbosityLevel) {
        case 0: // Terse
            announcement = trackTitle;
            if (!playbackState.isEmpty()) {
                announcement = playbackState + ": " + announcement;
            }
            break;
            
        case 1: // Normal
            announcement = trackTitle;
            if (!artist.isEmpty()) {
                announcement += " by " + artist;
            }
            if (!playbackState.isEmpty()) {
                announcement = playbackState + ": " + announcement;
            }
            break;
            
        case 2: // Verbose
            announcement = trackTitle;
            if (!artist.isEmpty()) {
                announcement += " by " + artist;
            }
            if (!duration.isEmpty()) {
                announcement += " (" + duration + ")";
            }
            if (!playbackState.isEmpty()) {
                announcement = "Now " + playbackState.toLower() + ": " + announcement;
            }
            break;
    }
    
    // Store for comparison
    m_lastPlaybackInfo = announcement;
    
    // Send to audio feedback service
    if (m_audioFeedbackService) {
        m_audioFeedbackService->announceTrackChange(trackTitle, artist, duration);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::PlaybackStatus);
}

void LiveRegionManager::announceTimeUpdate(const QString& currentTime, const QString& totalTime, const QString& remainingTime)
{
    if (currentTime.isEmpty()) {
        return;
    }
    
    QString announcement;
    
    // Format based on verbosity level
    switch (m_currentVerbosityLevel) {
        case 0: // Terse
            announcement = currentTime;
            break;
            
        case 1: // Normal
            announcement = currentTime;
            if (!totalTime.isEmpty()) {
                announcement += " of " + totalTime;
            }
            break;
            
        case 2: // Verbose
            announcement = "Current time: " + currentTime;
            if (!totalTime.isEmpty()) {
                announcement += " of " + totalTime;
            }
            if (!remainingTime.isEmpty()) {
                announcement += ", " + remainingTime + " remaining";
            }
            break;
    }
    
    // Store for comparison
    m_lastTimeInfo = announcement;
    
    // Send to audio feedback service with low priority (time updates are frequent)
    if (m_audioFeedbackService) {
        m_audioFeedbackService->queueAnnouncement(announcement, AudioFeedbackService::Priority::Low, 
                                                 AudioFeedbackService::FeedbackType::StatusChange);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::TimeUpdate);
}

void LiveRegionManager::announceSystemStatus(const QString& component, const QString& status, const QString& additionalInfo)
{
    if (component.isEmpty() || status.isEmpty()) {
        return;
    }
    
    QString announcement = component + ": " + status;
    if (!additionalInfo.isEmpty()) {
        announcement += " - " + additionalInfo;
    }
    
    // Send to audio feedback service
    if (m_audioFeedbackService) {
        m_audioFeedbackService->announceStatusChange(component, status, additionalInfo);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::SystemStatus);
}

void LiveRegionManager::announceDatabaseProgress(const QString& operation, int progress, int itemsProcessed, int totalItems)
{
    if (operation.isEmpty() || progress < 0 || progress > 100) {
        return;
    }
    
    QString announcement = operation + ": " + QString::number(progress) + "%";
    
    if (itemsProcessed >= 0 && totalItems > 0) {
        announcement += " (" + QString::number(itemsProcessed) + " of " + QString::number(totalItems) + ")";
    }
    
    // Send to audio feedback service
    if (m_audioFeedbackService) {
        QString estimatedTime;
        if (progress > 0 && itemsProcessed > 0 && totalItems > 0) {
            // Simple estimation based on current progress
            int remaining = totalItems - itemsProcessed;
            if (remaining > 0) {
                estimatedTime = "About " + QString::number((remaining * 100) / (progress * 10)) + " seconds remaining";
            }
        }
        m_audioFeedbackService->announceProgressUpdate(operation, progress, estimatedTime);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::DatabaseProgress);
}

void LiveRegionManager::announceQueueUpdate(const QString& queueName, const QString& action, int itemCount, int totalItems)
{
    if (queueName.isEmpty() || action.isEmpty() || itemCount < 0) {
        return;
    }
    
    QString announcement = queueName + ": " + QString::number(itemCount) + " item";
    if (itemCount != 1) {
        announcement += "s";
    }
    announcement += " " + action;
    
    if (totalItems > 0) {
        announcement += " (" + QString::number(totalItems) + " total)";
    }
    
    // Send to audio feedback service
    if (m_audioFeedbackService) {
        m_audioFeedbackService->announcePlaylistModification(action, itemCount, queueName);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::QueueUpdate);
}

void LiveRegionManager::announceCriticalAlert(const QString& alertMessage, const QString& alertType)
{
    if (alertMessage.isEmpty()) {
        return;
    }
    
    QString announcement = "ALERT";
    if (!alertType.isEmpty()) {
        announcement += " (" + alertType + ")";
    }
    announcement += ": " + alertMessage;
    
    // Send to audio feedback service with critical priority
    if (m_audioFeedbackService) {
        m_audioFeedbackService->announceCriticalAlert(alertMessage, true);
    }
    
    emit criticalAnnouncementMade(announcement);
    emit liveRegionUpdated(nullptr, announcement, UpdateType::CriticalAlert);
}

void LiveRegionManager::announceCountdown(int secondsRemaining, const QString& eventDescription)
{
    if (secondsRemaining < 0) {
        return;
    }
    
    // Only announce if within threshold
    if (secondsRemaining > m_config.countdownThresholdSeconds) {
        return;
    }
    
    QString announcement;
    if (secondsRemaining == 0) {
        announcement = "NOW";
        if (!eventDescription.isEmpty()) {
            announcement += ": " + eventDescription;
        }
    } else {
        announcement = QString::number(secondsRemaining) + " second";
        if (secondsRemaining != 1) {
            announcement += "s";
        }
        if (!eventDescription.isEmpty()) {
            announcement += " until " + eventDescription;
        }
    }
    
    // Send to audio feedback service with high priority
    if (m_audioFeedbackService) {
        m_audioFeedbackService->queueAnnouncement(announcement, AudioFeedbackService::Priority::High, 
                                                 AudioFeedbackService::FeedbackType::CriticalAlert);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::CountdownTimer);
}

void LiveRegionManager::requestTimeAnnouncement(bool includeRemaining)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Throttle manual time requests (minimum 1 second between requests)
    if (currentTime - m_lastTimeAnnouncementRequest < 1000) {
        return;
    }
    
    m_lastTimeAnnouncementRequest = currentTime;
    
    // Use last known time info or request current info
    QString announcement = m_lastTimeInfo;
    if (announcement.isEmpty()) {
        announcement = "Time information not available";
    }
    
    // Send to audio feedback service with normal priority
    if (m_audioFeedbackService) {
        m_audioFeedbackService->queueAnnouncement(announcement, AudioFeedbackService::Priority::Normal, 
                                                 AudioFeedbackService::FeedbackType::StatusChange);
    }
    
    emit liveRegionUpdated(nullptr, announcement, UpdateType::TimeUpdate);
}

// getConfig() method is already defined inline in the header file

void LiveRegionManager::setConfig(const LiveRegionConfig& config)
{
    m_config = config;
    
    // Update timer intervals
    if (m_updateTimer) {
        // Update timer interval is fixed, but we can enable/disable
        if (config.enabled) {
            if (!m_updateTimer->isActive()) {
                m_updateTimer->start();
            }
        } else {
            m_updateTimer->stop();
        }
    }
    
    if (m_timeUpdateTimer) {
        m_timeUpdateTimer->setInterval(config.timeUpdateIntervalMs);
        if (config.enabled && config.timeUpdateIntervalMs > 0) {
            if (!m_timeUpdateTimer->isActive()) {
                m_timeUpdateTimer->start();
            }
        } else {
            m_timeUpdateTimer->stop();
        }
    }
    
    // Save configuration
    saveConfiguration();
}

void LiveRegionManager::setEnabled(bool enabled)
{
    if (m_config.enabled == enabled) {
        return;
    }
    
    m_config.enabled = enabled;
    
    if (enabled) {
        if (m_updateTimer && !m_updateTimer->isActive()) {
            m_updateTimer->start();
        }
        if (m_timeUpdateTimer && m_config.timeUpdateIntervalMs > 0 && !m_timeUpdateTimer->isActive()) {
            m_timeUpdateTimer->start();
        }
    } else {
        if (m_updateTimer) {
            m_updateTimer->stop();
        }
        if (m_timeUpdateTimer) {
            m_timeUpdateTimer->stop();
        }
        
        // Clear pending updates
        clearAllPendingUpdates();
    }
    
    saveConfiguration();
}

QList<LiveRegionManager::LiveRegionInfo> LiveRegionManager::getLiveRegions() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_updateMutex));
    return m_liveRegions.values();
}

int LiveRegionManager::getLiveRegionCount() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_updateMutex));
    return m_liveRegions.size();
}

void LiveRegionManager::clearPendingUpdates(QWidget* widget)
{
    QMutexLocker locker(&m_updateMutex);
    
    // Remove pending updates for the specific widget
    auto it = m_pendingUpdates.begin();
    while (it != m_pendingUpdates.end()) {
        if (it->widget == widget) {
            it = m_pendingUpdates.erase(it);
        } else {
            ++it;
        }
    }
    
    // Reset pending count for the widget
    if (m_liveRegions.contains(widget)) {
        m_liveRegions[widget].pendingUpdates = 0;
    }
}

void LiveRegionManager::clearAllPendingUpdates()
{
    QMutexLocker locker(&m_updateMutex);
    
    m_pendingUpdates.clear();
    
    // Reset pending counts for all widgets
    for (auto& info : m_liveRegions) {
        info.pendingUpdates = 0;
    }
}

void LiveRegionManager::onAccessibilityStateChanged(bool enabled)
{
    qDebug() << "LiveRegionManager: Accessibility state changed to" << enabled;
    
    if (!enabled) {
        // Clear all pending updates when accessibility is disabled
        clearAllPendingUpdates();
    }
}

void LiveRegionManager::onVerbosityLevelChanged(int level)
{
    m_currentVerbosityLevel = level;
    qDebug() << "LiveRegionManager: Verbosity level changed to" << level;
}

void LiveRegionManager::processPendingUpdates()
{
    if (m_isProcessingUpdates || !m_config.enabled) {
        return;
    }
    
    QMutexLocker locker(&m_updateMutex);
    
    if (m_pendingUpdates.isEmpty()) {
        return;
    }
    
    m_isProcessingUpdates = true;
    
    // Process a limited number of updates per cycle to avoid blocking
    const int maxUpdatesPerCycle = 5;
    int updatesProcessed = 0;
    
    while (!m_pendingUpdates.isEmpty() && updatesProcessed < maxUpdatesPerCycle) {
        PendingUpdate update = m_pendingUpdates.dequeue();
        
        // Check if widget still exists and is registered
        if (update.widget && m_liveRegions.contains(update.widget)) {
            // Check if update is still relevant (not too old)
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            if (currentTime - update.timestamp < EXPIRED_UPDATE_CLEANUP_MS) {
                processLiveRegionUpdate(update.widget, update.content, update.updateType);
                updatesProcessed++;
            }
            
            // Decrement pending count
            if (m_liveRegions[update.widget].pendingUpdates > 0) {
                m_liveRegions[update.widget].pendingUpdates--;
            }
        }
    }
    
    m_isProcessingUpdates = false;
    
    // Clean up expired updates periodically
    if (m_updatesProcessed % 50 == 0) {
        cleanupExpiredUpdates();
    }
}

void LiveRegionManager::onWidgetDestroyed(QObject* obj)
{
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (widget) {
        removeLiveRegion(widget);
    }
}

void LiveRegionManager::onTimeUpdateTimer()
{
    // This timer can be used for periodic time announcements if needed
    // Currently, time updates are handled on-demand or through external triggers
}

bool LiveRegionManager::shouldThrottleUpdate(QWidget* widget, UpdateType updateType, bool isCritical) const
{
    if (isCritical && m_config.prioritizeCriticalUpdates) {
        return false;
    }
    
    if (!m_liveRegions.contains(widget)) {
        return false;
    }
    
    const LiveRegionInfo& info = m_liveRegions[widget];
    
    // Check time-based throttling
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeSinceLastUpdate = currentTime - info.lastUpdateTime;
    
    if (timeSinceLastUpdate < m_config.updateThrottleMs) {
        return true;
    }
    
    // Check pending update count
    if (info.pendingUpdates >= m_config.maxPendingUpdates) {
        return true;
    }
    
    return false;
}

void LiveRegionManager::addPendingUpdate(const PendingUpdate& update)
{
    // Check if we're at the limit
    if (m_pendingUpdates.size() >= m_config.maxPendingUpdates * m_liveRegions.size()) {
        // Remove oldest non-critical update
        auto it = m_pendingUpdates.begin();
        while (it != m_pendingUpdates.end()) {
            if (!it->isCritical) {
                m_pendingUpdates.erase(it);
                m_updatesThrottled++;
                emit updateThrottled(update.widget, update.content);
                break;
            }
            ++it;
        }
    }
    
    m_pendingUpdates.enqueue(update);
    
    // Increment pending count for the widget
    if (m_liveRegions.contains(update.widget)) {
        m_liveRegions[update.widget].pendingUpdates++;
    }
}

void LiveRegionManager::processLiveRegionUpdate(QWidget* widget, const QString& content, UpdateType updateType)
{
    if (!widget || !m_liveRegions.contains(widget)) {
        return;
    }
    
    LiveRegionInfo& info = m_liveRegions[widget];
    
    // Format content for announcement
    QString formattedContent = formatContentForAnnouncement(content, updateType);
    
    // Update live region info
    info.lastContent = formattedContent;
    info.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    
    // Send to Qt accessibility framework
    sendToAccessibilityFramework(widget, formattedContent);
    
    m_updatesProcessed++;
    emit liveRegionUpdated(widget, formattedContent, updateType);
}

void LiveRegionManager::sendToAccessibilityFramework(QWidget* widget, const QString& content)
{
    if (!widget || content.isEmpty()) {
        return;
    }
    
    // Create and post accessibility event
    QAccessibleEvent event(widget, QAccessible::ObjectShow);
    QAccessible::updateAccessibility(&event);
    
    // Also update the accessible description
    if (QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(widget)) {
        // The content will be announced by the screen reader
        qDebug() << "LiveRegionManager: Updated live region content for" << widget->objectName() << ":" << content;
    }
}

QString LiveRegionManager::formatContentForAnnouncement(const QString& content, UpdateType updateType) const
{
    if (content.isEmpty()) {
        return content;
    }
    
    // Apply verbosity-based formatting
    QString formatted = content;
    
    // Add context based on update type and verbosity level
    if (m_currentVerbosityLevel >= 2) { // Verbose
        switch (updateType) {
            case UpdateType::PlaybackStatus:
                // Content already formatted in announcePlaybackChange
                break;
            case UpdateType::TimeUpdate:
                // Content already formatted in announceTimeUpdate
                break;
            case UpdateType::SystemStatus:
                formatted = "System update: " + formatted;
                break;
            case UpdateType::DatabaseProgress:
                formatted = "Database operation: " + formatted;
                break;
            case UpdateType::QueueUpdate:
                formatted = "Queue update: " + formatted;
                break;
            case UpdateType::CriticalAlert:
                formatted = "Critical alert: " + formatted;
                break;
            case UpdateType::CountdownTimer:
                formatted = "Countdown: " + formatted;
                break;
            case UpdateType::GeneralStatus:
            default:
                // No additional formatting needed
                break;
        }
    }
    
    return formatted;
}

LiveRegionManager::LiveRegionType LiveRegionManager::getRecommendedLiveRegionType(UpdateType updateType) const
{
    switch (updateType) {
        case UpdateType::CriticalAlert:
        case UpdateType::CountdownTimer:
            return LiveRegionType::Assertive;
            
        case UpdateType::PlaybackStatus:
        case UpdateType::SystemStatus:
        case UpdateType::QueueUpdate:
            return LiveRegionType::Polite;
            
        case UpdateType::TimeUpdate:
        case UpdateType::DatabaseProgress:
        case UpdateType::GeneralStatus:
        default:
            return LiveRegionType::Polite;
    }
}

void LiveRegionManager::loadConfiguration()
{
    QSettings settings;
    settings.beginGroup("LiveRegionManager");
    
    m_config.enabled = settings.value("enabled", true).toBool();
    m_config.updateThrottleMs = settings.value("updateThrottleMs", DEFAULT_UPDATE_THROTTLE_MS).toInt();
    m_config.timeUpdateIntervalMs = settings.value("timeUpdateIntervalMs", DEFAULT_TIME_UPDATE_INTERVAL_MS).toInt();
    m_config.countdownThresholdSeconds = settings.value("countdownThresholdSeconds", DEFAULT_COUNTDOWN_THRESHOLD_SECONDS).toInt();
    m_config.allowTimeUpdatesOnDemand = settings.value("allowTimeUpdatesOnDemand", true).toBool();
    m_config.maxPendingUpdates = settings.value("maxPendingUpdates", MAX_PENDING_UPDATES_PER_REGION).toInt();
    m_config.prioritizeCriticalUpdates = settings.value("prioritizeCriticalUpdates", true).toBool();
    
    settings.endGroup();
}

void LiveRegionManager::saveConfiguration()
{
    QSettings settings;
    settings.beginGroup("LiveRegionManager");
    
    settings.setValue("enabled", m_config.enabled);
    settings.setValue("updateThrottleMs", m_config.updateThrottleMs);
    settings.setValue("timeUpdateIntervalMs", m_config.timeUpdateIntervalMs);
    settings.setValue("countdownThresholdSeconds", m_config.countdownThresholdSeconds);
    settings.setValue("allowTimeUpdatesOnDemand", m_config.allowTimeUpdatesOnDemand);
    settings.setValue("maxPendingUpdates", m_config.maxPendingUpdates);
    settings.setValue("prioritizeCriticalUpdates", m_config.prioritizeCriticalUpdates);
    
    settings.endGroup();
}

void LiveRegionManager::cleanupExpiredUpdates()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    auto it = m_pendingUpdates.begin();
    while (it != m_pendingUpdates.end()) {
        if (currentTime - it->timestamp > EXPIRED_UPDATE_CLEANUP_MS) {
            it = m_pendingUpdates.erase(it);
            m_updatesThrottled++;
        } else {
            ++it;
        }
    }
}