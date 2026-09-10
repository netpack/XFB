#include "WatchedFoldersDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

// Column order of the folder table.
enum Column {
    ColEnabled = 0,
    ColPath,
    ColDestination,
    ColRecursive,
    ColFolderGenres,
    ColGenre,
    ColPending,
    ColumnCount,
};

const LibraryWatcher::Destination kDestinations[] = {
    LibraryWatcher::Destination::Music,
    LibraryWatcher::Destination::Jingles,
    LibraryWatcher::Destination::Publicity,
    LibraryWatcher::Destination::Programs,
};

} // namespace

WatchedFoldersDialog::WatchedFoldersDialog(LibraryWatcher *watcher, QWidget *parent)
    : QDialog(parent)
    , m_watcher(watcher)
{
    setWindowTitle(tr("Watched Folders"));
    setModal(false);   // an import is not a reason to stop working

    m_genres = knownGenres();

    buildUi();
    if (m_watcher) {
        loadInto(m_watcher->config());
        connect(m_watcher, &LibraryWatcher::stateChanged,
                this, &WatchedFoldersDialog::refreshStatus);
    }
    refreshStatus();
}

void WatchedFoldersDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Material does not usually arrive through a dialog: the production "
           "desk drops a jingle package on the studio share, the agency pushes "
           "an advert overnight, this week's programme is copied off a stick. "
           "A watched folder says once and for all where new files in it "
           "belong, and XFB files them without anybody having to remember."),
        this);
    intro->setWordWrap(true);
    outer->addWidget(intro);

    // ------------------------------------------------------------- the switch --
    auto *whenBox = new QGroupBox(tr("When to look"), this);
    auto *whenForm = new QFormLayout(whenBox);

    m_enabled = new QCheckBox(tr("Watch these folders"), whenBox);
    m_enabled->setAccessibleName(tr("Watch these folders"));
    whenForm->addRow(m_enabled);

    m_poll = new QSpinBox(whenBox);
    m_poll->setRange(10, 3600);
    m_poll->setSuffix(tr(" s"));
    m_poll->setAccessibleName(tr("Seconds between two looks at the folders"));
    m_poll->setToolTip(
        tr("XFB is told about changes to a local folder as they happen; a "
           "network share tells it nothing at all, so it looks again this "
           "often."));
    whenForm->addRow(tr("Look every:"), m_poll);

    m_quiet = new QSpinBox(whenBox);
    m_quiet->setRange(2, 600);
    m_quiet->setSuffix(tr(" s"));
    m_quiet->setAccessibleName(tr("Seconds a file must stop changing before it is imported"));
    m_quiet->setToolTip(
        tr("A file copied over the network appears at once and keeps growing. "
           "Imported at that moment it would be a fragment for ever, so it has "
           "to have stopped changing for this long first."));
    whenForm->addRow(tr("A file has landed after:"), m_quiet);

    outer->addWidget(whenBox);

    // ------------------------------------------------------------ the folders --
    auto *folderBox = new QGroupBox(tr("Folders"), this);
    auto *folderLayout = new QVBoxLayout(folderBox);

    m_table = new QTableWidget(0, ColumnCount, folderBox);
    m_table->setHorizontalHeaderLabels({
        tr("On"),
        tr("Folder"),
        tr("Adds to"),
        tr("Subfolders"),
        tr("Folder names the genre"),
        tr("Genre"),
        tr("Waiting"),
    });
    m_table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAccessibleName(tr("Watched folders"));
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        m_remove->setEnabled(m_table->currentRow() >= 0);
    });
    folderLayout->addWidget(m_table);

    auto *buttonRow = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add a folder..."), folderBox);
    connect(add, &QPushButton::clicked, this, &WatchedFoldersDialog::addFolder);
    m_remove = new QPushButton(tr("Remove"), folderBox);
    m_remove->setEnabled(false);
    connect(m_remove, &QPushButton::clicked, this, &WatchedFoldersDialog::removeFolder);
    auto *scan = new QPushButton(tr("Scan now"), folderBox);
    scan->setToolTip(tr("Save these settings and import whatever is waiting, "
                        "without waiting for the next look."));
    connect(scan, &QPushButton::clicked, this, &WatchedFoldersDialog::scanNow);
    auto *count = new QPushButton(tr("Count what is waiting"), folderBox);
    connect(count, &QPushButton::clicked, this, &WatchedFoldersDialog::refreshPending);
    buttonRow->addWidget(add);
    buttonRow->addWidget(m_remove);
    buttonRow->addStretch(1);
    buttonRow->addWidget(count);
    buttonRow->addWidget(scan);
    folderLayout->addLayout(buttonRow);

    auto *note = new QLabel(
        tr("Everything already in a folder that is not in the library yet is "
           "imported on the first pass, not only what arrives afterwards. "
           "Files are never moved, renamed or deleted, and a file that "
           "disappears from a watched folder keeps its library row — the "
           "record check in the Database menu is what clears those."),
        folderBox);
    note->setWordWrap(true);
    folderLayout->addWidget(note);

    outer->addWidget(folderBox, 1);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    outer->addWidget(m_status);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &WatchedFoldersDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    outer->addWidget(buttons);
}

QStringList WatchedFoldersDialog::knownGenres() const
{
    QStringList genres;
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("xfb_connection")));
    query.prepare(QStringLiteral("select name from genres1 order by name"));
    if (query.exec()) {
        while (query.next())
            genres << query.value(0).toString();
    }
    return genres;
}

void WatchedFoldersDialog::loadInto(const LibraryWatcher::Config &config)
{
    m_enabled->setChecked(config.enabled);
    m_poll->setValue(config.pollSeconds);
    m_quiet->setValue(config.quietSeconds);

    m_table->setRowCount(0);
    for (const LibraryWatcher::Folder &folder : config.folders)
        appendRow(folder);
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
}

namespace
{
// A checkbox dropped into a table cell sits hard against its left edge, under
// a header that is centred over the column — so the tick never lines up with
// the thing it belongs to. Wrapping it centres it, and the wrapper is
// transparent so the row's own background (selected or not) shows through.
QWidget *centred(QCheckBox *box)
{
    auto *host = new QWidget;
    host->setAttribute(Qt::WA_TranslucentBackground);
    auto *layout = new QHBoxLayout(host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(box, 0, Qt::AlignCenter);
    return host;
}

// ...which means the checkbox a cell holds is no longer the widget the table
// hands back. Everything that reads a row goes through here instead.
QCheckBox *boxIn(QWidget *cell)
{
    if (!cell)
        return nullptr;
    if (auto *box = qobject_cast<QCheckBox *>(cell))
        return box;
    return cell->findChild<QCheckBox *>();
}
} // namespace

void WatchedFoldersDialog::appendRow(const LibraryWatcher::Folder &folder)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto *on = new QTableWidgetItem;
    on->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    on->setCheckState(folder.enabled ? Qt::Checked : Qt::Unchecked);
    on->setToolTip(tr("Keep the folder in the list without watching it"));
    m_table->setItem(row, ColEnabled, on);

    auto *path = new QTableWidgetItem(folder.path);
    path->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    path->setToolTip(folder.path);
    m_table->setItem(row, ColPath, path);

    auto *destination = new QComboBox(m_table);
    for (const LibraryWatcher::Destination d : kDestinations)
        destination->addItem(LibraryWatcher::destinationLabel(d), int(d));
    destination->setCurrentIndex(destination->findData(int(folder.destination)));
    destination->setAccessibleName(tr("What this folder adds to"));
    connect(destination, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, destination]() { updateRowEnabling(rowOf(destination)); });
    m_table->setCellWidget(row, ColDestination, destination);

    auto *recursive = new QCheckBox(m_table);
    recursive->setChecked(folder.recursive);
    recursive->setAccessibleName(tr("Look in subfolders too"));
    connect(recursive, &QCheckBox::toggled, this,
            [this, recursive]() { updateRowEnabling(rowOf(recursive)); });
    m_table->setCellWidget(row, ColRecursive, centred(recursive));

    auto *folderGenres = new QCheckBox(m_table);
    folderGenres->setChecked(folder.folderGenres);
    folderGenres->setAccessibleName(tr("The first subfolder names the genre"));
    folderGenres->setToolTip(
        tr("Rock/Nirvana/Lithium.mp3 is filed under Rock. A track sitting "
           "loose in the watched folder gets the genre chosen here."));
    m_table->setCellWidget(row, ColFolderGenres, centred(folderGenres));

    auto *genre = new QComboBox(m_table);
    genre->addItems(m_genres);
    if (!folder.genre.isEmpty()) {
        const int index = genre->findText(folder.genre, Qt::MatchFixedString);
        if (index >= 0) {
            genre->setCurrentIndex(index);
        } else {
            // A genre that has since been renamed or deleted: keep what the
            // folder was configured with rather than silently refiling it.
            genre->insertItem(0, folder.genre);
            genre->setCurrentIndex(0);
        }
    }
    genre->setAccessibleName(tr("Genre for tracks the folders cannot name"));
    m_table->setCellWidget(row, ColGenre, genre);

    auto *pending = new QTableWidgetItem(QStringLiteral("—"));
    pending->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    pending->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_table->setItem(row, ColPending, pending);

    updateRowEnabling(row);
}

int WatchedFoldersDialog::rowOf(const QWidget *cellWidget) const
{
    // Never a captured row index: removing a folder shifts every row below it,
    // and a control that then edits the wrong row is a bug nobody would look
    // for. The widget itself always knows where it is.
    // Some cells hold the control inside a wrapper that centres it, so the
    // widget a signal came from may be a child of what the table knows about.
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int column = 0; column < m_table->columnCount(); ++column) {
            const QWidget *cell = m_table->cellWidget(row, column);
            if (!cell)
                continue;
            if (cell == cellWidget || cell->isAncestorOf(cellWidget))
                return row;
        }
    }
    return -1;
}

void WatchedFoldersDialog::updateRowEnabling(int row)
{
    if (row < 0)
        return;

    auto *destination = qobject_cast<QComboBox *>(m_table->cellWidget(row, ColDestination));
    auto *recursive = boxIn(m_table->cellWidget(row, ColRecursive));
    auto *folderGenres = boxIn(m_table->cellWidget(row, ColFolderGenres));
    auto *genre = qobject_cast<QComboBox *>(m_table->cellWidget(row, ColGenre));
    if (!destination || !recursive || !folderGenres || !genre)
        return;

    // Only the music library has a genre column; the other three store a name
    // and a path, so a genre choice on those rows would be a lie.
    const bool isMusic = destination->currentData().toInt()
                         == int(LibraryWatcher::Destination::Music);
    // Without descending there are no subfolder names to file by, which is the
    // same rule the folder importer applies.
    folderGenres->setEnabled(isMusic && recursive->isChecked());
    genre->setEnabled(isMusic);
}

LibraryWatcher::Folder WatchedFoldersDialog::folderFromRow(int row) const
{
    LibraryWatcher::Folder folder;
    if (const QTableWidgetItem *path = m_table->item(row, ColPath))
        folder.path = path->text();
    if (const QTableWidgetItem *on = m_table->item(row, ColEnabled))
        folder.enabled = on->checkState() == Qt::Checked;
    if (auto *destination = qobject_cast<QComboBox *>(m_table->cellWidget(row, ColDestination)))
        folder.destination = LibraryWatcher::Destination(destination->currentData().toInt());
    if (auto *recursive = boxIn(m_table->cellWidget(row, ColRecursive)))
        folder.recursive = recursive->isChecked();
    if (auto *folderGenres = boxIn(m_table->cellWidget(row, ColFolderGenres)))
        folder.folderGenres = folderGenres->isChecked();
    if (auto *genre = qobject_cast<QComboBox *>(m_table->cellWidget(row, ColGenre)))
        folder.genre = genre->currentText();
    return folder;
}

LibraryWatcher::Config WatchedFoldersDialog::readFromForm() const
{
    LibraryWatcher::Config config = m_watcher ? m_watcher->config()
                                              : LibraryWatcher::Config();
    config.enabled = m_enabled->isChecked();
    config.pollSeconds = m_poll->value();
    config.quietSeconds = m_quiet->value();
    config.folders.clear();
    for (int row = 0; row < m_table->rowCount(); ++row)
        config.folders << folderFromRow(row);
    return config;
}

void WatchedFoldersDialog::addFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("Choose a folder to watch"));
    if (path.isEmpty())
        return;

    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, ColPath)
            && m_table->item(row, ColPath)->text() == path) {
            m_table->selectRow(row);
            return;   // already listed; selecting it says so better than a box
        }
    }

    LibraryWatcher::Folder folder;
    folder.path = path;
    if (!m_genres.isEmpty())
        folder.genre = m_genres.first();
    appendRow(folder);
    m_table->selectRow(m_table->rowCount() - 1);
    m_table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
}

void WatchedFoldersDialog::removeFolder()
{
    const int row = m_table->currentRow();
    if (row < 0)
        return;
    m_table->removeRow(row);
    m_remove->setEnabled(m_table->currentRow() >= 0);
}

void WatchedFoldersDialog::apply()
{
    if (!m_watcher)
        return;
    m_watcher->setConfig(readFromForm());
    emit announcementRequested(
        m_enabled->isChecked()
            ? tr("Watching %n folder(s)", nullptr, m_table->rowCount())
            : tr("Watched folders turned off"));
    refreshStatus();
}

void WatchedFoldersDialog::scanNow()
{
    if (!m_watcher)
        return;
    // Scan what is on screen, not what was saved last time: pressing Scan now
    // after adding a folder and not noticing it was never saved is exactly the
    // kind of thing that makes a feature look broken.
    m_watcher->setConfig(readFromForm());

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const int added = m_watcher->scanNow();
    QApplication::restoreOverrideCursor();

    emit announcementRequested(added > 0
                                   ? tr("%n file(s) added to the library", nullptr, added)
                                   : tr("Nothing new to add"));
    refreshPending();
    refreshStatus();
}

void WatchedFoldersDialog::refreshPending()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const LibraryWatcher::Folder folder = folderFromRow(row);
        const int pending = LibraryWatcher::pendingCount(folder);
        QTableWidgetItem *item = m_table->item(row, ColPending);
        if (!item)
            continue;
        if (pending < 0) {
            item->setText(tr("not there"));
            item->setToolTip(tr("The folder cannot be reached — an unmounted "
                                "share, or a path that has moved."));
        } else {
            item->setText(QString::number(pending));
            item->setToolTip(tr("Files in this folder that are not in the "
                                "library yet."));
        }
    }
    QApplication::restoreOverrideCursor();
}

void WatchedFoldersDialog::refreshStatus()
{
    if (!m_watcher) {
        m_status->clear();
        return;
    }

    const LibraryWatcher::Config config = m_watcher->config();
    if (!config.enabled) {
        m_status->setText(tr("Not watching. Nothing is imported until this is "
                             "turned on and saved."));
        return;
    }

    const QDateTime last = m_watcher->lastScan();
    if (!last.isValid()) {
        m_status->setText(tr("Watching. Nothing looked at yet."));
        return;
    }

    m_status->setText(tr("Last look %1 — %2")
                          .arg(last.toString(QStringLiteral("HH:mm:ss")),
                               m_watcher->lastSummary()));
}
