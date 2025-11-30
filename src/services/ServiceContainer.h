#ifndef SERVICECONTAINER_H
#define SERVICECONTAINER_H

#include "IService.h"
#include <QObject>
#include <QHash>
#include <QString>
#include <QSharedPointer>
#include <QMutex>
#include <QMutexLocker>
#include <functional>
#include <typeinfo>
#include <memory>

/**
 * @brief Dependency injection container for managing services
 * 
 * The ServiceContainer provides a centralized way to register, resolve, and manage
 * service dependencies throughout the XFB application. It supports both singleton
 * and transient service lifetimes.
 * 
 * @example
 * @code
 * ServiceContainer* container = ServiceContainer::instance();
 * 
 * // Register a singleton service
 * container->registerSingleton<AudioService>();
 * 
 * // Register with factory function
 * container->registerSingleton<DatabaseService>([](ServiceContainer* container) {
 *     return new DatabaseService(container->resolve<ConfigurationService>());
 * });
 * 
 * // Resolve a service
 * auto audioService = container->resolve<AudioService>();
 * @endcode
 * 
 * @since XFB 2.0
 */
class ServiceContainer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Service lifetime management
     */
    enum class ServiceLifetime {
        Singleton,  ///< Single instance shared across the application
        Transient   ///< New instance created for each resolution
    };

    /**
     * @brief Factory function type for creating services
     */
    using ServiceFactory = std::function<IService*(ServiceContainer*)>;

    /**
     * @brief Get the global service container instance
     * @return Pointer to the singleton ServiceContainer instance
     */
    static ServiceContainer* instance();

    /**
     * @brief Destroy the global service container instance
     */
    static void destroyInstance();

    /**
     * @brief Register a service type with singleton lifetime
     * @tparam T Service type that inherits from IService
     * @param factory Optional factory function for custom construction
     */
    template<typename T>
    void registerSingleton(ServiceFactory factory = nullptr) {
        static_assert(std::is_base_of_v<IService, T>, "T must inherit from IService");
        registerService(typeid(T).name(), ServiceLifetime::Singleton, factory ? factory : defaultFactory<T>());
    }

    /**
     * @brief Register a service type with transient lifetime
     * @tparam T Service type that inherits from IService
     * @param factory Optional factory function for custom construction
     */
    template<typename T>
    void registerTransient(ServiceFactory factory = nullptr) {
        static_assert(std::is_base_of_v<IService, T>, "T must inherit from IService");
        registerService(typeid(T).name(), ServiceLifetime::Transient, factory ? factory : defaultFactory<T>());
    }

    /**
     * @brief Resolve a service instance
     * @tparam T Service type to resolve
     * @return Pointer to the service instance, or nullptr if not found
     */
    template<typename T>
    T* resolve() {
        static_assert(std::is_base_of_v<IService, T>, "T must inherit from IService");
        return static_cast<T*>(resolveService(typeid(T).name()));
    }

    /**
     * @brief Check if a service type is registered
     * @tparam T Service type to check
     * @return true if the service is registered
     */
    template<typename T>
    bool isRegistered() const {
        static_assert(std::is_base_of_v<IService, T>, "T must inherit from IService");
        return isServiceRegistered(typeid(T).name());
    }

    /**
     * @brief Initialize all registered services
     * @return true if all services initialized successfully
     */
    bool initializeServices();

    /**
     * @brief Shutdown all services gracefully
     */
    void shutdownServices();

    /**
     * @brief Clear all service registrations and instances
     */
    void clear();

    /**
     * @brief Get the number of registered services
     * @return Number of registered services
     */
    int serviceCount() const;

signals:
    /**
     * @brief Emitted when a service is successfully registered
     * @param serviceName Name of the registered service
     */
    void serviceRegistered(const QString& serviceName);

    /**
     * @brief Emitted when a service is resolved
     * @param serviceName Name of the resolved service
     */
    void serviceResolved(const QString& serviceName);

    /**
     * @brief Emitted when service initialization fails
     * @param serviceName Name of the service that failed to initialize
     * @param error Error message
     */
    void serviceInitializationFailed(const QString& serviceName, const QString& error);

private:
    explicit ServiceContainer(QObject* parent = nullptr);
    ~ServiceContainer() override;

    /**
     * @brief Service registration information
     */
    struct ServiceRegistration {
        ServiceLifetime lifetime;
        ServiceFactory factory;
        QSharedPointer<IService> singletonInstance;
    };

    /**
     * @brief Register a service with the container
     * @param serviceName Unique service name (typically from typeid)
     * @param lifetime Service lifetime
     * @param factory Factory function for creating the service
     */
    void registerService(const QString& serviceName, ServiceLifetime lifetime, ServiceFactory factory);

    /**
     * @brief Resolve a service by name
     * @param serviceName Service name to resolve
     * @return Pointer to the service instance
     */
    IService* resolveService(const QString& serviceName);

    /**
     * @brief Check if a service is registered by name
     * @param serviceName Service name to check
     * @return true if registered
     */
    bool isServiceRegistered(const QString& serviceName) const;

    /**
     * @brief Create default factory function for a service type
     * @tparam T Service type
     * @return Factory function that creates instances of T
     */
    template<typename T>
    ServiceFactory defaultFactory() {
        return [](ServiceContainer*) -> IService* {
            return new T();
        };
    }

    static ServiceContainer* s_instance;
    static QMutex s_instanceMutex;

    mutable QMutex m_mutex;
    QHash<QString, ServiceRegistration> m_services;
    QList<QString> m_initializationOrder;
};

#endif // SERVICECONTAINER_H