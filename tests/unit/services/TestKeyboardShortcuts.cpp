#include <QTest>
#include <QApplication>
#include <QKeyEvent>
#include <QSignalSpy>
#include "../../../src/services/KeyboardNavigationController.h"
#include "../../../src/services/AccessibilityManager.h"

class TestKeyboardShortcuts : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDocumentedShortcutsRegistered();
    void testPlayerControlShortcuts();
    void testVolumeControlShortcuts();
    void testTimeAnnouncementShortcuts();
    void testNavigationShortcuts();
    void testAccessibilityShortcuts();
    void testShortcutConflicts();

private:
    KeyboardNavigationController* m_navigationController;
    AccessibilityManager* m_accessibilityManager;
    QWidget* m_testWidget;
};

void TestKeyboardShortcuts::initTestCase()
{
    m_testWidget = new QWidget();
    m_accessibilityManager = new AccessibilityManager(m_testWidget);
    m_accessibilityManager->initialize();
    
    m_navigationController = new KeyboardNavigationController(m_accessibilityManager, m_testWidget);
    m_navigationController->initialize();
}

void TestKeyboardShortcuts::cleanupTestCase()
{
    if (m_navigationController) {
        m_navigationController->shutdown();
        delete m_navigationController;
    }
    
    if (m_accessibilityManager) {
        m_accessibilityManager->shutdown();
        delete m_accessibilityManager;
    }
    
    delete m_testWidget;
}

void TestKeyboardShortcuts::testDocumentedShortcutsRegistered()
{
    // Test that all documented shortcuts from keyboard-reference.md are registered
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    
    // Player Controls
    QVERIFY(shortcuts.contains("play_pause"));
    QCOMPARE(shortcuts["play_pause"].sequence, QKeySequence(Qt::Key_Space));
    
    QVERIFY(shortcuts.contains("stop"));
    QCOMPARE(shortcuts["stop"].sequence, QKeySequence(Qt::CTRL | Qt::Key_Space));
    
    QVERIFY(shortcuts.contains("next_track"));
    QCOMPARE(shortcuts["next_track"].sequence, QKeySequence(Qt::CTRL | Qt::Key_Right));
    
    QVERIFY(shortcuts.contains("previous_track"));
    QCOMPARE(shortcuts["previous_track"].sequence, QKeySequence(Qt::CTRL | Qt::Key_Left));
    
    // Volume Control
    QVERIFY(shortcuts.contains("volume_up"));
    QCOMPARE(shortcuts["volume_up"].sequence, QKeySequence(Qt::CTRL | Qt::Key_Up));
    
    QVERIFY(shortcuts.contains("volume_down"));
    QCOMPARE(shortcuts["volume_down"].sequence, QKeySequence(Qt::CTRL | Qt::Key_Down));
    
    QVERIFY(shortcuts.contains("volume_up_fine"));
    QCOMPARE(shortcuts["volume_up_fine"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up));
    
    QVERIFY(shortcuts.contains("volume_down_fine"));
    QCOMPARE(shortcuts["volume_down_fine"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down));
    
    QVERIFY(shortcuts.contains("mute"));
    QCOMPARE(shortcuts["mute"].sequence, QKeySequence(Qt::CTRL | Qt::Key_M));
    
    QVERIFY(shortcuts.contains("reset_volume"));
    QCOMPARE(shortcuts["reset_volume"].sequence, QKeySequence(Qt::CTRL | Qt::Key_0));
}

void TestKeyboardShortcuts::testPlayerControlShortcuts()
{
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    
    // Seeking controls
    QVERIFY(shortcuts.contains("fast_forward"));
    QCOMPARE(shortcuts["fast_forward"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Right));
    
    QVERIFY(shortcuts.contains("rewind"));
    QCOMPARE(shortcuts["rewind"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Left));
}

void TestKeyboardShortcuts::testVolumeControlShortcuts()
{
    // Test that both documented and legacy volume shortcuts are available
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    
    // Documented shortcuts
    QVERIFY(shortcuts.contains("volume_up"));
    QVERIFY(shortcuts.contains("volume_down"));
    QVERIFY(shortcuts.contains("volume_up_fine"));
    QVERIFY(shortcuts.contains("volume_down_fine"));
    
    // Legacy shortcuts for compatibility
    QVERIFY(shortcuts.contains("volume_up_legacy"));
    QCOMPARE(shortcuts["volume_up_legacy"].sequence, QKeySequence(Qt::Key_Plus));
    
    QVERIFY(shortcuts.contains("volume_down_legacy"));
    QCOMPARE(shortcuts["volume_down_legacy"].sequence, QKeySequence(Qt::Key_Minus));
}

void TestKeyboardShortcuts::testTimeAnnouncementShortcuts()
{
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    
    // Time information shortcuts
    QVERIFY(shortcuts.contains("current_time"));
    QCOMPARE(shortcuts["current_time"].sequence, QKeySequence(Qt::CTRL | Qt::Key_T));
    
    QVERIFY(shortcuts.contains("remaining_time"));
    QCOMPARE(shortcuts["remaining_time"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    
    QVERIFY(shortcuts.contains("total_duration"));
    QCOMPARE(shortcuts["total_duration"].sequence, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T));
    
    QVERIFY(shortcuts.contains("track_info"));
    QCOMPARE(shortcuts["track_info"].sequence, QKeySequence(Qt::CTRL | Qt::Key_D));
}

void TestKeyboardShortcuts::testNavigationShortcuts()
{
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    
    // Navigation shortcuts (Alt+Letter)
    QVERIFY(shortcuts.contains("menu_bar"));
    QCOMPARE(shortcuts["menu_bar"].sequence, QKeySequence(Qt::ALT | Qt::Key_M));
    
    QVERIFY(shortcuts.contains("library"));
    QCOMPARE(shortcuts["library"].sequence, QKeySequence(Qt::ALT | Qt::Key_L));
    
    QVERIFY(shortcuts.contains("playlist"));
    QCOMPARE(shortcuts["playlist"].sequence, QKeySequence(Qt::ALT | Qt::Key_P));
    
    QVERIFY(shortcuts.contains("controls"));
    QCOMPARE(shortcuts["controls"].sequence, QKeySequence(Qt::ALT | Qt::Key_C));
    
    QVERIFY(shortcuts.contains("status"));
    QCOMPARE(shortcuts["status"].sequence, QKeySequence(Qt::ALT | Qt::Key_S));
}

void TestKeyboardShortcuts::testAccessibilityShortcuts()
{
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    
    // Accessibility shortcuts
    QVERIFY(shortcuts.contains("toggle_verbosity"));
    QCOMPARE(shortcuts["toggle_verbosity"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    
    QVERIFY(shortcuts.contains("where_am_i"));
    QCOMPARE(shortcuts["where_am_i"].sequence, QKeySequence(Qt::CTRL | Qt::Key_Question));
    
    QVERIFY(shortcuts.contains("whats_this"));
    QCOMPARE(shortcuts["whats_this"].sequence, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Question));
    
    QVERIFY(shortcuts.contains("accessibility_settings"));
    QCOMPARE(shortcuts["accessibility_settings"].sequence, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Comma));
}

void TestKeyboardShortcuts::testShortcutConflicts()
{
    // Test that there are no conflicting shortcuts
    auto shortcuts = m_navigationController->getKeyboardShortcuts();
    QSet<QKeySequence> usedSequences;
    
    for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it) {
        const QKeySequence& sequence = it.value().sequence;
        
        // Check for conflicts (same sequence used by different actions)
        if (usedSequences.contains(sequence)) {
            // Allow legacy shortcuts to coexist with primary shortcuts
            QString action = it.key();
            if (!action.endsWith("_legacy")) {
                QFAIL(QString("Shortcut conflict detected for sequence: %1 (action: %2)")
                      .arg(sequence.toString(), action).toLocal8Bit());
            }
        }
        
        usedSequences.insert(sequence);
    }
}

QTEST_MAIN(TestKeyboardShortcuts)
#include "TestKeyboardShortcuts.moc"