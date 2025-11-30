#include "BackgroundOperationFeedback.h"
#include "AudioFeedbackService.h"
#include "ServiceContainer.h"
#include <QDateTime>
#include <QSettings>
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>

BackgroundOperationFeedback::BackgroundOperationFeedback(QObject* parent)
    : QObject(parent)
    , m_audioFeedbackService(nullptr)
    , m_nextOperationId(1)
    , m_progressAnnouncementTimer(nullptr)
    , m_initialized(false)
    , m_totalOperationsStarted(0)
    , m_totalOperationsCompleted(0)
    , m_totalOperationsFailed(0)
{
    // Initialize default configuration
    m_config = FeedbackConfig();
}

BackgroundOperationFeedback::~BackgroundOperationFeedback()
{
    if (m_initialized) {
        shutdown();
    }
}

bool BackgroundOperationFeedback::initialize()
{
    qDebug() << "BackgroundOperationFeedback: Initializing background operation feedback service";
    
    try {
        // Get audio feedback service
        m_audioFeedbackService = ServiceContainer::instance()->resolve<AudioFeedbackService>();
        if (!m_audioFeedbackService) {
            qWarning() << "BackgroundOperationFeedback: AudioFeedbackService not available";
            return false;
        }
        
        // Load configuration
        loadConfiguration();
        
        // Initialize feedback system
        initializeFeedbackSystem();
        
        m_initialized = true;
        qDebug() << "BackgroundOperationFeedback: Initialization completed successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "BackgroundOperationFeedback: Exception during initialization:" << e.what();
        return false;
    }
}

void BackgroundOperationFeedback::shutdown()
{
    qDebug() << "BackgroundOperationFeedback: Shutting down background operation feedback service";
    
    if (m_initialized) {
        shutdownFeedbackSystem();
        saveConfiguration();
        m_initialized = false;
    }
}

void BackgroundOperationFeedback::initializeFeedbackSystem()
{
    // Create progress announcement timer
    m_progressAnnouncementTimer = new QTimer(this);
    m_progressAnnouncementTimer->setInterval(m_config.progressUpdateIntervalMs);
    m_progressAnnouncementTimer->setSingleShot(false);
    
    connect(m_progressAnnouncementTimer, &QTimer::timeout,
            this, &BackgroundOperationFeedback::onProgressAnnouncementTimer);
    
    // Start timer if enabled
    if (m_config.enabled) {
        m_progressAnnouncementTimer->start();
    }
    
    qDebug() << "BackgroundOperationFeedback: Feedback system initialized";
}

void BackgroundOperationFeedback::shutdownFeedbackSystem()
{
    if (m_progressAnnouncementTimer) {
        m_progressAnnouncementTimer->stop();
        m_progressAnnouncementTimer->deleteLater();
        m_progressAnnouncementTimer = nullptr;
    }
    
    // Clear all active operations
    QMutexLocker locker(&m_operationsMutex);
    m_activeOperations.clear();
    
    qDebug() << "BackgroundOperationFeedback: Feedback system shutdown completed";
}

int BackgroundOperationFeedback::startOperation(const QString& operationName, 
                                               const QString& description,
                                               OperationType type,
                                               Priority priority)
{
    if (!m_config.enabled || !m_initialized) {
        return -1;
    }
    
    QMutexLocker locker(&m_operationsMutex);
    
    // Check if we have too many concurrent operations
    if (m_activeOperations.size() >= m_config.maxConcurrentOperations) {
        cleanupCompletedOperations();
        
        if (m_activeOperations.size() >= m_config.maxConcurrentOperations) {
            qWarning() << "BackgroundOperationFeedback: Too many concurrent operations";
            return -1;
        }
    }
    
    // Create new operation
    int operationId = generateOperationId();
    OperationInfo operation;
    operation.id = operationId;
    operation.name = operationName;
    operation.description = description;
    operation.type = type;
    operation.priority = priority;
    operation.currentProgress = 0;
    operation.startTime = QDateTime::currentMSecsSinceEpoch();
    operation.lastProgressTime = operation.startTime;
    operation.lastAnnouncementTime = operation.startTime;
    operation.isActive = true;
    operation.hasError = false;
    
    m_activeOperations[operationId] = operation;
    m_totalOperationsStarted++;
    
    // Announce operation start if configured
    if (m_config.announceStart) {
        QString message;
        if (description.isEmpty()) {
            message = QString("Started %1").arg(operationName);
        } else {
            message = QString("Started %1: %2").arg(operationName, description);
        }
        
        m_audioFeedbackService->announceDatabaseOperation(
            operationName, message, false);
    }
    
    emit operationStarted(operationId, operationName, type);
    
    qDebug() << "BackgroundOperationFeedback: Started operation" << operationId << ":" << operationName;
    return operationId;
}

void BackgroundOperationFeedback::updateProgress(int operationId, 
                                                int percentage, 
                                                const QString& statusMessage,
                                                const QString& estimatedTimeRemaining)
{
    if (!m_config.enabled || !m_initialized) {
        return;
    }
    
    QMutexLocker locker(&m_operationsMutex);
    
    auto it = m_activeOperations.find(operationId);
    if (it == m_activeOperations.end() || !it->isActive) {
        return;
    }
    
    OperationInfo& operation = it.value();
    
    // Validate percentage
    percentage = qBound(0, percentage, 100);
    
    // Check if we should announce this progress update
    bool shouldAnnounce = shouldAnnounceProgress(operation, percentage);
    
    // Update operation info
    operation.currentProgress = percentage;
    operation.lastProgressTime = QDateTime::currentMSecsSinceEpoch();
    
    if (!statusMessage.isEmpty()) {
        operation.lastStatusMessage = statusMessage;
    }
    
    if (!estimatedTimeRemaining.isEmpty()) {
        operation.estimatedTimeRemaining = estimatedTimeRemaining;
    }
    
    // Announce progress if appropriate
    if (shouldAnnounce && m_config.announceProgress) {
        QString message;
        if (!statusMessage.isEmpty()) {
            message = statusMessage;
        } else {
            message = QString("%1% complete").arg(percentage);
        }
        
        if (!estimatedTimeRemaining.isEmpty()) {
            message += QString(". %1 remaining").arg(estimatedTimeRemaining);
        }
        
        m_audioFeedbackService->announceProgressUpdate(
            operation.name, percentage, estimatedTimeRemaining);
        
        operation.lastAnnouncementTime = operation.lastProgressTime;
    }
    
    emit operationProgressUpdated(operationId, percentage, statusMessage);
}

void BackgroundOperationFeedback::completeOperation(int operationId, 
                                                   bool success, 
                                                   const QString& completionMessage)
{
    if (!m_initialized) {
        return;
    }
    
    QMutexLocker locker(&m_operationsMutex);
    
    auto it = m_activeOperations.find(operationId);
    if (it == m_activeOperations.end()) {
        return;
    }
    
    OperationInfo& operation = it.value();
    operation.isActive = false;
    operation.currentProgress = success ? 100 : operation.currentProgress;
    
    if (success) {
        m_totalOperationsCompleted++;
    } else {
        m_totalOperationsFailed++;
    }
    
    // Announce completion if configured
    if (m_config.announceCompletion && m_config.enabled) {
        QString message = completionMessage;
        if (message.isEmpty()) {
            message = success ? "completed successfully" : "failed";
        }
        
        m_audioFeedbackService->announceDatabaseOperation(
            operation.name, message, true);
    }
    
    emit operationCompleted(operationId, success, completionMessage);
    
    qDebug() << "BackgroundOperationFeedback: Completed operation" << operationId 
             << ":" << operation.name << "Success:" << success;
    
    // Remove completed operation after a delay to allow for cleanup
    QTimer::singleShot(5000, this, [this, operationId]() {
        QMutexLocker locker(&m_operationsMutex);
        m_activeOperations.remove(operationId);
    });
}

void BackgroundOperationFeedback::cancelOperation(int operationId, const QString& reason)
{
    if (!m_initialized) {
        return;
    }
    
    QMutexLocker locker(&m_operationsMutex);
    
    auto it = m_activeOperations.find(operationId);
    if (it == m_activeOperations.end() || !it->isActive) {
        return;
    }
    
    OperationInfo& operation = it.value();
    operation.isActive = false;
    
    // Announce cancellation
    if (m_config.enabled) {
        QString message = QString("Operation cancelled");
        if (!reason.isEmpty()) {
            message += QString(": %1").arg(reason);
        }
        
        m_audioFeedbackService->announceError(message, operation.name);
    }
    
    qDebug() << "BackgroundOperationFeedback: Cancelled operation" << operationId 
             << ":" << operation.name << "Reason:" << reason;
    
    // Remove cancelled operation
    m_activeOperations.remove(operationId);
}

void BackgroundOperationFeedback::reportCriticalAlert(int operationId, 
                                                     const QString& alertMessage,
                                                     bool requiresImmediateAttention)
{
    if (!m_config.announceCriticalAlerts || !m_config.enabled || !m_initialized) {
        return;
    }
    
    QString context;
    if (operationId >= 0) {
        QMutexLocker locker(&m_operationsMutex);
        auto it = m_activeOperations.find(operationId);
        if (it != m_activeOperations.end()) {
            context = it->name;
        }
    }
    
    m_audioFeedbackService->announceCriticalAlert(alertMessage, requiresImmediateAttention);
    
    emit criticalAlertReported(operationId, alertMessage);
    
    qDebug() << "BackgroundOperationFeedback: Critical alert for operation" << operationId 
             << ":" << alertMessage;
}

void BackgroundOperationFeedback::reportError(int operationId, 
                                             const QString& errorMessage,
                                             bool isFatal)
{
    if (!m_config.enabled || !m_initialized) {
        return;
    }
    
    QString context;
    if (operationId >= 0) {
        QMutexLocker locker(&m_operationsMutex);
        auto it = m_activeOperations.find(operationId);
        if (it != m_activeOperations.end()) {
            OperationInfo& operation = it.value();
            operation.hasError = true;
            operation.errorMessage = errorMessage;
            context = operation.name;
            
            if (isFatal) {
                operation.isActive = false;
                m_totalOperationsFailed++;
            }
        }
    }
    
    m_audioFeedbackService->announceError(errorMessage, context);
    
    qDebug() << "BackgroundOperationFeedback: Error for operation" << operationId 
             << ":" << errorMessage << "Fatal:" << isFatal;
}

void BackgroundOperationFeedback::setConfig(const FeedbackConfig& config)
{
    m_config = config;
    
    if (m_progressAnnouncementTimer) {
        m_progressAnnouncementTimer->setInterval(m_config.progressUpdateIntervalMs);
        
        if (m_config.enabled) {
            if (!m_progressAnnouncementTimer->isActive()) {
                m_progressAnnouncementTimer->start();
            }
        } else {
            m_progressAnnouncementTimer->stop();
        }
    }
    
    saveConfiguration();
}

void BackgroundOperationFeedback::setEnabled(bool enabled)
{
    m_config.enabled = enabled;
    
    if (m_progressAnnouncementTimer) {
        if (enabled) {
            m_progressAnnouncementTimer->start();
        } else {
            m_progressAnnouncementTimer->stop();
        }
    }
    
    saveConfiguration();
}

int BackgroundOperationFeedback::getActiveOperationCount() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_operationsMutex));
    
    int count = 0;
    for (const auto& operation : m_activeOperations) {
        if (operation.isActive) {
            count++;
        }
    }
    return count;
}

QString BackgroundOperationFeedback::getOperationInfo(int operationId) const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_operationsMutex));
    
    auto it = m_activeOperations.find(operationId);
    if (it == m_activeOperations.end()) {
        return QString();
    }
    
    const OperationInfo& operation = it.value();
    return QString("%1: %2% - %3")
        .arg(operation.name)
        .arg(operation.currentProgress)
        .arg(operation.lastStatusMessage);
}

QList<int> BackgroundOperationFeedback::getActiveOperationIds() const
{
    QMutexLocker locker(const_cast<QMutex*>(&m_operationsMutex));
    
    QList<int> activeIds;
    for (auto it = m_activeOperations.begin(); it != m_activeOperations.end(); ++it) {
        if (it->isActive) {
            activeIds.append(it.key());
        }
    }
    return activeIds;
}

void BackgroundOperationFeedback::onDatabaseOperationStarted(const QString& operation, const QString& description)
{
    startOperation(operation, description, OperationType::DatabaseScan, Priority::Normal);
}

void BackgroundOperationFeedback::onDatabaseOperationProgress(const QString& operation, int percentage, const QString& status)
{
    // Find operation by name (this is a simplified approach)
    QMutexLocker locker(&m_operationsMutex);
    
    for (auto& op : m_activeOperations) {
        if (op.name == operation && op.isActive) {
            locker.unlock();
            updateProgress(op.id, percentage, status);
            return;
        }
    }
}

void BackgroundOperationFeedback::onDatabaseOperationCompleted(const QString& operation, bool success, const QString& message)
{
    // Find operation by name
    QMutexLocker locker(&m_operationsMutex);
    
    for (auto& op : m_activeOperations) {
        if (op.name == operation && op.isActive) {
            int operationId = op.id;
            locker.unlock();
            completeOperation(operationId, success, message);
            return;
        }
    }
}

void BackgroundOperationFeedback::onFileOperationProgress(const QString& operation, int fileCount, int totalFiles, const QString& currentFile)
{
    if (totalFiles <= 0) {
        return;
    }
    
    int percentage = (fileCount * 100) / totalFiles;
    QString status = QString("Processing %1 of %2 files").arg(fileCount).arg(totalFiles);
    
    if (!currentFile.isEmpty()) {
        status += QString(": %1").arg(currentFile);
    }
    
    // Find or create operation
    QMutexLocker locker(&m_operationsMutex);
    
    for (auto& op : m_activeOperations) {
        if (op.name == operation && op.isActive) {
            int operationId = op.id;
            locker.unlock();
            updateProgress(operationId, percentage, status);
            return;
        }
    }
    
    // Create new operation if not found
    locker.unlock();
    int operationId = startOperation(operation, QString("Processing %1 files").arg(totalFiles), 
                                   OperationType::FileImport, Priority::Normal);
    if (operationId >= 0) {
        updateProgress(operationId, percentage, status);
    }
}

void BackgroundOperationFeedback::onProgressAnnouncementTimer()
{
    if (!m_config.enabled || !m_config.announceProgress) {
        return;
    }
    
    QMutexLocker locker(&m_operationsMutex);
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    for (auto& operation : m_activeOperations) {
        if (!operation.isActive) {
            continue;
        }
        
        // Check if enough time has passed since last announcement
        qint64 timeSinceLastAnnouncement = currentTime - operation.lastAnnouncementTime;
        if (timeSinceLastAnnouncement >= m_config.progressUpdateIntervalMs) {
            
            // Announce current progress
            QString message = QString("%1: %2%").arg(operation.name).arg(operation.currentProgress);
            if (!operation.lastStatusMessage.isEmpty()) {
                message = operation.lastStatusMessage;
            }
            
            locker.unlock();
            m_audioFeedbackService->announceProgressUpdate(
                operation.name, operation.currentProgress, operation.estimatedTimeRemaining);
            locker.relock();
            
            operation.lastAnnouncementTime = currentTime;
        }
    }
}

int BackgroundOperationFeedback::generateOperationId()
{
    return m_nextOperationId++;
}

bool BackgroundOperationFeedback::shouldAnnounceProgress(const OperationInfo& operation, int newProgress) const
{
    // Always announce if this is the first progress update
    if (operation.currentProgress == 0 && newProgress > 0) {
        return true;
    }
    
    // Announce if progress change is significant
    int progressChange = newProgress - operation.currentProgress;
    if (progressChange >= m_config.minProgressChangePercent) {
        return true;
    }
    
    // Announce at specific milestones
    if (newProgress == 25 || newProgress == 50 || newProgress == 75 || newProgress == 100) {
        return true;
    }
    
    return false;
}

QString BackgroundOperationFeedback::formatOperationType(OperationType type) const
{
    switch (type) {
        case OperationType::DatabaseScan:
            return "Database Scan";
        case OperationType::DatabaseUpdate:
            return "Database Update";
        case OperationType::DatabaseBackup:
            return "Database Backup";
        case OperationType::DatabaseRestore:
            return "Database Restore";
        case OperationType::FileImport:
            return "File Import";
        case OperationType::FileExport:
            return "File Export";
        case OperationType::AudioConversion:
            return "Audio Conversion";
        case OperationType::AudioProcessing:
            return "Audio Processing";
        case OperationType::NetworkOperation:
            return "Network Operation";
        case OperationType::SystemMaintenance:
            return "System Maintenance";
        case OperationType::UserOperation:
        default:
            return "Operation";
    }
}

int BackgroundOperationFeedback::convertPriorityToAudioFeedback(Priority priority) const
{
    switch (priority) {
        case Priority::Low:
            return static_cast<int>(AudioFeedbackService::Priority::Low);
        case Priority::Normal:
            return static_cast<int>(AudioFeedbackService::Priority::Normal);
        case Priority::High:
            return static_cast<int>(AudioFeedbackService::Priority::High);
        case Priority::Critical:
            return static_cast<int>(AudioFeedbackService::Priority::Critical);
        default:
            return static_cast<int>(AudioFeedbackService::Priority::Normal);
    }
}

void BackgroundOperationFeedback::cleanupCompletedOperations()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    auto it = m_activeOperations.begin();
    while (it != m_activeOperations.end()) {
        if (!it->isActive && (currentTime - it->lastProgressTime) > OPERATION_CLEANUP_INTERVAL_MS) {
            it = m_activeOperations.erase(it);
        } else {
            ++it;
        }
    }
}

void BackgroundOperationFeedback::loadConfiguration()
{
    QSettings settings;
    settings.beginGroup("BackgroundOperationFeedback");
    
    m_config.enabled = settings.value("enabled", true).toBool();
    m_config.progressUpdateIntervalMs = settings.value("progressUpdateIntervalMs", DEFAULT_PROGRESS_INTERVAL_MS).toInt();
    m_config.minProgressChangePercent = settings.value("minProgressChangePercent", MIN_PROGRESS_CHANGE_PERCENT).toInt();
    m_config.announceStart = settings.value("announceStart", true).toBool();
    m_config.announceProgress = settings.value("announceProgress", true).toBool();
    m_config.announceCompletion = settings.value("announceCompletion", true).toBool();
    m_config.announceCriticalAlerts = settings.value("announceCriticalAlerts", true).toBool();
    m_config.maxConcurrentOperations = settings.value("maxConcurrentOperations", MAX_CONCURRENT_OPERATIONS).toInt();
    
    settings.endGroup();
}

void BackgroundOperationFeedback::saveConfiguration()
{
    QSettings settings;
    settings.beginGroup("BackgroundOperationFeedback");
    
    settings.setValue("enabled", m_config.enabled);
    settings.setValue("progressUpdateIntervalMs", m_config.progressUpdateIntervalMs);
    settings.setValue("minProgressChangePercent", m_config.minProgressChangePercent);
    settings.setValue("announceStart", m_config.announceStart);
    settings.setValue("announceProgress", m_config.announceProgress);
    settings.setValue("announceCompletion", m_config.announceCompletion);
    settings.setValue("announceCriticalAlerts", m_config.announceCriticalAlerts);
    settings.setValue("maxConcurrentOperations", m_config.maxConcurrentOperations);
    
    settings.endGroup();
}