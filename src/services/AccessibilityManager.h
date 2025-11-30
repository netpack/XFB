#ifndef ACCESSIBILITYMANAGER_H
#define ACCESSIBILITYMANAGER_H

#include "BaseService.h"
#include <QObject>
#include <QWidget>
#include <QSettings>
#include <QAccessible>
#include <QHash>
#include <QQueue>
#include <QTimer>

class WidgetAccessibilityEnhancer;
// Temporarily disabled for beta build:
// class KeyboardNavigationController;
// class PlayerKeyboardNavigationEnhancer;
// class DatabaseGridKeyboardNavigationEnhancer;
class AudioFeedbackService;
class PlayerAudioFeedbackIntegration;
class BackgroundOperationFeedback;
class LiveRegionManager;
class PlaybackStatusAnnouncer;
class SystemStatusAnnouncer;
class AccessibleHelpSystem;
class ContextSensitiveHelpService;
class player;

/**
 * @brief Central coordinator for all accessibility features in XFB
 * 
 * The AccessibilityManager serves as the main entry point for accessibility
 * functionality, coordinating between different accessibility services and
 * managing the overall accessibility state of the application.
 * 
 * This class follows the singleton pattern through the service container to ensure consistent
 * accessibility state across the entire application and integrates with Qt6's
 * QAccessible framework and AT-SPI bridge for ORCA communication.
 * 
 * @example
 * @code
 * auto* manager = ServiceContainer::instance()->resolve<AccessibilityManager>();
 * manager->enableAccessibility(true);
 * manager->setVerbosityLevel(AccessibilityManager::Verbose);
 * manager->announceMessage("Operation completed successfully");
 * @endcode
 * 
 * @see WidgetAccessibilityEnhancer, BaseService
 * @since XFB 2.0
 */
class AccessibilityManager : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Verbosity levels for accessibility announcements
     */
    enum class VerbosityLevel {
        Terse,      ///< Control type and name only
        Normal,     ///< Standard information level
        Verbose     ///< Including hints and descriptions
    };

    /**
     * @brief Priority levels for announcements
     */
    enum class Priority {
        Low,        ///< Background information
        Normal,     ///< Standard user actions
        High,       ///< Important notifications
        Critical    ///< Emergency alerts that interrupt other announcements
    };

    /**
     * @brief Accessibility metadata for widgets
     */
    struct AccessibilityMetadata {
        QString name;
        QString description;
        QAccessible::Role role;
        QAccessible::State state;
        QString helpText;
        QStringList shortcuts;
        bool isLiveRegion;
        
        AccessibilityMetadata() 
            : role(QAccessible::NoRole)
            , state(QAccessible::State())
            , isLiveRegion(false) {}
    };

    explicit AccessibilityManager(QObject* parent = nullptr);
    ~AccessibilityManager() override;

    /**
     * @brief Enable or disable accessibility features
     * @param enabled true to enable accessibility, false to disable
     */
    void enableAccessibility(bool enabled);

    /**
     * @brief Check if accessibility is currently enabled
     * @return true if accessibility is enabled
     */
    bool isAccessibilityEnabled() const { return m_accessibilityEnabled; }

    /**
     * @brief Set the verbosity level for announcements
     * @param level The verbosity level to set
     */
    void setVerbosityLevel(VerbosityLevel level);

    /**
     * @brief Get the current verbosity level
     * @return The current verbosity level
     */
    VerbosityLevel verbosityLevel() const { return m_verbosityLevel; }

    /**
     * @brief Register a widget with accessibility metadata
     * @param widget The widget to register
     * @param metadata Accessibility metadata for the widget
     */
    void registerAccessibleWidget(QWidget* widget, const AccessibilityMetadata& metadata);

    /**
     * @brief Unregister a widget from accessibility management
     * @param widget The widget to unregister
     */
    void unregisterAccessibleWidget(QWidget* widget);

    /**
     * @brief Get accessibility metadata for a widget
     * @param widget The widget to get metadata for
     * @return The accessibility metadata, or default metadata if not found
     */
    AccessibilityMetadata getAccessibilityMetadata(QWidget* widget) const;

    /**
     * @brief Announce a message to the screen reader
     * @param message The message to announce
     * @param priority Priority level of the announcement
     */
    void announceMessage(const QString& message, Priority priority = Priority::Normal);

    /**
     * @brief Load accessibility settings from persistent storage
     */
    void loadSettings();

    /**
     * @brief Save accessibility settings to persistent storage
     */
    void saveSettings();

    /**
     * @brief Get the widget accessibility enhancer
     * @return Pointer to the widget enhancer, or nullptr if not initialized
     */
    WidgetAccessibilityEnhancer* widgetEnhancer() const { return m_widgetEnhancer; }

    /**
     * @brief Get the keyboard navigation controller (temporarily disabled for beta build)
     * @return nullptr (keyboard navigation controller disabled)
     */
    // KeyboardNavigationController* keyboardNavigationController() const { return m_keyboardNavigationController; }
    void* keyboardNavigationController() const { return nullptr; }

    /**
     * @brief Initialize player-specific accessibility enhancements
     * @param playerWindow Pointer to the main player window
     * @return true if initialization was successful
     */
    bool initializePlayerAccessibility(player* playerWindow);

    /**
     * @brief Get the player keyboard navigation enhancer (temporarily disabled for beta build)
     * @return nullptr (player keyboard enhancer disabled)
     */
    // PlayerKeyboardNavigationEnhancer* playerKeyboardEnhancer() const { return m_playerKeyboardEnhancer; }
    void* playerKeyboardEnhancer() const { return nullptr; }

    /**
     * @brief Get the database grid keyboard navigation enhancer (temporarily disabled for beta build)
     * @return nullptr (database grid keyboard enhancer disabled)
     */
    // DatabaseGridKeyboardNavigationEnhancer* databaseGridKeyboardEnhancer() const { return m_databaseGridKeyboardEnhancer; }
    void* databaseGridKeyboardEnhancer() const { return nullptr; }

    /**
     * @brief Get the audio feedback service
     * @return Pointer to the audio feedback service, or nullptr if not initialized
     */
    AudioFeedbackService* audioFeedbackService() const { return m_audioFeedbackService; }

    /**
     * @brief Get the player audio feedback integration
     * @return Pointer to the player audio feedback integration, or nullptr if not initialized
     */
    PlayerAudioFeedbackIntegration* playerAudioFeedbackIntegration() const { return m_playerAudioFeedbackIntegration; }

    /**
     * @brief Get the background operation feedback service
     * @return Pointer to the background operation feedback service, or nullptr if not initialized
     */
    BackgroundOperationFeedback* backgroundOperationFeedback() const { return m_backgroundOperationFeedback; }

    /**
     * @brief Get the live region manager
     * @return Pointer to the live region manager, or nullptr if not initialized
     */
    LiveRegionManager* liveRegionManager() const { return m_liveRegionManager; }

    /**
     * @brief Get the playback status announcer
     * @return Pointer to the playback status announcer, or nullptr if not initialized
     */
    PlaybackStatusAnnouncer* playbackStatusAnnouncer() const { return m_playbackStatusAnnouncer; }

    /**
     * @brief Get the system status announcer
     * @return Pointer to the system status announcer, or nullptr if not initialized
     */
    SystemStatusAnnouncer* systemStatusAnnouncer() const { return m_systemStatusAnnouncer; }

    /**
     * @brief Get the accessible help system
     * @return Pointer to the help system, or nullptr if not initialized
     */
    AccessibleHelpSystem* accessibleHelpSystem() const { return m_accessibleHelpSystem; }

    /**
     * @brief Get the context-sensitive help service
     * @return Pointer to the context-sensitive help service, or nullptr if not initialized
     */
    ContextSensitiveHelpService* contextSensitiveHelpService() const { return m_contextSensitiveHelpService; }

public slots:
    /**
     * @brief Handle focus change events
     * @param old Previously focused widget
     * @param now Currently focused widget
     */
    void onFocusChanged(QWidget* old, QWidget* now);

    /**
     * @brief Handle application state changes
     * @param state New application state
     */
    void onApplicationStateChanged(Qt::ApplicationState state);

signals:
    /**
     * @brief Emitted when accessibility state changes
     * @param enabled true if accessibility was enabled, false if disabled
     */
    void accessibilityStateChanged(bool enabled);

    /**
     * @brief Emitted when an announcement is requested
     * @param message The message to announce
     * @param priority Priority level of the announcement
     */
    void announcementRequested(const QString& message, Priority priority);

    /**
     * @brief Emitted when verbosity level changes
     * @param level The new verbosity level
     */
    void verbosityLevelChanged(VerbosityLevel level);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Process queued announcements
     */
    void processAnnouncementQueue();

    /**
     * @brief Handle widget destruction
     * @param obj The destroyed object
     */
    void onWidgetDestroyed(QObject* obj);

private:
    /**
     * @brief Initialize Qt accessibility framework
     * @return true if initialization was successful
     */
    bool initializeQtAccessibility();

    /**
     * @brief Initialize AT-SPI bridge configuration
     * @return true if initialization was successful
     */
    bool initializeATSPIBridge();

    /**
     * @brief Setup application-level accessibility features
     */
    void setupApplicationAccessibility();

    /**
     * @brief Cleanup accessibility resources
     */
    void cleanupAccessibility();

    /**
     * @brief Queue an announcement for processing
     * @param message The message to queue
     * @param priority Priority level
     */
    void queueAnnouncement(const QString& message, Priority priority);

    /**
     * @brief Process a single announcement
     * @param message The message to announce
     * @param priority Priority level
     */
    void processAnnouncement(const QString& message, Priority priority);

    /**
     * @brief Check if AT-SPI is available on the system
     * @return true if AT-SPI is available
     */
    bool isATSPIAvailable() const;

    // Member variables
    bool m_accessibilityEnabled;
    VerbosityLevel m_verbosityLevel;
    WidgetAccessibilityEnhancer* m_widgetEnhancer;
    // Temporarily disabled for beta build:
    // KeyboardNavigationController* m_keyboardNavigationController;
    // PlayerKeyboardNavigationEnhancer* m_playerKeyboardEnhancer;
    // DatabaseGridKeyboardNavigationEnhancer* m_databaseGridKeyboardEnhancer;
    AudioFeedbackService* m_audioFeedbackService;
    PlayerAudioFeedbackIntegration* m_playerAudioFeedbackIntegration;
    BackgroundOperationFeedback* m_backgroundOperationFeedback;
    LiveRegionManager* m_liveRegionManager;
    PlaybackStatusAnnouncer* m_playbackStatusAnnouncer;
    SystemStatusAnnouncer* m_systemStatusAnnouncer;
    AccessibleHelpSystem* m_accessibleHelpSystem;
    ContextSensitiveHelpService* m_contextSensitiveHelpService;
    QSettings* m_settings;
    
    // Widget metadata management
    QHash<QWidget*, AccessibilityMetadata> m_widgetMetadata;
    
    // Announcement queue management
    struct AnnouncementItem {
        QString message;
        Priority priority;
        qint64 timestamp;
    };
    QQueue<AnnouncementItem> m_announcementQueue;
    QTimer* m_announcementTimer;
    bool m_processingAnnouncements;
    
    // Configuration constants
    static constexpr int ANNOUNCEMENT_PROCESSING_INTERVAL_MS = 100;
    static constexpr int MAX_ANNOUNCEMENT_QUEUE_SIZE = 50;
    static constexpr const char* SETTINGS_GROUP = "Accessibility";
    static constexpr const char* SETTINGS_ENABLED = "Enabled";
    static constexpr const char* SETTINGS_VERBOSITY = "VerbosityLevel";
};

Q_DECLARE_METATYPE(AccessibilityManager::VerbosityLevel)
Q_DECLARE_METATYPE(AccessibilityManager::Priority)

#endif // ACCESSIBILITYMANAGER_H