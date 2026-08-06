#include <QtTest/QtTest>

#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "PadBoard.h"

/**
 * @brief The Pads grid keeps exactly one pad in the Tab chain.
 *
 * The cart wall is 24 pads on a default grid and up to 96 on the largest one.
 * Every pad used to be Qt::StrongFocus, so a keyboard-only operator had to
 * press Tab once per pad to get past the panel — the arrows walked the grid,
 * but nothing made Tab skip it. The fix is the standard roving tabindex: the
 * current pad is the only one in the chain, the rest keep click focus so the
 * arrows and the mouse still reach them.
 *
 * These tests are about the focus chain, not about playback, so they never
 * assign a file to a pad and no media player is ever created.
 */
class TestPadBoardFocus : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void tabReachesExactlyOnePad();
    void arrowsStillWalkTheWholeGrid();
    void theStopFollowsTheFocusedPad();
    void theStopSurvivesABankSwitch();
    void theStopSurvivesAGridResize();
    void keysStillWorkOffTheTabChain();

private:
    /** A window that can press Tab: focusNextChild() is protected. */
    class TabWindow : public QWidget
    {
    public:
        using QWidget::focusNextChild;
    };

    /** Every pad Tab lands on, walking the chain from the widget above it. */
    QList<PadButton *> tabbablePads();
    /** The pads of the bank on screen, in grid order. */
    QList<PadButton *> visiblePads() const;
    void sendKey(int key);

    TabWindow *m_window = nullptr;
    QLineEdit *m_before = nullptr;
    PadBoardWidget *m_board = nullptr;
};

void TestPadBoardFocus::initTestCase()
{
    // Never read or rewrite the operator's real xfb.conf: the board saves its
    // grid size and every pad as soon as they change.
    QStandardPaths::setTestModeEnabled(true);
}

void TestPadBoardFocus::init()
{
    // The board persists its grid size and bank the moment they change, and
    // these tests change both. Start every one of them from the defaults.
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + QStringLiteral("/xfb.conf"),
                       QSettings::IniFormat);
    settings.remove(QStringLiteral("PadBoard"));
    settings.sync();

    m_window = new TabWindow;
    auto *layout = new QVBoxLayout(m_window);
    m_before = new QLineEdit(m_window);
    m_board = new PadBoardWidget(m_window);
    layout->addWidget(m_before);
    layout->addWidget(m_board, 1);
    layout->addWidget(new QLineEdit(m_window));

    m_window->show();
    // The offscreen platform never activates a window on its own, and an
    // inactive window gets no QFocusEvent at all — setFocus() would move the
    // focus without the board ever hearing about it.
    m_window->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(m_window));
}

void TestPadBoardFocus::cleanup()
{
    delete m_window;
    m_window = nullptr;
    m_before = nullptr;
    m_board = nullptr;
}

QList<PadButton *> TestPadBoardFocus::tabbablePads()
{
    QList<PadButton *> seen;
    m_before->setFocus(Qt::OtherFocusReason);
    // One lap of the chain: it comes back to where it started.
    for (int i = 0; i < 400; ++i) {
        m_window->focusNextChild();
        QWidget *focused = m_window->focusWidget();
        if (!focused || focused == m_before)
            break;
        if (auto *pad = qobject_cast<PadButton *>(focused))
            seen << pad;
    }
    return seen;
}

QList<PadButton *> TestPadBoardFocus::visiblePads() const
{
    const QList<PadButton *> pads = m_board->findChildren<PadButton *>();
    QList<PadButton *> onScreen;
    for (PadButton *pad : pads) {
        if (pad->isVisible())
            onScreen << pad;
    }
    return onScreen;
}

void TestPadBoardFocus::sendKey(int key)
{
    QWidget *focused = m_window->focusWidget();
    QVERIFY(focused);
    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
    QApplication::sendEvent(focused, &event);
}

void TestPadBoardFocus::tabReachesExactlyOnePad()
{
    // The grid really is big enough for this to matter.
    QVERIFY(visiblePads().size() >= 24);

    const QList<PadButton *> stops = tabbablePads();
    QCOMPARE(stops.size(), 1);
    // Nothing has been focused yet, so the stop is the first pad.
    QCOMPARE(stops.first()->row(), 0);
    QCOMPARE(stops.first()->col(), 0);
}

void TestPadBoardFocus::arrowsStillWalkTheWholeGrid()
{
    const QList<PadButton *> pads = visiblePads();
    QVERIFY(!pads.isEmpty());

    for (PadButton *target : pads) {
        // Walk there from the top-left: down the rows, then along the columns.
        pads.first()->setFocus(Qt::TabFocusReason);
        for (int r = 0; r < target->row(); ++r)
            sendKey(Qt::Key_Down);
        for (int c = 0; c < target->col(); ++c)
            sendKey(Qt::Key_Right);

        QCOMPARE(m_window->focusWidget(), static_cast<QWidget *>(target));
    }
}

void TestPadBoardFocus::theStopFollowsTheFocusedPad()
{
    PadButton *wanted = nullptr;
    const QList<PadButton *> pads = visiblePads();
    for (PadButton *pad : pads) {
        if (pad->row() == 2 && pad->col() == 3)
            wanted = pad;
    }
    QVERIFY(wanted);

    wanted->setFocus(Qt::TabFocusReason);

    const QList<PadButton *> stops = tabbablePads();
    QCOMPARE(stops.size(), 1);
    QCOMPARE(stops.first(), wanted);
}

void TestPadBoardFocus::theStopSurvivesABankSwitch()
{
    const QList<PadButton *> pads = visiblePads();
    for (PadButton *pad : pads) {
        if (pad->row() == 1 && pad->col() == 2)
            pad->setFocus(Qt::TabFocusReason);
    }

    auto *bankBox = m_board->findChild<QComboBox *>();
    QVERIFY(bankBox);
    bankBox->setCurrentIndex(3);

    // The next bank is a different set of pads, and it needs a stop of its own.
    const QList<PadButton *> stops = tabbablePads();
    QCOMPARE(stops.size(), 1);
    QCOMPARE(stops.first()->row(), 1);
    QCOMPARE(stops.first()->col(), 2);
}

void TestPadBoardFocus::theStopSurvivesAGridResize()
{
    const QList<QSpinBox *> spins = m_board->findChildren<QSpinBox *>();
    QCOMPARE(spins.size(), 2);
    QSpinBox *rows = spins.at(0);
    QSpinBox *cols = spins.at(1);

    for (PadButton *pad : visiblePads()) {
        if (pad->row() == 3 && pad->col() == 5)
            pad->setFocus(Qt::TabFocusReason);
    }

    // Shrink the grid out from under the stop: row 3 and column 5 are gone.
    rows->setValue(2);
    cols->setValue(2);

    QList<PadButton *> stops = tabbablePads();
    QCOMPARE(stops.size(), 1);
    QVERIFY(stops.first()->row() < 2);
    QVERIFY(stops.first()->col() < 2);

    // And growing it again must not hand out a second stop.
    rows->setValue(4);
    cols->setValue(6);

    stops = tabbablePads();
    QCOMPARE(stops.size(), 1);
}

void TestPadBoardFocus::keysStillWorkOffTheTabChain()
{
    // A standalone pad, so configureRequested does not open the board's modal
    // dialog and hang the run.
    auto *pad = new PadButton(1, 1, m_window);
    pad->setTabStop(false);
    pad->show(); // a child added after the window is up starts hidden

    int configureRequests = 0;
    int messages = 0;
    connect(pad, &PadButton::configureRequested,
            [&configureRequests](int, int) { ++configureRequests; });
    connect(pad, &PadButton::message, [&messages](const QString &) { ++messages; });

    pad->setFocus(Qt::TabFocusReason);
    QVERIFY(pad->hasFocus());

    QKeyEvent f2(QEvent::KeyPress, Qt::Key_F2, Qt::NoModifier);
    QApplication::sendEvent(pad, &f2);
    QCOMPARE(configureRequests, 1);

    // The pad is empty, so triggering it reports that rather than playing.
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(pad, &enter);
    QCOMPARE(messages, 1);

    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(pad, &escape);
    QVERIFY(escape.isAccepted());

    delete pad;
}

QTEST_MAIN(TestPadBoardFocus)
#include "test_pad_board_focus.moc"
