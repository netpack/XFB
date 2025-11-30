#include "BaseService.h"
#include <QDebug>
#include <QDateTime>

BaseService::BaseService(QObject* parent)
    : IService(parent)
    , m_initializationTimer(new QTimer(this))
{
    m_initializationTimer->setSingleShot(true);
    m_initializationTimer->setInterval(INITIALIZATION_TIMEOUT_MS);
    connect(m_initializationTimer, &QTimer::timeout, this, &BaseService::onInitializationTimeout);
}

BaseService::~BaseService()
{
    // Don't call virtual methods in destructor
    if (state() == ServiceState::Running) {
        setState(ServiceState::Stopping);
        // Call doShutdown directly instead of shutdown() to avoid virtual calls
        try {
            doShutdown();
            setState(ServiceState::Stopped);
        } catch (...) {
            // Ignore exceptions in destructor
        }
    }
}

bool BaseService::initialize()
{
    if (state() != ServiceState::Uninitialized) {
        logWarning("Service is already initialized or in process");
        return state() == ServiceState::Running;
    }

    logDebug("Starting initialization...");
    setState(ServiceState::Initializing);
    
    // Start initialization timeout timer
    m_initializationTimer->start();
    
    try {
        if (doInitialize()) {
            m_initializationTimer->stop();
            m_initializationTime = QDateTime::currentDateTime();
            m_uptimeTimer.start();
            setState(ServiceState::Running);
            clearError();
            logDebug("Initialization completed successfully");
            return true;
        } else {
            m_initializationTimer->stop();
            setError("Service initialization failed");
            logError("Initialization failed");
            return false;
        }
    } catch (const std::exception& e) {
        m_initializationTimer->stop();
        setError(QString("Exception during initialization: %1").arg(e.what()));
        logError(QString("Exception during initialization: %1").arg(e.what()));
        return false;
    } catch (...) {
        m_initializationTimer->stop();
        setError("Unknown exception during initialization");
        logError("Unknown exception during initialization");
        return false;
    }
}

void BaseService::shutdown()
{
    if (state() == ServiceState::Stopped || state() == ServiceState::Uninitialized) {
        return;
    }

    logDebug("Starting shutdown...");
    setState(ServiceState::Stopping);
    
    m_initializationTimer->stop();
    
    try {
        doShutdown();
        setState(ServiceState::Stopped);
        logDebug("Shutdown completed successfully");
    } catch (const std::exception& e) {
        setError(QString("Exception during shutdown: %1").arg(e.what()));
        logError(QString("Exception during shutdown: %1").arg(e.what()));
    } catch (...) {
        setError("Unknown exception during shutdown");
        logError("Unknown exception during shutdown");
    }
}

QString BaseService::serviceName() const
{
    return getServiceName();
}

qint64 BaseService::uptime() const
{
    if (!m_initializationTime.isValid() || !m_uptimeTimer.isValid()) {
        return -1;
    }
    return m_uptimeTimer.elapsed();
}

void BaseService::setError(const QString& error)
{
    m_lastError = error;
    setState(ServiceState::Error);
    emit serviceError(error);
    logError(error);
}

void BaseService::clearError()
{
    m_lastError.clear();
}

void BaseService::logDebug(const QString& message) const
{
    qDebug() << "[" << getServiceName() << "]" << message;
}

void BaseService::logWarning(const QString& message) const
{
    qWarning() << "[" << getServiceName() << "]" << message;
}

void BaseService::logError(const QString& message) const
{
    qCritical() << "[" << getServiceName() << "]" << message;
}

void BaseService::onInitializationTimeout()
{
    if (state() == ServiceState::Initializing) {
        setError("Service initialization timed out");
        logError("Initialization timed out after 30 seconds");
    }
}