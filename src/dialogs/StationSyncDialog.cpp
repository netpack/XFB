#include "StationSyncDialog.h"

#include "../services/MobileSyncServer.h"

#include <QCheckBox>
#include <QDialogButtonBox>
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
#include <QScrollArea>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

StationSyncDialog::StationSyncDialog(MobileSyncServer *server,
                                     StationSyncClient *client,
                                     QWidget *parent)
    : QDialog(parent)
    , m_server(server)
    , m_client(client)
{
    setWindowTitle(tr("Broadcast Redundancy"));
    setModal(false);   // a sync can take an hour; the operator keeps working

    auto *outer = new QVBoxLayout(this);
    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *intro = new QLabel(
        tr("Keep a second XFB on this network holding the same music, jingles, "
           "ads, programs, schedule and playlists as this one, so that if the "
           "studio machine fails the backup only has to be launched to go back "
           "on air.\n\n"
           "Everything is copied, on purpose. A machine that has to survive the "
           "studio going down cannot depend on anything in the studio, which "
           "means its own disk, its own copy of every file, and no share to be "
           "cut off from. That is what makes this different from Production "
           "Computers, where a second machine works on the station's own files "
           "over the network and copies nothing.\n\n"
           "Set this up on both machines: use the top half on the one that is "
           "on air, and the bottom half on the one standing by."), content);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    // ------------------------------------------------ the station on air ---
    auto *sourceBox = new QGroupBox(tr("This XFB is the station on air"), content);
    auto *sourceLayout = new QVBoxLayout(sourceBox);

    auto *sourceHelp = new QLabel(
        tr("Let a backup machine copy from this one. Nothing is sent anywhere "
           "on its own — the backup connects and asks, and only a machine that "
           "has been paired here can ask at all."), sourceBox);
    sourceHelp->setWordWrap(true);
    sourceLayout->addWidget(sourceHelp);

    m_serverStatus = new QLabel(sourceBox);
    sourceLayout->addWidget(m_serverStatus);
    m_serverAddress = new QLabel(sourceBox);
    m_serverAddress->setTextInteractionFlags(Qt::TextSelectableByMouse
                                             | Qt::TextSelectableByKeyboard);
    sourceLayout->addWidget(m_serverAddress);

    auto *sourceButtons = new QHBoxLayout;
    m_serverToggle = new QPushButton(tr("Start serving"), sourceBox);
    m_serverPair = new QPushButton(tr("Pair a backup machine"), sourceBox);
    sourceButtons->addWidget(m_serverToggle);
    sourceButtons->addWidget(m_serverPair);
    sourceButtons->addStretch();
    sourceLayout->addLayout(sourceButtons);

    m_serverCode = new QLabel(sourceBox);
    QFont codeFont = m_serverCode->font();
    codeFont.setPointSize(codeFont.pointSize() + 8);
    codeFont.setBold(true);
    m_serverCode->setFont(codeFont);
    m_serverCode->setTextInteractionFlags(Qt::TextSelectableByMouse
                                          | Qt::TextSelectableByKeyboard);
    sourceLayout->addWidget(m_serverCode);

    sourceLayout->addWidget(new QLabel(tr("Backup machines paired with this station:"),
                                       sourceBox));
    m_backupList = new QListWidget(sourceBox);
    m_backupList->setMinimumHeight(70);
    sourceLayout->addWidget(m_backupList);

    layout->addWidget(sourceBox);

    // ---------------------------------------------- the station standing by -
    auto *backupBox = new QGroupBox(tr("This XFB is the backup"), content);
    auto *backupLayout = new QVBoxLayout(backupBox);

    auto *backupHelp = new QLabel(
        tr("Copy everything from the station on air onto this machine. On that "
           "machine, open this same window, press \"Pair a backup machine\" and "
           "type the six digits it shows here."), backupBox);
    backupHelp->setWordWrap(true);
    backupLayout->addWidget(backupHelp);

    auto *form = new QFormLayout;
    m_peerHost = new QLineEdit(m_client ? m_client->peerHost() : QString(), backupBox);
    m_peerHost->setPlaceholderText(tr("for example 192.168.1.20"));
    form->addRow(tr("Station address:"), m_peerHost);

    m_peerPort = new QSpinBox(backupBox);
    m_peerPort->setRange(1, 65535);
    m_peerPort->setValue(m_client && m_client->peerPort()
                             ? m_client->peerPort() : MobileSyncServer::defaultPort());
    form->addRow(tr("Port:"), m_peerPort);

    m_peerCode = new QLineEdit(backupBox);
    m_peerCode->setPlaceholderText(tr("the six digits shown on the station"));
    m_peerCode->setMaxLength(6);
    form->addRow(tr("Pairing code:"), m_peerCode);
    backupLayout->addLayout(form);

    auto *pairRow = new QHBoxLayout;
    m_pairButton = new QPushButton(tr("Pair with this station"), backupBox);
    m_forgetButton = new QPushButton(tr("Forget it"), backupBox);
    pairRow->addWidget(m_pairButton);
    pairRow->addWidget(m_forgetButton);
    pairRow->addStretch();
    backupLayout->addLayout(pairRow);

    m_peerStatus = new QLabel(backupBox);
    m_peerStatus->setWordWrap(true);
    backupLayout->addWidget(m_peerStatus);

    auto *syncRow = new QHBoxLayout;
    m_syncButton = new QPushButton(tr("Sync now"), backupBox);
    syncRow->addWidget(m_syncButton);
    syncRow->addStretch();
    backupLayout->addLayout(syncRow);

    m_progress = new QProgressBar(backupBox);
    m_progress->setRange(0, 100);
    m_progress->setVisible(false);
    backupLayout->addWidget(m_progress);
    m_progressText = new QLabel(backupBox);
    m_progressText->setWordWrap(true);
    backupLayout->addWidget(m_progressText);

    m_syncOnStart = new QCheckBox(tr("Sync as soon as XFB starts"), backupBox);
    m_syncOnStart->setChecked(m_client && m_client->syncOnStart());
    backupLayout->addWidget(m_syncOnStart);

    auto *autoRow = new QHBoxLayout;
    autoRow->addWidget(new QLabel(tr("And every"), backupBox));
    m_autoMinutes = new QSpinBox(backupBox);
    m_autoMinutes->setRange(0, 1440);
    m_autoMinutes->setSuffix(tr(" minutes"));
    m_autoMinutes->setSpecialValueText(tr("never (only when asked)"));
    m_autoMinutes->setValue(m_client ? m_client->autoSyncMinutes() : 0);
    autoRow->addWidget(m_autoMinutes);
    autoRow->addStretch();
    backupLayout->addLayout(autoRow);

    m_lastSync = new QLabel(backupBox);
    m_lastSync->setWordWrap(true);
    backupLayout->addWidget(m_lastSync);

    // --- watching the studio ------------------------------------------------
    // A mirror of the station's music is not a backup if nobody notices the
    // studio has gone quiet. This machine polls the station's heartbeat and
    // says so, loudly, on this screen. It stops there: putting a machine on
    // air stays a decision a person makes, with the Take Over button.
    auto *watchRow = new QHBoxLayout;
    watchRow->addWidget(new QLabel(tr("Check the studio every"), backupBox));
    m_monitorSeconds = new QSpinBox(backupBox);
    m_monitorSeconds->setRange(0, 3600);
    m_monitorSeconds->setSuffix(tr(" seconds"));
    m_monitorSeconds->setSpecialValueText(tr("never"));
    m_monitorSeconds->setValue(m_client ? m_client->monitorSeconds() : 0);
    m_monitorSeconds->setAccessibleName(tr("How often to check the studio"));
    watchRow->addWidget(m_monitorSeconds);
    watchRow->addWidget(new QLabel(tr("and warn after"), backupBox));
    m_darkAfterSeconds = new QSpinBox(backupBox);
    m_darkAfterSeconds->setRange(15, 86400);
    m_darkAfterSeconds->setSuffix(tr(" seconds"));
    m_darkAfterSeconds->setValue(m_client ? m_client->darkAfterSeconds() : 90);
    m_darkAfterSeconds->setAccessibleName(
        tr("How long the studio may be dark before the alarm"));
    watchRow->addWidget(m_darkAfterSeconds);
    watchRow->addStretch();
    backupLayout->addLayout(watchRow);

    m_studioStatus = new QLabel(backupBox);
    m_studioStatus->setWordWrap(true);
    backupLayout->addWidget(m_studioStatus);

    auto *warning = new QLabel(
        tr("A sync makes this machine's catalogue match the station's: entries "
           "this machine has that the station does not are removed from the "
           "list. The audio files themselves are never deleted, and a copy of "
           "the database is put in XFB's backups folder before each sync."),
        backupBox);
    warning->setWordWrap(true);
    QFont warnFont = warning->font();
    warnFont.setItalic(true);
    warning->setFont(warnFont);
    backupLayout->addWidget(warning);

    layout->addWidget(backupBox);

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
    resize(560, 640);

    // ------------------------------------------------------------ wiring ---
    connect(m_serverToggle, &QPushButton::clicked, this, &StationSyncDialog::toggleServer);
    connect(m_serverPair, &QPushButton::clicked, this, &StationSyncDialog::startPairing);
    connect(m_pairButton, &QPushButton::clicked, this, &StationSyncDialog::pairWithStation);
    connect(m_syncButton, &QPushButton::clicked, this, &StationSyncDialog::syncNow);
    connect(m_forgetButton, &QPushButton::clicked, this, [this]() {
        if (!m_client)
            return;
        const auto answer = QMessageBox::question(
            this, tr("Broadcast Redundancy"),
            tr("Forget the station this machine mirrors? The music already "
               "copied here stays where it is; only the pairing is dropped."));
        if (answer != QMessageBox::Yes)
            return;
        m_client->forgetPeer();
        updateBackupSide();
    });

    connect(m_syncOnStart, &QCheckBox::toggled, this, [this](bool on) {
        if (m_client)
            m_client->setSyncOnStart(on);
    });
    connect(m_monitorSeconds, &QSpinBox::valueChanged, this, [this](int seconds) {
        if (m_client)
            m_client->setMonitorSeconds(seconds);
        updateBackupSide();
    });
    connect(m_darkAfterSeconds, &QSpinBox::valueChanged, this, [this](int seconds) {
        if (m_client)
            m_client->setDarkAfterSeconds(seconds);
        updateBackupSide();
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
        connect(m_client, &StationSyncClient::progress, this,
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
        connect(m_client, &StationSyncClient::stageChanged, this,
                [this](StationSyncClient::Stage, const QString &description) {
                    emit announcementRequested(description);
                });
        connect(m_client, &StationSyncClient::busyChanged, this,
                [this]() { updateBackupSide(); });
        connect(m_client, &StationSyncClient::finished, this,
                [this](const QString &summary) {
                    m_progress->setVisible(false);
                    m_progressText->setText(tr("Sync finished: %1").arg(summary));
                    emit announcementRequested(tr("Station sync finished: %1").arg(summary));
                    updateBackupSide();
                });
        connect(m_client, &StationSyncClient::failed, this,
                [this](const QString &reason) {
                    m_progress->setVisible(false);
                    m_progressText->setText(tr("Sync stopped: %1").arg(reason));
                    emit announcementRequested(tr("Station sync stopped: %1").arg(reason));
                    updateBackupSide();
                });
        connect(m_client, &StationSyncClient::pairingSucceeded, this,
                [this](const QString &name) {
                    m_peerCode->clear();
                    m_peerStatus->setText(tr("Paired with %1.").arg(name));
                    emit announcementRequested(tr("Paired with station %1").arg(name));
                    updateBackupSide();
                });
        connect(m_client, &StationSyncClient::pairingFailed, this,
                [this](const QString &reason) {
                    m_peerStatus->setText(tr("Could not pair: %1").arg(reason));
                    emit announcementRequested(tr("Pairing failed: %1").arg(reason));
                });
    }

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &StationSyncDialog::refresh);
    m_refreshTimer->start();

    refresh();
}

// ------------------------------------------------------ the station on air

void StationSyncDialog::toggleServer()
{
    if (!m_server)
        return;
    if (m_server->isListening()) {
        m_server->stop();
        emit announcementRequested(tr("This station has stopped serving backups"));
    } else if (m_server->start()) {
        emit announcementRequested(tr("This station is now serving backups on port %1")
                                       .arg(m_server->port()));
    }
    refresh();
}

void StationSyncDialog::startPairing()
{
    if (!m_server)
        return;
    if (!m_server->isListening() && !m_server->start()) {
        QMessageBox::warning(this, tr("Broadcast Redundancy"),
                             tr("XFB could not start serving on the network."));
        return;
    }
    // Station, not phone: this window is the only place that grants a token
    // allowed to read the whole station.
    m_server->beginPairing(MobileSyncServer::PeerRole::Station);
    refresh();
}

void StationSyncDialog::updateSourceSide()
{
    if (!m_server)
        return;

    const bool listening = m_server->isListening();
    m_serverStatus->setText(listening
        ? tr("Serving — a paired backup can copy from this station.")
        : tr("Not serving. Nothing on the network can reach this station."));
    m_serverToggle->setText(listening ? tr("Stop serving") : tr("Start serving"));

    const QStringList addresses = m_server->listenAddresses();
    m_serverAddress->setText(listening && !addresses.isEmpty()
        ? tr("Reachable at: %1").arg(addresses.join(QStringLiteral(", ")))
        : QString());

    if (m_server->pairingOpen()
        && m_server->pairingRole() == MobileSyncServer::PeerRole::Station) {
        m_serverCode->setText(tr("%1  (%2s left)")
                                  .arg(m_server->pairingCode())
                                  .arg(m_server->pairingSecondsRemaining()));
        m_serverCode->setVisible(true);
    } else {
        m_serverCode->setVisible(false);
    }
    m_serverPair->setEnabled(!m_server->pairingOpen());

    m_backupList->clear();
    const auto devices = m_server->pairedDevices();
    for (const auto &device : devices) {
        if (device.role != MobileSyncServer::PeerRole::Station)
            continue;   // phones are listed in the Sync to Phone window
        m_backupList->addItem(tr("%1 — paired %2, last seen %3")
                                  .arg(device.name,
                                       device.pairedAt.toString(Qt::TextDate),
                                       device.lastSeen.isValid()
                                           ? device.lastSeen.toString(Qt::TextDate)
                                           : tr("never")));
    }
    if (m_backupList->count() == 0)
        m_backupList->addItem(tr("No backup machine has been paired yet."));
}

// --------------------------------------------------- the station standing by

void StationSyncDialog::pairWithStation()
{
    if (!m_client)
        return;
    m_peerStatus->setText(tr("Pairing..."));
    m_client->pairWith(m_peerHost->text(), quint16(m_peerPort->value()),
                       m_peerCode->text());
}

void StationSyncDialog::syncNow()
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

void StationSyncDialog::updateBackupSide()
{
    if (!m_client)
        return;

    const bool paired = m_client->paired();
    m_peerStatus->setText(paired
        ? tr("Paired with %1. This machine can copy from it at any time.")
              .arg(m_client->peerName().isEmpty() ? m_client->peerHost()
                                                  : m_client->peerName())
        : tr("Not paired with any station yet."));

    m_syncButton->setEnabled(paired);
    m_syncButton->setText(m_client->busy() ? tr("Stop") : tr("Sync now"));
    m_forgetButton->setEnabled(paired);
    m_pairButton->setEnabled(!m_client->busy());

    const QDateTime last = m_client->lastSync();
    m_lastSync->setText(last.isValid()
        ? tr("Last sync: %1 — %2").arg(last.toString(Qt::TextDate),
                                       m_client->lastResult())
        : tr("This machine has never synced."));

    if (m_client->monitorSeconds() <= 0) {
        m_studioStatus->setText(tr("Not watching the studio."));
        m_studioStatus->setStyleSheet(QString());
    } else if (m_client->studioIsDark()) {
        m_studioStatus->setText(
            tr("THE STUDIO IS DARK. Nothing has been put on air automatically "
               "— that is still yours to decide."));
        m_studioStatus->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
    } else {
        const QDateTime good = m_client->lastGoodHeartbeat();
        m_studioStatus->setText(
            tr("Studio: %1%2")
                .arg(m_client->studioState().isEmpty() ? tr("not heard from yet")
                                                       : m_client->studioState(),
                     good.isValid()
                         ? tr(" — last making sound at %1")
                               .arg(good.toString(QStringLiteral("hh:mm:ss")))
                         : QString()));
        m_studioStatus->setStyleSheet(QString());
    }
}

void StationSyncDialog::refresh()
{
    updateSourceSide();
    updateBackupSide();
}
