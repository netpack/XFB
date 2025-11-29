#ifndef TESTSERVICECONTAINER_H
#define TESTSERVICECONTAINER_H

#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include "../../../src/services/ServiceContainer.h"
#include "../../../src/services/BaseService.h"

/**
 * @brief Mock service for testing purposes
 */
class MockService : public BaseService
{
    Q_OBJECT

public:
    explicit MockService(QObject* parent = nullptr) : BaseService(parent), m_initializeResult(true) {}
    
    void setInitializeResult(bool result) { m_initializeResult = result; }
    bool wasInitializeCalled() const { return m_initializeCalled; }
    bool wasShutdownCalled() const { return m_shutdownCalled; }

protected:
    bool doInitialize() override {
        m_initializeCalled = true;
        return m_initializeResult;
    }
    
    void doShutdown() override {
        m_shutdownCalled = true;
    }
    
    QString getServiceName() const override {
        return "MockService";
    }

private:
    bool m_initializeResult;
    bool m_initializeCalled = false;
    bool m_shutdownCalled = false;
};

/**
 * @brief Another mock service for dependency testing
 */
class AnotherMockService : public BaseService
{
    Q_OBJECT

public:
    explicit AnotherMockService(QObject* parent = nullptr) : BaseService(parent) {}

protected:
    QString getServiceName() const override {
        return "AnotherMockService";
    }
};

/**
 * @brief Unit tests for ServiceContainer
 */
class TestServiceContainer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test cases
    void testSingletonInstance();
    void testServiceRegistration();
    void testSingletonServiceResolution();
    void testTransientServiceResolution();
    void testServiceInitialization();
    void testServiceShutdown();
    void testServiceNotFound();
    void testDuplicateRegistration();
    void testServiceCount();
    void testClear();
    void testCustomFactory();
    void testSignalEmission();

private:
    ServiceContainer* m_container;
};

#endif // TESTSERVICECONTAINER_H