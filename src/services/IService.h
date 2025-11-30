#ifndef ISERVICE_H
#define ISERVICE_H

#include <QObject>
#include <QString>

/**
 * @brief Base interface for all services in the XFB application
 * 
 * This interface defines the common contract that all services must implement,
 * providing lifecycle management and basic service identification.
 * 
 * @since XFB 2.0
 */
class IService : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Service lifecycle states
     */
    enum class ServiceState {
        Uninitialized,  ///< Service has been created but not initialized
        Initializing,   ///< Service is currently being initialized
        Running,        ///< Service is running and ready to use
        Stopping,       ///< Service is being stopped
        Stopped,        ///< Service has been stopped
        Error           ///< Service encountered an error
    };

    explicit IService(QObject* parent = nullptr) : QObject(parent), m_state(ServiceState::Uninitialized) {}
    virtual ~IService() = default;

    /**
     * @brief Initialize the service
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown the service gracefully
     */
    virtual void shutdown() = 0;

    /**
     * @brief Get the service name for identification
     * @return The service name
     */
    virtual QString serviceName() const = 0;

    /**
     * @brief Get the current service state
     * @return The current state of the service
     */
    ServiceState state() const { return m_state; }

    /**
     * @brief Check if the service is running
     * @return true if the service is in Running state
     */
    bool isRunning() const { return m_state == ServiceState::Running; }

signals:
    /**
     * @brief Emitted when the service state changes
     * @param newState The new service state
     */
    void stateChanged(ServiceState newState);

    /**
     * @brief Emitted when a service error occurs
     * @param error Error message
     */
    void serviceError(const QString& error);

protected:
    /**
     * @brief Set the service state and emit stateChanged signal
     * @param newState The new state to set
     */
    void setState(ServiceState newState) {
        if (m_state != newState) {
            m_state = newState;
            emit stateChanged(newState);
        }
    }

private:
    ServiceState m_state;
};

Q_DECLARE_METATYPE(IService::ServiceState)

#endif // ISERVICE_H