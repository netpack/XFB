#include "ProductionSyncDialog.h"

#include "../services/MobileSyncServer.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

ProductionSyncDialog::ProductionSyncDialog(MobileSyncServer *server,
                                           ProductionSyncClient *client,
                                           QWidget *parent)
    : QDialog(parent)
    , m_server(server)
    , m_client(client)
{
    setWindowTitle(tr("Production Computers"));
    setModal(false);   // a fetch can take an hour; the operator keeps working

    auto *outer = new QVBoxLayout(this);
    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *intro = new QLabel(
        tr("Prepare the station's programme on another computer instead of on "
           "the one that is broadcasting. A production computer reads this "
           "station's catalogue, plays what it holds while the work is done, "
           "and sends new and changed entries back — where Auto Mode starts "
           "using them on its very next choice.\n\n"
           "This is not a backup and it does not stand in for one. It prepares "
           "what goes on air; a machine that can take over when the studio "
           "dies is what Broadcast Redundancy is for, and a station wanting "
           "both wants two separate machines.\n\n"
           "Set this up on both machines: use the top half on the one that is "
           "on air, and the bottom half on the one doing the preparing."),
        content);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    // ------------------------------------------------ the station on air ---
    auto *stationBox = new QGroupBox(tr("This XFB is the station on air"), content);
    auto *stationLayout = new QVBoxLayout(stationBox);

    auto *stationHelp = new QLabel(
        tr("Let a production computer read this station's catalogue and add to "
           "it. Only a machine paired from this window can do either — a phone "
           "or a backup station cannot, whatever it asks for."), stationBox);
    stationHelp->setWordWrap(true);
    stationLayout->addWidget(stationHelp);

    m_serverStatus = new QLabel(stationBox);
    stationLayout->addWidget(m_serverStatus);
    m_serverAddress = new QLabel(stationBox);
    m_serverAddress->setTextInteractionFlags(Qt::TextSelectableByMouse
                                             | Qt::TextSelectableByKeyboard);
    stationLayout->addWidget(m_serverAddress);

    auto *stationButtons = new QHBoxLayout;
    m_serverToggle = new QPushButton(tr("Start serving"), stationBox);
    m_serverPair = new QPushButton(tr("Pair a production computer"), stationBox);
    stationButtons->addWidget(m_serverToggle);
    stationButtons->addWidget(m_serverPair);
    stationButtons->addStretch();
    stationLayout->addLayout(stationButtons);

    m_serverCode = new QLabel(stationBox);
    QFont codeFont = m_serverCode->font();
    codeFont.setPointSize(codeFont.pointSize() + 8);
    codeFont.setBold(true);
    m_serverCode->setFont(codeFont);
    m_serverCode->setTextInteractionFlags(Qt::TextSelectableByMouse
                                          | Qt::TextSelectableByKeyboard);
    stationLayout->addWidget(m_serverCode);

    stationLayout->addWidget(new QLabel(tr("Production computers paired with this station:"),
                                        stationBox));
    m_productionList = new QListWidget(stationBox);
    m_productionList->setMinimumHeight(70);
    stationLayout->addWidget(m_productionList);

    m_allowDeletion = new QCheckBox(
        tr("Also delete the audio file when a production computer withdraws an entry"),
        stationBox);
    m_allowDeletion->setChecked(MobileSyncServer::allowsMediaDeletion());
    stationLayout->addWidget(m_allowDeletion);

    auto *deletionHelp = new QLabel(
        tr("Off, a withdrawn ad or jingle only disappears from this station's "
           "lists and its audio stays on disk, where it can be added back. On, "
           "the audio goes too — which keeps the station's folders from filling "
           "up with campaigns that ended, and cannot be undone from here. Only "
           "files inside this station's own music, jingle, ad and program "
           "folders are ever deleted, and never one that another entry still "
           "plays."), stationBox);
    deletionHelp->setWordWrap(true);
    QFont deletionFont = deletionHelp->font();
    deletionFont.setItalic(true);
    deletionHelp->setFont(deletionFont);
    stationLayout->addWidget(deletionHelp);

    m_incoming = new QLabel(stationBox);
    m_incoming->setWordWrap(true);
    stationLayout->addWidget(m_incoming);

    layout->addWidget(stationBox);

    // ------------------------------------------- the machine doing the work -
    auto *productionBox = new QGroupBox(tr("This XFB is a production computer"), content);
    auto *productionLayout = new QVBoxLayout(productionBox);

    auto *productionHelp = new QLabel(
        tr("Work on the station's programme here. On the station, open this "
           "same window, press \"Pair a production computer\" and type the six "
           "digits it shows here."), productionBox);
    productionHelp->setWordWrap(true);
    productionLayout->addWidget(productionHelp);

    auto *form = new QFormLayout;
    m_peerHost = new QLineEdit(m_client ? m_client->peerHost() : QString(), productionBox);
    m_peerHost->setPlaceholderText(tr("for example 192.168.1.20"));
    form->addRow(tr("Station address:"), m_peerHost);

    m_peerPort = new QSpinBox(productionBox);
    m_peerPort->setRange(1, 65535);
    m_peerPort->setValue(m_client && m_client->peerPort()
                             ? m_client->peerPort() : MobileSyncServer::defaultPort());
    form->addRow(tr("Port:"), m_peerPort);

    m_peerCode = new QLineEdit(productionBox);
    m_peerCode->setPlaceholderText(tr("the six digits shown on the station"));
    m_peerCode->setMaxLength(6);
    form->addRow(tr("Pairing code:"), m_peerCode);
    productionLayout->addLayout(form);

    auto *pairRow = new QHBoxLayout;
    m_pairButton = new QPushButton(tr("Pair with this station"), productionBox);
    m_forgetButton = new QPushButton(tr("Forget it"), productionBox);
    pairRow->addWidget(m_pairButton);
    pairRow->addWidget(m_forgetButton);
    pairRow->addStretch();
    productionLayout->addLayout(pairRow);

    m_peerStatus = new QLabel(productionBox);
    m_peerStatus->setWordWrap(true);
    productionLayout->addWidget(m_peerStatus);

    // --- where the audio this machine works with actually lives -------------
    auto *storageBox = new QGroupBox(tr("Where the music is kept"), productionBox);
    auto *storageLayout = new QVBoxLayout(storageBox);

    m_sharedStorage = new QRadioButton(
        tr("Work on the station's folders over the network"), storageBox);
    storageLayout->addWidget(m_sharedStorage);
    auto *sharedHelp = new QLabel(
        tr("Nothing is copied here. Point this XFB's music, jingle, ad and "
           "programme folders (in Options) at the station's own folders on the "
           "network, and this machine plays and edits the very files that go "
           "on air. A station's library is tens of gigabytes; this is the "
           "arrangement for a station with more than one desk."),
        storageBox);
    sharedHelp->setWordWrap(true);
    sharedHelp->setIndent(20);
    storageLayout->addWidget(sharedHelp);

    m_localStorage = new QRadioButton(
        tr("Keep a copy of the station's music on this machine"), storageBox);
    storageLayout->addWidget(m_localStorage);
    auto *localHelp = new QLabel(
        tr("Everything is copied down and new work is uploaded back. Right for "
           "a laptop that leaves the building, and for a small library; it "
           "costs a full copy of the station's media per machine."),
        storageBox);
    localHelp->setWordWrap(true);
    localHelp->setIndent(20);
    storageLayout->addWidget(localHelp);

    if (m_client && m_client->mediaStorage() == ProductionSyncClient::MediaStorage::LocalCopy)
        m_localStorage->setChecked(true);
    else
        m_sharedStorage->setChecked(true);

    m_folderSummary = new QLabel(storageBox);
    m_folderSummary->setWordWrap(true);
    m_folderSummary->setTextInteractionFlags(Qt::TextSelectableByMouse);
    storageLayout->addWidget(m_folderSummary);

    auto *checkRow = new QHBoxLayout;
    m_checkShare = new QPushButton(tr("Check the shared folders"), storageBox);
    m_checkShare->setToolTip(
        tr("Asks the station for a sample of what it holds and looks for those "
           "very files under this machine's folders. Two machines can agree on "
           "a folder name and still be looking at different disks; opening the "
           "files is the only thing that proves they are not."));
    checkRow->addWidget(m_checkShare);
    checkRow->addStretch();
    storageLayout->addLayout(checkRow);

    m_shareStatus = new QLabel(storageBox);
    m_shareStatus->setWordWrap(true);
    storageLayout->addWidget(m_shareStatus);

    productionLayout->addWidget(storageBox);

    auto *actionRow = new QHBoxLayout;
    m_syncButton = new QPushButton(tr("Fetch and publish"), productionBox);
    m_fetchButton = new QPushButton(tr("Fetch from the station"), productionBox);
    m_publishButton = new QPushButton(tr("Publish my work"), productionBox);
    actionRow->addWidget(m_syncButton);
    actionRow->addWidget(m_fetchButton);
    actionRow->addWidget(m_publishButton);
    actionRow->addStretch();
    productionLayout->addLayout(actionRow);

    productionLayout->addWidget(new QLabel(tr("Waiting to be published:"), productionBox));
    m_pendingList = new QListWidget(productionBox);
    m_pendingList->setMinimumHeight(70);
    productionLayout->addWidget(m_pendingList);

    m_progress = new QProgressBar(productionBox);
    m_progress->setRange(0, 100);
    m_progress->setVisible(false);
    productionLayout->addWidget(m_progress);
    m_progressText = new QLabel(productionBox);
    m_progressText->setWordWrap(true);
    productionLayout->addWidget(m_progressText);

    m_syncOnStart = new QCheckBox(tr("Fetch as soon as XFB starts"), productionBox);
    m_syncOnStart->setChecked(m_client && m_client->syncOnStart());
    productionLayout->addWidget(m_syncOnStart);

    m_publishAutomatically =
        new QCheckBox(tr("Publish my work on every automatic run too"), productionBox);
    m_publishAutomatically->setChecked(m_client && m_client->publishAutomatically());
    productionLayout->addWidget(m_publishAutomatically);

    auto *autoRow = new QHBoxLayout;
    autoRow->addWidget(new QLabel(tr("And every"), productionBox));
    m_autoMinutes = new QSpinBox(productionBox);
    m_autoMinutes->setRange(0, 1440);
    m_autoMinutes->setSuffix(tr(" minutes"));
    m_autoMinutes->setSpecialValueText(tr("never (only when asked)"));
    m_autoMinutes->setValue(m_client ? m_client->autoSyncMinutes() : 0);
    autoRow->addWidget(m_autoMinutes);
    autoRow->addStretch();
    productionLayout->addLayout(autoRow);

    m_lastSync = new QLabel(productionBox);
    m_lastSync->setWordWrap(true);
    productionLayout->addWidget(m_lastSync);

    auto *note = new QLabel(
        tr("Fetching merges rather than mirrors: entries prepared here that the "
           "station has not been sent yet are kept, and so are changes made "
           "here to entries the station has not touched. Where both ends "
           "changed the same entry, the station wins — it is the one on air. A "
           "copy of the database is put in XFB's backups folder before each "
           "fetch.\n\n"
           "On the station's shared folders a run is a matter of seconds — "
           "there is nothing to transfer but the entries themselves — so it is "
           "worth letting it run every few minutes. That is what makes this "
           "desk and the studio agree about what exists."),
        productionBox);
    note->setWordWrap(true);
    QFont noteFont = note->font();
    noteFont.setItalic(true);
    note->setFont(noteFont);
    productionLayout->addWidget(note);

    layout->addWidget(productionBox);

    auto *privacy = new QLabel(
        tr("Both halves talk plain HTTP over your own network: the pairing "
           "token keeps strangers out, it does not encrypt what travels. Use "
           "this on a network the station controls, not across the internet."),
        content);
    privacy->setWordWrap(true);
    layout->addWidget(privacy);

    // The window has a lot in it and a laptop screen does not; scrolling is
    // what stops the buttons being squeezed off the bottom.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    outer->addWidget(scroll);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    outer->addWidget(buttons);
    resize(580, 700);

    // ------------------------------------------------------------ wiring ---
    connect(m_serverToggle, &QPushButton::clicked, this, &ProductionSyncDialog::toggleServer);
    connect(m_serverPair, &QPushButton::clicked, this, &ProductionSyncDialog::startPairing);
    connect(m_pairButton, &QPushButton::clicked, this, &ProductionSyncDialog::pairWithStation);
    connect(m_syncButton, &QPushButton::clicked, this, &ProductionSyncDialog::syncNow);
    connect(m_fetchButton, &QPushButton::clicked, this, &ProductionSyncDialog::fetchNow);
    connect(m_publishButton, &QPushButton::clicked, this, &ProductionSyncDialog::publishNow);
    connect(m_forgetButton, &QPushButton::clicked, this, [this]() {
        if (!m_client)
            return;
        const auto answer = QMessageBox::question(
            this, tr("Production Computers"),
            tr("Forget the station this machine produces for? Everything "
               "already copied here stays where it is; only the pairing is "
               "dropped, and work not yet published stays unpublished."));
        if (answer != QMessageBox::Yes)
            return;
        m_client->forgetPeer();
        updateProductionSide();
    });

    connect(m_allowDeletion, &QCheckBox::toggled, this, [this](bool on) {
        MobileSyncServer::setAllowMediaDeletion(on);
        emit announcementRequested(on
            ? tr("Withdrawn entries will have their audio files deleted")
            : tr("Withdrawn entries will keep their audio files"));
    });

    connect(m_checkShare, &QPushButton::clicked, this, &ProductionSyncDialog::checkShare);
    connect(m_sharedStorage, &QRadioButton::toggled, this, [this](bool on) {
        if (!m_client || !on
            || m_client->mediaStorage() == ProductionSyncClient::MediaStorage::Shared) {
            return;
        }
        const auto answer = QMessageBox::question(
            this, tr("Production Computers"),
            tr("Work on the station's folders over the network?\n\n"
               "This XFB's media folders have to point at the station's own "
               "folders for that to mean anything — set them in Options if you "
               "have not already, then press \"Check the shared folders\".\n\n"
               "Nothing already on this machine is deleted, but what this XFB "
               "and the station last agreed on is dropped, so the next fetch "
               "has to run before anything can be published."));
        if (answer != QMessageBox::Yes) {
            QSignalBlocker block(m_localStorage);
            m_localStorage->setChecked(true);
            return;
        }
        m_client->setMediaStorage(ProductionSyncClient::MediaStorage::Shared);
        updateStorageSide();
        emit announcementRequested(tr("Working on the station's shared folders"));
    });
    connect(m_localStorage, &QRadioButton::toggled, this, [this](bool on) {
        if (!m_client || !on
            || m_client->mediaStorage() == ProductionSyncClient::MediaStorage::LocalCopy) {
            return;
        }
        const auto answer = QMessageBox::question(
            this, tr("Production Computers"),
            tr("Keep a copy of the station's music on this machine?\n\n"
               "The next fetch will copy the station's whole library down, "
               "which on a station of any size is a great many gigabytes and a "
               "long wait. Only worth it for a machine that has to work with "
               "the network unplugged."));
        if (answer != QMessageBox::Yes) {
            QSignalBlocker block(m_sharedStorage);
            m_sharedStorage->setChecked(true);
            return;
        }
        m_client->setMediaStorage(ProductionSyncClient::MediaStorage::LocalCopy);
        updateStorageSide();
        emit announcementRequested(tr("Keeping a local copy of the station's music"));
    });

    connect(m_syncOnStart, &QCheckBox::toggled, this, [this](bool on) {
        if (m_client)
            m_client->setSyncOnStart(on);
    });
    connect(m_publishAutomatically, &QCheckBox::toggled, this, [this](bool on) {
        if (m_client)
            m_client->setPublishAutomatically(on);
    });
    connect(m_autoMinutes, &QSpinBox::valueChanged, this, [this](int minutes) {
        if (m_client)
            m_client->setAutoSyncMinutes(minutes);
    });
    connect(m_peerHost, &QLineEdit::editingFinished, this, [this]() {
        if (m_client)
            m_client->setPeer(m_peerHost->text(), quint16(m_peerPort->value()));
    });

    if (m_client) {
        connect(m_client, &ProductionSyncClient::progress, this,
                [this](int percent, const QString &message) {
                    m_progress->setVisible(m_client->busy());
                    if (percent >= 0) {
                        m_progress->setRange(0, 100);
                        m_progress->setValue(percent);
                    } else {
                        m_progress->setRange(0, 0);   // busy indicator
                    }
                    m_progressText->setText(message);
                });
        connect(m_client, &ProductionSyncClient::stageChanged, this,
                [this](ProductionSyncClient::Stage, const QString &description) {
                    emit announcementRequested(description);
                });
        connect(m_client, &ProductionSyncClient::busyChanged, this,
                [this]() { updateProductionSide(); });
        connect(m_client, &ProductionSyncClient::pendingChangesChanged, this,
                [this]() { updateProductionSide(); });
        connect(m_client, &ProductionSyncClient::finished, this,
                [this](const QString &summary) {
                    m_progress->setVisible(false);
                    m_progressText->setText(tr("Finished: %1").arg(summary));
                    emit announcementRequested(tr("Production sync finished: %1").arg(summary));
                    updateProductionSide();
                });
        connect(m_client, &ProductionSyncClient::failed, this,
                [this](const QString &reason) {
                    m_progress->setVisible(false);
                    m_progressText->setText(tr("Stopped: %1").arg(reason));
                    emit announcementRequested(tr("Production sync stopped: %1").arg(reason));
                    updateProductionSide();
                });
        connect(m_client, &ProductionSyncClient::pairingSucceeded, this,
                [this](const QString &name) {
                    m_peerCode->clear();
                    m_peerStatus->setText(tr("Paired with %1.").arg(name));
                    emit announcementRequested(tr("Paired with station %1").arg(name));
                    updateProductionSide();
                });
        connect(m_client, &ProductionSyncClient::sharedStorageChecked, this,
                [this](bool ok, const QString &detail) {
                    m_shareStatus->setText(ok ? tr("Shared: %1").arg(detail)
                                              : tr("Not shared: %1").arg(detail));
                    emit announcementRequested(detail);
                });
        connect(m_client, &ProductionSyncClient::pairingFailed, this,
                [this](const QString &reason) {
                    m_peerStatus->setText(tr("Could not pair: %1").arg(reason));
                    emit announcementRequested(tr("Pairing failed: %1").arg(reason));
                });

        // What is waiting to go up is the question this window exists to
        // answer, so it is answered on opening rather than after the first run.
        m_client->refreshPendingChanges();
    }

    if (m_server) {
        connect(m_server, &MobileSyncServer::catalogueChangedByPeer, this,
                [this](const QString &device, const QString &summary) {
                    m_incoming->setText(tr("%1 has just published %2.").arg(device, summary));
                    emit announcementRequested(
                        tr("%1 published %2 to this station").arg(device, summary));
                });
    }

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &ProductionSyncDialog::refresh);
    m_refreshTimer->start();

    refresh();
}

// ------------------------------------------------------ the station on air

void ProductionSyncDialog::toggleServer()
{
    if (!m_server)
        return;
    if (m_server->isListening()) {
        m_server->stop();
        emit announcementRequested(tr("This station has stopped serving production computers"));
    } else if (m_server->start()) {
        emit announcementRequested(
            tr("This station is now serving production computers on port %1")
                .arg(m_server->port()));
    }
    refresh();
}

void ProductionSyncDialog::startPairing()
{
    if (!m_server)
        return;
    if (!m_server->isListening() && !m_server->start()) {
        QMessageBox::warning(this, tr("Production Computers"),
                             tr("XFB could not start serving on the network."));
        return;
    }
    // Production, not phone and not backup: this window is the only place that
    // grants a token allowed to add anything to this station.
    m_server->beginPairing(MobileSyncServer::PeerRole::Production);
    refresh();
}

void ProductionSyncDialog::updateStationSide()
{
    if (!m_server)
        return;

    const bool listening = m_server->isListening();
    m_serverStatus->setText(listening
        ? tr("Serving — a paired production computer can read this station and "
             "add to it.")
        : tr("Not serving. Nothing on the network can reach this station."));
    m_serverToggle->setText(listening ? tr("Stop serving") : tr("Start serving"));

    const QStringList addresses = m_server->listenAddresses();
    m_serverAddress->setText(listening && !addresses.isEmpty()
        ? tr("Reachable at: %1").arg(addresses.join(QStringLiteral(", ")))
        : QString());

    if (m_server->pairingOpen()
        && m_server->pairingRole() == MobileSyncServer::PeerRole::Production) {
        m_serverCode->setText(tr("%1  (%2s left)")
                                  .arg(m_server->pairingCode())
                                  .arg(m_server->pairingSecondsRemaining()));
        m_serverCode->setVisible(true);
    } else {
        m_serverCode->setVisible(false);
    }
    m_serverPair->setEnabled(!m_server->pairingOpen());

    // The setting lives in xfb.conf, which the Options dialog and a second
    // window can both write; reading it back keeps this checkbox honest.
    if (!m_allowDeletion->hasFocus()) {
        const bool allowed = MobileSyncServer::allowsMediaDeletion();
        if (m_allowDeletion->isChecked() != allowed) {
            QSignalBlocker blocker(m_allowDeletion);
            m_allowDeletion->setChecked(allowed);
        }
    }

    m_productionList->clear();
    const auto devices = m_server->pairedDevices();
    for (const auto &device : devices) {
        if (device.role != MobileSyncServer::PeerRole::Production)
            continue;   // phones and backups are listed in their own windows
        m_productionList->addItem(tr("%1 — paired %2, last seen %3")
                                      .arg(device.name,
                                           device.pairedAt.toString(Qt::TextDate),
                                           device.lastSeen.isValid()
                                               ? device.lastSeen.toString(Qt::TextDate)
                                               : tr("never")));
    }
    if (m_productionList->count() == 0)
        m_productionList->addItem(tr("No production computer has been paired yet."));
}

// ------------------------------------------------ the machine doing the work

void ProductionSyncDialog::pairWithStation()
{
    if (!m_client)
        return;
    m_peerStatus->setText(tr("Pairing..."));
    m_client->pairWith(m_peerHost->text(), quint16(m_peerPort->value()),
                       m_peerCode->text());
}

void ProductionSyncDialog::fetchNow()
{
    if (!m_client)
        return;
    if (m_client->busy()) {
        m_client->cancel();
        return;
    }
    m_progressText->clear();
    m_client->pull();
}

void ProductionSyncDialog::publishNow()
{
    if (!m_client || m_client->busy())
        return;
    m_progressText->clear();
    m_client->publish();
}

void ProductionSyncDialog::syncNow()
{
    if (!m_client)
        return;
    if (m_client->busy()) {
        m_client->cancel();
        return;
    }
    m_progressText->clear();
    m_client->sync();
}

void ProductionSyncDialog::checkShare()
{
    if (!m_client)
        return;
    m_shareStatus->setText(tr("Asking the station what it holds, and looking "
                              "for it here..."));
    m_client->checkSharedStorage();
}

void ProductionSyncDialog::updateStorageSide()
{
    if (!m_client)
        return;

    const bool shared = m_client->sharesMedia();
    m_folderSummary->setVisible(shared);
    m_checkShare->setVisible(shared);
    m_shareStatus->setVisible(shared);
    if (!shared)
        return;

    // Both columns, side by side, because the whole arrangement rests on these
    // being two names for one folder and the operator is the only one who can
    // say whether they are.
    struct Row { QString category; QString title; };
    const QVector<Row> categories = {
        {QStringLiteral("musics"),   tr("Music")},
        {QStringLiteral("jingles"),  tr("Jingles")},
        {QStringLiteral("pub"),      tr("Ads")},
        {QStringLiteral("programs"), tr("Programs")},
    };

    QStringList lines;
    for (const Row &row : categories) {
        const QString here = m_client->localRoot(row.category);
        const QString there = m_client->stationRoot(row.category);
        if (there.isEmpty()) {
            lines << tr("%1: %2 here.").arg(row.title,
                                            here.isEmpty() ? tr("nowhere set") : here);
        } else if (QDir::cleanPath(here) == QDir::cleanPath(there)) {
            lines << tr("%1: %2 — the same path on both.").arg(row.title, here);
        } else {
            lines << tr("%1: %2 here, %3 on the station.")
                         .arg(row.title, here.isEmpty() ? tr("nowhere set") : here, there);
        }
    }
    lines << tr("The two may reach the same folder by different names — a "
                "drive letter here for what the station calls a full path — "
                "which is fine. Only the check below can tell.");
    m_folderSummary->setText(lines.join(QLatin1Char('\n')));
}

void ProductionSyncDialog::updateProductionSide()
{
    if (!m_client)
        return;

    const bool paired = m_client->paired();
    const bool busy = m_client->busy();
    m_peerStatus->setText(paired
        ? tr("Paired with %1. This machine can fetch from it and publish to it.")
              .arg(m_client->peerName().isEmpty() ? m_client->peerHost()
                                                  : m_client->peerName())
        : tr("Not paired with any station yet."));

    m_syncButton->setEnabled(paired);
    m_syncButton->setText(busy ? tr("Stop") : tr("Fetch and publish"));
    m_fetchButton->setEnabled(paired && !busy);
    m_publishButton->setEnabled(paired && !busy);
    m_forgetButton->setEnabled(paired);
    m_pairButton->setEnabled(!busy);
    m_checkShare->setEnabled(paired && !busy);
    updateStorageSide();

    const QStringList pending = m_client->pendingChanges();
    m_pendingList->clear();
    if (pending.isEmpty()) {
        m_pendingList->addItem(paired
            ? tr("Nothing — the station has everything this machine has made.")
            : tr("Pair with a station to see what would be published."));
    } else {
        m_pendingList->addItems(pending);
    }

    const QDateTime last = m_client->lastSync();
    m_lastSync->setText(last.isValid()
        ? tr("Last run: %1 — %2").arg(last.toString(Qt::TextDate), m_client->lastResult())
        : tr("This machine has never fetched or published."));
}

void ProductionSyncDialog::refresh()
{
    updateStationSide();
    updateProductionSide();
}
