#ifndef BASESERVICE_H
#define BASESERVICE_H

#include "IService.h"
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>

/**
 * @brief Base implementation of IService providing common functionality
 * 
 * This class provides a concrete implementation of the IService interface
 * with common functionality that most services will need, such as
 * initialization tracking, error handling, and basic lifecycle management.
 * 
 * @since XFB 2.0
 */
class BaseService : public IService
{
    Q_OBJECT

public:
    explicit BaseService(QObject* parent = nullptr);
    ~BaseService() override;

    // IService interface implementation
    bool initialize() override;
    void shutdown() override;
    QString serviceName() const override;

    /**
     * @brief Get the time when the service was initialized
     * @return Initialization timestamp, or invalid QDateTime if not initialized
     */
    QDateTime initializationTime() const { return m_initializationTime; }

    /**
     * @brief Get the time elapsed since initialization
     * @return Elapsed time in milliseconds, or -1 if not initialized
     */
    qint64 uptime() const;

    /**
     * @brief Check if the service has encountered any errors
     * @return true if the service is in error state
     */
    bool hasError() const { return state() == ServiceState::Error; }

    /**
     * @brief Get the last error message
     * @return Last error message, or empty string if no error
     */
    QString lastError() const { return m_lastError; }

protected:
    /**
     * @brief Override this method to implement service-specific initialization
     * @return true if initialization was successful
     */
    virtual bool doInitialize() { return true; }

    /**
     * @brief Override this method to implement service-specific shutdown
     */
    virtual void doShutdown() {}

    /**
     * @brief Override this method to provide the service name
     * @return The service name
     */
    virtual QString getServiceName() const = 0;

    /**
     * @brief Set an error state with message
     * @param error Error message
     */
    void setError(const QString& error);

    /**
     * @brief Clear the error state
     */
    void clearError();

    /**
     * @brief Log a debug message with service name prefix
     * @param message Debug message
     */
    void logDebug(const QString& message) const;

    /**
     * @brief Log a warning message with service name prefix
     * @param message Warning message
     */
    void logWarning(const QString& message) const;

    /**
     * @brief Log an error message with service name prefix
     * @param message Error message
     */
    void logError(const QString& message) const;

private slots:
    /**
     * @brief Handle initialization timeout
     */
    void onInitializationTimeout();

private:
    QDateTime m_initializationTime;
    QElapsedTimer m_uptimeTimer;
    QString m_lastError;
    QTimer* m_initializationTimer;
    
    static constexpr int INITIALIZATION_TIMEOUT_MS = 30000; // 30 seconds
};

#endif // BASESERVICE_H