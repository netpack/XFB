#include "AccessibilityManager.h"
#include "WidgetAccessibilityEnhancer.h"
// Temporarily disabled for beta build:
// #include "KeyboardNavigationController.h"
// #include "PlayerKeyboardNavigationEnhancer.h"
// #include "DatabaseGridKeyboardNavigationEnhancer.h"
#include "AudioFeedbackService.h"
#include "PlayerAudioFeedbackIntegration.h"
#include "BackgroundOperationFeedback.h"
#include "LiveRegionManager.h"
#include "PlaybackStatusAnnouncer.h"
#include "SystemStatusAnnouncer.h"
// Temporarily disabled for beta build:
// #include "AccessibleHelpSystem.h"
// #include "ContextSensitiveHelpService.h"
#include "ServiceContainer.h"
#include "Logger.h"
#include <QApplication>
#include <QAccessible>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>

AccessibilityManager::AccessibilityManager(QObject* parent)
    : BaseService(parent)
    , m_accessibilityEnabled(false)
    , m_verbosityLevel(VerbosityLevel::Normal)
    , m_widgetEnhancer(nullptr)
    // Temporarily disabled for beta build:
    // , m_keyboardNavigationController(nullptr)
    // , m_playerKeyboardEnhancer(nullptr)
    // , m_databaseGridKeyboardEnhancer(nullptr)
    , m_audioFeedbackService(nullptr)
    , m_playerAudioFeedbackIntegration(nullptr)
    , m_backgroundOperationFeedback(nullptr)
    , m_liveRegionManager(nullptr)
    , m_playbackStatusAnnouncer(nullptr)
    , m_systemStatusAnnouncer(nullptr)
    , m_accessibleHelpSystem(nullptr)
    , m_contextSensitiveHelpService(nullptr)
    , m_settings(nullptr)
    , m_announcementTimer(new QTimer(this))
    , m_processingAnnouncements(false)
{
    // Initialize settings
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir configDir(configPath);
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    m_settings = new QSettings(configDir.absoluteFilePath("xfb_accessibility.ini"), 
                              QSettings::IniFormat, this);
    
    // Setup announcement timer
    m_announcementTimer->setSingleShot(false);
    m_announcementTimer->setInterval(ANNOUNCEMENT_PROCESSING_INTERVAL_MS);
    connect(m_announcementTimer, &QTimer::timeout,
            this, &AccessibilityManager::processAnnouncementQueue);
    
    // Connect to application focus changes
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        connect(app, &QApplication::focusChanged,
                this, &AccessibilityManager::onFocusChanged);
        connect(app, &QApplication::applicationStateChanged,
                this, &AccessibilityManager::onApplicationStateChanged);
    }
    
    logDebug("AccessibilityManager created");
}

AccessibilityManager::~AccessibilityManager()
{
    cleanupAccessibility();
    logDebug("AccessibilityManager destroyed");
}

bool AccessibilityManager::doInitialize()
{
    logDebug("Initializing AccessibilityManager");
    
    // Load settings first
    loadSettings();
    
    // Initialize Qt accessibility framework
    if (!initializeQtAccessibility()) {
        setError("Failed to initialize Qt accessibility framework");
        return false;
    }
    
    // Initialize AT-SPI bridge if available
    if (!initializeATSPIBridge()) {
        logWarning("AT-SPI bridge initialization failed - accessibility may be limited");
        // Don't fail initialization as basic Qt accessibility might still work
    }
    
    // Initialize widget enhancer
    m_widgetEnhancer = new WidgetAccessibilityEnhancer(this, this);
    if (!m_widgetEnhancer->initialize()) {
        setError("Failed to initialize WidgetAccessibilityEnhancer");
        return false;
    }
    
    // Keyboard navigation controller temporarily disabled for beta build
    // m_keyboardNavigationController = new KeyboardNavigationController(this, this);
    // if (!m_keyboardNavigationController->initialize()) {
    //     setError("Failed to initialize KeyboardNavigationController");
    //     return false;
    // }
    
    // Initialize audio feedback service
    m_audioFeedbackService = ServiceContainer::instance()->resolve<AudioFeedbackService>();
    if (!m_audioFeedbackService) {
        logWarning("AudioFeedbackService not available - audio feedback will be limited");
    }
    
    // Initialize background operation feedback
    m_backgroundOperationFeedback = new BackgroundOperationFeedback(this);
    if (!m_backgroundOperationFeedback->initialize()) {
        logWarning("Failed to initialize BackgroundOperationFeedback - background operation feedback will be limited");
        delete m_backgroundOperationFeedback;
        m_backgroundOperationFeedback = nullptr;
    }
    
    // Initialize live region manager
    m_liveRegionManager = ServiceContainer::instance()->resolve<LiveRegionManager>();
    if (!m_liveRegionManager) {
        logWarning("LiveRegionManager not available - live region announcements will be limited");
    }
    
    // Initialize playback status announcer
    m_playbackStatusAnnouncer = ServiceContainer::instance()->resolve<PlaybackStatusAnnouncer>();
    if (!m_playbackStatusAnnouncer) {
        logWarning("PlaybackStatusAnnouncer not available - playback status announcements will be limited");
    }
    
    // Initialize system status announcer
    m_systemStatusAnnouncer = ServiceContainer::instance()->resolve<SystemStatusAnnouncer>();
    if (!m_systemStatusAnnouncer) {
        logWarning("SystemStatusAnnouncer not available - system status announcements will be limited");
    }
    
    // Temporarily disabled for beta build - has compilation errors
    // Initialize accessible help system
    // m_accessibleHelpSystem = new AccessibleHelpSystem(this, this);
    // if (!m_accessibleHelpSystem->initialize()) {
    //     logWarning("Failed to initialize AccessibleHelpSystem - help functionality will be limited");
    //     delete m_accessibleHelpSystem;
    //     m_accessibleHelpSystem = nullptr;
    // }
    
    // Initialize context-sensitive help service
    // m_contextSensitiveHelpService = new ContextSensitiveHelpService(this, this);
    // if (m_contextSensitiveHelpService->initialize()) {
    //     // Connect help system to context service
    //     if (m_accessibleHelpSystem) {
    //         m_contextSensitiveHelpService->setHelpSystem(m_accessibleHelpSystem);
    //     }
    // } else {
    //     logWarning("Failed to initialize ContextSensitiveHelpService - context help will be limited");
    //     delete m_contextSensitiveHelpService;
    //     m_contextSensitiveHelpService = nullptr;
    // }
    
    // Setup application-level accessibility
    setupApplicationAccessibility();
    
    // Start announcement processing if accessibility is enabled
    if (m_accessibilityEnabled) {
        m_announcementTimer->start();
    }
    
    logDebug("AccessibilityManager initialized successfully");
    return true;
}

void AccessibilityManager::doShutdown()
{
    logDebug("Shutting down AccessibilityManager");
    
    // Save settings before shutdown
    saveSettings();
    
    // Stop announcement processing
    m_announcementTimer->stop();
    
    // Cleanup accessibility resources
    cleanupAccessibility();
    
    logDebug("AccessibilityManager shutdown complete");
}

QString AccessibilityManager::getServiceName() const
{
    return "AccessibilityManager";
}

void AccessibilityManager::enableAccessibility(bool enabled)
{
    if (m_accessibilityEnabled == enabled) {
        return;
    }
    
    m_accessibilityEnabled = enabled;
    
    if (enabled) {
        logDebug("Enabling accessibility features");
        
        // Start announcement processing
        if (!m_announcementTimer->isActive()) {
            m_announcementTimer->start();
        }
        
        // Enable Qt accessibility
        QAccessible::setActive(true);
        
        // Announce accessibility activation
        announceMessage("Accessibility features enabled", Priority::High);
    } else {
        logDebug("Disabling accessibility features");
        
        // Stop announcement processing
        m_announcementTimer->stop();
        
        // Clear announcement queue
        m_announcementQueue.clear();
        
        // Announce accessibility deactivation
        announceMessage("Accessibility features disabled", Priority::High);
    }
    
    emit accessibilityStateChanged(enabled);
}

void AccessibilityManager::setVerbosityLevel(VerbosityLevel level)
{
    if (m_verbosityLevel == level) {
        return;
    }
    
    m_verbosityLevel = level;
    
    QString levelName;
    switch (level) {
        case VerbosityLevel::Terse:
            levelName = "Terse";
            break;
        case VerbosityLevel::Normal:
            levelName = "Normal";
            break;
        case VerbosityLevel::Verbose:
            levelName = "Verbose";
            break;
    }
    
    logDebug(QString("Verbosity level changed to: %1").arg(levelName));
    announceMessage(QString("Verbosity level set to %1").arg(levelName), Priority::Normal);
    
    emit verbosityLevelChanged(level);
}

void AccessibilityManager::registerAccessibleWidget(QWidget* widget, const AccessibilityMetadata& metadata)
{
    if (!widget) {
        logWarning("Attempted to register null widget for accessibility");
        return;
    }
    
    // Store metadata
    m_widgetMetadata[widget] = metadata;
    
    // Connect to widget destruction to cleanup metadata
    connect(widget, &QObject::destroyed,
            this, &AccessibilityManager::onWidgetDestroyed);
    
    logDebug(QString("Registered accessibility metadata for widget: %1").arg(metadata.name));
}

void AccessibilityManager::unregisterAccessibleWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    if (m_widgetMetadata.contains(widget)) {
        m_widgetMetadata.remove(widget);
        disconnect(widget, &QObject::destroyed,
                  this, &AccessibilityManager::onWidgetDestroyed);
        
        logDebug("Unregistered accessibility metadata for widget");
    }
}

AccessibilityManager::AccessibilityMetadata AccessibilityManager::getAccessibilityMetadata(QWidget* widget) const
{
    return m_widgetMetadata.value(widget, AccessibilityMetadata());
}

void AccessibilityManager::announceMessage(const QString& message, Priority priority)
{
    if (!m_accessibilityEnabled || message.isEmpty()) {
        return;
    }
    
    queueAnnouncement(message, priority);
    emit announcementRequested(message, priority);
}

void AccessibilityManager::loadSettings()
{
    if (!m_settings) {
        return;
    }
    
    m_settings->beginGroup(SETTINGS_GROUP);
    
    // Load accessibility enabled state
    m_accessibilityEnabled = m_settings->value(SETTINGS_ENABLED, false).toBool();
    
    // Load verbosity level
    int verbosityInt = m_settings->value(SETTINGS_VERBOSITY, 
                                        static_cast<int>(VerbosityLevel::Normal)).toInt();
    m_verbosityLevel = static_cast<VerbosityLevel>(verbosityInt);
    
    m_settings->endGroup();
    
    logDebug(QString("Loaded accessibility settings - Enabled: %1, Verbosity: %2")
             .arg(m_accessibilityEnabled ? "true" : "false")
             .arg(static_cast<int>(m_verbosityLevel)));
}

void AccessibilityManager::saveSettings()
{
    if (!m_settings) {
        return;
    }
    
    m_settings->beginGroup(SETTINGS_GROUP);
    
    m_settings->setValue(SETTINGS_ENABLED, m_accessibilityEnabled);
    m_settings->setValue(SETTINGS_VERBOSITY, static_cast<int>(m_verbosityLevel));
    
    m_settings->endGroup();
    m_settings->sync();
    
    logDebug("Saved accessibility settings");
}

void AccessibilityManager::onFocusChanged(QWidget* old, QWidget* now)
{
    Q_UNUSED(old)
    
    if (!m_accessibilityEnabled || !now) {
        return;
    }
    
    // Get accessibility metadata for the focused widget
    AccessibilityMetadata metadata = getAccessibilityMetadata(now);
    
    if (!metadata.name.isEmpty()) {
        QString announcement = metadata.name;
        
        // Add additional information based on verbosity level
        if (m_verbosityLevel == VerbosityLevel::Verbose && !metadata.description.isEmpty()) {
            announcement += ", " + metadata.description;
        }
        
        announceMessage(announcement, Priority::Normal);
    }
}

void AccessibilityManager::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (!m_accessibilityEnabled) {
        return;
    }
    
    QString stateMessage;
    switch (state) {
        case Qt::ApplicationActive:
            stateMessage = "XFB application activated";
            break;
        case Qt::ApplicationInactive:
            stateMessage = "XFB application deactivated";
            break;
        case Qt::ApplicationSuspended:
            stateMessage = "XFB application suspended";
            break;
        case Qt::ApplicationHidden:
            stateMessage = "XFB application hidden";
            break;
    }
    
    if (!stateMessage.isEmpty()) {
        announceMessage(stateMessage, Priority::Low);
    }
}

void AccessibilityManager::processAnnouncementQueue()
{
    if (m_processingAnnouncements || m_announcementQueue.isEmpty()) {
        return;
    }
    
    m_processingAnnouncements = true;
    
    // Process one announcement per timer tick to avoid overwhelming the screen reader
    AnnouncementItem item = m_announcementQueue.dequeue();
    processAnnouncement(item.message, item.priority);
    
    m_processingAnnouncements = false;
    
    // Stop timer if queue is empty
    if (m_announcementQueue.isEmpty()) {
        m_announcementTimer->stop();
    }
}

void AccessibilityManager::onWidgetDestroyed(QObject* obj)
{
    // Remove metadata for destroyed widget
    QWidget* widget = static_cast<QWidget*>(obj);
    m_widgetMetadata.remove(widget);
}

bool AccessibilityManager::initializeQtAccessibility()
{
    // Check if Qt accessibility is available
    if (!QAccessible::isActive()) {
        logDebug("Activating Qt accessibility framework");
        QAccessible::setActive(true);
    }
    
    if (!QAccessible::isActive()) {
        logWarning("Qt accessibility framework is not available - running in limited mode");
        // Don't fail initialization, just log the limitation
        return true;
    }
    
    logDebug("Qt accessibility framework is active");
    return true;
}

bool AccessibilityManager::initializeATSPIBridge()
{
    // Check if AT-SPI is available on the system
    if (!isATSPIAvailable()) {
        logWarning("AT-SPI is not available on this system");
        return false;
    }
    
    // Qt6 automatically handles AT-SPI bridge initialization when QAccessible is active
    // We just need to ensure the environment is properly configured
    
    // Set environment variables for AT-SPI if not already set
    if (qEnvironmentVariableIsEmpty("QT_ACCESSIBILITY")) {
        qputenv("QT_ACCESSIBILITY", "1");
        logDebug("Set QT_ACCESSIBILITY environment variable");
    }
    
    logDebug("AT-SPI bridge configuration completed");
    return true;
}

void AccessibilityManager::setupApplicationAccessibility()
{
    // Set application-level accessibility properties
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setProperty("accessibleName", "XFB Radio Broadcasting Software");
        app->setProperty("accessibleDescription", 
                        "Professional radio automation and broadcasting software");
    }
    
    logDebug("Application accessibility properties configured");
}

bool AccessibilityManager::initializePlayerAccessibility(player* playerWindow)
{
    if (!playerWindow) {
        logError("Cannot initialize player accessibility with null player window");
        return false;
    }
    
    // Player keyboard navigation enhancers temporarily disabled for beta build
    // if (!m_keyboardNavigationController) {
    //     logError("KeyboardNavigationController must be initialized before player accessibility");
    //     return false;
    // }
    // 
    // // Initialize player keyboard navigation enhancer
    // m_playerKeyboardEnhancer = new PlayerKeyboardNavigationEnhancer(
    //     playerWindow, m_keyboardNavigationController, this, this);
    // 
    // if (!m_playerKeyboardEnhancer->initialize()) {
    //     logError("Failed to initialize PlayerKeyboardNavigationEnhancer");
    //     delete m_playerKeyboardEnhancer;
    //     m_playerKeyboardEnhancer = nullptr;
    //     return false;
    // }
    // 
    // // Initialize database grid keyboard navigation enhancer
    // m_databaseGridKeyboardEnhancer = new DatabaseGridKeyboardNavigationEnhancer(
    //     playerWindow, m_keyboardNavigationController, this, this);
    // 
    // if (!m_databaseGridKeyboardEnhancer->initialize()) {
    //     logError("Failed to initialize DatabaseGridKeyboardNavigationEnhancer");
    //     delete m_databaseGridKeyboardEnhancer;
    //     m_databaseGridKeyboardEnhancer = nullptr;
    //     return false;
    // }
    
    // Initialize player audio feedback integration
    if (m_audioFeedbackService) {
        m_playerAudioFeedbackIntegration = new PlayerAudioFeedbackIntegration(playerWindow, this);
        
        if (!m_playerAudioFeedbackIntegration->initialize()) {
            logWarning("Failed to initialize PlayerAudioFeedbackIntegration - audio feedback may be limited");
            delete m_playerAudioFeedbackIntegration;
            m_playerAudioFeedbackIntegration = nullptr;
        } else {
            logDebug("Player audio feedback integration initialized successfully");
        }
    }
    
    logDebug("Player accessibility initialized successfully");
    return true;
}

void AccessibilityManager::cleanupAccessibility()
{
    // Temporarily disabled for beta build
    // Shutdown context-sensitive help service
    // if (m_contextSensitiveHelpService) {
    //     m_contextSensitiveHelpService->shutdown();
    //     delete m_contextSensitiveHelpService;
    //     m_contextSensitiveHelpService = nullptr;
    // }
    
    // Shutdown accessible help system
    // if (m_accessibleHelpSystem) {
    //     m_accessibleHelpSystem->shutdown();
    //     delete m_accessibleHelpSystem;
    //     m_accessibleHelpSystem = nullptr;
    // }
    
    // Shutdown background operation feedback
    if (m_backgroundOperationFeedback) {
        m_backgroundOperationFeedback->shutdown();
        delete m_backgroundOperationFeedback;
        m_backgroundOperationFeedback = nullptr;
    }
    
    // Shutdown player audio feedback integration
    if (m_playerAudioFeedbackIntegration) {
        m_playerAudioFeedbackIntegration->shutdown();
        delete m_playerAudioFeedbackIntegration;
        m_playerAudioFeedbackIntegration = nullptr;
    }
    
    // Database grid and player keyboard enhancers shutdown temporarily disabled for beta build
    // if (m_databaseGridKeyboardEnhancer) {
    //     m_databaseGridKeyboardEnhancer->shutdown();
    //     delete m_databaseGridKeyboardEnhancer;
    //     m_databaseGridKeyboardEnhancer = nullptr;
    // }
    // 
    // if (m_playerKeyboardEnhancer) {
    //     m_playerKeyboardEnhancer->shutdown();
    //     delete m_playerKeyboardEnhancer;
    //     m_playerKeyboardEnhancer = nullptr;
    // }
    
    // Keyboard navigation controller shutdown temporarily disabled for beta build
    // if (m_keyboardNavigationController) {
    //     m_keyboardNavigationController->shutdown();
    //     delete m_keyboardNavigationController;
    //     m_keyboardNavigationController = nullptr;
    // }
    
    // Shutdown widget enhancer
    if (m_widgetEnhancer) {
        m_widgetEnhancer->shutdown();
        delete m_widgetEnhancer;
        m_widgetEnhancer = nullptr;
    }
    
    // Clear widget metadata
    m_widgetMetadata.clear();
    
    // Clear announcement queue
    m_announcementQueue.clear();
    
    logDebug("Accessibility resources cleaned up");
}

void AccessibilityManager::queueAnnouncement(const QString& message, Priority priority)
{
    // Handle critical priority announcements immediately
    if (priority == Priority::Critical) {
        // Clear queue for critical announcements
        m_announcementQueue.clear();
        processAnnouncement(message, priority);
        return;
    }
    
    // Check queue size limit
    if (m_announcementQueue.size() >= MAX_ANNOUNCEMENT_QUEUE_SIZE) {
        // Remove oldest low priority items to make room
        while (m_announcementQueue.size() >= MAX_ANNOUNCEMENT_QUEUE_SIZE && 
               !m_announcementQueue.isEmpty()) {
            AnnouncementItem oldest = m_announcementQueue.head();
            if (oldest.priority == Priority::Low) {
                m_announcementQueue.dequeue();
            } else {
                break;
            }
        }
        
        // If still full, remove oldest item regardless of priority
        if (m_announcementQueue.size() >= MAX_ANNOUNCEMENT_QUEUE_SIZE) {
            m_announcementQueue.dequeue();
        }
    }
    
    // Add to queue
    AnnouncementItem item;
    item.message = message;
    item.priority = priority;
    item.timestamp = QDateTime::currentMSecsSinceEpoch();
    
    m_announcementQueue.enqueue(item);
    
    // Start timer if not already running
    if (!m_announcementTimer->isActive()) {
        m_announcementTimer->start();
    }
}

void AccessibilityManager::processAnnouncement(const QString& message, Priority priority)
{
    Q_UNUSED(priority)
    
    // For now, we'll use Qt's accessibility framework to announce messages
    // In a full implementation, this would integrate with specific screen reader APIs
    
    // Create a temporary accessible object for announcements
    // This is a simplified approach - a full implementation might use live regions
    if (QAccessible::isActive()) {
        // Log the announcement for debugging
        logDebug(QString("Accessibility announcement: %1").arg(message));
        
        // In Qt6, we can use QAccessibleEvent to notify screen readers
        // This is a basic implementation that can be enhanced later
        if (QWidget* focusWidget = QApplication::focusWidget()) {
            QAccessibleEvent event(focusWidget, QAccessible::Alert);
            QAccessible::updateAccessibility(&event);
        }
    }
}

bool AccessibilityManager::isATSPIAvailable() const
{
    // Check if AT-SPI daemon is running
    QProcess process;
    process.start("pgrep", QStringList() << "at-spi-bus-launcher");
    process.waitForFinished(1000);
    
    if (process.exitCode() == 0) {
        logDebug("AT-SPI bus launcher is running");
        return true;
    }
    
    // Also check for at-spi2-core
    process.start("pgrep", QStringList() << "at-spi2-registryd");
    process.waitForFinished(1000);
    
    if (process.exitCode() == 0) {
        logDebug("AT-SPI2 registry daemon is running");
        return true;
    }
    
    logDebug("AT-SPI services not detected");
    return false;
}