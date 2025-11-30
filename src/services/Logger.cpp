#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QDebug>
#include <QCoreApplication>

// Static constants
const QString Logger::LOG_FILE_PREFIX = "xfb";
const QString Logger::LOG_FILE_EXTENSION = ".log";
const int Logger::MAINTENANCE_INTERVAL_MS = 60000; // 1 minute

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_maxFiles(10)
    , m_maxSizeMB(10)
    , m_minLevel(LogLevel::Info)
    , m_enabled(false)
    , m_maintenanceTimer(new QTimer(this))
{
    // Set up maintenance timer
    m_maintenanceTimer->setInterval(MAINTENANCE_INTERVAL_MS);
    m_maintenanceTimer->setSingleShot(false);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &Logger::performMaintenance);
}

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_logStream) {
        m_logStream->flush();
        m_logStream.reset();
    }
    
    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
    }
    
    m_maintenanceTimer->stop();
}

bool Logger::initialize(const QString& logDirectory, int maxFiles, int maxSizeMB, LogLevel minLevel)
{
    QMutexLocker locker(&m_mutex);
    
    m_logDirectory = logDirectory;
    m_maxFiles = maxFiles;
    m_maxSizeMB = maxSizeMB;
    m_minLevel = minLevel;
    
    if (!ensureLogDirectory()) {
        qWarning() << "Failed to create log directory:" << m_logDirectory;
        return false;
    }
    
    if (!createNewLogFile()) {
        qWarning() << "Failed to create initial log file";
        return false;
    }
    
    m_enabled = true;
    
    // Only start timer if we have an event loop running
    if (QCoreApplication::instance()) {
        m_maintenanceTimer->start();
    }
    
    // Write initialization message directly (avoid recursive mutex lock)
    if (m_logStream && m_logFile && m_logFile->isOpen()) {
        QString formattedMessage = formatMessage(LogLevel::Info, "Logger", "Logging system initialized", "System");
        *m_logStream << formattedMessage << Qt::endl;
        m_logStream->flush();
    }
    
    return true;
}

void Logger::writeLog(LogLevel level, const QString& component, const QString& message, const QString& category)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_enabled || level < m_minLevel) {
        return;
    }
    
    if (!m_logStream) {
        return;
    }
    
    QString formattedMessage = formatMessage(level, component, message, category);
    *m_logStream << formattedMessage << Qt::endl;
    m_logStream->flush();
    
    // Check if rotation is needed (without holding the lock too long)
    if (needsRotation()) {
        // Schedule rotation for next maintenance cycle
        QMetaObject::invokeMethod(this, &Logger::rotateIfNeeded, Qt::QueuedConnection);
    }
}

void Logger::setMinLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

QString Logger::getCurrentLogFile() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentLogFile;
}

QString Logger::getLogDirectory() const
{
    return m_logDirectory;
}

bool Logger::isEnabled() const
{
    return m_enabled;
}

void Logger::flush()
{
    QMutexLocker locker(&m_mutex);
    if (m_logStream) {
        m_logStream->flush();
    }
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Critical:
        return "CRIT";
    }
    return "UNKNOWN";
}

void Logger::rotateIfNeeded()
{
    QMutexLocker locker(&m_mutex);
    
    if (!needsRotation()) {
        return;
    }
    
    // Close current file
    if (m_logStream) {
        m_logStream->flush();
        m_logStream.reset();
    }
    
    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
    }
    
    // Create new log file
    if (createNewLogFile()) {
        writeLog(LogLevel::Info, "Logger", "Log file rotated", "System");
    }
}

void Logger::cleanupOldFiles()
{
    QMutexLocker locker(&m_mutex);
    
    QDir logDir(m_logDirectory);
    if (!logDir.exists()) {
        return;
    }
    
    // Get all log files
    QStringList filters;
    filters << QString("%1*%2").arg(LOG_FILE_PREFIX, LOG_FILE_EXTENSION);
    QFileInfoList logFiles = logDir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    
    // Remove excess files
    while (logFiles.size() > m_maxFiles) {
        QFileInfo oldestFile = logFiles.takeLast();
        if (QFile::remove(oldestFile.absoluteFilePath())) {
            qDebug() << "Removed old log file:" << oldestFile.fileName();
        }
    }
}

void Logger::performMaintenance()
{
    rotateIfNeeded();
    cleanupOldFiles();
}

bool Logger::createNewLogFile()
{
    QString fileName = getNextLogFileName();
    m_currentLogFile = QDir(m_logDirectory).absoluteFilePath(fileName);
    
    m_logFile = std::make_unique<QFile>(m_currentLogFile);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Failed to open log file:" << m_currentLogFile;
        m_logFile.reset();
        return false;
    }
    
    m_logStream = std::make_unique<QTextStream>(m_logFile.get());
    m_logStream->setEncoding(QStringConverter::Utf8);
    
    return true;
}

bool Logger::needsRotation() const
{
    if (!m_logFile) {
        return false;
    }
    
    // Check file size
    qint64 currentSize = m_logFile->size();
    qint64 maxSize = static_cast<qint64>(m_maxSizeMB) * 1024 * 1024;
    
    return currentSize >= maxSize;
}

QString Logger::getNextLogFileName() const
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_hhmmss");
    return QString("%1_%2%3").arg(LOG_FILE_PREFIX, timestamp, LOG_FILE_EXTENSION);
}

QString Logger::formatMessage(LogLevel level, const QString& component, const QString& message, const QString& category) const
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    return QString("[%1] [%2] [%3] [%4] %5")
           .arg(timestamp)
           .arg(levelToString(level), -5)  // Left-aligned, minimum 5 characters
           .arg(category, -10)             // Left-aligned, minimum 10 characters
           .arg(component, -15)            // Left-aligned, minimum 15 characters
           .arg(message);
}

bool Logger::ensureLogDirectory()
{
    QDir dir;
    return dir.mkpath(m_logDirectory);
}