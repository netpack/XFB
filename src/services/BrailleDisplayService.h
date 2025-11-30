#ifndef BRAILLEDISPLAYSERVICE_H
#define BRAILLEDISPLAYSERVICE_H

#include "BaseService.h"
#include "AccessibilitySettingsService.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QProcess>
#include <QMap>
#include <QThread>

/**
 * @brief Service for managing braille display support and configuration
 * 
 * The BrailleDisplayService provides comprehensive braille display support
 * including device detection, configuration management, and output formatting.
 * It integrates with the system's braille infrastructure and provides
 * optimized output for different braille display types.
 * 
 * @example
 * @code
 * auto* brailleService = ServiceContainer::instance()->resolve<BrailleDisplayService>();
 * brailleService->detectAvailableDevices();
 * brailleService->setActiveDevice("Braille Display 1");
 * brailleService->sendText("Hello World", BrailleDisplayService::BrailleFormat::Standard);
 * @endcode
 * 
 * @see AccessibilitySettingsService, BaseService
 * @since XFB 2.0
 */
class BrailleDisplayService : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Braille display device information
     */
    struct BrailleDevice {
        QString id;                 ///< Unique device identifier
        QString name;               ///< Human-readable device name
        QString manufacturer;       ///< Device manufacturer
        QString model;              ///< Device model
        QString driver;             ///< Driver name
        int cellCount;              ///< Number of braille cells
        bool isConnected;           ///< Connection status
        QString connectionType;     ///< Connection type (USB, Bluetooth, Serial)
        QString port;               ///< Connection port/address
        
        BrailleDevice()
            : cellCount(0)
            , isConnected(false) {}
            
        BrailleDevice(const QString& deviceId, const QString& deviceName)
            : id(deviceId)
            , name(deviceName)
            , cellCount(0)
            , isConnected(false) {}
    };

    /**
     * @brief Braille formatting options
     */
    enum class BrailleFormat {
        Standard,   ///< Standard braille formatting
        Compact,    ///< Compact format with abbreviations
        Detailed    ///< Detailed format with full descriptions
    };

    /**
     * @brief Braille output priority levels
     */
    enum class BraillePriority {
        Low,        ///< Background information
        Normal,     ///< Standard content
        High,       ///< Important notifications
        Critical    ///< Emergency alerts
    };

    /**
     * @brief Braille display capabilities
     */
    struct BrailleCapabilities {
        bool supportsRouting;       ///< Supports cursor routing keys
        bool supportsScrolling;     ///< Supports horizontal scrolling
        bool supportsStatusCells;   ///< Has dedicated status cells
        bool supportsVibration;     ///< Supports tactile feedback
        bool supportsAudio;         ///< Has audio feedback capability
        int maxCellCount;           ///< Maximum number of cells
        QStringList supportedTables; ///< Supported braille translation tables
        
        BrailleCapabilities()
            : supportsRouting(false)
            , supportsScrolling(false)
            , supportsStatusCells(false)
            , supportsVibration(false)
            , supportsAudio(false)
            , maxCellCount(0) {}
    };

    explicit BrailleDisplayService(QObject* parent = nullptr);
    ~BrailleDisplayService() override;

    /**
     * @brief Check if braille display support is available
     * @return true if braille support is available on the system
     */
    bool isBrailleSupported() const;

    /**
     * @brief Check if a braille display is currently connected
     * @return true if a braille display is connected and active
     */
    bool isBrailleDisplayConnected() const;

    /**
     * @brief Get the currently active braille device
     * @return Active braille device, or invalid device if none active
     */
    BrailleDevice activeDevice() const { return m_activeDevice; }

    /**
     * @brief Get list of available braille devices
     * @return List of detected braille devices
     */
    QList<BrailleDevice> availableDevices() const { return m_availableDevices; }

    /**
     * @brief Get braille capabilities for a device
     * @param deviceId Device identifier
     * @return Device capabilities, or default capabilities if device not found
     */
    BrailleCapabilities deviceCapabilities(const QString& deviceId) const;

    /**
     * @brief Set the active braille device
     * @param deviceId Device identifier to activate
     * @return true if device was activated successfully
     */
    bool setActiveDevice(const QString& deviceId);

    /**
     * @brief Send text to the braille display
     * @param text Text to display
     * @param format Formatting option
     * @param priority Output priority
     * @return true if text was sent successfully
     */
    bool sendText(const QString& text, BrailleFormat format = BrailleFormat::Standard, 
                  BraillePriority priority = BraillePriority::Normal);

    /**
     * @brief Clear the braille display
     * @return true if display was cleared successfully
     */
    bool clearDisplay();

    /**
     * @brief Set cursor position on braille display
     * @param position Cursor position (0-based)
     * @return true if cursor was positioned successfully
     */
    bool setCursorPosition(int position);

    /**
     * @brief Get current cursor position
     * @return Current cursor position, or -1 if not available
     */
    int cursorPosition() const { return m_cursorPosition; }

    /**
     * @brief Start automatic device detection
     * @return true if detection was started successfully
     */
    bool detectAvailableDevices();

    /**
     * @brief Stop device detection
     */
    void stopDeviceDetection();

    /**
     * @brief Check if device detection is in progress
     * @return true if detection is running
     */
    bool isDetectionInProgress() const { return m_detectionInProgress; }

    /**
     * @brief Format text according to braille formatting rules
     * @param text Input text
     * @param format Formatting option
     * @param maxLength Maximum length for the output
     * @return Formatted text suitable for braille display
     */
    QString formatTextForBraille(const QString& text, BrailleFormat format, int maxLength = -1) const;

    /**
     * @brief Get braille translation table names
     * @return List of available braille translation tables
     */
    QStringList availableTranslationTables() const;

    /**
     * @brief Set the braille translation table
     * @param tableName Translation table name
     * @return true if table was set successfully
     */
    bool setTranslationTable(const QString& tableName);

    /**
     * @brief Get current translation table
     * @return Current translation table name
     */
    QString currentTranslationTable() const { return m_currentTranslationTable; }

    /**
     * @brief Enable or disable braille display output
     * @param enabled true to enable, false to disable
     */
    void setBrailleEnabled(bool enabled);

    /**
     * @brief Check if braille output is enabled
     * @return true if braille output is enabled
     */
    bool isBrailleEnabled() const { return m_brailleEnabled; }

    /**
     * @brief Set braille output format
     * @param format New braille format
     */
    void setBrailleFormat(BrailleFormat format);

    /**
     * @brief Get current braille format
     * @return Current braille format
     */
    BrailleFormat brailleFormat() const { return m_brailleFormat; }

    /**
     * @brief Test braille display with sample text
     * @param deviceId Device to test (empty for active device)
     * @return true if test was successful
     */
    bool testBrailleDisplay(const QString& deviceId = QString());

    /**
     * @brief Get braille display status information
     * @return Status information string
     */
    QString getStatusInfo() const;

public slots:
    /**
     * @brief Handle settings changes from AccessibilitySettingsService
     * @param enabled Braille enabled state
     * @param format Braille format
     */
    void onBrailleSettingsChanged(bool enabled, AccessibilitySettingsService::BrailleFormat format);

    /**
     * @brief Handle device connection events
     */
    void onDeviceConnected();

    /**
     * @brief Handle device disconnection events
     */
    void onDeviceDisconnected();

signals:
    /**
     * @brief Emitted when device detection starts
     */
    void deviceDetectionStarted();

    /**
     * @brief Emitted when device detection completes
     * @param devicesFound Number of devices found
     */
    void deviceDetectionCompleted(int devicesFound);

    /**
     * @brief Emitted when a new device is detected
     * @param device Detected device information
     */
    void deviceDetected(const BrailleDevice& device);

    /**
     * @brief Emitted when a device is connected
     * @param deviceId Connected device identifier
     */
    void deviceConnected(const QString& deviceId);

    /**
     * @brief Emitted when a device is disconnected
     * @param deviceId Disconnected device identifier
     */
    void deviceDisconnected(const QString& deviceId);

    /**
     * @brief Emitted when the active device changes
     * @param deviceId New active device identifier
     */
    void activeDeviceChanged(const QString& deviceId);

    /**
     * @brief Emitted when braille output is sent
     * @param text Text that was sent
     * @param success Whether the output was successful
     */
    void brailleOutputSent(const QString& text, bool success);

    /**
     * @brief Emitted when braille settings change
     * @param enabled New enabled state
     * @param format New format
     */
    void brailleSettingsChanged(bool enabled, BrailleFormat format);

    /**
     * @brief Emitted when an error occurs
     * @param error Error message
     */
    void brailleError(const QString& error);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Handle device detection timeout
     */
    void onDetectionTimeout();

    /**
     * @brief Handle device detection process completion
     */
    void onDetectionProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * @brief Handle periodic device monitoring
     */
    void onDeviceMonitorTimeout();

private:
    /**
     * @brief Initialize braille system integration
     * @return true if initialization was successful
     */
    bool initializeBrailleSystem();

    /**
     * @brief Cleanup braille system resources
     */
    void cleanupBrailleSystem();

    /**
     * @brief Detect devices using system tools
     * @return List of detected devices
     */
    QList<BrailleDevice> detectSystemDevices();

    /**
     * @brief Detect devices using BRLTTY
     * @return List of detected devices
     */
    QList<BrailleDevice> detectBRLTTYDevices();

    /**
     * @brief Parse BRLTTY device information
     * @param output BRLTTY command output
     * @return Parsed device information
     */
    QList<BrailleDevice> parseBRLTTYOutput(const QString& output);

    /**
     * @brief Connect to a braille device
     * @param device Device to connect to
     * @return true if connection was successful
     */
    bool connectToDevice(const BrailleDevice& device);

    /**
     * @brief Disconnect from current device
     */
    void disconnectFromDevice();

    /**
     * @brief Send raw data to braille display
     * @param data Data to send
     * @return true if data was sent successfully
     */
    bool sendRawData(const QByteArray& data);

    /**
     * @brief Convert AccessibilitySettingsService format to internal format
     * @param format Settings service format
     * @return Internal braille format
     */
    BrailleFormat convertSettingsFormat(AccessibilitySettingsService::BrailleFormat format) const;

    /**
     * @brief Apply text formatting based on braille format
     * @param text Input text
     * @param format Formatting option
     * @param maxLength Maximum output length
     * @return Formatted text
     */
    QString applyBrailleFormatting(const QString& text, BrailleFormat format, int maxLength) const;

    /**
     * @brief Abbreviate text for compact format
     * @param text Input text
     * @param maxLength Maximum length
     * @return Abbreviated text
     */
    QString abbreviateText(const QString& text, int maxLength) const;

    /**
     * @brief Expand text for detailed format
     * @param text Input text
     * @return Expanded text with additional context
     */
    QString expandText(const QString& text) const;

    /**
     * @brief Check if BRLTTY is available on the system
     * @return true if BRLTTY is available
     */
    bool isBRLTTYAvailable() const;

    /**
     * @brief Get system braille configuration
     * @return Configuration information
     */
    QMap<QString, QString> getSystemBrailleConfig() const;

    /**
     * @brief Update device capabilities cache
     * @param device Device to update capabilities for
     */
    void updateDeviceCapabilities(const BrailleDevice& device);

    // Member variables
    bool m_brailleEnabled;
    BrailleFormat m_brailleFormat;
    BrailleDevice m_activeDevice;
    QList<BrailleDevice> m_availableDevices;
    QMap<QString, BrailleCapabilities> m_deviceCapabilities;
    
    // Detection and monitoring
    bool m_detectionInProgress;
    QTimer* m_detectionTimer;
    QTimer* m_deviceMonitorTimer;
    QProcess* m_detectionProcess;
    
    // Braille system integration
    QString m_currentTranslationTable;
    int m_cursorPosition;
    QString m_lastOutputText;
    
    // Configuration
    static constexpr int DETECTION_TIMEOUT_MS = 10000;  // 10 seconds
    static constexpr int DEVICE_MONITOR_INTERVAL_MS = 5000;  // 5 seconds
    static constexpr int DEFAULT_CELL_COUNT = 40;
    static constexpr const char* DEFAULT_TRANSLATION_TABLE = "en-us-g2.ctb";
};

Q_DECLARE_METATYPE(BrailleDisplayService::BrailleDevice)
Q_DECLARE_METATYPE(BrailleDisplayService::BrailleFormat)
Q_DECLARE_METATYPE(BrailleDisplayService::BraillePriority)

#endif // BRAILLEDISPLAYSERVICE_H