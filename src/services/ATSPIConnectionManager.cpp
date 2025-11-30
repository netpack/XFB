#include "ATSPIConnectionManager.h"
#include <QAccessible>
#include <QApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QWidget>

ATSPIConnectionManager::ATSPIConnectionManager(QObject *parent)
    : QObject(parent)
    , m_connectionState(Disconnected)
    , m_healthCheckTimer(new QTimer(this))
    , m_recoveryTimer(new QTimer(this))
    , m_bridgeProcess(nullptr)
    , m_autoRecoveryEnabled(true)
    , m_atSpiAvailable(false)
    , m_recoveryAttempts(0)
{
    // Set up timers
    m_healthCheckTimer->setInterval(HEALTH_CHECK_INTERVAL);
    m_healthCheckTimer->setSingleShot(false);
    
    m_recoveryTimer->setInterval(RECOVERY_DELAY);
    m_recoveryTimer->setSingleShot(true);
    
    connect(m_healthCheckTimer, &QTimer::timeout, this, &ATSPIConnectionManager::checkConnectionStatus);
    connect(m_recoveryTimer, &QTimer::timeout, this, &ATSPIConnectionManager::attemptRecovery);
}

ATSPIConnectionManager::~ATSPIConnectionManager()
{
    if (m_bridgeProcess) {
        stopATSPIBridge();
    }
}

void ATSPIConnectionManager::initialize()
{
    qDebug() << "Initializing AT-SPI Connection Manager";
    
    // Check if AT-SPI is available on the system
    m_atSpiAvailable = isATSPIAvailable();
    
    if (!m_atSpiAvailable) {
        qWarning() << "AT-SPI is not available on this system";
        updateConnectionState(Failed);
        return;
    }
    
    // Set up environment for AT-SPI
    setupEnvironmentForATSPI();
    
    // Start health check timer
    m_healthCheckTimer->start();
    
    // Attempt initial connection
    connectToATSPI();
    
    qDebug() << "AT-SPI Connection Manager initialized";
}

ATSPIConnectionManager::ConnectionState ATSPIConnectionManager::getConnectionState() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_connectionState;
}

bool ATSPIConnectionManager::isATSPIAvailable() const
{
    // Check for AT-SPI bridge executable
    QString bridgePath = getATSPIBridgePath();
    if (bridgePath.isEmpty()) {
        return false;
    }
    
    // Check if the bridge executable exists and is executable
    QFileInfo bridgeInfo(bridgePath);
    return bridgeInfo.exists() && bridgeInfo.isExecutable();
}

bool ATSPIConnectionManager::connectToATSPI()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (m_connectionState == Connecting || m_connectionState == Connected) {
        return true; // Already connecting or connected
    }
    
    if (!m_atSpiAvailable) {
        qWarning() << "Cannot connect to AT-SPI: not available";
        return false;
    }
    
    qDebug() << "Attempting to connect to AT-SPI";
    updateConnectionState(Connecting);
    
    // Start AT-SPI bridge if not running
    if (!isATSPIBridgeRunning()) {
        if (!startATSPIBridge()) {
            qWarning() << "Failed to start AT-SPI bridge";
            updateConnectionState(Failed);
            return false;
        }
    }
    
    // Test connection
    QTimer::singleShot(2000, this, [this]() {
        if (testATSPIConnection()) {
            updateConnectionState(Connected);
            emit connected();
            qDebug() << "Successfully connected to AT-SPI";
        } else {
            qWarning() << "AT-SPI connection test failed";
            updateConnectionState(Degraded);
        }
    });
    
    return true;
}

void ATSPIConnectionManager::disconnectFromATSPI()
{
    QMutexLocker locker(&m_stateMutex);
    
    qDebug() << "Disconnecting from AT-SPI";
    
    // Stop health check
    m_healthCheckTimer->stop();
    
    // Stop AT-SPI bridge if we started it
    if (m_bridgeProcess) {
        stopATSPIBridge();
    }
    
    // Disable Qt accessibility
    QAccessible::setActive(false);
    
    updateConnectionState(Disconnected);
    emit disconnected();
}

bool ATSPIConnectionManager::reconnectToATSPI()
{
    qDebug() << "Reconnecting to AT-SPI";
    
    // Disconnect first
    disconnectFromATSPI();
    
    // Wait a moment
    QThread::msleep(1000);
    
    // Reconnect
    return connectToATSPI();
}

void ATSPIConnectionManager::setAutoRecoveryEnabled(bool enabled)
{
    m_autoRecoveryEnabled = enabled;
    qDebug() << "AT-SPI auto-recovery" << (enabled ? "enabled" : "disabled");
}

bool ATSPIConnectionManager::isAutoRecoveryEnabled() const
{
    return m_autoRecoveryEnabled;
}

QString ATSPIConnectionManager::getConnectionStatusInfo() const
{
    QMutexLocker locker(&m_stateMutex);
    
    QString status;
    
    switch (m_connectionState) {
    case Disconnected:
        status = "Disconnected from AT-SPI";
        break;
    case Connecting:
        status = "Connecting to AT-SPI...";
        break;
    case Connected:
        status = "Connected to AT-SPI";
        break;
    case Degraded:
        status = "AT-SPI connection degraded";
        break;
    case Failed:
        status = "AT-SPI connection failed";
        break;
    }
    
    if (m_bridgeProcess && m_bridgeProcess->state() == QProcess::Running) {
        status += " (Bridge running)";
    }
    
    if (!m_atSpiAvailable) {
        status += " (AT-SPI not available)";
    }
    
    return status;
}

void ATSPIConnectionManager::performHealthCheck()
{
    checkConnectionStatus();
}

void ATSPIConnectionManager::attemptRecovery()
{
    QMutexLocker locker(&m_stateMutex);
    
    if (!m_autoRecoveryEnabled) {
        return;
    }
    
    // Check recovery cooldown
    QDateTime now = QDateTime::currentDateTime();
    if (m_lastRecoveryAttempt.isValid() && 
        m_lastRecoveryAttempt.msecsTo(now) < RECOVERY_COOLDOWN) {
        return;
    }
    
    // Check maximum recovery attempts
    if (m_recoveryAttempts >= MAX_RECOVERY_ATTEMPTS) {
        qWarning() << "Maximum AT-SPI recovery attempts reached";
        updateConnectionState(Failed);
        return;
    }
    
    qDebug() << "Attempting AT-SPI recovery (attempt" << (m_recoveryAttempts + 1) << ")";
    
    m_recoveryAttempts++;
    m_lastRecoveryAttempt = now;
    
    bool success = false;
    
    // Try different recovery strategies
    if (m_recoveryAttempts == 1) {
        // First attempt: restart bridge
        success = reconnectToATSPI();
    } else if (m_recoveryAttempts == 2) {
        // Second attempt: kill all AT-SPI processes and restart
        QProcess::execute("pkill", QStringList() << "-f" << "at-spi");
        QThread::msleep(2000);
        success = connectToATSPI();
    } else {
        // Final attempt: enable degraded mode
        QAccessible::setActive(true);
        updateConnectionState(Degraded);
        success = true;
    }
    
    emit recoveryAttempted(success);
    
    if (success && m_connectionState == Connected) {
        // Reset recovery attempts on successful connection
        m_recoveryAttempts = 0;
    }
}

void ATSPIConnectionManager::checkConnectionStatus()
{
    bool wasConnected = (m_connectionState == Connected);
    bool isConnected = testATSPIConnection();
    
    if (wasConnected && !isConnected) {
        qWarning() << "AT-SPI connection lost";
        updateConnectionState(Disconnected);
        emit disconnected();
        
        // Trigger recovery if enabled
        if (m_autoRecoveryEnabled) {
            m_recoveryTimer->start();
        }
    } else if (!wasConnected && isConnected) {
        qDebug() << "AT-SPI connection restored";
        updateConnectionState(Connected);
        emit connected();
        m_recoveryAttempts = 0; // Reset recovery attempts
    }
}

void ATSPIConnectionManager::onBridgeProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qWarning() << "AT-SPI bridge process finished with exit code" << exitCode 
               << "and status" << exitStatus;
    
    if (m_connectionState == Connected || m_connectionState == Connecting) {
        updateConnectionState(Disconnected);
        emit disconnected();
        
        // Trigger recovery if enabled
        if (m_autoRecoveryEnabled) {
            m_recoveryTimer->start();
        }
    }
}

void ATSPIConnectionManager::onBridgeProcessError(QProcess::ProcessError error)
{
    qWarning() << "AT-SPI bridge process error:" << error;
    
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = "Failed to start AT-SPI bridge";
        break;
    case QProcess::Crashed:
        errorMessage = "AT-SPI bridge crashed";
        break;
    case QProcess::Timedout:
        errorMessage = "AT-SPI bridge timed out";
        break;
    case QProcess::WriteError:
        errorMessage = "AT-SPI bridge write error";
        break;
    case QProcess::ReadError:
        errorMessage = "AT-SPI bridge read error";
        break;
    case QProcess::UnknownError:
        errorMessage = "Unknown AT-SPI bridge error";
        break;
    }
    
    emit connectionError(errorMessage);
    
    updateConnectionState(Failed);
    
    // Trigger recovery if enabled
    if (m_autoRecoveryEnabled) {
        m_recoveryTimer->start();
    }
}

bool ATSPIConnectionManager::startATSPIBridge()
{
    if (m_bridgeProcess) {
        stopATSPIBridge();
    }
    
    QString bridgePath = getATSPIBridgePath();
    if (bridgePath.isEmpty()) {
        qWarning() << "AT-SPI bridge executable not found";
        return false;
    }
    
    qDebug() << "Starting AT-SPI bridge:" << bridgePath;
    
    m_bridgeProcess = new QProcess(this);
    
    connect(m_bridgeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ATSPIConnectionManager::onBridgeProcessFinished);
    connect(m_bridgeProcess, &QProcess::errorOccurred,
            this, &ATSPIConnectionManager::onBridgeProcessError);
    
    // Set up environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("AT_SPI_BUS", "1");
    m_bridgeProcess->setProcessEnvironment(env);
    
    // Start the bridge
    QStringList arguments = getATSPIBridgeArguments();
    m_bridgeProcess->start(bridgePath, arguments);
    
    if (!m_bridgeProcess->waitForStarted(5000)) {
        qWarning() << "Failed to start AT-SPI bridge process";
        delete m_bridgeProcess;
        m_bridgeProcess = nullptr;
        return false;
    }
    
    qDebug() << "AT-SPI bridge started successfully";
    return true;
}

bool ATSPIConnectionManager::stopATSPIBridge()
{
    if (!m_bridgeProcess) {
        return true;
    }
    
    qDebug() << "Stopping AT-SPI bridge";
    
    // Disconnect signals to avoid recursive calls
    disconnect(m_bridgeProcess, nullptr, this, nullptr);
    
    // Terminate the process
    m_bridgeProcess->terminate();
    
    if (!m_bridgeProcess->waitForFinished(3000)) {
        qWarning() << "AT-SPI bridge did not terminate gracefully, killing";
        m_bridgeProcess->kill();
        m_bridgeProcess->waitForFinished(1000);
    }
    
    delete m_bridgeProcess;
    m_bridgeProcess = nullptr;
    
    return true;
}

bool ATSPIConnectionManager::isATSPIBridgeRunning() const
{
    // Check if our process is running
    if (m_bridgeProcess && m_bridgeProcess->state() == QProcess::Running) {
        return true;
    }
    
    // Check if any AT-SPI bridge is running system-wide
    QProcess process;
    process.start("pgrep", QStringList() << "-f" << "at-spi-bus-launcher");
    process.waitForFinished(3000);
    
    return process.exitCode() == 0;
}

bool ATSPIConnectionManager::testATSPIConnection() const
{
    // Test if QAccessible is active and working
    if (!QAccessible::isActive()) {
        return false;
    }
    
    // Try to create a simple accessible interface
    QWidget* testWidget = qApp->activeWindow();
    if (!testWidget) {
        // Create a temporary widget for testing
        testWidget = new QWidget();
        testWidget->hide();
    }
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(testWidget);
    bool hasInterface = (interface != nullptr);
    
    // Clean up temporary widget if we created one
    if (testWidget != qApp->activeWindow()) {
        delete testWidget;
    }
    
    return hasInterface;
}

void ATSPIConnectionManager::updateConnectionState(ConnectionState newState)
{
    if (m_connectionState != newState) {
        ConnectionState oldState = m_connectionState;
        m_connectionState = newState;
        
        qDebug() << "AT-SPI connection state changed from" << oldState << "to" << newState;
        emit connectionStateChanged(newState);
    }
}

QString ATSPIConnectionManager::getATSPIBridgePath() const
{
    // Common locations for AT-SPI bridge
    QStringList possiblePaths = {
        "/usr/libexec/at-spi-bus-launcher",
        "/usr/lib/at-spi2-core/at-spi-bus-launcher",
        "/usr/lib/x86_64-linux-gnu/at-spi2-core/at-spi-bus-launcher",
        "/usr/lib/aarch64-linux-gnu/at-spi2-core/at-spi-bus-launcher"
    };
    
    for (const QString& path : possiblePaths) {
        QFileInfo info(path);
        if (info.exists() && info.isExecutable()) {
            return path;
        }
    }
    
    // Try to find it in PATH
    QProcess process;
    process.start("which", QStringList() << "at-spi-bus-launcher");
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0) {
        QString path = process.readAllStandardOutput().trimmed();
        if (!path.isEmpty()) {
            return path;
        }
    }
    
    return QString();
}

QStringList ATSPIConnectionManager::getATSPIBridgeArguments() const
{
    // Most AT-SPI bridges don't need special arguments
    return QStringList();
}

void ATSPIConnectionManager::setupEnvironmentForATSPI()
{
    // Set up environment variables for AT-SPI
    qputenv("AT_SPI_BUS", "1");
    qputenv("QT_ACCESSIBILITY", "1");
    
    // Enable Qt accessibility
    QAccessible::setActive(true);
    
    qDebug() << "Environment set up for AT-SPI";
}