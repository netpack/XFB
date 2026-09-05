#include <QtTest/QtTest>

#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>

#include "add_program.h"
#include "add_pub.h"

/**
 * @brief Cancel closes the Add a publicity and Add a program windows.
 *
 * Both had a Cancel button in the .ui and no slot behind it. Qt's
 * connectSlotsByName had nothing called on_pushButton_2_clicked to find, so
 * the button was inert and the only way out of either window was Save or the
 * title bar — on a full-screen studio machine, sometimes neither.
 *
 * The test is written the way player.cpp uses these dialogs: exec(), and
 * "works" means the button ends that event loop. A watchdog closes the window
 * if it does not, which is what the bug looked like and what makes the test
 * fail rather than hang.
 *
 * Offscreen, because it builds real widgets; the dialogs' constructors write a
 * placeholder row, so an in-memory database with their three tables stands in
 * for the station's.
 */
class TestAddDialogCancel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void cancelClosesAddPublicity();
    void cancelClosesAddProgram();

private:
    /** Clicks Cancel from inside exec() and reports what happened. */
    template <typename Dialog> void exerciseCancel();
};

void TestAddDialogCancel::initTestCase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                QStringLiteral("xfb_connection"));
    db.setDatabaseName(QStringLiteral(":memory:"));
    QVERIFY2(db.open(), "the test database could not be opened");

    QSqlQuery query(db);
    QVERIFY(query.exec("CREATE TABLE pub (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "name TEXT, path TEXT)"));
    QVERIFY(query.exec("CREATE TABLE programs (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "name TEXT, path TEXT)"));
    // The scheduler insert these dialogs use lists fifteen values positionally.
    QVERIFY(query.exec("CREATE TABLE scheduler (id, c2, c3, c4, c5, c6, c7, c8, "
                       "c9, c10, c11, c12, c13, c14, c15)"));
}

void TestAddDialogCancel::cleanupTestCase()
{
    QSqlDatabase::database(QStringLiteral("xfb_connection")).close();
    QSqlDatabase::removeDatabase(QStringLiteral("xfb_connection"));
}

template <typename Dialog>
void TestAddDialogCancel::exerciseCancel()
{
    Dialog dialog;

    auto *cancel = dialog.template findChild<QPushButton *>(QStringLiteral("pushButton_2"));
    QVERIFY2(cancel, "the window has no pushButton_2");
    QCOMPARE(cancel->text(), QStringLiteral("Cancel"));

    QTimer::singleShot(0, cancel, [cancel]() { cancel->click(); });

    // If Cancel is inert this is what ends the window, and the checks below
    // then say so instead of the test hanging until the harness kills it.
    QTimer watchdog;
    watchdog.setSingleShot(true);
    connect(&watchdog, &QTimer::timeout, &dialog, [&dialog]() {
        dialog.done(QDialog::Accepted);
    });
    watchdog.start(2000);

    const int result = dialog.exec();

    QVERIFY2(watchdog.isActive(), "Cancel did not close the window; the watchdog did");
    QCOMPARE(result, int(QDialog::Rejected));
    QVERIFY(!dialog.isVisible());
}

void TestAddDialogCancel::cancelClosesAddPublicity()
{
    exerciseCancel<add_pub>();
}

void TestAddDialogCancel::cancelClosesAddProgram()
{
    exerciseCancel<add_program>();
}

QTEST_MAIN(TestAddDialogCancel)
#include "test_add_dialog_cancel.moc"
