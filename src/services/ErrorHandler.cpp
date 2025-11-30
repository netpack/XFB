#include "ErrorHandler.h"
#include "Logger.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <mutex>

// Logging categories
Q_LOGGING_CATEGORY(xfbCore, "xfb.core")
Q_LOGGING_CATEGORY(xfbAudio, "xfb.audio")
Q_LOGGING_CATEGORY(xfbDatabase, "xfb.database")
Q_LOGGING_CATEGORY(xfbNetwork, "xfb.network")
Q_LOGGING_CATEGORY(xfbUI, "xfb.ui")

// Static members
std::unique_ptr<ErrorHandler> ErrorHandler::s_instance = nullptr;
std::once_flag ErrorHandler::s_onceFlag;

ErrorHandler::ErrorHandler(QObject* parent)
    : QObject(parent)
    , m_logger(std::make_unique<Logger>(this))
    , m_logLevel(ErrorSeverity::Info)
    , m_userDialogsEnabled(true)
    , m_initialized(false)
{
}

ErrorHandler::~ErrorHandler()
{
    if (m_logger) {
        m_logger->flush();
    }
}

ErrorHandler& ErrorHandler::instance()
{
    std::call_once(s_onceFlag, []() {
        s_instance = std::unique_ptr<ErrorHandler>(new ErrorHandler());
    });
    return *s_instance;
}

bool ErrorHandler::initialize(const QString& logDirectory, int maxLogFiles, int maxLogSizeMB)
{
    if (m_initialized) {
        return true;
    }

    // Determine log directory
    QString logDir = logDirectory;
    if (logDir.isEmpty()) {
        logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (logDir.isEmpty()) {
            logDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        }
        logDir += "/logs";
    }

    // Create log directory if it doesn't exist
    QDir dir;
    if (!dir.mkpath(logDir)) {
        qWarning() << "Failed to create log directory:" << logDir;
        return false;
    }

    m_logDirectory = logDir;

    // Initialize logger
    if (!m_logger->initialize(logDir, maxLogFiles, maxLogSizeMB, Logger::LogLevel::Info)) {
        qWarning() << "Failed to initialize logger";
        return false;
    }

    m_initialized = true;

    // Log initialization
    logMessage(ErrorSeverity::Info, "ErrorHandler", "Error handling system initialized", ErrorCategory::General);

    return true;
}

void ErrorHandler::handleError(ErrorSeverity severity,
                             const QString& component,
                             const QString& message,
                             const QString& details,
                             ErrorCategory category)
{
    ErrorHandler& handler = instance();
    
    // Ensure initialization
    if (!handler.m_initialized) {
        handler.initialize();
    }
    
    handler.handleErrorInternal(severity, component, message, details, category);
}

void ErrorHandler::handleErrorInternal(ErrorSeverity severity,
                                     const QString& component,
                                     const QString& message,
                                     const QString& details,
                                     ErrorCategory category)
{
    // Log the error
    QString fullMessage = message;
    if (!details.isEmpty()) {
        fullMessage += QString(" - Details: %1").arg(details);
    }

    if (m_logger && m_logger->isEnabled()) {
        m_logger->writeLog(static_cast<Logger::LogLevel>(severity),
                          component,
                          fullMessage,
                          categoryToString(category));
    }

    // Emit signal for programmatic handling
    emit errorOccurred(severity, component, message, details, category);

    // Show user dialog for errors and critical issues
    if (m_userDialogsEnabled && (severity == ErrorSeverity::Error || severity == ErrorSeverity::Critical)) {
        QMessageBox::Icon icon = (severity == ErrorSeverity::Critical) ? 
                                QMessageBox::Critical : QMessageBox::Warning;
        
        QString title = QString("%1 - %2").arg(severityToString(severity), component);
        showUserDialog(icon, title, message, details, nullptr);
    }

    // For critical errors, also output to Qt's logging system
    if (severity == ErrorSeverity::Critical) {
        qCritical() << "CRITICAL ERROR in" << component << ":" << message;
        if (!details.isEmpty()) {
            qCritical() << "Details:" << details;
        }
    }
}

void ErrorHandler::showUserError(const QString& title,
                                const QString& message,
                                const QString& details,
                                QWidget* parent)
{
    ErrorHandler& handler = instance();
    handler.showUserDialog(QMessageBox::Critical, title, message, details, parent);
}

void ErrorHandler::showUserWarning(const QString& title,
                                  const QString& message,
                                  const QString& details,
                                  QWidget* parent)
{
    ErrorHandler& handler = instance();
    handler.showUserDialog(QMessageBox::Warning, title, message, details, parent);
}

void ErrorHandler::showUserInfo(const QString& title,
                               const QString& message,
                               const QString& details,
                               QWidget* parent)
{
    ErrorHandler& handler = instance();
    handler.showUserDialog(QMessageBox::Information, title, message, details, parent);
}

void ErrorHandler::showUserDialog(QMessageBox::Icon icon,
                                 const QString& title,
                                 const QString& message,
                                 const QString& details,
                                 QWidget* parent)
{
    // Ensure we're on the main thread for UI operations
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, [=]() {
            showUserDialog(icon, title, message, details, parent);
        }, Qt::QueuedConnection);
        return;
    }

    QMessageBox msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    
    if (!details.isEmpty()) {
        msgBox.setDetailedText(details);
    }
    
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void ErrorHandler::logMessage(ErrorSeverity severity,
                            const QString& component,
                            const QString& message,
                            ErrorCategory category)
{
    ErrorHandler& handler = instance();
    
    // Ensure initialization
    if (!handler.m_initialized) {
        handler.initialize();
    }

    if (handler.m_logger && handler.m_logger->isEnabled()) {
        handler.m_logger->writeLog(static_cast<Logger::LogLevel>(severity),
                                  component,
                                  message,
                                  categoryToString(category));
    }

    // Emit signal
    emit handler.messageLogged(severity, component, message, category);

    // Also use Qt's logging system for appropriate categories
    switch (category) {
    case ErrorCategory::Audio:
        switch (severity) {
        case ErrorSeverity::Info:
            qCInfo(xfbAudio) << component << ":" << message;
            break;
        case ErrorSeverity::Warning:
            qCWarning(xfbAudio) << component << ":" << message;
            break;
        case ErrorSeverity::Error:
        case ErrorSeverity::Critical:
            qCCritical(xfbAudio) << component << ":" << message;
            break;
        }
        break;
    case ErrorCategory::Database:
        switch (severity) {
        case ErrorSeverity::Info:
            qCInfo(xfbDatabase) << component << ":" << message;
            break;
        case ErrorSeverity::Warning:
            qCWarning(xfbDatabase) << component << ":" << message;
            break;
        case ErrorSeverity::Error:
        case ErrorSeverity::Critical:
            qCCritical(xfbDatabase) << component << ":" << message;
            break;
        }
        break;
    case ErrorCategory::Network:
        switch (severity) {
        case ErrorSeverity::Info:
            qCInfo(xfbNetwork) << component << ":" << message;
            break;
        case ErrorSeverity::Warning:
            qCWarning(xfbNetwork) << component << ":" << message;
            break;
        case ErrorSeverity::Error:
        case ErrorSeverity::Critical:
            qCCritical(xfbNetwork) << component << ":" << message;
            break;
        }
        break;
    case ErrorCategory::UI:
        switch (severity) {
        case ErrorSeverity::Info:
            qCInfo(xfbUI) << component << ":" << message;
            break;
        case ErrorSeverity::Warning:
            qCWarning(xfbUI) << component << ":" << message;
            break;
        case ErrorSeverity::Error:
        case ErrorSeverity::Critical:
            qCCritical(xfbUI) << component << ":" << message;
            break;
        }
        break;
    default:
        switch (severity) {
        case ErrorSeverity::Info:
            qCInfo(xfbCore) << component << ":" << message;
            break;
        case ErrorSeverity::Warning:
            qCWarning(xfbCore) << component << ":" << message;
            break;
        case ErrorSeverity::Error:
        case ErrorSeverity::Critical:
            qCCritical(xfbCore) << component << ":" << message;
            break;
        }
        break;
    }
}

void ErrorHandler::setLogLevel(ErrorSeverity severity)
{
    m_logLevel = severity;
    if (m_logger) {
        m_logger->setMinLevel(static_cast<Logger::LogLevel>(severity));
    }
}

void ErrorHandler::setUserDialogsEnabled(bool enabled)
{
    m_userDialogsEnabled = enabled;
}

QString ErrorHandler::getLogDirectory() const
{
    return m_logDirectory;
}

QString ErrorHandler::severityToString(ErrorSeverity severity)
{
    switch (severity) {
    case ErrorSeverity::Info:
        return "INFO";
    case ErrorSeverity::Warning:
        return "WARNING";
    case ErrorSeverity::Error:
        return "ERROR";
    case ErrorSeverity::Critical:
        return "CRITICAL";
    }
    return "UNKNOWN";
}

QString ErrorHandler::categoryToString(ErrorCategory category)
{
    switch (category) {
    case ErrorCategory::General:
        return "General";
    case ErrorCategory::Audio:
        return "Audio";
    case ErrorCategory::Database:
        return "Database";
    case ErrorCategory::Network:
        return "Network";
    case ErrorCategory::UI:
        return "UI";
    case ErrorCategory::File:
        return "File";
    }
    return "Unknown";
}