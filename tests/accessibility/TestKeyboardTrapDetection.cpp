#include <QTest>
#include <QApplication>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QTimer>
#include "../../src/player.h"
#include "../../src/services/KeyboardNavigationController.h"
#include "../../src/services/AccessibilityManager.h"

class TestKeyboardTrapDetection : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testMusicViewTabNavigation();
    void testJinglesViewTabNavigation();
    void testPubViewTabNavigation();
    void testProgramsViewTabNavigation();
    void testPlaylistTabNavigation();
    void testFullInterfaceTabNavigation();
    void testReverseTabNavigation();
    void testKeyboardTrapDetection();

private:
    player* m_player;
    KeyboardNavigationController* m_navigationController;
    AccessibilityManager* m_accessibilityManager;
    
    bool canTabOutOfWidget(QWidget* widget);
    bool canTabIntoWidget(QWidget* widget);
    QWidget* getNextFocusableWidget(QWidget* current, bool forward = true);
    void simulateTabKey(QWidget* widget, bool forward = true);
};

void TestKeyboardTrapDetection::initTestCase()
{
    // Initialize the main player window
    m_player = new player();
    m_player->show();
    
    // Initialize accessibility services
    m_accessibilityManager = new AccessibilityManager(m_player);
    m_accessibilityManager->initialize();
    
    m_navigationController = new KeyboardNavigationController(m_player, m_accessibilityManager);
    m_navigationController->initialize();
    
    // Wait for UI to be fully initialized
    QTest::qWait(500);
}

void TestKeyboardTrapDetection::cleanupTestCase()
{
    if (m_navigationController) {
        m_navigationController->shutdown();
        delete m_navigationController;
    }
    
    if (m_accessibilityManager) {
        m_accessibilityManager->shutdown();
        delete m_accessibilityManager;
    }
    
    if (m_player) {
        m_player->close();
        delete m_player;
    }
}

void TestKeyboardTrapDetection::testMusicViewTabNavigation()
{
    QTableView* musicView = m_player->getMusicView();
    QVERIFY(musicView != nullptr);
    
    // Test that we can tab into the music view
    QVERIFY(canTabIntoWidget(musicView));
    
    // Test that we can tab out of the music view
    QVERIFY(canTabOutOfWidget(musicView));
    
    // Verify focus policies are set correctly
    QCOMPARE(musicView->focusPolicy(), Qt::StrongFocus);
    QVERIFY(musicView->testAttribute(Qt::WA_KeyboardFocusChange));
}

void TestKeyboardTrapDetection::testJinglesViewTabNavigation()
{
    QTableView* jinglesView = m_player->findChild<QTableView*>("jinglesView");
    QVERIFY(jinglesView != nullptr);
    
    QVERIFY(canTabIntoWidget(jinglesView));
    QVERIFY(canTabOutOfWidget(jinglesView));
    
    QCOMPARE(jinglesView->focusPolicy(), Qt::StrongFocus);
    QVERIFY(jinglesView->testAttribute(Qt::WA_KeyboardFocusChange));
}

void TestKeyboardTrapDetection::testPubViewTabNavigation()
{
    QTableView* pubView = m_player->findChild<QTableView*>("pubView");
    QVERIFY(pubView != nullptr);
    
    QVERIFY(canTabIntoWidget(pubView));
    QVERIFY(canTabOutOfWidget(pubView));
    
    QCOMPARE(pubView->focusPolicy(), Qt::StrongFocus);
    QVERIFY(pubView->testAttribute(Qt::WA_KeyboardFocusChange));
}

void TestKeyboardTrapDetection::testProgramsViewTabNavigation()
{
    QTableView* programsView = m_player->findChild<QTableView*>("programsView");
    QVERIFY(programsView != nullptr);
    
    QVERIFY(canTabIntoWidget(programsView));
    QVERIFY(canTabOutOfWidget(programsView));
    
    QCOMPARE(programsView->focusPolicy(), Qt::StrongFocus);
    QVERIFY(programsView->testAttribute(Qt::WA_KeyboardFocusChange));
}

void TestKeyboardTrapDetection::testPlaylistTabNavigation()
{
    QListWidget* playlist = m_player->findChild<QListWidget*>("playlist");
    QVERIFY(playlist != nullptr);
    
    QVERIFY(canTabIntoWidget(playlist));
    QVERIFY(canTabOutOfWidget(playlist));
    
    QCOMPARE(playlist->focusPolicy(), Qt::StrongFocus);
    QVERIFY(playlist->testAttribute(Qt::WA_KeyboardFocusChange));
}

void TestKeyboardTrapDetection::testFullInterfaceTabNavigation()
{
    // Test complete tab navigation through the entire interface
    QWidget* startWidget = m_player->findChild<QPushButton*>();
    QVERIFY(startWidget != nullptr);
    
    startWidget->setFocus();
    QWidget* currentWidget = startWidget;
    QSet<QWidget*> visitedWidgets;
    int maxIterations = 100; // Prevent infinite loops
    int iterations = 0;
    
    do {
        visitedWidgets.insert(currentWidget);
        
        // Simulate tab key
        simulateTabKey(currentWidget, true);
        QTest::qWait(10); // Small delay for focus change
        
        QWidget* newFocus = QApplication::focusWidget();
        
        // Verify we moved to a different widget or completed the cycle
        if (newFocus == startWidget && iterations > 5) {
            // Successfully completed the tab cycle
            break;
        }
        
        QVERIFY2(newFocus != currentWidget || iterations == 0, 
                "Keyboard trap detected - focus didn't move from widget");
        
        currentWidget = newFocus;
        iterations++;
        
    } while (iterations < maxIterations && currentWidget != startWidget);
    
    QVERIFY2(iterations < maxIterations, "Tab navigation took too many iterations - possible infinite loop");
}

void TestKeyboardTrapDetection::testReverseTabNavigation()
{
    // Test Shift+Tab navigation
    QWidget* startWidget = m_player->findChild<QPushButton*>();
    QVERIFY(startWidget != nullptr);
    
    startWidget->setFocus();
    QWidget* currentWidget = startWidget;
    int maxIterations = 100;
    int iterations = 0;
    
    do {
        // Simulate Shift+Tab key
        simulateTabKey(currentWidget, false);
        QTest::qWait(10);
        
        QWidget* newFocus = QApplication::focusWidget();
        
        if (newFocus == startWidget && iterations > 5) {
            break;
        }
        
        QVERIFY2(newFocus != currentWidget || iterations == 0, 
                "Reverse keyboard trap detected - focus didn't move from widget");
        
        currentWidget = newFocus;
        iterations++;
        
    } while (iterations < maxIterations && currentWidget != startWidget);
    
    QVERIFY2(iterations < maxIterations, "Reverse tab navigation took too many iterations");
}

void TestKeyboardTrapDetection::testKeyboardTrapDetection()
{
    // Test all table views and list widgets specifically
    QList<QTableView*> tableViews = m_player->findChildren<QTableView*>();
    QList<QListWidget*> listWidgets = m_player->findChildren<QListWidget*>();
    
    // Test each table view
    for (QTableView* table : tableViews) {
        if (table->isVisible() && table->isEnabled()) {
            QVERIFY2(canTabOutOfWidget(table), 
                    QString("Keyboard trap detected in table: %1").arg(table->objectName()).toLocal8Bit());
        }
    }
    
    // Test each list widget
    for (QListWidget* list : listWidgets) {
        if (list->isVisible() && list->isEnabled()) {
            QVERIFY2(canTabOutOfWidget(list), 
                    QString("Keyboard trap detected in list: %1").arg(list->objectName()).toLocal8Bit());
        }
    }
}

bool TestKeyboardTrapDetection::canTabOutOfWidget(QWidget* widget)
{
    if (!widget || !widget->isVisible() || !widget->isEnabled()) {
        return false;
    }
    
    // Set focus to the widget
    widget->setFocus();
    QTest::qWait(10);
    
    if (QApplication::focusWidget() != widget) {
        return false; // Couldn't focus the widget
    }
    
    // Try to tab out
    simulateTabKey(widget, true);
    QTest::qWait(10);
    
    QWidget* newFocus = QApplication::focusWidget();
    
    // Check if focus moved to a different widget
    return newFocus != widget;
}

bool TestKeyboardTrapDetection::canTabIntoWidget(QWidget* widget)
{
    if (!widget || !widget->isVisible() || !widget->isEnabled()) {
        return false;
    }
    
    // Find a widget before this one in the tab order
    QWidget* previousWidget = widget->previousInFocusChain();
    if (!previousWidget || previousWidget == widget) {
        return false;
    }
    
    // Focus the previous widget
    previousWidget->setFocus();
    QTest::qWait(10);
    
    // Tab to our target widget
    simulateTabKey(previousWidget, true);
    QTest::qWait(10);
    
    // Check if we reached our target widget (or at least moved focus)
    return QApplication::focusWidget() != previousWidget;
}

void TestKeyboardTrapDetection::simulateTabKey(QWidget* widget, bool forward)
{
    Qt::Key key = forward ? Qt::Key_Tab : Qt::Key_Backtab;
    Qt::KeyboardModifiers modifiers = forward ? Qt::NoModifier : Qt::ShiftModifier;
    
    QKeyEvent keyPress(QEvent::KeyPress, key, modifiers);
    QKeyEvent keyRelease(QEvent::KeyRelease, key, modifiers);
    
    QApplication::sendEvent(widget, &keyPress);
    QApplication::sendEvent(widget, &keyRelease);
}

QTEST_MAIN(TestKeyboardTrapDetection)
#include "TestKeyboardTrapDetection.moc"