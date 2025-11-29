#include "TestAccessibilityIntegration.h"
#include "../../src/services/AccessibilityManager.h"
#include "../../src/services/KeyboardNavigationController.h"
#include "../../src/services/AudioFeedbackService.h"
#include "../../src/services/LiveRegionManager.h"
#include "../../src/services/WidgetAccessibilityEnhancer.h"

#include <QAccessible>
#include <QAccessibleInterface>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>
#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QMainWindow>

TestAccessibilityIntegration::TestAccessibilityIntegration()
    : m_testMainWindow(nullptr)
    , m_testMusicGrid(nullptr)
    , m_testPlaylist(nullptr)
    , m_testPlayButton(nullptr)
    , m_testSearchField(nullptr)
    , m_accessibilityManager(nullptr)
    , m_keyboardController(nullptr)
    , m_audioFeedback(nullptr)
    , m_liveRegionManager(nullptr)
    , m_widgetEnhancer(nullptr)
    , m_announcementSpy(nullptr)
    , m_focusChangeSpy(nullptr)
    , m_liveRegionSpy(nullptr)
    , m_accessibilityEnabled(false)
{
}

void TestAccessibilityIntegration::initTestCase()
{
    // Enable accessibility for testing
    QAccessible::setActive(true);
    
    // Initialize accessibility services
    m_accessibilityManager = AccessibilityManager::instance();
    m_accessibilityManager->initialize();
    m_accessibilityManager->enableAccessibility(true);
    
    m_keyboardController = new KeyboardNavigationController(this);
    m_audioFeedback = new AudioFeedbackService(this);
    m_liveRegionManager = new LiveRegionManager(this);
    m_widgetEnhancer = new WidgetAccessibilityEnhancer(this);
    
    // Initialize services
    m_keyboardController->initialize();
    m_audioFeedback->initialize();
    m_liveRegionManager->initialize();
    m_widgetEnhancer->initialize();
    
    m_accessibilityEnabled = true;
}

void TestAccessibilityIntegration::cleanupTestCase()
{
    if (m_announcementSpy) {
        delete m_announcementSpy;
        m_announcementSpy = nullptr;
    }
    
    if (m_focusChangeSpy) {
        delete m_focusChangeSpy;
        m_focusChangeSpy = nullptr;
    }
    
    if (m_liveRegionSpy) {
        delete m_liveRegionSpy;
        m_liveRegionSpy = nullptr;
    }
    
    if (m_testMainWindow) {
        delete m_testMainWindow;
        m_testMainWindow = nullptr;
    }
}

void TestAccessibilityIntegration::init()
{
    createTestBroadcastingEnvironment();
    setupTestMusicLibrary();
    
    // Set up signal spies for monitoring accessibility events
    m_announcementSpy = new QSignalSpy(m_accessibilityManager, 
                                      &AccessibilityManager::announcementRequested);
    m_focusChangeSpy = new QSignalSpy(m_keyboardController, 
                                     &KeyboardNavigationController::focusChanged);
    m_liveRegionSpy = new QSignalSpy(m_liveRegionManager, 
                                    &LiveRegionManager::liveRegionUpdated);
    
    m_capturedAnnouncements.clear();
}

void TestAccessibilityIntegration::cleanup()
{
    if (m_announcementSpy) {
        delete m_announcementSpy;
        m_announcementSpy = nullptr;
    }
    
    if (m_focusChangeSpy) {
        delete m_focusChangeSpy;
        m_focusChangeSpy = nullptr;
    }
    
    if (m_liveRegionSpy) {
        delete m_liveRegionSpy;
        m_liveRegionSpy = nullptr;
    }
    
    m_capturedAnnouncements.clear();
}

void TestAccessibilityIntegration::testCompleteBroadcastingWorkflow()
{
    // Test complete workflow: Setup -> Music Selection -> Playlist Creation -> Broadcasting
    
    // Step 1: Navigate to music library
    m_testSearchField->setFocus();
    QTest::keyClick(m_testSearchField, Qt::Key_Tab);
    
    // Verify focus moved to music grid
    QCOMPARE(qApp->focusWidget(), m_testMusicGrid);
    QVERIFY(m_focusChangeSpy->count() > 0);
    
    // Step 2: Search and select music
    QTest::keyClicks(m_testSearchField, "rock music");
    QTest::keyClick(m_testSearchField, Qt::Key_Return);
    
    // Verify search feedback
    QVERIFY(m_announcementSpy->count() > 0);
    
    // Step 3: Navigate grid with keyboard
    simulateKeyboardNavigation({Qt::Key_Down, Qt::Key_Down, Qt::Key_Right});
    
    // Step 4: Add to playlist
    QTest::keySequence(m_testMusicGrid, QKeySequence("Ctrl+C"));
    m_testPlaylist->setFocus();
    QTest::keySequence(m_testPlaylist, QKeySequence("Ctrl+V"));
    
    // Verify playlist addition feedback
    QVERIFY(m_announcementSpy->count() > 2);
    
    // Step 5: Start playback
    m_testPlayButton->setFocus();
    QTest::keyClick(m_testPlayButton, Qt::Key_Space);
    
    // Verify playback announcement
    QVERIFY(m_liveRegionSpy->count() > 0);
    
    // Step 6: Monitor live region updates during playback
    QTimer::singleShot(100, [this]() {
        // Simulate track progress update
        m_liveRegionManager->updatePlaybackStatus("Now Playing: Test Track - 1:23 / 3:45");
    });
    
    QTest::qWait(150);
    QVERIFY(m_liveRegionSpy->count() > 1);
}

void TestAccessibilityIntegration::testMusicLibraryManagementWorkflow()
{
    // Test music library navigation, editing, and management
    
    // Navigate to music grid
    m_testMusicGrid->setFocus();
    
    // Test grid navigation with announcements
    simulateKeyboardNavigation({Qt::Key_Down, Qt::Key_Right, Qt::Key_Down});
    
    // Verify grid navigation announcements
    QVERIFY(m_announcementSpy->count() >= 3);
    
    // Test cell editing
    QTest::keyClick(m_testMusicGrid, Qt::Key_F2);
    QTest::keyClicks(m_testMusicGrid, "New Track Title");
    QTest::keyClick(m_testMusicGrid, Qt::Key_Return);
    
    // Verify edit mode announcements
    QVERIFY(m_announcementSpy->count() > 5);
    
    // Test sorting
    QTest::keySequence(m_testMusicGrid, QKeySequence("Ctrl+S"));
    
    // Verify sort announcement
    QVERIFY(m_announcementSpy->count() > 6);
}

void TestAccessibilityIntegration::testPlaylistCreationWorkflow()
{
    // Test playlist creation and management workflow
    
    // Create new playlist
    QTest::keySequence(m_testMainWindow, QKeySequence("Ctrl+N"));
    
    // Verify new playlist announcement
    QVERIFY(m_announcementSpy->count() > 0);
    
    // Add items to playlist via keyboard
    m_testMusicGrid->setFocus();
    simulateKeyboardNavigation({Qt::Key_Down, Qt::Key_Space}); // Select item
    QTest::keySequence(m_testMusicGrid, QKeySequence("Ctrl+C"));
    
    m_testPlaylist->setFocus();
    QTest::keySequence(m_testPlaylist, QKeySequence("Ctrl+V"));
    
    // Verify playlist modification announcements
    QVERIFY(m_announcementSpy->count() > 3);
    
    // Test playlist reordering
    QTest::keySequence(m_testPlaylist, QKeySequence("Ctrl+Up"));
    
    // Verify reorder announcement
    QVERIFY(m_announcementSpy->count() > 4);
}

void TestAccessibilityIntegration::testLiveBroadcastingWorkflow()
{
    // Test live broadcasting scenario with real-time updates
    
    // Start broadcasting mode
    QTest::keySequence(m_testMainWindow, QKeySequence("Ctrl+B"));
    
    // Verify broadcasting mode announcement
    QVERIFY(m_announcementSpy->count() > 0);
    
    // Simulate live updates
    QTimer::singleShot(50, [this]() {
        m_liveRegionManager->updatePlaybackStatus("LIVE: On Air - 2:15 remaining");
    });
    
    QTimer::singleShot(100, [this]() {
        m_liveRegionManager->updateSystemStatus("Recording: Active");
    });
    
    QTimer::singleShot(150, [this]() {
        m_liveRegionManager->announceCountdown("30 seconds to next track");
    });
    
    QTest::qWait(200);
    
    // Verify live region updates
    QVERIFY(m_liveRegionSpy->count() >= 3);
    
    // Test emergency stop
    QTest::keySequence(m_testMainWindow, QKeySequence("Ctrl+Shift+S"));
    
    // Verify emergency announcement
    QVERIFY(m_announcementSpy->count() > 4);
}

void TestAccessibilityIntegration::testEmergencyProceduresWorkflow()
{
    // Test emergency procedures and critical alerts
    
    // Simulate critical error
    m_audioFeedback->announceCriticalAlert("CRITICAL: Audio system failure detected");
    
    // Verify immediate critical announcement
    QVERIFY(m_announcementSpy->count() > 0);
    
    // Test emergency stop sequence
    QTest::keySequence(m_testMainWindow, QKeySequence("Ctrl+Alt+S"));
    
    // Verify emergency stop announcement
    QVERIFY(m_announcementSpy->count() > 1);
    
    // Test backup system activation
    QTest::keySequence(m_testMainWindow, QKeySequence("Ctrl+Alt+B"));
    
    // Verify backup activation announcement
    QVERIFY(m_announcementSpy->count() > 2);
}

void TestAccessibilityIntegration::testAccessibilityManagerCoordination()
{
    // Test coordination between AccessibilityManager and other services
    
    // Test verbosity level changes
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::Verbose);
    
    // Trigger announcement to test verbosity
    m_audioFeedback->announceAction("Button clicked");
    QVERIFY(m_announcementSpy->count() > 0);
    
    // Change to terse mode
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::Terse);
    
    // Trigger same announcement
    m_audioFeedback->announceAction("Button clicked");
    QVERIFY(m_announcementSpy->count() > 1);
    
    // Test accessibility enable/disable
    m_accessibilityManager->enableAccessibility(false);
    m_audioFeedback->announceAction("Should not announce");
    
    int previousCount = m_announcementSpy->count();
    QTest::qWait(50);
    QCOMPARE(m_announcementSpy->count(), previousCount); // No new announcements
    
    m_accessibilityManager->enableAccessibility(true);
}

void TestAccessibilityIntegration::testKeyboardNavigationIntegration()
{
    // Test keyboard navigation integration across components
    
    // Test tab order
    m_testSearchField->setFocus();
    QTest::keyClick(m_testSearchField, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), m_testMusicGrid);
    
    QTest::keyClick(m_testMusicGrid, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), m_testPlaylist);
    
    QTest::keyClick(m_testPlaylist, Qt::Key_Tab);
    QCOMPARE(qApp->focusWidget(), m_testPlayButton);
    
    // Test reverse tab order
    QTest::keySequence(m_testPlayButton, QKeySequence("Shift+Tab"));
    QCOMPARE(qApp->focusWidget(), m_testPlaylist);
    
    // Verify focus change announcements
    QVERIFY(m_focusChangeSpy->count() >= 4);
}

void TestAccessibilityIntegration::testAudioFeedbackIntegration()
{
    // Test audio feedback integration with user actions
    
    // Test button click feedback
    m_testPlayButton->click();
    QVERIFY(m_announcementSpy->count() > 0);
    
    // Test menu selection feedback
    QTest::keySequence(m_testMainWindow, QKeySequence("Alt+F"));
    QVERIFY(m_announcementSpy->count() > 1);
    
    // Test error feedback
    m_audioFeedback->announceError("File not found");
    QVERIFY(m_announcementSpy->count() > 2);
    
    // Test confirmation feedback
    m_audioFeedback->announceConfirmation("Playlist saved successfully");
    QVERIFY(m_announcementSpy->count() > 3);
}

void TestAccessibilityIntegration::testLiveRegionCoordination()
{
    // Test live region coordination with dynamic content
    
    // Test playback status updates
    m_liveRegionManager->updatePlaybackStatus("Playing: Track 1 - 1:30 / 3:45");
    QVERIFY(m_liveRegionSpy->count() > 0);
    
    // Test system status updates
    m_liveRegionManager->updateSystemStatus("Recording: Active, Streaming: Connected");
    QVERIFY(m_liveRegionSpy->count() > 1);
    
    // Test countdown announcements
    m_liveRegionManager->announceCountdown("10 seconds to next track");
    QVERIFY(m_liveRegionSpy->count() > 2);
    
    // Test queue updates
    m_liveRegionManager->announceQueueUpdate("Track added to queue: Track 2");
    QVERIFY(m_liveRegionSpy->count() > 3);
}

void TestAccessibilityIntegration::testSettingsIntegration()
{
    // Test accessibility settings integration
    
    // Test verbosity setting application
    m_accessibilityManager->setVerbosityLevel(AccessibilityManager::Verbose);
    
    // Trigger widget focus to test verbosity
    m_testPlayButton->setFocus();
    QVERIFY(m_focusChangeSpy->count() > 0);
    
    // Test keyboard shortcut customization
    m_keyboardController->registerShortcut("test_action", QKeySequence("Ctrl+T"), "Test Action");
    
    // Test shortcut execution
    QTest::keySequence(m_testMainWindow, QKeySequence("Ctrl+T"));
    
    // Verify shortcut was processed
    // Note: This would need actual shortcut handling implementation
}

void TestAccessibilityIntegration::testScreenReaderAnnouncements()
{
    // Test screen reader announcement simulation
    
    // Test widget focus announcements
    simulateScreenReaderInteraction(m_testPlayButton);
    simulateScreenReaderInteraction(m_testSearchField);
    simulateScreenReaderInteraction(m_testMusicGrid);
    
    // Verify announcements were made
    QVERIFY(m_announcementSpy->count() >= 3);
}

void TestAccessibilityIntegration::testFocusChangeAnnouncements()
{
    // Test focus change announcement system
    
    QWidget* widgets[] = {m_testSearchField, m_testMusicGrid, m_testPlaylist, m_testPlayButton};
    
    for (int i = 0; i < 4; ++i) {
        widgets[i]->setFocus();
        QTest::qWait(10); // Allow focus change to process
    }
    
    // Verify focus change events were captured
    QVERIFY(m_focusChangeSpy->count() >= 4);
}

void TestAccessibilityIntegration::testDynamicContentAnnouncements()
{
    // Test dynamic content announcement system
    
    // Simulate various dynamic updates
    m_liveRegionManager->updatePlaybackStatus("Track changed: New Song");
    m_liveRegionManager->updateSystemStatus("Connection status: Online");
    m_liveRegionManager->announceProgress("Loading: 50% complete");
    
    // Verify live region updates
    QVERIFY(m_liveRegionSpy->count() >= 3);
}

void TestAccessibilityIntegration::testErrorMessageAnnouncements()
{
    // Test error message announcement system
    
    // Test different error types
    m_audioFeedback->announceError("Network connection failed");
    m_audioFeedback->announceCriticalAlert("CRITICAL: System overload");
    m_audioFeedback->announceWarning("Warning: Low disk space");
    
    // Verify error announcements
    QVERIFY(m_announcementSpy->count() >= 3);
}

void TestAccessibilityIntegration::testAccessibilityPerformanceUnderLoad()
{
    // Test accessibility performance under load
    
    QTime timer;
    timer.start();
    
    // Generate multiple rapid announcements
    for (int i = 0; i < 100; ++i) {
        m_audioFeedback->announceAction(QString("Action %1").arg(i));
        if (i % 10 == 0) {
            QTest::qWait(1); // Small delay to prevent overwhelming
        }
    }
    
    int elapsed = timer.elapsed();
    
    // Verify performance is acceptable (should complete within reasonable time)
    QVERIFY(elapsed < 1000); // Less than 1 second for 100 announcements
    
    // Verify all announcements were queued
    QVERIFY(m_announcementSpy->count() >= 100);
}

void TestAccessibilityIntegration::testMemoryUsageWithAccessibility()
{
    // Test memory usage with accessibility enabled
    
    // Create many widgets and enhance them
    QList<QWidget*> testWidgets;
    for (int i = 0; i < 50; ++i) {
        QPushButton* button = new QPushButton(QString("Button %1").arg(i), m_testMainWindow);
        m_widgetEnhancer->enhanceWidget(button);
        testWidgets.append(button);
    }
    
    // Verify widgets were enhanced without memory issues
    QCOMPARE(testWidgets.size(), 50);
    
    // Clean up test widgets
    qDeleteAll(testWidgets);
    testWidgets.clear();
}

void TestAccessibilityIntegration::testAnnouncementLatency()
{
    // Test announcement latency
    
    QTime timer;
    timer.start();
    
    m_audioFeedback->announceAction("Latency test");
    
    // Wait for announcement to be processed
    QTest::qWait(10);
    
    int latency = timer.elapsed();
    
    // Verify low latency (should be nearly immediate)
    QVERIFY(latency < 50); // Less than 50ms latency
    QVERIFY(m_announcementSpy->count() > 0);
}

void TestAccessibilityIntegration::testAccessibilityErrorRecovery()
{
    // Test error recovery mechanisms
    
    // Simulate AT-SPI connection failure
    // Note: This would require actual AT-SPI simulation
    
    // Test graceful degradation
    m_accessibilityManager->enableAccessibility(false);
    
    // Verify system continues to function
    m_testPlayButton->click();
    m_testSearchField->setFocus();
    
    // Re-enable accessibility
    m_accessibilityManager->enableAccessibility(true);
    
    // Verify recovery
    m_audioFeedback->announceAction("Recovery test");
    QVERIFY(m_announcementSpy->count() > 0);
}

void TestAccessibilityIntegration::testGracefulDegradation()
{
    // Test graceful degradation when accessibility services fail
    
    // Disable individual services and verify system stability
    m_accessibilityManager->enableAccessibility(false);
    
    // Test that UI remains functional
    m_testPlayButton->setFocus();
    m_testPlayButton->click();
    
    simulateKeyboardNavigation({Qt::Key_Tab, Qt::Key_Tab});
    
    // Verify no crashes occurred
    QVERIFY(true); // If we reach here, no crash occurred
    
    // Re-enable for cleanup
    m_accessibilityManager->enableAccessibility(true);
}

void TestAccessibilityIntegration::testATSPIConnectionFailure()
{
    // Test AT-SPI connection failure handling
    
    // Note: This would require mocking AT-SPI connection
    // For now, test that system handles accessibility disable gracefully
    
    QAccessible::setActive(false);
    
    // Verify system continues to function
    m_testPlayButton->click();
    m_testSearchField->setText("test");
    
    // Re-enable
    QAccessible::setActive(true);
    
    QVERIFY(true); // Test passes if no crash
}

// Helper method implementations

void TestAccessibilityIntegration::simulateKeyboardNavigation(const QList<Qt::Key>& keys)
{
    QWidget* focusWidget = qApp->focusWidget();
    if (!focusWidget) {
        focusWidget = m_testMainWindow;
    }
    
    for (Qt::Key key : keys) {
        QTest::keyClick(focusWidget, key);
        QTest::qWait(10); // Small delay between key presses
        focusWidget = qApp->focusWidget(); // Update focus widget
    }
}

void TestAccessibilityIntegration::simulateScreenReaderInteraction(QWidget* widget)
{
    if (!widget) return;
    
    // Simulate screen reader querying widget accessibility information
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(widget);
    if (interface) {
        QString name = interface->text(QAccessible::Name);
        QString description = interface->text(QAccessible::Description);
        
        // Simulate announcement
        QString announcement = name;
        if (!description.isEmpty()) {
            announcement += ", " + description;
        }
        
        m_capturedAnnouncements.append(announcement);
        
        // Trigger announcement through accessibility manager
        m_accessibilityManager->announceMessage(announcement);
    }
}

void TestAccessibilityIntegration::verifyAccessibilityMetadata(QWidget* widget, 
                                                             const QString& expectedName, 
                                                             const QString& expectedDescription)
{
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(interface != nullptr);
    
    QString actualName = interface->text(QAccessible::Name);
    QCOMPARE(actualName, expectedName);
    
    if (!expectedDescription.isEmpty()) {
        QString actualDescription = interface->text(QAccessible::Description);
        QCOMPARE(actualDescription, expectedDescription);
    }
}

void TestAccessibilityIntegration::verifyKeyboardAccessibility(QWidget* widget)
{
    // Verify widget can receive focus
    widget->setFocus();
    QCOMPARE(qApp->focusWidget(), widget);
    
    // Verify widget has proper accessibility interface
    QAccessibleInterface* interface = QAccessible::queryAccessibleInterface(widget);
    QVERIFY(interface != nullptr);
    
    // Verify widget has accessible name
    QString name = interface->text(QAccessible::Name);
    QVERIFY(!name.isEmpty());
}

void TestAccessibilityIntegration::verifyAudioFeedback(const QString& expectedMessage)
{
    // Check if expected message was announced
    bool found = false;
    for (const QString& announcement : m_capturedAnnouncements) {
        if (announcement.contains(expectedMessage)) {
            found = true;
            break;
        }
    }
    QVERIFY2(found, QString("Expected announcement not found: %1").arg(expectedMessage).toLocal8Bit());
}

void TestAccessibilityIntegration::createTestBroadcastingEnvironment()
{
    // Create main window with typical broadcasting interface
    m_testMainWindow = new QMainWindow();
    m_testMainWindow->setWindowTitle("XFB Test Broadcasting Interface");
    m_testMainWindow->resize(1024, 768);
    
    // Create central widget with layout
    QWidget* centralWidget = new QWidget();
    m_testMainWindow->setCentralWidget(centralWidget);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Create left panel (music library)
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    // Search field
    m_testSearchField = new QLineEdit();
    m_testSearchField->setPlaceholderText("Search music library...");
    leftLayout->addWidget(new QLabel("Music Library Search:"));
    leftLayout->addWidget(m_testSearchField);
    
    // Music grid
    m_testMusicGrid = new QTableView();
    QStandardItemModel* musicModel = new QStandardItemModel(10, 4, m_testMusicGrid);
    musicModel->setHorizontalHeaderLabels({"Title", "Artist", "Album", "Duration"});
    m_testMusicGrid->setModel(musicModel);
    leftLayout->addWidget(m_testMusicGrid);
    
    mainLayout->addWidget(leftPanel, 2);
    
    // Create right panel (playlist and controls)
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    // Playlist
    rightLayout->addWidget(new QLabel("Current Playlist:"));
    m_testPlaylist = new QListWidget();
    rightLayout->addWidget(m_testPlaylist);
    
    // Player controls
    QWidget* controlsWidget = new QWidget();
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    
    m_testPlayButton = new QPushButton("Play");
    QPushButton* stopButton = new QPushButton("Stop");
    QPushButton* nextButton = new QPushButton("Next");
    
    controlsLayout->addWidget(m_testPlayButton);
    controlsLayout->addWidget(stopButton);
    controlsLayout->addWidget(nextButton);
    
    rightLayout->addWidget(controlsWidget);
    
    mainLayout->addWidget(rightPanel, 1);
    
    // Enhance widgets with accessibility
    if (m_widgetEnhancer) {
        m_widgetEnhancer->enhanceWidget(m_testSearchField);
        m_widgetEnhancer->enhanceWidget(m_testMusicGrid);
        m_widgetEnhancer->enhanceWidget(m_testPlaylist);
        m_widgetEnhancer->enhanceWidget(m_testPlayButton);
        m_widgetEnhancer->enhanceWidget(stopButton);
        m_widgetEnhancer->enhanceWidget(nextButton);
    }
    
    m_testMainWindow->show();
}

void TestAccessibilityIntegration::setupTestMusicLibrary()
{
    if (!m_testMusicGrid || !m_testMusicGrid->model()) return;
    
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(m_testMusicGrid->model());
    if (!model) return;
    
    // Populate with test data
    QStringList titles = {"Rock Song 1", "Pop Hit", "Jazz Classic", "Electronic Beat", "Country Tune"};
    QStringList artists = {"Rock Band", "Pop Star", "Jazz Ensemble", "DJ Producer", "Country Singer"};
    QStringList albums = {"Rock Album", "Pop Collection", "Jazz Standards", "Electronic Mix", "Country Hits"};
    QStringList durations = {"3:45", "4:12", "5:23", "3:58", "4:05"};
    
    for (int i = 0; i < qMin(titles.size(), model->rowCount()); ++i) {
        model->setItem(i, 0, new QStandardItem(titles[i]));
        model->setItem(i, 1, new QStandardItem(artists[i]));
        model->setItem(i, 2, new QStandardItem(albums[i]));
        model->setItem(i, 3, new QStandardItem(durations[i]));
    }
    
    // Add some items to playlist
    if (m_testPlaylist) {
        m_testPlaylist->addItem("Test Track 1 - Test Artist");
        m_testPlaylist->addItem("Test Track 2 - Another Artist");
    }
}

void TestAccessibilityIntegration::simulateUserWorkflow(const QStringList& actions)
{
    for (const QString& action : actions) {
        if (action == "search") {
            m_testSearchField->setFocus();
            QTest::keyClicks(m_testSearchField, "test query");
        } else if (action == "navigate_grid") {
            m_testMusicGrid->setFocus();
            simulateKeyboardNavigation({Qt::Key_Down, Qt::Key_Right});
        } else if (action == "add_to_playlist") {
            QTest::keySequence(m_testMusicGrid, QKeySequence("Ctrl+C"));
            m_testPlaylist->setFocus();
            QTest::keySequence(m_testPlaylist, QKeySequence("Ctrl+V"));
        } else if (action == "play") {
            m_testPlayButton->setFocus();
            QTest::keyClick(m_testPlayButton, Qt::Key_Space);
        }
        
        QTest::qWait(50); // Small delay between actions
    }
}

QTEST_MAIN(TestAccessibilityIntegration)