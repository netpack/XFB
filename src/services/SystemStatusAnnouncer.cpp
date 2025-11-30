#include "SystemStatusAnnouncer.h"
#include "LiveRegionManager.h"
#include "AccessibilityManager.h"
#include "AudioFeedbackService.h"
#include "ServiceContainer.h"
#include <QDateTime>
#include <QDebug>
#include <QSettings>

SystemStatusAnnouncer::SystemStatusAnnouncer(QObject* parent)
    : BaseService(parent)
    , m_liveRegionManager(nullptr)
    , m_accessibilityManager(nullptr)
    , m_audioFeedbackService(nullptr)
    , m_queueProcessingTimer(nullptr)
    , m_progressUpdateTimer(nullptr)
    , m_criticalAlertRepeatTimer(nullptr)
    , m_isProcessingQueue(false)
    , m_currentVerbosityLevel(1)
    , m_hasPendingCriticalAlert(false)
    , m_announcementsProcessed(0)
    , m_criticalAlertsAnnounced(0)
    , m_lastProgressUpdate(0)
{
    // Initialize default configuration
    m_config.enabled = true;
    m_config.recordingStatusEnabled = true;
    m_config.streamingStatusEnabled = true;
    m_config.databaseProgressEnabled = true;
    m_config.networkStatusEnabled = true;
    m_config.criticalAlertsEnabled = true;
    m_config.progressUpdatesEnabled = true;
    m_config.progressUpdateIntervalMs = DEFAULT_PROGRESS_UPDATE_INTERVAL_MS;
    m_config.criticalAlertRepeatIntervalMs = DEFAULT_CRITICAL_ALERT_REPEAT_INTERVAL_MS;
    m_config.allowCriticalInterruption = true;
    m_config.maxQueuedAnnouncements = MAX_QUEUED_ANNOUNCEMENTS;
}

SystemStatusAnnouncer::~SystemStatusAnnouncer()
{
    shutdown();
}

bool SystemStatusAnnouncer::doInitialize()
{
    qDebug() << "SystemStatusAnnouncer: Initializing system status announcements";
    
    try {
        // Get service dependencies
        auto* serviceContainer = ServiceContainer::instance();
        m_liveRegionManager = serviceContainer->resolve<LiveRegionManager>();
        m_accessibilityManager = serviceContainer->resolve<AccessibilityManager>();
        m_audioFeedbackService = serviceContainer->resolve<AudioFeedbackService>();
        
        if (!m_liveRegionManager) {
            qWarning() << "SystemStatusAnnouncer: LiveRegionManager not available";
            return false;
        }
        
        if (!m_accessibilityManager) {
            qWarning() << "SystemStatusAnnouncer: AccessibilityManager not available";
            return false;
        }
        
        if (!m_audioFeedbackService) {
            qWarning() << "SystemStatusAnnouncer: AudioFeedbackService not available";
            return false;
        }
        
        // Initialize announcement system
        initializeAnnouncementSystem();
        
        // Connect to accessibility manager signals
        connect(m_accessibilityManager, &AccessibilityManager::accessibilityStateChanged,
                this, &SystemStatusAnnouncer::onAccessibilityStateChanged);
        connect(m_accessibilityManager, &AccessibilityManager::verbosityLevelChanged,
                this, [this](AccessibilityManager::VerbosityLevel level) {
                    onVerbosityLevelChanged(static_cast<int>(level));
                });
        
        // Load configuration
        loadConfiguration();
        
        qDebug() << "SystemStatusAnnouncer: Initialization completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "SystemStatusAnnouncer: Exception during initialization:" << e.what();
        return false;
    }
}

void SystemStatusAnnouncer::doShutdown()
{
    qDebug() << "SystemStatusAnnouncer: Shutting down system status announcements";
    
    // Save configuration
    saveConfiguration();
    
    // Shutdown announcement system
    shutdownAnnouncementSystem();
    
    // Clear announcement queue
    clearAnnouncementQueue(false);
    
    // Reset service dependencies
    m_liveRegionManager = nullptr;
    m_accessibilityManager = nullptr;
    m_audioFeedbackService = nullptr;
    
    qDebug() << "SystemStatusAnnouncer: Shutdown completed";
}

QString SystemStatusAnnouncer::getServiceName() const
{
    return "SystemStatusAnnouncer";
}

void SystemStatusAnnouncer::initializeAnnouncementSystem()
{
    // Create queue processing timer
    m_queueProcessingTimer = new QTimer(this);
    m_queueProcessingTimer->setInterval(QUEUE_PROCESSING_INTERVAL_MS);
    m_queueProcessingTimer->setSingleShot(false);
    connect(m_queueProcessingTimer, &QTimer::timeout, this, &SystemStatusAnnouncer::processAnnouncementQueue);
    
    // Create progress update timer
    m_progressUpdateTimer = new QTimer(this);
    m_progressUpdateTimer->setInterval(m_config.progressUpdateIntervalMs);
    m_progressUpdateTimer->setSingleShot(false);
    connect(m_progressUpdateTimer, &QTimer::timeout, this, &SystemStatusAnnouncer::onProgressUpdateTimer);
    
    // Create critical alert repeat timer
    m_criticalAlertRepeatTimer = new QTimer(this);
    m_criticalAlertRepeatTimer->setInterval(m_config.criticalAlertRepeatIntervalMs);
    m_criticalAlertRepeatTimer->setSingleShot(false);
    connect(m_criticalAlertRepeatTimer, &QTimer::timeout, this, &SystemStatusAnnouncer::onCriticalAlertRepeatTimer);
    
    // Start timers if enabled
    if (m_config.enabled) {
        m_queueProcessingTimer->start();
        if (m_config.progressUpdatesEnabled) {
            m_progressUpdateTimer->start();
        }
    }
}

void SystemStatusAnnouncer::shutdownAnnouncementSystem()
{
    // Stop and cleanup timers
    if (m_queueProcessingTimer) {
        m_queueProcessingTimer->stop();
        m_queueProcessingTimer->deleteLater();
        m_queueProcessingTimer = nullptr;
    }
    
    if (m_progressUpdateTimer) {
        m_progressUpdateTimer->stop();
        m_progressUpdateTimer->deleteLater();
        m_progressUpdateTimer = nullptr;
    }
    
    if (m_criticalAlertRepeatTimer) {
        m_criticalAlertRepeatTimer->stop();
        m_criticalAlertRepeatTimer->deleteLater();
        m_criticalAlertRepeatTimer = nullptr;
    }
    
    // Reset processing state
    m_isProcessingQueue = false;
    m_hasPendingCriticalAlert = false;
}

void SystemStatusAnnouncer::announceRecordingStatus(const QString& status, const QString& recordingName, const QString& additionalInfo)
{
    if (!m_config.enabled || !m_config.recordingStatusEnabled) {
        return;
    }
    
    QString component = "Recording";
    if (!recordingName.isEmpty()) {
        component += " (" + recordingName + ")";
    }
    
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::Recorder, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::Recorder;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceStreamingStatus(const QString& status, const QString& streamName, const QString& additionalInfo)
{
    if (!m_config.enabled || !m_config.streamingStatusEnabled) {
        return;
    }
    
    QString component = "Streaming";
    if (!streamName.isEmpty()) {
        component += " (" + streamName + ")";
    }
    
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::Streamer, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::Streamer;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceDatabaseProgress(const QString& operation, int progress, int itemsProcessed, int totalItems, const QString& estimatedTimeRemaining)
{
    if (!m_config.enabled || !m_config.databaseProgressEnabled) {
        return;
    }
    
    // Throttle progress updates to avoid spam
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastProgressUpdate < m_config.progressUpdateIntervalMs) {
        return;
    }
    
    m_lastProgressUpdate = currentTime;
    
    QString additionalInfo;
    if (itemsProcessed >= 0 && totalItems > 0) {
        additionalInfo = QString("(%1 of %2)").arg(itemsProcessed).arg(totalItems);
    }
    if (!estimatedTimeRemaining.isEmpty()) {
        if (!additionalInfo.isEmpty()) {
            additionalInfo += " ";
        }
        additionalInfo += estimatedTimeRemaining;
    }
    
    // Use live region manager directly for progress updates
    if (m_liveRegionManager) {
        m_liveRegionManager->announceDatabaseProgress(operation, progress, itemsProcessed, totalItems);
    }
    
    emit databaseProgressAnnounced(operation, progress);
}

void SystemStatusAnnouncer::announceDatabaseCompletion(const QString& operation, bool success, const QString& resultMessage)
{
    if (!m_config.enabled || !m_config.databaseProgressEnabled) {
        return;
    }
    
    QString status = success ? "completed" : "failed";
    StatusType statusType = success ? StatusType::Completed : StatusType::Failed;
    Priority priority = success ? Priority::Normal : Priority::High;
    
    QueuedAnnouncement announcement;
    announcement.component = "Database " + operation;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = resultMessage;
    announcement.componentType = ComponentType::Database;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceNetworkStatus(const QString& component, const QString& status, const QString& additionalInfo)
{
    if (!m_config.enabled || !m_config.networkStatusEnabled) {
        return;
    }
    
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::Network, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::Network;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceFileSystemOperation(const QString& operation, const QString& fileName, const QString& status, const QString& additionalInfo)
{
    if (!m_config.enabled) {
        return;
    }
    
    QString component = operation;
    if (!fileName.isEmpty()) {
        component += " (" + fileName + ")";
    }
    
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::FileSystem, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::FileSystem;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceAudioDeviceStatus(const QString& deviceName, const QString& status, const QString& deviceType)
{
    if (!m_config.enabled) {
        return;
    }
    
    QString component = "Audio Device";
    if (!deviceType.isEmpty()) {
        component += " (" + deviceType + ")";
    }
    if (!deviceName.isEmpty()) {
        component += ": " + deviceName;
    }
    
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::AudioDevice, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.componentType = ComponentType::AudioDevice;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceSchedulerEvent(const QString& eventType, const QString& programName, const QString& scheduledTime)
{
    if (!m_config.enabled) {
        return;
    }
    
    QString component = "Scheduler";
    QString additionalInfo;
    
    if (!programName.isEmpty()) {
        additionalInfo = programName;
    }
    if (!scheduledTime.isEmpty()) {
        if (!additionalInfo.isEmpty()) {
            additionalInfo += " at ";
        }
        additionalInfo += scheduledTime;
    }
    
    StatusType statusType = parseStatusType(eventType);
    Priority priority = getPriorityForStatus(ComponentType::Scheduler, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = eventType;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::Scheduler;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceCriticalAlert(const QString& alertMessage, const QString& alertType, bool requiresImmediateAction)
{
    if (!m_config.enabled || !m_config.criticalAlertsEnabled) {
        return;
    }
    
    Priority priority = requiresImmediateAction ? Priority::Emergency : Priority::Critical;
    
    QueuedAnnouncement announcement;
    announcement.component = "System Alert";
    announcement.status = alertMessage;
    announcement.priority = priority;
    announcement.additionalInfo = alertType;
    announcement.componentType = ComponentType::SystemResource;
    announcement.statusType = StatusType::Critical;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    announcement.isRepeating = true;
    announcement.repeatCount = 0;
    
    // Store for potential repeating
    m_lastCriticalAlert = announcement;
    m_hasPendingCriticalAlert = true;
    
    // Process immediately
    processAnnouncement(announcement);
    
    // Start repeat timer if configured
    if (m_config.criticalAlertRepeatIntervalMs > 0 && m_criticalAlertRepeatTimer) {
        m_criticalAlertRepeatTimer->start();
    }
    
    // Use live region manager for critical alerts
    if (m_liveRegionManager) {
        m_liveRegionManager->announceCriticalAlert(alertMessage, alertType);
    }
    
    emit criticalAlertAnnounced(alertMessage, alertType);
    m_criticalAlertsAnnounced++;
}

void SystemStatusAnnouncer::announceResourceStatus(const QString& resourceType, const QString& status, int usagePercentage)
{
    if (!m_config.enabled) {
        return;
    }
    
    QString component = resourceType + " Resource";
    QString additionalInfo;
    
    if (usagePercentage >= 0) {
        additionalInfo = QString("%1% usage").arg(usagePercentage);
    }
    
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::SystemResource, statusType);
    
    // Increase priority for high resource usage
    if (usagePercentage >= 90) {
        priority = Priority::High;
    } else if (usagePercentage >= 95) {
        priority = Priority::Critical;
    }
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::SystemResource;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceExternalProcessStatus(const QString& processName, const QString& status, const QString& additionalInfo)
{
    if (!m_config.enabled) {
        return;
    }
    
    QString component = "Process: " + processName;
    StatusType statusType = parseStatusType(status);
    Priority priority = getPriorityForStatus(ComponentType::ExternalProcess, statusType);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::ExternalProcess;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

void SystemStatusAnnouncer::announceSystemStatus(const QString& component, const QString& status, Priority priority, const QString& additionalInfo)
{
    if (!m_config.enabled) {
        return;
    }
    
    StatusType statusType = parseStatusType(status);
    
    QueuedAnnouncement announcement;
    announcement.component = component;
    announcement.status = status;
    announcement.priority = priority;
    announcement.additionalInfo = additionalInfo;
    announcement.componentType = ComponentType::SystemResource;
    announcement.statusType = statusType;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    if (shouldProcessImmediately(priority)) {
        processAnnouncement(announcement);
    } else {
        queueAnnouncement(announcement);
    }
}

// getConfig() method is already defined inline in the header file

void SystemStatusAnnouncer::setConfig(const AnnouncementConfig& config)
{
    m_config = config;
    
    // Update timer intervals
    if (m_progressUpdateTimer) {
        m_progressUpdateTimer->setInterval(config.progressUpdateIntervalMs);
        if (config.enabled && config.progressUpdatesEnabled) {
            if (!m_progressUpdateTimer->isActive()) {
                m_progressUpdateTimer->start();
            }
        } else {
            m_progressUpdateTimer->stop();
        }
    }
    
    if (m_criticalAlertRepeatTimer) {
        m_criticalAlertRepeatTimer->setInterval(config.criticalAlertRepeatIntervalMs);
    }
    
    if (m_queueProcessingTimer) {
        if (config.enabled) {
            if (!m_queueProcessingTimer->isActive()) {
                m_queueProcessingTimer->start();
            }
        } else {
            m_queueProcessingTimer->stop();
        }
    }
    
    saveConfiguration();
}

void SystemStatusAnnouncer::setEnabled(bool enabled)
{
    if (m_config.enabled == enabled) {
        return;
    }
    
    m_config.enabled = enabled;
    
    if (enabled) {
        if (m_queueProcessingTimer && !m_queueProcessingTimer->isActive()) {
            m_queueProcessingTimer->start();
        }
        if (m_progressUpdateTimer && m_config.progressUpdatesEnabled && !m_progressUpdateTimer->isActive()) {
            m_progressUpdateTimer->start();
        }
    } else {
        if (m_queueProcessingTimer) {
            m_queueProcessingTimer->stop();
        }
        if (m_progressUpdateTimer) {
            m_progressUpdateTimer->stop();
        }
        if (m_criticalAlertRepeatTimer) {
            m_criticalAlertRepeatTimer->stop();
        }
        
        // Clear queue when disabled
        clearAnnouncementQueue(false);
    }
    
    saveConfiguration();
}

void SystemStatusAnnouncer::setRecordingStatusEnabled(bool enabled)
{
    if (m_config.recordingStatusEnabled != enabled) {
        m_config.recordingStatusEnabled = enabled;
        saveConfiguration();
    }
}

void SystemStatusAnnouncer::setStreamingStatusEnabled(bool enabled)
{
    if (m_config.streamingStatusEnabled != enabled) {
        m_config.streamingStatusEnabled = enabled;
        saveConfiguration();
    }
}

void SystemStatusAnnouncer::setDatabaseProgressEnabled(bool enabled)
{
    if (m_config.databaseProgressEnabled != enabled) {
        m_config.databaseProgressEnabled = enabled;
        saveConfiguration();
    }
}

void SystemStatusAnnouncer::setCriticalAlertsEnabled(bool enabled)
{
    if (m_config.criticalAlertsEnabled != enabled) {
        m_config.criticalAlertsEnabled = enabled;
        
        if (!enabled && m_criticalAlertRepeatTimer) {
            m_criticalAlertRepeatTimer->stop();
            m_hasPendingCriticalAlert = false;
        }
        
        saveConfiguration();
    }
}

void SystemStatusAnnouncer::clearAnnouncementQueue(bool preserveCritical)
{
    QMutexLocker locker(&m_queueMutex);
    
    if (!preserveCritical) {
        m_announcementQueue.clear();
        return;
    }
    
    // Remove non-critical announcements
    QQueue<QueuedAnnouncement> filteredQueue;
    while (!m_announcementQueue.isEmpty()) {
        QueuedAnnouncement announcement = m_announcementQueue.dequeue();
        if (announcement.priority >= Priority::Critical) {
            filteredQueue.enqueue(announcement);
        }
    }
    
    m_announcementQueue = filteredQueue;
}

int SystemStatusAnnouncer::getQueuedAnnouncementCount() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_queueMutex));
    return m_announcementQueue.size();
}

void SystemStatusAnnouncer::onAccessibilityStateChanged(bool enabled)
{
    qDebug() << "SystemStatusAnnouncer: Accessibility state changed to" << enabled;
    
    if (!enabled) {
        // Clear queue when accessibility is disabled
        clearAnnouncementQueue(false);
        
        // Stop critical alert repeating
        if (m_criticalAlertRepeatTimer) {
            m_criticalAlertRepeatTimer->stop();
        }
        m_hasPendingCriticalAlert = false;
    }
}

void SystemStatusAnnouncer::onVerbosityLevelChanged(int level)
{
    m_currentVerbosityLevel = level;
    qDebug() << "SystemStatusAnnouncer: Verbosity level changed to" << level;
}

void SystemStatusAnnouncer::processAnnouncementQueue()
{
    if (m_isProcessingQueue || !m_config.enabled) {
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    
    if (m_announcementQueue.isEmpty()) {
        return;
    }
    
    m_isProcessingQueue = true;
    
    // Process a limited number of announcements per cycle
    const int maxAnnouncementsPerCycle = 3;
    int processed = 0;
    
    while (!m_announcementQueue.isEmpty() && processed < maxAnnouncementsPerCycle) {
        QueuedAnnouncement announcement = m_announcementQueue.dequeue();
        processAnnouncement(announcement);
        processed++;
    }
    
    m_isProcessingQueue = false;
}

void SystemStatusAnnouncer::onProgressUpdateTimer()
{
    // This timer can be used for periodic progress checks if needed
    // Currently, progress updates are handled through direct calls
}

void SystemStatusAnnouncer::onCriticalAlertRepeatTimer()
{
    if (!m_hasPendingCriticalAlert || !m_config.criticalAlertsEnabled) {
        m_criticalAlertRepeatTimer->stop();
        return;
    }
    
    if (m_lastCriticalAlert.repeatCount >= MAX_CRITICAL_ALERT_REPEATS) {
        m_criticalAlertRepeatTimer->stop();
        m_hasPendingCriticalAlert = false;
        return;
    }
    
    // Repeat the critical alert
    m_lastCriticalAlert.repeatCount++;
    processAnnouncement(m_lastCriticalAlert);
}

void SystemStatusAnnouncer::queueAnnouncement(const QueuedAnnouncement& announcement)
{
    QMutexLocker locker(&m_queueMutex);
    
    // Check for queue overflow
    if (m_announcementQueue.size() >= m_config.maxQueuedAnnouncements) {
        handleQueueOverflow();
    }
    
    m_announcementQueue.enqueue(announcement);
}

void SystemStatusAnnouncer::processAnnouncement(const QueuedAnnouncement& announcement)
{
    QString message = formatAnnouncementMessage(
        announcement.component,
        announcement.status,
        announcement.additionalInfo,
        announcement.componentType,
        announcement.statusType
    );
    
    // Send to live region manager
    sendToLiveRegionManager(message, announcement.priority, announcement.componentType);
    
    // Send to audio feedback service
    if (m_audioFeedbackService) {
        AudioFeedbackService::Priority audioPriority;
        switch (announcement.priority) {
            case Priority::Low:
                audioPriority = AudioFeedbackService::Priority::Low;
                break;
            case Priority::Normal:
                audioPriority = AudioFeedbackService::Priority::Normal;
                break;
            case Priority::High:
                audioPriority = AudioFeedbackService::Priority::High;
                break;
            case Priority::Critical:
            case Priority::Emergency:
                audioPriority = AudioFeedbackService::Priority::Critical;
                break;
        }
        
        m_audioFeedbackService->queueAnnouncement(message, audioPriority, AudioFeedbackService::FeedbackType::StatusChange);
    }
    
    m_announcementsProcessed++;
    emit systemStatusAnnounced(announcement.component, announcement.status, announcement.priority);
}

QString SystemStatusAnnouncer::formatAnnouncementMessage(const QString& component, const QString& status, const QString& additionalInfo, ComponentType componentType, StatusType statusType) const
{
    QString message;
    
    // Format based on verbosity level
    switch (m_currentVerbosityLevel) {
        case 0: // Terse
            message = component + ": " + status;
            break;
            
        case 1: // Normal
            message = component + " " + status;
            if (!additionalInfo.isEmpty()) {
                message += " - " + additionalInfo;
            }
            break;
            
        case 2: // Verbose
            // Add context based on component type
            QString prefix;
            switch (componentType) {
                case ComponentType::Recorder:
                    prefix = "Recording system: ";
                    break;
                case ComponentType::Streamer:
                    prefix = "Streaming system: ";
                    break;
                case ComponentType::Database:
                    prefix = "Database: ";
                    break;
                case ComponentType::Network:
                    prefix = "Network: ";
                    break;
                case ComponentType::AudioDevice:
                    prefix = "Audio device: ";
                    break;
                case ComponentType::Scheduler:
                    prefix = "Scheduler: ";
                    break;
                case ComponentType::SystemResource:
                    prefix = "System: ";
                    break;
                default:
                    prefix = "System: ";
                    break;
            }
            
            message = prefix + component + " " + status;
            if (!additionalInfo.isEmpty()) {
                message += " (" + additionalInfo + ")";
            }
            break;
    }
    
    return message;
}

SystemStatusAnnouncer::Priority SystemStatusAnnouncer::getPriorityForStatus(ComponentType componentType, StatusType statusType) const
{
    // Determine priority based on component and status type
    switch (statusType) {
        case StatusType::Critical:
        case StatusType::Error:
            return Priority::Critical;
            
        case StatusType::Failed:
        case StatusType::Disconnected:
            if (componentType == ComponentType::Recorder || componentType == ComponentType::Streamer) {
                return Priority::High;
            }
            return Priority::Normal;
            
        case StatusType::Warning:
            return Priority::Normal;
            
        case StatusType::Started:
        case StatusType::Connected:
        case StatusType::Completed:
        case StatusType::Recovered:
            return Priority::Normal;
            
        case StatusType::Stopped:
        case StatusType::InProgress:
        case StatusType::Recovering:
        default:
            return Priority::Low;
    }
}

SystemStatusAnnouncer::StatusType SystemStatusAnnouncer::parseStatusType(const QString& status) const
{
    QString lowerStatus = status.toLower();
    
    if (lowerStatus.contains("start") || lowerStatus.contains("begin")) {
        return StatusType::Started;
    } else if (lowerStatus.contains("stop") || lowerStatus.contains("end")) {
        return StatusType::Stopped;
    } else if (lowerStatus.contains("connect")) {
        return StatusType::Connected;
    } else if (lowerStatus.contains("disconnect")) {
        return StatusType::Disconnected;
    } else if (lowerStatus.contains("progress") || lowerStatus.contains("running")) {
        return StatusType::InProgress;
    } else if (lowerStatus.contains("complet") || lowerStatus.contains("finish")) {
        return StatusType::Completed;
    } else if (lowerStatus.contains("fail")) {
        return StatusType::Failed;
    } else if (lowerStatus.contains("error")) {
        return StatusType::Error;
    } else if (lowerStatus.contains("critical") || lowerStatus.contains("alert")) {
        return StatusType::Critical;
    } else if (lowerStatus.contains("warn")) {
        return StatusType::Warning;
    } else if (lowerStatus.contains("recover")) {
        return lowerStatus.contains("ing") ? StatusType::Recovering : StatusType::Recovered;
    }
    
    return StatusType::InProgress;
}

bool SystemStatusAnnouncer::shouldProcessImmediately(Priority priority) const
{
    return priority >= Priority::High || (m_config.allowCriticalInterruption && priority >= Priority::Critical);
}

int SystemStatusAnnouncer::handleQueueOverflow()
{
    int removed = 0;
    
    // Remove low-priority announcements first
    QQueue<QueuedAnnouncement> filteredQueue;
    while (!m_announcementQueue.isEmpty()) {
        QueuedAnnouncement announcement = m_announcementQueue.dequeue();
        if (announcement.priority >= Priority::Normal) {
            filteredQueue.enqueue(announcement);
        } else {
            removed++;
        }
    }
    
    m_announcementQueue = filteredQueue;
    return removed;
}

void SystemStatusAnnouncer::loadConfiguration()
{
    QSettings settings;
    settings.beginGroup("SystemStatusAnnouncer");
    
    m_config.enabled = settings.value("enabled", true).toBool();
    m_config.recordingStatusEnabled = settings.value("recordingStatusEnabled", true).toBool();
    m_config.streamingStatusEnabled = settings.value("streamingStatusEnabled", true).toBool();
    m_config.databaseProgressEnabled = settings.value("databaseProgressEnabled", true).toBool();
    m_config.networkStatusEnabled = settings.value("networkStatusEnabled", true).toBool();
    m_config.criticalAlertsEnabled = settings.value("criticalAlertsEnabled", true).toBool();
    m_config.progressUpdatesEnabled = settings.value("progressUpdatesEnabled", true).toBool();
    m_config.progressUpdateIntervalMs = settings.value("progressUpdateIntervalMs", DEFAULT_PROGRESS_UPDATE_INTERVAL_MS).toInt();
    m_config.criticalAlertRepeatIntervalMs = settings.value("criticalAlertRepeatIntervalMs", DEFAULT_CRITICAL_ALERT_REPEAT_INTERVAL_MS).toInt();
    m_config.allowCriticalInterruption = settings.value("allowCriticalInterruption", true).toBool();
    m_config.maxQueuedAnnouncements = settings.value("maxQueuedAnnouncements", MAX_QUEUED_ANNOUNCEMENTS).toInt();
    
    settings.endGroup();
}

void SystemStatusAnnouncer::saveConfiguration()
{
    QSettings settings;
    settings.beginGroup("SystemStatusAnnouncer");
    
    settings.setValue("enabled", m_config.enabled);
    settings.setValue("recordingStatusEnabled", m_config.recordingStatusEnabled);
    settings.setValue("streamingStatusEnabled", m_config.streamingStatusEnabled);
    settings.setValue("databaseProgressEnabled", m_config.databaseProgressEnabled);
    settings.setValue("networkStatusEnabled", m_config.networkStatusEnabled);
    settings.setValue("criticalAlertsEnabled", m_config.criticalAlertsEnabled);
    settings.setValue("progressUpdatesEnabled", m_config.progressUpdatesEnabled);
    settings.setValue("progressUpdateIntervalMs", m_config.progressUpdateIntervalMs);
    settings.setValue("criticalAlertRepeatIntervalMs", m_config.criticalAlertRepeatIntervalMs);
    settings.setValue("allowCriticalInterruption", m_config.allowCriticalInterruption);
    settings.setValue("maxQueuedAnnouncements", m_config.maxQueuedAnnouncements);
    
    settings.endGroup();
}

void SystemStatusAnnouncer::sendToLiveRegionManager(const QString& message, Priority priority, ComponentType componentType)
{
    if (!m_liveRegionManager) {
        return;
    }
    
    // Determine the appropriate live region update type
    LiveRegionManager::UpdateType updateType;
    switch (componentType) {
        case ComponentType::Database:
            updateType = LiveRegionManager::UpdateType::DatabaseProgress;
            break;
        case ComponentType::Network:
        case ComponentType::Streamer:
        case ComponentType::Recorder:
            updateType = LiveRegionManager::UpdateType::SystemStatus;
            break;
        default:
            if (priority >= Priority::Critical) {
                updateType = LiveRegionManager::UpdateType::CriticalAlert;
            } else {
                updateType = LiveRegionManager::UpdateType::SystemStatus;
            }
            break;
    }
    
    // Send to live region manager
    if (priority >= Priority::Critical) {
        m_liveRegionManager->announceCriticalAlert(message);
    } else {
        m_liveRegionManager->announceSystemStatus("System", message);
    }
}