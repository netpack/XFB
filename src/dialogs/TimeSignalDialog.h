#ifndef TIMESIGNALDIALOG_H
#define TIMESIGNALDIALOG_H

#include <QDialog>
#include <QList>

#include "../services/TimeSignal.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QToolButton;

/**
 * @brief The time signal editor: every signal the station has, and every knob
 *        on the one being edited.
 *
 * The list on the left is the station's whole set — the pips, the hour ident,
 * the half-past sweeper — and the form on the right is one of them in full.
 * Nothing is hidden behind a mode: days, hours, the second past the hour, what
 * plays, how it reaches the air, how far the music ducks under it and how late
 * is too late are all on one page, because "fine control" that needs three
 * windows is not control.
 *
 * The **Preview** box under the settings is the honest part. It lists the next
 * firings with the file each one has resolved to, so a pattern that names an
 * hour nothing has been recorded for shows up as a hole *here*, at four in the
 * afternoon with time to fix it, rather than at three in the morning as
 * silence.
 *
 * Built in code with no .ui file: the committed ui_*.h headers in src/ shadow
 * the generated ones, and a stale one fails silently.
 */
class TimeSignalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimeSignalDialog(QWidget *parent = nullptr);

    /** Reloads the signal list and the settings from the database. */
    void reload();

protected:
    /** The pickers are filled from the library, and the library changes while
     *  this window is open — a jingle added from the Database menu has to be
     *  in the list when the operator comes back to this window, not the next
     *  time XFB starts. */
    void showEvent(QShowEvent *event) override;

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);
    /** A signal or a setting changed; the player should look again. */
    void signalsChanged();
    /** "Test now": put this signal to air as if it were due this second. */
    void testRequested(const TimeSignal::Signal &signal);

private slots:
    void currentSignalChanged();
    void addSignal();
    void duplicateSignal();
    void removeSignal();
    void applyEdits();
    void revertEdits();
    void testCurrent();
    void sourceChanged();
    void placementChanged();
    void browseForFile();
    void applyHourRange();
    void allHours();
    void noHours();
    void everyDay();
    void weekdays();
    void weekend();
    void applySettings();
    void refreshPreview();

private:
    QWidget *buildSignalsTab();
    QWidget *buildSettingsTab();
    QGroupBox *buildDaysBox(QWidget *parent);
    QGroupBox *buildHoursBox(QWidget *parent);

    void refreshTable(qint64 selectId);
    void loadIntoForm(const TimeSignal::Signal &signal);
    TimeSignal::Signal formToSignal() const;
    void refreshJingleNames();
    void updatePatternPreview();
    void setFormEnabled(bool on);
    void setDirty(bool dirty);
    void announce(const QString &message);
    /** True when the operator either saved or discarded pending edits. */
    bool confirmDiscard();
    qint64 currentSignalId() const;

    // --- the list ---
    QTableWidget *m_table     = nullptr;
    QPushButton  *m_addButton = nullptr;
    QPushButton  *m_dupButton = nullptr;
    QPushButton  *m_removeButton = nullptr;
    QPushButton  *m_testButton = nullptr;

    // --- the form ---
    QLineEdit *m_label   = nullptr;
    QCheckBox *m_enabled = nullptr;
    QCheckBox *m_days[7] = { nullptr, nullptr, nullptr, nullptr,
                             nullptr, nullptr, nullptr };
    QCheckBox *m_hours[24] = {};
    QSpinBox  *m_rangeFrom = nullptr;
    QSpinBox  *m_rangeTo   = nullptr;
    QSpinBox  *m_atMinutes = nullptr;
    QSpinBox  *m_atSeconds = nullptr;

    QComboBox      *m_source      = nullptr;
    QStackedWidget *m_sourceStack = nullptr;
    QComboBox      *m_jingleName  = nullptr;
    QLineEdit      *m_filePath    = nullptr;
    /** Editable, like the jingle picker: the tokens are the part nobody
     *  remembers, so the ready-made patterns are in the list and anything
     *  else can still be typed over them. */
    QComboBox      *m_pattern     = nullptr;
    QLabel         *m_patternPreview = nullptr;

    QComboBox *m_placement = nullptr;
    QLabel    *m_placementHint = nullptr;
    QSpinBox  *m_volume    = nullptr;
    QSpinBox  *m_duck      = nullptr;
    QLabel    *m_duckLabel = nullptr;
    QSpinBox  *m_grace     = nullptr;

    QPushButton *m_applyButton  = nullptr;
    QPushButton *m_revertButton = nullptr;

    // --- settings and preview ---
    QCheckBox    *m_masterEnabled = nullptr;
    QCheckBox    *m_whenLive = nullptr;
    QSpinBox     *m_tick     = nullptr;
    QLabel       *m_settingsStatus = nullptr;
    QTableWidget *m_preview  = nullptr;
    QLabel       *m_previewStatus = nullptr;

    QList<TimeSignal::Signal> m_signals;
    qint64 m_editingId = -1;
    bool   m_dirty   = false;
    bool   m_loading = false;
};

#endif // TIMESIGNALDIALOG_H
