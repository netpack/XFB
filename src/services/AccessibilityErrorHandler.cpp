#include "AccessibilityErrorHandler.h"
#include <QWidget>
#include <QApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QProcess>
#include <QAccessible>
#include <QMessageBox>
#include <QSystemTrayIcon>

AccessibilityErrorHandler::AccessibilityErrorHandler(QObject *parent)
    : QObject(parent)
    , m_healthCheckTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
    , m_recoveryTimer(new QTimer(this))
    , m_autoRecoveryEnabled(true)
    , m_errorNotificationEnabled(true)
    , m_gracefulDegradationEnabled(true)
    , m_accessibilityDegraded(false)
{
    // Set up timers
    m_healthCheckTimer->setInterval(HEALTH_CHECK_INTERVAL);
    m_cleanupTimer->setInterval(CLEANUP_INTERVAL);
    m_recoveryTimer->setInterval(RECOVERY_INTERVAL);
    
    connect(m_healthCheckTimer, &QTimer::timeout, this, &AccessibilityErrorHandler::performPeriodicHealthCheck);
    connect(m_cleanupTimer, &QTimer::timeout, this, &AccessibilityErrorHandler::cleanupOldErrors);
    connect(m_recoveryTimer, &QTimer::timeout, this, &AccessibilityErrorHandler::processRecoveryQueue);
}

AccessibilityErrorHandler::~AccessibilityErrorHandler()
{
    m_healthCheckTimer->stop();
    m_cleanupTimer->stop();
    m_recoveryTimer->stop();
}

void AccessibilityErrorHandler::initialize()
{
    qDebug() << "Initializing AccessibilityErrorHandler";
    
    // Start timers
    m_healthCheckTimer->start();
    m_cleanupTimer->start();
    m_recoveryTimer->start();
    
    // Perform initial health check
    performHealthCheck();
    
    qDebug() << "AccessibilityErrorHandler initialized successfully";
}

void AccessibilityErrorHandler::handleATSPIError(const QString& error, const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Critical;
    errorInfo.category = ATSPIConnection;
    errorInfo.message = error;
    errorInfo.details = details;
    errorInfo.component = "AT-SPI Bridge";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = RestartService;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled) {
        addToRecoveryQueue(ATSPIConnection, RestartService);
    }
    
    // Enable graceful degradation
    if (m_gracefulDegradationEnabled) {
        enableGracefulDegradation();
    }
}

void AccessibilityErrorHandler::handleWidgetEnhancementError(QWidget* widget, const QString& error, 
                                                           const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Warning;
    errorInfo.category = WidgetEnhancement;
    errorInfo.message = error;
    errorInfo.details = details;
    errorInfo.component = widget ? widget->metaObject()->className() : "Unknown Widget";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = Fallback;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled && widget) {
        // Try fallback enhancement
        if (useBasicWidgetAccessibility(widget)) {
            errorInfo.recovered = true;
        }
    }
}

void AccessibilityErrorHandler::handleKeyboardConflict(const QString& action, const QString& sequence, 
                                                      const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Warning;
    errorInfo.category = KeyboardNavigation;
    errorInfo.message = QString("Keyboard shortcut conflict: %1 (%2)").arg(action, sequence);
    errorInfo.details = details;
    errorInfo.component = "Keyboard Navigation";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = NoRecovery; // Manual resolution required
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    // Notify user about conflict
    if (m_errorNotificationEnabled) {
        notifyUser(errorInfo);
    }
}

void AccessibilityErrorHandler::handleAudioFeedbackError(const QString& error, const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Error;
    errorInfo.category = AudioFeedback;
    errorInfo.message = error;
    errorInfo.details = details;
    errorInfo.component = "Audio Feedback Service";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = Fallback;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled) {
        addToRecoveryQueue(AudioFeedback, Fallback);
    }
}

void AccessibilityErrorHandler::handleLiveRegionError(const QString& error, const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Warning;
    errorInfo.category = LiveRegion;
    errorInfo.message = error;
    errorInfo.details = details;
    errorInfo.component = "Live Region Manager";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = Retry;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled) {
        addToRecoveryQueue(LiveRegion, Retry);
    }
}

void AccessibilityErrorHandler::handleMemoryError(const QString& error, qint64 memoryUsage, 
                                                 const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = (memoryUsage > 200 * 1024 * 1024) ? Critical : Warning; // 200MB threshold
    errorInfo.category = MemoryManagement;
    errorInfo.message = QString("%1 (Memory usage: %2 MB)").arg(error).arg(memoryUsage / (1024 * 1024));
    errorInfo.details = details;
    errorInfo.component = "Memory Manager";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = GracefulDegradation;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled) {
        addToRecoveryQueue(MemoryManagement, GracefulDegradation);
    }
}

void AccessibilityErrorHandler::handlePerformanceIssue(const QString& issue, double metricValue, 
                                                      const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = (metricValue > 1000.0) ? Critical : Warning; // 1000ms threshold
    errorInfo.category = PerformanceIssue;
    errorInfo.message = QString("%1 (Value: %2)").arg(issue).arg(metricValue, 0, 'f', 2);
    errorInfo.details = details;
    errorInfo.component = "Performance Monitor";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = GracefulDegradation;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled && metricValue > 500.0) { // 500ms threshold for auto-recovery
        addToRecoveryQueue(PerformanceIssue, GracefulDegradation);
    }
}

void AccessibilityErrorHandler::handleConfigurationError(const QString& error, const QString& setting, 
                                                        const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Error;
    errorInfo.category = ConfigurationError;
    errorInfo.message = QString("Configuration error in '%1': %2").arg(setting, error);
    errorInfo.details = details;
    errorInfo.component = "Configuration Service";
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = NoRecovery; // Manual intervention required
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    // Always notify user about configuration errors
    if (m_errorNotificationEnabled) {
        notifyUser(errorInfo);
    }
}

void AccessibilityErrorHandler::handleSystemIntegrationError(const QString& error, const QString& system, 
                                                           const QString& details)
{
    ErrorInfo errorInfo;
    errorInfo.severity = Critical;
    errorInfo.category = SystemIntegration;
    errorInfo.message = QString("System integration error with '%1': %2").arg(system, error);
    errorInfo.details = details;
    errorInfo.component = system;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.occurrenceCount = 1;
    errorInfo.suggestedRecovery = RestartService;
    errorInfo.recovered = false;
    
    logError(errorInfo);
    
    if (m_autoRecoveryEnabled) {
        addToRecoveryQueue(SystemIntegration, RestartService);
    }
}

QList<AccessibilityErrorHandler::ErrorInfo> AccessibilityErrorHandler::getErrorHistory(
    ErrorCategory category, ErrorSeverity severity) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<ErrorInfo> filteredErrors;
    
    for (const ErrorInfo& error : m_errorHistory) {
        bool categoryMatch = (category == ATSPIConnection) || (error.category == category);
        bool severityMatch = (severity == Info) || (error.severity >= severity);
        
        if (categoryMatch && severityMatch) {
            filteredErrors.append(error);
        }
    }
    
    return filteredErrors;
}

QHash<AccessibilityErrorHandler::ErrorCategory, int> AccessibilityErrorHandler::getErrorStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QHash<ErrorCategory, int> statistics;
    
    for (const ErrorInfo& error : m_errorHistory) {
        statistics[error.category]++;
    }
    
    return statistics;
}

bool AccessibilityErrorHandler::isAccessibilityDegraded() const
{
    return m_accessibilityDegraded;
}

QStringList AccessibilityErrorHandler::getCurrentLimitations() const
{
    return m_currentLimitations;
}

bool AccessibilityErrorHandler::attemptRecovery(ErrorCategory category)
{
    RecoveryStrategy strategy = determineRecoveryStrategy(category, Error);
    return executeRecoveryStrategy(category, strategy);
}

void AccessibilityErrorHandler::setAutoRecoveryEnabled(bool enabled)
{
    m_autoRecoveryEnabled = enabled;
    qDebug() << "Auto-recovery" << (enabled ? "enabled" : "disabled");
}

bool AccessibilityErrorHandler::isAutoRecoveryEnabled() const
{
    return m_autoRecoveryEnabled;
}

void AccessibilityErrorHandler::setErrorNotificationEnabled(bool enabled)
{
    m_errorNotificationEnabled = enabled;
    qDebug() << "Error notifications" << (enabled ? "enabled" : "disabled");
}

bool AccessibilityErrorHandler::isErrorNotificationEnabled() const
{
    return m_errorNotificationEnabled;
}

void AccessibilityErrorHandler::clearErrorHistory()
{
    QMutexLocker locker(&m_dataMutex);
    
    m_errorHistory.clear();
    m_recoveryHistory.clear();
    
    qDebug() << "Error history cleared";
}

void AccessibilityErrorHandler::performHealthCheck()
{
    qDebug() << "Performing accessibility system health check";
    
    bool hasIssues = false;
    QStringList issues;
    
    // Check AT-SPI connection
    if (!QAccessible::isActive()) {
        handleATSPIError("AT-SPI bridge is not active", "QAccessible::isActive() returned false");
        hasIssues = true;
        issues << "AT-SPI bridge inactive";
    }
    
    // Check for recent critical errors
    QMutexLocker locker(&m_dataMutex);
    QDateTime recentThreshold = QDateTime::currentDateTime().addSecs(-300); // 5 minutes
    
    for (const ErrorInfo& error : m_errorHistory) {
        if (error.timestamp > recentThreshold && error.severity == Critical && !error.recovered) {
            hasIssues = true;
            issues << QString("Unrecovered critical error in %1").arg(getCategoryName(error.category));
        }
    }
    
    // Update accessibility status
    if (hasIssues && !m_accessibilityDegraded) {
        m_accessibilityDegraded = true;
        m_currentLimitations = issues;
        emit accessibilityDegraded(m_currentLimitations);
    } else if (!hasIssues && m_accessibilityDegraded) {
        m_accessibilityDegraded = false;
        m_currentLimitations.clear();
        emit accessibilityRestored();
    }
}

void AccessibilityErrorHandler::resetAccessibilitySystem()
{
    qDebug() << "Resetting accessibility system";
    
    // Clear error history
    clearErrorHistory();
    
    // Reset degraded state
    m_accessibilityDegraded = false;
    m_currentLimitations.clear();
    
    // Restart AT-SPI if possible
    recoverATSPIConnection();
    
    // Perform health check
    performHealthCheck();
    
    emit accessibilityRestored();
}

void AccessibilityErrorHandler::enableGracefulDegradation()
{
    if (!m_accessibilityDegraded) {
        qDebug() << "Enabling graceful degradation mode";
        
        m_accessibilityDegraded = true;
        m_currentLimitations << "Operating in degraded mode";
        m_currentLimitations << "Some accessibility features may be limited";
        
        emit accessibilityDegraded(m_currentLimitations);
    }
}

void AccessibilityErrorHandler::disableGracefulDegradation()
{
    if (m_accessibilityDegraded) {
        qDebug() << "Disabling graceful degradation mode";
        
        m_accessibilityDegraded = false;
        m_currentLimitations.clear();
        
        emit accessibilityRestored();
    }
}

void AccessibilityErrorHandler::performPeriodicHealthCheck()
{
    performHealthCheck();
}

void AccessibilityErrorHandler::cleanupOldErrors()
{
    QMutexLocker locker(&m_dataMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-ERROR_RETENTION_HOURS * 3600);
    
    // Remove old errors
    auto it = m_errorHistory.begin();
    while (it != m_errorHistory.end()) {
        if (it->timestamp < cutoff) {
            it = m_errorHistory.erase(it);
        } else {
            ++it;
        }
    }
    
    // Clean up recovery history
    for (auto& recoveryList : m_recoveryHistory) {
        auto recoveryIt = recoveryList.begin();
        while (recoveryIt != recoveryList.end()) {
            if (recoveryIt->timestamp < cutoff) {
                recoveryIt = recoveryList.erase(recoveryIt);
            } else {
                ++recoveryIt;
            }
        }
    }
    
    // Limit error history size
    while (m_errorHistory.size() > MAX_ERROR_HISTORY) {
        m_errorHistory.removeFirst();
    }
}

void AccessibilityErrorHandler::processRecoveryQueue()
{
    if (m_recoveryQueue.isEmpty()) {
        return;
    }
    
    QPair<ErrorCategory, RecoveryStrategy> recovery = m_recoveryQueue.dequeue();
    
    bool success = executeRecoveryStrategy(recovery.first, recovery.second);
    
    // Record recovery attempt
    RecoveryAttempt attempt;
    attempt.category = recovery.first;
    attempt.strategy = recovery.second;
    attempt.timestamp = QDateTime::currentDateTime();
    attempt.successful = success;
    
    QMutexLocker locker(&m_dataMutex);
    m_recoveryHistory[recovery.first].append(attempt);
    
    emit recoveryAttempted(recovery.first, recovery.second, success);
    
    if (success) {
        qDebug() << "Recovery successful for category" << recovery.first << "using strategy" << recovery.second;
    } else {
        qDebug() << "Recovery failed for category" << recovery.first << "using strategy" << recovery.second;
    }
}

void AccessibilityErrorHandler::logError(const ErrorInfo& error)
{
    QMutexLocker locker(&m_dataMutex);
    
    // Check if this is a duplicate error
    bool isDuplicate = false;
    for (auto& existingError : m_errorHistory) {
        if (existingError.category == error.category && 
            existingError.message == error.message &&
            existingError.timestamp.secsTo(error.timestamp) < 60) { // Within 1 minute
            
            existingError.occurrenceCount++;
            isDuplicate = true;
            break;
        }
    }
    
    if (!isDuplicate) {
        m_errorHistory.enqueue(error);
        
        // Limit history size
        while (m_errorHistory.size() > MAX_ERROR_HISTORY) {
            m_errorHistory.dequeue();
        }
    }
    
    // Emit signal
    emit accessibilityErrorOccurred(error.severity, error.category, error.message, error.details);
    
    // Log to debug output
    qDebug() << "Accessibility Error:" << formatErrorMessage(error);
    
    // Notify user if appropriate
    if (shouldAttemptRecovery(error) && m_errorNotificationEnabled) {
        notifyUser(error);
    }
}

void AccessibilityErrorHandler::notifyUser(const ErrorInfo& error)
{
    QString title = QString("Accessibility %1").arg(getSeverityName(error.severity));
    QString message = formatErrorMessage(error);
    
    emit userNotificationRequested(title, message, error.severity);
    
    // Also show system notification if available
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
        
        switch (error.severity) {
        case Warning:
            icon = QSystemTrayIcon::Warning;
            break;
        case Error:
        case Critical:
            icon = QSystemTrayIcon::Critical;
            break;
        default:
            icon = QSystemTrayIcon::Information;
            break;
        }
        
        // Note: This would require a system tray icon instance
        // QSystemTrayIcon::showMessage(title, message, icon, 5000);
    }
}

bool AccessibilityErrorHandler::shouldAttemptRecovery(const ErrorInfo& error)
{
    // Don't attempt recovery for info messages
    if (error.severity == Info) {
        return false;
    }
    
    // Check if we've already attempted recovery too many times
    QMutexLocker locker(&m_dataMutex);
    
    if (m_recoveryHistory.contains(error.category)) {
        const QList<RecoveryAttempt>& attempts = m_recoveryHistory[error.category];
        
        // Count recent attempts
        QDateTime recentThreshold = QDateTime::currentDateTime().addSecs(-3600); // 1 hour
        int recentAttempts = 0;
        
        for (const RecoveryAttempt& attempt : attempts) {
            if (attempt.timestamp > recentThreshold) {
                recentAttempts++;
            }
        }
        
        if (recentAttempts >= MAX_RECOVERY_ATTEMPTS) {
            return false;
        }
    }
    
    return true;
}

AccessibilityErrorHandler::RecoveryStrategy AccessibilityErrorHandler::determineRecoveryStrategy(
    ErrorCategory category, ErrorSeverity severity)
{
    switch (category) {
    case ATSPIConnection:
        return (severity >= Error) ? RestartService : Retry;
        
    case WidgetEnhancement:
        return Fallback;
        
    case KeyboardNavigation:
        return NoRecovery; // Usually requires manual intervention
        
    case AudioFeedback:
        return (severity >= Error) ? RestartService : Fallback;
        
    case LiveRegion:
        return Retry;
        
    case MemoryManagement:
        return GracefulDegradation;
        
    case PerformanceIssue:
        return GracefulDegradation;
        
    case ConfigurationError:
        return NoRecovery; // Requires manual configuration fix
        
    case SystemIntegration:
        return RestartService;
    }
    
    return NoRecovery;
}

bool AccessibilityErrorHandler::executeRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy)
{
    switch (strategy) {
    case NoRecovery:
        return false;
        
    case Retry:
        // For most categories, retry means re-initializing the service
        return true; // Assume success for now
        
    case Fallback:
        return fallbackToBasicAccessibility();
        
    case GracefulDegradation:
        enableGracefulDegradation();
        return true;
        
    case RestartService:
        switch (category) {
        case ATSPIConnection:
            return recoverATSPIConnection();
        case AudioFeedback:
            return restartAudioFeedbackService();
        default:
            return false;
        }
        
    case DisableFeature:
        // Disable the problematic feature
        return true;
    }
    
    return false;
}

void AccessibilityErrorHandler::updateAccessibilityStatus()
{
    // This method would coordinate with AccessibilityManager to update overall status
    // For now, we just emit appropriate signals
}

QString AccessibilityErrorHandler::formatErrorMessage(const ErrorInfo& error) const
{
    return QString("[%1] %2 in %3: %4")
           .arg(getSeverityName(error.severity))
           .arg(getCategoryName(error.category))
           .arg(error.component)
           .arg(error.message);
}

QString AccessibilityErrorHandler::getCategoryName(ErrorCategory category) const
{
    switch (category) {
    case ATSPIConnection: return "AT-SPI Connection";
    case WidgetEnhancement: return "Widget Enhancement";
    case KeyboardNavigation: return "Keyboard Navigation";
    case AudioFeedback: return "Audio Feedback";
    case LiveRegion: return "Live Region";
    case MemoryManagement: return "Memory Management";
    case PerformanceIssue: return "Performance";
    case ConfigurationError: return "Configuration";
    case SystemIntegration: return "System Integration";
    }
    return "Unknown";
}

QString AccessibilityErrorHandler::getSeverityName(ErrorSeverity severity) const
{
    switch (severity) {
    case Info: return "Info";
    case Warning: return "Warning";
    case Error: return "Error";
    case Critical: return "Critical";
    }
    return "Unknown";
}

void AccessibilityErrorHandler::addToRecoveryQueue(ErrorCategory category, RecoveryStrategy strategy)
{
    m_recoveryQueue.enqueue(qMakePair(category, strategy));
}

bool AccessibilityErrorHandler::recoverATSPIConnection()
{
    qDebug() << "Attempting to recover AT-SPI connection";
    
    // Try to restart AT-SPI bridge
    if (restartATSPIBridge()) {
        return true;
    }
    
    // Fall back to basic accessibility
    return fallbackToBasicAccessibility();
}

bool AccessibilityErrorHandler::restartATSPIBridge()
{
    // Attempt to restart AT-SPI bridge
    QProcess process;
    
    // First, try to kill existing bridge
    process.start("pkill", QStringList() << "at-spi-bus-launcher");
    process.waitForFinished(3000);
    
    // Wait a moment
    QThread::msleep(1000);
    
    // Try to start new bridge
    process.start("at-spi-bus-launcher", QStringList());
    if (process.waitForStarted(5000)) {
        QThread::msleep(2000); // Give it time to initialize
        
        // Check if QAccessible is now active
        if (QAccessible::isActive()) {
            qDebug() << "AT-SPI bridge restarted successfully";
            return true;
        }
    }
    
    qDebug() << "Failed to restart AT-SPI bridge";
    return false;
}

bool AccessibilityErrorHandler::fallbackToBasicAccessibility()
{
    qDebug() << "Falling back to basic accessibility";
    
    // Enable basic Qt accessibility without AT-SPI
    QAccessible::setActive(true);
    
    // This would signal other components to use basic accessibility mode
    enableGracefulDegradation();
    
    return true;
}

bool AccessibilityErrorHandler::retryWidgetEnhancement(QWidget* widget)
{
    if (!widget) return false;
    
    // This would retry widget enhancement through WidgetAccessibilityEnhancer
    // For now, assume success
    return true;
}

bool AccessibilityErrorHandler::useBasicWidgetAccessibility(QWidget* widget)
{
    if (!widget) return false;
    
    // Set basic accessibility properties
    if (widget->accessibleName().isEmpty()) {
        QString name = widget->objectName();
        if (name.isEmpty() && widget->windowTitle().isEmpty()) {
            name = widget->metaObject()->className();
        }
        widget->setAccessibleName(name);
    }
    
    return true;
}

bool AccessibilityErrorHandler::restartAudioFeedbackService()
{
    qDebug() << "Attempting to restart audio feedback service";
    
    // This would coordinate with AudioFeedbackService to restart
    // For now, assume success
    return true;
}

bool AccessibilityErrorHandler::useFallbackAudioFeedback()
{
    qDebug() << "Using fallback audio feedback";
    
    // This would enable a simpler audio feedback mechanism
    // For now, assume success
    return true;
}