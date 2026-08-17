#include "MobileSyncDialog.h"

#include "../services/MobileSyncServer.h"
#include "../QrCode.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

MobileSyncDialog::MobileSyncDialog(MobileSyncServer *server, QWidget *parent)
    : QDialog(parent)
    , m_server(server)
{
    setWindowTitle(tr("Sync to Phone"));
    setModal(false);   // the operator may want to keep working while pairing

    auto *layout = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Serve this station's library and playlists to the XFB app on your "
           "phone, over your own network, so you can take a set with you and "
           "play it offline."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    // --- server state ------------------------------------------------------
    auto *serverBox = new QGroupBox(tr("Server"), this);
    auto *serverLayout = new QVBoxLayout(serverBox);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse
                                           | Qt::TextSelectableByKeyboard);
    serverLayout->addWidget(m_statusLabel);

    m_addressLabel = new QLabel(this);
    m_addressLabel->setWordWrap(true);
    m_addressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse
                                            | Qt::TextSelectableByKeyboard);
    serverLayout->addWidget(m_addressLabel);

    m_toggleButton = new QPushButton(this);
    m_toggleButton->setAccessibleName(tr("Start or stop the phone sync server"));
    connect(m_toggleButton, &QPushButton::clicked, this, &MobileSyncDialog::toggleServer);
    serverLayout->addWidget(m_toggleButton);

    m_autoStart = new QCheckBox(tr("Start serving when XFB starts"), this);
    {
        QSettings settings;
        m_autoStart->setChecked(
            settings.value(QStringLiteral("MobileSync/AutoStart"), false).toBool());
    }
    connect(m_autoStart, &QCheckBox::toggled, this, [](bool on) {
        QSettings settings;
        settings.setValue(QStringLiteral("MobileSync/AutoStart"), on);
    });
    serverLayout->addWidget(m_autoStart);

    layout->addWidget(serverBox);

    // --- pairing -----------------------------------------------------------
    auto *pairBox = new QGroupBox(tr("Pair a phone"), this);
    auto *pairLayout = new QVBoxLayout(pairBox);

    m_pairButton = new QPushButton(tr("Show a pairing code"), this);
    connect(m_pairButton, &QPushButton::clicked, this, &MobileSyncDialog::startPairing);
    pairLayout->addWidget(m_pairButton);

    // Which interface the phone should reach us on. A studio machine often has
    // several — a VPN or a virtual adapter can easily sort first — and encoding
    // the wrong one produces a QR that fails with no explanation.
    auto *addressRow = new QHBoxLayout;
    addressRow->addWidget(new QLabel(tr("Reachable at:"), this));
    m_addressBox = new QComboBox(this);
    m_addressBox->setAccessibleName(tr("Network address the phone should connect to"));
    connect(m_addressBox, &QComboBox::currentTextChanged,
            this, [this]() { updatePairingCode(); });
    addressRow->addWidget(m_addressBox, 1);
    pairLayout->addLayout(addressRow);

    m_qrLabel = new QLabel(this);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setAccessibleName(tr("Pairing QR code"));
    m_qrLabel->setStyleSheet(QStringLiteral("background: white; padding: 8px;"));
    pairLayout->addWidget(m_qrLabel);

    m_codeLabel = new QLabel(this);
    m_codeLabel->setAlignment(Qt::AlignCenter);
    m_codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse
                                         | Qt::TextSelectableByKeyboard);
    {
        QFont codeFont = m_codeLabel->font();
        codeFont.setPointSize(codeFont.pointSize() + 10);
        codeFont.setBold(true);
        m_codeLabel->setFont(codeFont);
    }
    pairLayout->addWidget(m_codeLabel);

    layout->addWidget(pairBox);

    // --- paired devices ----------------------------------------------------
    auto *deviceBox = new QGroupBox(tr("Paired phones"), this);
    auto *deviceLayout = new QVBoxLayout(deviceBox);

    m_deviceList = new QListWidget(this);
    m_deviceList->setAccessibleName(tr("Phones paired with this station"));
    connect(m_deviceList, &QListWidget::itemSelectionChanged,
            this, &MobileSyncDialog::updateRevokeButton);
    deviceLayout->addWidget(m_deviceList);

    m_revokeButton = new QPushButton(tr("Remove the selected phone"), this);
    connect(m_revokeButton, &QPushButton::clicked, this, &MobileSyncDialog::revokeSelected);
    deviceLayout->addWidget(m_revokeButton);

    layout->addWidget(deviceBox);

    // --- playlists folder --------------------------------------------------
    auto *folderRow = new QHBoxLayout;
    m_playlistsLabel = new QLabel(this);
    m_playlistsLabel->setWordWrap(true);
    folderRow->addWidget(m_playlistsLabel, 1);
    auto *chooseButton = new QPushButton(tr("Choose..."), this);
    chooseButton->setAccessibleName(tr("Choose the folder holding saved playlists"));
    connect(chooseButton, &QPushButton::clicked,
            this, &MobileSyncDialog::choosePlaylistsDirectory);
    folderRow->addWidget(chooseButton);
    layout->addLayout(folderRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    if (m_server) {
        connect(m_server, &MobileSyncServer::started, this, &MobileSyncDialog::refresh);
        connect(m_server, &MobileSyncServer::stopped, this, &MobileSyncDialog::refresh);
        connect(m_server, &MobileSyncServer::pairingWindowChanged,
                this, &MobileSyncDialog::refresh);
        connect(m_server, &MobileSyncServer::devicePaired, this,
                [this](const QString &name) {
                    emit announcementRequested(tr("%1 paired with XFB").arg(name));
                    refresh();
                });
    }

    // The pairing code counts down, so the dialog has to tick.
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &MobileSyncDialog::refresh);
    m_refreshTimer->start();

    refresh();
    resize(480, 620);
}

void MobileSyncDialog::toggleServer()
{
    if (!m_server)
        return;

    if (m_server->isListening()) {
        m_server->stop();
        emit announcementRequested(tr("Phone sync stopped"));
    } else if (m_server->start()) {
        emit announcementRequested(
            tr("Phone sync started on port %1").arg(m_server->port()));
    }
    refresh();
}

void MobileSyncDialog::startPairing()
{
    if (!m_server)
        return;

    // Pairing is pointless if nothing is listening; just do the obvious thing.
    if (!m_server->isListening() && !m_server->start())
        return;

    const QString code = m_server->beginPairing();

    // Read out digit by digit — a screen reader would otherwise say "four
    // hundred and eighty-two thousand..." for a code the operator must type.
    QStringList digits;
    for (const QChar digit : code)
        digits << QString(digit);
    emit announcementRequested(tr("Pairing code %1").arg(digits.join(QLatin1Char(' '))));

    refresh();
}

void MobileSyncDialog::revokeSelected()
{
    if (!m_server)
        return;

    QListWidgetItem *item = m_deviceList->currentItem();
    if (!item)
        return;

    const QString tokenId = item->data(Qt::UserRole).toString();

    QString name;
    for (const auto &device : m_server->pairedDevices()) {
        if (device.tokenId == tokenId) {
            name = device.name;
            break;
        }
    }

    m_server->revokeDevice(tokenId);
    emit announcementRequested(name.isEmpty() ? tr("Phone removed")
                                              : tr("%1 removed").arg(name));
    refresh();
}

void MobileSyncDialog::choosePlaylistsDirectory()
{
    if (!m_server)
        return;

    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Folder holding saved playlists"), m_server->playlistsDirectory());
    if (!dir.isEmpty()) {
        m_server->setPlaylistsDirectory(dir);
        refresh();
    }
}

void MobileSyncDialog::refresh()
{
    if (!m_server)
        return;

    const bool listening = m_server->isListening();

    m_statusLabel->setText(listening
        ? tr("Serving on port %1.").arg(m_server->port())
        : tr("Not serving."));
    m_toggleButton->setText(listening ? tr("Stop serving") : tr("Start serving"));

    const QStringList addresses = m_server->listenAddresses();
    m_addressLabel->setText(addresses.isEmpty()
        ? QString()
        : tr("On this network: %1").arg(addresses.join(QStringLiteral("   "))));

    updateAddressChoices();
    updatePairingCode();

    updateDeviceList();

    m_playlistsLabel->setText(tr("Saved playlists: %1").arg(m_server->playlistsDirectory()));
}

void MobileSyncDialog::updateDeviceList()
{
    // refresh() runs once a second to move the pairing countdown, so this must
    // not disturb the list the operator is working in: clearing it would drop
    // the selection a moment after it was made, leaving the remove button with
    // nothing to act on.
    const auto devices = m_server->pairedDevices();

    QStringList labels;
    QStringList tokenIds;
    labels.reserve(devices.size());
    tokenIds.reserve(devices.size());
    for (const auto &device : devices) {
        labels << (device.lastSeen.isValid()
            ? tr("%1 — last seen %2").arg(device.name,
                                          device.lastSeen.toString(QStringLiteral("d MMM, HH:mm")))
            : device.name);
        tokenIds << device.tokenId;
    }

    QStringList shownLabels;
    shownLabels.reserve(m_deviceList->count());
    for (int i = 0; i < m_deviceList->count(); ++i)
        shownLabels << m_deviceList->item(i)->text();

    if (shownLabels != labels) {
        // "last seen" ticks over as a phone checks in, so a rebuild still
        // happens now and then; carry the selection across it by token rather
        // than by row, which a device appearing or leaving would shift.
        const QListWidgetItem *chosen = m_deviceList->currentItem();
        const QString chosenToken = chosen ? chosen->data(Qt::UserRole).toString() : QString();

        {
            QSignalBlocker blocker(m_deviceList);
            m_deviceList->clear();
            for (int i = 0; i < labels.size(); ++i) {
                auto *item = new QListWidgetItem(labels.at(i), m_deviceList);
                item->setData(Qt::UserRole, tokenIds.at(i));
            }
            const int index = tokenIds.indexOf(chosenToken);
            if (!chosenToken.isEmpty() && index >= 0)
                m_deviceList->setCurrentRow(index);
        }
    }

    updateRevokeButton();
}

void MobileSyncDialog::updateRevokeButton()
{
    // Enabled by selection, not merely by the list being non-empty: the button
    // acts on the current item, so with nothing chosen it would do nothing.
    m_revokeButton->setEnabled(m_deviceList->currentItem() != nullptr);
}

void MobileSyncDialog::updateAddressChoices()
{
    const QStringList hosts = m_server->hostAddresses();

    QStringList current;
    current.reserve(m_addressBox->count());
    for (int i = 0; i < m_addressBox->count(); ++i)
        current << m_addressBox->itemText(i);
    if (current == hosts)
        return;   // rebuilding would fight the operator's own selection

    const QString chosen = m_addressBox->currentText();
    {
        QSignalBlocker blocker(m_addressBox);
        m_addressBox->clear();
        m_addressBox->addItems(hosts);
        const int index = m_addressBox->findText(chosen);
        if (index >= 0)
            m_addressBox->setCurrentIndex(index);
    }
    m_addressBox->setEnabled(!hosts.isEmpty());
}

void MobileSyncDialog::updatePairingCode()
{
    if (!m_server->pairingOpen()) {
        m_codeLabel->clear();
        m_qrLabel->clear();
        m_shownUri.clear();
        return;
    }

    m_codeLabel->setText(tr("%1\nexpires in %2s")
                             .arg(m_server->pairingCode())
                             .arg(m_server->pairingSecondsRemaining()));

    const QString uri = m_server->pairingUri(m_addressBox->currentText());
    if (uri == m_shownUri)
        return;   // only the countdown moved
    m_shownUri = uri;

    if (uri.isEmpty()) {
        m_qrLabel->clear();
        m_qrLabel->setToolTip(QString());
        return;
    }

    const QrCode::Matrix matrix = QrCode::encode(uri.toUtf8());
    if (!matrix.isValid()) {
        m_qrLabel->setText(tr("(the code is too long to show as a QR)"));
        return;
    }

    // Painted at the screen's own pixel ratio: a QR scaled up by the window
    // system picks up soft edges that make it slower to scan.
    //
    // The quiet zone is six modules rather than the specified four, and the
    // label carries a white background of its own, because under a dark theme
    // the symbol would otherwise meet a dark dialog right at the minimum
    // margin and some scanners refuse to lock on.
    const int quiet = 6;
    const int modules = matrix.size + quiet * 2;
    const qreal ratio = devicePixelRatioF();
    const int scale = std::max(3, int((220 * ratio) / modules));
    const int side = modules * scale;

    QImage image(side, side, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    for (int y = 0; y < matrix.size; ++y)
        for (int x = 0; x < matrix.size; ++x)
            if (matrix.at(x, y))
                painter.drawRect((x + quiet) * scale, (y + quiet) * scale, scale, scale);
    painter.end();

    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(ratio);
    m_qrLabel->setPixmap(pixmap);
    m_qrLabel->setToolTip(uri);
}
