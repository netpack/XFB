#include "StreamDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

/** Colour the state label the way the rest of XFB colours live state. */
QString stateColour(IcecastSource::State state)
{
    switch (state) {
    case IcecastSource::State::Connected:  return QStringLiteral("green");
    case IcecastSource::State::Connecting: return QStringLiteral("orange");
    case IcecastSource::State::Disconnected: break;
    }
    return QStringLiteral("gray");
}

} // namespace

StreamDialog::StreamDialog(StreamService *service, QWidget *parent)
    : QDialog(parent)
    , m_service(service)
{
    setWindowTitle(tr("Stream to Icecast"));
    setModal(false);   // the operator keeps working while the station is on air

    m_mounts = m_service ? m_service->mounts() : QVector<StreamService::Mount>();
    buildUi();

    if (m_service) {
        connect(m_service, &StreamService::logMessage, this, &StreamDialog::appendLog);
        connect(m_service, &StreamService::statusChanged,
                this, &StreamDialog::refreshStatus);
        connect(m_service, &StreamService::activeChanged,
                this, [this](bool) { refreshStatus(); });
        connect(m_service, &StreamService::nowPlayingChanged, this,
                [this](const QString &song) {
            m_nowPlayingLabel->setText(tr("Now playing sent: %1").arg(song));
        });
    }

    refreshList();
    if (!m_mounts.isEmpty())
        selectMount(0);
    else
        loadIntoForm(StreamService::defaultMount());
    refreshStatus();
}

void StreamDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Send what XFB is playing straight to an Icecast server, with the "
           "track titles, without a second program listening to the sound "
           "card. The external Icecast and butt buttons on the main window "
           "still work exactly as before; this is an alternative, not a "
           "replacement."), this);
    intro->setWordWrap(true);
    outer->addWidget(intro);

    auto *middle = new QHBoxLayout;
    outer->addLayout(middle, 1);

    // ------------------------------------------------------ the mount list --
    auto *listColumn = new QVBoxLayout;
    listColumn->addWidget(new QLabel(tr("Mounts"), this));

    m_list = new QListWidget(this);
    m_list->setMinimumWidth(190);
    m_list->setAccessibleName(tr("Configured stream mounts"));
    connect(m_list, &QListWidget::currentRowChanged, this, &StreamDialog::selectMount);
    listColumn->addWidget(m_list, 1);

    auto *listButtons = new QHBoxLayout;
    m_addButton = new QPushButton(tr("Add"), this);
    m_duplicateButton = new QPushButton(tr("Duplicate"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    connect(m_addButton, &QPushButton::clicked, this, &StreamDialog::addMount);
    connect(m_duplicateButton, &QPushButton::clicked, this, &StreamDialog::duplicateMount);
    connect(m_removeButton, &QPushButton::clicked, this, &StreamDialog::removeMount);
    listButtons->addWidget(m_addButton);
    listButtons->addWidget(m_duplicateButton);
    listButtons->addWidget(m_removeButton);
    listColumn->addLayout(listButtons);
    middle->addLayout(listColumn);

    // ------------------------------------------------ the selected mount ---
    auto *formColumn = new QVBoxLayout;

    auto *serverBox = new QGroupBox(tr("Server"), this);
    auto *serverForm = new QFormLayout(serverBox);

    m_enabled = new QCheckBox(tr("Stream to this mount"), serverBox);
    serverForm->addRow(m_enabled);

    m_name = new QLineEdit(serverBox);
    m_name->setPlaceholderText(tr("Website 128k"));
    serverForm->addRow(tr("Label:"), m_name);

    m_host = new QLineEdit(serverBox);
    m_host->setPlaceholderText(QStringLiteral("stream.example.org"));
    serverForm->addRow(tr("Host:"), m_host);

    m_port = new QSpinBox(serverBox);
    m_port->setRange(1, 65535);
    m_port->setValue(8000);
    serverForm->addRow(tr("Port:"), m_port);

    m_mount = new QLineEdit(serverBox);
    m_mount->setPlaceholderText(QStringLiteral("/stream"));
    serverForm->addRow(tr("Mount point:"), m_mount);

    m_user = new QLineEdit(serverBox);
    m_user->setPlaceholderText(QStringLiteral("source"));
    serverForm->addRow(tr("User name:"), m_user);

    m_password = new QLineEdit(serverBox);
    m_password->setEchoMode(QLineEdit::Password);
    serverForm->addRow(tr("Password:"), m_password);

    m_usePut = new QCheckBox(tr("Use HTTP PUT instead of SOURCE"), serverBox);
    m_usePut->setToolTip(tr("Icecast 2.4.1 and later prefer PUT. If the server "
                            "refuses the source connection, try the other one."));
    serverForm->addRow(m_usePut);

    m_codec = new QComboBox(serverBox);
    m_codec->addItem(tr("MP3 (libmp3lame)"),
                     StreamEncoder::codecName(StreamEncoder::Codec::Mp3));
    m_codec->addItem(tr("Opus (Ogg)"),
                     StreamEncoder::codecName(StreamEncoder::Codec::Opus));
    serverForm->addRow(tr("Codec:"), m_codec);

    m_bitrate = new QComboBox(serverBox);
    serverForm->addRow(tr("Bitrate:"), m_bitrate);
    connect(m_codec, &QComboBox::currentIndexChanged, this, [this](int) {
        updateBitrateChoices();
    });

    formColumn->addWidget(serverBox);

    auto *metaBox = new QGroupBox(tr("What listeners and directories see"), this);
    auto *metaForm = new QFormLayout(metaBox);

    m_stationName = new QLineEdit(metaBox);
    metaForm->addRow(tr("Station name:"), m_stationName);
    m_genre = new QLineEdit(metaBox);
    metaForm->addRow(tr("Genre:"), m_genre);
    m_description = new QLineEdit(metaBox);
    metaForm->addRow(tr("Description:"), m_description);
    m_url = new QLineEdit(metaBox);
    m_url->setPlaceholderText(QStringLiteral("https://"));
    metaForm->addRow(tr("Web site:"), m_url);
    m_public = new QCheckBox(tr("List this mount in public directories"), metaBox);
    metaForm->addRow(m_public);

    formColumn->addWidget(metaBox);
    formColumn->addStretch();
    middle->addLayout(formColumn, 1);

    // -------------------------------------------------------- going on air --
    m_autoStart = new QCheckBox(tr("Go on air automatically when XFB starts"), this);
    m_autoStart->setChecked(StreamService::autoStartEnabled());
    connect(m_autoStart, &QCheckBox::toggled, this, [](bool on) {
        StreamService::setAutoStartEnabled(on);
    });
    outer->addWidget(m_autoStart);

    auto *actions = new QHBoxLayout;
    m_applyButton = new QPushButton(tr("Save settings"), this);
    connect(m_applyButton, &QPushButton::clicked, this, &StreamDialog::applyChanges);
    m_onAirButton = new QPushButton(tr("Go on air"), this);
    connect(m_onAirButton, &QPushButton::clicked, this, &StreamDialog::toggleOnAir);
    actions->addWidget(m_applyButton);
    actions->addWidget(m_onAirButton);
    actions->addStretch();
    outer->addLayout(actions);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    outer->addWidget(m_statusLabel);

    m_nowPlayingLabel = new QLabel(this);
    m_nowPlayingLabel->setWordWrap(true);
    outer->addWidget(m_nowPlayingLabel);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);   // a station runs for weeks
    m_log->setAccessibleName(tr("Streaming log"));
    m_log->setMinimumHeight(120);
    outer->addWidget(m_log);

    auto *close = new QPushButton(tr("Close"), this);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    auto *closeRow = new QHBoxLayout;
    closeRow->addStretch();
    closeRow->addWidget(close);
    outer->addLayout(closeRow);

    m_removeButton->setEnabled(false);
    m_duplicateButton->setEnabled(false);
    updateBitrateChoices();
    resize(820, 720);
}

void StreamDialog::updateBitrateChoices()
{
    const StreamEncoder::Codec codec =
        StreamEncoder::codecFromName(m_codec->currentData().toString());
    const int previous = m_bitrate->currentData().isValid()
                             ? m_bitrate->currentData().toInt()
                             : 128;

    const bool wasLoading = m_loading;
    m_loading = true;
    m_bitrate->clear();
    int bestRow = 0;
    const QList<int> rates = StreamEncoder::bitratesFor(codec);
    for (int i = 0; i < rates.size(); ++i) {
        m_bitrate->addItem(tr("%1 kbit/s").arg(rates.at(i)), rates.at(i));
        if (rates.at(i) == previous)
            bestRow = i;
        else if (rates.at(i) <= previous)
            bestRow = i;   // nearest lower, when the exact rate is gone
    }
    m_bitrate->setCurrentIndex(bestRow);
    m_loading = wasLoading;
}

// --------------------------------------------------------------- mount list

void StreamDialog::refreshList()
{
    const int keep = m_current;
    const bool wasLoading = m_loading;
    m_loading = true;
    m_list->clear();

    const QVector<IcecastSource::State> states =
        m_service ? m_service->mountStates() : QVector<IcecastSource::State>();

    for (int i = 0; i < m_mounts.size(); ++i) {
        const StreamService::Mount &mount = m_mounts.at(i);
        QString label = mount.name.isEmpty() ? mount.server.mount : mount.name;
        if (!mount.enabled)
            label += tr(" (off)");
        else if (i < states.size() && states.at(i) == IcecastSource::State::Connected)
            label += tr(" - on air");
        else if (i < states.size() && states.at(i) == IcecastSource::State::Connecting)
            label += tr(" - connecting");
        m_list->addItem(label);
    }

    if (keep >= 0 && keep < m_mounts.size())
        m_list->setCurrentRow(keep);
    m_loading = wasLoading;
}

void StreamDialog::selectMount(int row)
{
    // refreshList() rebuilds the rows, which makes the list emit
    // currentRowChanged twice on its own. Acting on that would reload the
    // form from the stored mount and throw away whatever the operator was
    // half-way through typing — and status refreshes are frequent.
    if (m_loading || row == m_current)
        return;
    if (m_current >= 0 && m_current < m_mounts.size())
        commitCurrent();

    m_current = row;
    const bool valid = (row >= 0 && row < m_mounts.size());
    if (valid)
        loadIntoForm(m_mounts.at(row));
    m_removeButton->setEnabled(valid);
    m_duplicateButton->setEnabled(valid);
}

void StreamDialog::addMount()
{
    commitCurrent();
    m_mounts.append(StreamService::defaultMount());
    m_current = -1;                 // force selectMount() to load the new one
    refreshList();
    m_list->setCurrentRow(m_mounts.size() - 1);
}

void StreamDialog::duplicateMount()
{
    if (m_current < 0 || m_current >= m_mounts.size())
        return;
    commitCurrent();

    // The point of a second mount is usually the same server at a different
    // codec, so copy everything and let the operator change the two fields
    // that differ.
    StreamService::Mount copy = m_mounts.at(m_current);
    copy.name = tr("%1 (copy)").arg(copy.name);
    copy.server.mount += QStringLiteral("2");
    m_mounts.append(copy);
    m_current = -1;
    refreshList();
    m_list->setCurrentRow(m_mounts.size() - 1);
}

void StreamDialog::removeMount()
{
    if (m_current < 0 || m_current >= m_mounts.size())
        return;
    const QString label = m_mounts.at(m_current).name;
    if (QMessageBox::question(this, tr("Remove mount"),
                              tr("Remove the mount \"%1\"?").arg(label))
            != QMessageBox::Yes) {
        return;
    }
    m_mounts.remove(m_current);
    m_current = -1;
    refreshList();
    if (!m_mounts.isEmpty())
        m_list->setCurrentRow(0);
    else
        loadIntoForm(StreamService::defaultMount());
}

// -------------------------------------------------------------------- form

void StreamDialog::loadIntoForm(const StreamService::Mount &mount)
{
    m_loading = true;
    m_enabled->setChecked(mount.enabled);
    m_name->setText(mount.name);
    m_host->setText(mount.server.host);
    m_port->setValue(mount.server.port);
    m_mount->setText(mount.server.mount);
    m_user->setText(mount.server.user);
    m_password->setText(mount.server.password);
    m_usePut->setChecked(mount.server.usePut);

    const int codecRow = m_codec->findData(
        StreamEncoder::codecName(mount.encoder.codec));
    m_codec->setCurrentIndex(codecRow < 0 ? 0 : codecRow);
    updateBitrateChoices();
    const int bitrateRow = m_bitrate->findData(mount.encoder.bitrateKbps);
    if (bitrateRow >= 0)
        m_bitrate->setCurrentIndex(bitrateRow);

    m_stationName->setText(mount.server.name);
    m_genre->setText(mount.server.genre);
    m_description->setText(mount.server.description);
    m_url->setText(mount.server.url);
    m_public->setChecked(mount.server.isPublic);
    m_loading = false;
}

void StreamDialog::storeFromForm(StreamService::Mount &mount) const
{
    mount.enabled = m_enabled->isChecked();
    mount.name = m_name->text().trimmed();
    mount.server.host = m_host->text().trimmed();
    mount.server.port = static_cast<quint16>(m_port->value());
    mount.server.mount = m_mount->text().trimmed();
    mount.server.user = m_user->text().trimmed();
    mount.server.password = m_password->text();
    mount.server.usePut = m_usePut->isChecked();

    mount.encoder.codec =
        StreamEncoder::codecFromName(m_codec->currentData().toString());
    mount.encoder.bitrateKbps = m_bitrate->currentData().toInt();
    // Kept in step deliberately: a mount whose announced Content-Type does
    // not match its codec plays as noise.
    mount.server.contentType = StreamEncoder::contentTypeFor(mount.encoder.codec);
    mount.server.bitrateKbps = mount.encoder.bitrateKbps;

    mount.server.name = m_stationName->text().trimmed();
    mount.server.genre = m_genre->text().trimmed();
    mount.server.description = m_description->text().trimmed();
    mount.server.url = m_url->text().trimmed();
    mount.server.isPublic = m_public->isChecked();

    if (mount.name.isEmpty())
        mount.name = mount.server.mount;
}

void StreamDialog::commitCurrent()
{
    if (m_loading || m_current < 0 || m_current >= m_mounts.size())
        return;
    storeFromForm(m_mounts[m_current]);
}

void StreamDialog::applyChanges()
{
    commitCurrent();
    if (!m_service)
        return;
    m_service->setMounts(m_mounts);
    refreshList();
    appendLog(tr("Settings saved."));
    emit announcementRequested(tr("Stream settings saved"));
}

// ---------------------------------------------------------------- on air

void StreamDialog::toggleOnAir()
{
    if (!m_service)
        return;

    if (m_service->isActive()) {
        m_service->stop();
        emit announcementRequested(tr("Streaming stopped"));
        return;
    }

    commitCurrent();

    // Saving first is not a nicety: the service streams what it was given,
    // and an operator who edited the password and pressed "Go on air"
    // means the new password.
    m_service->setMounts(m_mounts);
    refreshList();

    bool anyEnabled = false;
    for (const StreamService::Mount &mount : m_mounts) {
        if (mount.enabled && !mount.server.host.trimmed().isEmpty()) {
            anyEnabled = true;
            break;
        }
    }
    if (!anyEnabled) {
        QMessageBox::warning(this, tr("Nothing to stream to"),
                             tr("Enable at least one mount and give it a host "
                                "before going on air."));
        return;
    }

    m_service->start();
    emit announcementRequested(tr("Streaming started"));
}

void StreamDialog::refreshStatus()
{
    if (!m_service)
        return;

    const bool active = m_service->isActive();
    m_onAirButton->setText(active ? tr("Go off air") : tr("Go on air"));

    const QVector<IcecastSource::State> states = m_service->mountStates();
    const QVector<QString> errors = m_service->mountErrors();

    QStringList lines;
    for (int i = 0; i < m_mounts.size() && i < states.size(); ++i) {
        const StreamService::Mount &mount = m_mounts.at(i);
        if (!mount.enabled)
            continue;
        QString what;
        switch (states.at(i)) {
        case IcecastSource::State::Connected:
            what = tr("on air");
            break;
        case IcecastSource::State::Connecting:
            what = tr("connecting");
            break;
        case IcecastSource::State::Disconnected:
            what = errors.value(i).isEmpty() ? tr("off") : errors.value(i);
            break;
        }
        lines << QStringLiteral("<b>%1</b>: <span style=\"color:%2;\">%3</span>")
                     .arg(mount.name.toHtmlEscaped(),
                          stateColour(states.at(i)),
                          what.toHtmlEscaped());
    }

    if (!active)
        m_statusLabel->setText(tr("Not streaming."));
    else if (lines.isEmpty())
        m_statusLabel->setText(tr("Streaming, but no mount is enabled."));
    else
        m_statusLabel->setText(lines.join(QStringLiteral("<br>")));

    refreshList();
}

void StreamDialog::appendLog(const QString &message)
{
    m_log->appendPlainText(QStringLiteral("%1  %2")
                               .arg(QDateTime::currentDateTime().toString("hh:mm:ss"),
                                    message));
}
