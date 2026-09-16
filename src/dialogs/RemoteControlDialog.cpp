#include "RemoteControlDialog.h"

#include "../services/RemoteControlServer.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

constexpr int kMaxActivityLines = 200;

QString shortTime(const QDateTime &when)
{
    return when.isValid() ? QLocale().toString(when, QLocale::ShortFormat) : QString();
}

} // namespace

RemoteControlDialog::RemoteControlDialog(RemoteControlServer *server, QWidget *parent)
    : QDialog(parent)
    , m_server(server)
{
    setWindowTitle(tr("Remote Control"));
    setModal(false);

    auto *layout = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Let other programs control this station over the network — a Stream "
           "Deck, a home-automation panel, a script or a web page of your own. "
           "They can see what is on air and the running order, and, with a "
           "control key, play, stop, skip, change the volume and the running "
           "order, and start the recording or the stream."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    // Said where the decision is made, as in the phone sync window.
    auto *notice = new QLabel(
        tr("The connection is not encrypted. A key keeps strangers out, but "
           "anybody who can watch this network can read the traffic, key "
           "included. Use it on the station's own network, or choose \"This "
           "computer only\" and put an SSH tunnel or a TLS proxy in front."), this);
    notice->setWordWrap(true);
    QFont small = notice->font();
    small.setPointSizeF(small.pointSizeF() * 0.92);
    notice->setFont(small);
    notice->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(notice);

    // --- the server -------------------------------------------------------
    auto *serverBox = new QGroupBox(tr("Server"), this);
    auto *serverForm = new QFormLayout(serverBox);

    m_enabled = new QCheckBox(tr("&Accept remote control"), serverBox);
    m_enabled->setToolTip(tr("Remembered: XFB listens again the next time it starts."));
    serverForm->addRow(m_enabled);

    m_bind = new QComboBox(serverBox);
    m_bind->addItem(tr("Every network connection"), false);
    m_bind->addItem(tr("This computer only"), true);
    m_bind->setCurrentIndex(m_bind->findData(RemoteControlServer::localOnlySetting()));
    serverForm->addRow(tr("&Listen on:"), m_bind);

    m_port = new QSpinBox(serverBox);
    m_port->setRange(1024, 65535);
    m_port->setValue(RemoteControlServer::portSetting());
    serverForm->addRow(tr("&Port:"), m_port);

    m_status = new QLabel(serverBox);
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    serverForm->addRow(tr("Status:"), m_status);

    layout->addWidget(serverBox);

    // --- keys ---------------------------------------------------------------
    auto *keysBox = new QGroupBox(tr("Keys"), this);
    auto *keysLayout = new QVBoxLayout(keysBox);

    auto *keysIntro = new QLabel(
        tr("Each program gets a key of its own, so one can be revoked without "
           "touching the others. A read key can only look; a control key can "
           "change what goes on air."), keysBox);
    keysIntro->setWordWrap(true);
    keysLayout->addWidget(keysIntro);

    m_keys = new QTreeWidget(keysBox);
    m_keys->setRootIsDecorated(false);
    m_keys->setUniformRowHeights(true);
    m_keys->setHeaderLabels({tr("Name"), tr("Access"), tr("Key"), tr("Created"), tr("Last used")});
    m_keys->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_keys->setAccessibleName(tr("Remote control keys"));
    m_keys->setMinimumHeight(110);
    keysLayout->addWidget(m_keys);

    auto *keyButtons = new QHBoxLayout;
    auto *newKey = new QPushButton(tr("&New key..."), keysBox);
    m_revoke = new QPushButton(tr("&Revoke"), keysBox);
    m_revoke->setEnabled(false);
    keyButtons->addWidget(newKey);
    keyButtons->addWidget(m_revoke);
    keyButtons->addStretch(1);
    keysLayout->addLayout(keyButtons);

    m_example = new QLabel(keysBox);
    m_example->setWordWrap(true);
    m_example->setTextFormat(Qt::PlainText);
    m_example->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    keysLayout->addWidget(m_example);

    layout->addWidget(keysBox, 1);

    // --- activity -----------------------------------------------------------
    auto *activityBox = new QGroupBox(tr("Activity"), this);
    auto *activityLayout = new QVBoxLayout(activityBox);
    m_activity = new QListWidget(activityBox);
    m_activity->setAccessibleName(tr("Remote control activity"));
    m_activity->setMinimumHeight(90);
    activityLayout->addWidget(m_activity);
    layout->addWidget(activityBox, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    connect(m_enabled, &QCheckBox::toggled, this, &RemoteControlDialog::setServerEnabled);
    connect(m_bind, &QComboBox::currentIndexChanged, this, [this]() {
        RemoteControlServer::setLocalOnlySetting(m_bind->currentData().toBool());
        restartIfRunning();
    });
    connect(m_port, &QSpinBox::editingFinished, this, [this]() {
        if (quint16(m_port->value()) == RemoteControlServer::portSetting())
            return;
        RemoteControlServer::setPortSetting(quint16(m_port->value()));
        restartIfRunning();
    });
    connect(newKey, &QPushButton::clicked, this, &RemoteControlDialog::createKey);
    connect(m_revoke, &QPushButton::clicked, this, &RemoteControlDialog::revokeSelectedKey);
    connect(m_keys, &QTreeWidget::itemSelectionChanged, this, [this]() {
        m_revoke->setEnabled(!m_keys->selectedItems().isEmpty());
    });

    connect(m_server, &RemoteControlServer::started, this, [this]() {
        m_lastError.clear();
        refresh();
    });
    connect(m_server, &RemoteControlServer::stopped, this, &RemoteControlDialog::refresh);
    connect(m_server, &RemoteControlServer::errorOccurred, this, [this](const QString &message) {
        m_lastError = message;
        refresh();
    });
    connect(m_server, &RemoteControlServer::commandExecuted, this,
            [this](const QString &keyName, const QString &command, int status) {
        addActivity(status == 200 ? tr("%1: %2").arg(keyName, command)
                                  : tr("%1: %2 (refused, %3)").arg(keyName, command, QString::number(status)));
        refreshKeys();  // last used
    });
    connect(m_server, &RemoteControlServer::addressBlocked, this, [this](const QString &address) {
        const QString line = tr("%1 was refused for five minutes after too many wrong keys").arg(address);
        addActivity(line);
        emit announcementRequested(line);
    });

    refresh();
    resize(640, 700);
}

void RemoteControlDialog::setServerEnabled(bool enabled)
{
    if (!enabled) {
        m_server->stop();
        RemoteControlServer::setEnabledSetting(false);
        emit announcementRequested(tr("Remote control switched off"));
        refresh();
        return;
    }

    if (m_server->start()) {
        RemoteControlServer::setEnabledSetting(true);
        emit announcementRequested(tr("Remote control switched on"));
    } else {
        // Not remembered as on: a port that is taken now will likely be taken
        // at the next start as well, and failing silently there is worse.
        RemoteControlServer::setEnabledSetting(false);
        emit announcementRequested(tr("Remote control could not start: %1").arg(m_lastError));
    }
    refresh();
}

void RemoteControlDialog::restartIfRunning()
{
    if (!m_server->isListening()) {
        refresh();
        return;
    }
    m_server->stop();
    if (!m_server->start())
        RemoteControlServer::setEnabledSetting(false);
    refresh();
}

void RemoteControlDialog::createKey()
{
    QDialog ask(this);
    ask.setWindowTitle(tr("New Key"));
    auto *form = new QFormLayout(&ask);

    auto *name = new QLineEdit(&ask);
    name->setPlaceholderText(tr("Studio Stream Deck"));
    form->addRow(tr("&Name:"), name);

    auto *access = new QComboBox(&ask);
    access->addItem(tr("Read only — status, running order, library"),
                    RemoteControlServer::scopeName(RemoteControlServer::Scope::Read));
    access->addItem(tr("Control — can change what goes on air"),
                    RemoteControlServer::scopeName(RemoteControlServer::Scope::Control));
    form->addRow(tr("&Access:"), access);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &ask);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("&Make the key"));
    connect(buttons, &QDialogButtonBox::accepted, &ask, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &ask, &QDialog::reject);
    form->addRow(buttons);

    if (ask.exec() != QDialog::Accepted)
        return;

    const QString key = m_server->createKey(
        name->text(), RemoteControlServer::scopeFromName(access->currentData().toString()));
    refreshKeys();

    // The one and only time the key exists outside the program using it.
    QDialog show(this);
    show.setWindowTitle(tr("Your New Key"));
    auto *showLayout = new QVBoxLayout(&show);
    auto *warning = new QLabel(
        tr("Copy this key into the program that will use it now. XFB keeps only a "
           "fingerprint of it and cannot show it again; if it is lost, revoke it "
           "and make another."), &show);
    warning->setWordWrap(true);
    showLayout->addWidget(warning);

    auto *field = new QLineEdit(key, &show);
    field->setReadOnly(true);
    field->setAccessibleName(tr("New key"));
    QFont mono = field->font();
    mono.setFamily(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    field->setFont(mono);
    field->setMinimumWidth(field->fontMetrics().horizontalAdvance(key) + 24);
    showLayout->addWidget(field);

    auto *showButtons = new QDialogButtonBox(QDialogButtonBox::Close, &show);
    auto *copy = showButtons->addButton(tr("&Copy"), QDialogButtonBox::ActionRole);
    connect(copy, &QPushButton::clicked, &show, [key, copy]() {
        QApplication::clipboard()->setText(key);
        copy->setText(tr("Copied"));
    });
    connect(showButtons, &QDialogButtonBox::rejected, &show, &QDialog::reject);
    showLayout->addWidget(showButtons);

    field->selectAll();
    field->setFocus();
    show.exec();
}

void RemoteControlDialog::revokeSelectedKey()
{
    const QList<QTreeWidgetItem *> selected = m_keys->selectedItems();
    if (selected.isEmpty())
        return;
    const QString id = selected.first()->data(0, Qt::UserRole).toString();
    const QString name = selected.first()->text(0);

    if (QMessageBox::question(this, tr("Revoke Key"),
                              tr("Revoke \"%1\"? Whatever uses it stops working at once.").arg(name))
        != QMessageBox::Yes)
        return;

    m_server->revokeKey(id);
    addActivity(tr("Key \"%1\" revoked").arg(name));
    emit announcementRequested(tr("Key %1 revoked").arg(name));
    refresh();
}

void RemoteControlDialog::refresh()
{
    const bool listening = m_server->isListening();
    {
        const QSignalBlocker blocker(m_enabled);
        m_enabled->setChecked(listening);
    }

    if (listening) {
        const QStringList addresses = m_server->listenAddresses();
        QString text = tr("Listening on %1").arg(addresses.isEmpty()
                                                     ? tr("port %1").arg(m_server->port())
                                                     : addresses.join(QStringLiteral(", ")));
        if (m_server->keys().isEmpty())
            text += QLatin1Char('\n') + tr("There is no key yet, so nothing can use it until you make one.");
        m_status->setText(text);
    } else if (!m_lastError.isEmpty()) {
        m_status->setText(tr("Off — %1").arg(m_lastError));
    } else {
        m_status->setText(tr("Off"));
    }

    const QStringList addresses = m_server->listenAddresses();
    const QString base = addresses.isEmpty()
        ? QStringLiteral("http://127.0.0.1:%1").arg(RemoteControlServer::portSetting())
        : addresses.first();
    m_example->setText(tr("Try it:  curl -H \"Authorization: Bearer <key>\" %1/api/v1/status")
                           .arg(base));

    refreshKeys();
}

void RemoteControlDialog::refreshKeys()
{
    const QString selectedId = m_keys->selectedItems().isEmpty()
        ? QString() : m_keys->selectedItems().first()->data(0, Qt::UserRole).toString();

    m_keys->clear();
    for (const RemoteControlServer::ApiKey &key : m_server->keys()) {
        auto *item = new QTreeWidgetItem(m_keys);
        item->setText(0, key.name);
        item->setText(1, key.scope == RemoteControlServer::Scope::Control ? tr("Control") : tr("Read only"));
        item->setText(2, QStringLiteral("xfb_%1…").arg(key.id));
        item->setText(3, shortTime(key.created));
        item->setText(4, key.lastUsed.isValid() ? shortTime(key.lastUsed) : tr("Never"));
        item->setData(0, Qt::UserRole, key.id);
        if (key.id == selectedId)
            item->setSelected(true);
    }
    for (int column = 1; column < m_keys->columnCount(); ++column)
        m_keys->resizeColumnToContents(column);
    m_revoke->setEnabled(!m_keys->selectedItems().isEmpty());
}

void RemoteControlDialog::addActivity(const QString &line)
{
    m_activity->insertItem(0, QStringLiteral("%1  %2")
                                  .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")), line));
    while (m_activity->count() > kMaxActivityLines)
        delete m_activity->takeItem(m_activity->count() - 1);
}
