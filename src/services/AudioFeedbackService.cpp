#include "AudioFeedbackService.h"
#include "AccessibilityManager.h"
#include "ServiceContainer.h"
#include <QApplication>
#include <QAccessibleEvent>
#include <QDateTime>
#include <QSettings>
#include <QDebug>
#include <QMutexLocker>
#include <algorithm>

AudioFeedbackService::AudioFeedbackService(QObject* parent)
    : BaseService(parent)
    , m_processingTimer(nullptr)
    , m_isProcessing(false)
    , m_isInitialized(false)
    , m_accessibilityManager(nullptr)
    , m_lastAnnouncementTime(0)
    , m_announcementsProcessed(0)
    , m_announcementsDropped(0)
    , m_currentVerbosityLevel(1) // Normal verbosity by default
{
    // Initialize default configuration
    m_config = AnnouncementConfig();
}

AudioFeedbackService::~AudioFeedbackService()
{
    if (m_isInitialized) {
        shutdownAnnouncementSystem();
    }
}

bool AudioFeedbackService::doInitialize()
{
    qDebug() << "AudioFeedbackService: Initializing audio feedback service";
    
    try {
        // Get accessibility manager reference
        m_accessibilityManager = ServiceContainer::instance()->resolve<AccessibilityManager>();
        if (!m_accessibilityManager) {
            qWarning() << "AudioFeedbackService: AccessibilityManager not available";
            return false;
        }
        
        // Load configuration from settings
        loadConfiguration();
        
        // Initialize announcement processing system
        initializeAnnouncementSystem();
        
        // Connect to accessibility manager signals
        connect(m_accessibilityManager, &AccessibilityManager::accessibilityStateChanged,
                this, &AudioFeedbackService::onAccessibilityStateChanged);
        
        connect(m_accessibilityManager, &AccessibilityManager::verbosityLevelChanged,
                this, [this](AccessibilityManager::VerbosityLevel level) {
                    onVerbosityLevelChanged(static_cast<int>(level));
                });
        
        // Set initial verbosity level
        m_currentVerbosityLevel = static_cast<int>(m_accessibilityManager->verbosityLevel());
        
        m_isInitialized = true;
        qDebug() << "AudioFeedbackService: Initialization completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "AudioFeedbackService: Exception during initialization:" << e.what();
        return false;
    }
}

void AudioFeedbackService::doShutdown()
{
    qDebug() << "AudioFeedbackService: Shutting down audio feedback service";
    
    if (m_isInitialized) {
        shutdownAnnouncementSystem();
        saveConfiguration();
        m_isInitialized = false;
    }
}

QString AudioFeedbackService::getServiceName() const
{
    return "AudioFeedbackService";
}

void AudioFeedbackService::initializeAnnouncementSystem()
{
    // Create and configure processing timer
    m_processingTimer = new QTimer(this);
    m_processingTimer->setInterval(m_config.processingIntervalMs);
    m_processingTimer->setSingleShot(false);
    
    connect(m_processingTimer, &QTimer::timeout,
            this, &AudioFeedbackService::processNextAnnouncement);
    
    // Start processing if enabled
    if (m_config.enabled && m_accessibilityManager && m_accessibilityManager->isAccessibilityEnabled()) {
        m_processingTimer->start();
    }
    
    qDebug() << "AudioFeedbackService: Announcement system initialized";
}

void AudioFeedbackService::shutdownAnnouncementSystem()
{
    if (m_processingTimer) {
        m_processingTimer->stop();
        m_processingTimer->deleteLater();
        m_processingTimer = nullptr;
    }
    
    // Clear any remaining announcements
    QMutexLocker locker(&m_queueMutex);
    m_announcementQueue.clear();
    m_isProcessing = false;
    
    qDebug() << "AudioFeedbackService: Announcement system shutdown completed";
}

void AudioFeedbackService::announceButtonClick(const QString& buttonText, const QString& confirmationMessage)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = buttonText;
    } else if (m_currentVerbosityLevel == 1) { // Normal
        message = QString("Button %1 activated").arg(buttonText);
        if (!confirmationMessage.isEmpty()) {
            message += QString(". %1").arg(confirmationMessage);
        }
    } else { // Verbose
        message = QString("Button %1 has been activated").arg(buttonText);
        if (!confirmationMessage.isEmpty()) {
            message += QString(". %1").arg(confirmationMessage);
        }
    }
    
    queueAnnouncement(message, Priority::Normal, FeedbackType::ButtonClick);
}

void AudioFeedbackService::announceMenuSelection(const QString& menuPath, const QString& description)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        // Extract just the final menu item
        QStringList parts = menuPath.split(" > ");
        message = parts.isEmpty() ? menuPath : parts.last();
    } else if (m_currentVerbosityLevel == 1) { // Normal
        message = QString("Menu %1 selected").arg(menuPath);
        if (!description.isEmpty()) {
            message += QString(". %1").arg(description);
        }
    } else { // Verbose
        message = QString("Menu item %1 has been selected").arg(menuPath);
        if (!description.isEmpty()) {
            message += QString(". %1").arg(description);
        }
    }
    
    queueAnnouncement(message, Priority::Normal, FeedbackType::MenuSelection);
}

void AudioFeedbackService::announceSaveOperation(const QString& itemType, const QString& itemName, bool success)
{
    QString message;
    
    if (success) {
        if (m_currentVerbosityLevel == 0) { // Terse
            message = QString("%1 saved").arg(itemName.isEmpty() ? itemType : itemName);
        } else if (m_currentVerbosityLevel == 1) { // Normal
            message = QString("%1 %2 saved successfully").arg(itemType, itemName);
        } else { // Verbose
            message = QString("The %1 %2 has been saved successfully").arg(itemType, itemName);
        }
    } else {
        message = QString("Failed to save %1 %2").arg(itemType, itemName);
    }
    
    Priority priority = success ? Priority::Normal : Priority::High;
    queueAnnouncement(message, priority, FeedbackType::SaveOperation);
}

void AudioFeedbackService::announceLoadOperation(const QString& itemType, const QString& itemName, bool success)
{
    QString message;
    
    if (success) {
        if (m_currentVerbosityLevel == 0) { // Terse
            message = QString("%1 loaded").arg(itemName.isEmpty() ? itemType : itemName);
        } else if (m_currentVerbosityLevel == 1) { // Normal
            message = QString("%1 %2 loaded successfully").arg(itemType, itemName);
        } else { // Verbose
            message = QString("The %1 %2 has been loaded successfully").arg(itemType, itemName);
        }
    } else {
        message = QString("Failed to load %1 %2").arg(itemType, itemName);
    }
    
    Priority priority = success ? Priority::Normal : Priority::High;
    queueAnnouncement(message, priority, FeedbackType::LoadOperation);
}

void AudioFeedbackService::announcePlaybackStateChange(const QString& newState, const QString& trackInfo)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = newState;
    } else if (m_currentVerbosityLevel == 1) { // Normal
        message = QString("Playback %1").arg(newState);
        if (!trackInfo.isEmpty()) {
            message += QString(". %1").arg(trackInfo);
        }
    } else { // Verbose
        message = QString("Playback state changed to %1").arg(newState);
        if (!trackInfo.isEmpty()) {
            message += QString(". Currently playing: %1").arg(trackInfo);
        }
    }
    
    queueAnnouncement(message, Priority::Normal, FeedbackType::PlaybackChange);
}

void AudioFeedbackService::announceTrackChange(const QString& trackTitle, const QString& artist, const QString& duration)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = trackTitle;
    } else {
        message = QString("Now playing: %1").arg(trackTitle);
        
        if (!artist.isEmpty()) {
            message += QString(" by %1").arg(artist);
        }
        
        if (!duration.isEmpty() && m_currentVerbosityLevel == 2) { // Verbose only
            message += QString(". Duration: %1").arg(duration);
        }
    }
    
    queueAnnouncement(message, Priority::Normal, FeedbackType::TrackChange);
}

void AudioFeedbackService::announcePlaylistModification(const QString& action, int itemCount, const QString& playlistName)
{
    QString message;
    QString itemText = (itemCount == 1) ? "item" : "items";
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = QString("%1 %2 %3").arg(QString::number(itemCount), itemText, action);
    } else if (m_currentVerbosityLevel == 1) { // Normal
        message = QString("%1 %2 %3").arg(QString::number(itemCount), itemText, action);
        if (!playlistName.isEmpty()) {
            message += QString(" in %1").arg(playlistName);
        }
    } else { // Verbose
        message = QString("%1 %2 have been %3").arg(QString::number(itemCount), itemText, action);
        if (!playlistName.isEmpty()) {
            message += QString(" in playlist %1").arg(playlistName);
        }
    }
    
    queueAnnouncement(message, Priority::Normal, FeedbackType::PlaylistModified);
}

void AudioFeedbackService::announceDatabaseOperation(const QString& operation, const QString& status, bool isComplete)
{
    QString message;
    
    if (isComplete) {
        if (m_currentVerbosityLevel == 0) { // Terse
            message = QString("%1 complete").arg(operation);
        } else {
            message = QString("Database %1 completed. %2").arg(operation, status);
        }
    } else {
        if (m_currentVerbosityLevel == 0) { // Terse
            message = status;
        } else {
            message = QString("Database %1: %2").arg(operation, status);
        }
    }
    
    Priority priority = isComplete ? Priority::Normal : Priority::Low;
    queueAnnouncement(message, priority, FeedbackType::DatabaseOperation);
}

void AudioFeedbackService::announceError(const QString& errorMessage, const QString& context)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = QString("Error: %1").arg(errorMessage);
    } else {
        message = QString("Error");
        if (!context.isEmpty()) {
            message += QString(" in %1").arg(context);
        }
        message += QString(": %1").arg(errorMessage);
    }
    
    queueAnnouncement(message, Priority::High, FeedbackType::ErrorMessage);
}

void AudioFeedbackService::announceProgressUpdate(const QString& operation, int percentage, const QString& estimatedTimeRemaining)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = QString("%1%").arg(percentage);
    } else if (m_currentVerbosityLevel == 1) { // Normal
        message = QString("%1 %2% complete").arg(operation, QString::number(percentage));
    } else { // Verbose
        message = QString("%1 is %2% complete").arg(operation, QString::number(percentage));
        if (!estimatedTimeRemaining.isEmpty()) {
            message += QString(". Estimated time remaining: %1").arg(estimatedTimeRemaining);
        }
    }
    
    queueAnnouncement(message, Priority::Low, FeedbackType::ProgressUpdate);
}

void AudioFeedbackService::announceCriticalAlert(const QString& alertMessage, bool requiresImmediateAttention)
{
    QString message;
    
    if (requiresImmediateAttention) {
        message = QString("Critical Alert: %1").arg(alertMessage);
    } else {
        message = QString("Alert: %1").arg(alertMessage);
    }
    
    queueAnnouncement(message, Priority::Critical, FeedbackType::CriticalAlert);
}

void AudioFeedbackService::announceStatusChange(const QString& component, const QString& newStatus, const QString& additionalInfo)
{
    QString message;
    
    if (m_currentVerbosityLevel == 0) { // Terse
        message = QString("%1 %2").arg(component, newStatus);
    } else if (m_currentVerbosityLevel == 1) { // Normal
        message = QString("%1 status: %2").arg(component, newStatus);
        if (!additionalInfo.isEmpty()) {
            message += QString(". %1").arg(additionalInfo);
        }
    } else { // Verbose
        message = QString("%1 status has changed to %2").arg(component, newStatus);
        if (!additionalInfo.isEmpty()) {
            message += QString(". %1").arg(additionalInfo);
        }
    }
    
    queueAnnouncement(message, Priority::Normal, FeedbackType::StatusChange);
}

void AudioFeedbackService::queueAnnouncement(const QString& message, Priority priority, FeedbackType feedbackType)
{
    if (!m_config.enabled || !m_isInitialized) {
        return;
    }
    
    // Validate message length
    QString processedMessage = message;
    if (processedMessage.length() > m_config.maxAnnouncementLengthChars) {
        processedMessage = processedMessage.left(m_config.maxAnnouncementLengthChars - 3) + "...";
    }
    
    // Format message based on verbosity
    processedMessage = formatMessageForVerbosity(processedMessage, feedbackType);
    
    AnnouncementItem item(processedMessage, priority, feedbackType);
    addToQueue(item);
}

void AudioFeedbackService::addToQueue(const AnnouncementItem& item)
{
    QMutexLocker locker(&m_queueMutex);
    
    // Handle critical announcements that should interrupt
    if (item.priority == Priority::Critical && shouldInterruptForPriority(item.priority)) {
        // Clear lower priority items and add critical item to front
        auto it = m_announcementQueue.begin();
        while (it != m_announcementQueue.end()) {
            if (static_cast<int>(it->priority) < static_cast<int>(Priority::Critical)) {
                it = m_announcementQueue.erase(it);
                m_announcementsDropped++;
            } else {
                ++it;
            }
        }
        
        m_announcementQueue.prepend(item);
        emit criticalAnnouncementInterrupted(item.message);
        return;
    }
    
    // Check queue size and handle overflow
    if (m_announcementQueue.size() >= m_config.maxQueueSize) {
        int dropped = handleQueueOverflow();
        if (dropped > 0) {
            emit queueOverflow(dropped);
        }
    }
    
    // Add item to queue in priority order
    auto insertPos = std::lower_bound(m_announcementQueue.begin(), m_announcementQueue.end(), item);
    m_announcementQueue.insert(insertPos, item);
}

void AudioFeedbackService::clearAnnouncementQueue(bool preserveCritical)
{
    QMutexLocker locker(&m_queueMutex);
    
    if (preserveCritical) {
        auto it = m_announcementQueue.begin();
        while (it != m_announcementQueue.end()) {
            if (it->priority != Priority::Critical) {
                it = m_announcementQueue.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        m_announcementQueue.clear();
    }
}

void AudioFeedbackService::setConfig(const AnnouncementConfig& config)
{
    m_config = config;
    
    if (m_processingTimer) {
        m_processingTimer->setInterval(m_config.processingIntervalMs);
        
        if (m_config.enabled && m_accessibilityManager && m_accessibilityManager->isAccessibilityEnabled()) {
            if (!m_processingTimer->isActive()) {
                m_processingTimer->start();
            }
        } else {
            m_processingTimer->stop();
        }
    }
    
    saveConfiguration();
}

void AudioFeedbackService::setEnabled(bool enabled)
{
    m_config.enabled = enabled;
    
    if (m_processingTimer) {
        if (enabled && m_accessibilityManager && m_accessibilityManager->isAccessibilityEnabled()) {
            m_processingTimer->start();
        } else {
            m_processingTimer->stop();
        }
    }
    
    saveConfiguration();
}

int AudioFeedbackService::getQueueSize() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_queueMutex));
    return m_announcementQueue.size();
}

void AudioFeedbackService::processNextAnnouncement()
{
    if (m_isProcessing || !m_config.enabled) {
        return;
    }
    
    AnnouncementItem item;
    {
        QMutexLocker locker(&m_queueMutex);
        if (m_announcementQueue.isEmpty()) {
            return;
        }
        
        item = m_announcementQueue.dequeue();
    }
    
    m_isProcessing = true;
    processAnnouncementItem(item);
    m_isProcessing = false;
}

void AudioFeedbackService::processAnnouncementItem(const AnnouncementItem& item)
{
    try {
        sendToAccessibilityFramework(item.message, item.priority);
        
        m_announcementsProcessed++;
        m_lastAnnouncementTime = QDateTime::currentMSecsSinceEpoch();
        
        emit announcementDelivered(item.message, item.priority, item.feedbackType);
        
    } catch (const std::exception& e) {
        qWarning() << "AudioFeedbackService: Error processing announcement:" << e.what();
    }
}

void AudioFeedbackService::sendToAccessibilityFramework(const QString& message, Priority priority)
{
    if (!m_accessibilityManager || !m_accessibilityManager->isAccessibilityEnabled()) {
        return;
    }
    
    // Convert our priority to AccessibilityManager priority
    AccessibilityManager::Priority amPriority = AccessibilityManager::Priority::Normal;
    switch (priority) {
        case Priority::Low:
            amPriority = AccessibilityManager::Priority::Low;
            break;
        case Priority::Normal:
            amPriority = AccessibilityManager::Priority::Normal;
            break;
        case Priority::High:
            amPriority = AccessibilityManager::Priority::High;
            break;
        case Priority::Critical:
            amPriority = AccessibilityManager::Priority::Critical;
            break;
    }
    
    // Send announcement through accessibility manager
    m_accessibilityManager->announceMessage(message, amPriority);
}

QString AudioFeedbackService::formatMessageForVerbosity(const QString& message, FeedbackType feedbackType) const
{
    // Message is already formatted based on verbosity level in the specific announce methods
    // This method can be used for additional formatting if needed
    Q_UNUSED(feedbackType)
    return message;
}

int AudioFeedbackService::handleQueueOverflow()
{
    int removed = 0;
    
    // Remove low priority items first
    auto it = m_announcementQueue.begin();
    while (it != m_announcementQueue.end() && m_announcementQueue.size() >= m_config.maxQueueSize) {
        if (it->priority == Priority::Low) {
            it = m_announcementQueue.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    // If still too full, remove normal priority items
    it = m_announcementQueue.begin();
    while (it != m_announcementQueue.end() && m_announcementQueue.size() >= m_config.maxQueueSize) {
        if (it->priority == Priority::Normal) {
            it = m_announcementQueue.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    m_announcementsDropped += removed;
    return removed;
}

bool AudioFeedbackService::shouldInterruptForPriority(Priority priority) const
{
    return m_config.allowInterruption && priority == Priority::Critical;
}

void AudioFeedbackService::onVerbosityLevelChanged(int level)
{
    m_currentVerbosityLevel = level;
    qDebug() << "AudioFeedbackService: Verbosity level changed to" << level;
}

void AudioFeedbackService::onAccessibilityStateChanged(bool enabled)
{
    if (m_processingTimer) {
        if (enabled && m_config.enabled) {
            m_processingTimer->start();
        } else {
            m_processingTimer->stop();
        }
    }
    
    qDebug() << "AudioFeedbackService: Accessibility state changed to" << enabled;
}

void AudioFeedbackService::loadConfiguration()
{
    QSettings settings;
    settings.beginGroup("AudioFeedback");
    
    m_config.enabled = settings.value("enabled", true).toBool();
    m_config.maxQueueSize = settings.value("maxQueueSize", 50).toInt();
    m_config.processingIntervalMs = settings.value("processingIntervalMs", 100).toInt();
    m_config.criticalInterruptDelayMs = settings.value("criticalInterruptDelayMs", 50).toInt();
    m_config.allowInterruption = settings.value("allowInterruption", true).toBool();
    m_config.maxAnnouncementLengthChars = settings.value("maxAnnouncementLengthChars", 500).toInt();
    
    settings.endGroup();
}

void AudioFeedbackService::saveConfiguration()
{
    QSettings settings;
    settings.beginGroup("AudioFeedback");
    
    settings.setValue("enabled", m_config.enabled);
    settings.setValue("maxQueueSize", m_config.maxQueueSize);
    settings.setValue("processingIntervalMs", m_config.processingIntervalMs);
    settings.setValue("criticalInterruptDelayMs", m_config.criticalInterruptDelayMs);
    settings.setValue("allowInterruption", m_config.allowInterruption);
    settings.setValue("maxAnnouncementLengthChars", m_config.maxAnnouncementLengthChars);
    
    settings.endGroup();
}