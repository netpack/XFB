#ifndef ACCESSIBILITYSETTINGSSERVICE_H
#define ACCESSIBILITYSETTINGSSERVICE_H

#include "BaseService.h"
#include <QObject>
#include <QSettings>
#include <QKeySequence>
#include <QMap>
#include <QStringList>
#include <QTimer>

/**
 * @brief Service for managing accessibility settings and user preferences
 * 
 * The AccessibilitySettingsService provides comprehensive management of user
 * accessibility preferences including verbosity levels, timing options,
 * keyboard shortcuts, and braille display settings. It handles settings
 * persistence, validation, and provides a centralized interface for all
 * accessibility configuration needs.
 * 
 * @example
 * @code
 * auto* settingsService = ServiceContainer::instance()->resolve<AccessibilitySettingsService>();
 * settingsService->setVerbosityLevel(AccessibilitySettingsService::VerbosityLevel::Verbose);
 * settingsService->setCustomKeyboardShortcut("announce_time", QKeySequence("Ctrl+T"));
 * settingsService->saveSettings();
 * @endcode
 * 
 * @see AccessibilityManager, BaseService
 * @since XFB 2.0
 */
class AccessibilitySettingsService : public BaseService
{
    Q_OBJECT

public:
    /**
     * @brief Verbosity levels for accessibility announcements
     */
    enum class VerbosityLevel {
        Terse,      ///< Control type and name only
        Normal,     ///< Standard information level (default)
        Verbose     ///< Including hints, descriptions, and additional context
    };

    /**
     * @brief Announcement timing preferences
     */
    enum class AnnouncementTiming {
        Immediate,  ///< Announce immediately when events occur
        Delayed,    ///< Brief delay to avoid overwhelming announcements
        OnRequest   ///< Only announce when explicitly requested
    };

    /**
     * @brief Braille display formatting options
     */
    enum class BrailleFormat {
        Standard,   ///< Standard braille formatting
        Compact,    ///< Compact format with abbreviations
        Detailed    ///< Detailed format with full descriptions
    };

    /**
     * @brief Keyboard shortcut configuration
     */
    struct KeyboardShortcut {
        QString action;         ///< Action identifier
        QKeySequence sequence;  ///< Key sequence
        QString description;    ///< Human-readable description
        QString context;        ///< Context where shortcut applies
        bool customizable;      ///< Whether user can customize this shortcut
        
        KeyboardShortcut() : customizable(true) {}
        KeyboardShortcut(const QString& act, const QKeySequence& seq, 
                        const QString& desc, const QString& ctx = QString(), 
                        bool custom = true)
            : action(act), sequence(seq), description(desc), context(ctx), customizable(custom) {}
    };

    /**
     * @brief Complete accessibility settings configuration
     */
    struct AccessibilitySettings {
        bool enabled;                                   ///< Master accessibility enable/disable
        VerbosityLevel verbosity;                      ///< Announcement verbosity level
        AnnouncementTiming timing;                     ///< Announcement timing preference
        bool brailleEnabled;                           ///< Braille display support enabled
        BrailleFormat brailleFormat;                   ///< Braille formatting preference
        QMap<QString, QKeySequence> customShortcuts;   ///< Custom keyboard shortcuts
        bool highContrastMode;                         ///< High contrast visual mode
        int announcementDelay;                         ///< Delay in milliseconds for delayed timing
        bool interruptOnCritical;                      ///< Interrupt announcements for critical alerts
        bool announceTooltips;                         ///< Announce tooltip content
        bool announceStateChanges;                     ///< Announce widget state changes
        QString brailleDevice;                         ///< Braille display device identifier
        
        AccessibilitySettings()
            : enabled(false)
            , verbosity(VerbosityLevel::Normal)
            , timing(AnnouncementTiming::Immediate)
            , brailleEnabled(false)
            , brailleFormat(BrailleFormat::Standard)
            , highContrastMode(false)
            , announcementDelay(500)
            , interruptOnCritical(true)
            , announceTooltips(true)
            , announceStateChanges(true) {}
    };

    explicit AccessibilitySettingsService(QObject* parent = nullptr);
    ~AccessibilitySettingsService() override;

    // Settings access methods
    
    /**
     * @brief Get the current accessibility settings
     * @return Complete accessibility settings structure
     */
    AccessibilitySettings settings() const { return m_settings; }

    /**
     * @brief Check if accessibility is enabled
     * @return true if accessibility is enabled
     */
    bool isAccessibilityEnabled() const { return m_settings.enabled; }

    /**
     * @brief Get the current verbosity level
     * @return Current verbosity level
     */
    VerbosityLevel verbosityLevel() const { return m_settings.verbosity; }

    /**
     * @brief Get the current announcement timing preference
     * @return Current timing preference
     */
    AnnouncementTiming announcementTiming() const { return m_settings.timing; }

    /**
     * @brief Check if braille display is enabled
     * @return true if braille display is enabled
     */
    bool isBrailleEnabled() const { return m_settings.brailleEnabled; }

    /**
     * @brief Get the current braille format
     * @return Current braille format
     */
    BrailleFormat brailleFormat() const { return m_settings.brailleFormat; }

    /**
     * @brief Check if high contrast mode is enabled
     * @return true if high contrast mode is enabled
     */
    bool isHighContrastMode() const { return m_settings.highContrastMode; }

    /**
     * @brief Get the announcement delay in milliseconds
     * @return Delay in milliseconds
     */
    int announcementDelay() const { return m_settings.announcementDelay; }

    /**
     * @brief Get all available keyboard shortcuts
     * @return Map of action identifiers to keyboard shortcuts
     */
    QMap<QString, KeyboardShortcut> availableKeyboardShortcuts() const { return m_availableShortcuts; }

    /**
     * @brief Get a specific keyboard shortcut
     * @param action Action identifier
     * @return Keyboard shortcut for the action, or default if not found
     */
    KeyboardShortcut keyboardShortcut(const QString& action) const;

    /**
     * @brief Get the key sequence for a specific action
     * @param action Action identifier
     * @return Key sequence, or empty sequence if not found
     */
    QKeySequence keySequenceForAction(const QString& action) const;

    /**
     * @brief Check if a key sequence conflicts with existing shortcuts
     * @param sequence Key sequence to check
     * @param excludeAction Action to exclude from conflict check
     * @return Action that conflicts, or empty string if no conflict
     */
    QString checkKeySequenceConflict(const QKeySequence& sequence, const QString& excludeAction = QString()) const;

    // Settings modification methods

    /**
     * @brief Enable or disable accessibility
     * @param enabled true to enable, false to disable
     */
    void setAccessibilityEnabled(bool enabled);

    /**
     * @brief Set the verbosity level
     * @param level New verbosity level
     */
    void setVerbosityLevel(VerbosityLevel level);

    /**
     * @brief Set the announcement timing preference
     * @param timing New timing preference
     */
    void setAnnouncementTiming(AnnouncementTiming timing);

    /**
     * @brief Enable or disable braille display
     * @param enabled true to enable, false to disable
     */
    void setBrailleEnabled(bool enabled);

    /**
     * @brief Set the braille format
     * @param format New braille format
     */
    void setBrailleFormat(BrailleFormat format);

    /**
     * @brief Enable or disable high contrast mode
     * @param enabled true to enable, false to disable
     */
    void setHighContrastMode(bool enabled);

    /**
     * @brief Set the announcement delay
     * @param delay Delay in milliseconds (must be >= 0)
     */
    void setAnnouncementDelay(int delay);

    /**
     * @brief Set a custom keyboard shortcut
     * @param action Action identifier
     * @param sequence New key sequence
     * @return true if shortcut was set successfully, false if conflict or invalid
     */
    bool setCustomKeyboardShortcut(const QString& action, const QKeySequence& sequence);

    /**
     * @brief Reset a keyboard shortcut to its default value
     * @param action Action identifier
     * @return true if shortcut was reset successfully
     */
    bool resetKeyboardShortcut(const QString& action);

    /**
     * @brief Reset all keyboard shortcuts to default values
     */
    void resetAllKeyboardShortcuts();

    /**
     * @brief Set the braille device identifier
     * @param device Device identifier
     */
    void setBrailleDevice(const QString& device);

    /**
     * @brief Apply a complete settings configuration
     * @param settings New settings to apply
     * @return true if settings were applied successfully
     */
    bool applySettings(const AccessibilitySettings& settings);

    // Persistence methods

    /**
     * @brief Load settings from persistent storage
     * @return true if settings were loaded successfully
     */
    bool loadSettings();

    /**
     * @brief Save current settings to persistent storage
     * @return true if settings were saved successfully
     */
    bool saveSettings();

    /**
     * @brief Reset all settings to default values
     */
    void resetToDefaults();

    /**
     * @brief Export settings to a file
     * @param filePath Path to export file
     * @return true if export was successful
     */
    bool exportSettings(const QString& filePath) const;

    /**
     * @brief Import settings from a file
     * @param filePath Path to import file
     * @return true if import was successful
     */
    bool importSettings(const QString& filePath);

    // Validation methods

    /**
     * @brief Validate settings configuration
     * @param settings Settings to validate
     * @return List of validation errors, empty if valid
     */
    QStringList validateSettings(const AccessibilitySettings& settings) const;

    /**
     * @brief Check if a verbosity level is valid
     * @param level Verbosity level to check
     * @return true if valid
     */
    static bool isValidVerbosityLevel(VerbosityLevel level);

    /**
     * @brief Check if an announcement timing is valid
     * @param timing Timing to check
     * @return true if valid
     */
    static bool isValidAnnouncementTiming(AnnouncementTiming timing);

    /**
     * @brief Check if a braille format is valid
     * @param format Format to check
     * @return true if valid
     */
    static bool isValidBrailleFormat(BrailleFormat format);

    // Utility methods

    /**
     * @brief Convert verbosity level to string
     * @param level Verbosity level
     * @return String representation
     */
    static QString verbosityLevelToString(VerbosityLevel level);

    /**
     * @brief Convert string to verbosity level
     * @param str String representation
     * @return Verbosity level, or Normal if invalid
     */
    static VerbosityLevel verbosityLevelFromString(const QString& str);

    /**
     * @brief Convert announcement timing to string
     * @param timing Announcement timing
     * @return String representation
     */
    static QString announcementTimingToString(AnnouncementTiming timing);

    /**
     * @brief Convert string to announcement timing
     * @param str String representation
     * @return Announcement timing, or Immediate if invalid
     */
    static AnnouncementTiming announcementTimingFromString(const QString& str);

    /**
     * @brief Convert braille format to string
     * @param format Braille format
     * @return String representation
     */
    static QString brailleFormatToString(BrailleFormat format);

    /**
     * @brief Convert string to braille format
     * @param str String representation
     * @return Braille format, or Standard if invalid
     */
    static BrailleFormat brailleFormatFromString(const QString& str);

public slots:
    /**
     * @brief Handle settings change from external source
     */
    void onExternalSettingsChanged();

signals:
    /**
     * @brief Emitted when accessibility enabled state changes
     * @param enabled New enabled state
     */
    void accessibilityEnabledChanged(bool enabled);

    /**
     * @brief Emitted when verbosity level changes
     * @param level New verbosity level
     */
    void verbosityLevelChanged(VerbosityLevel level);

    /**
     * @brief Emitted when announcement timing changes
     * @param timing New timing preference
     */
    void announcementTimingChanged(AnnouncementTiming timing);

    /**
     * @brief Emitted when braille settings change
     * @param enabled Braille enabled state
     * @param format Braille format
     */
    void brailleSettingsChanged(bool enabled, BrailleFormat format);

    /**
     * @brief Emitted when keyboard shortcut changes
     * @param action Action identifier
     * @param sequence New key sequence
     */
    void keyboardShortcutChanged(const QString& action, const QKeySequence& sequence);

    /**
     * @brief Emitted when high contrast mode changes
     * @param enabled New high contrast state
     */
    void highContrastModeChanged(bool enabled);

    /**
     * @brief Emitted when settings are loaded from storage
     */
    void settingsLoaded();

    /**
     * @brief Emitted when settings are saved to storage
     */
    void settingsSaved();

    /**
     * @brief Emitted when settings are reset to defaults
     */
    void settingsReset();

    /**
     * @brief Emitted when settings validation fails
     * @param errors List of validation errors
     */
    void settingsValidationFailed(const QStringList& errors);

protected:
    // BaseService interface implementation
    bool doInitialize() override;
    void doShutdown() override;
    QString getServiceName() const override;

private slots:
    /**
     * @brief Handle auto-save timer timeout
     */
    void onAutoSaveTimeout();

private:
    /**
     * @brief Initialize default keyboard shortcuts
     */
    void initializeDefaultKeyboardShortcuts();

    /**
     * @brief Initialize settings from QSettings
     */
    void initializeSettingsFromStorage();

    /**
     * @brief Apply default settings values
     */
    void applyDefaultSettings();

    /**
     * @brief Validate and sanitize settings
     * @param settings Settings to validate and sanitize
     * @return Sanitized settings
     */
    AccessibilitySettings validateAndSanitizeSettings(const AccessibilitySettings& settings) const;

    /**
     * @brief Save settings to QSettings
     */
    void saveSettingsToStorage();

    /**
     * @brief Load settings from QSettings
     */
    void loadSettingsFromStorage();

    /**
     * @brief Setup auto-save functionality
     */
    void setupAutoSave();

    /**
     * @brief Emit appropriate change signals
     * @param oldSettings Previous settings
     * @param newSettings New settings
     */
    void emitChangeSignals(const AccessibilitySettings& oldSettings, const AccessibilitySettings& newSettings);

    // Member variables
    AccessibilitySettings m_settings;
    QMap<QString, KeyboardShortcut> m_availableShortcuts;
    QMap<QString, KeyboardShortcut> m_defaultShortcuts;
    QSettings* m_qsettings;
    QTimer* m_autoSaveTimer;
    bool m_settingsChanged;
    bool m_loadingSettings;

    // Configuration constants
    static constexpr int AUTO_SAVE_INTERVAL_MS = 5000;  // 5 seconds
    static constexpr int MIN_ANNOUNCEMENT_DELAY = 0;
    static constexpr int MAX_ANNOUNCEMENT_DELAY = 5000;  // 5 seconds
    static constexpr const char* SETTINGS_GROUP = "AccessibilitySettings";
    static constexpr const char* SETTINGS_ENABLED = "Enabled";
    static constexpr const char* SETTINGS_VERBOSITY = "VerbosityLevel";
    static constexpr const char* SETTINGS_TIMING = "AnnouncementTiming";
    static constexpr const char* SETTINGS_BRAILLE_ENABLED = "BrailleEnabled";
    static constexpr const char* SETTINGS_BRAILLE_FORMAT = "BrailleFormat";
    static constexpr const char* SETTINGS_HIGH_CONTRAST = "HighContrastMode";
    static constexpr const char* SETTINGS_ANNOUNCEMENT_DELAY = "AnnouncementDelay";
    static constexpr const char* SETTINGS_INTERRUPT_CRITICAL = "InterruptOnCritical";
    static constexpr const char* SETTINGS_ANNOUNCE_TOOLTIPS = "AnnounceTooltips";
    static constexpr const char* SETTINGS_ANNOUNCE_STATE_CHANGES = "AnnounceStateChanges";
    static constexpr const char* SETTINGS_BRAILLE_DEVICE = "BrailleDevice";
    static constexpr const char* SETTINGS_CUSTOM_SHORTCUTS = "CustomShortcuts";
};

Q_DECLARE_METATYPE(AccessibilitySettingsService::VerbosityLevel)
Q_DECLARE_METATYPE(AccessibilitySettingsService::AnnouncementTiming)
Q_DECLARE_METATYPE(AccessibilitySettingsService::BrailleFormat)

#endif // ACCESSIBILITYSETTINGSSERVICE_H