#ifndef ACCESSIBILITYERRORHANDLER_H
#define ACCESSIBILITYERRORHANDLER_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QHash>
#include <QDateTime>
#include <QThread>

class QWidget;

/**
 * @brief Error handling and recovery system for accessibility operations
 * 
 * Provides graceful degradation, error recovery mechanisms, and user
 * notification for accessibility-related failures.
 */
class AccessibilityErrorHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Error severity levels
     */
    enum ErrorSeverity {
        Info,       ///< Informational message
        Warning,    ///< Warning that doesn't affect functionality
        Error,      ///< Error that affects some functionality
        Critical    ///< Critical error that severely impacts accessibility
    };

    /**
     * @brief Error categories
     */
    enum ErrorCategory {
        ATSPIConnection,        ///< AT-SPI bridge connection errors
        WidgetEnhancement,      ///< Widget accessibility enhancement errors
        KeyboardNavigation,     ///< Keyboard navigation errors
        AudioFeedback,          ///< Audio feedback system errors
        LiveRegion,            ///< Live region update errors
        MemoryManagement,      ///< Memory management errors
        PerformanceIssue,      ///< Performance-related issues
        ConfigurationError,    ///< Configuration or settings errors
        SystemIntegration      ///< System integration errors
    };

    /**
     * @brief Recovery strategies
     */
    enum RecoveryStrategy {
        NoRecovery,            ///< No automatic recovery
        Retry,                 ///< Retry the failed operation
        Fallback,              ///< Use fallback mechanism
        GracefulDegradation,   ///< Reduce functionality gracefully
        RestartService,        ///< Restart the affected service
        DisableFeature         ///< Disable the problematic feature
    };

    /**
     * @brief Error information structure
     */
    struct ErrorInfo {
        ErrorSeverity severity;
        ErrorCategory category;
        QString message;
        QString details;
        QString component;
        QDateTime timestamp;
        int occurrenceCount;
        RecoveryStrategy suggestedRecovery;
        bool recovered;
    };

    explicit AccessibilityErrorHandler(QObject *parent = nullptr);
    ~AccessibilityErrorHandler();

    /**
     * @brief Initialize error handling system
     */
    void initialize();

    /**
     * @brief Handle AT-SPI connection error
     * @param error Error message
     * @param details Additional error details
     */
    void handleATSPIError(const QString& error, const QString& details = QString());

    /**
     * @brief Handle widget enhancement error
     * @param widget Widget that failed to enhance
     * @param error Error message
     * @param details Additional error details
     */
    void handleWidgetEnhancementError(QWidget* widget, const QString& error, 
                                    const QString& details = QString());

    /**
     * @brief Handle keyboard navigation conflict
     * @param action Action that caused the conflict
     * @param sequence Key sequence that conflicts
     * @param details Additional details
     */
    void handleKeyboardConflict(const QString& action, const QString& sequence, 
                              const QString& details = QString());

    /**
     * @brief Handle audio feedback error
     * @param error Error message
     * @param details Additional error details
     */
    void handleAudioFeedbackError(const QString& error, const QString& details = QString());

    /**
     * @brief Handle live region error
     * @param error Error message
     * @param details Additional error details
     */
    void handleLiveRegionError(const QString& error, const QString& details = QString());

    /**
     * @brief Handle memory management error
     * @param error Error message
     * @param memoryUsage Current memory usage
     * @param details Additional error details
     */
    void handleMemoryError(const QString& error, qint64 memoryUsage, 
                          const QString& details = QString());

    /**
     * @brief Handle performance issue
     * @param issue Performance issue description
     * @param metricValue Metric value that triggered the issue
     * @param details Additional details
     */
    void handlePerformanceIssue(const QString& issue, double metricValue, 
                               const QString& details = QString());

    /**
     * @brief Handle configuration error
     * @param error Configuration error message
     * @param setting Setting that caused the error
     * @param details Additional details
     */
    void handleConfigurationError(const QString& error, const QString& setting, 
                                 const QString& details = QString());

    /**
     * @brief Handle system integration error
     * @param error Error message
     * @param system System component that failed
     * @param details Additional details
     */
    void handleSystemIntegrationError(const QString& error, const QString& system, 
                                     const QString& details = QString());

    /**
     * @brief Get error history
     * @param category Optional category filter
     * @param severity Optional severity filter
     * @return List of error information
     */
    QList<ErrorInfo> getErrorHistory(ErrorCategory category = ATSPIConnection, 
                                   ErrorSeverity severity = Info) const;

    /**
     * @brief Get error statistics
     * @return Hash of error categories to occurrence counts
     */
    QHash<ErrorCategory, int> getErrorStatistics() const;

    /**
     * @brief Check if accessibility is in degraded mode
     * @return True if accessibility is degraded
     */
    bool isAccessibilityDegraded() const;

    /**
     * @brief Get current accessibility limitations
     * @return List of current limitations
     */
    QStringList getCurrentLimitations() const;

    /**
     * @brief Attempt recovery for a specific error category
     * @param category Error category to recover
     * @return True if recovery was attempted
     */
    bool attemptRecovery(ErrorCategory category);

    /**
     * @brief Enable or disable automatic recovery
     * @param enabled Whether to enable automatic recovery
     */
    void setAutoRecoveryEnabled(bool enabled);

    /**
     * @brief Check if automatic recovery is enabled
     * @return True if auto-recovery is enabled
     */
    bool isAutoRecoveryEnabled() const;

    /**
     * @brief Set error notification enabled
     * @param enabled Whether to enable user notifications
     */
    void setErrorNotificationEnabled(bool enabled);

    /**
     * @brief Check if error notifications are enabled
     * @return True if notifications are enabled
     */
    bool isErrorNotificationEnabled() const;

    /**
     * @brief Clear error history
     */
    void clearErrorHistory();

public slots:
    /**
     * @brief Perform system health check
     */
    void performHealthCheck();

    /**
     * @brief Reset accessibility system
     */
    void resetAccessibilitySystem();

    /**
     * @brief Enable graceful degradation mode
     */
    void enableGracefulDegradation();

    /**
     * @brief Disable graceful degradation mode
     */
    void disableGracefulDegradation();

signals:
    /**
     * @brief Emitted when an accessibility error occurs
     * @param severity Error severity
     * @param category Error category
     * @param message Error message
     * @param details Additional details
     */
    void accessibilityErrorOccurred(ErrorSeverity severity, ErrorCategory category, 
                                   const QString& message, const QString& details);

    /**
     * @brief Emitted when error recovery is attempted
     * @param category Error category being recovered
     * @param strategy Recovery strategy used
     * @param success Whether recovery was successful
     */
    void recoveryAttempted(ErrorCategory category, RecoveryStrategy strategy, bool success);

    /**
     * @brief Emitted when accessibility enters degraded mode
     * @param limitations List of current limitations
     */
    void accessibilityDegraded(const QStringList& limitations);

    /**
     * @brief Emitted when accessibility is restored from degraded mode
     */
    void accessibilityRestored();

    /**
     * @brief Emitted when user notification should be shown
     * @param title Notification title
     * @param message Notification message
     * @param severity Error severity
     */
    void userNotificationRequested(const QString& title, const QString& message, 
                                  ErrorSeverity severity);

private slots:
    void performPeriodicHealthCheck();
    void cleanupOldErrors();
    void processRecoveryQueue();

private:
    /**
     * @brief Recovery attempt information
     */
    struct RecoveryAttempt {
        ErrorCategory category;
        RecoveryStrategy strategy;
        QDateTime timestamp;
        bool successful;
    };

    // Helper methods
    void logError(const ErrorInfo& error);
    void notifyUser(const ErrorInfo& error);
    bool shouldAttemptRecovery(const ErrorInfo& error);
    RecoveryStrategy determineRecoveryStrategy(ErrorCategory category, ErrorSeverity severity);
    bool executeRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy);
    void updateAccessibilityStatus();
    QString formatErrorMessage(const ErrorInfo& error) const;
    QString getCategoryName(ErrorCategory category) const;
    QString getSeverityName(ErrorSeverity severity) const;
    void addToRecoveryQueue(ErrorCategory category, RecoveryStrategy strategy);

    // AT-SPI specific recovery methods
    bool recoverATSPIConnection();
    bool restartATSPIBridge();
    bool fallbackToBasicAccessibility();

    // Widget enhancement recovery methods
    bool retryWidgetEnhancement(QWidget* widget);
    bool useBasicWidgetAccessibility(QWidget* widget);

    // Audio feedback recovery methods
    bool restartAudioFeedbackService();
    bool useFallbackAudioFeedback();

    // Member variables
    QQueue<ErrorInfo> m_errorHistory;
    QHash<ErrorCategory, QList<RecoveryAttempt>> m_recoveryHistory;
    QQueue<QPair<ErrorCategory, RecoveryStrategy>> m_recoveryQueue;
    QTimer* m_healthCheckTimer;
    QTimer* m_cleanupTimer;
    QTimer* m_recoveryTimer;
    mutable QMutex m_dataMutex;
    
    bool m_autoRecoveryEnabled;
    bool m_errorNotificationEnabled;
    bool m_gracefulDegradationEnabled;
    bool m_accessibilityDegraded;
    QStringList m_currentLimitations;
    
    // Configuration
    static const int MAX_ERROR_HISTORY = 1000;
    static const int HEALTH_CHECK_INTERVAL = 30000; // 30 seconds
    static const int CLEANUP_INTERVAL = 300000; // 5 minutes
    static const int RECOVERY_INTERVAL = 5000; // 5 seconds
    static const int ERROR_RETENTION_HOURS = 24;
    static const int MAX_RECOVERY_ATTEMPTS = 3;
};

Q_DECLARE_METATYPE(AccessibilityErrorHandler::ErrorSeverity)
Q_DECLARE_METATYPE(AccessibilityErrorHandler::ErrorCategory)
Q_DECLARE_METATYPE(AccessibilityErrorHandler::RecoveryStrategy)
Q_DECLARE_METATYPE(AccessibilityErrorHandler::ErrorInfo)

#endif // ACCESSIBILITYERRORHANDLER_H