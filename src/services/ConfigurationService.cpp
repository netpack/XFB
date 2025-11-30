#include "ConfigurationService.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMutexLocker>
#include <QMutex>

ConfigurationService::ConfigurationService(QObject* parent)
    : BaseService(parent)
{
    // Configuration will be stored in application data directory
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    
    QString configFile = QDir(configPath).filePath("xfb.conf");
    m_settings = std::make_unique<QSettings>(configFile, QSettings::IniFormat);
}

ConfigurationService::~ConfigurationService()
{
    shutdown();
}

bool ConfigurationService::initialize()
{
    // BaseService::initialize() will call doInitialize()
    return BaseService::initialize();
}

bool ConfigurationService::doInitialize()
{
    try {
        loadDefaults();
        
        if (!validateConfiguration()) {
            qWarning() << "ConfigurationService: Invalid configuration detected, resetting to defaults";
            resetToDefaults();
        }
        
        qDebug() << "ConfigurationService: Initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "ConfigurationService: Initialization failed:" << e.what();
        emit configurationError(QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void ConfigurationService::shutdown()
{
    BaseService::shutdown();
}

void ConfigurationService::doShutdown()
{
    sync();
    qDebug() << "ConfigurationService: Shutdown completed";
}

QVariant ConfigurationService::getValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    return m_settings->value(key, defaultValue);
}

void ConfigurationService::setValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    
    if (!validateKey(key, value)) {
        qWarning() << "ConfigurationService: Invalid value for key" << key;
        emit configurationError(QString("Invalid value for configuration key: %1").arg(key));
        return;
    }
    
    QVariant oldValue = m_settings->value(key);
    if (oldValue != value) {
        m_settings->setValue(key, value);
        emit configurationChanged(key, value);
    }
}

QString ConfigurationService::getLanguage() const
{
    return getValue("ui/language", "en").toString();
}

void ConfigurationService::setLanguage(const QString& language)
{
    setValue("ui/language", language);
}

bool ConfigurationService::isDarkModeEnabled() const
{
    return getValue("ui/darkMode", false).toBool();
}

void ConfigurationService::setDarkModeEnabled(bool enabled)
{
    setValue("ui/darkMode", enabled);
}

bool ConfigurationService::isFullScreenEnabled() const
{
    return getValue("ui/fullScreen", false).toBool();
}

void ConfigurationService::setFullScreenEnabled(bool enabled)
{
    setValue("ui/fullScreen", enabled);
}

QString ConfigurationService::getDatabasePath() const
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return getValue("database/path", QDir(defaultPath).filePath("xfb.db")).toString();
}

void ConfigurationService::setDatabasePath(const QString& path)
{
    setValue("database/path", path);
}

int ConfigurationService::getDefaultVolume() const
{
    return getValue("audio/defaultVolume", 75).toInt();
}

void ConfigurationService::setDefaultVolume(int volume)
{
    setValue("audio/defaultVolume", qBound(0, volume, 100));
}

bool ConfigurationService::validateConfiguration() const
{
    // Don't lock mutex here since getValue() already handles locking
    
    // Validate language
    QString language = getValue("ui/language", "en").toString();
    if (language.isEmpty() || language.length() > 5) {
        return false;
    }
    
    // Validate volume
    int volume = getValue("audio/defaultVolume", 75).toInt();
    if (volume < 0 || volume > 100) {
        return false;
    }
    
    // Validate database path
    QString dbPath = getValue("database/path").toString();
    if (!dbPath.isEmpty()) {
        QFileInfo fileInfo(dbPath);
        if (!fileInfo.dir().exists()) {
            return false;
        }
    }
    
    return true;
}

void ConfigurationService::resetToDefaults()
{
    QMutexLocker locker(&m_mutex);
    
    m_settings->clear();
    
    // Load defaults directly without using setValue to avoid recursive locking
    m_settings->setValue("ui/language", "en");
    m_settings->setValue("ui/darkMode", false);
    m_settings->setValue("ui/fullScreen", false);
    m_settings->setValue("audio/defaultVolume", 75);
    m_settings->setValue("audio/outputDevice", "");
    m_settings->setValue("audio/inputDevice", "");
    
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_settings->setValue("database/path", QDir(defaultPath).filePath("xfb.db"));
    m_settings->setValue("database/autoBackup", true);
    m_settings->setValue("database/backupRetentionDays", 30);
    
    locker.unlock(); // Unlock before emitting signal
    emit configurationChanged("*", QVariant()); // Signal that all configuration changed
}

void ConfigurationService::sync()
{
    QMutexLocker locker(&m_mutex);
    m_settings->sync();
}

void ConfigurationService::loadDefaults()
{
    // UI defaults
    if (!m_settings->contains("ui/language")) {
        m_settings->setValue("ui/language", "en");
    }
    if (!m_settings->contains("ui/darkMode")) {
        m_settings->setValue("ui/darkMode", false);
    }
    if (!m_settings->contains("ui/fullScreen")) {
        m_settings->setValue("ui/fullScreen", false);
    }
    
    // Audio defaults
    if (!m_settings->contains("audio/defaultVolume")) {
        m_settings->setValue("audio/defaultVolume", 75);
    }
    if (!m_settings->contains("audio/outputDevice")) {
        m_settings->setValue("audio/outputDevice", "");
    }
    if (!m_settings->contains("audio/inputDevice")) {
        m_settings->setValue("audio/inputDevice", "");
    }
    
    // Database defaults
    if (!m_settings->contains("database/path")) {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_settings->setValue("database/path", QDir(defaultPath).filePath("xfb.db"));
    }
    if (!m_settings->contains("database/autoBackup")) {
        m_settings->setValue("database/autoBackup", true);
    }
    if (!m_settings->contains("database/backupRetentionDays")) {
        m_settings->setValue("database/backupRetentionDays", 30);
    }
}

bool ConfigurationService::validateKey(const QString& key, const QVariant& value) const
{
    if (key.startsWith("ui/language")) {
        QString lang = value.toString();
        return !lang.isEmpty() && lang.length() <= 5;
    }
    
    if (key.startsWith("audio/defaultVolume")) {
        int volume = value.toInt();
        return volume >= 0 && volume <= 100;
    }
    
    if (key.startsWith("ui/darkMode") || key.startsWith("ui/fullScreen") || 
        key.startsWith("database/autoBackup")) {
        return value.canConvert<bool>();
    }
    
    if (key.startsWith("database/backupRetentionDays")) {
        int days = value.toInt();
        return days >= 1 && days <= 365;
    }
    
    // Default: allow all other keys
    return true;
}