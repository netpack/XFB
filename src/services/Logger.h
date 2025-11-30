#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QTimer>
#include <memory>

/**
 * @brief File-based logging system with rotation and filtering
 * 
 * The Logger class provides thread-safe file logging with automatic
 * file rotation based on size and age. It supports different log levels
 * and categories for better organization of log messages.
 * 
 * @since XFB 2.0
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Log levels matching ErrorHandler severity levels
     */
    enum class LogLevel {
        Info = 0,
        Warning = 1,
        Error = 2,
        Critical = 3
    };

    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit Logger(QObject* parent = nullptr);

    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~Logger() override;

    /**
     * @brief Initialize the logger
     * @param logDirectory Directory where log files will be stored
     * @param maxFiles Maximum number of log files to keep
     * @param maxSizeMB Maximum size of each log file in MB
     * @param minLevel Minimum log level to write
     * @return true if initialization was successful
     */
    bool initialize(const QString& logDirectory,
                   int maxFiles = 10,
                   int maxSizeMB = 10,
                   LogLevel minLevel = LogLevel::Info);

    /**
     * @brief Write a log message
     * @param level Log level
     * @param component Component generating the message
     * @param message Message text
     * @param category Message category
     */
    void writeLog(LogLevel level,
                 const QString& component,
                 const QString& message,
                 const QString& category = "General");

    /**
     * @brief Set the minimum log level
     * @param level Minimum level to log
     */
    void setMinLevel(LogLevel level);

    /**
     * @brief Get the current log file path
     * @return Path to the current log file
     */
    QString getCurrentLogFile() const;

    /**
     * @brief Get the log directory
     * @return Path to the log directory
     */
    QString getLogDirectory() const;

    /**
     * @brief Check if logging is enabled
     * @return true if logging is active
     */
    bool isEnabled() const;

    /**
     * @brief Flush any pending log messages to disk
     */
    void flush();

    /**
     * @brief Convert log level to string
     * @param level Log level
     * @return String representation
     */
    static QString levelToString(LogLevel level);

public slots:
    /**
     * @brief Rotate log files if needed
     */
    void rotateIfNeeded();

    /**
     * @brief Clean up old log files
     */
    void cleanupOldFiles();

private slots:
    /**
     * @brief Periodic maintenance timer
     */
    void performMaintenance();

private:
    /**
     * @brief Create a new log file
     * @return true if successful
     */
    bool createNewLogFile();

    /**
     * @brief Check if current log file needs rotation
     * @return true if rotation is needed
     */
    bool needsRotation() const;

    /**
     * @brief Get the next log file name
     * @return Path to the next log file
     */
    QString getNextLogFileName() const;

    /**
     * @brief Format a log message
     * @param level Log level
     * @param component Component name
     * @param message Message text
     * @param category Message category
     * @return Formatted log message
     */
    QString formatMessage(LogLevel level,
                         const QString& component,
                         const QString& message,
                         const QString& category) const;

    /**
     * @brief Ensure log directory exists
     * @return true if directory exists or was created
     */
    bool ensureLogDirectory();

    QString m_logDirectory;
    QString m_currentLogFile;
    std::unique_ptr<QFile> m_logFile;
    std::unique_ptr<QTextStream> m_logStream;
    
    int m_maxFiles;
    int m_maxSizeMB;
    LogLevel m_minLevel;
    bool m_enabled;
    
    mutable QMutex m_mutex;
    QTimer* m_maintenanceTimer;
    
    static const QString LOG_FILE_PREFIX;
    static const QString LOG_FILE_EXTENSION;
    static const int MAINTENANCE_INTERVAL_MS;
};

#endif // LOGGER_H