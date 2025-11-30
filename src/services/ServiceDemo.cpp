#include "ServiceContainer.h"
#include "BaseService.h"
#include <QCoreApplication>
#include <QDebug>

/**
 * @brief Example service demonstrating the service layer usage
 */
class ExampleService : public BaseService
{
    Q_OBJECT

public:
    explicit ExampleService(QObject* parent = nullptr) : BaseService(parent) {}

protected:
    bool doInitialize() override {
        logDebug("Performing example service initialization...");
        // Simulate some initialization work
        return true;
    }
    
    void doShutdown() override {
        logDebug("Performing example service cleanup...");
        // Simulate cleanup work
    }
    
    QString getServiceName() const override {
        return "ExampleService";
    }

public slots:
    void doSomeWork() {
        if (!isRunning()) {
            logError("Service is not running!");
            return;
        }
        logDebug("Doing some important work...");
    }
};

/**
 * @brief Demo function showing service container usage
 */
void demonstrateServiceContainer()
{
    qDebug() << "=== Service Container Demo ===";
    
    // Get the service container instance
    ServiceContainer* container = ServiceContainer::instance();
    
    // Register services
    qDebug() << "Registering ExampleService...";
    container->registerSingleton<ExampleService>();
    
    // Initialize all services
    qDebug() << "Initializing services...";
    bool success = container->initializeServices();
    qDebug() << "Initialization result:" << success;
    
    // Resolve and use a service
    qDebug() << "Resolving ExampleService...";
    ExampleService* service = container->resolve<ExampleService>();
    if (service) {
        qDebug() << "Service state:" << (int)service->state();
        qDebug() << "Service uptime:" << service->uptime() << "ms";
        service->doSomeWork();
    }
    
    // Shutdown services
    qDebug() << "Shutting down services...";
    container->shutdownServices();
    
    qDebug() << "=== Demo Complete ===";
}

#include "ServiceDemo.moc"