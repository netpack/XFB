#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QWidget>
#include <memory>
#include <mutex>

Q_DECLARE_LOGGING_CATEGORY(xfbCore)
Q_DECLARE_LOGGING_CATEGORY(xfbAudio)
Q_DECLARE_LOGGING_CATEGORY(xfbDatabase)
Q_DECLARE_LOGGING_CATEGORY(xfbNetwork)
Q_DECLARE_LOGGING_CATEGORY(xfbUI)

class Logger;

/**
 * @brief Centralized error handling and logging system for XFB
 * 
 * The ErrorHandler provides a unified interface for error reporting, logging,
 * and user notification throughout the XFB application. It supports different
 * severity levels and provides both programmatic and user-friendly error handling.
 * 
 * @example
 * @code
 * ErrorHandler::handleError(ErrorHandler::ErrorSeverity::Error,
 *                          "AudioService",
 *                          "Failed to initialize audio device",
 *                          "Device not found: default");
 * @endcode
 * 
 * @since XFB 2.0
 */
class ErrorHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Error severity levels
     */
    enum class ErrorSeverity {
        Info,       ///< Informational messages
        Warning,    ///< Warning messages that don't prevent operation
        Error,      ///< Error messages that affect functionality
        Critical    ///< Critical errors that may cause application failure
    };

    /**
     * @brief Error categories for better organization
     */
    enum class ErrorCategory {
        General,    ///< General application errors
        Audio,      ///< Audio system errors
        Database,   ///< Database operation errors
        Network,    ///< Network operation errors
        UI,         ///< User interface errors
        File        ///< File system errors
    };

    /**
     * @brief Get the singleton instance of ErrorHandler
     * @return Reference to the ErrorHandler instance
     */
    static ErrorHandler& instance();

    /**
     * @brief Initialize the error handling system
     * @param logDirectory Directory where log files will be stored
     * @param maxLogFiles Maximum number of log files to keep
     * @param maxLogSizeMB Maximum size of each log file in MB
     * @return true if initialization was successful
     */
    bool initialize(const QString& logDirectory = QString(),
                   int maxLogFiles = 10,
                   int maxLogSizeMB = 10);

    /**
     * @brief Handle an error with specified severity and details
     * @param severity Error severity level
     * @param component Component where the error occurred
     * @param message Error message
     * @param details Additional error details (optional)
     * @param category Error category (optional)
     */
    static void handleError(ErrorSeverity severity,
                          const QString& component,
                          const QString& message,
                          const QString& details = QString(),
                          ErrorCategory category = ErrorCategory::General);

    /**
     * @brief Show a user-friendly error dialog
     * @param title Dialog title
     * @param message Error message to display
     * @param details Additional details (optional)
     * @param parent Parent widget for the dialog
     */
    static void showUserError(const QString& title,
                            const QString& message,
                            const QString& details = QString(),
                            QWidget* parent = nullptr);

    /**
     * @brief Show a user-friendly warning dialog
     * @param title Dialog title
     * @param message Warning message to display
     * @param details Additional details (optional)
     * @param parent Parent widget for the dialog
     */
    static void showUserWarning(const QString& title,
                              const QString& message,
                              const QString& details = QString(),
                              QWidget* parent = nullptr);

    /**
     * @brief Show a user-friendly information dialog
     * @param title Dialog title
     * @param message Information message to display
     * @param details Additional details (optional)
     * @param parent Parent widget for the dialog
     */
    static void showUserInfo(const QString& title,
                           const QString& message,
                           const QString& details = QString(),
                           QWidget* parent = nullptr);

    /**
     * @brief Log a message with specified severity
     * @param severity Message severity
     * @param component Component generating the message
     * @param message Message text
     * @param category Message category
     */
    static void logMessage(ErrorSeverity severity,
                         const QString& component,
                         const QString& message,
                         ErrorCategory category = ErrorCategory::General);

    /**
     * @brief Set the minimum severity level for logging
     * @param severity Minimum severity level
     */
    void setLogLevel(ErrorSeverity severity);

    /**
     * @brief Set whether to show user dialogs for errors
     * @param enabled true to show dialogs, false to suppress them
     */
    void setUserDialogsEnabled(bool enabled);

    /**
     * @brief Get the current log directory
     * @return Path to the log directory
     */
    QString getLogDirectory() const;

    /**
     * @brief Convert severity enum to string
     * @param severity Severity level
     * @return String representation of severity
     */
    static QString severityToString(ErrorSeverity severity);

    /**
     * @brief Convert category enum to string
     * @param category Error category
     * @return String representation of category
     */
    static QString categoryToString(ErrorCategory category);

signals:
    /**
     * @brief Emitted when an error occurs
     * @param severity Error severity
     * @param component Component where error occurred
     * @param message Error message
     * @param details Additional details
     * @param category Error category
     */
    void errorOccurred(ErrorSeverity severity,
                      const QString& component,
                      const QString& message,
                      const QString& details,
                      ErrorCategory category);

    /**
     * @brief Emitted when a log message is written
     * @param severity Message severity
     * @param component Component generating the message
     * @param message Message text
     * @param category Message category
     */
    void messageLogged(ErrorSeverity severity,
                      const QString& component,
                      const QString& message,
                      ErrorCategory category);

public:
    ~ErrorHandler() override;

private:
    explicit ErrorHandler(QObject* parent = nullptr);

    void handleErrorInternal(ErrorSeverity severity,
                           const QString& component,
                           const QString& message,
                           const QString& details,
                           ErrorCategory category);

    void showUserDialog(QMessageBox::Icon icon,
                       const QString& title,
                       const QString& message,
                       const QString& details,
                       QWidget* parent);

    std::unique_ptr<Logger> m_logger;
    ErrorSeverity m_logLevel;
    bool m_userDialogsEnabled;
    QString m_logDirectory;
    bool m_initialized;

    // Singleton pattern
    static std::unique_ptr<ErrorHandler> s_instance;
    static std::once_flag s_onceFlag;
};

#endif // ERRORHANDLER_H