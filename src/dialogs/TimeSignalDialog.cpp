#include "TimeSignalDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

#include "../audioformats.h"

namespace {

enum SignalColumn { ColOn = 0, ColName, ColWhen, ColPlays, ColCount };
enum PreviewColumn { PrevWhen = 0, PrevName, PrevPlays, PrevCount };

/** The stacked pages, in the order the source combo lists them. */
enum SourcePage { PageJingle = 0, PageFile, PagePattern };

} // namespace

TimeSignalDialog::TimeSignalDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Time Signals"));
    setModal(false);
    resize(1020, 700);
    setAccessibleName(tr("Time signals"));

    auto *layout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);
    tabs->setAccessibleName(tr("Time signal sections"));
    tabs->addTab(buildSignalsTab(), tr("&Signals"));
    tabs->addTab(buildSettingsTab(), tr("Settings and &preview"));
    layout->addWidget(tabs, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    reload();
}

// --------------------------------------------------------- the signals tab --

QWidget *TimeSignalDialog::buildSignalsTab()
{
    auto *page = new QWidget(this);
    auto *outer = new QVBoxLayout(page);

    auto *splitter = new QSplitter(Qt::Horizontal, page);

    // --- the list ---
    auto *listSide = new QWidget(splitter);
    auto *listLayout = new QVBoxLayout(listSide);
    listLayout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(0, ColCount, listSide);
    m_table->setHorizontalHeaderLabels({ tr("On"), tr("Name"), tr("When"), tr("Plays") });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColWhen, QHeaderView::Stretch);
    m_table->setAccessibleName(tr("The station's time signals"));
    m_table->setAccessibleDescription(
        tr("One row per signal. Choose one to edit it in the form beside this list."));
    listLayout->addWidget(m_table, 1);

    auto *listButtons = new QHBoxLayout;
    m_addButton = new QPushButton(tr("&Add"), listSide);
    m_addButton->setAccessibleName(tr("Add a time signal"));
    m_dupButton = new QPushButton(tr("D&uplicate"), listSide);
    m_dupButton->setAccessibleName(tr("Duplicate the selected time signal"));
    m_removeButton = new QPushButton(tr("&Remove"), listSide);
    m_removeButton->setAccessibleName(tr("Remove the selected time signal"));
    m_testButton = new QPushButton(tr("&Test now"), listSide);
    m_testButton->setAccessibleName(tr("Play this signal now, as if it were due"));
    m_testButton->setToolTip(tr("Puts the signal to air the way it would go to "
                                "air on the hour, using the audio for the hour "
                                "it is now."));
    listButtons->addWidget(m_addButton);
    listButtons->addWidget(m_dupButton);
    listButtons->addWidget(m_removeButton);
    listButtons->addStretch();
    listButtons->addWidget(m_testButton);
    listLayout->addLayout(listButtons);

    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &TimeSignalDialog::currentSignalChanged);
    connect(m_addButton, &QPushButton::clicked, this, &TimeSignalDialog::addSignal);
    connect(m_dupButton, &QPushButton::clicked, this, &TimeSignalDialog::duplicateSignal);
    connect(m_removeButton, &QPushButton::clicked, this, &TimeSignalDialog::removeSignal);
    connect(m_testButton, &QPushButton::clicked, this, &TimeSignalDialog::testCurrent);

    splitter->addWidget(listSide);

    // --- the form ---
    auto *formSide = new QWidget(splitter);
    auto *formLayout = new QVBoxLayout(formSide);
    formLayout->setContentsMargins(0, 0, 0, 0);

    auto *whatBox = new QGroupBox(tr("The signal"), formSide);
    auto *whatForm = new QFormLayout(whatBox);

    m_label = new QLineEdit(whatBox);
    m_label->setAccessibleName(tr("What you call this signal"));
    whatForm->addRow(tr("&Name:"), m_label);

    m_enabled = new QCheckBox(tr("This signal is switched on"), whatBox);
    m_enabled->setAccessibleName(tr("Whether this signal fires at all"));
    whatForm->addRow(QString(), m_enabled);

    auto *atRow = new QHBoxLayout;
    m_atMinutes = new QSpinBox(whatBox);
    m_atMinutes->setRange(0, 59);
    m_atMinutes->setSuffix(tr(" min"));
    m_atMinutes->setAccessibleName(tr("Minutes past the hour"));
    m_atSeconds = new QSpinBox(whatBox);
    m_atSeconds->setRange(0, 59);
    m_atSeconds->setSuffix(tr(" s"));
    m_atSeconds->setAccessibleName(tr("Seconds past the hour"));
    atRow->addWidget(m_atMinutes);
    atRow->addWidget(m_atSeconds);
    atRow->addStretch();
    whatForm->addRow(tr("A&t:"), atRow);

    formLayout->addWidget(whatBox);
    formLayout->addWidget(buildDaysBox(formSide));
    formLayout->addWidget(buildHoursBox(formSide));

    // --- what plays ---
    auto *audioBox = new QGroupBox(tr("What plays"), formSide);
    auto *audioForm = new QFormLayout(audioBox);

    m_source = new QComboBox(audioBox);
    m_source->addItem(TimeSignal::sourceLabel(TimeSignal::Source::Jingle));
    m_source->addItem(TimeSignal::sourceLabel(TimeSignal::Source::File));
    m_source->addItem(TimeSignal::sourceLabel(TimeSignal::Source::HourPattern));
    m_source->setAccessibleName(tr("Where the audio comes from"));
    audioForm->addRow(tr("&Audio:"), m_source);

    m_sourceStack = new QStackedWidget(audioBox);

    auto *jinglePage = new QWidget(m_sourceStack);
    auto *jingleLayout = new QHBoxLayout(jinglePage);
    jingleLayout->setContentsMargins(0, 0, 0, 0);
    m_jingleName = new QComboBox(jinglePage);
    m_jingleName->setEditable(true);
    // Pick from the list or type the name — but typing one must not add a
    // phantom entry to the list of what the station actually has.
    m_jingleName->setInsertPolicy(QComboBox::NoInsert);
    m_jingleName->setAccessibleName(tr("The jingle to play, by name"));
    m_jingleName->setToolTip(tr("Every jingle in the library is in this list. "
                                "You can type a name instead — a name that "
                                "matches nothing plays nothing, and the "
                                "preview says so."));
    jingleLayout->addWidget(m_jingleName, 1);
    m_sourceStack->addWidget(jinglePage);

    auto *filePage = new QWidget(m_sourceStack);
    auto *fileLayout = new QHBoxLayout(filePage);
    fileLayout->setContentsMargins(0, 0, 0, 0);
    m_filePath = new QLineEdit(filePage);
    m_filePath->setAccessibleName(tr("The file to play"));
    auto *browse = new QPushButton(tr("&Browse..."), filePage);
    browse->setAccessibleName(tr("Choose the file to play"));
    fileLayout->addWidget(m_filePath, 1);
    fileLayout->addWidget(browse);
    m_sourceStack->addWidget(filePage);
    connect(browse, &QPushButton::clicked, this, &TimeSignalDialog::browseForFile);

    auto *patternPage = new QWidget(m_sourceStack);
    auto *patternLayout = new QVBoxLayout(patternPage);
    patternLayout->setContentsMargins(0, 0, 0, 0);
    m_pattern = new QComboBox(patternPage);
    m_pattern->setEditable(true);
    m_pattern->setInsertPolicy(QComboBox::NoInsert);
    // The tokens are the part nobody remembers, so the shapes a station
    // actually names its idents with are in the list. Anything else can be
    // typed straight over them, exactly like the jingle picker above.
    m_pattern->addItems({ QStringLiteral("Hour %H"),
                          QStringLiteral("Hour %h"),
                          QStringLiteral("It is %I %p"),
                          QStringLiteral("Time %H00"),
                          QStringLiteral("ID %H") });
    m_pattern->setCurrentText(QString());
    m_pattern->lineEdit()->setPlaceholderText(QStringLiteral("Hour %H"));
    m_pattern->setAccessibleName(tr("The jingle name, with the hour in it"));
    m_pattern->setToolTip(tr("%H two-digit hour, %h hour, %I on a twelve-hour "
                             "clock, %p am or pm. \"Hour %H\" looks for the "
                             "jingle called Hour 09 at nine o'clock."));
    m_patternPreview = new QLabel(patternPage);
    m_patternPreview->setWordWrap(true);
    m_patternPreview->setAccessibleName(tr("What this pattern finds"));
    patternLayout->addWidget(m_pattern);
    patternLayout->addWidget(m_patternPreview);
    m_sourceStack->addWidget(patternPage);

    audioForm->addRow(QString(), m_sourceStack);
    connect(m_source, &QComboBox::currentIndexChanged,
            this, &TimeSignalDialog::sourceChanged);
    connect(m_pattern, &QComboBox::currentTextChanged,
            this, &TimeSignalDialog::updatePatternPreview);

    formLayout->addWidget(audioBox);

    // --- how it gets to air ---
    auto *airBox = new QGroupBox(tr("How it gets to air"), formSide);
    auto *airForm = new QFormLayout(airBox);

    m_placement = new QComboBox(airBox);
    m_placement->addItem(TimeSignal::placementLabel(TimeSignal::Placement::Next));
    m_placement->addItem(TimeSignal::placementLabel(TimeSignal::Placement::Interrupt));
    m_placement->addItem(TimeSignal::placementLabel(TimeSignal::Placement::OverTheTop));
    m_placement->setAccessibleName(tr("How the signal reaches the air"));
    airForm->addRow(tr("&Placement:"), m_placement);
    connect(m_placement, &QComboBox::currentIndexChanged,
            this, &TimeSignalDialog::placementChanged);

    m_placementHint = new QLabel(airBox);
    m_placementHint->setWordWrap(true);
    m_placementHint->setAccessibleName(tr("What this placement does"));
    airForm->addRow(QString(), m_placementHint);

    m_volume = new QSpinBox(airBox);
    m_volume->setRange(0, 100);
    m_volume->setSuffix(QStringLiteral(" %"));
    m_volume->setAccessibleName(tr("The level of the signal itself"));
    airForm->addRow(tr("&Level:"), m_volume);

    m_duck = new QSpinBox(airBox);
    m_duck->setRange(0, 100);
    m_duck->setSuffix(QStringLiteral(" %"));
    m_duck->setAccessibleName(tr("Where the music sits while the signal plays"));
    m_duckLabel = new QLabel(tr("&Music ducks to:"), airBox);
    m_duckLabel->setBuddy(m_duck);
    airForm->addRow(m_duckLabel, m_duck);

    m_grace = new QSpinBox(airBox);
    m_grace->setRange(1, 600);
    m_grace->setSuffix(tr(" s"));
    m_grace->setAccessibleName(tr("How late still counts as on time"));
    m_grace->setToolTip(tr("A signal that has not gone to air within this many "
                           "seconds of its time is skipped for that hour, "
                           "rather than played at the wrong minute."));
    airForm->addRow(tr("&Still counts for:"), m_grace);

    formLayout->addWidget(airBox);

    auto *applyRow = new QHBoxLayout;
    m_applyButton = new QPushButton(tr("&Save signal"), formSide);
    m_applyButton->setAccessibleName(tr("Save the changes to this signal"));
    m_revertButton = new QPushButton(tr("Re&vert"), formSide);
    m_revertButton->setAccessibleName(tr("Throw away the unsaved changes"));
    applyRow->addStretch();
    applyRow->addWidget(m_revertButton);
    applyRow->addWidget(m_applyButton);
    formLayout->addLayout(applyRow);
    formLayout->addStretch();

    connect(m_applyButton, &QPushButton::clicked, this, &TimeSignalDialog::applyEdits);
    connect(m_revertButton, &QPushButton::clicked, this, &TimeSignalDialog::revertEdits);

    // Anything the operator touches marks the form dirty, so Save is never a
    // guess about whether there is something to save.
    const auto touched = [this]() { if (!m_loading) setDirty(true); };
    connect(m_label, &QLineEdit::textChanged, this, touched);
    connect(m_enabled, &QCheckBox::toggled, this, touched);
    connect(m_atMinutes, &QSpinBox::valueChanged, this, touched);
    connect(m_atSeconds, &QSpinBox::valueChanged, this, touched);
    connect(m_jingleName, &QComboBox::currentTextChanged, this, touched);
    connect(m_filePath, &QLineEdit::textChanged, this, touched);
    connect(m_pattern, &QComboBox::currentTextChanged, this, touched);
    connect(m_source, &QComboBox::currentIndexChanged, this, touched);
    connect(m_placement, &QComboBox::currentIndexChanged, this, touched);
    connect(m_volume, &QSpinBox::valueChanged, this, touched);
    connect(m_duck, &QSpinBox::valueChanged, this, touched);
    connect(m_grace, &QSpinBox::valueChanged, this, touched);
    for (QCheckBox *box : m_days)
        connect(box, &QCheckBox::toggled, this, touched);
    for (QCheckBox *box : m_hours)
        connect(box, &QCheckBox::toggled, this, touched);

    splitter->addWidget(formSide);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    outer->addWidget(splitter, 1);

    return page;
}

QGroupBox *TimeSignalDialog::buildDaysBox(QWidget *parent)
{
    auto *box = new QGroupBox(tr("On these days"), parent);
    auto *layout = new QVBoxLayout(box);

    auto *row = new QHBoxLayout;
    const QLocale locale;
    for (int day = 1; day <= 7; ++day) {
        auto *check = new QCheckBox(locale.dayName(day, QLocale::ShortFormat), box);
        check->setAccessibleName(tr("Fires on %1")
                                     .arg(locale.dayName(day, QLocale::LongFormat)));
        m_days[day - 1] = check;
        row->addWidget(check);
    }
    row->addStretch();
    layout->addLayout(row);

    auto *quick = new QHBoxLayout;
    auto *all = new QPushButton(tr("Every da&y"), box);
    auto *week = new QPushButton(tr("Monday to &Friday"), box);
    auto *wknd = new QPushButton(tr("&Weekend"), box);
    quick->addWidget(all);
    quick->addWidget(week);
    quick->addWidget(wknd);
    quick->addStretch();
    layout->addLayout(quick);

    connect(all, &QPushButton::clicked, this, &TimeSignalDialog::everyDay);
    connect(week, &QPushButton::clicked, this, &TimeSignalDialog::weekdays);
    connect(wknd, &QPushButton::clicked, this, &TimeSignalDialog::weekend);

    return box;
}

QGroupBox *TimeSignalDialog::buildHoursBox(QWidget *parent)
{
    auto *box = new QGroupBox(tr("In these hours"), parent);
    auto *layout = new QVBoxLayout(box);

    // Two rows of twelve: midnight to eleven, noon to eleven at night. A
    // twenty-four column strip is unreadable on a small screen and XFB is
    // expected to run on one.
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(4);
    for (int hour = 0; hour <= 23; ++hour) {
        auto *check = new QCheckBox(QStringLiteral("%1").arg(hour, 2, 10, QLatin1Char('0')), box);
        check->setAccessibleName(tr("Fires in the hour beginning %1:00")
                                     .arg(hour, 2, 10, QLatin1Char('0')));
        m_hours[hour] = check;
        grid->addWidget(check, hour / 12, hour % 12);
    }
    layout->addLayout(grid);

    auto *quick = new QHBoxLayout;
    auto *all = new QPushButton(tr("A&ll hours"), box);
    auto *none = new QPushButton(tr("N&one"), box);
    m_rangeFrom = new QSpinBox(box);
    m_rangeFrom->setRange(0, 23);
    m_rangeFrom->setAccessibleName(tr("First hour of the range"));
    m_rangeTo = new QSpinBox(box);
    m_rangeTo->setRange(0, 23);
    m_rangeTo->setValue(23);
    m_rangeTo->setAccessibleName(tr("Last hour of the range"));
    auto *apply = new QPushButton(tr("Set that &range"), box);
    apply->setAccessibleName(tr("Switch on only the hours in this range"));

    quick->addWidget(all);
    quick->addWidget(none);
    quick->addSpacing(12);
    quick->addWidget(new QLabel(tr("From"), box));
    quick->addWidget(m_rangeFrom);
    quick->addWidget(new QLabel(tr("to"), box));
    quick->addWidget(m_rangeTo);
    quick->addWidget(apply);
    quick->addStretch();
    layout->addLayout(quick);

    connect(all, &QPushButton::clicked, this, &TimeSignalDialog::allHours);
    connect(none, &QPushButton::clicked, this, &TimeSignalDialog::noHours);
    connect(apply, &QPushButton::clicked, this, &TimeSignalDialog::applyHourRange);

    return box;
}

// -------------------------------------------------------- the settings tab --

QWidget *TimeSignalDialog::buildSettingsTab()
{
    auto *page = new QWidget(this);
    auto *outer = new QVBoxLayout(page);

    auto *box = new QGroupBox(tr("Time signals, station-wide"), page);
    auto *form = new QFormLayout(box);

    m_masterEnabled = new QCheckBox(tr("Put time signals to air"), box);
    m_masterEnabled->setAccessibleName(tr("Whether any time signal fires at all"));
    m_masterEnabled->setToolTip(tr("With this off, nothing in this window "
                                   "reaches the air and XFB behaves exactly as "
                                   "it did before the feature existed."));
    form->addRow(QString(), m_masterEnabled);

    m_whenLive = new QCheckBox(tr("Also when an operator is driving"), box);
    m_whenLive->setAccessibleName(
        tr("Fire time signals even when Auto Mode is off"));
    m_whenLive->setToolTip(tr("The hour clock only fires under Auto Mode. A "
                              "station's pips are usually the station's "
                              "whoever is at the desk, so this starts on."));
    form->addRow(QString(), m_whenLive);

    m_tick = new QSpinBox(box);
    m_tick->setRange(500, 10000);
    m_tick->setSingleStep(500);
    m_tick->setSuffix(tr(" ms"));
    m_tick->setAccessibleName(tr("How often the clock is checked"));
    m_tick->setToolTip(tr("How closely a signal can hit its second. One second "
                          "is right for the pips; a slower tick costs nothing "
                          "but accuracy."));
    form->addRow(tr("&Check the clock every:"), m_tick);

    m_settingsStatus = new QLabel(box);
    m_settingsStatus->setWordWrap(true);
    form->addRow(QString(), m_settingsStatus);

    auto *applyRow = new QHBoxLayout;
    auto *apply = new QPushButton(tr("A&pply"), box);
    apply->setAccessibleName(tr("Save these settings"));
    applyRow->addStretch();
    applyRow->addWidget(apply);
    form->addRow(QString(), applyRow);
    connect(apply, &QPushButton::clicked, this, &TimeSignalDialog::applySettings);

    outer->addWidget(box);

    // --- the preview ---
    auto *previewBox = new QGroupBox(tr("What happens next"), page);
    auto *previewLayout = new QVBoxLayout(previewBox);

    m_preview = new QTableWidget(0, PrevCount, previewBox);
    m_preview->setHorizontalHeaderLabels({ tr("At"), tr("Signal"), tr("Plays") });
    m_preview->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_preview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_preview->verticalHeader()->setVisible(false);
    m_preview->horizontalHeader()->setSectionResizeMode(PrevPlays, QHeaderView::Stretch);
    m_preview->setAccessibleName(tr("The next time signals due"));
    m_preview->setAccessibleDescription(
        tr("Each row is a firing that is coming, and the audio it has found. "
           "A row that says nothing was found is a hole you can still fix."));
    previewLayout->addWidget(m_preview, 1);

    m_previewStatus = new QLabel(previewBox);
    m_previewStatus->setWordWrap(true);
    previewLayout->addWidget(m_previewStatus);

    auto *refresh = new QPushButton(tr("&Refresh"), previewBox);
    refresh->setAccessibleName(tr("Look at the coming signals again"));
    auto *refreshRow = new QHBoxLayout;
    refreshRow->addStretch();
    refreshRow->addWidget(refresh);
    previewLayout->addLayout(refreshRow);
    connect(refresh, &QPushButton::clicked, this, &TimeSignalDialog::refreshPreview);

    outer->addWidget(previewBox, 1);

    return page;
}

// ------------------------------------------------------------------ loading --

void TimeSignalDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // A jingle added from the Database menu while this window sat open behind
    // it has to be in the list when the operator comes back to it. Cheap: one
    // query over a table with a few dozen rows.
    refreshJingleNames();
    updatePatternPreview();
    refreshPreview();
}

void TimeSignalDialog::reload()
{
    const qint64 keep = m_editingId;
    refreshJingleNames();

    const TimeSignal::Settings settings = TimeSignal::settings();
    m_loading = true;
    m_masterEnabled->setChecked(settings.enabled);
    m_whenLive->setChecked(settings.whenLive);
    m_tick->setValue(settings.tickMs);
    m_loading = false;
    m_settingsStatus->setText(settings.enabled
        ? tr("Time signals are on.")
        : tr("Time signals are off; nothing in this window reaches the air."));

    refreshTable(keep);
    refreshPreview();
}

void TimeSignalDialog::refreshJingleNames()
{
    const QString current = m_jingleName->currentText();
    m_jingleName->clear();
    m_jingleName->addItems(TimeSignal::jingleNames());
    m_jingleName->setCurrentText(current);
}

void TimeSignalDialog::refreshTable(qint64 selectId)
{
    m_signals = TimeSignal::allSignals();

    m_loading = true;
    m_table->setRowCount(m_signals.size());
    int selectRow = -1;
    for (int row = 0; row < m_signals.size(); ++row) {
        const TimeSignal::Signal &signal = m_signals.at(row);

        auto *on = new QTableWidgetItem(signal.enabled ? tr("yes") : tr("no"));
        on->setData(Qt::UserRole, signal.id);
        m_table->setItem(row, ColOn, on);

        m_table->setItem(row, ColName,
                         new QTableWidgetItem(signal.label.isEmpty()
                                                  ? tr("(unnamed)") : signal.label));
        m_table->setItem(row, ColWhen, new QTableWidgetItem(TimeSignal::describe(signal)));

        QString plays;
        switch (signal.source) {
        case TimeSignal::Source::Jingle:
            plays = signal.reference.isEmpty() ? tr("(nothing chosen)") : signal.reference;
            break;
        case TimeSignal::Source::File:
            plays = QFileInfo(signal.reference).fileName();
            break;
        case TimeSignal::Source::HourPattern:
            plays = tr("pattern: %1").arg(signal.reference);
            break;
        }
        m_table->setItem(row, ColPlays, new QTableWidgetItem(plays));

        if (signal.id == selectId)
            selectRow = row;
    }
    m_loading = false;

    if (selectRow < 0 && !m_signals.isEmpty())
        selectRow = 0;
    if (selectRow >= 0)
        m_table->selectRow(selectRow);
    else {
        m_editingId = -1;
        setFormEnabled(false);
    }
    setDirty(false);
}

void TimeSignalDialog::loadIntoForm(const TimeSignal::Signal &signal)
{
    m_loading = true;

    m_label->setText(signal.label);
    m_enabled->setChecked(signal.enabled);
    m_atMinutes->setValue(signal.offsetSeconds / 60);
    m_atSeconds->setValue(signal.offsetSeconds % 60);

    for (int day = 1; day <= 7; ++day)
        m_days[day - 1]->setChecked(signal.firesOnDay(day));
    for (int hour = 0; hour <= 23; ++hour)
        m_hours[hour]->setChecked(signal.firesInHour(hour));

    switch (signal.source) {
    case TimeSignal::Source::Jingle:
        m_source->setCurrentIndex(PageJingle);
        m_jingleName->setCurrentText(signal.reference);
        break;
    case TimeSignal::Source::File:
        m_source->setCurrentIndex(PageFile);
        m_filePath->setText(signal.reference);
        break;
    case TimeSignal::Source::HourPattern:
        m_source->setCurrentIndex(PagePattern);
        m_pattern->setCurrentText(signal.reference);
        break;
    }
    m_sourceStack->setCurrentIndex(m_source->currentIndex());

    m_placement->setCurrentIndex(int(signal.placement));
    m_volume->setValue(signal.volumePercent);
    m_duck->setValue(signal.duckPercent);
    m_grace->setValue(signal.graceSeconds);

    m_loading = false;

    setFormEnabled(true);
    placementChanged();
    updatePatternPreview();
    setDirty(false);
}

TimeSignal::Signal TimeSignalDialog::formToSignal() const
{
    TimeSignal::Signal signal;
    signal.id      = m_editingId;
    signal.label   = m_label->text().trimmed();
    signal.enabled = m_enabled->isChecked();

    signal.dayMask = 0;
    for (int day = 1; day <= 7; ++day)
        if (m_days[day - 1]->isChecked())
            signal.dayMask |= quint8(1) << (day - 1);

    signal.hourMask = 0;
    for (int hour = 0; hour <= 23; ++hour)
        if (m_hours[hour]->isChecked())
            signal.hourMask |= quint32(1) << hour;

    signal.offsetSeconds = m_atMinutes->value() * 60 + m_atSeconds->value();

    switch (m_source->currentIndex()) {
    case PageFile:
        signal.source    = TimeSignal::Source::File;
        signal.reference = m_filePath->text().trimmed();
        break;
    case PagePattern:
        signal.source    = TimeSignal::Source::HourPattern;
        signal.reference = m_pattern->currentText().trimmed();
        break;
    default:
        signal.source    = TimeSignal::Source::Jingle;
        signal.reference = m_jingleName->currentText().trimmed();
        break;
    }

    signal.placement     = TimeSignal::Placement(m_placement->currentIndex());
    signal.volumePercent = m_volume->value();
    signal.duckPercent   = m_duck->value();
    signal.graceSeconds  = m_grace->value();
    return signal;
}

// ----------------------------------------------------------------- editing --

qint64 TimeSignalDialog::currentSignalId() const
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_signals.size())
        return -1;
    const QTableWidgetItem *item = m_table->item(row, ColOn);
    return item ? item->data(Qt::UserRole).toLongLong() : -1;
}

void TimeSignalDialog::currentSignalChanged()
{
    if (m_loading)
        return;
    const qint64 id = currentSignalId();
    if (id == m_editingId)
        return;
    if (!confirmDiscard()) {
        // Put the selection back where it was rather than silently losing the
        // edits the operator said they wanted to keep.
        m_loading = true;
        for (int row = 0; row < m_table->rowCount(); ++row) {
            const QTableWidgetItem *item = m_table->item(row, ColOn);
            if (item && item->data(Qt::UserRole).toLongLong() == m_editingId) {
                m_table->selectRow(row);
                break;
            }
        }
        m_loading = false;
        return;
    }

    m_editingId = id;
    if (id <= 0) {
        setFormEnabled(false);
        return;
    }
    for (const TimeSignal::Signal &signal : std::as_const(m_signals)) {
        if (signal.id == id) {
            loadIntoForm(signal);
            announce(tr("Editing %1. %2")
                         .arg(signal.label.isEmpty() ? tr("an unnamed signal") : signal.label,
                              TimeSignal::describe(signal)));
            return;
        }
    }
}

void TimeSignalDialog::addSignal()
{
    if (!confirmDiscard())
        return;

    TimeSignal::Signal signal = TimeSignal::defaultSignal();
    if (!TimeSignal::save(signal)) {
        QMessageBox::warning(this, tr("Time signals"),
                             tr("The signal could not be saved."));
        return;
    }
    m_editingId = signal.id;
    refreshTable(signal.id);
    // Explicitly, rather than leaving it to the selection change: selecting
    // the row that is already being edited is not a change, so the form would
    // stay as refreshTable() left it — and for the station's *first* signal
    // that is disabled, which greys out the whole window at the one moment
    // the operator has something to type into it.
    loadIntoForm(signal);
    emit signalsChanged();
    refreshPreview();
    announce(tr("A new time signal was added. It has no audio yet."));
    m_label->setFocus();
}

void TimeSignalDialog::duplicateSignal()
{
    if (!confirmDiscard())
        return;

    const qint64 id = currentSignalId();
    if (id <= 0)
        return;
    TimeSignal::Signal signal = TimeSignal::signalById(id);
    signal.id = -1;
    signal.label = signal.label.isEmpty() ? tr("Copy") : tr("%1 (copy)").arg(signal.label);
    if (!TimeSignal::save(signal))
        return;
    m_editingId = signal.id;
    refreshTable(signal.id);
    loadIntoForm(signal);   // see addSignal()
    emit signalsChanged();
    refreshPreview();
    announce(tr("Duplicated as %1.").arg(signal.label));
}

void TimeSignalDialog::removeSignal()
{
    const qint64 id = currentSignalId();
    if (id <= 0)
        return;

    const TimeSignal::Signal signal = TimeSignal::signalById(id);
    const QString name = signal.label.isEmpty() ? tr("this signal") : signal.label;
    if (QMessageBox::question(this, tr("Time signals"),
                              tr("Remove %1? The audio file itself is left alone.")
                                  .arg(name)) != QMessageBox::Yes)
        return;

    if (!TimeSignal::remove(id))
        return;
    m_dirty = false;
    m_editingId = -1;
    refreshTable(-1);
    emit signalsChanged();
    refreshPreview();
    announce(tr("%1 was removed.").arg(name));
}

void TimeSignalDialog::applyEdits()
{
    if (m_editingId <= 0)
        return;

    TimeSignal::Signal signal = formToSignal();

    // Two things are worth refusing to save quietly, because both are silence
    // the operator would only find out about on air.
    if (signal.dayMask == 0 || signal.hourMask == 0) {
        QMessageBox::warning(this, tr("Time signals"),
                             tr("This signal has no days or no hours set, so it "
                                "would never fire."));
        return;
    }
    if (signal.reference.isEmpty()) {
        QMessageBox::warning(this, tr("Time signals"),
                             tr("This signal has no audio chosen."));
        return;
    }

    if (!TimeSignal::save(signal)) {
        QMessageBox::warning(this, tr("Time signals"),
                             tr("The signal could not be saved."));
        return;
    }

    refreshTable(signal.id);
    emit signalsChanged();
    refreshPreview();
    announce(tr("Saved. %1").arg(TimeSignal::describe(signal)));
}

void TimeSignalDialog::revertEdits()
{
    if (m_editingId <= 0)
        return;
    loadIntoForm(TimeSignal::signalById(m_editingId));
    announce(tr("The unsaved changes were thrown away."));
}

void TimeSignalDialog::testCurrent()
{
    const qint64 id = currentSignalId();
    if (id <= 0)
        return;

    // The signal as it stands in the form, not as it stands in the database:
    // "test now" that tested something else would be worthless.
    TimeSignal::Signal signal = formToSignal();
    if (signal.reference.isEmpty()) {
        QMessageBox::information(this, tr("Time signals"),
                                 tr("There is no audio to test yet."));
        return;
    }

    const int hour = QTime::currentTime().hour();
    if (TimeSignal::mediaPathFor(signal, hour).isEmpty()) {
        QMessageBox::information(
            this, tr("Time signals"),
            tr("Nothing plays for the hour it is now: this signal is looking "
               "for \"%1\".").arg(TimeSignal::referenceFor(signal, hour)));
        return;
    }
    emit testRequested(signal);
    announce(tr("Testing the signal now."));
}

// ---------------------------------------------------------------- the knobs --

void TimeSignalDialog::sourceChanged()
{
    m_sourceStack->setCurrentIndex(m_source->currentIndex());
    updatePatternPreview();
}

void TimeSignalDialog::placementChanged()
{
    const auto placement = TimeSignal::Placement(m_placement->currentIndex());

    // Ducking is only meaningful when there is something still playing under
    // the signal, so the control says so by going away rather than lying.
    const bool over = placement == TimeSignal::Placement::OverTheTop;
    m_duck->setEnabled(over);
    m_duckLabel->setEnabled(over);

    switch (placement) {
    case TimeSignal::Placement::Next:
        m_placementHint->setText(
            tr("Goes to the top of the running order. Never interrupts, so it "
               "plays when the current track ends — up to a whole track late."));
        break;
    case TimeSignal::Placement::Interrupt:
        m_placementHint->setText(
            tr("Stops the deck where it is and plays the signal, then the "
               "running order carries on with the next item. On time, and "
               "audibly so."));
        break;
    case TimeSignal::Placement::OverTheTop:
        m_placementHint->setText(
            tr("Plays on its own output while the music keeps going "
               "underneath, ducked to the level below. On time, and nothing "
               "is lost."));
        break;
    }
}

void TimeSignalDialog::browseForFile()
{
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Choose the time signal"), m_filePath->text(),
        AudioFormats::fileDialogFilterString());
    if (!file.isEmpty())
        m_filePath->setText(file);
}

void TimeSignalDialog::updatePatternPreview()
{
    if (m_source->currentIndex() != PagePattern) {
        m_patternPreview->clear();
        return;
    }

    TimeSignal::Signal probe;
    probe.source    = TimeSignal::Source::HourPattern;
    probe.reference = m_pattern->currentText().trimmed();
    if (probe.reference.isEmpty()) {
        m_patternPreview->setText(tr("Type a name with the hour in it, for "
                                     "example \"Hour %H\"."));
        return;
    }

    // How many of the twenty-four hours this pattern can actually find audio
    // for. That number is the whole reason the field exists, so it is on
    // screen while the operator types rather than discovered on air.
    QStringList missing;
    for (int hour = 0; hour <= 23; ++hour)
        if (TimeSignal::mediaPathFor(probe, hour).isEmpty())
            missing << QStringLiteral("%1").arg(hour, 2, 10, QLatin1Char('0'));

    const int nowHour = QTime::currentTime().hour();
    const QString nowName = TimeSignal::resolvePattern(probe.reference, nowHour);

    if (missing.isEmpty())
        m_patternPreview->setText(tr("Right now this finds \"%1\", and every "
                                     "hour of the day has a jingle.").arg(nowName));
    else if (missing.size() == 24)
        m_patternPreview->setText(tr("Right now this looks for \"%1\", and no "
                                     "hour has a jingle by that name yet.").arg(nowName));
    else
        m_patternPreview->setText(tr("Right now this finds \"%1\". No jingle for "
                                     "the hours: %2.")
                                      .arg(nowName, missing.join(QStringLiteral(", "))));
}

void TimeSignalDialog::applyHourRange()
{
    const int from = m_rangeFrom->value();
    const int to   = m_rangeTo->value();
    for (int hour = 0; hour <= 23; ++hour) {
        // A range that wraps midnight is a real thing a station asks for
        // ("the overnight ident, eleven to five"), so it is honoured rather
        // than rejected.
        const bool on = (from <= to) ? (hour >= from && hour <= to)
                                     : (hour >= from || hour <= to);
        m_hours[hour]->setChecked(on);
    }
    announce(tr("The hours %1 to %2 are on.")
                 .arg(from, 2, 10, QLatin1Char('0'))
                 .arg(to, 2, 10, QLatin1Char('0')));
}

void TimeSignalDialog::allHours()
{
    for (QCheckBox *box : m_hours)
        box->setChecked(true);
    announce(tr("Every hour is on."));
}

void TimeSignalDialog::noHours()
{
    for (QCheckBox *box : m_hours)
        box->setChecked(false);
    announce(tr("No hours are on; this signal will not fire."));
}

void TimeSignalDialog::everyDay()
{
    for (QCheckBox *box : m_days)
        box->setChecked(true);
    announce(tr("Every day is on."));
}

void TimeSignalDialog::weekdays()
{
    for (int day = 0; day < 7; ++day)
        m_days[day]->setChecked(day < 5);
    announce(tr("Monday to Friday."));
}

void TimeSignalDialog::weekend()
{
    for (int day = 0; day < 7; ++day)
        m_days[day]->setChecked(day >= 5);
    announce(tr("Saturday and Sunday."));
}

// --------------------------------------------------------------- settings --

void TimeSignalDialog::applySettings()
{
    TimeSignal::Settings settings;
    settings.enabled  = m_masterEnabled->isChecked();
    settings.whenLive = m_whenLive->isChecked();
    settings.tickMs   = m_tick->value();
    TimeSignal::saveSettings(settings);

    m_settingsStatus->setText(settings.enabled
        ? tr("Time signals are on.")
        : tr("Time signals are off; nothing in this window reaches the air."));

    emit signalsChanged();
    announce(settings.enabled ? tr("Time signals are on.")
                              : tr("Time signals are off."));
    refreshPreview();
}

void TimeSignalDialog::refreshPreview()
{
    const QList<TimeSignal::Signal> all = TimeSignal::allSignals();
    const QList<TimeSignal::Upcoming> next =
        TimeSignal::upcoming(all, QDateTime::currentDateTime(), 12);

    m_preview->setRowCount(next.size());
    int holes = 0;
    for (int row = 0; row < next.size(); ++row) {
        const TimeSignal::Upcoming &u = next.at(row);
        m_preview->setItem(row, PrevWhen,
                           new QTableWidgetItem(u.when.toString(QStringLiteral("ddd HH:mm:ss"))));
        m_preview->setItem(row, PrevName,
                           new QTableWidgetItem(u.label.isEmpty() ? tr("(unnamed)") : u.label));

        QString plays;
        if (u.path.isEmpty()) {
            ++holes;
            plays = u.reference.isEmpty()
                        ? tr("nothing — no audio chosen")
                        : tr("nothing — no jingle called \"%1\"").arg(u.reference);
        } else {
            plays = QFileInfo(u.path).fileName();
        }
        m_preview->setItem(row, PrevPlays, new QTableWidgetItem(plays));
    }

    if (next.isEmpty())
        m_previewStatus->setText(tr("Nothing is due: there are no signals, or "
                                    "none of them is switched on."));
    else if (holes == 0)
        m_previewStatus->setText(tr("The next %1 firings all have audio.")
                                     .arg(next.size()));
    else
        m_previewStatus->setText(tr("%1 of the next %2 firings would play "
                                    "nothing. Those hours have no jingle yet.")
                                     .arg(holes).arg(next.size()));
}

// ------------------------------------------------------------------ plumbing --

void TimeSignalDialog::setFormEnabled(bool on)
{
    for (QWidget *w : { static_cast<QWidget *>(m_label), static_cast<QWidget *>(m_enabled),
                        static_cast<QWidget *>(m_atMinutes), static_cast<QWidget *>(m_atSeconds),
                        static_cast<QWidget *>(m_source), static_cast<QWidget *>(m_sourceStack),
                        static_cast<QWidget *>(m_placement), static_cast<QWidget *>(m_volume),
                        static_cast<QWidget *>(m_grace), static_cast<QWidget *>(m_applyButton),
                        static_cast<QWidget *>(m_revertButton) })
        w->setEnabled(on);
    for (QCheckBox *box : m_days)
        box->setEnabled(on);
    for (QCheckBox *box : m_hours)
        box->setEnabled(on);
    m_duck->setEnabled(on && m_placement->currentIndex()
                                == int(TimeSignal::Placement::OverTheTop));
    m_dupButton->setEnabled(on);
    m_removeButton->setEnabled(on);
    m_testButton->setEnabled(on);
}

void TimeSignalDialog::setDirty(bool dirty)
{
    m_dirty = dirty;
    if (m_applyButton)
        m_applyButton->setText(dirty ? tr("&Save signal *") : tr("&Save signal"));
}

bool TimeSignalDialog::confirmDiscard()
{
    if (!m_dirty || m_editingId <= 0)
        return true;

    const auto answer = QMessageBox::question(
        this, tr("Time signals"),
        tr("This signal has changes that are not saved. Save them?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Save) {
        applyEdits();
        return !m_dirty;
    }
    setDirty(false);
    return true;
}

void TimeSignalDialog::announce(const QString &message)
{
    emit announcementRequested(message);
}
