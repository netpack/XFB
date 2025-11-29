// Quick test to verify keyboard navigation fix
// This can be integrated into the existing test suite

#include <QApplication>
#include <QTableView>
#include <QKeyEvent>
#include <QTest>
#include <QFocusEvent>

class KeyboardNavigationTest : public QObject
{
    Q_OBJECT

private slots:
    void testTableViewTabNavigation()
    {
        // Create a simple test setup
        QWidget parent;
        QTableView* table = new QTableView(&parent);
        QPushButton* button1 = new QPushButton("Before", &parent);
        QPushButton* button2 = new QPushButton("After", &parent);
        
        // Set up focus chain: button1 -> table -> button2
        QWidget::setTabOrder(button1, table);
        QWidget::setTabOrder(table, button2);
        
        // Apply our accessibility fixes
        table->setFocusPolicy(Qt::StrongFocus);
        table->setTabKeyNavigation(true);
        table->setAttribute(Qt::WA_KeyboardFocusChange, true);
        
        parent.show();
        
        // Start with focus on button1
        button1->setFocus();
        QVERIFY(button1->hasFocus());
        
        // Tab to table
        QTest::keyClick(&parent, Qt::Key_Tab);
        QVERIFY(table->hasFocus());
        
        // Tab out of table - this should work now
        QTest::keyClick(&parent, Qt::Key_Tab);
        QVERIFY(button2->hasFocus());
        
        // Tab back to table
        QTest::keyClick(&parent, Qt::Key_Tab, Qt::ShiftModifier);
        QVERIFY(table->hasFocus());
        
        // Tab back to button1
        QTest::keyClick(&parent, Qt::Key_Tab, Qt::ShiftModifier);
        QVERIFY(button1->hasFocus());
    }
};

// This test verifies that:
// 1. Users can tab into the table
// 2. Users can tab out of the table (no keyboard trap)
// 3. Shift+Tab works in both directions
// 4. Focus chain is properly maintained