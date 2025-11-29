#include "TestServiceContainer.h"

void TestServiceContainer::initTestCase()
{
    // Register meta types for signal testing
    qRegisterMetaType<IService::ServiceState>("IService::ServiceState");
}

void TestServiceContainer::cleanupTestCase()
{
    ServiceContainer::destroyInstance();
}

void TestServiceContainer::init()
{
    m_container = ServiceContainer::instance();
    m_container->clear();
}

void TestServiceContainer::cleanup()
{
    if (m_container) {
        m_container->clear();
    }
}

void TestServiceContainer::testSingletonInstance()
{
    ServiceContainer* instance1 = ServiceContainer::instance();
    ServiceContainer* instance2 = ServiceContainer::instance();
    
    QVERIFY(instance1 != nullptr);
    QVERIFY(instance2 != nullptr);
    QCOMPARE(instance1, instance2); // Should be the same instance
}

void TestServiceContainer::testServiceRegistration()
{
    QCOMPARE(m_container->serviceCount(), 0);
    
    QSignalSpy registeredSpy(m_container, &ServiceContainer::serviceRegistered);
    
    m_container->registerSingleton<MockService>();
    
    QCOMPARE(m_container->serviceCount(), 1);
    QVERIFY(m_container->isRegistered<MockService>());
    QCOMPARE(registeredSpy.count(), 1);
    
    QString serviceName = registeredSpy.at(0).at(0).toString();
    QVERIFY(!serviceName.isEmpty());
}

void TestServiceContainer::testSingletonServiceResolution()
{
    m_container->registerSingleton<MockService>();
    
    QSignalSpy resolvedSpy(m_container, &ServiceContainer::serviceResolved);
    
    MockService* service1 = m_container->resolve<MockService>();
    MockService* service2 = m_container->resolve<MockService>();
    
    QVERIFY(service1 != nullptr);
    QVERIFY(service2 != nullptr);
    QCOMPARE(service1, service2); // Should be the same instance for singleton
    QCOMPARE(resolvedSpy.count(), 2);
}

void TestServiceContainer::testTransientServiceResolution()
{
    m_container->registerTransient<MockService>();
    
    MockService* service1 = m_container->resolve<MockService>();
    MockService* service2 = m_container->resolve<MockService>();
    
    QVERIFY(service1 != nullptr);
    QVERIFY(service2 != nullptr);
    QVERIFY(service1 != service2); // Should be different instances for transient
    
    // Clean up transient instances
    delete service1;
    delete service2;
}

void TestServiceContainer::testServiceInitialization()
{
    m_container->registerSingleton<MockService>();
    
    bool result = m_container->initializeServices();
    QVERIFY(result);
    
    MockService* service = m_container->resolve<MockService>();
    QVERIFY(service != nullptr);
    QVERIFY(service->wasInitializeCalled());
    QVERIFY(service->isRunning());
}

void TestServiceContainer::testServiceShutdown()
{
    m_container->registerSingleton<MockService>();
    m_container->initializeServices();
    
    MockService* service = m_container->resolve<MockService>();
    QVERIFY(service != nullptr);
    QVERIFY(service->isRunning());
    
    m_container->shutdownServices();
    
    QVERIFY(service->wasShutdownCalled());
    QCOMPARE(service->state(), IService::ServiceState::Stopped);
}

void TestServiceContainer::testServiceNotFound()
{
    MockService* service = m_container->resolve<MockService>();
    QVERIFY(service == nullptr);
}

void TestServiceContainer::testDuplicateRegistration()
{
    m_container->registerSingleton<MockService>();
    QCOMPARE(m_container->serviceCount(), 1);
    
    // Try to register the same service again
    m_container->registerSingleton<MockService>();
    QCOMPARE(m_container->serviceCount(), 1); // Should still be 1
}

void TestServiceContainer::testServiceCount()
{
    QCOMPARE(m_container->serviceCount(), 0);
    
    m_container->registerSingleton<MockService>();
    QCOMPARE(m_container->serviceCount(), 1);
    
    m_container->registerSingleton<AnotherMockService>();
    QCOMPARE(m_container->serviceCount(), 2);
}

void TestServiceContainer::testClear()
{
    m_container->registerSingleton<MockService>();
    m_container->registerSingleton<AnotherMockService>();
    QCOMPARE(m_container->serviceCount(), 2);
    
    m_container->clear();
    QCOMPARE(m_container->serviceCount(), 0);
    QVERIFY(!m_container->isRegistered<MockService>());
    QVERIFY(!m_container->isRegistered<AnotherMockService>());
}

void TestServiceContainer::testCustomFactory()
{
    bool factoryCalled = false;
    
    m_container->registerSingleton<MockService>([&factoryCalled](ServiceContainer*) -> IService* {
        factoryCalled = true;
        return new MockService();
    });
    
    MockService* service = m_container->resolve<MockService>();
    QVERIFY(service != nullptr);
    QVERIFY(factoryCalled);
}

void TestServiceContainer::testSignalEmission()
{
    QSignalSpy registeredSpy(m_container, &ServiceContainer::serviceRegistered);
    QSignalSpy resolvedSpy(m_container, &ServiceContainer::serviceResolved);
    
    m_container->registerSingleton<MockService>();
    QCOMPARE(registeredSpy.count(), 1);
    
    MockService* service = m_container->resolve<MockService>();
    QVERIFY(service != nullptr);
    QCOMPARE(resolvedSpy.count(), 1);
}

QTEST_MAIN(TestServiceContainer)