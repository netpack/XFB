#include "DeadAirDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

DeadAirDialog::DeadAirDialog(DeadAirWatchdog *watchdog, QWidget *parent)
    : QDialog(parent)
    , m_watchdog(watchdog)
{
    setWindowTitle(tr("Dead-Air Watchdog"));
    setModal(false);   // the operator keeps working while the station is on air

    buildUi();
    if (m_watchdog) {
        loadInto(m_watchdog->config());
        connect(m_watchdog, &DeadAirWatchdog::stateChanged,
                this, &DeadAirDialog::refreshStatus);
    }

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &DeadAirDialog::refreshStatus);
    m_refreshTimer->start();

    previewFallback();
    refreshStatus();
}

void DeadAirDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Silence is the one fault a station cannot survive quietly: the "
           "transport still says \"playing\", the clock still runs, and "
           "nobody in the building hears anything because the monitors are "
           "fed from the same dead output. When XFB stops making sound, this "
           "puts something on and tells you."), this);
    intro->setWordWrap(true);
    outer->addWidget(intro);

    // ------------------------------------------------------------- listening --
    auto *listenBox = new QGroupBox(tr("What counts as dead air"), this);
    auto *listenForm = new QFormLayout(listenBox);

    m_enabled = new QCheckBox(tr("Watch for dead air"), listenBox);
    m_enabled->setAccessibleName(tr("Watch for dead air"));
    listenForm->addRow(m_enabled);

    m_silenceDb = new QDoubleSpinBox(listenBox);
    m_silenceDb->setRange(-90.0, -10.0);
    m_silenceDb->setDecimals(0);
    m_silenceDb->setSingleStep(1.0);
    m_silenceDb->setSuffix(tr(" dBFS"));
    m_silenceDb->setAccessibleName(tr("Silence threshold in dBFS"));
    listenForm->addRow(tr("Silence below:"), m_silenceDb);

    m_tripSeconds = new QSpinBox(listenBox);
    m_tripSeconds->setRange(3, 600);
    m_tripSeconds->setSuffix(tr(" s"));
    m_tripSeconds->setAccessibleName(tr("Seconds of silence before the fallback starts"));
    listenForm->addRow(tr("For longer than:"), m_tripSeconds);

    m_recoverSeconds = new QSpinBox(listenBox);
    m_recoverSeconds->setRange(2, 600);
    m_recoverSeconds->setSuffix(tr(" s"));
    m_recoverSeconds->setAccessibleName(tr("Seconds of healthy audio that end an incident"));
    listenForm->addRow(tr("Audio is back after:"), m_recoverSeconds);

    m_rearmSeconds = new QSpinBox(listenBox);
    m_rearmSeconds->setRange(10, 3600);
    m_rearmSeconds->setSuffix(tr(" s"));
    m_rearmSeconds->setToolTip(
        tr("After an incident the watchdog stays quiet for this long, so a "
           "station that keeps failing is rescued once and reported once "
           "rather than a hundred times."));
    m_rearmSeconds->setAccessibleName(tr("Quiet period between two rescues"));
    listenForm->addRow(tr("Do not act again for:"), m_rearmSeconds);

    m_watchStopped = new QCheckBox(
        tr("A loaded running order with nothing playing is dead air too"),
        listenBox);
    m_watchStopped->setToolTip(
        tr("Pressing Stop yourself never trips the watchdog; this is about "
           "playback that ended on its own and never came back."));
    listenForm->addRow(m_watchStopped);

    auto *note = new QLabel(
        tr("Silence is measured from the level meter, which only runs while "
           "the FX engine is the active audio path. With plain passthrough "
           "playback there is nothing to measure and only a frozen transport "
           "will trip the watchdog."), listenBox);
    note->setWordWrap(true);
    listenForm->addRow(note);

    outer->addWidget(listenBox);

    // -------------------------------------------------------------- fallback --
    auto *fallbackBox = new QGroupBox(tr("What to put on"), this);
    auto *fallbackForm = new QFormLayout(fallbackBox);

    auto *playlistRow = new QHBoxLayout;
    m_playlist = new QLineEdit(fallbackBox);
    m_playlist->setPlaceholderText(tr("A saved XFB playlist (.xml)"));
    m_playlist->setAccessibleName(tr("Fallback playlist file"));
    auto *playlistBrowse = new QPushButton(tr("Browse..."), fallbackBox);
    connect(playlistBrowse, &QPushButton::clicked, this, &DeadAirDialog::browsePlaylist);
    playlistRow->addWidget(m_playlist, 1);
    playlistRow->addWidget(playlistBrowse);
    fallbackForm->addRow(tr("Playlist:"), playlistRow);

    auto *folderRow = new QHBoxLayout;
    m_folder = new QLineEdit(fallbackBox);
    m_folder->setPlaceholderText(tr("A folder of evergreen tracks"));
    m_folder->setAccessibleName(tr("Fallback folder"));
    auto *folderBrowse = new QPushButton(tr("Browse..."), fallbackBox);
    connect(folderBrowse, &QPushButton::clicked, this, &DeadAirDialog::browseFolder);
    folderRow->addWidget(m_folder, 1);
    folderRow->addWidget(folderBrowse);
    fallbackForm->addRow(tr("Or folder:"), folderRow);

    m_shuffle = new QCheckBox(tr("Play the folder in a random order"), fallbackBox);
    fallbackForm->addRow(m_shuffle);

    m_trackCount = new QSpinBox(fallbackBox);
    m_trackCount->setRange(1, 200);
    m_trackCount->setAccessibleName(tr("How many fallback tracks to queue"));
    fallbackForm->addRow(tr("Queue at most:"), m_trackCount);

    m_preview = new QLabel(fallbackBox);
    m_preview->setWordWrap(true);
    fallbackForm->addRow(m_preview);

    connect(m_playlist, &QLineEdit::textChanged, this, &DeadAirDialog::previewFallback);
    connect(m_folder, &QLineEdit::textChanged, this, &DeadAirDialog::previewFallback);
    connect(m_trackCount, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DeadAirDialog::previewFallback);

    outer->addWidget(fallbackBox);

    // ----------------------------------------------------------------- alerts --
    auto *alertBox = new QGroupBox(tr("Telling somebody"), this);
    auto *alertLayout = new QVBoxLayout(alertBox);
    m_notifyScreen = new QCheckBox(tr("Put an alert on screen"), alertBox);
    m_notifyPhone = new QCheckBox(
        tr("Publish the incident for a paired phone to pick up"), alertBox);
    m_notifyPhone->setToolTip(
        tr("XFB's sync server is pull-only — it cannot reach out to a phone. "
           "The incident is offered on the sync server instead, and the "
           "companion app sees it the next time it looks."));
    alertLayout->addWidget(m_notifyScreen);
    alertLayout->addWidget(m_notifyPhone);
    outer->addWidget(alertBox);

    // ----------------------------------------------------------------- status --
    auto *statusBox = new QGroupBox(tr("Right now"), this);
    auto *statusLayout = new QVBoxLayout(statusBox);
    m_status = new QLabel(statusBox);
    m_incident = new QLabel(statusBox);
    m_incident->setWordWrap(true);
    m_clearButton = new QPushButton(tr("I have fixed it — clear the incident"), statusBox);
    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        if (m_watchdog)
            m_watchdog->clearIncident();
        refreshStatus();
    });
    statusLayout->addWidget(m_status);
    statusLayout->addWidget(m_incident);
    statusLayout->addWidget(m_clearButton);
    outer->addWidget(statusBox);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &DeadAirDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    outer->addWidget(buttons);
}

void DeadAirDialog::loadInto(const DeadAirWatchdog::Config &config)
{
    m_enabled->setChecked(config.enabled);
    m_silenceDb->setValue(config.silenceDb);
    m_tripSeconds->setValue(config.tripSeconds);
    m_recoverSeconds->setValue(config.recoverSeconds);
    m_rearmSeconds->setValue(config.rearmSeconds);
    m_watchStopped->setChecked(config.watchStopped);
    m_playlist->setText(config.fallbackPlaylist);
    m_folder->setText(config.fallbackFolder);
    m_shuffle->setChecked(config.shuffleFolder);
    m_trackCount->setValue(config.fallbackTracks);
    m_notifyScreen->setChecked(config.notifyScreen);
    m_notifyPhone->setChecked(config.notifyPhone);
}

DeadAirWatchdog::Config DeadAirDialog::readFromForm() const
{
    DeadAirWatchdog::Config config;
    config.enabled = m_enabled->isChecked();
    config.silenceDb = m_silenceDb->value();
    config.tripSeconds = m_tripSeconds->value();
    config.recoverSeconds = m_recoverSeconds->value();
    config.rearmSeconds = m_rearmSeconds->value();
    config.watchStopped = m_watchStopped->isChecked();
    config.fallbackPlaylist = m_playlist->text().trimmed();
    config.fallbackFolder = m_folder->text().trimmed();
    config.shuffleFolder = m_shuffle->isChecked();
    config.fallbackTracks = m_trackCount->value();
    config.notifyScreen = m_notifyScreen->isChecked();
    config.notifyPhone = m_notifyPhone->isChecked();
    return config;
}

void DeadAirDialog::apply()
{
    if (!m_watchdog)
        return;
    const DeadAirWatchdog::Config config = readFromForm();
    m_watchdog->setConfig(config);

    const QString said = config.enabled
        ? tr("Dead-air watchdog on: silence below %1 dBFS for %2 seconds "
             "starts the fallback.")
              .arg(config.silenceDb, 0, 'f', 0).arg(config.tripSeconds)
        : tr("Dead-air watchdog off.");
    emit announcementRequested(said);
    refreshStatus();
}

void DeadAirDialog::browsePlaylist()
{
    const QString chosen = QFileDialog::getOpenFileName(
        this, tr("Fallback playlist"),
        QFileInfo(m_playlist->text()).absolutePath(),
        tr("XFB playlists (*.xml)"));
    if (!chosen.isEmpty())
        m_playlist->setText(chosen);
}

void DeadAirDialog::browseFolder()
{
    const QString chosen = QFileDialog::getExistingDirectory(
        this, tr("Folder of evergreen tracks"), m_folder->text());
    if (!chosen.isEmpty())
        m_folder->setText(chosen);
}

void DeadAirDialog::previewFallback()
{
    const DeadAirWatchdog::Config config = readFromForm();
    const QStringList tracks = DeadAirWatchdog::resolveFallbackTracks(config);
    if (tracks.isEmpty()) {
        m_preview->setText(tr("Nothing to fall back on — the watchdog would "
                              "raise the alarm but could not put anything on."));
    } else {
        m_preview->setText(tr("%n track(s) ready, starting with \"%1\".", "",
                              tracks.size())
                               .arg(QFileInfo(tracks.first()).fileName()));
    }
}

void DeadAirDialog::refreshStatus()
{
    if (!m_watchdog) {
        m_status->setText(tr("The watchdog is not running."));
        m_clearButton->setEnabled(false);
        return;
    }

    QString state;
    switch (m_watchdog->airState()) {
    case DeadAirWatchdog::AirState::Off:      state = tr("idle"); break;
    case DeadAirWatchdog::AirState::Playing:  state = tr("on air"); break;
    case DeadAirWatchdog::AirState::Silent:   state = tr("silent"); break;
    case DeadAirWatchdog::AirState::Stalled:  state = tr("stalled"); break;
    case DeadAirWatchdog::AirState::Fallback: state = tr("running the fallback"); break;
    }

    const double level = m_watchdog->lastLevelDb();
    m_status->setText(tr("Station: %1 — last master peak %2 dBFS — watching for %3 s")
                          .arg(state)
                          .arg(level <= -119.0 ? tr("none") : QString::number(level, 'f', 1))
                          .arg(m_watchdog->uptimeSeconds()));

    const QString incident = m_watchdog->lastIncident();
    m_incident->setText(incident.isEmpty()
                            ? tr("No incident recorded since XFB started.")
                            : tr("Last incident: %1").arg(incident));
    m_clearButton->setEnabled(m_watchdog->isTripped());
}
