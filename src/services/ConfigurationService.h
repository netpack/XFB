#ifndef CONFIGURATIONSERVICE_H
#define CONFIGURATIONSERVICE_H

#include "BaseService.h"
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QMutex>
#include <memory>

/**
 * @brief Service for managing application configuration
 * 
 * The ConfigurationService provides centralized configuration management
 * with validation and secure storage capabilities.
 * 
 * @since XFB 2.0
 */
class ConfigurationService : public BaseService
{
    Q_OBJECT

public:
    explicit ConfigurationService(QObject* parent = nullptr);
    ~ConfigurationService() override;

    // BaseService interface
    bool initialize() override;
    void shutdown() override;
    QString serviceName() const override { return "ConfigurationService"; }
    QString getServiceName() const override { return "ConfigurationService"; }

    /**
     * @brief Get configuration value
     * @param key Configuration key
     * @param defaultValue Default value if key doesn't exist
     * @return Configuration value
     */
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief Set configuration value
     * @param key Configuration key
     * @param value Configuration value
     */
    void setValue(const QString& key, const QVariant& value);

    /**
     * @brief Get application language
     * @return Language code (e.g., "en", "pt")
     */
    QString getLanguage() const;

    /**
     * @brief Set application language
     * @param language Language code
     */
    void setLanguage(const QString& language);

    /**
     * @brief Check if dark mode is enabled
     * @return true if dark mode is enabled
     */
    bool isDarkModeEnabled() const;

    /**
     * @brief Enable or disable dark mode
     * @param enabled true to enable dark mode
     */
    void setDarkModeEnabled(bool enabled);

    /**
     * @brief Check if full screen mode is enabled
     * @return true if full screen mode is enabled
     */
    bool isFullScreenEnabled() const;

    /**
     * @brief Enable or disable full screen mode
     * @param enabled true to enable full screen mode
     */
    void setFullScreenEnabled(bool enabled);

    /**
     * @brief Get database path
     * @return Path to the database file
     */
    QString getDatabasePath() const;

    /**
     * @brief Set database path
     * @param path Path to the database file
     */
    void setDatabasePath(const QString& path);

    /**
     * @brief Get default volume level
     * @return Volume level (0-100)
     */
    int getDefaultVolume() const;

    /**
     * @brief Set default volume level
     * @param volume Volume level (0-100)
     */
    void setDefaultVolume(int volume);

    /**
     * @brief Validate current configuration
     * @return true if configuration is valid
     */
    bool validateConfiguration() const;

    /**
     * @brief Reset configuration to defaults
     */
    void resetToDefaults();

    /**
     * @brief Sync configuration to storage
     */
    void sync();

protected:
    // BaseService interface
    bool doInitialize() override;
    void doShutdown() override;

signals:
    /**
     * @brief Emitted when configuration value changes
     * @param key Configuration key that changed
     * @param value New value
     */
    void configurationChanged(const QString& key, const QVariant& value);

    /**
     * @brief Emitted when configuration error occurs
     * @param error Error message
     */
    void configurationError(const QString& error);

private:
    /**
     * @brief Load default configuration values
     */
    void loadDefaults();

    /**
     * @brief Validate a specific configuration key
     * @param key Configuration key
     * @param value Configuration value
     * @return true if valid
     */
    bool validateKey(const QString& key, const QVariant& value) const;

private:
    std::unique_ptr<QSettings> m_settings;
    mutable QMutex m_mutex;
};

#endif // CONFIGURATIONSERVICE_H