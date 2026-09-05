#include "QuotaDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

const char *const kConnection = "xfb_connection";

/** The library, on the one connection every other query in XFB uses. */
QSqlDatabase editorDb()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

/** Bounded so an eight-thousand-track library cannot freeze the window. The
 *  search box is how the operator reaches past it, and the summary says so. */
constexpr int kTrackListLimit = 2000;

/** Rows the detail table shows. The CSV carries the whole month. */
constexpr int kDetailLimit = 3000;

/** Widest a column is allowed to get. resizeColumnsToContents() sizes to the
 *  longest cell, and one forty-word track title otherwise pushes every other
 *  column off the right of the window. */
constexpr int kMaxColumnWidth = 260;

/** resizeColumnsToContents(), then that cap. */
void fitColumns(QTableWidget *table)
{
    table->resizeColumnsToContents();
    for (int c = 0; c < table->columnCount(); ++c)
        table->setColumnWidth(c, qMin(table->columnWidth(c), kMaxColumnWidth));
}

enum TrackColumn {
    ColArtist = 0, ColTitle, ColCountry, ColOrigin, ColLanguage,
    ColPerformer, ColReleased, ColCount
};

enum DetailColumn {
    DetWhen = 0, DetArtist, DetTitle, DetLength, DetPrime, DetOrigin,
    DetCounts, DetCount
};

/** One CSV field, quoted the way a spreadsheet expects. */
QString csvField(const QString &value)
{
    QString out = value;
    out.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + out + QLatin1Char('"');
}

/** A length in milliseconds as mm:ss, or an em dash when it is not known. */
QString lengthText(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("—");
    const qint64 seconds = ms / 1000;
    return QStringLiteral("%1:%2")
        .arg(seconds / 60)
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

QString escape(const QString &text)
{
    return text.toHtmlEscaped();
}

} // namespace

QuotaDialog::QuotaDialog(QWidget *parent, const QList<qint64> &preselected)
    : QDialog(parent)
{
    setWindowTitle(tr("Music Quota"));
    setModal(false);            // marking the library must not stop the station
    setAccessibleName(tr("Music quota"));
    setAccessibleDescription(
        tr("Mark which tracks count towards the national music quota, and "
           "measure what actually went to air against it."));

    m_settings = MusicQuota::settings();

    auto *layout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);
    tabs->setAccessibleName(tr("Music quota sections"));
    tabs->addTab(buildTracksTab(), tr("&Marking"));
    tabs->addTab(buildReportTab(), tr("&Report"));
    tabs->addTab(buildRulesTab(), tr("R&ules"));
    layout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)
        ->setAccessibleName(tr("Close the music quota window"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    resize(1000, 660);

    loadSettingsInto(m_settings);
    reloadTracks();
    if (!preselected.isEmpty())
        selectMusicIds(preselected);
    showThisMonth();
}

void QuotaDialog::preselect(const QList<qint64> &ids)
{
    reloadTracks();
    selectMusicIds(ids);
}

// ------------------------------------------------------- the marking tab --

QWidget *QuotaDialog::buildTracksTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("A track with no mark is not counted against the station — it is "
           "counted as unmarked, and the report says how much of the month "
           "that was. Mark the library a handful of albums at a time; the "
           "figures get sharper as you go."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *searchRow = new QHBoxLayout;
    auto *searchLabel = new QLabel(tr("&Find:"), page);
    m_search = new QLineEdit(page);
    m_search->setPlaceholderText(tr("Artist or title"));
    m_search->setClearButtonEnabled(true);
    m_search->setAccessibleName(tr("Search the library by artist or title"));
    searchLabel->setBuddy(m_search);
    connect(m_search, &QLineEdit::returnPressed, this, &QuotaDialog::reloadTracks);
    auto *searchButton = new QPushButton(tr("&Search"), page);
    searchButton->setAccessibleName(tr("Search the library"));
    connect(searchButton, &QPushButton::clicked, this, &QuotaDialog::reloadTracks);
    m_unmarkedOnly = new QCheckBox(tr("&Only tracks with no mark"), page);
    m_unmarkedOnly->setAccessibleName(tr("Show only tracks that carry no mark"));
    connect(m_unmarkedOnly, &QCheckBox::toggled, this, &QuotaDialog::reloadTracks);
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(m_search, 1);
    searchRow->addWidget(searchButton);
    searchRow->addWidget(m_unmarkedOnly);
    layout->addLayout(searchRow);

    m_tracks = new QTableWidget(page);
    m_tracks->setColumnCount(ColCount);
    m_tracks->setHorizontalHeaderLabels({ tr("Artist"), tr("Title"),
                                          tr("Country (library)"), tr("Origin"),
                                          tr("Language"), tr("Performers"),
                                          tr("First published") });
    m_tracks->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tracks->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tracks->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tracks->setAlternatingRowColors(true);
    m_tracks->setSortingEnabled(true);
    m_tracks->verticalHeader()->setVisible(false);
    m_tracks->horizontalHeader()->setStretchLastSection(true);
    m_tracks->setAccessibleName(tr("Library tracks and their quota marks"));
    m_tracks->setAccessibleDescription(
        tr("Select one or more tracks, then set what is true of them below "
           "and choose Apply to Selection."));
    connect(m_tracks, &QTableWidget::itemSelectionChanged,
            this, &QuotaDialog::updateSelectionSummary);
    layout->addWidget(m_tracks, 1);

    m_trackSummary = new QLabel(page);
    m_trackSummary->setWordWrap(true);
    m_trackSummary->setAccessibleName(tr("Marking summary"));
    layout->addWidget(m_trackSummary);

    // ------------------------------------------------------- bulk editing --
    auto *editBox = new QGroupBox(tr("Set on the selected tracks"), page);
    auto *editLayout = new QFormLayout(editBox);

    auto *editNote = new QLabel(
        tr("Only the ticked lines are written; everything left unticked keeps "
           "whatever the selected tracks already had."), editBox);
    editNote->setWordWrap(true);
    editLayout->addRow(editNote);

    auto *originRow = new QHBoxLayout;
    m_setOrigin = new QCheckBox(tr("&Origin:"), editBox);
    m_setOrigin->setAccessibleName(tr("Set the origin on the selected tracks"));
    m_origin = new QComboBox(editBox);
    m_origin->addItem(MusicQuota::originLabel(MusicQuota::National),
                      int(MusicQuota::National));
    m_origin->addItem(MusicQuota::originLabel(MusicQuota::International),
                      int(MusicQuota::International));
    m_origin->addItem(MusicQuota::originLabel(MusicQuota::OriginUnset),
                      int(MusicQuota::OriginUnset));
    m_origin->setAccessibleName(tr("Origin to set"));
    originRow->addWidget(m_setOrigin);
    originRow->addWidget(m_origin);
    originRow->addStretch(1);
    editLayout->addRow(originRow);

    auto *languageRow = new QHBoxLayout;
    m_setLanguage = new QCheckBox(tr("&Language:"), editBox);
    m_setLanguage->setAccessibleName(tr("Set the language on the selected tracks"));
    m_language = new QComboBox(editBox);
    m_language->setAccessibleName(tr("Language to set"));
    languageRow->addWidget(m_setLanguage);
    languageRow->addWidget(m_language);
    languageRow->addStretch(1);
    editLayout->addRow(languageRow);

    auto *performerRow = new QHBoxLayout;
    m_setPerformer = new QCheckBox(tr("&Performers:"), editBox);
    m_setPerformer->setAccessibleName(
        tr("Set the performers' citizenship on the selected tracks"));
    m_performer = new QComboBox(editBox);
    m_performer->addItem(tr("Citizens of an EU Member State"), 1);
    m_performer->addItem(tr("Not EU citizens"), 0);
    m_performer->addItem(tr("Not said"), -1);
    m_performer->setAccessibleName(tr("Citizenship to set"));
    performerRow->addWidget(m_setPerformer);
    performerRow->addWidget(m_performer);
    performerRow->addStretch(1);
    editLayout->addRow(performerRow);

    auto *releaseRow = new QHBoxLayout;
    m_setRelease = new QCheckBox(tr("First p&ublished:"), editBox);
    m_setRelease->setAccessibleName(
        tr("Set the first publication date on the selected tracks"));
    m_release = new QDateEdit(QDate::currentDate(), editBox);
    m_release->setCalendarPopup(true);
    m_release->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_release->setAccessibleName(tr("Date the record was first published or broadcast"));
    releaseRow->addWidget(m_setRelease);
    releaseRow->addWidget(m_release);
    releaseRow->addStretch(1);
    editLayout->addRow(releaseRow);

    auto *musicRow = new QHBoxLayout;
    m_setIsMusic = new QCheckBox(tr("Part of the &musical programming:"), editBox);
    m_setIsMusic->setAccessibleName(
        tr("Say whether the selected tracks are musical programming at all"));
    m_isMusic = new QComboBox(editBox);
    m_isMusic->addItem(tr("Yes"), 1);
    m_isMusic->addItem(tr("No — leave it out of the figures"), 0);
    m_isMusic->setAccessibleName(tr("Whether these rows are musical programming"));
    musicRow->addWidget(m_setIsMusic);
    musicRow->addWidget(m_isMusic);
    musicRow->addStretch(1);
    editLayout->addRow(musicRow);

    auto *buttonRow = new QHBoxLayout;
    auto *apply = new QPushButton(tr("&Apply to Selection"), editBox);
    apply->setAccessibleName(tr("Write these marks onto the selected tracks"));
    connect(apply, &QPushButton::clicked, this, &QuotaDialog::applyToSelection);
    auto *clear = new QPushButton(tr("&Remove the Marks"), editBox);
    clear->setAccessibleName(
        tr("Remove the quota marks from the selected tracks entirely"));
    connect(clear, &QPushButton::clicked, this, &QuotaDialog::clearSelection);
    auto *seed = new QPushButton(tr("&Fill in from the Country column..."), editBox);
    seed->setAccessibleName(
        tr("Give unmarked tracks a first origin from the library's country column"));
    seed->setToolTip(
        tr("Marks every track that has no mark yet as national or "
           "international according to what its Country column says, and takes "
           "its first publication date from the library. Tracks whose country "
           "is empty or is a placeholder are left unmarked, and a track that "
           "has already been marked is never changed."));
    connect(seed, &QPushButton::clicked, this, &QuotaDialog::seedFromLibrary);
    buttonRow->addWidget(apply);
    buttonRow->addWidget(clear);
    buttonRow->addStretch(1);
    buttonRow->addWidget(seed);
    editLayout->addRow(buttonRow);

    layout->addWidget(editBox);
    return page;
}

void QuotaDialog::reloadTracks()
{
    QSqlDatabase db = editorDb();
    if (!db.isOpen()) {
        m_trackSummaryBase = tr("The library database is not open.");
        updateSelectionSummary();
        return;
    }

    const QString needle = m_search ? m_search->text().trimmed() : QString();
    const bool unmarkedOnly = m_unmarkedOnly && m_unmarkedOnly->isChecked();

    QString sql = QStringLiteral(
        "SELECT m.id, m.artist, m.song, m.country FROM musics m");
    if (unmarkedOnly)
        sql += QStringLiteral(" LEFT JOIN music_quota q ON q.music_id = m.id");
    QStringList where{ QStringLiteral("m.id IS NOT NULL") };
    if (unmarkedOnly)
        where << QStringLiteral("(q.music_id IS NULL OR q.origin = 'unset')");
    if (!needle.isEmpty())
        where << QStringLiteral("(m.artist LIKE :q OR m.song LIKE :q)");
    sql += QStringLiteral(" WHERE ") + where.join(QStringLiteral(" AND "));
    sql += QStringLiteral(" ORDER BY m.artist, m.song LIMIT :limit");

    QSqlQuery qry(db);
    qry.prepare(sql);
    if (!needle.isEmpty())
        qry.bindValue(QStringLiteral(":q"),
                      QLatin1Char('%') + needle + QLatin1Char('%'));
    qry.bindValue(QStringLiteral(":limit"), kTrackListLimit);
    if (!qry.exec()) {
        m_trackSummaryBase = tr("Could not read the library: %1")
                                 .arg(qry.lastError().text());
        updateSelectionSummary();
        return;
    }

    struct Row { qint64 id; QString artist; QString title; QString country; };
    QList<Row> rows;
    QList<qint64> ids;
    while (qry.next()) {
        Row r{ qry.value(0).toLongLong(), qry.value(1).toString(),
               qry.value(2).toString(), qry.value(3).toString() };
        rows.append(r);
        ids.append(r.id);
    }
    const QHash<qint64, MusicQuota::Mark> marks = MusicQuota::marksFor(ids);

    const bool wasSorting = m_tracks->isSortingEnabled();
    m_tracks->setSortingEnabled(false);
    m_tracks->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const Row &r = rows.at(i);
        const MusicQuota::Mark mark = marks.value(r.id);

        auto *artistItem = new QTableWidgetItem(r.artist);
        artistItem->setData(Qt::UserRole, QVariant(r.id));
        m_tracks->setItem(i, ColArtist, artistItem);
        m_tracks->setItem(i, ColTitle, new QTableWidgetItem(r.title));
        m_tracks->setItem(i, ColCountry, new QTableWidgetItem(r.country));
        m_tracks->setItem(i, ColOrigin,
                          new QTableWidgetItem(mark.stored && !mark.isMusic
                                                   ? tr("Not musical programming")
                                                   : MusicQuota::originLabel(mark.origin)));
        m_tracks->setItem(i, ColLanguage,
                          new QTableWidgetItem(
                              MusicQuota::languageLabel(mark.language, m_settings)));
        QString performer = tr("Not said");
        if (mark.performerEu == 1)
            performer = tr("EU citizens");
        else if (mark.performerEu == 0)
            performer = tr("Not EU citizens");
        m_tracks->setItem(i, ColPerformer, new QTableWidgetItem(performer));
        m_tracks->setItem(i, ColReleased,
                          new QTableWidgetItem(
                              mark.releaseDate.isValid()
                                  ? mark.releaseDate.toString(QStringLiteral("yyyy-MM-dd"))
                                  : QStringLiteral("—")));

        // A screen reader reading one cell should still know which track it is,
        // and whether anything has been said about it at all.
        for (int c = 0; c < ColCount; ++c) {
            if (QTableWidgetItem *item = m_tracks->item(i, c)) {
                item->setToolTip(tr("%1 — %2").arg(r.artist, r.title));
                item->setData(Qt::AccessibleDescriptionRole,
                              MusicQuota::describeMark(mark, m_settings));
            }
        }
    }
    m_tracks->setSortingEnabled(wasSorting);
    fitColumns(m_tracks);

    const int library = MusicQuota::libraryTrackCount();
    const int marked  = MusicQuota::markedTrackCount();
    QString summary = tr("%n track(s) listed.", nullptr, rows.size());
    if (rows.size() >= kTrackListLimit)
        summary += QLatin1Char(' ')
                   + tr("That is as many as this list shows at once — search to "
                        "reach the rest.");
    summary += QLatin1Char(' ')
               + tr("%1 of the %2 tracks in the library carry a mark (%3).")
                     .arg(marked)
                     .arg(library)
                     .arg(MusicQuota::percentText(library > 0 ? marked * 100.0 / library
                                                              : 0.0));
    m_trackSummaryBase = summary;
    updateSelectionSummary();
}

QList<qint64> QuotaDialog::selectedMusicIds() const
{
    QList<qint64> ids;
    if (!m_tracks || !m_tracks->selectionModel())
        return ids;
    const QModelIndexList rows = m_tracks->selectionModel()->selectedRows(ColArtist);
    for (const QModelIndex &index : rows) {
        const qint64 id = index.data(Qt::UserRole).toLongLong();
        if (id > 0 && !ids.contains(id))
            ids.append(id);
    }
    return ids;
}

void QuotaDialog::selectMusicIds(const QList<qint64> &ids)
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

void QuotaDialog::updateSelectionSummary()
{
    // Composed rather than appended: the label carries three things that
    // change at different moments, and appending would make a reload repeat
    // whichever of them it did not write.
    if (!m_trackSummary)
        return;
    QStringList parts;
    if (!m_trackStatus.isEmpty())
        parts << m_trackStatus;
    if (!m_trackSummaryBase.isEmpty())
        parts << m_trackSummaryBase;
    const int count = selectedMusicIds().size();
    if (count > 0)
        parts << tr("Selected: %n track(s).", nullptr, count);
    m_trackSummary->setText(parts.join(QLatin1Char(' ')));
}

void QuotaDialog::applyToSelection()
{
    const QList<qint64> ids = selectedMusicIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, tr("Music Quota"),
                                 tr("Select the tracks to mark first."));
        return;
    }
    if (!m_setOrigin->isChecked() && !m_setLanguage->isChecked()
        && !m_setPerformer->isChecked() && !m_setRelease->isChecked()
        && !m_setIsMusic->isChecked()) {
        QMessageBox::information(this, tr("Music Quota"),
                                 tr("Tick at least one line to write."));
        return;
    }

    const QHash<qint64, MusicQuota::Mark> existing = MusicQuota::marksFor(ids);
    QSqlDatabase db = editorDb();
    db.transaction();
    int written = 0;
    for (qint64 id : ids) {
        // Start from what the track already had, so an unticked line is kept
        // rather than reset — this is the whole point of the tick boxes.
        MusicQuota::Mark mark = existing.value(id);
        mark.musicId = id;
        if (m_setOrigin->isChecked())
            mark.origin = MusicQuota::Origin(m_origin->currentData().toInt());
        if (m_setLanguage->isChecked())
            mark.language = MusicQuota::Language(m_language->currentData().toInt());
        if (m_setPerformer->isChecked())
            mark.performerEu = m_performer->currentData().toInt();
        if (m_setRelease->isChecked())
            mark.releaseDate = m_release->date();
        if (m_setIsMusic->isChecked())
            mark.isMusic = m_isMusic->currentData().toInt() != 0;
        if (MusicQuota::saveMark(mark))
            ++written;
    }
    db.commit();

    m_trackStatus = tr("Marked %n track(s).", nullptr, written);
    reloadTracks();
    selectMusicIds(ids);
    emit announcementRequested(m_trackStatus);
}

void QuotaDialog::clearSelection()
{
    const QList<qint64> ids = selectedMusicIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, tr("Music Quota"),
                                 tr("Select the tracks to unmark first."));
        return;
    }
    // Only the ones that actually carry a mark, so the question and the count
    // afterwards are about the rows that change rather than about the size of
    // the selection.
    const QHash<qint64, MusicQuota::Mark> existing = MusicQuota::marksFor(ids);
    if (existing.isEmpty()) {
        QMessageBox::information(this, tr("Music Quota"),
                                 tr("None of the selected tracks carries a mark."));
        return;
    }
    if (QMessageBox::question(
            this, tr("Music Quota"),
            tr("Remove the quota marks from %n track(s)? They go back to "
               "being unmarked, which is not the same as international.",
               nullptr, existing.size()))
        != QMessageBox::Yes)
        return;

    QSqlDatabase db = editorDb();
    db.transaction();
    int removed = 0;
    for (qint64 id : ids) {
        if (existing.contains(id) && MusicQuota::clearMark(id))
            ++removed;
    }
    db.commit();

    m_trackStatus = tr("Removed the marks from %n track(s).", nullptr, removed);
    reloadTracks();
    selectMusicIds(ids);
    emit announcementRequested(m_trackStatus);
}

void QuotaDialog::seedFromLibrary()
{
    const MusicQuota::Settings s = readSettingsFromForm();
    if (s.countryName.trimmed().isEmpty()) {
        QMessageBox::information(
            this, tr("Music Quota"),
            tr("Set the country the station broadcasts from on the Rules tab "
               "first — that is the name this matches against."));
        return;
    }
    if (QMessageBox::question(
            this, tr("Fill in from the Country column"),
            tr("Every track that carries no mark yet and whose Country column "
               "actually names a country will be marked national or "
               "international according to whether that country is \"%1\", and "
               "given the first publication date the library holds for it."
               "\n\nTracks whose country is empty or is the downloader's "
               "\"Other country / language\" are left unmarked: that is a "
               "placeholder, not an answer. Nothing already marked is touched, "
               "and the language and the performers' citizenship are never "
               "guessed at — the library does not hold them."
               "\n\nGo ahead?").arg(s.countryName.trimmed()))
        != QMessageBox::Yes)
        return;

    const int written = MusicQuota::seedFromLibrary(s);
    reloadTracks();
    const QString message =
        written > 0 ? tr("Gave a first mark to %n track(s).", nullptr, written)
                    : tr("Nothing to fill in: every track either carries a mark "
                         "already or has no country recorded against it.");
    QMessageBox::information(this, tr("Fill in from the Country column"), message);
    emit announcementRequested(message);
}

// -------------------------------------------------------- the report tab --

QWidget *QuotaDialog::buildReportTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("Built from the as-run log — what actually went to air, not what "
           "was scheduled. The law counts the month as a whole, and counts "
           "the weekday daytime inside it again on its own, so both are "
           "shown."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *filterBox = new QGroupBox(tr("Period"), page);
    filterBox->setAccessibleName(tr("Report period"));
    auto *filterRow = new QHBoxLayout(filterBox);

    auto *fromLabel = new QLabel(tr("F&rom:"), filterBox);
    m_from = new QDateEdit(QDate::currentDate().addDays(-30), filterBox);
    m_from->setCalendarPopup(true);
    m_from->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_from->setAccessibleName(tr("First day of the period"));
    fromLabel->setBuddy(m_from);

    auto *toLabel = new QLabel(tr("&to:"), filterBox);
    m_to = new QDateEdit(QDate::currentDate(), filterBox);
    m_to->setCalendarPopup(true);
    m_to->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_to->setAccessibleName(tr("Last day of the period, counted in full"));
    toLabel->setBuddy(m_to);

    auto *thisMonth = new QPushButton(tr("T&his month"), filterBox);
    thisMonth->setAccessibleName(tr("Set the period to the current month"));
    connect(thisMonth, &QPushButton::clicked, this, &QuotaDialog::showThisMonth);
    auto *lastMonth = new QPushButton(tr("&Last month"), filterBox);
    lastMonth->setAccessibleName(tr("Set the period to the previous month"));
    connect(lastMonth, &QPushButton::clicked, this, &QuotaDialog::showLastMonth);

    auto *basisLabel = new QLabel(tr("&Measured by:"), filterBox);
    m_basis = new QComboBox(filterBox);
    m_basis->addItem(tr("Airtime"), true);
    m_basis->addItem(tr("Number of records"), false);
    m_basis->setAccessibleName(tr("Whether the percentages are of airtime or of "
                                  "the number of records"));
    basisLabel->setBuddy(m_basis);
    connect(m_basis, &QComboBox::currentIndexChanged, this, &QuotaDialog::buildReport);

    auto *build = new QPushButton(tr("&Build the report"), filterBox);
    build->setAccessibleName(tr("Measure the period against the station's quota"));
    build->setDefault(true);
    connect(build, &QPushButton::clicked, this, &QuotaDialog::buildReport);

    m_export = new QPushButton(tr("&Export as CSV..."), filterBox);
    m_export->setAccessibleName(tr("Save every record in the period as a CSV file"));
    m_export->setEnabled(false);
    connect(m_export, &QPushButton::clicked, this, &QuotaDialog::exportReportCsv);

    filterRow->addWidget(fromLabel);
    filterRow->addWidget(m_from);
    filterRow->addWidget(toLabel);
    filterRow->addWidget(m_to);
    filterRow->addWidget(thisMonth);
    filterRow->addWidget(lastMonth);
    filterRow->addSpacing(12);
    filterRow->addWidget(basisLabel);
    filterRow->addWidget(m_basis);
    filterRow->addStretch(1);
    filterRow->addWidget(build);
    filterRow->addWidget(m_export);
    layout->addWidget(filterBox);

    m_summary = new QTextBrowser(page);
    m_summary->setAccessibleName(tr("Quota figures for the period"));
    m_summary->setOpenExternalLinks(false);
    m_summary->setMinimumHeight(240);
    layout->addWidget(m_summary, 2);

    m_detail = new QTableWidget(page);
    m_detail->setColumnCount(DetCount);
    m_detail->setHorizontalHeaderLabels({ tr("Started"), tr("Artist"), tr("Title"),
                                          tr("Length"), tr("Weekday daytime"),
                                          tr("Origin"), tr("Counts towards") });
    m_detail->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detail->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detail->setAlternatingRowColors(true);
    m_detail->setSortingEnabled(true);
    m_detail->verticalHeader()->setVisible(false);
    m_detail->horizontalHeader()->setStretchLastSection(true);
    m_detail->setAccessibleName(tr("Every music record that went to air in the period"));
    layout->addWidget(m_detail, 3);

    return page;
}

void QuotaDialog::showThisMonth()
{
    const QDate today = QDate::currentDate();
    m_from->setDate(QDate(today.year(), today.month(), 1));
    m_to->setDate(today);
    buildReport();
}

void QuotaDialog::showLastMonth()
{
    const QDate first = QDate(QDate::currentDate().year(),
                              QDate::currentDate().month(), 1).addMonths(-1);
    m_from->setDate(first);
    m_to->setDate(first.addMonths(1).addDays(-1));
    buildReport();
}

void QuotaDialog::buildReport()
{
    if (!m_from || !m_to || !m_summary)
        return;
    if (m_from->date() > m_to->date()) {
        m_summary->setHtml(tr("<p>The period starts after it ends.</p>"));
        m_export->setEnabled(false);
        return;
    }

    MusicQuota::Settings s = m_settings;
    s.byDuration = m_basis->currentData().toBool();
    // The basis is a property of the station's reporting, not of one run of
    // the window, so a change to it sticks.
    if (s.byDuration != m_settings.byDuration) {
        m_settings.byDuration = s.byDuration;
        MusicQuota::saveSettings(m_settings);
    }

    // The end date is counted in full: an operator asking for "to the 31st"
    // means the 31st, not midnight at the start of it.
    m_report = MusicQuota::build(QDateTime(m_from->date(), QTime(0, 0)),
                                 QDateTime(m_to->date().addDays(1), QTime(0, 0)), s);
    m_summary->setHtml(renderReport(m_report));
    m_export->setEnabled(!m_report.entries.isEmpty());

    // --- the detail table ---
    const bool wasSorting = m_detail->isSortingEnabled();
    m_detail->setSortingEnabled(false);
    const int shown = qMin(m_report.entries.size(), kDetailLimit);
    m_detail->setRowCount(shown);
    for (int i = 0; i < shown; ++i) {
        const MusicQuota::Entry &entry = m_report.entries.at(i);
        m_detail->setItem(i, DetWhen,
                          new QTableWidgetItem(
                              entry.when.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
        m_detail->setItem(i, DetArtist, new QTableWidgetItem(entry.artist));
        m_detail->setItem(i, DetTitle, new QTableWidgetItem(entry.title));
        m_detail->setItem(i, DetLength, new QTableWidgetItem(lengthText(entry.ms)));
        m_detail->setItem(i, DetPrime,
                          new QTableWidgetItem(entry.prime ? tr("Yes") : tr("No")));
        m_detail->setItem(i, DetOrigin,
                          new QTableWidgetItem(
                              entry.unidentified
                                  ? tr("Outside the library")
                                  : MusicQuota::originLabel(entry.mark.origin)));

        QStringList counts;
        if (entry.national)
            counts << tr("the quota");
        if (entry.inLanguageQuota)
            counts << tr("the language share");
        if (entry.recent)
            counts << tr("the recent share");
        if (counts.isEmpty())
            counts << (entry.mark.isUnmarked() ? tr("nothing — not marked")
                                               : tr("nothing"));
        m_detail->setItem(i, DetCounts,
                          new QTableWidgetItem(counts.join(QStringLiteral(", "))));

        for (int c = 0; c < DetCount; ++c) {
            if (QTableWidgetItem *item = m_detail->item(i, c))
                item->setToolTip(tr("%1 — %2").arg(entry.artist, entry.title));
        }
    }
    m_detail->setSortingEnabled(wasSorting);
    fitColumns(m_detail);

    const MusicQuota::Slice &all = m_report.overall;
    emit announcementRequested(
        tr("Quota report built. %1 of the musical programming was music from %2.")
            .arg(MusicQuota::percentText(all.percentOfMusic(all.national, s.byDuration)),
                 s.countryName));
}

QString QuotaDialog::renderSlice(const QString &heading,
                                 const MusicQuota::Slice &slice,
                                 const MusicQuota::Settings &s) const
{
    QString html = QStringLiteral("<h3>%1</h3>").arg(escape(heading));

    if (slice.music.isEmpty()) {
        html += QStringLiteral("<p>%1</p>")
                    .arg(escape(tr("No music went to air in this window.")));
        return html;
    }

    html += QStringLiteral("<p>%1</p>")
                .arg(escape(tr("%n record(s), %1 of airtime.", nullptr,
                               slice.music.tracks)
                                .arg(MusicQuota::airtimeText(slice.music.ms))));

    html += QStringLiteral("<table cellpadding='4' cellspacing='0' border='0'>");
    html += QStringLiteral("<tr><th align='left'>%1</th><th align='right'>%2</th>"
                           "<th align='right'>%3</th><th align='left'>%4</th></tr>")
                .arg(escape(tr("Requirement")), escape(tr("Required")),
                     escape(tr("Aired")), escape(tr("Result")));

    const QList<MusicQuota::Requirement> requirements =
        MusicQuota::requirementsFor(slice, s);
    for (const MusicQuota::Requirement &req : requirements) {
        QString verdict;
        if (!req.measurable)
            verdict = tr("nothing to measure");
        else if (req.met)
            verdict = tr("met");
        else
            verdict = tr("SHORT by %1")
                          .arg(MusicQuota::percentText(req.required - req.achieved));
        html += QStringLiteral("<tr><td>%1<br><small>%2</small></td>"
                               "<td align='right'>%3</td><td align='right'>%4</td>"
                               "<td>%5</td></tr>")
                    .arg(escape(req.label), escape(req.basis),
                         escape(MusicQuota::percentText(req.required)),
                         escape(req.measurable ? MusicQuota::percentText(req.achieved)
                                               : QStringLiteral("—")),
                         escape(verdict));
    }
    html += QStringLiteral("</table>");

    // What the figures cannot see. Said plainly rather than left for somebody
    // to work out from a number that looks wrong.
    QStringList caveats;
    if (!slice.unmarked.isEmpty()) {
        caveats << tr("%1 of the window is music nothing has been said about "
                      "(%n record(s)). It counts in the total but towards no "
                      "quota, so the figures above are a floor, not the truth.",
                      nullptr, slice.unmarked.tracks)
                       .arg(MusicQuota::percentText(
                           slice.percentOfMusic(slice.unmarked, s.byDuration)));
    }
    if (!slice.unidentified.isEmpty()) {
        caveats << tr("%n of those record(s) were played from outside the "
                      "library, so there is nothing to mark them on.",
                      nullptr, slice.unidentified.tracks);
    }
    if (!slice.incomplete.isEmpty()) {
        caveats << tr("%n record(s) counted as national are missing a language "
                      "or a publication date, so they cannot reach either "
                      "sub-quota until that is filled in.",
                      nullptr, slice.incomplete.tracks);
    }
    if (s.byDuration && slice.withoutLength > 0) {
        caveats << tr("%n record(s) aired with no length recorded and so "
                      "contributed no airtime. Measuring by the number of "
                      "records instead avoids that.",
                      nullptr, slice.withoutLength);
    }
    for (const QString &caveat : caveats)
        html += QStringLiteral("<p><small>%1</small></p>").arg(escape(caveat));

    return html;
}

QString QuotaDialog::renderReport(const MusicQuota::Report &report) const
{
    if (!report.databaseOpen)
        return tr("<p>The library database is not open.</p>");

    const MusicQuota::Settings &s = report.settings;
    const QLocale locale;
    QString html = QStringLiteral("<h2>%1</h2>")
                       .arg(escape(tr("%1 to %2")
                                       .arg(locale.toString(report.from.date(),
                                                            QLocale::LongFormat),
                                            locale.toString(report.to.date().addDays(-1),
                                                            QLocale::LongFormat))));
    html += QStringLiteral("<p>%1</p>")
                .arg(escape(s.byDuration
                                ? tr("Percentages are of airtime.")
                                : tr("Percentages are of the number of records.")));

    if (!s.enabled) {
        html += QStringLiteral("<p><b>%1</b></p>")
                    .arg(escape(tr("The quota is switched off on the Rules tab. "
                                   "The figures below are still measured; "
                                   "nothing in XFB acts on them.")));
    }

    html += renderSlice(tr("The whole period"), report.overall, s);
    html += renderSlice(tr("Weekday daytime (%1)").arg(MusicQuota::describePrimeTime(s)),
                        report.prime, s);

    if (!report.excluded.isEmpty()) {
        html += QStringLiteral("<p><small>%1</small></p>")
                    .arg(escape(tr("%n record(s) marked as not being musical "
                                   "programming were left out of both sides of "
                                   "every fraction.", nullptr,
                                   report.excluded.tracks)));
    }

    html += QStringLiteral("<p><small>%1</small></p>")
                .arg(escape(tr("%1 of the %2 tracks in the library carry a mark.")
                                .arg(report.markedTracks)
                                .arg(report.libraryTracks)));

    if (!s.exemptionNote.trimmed().isEmpty()) {
        html += QStringLiteral("<h3>%1</h3><p>%2</p>")
                    .arg(escape(tr("Exemption on record")),
                         escape(s.exemptionNote.trimmed()));
    }

    html += QStringLiteral("<p><small>%1</small></p>")
                .arg(escape(tr("XFB counts what its as-run log recorded, using "
                               "the percentages set on the Rules tab. What the "
                               "regulator requires of this station, and how it "
                               "wants it counted, is the station's own to "
                               "confirm.")));
    return html;
}

void QuotaDialog::exportReportCsv()
{
    if (m_report.entries.isEmpty())
        return;

    const MusicQuota::Settings &s = m_report.settings;
    const MusicQuota::Slice &all = m_report.overall;
    const MusicQuota::Slice &prime = m_report.prime;

    QStringList preamble;
    preamble << tr("XFB music quota report")
             << tr("Period: %1 to %2")
                    .arg(m_report.from.date().toString(Qt::ISODate),
                         m_report.to.date().addDays(-1).toString(Qt::ISODate))
             << tr("Measured by: %1").arg(s.byDuration ? tr("airtime")
                                                       : tr("number of records"))
             << tr("Weekday daytime window: %1").arg(MusicQuota::describePrimeTime(s));
    const QList<MusicQuota::Requirement> overallReqs =
        MusicQuota::requirementsFor(all, s);
    const QList<MusicQuota::Requirement> primeReqs =
        MusicQuota::requirementsFor(prime, s);
    for (int i = 0; i < overallReqs.size(); ++i) {
        preamble << tr("%1 — required %2, whole period %3, weekday daytime %4")
                        .arg(overallReqs.at(i).label,
                             MusicQuota::percentText(overallReqs.at(i).required),
                             MusicQuota::percentText(overallReqs.at(i).achieved),
                             MusicQuota::percentText(primeReqs.at(i).achieved));
    }
    if (!all.unmarked.isEmpty()) {
        preamble << tr("Unmarked music in the period: %1")
                        .arg(MusicQuota::percentText(
                            all.percentOfMusic(all.unmarked, s.byDuration)));
    }
    if (!s.exemptionNote.trimmed().isEmpty())
        preamble << tr("Exemption on record: %1").arg(s.exemptionNote.trimmed());

    const QStringList header{ tr("Started"), tr("Artist"), tr("Title"),
                              tr("Length (s)"), tr("Weekday daytime"),
                              tr("Origin"), tr("Language"), tr("Performers"),
                              tr("First published"), tr("Counts towards the quota"),
                              tr("Counts towards the language share"),
                              tr("Counts towards the recent share") };
    QList<QStringList> rows;
    rows.reserve(m_report.entries.size());
    for (const MusicQuota::Entry &entry : m_report.entries) {
        QString performer = tr("not said");
        if (entry.mark.performerEu == 1)
            performer = tr("EU citizens");
        else if (entry.mark.performerEu == 0)
            performer = tr("not EU citizens");
        rows << QStringList{
            entry.when.toString(Qt::ISODate),
            entry.artist,
            entry.title,
            QString::number(entry.ms / 1000),
            entry.prime ? tr("yes") : tr("no"),
            entry.unidentified ? tr("outside the library")
                               : MusicQuota::originLabel(entry.mark.origin),
            MusicQuota::languageLabel(entry.mark.language, s),
            performer,
            entry.mark.releaseDate.isValid()
                ? entry.mark.releaseDate.toString(Qt::ISODate) : QString(),
            entry.national ? tr("yes") : tr("no"),
            entry.inLanguageQuota ? tr("yes") : tr("no"),
            entry.recent ? tr("yes") : tr("no")
        };
    }

    const QString suggested =
        tr("quota-%1.csv").arg(m_report.from.date().toString(QStringLiteral("yyyy-MM")));
    if (writeCsv(suggested, preamble, header, rows))
        emit announcementRequested(tr("Quota report exported."));
}

bool QuotaDialog::writeCsv(const QString &suggestedName, const QStringList &preamble,
                           const QStringList &header, const QList<QStringList> &rows)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export as CSV"),
        dir.isEmpty() ? suggestedName : dir + QLatin1Char('/') + suggestedName,
        tr("Comma-separated values (*.csv)"));
    if (path.isEmpty())
        return false;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export as CSV"),
                             tr("Could not write %1: %2").arg(path, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    // A BOM: without it Excel opens accented artist names as mojibake, and
    // this file is going to a regulator.
    out.setGenerateByteOrderMark(true);

    for (const QString &line : preamble)
        out << csvField(line) << "\n";
    if (!preamble.isEmpty())
        out << "\n";

    QStringList headerFields;
    for (const QString &cell : header)
        headerFields << csvField(cell);
    out << headerFields.join(QLatin1Char(',')) << "\n";

    for (const QStringList &row : rows) {
        QStringList fields;
        for (const QString &cell : row)
            fields << csvField(cell);
        out << fields.join(QLatin1Char(',')) << "\n";
    }
    out.flush();

    if (!file.commit()) {
        QMessageBox::warning(this, tr("Export as CSV"),
                             tr("Could not finish writing %1: %2")
                                 .arg(path, file.errorString()));
        return false;
    }
    return true;
}

// --------------------------------------------------------- the rules tab --

QWidget *QuotaDialog::buildRulesTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("XFB starts with the Portuguese figures — the Lei da Rádio as "
           "amended by Lei n.º 16/2024: 30% of the musical programming from "
           "Portugal, 60% of that sung in Portuguese by performers who are EU "
           "citizens, 35% of it first published in the last twelve months, "
           "counted over the month and again over 07:00 to 20:00 on weekdays. "
           "Every one of those numbers is editable, because the obligation is "
           "not the same in the next country and not the same next year. What "
           "this station owes, and any exemption it holds, is the station's "
           "own to confirm — XFB only makes the answer countable."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *form = new QFormLayout;

    m_enabled = new QCheckBox(tr("This station is under a music &quota"), page);
    m_enabled->setAccessibleName(tr("Whether a national music quota applies here"));
    form->addRow(m_enabled);

    m_country = new QLineEdit(page);
    m_country->setAccessibleName(tr("The country whose music counts as national"));
    m_country->setToolTip(tr("Written exactly as the library's Country column "
                             "spells it, so filling in from that column can "
                             "match it."));
    form->addRow(tr("Music is national when it comes &from:"), m_country);

    m_languageName = new QLineEdit(page);
    m_languageName->setAccessibleName(tr("The language the sub-quota is about"));
    form->addRow(tr("The language of the s&ub-quota:"), m_languageName);

    m_nationalPercent = new QDoubleSpinBox(page);
    m_nationalPercent->setRange(0.0, 100.0);
    m_nationalPercent->setDecimals(1);
    m_nationalPercent->setSuffix(QStringLiteral(" %"));
    m_nationalPercent->setAccessibleName(
        tr("Least national music, as a percentage of the musical programming"));
    form->addRow(tr("&National music, at least:"), m_nationalPercent);

    m_languageShare = new QDoubleSpinBox(page);
    m_languageShare->setRange(0.0, 100.0);
    m_languageShare->setDecimals(1);
    m_languageShare->setSuffix(QStringLiteral(" %"));
    m_languageShare->setAccessibleName(
        tr("Least of the national quota sung in the national language"));
    form->addRow(tr("Of that, sung in the &language, at least:"), m_languageShare);

    m_requireEu = new QCheckBox(
        tr("...and performed by citizens of an &EU Member State"), page);
    m_requireEu->setAccessibleName(
        tr("Whether the language share also requires EU citizenship"));
    form->addRow(QString(), m_requireEu);

    m_recentShare = new QDoubleSpinBox(page);
    m_recentShare->setRange(0.0, 100.0);
    m_recentShare->setDecimals(1);
    m_recentShare->setSuffix(QStringLiteral(" %"));
    m_recentShare->setAccessibleName(
        tr("Least of the national quota that must be recent"));
    form->addRow(tr("Of that, recently published, at least:"), m_recentShare);

    m_recentMonths = new QSpinBox(page);
    m_recentMonths->setRange(1, 120);
    m_recentMonths->setSuffix(tr(" months"));
    m_recentMonths->setAccessibleName(
        tr("How long after publication a record still counts as recent"));
    form->addRow(tr("\"Recently\" &means published within:"), m_recentMonths);

    layout->addLayout(form);

    // --- the second window the law measures on its own ---
    auto *primeBox = new QGroupBox(tr("The daytime window measured separately"), page);
    primeBox->setAccessibleName(tr("Prime-time window"));
    auto *primeLayout = new QVBoxLayout(primeBox);

    auto *hourRow = new QHBoxLayout;
    m_primeStart = new QSpinBox(primeBox);
    m_primeStart->setRange(0, 23);
    m_primeStart->setSuffix(tr(":00"));
    m_primeStart->setAccessibleName(tr("Hour the window starts"));
    m_primeEnd = new QSpinBox(primeBox);
    m_primeEnd->setRange(1, 24);
    m_primeEnd->setSuffix(tr(":00"));
    m_primeEnd->setAccessibleName(tr("Hour the window ends"));
    hourRow->addWidget(new QLabel(tr("From"), primeBox));
    hourRow->addWidget(m_primeStart);
    hourRow->addWidget(new QLabel(tr("to"), primeBox));
    hourRow->addWidget(m_primeEnd);
    hourRow->addStretch(1);
    primeLayout->addLayout(hourRow);

    auto *dayRow = new QHBoxLayout;
    const QLocale locale;
    for (int d = 1; d <= 7; ++d) {
        m_primeDay[d - 1] = new QCheckBox(locale.dayName(d, QLocale::ShortFormat),
                                          primeBox);
        m_primeDay[d - 1]->setAccessibleName(
            tr("Include %1 in the window").arg(locale.dayName(d)));
        dayRow->addWidget(m_primeDay[d - 1]);
    }
    dayRow->addStretch(1);
    primeLayout->addLayout(dayRow);
    layout->addWidget(primeBox);

    m_includeUnlibraried = new QCheckBox(
        tr("Count music played from &outside the library"), page);
    m_includeUnlibraried->setAccessibleName(
        tr("Whether files played without being in the library count in the total"));
    m_includeUnlibraried->setToolTip(
        tr("A file opened straight into the player has nothing to look up, so "
           "it can only ever count as unmarked. Leaving it in keeps the total "
           "honest; taking it out measures only the library."));
    layout->addWidget(m_includeUnlibraried);

    auto *exemptionLabel = new QLabel(tr("E&xemption on record:"), page);
    m_exemption = new QPlainTextEdit(page);
    m_exemption->setPlaceholderText(
        tr("Anything the regulator has granted this station. Printed on the "
           "report; it never changes a figure."));
    m_exemption->setMaximumHeight(70);
    m_exemption->setAccessibleName(tr("The exemption this station holds, if any"));
    exemptionLabel->setBuddy(m_exemption);
    layout->addWidget(exemptionLabel);
    layout->addWidget(m_exemption);

    auto *buttonRow = new QHBoxLayout;
    auto *save = new QPushButton(tr("&Save the rules"), page);
    save->setAccessibleName(tr("Save these quota rules"));
    connect(save, &QPushButton::clicked, this, &QuotaDialog::applySettings);
    auto *preset = new QPushButton(tr("&Portuguese defaults"), page);
    preset->setAccessibleName(tr("Put the Portuguese figures back"));
    connect(preset, &QPushButton::clicked, this, &QuotaDialog::restorePortugalPreset);
    buttonRow->addWidget(save);
    buttonRow->addStretch(1);
    buttonRow->addWidget(preset);
    layout->addLayout(buttonRow);

    m_rulesStatus = new QLabel(page);
    m_rulesStatus->setWordWrap(true);
    m_rulesStatus->setAccessibleName(tr("Rules status"));
    layout->addWidget(m_rulesStatus);

    layout->addStretch(1);
    return page;
}

void QuotaDialog::loadSettingsInto(const MusicQuota::Settings &s)
{
    m_enabled->setChecked(s.enabled);
    m_country->setText(s.countryName);
    m_languageName->setText(s.languageName);
    m_nationalPercent->setValue(s.nationalPercent);
    m_languageShare->setValue(s.languageSharePercent);
    m_requireEu->setChecked(s.requireEuPerformer);
    m_recentShare->setValue(s.recentSharePercent);
    m_recentMonths->setValue(s.recentMonths);
    m_primeStart->setValue(s.primeStartHour);
    m_primeEnd->setValue(s.primeEndHour);
    for (int d = 0; d < 7; ++d)
        m_primeDay[d]->setChecked(s.primeDayMask & (1u << d));
    m_includeUnlibraried->setChecked(s.includeUnlibraried);
    m_exemption->setPlainText(s.exemptionNote);
    {   // Loading the form is not the operator changing the basis, and the
        // signal would rebuild the report in the middle of it.
        const QSignalBlocker blocker(m_basis);
        m_basis->setCurrentIndex(s.byDuration ? 0 : 1);
    }

    // The language names carry the station's own word for its language, so
    // they are built here rather than once at construction.
    if (m_language) {
        const int previous = m_language->currentIndex();
        m_language->clear();
        m_language->addItem(
            MusicQuota::languageLabel(MusicQuota::NationalLanguage, s),
            int(MusicQuota::NationalLanguage));
        m_language->addItem(MusicQuota::languageLabel(MusicQuota::OtherLanguage, s),
                            int(MusicQuota::OtherLanguage));
        m_language->addItem(MusicQuota::languageLabel(MusicQuota::Instrumental, s),
                            int(MusicQuota::Instrumental));
        m_language->addItem(MusicQuota::languageLabel(MusicQuota::LanguageUnset, s),
                            int(MusicQuota::LanguageUnset));
        if (previous >= 0 && previous < m_language->count())
            m_language->setCurrentIndex(previous);
    }
}

MusicQuota::Settings QuotaDialog::readSettingsFromForm() const
{
    MusicQuota::Settings s = m_settings;
    s.enabled              = m_enabled->isChecked();
    s.countryName          = m_country->text().trimmed();
    s.languageName         = m_languageName->text().trimmed();
    s.nationalPercent      = m_nationalPercent->value();
    s.languageSharePercent = m_languageShare->value();
    s.requireEuPerformer   = m_requireEu->isChecked();
    s.recentSharePercent   = m_recentShare->value();
    s.recentMonths         = m_recentMonths->value();
    s.primeStartHour       = m_primeStart->value();
    s.primeEndHour         = m_primeEnd->value();
    quint8 mask = 0;
    for (int d = 0; d < 7; ++d) {
        if (m_primeDay[d]->isChecked())
            mask |= quint8(1u << d);
    }
    s.primeDayMask         = mask;
    s.includeUnlibraried   = m_includeUnlibraried->isChecked();
    s.exemptionNote        = m_exemption->toPlainText().trimmed();
    s.byDuration           = m_basis->currentData().toBool();
    return s;
}

void QuotaDialog::applySettings()
{
    MusicQuota::Settings s = readSettingsFromForm();
    if (s.primeEndHour <= s.primeStartHour) {
        QMessageBox::warning(this, tr("Music Quota"),
                             tr("The daytime window ends before it starts."));
        return;
    }
    if (s.primeDayMask == 0) {
        QMessageBox::warning(this, tr("Music Quota"),
                             tr("Pick at least one weekday for the window "
                                "measured separately."));
        return;
    }
    MusicQuota::saveSettings(s);
    m_settings = MusicQuota::settings();
    loadSettingsInto(m_settings);
    m_rulesStatus->setText(tr("Saved. %1 of the musical programming from %2, "
                              "measured over the month and again over %3.")
                               .arg(MusicQuota::percentText(m_settings.nationalPercent),
                                    m_settings.countryName,
                                    MusicQuota::describePrimeTime(m_settings)));
    // The marking table prints the station's own language name, and the
    // report is measured with these numbers, so both are now out of date.
    reloadTracks();
    buildReport();
    emit announcementRequested(tr("Quota rules saved."));
}

void QuotaDialog::restorePortugalPreset()
{
    MusicQuota::Settings s = MusicQuota::portugalPreset();
    // The two things that are about this installation rather than about the
    // law are kept: whether the station is under a quota at all, and what it
    // has written down about its exemption.
    s.enabled = m_enabled->isChecked();
    s.exemptionNote = m_exemption->toPlainText().trimmed();
    loadSettingsInto(s);
    m_rulesStatus->setText(tr("The Portuguese figures are in the form. "
                              "Choose Save the rules to keep them."));
}
