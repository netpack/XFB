#ifndef ATSPICONNECTIONMANAGER_H
#define ATSPICONNECTIONMANAGER_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QMutex>
#include <QDateTime>
#include <QWidget>

/**
 * @brief Manages AT-SPI connection and recovery
 * 
 * Handles AT-SPI bridge connection, monitoring, and automatic recovery
 * when connection is lost or fails.
 */
class ATSPIConnectionManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Connection states
     */
    enum ConnectionState {
        Disconnected,   ///< No AT-SPI connection
        Connecting,     ///< Attempting to connect
        Connected,      ///< Successfully connected
        Degraded,       ///< Connected but with limited functionality
        Failed          ///< Connection failed and recovery not possible
    };

    explicit ATSPIConnectionManager(QObject *parent = nullptr);
    ~ATSPIConnectionManager();

    /**
     * @brief Initialize AT-SPI connection manager
     */
    void initialize();

    /**
     * @brief Get current connection state
     * @return Current connection state
     */
    ConnectionState getConnectionState() const;

    /**
     * @brief Check if AT-SPI is available on the system
     * @return True if AT-SPI is available
     */
    bool isATSPIAvailable() const;

    /**
     * @brief Attempt to establish AT-SPI connection
     * @return True if connection attempt was started
     */
    bool connectToATSPI();

    /**
     * @brief Disconnect from AT-SPI
     */
    void disconnectFromATSPI();

    /**
     * @brief Force reconnection to AT-SPI
     * @return True if reconnection attempt was started
     */
    bool reconnectToATSPI();

    /**
     * @brief Enable or disable automatic recovery
     * @param enabled Whether to enable automatic recovery
     */
    void setAutoRecoveryEnabled(bool enabled);

    /**
     * @brief Check if automatic recovery is enabled
     * @return True if auto-recovery is enabled
     */
    bool isAutoRecoveryEnabled() const;

    /**
     * @brief Get connection status information
     * @return Status information string
     */
    QString getConnectionStatusInfo() const;

public slots:
    /**
     * @brief Perform connection health check
     */
    void performHealthCheck();

    /**
     * @brief Attempt recovery if connection is lost
     */
    void attemptRecovery();

signals:
    /**
     * @brief Emitted when connection state changes
     * @param state New connection state
     */
    void connectionStateChanged(ConnectionState state);

    /**
     * @brief Emitted when AT-SPI connection is established
     */
    void connected();

    /**
     * @brief Emitted when AT-SPI connection is lost
     */
    void disconnected();

    /**
     * @brief Emitted when connection recovery is attempted
     * @param successful Whether recovery was successful
     */
    void recoveryAttempted(bool successful);

    /**
     * @brief Emitted when connection error occurs
     * @param error Error message
     */
    void connectionError(const QString& error);

private slots:
    void checkConnectionStatus();
    void onBridgeProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onBridgeProcessError(QProcess::ProcessError error);

private:
    // Helper methods
    bool startATSPIBridge();
    bool stopATSPIBridge();
    bool isATSPIBridgeRunning() const;
    bool testATSPIConnection() const;
    void updateConnectionState(ConnectionState newState);
    QString getATSPIBridgePath() const;
    QStringList getATSPIBridgeArguments() const;
    void setupEnvironmentForATSPI();

    // Member variables
    ConnectionState m_connectionState;
    QTimer* m_healthCheckTimer;
    QTimer* m_recoveryTimer;
    QProcess* m_bridgeProcess;
    mutable QMutex m_stateMutex;
    
    bool m_autoRecoveryEnabled;
    bool m_atSpiAvailable;
    int m_recoveryAttempts;
    QDateTime m_lastRecoveryAttempt;
    
    // Configuration
    static const int HEALTH_CHECK_INTERVAL = 10000; // 10 seconds
    static const int RECOVERY_DELAY = 5000; // 5 seconds
    static const int MAX_RECOVERY_ATTEMPTS = 3;
    static const int RECOVERY_COOLDOWN = 60000; // 1 minute
};

Q_DECLARE_METATYPE(ATSPIConnectionManager::ConnectionState)

#endif // ATSPICONNECTIONMANAGER_H