#include "RotationDialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QVariant>

namespace {

/** How many library rows the editor lists at once. The search narrows it. */
constexpr int kTrackListLimit = 2000;

QSqlDatabase editorDb()
{
    return QSqlDatabase::database(QStringLiteral("xfb_connection"));
}

enum TrackColumn {
    ColArtist = 0, ColTitle, ColCategory, ColHours, ColDays, ColDates,
    ColWeight, ColCount
};

} // namespace

RotationDialog::RotationDialog(QWidget *parent, const QList<qint64> &preselected)
    : QDialog(parent)
{
    setWindowTitle(tr("Rotation Rules"));
    setModal(false);   // the operator keeps working while the station is on air

    auto *outer = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("A good picker is not a rotation. What a listener notices is the "
           "repetition nobody controlled for: the same artist twice in half an "
           "hour, the same song either side of a shift, a Christmas record in "
           "June. These rules are preferences, not filters — when the library "
           "cannot satisfy them all, Auto Mode gives them up one at a time "
           "rather than going silent, and the last tab says which ones it had "
           "to give up."), this);
    intro->setWordWrap(true);
    outer->addWidget(intro);

    auto *tabs = new QTabWidget(this);
    tabs->setAccessibleName(tr("Rotation sections"));
    tabs->addTab(buildSettingsTab(), tr("&Station Rules"));
    tabs->addTab(buildTracksTab(), tr("&Tracks"));
    tabs->addTab(buildWhyTab(), tr("&Why This Track?"));
    outer->addWidget(tabs, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->setAccessibleName(tr("Dialog buttons"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    outer->addWidget(buttons);

    loadSettingsInto(RotationRules::settings());
    reloadTracks();
    refreshDecisions();
    if (!preselected.isEmpty())
        selectMusicIds(preselected);

    connect(RotationRules::instance(), &RotationRules::decisionRecorded,
            this, &RotationDialog::refreshDecisions);

    resize(940, 680);
}

void RotationDialog::preselect(const QList<qint64> &ids)
{
    reloadTracks();
    if (!ids.isEmpty())
        selectMusicIds(ids);
}

// ------------------------------------------------------------- settings ----

QWidget *RotationDialog::buildSettingsTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *sepBox = new QGroupBox(tr("Separation"), page);
    auto *sepForm = new QFormLayout(sepBox);

    m_enabled = new QCheckBox(tr("Apply rotation rules when Auto Mode picks"), sepBox);
    m_enabled->setAccessibleName(tr("Apply rotation rules when Auto Mode picks"));
    m_enabled->setToolTip(tr("With this off, Auto Mode behaves exactly as it did "
                             "before rotation existed: this hour's genre, a "
                             "matching tempo, and nothing it played recently."));
    sepForm->addRow(m_enabled);

    m_artistMinutes = new QSpinBox(sepBox);
    m_artistMinutes->setRange(0, 24 * 60);
    m_artistMinutes->setSuffix(tr(" min"));
    m_artistMinutes->setSpecialValueText(tr("off"));
    m_artistMinutes->setAccessibleName(tr("Minutes before the same artist may return"));
    sepForm->addRow(tr("Same &artist not within:"), m_artistMinutes);

    m_titleHours = new QSpinBox(sepBox);
    m_titleHours->setRange(0, 24 * 30);
    m_titleHours->setSuffix(tr(" h"));
    m_titleHours->setSpecialValueText(tr("off"));
    m_titleHours->setAccessibleName(tr("Hours before the same title may return"));
    sepForm->addRow(tr("Same t&itle not within:"), m_titleHours);

    auto *sepNote = new QLabel(
        tr("Both are measured against the as-run log — what actually went to "
           "air, not what was queued. On an install whose log is still empty, "
           "the library's own last-played stamp is used instead."), sepBox);
    sepNote->setWordWrap(true);
    sepForm->addRow(sepNote);
    layout->addWidget(sepBox);

    auto *weightBox = new QGroupBox(tr("Category weights"), page);
    auto *weightForm = new QFormLayout(weightBox);

    auto *weightNote = new QLabel(
        tr("How often a category comes up relative to the others. A power "
           "track at 10 against a gold track at 2 is picked five times as "
           "often when both are eligible."), weightBox);
    weightNote->setWordWrap(true);
    weightForm->addRow(weightNote);

    m_powerWeight = new QDoubleSpinBox(weightBox);
    m_powerWeight->setRange(0.0, 1000.0);
    m_powerWeight->setDecimals(1);
    m_powerWeight->setAccessibleName(tr("Power category weight"));
    weightForm->addRow(tr("&Power:"), m_powerWeight);

    m_secondaryWeight = new QDoubleSpinBox(weightBox);
    m_secondaryWeight->setRange(0.0, 1000.0);
    m_secondaryWeight->setDecimals(1);
    m_secondaryWeight->setAccessibleName(tr("Secondary category weight"));
    weightForm->addRow(tr("&Secondary:"), m_secondaryWeight);

    m_goldWeight = new QDoubleSpinBox(weightBox);
    m_goldWeight->setRange(0.0, 1000.0);
    m_goldWeight->setDecimals(1);
    m_goldWeight->setAccessibleName(tr("Gold category weight"));
    weightForm->addRow(tr("&Gold:"), m_goldWeight);

    m_defaultCategory = new QComboBox(weightBox);
    for (const QString &key : RotationRules::categoryKeys())
        m_defaultCategory->addItem(RotationRules::categoryLabel(key), key);
    m_defaultCategory->setAccessibleName(tr("Category for tracks with no rules of their own"));
    m_defaultCategory->setToolTip(tr("Nobody categorises ten thousand tracks by "
                                     "hand on day one, so a track with no "
                                     "rotation row of its own runs on this."));
    weightForm->addRow(tr("&Default category:"), m_defaultCategory);

    m_candidateLimit = new QSpinBox(weightBox);
    m_candidateLimit->setRange(20, 5000);
    m_candidateLimit->setAccessibleName(tr("How many candidates each pick considers"));
    m_candidateLimit->setToolTip(tr("A random sample of the eligible pool, large "
                                    "enough to be a fair draw and small enough "
                                    "that a huge library still picks instantly."));
    weightForm->addRow(tr("Candidates per pic&k:"), m_candidateLimit);
    layout->addWidget(weightBox);

    m_settingsStatus = new QLabel(page);
    m_settingsStatus->setWordWrap(true);
    m_settingsStatus->setAccessibleName(tr("Rotation settings status"));
    layout->addWidget(m_settingsStatus);

    auto *row = new QHBoxLayout;
    auto *defaults = new QPushButton(tr("&Restore Defaults"), page);
    defaults->setAccessibleName(tr("Restore the default rotation settings"));
    connect(defaults, &QPushButton::clicked, this, &RotationDialog::restoreDefaultSettings);
    auto *apply = new QPushButton(tr("&Apply"), page);
    apply->setAccessibleName(tr("Apply the rotation settings"));
    apply->setDefault(true);
    connect(apply, &QPushButton::clicked, this, &RotationDialog::applySettings);
    row->addWidget(defaults);
    row->addStretch(1);
    row->addWidget(apply);
    layout->addLayout(row);
    layout->addStretch(1);
    return page;
}

void RotationDialog::loadSettingsInto(const RotationRules::Settings &s)
{
    m_enabled->setChecked(s.enabled);
    m_artistMinutes->setValue(s.artistSeparationMinutes);
    m_titleHours->setValue(s.titleSeparationHours);
    m_powerWeight->setValue(s.powerWeight);
    m_secondaryWeight->setValue(s.secondaryWeight);
    m_goldWeight->setValue(s.goldWeight);
    const int index = m_defaultCategory->findData(s.defaultCategory);
    m_defaultCategory->setCurrentIndex(index >= 0 ? index : 1);
    m_candidateLimit->setValue(s.candidateLimit);
}

RotationRules::Settings RotationDialog::readSettingsFromForm() const
{
    RotationRules::Settings s;
    s.enabled = m_enabled->isChecked();
    s.artistSeparationMinutes = m_artistMinutes->value();
    s.titleSeparationHours = m_titleHours->value();
    s.powerWeight = m_powerWeight->value();
    s.secondaryWeight = m_secondaryWeight->value();
    s.goldWeight = m_goldWeight->value();
    s.defaultCategory = m_defaultCategory->currentData().toString();
    s.candidateLimit = m_candidateLimit->value();
    return s;
}

void RotationDialog::applySettings()
{
    RotationRules::saveSettings(readSettingsFromForm());
    const QString message = tr("Rotation settings saved. %n track(s) have rules "
                               "of their own.", nullptr, RotationRules::ruleCount());
    m_settingsStatus->setText(message);
    emit announcementRequested(message);
}

void RotationDialog::restoreDefaultSettings()
{
    loadSettingsInto(RotationRules::Settings());
    m_settingsStatus->setText(tr("Defaults restored — press Apply to keep them."));
    emit announcementRequested(m_settingsStatus->text());
}

// --------------------------------------------------------------- tracks ----

QWidget *RotationDialog::buildTracksTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *searchRow = new QHBoxLayout;
    auto *searchLabel = new QLabel(tr("&Find:"), page);
    m_search = new QLineEdit(page);
    m_search->setPlaceholderText(tr("Artist or title"));
    m_search->setClearButtonEnabled(true);
    m_search->setAccessibleName(tr("Search the library by artist or title"));
    searchLabel->setBuddy(m_search);
    connect(m_search, &QLineEdit::returnPressed, this, &RotationDialog::reloadTracks);
    auto *searchButton = new QPushButton(tr("&Search"), page);
    searchButton->setAccessibleName(tr("Search the library"));
    connect(searchButton, &QPushButton::clicked, this, &RotationDialog::reloadTracks);
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(m_search, 1);
    searchRow->addWidget(searchButton);
    layout->addLayout(searchRow);

    m_tracks = new QTableWidget(page);
    m_tracks->setColumnCount(ColCount);
    m_tracks->setHorizontalHeaderLabels({ tr("Artist"), tr("Title"), tr("Category"),
                                          tr("Hours"), tr("Days"), tr("Dates"),
                                          tr("Weight") });
    m_tracks->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tracks->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tracks->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tracks->setAlternatingRowColors(true);
    m_tracks->setSortingEnabled(true);
    m_tracks->verticalHeader()->setVisible(false);
    m_tracks->horizontalHeader()->setStretchLastSection(true);
    m_tracks->setAccessibleName(tr("Library tracks and their rotation rules"));
    m_tracks->setAccessibleDescription(
        tr("Select one or more tracks, then set their rules below and choose "
           "Apply to Selection."));
    connect(m_tracks, &QTableWidget::itemSelectionChanged,
            this, &RotationDialog::updateSelectionSummary);
    layout->addWidget(m_tracks, 1);

    m_trackSummary = new QLabel(page);
    m_trackSummary->setWordWrap(true);
    m_trackSummary->setAccessibleName(tr("Track selection summary"));
    layout->addWidget(m_trackSummary);

    // ------------------------------------------------------- bulk editing --
    auto *editBox = new QGroupBox(tr("Set on the selected tracks"), page);
    auto *editLayout = new QVBoxLayout(editBox);

    auto *editNote = new QLabel(
        tr("Only the ticked rows are written; everything left unticked keeps "
           "whatever the selected tracks already had."), editBox);
    editNote->setWordWrap(true);
    editLayout->addWidget(editNote);

    auto *categoryRow = new QHBoxLayout;
    m_setCategory = new QCheckBox(tr("&Category:"), editBox);
    m_setCategory->setAccessibleName(tr("Set the category on the selected tracks"));
    m_category = new QComboBox(editBox);
    for (const QString &key : RotationRules::categoryKeys())
        m_category->addItem(RotationRules::categoryLabel(key), key);
    m_category->setAccessibleName(tr("Category to set"));
    categoryRow->addWidget(m_setCategory);
    categoryRow->addWidget(m_category);
    categoryRow->addStretch(1);
    editLayout->addLayout(categoryRow);

    // Dayparting: hours.
    m_setHours = new QCheckBox(tr("&Hours the track may play:"), editBox);
    m_setHours->setAccessibleName(tr("Set the permitted hours on the selected tracks"));
    editLayout->addWidget(m_setHours);

    auto *hourGrid = new QGridLayout;
    for (int h = 0; h < 24; ++h) {
        m_hour[h] = new QCheckBox(QStringLiteral("%1").arg(h, 2, 10, QLatin1Char('0')),
                                  editBox);
        m_hour[h]->setChecked(true);
        m_hour[h]->setAccessibleName(tr("Allow the %1 o'clock hour")
                                         .arg(h, 2, 10, QLatin1Char('0')));
        hourGrid->addWidget(m_hour[h], h / 12, h % 12);
    }
    editLayout->addLayout(hourGrid);

    auto *hourButtons = new QHBoxLayout;
    m_allHours = new QPushButton(tr("All ho&urs"), editBox);
    m_allHours->setAccessibleName(tr("Tick every hour"));
    connect(m_allHours, &QPushButton::clicked, this, &RotationDialog::selectAllHours);
    m_noHours = new QPushButton(tr("&No hours"), editBox);
    m_noHours->setAccessibleName(tr("Untick every hour"));
    connect(m_noHours, &QPushButton::clicked, this, &RotationDialog::clearAllHours);
    hourButtons->addWidget(m_allHours);
    hourButtons->addWidget(m_noHours);
    hourButtons->addStretch(1);
    editLayout->addLayout(hourButtons);

    // Dayparting: weekdays.
    m_setDays = new QCheckBox(tr("&Weekdays the track may play:"), editBox);
    m_setDays->setAccessibleName(tr("Set the permitted weekdays on the selected tracks"));
    editLayout->addWidget(m_setDays);

    auto *dayRow = new QHBoxLayout;
    const QStringList dayNames = { tr("Mon"), tr("Tue"), tr("Wed"), tr("Thu"),
                                   tr("Fri"), tr("Sat"), tr("Sun") };
    for (int d = 0; d < 7; ++d) {
        m_day[d] = new QCheckBox(dayNames.at(d), editBox);
        m_day[d]->setChecked(true);
        m_day[d]->setAccessibleName(tr("Allow %1").arg(dayNames.at(d)));
        dayRow->addWidget(m_day[d]);
    }
    dayRow->addStretch(1);
    editLayout->addLayout(dayRow);

    // Date windows.
    auto *dateForm = new QFormLayout;
    auto *startRow = new QHBoxLayout;
    m_setStart = new QCheckBox(tr("Not &before:"), editBox);
    m_setStart->setAccessibleName(tr("Set the earliest date on the selected tracks"));
    m_startDate = new QDateEdit(QDate::currentDate(), editBox);
    m_startDate->setCalendarPopup(true);
    m_startDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_startDate->setAccessibleName(tr("Earliest date the track may play"));
    startRow->addWidget(m_setStart);
    startRow->addWidget(m_startDate);
    startRow->addStretch(1);
    dateForm->addRow(startRow);

    auto *endRow = new QHBoxLayout;
    m_setEnd = new QCheckBox(tr("&Expires after:"), editBox);
    m_setEnd->setAccessibleName(tr("Set the expiry date on the selected tracks"));
    m_endDate = new QDateEdit(QDate::currentDate().addYears(1), editBox);
    m_endDate->setCalendarPopup(true);
    m_endDate->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_endDate->setAccessibleName(tr("Last date the track may play"));
    endRow->addWidget(m_setEnd);
    endRow->addWidget(m_endDate);
    endRow->addStretch(1);
    dateForm->addRow(endRow);
    editLayout->addLayout(dateForm);

    auto *dateNote = new QLabel(
        tr("An unticked date means no limit. Leaving both unticked and ticking "
           "nothing else clears nothing — use Clear Rules for that."), editBox);
    dateNote->setWordWrap(true);
    editLayout->addWidget(dateNote);

    // Per-track weight override.
    auto *weightRow = new QHBoxLayout;
    m_setWeight = new QCheckBox(tr("Wei&ght:"), editBox);
    m_setWeight->setAccessibleName(tr("Set the weight on the selected tracks"));
    m_weight = new QDoubleSpinBox(editBox);
    m_weight->setRange(0.0, 1000.0);
    m_weight->setDecimals(1);
    m_weight->setValue(5.0);
    m_weight->setAccessibleName(tr("Weight to set"));
    m_weightFromCategory = new QCheckBox(tr("use the category's weight"), editBox);
    m_weightFromCategory->setChecked(true);
    m_weightFromCategory->setAccessibleName(tr("Take the weight from the category "
                                               "instead of setting it per track"));
    connect(m_weightFromCategory, &QCheckBox::toggled, m_weight, &QWidget::setDisabled);
    m_weight->setEnabled(false);
    weightRow->addWidget(m_setWeight);
    weightRow->addWidget(m_weight);
    weightRow->addWidget(m_weightFromCategory);
    weightRow->addStretch(1);
    editLayout->addLayout(weightRow);

    auto *actionRow = new QHBoxLayout;
    auto *clearButton = new QPushButton(tr("C&lear Rules"), editBox);
    clearButton->setAccessibleName(tr("Remove all rotation rules from the selected tracks"));
    clearButton->setToolTip(tr("The tracks go back to the station defaults."));
    connect(clearButton, &QPushButton::clicked, this, &RotationDialog::clearSelection);
    auto *applyButton = new QPushButton(tr("A&pply to Selection"), editBox);
    applyButton->setAccessibleName(tr("Apply the ticked rules to the selected tracks"));
    connect(applyButton, &QPushButton::clicked, this, &RotationDialog::applyToSelection);
    actionRow->addWidget(clearButton);
    actionRow->addStretch(1);
    actionRow->addWidget(applyButton);
    editLayout->addLayout(actionRow);

    layout->addWidget(editBox);
    return page;
}

void RotationDialog::selectAllHours()
{
    for (int h = 0; h < 24; ++h)
        m_hour[h]->setChecked(true);
}

void RotationDialog::clearAllHours()
{
    for (int h = 0; h < 24; ++h)
        m_hour[h]->setChecked(false);
}

void RotationDialog::reloadTracks()
{
    QSqlDatabase db = editorDb();
    if (!db.isOpen()) {
        m_trackSummary->setText(tr("The library database is not open."));
        return;
    }

    const QString needle = m_search ? m_search->text().trimmed() : QString();
    QSqlQuery qry(db);
    if (needle.isEmpty()) {
        qry.prepare(QStringLiteral("SELECT id, artist, song FROM musics"
                                   " ORDER BY artist, song LIMIT :limit"));
    } else {
        qry.prepare(QStringLiteral("SELECT id, artist, song FROM musics"
                                   " WHERE artist LIKE :q OR song LIKE :q"
                                   " ORDER BY artist, song LIMIT :limit"));
        qry.bindValue(QStringLiteral(":q"),
                      QLatin1Char('%') + needle + QLatin1Char('%'));
    }
    qry.bindValue(QStringLiteral(":limit"), kTrackListLimit);
    if (!qry.exec()) {
        m_trackSummary->setText(tr("Could not read the library: %1")
                                    .arg(qry.lastError().text()));
        return;
    }

    struct Row { qint64 id; QString artist; QString title; };
    QList<Row> rows;
    QList<qint64> ids;
    while (qry.next()) {
        Row r{ qry.value(0).toLongLong(), qry.value(1).toString(),
               qry.value(2).toString() };
        rows.append(r);
        ids.append(r.id);
    }
    const QHash<qint64, RotationRules::Rule> rules = RotationRules::rulesFor(ids);
    const RotationRules::Settings s = RotationRules::settings();

    const bool wasSorting = m_tracks->isSortingEnabled();
    m_tracks->setSortingEnabled(false);
    m_tracks->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const Row &r = rows.at(i);
        const bool hasRule = rules.contains(r.id);
        const RotationRules::Rule rule =
            hasRule ? rules.value(r.id) : RotationRules::defaultRule(r.id, s);

        auto *artistItem = new QTableWidgetItem(r.artist);
        artistItem->setData(Qt::UserRole, QVariant(r.id));
        m_tracks->setItem(i, ColArtist, artistItem);
        m_tracks->setItem(i, ColTitle, new QTableWidgetItem(r.title));
        m_tracks->setItem(i, ColCategory,
                          new QTableWidgetItem(RotationRules::categoryLabel(rule.category)));
        m_tracks->setItem(i, ColHours,
                          new QTableWidgetItem(RotationRules::describeHours(rule.hourMask)));
        m_tracks->setItem(i, ColDays,
                          new QTableWidgetItem(RotationRules::describeDays(rule.dayMask)));
        m_tracks->setItem(i, ColDates,
                          new QTableWidgetItem(RotationRules::describeDates(rule.startDate,
                                                                           rule.endDate)));
        const QString weightText =
            rule.weight >= 0.0 ? QString::number(rule.weight, 'f', 1)
                               : tr("%1 (category)")
                                     .arg(RotationRules::categoryWeight(rule.category, s),
                                          0, 'f', 1);
        m_tracks->setItem(i, ColWeight, new QTableWidgetItem(weightText));

        // A screen reader reading one cell should still know which track it is.
        for (int c = 0; c < ColCount; ++c) {
            if (QTableWidgetItem *item = m_tracks->item(i, c)) {
                item->setToolTip(tr("%1 — %2").arg(r.artist, r.title));
                if (!hasRule)
                    item->setData(Qt::AccessibleDescriptionRole,
                                  tr("on the station defaults"));
            }
        }
    }
    m_tracks->setSortingEnabled(wasSorting);
    m_tracks->resizeColumnsToContents();

    m_trackSummary->setText(
        tr("%n track(s) listed; %1 in the library have rules of their own.",
           nullptr, rows.size()).arg(RotationRules::ruleCount()));
    updateSelectionSummary();
}

QList<qint64> RotationDialog::selectedMusicIds() const
{
    QList<qint64> ids;
    if (!m_tracks)
        return ids;
    const QModelIndexList selected = m_tracks->selectionModel()
                                         ? m_tracks->selectionModel()->selectedRows()
                                         : QModelIndexList();
    for (const QModelIndex &index : selected) {
        if (QTableWidgetItem *item = m_tracks->item(index.row(), ColArtist)) {
            const qint64 id = item->data(Qt::UserRole).toLongLong();
            if (id > 0 && !ids.contains(id))
                ids.append(id);
        }
    }
    return ids;
}

void RotationDialog::selectMusicIds(const QList<qint64> &ids)
{
    if (!m_tracks || ids.isEmpty())
        return;
    m_tracks->clearSelection();
    for (int row = 0; row < m_tracks->rowCount(); ++row) {
        QTableWidgetItem *item = m_tracks->item(row, ColArtist);
        if (item && ids.contains(item->data(Qt::UserRole).toLongLong()))
            m_tracks->selectRow(row);
    }
    updateSelectionSummary();
}

void RotationDialog::updateSelectionSummary()
{
    if (!m_trackSummary)
        return;
    const int count = selectedMusicIds().size();
    const QString base = tr("%n track(s) selected.", nullptr, count);
    m_trackSummary->setText(base);
}

void RotationDialog::applyToSelection()
{
    const QList<qint64> ids = selectedMusicIds();
    if (ids.isEmpty()) {
        m_trackSummary->setText(tr("Select some tracks first."));
        emit announcementRequested(m_trackSummary->text());
        return;
    }

    const QHash<qint64, RotationRules::Rule> existing = RotationRules::rulesFor(ids);
    const RotationRules::Settings s = RotationRules::settings();

    QList<int> hours;
    for (int h = 0; h < 24; ++h) {
        if (m_hour[h]->isChecked())
            hours << h;
    }
    quint8 dayMask = 0;
    for (int d = 0; d < 7; ++d) {
        if (m_day[d]->isChecked())
            dayMask |= quint8(1u << d);
    }

    int written = 0;
    QSqlDatabase db = editorDb();
    const bool inTransaction = db.isOpen() && db.transaction();
    for (qint64 id : ids) {
        RotationRules::Rule rule = existing.contains(id)
                                       ? existing.value(id)
                                       : RotationRules::defaultRule(id, s);
        rule.musicId = id;
        if (m_setCategory->isChecked())
            rule.category = m_category->currentData().toString();
        if (m_setHours->isChecked())
            rule.hourMask = RotationRules::hoursFromList(hours);
        if (m_setDays->isChecked())
            rule.dayMask = dayMask ? dayMask : quint8(0x7F);
        if (m_setStart->isChecked())
            rule.startDate = m_startDate->date();
        if (m_setEnd->isChecked())
            rule.endDate = m_endDate->date();
        if (m_setWeight->isChecked())
            rule.weight = m_weightFromCategory->isChecked() ? -1.0 : m_weight->value();
        if (RotationRules::saveRule(rule))
            ++written;
    }
    if (inTransaction)
        db.commit();

    reloadTracks();
    selectMusicIds(ids);
    const QString message = tr("Rotation rules written for %n track(s).",
                               nullptr, written);
    m_trackSummary->setText(message);
    emit announcementRequested(message);
}

void RotationDialog::clearSelection()
{
    const QList<qint64> ids = selectedMusicIds();
    if (ids.isEmpty()) {
        m_trackSummary->setText(tr("Select some tracks first."));
        emit announcementRequested(m_trackSummary->text());
        return;
    }
    int cleared = 0;
    QSqlDatabase db = editorDb();
    const bool inTransaction = db.isOpen() && db.transaction();
    for (qint64 id : ids) {
        if (RotationRules::clearRule(id))
            ++cleared;
    }
    if (inTransaction)
        db.commit();

    reloadTracks();
    selectMusicIds(ids);
    const QString message = tr("%n track(s) put back on the station defaults.",
                               nullptr, cleared);
    m_trackSummary->setText(message);
    emit announcementRequested(message);
}

// ------------------------------------------------------ why this track? ----

QWidget *RotationDialog::buildWhyTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *note = new QLabel(
        tr("The last few Auto Mode picks, and what each one cost. A pick with "
           "nothing in the \"Relaxed\" column satisfied every rule. A column "
           "that is never empty is telling you a rule is wrong for this "
           "library — a 40 minute artist separation cannot be kept by a "
           "library of two hundred tracks."), page);
    note->setWordWrap(true);
    layout->addWidget(note);

    m_decisions = new QTableWidget(page);
    m_decisions->setColumnCount(6);
    m_decisions->setHorizontalHeaderLabels({ tr("Time"), tr("Track"), tr("Pass"),
                                             tr("Relaxed"), tr("Pool"),
                                             tr("Category / weight") });
    m_decisions->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_decisions->setSelectionMode(QAbstractItemView::SingleSelection);
    m_decisions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_decisions->setAlternatingRowColors(true);
    m_decisions->verticalHeader()->setVisible(false);
    m_decisions->horizontalHeader()->setStretchLastSection(true);
    m_decisions->setAccessibleName(tr("Recent Auto Mode picks"));
    m_decisions->setAccessibleDescription(
        tr("Select a pick to read why the tracks it turned down were turned down."));
    connect(m_decisions, &QTableWidget::itemSelectionChanged,
            this, &RotationDialog::showDecisionDetail);
    layout->addWidget(m_decisions, 2);

    m_detail = new QTextBrowser(page);
    m_detail->setAccessibleName(tr("Why this track was picked"));
    layout->addWidget(m_detail, 1);

    auto *row = new QHBoxLayout;
    auto *refresh = new QPushButton(tr("&Refresh"), page);
    refresh->setAccessibleName(tr("Reload the recent picks"));
    connect(refresh, &QPushButton::clicked, this, &RotationDialog::refreshDecisions);
    auto *clear = new QPushButton(tr("&Clear"), page);
    clear->setAccessibleName(tr("Forget the recent picks"));
    connect(clear, &QPushButton::clicked, this, []() {
        RotationRules::instance()->clearDecisions();
    });
    row->addWidget(refresh);
    row->addWidget(clear);
    row->addStretch(1);
    layout->addLayout(row);
    return page;
}

void RotationDialog::refreshDecisions()
{
    if (!m_decisions)
        return;
    m_decisionRows = RotationRules::instance()->recentDecisions();
    m_decisions->setRowCount(m_decisionRows.size());
    for (int i = 0; i < m_decisionRows.size(); ++i) {
        const RotationRules::Decision &d = m_decisionRows.at(i);
        const QString track = d.artist.isEmpty()
                                  ? (d.title.isEmpty() ? d.path : d.title)
                                  : tr("%1 — %2").arg(d.artist, d.title);
        m_decisions->setItem(i, 0,
                             new QTableWidgetItem(d.when.toString(QStringLiteral("hh:mm:ss"))));
        m_decisions->setItem(i, 1, new QTableWidgetItem(track));
        m_decisions->setItem(i, 2, new QTableWidgetItem(d.passName));
        m_decisions->setItem(i, 3,
                             new QTableWidgetItem(d.rulesActive
                                                      ? d.relaxedRules.join(QStringLiteral(", "))
                                                      : tr("rotation switched off")));
        m_decisions->setItem(i, 4,
                             new QTableWidgetItem(tr("%1 of %2")
                                                      .arg(d.survivors).arg(d.poolSize)));
        m_decisions->setItem(i, 5,
                             new QTableWidgetItem(
                                 d.rulesActive
                                     ? tr("%1 / %2").arg(RotationRules::categoryLabel(d.category),
                                                         QString::number(d.weight, 'f', 1))
                                     : QStringLiteral("-")));
    }
    m_decisions->resizeColumnsToContents();
    if (m_decisionRows.isEmpty() && m_detail) {
        m_detail->setPlainText(
            tr("Auto Mode has not picked anything since XFB started. Turn Auto "
               "Mode on and the reason for each pick appears here."));
    }
}

void RotationDialog::showDecisionDetail()
{
    if (!m_detail || !m_decisions)
        return;
    const int row = m_decisions->currentRow();
    if (row < 0 || row >= m_decisionRows.size())
        return;
    const RotationRules::Decision &d = m_decisionRows.at(row);

    QStringList lines;
    lines << tr("Picked: %1").arg(d.path);
    lines << tr("At: %1").arg(d.when.toString(Qt::ISODate));
    lines << tr("Pass: %1 (%2 candidates offered, %3 survived the rules)")
                 .arg(d.passName).arg(d.poolSize).arg(d.survivors);
    if (!d.rulesActive) {
        lines << tr("Rotation rules were switched off for this pick.");
    } else {
        lines << tr("Category: %1, weight %2")
                     .arg(RotationRules::categoryLabel(d.category),
                          QString::number(d.weight, 'f', 1));
        lines << (d.relaxedRules.isEmpty()
                      ? tr("Every rotation rule was honoured.")
                      : tr("Rules relaxed, in the order they were given up: %1")
                            .arg(d.relaxedRules.join(QStringLiteral(", "))));
        lines << (d.historyFromAirLog
                      ? tr("Recent play history came from the as-run log.")
                      : tr("The as-run log is still empty, so the library's "
                           "last-played stamps were used instead."));
        if (!d.rejections.isEmpty()) {
            lines << QString();
            lines << tr("Some of what it turned down:");
            for (const QString &line : d.rejections)
                lines << QStringLiteral("  • ") + line;
        }
    }
    m_detail->setPlainText(lines.join(QLatin1Char('\n')));
}
