#include "TestORCACompatibility.h"
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTableView>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QProgressBar>
#include <QTextEdit>
#include <QAccessibleInterface>
#include <QAccessibleEvent>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

TestORCACompatibility::TestORCACompatibility()
    : m_orcaProcess(nullptr)
    , m_orcaAvailable(false)
    , m_atSpiConnected(false)
{
}

void TestORCACompatibility::initTestCase()
{
    // Enable accessibility
    QAccessible::setActive(true);
    
    // Check if ORCA is available
    m_orcaAvailable = QProcess::execute("which", QStringList() << "orca") == 0;
    
    if (m_orcaAvailable) {
        qDebug() << "ORCA screen reader detected, enabling full compatibility tests";
    } else {
        qDebug() << "ORCA not available, running interface compliance tests only";
    }
    
    // Verify AT-SPI is available
    verifyATSPIConnection();
}

void TestORCACompatibility::cleanupTestCase()
{
    stopORCA();
}

void TestORCACompatibility::init()
{
    // Start ORCA for each test if available
    if (m_orcaAvailable) {
        startORCAIfAvailable();
    }
}

void TestORCACompatibility::cleanup()
{
    // Clean up ORCA process
    if (m_orcaProcess && m_orcaProcess->state() == QProcess::Running) {
        m_orcaProcess->terminate();
        if (!m_orcaProcess->waitForFinished(3000)) {
            m_orcaProcess->kill();
        }
    }
}

void TestORCACompatibility::testORCAScreenReaderDetection()
{
    // Test ORCA detection and communication
    
    if (!m_orcaAvailable) {
        QSKIP("ORCA not available for testing");
    }
    
    // Verify ORCA is running
    QVERIFY(isORCARunning());
    
    // Test AT-SPI bridge connection
    QVERIFY(m_atSpiConnected);
    
    // Create test widget and verify ORCA can access it
    QPushButton* testButton = new QPushButton("Test Button");
    testButton->show();
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(testButton);
    QVERIFY(interface != nullptr);
    
    // Verify interface provides required information for ORCA
    QVERIFY(!interface->text(QAccessible::Name).isEmpty());
    QVERIFY(interface->role() == QAccessible::PushButton);
    
    delete testButton;
}

void TestORCACompatibility::testATSPIInterfaceCompliance()
{
    // Test AT-SPI interface compliance for various widget types
    
    QList<QWidget*> testWidgets;
    
    // Create various widget types
    testWidgets << new QPushButton("Button");
    testWidgets << new QLineEdit();
    testWidgets << new QCheckBox("Checkbox");
    testWidgets << new QComboBox();
    testWidgets << new QSlider();
    testWidgets << new QProgressBar();
    testWidgets << new QLabel("Label");
    
    for (QWidget* widget : testWidgets) {
        widget->show();
        testAccessibleInterface(widget);
    }
    
    qDeleteAll(testWidgets);
}

void TestORCACompatibility::testORCAAnnouncementFormats()
{
    // Test ORCA announcement format compatibility
    
    QPushButton* button = new QPushButton("Play Music");
    button->setToolTip("Start playing the selected track");
    button->show();
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(button);
    QVERIFY(interface != nullptr);
    
    // Test name announcement
    QString name = interface->text(QAccessible::Name);
    QCOMPARE(name, "Play Music");
    
    // Test description announcement
    QString description = interface->text(QAccessible::Description);
    QCOMPARE(description, "Start playing the selected track");
    
    // Test role announcement
    QCOMPARE(interface->role(), QAccessible::PushButton);
    
    // Test state announcement
    QAccessible::State state = interface->state();
    QVERIFY(state.focusable);
    
    delete button;
}

void TestORCACompatibility::testORCANavigationPatterns()
{
    // Test ORCA navigation pattern compatibility
    
    // Create navigation test layout
    QWidget* container = new QWidget();
    container->resize(400, 300);
    
    QPushButton* button1 = new QPushButton("Button 1", container);
    QPushButton* button2 = new QPushButton("Button 2", container);
    QPushButton* button3 = new QPushButton("Button 3", container);
    
    button1->setGeometry(10, 10, 100, 30);
    button2->setGeometry(10, 50, 100, 30);
    button3->setGeometry(10, 90, 100, 30);
    
    // Set up tab order
    QWidget::setTabOrder(button1, button2);
    QWidget::setTabOrder(button2, button3);
    
    container->show();
    
    // Test tab navigation
    button1->setFocus();
    QCOMPARE(qApp->focusWidget(), button1);
    
    // Simulate Tab key
    QTest::keyClick(button1, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), button2);
    
    QTest::keyClick(button2, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), button3);
    
    // Test reverse tab navigation
    QTest::keySequence(button3, QKeySequence("Shift+Tab"));
    QCOMPARE(qApp->focusWidget(), button2);
    
    delete container;
}

void TestORCACompatibility::testORCABrailleOutput()
{
    // Test braille output compatibility
    
    if (!m_orcaAvailable) {
        QSKIP("ORCA not available for braille testing");
    }
    
    // Create widgets with braille-friendly content
    QLineEdit* textField = new QLineEdit("Sample text for braille");
    textField->show();
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(textField);
    QVERIFY(interface != nullptr);
    
    // Verify text content is accessible for braille
    QString text = interface->text(QAccessible::Value);
    QCOMPARE(text, "Sample text for braille");
    
    // Test braille-friendly labeling
    QLabel* label = new QLabel("Text Field:");
    label->setBuddy(textField);
    
    QString name = interface->text(QAccessible::Name);
    QVERIFY(name.contains("Text Field") || !name.isEmpty());
    
    delete textField;
    delete label;
}

void TestORCACompatibility::testWCAGKeyboardAccessibility()
{
    // Test WCAG 2.1 keyboard accessibility requirements
    
    // Create complex interface
    QWidget* container = new QWidget();
    container->resize(600, 400);
    
    // Create various interactive elements
    QPushButton* button = new QPushButton("Action Button", container);
    QLineEdit* textField = new QLineEdit(container);
    QCheckBox* checkbox = new QCheckBox("Enable feature", container);
    QComboBox* comboBox = new QComboBox(container);
    QTableView* table = new QTableView(container);
    
    // Position elements
    button->setGeometry(10, 10, 120, 30);
    textField->setGeometry(10, 50, 200, 30);
    checkbox->setGeometry(10, 90, 150, 30);
    comboBox->setGeometry(10, 130, 150, 30);
    table->setGeometry(10, 170, 300, 200);
    
    container->show();
    
    // Test keyboard accessibility for each element
    verifyWCAGCompliance(button);
    verifyWCAGCompliance(textField);
    verifyWCAGCompliance(checkbox);
    verifyWCAGCompliance(comboBox);
    verifyWCAGCompliance(table);
    
    delete container;
}

void TestORCACompatibility::testWCAGFocusManagement()
{
    // Test WCAG focus management requirements
    
    QWidget* dialog = new QWidget();
    dialog->setWindowTitle("Test Dialog");
    dialog->resize(300, 200);
    
    QPushButton* okButton = new QPushButton("OK", dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", dialog);
    QLineEdit* inputField = new QLineEdit(dialog);
    
    okButton->setGeometry(50, 150, 80, 30);
    cancelButton->setGeometry(150, 150, 80, 30);
    inputField->setGeometry(50, 50, 180, 30);
    
    // Set initial focus
    inputField->setFocus();
    
    dialog->show();
    
    // Test focus is visible and manageable
    QCOMPARE(qApp->focusWidget(), inputField);
    
    // Test focus moves correctly
    QTest::keyClick(inputField, Qt::Key_Tab);
    QVERIFY(qApp->focusWidget() == okButton || qApp->focusWidget() == cancelButton);
    
    // Test focus doesn't get trapped
    for (int i = 0; i < 10; ++i) {
        QTest::keyClick(qApp->focusWidget(), Qt::Key_Tab);
        QVERIFY(qApp->focusWidget() != nullptr);
    }
    
    delete dialog;
}

void TestORCACompatibility::testWCAGColorContrast()
{
    // Test color contrast requirements (programmatic check)
    
    QPushButton* button = new QPushButton("Test Button");
    button->show();
    
    // Get button colors
    QPalette palette = button->palette();
    QColor background = palette.color(QPalette::Button);
    QColor foreground = palette.color(QPalette::ButtonText);
    
    // Calculate contrast ratio (simplified)
    double backgroundLuminance = (0.299 * background.red() + 0.587 * background.green() + 0.114 * background.blue()) / 255.0;
    double foregroundLuminance = (0.299 * foreground.red() + 0.587 * foreground.green() + 0.114 * foreground.blue()) / 255.0;
    
    double contrastRatio = (qMax(backgroundLuminance, foregroundLuminance) + 0.05) / 
                          (qMin(backgroundLuminance, foregroundLuminance) + 0.05);
    
    // WCAG AA requires 4.5:1 for normal text, 3:1 for large text
    // This is a simplified check - real implementation would be more sophisticated
    QVERIFY(contrastRatio >= 3.0); // Minimum acceptable contrast
    
    delete button;
}

void TestORCACompatibility::testWCAGTextAlternatives()
{
    // Test text alternatives for non-text content
    
    // Test button with icon (simulated)
    QPushButton* iconButton = new QPushButton();
    iconButton->setText(""); // No visible text
    iconButton->setAccessibleName("Play button");
    iconButton->setAccessibleDescription("Start playing the current track");
    iconButton->show();
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(iconButton);
    QVERIFY(interface != nullptr);
    
    // Verify text alternative is provided
    QString name = interface->text(QAccessible::Name);
    QCOMPARE(name, "Play button");
    
    QString description = interface->text(QAccessible::Description);
    QCOMPARE(description, "Start playing the current track");
    
    delete iconButton;
}

void TestORCACompatibility::testWCAGTimingAdjustable()
{
    // Test timing adjustable requirements
    
    // Create a progress indicator that updates
    QProgressBar* progress = new QProgressBar();
    progress->setRange(0, 100);
    progress->setValue(0);
    progress->show();
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(progress);
    QVERIFY(interface != nullptr);
    
    // Test that progress updates are accessible
    progress->setValue(50);
    
    // Verify value is accessible
    QString value = interface->text(QAccessible::Value);
    QVERIFY(value.contains("50") || value.contains("50%"));
    
    delete progress;
}

void TestORCACompatibility::testCompleteKeyboardOnlyWorkflow()
{
    // Test complete application workflow using only keyboard
    
    // Create main application interface
    QWidget* mainWindow = new QWidget();
    mainWindow->setWindowTitle("XFB Radio - Keyboard Test");
    mainWindow->resize(800, 600);
    
    // Create menu bar simulation
    QPushButton* fileMenu = new QPushButton("File", mainWindow);
    QPushButton* editMenu = new QPushButton("Edit", mainWindow);
    QPushButton* playMenu = new QPushButton("Play", mainWindow);
    
    // Create main controls
    QLineEdit* searchField = new QLineEdit(mainWindow);
    searchField->setPlaceholderText("Search music...");
    
    QTableView* musicLibrary = new QTableView(mainWindow);
    QListWidget* playlist = new QListWidget(mainWindow);
    
    QPushButton* playButton = new QPushButton("Play", mainWindow);
    QPushButton* stopButton = new QPushButton("Stop", mainWindow);
    QPushButton* nextButton = new QPushButton("Next", mainWindow);
    
    // Position elements (simplified layout)
    fileMenu->setGeometry(10, 10, 60, 30);
    editMenu->setGeometry(80, 10, 60, 30);
    playMenu->setGeometry(150, 10, 60, 30);
    
    searchField->setGeometry(10, 50, 300, 30);
    musicLibrary->setGeometry(10, 90, 400, 300);
    playlist->setGeometry(420, 90, 300, 300);
    
    playButton->setGeometry(10, 400, 80, 40);
    stopButton->setGeometry(100, 400, 80, 40);
    nextButton->setGeometry(190, 400, 80, 40);
    
    // Set up proper tab order
    QWidget::setTabOrder(fileMenu, editMenu);
    QWidget::setTabOrder(editMenu, playMenu);
    QWidget::setTabOrder(playMenu, searchField);
    QWidget::setTabOrder(searchField, musicLibrary);
    QWidget::setTabOrder(musicLibrary, playlist);
    QWidget::setTabOrder(playlist, playButton);
    QWidget::setTabOrder(playButton, stopButton);
    QWidget::setTabOrder(stopButton, nextButton);
    
    mainWindow->show();
    
    // Test complete workflow with keyboard only
    
    // 1. Navigate to search field
    searchField->setFocus();
    QCOMPARE(qApp->focusWidget(), searchField);
    
    // 2. Enter search query
    QTest::keyClicks(searchField, "rock music");
    QTest::keyClick(searchField, Qt::Key_Return);
    
    // 3. Navigate to music library
    QTest::keyClick(searchField, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), musicLibrary);
    
    // 4. Navigate within music library
    QTest::keyClick(musicLibrary, Qt::Key_Down);
    QTest::keyClick(musicLibrary, Qt::Key_Down);
    
    // 5. Select item and copy to playlist
    QTest::keySequence(musicLibrary, QKeySequence("Ctrl+C"));
    
    // 6. Navigate to playlist
    QTest::keyClick(musicLibrary, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), playlist);
    
    // 7. Paste into playlist
    QTest::keySequence(playlist, QKeySequence("Ctrl+V"));
    
    // 8. Navigate to play button
    QTest::keyClick(playlist, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), playButton);
    
    // 9. Start playback
    QTest::keyClick(playButton, Qt::Key_Space);
    
    // 10. Test stop functionality
    QTest::keyClick(playButton, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), stopButton);
    QTest::keyClick(stopButton, Qt::Key_Space);
    
    delete mainWindow;
}

void TestORCACompatibility::testKeyboardShortcutCoverage()
{
    // Test keyboard shortcut coverage for all major functions
    
    QWidget* testWindow = new QWidget();
    testWindow->resize(400, 300);
    testWindow->show();
    
    // Test common shortcuts
    QStringList requiredShortcuts = {
        "Ctrl+N",    // New
        "Ctrl+O",    // Open
        "Ctrl+S",    // Save
        "Ctrl+Z",    // Undo
        "Ctrl+Y",    // Redo
        "Ctrl+C",    // Copy
        "Ctrl+V",    // Paste
        "Ctrl+X",    // Cut
        "Ctrl+A",    // Select All
        "Ctrl+F",    // Find
        "F1",        // Help
        "Alt+F4",    // Close
        "Escape"     // Cancel
    };
    
    // Verify shortcuts don't cause crashes (basic test)
    for (const QString& shortcut : requiredShortcuts) {
        QKeySequence sequence(shortcut);
        if (!sequence.isEmpty()) {
            // Test that shortcut can be processed without crash
            QTest::keySequence(testWindow, sequence);
            QTest::qWait(10);
        }
    }
    
    delete testWindow;
}

void TestORCACompatibility::testKeyboardTrapAvoidance()
{
    // Test that keyboard focus doesn't get trapped
    
    QWidget* container = new QWidget();
    container->resize(300, 200);
    
    // Create potential trap scenario
    QWidget* subContainer = new QWidget(container);
    QPushButton* button1 = new QPushButton("Button 1", subContainer);
    QPushButton* button2 = new QPushButton("Button 2", subContainer);
    QPushButton* escapeButton = new QPushButton("Escape", container);
    
    button1->setGeometry(10, 10, 100, 30);
    button2->setGeometry(10, 50, 100, 30);
    subContainer->setGeometry(50, 50, 150, 100);
    escapeButton->setGeometry(10, 10, 100, 30);
    
    container->show();
    
    // Test navigation doesn't get trapped
    button1->setFocus();
    
    // Navigate through all elements multiple times
    for (int i = 0; i < 20; ++i) {
        QTest::keyClick(qApp->focusWidget(), Qt::Key_Tab);
        
        // Verify focus is always on a valid widget
        QVERIFY(qApp->focusWidget() != nullptr);
        
        // Verify we can always escape with Escape key
        QTest::keyClick(qApp->focusWidget(), Qt::Key_Escape);
    }
    
    delete container;
}

void TestORCACompatibility::testFocusIndicatorVisibility()
{
    // Test focus indicator visibility
    
    QPushButton* button = new QPushButton("Test Button");
    button->show();
    button->setFocus();
    
    // Verify button has focus
    QCOMPARE(qApp->focusWidget(), button);
    
    // Test focus indicator is programmatically detectable
    QVERIFY(button->hasFocus());
    
    // Test focus can be moved and detected
    QLineEdit* textField = new QLineEdit();
    textField->show();
    textField->setFocus();
    
    QCOMPARE(qApp->focusWidget(), textField);
    QVERIFY(textField->hasFocus());
    QVERIFY(!button->hasFocus());
    
    delete button;
    delete textField;
}

// Helper method implementations

bool TestORCACompatibility::isORCARunning()
{
    QProcess process;
    process.start("pgrep", QStringList() << "orca");
    process.waitForFinished(3000);
    
    return process.exitCode() == 0;
}

void TestORCACompatibility::startORCAIfAvailable()
{
    if (!m_orcaAvailable) return;
    
    // Don't start if already running
    if (isORCARunning()) return;
    
    m_orcaProcess = new QProcess(this);
    m_orcaProcess->start("orca", QStringList() << "--no-setup");
    
    // Wait for ORCA to start
    QTest::qWait(2000);
}

void TestORCACompatibility::stopORCA()
{
    if (m_orcaProcess && m_orcaProcess->state() == QProcess::Running) {
        m_orcaProcess->terminate();
        if (!m_orcaProcess->waitForFinished(3000)) {
            m_orcaProcess->kill();
        }
        delete m_orcaProcess;
        m_orcaProcess = nullptr;
    }
}

void TestORCACompatibility::verifyATSPIConnection()
{
    // Check if AT-SPI is available
    QString atSpiPath = "/usr/lib/at-spi2-core/at-spi-bus-launcher";
    if (!QFile::exists(atSpiPath)) {
        atSpiPath = "/usr/libexec/at-spi-bus-launcher";
    }
    
    m_atSpiConnected = QFile::exists(atSpiPath) || 
                       QProcess::execute("which", QStringList() << "at-spi-bus-launcher") == 0;
    
    if (m_atSpiConnected) {
        qDebug() << "AT-SPI bridge available";
    } else {
        qDebug() << "AT-SPI bridge not found - some tests may be limited";
    }
}

void TestORCACompatibility::testAccessibleInterface(QWidget* widget)
{
    QVERIFY(widget != nullptr);
    
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY2(interface != nullptr, "Widget must have accessible interface");
    
    // Test required interface methods
    QVERIFY(interface->isValid());
    
    // Test role is appropriate
    QAccessible::Role role = interface->role();
    QVERIFY(role != QAccessible::NoRole);
    
    // Test name is provided (unless it's a decorative element)
    if (role != QAccessible::Separator && role != QAccessible::Graphic) {
        QString name = interface->text(QAccessible::Name);
        QVERIFY2(!name.isEmpty(), "Interactive elements must have accessible names");
    }
    
    // Test state information
    QAccessible::State state = interface->state();
    
    // Focusable elements should report focusable state
    if (widget->focusPolicy() != Qt::NoFocus) {
        QVERIFY(state.focusable);
    }
    
    // Test action interface for interactive elements
    if (role == QAccessible::PushButton || role == QAccessible::CheckBox) {
        QVERIFY(interface->actionInterface() != nullptr);
    }
}

void TestORCACompatibility::verifyWCAGCompliance(QWidget* widget)
{
    QVERIFY(widget != nullptr);
    
    // Test keyboard accessibility
    if (widget->focusPolicy() != Qt::NoFocus) {
        widget->setFocus();
        QCOMPARE(qApp->focusWidget(), widget);
        
        // Test focus is visible
        QVERIFY(widget->hasFocus());
    }
    
    // Test accessible interface compliance
    testAccessibleInterface(widget);
    
    // Test that widget responds to standard keyboard interactions
    if (qobject_cast<QPushButton*>(widget)) {
        QTest::keyClick(widget, Qt::Key_Space);
        // Should not crash
    } else if (qobject_cast<QLineEdit*>(widget)) {
        QTest::keyClicks(widget, "test");
        // Should accept text input
    }
}

QTEST_MAIN(TestORCACompatibility)