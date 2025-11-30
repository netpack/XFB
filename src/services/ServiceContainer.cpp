#include "ServiceContainer.h"
#include <QDebug>
#include <QCoreApplication>

ServiceContainer* ServiceContainer::s_instance = nullptr;
QMutex ServiceContainer::s_instanceMutex;

ServiceContainer::ServiceContainer(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<IService::ServiceState>("IService::ServiceState");
}

ServiceContainer::~ServiceContainer()
{
    shutdownServices();
    clear();
}

ServiceContainer* ServiceContainer::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new ServiceContainer(QCoreApplication::instance());
    }
    return s_instance;
}

void ServiceContainer::destroyInstance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void ServiceContainer::registerService(const QString& serviceName, ServiceLifetime lifetime, ServiceFactory factory)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_services.contains(serviceName)) {
        qWarning() << "Service already registered:" << serviceName;
        return;
    }

    ServiceRegistration registration;
    registration.lifetime = lifetime;
    registration.factory = factory;
    
    m_services.insert(serviceName, registration);
    m_initializationOrder.append(serviceName);
    
    emit serviceRegistered(serviceName);
    
    qDebug() << "Registered service:" << serviceName 
             << "with lifetime:" << (lifetime == ServiceLifetime::Singleton ? "Singleton" : "Transient");
}

IService* ServiceContainer::resolveService(const QString& serviceName)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_services.contains(serviceName)) {
        qWarning() << "Service not registered:" << serviceName;
        return nullptr;
    }

    ServiceRegistration& registration = m_services[serviceName];
    
    if (registration.lifetime == ServiceLifetime::Singleton) {
        if (!registration.singletonInstance) {
            // Create singleton instance
            IService* service = registration.factory(this);
            if (service) {
                registration.singletonInstance = QSharedPointer<IService>(service);
                qDebug() << "Created singleton instance for service:" << serviceName;
            } else {
                qCritical() << "Failed to create service instance:" << serviceName;
                return nullptr;
            }
        }
        
        emit serviceResolved(serviceName);
        return registration.singletonInstance.data();
    } else {
        // Create transient instance
        IService* service = registration.factory(this);
        if (service) {
            qDebug() << "Created transient instance for service:" << serviceName;
            emit serviceResolved(serviceName);
        } else {
            qCritical() << "Failed to create transient service instance:" << serviceName;
        }
        return service;
    }
}

bool ServiceContainer::isServiceRegistered(const QString& serviceName) const
{
    QMutexLocker locker(&m_mutex);
    return m_services.contains(serviceName);
}

bool ServiceContainer::initializeServices()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "Initializing" << m_services.size() << "services...";
    
    bool allSuccessful = true;
    
    // Initialize services in registration order
    for (const QString& serviceName : m_initializationOrder) {
        if (!m_services.contains(serviceName)) {
            continue; // Service might have been removed
        }
        
        ServiceRegistration& registration = m_services[serviceName];
        
        // For singletons, create and initialize the instance
        if (registration.lifetime == ServiceLifetime::Singleton) {
            if (!registration.singletonInstance) {
                IService* service = registration.factory(this);
                if (service) {
                    registration.singletonInstance = QSharedPointer<IService>(service);
                } else {
                    qCritical() << "Failed to create service:" << serviceName;
                    emit serviceInitializationFailed(serviceName, "Failed to create service instance");
                    allSuccessful = false;
                    continue;
                }
            }
            
            IService* service = registration.singletonInstance.data();
            if (service && service->state() == IService::ServiceState::Uninitialized) {
                qDebug() << "Initializing service:" << serviceName;
                
                try {
                    if (!service->initialize()) {
                        qWarning() << "Service initialization failed:" << serviceName << "- continuing with other services";
                        emit serviceInitializationFailed(serviceName, "Service initialization returned false");
                        // Don't set allSuccessful = false for optional services like AudioService
                        if (serviceName != "AudioService") {
                            allSuccessful = false;
                        }
                    } else {
                        qDebug() << "Successfully initialized service:" << serviceName;
                    }
                } catch (const std::exception& e) {
                    qWarning() << "Exception during service initialization:" << serviceName << "-" << e.what();
                    emit serviceInitializationFailed(serviceName, QString("Exception: %1").arg(e.what()));
                    // Don't fail completely for optional services
                    if (serviceName != "AudioService") {
                        allSuccessful = false;
                    }
                } catch (...) {
                    qWarning() << "Unknown exception during service initialization:" << serviceName;
                    emit serviceInitializationFailed(serviceName, "Unknown exception occurred");
                    // Don't fail completely for optional services
                    if (serviceName != "AudioService") {
                        allSuccessful = false;
                    }
                }
            }
        }
        // Transient services are initialized when resolved
    }
    
    qDebug() << "Service initialization completed. Success:" << allSuccessful;
    return allSuccessful;
}

void ServiceContainer::shutdownServices()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "Shutting down services...";
    
    // Shutdown services in reverse order
    for (auto it = m_initializationOrder.rbegin(); it != m_initializationOrder.rend(); ++it) {
        const QString& serviceName = *it;
        
        if (!m_services.contains(serviceName)) {
            continue;
        }
        
        ServiceRegistration& registration = m_services[serviceName];
        
        if (registration.singletonInstance && registration.singletonInstance->isRunning()) {
            qDebug() << "Shutting down service:" << serviceName;
            registration.singletonInstance->shutdown();
        }
    }
    
    qDebug() << "Service shutdown completed.";
}

void ServiceContainer::clear()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "Clearing service container...";
    
    m_services.clear();
    m_initializationOrder.clear();
    
    qDebug() << "Service container cleared.";
}

int ServiceContainer::serviceCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_services.size();
}