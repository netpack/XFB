#include "AccessibilityPreferencesDialog.h"
#include "../services/AccessibilitySettingsService.h"
#include "../services/BrailleDisplayService.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QAccessible>
#include <QStyle>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QFrame>
#include <QScrollArea>
#include <QDebug>

AccessibilityPreferencesDialog::AccessibilityPreferencesDialog(AccessibilitySettingsService* settingsService, BrailleDisplayService* brailleService, QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_settingsService(settingsService)
    , m_brailleService(brailleService)
    , m_settingsChanged(false)
    , m_loadingSettings(false)
    , m_brailleDetectionTimer(new QTimer(this))
{
    setWindowTitle(tr("Accessibility Preferences"));
    setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    setModal(true);
    resize(800, 600);
    
    // Setup UI
    setupUI();
    setupAccessibility();
    connectSignals();
    
    // Load current settings
    loadSettings();
    
    // Setup braille detection timer
    m_brailleDetectionTimer->setSingleShot(true);
    m_brailleDetectionTimer->setInterval(BRAILLE_DETECTION_TIMEOUT_MS);
    connect(m_brailleDetectionTimer, &QTimer::timeout, this, &AccessibilityPreferencesDialog::onBrailleDeviceDetected);
}

AccessibilityPreferencesDialog::~AccessibilityPreferencesDialog()
{
    // Destructor - Qt handles cleanup of child widgets
}

void AccessibilityPreferencesDialog::setupUI()
{
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);
    
    // Setup tabs
    setupGeneralTab();
    setupKeyboardShortcutsTab();
    setupBrailleTab();
    setupAdvancedTab();
    
    // Setup buttons
    setupButtons();
    
    // Add buttons to layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
}

void AccessibilityPreferencesDialog::setupGeneralTab()
{
    m_generalTab = new QWidget();
    m_tabWidget->addTab(m_generalTab, tr("&General"));
    
    QVBoxLayout* layout = new QVBoxLayout(m_generalTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);
    
    // Accessibility Enable Group
    QGroupBox* enableGroup = new QGroupBox(tr("Accessibility Features"), m_generalTab);
    QVBoxLayout* enableLayout = new QVBoxLayout(enableGroup);
    
    m_enableAccessibilityCheckBox = new QCheckBox(tr("&Enable accessibility features"), enableGroup);
    m_enableAccessibilityCheckBox->setToolTip(tr("Enable or disable all accessibility features including screen reader support"));
    enableLayout->addWidget(m_enableAccessibilityCheckBox);
    
    layout->addWidget(enableGroup);
    
    // Verbosity Group
    QGroupBox* verbosityGroup = new QGroupBox(tr("Announcement Verbosity"), m_generalTab);
    QFormLayout* verbosityLayout = new QFormLayout(verbosityGroup);
    
    m_verbosityComboBox = new QComboBox(verbosityGroup);
    m_verbosityComboBox->addItem(tr("Terse - Control type and name only"), static_cast<int>(AccessibilitySettingsService::VerbosityLevel::Terse));
    m_verbosityComboBox->addItem(tr("Normal - Standard information level"), static_cast<int>(AccessibilitySettingsService::VerbosityLevel::Normal));
    m_verbosityComboBox->addItem(tr("Verbose - Including hints and descriptions"), static_cast<int>(AccessibilitySettingsService::VerbosityLevel::Verbose));
    m_verbosityComboBox->setToolTip(tr("Choose how much information is announced by the screen reader"));
    
    verbosityLayout->addRow(tr("&Verbosity level:"), m_verbosityComboBox);
    layout->addWidget(verbosityGroup);
    
    // Timing Group
    QGroupBox* timingGroup = new QGroupBox(tr("Announcement Timing"), m_generalTab);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);
    
    m_timingComboBox = new QComboBox(timingGroup);
    m_timingComboBox->addItem(tr("Immediate - Announce right away"), static_cast<int>(AccessibilitySettingsService::AnnouncementTiming::Immediate));
    m_timingComboBox->addItem(tr("Delayed - Brief delay to avoid overwhelming"), static_cast<int>(AccessibilitySettingsService::AnnouncementTiming::Delayed));
    m_timingComboBox->addItem(tr("On Request - Only when explicitly requested"), static_cast<int>(AccessibilitySettingsService::AnnouncementTiming::OnRequest));
    m_timingComboBox->setToolTip(tr("Choose when announcements are made"));
    
    timingLayout->addRow(tr("&Timing:"), m_timingComboBox);
    
    m_delaySpinBox = new QSpinBox(timingGroup);
    m_delaySpinBox->setRange(0, 5000);
    m_delaySpinBox->setSuffix(tr(" ms"));
    m_delaySpinBox->setToolTip(tr("Delay in milliseconds for delayed timing mode"));
    
    timingLayout->addRow(tr("&Delay:"), m_delaySpinBox);
    layout->addWidget(timingGroup);
    
    // Behavior Group
    QGroupBox* behaviorGroup = new QGroupBox(tr("Announcement Behavior"), m_generalTab);
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);
    
    m_interruptCriticalCheckBox = new QCheckBox(tr("&Interrupt announcements for critical alerts"), behaviorGroup);
    m_interruptCriticalCheckBox->setToolTip(tr("Allow critical alerts to interrupt ongoing announcements"));
    behaviorLayout->addWidget(m_interruptCriticalCheckBox);
    
    m_announceTooltipsCheckBox = new QCheckBox(tr("Announce &tooltip content"), behaviorGroup);
    m_announceTooltipsCheckBox->setToolTip(tr("Announce tooltip text when hovering over controls"));
    behaviorLayout->addWidget(m_announceTooltipsCheckBox);
    
    m_announceStateChangesCheckBox = new QCheckBox(tr("Announce &state changes"), behaviorGroup);
    m_announceStateChangesCheckBox->setToolTip(tr("Announce when control states change (checked, expanded, etc.)"));
    behaviorLayout->addWidget(m_announceStateChangesCheckBox);
    
    layout->addWidget(behaviorGroup);
    
    // Visual Group
    QGroupBox* visualGroup = new QGroupBox(tr("Visual Accessibility"), m_generalTab);
    QVBoxLayout* visualLayout = new QVBoxLayout(visualGroup);
    
    m_highContrastCheckBox = new QCheckBox(tr("Enable &high contrast mode"), visualGroup);
    m_highContrastCheckBox->setToolTip(tr("Use high contrast colors for better visibility"));
    visualLayout->addWidget(m_highContrastCheckBox);
    
    layout->addWidget(visualGroup);
    
    // Test Group
    QGroupBox* testGroup = new QGroupBox(tr("Testing"), m_generalTab);
    QHBoxLayout* testLayout = new QHBoxLayout(testGroup);
    
    m_testAnnouncementButton = new QPushButton(tr("&Test Announcement"), testGroup);
    m_testAnnouncementButton->setToolTip(tr("Test the current announcement settings"));
    testLayout->addWidget(m_testAnnouncementButton);
    testLayout->addStretch();
    
    layout->addWidget(testGroup);
    
    layout->addStretch();
}

void AccessibilityPreferencesDialog::setupKeyboardShortcutsTab()
{
    m_keyboardTab = new QWidget();
    m_tabWidget->addTab(m_keyboardTab, tr("&Keyboard Shortcuts"));
    
    QHBoxLayout* layout = new QHBoxLayout(m_keyboardTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);
    
    // Left side - shortcuts list
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    QLabel* listLabel = new QLabel(tr("Available &shortcuts:"), m_keyboardTab);
    leftLayout->addWidget(listLabel);
    
    m_shortcutsList = new QListWidget(m_keyboardTab);
    m_shortcutsList->setToolTip(tr("List of available keyboard shortcuts"));
    listLabel->setBuddy(m_shortcutsList);
    leftLayout->addWidget(m_shortcutsList);
    
    layout->addLayout(leftLayout, 1);
    
    // Right side - shortcut editing
    QVBoxLayout* rightLayout = new QVBoxLayout();
    
    QGroupBox* editGroup = new QGroupBox(tr("Edit Shortcut"), m_keyboardTab);
    QFormLayout* editLayout = new QFormLayout(editGroup);
    
    m_shortcutDescriptionLabel = new QLabel(tr("No shortcut selected"), editGroup);
    m_shortcutDescriptionLabel->setWordWrap(true);
    editLayout->addRow(tr("Description:"), m_shortcutDescriptionLabel);
    
    m_shortcutContextLabel = new QLabel(tr(""), editGroup);
    editLayout->addRow(tr("Context:"), m_shortcutContextLabel);
    
    m_shortcutEdit = new QKeySequenceEdit(editGroup);
    m_shortcutEdit->setToolTip(tr("Press the key combination you want to assign"));
    editLayout->addRow(tr("&Key sequence:"), m_shortcutEdit);
    
    m_conflictWarningLabel = new QLabel(editGroup);
    m_conflictWarningLabel->setStyleSheet("color: red; font-weight: bold;");
    m_conflictWarningLabel->setWordWrap(true);
    m_conflictWarningLabel->hide();
    editLayout->addRow(m_conflictWarningLabel);
    
    rightLayout->addWidget(editGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_resetShortcutButton = new QPushButton(tr("&Reset to Default"), m_keyboardTab);
    m_resetShortcutButton->setToolTip(tr("Reset the selected shortcut to its default value"));
    m_resetShortcutButton->setEnabled(false);
    buttonLayout->addWidget(m_resetShortcutButton);
    
    buttonLayout->addStretch();
    
    m_resetAllShortcutsButton = new QPushButton(tr("Reset &All"), m_keyboardTab);
    m_resetAllShortcutsButton->setToolTip(tr("Reset all shortcuts to their default values"));
    buttonLayout->addWidget(m_resetAllShortcutsButton);
    
    rightLayout->addLayout(buttonLayout);
    rightLayout->addStretch();
    
    layout->addLayout(rightLayout, 1);
}

void AccessibilityPreferencesDialog::setupBrailleTab()
{
    m_brailleTab = new QWidget();
    m_tabWidget->addTab(m_brailleTab, tr("&Braille Display"));
    
    QVBoxLayout* layout = new QVBoxLayout(m_brailleTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);
    
    // Enable Group
    QGroupBox* enableGroup = new QGroupBox(tr("Braille Display Support"), m_brailleTab);
    QVBoxLayout* enableLayout = new QVBoxLayout(enableGroup);
    
    m_brailleEnabledCheckBox = new QCheckBox(tr("&Enable braille display support"), enableGroup);
    m_brailleEnabledCheckBox->setToolTip(tr("Enable support for braille displays"));
    enableLayout->addWidget(m_brailleEnabledCheckBox);
    
    layout->addWidget(enableGroup);
    
    // Configuration Group
    QGroupBox* configGroup = new QGroupBox(tr("Braille Configuration"), m_brailleTab);
    QFormLayout* configLayout = new QFormLayout(configGroup);
    
    m_brailleFormatComboBox = new QComboBox(configGroup);
    m_brailleFormatComboBox->addItem(tr("Standard - Standard braille formatting"), static_cast<int>(AccessibilitySettingsService::BrailleFormat::Standard));
    m_brailleFormatComboBox->addItem(tr("Compact - Compact format with abbreviations"), static_cast<int>(AccessibilitySettingsService::BrailleFormat::Compact));
    m_brailleFormatComboBox->addItem(tr("Detailed - Detailed format with full descriptions"), static_cast<int>(AccessibilitySettingsService::BrailleFormat::Detailed));
    m_brailleFormatComboBox->setToolTip(tr("Choose the braille formatting style"));
    
    configLayout->addRow(tr("&Format:"), m_brailleFormatComboBox);
    
    // Device selection
    QHBoxLayout* deviceLayout = new QHBoxLayout();
    
    m_brailleDeviceComboBox = new QComboBox(configGroup);
    m_brailleDeviceComboBox->setEditable(true);
    m_brailleDeviceComboBox->setToolTip(tr("Select or enter the braille device identifier"));
    deviceLayout->addWidget(m_brailleDeviceComboBox);
    
    m_detectBrailleButton = new QPushButton(tr("&Detect"), configGroup);
    m_detectBrailleButton->setToolTip(tr("Automatically detect connected braille displays"));
    deviceLayout->addWidget(m_detectBrailleButton);
    
    configLayout->addRow(tr("&Device:"), deviceLayout);
    
    // Detection progress
    m_brailleDetectionProgress = new QProgressBar(configGroup);
    m_brailleDetectionProgress->setVisible(false);
    configLayout->addRow(m_brailleDetectionProgress);
    
    layout->addWidget(configGroup);
    
    // Preview Group
    QGroupBox* previewGroup = new QGroupBox(tr("Braille Preview"), m_brailleTab);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    
    QLabel* previewLabel = new QLabel(tr("Preview of braille output:"), previewGroup);
    previewLayout->addWidget(previewLabel);
    
    m_braillePreviewText = new QTextEdit(previewGroup);
    m_braillePreviewText->setReadOnly(true);
    m_braillePreviewText->setMaximumHeight(100);
    m_braillePreviewText->setToolTip(tr("Preview of how text will appear on the braille display"));
    m_braillePreviewText->setPlainText(tr("Sample text: XFB Radio Broadcasting Software - Main Window"));
    previewLayout->addWidget(m_braillePreviewText);
    
    layout->addWidget(previewGroup);
    
    layout->addStretch();
}

void AccessibilityPreferencesDialog::setupAdvancedTab()
{
    m_advancedTab = new QWidget();
    m_tabWidget->addTab(m_advancedTab, tr("&Advanced"));
    
    QVBoxLayout* layout = new QVBoxLayout(m_advancedTab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);
    
    // Import/Export Group
    QGroupBox* importExportGroup = new QGroupBox(tr("Settings Management"), m_advancedTab);
    QHBoxLayout* importExportLayout = new QHBoxLayout(importExportGroup);
    
    m_importButton = new QPushButton(tr("&Import Settings..."), importExportGroup);
    m_importButton->setToolTip(tr("Import accessibility settings from a file"));
    importExportLayout->addWidget(m_importButton);
    
    m_exportButton = new QPushButton(tr("&Export Settings..."), importExportGroup);
    m_exportButton->setToolTip(tr("Export current accessibility settings to a file"));
    importExportLayout->addWidget(m_exportButton);
    
    importExportLayout->addStretch();
    
    layout->addWidget(importExportGroup);
    
    // Diagnostics Group
    QGroupBox* diagnosticsGroup = new QGroupBox(tr("Accessibility Diagnostics"), m_advancedTab);
    QVBoxLayout* diagnosticsLayout = new QVBoxLayout(diagnosticsGroup);
    
    QLabel* diagnosticsLabel = new QLabel(tr("System accessibility information:"), diagnosticsGroup);
    diagnosticsLayout->addWidget(diagnosticsLabel);
    
    m_diagnosticsText = new QTextEdit(diagnosticsGroup);
    m_diagnosticsText->setReadOnly(true);
    m_diagnosticsText->setToolTip(tr("Information about the accessibility system status"));
    diagnosticsLayout->addWidget(m_diagnosticsText);
    
    layout->addWidget(diagnosticsGroup);
    
    // Update diagnostics
    updateDiagnosticsInfo();
}

void AccessibilityPreferencesDialog::setupButtons()
{
    m_okButton = new QPushButton(tr("&OK"), this);
    m_okButton->setDefault(true);
    m_okButton->setToolTip(tr("Apply settings and close dialog"));
    
    m_applyButton = new QPushButton(tr("&Apply"), this);
    m_applyButton->setToolTip(tr("Apply settings without closing dialog"));
    
    m_cancelButton = new QPushButton(tr("&Cancel"), this);
    m_cancelButton->setToolTip(tr("Close dialog without applying changes"));
    
    m_resetButton = new QPushButton(tr("&Reset to Defaults"), this);
    m_resetButton->setToolTip(tr("Reset all settings to their default values"));
}

void AccessibilityPreferencesDialog::setupAccessibility()
{
    // Set accessible names and descriptions for all controls
    setAccessibleName(tr("Accessibility Preferences Dialog"));
    setAccessibleDescription(tr("Configure accessibility settings for XFB Radio Broadcasting Software"));
    
    // Tab widget
    m_tabWidget->setAccessibleName(tr("Settings Categories"));
    m_tabWidget->setAccessibleDescription(tr("Choose a category to configure accessibility settings"));
    
    // Set tab order for proper keyboard navigation
    setTabOrder(m_tabWidget, m_okButton);
    setTabOrder(m_okButton, m_applyButton);
    setTabOrder(m_applyButton, m_cancelButton);
    setTabOrder(m_cancelButton, m_resetButton);
}

void AccessibilityPreferencesDialog::connectSignals()
{
    // Button connections
    connect(m_okButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::accept);
    connect(m_applyButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::reject);
    connect(m_resetButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onResetClicked);
    
    // General tab connections
    connect(m_enableAccessibilityCheckBox, &QCheckBox::toggled, this, &AccessibilityPreferencesDialog::onSettingsChanged);
    connect(m_verbosityComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AccessibilityPreferencesDialog::onVerbosityLevelChanged);
    connect(m_timingComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AccessibilityPreferencesDialog::onAnnouncementTimingChanged);
    connect(m_delaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AccessibilityPreferencesDialog::onSettingsChanged);
    connect(m_interruptCriticalCheckBox, &QCheckBox::toggled, this, &AccessibilityPreferencesDialog::onSettingsChanged);
    connect(m_announceTooltipsCheckBox, &QCheckBox::toggled, this, &AccessibilityPreferencesDialog::onSettingsChanged);
    connect(m_announceStateChangesCheckBox, &QCheckBox::toggled, this, &AccessibilityPreferencesDialog::onSettingsChanged);
    connect(m_highContrastCheckBox, &QCheckBox::toggled, this, &AccessibilityPreferencesDialog::onSettingsChanged);
    connect(m_testAnnouncementButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onTestAnnouncementClicked);
    
    // Keyboard shortcuts tab connections
    connect(m_shortcutsList, &QListWidget::currentRowChanged, this, &AccessibilityPreferencesDialog::onShortcutSelectionChanged);
    connect(m_shortcutEdit, &QKeySequenceEdit::keySequenceChanged, this, &AccessibilityPreferencesDialog::onShortcutEdited);
    connect(m_resetShortcutButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onResetShortcutClicked);
    connect(m_resetAllShortcutsButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onResetAllShortcutsClicked);
    
    // Braille tab connections
    connect(m_brailleEnabledCheckBox, &QCheckBox::toggled, this, &AccessibilityPreferencesDialog::onBrailleEnabledChanged);
    connect(m_brailleFormatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AccessibilityPreferencesDialog::onBrailleFormatChanged);
    connect(m_detectBrailleButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onDetectBrailleDeviceClicked);
    
    // Advanced tab connections
    connect(m_importButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onImportSettingsClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &AccessibilityPreferencesDialog::onExportSettingsClicked);
}

void AccessibilityPreferencesDialog::loadSettings()
{
    if (!m_settingsService) {
        return;
    }
    
    m_loadingSettings = true;
    
    auto settings = m_settingsService->settings();
    
    // Load general settings
    m_enableAccessibilityCheckBox->setChecked(settings.enabled);
    
    // Set verbosity level
    int verbosityIndex = m_verbosityComboBox->findData(static_cast<int>(settings.verbosity));
    if (verbosityIndex >= 0) {
        m_verbosityComboBox->setCurrentIndex(verbosityIndex);
    }
    
    // Set timing
    int timingIndex = m_timingComboBox->findData(static_cast<int>(settings.timing));
    if (timingIndex >= 0) {
        m_timingComboBox->setCurrentIndex(timingIndex);
    }
    
    m_delaySpinBox->setValue(settings.announcementDelay);
    m_interruptCriticalCheckBox->setChecked(settings.interruptOnCritical);
    m_announceTooltipsCheckBox->setChecked(settings.announceTooltips);
    m_announceStateChangesCheckBox->setChecked(settings.announceStateChanges);
    m_highContrastCheckBox->setChecked(settings.highContrastMode);
    
    // Load braille settings
    m_brailleEnabledCheckBox->setChecked(settings.brailleEnabled);
    
    int brailleFormatIndex = m_brailleFormatComboBox->findData(static_cast<int>(settings.brailleFormat));
    if (brailleFormatIndex >= 0) {
        m_brailleFormatComboBox->setCurrentIndex(brailleFormatIndex);
    }
    
    m_brailleDeviceComboBox->setCurrentText(settings.brailleDevice);
    
    // Load keyboard shortcuts
    populateKeyboardShortcuts();
    
    // Update UI state
    updateUIState();
    
    m_settingsChanged = false;
    m_loadingSettings = false;
}

void AccessibilityPreferencesDialog::populateKeyboardShortcuts()
{
    if (!m_settingsService) {
        return;
    }
    
    m_shortcutsList->clear();
    m_currentShortcuts = m_settingsService->availableKeyboardShortcuts();
    
    // Group shortcuts by context
    QMap<QString, QStringList> contextGroups;
    
    for (auto it = m_currentShortcuts.constBegin(); it != m_currentShortcuts.constEnd(); ++it) {
        QString context = it.value().context.isEmpty() ? tr("General") : it.value().context;
        contextGroups[context].append(it.key());
    }
    
    // Add shortcuts to list grouped by context
    for (auto contextIt = contextGroups.constBegin(); contextIt != contextGroups.constEnd(); ++contextIt) {
        // Add context header
        QListWidgetItem* headerItem = new QListWidgetItem(QString("--- %1 ---").arg(contextIt.key()));
        headerItem->setFlags(Qt::ItemIsEnabled); // Not selectable
        headerItem->setData(Qt::UserRole, QString()); // Empty action name
        QFont headerFont = headerItem->font();
        headerFont.setBold(true);
        headerItem->setFont(headerFont);
        m_shortcutsList->addItem(headerItem);
        
        // Add shortcuts in this context
        for (const QString& action : contextIt.value()) {
            const auto& shortcut = m_currentShortcuts.value(action);
            QString displayText = QString("%1 - %2").arg(shortcut.description).arg(shortcut.sequence.toString());
            
            QListWidgetItem* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, action);
            item->setToolTip(shortcut.description);
            m_shortcutsList->addItem(item);
        }
    }
}

void AccessibilityPreferencesDialog::updateShortcutDisplay()
{
    int currentRow = m_shortcutsList->currentRow();
    if (currentRow < 0) {
        return;
    }
    
    QListWidgetItem* item = m_shortcutsList->item(currentRow);
    if (!item) {
        return;
    }
    
    QString action = item->data(Qt::UserRole).toString();
    if (action.isEmpty()) {
        return; // Header item
    }
    
    const auto& shortcut = m_currentShortcuts.value(action);
    QString displayText = QString("%1 - %2").arg(shortcut.description).arg(shortcut.sequence.toString());
    item->setText(displayText);
}

bool AccessibilityPreferencesDialog::applySettings()
{
    if (!m_settingsService) {
        return false;
    }
    
    if (!validateSettings()) {
        return false;
    }
    
    auto settings = getCurrentUISettings();
    
    if (!m_settingsService->applySettings(settings)) {
        QMessageBox::warning(this, tr("Settings Error"), 
                           tr("Failed to apply accessibility settings. Please check your configuration."));
        return false;
    }
    
    m_settingsChanged = false;
    return true;
}

AccessibilitySettingsService::AccessibilitySettings AccessibilityPreferencesDialog::getCurrentUISettings() const
{
    AccessibilitySettingsService::AccessibilitySettings settings;
    
    // General settings
    settings.enabled = m_enableAccessibilityCheckBox->isChecked();
    settings.verbosity = static_cast<AccessibilitySettingsService::VerbosityLevel>(
        m_verbosityComboBox->currentData().toInt());
    settings.timing = static_cast<AccessibilitySettingsService::AnnouncementTiming>(
        m_timingComboBox->currentData().toInt());
    settings.announcementDelay = m_delaySpinBox->value();
    settings.interruptOnCritical = m_interruptCriticalCheckBox->isChecked();
    settings.announceTooltips = m_announceTooltipsCheckBox->isChecked();
    settings.announceStateChanges = m_announceStateChangesCheckBox->isChecked();
    settings.highContrastMode = m_highContrastCheckBox->isChecked();
    
    // Braille settings
    settings.brailleEnabled = m_brailleEnabledCheckBox->isChecked();
    settings.brailleFormat = static_cast<AccessibilitySettingsService::BrailleFormat>(
        m_brailleFormatComboBox->currentData().toInt());
    settings.brailleDevice = m_brailleDeviceComboBox->currentText();
    
    // Custom shortcuts
    for (auto it = m_currentShortcuts.constBegin(); it != m_currentShortcuts.constEnd(); ++it) {
        settings.customShortcuts[it.key()] = it.value().sequence;
    }
    
    return settings;
}

bool AccessibilityPreferencesDialog::validateSettings()
{
    auto settings = getCurrentUISettings();
    QStringList errors = m_settingsService->validateSettings(settings);
    
    if (!errors.isEmpty()) {
        showValidationErrors(errors);
        return false;
    }
    
    return true;
}

void AccessibilityPreferencesDialog::showValidationErrors(const QStringList& errors)
{
    QString message = tr("The following validation errors were found:\n\n");
    message += errors.join("\n");
    
    QMessageBox::warning(this, tr("Validation Errors"), message);
}

void AccessibilityPreferencesDialog::updateUIState()
{
    bool accessibilityEnabled = m_enableAccessibilityCheckBox->isChecked();
    
    // Enable/disable controls based on accessibility state
    m_verbosityComboBox->setEnabled(accessibilityEnabled);
    m_timingComboBox->setEnabled(accessibilityEnabled);
    m_delaySpinBox->setEnabled(accessibilityEnabled && 
        m_timingComboBox->currentData().toInt() == static_cast<int>(AccessibilitySettingsService::AnnouncementTiming::Delayed));
    m_interruptCriticalCheckBox->setEnabled(accessibilityEnabled);
    m_announceTooltipsCheckBox->setEnabled(accessibilityEnabled);
    m_announceStateChangesCheckBox->setEnabled(accessibilityEnabled);
    m_testAnnouncementButton->setEnabled(accessibilityEnabled);
    
    // Braille controls
    setBrailleControlsEnabled(accessibilityEnabled && m_brailleEnabledCheckBox->isChecked());
    
    // Update apply button state
    m_applyButton->setEnabled(m_settingsChanged);
}

void AccessibilityPreferencesDialog::setBrailleControlsEnabled(bool enabled)
{
    m_brailleFormatComboBox->setEnabled(enabled);
    m_brailleDeviceComboBox->setEnabled(enabled);
    m_detectBrailleButton->setEnabled(enabled);
    m_braillePreviewText->setEnabled(enabled);
}

void AccessibilityPreferencesDialog::updateDiagnosticsInfo()
{
    QString diagnostics;
    
    diagnostics += tr("Accessibility System Information:\n");
    diagnostics += tr("Qt Accessibility: %1\n").arg(QAccessible::isActive() ? tr("Available") : tr("Not Available"));
    diagnostics += tr("Platform: %1\n").arg(QApplication::platformName());
    
    // Add more diagnostic information as needed
    diagnostics += tr("\nAccessibility Features:\n");
    diagnostics += tr("Screen Reader Support: Available\n");
    diagnostics += tr("Keyboard Navigation: Available\n");
    diagnostics += tr("High Contrast: Available\n");
    
    // Braille display information
    if (m_brailleService) {
        diagnostics += tr("Braille Display Service: Available\n");
        diagnostics += tr("Braille System Support: %1\n").arg(m_brailleService->isBrailleSupported() ? tr("Yes") : tr("No"));
        diagnostics += tr("Connected Braille Display: %1\n").arg(m_brailleService->isBrailleDisplayConnected() ? tr("Yes") : tr("No"));
        diagnostics += tr("Available Braille Devices: %1\n").arg(m_brailleService->availableDevices().size());
        
        if (m_brailleService->isBrailleDisplayConnected()) {
            auto device = m_brailleService->activeDevice();
            diagnostics += tr("Active Device: %1\n").arg(device.name);
            diagnostics += tr("Device Cells: %1\n").arg(device.cellCount);
        }
    } else {
        diagnostics += tr("Braille Display Service: Not Available\n");
    }
    
    m_diagnosticsText->setPlainText(diagnostics);
}

// Slot implementations

void AccessibilityPreferencesDialog::accept()
{
    if (applySettings()) {
        QDialog::accept();
    }
}

void AccessibilityPreferencesDialog::reject()
{
    QDialog::reject();
}

void AccessibilityPreferencesDialog::onApplyClicked()
{
    applySettings();
}

void AccessibilityPreferencesDialog::onResetClicked()
{
    int result = QMessageBox::question(this, tr("Reset Settings"), 
                                     tr("Are you sure you want to reset all accessibility settings to their default values?"),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        resetToDefaults();
    }
}

void AccessibilityPreferencesDialog::resetToDefaults()
{
    if (m_settingsService) {
        m_settingsService->resetToDefaults();
        loadSettings();
    }
}

void AccessibilityPreferencesDialog::onTestAnnouncementClicked()
{
    if (m_settingsService) {
        // This would trigger a test announcement through the accessibility system
        // For now, show a message box as a placeholder
        QMessageBox::information(this, tr("Test Announcement"), 
                               tr("This is a test announcement with current settings.\n\nVerbosity: %1\nTiming: %2")
                               .arg(m_verbosityComboBox->currentText())
                               .arg(m_timingComboBox->currentText()));
    }
}

void AccessibilityPreferencesDialog::onVerbosityLevelChanged()
{
    onSettingsChanged();
}

void AccessibilityPreferencesDialog::onAnnouncementTimingChanged()
{
    updateUIState();
    onSettingsChanged();
}

void AccessibilityPreferencesDialog::onBrailleEnabledChanged()
{
    updateUIState();
    onSettingsChanged();
}

void AccessibilityPreferencesDialog::onBrailleFormatChanged()
{
    // Update braille preview
    QString sampleText = tr("Sample text: XFB Radio Broadcasting Software - Main Window");
    
    auto format = static_cast<AccessibilitySettingsService::BrailleFormat>(
        m_brailleFormatComboBox->currentData().toInt());
    
    switch (format) {
        case AccessibilitySettingsService::BrailleFormat::Compact:
            sampleText = tr("XFB Radio - Main Win");
            break;
        case AccessibilitySettingsService::BrailleFormat::Detailed:
            sampleText = tr("XFB Radio Broadcasting Software Application - Main Window Interface");
            break;
        default:
            break;
    }
    
    m_braillePreviewText->setPlainText(sampleText);
    onSettingsChanged();
}

void AccessibilityPreferencesDialog::onShortcutSelectionChanged()
{
    int currentRow = m_shortcutsList->currentRow();
    if (currentRow < 0) {
        m_shortcutDescriptionLabel->setText(tr("No shortcut selected"));
        m_shortcutContextLabel->setText("");
        m_shortcutEdit->clear();
        m_resetShortcutButton->setEnabled(false);
        m_conflictWarningLabel->hide();
        return;
    }
    
    QListWidgetItem* item = m_shortcutsList->item(currentRow);
    if (!item) {
        return;
    }
    
    QString action = item->data(Qt::UserRole).toString();
    if (action.isEmpty()) {
        // Header item
        m_shortcutDescriptionLabel->setText(tr("Category header"));
        m_shortcutContextLabel->setText("");
        m_shortcutEdit->clear();
        m_resetShortcutButton->setEnabled(false);
        m_conflictWarningLabel->hide();
        return;
    }
    
    const auto& shortcut = m_currentShortcuts.value(action);
    m_shortcutDescriptionLabel->setText(shortcut.description);
    m_shortcutContextLabel->setText(shortcut.context);
    m_shortcutEdit->setKeySequence(shortcut.sequence);
    m_resetShortcutButton->setEnabled(shortcut.customizable);
    m_shortcutEdit->setEnabled(shortcut.customizable);
    m_conflictWarningLabel->hide();
}

void AccessibilityPreferencesDialog::onShortcutEdited()
{
    int currentRow = m_shortcutsList->currentRow();
    if (currentRow < 0) {
        return;
    }
    
    QListWidgetItem* item = m_shortcutsList->item(currentRow);
    if (!item) {
        return;
    }
    
    QString action = item->data(Qt::UserRole).toString();
    if (action.isEmpty()) {
        return;
    }
    
    QKeySequence newSequence = m_shortcutEdit->keySequence();
    
    // Check for conflicts
    QString conflictAction = m_settingsService->checkKeySequenceConflict(newSequence, action);
    if (!conflictAction.isEmpty()) {
        const auto& conflictShortcut = m_currentShortcuts.value(conflictAction);
        m_conflictWarningLabel->setText(tr("Warning: This key sequence is already used by '%1'")
                                      .arg(conflictShortcut.description));
        m_conflictWarningLabel->show();
    } else {
        m_conflictWarningLabel->hide();
    }
    
    // Update the shortcut
    auto& shortcut = m_currentShortcuts[action];
    shortcut.sequence = newSequence;
    
    updateShortcutDisplay();
    onSettingsChanged();
}

void AccessibilityPreferencesDialog::onResetShortcutClicked()
{
    int currentRow = m_shortcutsList->currentRow();
    if (currentRow < 0) {
        return;
    }
    
    QListWidgetItem* item = m_shortcutsList->item(currentRow);
    if (!item) {
        return;
    }
    
    QString action = item->data(Qt::UserRole).toString();
    if (action.isEmpty()) {
        return;
    }
    
    // Reset to default
    auto defaultShortcuts = m_settingsService->availableKeyboardShortcuts();
    if (defaultShortcuts.contains(action)) {
        m_currentShortcuts[action] = defaultShortcuts.value(action);
        m_shortcutEdit->setKeySequence(m_currentShortcuts[action].sequence);
        updateShortcutDisplay();
        m_conflictWarningLabel->hide();
        onSettingsChanged();
    }
}

void AccessibilityPreferencesDialog::onResetAllShortcutsClicked()
{
    int result = QMessageBox::question(this, tr("Reset All Shortcuts"), 
                                     tr("Are you sure you want to reset all keyboard shortcuts to their default values?"),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        m_currentShortcuts = m_settingsService->availableKeyboardShortcuts();
        populateKeyboardShortcuts();
        onShortcutSelectionChanged(); // Update the edit controls
        onSettingsChanged();
    }
}

void AccessibilityPreferencesDialog::onDetectBrailleDeviceClicked()
{
    if (!m_brailleService) {
        QMessageBox::warning(this, tr("Braille Service Error"), 
                           tr("Braille display service is not available."));
        return;
    }
    
    m_brailleDetectionProgress->setVisible(true);
    m_brailleDetectionProgress->setRange(0, 0); // Indeterminate progress
    m_detectBrailleButton->setEnabled(false);
    
    // Connect to braille service signals
    connect(m_brailleService, &BrailleDisplayService::deviceDetectionCompleted,
            this, &AccessibilityPreferencesDialog::onBrailleDeviceDetected, Qt::UniqueConnection);
    
    // Start actual device detection
    if (!m_brailleService->detectAvailableDevices()) {
        m_brailleDetectionProgress->setVisible(false);
        m_detectBrailleButton->setEnabled(true);
        QMessageBox::warning(this, tr("Detection Error"), 
                           tr("Failed to start braille device detection."));
        return;
    }
    
    // Start timeout timer as backup
    m_brailleDetectionTimer->start();
}

void AccessibilityPreferencesDialog::onBrailleDeviceDetected()
{
    m_brailleDetectionProgress->setVisible(false);
    m_detectBrailleButton->setEnabled(true);
    m_brailleDetectionTimer->stop();
    
    if (!m_brailleService) {
        return;
    }
    
    // Get detected devices from the braille service
    auto devices = m_brailleService->availableDevices();
    
    m_brailleDeviceComboBox->clear();
    
    if (!devices.isEmpty()) {
        for (const auto& device : devices) {
            m_brailleDeviceComboBox->addItem(device.name, device.id);
        }
        
        QMessageBox::information(this, tr("Device Detection"), 
                               tr("Found %1 braille device(s)").arg(devices.size()));
    } else {
        QMessageBox::information(this, tr("Device Detection"), 
                               tr("No braille devices were detected"));
    }
}

void AccessibilityPreferencesDialog::onImportSettingsClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
        tr("Import Accessibility Settings"), 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("JSON Files (*.json);;All Files (*)"));
    
    if (!fileName.isEmpty() && m_settingsService) {
        if (m_settingsService->importSettings(fileName)) {
            loadSettings();
            QMessageBox::information(this, tr("Import Successful"), 
                                   tr("Accessibility settings have been imported successfully."));
        } else {
            QMessageBox::warning(this, tr("Import Failed"), 
                               tr("Failed to import accessibility settings from the selected file."));
        }
    }
}

void AccessibilityPreferencesDialog::onExportSettingsClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Accessibility Settings"), 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/accessibility_settings.json",
        tr("JSON Files (*.json);;All Files (*)"));
    
    if (!fileName.isEmpty() && m_settingsService) {
        if (m_settingsService->exportSettings(fileName)) {
            QMessageBox::information(this, tr("Export Successful"), 
                                   tr("Accessibility settings have been exported successfully."));
        } else {
            QMessageBox::warning(this, tr("Export Failed"), 
                               tr("Failed to export accessibility settings to the selected file."));
        }
    }
}

void AccessibilityPreferencesDialog::onSettingsChanged()
{
    if (!m_loadingSettings) {
        m_settingsChanged = true;
        updateUIState();
    }
}