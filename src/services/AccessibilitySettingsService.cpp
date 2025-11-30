#include "AccessibilitySettingsService.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

AccessibilitySettingsService::AccessibilitySettingsService(QObject* parent)
    : BaseService(parent)
    , m_qsettings(nullptr)
    , m_autoSaveTimer(nullptr)
    , m_settingsChanged(false)
    , m_loadingSettings(false)
{
    // Initialize settings with defaults
    applyDefaultSettings();
    
    // Initialize default keyboard shortcuts
    initializeDefaultKeyboardShortcuts();
}

AccessibilitySettingsService::~AccessibilitySettingsService()
{
    // Save any pending changes before destruction
    if (m_settingsChanged && m_qsettings) {
        saveSettings();
    }
}

bool AccessibilitySettingsService::doInitialize()
{
    logDebug("Initializing AccessibilitySettingsService");
    
    try {
        // Initialize QSettings
        m_qsettings = new QSettings(this);
        
        // Setup auto-save functionality
        setupAutoSave();
        
        // Load settings from storage
        if (!loadSettings()) {
            logWarning("Failed to load accessibility settings, using defaults");
            applyDefaultSettings();
        }
        
        logDebug("AccessibilitySettingsService initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        setError(QString("Failed to initialize AccessibilitySettingsService: %1").arg(e.what()));
        return false;
    } catch (...) {
        setError("Failed to initialize AccessibilitySettingsService: Unknown error");
        return false;
    }
}

void AccessibilitySettingsService::doShutdown()
{
    logDebug("Shutting down AccessibilitySettingsService");
    
    // Save any pending changes
    if (m_settingsChanged) {
        saveSettings();
    }
    
    // Stop auto-save timer
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
    }
    
    logDebug("AccessibilitySettingsService shutdown complete");
}

QString AccessibilitySettingsService::getServiceName() const
{
    return "AccessibilitySettingsService";
}

void AccessibilitySettingsService::initializeDefaultKeyboardShortcuts()
{
    // Clear existing shortcuts
    m_defaultShortcuts.clear();
    m_availableShortcuts.clear();
    
    // Define default keyboard shortcuts based on requirements
    m_defaultShortcuts.insert("announce_time", 
        KeyboardShortcut("announce_time", QKeySequence("Ctrl+T"), 
                        "Announce current playback time", "Player", true));
    
    m_defaultShortcuts.insert("announce_track", 
        KeyboardShortcut("announce_track", QKeySequence("Ctrl+Shift+T"), 
                        "Announce current track information", "Player", true));
    
    m_defaultShortcuts.insert("announce_status", 
        KeyboardShortcut("announce_status", QKeySequence("Ctrl+S"), 
                        "Announce system status", "Global", true));
    
    m_defaultShortcuts.insert("toggle_verbosity", 
        KeyboardShortcut("toggle_verbosity", QKeySequence("Ctrl+Alt+V"), 
                        "Toggle verbosity level", "Global", true));
    
    m_defaultShortcuts.insert("repeat_last_announcement", 
        KeyboardShortcut("repeat_last_announcement", QKeySequence("Ctrl+R"), 
                        "Repeat last announcement", "Global", true));
    
    m_defaultShortcuts.insert("navigate_next_item", 
        KeyboardShortcut("navigate_next_item", QKeySequence("Ctrl+Right"), 
                        "Navigate to next item", "Navigation", true));
    
    m_defaultShortcuts.insert("navigate_previous_item", 
        KeyboardShortcut("navigate_previous_item", QKeySequence("Ctrl+Left"), 
                        "Navigate to previous item", "Navigation", true));
    
    m_defaultShortcuts.insert("select_all_items", 
        KeyboardShortcut("select_all_items", QKeySequence("Ctrl+A"), 
                        "Select all items", "Navigation", false)); // Not customizable - standard shortcut
    
    m_defaultShortcuts.insert("copy_items", 
        KeyboardShortcut("copy_items", QKeySequence("Ctrl+C"), 
                        "Copy selected items", "Navigation", false)); // Not customizable - standard shortcut
    
    m_defaultShortcuts.insert("cut_items", 
        KeyboardShortcut("cut_items", QKeySequence("Ctrl+X"), 
                        "Cut selected items", "Navigation", false)); // Not customizable - standard shortcut
    
    m_defaultShortcuts.insert("paste_items", 
        KeyboardShortcut("paste_items", QKeySequence("Ctrl+V"), 
                        "Paste items", "Navigation", false)); // Not customizable - standard shortcut
    
    m_defaultShortcuts.insert("delete_items", 
        KeyboardShortcut("delete_items", QKeySequence("Delete"), 
                        "Delete selected items", "Navigation", true));
    
    m_defaultShortcuts.insert("edit_item", 
        KeyboardShortcut("edit_item", QKeySequence("F2"), 
                        "Edit selected item", "Navigation", true));
    
    m_defaultShortcuts.insert("cancel_operation", 
        KeyboardShortcut("cancel_operation", QKeySequence("Escape"), 
                        "Cancel current operation", "Global", false)); // Not customizable - standard shortcut
    
    m_defaultShortcuts.insert("show_help", 
        KeyboardShortcut("show_help", QKeySequence("F1"), 
                        "Show context-sensitive help", "Global", true));
    
    m_defaultShortcuts.insert("accessibility_preferences", 
        KeyboardShortcut("accessibility_preferences", QKeySequence("Ctrl+Alt+A"), 
                        "Open accessibility preferences", "Global", true));
    
    // Copy defaults to available shortcuts
    m_availableShortcuts = m_defaultShortcuts;
    
    logDebug(QString("Initialized %1 default keyboard shortcuts").arg(m_defaultShortcuts.size()));
}

void AccessibilitySettingsService::applyDefaultSettings()
{
    m_settings = AccessibilitySettings(); // Use default constructor values
    
    // Apply any custom defaults here if needed
    m_settings.enabled = false; // Start disabled by default
    m_settings.verbosity = VerbosityLevel::Normal;
    m_settings.timing = AnnouncementTiming::Immediate;
    m_settings.announcementDelay = 500;
    
    logDebug("Applied default accessibility settings");
}

void AccessibilitySettingsService::setupAutoSave()
{
    if (!m_autoSaveTimer) {
        m_autoSaveTimer = new QTimer(this);
        m_autoSaveTimer->setSingleShot(true);
        m_autoSaveTimer->setInterval(AUTO_SAVE_INTERVAL_MS);
        
        connect(m_autoSaveTimer, &QTimer::timeout,
                this, &AccessibilitySettingsService::onAutoSaveTimeout);
    }
}

AccessibilitySettingsService::KeyboardShortcut AccessibilitySettingsService::keyboardShortcut(const QString& action) const
{
    return m_availableShortcuts.value(action, KeyboardShortcut());
}

QKeySequence AccessibilitySettingsService::keySequenceForAction(const QString& action) const
{
    auto shortcut = m_availableShortcuts.value(action);
    return shortcut.sequence;
}

QString AccessibilitySettingsService::checkKeySequenceConflict(const QKeySequence& sequence, const QString& excludeAction) const
{
    if (sequence.isEmpty()) {
        return QString(); // Empty sequences don't conflict
    }
    
    for (auto it = m_availableShortcuts.constBegin(); it != m_availableShortcuts.constEnd(); ++it) {
        if (it.key() != excludeAction && it.value().sequence == sequence) {
            return it.key();
        }
    }
    
    return QString(); // No conflict found
}

void AccessibilitySettingsService::setAccessibilityEnabled(bool enabled)
{
    if (m_settings.enabled != enabled) {
        m_settings.enabled = enabled;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        emit accessibilityEnabledChanged(enabled);
        logDebug(QString("Accessibility %1").arg(enabled ? "enabled" : "disabled"));
    }
}

void AccessibilitySettingsService::setVerbosityLevel(VerbosityLevel level)
{
    if (!isValidVerbosityLevel(level)) {
        logWarning(QString("Invalid verbosity level: %1").arg(static_cast<int>(level)));
        return;
    }
    
    if (m_settings.verbosity != level) {
        m_settings.verbosity = level;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        emit verbosityLevelChanged(level);
        logDebug(QString("Verbosity level changed to: %1").arg(verbosityLevelToString(level)));
    }
}

void AccessibilitySettingsService::setAnnouncementTiming(AnnouncementTiming timing)
{
    if (!isValidAnnouncementTiming(timing)) {
        logWarning(QString("Invalid announcement timing: %1").arg(static_cast<int>(timing)));
        return;
    }
    
    if (m_settings.timing != timing) {
        m_settings.timing = timing;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        emit announcementTimingChanged(timing);
        logDebug(QString("Announcement timing changed to: %1").arg(announcementTimingToString(timing)));
    }
}

void AccessibilitySettingsService::setBrailleEnabled(bool enabled)
{
    if (m_settings.brailleEnabled != enabled) {
        m_settings.brailleEnabled = enabled;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        emit brailleSettingsChanged(enabled, m_settings.brailleFormat);
        logDebug(QString("Braille display %1").arg(enabled ? "enabled" : "disabled"));
    }
}

void AccessibilitySettingsService::setBrailleFormat(BrailleFormat format)
{
    if (!isValidBrailleFormat(format)) {
        logWarning(QString("Invalid braille format: %1").arg(static_cast<int>(format)));
        return;
    }
    
    if (m_settings.brailleFormat != format) {
        m_settings.brailleFormat = format;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        emit brailleSettingsChanged(m_settings.brailleEnabled, format);
        logDebug(QString("Braille format changed to: %1").arg(brailleFormatToString(format)));
    }
}

void AccessibilitySettingsService::setHighContrastMode(bool enabled)
{
    if (m_settings.highContrastMode != enabled) {
        m_settings.highContrastMode = enabled;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        emit highContrastModeChanged(enabled);
        logDebug(QString("High contrast mode %1").arg(enabled ? "enabled" : "disabled"));
    }
}

void AccessibilitySettingsService::setAnnouncementDelay(int delay)
{
    // Validate delay range
    int clampedDelay = qBound(MIN_ANNOUNCEMENT_DELAY, delay, MAX_ANNOUNCEMENT_DELAY);
    
    if (delay != clampedDelay) {
        logWarning(QString("Announcement delay %1 clamped to valid range [%2, %3]")
                  .arg(delay).arg(MIN_ANNOUNCEMENT_DELAY).arg(MAX_ANNOUNCEMENT_DELAY));
    }
    
    if (m_settings.announcementDelay != clampedDelay) {
        m_settings.announcementDelay = clampedDelay;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        logDebug(QString("Announcement delay changed to: %1ms").arg(clampedDelay));
    }
}

bool AccessibilitySettingsService::setCustomKeyboardShortcut(const QString& action, const QKeySequence& sequence)
{
    if (!m_availableShortcuts.contains(action)) {
        logWarning(QString("Unknown keyboard shortcut action: %1").arg(action));
        return false;
    }
    
    KeyboardShortcut shortcut = m_availableShortcuts.value(action);
    if (!shortcut.customizable) {
        logWarning(QString("Keyboard shortcut for action '%1' is not customizable").arg(action));
        return false;
    }
    
    // Check for conflicts
    QString conflictAction = checkKeySequenceConflict(sequence, action);
    if (!conflictAction.isEmpty()) {
        logWarning(QString("Keyboard shortcut conflict: sequence '%1' already used by action '%2'")
                  .arg(sequence.toString()).arg(conflictAction));
        return false;
    }
    
    // Update the shortcut
    shortcut.sequence = sequence;
    m_availableShortcuts[action] = shortcut;
    
    // Update custom shortcuts in settings
    if (sequence.isEmpty()) {
        m_settings.customShortcuts.remove(action);
    } else {
        m_settings.customShortcuts[action] = sequence;
    }
    
    m_settingsChanged = true;
    
    if (m_autoSaveTimer) {
        m_autoSaveTimer->start();
    }
    
    emit keyboardShortcutChanged(action, sequence);
    logDebug(QString("Keyboard shortcut for action '%1' changed to: %2")
            .arg(action).arg(sequence.toString()));
    
    return true;
}

bool AccessibilitySettingsService::resetKeyboardShortcut(const QString& action)
{
    if (!m_availableShortcuts.contains(action)) {
        logWarning(QString("Unknown keyboard shortcut action: %1").arg(action));
        return false;
    }
    
    if (!m_defaultShortcuts.contains(action)) {
        logWarning(QString("No default shortcut available for action: %1").arg(action));
        return false;
    }
    
    KeyboardShortcut defaultShortcut = m_defaultShortcuts.value(action);
    m_availableShortcuts[action] = defaultShortcut;
    
    // Remove from custom shortcuts
    m_settings.customShortcuts.remove(action);
    m_settingsChanged = true;
    
    if (m_autoSaveTimer) {
        m_autoSaveTimer->start();
    }
    
    emit keyboardShortcutChanged(action, defaultShortcut.sequence);
    logDebug(QString("Keyboard shortcut for action '%1' reset to default: %2")
            .arg(action).arg(defaultShortcut.sequence.toString()));
    
    return true;
}

void AccessibilitySettingsService::resetAllKeyboardShortcuts()
{
    m_availableShortcuts = m_defaultShortcuts;
    m_settings.customShortcuts.clear();
    m_settingsChanged = true;
    
    if (m_autoSaveTimer) {
        m_autoSaveTimer->start();
    }
    
    // Emit signals for all shortcuts
    for (auto it = m_availableShortcuts.constBegin(); it != m_availableShortcuts.constEnd(); ++it) {
        emit keyboardShortcutChanged(it.key(), it.value().sequence);
    }
    
    logDebug("All keyboard shortcuts reset to defaults");
}

void AccessibilitySettingsService::setBrailleDevice(const QString& device)
{
    if (m_settings.brailleDevice != device) {
        m_settings.brailleDevice = device;
        m_settingsChanged = true;
        
        if (m_autoSaveTimer) {
            m_autoSaveTimer->start();
        }
        
        logDebug(QString("Braille device changed to: %1").arg(device.isEmpty() ? "None" : device));
    }
}

bool AccessibilitySettingsService::applySettings(const AccessibilitySettings& settings)
{
    // Validate settings first
    QStringList errors = validateSettings(settings);
    if (!errors.isEmpty()) {
        emit settingsValidationFailed(errors);
        logWarning(QString("Settings validation failed: %1").arg(errors.join("; ")));
        return false;
    }
    
    AccessibilitySettings oldSettings = m_settings;
    m_settings = validateAndSanitizeSettings(settings);
    
    // Apply custom shortcuts
    for (auto it = m_settings.customShortcuts.constBegin(); it != m_settings.customShortcuts.constEnd(); ++it) {
        if (m_availableShortcuts.contains(it.key())) {
            KeyboardShortcut shortcut = m_availableShortcuts.value(it.key());
            shortcut.sequence = it.value();
            m_availableShortcuts[it.key()] = shortcut;
        }
    }
    
    m_settingsChanged = true;
    
    if (m_autoSaveTimer) {
        m_autoSaveTimer->start();
    }
    
    // Emit change signals
    emitChangeSignals(oldSettings, m_settings);
    
    logDebug("Applied new accessibility settings");
    return true;
}

bool AccessibilitySettingsService::loadSettings()
{
    if (!m_qsettings) {
        logError("QSettings not initialized");
        return false;
    }
    
    m_loadingSettings = true;
    
    try {
        m_qsettings->beginGroup(SETTINGS_GROUP);
        
        // Load basic settings
        m_settings.enabled = m_qsettings->value(SETTINGS_ENABLED, false).toBool();
        
        QString verbosityStr = m_qsettings->value(SETTINGS_VERBOSITY, "Normal").toString();
        m_settings.verbosity = verbosityLevelFromString(verbosityStr);
        
        QString timingStr = m_qsettings->value(SETTINGS_TIMING, "Immediate").toString();
        m_settings.timing = announcementTimingFromString(timingStr);
        
        m_settings.brailleEnabled = m_qsettings->value(SETTINGS_BRAILLE_ENABLED, false).toBool();
        
        QString brailleFormatStr = m_qsettings->value(SETTINGS_BRAILLE_FORMAT, "Standard").toString();
        m_settings.brailleFormat = brailleFormatFromString(brailleFormatStr);
        
        m_settings.highContrastMode = m_qsettings->value(SETTINGS_HIGH_CONTRAST, false).toBool();
        m_settings.announcementDelay = m_qsettings->value(SETTINGS_ANNOUNCEMENT_DELAY, 500).toInt();
        m_settings.interruptOnCritical = m_qsettings->value(SETTINGS_INTERRUPT_CRITICAL, true).toBool();
        m_settings.announceTooltips = m_qsettings->value(SETTINGS_ANNOUNCE_TOOLTIPS, true).toBool();
        m_settings.announceStateChanges = m_qsettings->value(SETTINGS_ANNOUNCE_STATE_CHANGES, true).toBool();
        m_settings.brailleDevice = m_qsettings->value(SETTINGS_BRAILLE_DEVICE, QString()).toString();
        
        // Load custom keyboard shortcuts
        m_qsettings->beginGroup(SETTINGS_CUSTOM_SHORTCUTS);
        QStringList shortcutKeys = m_qsettings->childKeys();
        m_settings.customShortcuts.clear();
        
        for (const QString& key : shortcutKeys) {
            QString sequenceStr = m_qsettings->value(key).toString();
            QKeySequence sequence = QKeySequence::fromString(sequenceStr);
            if (!sequence.isEmpty()) {
                m_settings.customShortcuts[key] = sequence;
                
                // Update available shortcuts
                if (m_availableShortcuts.contains(key)) {
                    KeyboardShortcut shortcut = m_availableShortcuts.value(key);
                    shortcut.sequence = sequence;
                    m_availableShortcuts[key] = shortcut;
                }
            }
        }
        
        m_qsettings->endGroup(); // SETTINGS_CUSTOM_SHORTCUTS
        m_qsettings->endGroup(); // SETTINGS_GROUP
        
        // Validate and sanitize loaded settings
        m_settings = validateAndSanitizeSettings(m_settings);
        
        m_settingsChanged = false;
        m_loadingSettings = false;
        
        emit settingsLoaded();
        logDebug("Accessibility settings loaded successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        m_loadingSettings = false;
        logError(QString("Failed to load accessibility settings: %1").arg(e.what()));
        return false;
    } catch (...) {
        m_loadingSettings = false;
        logError("Failed to load accessibility settings: Unknown error");
        return false;
    }
}

bool AccessibilitySettingsService::saveSettings()
{
    if (!m_qsettings) {
        logError("QSettings not initialized");
        return false;
    }
    
    try {
        m_qsettings->beginGroup(SETTINGS_GROUP);
        
        // Save basic settings
        m_qsettings->setValue(SETTINGS_ENABLED, m_settings.enabled);
        m_qsettings->setValue(SETTINGS_VERBOSITY, verbosityLevelToString(m_settings.verbosity));
        m_qsettings->setValue(SETTINGS_TIMING, announcementTimingToString(m_settings.timing));
        m_qsettings->setValue(SETTINGS_BRAILLE_ENABLED, m_settings.brailleEnabled);
        m_qsettings->setValue(SETTINGS_BRAILLE_FORMAT, brailleFormatToString(m_settings.brailleFormat));
        m_qsettings->setValue(SETTINGS_HIGH_CONTRAST, m_settings.highContrastMode);
        m_qsettings->setValue(SETTINGS_ANNOUNCEMENT_DELAY, m_settings.announcementDelay);
        m_qsettings->setValue(SETTINGS_INTERRUPT_CRITICAL, m_settings.interruptOnCritical);
        m_qsettings->setValue(SETTINGS_ANNOUNCE_TOOLTIPS, m_settings.announceTooltips);
        m_qsettings->setValue(SETTINGS_ANNOUNCE_STATE_CHANGES, m_settings.announceStateChanges);
        m_qsettings->setValue(SETTINGS_BRAILLE_DEVICE, m_settings.brailleDevice);
        
        // Save custom keyboard shortcuts
        m_qsettings->beginGroup(SETTINGS_CUSTOM_SHORTCUTS);
        m_qsettings->remove(""); // Clear existing shortcuts
        
        for (auto it = m_settings.customShortcuts.constBegin(); it != m_settings.customShortcuts.constEnd(); ++it) {
            m_qsettings->setValue(it.key(), it.value().toString());
        }
        
        m_qsettings->endGroup(); // SETTINGS_CUSTOM_SHORTCUTS
        m_qsettings->endGroup(); // SETTINGS_GROUP
        
        // Ensure settings are written to disk
        m_qsettings->sync();
        
        m_settingsChanged = false;
        
        emit settingsSaved();
        logDebug("Accessibility settings saved successfully");
        
        return true;
        
    } catch (const std::exception& e) {
        logError(QString("Failed to save accessibility settings: %1").arg(e.what()));
        return false;
    } catch (...) {
        logError("Failed to save accessibility settings: Unknown error");
        return false;
    }
}

void AccessibilitySettingsService::resetToDefaults()
{
    AccessibilitySettings oldSettings = m_settings;
    
    applyDefaultSettings();
    resetAllKeyboardShortcuts();
    
    m_settingsChanged = true;
    
    if (m_autoSaveTimer) {
        m_autoSaveTimer->start();
    }
    
    emitChangeSignals(oldSettings, m_settings);
    emit settingsReset();
    
    logDebug("Accessibility settings reset to defaults");
}

QStringList AccessibilitySettingsService::validateSettings(const AccessibilitySettings& settings) const
{
    QStringList errors;
    
    if (!isValidVerbosityLevel(settings.verbosity)) {
        errors << QString("Invalid verbosity level: %1").arg(static_cast<int>(settings.verbosity));
    }
    
    if (!isValidAnnouncementTiming(settings.timing)) {
        errors << QString("Invalid announcement timing: %1").arg(static_cast<int>(settings.timing));
    }
    
    if (!isValidBrailleFormat(settings.brailleFormat)) {
        errors << QString("Invalid braille format: %1").arg(static_cast<int>(settings.brailleFormat));
    }
    
    if (settings.announcementDelay < MIN_ANNOUNCEMENT_DELAY || settings.announcementDelay > MAX_ANNOUNCEMENT_DELAY) {
        errors << QString("Announcement delay %1 out of valid range [%2, %3]")
                  .arg(settings.announcementDelay).arg(MIN_ANNOUNCEMENT_DELAY).arg(MAX_ANNOUNCEMENT_DELAY);
    }
    
    // Validate custom shortcuts for conflicts
    QMap<QKeySequence, QString> sequenceMap;
    for (auto it = settings.customShortcuts.constBegin(); it != settings.customShortcuts.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            if (sequenceMap.contains(it.value())) {
                errors << QString("Keyboard shortcut conflict: sequence '%1' used by both '%2' and '%3'")
                          .arg(it.value().toString()).arg(sequenceMap.value(it.value())).arg(it.key());
            } else {
                sequenceMap[it.value()] = it.key();
            }
        }
    }
    
    return errors;
}

AccessibilitySettingsService::AccessibilitySettings AccessibilitySettingsService::validateAndSanitizeSettings(const AccessibilitySettings& settings) const
{
    AccessibilitySettings sanitized = settings;
    
    // Sanitize verbosity level
    if (!isValidVerbosityLevel(sanitized.verbosity)) {
        sanitized.verbosity = VerbosityLevel::Normal;
    }
    
    // Sanitize announcement timing
    if (!isValidAnnouncementTiming(sanitized.timing)) {
        sanitized.timing = AnnouncementTiming::Immediate;
    }
    
    // Sanitize braille format
    if (!isValidBrailleFormat(sanitized.brailleFormat)) {
        sanitized.brailleFormat = BrailleFormat::Standard;
    }
    
    // Sanitize announcement delay
    sanitized.announcementDelay = qBound(MIN_ANNOUNCEMENT_DELAY, sanitized.announcementDelay, MAX_ANNOUNCEMENT_DELAY);
    
    return sanitized;
}

void AccessibilitySettingsService::emitChangeSignals(const AccessibilitySettings& oldSettings, const AccessibilitySettings& newSettings)
{
    if (m_loadingSettings) {
        return; // Don't emit signals during loading
    }
    
    if (oldSettings.enabled != newSettings.enabled) {
        emit accessibilityEnabledChanged(newSettings.enabled);
    }
    
    if (oldSettings.verbosity != newSettings.verbosity) {
        emit verbosityLevelChanged(newSettings.verbosity);
    }
    
    if (oldSettings.timing != newSettings.timing) {
        emit announcementTimingChanged(newSettings.timing);
    }
    
    if (oldSettings.brailleEnabled != newSettings.brailleEnabled || 
        oldSettings.brailleFormat != newSettings.brailleFormat) {
        emit brailleSettingsChanged(newSettings.brailleEnabled, newSettings.brailleFormat);
    }
    
    if (oldSettings.highContrastMode != newSettings.highContrastMode) {
        emit highContrastModeChanged(newSettings.highContrastMode);
    }
}

void AccessibilitySettingsService::onAutoSaveTimeout()
{
    if (m_settingsChanged) {
        saveSettings();
    }
}

void AccessibilitySettingsService::onExternalSettingsChanged()
{
    // Reload settings from storage if they were changed externally
    if (!m_loadingSettings && !m_settingsChanged) {
        loadSettings();
    }
}

// Static validation methods

bool AccessibilitySettingsService::isValidVerbosityLevel(VerbosityLevel level)
{
    return level == VerbosityLevel::Terse || 
           level == VerbosityLevel::Normal || 
           level == VerbosityLevel::Verbose;
}

bool AccessibilitySettingsService::isValidAnnouncementTiming(AnnouncementTiming timing)
{
    return timing == AnnouncementTiming::Immediate || 
           timing == AnnouncementTiming::Delayed || 
           timing == AnnouncementTiming::OnRequest;
}

bool AccessibilitySettingsService::isValidBrailleFormat(BrailleFormat format)
{
    return format == BrailleFormat::Standard || 
           format == BrailleFormat::Compact || 
           format == BrailleFormat::Detailed;
}

// String conversion methods

QString AccessibilitySettingsService::verbosityLevelToString(VerbosityLevel level)
{
    switch (level) {
        case VerbosityLevel::Terse: return "Terse";
        case VerbosityLevel::Normal: return "Normal";
        case VerbosityLevel::Verbose: return "Verbose";
        default: return "Normal";
    }
}

AccessibilitySettingsService::VerbosityLevel AccessibilitySettingsService::verbosityLevelFromString(const QString& str)
{
    if (str.compare("Terse", Qt::CaseInsensitive) == 0) return VerbosityLevel::Terse;
    if (str.compare("Verbose", Qt::CaseInsensitive) == 0) return VerbosityLevel::Verbose;
    return VerbosityLevel::Normal; // Default
}

QString AccessibilitySettingsService::announcementTimingToString(AnnouncementTiming timing)
{
    switch (timing) {
        case AnnouncementTiming::Immediate: return "Immediate";
        case AnnouncementTiming::Delayed: return "Delayed";
        case AnnouncementTiming::OnRequest: return "OnRequest";
        default: return "Immediate";
    }
}

AccessibilitySettingsService::AnnouncementTiming AccessibilitySettingsService::announcementTimingFromString(const QString& str)
{
    if (str.compare("Delayed", Qt::CaseInsensitive) == 0) return AnnouncementTiming::Delayed;
    if (str.compare("OnRequest", Qt::CaseInsensitive) == 0) return AnnouncementTiming::OnRequest;
    return AnnouncementTiming::Immediate; // Default
}

QString AccessibilitySettingsService::brailleFormatToString(BrailleFormat format)
{
    switch (format) {
        case BrailleFormat::Standard: return "Standard";
        case BrailleFormat::Compact: return "Compact";
        case BrailleFormat::Detailed: return "Detailed";
        default: return "Standard";
    }
}

AccessibilitySettingsService::BrailleFormat AccessibilitySettingsService::brailleFormatFromString(const QString& str)
{
    if (str.compare("Compact", Qt::CaseInsensitive) == 0) return BrailleFormat::Compact;
    if (str.compare("Detailed", Qt::CaseInsensitive) == 0) return BrailleFormat::Detailed;
    return BrailleFormat::Standard; // Default
}

bool AccessibilitySettingsService::exportSettings(const QString& filePath) const
{
    try {
        QJsonObject settingsObj;
        settingsObj["enabled"] = m_settings.enabled;
        settingsObj["verbosity"] = verbosityLevelToString(m_settings.verbosity);
        settingsObj["timing"] = announcementTimingToString(m_settings.timing);
        settingsObj["brailleEnabled"] = m_settings.brailleEnabled;
        settingsObj["brailleFormat"] = brailleFormatToString(m_settings.brailleFormat);
        settingsObj["highContrastMode"] = m_settings.highContrastMode;
        settingsObj["announcementDelay"] = m_settings.announcementDelay;
        settingsObj["interruptOnCritical"] = m_settings.interruptOnCritical;
        settingsObj["announceTooltips"] = m_settings.announceTooltips;
        settingsObj["announceStateChanges"] = m_settings.announceStateChanges;
        settingsObj["brailleDevice"] = m_settings.brailleDevice;
        
        // Export custom shortcuts
        QJsonObject shortcutsObj;
        for (auto it = m_settings.customShortcuts.constBegin(); it != m_settings.customShortcuts.constEnd(); ++it) {
            shortcutsObj[it.key()] = it.value().toString();
        }
        settingsObj["customShortcuts"] = shortcutsObj;
        
        QJsonDocument doc(settingsObj);
        
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            logError(QString("Failed to open file for export: %1").arg(filePath));
            return false;
        }
        
        file.write(doc.toJson());
        file.close();
        
        logDebug(QString("Settings exported to: %1").arg(filePath));
        return true;
        
    } catch (const std::exception& e) {
        logError(QString("Failed to export settings: %1").arg(e.what()));
        return false;
    } catch (...) {
        logError("Failed to export settings: Unknown error");
        return false;
    }
}

bool AccessibilitySettingsService::importSettings(const QString& filePath)
{
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            logError(QString("Failed to open file for import: %1").arg(filePath));
            return false;
        }
        
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            logError(QString("Failed to parse JSON: %1").arg(error.errorString()));
            return false;
        }
        
        QJsonObject settingsObj = doc.object();
        
        AccessibilitySettings importedSettings;
        importedSettings.enabled = settingsObj["enabled"].toBool();
        importedSettings.verbosity = verbosityLevelFromString(settingsObj["verbosity"].toString());
        importedSettings.timing = announcementTimingFromString(settingsObj["timing"].toString());
        importedSettings.brailleEnabled = settingsObj["brailleEnabled"].toBool();
        importedSettings.brailleFormat = brailleFormatFromString(settingsObj["brailleFormat"].toString());
        importedSettings.highContrastMode = settingsObj["highContrastMode"].toBool();
        importedSettings.announcementDelay = settingsObj["announcementDelay"].toInt();
        importedSettings.interruptOnCritical = settingsObj["interruptOnCritical"].toBool();
        importedSettings.announceTooltips = settingsObj["announceTooltips"].toBool();
        importedSettings.announceStateChanges = settingsObj["announceStateChanges"].toBool();
        importedSettings.brailleDevice = settingsObj["brailleDevice"].toString();
        
        // Import custom shortcuts
        QJsonObject shortcutsObj = settingsObj["customShortcuts"].toObject();
        for (auto it = shortcutsObj.constBegin(); it != shortcutsObj.constEnd(); ++it) {
            QKeySequence sequence = QKeySequence::fromString(it.value().toString());
            if (!sequence.isEmpty()) {
                importedSettings.customShortcuts[it.key()] = sequence;
            }
        }
        
        // Apply imported settings
        bool success = applySettings(importedSettings);
        
        if (success) {
            logDebug(QString("Settings imported from: %1").arg(filePath));
        }
        
        return success;
        
    } catch (const std::exception& e) {
        logError(QString("Failed to import settings: %1").arg(e.what()));
        return false;
    } catch (...) {
        logError("Failed to import settings: Unknown error");
        return false;
    }
}