#include "HourClockDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "../ui/HourClockWheel.h"

namespace {

/** Every column of the text twin, in the order a person reads them. */
enum SlotColumn { ColStart = 0, ColLength, ColType, ColName, ColItem,
                  ColTiming, ColCount };

QStringList weekdayNames()
{
    QStringList out;
    QLocale locale;
    for (int day = 1; day <= 7; ++day)
        out << locale.dayName(day, QLocale::LongFormat);
    return out;
}

} // namespace

HourClockDialog::HourClockDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Hour Clocks"));
    setModal(false);
    resize(1040, 700);
    setAccessibleName(tr("Hour clocks"));

    auto *layout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);
    tabs->setAccessibleName(tr("Hour clock sections"));
    tabs->addTab(buildClockTab(), tr("&Clock"));
    tabs->addTab(buildWeekTab(), tr("&Week"));
    tabs->addTab(buildAutoModeTab(), tr("&Auto Mode"));
    layout->addWidget(tabs, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    reload();
}

// ------------------------------------------------------------- the clock tab --

QWidget *HourClockDialog::buildClockTab()
{
    auto *page = new QWidget(this);
    auto *outer = new QVBoxLayout(page);

    // --- the library row ---
    auto *libraryRow = new QHBoxLayout;
    auto *pickLabel = new QLabel(tr("C&lock:"), page);
    m_clockCombo = new QComboBox(page);
    m_clockCombo->setAccessibleName(tr("Clock being edited"));
    m_clockCombo->setMinimumWidth(220);
    pickLabel->setBuddy(m_clockCombo);

    m_newButton = new QPushButton(tr("&New..."), page);
    m_newButton->setAccessibleName(tr("Create an empty clock"));
    m_exampleButton = new QPushButton(tr("New from &Example..."), page);
    m_exampleButton->setAccessibleName(
        tr("Create a clock from the worked example: news, sweeps, ad breaks"));
    m_dupButton = new QPushButton(tr("&Duplicate..."), page);
    m_dupButton->setAccessibleName(tr("Duplicate this clock"));
    m_renameButton = new QPushButton(tr("Re&name..."), page);
    m_renameButton->setAccessibleName(tr("Rename this clock"));
    m_deleteButton = new QPushButton(tr("De&lete"), page);
    m_deleteButton->setAccessibleName(tr("Delete this clock"));

    libraryRow->addWidget(pickLabel);
    libraryRow->addWidget(m_clockCombo, 1);
    libraryRow->addWidget(m_newButton);
    libraryRow->addWidget(m_exampleButton);
    libraryRow->addWidget(m_dupButton);
    libraryRow->addWidget(m_renameButton);
    libraryRow->addWidget(m_deleteButton);
    outer->addLayout(libraryRow);

    connect(m_clockCombo, &QComboBox::currentIndexChanged,
            this, &HourClockDialog::currentClockChanged);
    connect(m_newButton, &QPushButton::clicked, this, &HourClockDialog::newClock);
    connect(m_exampleButton, &QPushButton::clicked,
            this, &HourClockDialog::newExampleClock);
    connect(m_dupButton, &QPushButton::clicked,
            this, &HourClockDialog::duplicateCurrentClock);
    connect(m_renameButton, &QPushButton::clicked,
            this, &HourClockDialog::renameCurrentClock);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &HourClockDialog::deleteCurrentClock);

    // --- the two views, side by side ---
    auto *splitter = new QSplitter(Qt::Horizontal, page);

    m_wheel = new HourClockWheel(splitter);
    connect(m_wheel, &HourClockWheel::selectionChanged,
            this, &HourClockDialog::wheelSelectionChanged);
    connect(m_wheel, &HourClockWheel::editRequested,
            this, &HourClockDialog::wheelEditRequested);
    connect(m_wheel, &HourClockWheel::slotMoved,
            this, &HourClockDialog::wheelSlotMoved);
    connect(m_wheel, &HourClockWheel::slotResized,
            this, &HourClockDialog::wheelSlotResized);
    connect(m_wheel, &HourClockWheel::announcementRequested,
            this, &HourClockDialog::announce);
    splitter->addWidget(m_wheel);

    // --- the text twin ---
    auto *twin = new QWidget(splitter);
    auto *twinLayout = new QVBoxLayout(twin);
    twinLayout->setContentsMargins(0, 0, 0, 0);

    auto *twinCaption = new QLabel(
        tr("The same hour as a list. Everything the wheel can do is here, "
           "from the keyboard."), twin);
    twinCaption->setWordWrap(true);
    twinLayout->addWidget(twinCaption);

    m_slotTable = new QTableWidget(0, ColCount, twin);
    m_slotTable->setHorizontalHeaderLabels(
        { tr("Start"), tr("Length"), tr("Type"), tr("Name"), tr("Genre or item"),
          tr("Timing") });
    m_slotTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_slotTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_slotTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_slotTable->verticalHeader()->setVisible(false);
    m_slotTable->horizontalHeader()->setStretchLastSection(true);
    m_slotTable->setAccessibleName(tr("Slots of this hour, in order"));
    m_slotTable->setAccessibleDescription(
        tr("Each row is one item of the hour. Choose a row, then change it in "
           "the fields below. Alt and Up or Down moves the selected item in "
           "the running order."));
    twinLayout->addWidget(m_slotTable, 1);
    connect(m_slotTable, &QTableWidget::itemSelectionChanged,
            this, &HourClockDialog::slotRowChanged);

    auto *listButtons = new QHBoxLayout;
    m_addButton = new QPushButton(tr("&Add slot"), twin);
    m_addButton->setAccessibleName(tr("Add a slot after the selected one"));
    m_removeButton = new QPushButton(tr("&Remove slot"), twin);
    m_removeButton->setAccessibleName(tr("Remove the selected slot"));
    m_upButton = new QPushButton(tr("Move &up"), twin);
    m_upButton->setAccessibleName(tr("Move the selected slot earlier"));
    m_upButton->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));
    m_downButton = new QPushButton(tr("Move do&wn"), twin);
    m_downButton->setAccessibleName(tr("Move the selected slot later"));
    m_downButton->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));
    listButtons->addWidget(m_addButton);
    listButtons->addWidget(m_removeButton);
    listButtons->addWidget(m_upButton);
    listButtons->addWidget(m_downButton);
    listButtons->addStretch(1);
    twinLayout->addLayout(listButtons);

    connect(m_addButton, &QPushButton::clicked, this, &HourClockDialog::addSlot);
    connect(m_removeButton, &QPushButton::clicked, this, &HourClockDialog::removeSlot);
    connect(m_upButton, &QPushButton::clicked, this, &HourClockDialog::moveSlotUp);
    connect(m_downButton, &QPushButton::clicked, this, &HourClockDialog::moveSlotDown);

    // The editable half. A form, so a screen reader reads a label for every
    // field, and every field is one Tab away from the row it is editing.
    auto *form = new QGroupBox(tr("Selected slot"), twin);
    form->setAccessibleName(tr("Fields of the selected slot"));
    auto *formLayout = new QFormLayout(form);

    m_slotType = new QComboBox(form);
    for (HourClock::SlotType type : HourClock::slotTypes())
        m_slotType->addItem(HourClock::typeLabel(type), static_cast<int>(type));
    m_slotType->setAccessibleName(tr("What this slot puts on air"));
    formLayout->addRow(tr("&Type:"), m_slotType);
    connect(m_slotType, &QComboBox::currentIndexChanged,
            this, &HourClockDialog::slotTypeChanged);

    m_slotLabel = new QLineEdit(form);
    m_slotLabel->setAccessibleName(tr("Name of this slot"));
    formLayout->addRow(tr("Na&me:"), m_slotLabel);

    m_slotReference = new QComboBox(form);
    m_slotReference->setEditable(true);
    m_slotReference->setInsertPolicy(QComboBox::NoInsert);
    m_slotReference->setAccessibleName(tr("Genre or item this slot draws from"));
    m_referenceLabel = new QLabel(tr("&Genre:"), form);
    m_referenceLabel->setBuddy(m_slotReference);
    formLayout->addRow(m_referenceLabel, m_slotReference);

    m_slotHard = new QCheckBox(tr("&Fixed time — this item must start exactly "
                                  "at its start time"), form);
    m_slotHard->setAccessibleName(
        tr("Hard-timed: the item starts at its own time whatever else happens"));
    formLayout->addRow(QString(), m_slotHard);

    m_slotStart = new QTimeEdit(form);
    m_slotStart->setDisplayFormat(QStringLiteral("mm:ss"));
    m_slotStart->setMinimumTime(QTime(0, 0, 0));
    m_slotStart->setMaximumTime(QTime(0, 59, 59));
    m_slotStart->setAccessibleName(
        tr("Start of this slot, minutes and seconds past the hour"));
    formLayout->addRow(tr("&Start past the hour:"), m_slotStart);

    auto *lengthRow = new QHBoxLayout;
    m_slotMinutes = new QSpinBox(form);
    m_slotMinutes->setRange(0, 60);
    m_slotMinutes->setSuffix(tr(" min"));
    m_slotMinutes->setAccessibleName(tr("Length of this slot in minutes"));
    m_slotSeconds = new QSpinBox(form);
    m_slotSeconds->setRange(0, 59);
    m_slotSeconds->setSuffix(tr(" s"));
    m_slotSeconds->setAccessibleName(tr("Extra seconds of length"));
    lengthRow->addWidget(m_slotMinutes);
    lengthRow->addWidget(m_slotSeconds);
    lengthRow->addStretch(1);
    // addRow() with a layout builds a label with no buddy, and a label with no
    // buddy prints its "&" instead of underlining the letter after it — the row
    // read "&Length:" on screen, in every language.
    auto *lengthLabel = new QLabel(tr("&Length:"), form);
    lengthLabel->setBuddy(m_slotMinutes);
    formLayout->addRow(lengthLabel, lengthRow);

    m_applyButton = new QPushButton(tr("A&pply to slot"), form);
    m_applyButton->setAccessibleName(tr("Apply these fields to the selected slot"));
    m_applyButton->setDefault(false);
    m_applyButton->setAutoDefault(false);
    formLayout->addRow(QString(), m_applyButton);
    connect(m_applyButton, &QPushButton::clicked,
            this, &HourClockDialog::applySlotEdits);

    twinLayout->addWidget(form);
    splitter->addWidget(twin);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    outer->addWidget(splitter, 1);

    // --- the live total ---
    m_totalLabel = new QLabel(page);
    m_totalLabel->setWordWrap(true);
    m_totalLabel->setAccessibleName(tr("How full this hour is"));
    outer->addWidget(m_totalLabel);

    auto *saveRow = new QHBoxLayout;
    m_saveButton = new QPushButton(tr("&Save clock"), page);
    m_saveButton->setAccessibleName(tr("Save this clock to the database"));
    m_saveButton->setShortcut(QKeySequence::Save);
    m_revertButton = new QPushButton(tr("Re&vert"), page);
    m_revertButton->setAccessibleName(tr("Discard the unsaved changes"));
    saveRow->addStretch(1);
    saveRow->addWidget(m_revertButton);
    saveRow->addWidget(m_saveButton);
    outer->addLayout(saveRow);
    connect(m_saveButton, &QPushButton::clicked,
            this, &HourClockDialog::saveCurrentClock);
    connect(m_revertButton, &QPushButton::clicked,
            this, &HourClockDialog::revertCurrentClock);

    // Delete removes the selected slot when the list has focus.
    auto *del = new QShortcut(QKeySequence(Qt::Key_Delete), m_slotTable);
    del->setContext(Qt::WidgetWithChildrenShortcut);
    connect(del, &QShortcut::activated, this, &HourClockDialog::removeSlot);

    return page;
}

// -------------------------------------------------------------- the week tab --

QWidget *HourClockDialog::buildWeekTab()
{
    auto *page = new QWidget(this);
    auto *outer = new QVBoxLayout(page);

    auto *caption = new QLabel(
        tr("Which clock runs in which hour. An hour with no clock keeps the "
           "plain hour grid it has always had, so a half-programmed week "
           "still goes to air."), page);
    caption->setWordWrap(true);
    outer->addWidget(caption);

    m_assignGrid = new QTableWidget(24, 7, page);
    m_assignGrid->setHorizontalHeaderLabels(weekdayNames());
    QStringList hours;
    for (int hour = 0; hour < 24; ++hour)
        hours << QStringLiteral("%1:00").arg(hour, 2, 10, QLatin1Char('0'));
    m_assignGrid->setVerticalHeaderLabels(hours);
    m_assignGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_assignGrid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_assignGrid->setAccessibleName(tr("Clock assigned to each hour of the week"));
    m_assignGrid->setAccessibleDescription(
        tr("Rows are hours, columns are weekdays. Press Return on a cell to "
           "give that hour the clock chosen below."));
    m_assignGrid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    outer->addWidget(m_assignGrid, 1);
    connect(m_assignGrid, &QTableWidget::cellActivated,
            this, &HourClockDialog::assignmentCellActivated);

    auto *controls = new QGroupBox(tr("Assign"), page);
    auto *form = new QFormLayout(controls);

    m_assignClock = new QComboBox(controls);
    m_assignClock->setAccessibleName(tr("Clock to assign"));
    form->addRow(tr("Cl&ock:"), m_assignClock);

    m_assignDay = new QComboBox(controls);
    m_assignDay->addItem(tr("Every day"), 0);
    const QStringList days = weekdayNames();
    for (int day = 1; day <= 7; ++day)
        m_assignDay->addItem(days.at(day - 1), day);
    m_assignDay->addItem(tr("Monday to Friday"), 8);
    m_assignDay->addItem(tr("Saturday and Sunday"), 9);
    m_assignDay->setAccessibleName(tr("Days to assign it to"));
    form->addRow(tr("&Days:"), m_assignDay);

    auto *hourRow = new QHBoxLayout;
    m_assignFrom = new QSpinBox(controls);
    m_assignFrom->setRange(0, 23);
    m_assignFrom->setAccessibleName(tr("First hour"));
    m_assignTo = new QSpinBox(controls);
    m_assignTo->setRange(0, 23);
    m_assignTo->setValue(23);
    m_assignTo->setAccessibleName(tr("Last hour"));
    hourRow->addWidget(m_assignFrom);
    hourRow->addWidget(new QLabel(tr("to"), controls));
    hourRow->addWidget(m_assignTo);
    hourRow->addStretch(1);
    form->addRow(tr("&Hours:"), hourRow);

    auto *buttons = new QHBoxLayout;
    auto *assign = new QPushButton(tr("A&ssign"), controls);
    assign->setAccessibleName(tr("Assign the chosen clock to those hours"));
    auto *clear = new QPushButton(tr("&Clear those hours"), controls);
    clear->setAccessibleName(tr("Remove the clock from those hours"));
    buttons->addWidget(assign);
    buttons->addWidget(clear);
    buttons->addStretch(1);
    form->addRow(QString(), buttons);
    connect(assign, &QPushButton::clicked, this, &HourClockDialog::assignRange);
    connect(clear, &QPushButton::clicked, this, &HourClockDialog::clearRange);

    m_assignStatus = new QLabel(controls);
    m_assignStatus->setWordWrap(true);
    m_assignStatus->setAccessibleName(tr("What the last assignment did"));
    form->addRow(QString(), m_assignStatus);

    outer->addWidget(controls);
    return page;
}

// ---------------------------------------------------------- the Auto Mode tab --

QWidget *HourClockDialog::buildAutoModeTab()
{
    auto *page = new QWidget(this);
    auto *outer = new QVBoxLayout(page);

    auto *caption = new QLabel(
        tr("Until this is switched on, nothing here changes what goes to air. "
           "Auto Mode keeps using the hour grid — one genre per hour per "
           "weekday — exactly as before."), page);
    caption->setWordWrap(true);
    outer->addWidget(caption);

    m_enabled = new QCheckBox(tr("Let Auto Mode &follow the hour clock"), page);
    m_enabled->setAccessibleName(
        tr("Auto Mode fills the clock's current music sweep instead of the "
           "hour's genre"));
    outer->addWidget(m_enabled);

    m_fireHard = new QCheckBox(tr("Put &fixed-time items on air at their time"), page);
    m_fireHard->setAccessibleName(
        tr("News, ad breaks and jingles marked fixed are queued when they are due"));
    outer->addWidget(m_fireHard);

    auto *form = new QFormLayout;
    m_fireWindow = new QSpinBox(page);
    m_fireWindow->setRange(10, 300);
    m_fireWindow->setSuffix(tr(" s"));
    m_fireWindow->setAccessibleName(
        tr("How close to its time a fixed item counts as due"));
    form->addRow(tr("Fi&ring window:"), m_fireWindow);
    outer->addLayout(form);

    auto *explain = new QLabel(
        tr("A clock never narrows what Auto Mode may play to nothing. The "
           "sweep's genre only shapes the first two of the picker's three "
           "passes; the last pass is the whole library, and the rotation "
           "rules relax in their usual published order underneath. A station "
           "with three tracks and a clock still plays those three tracks."),
        page);
    explain->setWordWrap(true);
    outer->addWidget(explain);

    auto *apply = new QPushButton(tr("A&pply"), page);
    apply->setAccessibleName(tr("Apply these Auto Mode settings"));
    connect(apply, &QPushButton::clicked,
            this, &HourClockDialog::applyAutoModeSettings);
    outer->addWidget(apply);

    m_autoStatus = new QLabel(page);
    m_autoStatus->setWordWrap(true);
    m_autoStatus->setAccessibleName(tr("Auto Mode setting status"));
    outer->addWidget(m_autoStatus);

    outer->addStretch(1);
    return page;
}

// ------------------------------------------------------------------ loading --

void HourClockDialog::reload()
{
    const HourClock::Settings s = HourClock::settings();
    m_enabled->setChecked(s.enabled);
    m_fireHard->setChecked(s.fireHardTimed);
    m_fireWindow->setValue(s.fireWindowSeconds);

    reloadClockCombo(m_clock.id);
    refreshAssignmentGrid();
}

void HourClockDialog::reloadClockCombo(qint64 selectId)
{
    const bool wasLoading = m_loading;
    m_loading = true;

    m_clockCombo->clear();
    m_assignClock->clear();
    m_assignClock->addItem(tr("(no clock — use the hour grid)"), QVariant(qint64(-1)));

    const QList<HourClock::ClockInfo> clocks = HourClock::clockList();
    for (const HourClock::ClockInfo &info : clocks) {
        const QString text = tr("%1 (%n slot(s))", "", info.slotCount).arg(info.name);
        m_clockCombo->addItem(text, QVariant(info.id));
        m_assignClock->addItem(info.name, QVariant(info.id));
    }

    int index = -1;
    for (int i = 0; i < m_clockCombo->count(); ++i) {
        if (m_clockCombo->itemData(i).toLongLong() == selectId) {
            index = i;
            break;
        }
    }
    if (index < 0 && m_clockCombo->count() > 0)
        index = 0;
    if (index >= 0)
        m_clockCombo->setCurrentIndex(index);

    m_loading = wasLoading;

    const bool has = m_clockCombo->count() > 0;
    m_dupButton->setEnabled(has);
    m_renameButton->setEnabled(has);
    m_deleteButton->setEnabled(has);
    m_saveButton->setEnabled(has);
    m_revertButton->setEnabled(has);

    if (has)
        loadClockIntoEditor(m_clockCombo->currentData().toLongLong());
    else {
        m_clock = HourClock::Clock();
        refreshViews();
    }
}

void HourClockDialog::loadClockIntoEditor(qint64 id)
{
    m_clock = (id > 0) ? HourClock::loadClock(id) : HourClock::Clock();
    setDirty(false);
    refreshViews();
    selectSlot(m_clock.items.isEmpty() ? -1 : 0, false);
}

void HourClockDialog::currentClockChanged(int comboIndex)
{
    if (m_loading || comboIndex < 0)
        return;
    if (m_dirty && !confirmDiscard()) {
        // Put the combo back where it was without re-entering this slot.
        m_loading = true;
        for (int i = 0; i < m_clockCombo->count(); ++i) {
            if (m_clockCombo->itemData(i).toLongLong() == m_clock.id) {
                m_clockCombo->setCurrentIndex(i);
                break;
            }
        }
        m_loading = false;
        return;
    }
    loadClockIntoEditor(m_clockCombo->itemData(comboIndex).toLongLong());
}

// ------------------------------------------------------------ both the views --

void HourClockDialog::refreshViews()
{
    HourClock::normalise(m_clock);
    m_timeline = HourClock::resolve(m_clock);
    m_wheel->setClock(m_clock);
    refreshSlotTable();

    QString total = HourClock::describeTotal(m_timeline);
    if (!m_timeline.warnings.isEmpty() && m_timeline.warnings.size() > 1) {
        total += QLatin1Char(' ')
                 + m_timeline.warnings.first();
    }
    if (m_dirty)
        total = tr("Unsaved changes. ") + total;
    m_totalLabel->setText(total);
    m_totalLabel->setAccessibleDescription(total);
}

void HourClockDialog::refreshSlotTable()
{
    const bool wasLoading = m_loading;
    m_loading = true;

    const int keepRow = m_slotTable->currentRow();
    m_slotTable->setRowCount(m_timeline.items.size());
    for (int i = 0; i < m_timeline.items.size(); ++i) {
        const HourClock::ResolvedSlot &r = m_timeline.items.at(i);
        const QString spoken = HourClock::describeSlot(r);

        auto put = [&](int column, const QString &text) {
            QTableWidgetItem *item = m_slotTable->item(i, column);
            if (!item) {
                item = new QTableWidgetItem;
                m_slotTable->setItem(i, column, item);
            }
            item->setText(text);
            // Every cell says the whole row, so arrowing across the table with
            // a screen reader never loses which slot is being read.
            item->setData(Qt::AccessibleTextRole, text);
            item->setData(Qt::AccessibleDescriptionRole, spoken);
            item->setToolTip(spoken);
        };

        put(ColStart,  HourClock::formatOffset(r.start));
        put(ColLength, HourClock::formatLength(r.length));
        put(ColType,   HourClock::typeLabel(r.slot.type));
        put(ColName,   r.slot.label);
        put(ColItem,   r.slot.reference);
        QString timing = r.slot.hardTimed ? tr("Fixed at %1")
                                                .arg(HourClock::formatOffset(
                                                    r.slot.startSeconds))
                                          : tr("Floating");
        if (r.slackSeconds > 0)
            timing += tr(", +%1 absorbed").arg(HourClock::formatLength(r.slackSeconds));
        if (r.overruns)
            timing += tr(", overruns");
        put(ColTiming, timing);
    }
    m_slotTable->resizeColumnsToContents();
    if (keepRow >= 0 && keepRow < m_slotTable->rowCount())
        m_slotTable->selectRow(keepRow);

    m_loading = wasLoading;
}

int HourClockDialog::currentSlotIndex() const
{
    const int row = m_slotTable->currentRow();
    return (row >= 0 && row < m_clock.items.size()) ? row : -1;
}

void HourClockDialog::selectSlot(int index, bool announceIt)
{
    const bool wasLoading = m_loading;
    m_loading = true;
    if (index >= 0 && index < m_slotTable->rowCount())
        m_slotTable->selectRow(index);
    else
        m_slotTable->clearSelection();
    m_wheel->setSelectedIndex(index);
    m_loading = wasLoading;

    loadSlotIntoForm(index);
    if (announceIt && index >= 0 && index < m_timeline.items.size())
        announce(HourClock::describeSlot(m_timeline.items.at(index)));
}

void HourClockDialog::loadSlotIntoForm(int index)
{
    const bool wasLoading = m_loading;
    m_loading = true;

    const bool has = (index >= 0 && index < m_clock.items.size());
    m_slotType->setEnabled(has);
    m_slotLabel->setEnabled(has);
    m_slotReference->setEnabled(has);
    m_slotStart->setEnabled(has);
    m_slotMinutes->setEnabled(has);
    m_slotSeconds->setEnabled(has);
    m_slotHard->setEnabled(has);
    m_applyButton->setEnabled(has);
    m_removeButton->setEnabled(has);
    m_upButton->setEnabled(has && index > 0);
    m_downButton->setEnabled(has && index < m_clock.items.size() - 1);

    if (!has) {
        m_slotLabel->clear();
        m_slotReference->setCurrentText(QString());
        m_loading = wasLoading;
        return;
    }

    const HourClock::Slot &s = m_clock.items.at(index);
    m_slotType->setCurrentIndex(m_slotType->findData(static_cast<int>(s.type)));
    refreshReferenceChoices(s.type);
    m_slotLabel->setText(s.label);
    m_slotReference->setCurrentText(s.reference);
    // A floating slot has no start of its own: show where it actually lands,
    // so the field is never a lie, and let the operator type one in — which is
    // what pinning it means.
    const int shown = s.hardTimed
                          ? s.startSeconds
                          : (index < m_timeline.items.size()
                                 ? m_timeline.items.at(index).start
                                 : s.startSeconds);
    m_slotStart->setTime(QTime(0, shown / 60, shown % 60));
    m_slotStart->setEnabled(true);
    m_slotMinutes->setValue(qMax(0, s.lengthSeconds) / 60);
    m_slotSeconds->setValue(qMax(0, s.lengthSeconds) % 60);
    m_slotHard->setChecked(s.hardTimed);

    m_loading = wasLoading;
}

void HourClockDialog::refreshReferenceChoices(HourClock::SlotType type)
{
    const QString keep = m_slotReference->currentText();
    m_slotReference->clear();
    m_slotReference->addItem(QString());

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
    QStringList values;
    if (db.isOpen()) {
        if (type == HourClock::SlotType::MusicSweep) {
            m_referenceLabel->setText(tr("&Genre:"));
            QSqlQuery genres(db);
            if (genres.exec(QStringLiteral("select name from genres1"
                                           " order by name collate nocase"))) {
                while (genres.next())
                    values << genres.value(0).toString();
            }
            QSqlQuery used(db);
            if (used.exec(QStringLiteral("select distinct genre1 from musics"
                                         " order by genre1 collate nocase"))) {
                while (used.next())
                    values << used.value(0).toString();
            }
        } else {
            m_referenceLabel->setText(tr("&Item:"));
            const QString table = HourClock::mediaTable(type);
            if (!table.isEmpty()) {
                QSqlQuery names(db);
                if (names.exec(QStringLiteral("select name from %1"
                                              " order by name collate nocase")
                                   .arg(table))) {
                    while (names.next())
                        values << names.value(0).toString();
                }
            }
        }
    }

    QSet<QString> seen;
    for (const QString &value : std::as_const(values)) {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty() || seen.contains(trimmed.toCaseFolded()))
            continue;
        seen.insert(trimmed.toCaseFolded());
        m_slotReference->addItem(trimmed);
    }
    m_slotReference->setCurrentText(keep);
    m_slotReference->setAccessibleName(
        type == HourClock::SlotType::MusicSweep
            ? tr("Genre this sweep draws from — empty means any genre")
            : tr("Item this slot plays — empty means any from that library"));
}

// ------------------------------------------------------ text twin editing --

void HourClockDialog::slotRowChanged()
{
    if (m_loading)
        return;
    const int index = currentSlotIndex();
    m_wheel->setSelectedIndex(index);
    loadSlotIntoForm(index);
    if (index >= 0 && index < m_timeline.items.size())
        announce(HourClock::describeSlot(m_timeline.items.at(index)));
}

void HourClockDialog::slotTypeChanged()
{
    if (m_loading)
        return;
    const auto type = static_cast<HourClock::SlotType>(m_slotType->currentData().toInt());
    refreshReferenceChoices(type);
}

void HourClockDialog::addSlot()
{
    if (m_clockCombo->count() == 0) {
        announce(tr("Create a clock first."));
        return;
    }
    HourClock::Slot s;
    s.type          = HourClock::SlotType::MusicSweep;
    s.label         = tr("Music sweep");
    s.lengthSeconds = 600;
    s.hardTimed     = false;

    const int at = currentSlotIndex();
    const int where = (at < 0) ? m_clock.items.size() : at + 1;
    m_clock.items.insert(where, s);
    setDirty(true);
    refreshViews();
    selectSlot(where, false);
    announce(tr("Slot added at position %1 of %2. It is a floating music "
                "sweep of ten minutes until you change it.")
                 .arg(where + 1).arg(m_clock.items.size()));
}

void HourClockDialog::removeSlot()
{
    const int index = currentSlotIndex();
    if (index < 0)
        return;
    const QString what = m_clock.items.at(index).label.isEmpty()
                             ? HourClock::typeLabel(m_clock.items.at(index).type)
                             : m_clock.items.at(index).label;
    m_clock.items.removeAt(index);
    setDirty(true);
    refreshViews();
    selectSlot(qMin(index, m_clock.items.size() - 1), false);
    announce(tr("%1 removed. %n slot(s) left in the hour.", "",
                m_clock.items.size()).arg(what));
}

void HourClockDialog::moveSlot(int from, int to)
{
    if (from < 0 || from >= m_clock.items.size()
        || to < 0 || to >= m_clock.items.size() || from == to)
        return;
    m_clock.items.move(from, to);
    setDirty(true);
    refreshViews();
    selectSlot(to, false);
    announce(tr("Moved to position %1 of %2. %3")
                 .arg(to + 1)
                 .arg(m_clock.items.size())
                 .arg(to < m_timeline.items.size()
                          ? HourClock::describeSlot(m_timeline.items.at(to))
                          : QString()));
}

void HourClockDialog::moveSlotUp()
{
    const int index = currentSlotIndex();
    if (index > 0)
        moveSlot(index, index - 1);
}

void HourClockDialog::moveSlotDown()
{
    const int index = currentSlotIndex();
    if (index >= 0 && index < m_clock.items.size() - 1)
        moveSlot(index, index + 1);
}

void HourClockDialog::applySlotEdits()
{
    const int index = currentSlotIndex();
    if (index < 0)
        return;
    HourClock::Slot &s = m_clock.items[index];
    s.type      = static_cast<HourClock::SlotType>(m_slotType->currentData().toInt());
    s.label     = m_slotLabel->text().trimmed();
    s.reference = m_slotReference->currentText().trimmed();
    s.hardTimed = m_slotHard->isChecked();
    const QTime start = m_slotStart->time();
    s.startSeconds  = start.minute() * 60 + start.second();
    s.lengthSeconds = qMax(1, m_slotMinutes->value() * 60 + m_slotSeconds->value());

    setDirty(true);
    refreshViews();
    selectSlot(index, false);
    if (index < m_timeline.items.size())
        announce(tr("Slot updated. %1. %2")
                     .arg(HourClock::describeSlot(m_timeline.items.at(index)),
                          HourClock::describeTotal(m_timeline)));
}

// ---------------------------------------------------------- wheel editing --

void HourClockDialog::wheelSelectionChanged(int index)
{
    if (m_loading)
        return;
    const bool wasLoading = m_loading;
    m_loading = true;
    if (index >= 0 && index < m_slotTable->rowCount())
        m_slotTable->selectRow(index);
    m_loading = wasLoading;
    loadSlotIntoForm(index);
}

void HourClockDialog::wheelEditRequested(int index)
{
    selectSlot(index, true);
    m_slotType->setFocus(Qt::OtherFocusReason);
}

void HourClockDialog::wheelSlotMoved(int index, int newStartSeconds)
{
    if (index < 0 || index >= m_clock.items.size())
        return;
    HourClock::Slot &s = m_clock.items[index];
    if (s.hardTimed) {
        // A fixed item is moved by changing the time it is fixed to. Nothing
        // else in the hour may move it, so the drag is the only thing that can.
        const int wanted = qBound(0, newStartSeconds, HourClock::kHourSeconds - 1);
        if (wanted == s.startSeconds)
            return;
        s.startSeconds = wanted;
        setDirty(true);
        refreshViews();
        loadSlotIntoForm(index);
        return;
    }

    // A floating item has no time of its own: dragging it changes where it
    // sits in the running order.
    int target = 0;
    for (int i = 0; i < m_timeline.items.size(); ++i) {
        if (i == index)
            continue;
        if (m_timeline.items.at(i).start < newStartSeconds)
            ++target;
    }
    target = qBound(0, target, m_clock.items.size() - 1);
    if (target == index)
        return;
    m_clock.items.move(index, target);
    setDirty(true);
    refreshViews();
    const bool wasLoading = m_loading;
    m_loading = true;
    m_slotTable->selectRow(target);
    m_loading = wasLoading;
    loadSlotIntoForm(target);
    m_wheel->retargetDrag(target);
}

void HourClockDialog::wheelSlotResized(int index, int newLengthSeconds)
{
    if (index < 0 || index >= m_clock.items.size())
        return;
    const int wanted = qBound(1, newLengthSeconds, HourClock::kHourSeconds);
    if (wanted == m_clock.items.at(index).lengthSeconds)
        return;
    m_clock.items[index].lengthSeconds = wanted;
    setDirty(true);
    refreshViews();
    loadSlotIntoForm(index);
}

// ----------------------------------------------------------------- library --

void HourClockDialog::newClock()
{
    if (m_dirty && !confirmDiscard())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("New clock"), tr("Name for the new clock:"),
        QLineEdit::Normal, HourClock::uniqueName(tr("New clock")), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    HourClock::Clock clock;
    clock.name = HourClock::uniqueName(name);
    if (!HourClock::saveClock(clock)) {
        QMessageBox::warning(this, tr("New clock"),
                             tr("The clock could not be created."));
        return;
    }
    reloadClockCombo(clock.id);
    emit clocksChanged();
    announce(tr("Clock %1 created and empty.").arg(clock.name));
}

void HourClockDialog::newExampleClock()
{
    if (m_dirty && !confirmDiscard())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("New clock from the example"), tr("Name for the new clock:"),
        QLineEdit::Normal, HourClock::uniqueName(tr("Weekday hour")), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    HourClock::Clock clock = HourClock::exampleClock(HourClock::uniqueName(name));
    if (!HourClock::saveClock(clock)) {
        QMessageBox::warning(this, tr("New clock"),
                             tr("The clock could not be created."));
        return;
    }
    reloadClockCombo(clock.id);
    emit clocksChanged();
    announce(tr("Clock %1 created with %n slot(s): news on the hour, sweeps, "
                "a jingle and two ad breaks.", "", clock.items.size())
                 .arg(clock.name));
}

void HourClockDialog::duplicateCurrentClock()
{
    if (m_clock.id <= 0)
        return;
    if (m_dirty && !confirmDiscard())
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("Duplicate clock"), tr("Name for the copy:"),
        QLineEdit::Normal, HourClock::uniqueName(m_clock.name), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    const qint64 id = HourClock::duplicateClock(m_clock.id, name);
    if (id <= 0) {
        QMessageBox::warning(this, tr("Duplicate clock"),
                             tr("The clock could not be duplicated."));
        return;
    }
    reloadClockCombo(id);
    emit clocksChanged();
    announce(tr("Clock duplicated as %1.").arg(m_clock.name));
}

void HourClockDialog::renameCurrentClock()
{
    if (m_clock.id <= 0)
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("Rename clock"), tr("New name:"),
        QLineEdit::Normal, m_clock.name, &ok);
    if (!ok || name.trimmed().isEmpty() || name == m_clock.name)
        return;
    if (!HourClock::renameClock(m_clock.id, HourClock::uniqueName(name))) {
        QMessageBox::warning(this, tr("Rename clock"),
                             tr("The clock could not be renamed."));
        return;
    }
    const qint64 id = m_clock.id;
    reloadClockCombo(id);
    refreshAssignmentGrid();
    emit clocksChanged();
    announce(tr("Clock renamed to %1.").arg(m_clock.name));
}

void HourClockDialog::deleteCurrentClock()
{
    if (m_clock.id <= 0)
        return;
    const QString name = m_clock.name;
    if (QMessageBox::question(
            this, tr("Delete clock"),
            tr("Delete the clock \"%1\"? Every hour it is assigned to falls "
               "back to the plain hour grid.").arg(name))
        != QMessageBox::Yes) {
        return;
    }
    if (!HourClock::deleteClock(m_clock.id)) {
        QMessageBox::warning(this, tr("Delete clock"),
                             tr("The clock could not be deleted."));
        return;
    }
    m_clock = HourClock::Clock();
    setDirty(false);
    reloadClockCombo(-1);
    refreshAssignmentGrid();
    emit clocksChanged();
    announce(tr("Clock %1 deleted.").arg(name));
}

void HourClockDialog::saveCurrentClock()
{
    if (m_clock.name.trimmed().isEmpty()) {
        announce(tr("This clock has no name and cannot be saved."));
        return;
    }
    if (!HourClock::saveClock(m_clock)) {
        QMessageBox::warning(this, tr("Save clock"),
                             tr("The clock could not be saved."));
        return;
    }
    setDirty(false);
    const qint64 id = m_clock.id;
    reloadClockCombo(id);
    emit clocksChanged();
    announce(tr("Clock %1 saved. %2")
                 .arg(m_clock.name, HourClock::describeTotal(m_timeline)));
}

void HourClockDialog::revertCurrentClock()
{
    if (m_clock.id <= 0)
        return;
    loadClockIntoEditor(m_clock.id);
    announce(tr("Unsaved changes discarded."));
}

bool HourClockDialog::confirmDiscard()
{
    const auto answer = QMessageBox::question(
        this, tr("Unsaved clock"),
        tr("\"%1\" has unsaved changes. Save them?").arg(m_clock.name),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Save)
        saveCurrentClock();
    setDirty(false);
    return true;
}

void HourClockDialog::setDirty(bool dirty)
{
    m_dirty = dirty;
    if (m_saveButton)
        m_saveButton->setEnabled(m_clock.id > 0 || !m_clock.name.isEmpty());
}

// ------------------------------------------------------------------- week --

void HourClockDialog::refreshAssignmentGrid()
{
    const QHash<int, qint64> assigned = HourClock::assignments();
    QHash<qint64, QString> names;
    for (const HourClock::ClockInfo &info : HourClock::clockList())
        names.insert(info.id, info.name);

    const QStringList days = weekdayNames();
    for (int hour = 0; hour < 24; ++hour) {
        for (int day = 1; day <= 7; ++day) {
            QTableWidgetItem *item = m_assignGrid->item(hour, day - 1);
            if (!item) {
                item = new QTableWidgetItem;
                m_assignGrid->setItem(hour, day - 1, item);
            }
            const qint64 id = assigned.value(day * 24 + hour, -1);
            const QString name = names.value(id);
            item->setText(name);
            const QString spoken =
                name.isEmpty()
                    ? tr("%1 at %2: no clock, the hour grid is used")
                          .arg(days.at(day - 1))
                          .arg(hour)
                    : tr("%1 at %2: clock %3")
                          .arg(days.at(day - 1)).arg(hour).arg(name);
            item->setData(Qt::AccessibleTextRole, spoken);
            item->setToolTip(spoken);
        }
    }
}

void HourClockDialog::assignmentCellActivated(int row, int column)
{
    const int hour = row;
    const int day  = column + 1;
    const qint64 id = m_assignClock->currentData().toLongLong();
    if (!HourClock::assignClock(day, hour, id)) {
        announce(tr("That hour could not be assigned."));
        return;
    }
    refreshAssignmentGrid();
    emit clocksChanged();
    const QString name = m_assignClock->currentText();
    announce(id > 0 ? tr("%1 at %2 now runs the clock %3.")
                          .arg(weekdayNames().at(column)).arg(hour).arg(name)
                    : tr("%1 at %2 is back on the plain hour grid.")
                          .arg(weekdayNames().at(column)).arg(hour));
}

void HourClockDialog::assignRange()
{
    const qint64 id = m_assignClock->currentData().toLongLong();
    if (id <= 0) {
        clearRange();
        return;
    }
    const int daySelector = m_assignDay->currentData().toInt();
    const int from = qMin(m_assignFrom->value(), m_assignTo->value());
    const int to   = qMax(m_assignFrom->value(), m_assignTo->value());

    int touched = 0;
    for (int day = 1; day <= 7; ++day) {
        if (daySelector >= 1 && daySelector <= 7 && day != daySelector) continue;
        if (daySelector == 8 && day > 5) continue;
        if (daySelector == 9 && day <= 5) continue;
        for (int hour = from; hour <= to; ++hour) {
            if (HourClock::assignClock(day, hour, id))
                ++touched;
        }
    }
    refreshAssignmentGrid();
    emit clocksChanged();
    const QString message = tr("%1 assigned to %n hour(s) of the week.", "", touched)
                                .arg(m_assignClock->currentText());
    m_assignStatus->setText(message);
    announce(message);
}

void HourClockDialog::clearRange()
{
    const int daySelector = m_assignDay->currentData().toInt();
    const int from = qMin(m_assignFrom->value(), m_assignTo->value());
    const int to   = qMax(m_assignFrom->value(), m_assignTo->value());

    int touched = 0;
    for (int day = 1; day <= 7; ++day) {
        if (daySelector >= 1 && daySelector <= 7 && day != daySelector) continue;
        if (daySelector == 8 && day > 5) continue;
        if (daySelector == 9 && day <= 5) continue;
        for (int hour = from; hour <= to; ++hour) {
            if (HourClock::clearAssignment(day, hour))
                ++touched;
        }
    }
    refreshAssignmentGrid();
    emit clocksChanged();
    const QString message = tr("%n hour(s) put back on the plain hour grid.",
                               "", touched);
    m_assignStatus->setText(message);
    announce(message);
}

// -------------------------------------------------------------- Auto Mode --

void HourClockDialog::applyAutoModeSettings()
{
    HourClock::Settings s = HourClock::settings();
    s.enabled           = m_enabled->isChecked();
    s.fireHardTimed     = m_fireHard->isChecked();
    s.fireWindowSeconds = m_fireWindow->value();
    HourClock::saveSettings(s);
    emit clocksChanged();

    const QString message =
        s.enabled
            ? tr("Auto Mode now follows the hour clock, and fixed items %1.")
                  .arg(s.fireHardTimed ? tr("go on air at their time")
                                       : tr("are left to the operator"))
            : tr("Auto Mode is back on the plain hour grid. Nothing about the "
                 "clocks reaches the air.");
    m_autoStatus->setText(message);
    announce(message);
}

void HourClockDialog::announce(const QString &message)
{
    if (!message.trimmed().isEmpty())
        emit announcementRequested(message);
}
