#ifndef SCHEDULEDIALOG_H
#define SCHEDULEDIALOG_H

#include <QDialog>

#include "../services/ProgrammeSchedule.h"

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QTableView;

/**
 * @brief What is booked to go on air, and when.
 *
 * The as-run log's mirror image: that window answers "what did we play", this
 * one answers "what are we going to". Two tabs, because an operator arrives
 * with one of two questions and they want different shapes of answer — what
 * is going out over the next few days, in the order the listener will hear
 * it, and the standing bookings behind that, each with whether XFB can
 * actually honour it.
 *
 * The second tab is the one that earns its place: a booking whose file has
 * been moved, or a one-off whose minute went by while XFB was closed, is
 * invisible everywhere else in the application and silently airs nothing.
 *
 * Read-only. Bookings are made in "Add a publicity" and "Add a program", and
 * this deliberately does not become a second place to change them.
 *
 * Built in code, no .ui file: the committed ui_*.h headers in src/ shadow the
 * generated ones and a stale one fails silently.
 *
 * Everything here is reachable from the keyboard and named for a screen
 * reader.
 */
class ScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScheduleDialog(QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

protected:
    /** The window is kept alive between openings, so what it shows is
     *  refreshed every time it comes back rather than only once. */
    void showEvent(QShowEvent *event) override;

private slots:
    void refreshUpcoming();
    void refreshBookings();
    void exportUpcomingCsv();
    void showBookingDetail();

private:
    QWidget *buildUpcomingTab();
    QWidget *buildBookingsTab();

    /** Common column layout for both tables, plus their accessible names. */
    static void prepareTable(QTableView *view, const QString &name,
                             const QString &description);

    /** "in 20 minutes", "tomorrow at 08:10" — how far off an airing is. */
    static QString relativeText(const QDateTime &when, const QDateTime &now);

    /** Writes rows to a CSV the operator picks. Returns false when cancelled. */
    bool writeCsv(const QString &suggestedName, const QStringList &preamble,
                  const QStringList &header, const QList<QStringList> &rows);

    // --- what is coming up ---
    QDateTimeEdit      *m_from = nullptr;
    QDateTimeEdit      *m_to = nullptr;
    QComboBox          *m_kind = nullptr;
    QLineEdit          *m_search = nullptr;
    QPushButton        *m_refresh = nullptr;
    QPushButton        *m_export = nullptr;
    QTableView         *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel             *m_summary = nullptr;

    // --- the bookings behind it ---
    QCheckBox          *m_onlyProblems = nullptr;
    QPushButton        *m_refreshBookings = nullptr;
    QTableView         *m_bookingTable = nullptr;
    QStandardItemModel *m_bookingModel = nullptr;
    QLabel             *m_bookingSummary = nullptr;
    QLabel             *m_bookingDetail = nullptr;
};

#endif // SCHEDULEDIALOG_H
