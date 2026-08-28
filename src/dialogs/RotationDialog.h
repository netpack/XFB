#ifndef ROTATIONDIALOG_H
#define ROTATIONDIALOG_H

#include <QDialog>
#include <QList>

#include "../services/RotationRules.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextBrowser;

/**
 * @brief The rotation editor, and the window that answers "why did it pick
 *        that?".
 *
 * Three tabs, because they are three different jobs done by the same person on
 * different days: the station's separation rules and category weights, the
 * per-track rules applied in bulk over a selection, and the last few Auto Mode
 * picks with the reason each one came out the way it did. The third is not a
 * nicety — a rotation you cannot inspect is a rotation you cannot fix, and it
 * is the only place the relaxation ladder becomes visible.
 *
 * Built in code, no .ui file: the committed ui_*.h headers in src/ shadow the
 * generated ones and a stale one fails silently.
 *
 * Everything is reachable from the keyboard and named for a screen reader.
 */
class RotationDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param preselected music ids to select in the track list on opening —
     *        what the operator had highlighted in the music table.
     */
    explicit RotationDialog(QWidget *parent = nullptr,
                            const QList<qint64> &preselected = QList<qint64>());

    /**
     * Reloads the track list and highlights @a ids. The window is kept alive
     * between openings, so reopening it with a different selection in the
     * music table has to move the selection here too.
     */
    void preselect(const QList<qint64> &ids);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

private slots:
    void applySettings();
    void restoreDefaultSettings();
    void reloadTracks();
    void applyToSelection();
    void clearSelection();
    void selectAllHours();
    void clearAllHours();
    void refreshDecisions();
    void showDecisionDetail();

private:
    QWidget *buildSettingsTab();
    QWidget *buildTracksTab();
    QWidget *buildWhyTab();

    void loadSettingsInto(const RotationRules::Settings &s);
    RotationRules::Settings readSettingsFromForm() const;

    /** Music ids of the rows the operator has highlighted. */
    QList<qint64> selectedMusicIds() const;
    void selectMusicIds(const QList<qint64> &ids);
    void updateSelectionSummary();

    // --- station settings ---
    QCheckBox      *m_enabled = nullptr;
    QSpinBox       *m_artistMinutes = nullptr;
    QSpinBox       *m_titleHours = nullptr;
    QDoubleSpinBox *m_powerWeight = nullptr;
    QDoubleSpinBox *m_secondaryWeight = nullptr;
    QDoubleSpinBox *m_goldWeight = nullptr;
    QComboBox      *m_defaultCategory = nullptr;
    QSpinBox       *m_candidateLimit = nullptr;
    QLabel         *m_settingsStatus = nullptr;

    // --- track editor ---
    QLineEdit    *m_search = nullptr;
    QTableWidget *m_tracks = nullptr;
    QLabel       *m_trackSummary = nullptr;

    QCheckBox    *m_setCategory = nullptr;
    QComboBox    *m_category = nullptr;
    QCheckBox    *m_setHours = nullptr;
    QCheckBox    *m_hour[24] = {};
    QPushButton  *m_allHours = nullptr;
    QPushButton  *m_noHours = nullptr;
    QCheckBox    *m_setDays = nullptr;
    QCheckBox    *m_day[7] = {};
    QCheckBox    *m_setStart = nullptr;
    QDateEdit    *m_startDate = nullptr;
    QCheckBox    *m_setEnd = nullptr;
    QDateEdit    *m_endDate = nullptr;
    QCheckBox    *m_setWeight = nullptr;
    QDoubleSpinBox *m_weight = nullptr;
    QCheckBox    *m_weightFromCategory = nullptr;

    // --- why this track ---
    QTableWidget *m_decisions = nullptr;
    QTextBrowser *m_detail = nullptr;
    QList<RotationRules::Decision> m_decisionRows;
};

#endif // ROTATIONDIALOG_H
