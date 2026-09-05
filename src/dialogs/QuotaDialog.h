#ifndef QUOTADIALOG_H
#define QUOTADIALOG_H

#include <QDialog>
#include <QList>

#include "../services/MusicQuota.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextBrowser;

/**
 * @brief Marking the library for the national music quota, and reading back
 *        what actually aired against it.
 *
 * Three tabs, because they are three jobs done on three different days:
 *
 *  - **Marking** — say of a track where it comes from, what language it is
 *    sung in, whether its performers are EU citizens and when it was first
 *    published. Done in bulk over a selection, because a library is thousands
 *    of tracks and nobody is going to open a window per record.
 *  - **Report** — the month, and the weekday daytime inside it, measured
 *    against the station's obligations, with the detail the figures came from
 *    and a CSV of it for whoever asks.
 *  - **Rules** — the percentages themselves, which are not the same law in
 *    every country and not the same law every year.
 *
 * Built in code, no .ui file: the committed ui_*.h headers in src/ shadow the
 * generated ones and a stale one fails silently.
 *
 * Everything is reachable from the keyboard and named for a screen reader —
 * this window produces evidence, and evidence a blind operator cannot read is
 * not evidence.
 */
class QuotaDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param preselected music ids to select in the track list on opening —
     *        what the operator had highlighted in the music table.
     */
    explicit QuotaDialog(QWidget *parent = nullptr,
                         const QList<qint64> &preselected = QList<qint64>());

    /** Reloads the track list and highlights @a ids. The window is kept alive
     *  between openings, so a new selection has to move the selection here. */
    void preselect(const QList<qint64> &ids);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void reloadTracks();
    void applyToSelection();
    void clearSelection();
    void seedFromLibrary();
    void updateSelectionSummary();

    void buildReport();
    void exportReportCsv();
    void showThisMonth();
    void showLastMonth();

    void applySettings();
    void restorePortugalPreset();

private:
    QWidget *buildTracksTab();
    QWidget *buildReportTab();
    QWidget *buildRulesTab();

    void loadSettingsInto(const MusicQuota::Settings &s);
    MusicQuota::Settings readSettingsFromForm() const;

    QList<qint64> selectedMusicIds() const;
    void selectMusicIds(const QList<qint64> &ids);

    /** The report as the summary pane shows it. */
    QString renderReport(const MusicQuota::Report &report) const;
    /** One window's figures as an HTML block. */
    QString renderSlice(const QString &heading,
                        const MusicQuota::Slice &slice,
                        const MusicQuota::Settings &s) const;

    bool writeCsv(const QString &suggestedName, const QStringList &preamble,
                  const QStringList &header, const QList<QStringList> &rows);

    // --- marking ---
    QLineEdit    *m_search = nullptr;
    QCheckBox    *m_unmarkedOnly = nullptr;
    QTableWidget *m_tracks = nullptr;
    QLabel       *m_trackSummary = nullptr;
    /** What the list itself says, without the selection count or the result
     *  of the last write — those are composed onto it. */
    QString       m_trackSummaryBase;
    QString       m_trackStatus;

    QCheckBox *m_setOrigin = nullptr;
    QComboBox *m_origin = nullptr;
    QCheckBox *m_setLanguage = nullptr;
    QComboBox *m_language = nullptr;
    QCheckBox *m_setPerformer = nullptr;
    QComboBox *m_performer = nullptr;
    QCheckBox *m_setRelease = nullptr;
    QDateEdit *m_release = nullptr;
    QCheckBox *m_setIsMusic = nullptr;
    QComboBox *m_isMusic = nullptr;

    // --- report ---
    QDateEdit    *m_from = nullptr;
    QDateEdit    *m_to = nullptr;
    QComboBox    *m_basis = nullptr;
    QTextBrowser *m_summary = nullptr;
    QTableWidget *m_detail = nullptr;
    QPushButton  *m_export = nullptr;
    MusicQuota::Report m_report;

    // --- rules ---
    QCheckBox      *m_enabled = nullptr;
    QLineEdit      *m_country = nullptr;
    QLineEdit      *m_languageName = nullptr;
    QDoubleSpinBox *m_nationalPercent = nullptr;
    QDoubleSpinBox *m_languageShare = nullptr;
    QCheckBox      *m_requireEu = nullptr;
    QDoubleSpinBox *m_recentShare = nullptr;
    QSpinBox       *m_recentMonths = nullptr;
    QSpinBox       *m_primeStart = nullptr;
    QSpinBox       *m_primeEnd = nullptr;
    QCheckBox      *m_primeDay[7] = {};
    QCheckBox      *m_includeUnlibraried = nullptr;
    QPlainTextEdit *m_exemption = nullptr;
    QLabel         *m_rulesStatus = nullptr;

    MusicQuota::Settings m_settings;
};

#endif // QUOTADIALOG_H
