#ifndef AIRLOGDIALOG_H
#define AIRLOGDIALOG_H

#include <QDialog>

#include "../services/AirLog.h"

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStandardItemModel;
class QTableView;
class QTextBrowser;

/**
 * @brief The as-run log, and the advertiser report built from it.
 *
 * Two things a station needs from the same rows: "what did we play" and
 * "prove my spot ran", so they are two tabs of one window rather than two
 * windows that would both have to be found.
 *
 * Built in code, no .ui file: the committed ui_*.h headers in src/ shadow the
 * generated ones and a stale one fails silently.
 *
 * Everything here is reachable from the keyboard and named for a screen
 * reader — the log is evidence, and evidence a blind operator cannot read is
 * not evidence.
 */
class AirLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AirLogDialog(QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void refreshLog();
    void exportLogCsv();
    void refreshReport();
    void exportReportCsv();
    void applyRetention();

private:
    QWidget *buildLogTab();
    QWidget *buildReportTab();
    void     reloadAdvertisements();
    /** Common column layout for both tables, plus their accessible names. */
    static void prepareTable(QTableView *view, const QString &name,
                             const QString &description);
    /** Writes rows to a CSV the operator picks. Returns false when cancelled. */
    bool writeCsv(const QString &suggestedName, const QStringList &preamble,
                  const QStringList &header,
                  const QList<QStringList> &rows);

    // --- the log ---
    QDateTimeEdit      *m_from = nullptr;
    QDateTimeEdit      *m_to = nullptr;
    QComboBox          *m_source = nullptr;
    QLineEdit          *m_search = nullptr;
    QPushButton        *m_refresh = nullptr;
    QPushButton        *m_exportLog = nullptr;
    QTableView         *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel             *m_summary = nullptr;
    QSpinBox           *m_retention = nullptr;

    // --- the advertiser report ---
    QComboBox          *m_advert = nullptr;
    QDateTimeEdit      *m_reportFrom = nullptr;
    QDateTimeEdit      *m_reportTo = nullptr;
    QPushButton        *m_buildReport = nullptr;
    QPushButton        *m_exportReport = nullptr;
    QTextBrowser       *m_reportHeader = nullptr;
    QTableView         *m_reportTable = nullptr;
    QStandardItemModel *m_reportModel = nullptr;
    QList<AirLog::Record> m_reportRows;
    QString             m_reportTitle;
};

#endif // AIRLOGDIALOG_H
