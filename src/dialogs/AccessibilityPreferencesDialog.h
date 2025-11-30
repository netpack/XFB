#ifndef ACCESSIBILITYPREFERENCESDIALOG_H
#define ACCESSIBILITYPREFERENCESDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QTextEdit>
#include <QProgressBar>
#include <QTimer>
#include <QMap>

#include "../services/AccessibilitySettingsService.h"

class BrailleDisplayService;



/**
 * @brief Dialog for configuring accessibility preferences
 * 
 * This dialog provides a comprehensive interface for users to configure
 * all accessibility settings including verbosity levels, keyboard shortcuts,
 * braille display options, and timing preferences. The dialog is designed
 * to be fully accessible itself, with proper labeling and keyboard navigation.
 * 
 * @example
 * @code
 * AccessibilityPreferencesDialog dialog(settingsService, this);
 * if (dialog.exec() == QDialog::Accepted) {
 *     // Settings have been applied
 * }
 * @endcode
 * 
 * @see AccessibilitySettingsService
 * @since XFB 2.0
 */
class AccessibilityPreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccessibilityPreferencesDialog(AccessibilitySettingsService* settingsService, 
                                          BrailleDisplayService* brailleService = nullptr,
                                          QWidget* parent = nullptr);
    ~AccessibilityPreferencesDialog() override;

    /**
     * @brief Load current settings into the dialog
     */
    void loadSettings();

    /**
     * @brief Apply settings from the dialog
     * @return true if settings were applied successfully
     */
    bool applySettings();

    /**
     * @brief Reset all settings to defaults
     */
    void resetToDefaults();

public slots:
    /**
     * @brief Handle OK button click
     */
    void accept() override;

    /**
     * @brief Handle Apply button click
     */
    void onApplyClicked();

    /**
     * @brief Handle Reset button click
     */
    void onResetClicked();

    /**
     * @brief Handle Cancel button click
     */
    void reject() override;

    /**
     * @brief Handle Test Announcement button click
     */
    void onTestAnnouncementClicked();

    /**
     * @brief Handle Import Settings button click
     */
    void onImportSettingsClicked();

    /**
     * @brief Handle Export Settings button click
     */
    void onExportSettingsClicked();

private slots:
    /**
     * @brief Handle verbosity level change
     */
    void onVerbosityLevelChanged();

    /**
     * @brief Handle announcement timing change
     */
    void onAnnouncementTimingChanged();

    /**
     * @brief Handle braille enabled state change
     */
    void onBrailleEnabledChanged();

    /**
     * @brief Handle braille format change
     */
    void onBrailleFormatChanged();

    /**
     * @brief Handle keyboard shortcut selection change
     */
    void onShortcutSelectionChanged();

    /**
     * @brief Handle keyboard shortcut edit
     */
    void onShortcutEdited();

    /**
     * @brief Handle reset single shortcut
     */
    void onResetShortcutClicked();

    /**
     * @brief Handle reset all shortcuts
     */
    void onResetAllShortcutsClicked();

    /**
     * @brief Handle braille device detection
     */
    void onDetectBrailleDeviceClicked();

    /**
     * @brief Handle braille device detection completion
     */
    void onBrailleDeviceDetected();

    /**
     * @brief Handle settings validation
     */
    void onSettingsChanged();

private:
    /**
     * @brief Setup the user interface
     */
    void setupUI();

    /**
     * @brief Setup the general settings tab
     */
    void setupGeneralTab();

    /**
     * @brief Setup the keyboard shortcuts tab
     */
    void setupKeyboardShortcutsTab();

    /**
     * @brief Setup the braille settings tab
     */
    void setupBrailleTab();

    /**
     * @brief Setup the advanced settings tab
     */
    void setupAdvancedTab();

    /**
     * @brief Setup dialog buttons
     */
    void setupButtons();

    /**
     * @brief Setup accessibility enhancements for the dialog itself
     */
    void setupAccessibility();

    /**
     * @brief Connect all signals and slots
     */
    void connectSignals();

    /**
     * @brief Populate keyboard shortcuts list
     */
    void populateKeyboardShortcuts();

    /**
     * @brief Update keyboard shortcut display
     */
    void updateShortcutDisplay();

    /**
     * @brief Validate current settings
     * @return true if settings are valid
     */
    bool validateSettings();

    /**
     * @brief Show validation errors
     * @param errors List of validation errors
     */
    void showValidationErrors(const QStringList& errors);

    /**
     * @brief Update braille device list
     */
    void updateBrailleDeviceList();

    /**
     * @brief Update diagnostics information display
     */
    void updateDiagnosticsInfo();

    /**
     * @brief Enable or disable braille controls
     * @param enabled true to enable braille controls
     */
    void setBrailleControlsEnabled(bool enabled);

    /**
     * @brief Update UI state based on current settings
     */
    void updateUIState();

    /**
     * @brief Get current settings from UI
     * @return Settings structure with current UI values
     */
    AccessibilitySettingsService::AccessibilitySettings getCurrentUISettings() const;

    // UI Components
    QTabWidget* m_tabWidget;
    
    // General Tab
    QWidget* m_generalTab;
    QCheckBox* m_enableAccessibilityCheckBox;
    QComboBox* m_verbosityComboBox;
    QComboBox* m_timingComboBox;
    QSpinBox* m_delaySpinBox;
    QCheckBox* m_interruptCriticalCheckBox;
    QCheckBox* m_announceTooltipsCheckBox;
    QCheckBox* m_announceStateChangesCheckBox;
    QCheckBox* m_highContrastCheckBox;
    QPushButton* m_testAnnouncementButton;
    
    // Keyboard Shortcuts Tab
    QWidget* m_keyboardTab;
    QListWidget* m_shortcutsList;
    QKeySequenceEdit* m_shortcutEdit;
    QPushButton* m_resetShortcutButton;
    QPushButton* m_resetAllShortcutsButton;
    QLabel* m_shortcutDescriptionLabel;
    QLabel* m_shortcutContextLabel;
    QLabel* m_conflictWarningLabel;
    
    // Braille Tab
    QWidget* m_brailleTab;
    QCheckBox* m_brailleEnabledCheckBox;
    QComboBox* m_brailleFormatComboBox;
    QComboBox* m_brailleDeviceComboBox;
    QPushButton* m_detectBrailleButton;
    QProgressBar* m_brailleDetectionProgress;
    QTextEdit* m_braillePreviewText;
    QTimer* m_brailleDetectionTimer;
    
    // Advanced Tab
    QWidget* m_advancedTab;
    QPushButton* m_importButton;
    QPushButton* m_exportButton;
    QTextEdit* m_diagnosticsText;
    
    // Dialog Buttons
    QPushButton* m_okButton;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QPushButton* m_resetButton;
    
    // Data
    AccessibilitySettingsService* m_settingsService;
    BrailleDisplayService* m_brailleService;
    QMap<QString, AccessibilitySettingsService::KeyboardShortcut> m_currentShortcuts;
    bool m_settingsChanged;
    bool m_loadingSettings;
    
    // Constants
    static constexpr int BRAILLE_DETECTION_TIMEOUT_MS = 5000;
};

#endif // ACCESSIBILITYPREFERENCESDIALOG_H