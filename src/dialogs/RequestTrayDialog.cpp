#include "RequestTrayDialog.h"

#include "../services/MobileSyncServer.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

RequestTrayDialog::RequestTrayDialog(MobileSyncServer *server, QWidget *parent)
    : QDialog(parent)
    , m_server(server)
{
    setWindowTitle(tr("Listener Requests"));
    setModal(false);   // the operator keeps working while the station is on air

    buildUi();
    loadSettings();
    refresh();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(5000);
    connect(m_refreshTimer, &QTimer::timeout, this, &RequestTrayDialog::refresh);
    m_refreshTimer->start();
}

void RequestTrayDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);

    // ------------------------------------------------------- the public page --
    auto *pageBox = new QGroupBox(tr("The public page"), this);
    auto *pageLayout = new QVBoxLayout(pageBox);

    // The warning comes before the switch, not after it, and it is not a
    // tooltip: an operator who turns this on has to have read what it does.
    auto *warning = new QLabel(
        tr("Switching this on serves a web page to anybody who can reach this "
           "computer over the network — everyone on the studio Wi-Fi, and, if "
           "this machine is reachable from the internet or someone has "
           "forwarded a port to it, anybody at all.\n\n"
           "The page carries what is on air, what played in the last hour, "
           "and — if you allow it — a search of your titles and artists. It "
           "serves no audio and no files of any kind, and it cannot put "
           "anything on air. It is off until you switch it on here, and "
           "switching it off closes it again at once."), pageBox);
    warning->setWordWrap(true);
    warning->setStyleSheet(QStringLiteral("font-weight:600;"));
    warning->setAccessibleName(tr("What enabling the public page does"));
    pageLayout->addWidget(warning);

    m_pageEnabled = new QCheckBox(
        tr("Serve a public now-playing page on this network"), pageBox);
    m_pageEnabled->setAccessibleName(tr("Serve a public now-playing page"));
    connect(m_pageEnabled, &QCheckBox::toggled, this, &RequestTrayDialog::applySettings);
    pageLayout->addWidget(m_pageEnabled);

    m_requestsEnabled = new QCheckBox(
        tr("Let listeners search the library and ask for a track"), pageBox);
    m_requestsEnabled->setToolTip(
        tr("Listeners see titles and artists only. They never see a file, a "
           "folder or anything they could download, and what they ask for "
           "arrives in the list below — it does not go anywhere near the "
           "running order until you put it there."));
    m_requestsEnabled->setAccessibleName(tr("Accept listener requests"));
    connect(m_requestsEnabled, &QCheckBox::toggled, this, &RequestTrayDialog::applySettings);
    pageLayout->addWidget(m_requestsEnabled);

    auto *form = new QFormLayout;

    m_stationName = new QLineEdit(pageBox);
    m_stationName->setMaxLength(60);
    m_stationName->setPlaceholderText(tr("This computer's name is used when empty"));
    m_stationName->setAccessibleName(tr("Station name shown on the page"));
    connect(m_stationName, &QLineEdit::editingFinished,
            this, &RequestTrayDialog::applySettings);
    form->addRow(tr("Call the station:"), m_stationName);

    m_tagline = new QLineEdit(pageBox);
    m_tagline->setMaxLength(120);
    m_tagline->setPlaceholderText(tr("A line of your own under the name (optional)"));
    m_tagline->setAccessibleName(tr("Tagline shown under the station name"));
    connect(m_tagline, &QLineEdit::editingFinished,
            this, &RequestTrayDialog::applySettings);
    form->addRow(tr("Tagline:"), m_tagline);

    pageLayout->addLayout(form);

    auto *addressRow = new QHBoxLayout;
    m_addresses = new QLabel(pageBox);
    m_addresses->setWordWrap(true);
    m_addresses->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_addresses->setAccessibleName(tr("Where the page can be reached"));
    addressRow->addWidget(m_addresses, 1);

    m_serveButton = new QPushButton(tr("Start serving"), pageBox);
    m_serveButton->setToolTip(
        tr("The page rides on the same server the phone and station sync use. "
           "Nothing is served at all until that server is listening."));
    connect(m_serveButton, &QPushButton::clicked, this, [this]() {
        if (!m_server)
            return;
        if (m_server->isListening()) {
            m_server->stop();
            emit announcementRequested(tr("The public page is no longer being served."));
        } else if (m_server->start()) {
            emit announcementRequested(
                tr("Now serving on port %1.").arg(m_server->port()));
        }
        refreshAddresses();
    });
    addressRow->addWidget(m_serveButton);
    pageLayout->addLayout(addressRow);

    outer->addWidget(pageBox);

    // ------------------------------------------------------------ the tray --
    auto *trayBox = new QGroupBox(tr("What listeners have asked for"), this);
    auto *trayLayout = new QVBoxLayout(trayBox);

    auto *note = new QLabel(
        tr("Nothing here plays by itself. A request sits in this list until "
           "you add it to the running order, mark it played or dismiss it."),
        trayBox);
    note->setWordWrap(true);
    trayLayout->addWidget(note);

    m_table = new QTableWidget(0, 5, trayBox);
    m_table->setHorizontalHeaderLabels({ tr("Received"), tr("Track"), tr("From"),
                                         tr("Dedication"), tr("Status") });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->setAccessibleName(tr("Listener requests"));
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &RequestTrayDialog::showSelectedDetail);
    trayLayout->addWidget(m_table, 1);

    // A read-only plain-text pane on purpose. Everything in it was typed by a
    // stranger, and a plain-text widget cannot be talked into rendering any
    // of it as markup, a link or anything else with behaviour.
    m_detail = new QPlainTextEdit(trayBox);
    m_detail->setReadOnly(true);
    m_detail->setMaximumHeight(90);
    m_detail->setPlaceholderText(tr("Select a request to read it in full."));
    m_detail->setAccessibleName(tr("The selected request in full"));
    trayLayout->addWidget(m_detail);

    m_showHandled = new QCheckBox(tr("Show the ones already dealt with"), trayBox);
    connect(m_showHandled, &QCheckBox::toggled, this, &RequestTrayDialog::refresh);
    trayLayout->addWidget(m_showHandled);

    auto *buttons = new QHBoxLayout;
    m_addButton = new QPushButton(tr("Add to the playlist"), trayBox);
    m_addButton->setToolTip(tr("Puts the track at the end of the running order. "
                               "It does not start playing it."));
    connect(m_addButton, &QPushButton::clicked,
            this, &RequestTrayDialog::addSelectedToPlaylist);
    buttons->addWidget(m_addButton);

    m_playedButton = new QPushButton(tr("Mark as played"), trayBox);
    connect(m_playedButton, &QPushButton::clicked,
            this, &RequestTrayDialog::markSelectedPlayed);
    buttons->addWidget(m_playedButton);

    m_dismissButton = new QPushButton(tr("Dismiss"), trayBox);
    connect(m_dismissButton, &QPushButton::clicked,
            this, &RequestTrayDialog::dismissSelected);
    buttons->addWidget(m_dismissButton);

    buttons->addStretch(1);

    auto *refreshButton = new QPushButton(tr("Refresh"), trayBox);
    connect(refreshButton, &QPushButton::clicked, this, &RequestTrayDialog::refresh);
    buttons->addWidget(refreshButton);

    trayLayout->addLayout(buttons);

    m_summary = new QLabel(trayBox);
    m_summary->setAccessibleName(tr("How many requests are waiting"));
    trayLayout->addWidget(m_summary);

    outer->addWidget(trayBox, 1);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::close);
    outer->addWidget(box);

    resize(720, 640);
}

void RequestTrayDialog::loadSettings()
{
    m_loading = true;
    m_pageEnabled->setChecked(RequestLine::pageEnabled());
    m_requestsEnabled->setChecked(RequestLine::requestsEnabled());
    m_stationName->setText(RequestLine::stationName());
    m_tagline->setText(RequestLine::tagline());
    m_loading = false;

    m_requestsEnabled->setEnabled(m_pageEnabled->isChecked());
    refreshAddresses();
}

void RequestTrayDialog::applySettings()
{
    if (m_loading)
        return;

    const bool wasOn = RequestLine::pageEnabled();
    const bool nowOn = m_pageEnabled->isChecked();

    RequestLine::setPageEnabled(nowOn);
    RequestLine::setRequestsEnabled(m_requestsEnabled->isChecked());
    RequestLine::setStationName(m_stationName->text());
    RequestLine::setTagline(m_tagline->text());

    m_requestsEnabled->setEnabled(nowOn);

    if (nowOn != wasOn) {
        emit announcementRequested(nowOn
            ? tr("The public page is on. Anyone who can reach this computer can read it.")
            : tr("The public page is off."));
    }

    refreshAddresses();
}

void RequestTrayDialog::refreshAddresses()
{
    const bool listening = m_server && m_server->isListening();
    m_serveButton->setText(listening ? tr("Stop serving") : tr("Start serving"));

    if (!m_pageEnabled->isChecked()) {
        m_addresses->setText(tr("The page is switched off; nothing is being served."));
        return;
    }
    if (!listening) {
        m_addresses->setText(
            tr("The page is switched on, but the network server is not "
               "running, so nothing is reachable yet."));
        return;
    }

    const QStringList addresses = m_server->publicPageAddresses();
    m_addresses->setText(addresses.isEmpty()
        ? tr("Serving on port %1, but this computer has no network address.")
              .arg(m_server->port())
        : tr("Anyone on this network can open: %1").arg(addresses.join(QStringLiteral("  "))));
}

void RequestTrayDialog::refresh()
{
    const RequestLine::Entry previous = selectedEntry();

    m_rows = RequestLine::list(!m_showHandled->isChecked());

    m_table->setRowCount(m_rows.size());
    int restore = -1;
    for (int row = 0; row < m_rows.size(); ++row) {
        const RequestLine::Entry &entry = m_rows.at(row);
        if (entry.id == previous.id)
            restore = row;

        QString track = entry.title;
        if (!entry.artist.isEmpty())
            track = tr("%1 — %2").arg(entry.title, entry.artist);

        QString status;
        if (entry.status == RequestLine::statusQueued())         status = tr("Added");
        else if (entry.status == RequestLine::statusPlayed())    status = tr("Played");
        else if (entry.status == RequestLine::statusDismissed()) status = tr("Dismissed");
        else                                                     status = tr("Waiting");

        // Table cells are drawn as plain text by the default delegate, so a
        // dedication full of angle brackets appears as a dedication full of
        // angle brackets. Nothing here interprets what a listener typed.
        const QString cells[] = {
            entry.receivedAt.toString(QStringLiteral("ddd HH:mm")),
            track,
            entry.requester,
            entry.dedication,
            status,
        };
        for (int column = 0; column < 5; ++column) {
            auto *item = new QTableWidgetItem(cells[column]);
            item->setData(Qt::AccessibleTextRole, cells[column]);
            m_table->setItem(row, column, item);
        }
    }

    if (restore >= 0)
        m_table->selectRow(restore);

    const int waiting = RequestLine::pendingCount();
    m_summary->setText(waiting == 0
        ? tr("Nothing waiting.")
        : tr("%n request(s) waiting.", "", waiting));

    showSelectedDetail();
}

RequestLine::Entry RequestTrayDialog::selectedEntry() const
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_rows.size() || m_table->selectedItems().isEmpty())
        return RequestLine::Entry();
    return m_rows.at(row);
}

void RequestTrayDialog::showSelectedDetail()
{
    const RequestLine::Entry entry = selectedEntry();
    const bool have = entry.id > 0;

    m_addButton->setEnabled(have && !entry.path.isEmpty());
    m_playedButton->setEnabled(have);
    m_dismissButton->setEnabled(have);

    if (!have) {
        m_detail->clear();
        return;
    }

    QStringList lines;
    lines << tr("Track: %1 — %2").arg(entry.title, entry.artist);
    lines << tr("Asked for at %1")
                 .arg(entry.receivedAt.toString(QStringLiteral("ddd d MMM, HH:mm")));
    if (!entry.requester.isEmpty())
        lines << tr("From: %1").arg(entry.requester);
    if (!entry.dedication.isEmpty())
        lines << tr("Dedication: %1").arg(entry.dedication);
    if (!entry.fromAddress.isEmpty())
        lines << tr("Sent from %1").arg(entry.fromAddress);

    // setPlainText, never setHtml: the two middle lines are a stranger's.
    m_detail->setPlainText(lines.join(QLatin1Char('\n')));
}

void RequestTrayDialog::setSelectedStatus(const QString &status, const QString &spoken)
{
    const RequestLine::Entry entry = selectedEntry();
    if (entry.id <= 0)
        return;
    if (!RequestLine::setStatus(entry.id, status))
        return;
    emit announcementRequested(spoken);
    refresh();
}

void RequestTrayDialog::addSelectedToPlaylist()
{
    const RequestLine::Entry entry = selectedEntry();
    if (entry.id <= 0)
        return;

    // The path stored with the request is the one the library held when it
    // arrived; a track deleted since is not something to fail quietly over.
    if (entry.path.isEmpty() || !QFileInfo::exists(entry.path)) {
        QMessageBox::warning(this, tr("Listener Requests"),
                             tr("That track is no longer where the library "
                                "says it is, so it cannot be added."));
        return;
    }

    emit addToPlaylistRequested(entry.path);
    setSelectedStatus(RequestLine::statusQueued(),
                      tr("%1 added to the running order.").arg(entry.title));
}

void RequestTrayDialog::markSelectedPlayed()
{
    setSelectedStatus(RequestLine::statusPlayed(), tr("Request marked as played."));
}

void RequestTrayDialog::dismissSelected()
{
    setSelectedStatus(RequestLine::statusDismissed(), tr("Request dismissed."));
}
