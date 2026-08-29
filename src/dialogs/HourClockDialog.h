#ifndef HOURCLOCKDIALOG_H
#define HOURCLOCKDIALOG_H

#include <QDialog>
#include <QList>

#include "../services/HourClock.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimeEdit;
class HourClockWheel;

/**
 * @brief The hour clock editor: the wheel, its text twin, the clock library
 *        and the week's assignments.
 *
 * ### The text twin is not a description of the wheel
 *
 * It is the same model, edited the same way. Every operation the mouse can do
 * on the wheel — add, remove, reorder, retime, resize, change type — is a
 * control in the list beside it, reachable by Tab, named for a screen reader
 * and announced when it changes something. Neither view is the master: both
 * write to `m_clock`, and refreshViews() re-resolves and repaints both. A
 * blind operator programmes the same hour the sighted one does, in the same
 * window, and neither is told to use the other's tool.
 *
 * Built in code with no .ui file: the committed ui_*.h headers in src/ shadow
 * the generated ones, and a stale one fails silently.
 */
class HourClockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HourClockDialog(QWidget *parent = nullptr);

    /** Reloads the clock list and the assignment grid from the database. */
    void reload();

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);
    /** The settings or the clocks changed; Auto Mode should look again. */
    void clocksChanged();

private slots:
    // --- library ---
    void currentClockChanged(int comboIndex);
    void newClock();
    void newExampleClock();
    void duplicateCurrentClock();
    void renameCurrentClock();
    void deleteCurrentClock();
    void saveCurrentClock();
    void revertCurrentClock();

    // --- the text twin ---
    void slotRowChanged();
    void addSlot();
    void removeSlot();
    void moveSlotUp();
    void moveSlotDown();
    void applySlotEdits();
    void slotTypeChanged();

    // --- the wheel ---
    void wheelSelectionChanged(int index);
    void wheelEditRequested(int index);
    void wheelSlotMoved(int index, int newStartSeconds);
    void wheelSlotResized(int index, int newLengthSeconds);

    // --- the week ---
    void assignRange();
    void clearRange();
    void assignmentCellActivated(int row, int column);

    // --- Auto Mode ---
    void applyAutoModeSettings();

private:
    QWidget *buildClockTab();
    QWidget *buildWeekTab();
    QWidget *buildAutoModeTab();

    void reloadClockCombo(qint64 selectId);
    void loadClockIntoEditor(qint64 id);
    void refreshViews();               ///< re-resolve, repaint both views
    void refreshSlotTable();
    void loadSlotIntoForm(int index);
    void refreshReferenceChoices(HourClock::SlotType type);
    void refreshAssignmentGrid();
    void setDirty(bool dirty);
    void announce(const QString &message);
    /** True when the operator either saved or discarded pending edits. */
    bool confirmDiscard();

    int  currentSlotIndex() const;
    void selectSlot(int index, bool announceIt);
    void moveSlot(int from, int to);

    // --- library row ---
    QComboBox   *m_clockCombo  = nullptr;
    QPushButton *m_newButton   = nullptr;
    QPushButton *m_exampleButton = nullptr;
    QPushButton *m_dupButton   = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_saveButton  = nullptr;
    QPushButton *m_revertButton = nullptr;

    // --- views ---
    HourClockWheel *m_wheel = nullptr;
    QTableWidget   *m_slotTable = nullptr;
    QLabel         *m_totalLabel = nullptr;

    // --- the slot form (the editable half of the text twin) ---
    QComboBox *m_slotType    = nullptr;
    QLineEdit *m_slotLabel   = nullptr;
    QComboBox *m_slotReference = nullptr;
    QLabel    *m_referenceLabel = nullptr;
    QTimeEdit *m_slotStart   = nullptr;
    QSpinBox  *m_slotMinutes = nullptr;
    QSpinBox  *m_slotSeconds = nullptr;
    QCheckBox *m_slotHard    = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_upButton = nullptr;
    QPushButton *m_downButton = nullptr;

    // --- the week ---
    QTableWidget *m_assignGrid = nullptr;
    QComboBox    *m_assignClock = nullptr;
    QComboBox    *m_assignDay  = nullptr;
    QSpinBox     *m_assignFrom = nullptr;
    QSpinBox     *m_assignTo   = nullptr;
    QLabel       *m_assignStatus = nullptr;

    // --- Auto Mode ---
    QCheckBox *m_enabled = nullptr;
    QCheckBox *m_fireHard = nullptr;
    QSpinBox  *m_fireWindow = nullptr;
    QLabel    *m_autoStatus = nullptr;

    HourClock::Clock    m_clock;
    HourClock::Timeline m_timeline;
    bool m_dirty = false;
    bool m_loading = false;
};

#endif // HOURCLOCKDIALOG_H
