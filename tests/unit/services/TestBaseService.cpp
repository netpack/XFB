#include "TestBaseService.h"

void TestBaseService::initTestCase()
{
    qRegisterMetaType<IService::ServiceState>("IService::ServiceState");
}

void TestBaseService::init()
{
    m_service = new TestableService(this);
}

void TestBaseService::cleanup()
{
    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }
}

void TestBaseService::testInitialState()
{
    QCOMPARE(m_service->state(), IService::ServiceState::Uninitialized);
    QVERIFY(!m_service->isRunning());
    QVERIFY(!m_service->hasError());
    QVERIFY(m_service->lastError().isEmpty());
    QCOMPARE(m_service->uptime(), -1);
}

void TestBaseService::testSuccessfulInitialization()
{
    QSignalSpy stateChangedSpy(m_service, &IService::stateChanged);
    
    bool result = m_service->initialize();
    
    QVERIFY(result);
    QVERIFY(m_service->wasInitializeCalled());
    QCOMPARE(m_service->state(), IService::ServiceState::Running);
    QVERIFY(m_service->isRunning());
    QVERIFY(!m_service->hasError());
    QVERIFY(m_service->initializationTime().isValid());
    QVERIFY(m_service->uptime() >= 0);
    
    // Should have emitted state changes: Uninitialized -> Initializing -> Running
    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(stateChangedSpy.at(0).at(0).value<IService::ServiceState>(), IService::ServiceState::Initializing);
    QCOMPARE(stateChangedSpy.at(1).at(0).value<IService::ServiceState>(), IService::ServiceState::Running);
}

void TestBaseService::testFailedInitialization()
{
    QSignalSpy stateChangedSpy(m_service, &IService::stateChanged);
    QSignalSpy errorSpy(m_service, &IService::serviceError);
    
    m_service->setInitializeResult(false);
    bool result = m_service->initialize();
    
    QVERIFY(!result);
    QVERIFY(m_service->wasInitializeCalled());
    QCOMPARE(m_service->state(), IService::ServiceState::Error);
    QVERIFY(!m_service->isRunning());
    QVERIFY(m_service->hasError());
    QVERIFY(!m_service->lastError().isEmpty());
    
    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 1);
}

void TestBaseService::testExceptionDuringInitialization()
{
    ExceptionService* exceptionService = new ExceptionService(this);
    QSignalSpy errorSpy(exceptionService, &IService::serviceError);
    
    bool result = exceptionService->initialize();
    
    QVERIFY(!result);
    QCOMPARE(exceptionService->state(), IService::ServiceState::Error);
    QVERIFY(exceptionService->hasError());
    QVERIFY(exceptionService->lastError().contains("Exception during initialization"));
    QCOMPARE(errorSpy.count(), 1);
    
    delete exceptionService;
}

void TestBaseService::testShutdown()
{
    m_service->initialize();
    QVERIFY(m_service->isRunning());
    
    QSignalSpy stateChangedSpy(m_service, &IService::stateChanged);
    
    m_service->shutdown();
    
    QVERIFY(m_service->wasShutdownCalled());
    QCOMPARE(m_service->state(), IService::ServiceState::Stopped);
    QVERIFY(!m_service->isRunning());
    
    // Should have emitted: Stopping -> Stopped
    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(stateChangedSpy.at(0).at(0).value<IService::ServiceState>(), IService::ServiceState::Stopping);
    QCOMPARE(stateChangedSpy.at(1).at(0).value<IService::ServiceState>(), IService::ServiceState::Stopped);
}

void TestBaseService::testDoubleInitialization()
{
    bool result1 = m_service->initialize();
    QVERIFY(result1);
    QVERIFY(m_service->isRunning());
    
    // Try to initialize again
    bool result2 = m_service->initialize();
    QVERIFY(result2); // Should return true since it's already running
    QVERIFY(m_service->isRunning());
}

void TestBaseService::testShutdownWithoutInitialization()
{
    QCOMPARE(m_service->state(), IService::ServiceState::Uninitialized);
    
    // Should not crash or change state
    m_service->shutdown();
    QCOMPARE(m_service->state(), IService::ServiceState::Uninitialized);
}

void TestBaseService::testServiceName()
{
    QString name = m_service->serviceName();
    QCOMPARE(name, "TestableService");
}

void TestBaseService::testUptime()
{
    QCOMPARE(m_service->uptime(), -1); // Not initialized
    
    m_service->initialize();
    QTest::qWait(10); // Wait a bit
    
    qint64 uptime = m_service->uptime();
    QVERIFY(uptime >= 0);
    QVERIFY(uptime >= 10); // Should be at least 10ms
}

void TestBaseService::testErrorHandling()
{
    m_service->initialize();
    QVERIFY(!m_service->hasError());
    QVERIFY(m_service->lastError().isEmpty());
    
    QSignalSpy errorSpy(m_service, &IService::serviceError);
    
    // Simulate an error (this is a protected method, so we'd need to expose it for testing)
    // For now, we'll test through failed initialization
    TestableService* errorService = new TestableService(this);
    errorService->setInitializeResult(false);
    errorService->initialize();
    
    QVERIFY(errorService->hasError());
    QVERIFY(!errorService->lastError().isEmpty());
    
    delete errorService;
}

void TestBaseService::testSignalEmission()
{
    QSignalSpy stateChangedSpy(m_service, &IService::stateChanged);
    QSignalSpy errorSpy(m_service, &IService::serviceError);
    
    // Test successful initialization signals
    m_service->initialize();
    QCOMPARE(stateChangedSpy.count(), 2);
    QCOMPARE(errorSpy.count(), 0);
    
    stateChangedSpy.clear();
    
    // Test shutdown signals
    m_service->shutdown();
    QCOMPARE(stateChangedSpy.count(), 2);
}

void TestBaseService::testInitializationTimeout()
{
    // This test would require a service that takes longer than 30 seconds to initialize
    // For practical testing, we'll create a service with a shorter timeout
    // In a real implementation, you might want to make the timeout configurable for testing
    
    TestableService* slowService = new TestableService(this);
    slowService->setInitializeDelay(100); // 100ms delay, much less than 30s timeout
    
    bool result = slowService->initialize();
    QVERIFY(result); // Should succeed within timeout
    
    delete slowService;
}

QTEST_MAIN(TestBaseService)