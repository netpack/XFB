#ifndef TESTBASESERVICE_H
#define TESTBASESERVICE_H

#include <QtTest/QtTest>
#include <QObject>
#include <QSignalSpy>
#include "../../../src/services/BaseService.h"

/**
 * @brief Concrete implementation of BaseService for testing
 */
class TestableService : public BaseService
{
    Q_OBJECT

public:
    explicit TestableService(QObject* parent = nullptr) 
        : BaseService(parent), m_initializeResult(true), m_initializeDelay(0) {}
    
    void setInitializeResult(bool result) { m_initializeResult = result; }
    void setInitializeDelay(int delayMs) { m_initializeDelay = delayMs; }
    bool wasInitializeCalled() const { return m_initializeCalled; }
    bool wasShutdownCalled() const { return m_shutdownCalled; }

protected:
    bool doInitialize() override {
        m_initializeCalled = true;
        if (m_initializeDelay > 0) {
            QTest::qWait(m_initializeDelay);
        }
        return m_initializeResult;
    }
    
    void doShutdown() override {
        m_shutdownCalled = true;
    }
    
    QString getServiceName() const override {
        return "TestableService";
    }

private:
    bool m_initializeResult;
    int m_initializeDelay;
    bool m_initializeCalled = false;
    bool m_shutdownCalled = false;
};

/**
 * @brief Service that throws exception during initialization
 */
class ExceptionService : public BaseService
{
    Q_OBJECT

public:
    explicit ExceptionService(QObject* parent = nullptr) : BaseService(parent) {}

protected:
    bool doInitialize() override {
        throw std::runtime_error("Test exception");
    }
    
    QString getServiceName() const override {
        return "ExceptionService";
    }
};

/**
 * @brief Unit tests for BaseService
 */
class TestBaseService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    // Test cases
    void testInitialState();
    void testSuccessfulInitialization();
    void testFailedInitialization();
    void testExceptionDuringInitialization();
    void testShutdown();
    void testDoubleInitialization();
    void testShutdownWithoutInitialization();
    void testServiceName();
    void testUptime();
    void testErrorHandling();
    void testSignalEmission();
    void testInitializationTimeout();

private:
    TestableService* m_service;
};

#endif // TESTBASESERVICE_H