#ifndef TESTACCESSIBILITYINTEGRATION_H
#define TESTACCESSIBILITYINTEGRATION_H

#include <QObject>
#include <QTest>
#include <QApplication>
#include <QWidget>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QAccessible>
#include <QAccessibleInterface>
#include <QKeyEvent>
#include <QTimer>
#include <QSignalSpy>

class AccessibilityManager;
class KeyboardNavigationController;
class AudioFeedbackService;
class LiveRegionManager;
class WidgetAccessibilityEnhancer;

/**
 * @brief Comprehensive integration tests for accessibility system
 * 
 * Tests end-to-end accessibility workflows including:
 * - Complete broadcasting workflow with screen reader simulation
 * - Cross-component accessibility coordination
 * - Keyboard navigation flows
 * - Audio feedback integration
 * - Live region updates
 */
class TestAccessibilityIntegration : public QObject
{
    Q_OBJECT

public:
    TestAccessibilityIntegration();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // End-to-end workflow tests
    void testCompleteBroadcastingWorkflow();
    void testMusicLibraryManagementWorkflow();
    void testPlaylistCreationWorkflow();
    void testLiveBroadcastingWorkflow();
    void testEmergencyProceduresWorkflow();

    // Cross-component coordination tests
    void testAccessibilityManagerCoordination();
    void testKeyboardNavigationIntegration();
    void testAudioFeedbackIntegration();
    void testLiveRegionCoordination();
    void testSettingsIntegration();

    // Screen reader simulation tests
    void testScreenReaderAnnouncements();
    void testFocusChangeAnnouncements();
    void testDynamicContentAnnouncements();
    void testErrorMessageAnnouncements();

    // Performance integration tests
    void testAccessibilityPerformanceUnderLoad();
    void testMemoryUsageWithAccessibility();
    void testAnnouncementLatency();

    // Error handling integration tests
    void testAccessibilityErrorRecovery();
    void testGracefulDegradation();
    void testATSPIConnectionFailure();

private:
    // Helper methods
    void simulateKeyboardNavigation(const QList<Qt::Key>& keys);
    void simulateScreenReaderInteraction(QWidget* widget);
    void verifyAccessibilityMetadata(QWidget* widget, const QString& expectedName, 
                                   const QString& expectedDescription = QString());
    void verifyKeyboardAccessibility(QWidget* widget);
    void verifyAudioFeedback(const QString& expectedMessage);
    void createTestBroadcastingEnvironment();
    void setupTestMusicLibrary();
    void simulateUserWorkflow(const QStringList& actions);

    // Test data and fixtures
    QWidget* m_testMainWindow;
    QTableView* m_testMusicGrid;
    QListWidget* m_testPlaylist;
    QPushButton* m_testPlayButton;
    QLineEdit* m_testSearchField;
    
    AccessibilityManager* m_accessibilityManager;
    KeyboardNavigationController* m_keyboardController;
    AudioFeedbackService* m_audioFeedback;
    LiveRegionManager* m_liveRegionManager;
    WidgetAccessibilityEnhancer* m_widgetEnhancer;
    
    QSignalSpy* m_announcementSpy;
    QSignalSpy* m_focusChangeSpy;
    QSignalSpy* m_liveRegionSpy;
    
    QStringList m_capturedAnnouncements;
    bool m_accessibilityEnabled;
};

#endif // TESTACCESSIBILITYINTEGRATION_H