#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "QFile"
#include "QDebug"
#include "QTextStream"
#include "QProcess"
#include <QMessageBox>
#include <QtSql>
#include "commonFunctions.h"
#include "secretstore.h"
#include "audio/CueBus.h"
#include "audio/FxParams.h"
#include "IconTheme.h"
#include "ThemeManager.h"
#include "services/AccessControl.h"
#include <QDebug>
#include "player.h"
#include "externaldownloader.h"
#include <QAudio>
#include <QMediaRecorder>
#include <QMediaDevices> // Qt6 replacement for QAudioDeviceInfo
#include <QAudioInput> // Qt6 for audio input
#include <QAudioOutput> // Qt6 for audio output
#include <QAudioDevice> // Qt6 for audio device information
#include <QtMultimedia>
#include <QFileDialog>
#include <stdlib.h>
#include <math.h>
#include <QPainter>
#include <QVBoxLayout>
// QAudioDeviceInfo is deprecated in Qt6, already included QMediaDevices above
#include <QAudioInput>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QHeaderView>
#include <QStyle>
#include <QTcpSocket>
#include <QFontDialog>
#include <QUrl>
#include <QtWidgets>

optionsDialog::optionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::optionsDialog)
{
    ui->setupUi(this);

    audioRecorder = new QMediaRecorder(this);

    //Audio devices
    const QList<QAudioDevice> inputDevices = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : inputDevices) {
            ui->cboxRecDev->addItem(device.description(), QVariant(device.id()));
            qDebug()<<"Audio Hardware detected on this system (optionsdialog.cpp): "<<device.description();
        }

    //Audio codecs
    foreach (const QMediaFormat::AudioCodec &codec, audioRecorder->mediaFormat().supportedAudioCodecs(QMediaFormat::Encode)) {
            QString codecName = QMediaFormat::audioCodecName(codec);
            ui->comboBox_codec->addItem(codecName, QVariant(codecName));
            qDebug()<<"Audio Codecs on this system (optionsdialog.cpp): "<<QVariant(codecName);
        }

    //Containers
    foreach (const QMediaFormat::FileFormat &format, audioRecorder->mediaFormat().supportedFileFormats(QMediaFormat::Encode)) {
            QString containerName = QMediaFormat::fileFormatName(format);
            ui->comboBox_container->addItem(containerName, QVariant(containerName));
            qDebug()<<"Audio Containers on this system (optionsdialog.cpp): "<<QVariant(containerName);
        }
    // --- Load Settings using QSettings from WRITABLE Location ---
    qDebug() << "Loading settings using QSettings...";
    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = writableConfigPath + "/" + configFileName;

    QSettings settings(configFilePath, QSettings::IniFormat);
    qDebug() << "Reading settings from:" << settings.fileName();

    // -- General Tab --
    // Language. The item text is the language's own endonym (never translated,
    // so a speaker can always find their language) and the code is carried in
    // the item data, so matching never depends on the displayed string.
    ui->cbox_lang->clear();
    ui->cbox_lang->addItem(QStringLiteral("English"), QStringLiteral("en"));
    ui->cbox_lang->addItem(QStringLiteral("Français"), QStringLiteral("fr"));
    ui->cbox_lang->addItem(QStringLiteral("Português"), QStringLiteral("pt"));
    m_initialLanguage = settings.value("Language", "en").toString(); // Default "en"
    const int langIndex = ui->cbox_lang->findData(m_initialLanguage);
    ui->cbox_lang->setCurrentIndex(langIndex >= 0 ? langIndex : 0); // Default to English

    // General Checkboxes
    ui->checkBox_disableSeekBar->setChecked(settings.value("Disable_Seek_Bar", false).toBool());
    ui->checkBox_disableVolume->setChecked(settings.value("Disable_Volume", false).toBool());
    ui->checkBox_fullScreen->setChecked(settings.value("FullScreen", false).toBool());

    // Theme + accent color (ThemeManager owns the ids and defaults)
    ui->combo_theme->clear();
    const QStringList themeIds = ThemeManager::themeIds();
    for (const QString &themeId : themeIds)
        ui->combo_theme->addItem(ThemeManager::themeName(themeId), themeId);
    const int themeIndex = themeIds.indexOf(ThemeManager::configuredTheme());
    ui->combo_theme->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    m_accentColor = ThemeManager::configuredAccent();
    updateAccentButton();
    connect(ui->combo_theme, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { updateAccentButton(); });

    // Icon style. One set of artwork, drawn several ways (IconTheme owns the
    // recipes) — and each entry carries a sample drawn its own way, because
    // "Accent tinted" says nothing until you have seen it.
    ui->combo_iconTheme->clear();
    const QStringList iconThemeIds = IconTheme::themeIds();
    for (const QString &iconThemeId : iconThemeIds) {
        ui->combo_iconTheme->addItem(
            IconTheme::preview(iconThemeId, QStringLiteral(":/icons/flat/Radio-48.png")),
            IconTheme::themeName(iconThemeId), iconThemeId);
    }
    const int iconThemeIndex = iconThemeIds.indexOf(IconTheme::configuredTheme());
    ui->combo_iconTheme->setCurrentIndex(iconThemeIndex >= 0 ? iconThemeIndex : 0);
    ui->checkBox_enableTorrents->setChecked(settings.value("EnableTorrents", false).toBool());
    // The torrent feature is one an administrator hands out per role rather
    // than one every operator may switch on. Without "Support .torrent files"
    // the switch is not shown at all — a station that does not do this should
    // not have to explain a greyed-out box to its presenters. The value itself
    // is left alone and written back untouched on OK, so an operator who
    // cannot see the switch cannot turn the station's setting off either.
    ui->checkBox_enableTorrents->setVisible(
        AccessControl::instance().allows(QStringLiteral("downloads.torrents")));

    // The same for the whole Downloads tab: it is nothing but settings for the
    // downloader — the audio format it produces, where yt-dlp keeps itself — so
    // on a desk that does not have the downloader it is a tab of settings for a
    // feature the operator cannot reach. Removed rather than disabled; the page
    // is not deleted, so it comes back with the permission.
    if (!AccessControl::instance().allows(QStringLiteral("downloads.external"))) {
        const int ytdlpTabIndex = ui->SystemResouces->indexOf(ui->tab_ytdlp);
        if (ytdlpTabIndex != -1) {
            ui->SystemResouces->removeTab(ytdlpTabIndex);
            // removeTab() leaves the page parentless — hand it back to the
            // dialog so it is owned (and destroyed) with it.
            ui->tab_ytdlp->setParent(this);
            ui->tab_ytdlp->hide();
        }
    }
    ui->checkBox_showFxTab->setChecked(settings.value("ShowFxTab", true).toBool());
    ui->checkBox_showPadsTab->setChecked(settings.value("ShowPadsTab", true).toBool());
    ui->checkBox_autoAutoMix->setChecked(settings.value("AutoAutoMix", false).toBool());
    ui->checkBox_bpmMatch->setChecked(settings.value("AutoModeMatchBpm", false).toBool());
    ui->spin_bpmTolerance->setValue(
        qBound(1, settings.value("AutoModeBpmTolerance", 8).toInt(), 60));
    ui->spin_bpmTolerance->setEnabled(ui->checkBox_bpmMatch->isChecked());
    ui->label_bpmTolerance->setEnabled(ui->checkBox_bpmMatch->isChecked());
    connect(ui->checkBox_bpmMatch, &QCheckBox::toggled, this, [this](bool on) {
        ui->spin_bpmTolerance->setEnabled(on);
        ui->label_bpmTolerance->setEnabled(on);
    });
    // EBU R128 loudness normalisation. Live-applied through
    // player::updateConfig(), which this dialog's finished() signal drives.
    ui->checkBox_loudnessNormalize->setChecked(
        settings.value("LoudnessNormalize", false).toBool());
    ui->spin_loudnessTarget->setValue(
        qBound(-23.0, settings.value("LoudnessTargetLufs", -16.0).toDouble(), -9.0));
    ui->spin_loudnessCeiling->setValue(
        qBound(-9.0, settings.value("LoudnessCeilingDbTp", -1.0).toDouble(), 0.0));
    {
        const bool on = ui->checkBox_loudnessNormalize->isChecked();
        ui->spin_loudnessTarget->setEnabled(on);
        ui->spin_loudnessCeiling->setEnabled(on);
        ui->label_loudnessTarget->setEnabled(on);
        ui->label_loudnessCeiling->setEnabled(on);
    }
    connect(ui->checkBox_loudnessNormalize, &QCheckBox::toggled, this, [this](bool on) {
        ui->spin_loudnessTarget->setEnabled(on);
        ui->spin_loudnessCeiling->setEnabled(on);
        ui->label_loudnessTarget->setEnabled(on);
        ui->label_loudnessCeiling->setEnabled(on);
    });

    // Row numbers down the side of the four library lists. Jingles is the only
    // one that ever had them, so that is what the defaults say — anyone who
    // never opens this page sees exactly what they saw before.
    ui->checkBox_rowNumbersMusic->setChecked(
        settings.value("RowNumbers/Music", false).toBool());
    ui->checkBox_rowNumbersJingles->setChecked(
        settings.value("RowNumbers/Jingles", true).toBool());
    ui->checkBox_rowNumbersPub->setChecked(
        settings.value("RowNumbers/Pub", false).toBool());
    ui->checkBox_rowNumbersPrograms->setChecked(
        settings.value("RowNumbers/Programs", false).toBool());

    ui->checkBox_levelMeter->setChecked(settings.value("ShowLevelMeter", false).toBool());
    ui->combo_levelMeterPos->setCurrentIndex(
        settings.value("LevelMeterPlacement", "volume").toString() == "side" ? 1 : 0);
    ui->checkBox_retune432->setChecked(FxSettings::loadRetune432());

    // Application font size. 0/unset means "use the current default"; fall back
    // to the running app's point size, then to 10 pt.
    {
        int fontSize = settings.value("FontSize", 0).toInt();
        if (fontSize <= 0) {
            fontSize = QApplication::font().pointSize();
        }
        if (fontSize <= 0) {
            fontSize = 10;
        }
        ui->spin_fontSize->setValue(fontSize);
    }

    // The now-playing elapsed-time clock. Stored as QFont::toString(); an
    // empty value means "whatever XFB ships with", which is why the custom
    // flag is kept rather than comparing fonts.
    {
        const QString spec =
            settings.value(ThemeManager::nowPlayingClockFontKey()).toString();
        QFont picked;
        m_clockFontCustom = !spec.isEmpty() && picked.fromString(spec);
        m_clockFont = m_clockFontCustom
                          ? ThemeManager::clampNowPlayingClockFont(picked)
                          : ThemeManager::defaultNowPlayingClockFont();
        ui->bt_clockFont->setAccessibleName(tr("Choose the now-playing clock typeface"));
        ui->bt_clockFontReset->setAccessibleName(
            tr("Use the default now-playing clock typeface"));
        updateClockFontSample();
    }

    // -- Database Tab --
    // The file the open connection is actually using, not the "Database"
    // setting: on installs that predate the current layout that setting still
    // holds a relative path pointing at nothing, and an administrator reading
    // this tab needs the file they would back up or hand to support.
    txt_selected_db = databasePath();
    if (txt_selected_db.isEmpty())
        txt_selected_db = settings.value("Database").toString();
    ui->txt_selected_db->setText(txt_selected_db.isEmpty() ? tr("[NO DATABASE SET]")
                                                           : txt_selected_db);
    ui->txt_selected_db->setToolTip(txt_selected_db);
    ui->txt_selected_db->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // -- Recording and Paths Tab --
    // Find and set saved device/codec/container
    QString savedRecDeviceDesc = settings.value("RecDevice").toString();
    int recDevIndex = ui->cboxRecDev->findText(savedRecDeviceDesc);
    if (recDevIndex != -1) { ui->cboxRecDev->setCurrentIndex(recDevIndex); }
    else if (!inputDevices.isEmpty()) { ui->cboxRecDev->setCurrentIndex(0); qWarning() << "Saved RecDevice not found, using default:" << ui->cboxRecDev->currentText();}
    else { qWarning() << "No recording devices available to select."; }

    QVariant codecVariant = settings.value("RecCodec");
    if (codecVariant.isValid()) {
        QMediaFormat::AudioCodec savedCodec = codecVariant.value<QMediaFormat::AudioCodec>();
        int codecIndex = ui->comboBox_codec->findData(QVariant::fromValue(savedCodec));
        if (codecIndex != -1) { ui->comboBox_codec->setCurrentIndex(codecIndex); }
        else if (ui->comboBox_codec->count() > 0) { ui->comboBox_codec->setCurrentIndex(0); qWarning() << "Saved RecCodec not found, using default:" << ui->comboBox_codec->currentText();}
    } else if (ui->comboBox_codec->count() > 0) { ui->comboBox_codec->setCurrentIndex(0); } // Fallback if key doesn't exist

    QVariant containerVariant = settings.value("RecContainer");
    if (containerVariant.isValid()) {
        QMediaFormat::FileFormat savedFormat = containerVariant.value<QMediaFormat::FileFormat>();
        int formatIndex = ui->comboBox_container->findData(QVariant::fromValue(savedFormat));
        if (formatIndex != -1) { ui->comboBox_container->setCurrentIndex(formatIndex); }
        else if (ui->comboBox_container->count() > 0) { ui->comboBox_container->setCurrentIndex(0); qWarning() << "Saved RecContainer not found, using default:" << ui->comboBox_container->currentText();}
    } else if (ui->comboBox_container->count() > 0) { ui->comboBox_container->setCurrentIndex(0); } // Fallback

    // Paths
    ui->txt_savePath->setText(settings.value("SavePath").toString());
    ui->txt_programsPath->setText(settings.value("ProgramsPath").toString());
    ui->txt_musicPath->setText(settings.value("MusicPath").toString());
    ui->txt_jinglePath->setText(settings.value("JinglePath").toString());

    // -- Network Tab --
    ui->txt_FTPlocalTempFolder->setText(settings.value("FTPPath").toString()); // Moved FTP path here
    ui->txt_takeOverlocalTempFolder->setText(settings.value("TakeOverPath").toString()); // Moved TakeOver path here

    bool networkingEnabled = settings.value("Enable_Networking", false).toBool();
    ui->cbox_enableNetworking->setChecked(networkingEnabled);
    // Update enabled state based on checkbox AFTER setting its state
    on_cbox_enableNetworking_toggled(networkingEnabled); // Call slot directly

    ui->txt_server->setText(settings.value("Server_URL").toString());
    ui->txt_port->setText(settings.value("Port").toString());
    ui->txt_user->setText(settings.value("User").toString());
    ui->txt_password->setText(SecretStore::open(settings.value("Pass").toString()));
    ui->cbox_role->setCurrentText(settings.value("Role", "Client").toString());

    // Commercial Hour
    QString comHourString = settings.value("ComHour", "00:00:00").toString(); // Default time
    QTime comTime = QTime::fromString(comHourString, Qt::ISODate); // Use standard format
    if (!comTime.isValid()) {
        comTime = QTime::fromString(comHourString, "HH:mm:ss"); // Try legacy format
    }
    if (!comTime.isValid()) {
        comTime = QTime(0, 0, 0); // Fallback default
        qWarning() << "Could not parse ComHour from settings, using default:" << comHourString;
    }
    ui->cboxComHour->setTime(comTime);

    // -- System Resources Tab -- (No settings loaded here typically)

    // -- Downloads (yt-dlp) Tab --
    {
        QString fmt = settings.value("MusicFormat", "opus").toString().trimmed().toLower();
        int fmtIdx = ui->cbox_ytdlpFormat->findText(fmt);
        ui->cbox_ytdlpFormat->setCurrentIndex(fmtIdx >= 0 ? fmtIdx : 0);
    }
    ui->checkBox_ytdlpKeepVideo->setChecked(settings.value("MusicKeepVideo", false).toBool());
    ui->checkBox_ytdlpEmbedThumbnail->setChecked(settings.value("MusicEmbedThumbnail", false).toBool());
    ui->checkBox_ytdlpEmbedMetadata->setChecked(settings.value("MusicEmbedMetadata", true).toBool());

    // Spotify: optional, and only needed to read playlists longer than the 100
    // tracks the public pages hand out. Sealed like the other stored password.
    ui->txt_spotifyClientId->setText(settings.value("SpotifyClientId").toString());
    ui->txt_spotifyClientSecret->setText(
        SecretStore::open(settings.value("SpotifyClientSecret").toString()));

    // A QFormLayout label is not a buddy, so without these every path field
    // and every "..." button announces itself as the group box it sits in.
    const struct { QWidget *field; const char *name; } named[] = {
        { ui->cboxRecDev,        QT_TR_NOOP("Recording device") },
        { ui->comboBox_codec,    QT_TR_NOOP("Recording codec") },
        { ui->comboBox_container,QT_TR_NOOP("Recording container") },
        { ui->txt_savePath,      QT_TR_NOOP("Folder recordings are saved in") },
        { ui->bt_browseSavePath, QT_TR_NOOP("Choose the folder recordings are saved in") },
        { ui->txt_programsPath,  QT_TR_NOOP("Programmes folder") },
        { ui->bt_browse_programPath, QT_TR_NOOP("Choose the programmes folder") },
        { ui->txt_musicPath,     QT_TR_NOOP("Music folder") },
        { ui->bt_browse_musicPath, QT_TR_NOOP("Choose the music folder") },
        { ui->txt_jinglePath,    QT_TR_NOOP("Jingles folder") },
        { ui->bt_browse_jinglePath, QT_TR_NOOP("Choose the jingles folder") },
        { ui->txt_server,        QT_TR_NOOP("Station server URL") },
        { ui->txt_port,          QT_TR_NOOP("Station server port") },
        { ui->txt_user,          QT_TR_NOOP("Station server user") },
        { ui->txt_password,      QT_TR_NOOP("Station server password") },
        { ui->cbox_role,         QT_TR_NOOP("This desk's role") },
        { ui->cboxComHour,       QT_TR_NOOP("Communications hour") },
        { ui->txt_FTPlocalTempFolder, QT_TR_NOOP("FTP temporary folder") },
        { ui->bt_browseFTPlocalFolder, QT_TR_NOOP("Choose the FTP temporary folder") },
        { ui->txt_takeOverlocalTempFolder, QT_TR_NOOP("TakeOver temporary folder") },
        { ui->bt_browseTakeOverlocalFolder, QT_TR_NOOP("Choose the TakeOver temporary folder") },
        { ui->txt_terminal,      QT_TR_NOOP("Diagnostics output") },
    };
    for (const auto &entry : named)
        entry.field->setAccessibleName(tr(entry.name));

    // Cue bus / output routing. Built in C++ rather than in the .ui file:
    // the device lists only exist at runtime anyway, and a .ui edit here
    // would mean regenerating ui_optionsdialog.h by hand.
    buildCueTab();

    // The Database tab opens already showing what is in there, rather than
    // making the operator press something to find out.
    ui->tbl_dbCounts->setAccessibleName(tr("What the database holds"));
    refreshDatabaseCounts();

    qDebug() << "Finished loading settings in options dialog.";
    // Dialog styling comes from the application-wide theme (ThemeManager)
}

// ---------------------------------------------------------------------------
// "Cue and outputs" tab
//
// Two device pickers — where XFB goes to air, and the private ear it cues
// into — plus the cue monitor level and the spoken-cue switches. Everything
// here is live-applied: the player re-reads the Cue/ group from the dialog's
// finished() signal, so a device swapped mid-show reaches the next cue.
// ---------------------------------------------------------------------------

void optionsDialog::fillDeviceCombo(QComboBox *combo, const QByteArray &storedId,
                                    bool allowSystemDefault)
{
    combo->clear();
    if (allowSystemDefault) {
        const QAudioDevice def = QMediaDevices::defaultAudioOutput();
        combo->addItem(def.isNull() ? tr("System default")
                                    : tr("System default (%1)").arg(def.description()),
                       QVariant(QByteArray()));
    } else {
        // The cue output has no "system default" entry on purpose: the
        // default is the output that goes to air, so offering it as the
        // private ear would be offering a way to put an audition to air.
        combo->addItem(tr("None — cueing off"), QVariant(QByteArray()));
    }

    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : devices) {
        QString label = device.description();
        if (device.maximumChannelCount() < 2)
            label = tr("%1 (mono)").arg(label);
        combo->addItem(label, QVariant(device.id()));
    }

    int index = combo->findData(QVariant(storedId));
    if (index < 0 && !storedId.isEmpty()) {
        // Configured but not plugged in right now: keep the choice visible
        // instead of silently resetting it to the default.
        combo->addItem(tr("%1 (not connected)").arg(QString::fromUtf8(storedId)),
                       QVariant(storedId));
        index = combo->count() - 1;
    }
    combo->setCurrentIndex(index < 0 ? 0 : index);
}

void optionsDialog::refreshCueWarning()
{
    if (!m_cueWarning || !m_cueOutputCombo || !m_mainOutputCombo)
        return;

    const QByteArray cueId = m_cueOutputCombo->currentData().toByteArray();
    const QByteArray mainId = m_mainOutputCombo->currentData().toByteArray();
    const QAudioDevice onAir = mainId.isEmpty() ? QMediaDevices::defaultAudioOutput()
                                                : QAudioDevice();
    const QByteArray resolvedMain = mainId.isEmpty() ? onAir.id() : mainId;

    if (cueId.isEmpty()) {
        m_cueWarning->setText(tr("Cueing is off. Pick a second output — headphones on "
                                 "another sound card — to audition tracks without "
                                 "putting them to air."));
    } else if (cueId == resolvedMain) {
        m_cueWarning->setText(tr("⚠ The cue output is the same device as the on-air "
                                 "output. Cueing stays disabled: an audition would go "
                                 "to air."));
    } else {
        m_cueWarning->setText(tr("Cue audio plays only on the cue output. It never "
                                 "reaches the on-air output or the stream."));
    }
    m_cueWarning->setAccessibleName(m_cueWarning->text());
}

void optionsDialog::buildCueTab()
{
    if (!ui->SystemResouces)
        return;

    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QSettings settings(writableConfigPath + "/" + configFileName, QSettings::IniFormat);

    auto *page = new QWidget(ui->SystemResouces);
    auto *outer = new QVBoxLayout(page);
    auto *form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_mainOutputCombo = new QComboBox(page);
    m_mainOutputCombo->setAccessibleName(tr("On-air output device"));
    m_mainOutputCombo->setToolTip(tr("Where the main player, the DJ decks, the pads "
                                     "and the stream player send their audio."));
    fillDeviceCombo(m_mainOutputCombo,
                    settings.value("Cue/MainOutputDevice").toByteArray(), true);
    form->addRow(tr("On-air output:"), m_mainOutputCombo);

    m_cueOutputCombo = new QComboBox(page);
    m_cueOutputCombo->setAccessibleName(tr("Cue output device (headphones)"));
    m_cueOutputCombo->setToolTip(tr("The private ear. Auditions and spoken cues play "
                                    "here and nowhere else."));
    fillDeviceCombo(m_cueOutputCombo,
                    settings.value("Cue/CueOutputDevice").toByteArray(), false);
    form->addRow(tr("Cue output:"), m_cueOutputCombo);

    auto *volumeRow = new QWidget(page);
    auto *volumeLayout = new QHBoxLayout(volumeRow);
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    m_cueVolume = new QSlider(Qt::Horizontal, volumeRow);
    m_cueVolume->setRange(0, 100);
    m_cueVolume->setValue(qBound(0, settings.value("Cue/Volume", 80).toInt(), 100));
    m_cueVolume->setAccessibleName(tr("Cue monitor level"));
    m_cueVolumeLabel = new QLabel(QString::number(m_cueVolume->value()) + "%", volumeRow);
    m_cueVolumeLabel->setAccessibleName(tr("Cue monitor level in percent"));
    m_cueVolumeLabel->setMinimumWidth(45);
    volumeLayout->addWidget(m_cueVolume);
    volumeLayout->addWidget(m_cueVolumeLabel);
    connect(m_cueVolume, &QSlider::valueChanged, this, [this](int v) {
        m_cueVolumeLabel->setText(QString::number(v) + "%");
    });
    form->addRow(tr("Cue level:"), volumeRow);

    outer->addLayout(form);

    m_cueWarning = new QLabel(page);
    m_cueWarning->setWordWrap(true);
    m_cueWarning->setAccessibleName(tr("Cue routing status"));
    outer->addWidget(m_cueWarning);
    connect(m_cueOutputCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { refreshCueWarning(); });
    connect(m_mainOutputCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { refreshCueWarning(); });

    auto *speechBox = new QGroupBox(tr("Spoken cues in the private ear"), page);
    auto *speechLayout = new QVBoxLayout(speechBox);

    m_cueSpeak = new QCheckBox(tr("Speak XFB's own announcements into the cue output"),
                               speechBox);
    m_cueSpeak->setAccessibleName(tr("Speak XFB's announcements into the cue output"));
    m_cueSpeak->setToolTip(tr("XFB's own status messages are also spoken privately. "
                              "Your screen reader keeps speaking through its own "
                              "output — XFB cannot move that."));
    m_cueSpeak->setChecked(settings.value("Cue/SpeakAnnouncements", false).toBool());
    speechLayout->addWidget(m_cueSpeak);

    m_cueCountdown = new QCheckBox(tr("Count the on-air track down (30, 20, 10, 5 seconds) "
                                      "and say the intro length"), speechBox);
    m_cueCountdown->setAccessibleName(tr("Spoken countdown to the end of the on-air track"));
    m_cueCountdown->setToolTip(tr("Spoken in the cue headphones only, so it never goes "
                                  "to air."));
    m_cueCountdown->setChecked(settings.value("Cue/SpokenCountdown", false).toBool());
    speechLayout->addWidget(m_cueCountdown);

    auto *speechNote = new QLabel(speechBox);
    speechNote->setWordWrap(true);
    speechNote->setText(CueBus::speechAvailable()
        ? tr("Spoken cues are rendered by the system's own speech tool.")
        : tr("No text-to-speech tool was found on this system, so spoken cues "
             "cannot be produced. Everything is still shown on screen and sent "
             "to your screen reader."));
    speechNote->setAccessibleName(speechNote->text());
    speechLayout->addWidget(speechNote);

    outer->addWidget(speechBox);
    outer->addStretch(1);

    // Third, right after Playback: it is where the sound comes out, not an
    // afterthought behind the diagnostics.
    ui->SystemResouces->insertTab(2, page, tr("Cue and outputs"));
    refreshCueWarning();
}

void optionsDialog::saveCueSettings(QSettings &settings)
{
    if (!m_mainOutputCombo || !m_cueOutputCombo)
        return;
    settings.setValue("Cue/MainOutputDevice", m_mainOutputCombo->currentData().toByteArray());
    settings.setValue("Cue/CueOutputDevice", m_cueOutputCombo->currentData().toByteArray());
    settings.setValue("Cue/Volume", m_cueVolume ? m_cueVolume->value() : 80);
    settings.setValue("Cue/SpeakAnnouncements", m_cueSpeak && m_cueSpeak->isChecked());
    settings.setValue("Cue/SpokenCountdown", m_cueCountdown && m_cueCountdown->isChecked());
}

void optionsDialog::updateAccentButton()
{
    // The button doubles as the swatch: theme default or the custom pick
    const QString themeId = ui->combo_theme->currentData().toString();
    const QColor effective = m_accentColor.isValid()
                                 ? m_accentColor
                                 : ThemeManager::themeAccent(themeId);
    ui->bt_accentColor->setStyleSheet(
        QStringLiteral("background-color: %1; border: 1px solid palette(mid); "
                       "border-radius: 3px;")
            .arg(effective.name(QColor::HexRgb)));
    ui->bt_accentReset->setEnabled(m_accentColor.isValid());
}

void optionsDialog::updateClockFontSample()
{
    // The sample is the clock itself: the same wording the player panel shows,
    // drawn in the font about to be saved, so the choice is judged by eye
    // rather than by the name of a typeface.
    ui->lbl_clockFontSample->setFont(m_clockFont);
    ui->lbl_clockFontSample->setText(tr("0:01:23 of 0:03:45"));

    // macOS names its interface typeface ".AppleSystemUIFont", which is not a
    // name to put in front of anyone; every platform's private families start
    // with a dot the same way.
    QString family = m_clockFont.family();
    if (family.startsWith(QLatin1Char('.')) || family.isEmpty())
        family = tr("System");
    const QString name = QStringLiteral("%1 %2 pt").arg(family).arg(m_clockFont.pointSize());
    ui->bt_clockFont->setText(m_clockFontCustom ? name : tr("Choose… (%1)").arg(name));
    ui->bt_clockFontReset->setEnabled(m_clockFontCustom);
    ui->lbl_clockFontSample->setAccessibleName(
        tr("Now-playing clock sample, %1").arg(name));
}

void optionsDialog::on_bt_clockFont_clicked()
{
    bool accepted = false;
    const QFont picked = QFontDialog::getFont(
        &accepted, m_clockFont, this, tr("Pick the now-playing clock typeface"));
    if (!accepted)
        return;
    // The player panel gives the clock one row; a size past what that row can
    // grow to would ride over the transport buttons, so it is held back here
    // rather than silently drawn wrong.
    m_clockFont = ThemeManager::clampNowPlayingClockFont(picked);
    m_clockFontCustom = true;
    updateClockFontSample();
}

void optionsDialog::on_bt_clockFontReset_clicked()
{
    m_clockFont = ThemeManager::defaultNowPlayingClockFont();
    m_clockFontCustom = false;
    updateClockFontSample();
}

void optionsDialog::on_bt_accentColor_clicked()
{
    const QString themeId = ui->combo_theme->currentData().toString();
    const QColor initial = m_accentColor.isValid()
                               ? m_accentColor
                               : ThemeManager::themeAccent(themeId);
    const QColor picked =
        QColorDialog::getColor(initial, this, tr("Pick the accent color"));
    if (picked.isValid()) {
        m_accentColor = picked;
        updateAccentButton();
    }
}

void optionsDialog::on_bt_accentReset_clicked()
{
    m_accentColor = QColor();
    updateAccentButton();
}



optionsDialog::~optionsDialog()
{
    delete ui;
}


void optionsDialog::on_checkBox_disableSeekBar_toggled(bool checked)
{
   qDebug() << "Disable the seek bar: " << checked;
}

void optionsDialog::saveSettings2Db()
{
    qDebug() << "Saving settings using QSettings...";

    // --- Use QSettings with the WRITABLE configuration file path ---
    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = writableConfigPath + "/" + configFileName;

    // Create QSettings object pointing to the correct file
    QSettings settings(configFilePath, QSettings::IniFormat);
    qDebug() << "Saving settings to:" << settings.fileName();

    // --- Use settings.setValue() to save ---
    settings.setValue("Database", ui->txt_selected_db->text()); // Assuming display only, save it back if needed
    settings.setValue("Disable_Seek_Bar", ui->checkBox_disableSeekBar->isChecked());
    settings.setValue("Disable_Volume", ui->checkBox_disableVolume->isChecked());
    settings.setValue("FullScreen", ui->checkBox_fullScreen->isChecked());
    settings.setValue("Theme", ui->combo_theme->currentData().toString());
    settings.setValue("IconTheme", ui->combo_iconTheme->currentData().toString());
    settings.setValue("AccentColor",
                      m_accentColor.isValid() ? m_accentColor.name(QColor::HexRgb)
                                              : QString());
    // DarkMode is kept in sync by ThemeManager::apply() for legacy readers;
    // the theme itself is applied after the sync() below.
    settings.setValue("EnableTorrents", ui->checkBox_enableTorrents->isChecked());
    settings.setValue("ShowFxTab", ui->checkBox_showFxTab->isChecked());
    settings.setValue("ShowPadsTab", ui->checkBox_showPadsTab->isChecked());
    settings.setValue("AutoAutoMix", ui->checkBox_autoAutoMix->isChecked());
    settings.setValue("AutoModeMatchBpm", ui->checkBox_bpmMatch->isChecked());
    settings.setValue("AutoModeBpmTolerance", ui->spin_bpmTolerance->value());
    settings.setValue("LoudnessNormalize", ui->checkBox_loudnessNormalize->isChecked());
    settings.setValue("LoudnessTargetLufs", ui->spin_loudnessTarget->value());
    settings.setValue("LoudnessCeilingDbTp", ui->spin_loudnessCeiling->value());
    settings.setValue("RowNumbers/Music", ui->checkBox_rowNumbersMusic->isChecked());
    settings.setValue("RowNumbers/Jingles", ui->checkBox_rowNumbersJingles->isChecked());
    settings.setValue("RowNumbers/Pub", ui->checkBox_rowNumbersPub->isChecked());
    settings.setValue("RowNumbers/Programs", ui->checkBox_rowNumbersPrograms->isChecked());
    settings.setValue("ShowLevelMeter", ui->checkBox_levelMeter->isChecked());
    settings.setValue("LevelMeterPlacement",
                      ui->combo_levelMeterPos->currentIndex() == 1 ? "side" : "volume");
    FxSettings::saveRetune432(ui->checkBox_retune432->isChecked());

    // Application font size — persist and apply immediately (a restart ensures
    // every already-open view picks it up fully).
    {
        const int fontSize = ui->spin_fontSize->value();
        settings.setValue("FontSize", fontSize);
        if (fontSize > 0 && qApp) {
            QFont appFont = qApp->font();
            appFont.setPointSize(fontSize);
            qApp->setFont(appFont);
        }
    }

    // The now-playing clock. Empty means the default, so an install that never
    // touched it keeps following the application typeface.
    settings.setValue(ThemeManager::nowPlayingClockFontKey(),
                      m_clockFontCustom ? m_clockFont.toString() : QString());

    // Language — read the code from the item data, never from the shown text.
    const QString language = ui->cbox_lang->currentData().toString();
    settings.setValue("Language", language.isEmpty() ? QStringLiteral("en") : language);
    m_languageChanged = (language != m_initialLanguage);

    // Recording (Save description and enum values)
    settings.setValue("RecDevice", ui->cboxRecDev->currentText());
    // Ensure data is valid before saving (use index check if needed)
    if (ui->comboBox_codec->currentIndex() >= 0) {
        settings.setValue("RecCodec", ui->comboBox_codec->currentData());
    } else {
        settings.remove("RecCodec"); // Or set to default
    }
    if (ui->comboBox_container->currentIndex() >= 0) {
        settings.setValue("RecContainer", ui->comboBox_container->currentData());
    } else {
        settings.remove("RecContainer"); // Or set to default
    }


    // Paths
    settings.setValue("SavePath", ui->txt_savePath->text());
    settings.setValue("ProgramsPath", ui->txt_programsPath->text());
    settings.setValue("MusicPath", ui->txt_musicPath->text());
    settings.setValue("JinglePath", ui->txt_jinglePath->text());
    settings.setValue("FTPPath", ui->txt_FTPlocalTempFolder->text());
    settings.setValue("TakeOverPath", ui->txt_takeOverlocalTempFolder->text());

    // Commercial Hour
    settings.setValue("ComHour", ui->cboxComHour->time().toString(Qt::ISODate)); // Save in standard format

    // Downloads (yt-dlp)
    settings.setValue("MusicFormat", ui->cbox_ytdlpFormat->currentText().trimmed().toLower());
    settings.setValue("MusicKeepVideo", ui->checkBox_ytdlpKeepVideo->isChecked());
    settings.setValue("MusicEmbedThumbnail", ui->checkBox_ytdlpEmbedThumbnail->isChecked());
    settings.setValue("MusicEmbedMetadata", ui->checkBox_ytdlpEmbedMetadata->isChecked());
    settings.setValue("SpotifyClientId", ui->txt_spotifyClientId->text().trimmed());
    settings.setValue("SpotifyClientSecret",
                      SecretStore::seal(ui->txt_spotifyClientSecret->text().trimmed()));

    // Networking
    bool networkingEnabled = ui->cbox_enableNetworking->isChecked();
    settings.setValue("Enable_Networking", networkingEnabled);
    if (networkingEnabled) {
        settings.setValue("Server_URL", ui->txt_server->text());
        settings.setValue("Port", ui->txt_port->text());
        settings.setValue("User", ui->txt_user->text());
        settings.setValue("Pass", SecretStore::seal(ui->txt_password->text())); // obfuscated; config is chmod 600
        settings.setValue("Role", ui->cbox_role->currentText());

        // --- .netrc logic ---
        // WARNING: Storing plain text passwords is a security risk.
        QFile netrcFile(QDir::homePath() + "/.netrc");
        // Try reading existing content first to avoid duplicates more robustly
        QString existingContent;
        if (netrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            existingContent = QString::fromUtf8(netrcFile.readAll());
            netrcFile.close();
        } else {
            qWarning() << "Could not read existing ~/.netrc file (may not exist yet).";
        }

        // Prepare the new entry
        QString serverUrlStr = ui->txt_server->text();
        QUrl serverUrl(serverUrlStr); // Use QUrl for parsing
        QString hostname = serverUrl.isValid() ? serverUrl.host() : QString();
        QString newUser = ui->txt_user->text();
        QString newPassword = ui->txt_password->text(); // Still insecure
        QString newMachineEntry = QString("machine %1 login %2 password %3\n").arg(hostname, newUser, newPassword);

        bool entryExists = false;
        if (!hostname.isEmpty() && !newUser.isEmpty()) {
            // Basic check if a similar entry exists (could be improved with regex)
            QString searchPattern = QString("machine %1 login %2 ").arg(hostname, newUser);
            if (existingContent.contains(searchPattern)) {
                entryExists = true;
                qInfo() << ".netrc entry for" << hostname << "and user" << newUser << "likely already exists. Not adding duplicate.";
            }
        }

        if (!hostname.isEmpty() && !newUser.isEmpty() && !entryExists) {
            // Append the new entry if it doesn't seem to exist
            if (netrcFile.open(QIODevice::Append | QIODevice::Text)) {
                qInfo() << "Appending entry to .netrc for machine:" << hostname;
                QTextStream outF(&netrcFile);
                outF << newMachineEntry;
                netrcFile.close();
                // Set restrictive permissions
                if (!netrcFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
                    qWarning() << "Could not set permissions on ~/.netrc file.";
                }
            } else {
                qWarning() << "Could not open ~/.netrc file for appending.";
            }
        } else if (hostname.isEmpty()) {
            qWarning() << "Could not determine hostname from server URL for .netrc:" << serverUrlStr;
        }
        // --- End .netrc logic ---

    } else {
        // Clear network settings if disabled
        settings.remove("Server_URL"); // Use remove or set to empty
        settings.remove("Port");
        settings.remove("User");
        settings.remove("Pass");
        settings.setValue("Role", "Client"); // Set default role
    }

    saveCueSettings(settings); // on-air / cue output devices, spoken cues

    // QSettings automatically saves on destruction or explicitly via sync()
    settings.sync(); // Force save to file immediately
    SecretStore::restrictFile(settings.fileName()); // config may hold credentials: owner-only

    // Re-theme the whole application now that Theme/AccentColor are saved,
    // so Save gives instant visual feedback. The dialog's own icons follow;
    // the main window re-themes itself in updateConfig(), which the dialog's
    // finished() signal reaches.
    ThemeManager::apply(qobject_cast<QApplication *>(QApplication::instance()));
    IconTheme::reload();
    IconTheme::retheme(this);
    qDebug() << "Settings save attempt finished for" << settings.fileName();
    if (settings.status() != QSettings::NoError) {
        qWarning() << "Error during QSettings sync:" << settings.status();
        QMessageBox::warning(this, tr("Settings Error"), tr("Could not save settings to configuration file."));
    } else {
        qInfo() << "Settings saved successfully.";
        // Optional: QMessageBox::information(this, tr("Settings Saved"), tr("Settings saved successfully."));
    }

    // The translator is installed once at startup, so a new language only takes
    // effect on the next run. Say so instead of leaving the UI looking unchanged.
    if (m_languageChanged) {
        m_initialLanguage = ui->cbox_lang->currentData().toString();
        m_languageChanged = false;
        QMessageBox::information(this, tr("Language changed"),
                                 tr("The new language will be used the next time XFB starts."));
    }
}

void optionsDialog::on_bt_save_settings_clicked()
{
    saveSettings2Db();

}

void optionsDialog::on_pushButton_clicked()
{
    saveSettings2Db();

    // accept() (not hide()) so QDialog::finished fires: the player relies
    // on it to re-apply settings live (level meter, seek bar, FX tab...).
    // hide() also leaked the dialog, since WA_DeleteOnClose never triggered.
    this->accept();
}

void optionsDialog::on_pushButton_2_clicked()
{
    this->reject(); // close without saving; finished still fires
}

// ---------------------------------------------------------------------------
// Diagnostics
//
// Everything an administrator standing at a desk that is misbehaving would
// otherwise have to go and find: what this machine is, where XFB keeps its
// things, which sound cards it can see, whether the station server answers.
// Each button appends to the same output box, and the box can be copied or
// saved whole — which is the form a problem report should arrive in.
// ---------------------------------------------------------------------------

void optionsDialog::reportSection(const QString &heading, const QStringList &lines)
{
    ui->txt_terminal->appendPlainText(QStringLiteral("── %1 ──").arg(heading));
    for (const QString &line : lines)
        ui->txt_terminal->appendPlainText(QStringLiteral("  ") + line);
    ui->txt_terminal->appendPlainText(QString());
}

void optionsDialog::reportCommand(const QString &heading, const QString &command)
{
    QProcess sh;
#ifdef Q_OS_WIN
    sh.start(QStringLiteral("cmd"), QStringList() << QStringLiteral("/c") << command);
#else
    sh.start(QStringLiteral("sh"), QStringList() << QStringLiteral("-c") << command);
#endif
    // Bounded: a tool that never returns must not take the dialog with it.
    if (!sh.waitForStarted(3000) || !sh.waitForFinished(10000)) {
        sh.kill();
        reportSection(heading, { tr("no answer") });
        return;
    }
    const QString output = QString::fromLocal8Bit(sh.readAll()).trimmed();
    reportSection(heading, output.isEmpty()
                               ? QStringList{ tr("nothing to report") }
                               : output.split(QLatin1Char('\n')));
}

namespace
{
/** "12,3 GB free of 465 GB", or why that could not be answered. */
QString describeSpace(const QString &path)
{
    QStorageInfo storage(path);
    if (!storage.isValid() || !storage.isReady())
        return QObject::tr("free space unknown");
    const auto gb = [](qint64 bytes) { return double(bytes) / (1024.0 * 1024.0 * 1024.0); };
    return QObject::tr("%1 GB free of %2 GB")
        .arg(gb(storage.bytesAvailable()), 0, 'f', 1)
        .arg(gb(storage.bytesTotal()), 0, 'f', 1);
}

/** One line per configured path: where it is, and whether it is really there. */
QString describePath(const QString &label, const QString &path, bool expectFile)
{
    if (path.isEmpty())
        return QStringLiteral("%1: %2").arg(label, QObject::tr("not set"));
    const QFileInfo info(path);
    QString state;
    if (!info.exists())
        state = QObject::tr("MISSING");
    else if (expectFile)
        state = QObject::tr("%1 KB").arg(info.size() / 1024);
    else if (!info.isDir())
        state = QObject::tr("not a folder");
    else
        state = info.isWritable() ? describeSpace(path) : QObject::tr("READ-ONLY");
    return QStringLiteral("%1: %2  [%3]").arg(label, path, state);
}
} // namespace

void optionsDialog::on_bt_system_clicked()
{
    QStringList lines;
    lines << tr("XFB %1").arg(QCoreApplication::applicationVersion())
          << tr("Qt %1 (built against %2)").arg(qVersion(), QT_VERSION_STR)
          << tr("%1 %2 (%3)").arg(QSysInfo::prettyProductName(),
                                  QSysInfo::productVersion(),
                                  QSysInfo::currentCpuArchitecture())
          << tr("Host: %1").arg(QSysInfo::machineHostName())
          << tr("Style: %1").arg(QApplication::style() ? QApplication::style()->name()
                                                       : tr("unknown"))
          << tr("Theme: %1, icons: %2").arg(ThemeManager::configuredTheme(),
                                            IconTheme::configuredTheme())
          << tr("Language: %1").arg(ui->cbox_lang->currentData().toString());

    // The two programs every download goes through, looked up exactly the way
    // the downloader itself looks them up.
    const QString ytdlp = findYtDlpExecutable();
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    lines << tr("yt-dlp: %1").arg(ytdlp.isEmpty() ? tr("not found") : ytdlp)
          << tr("ffmpeg: %1").arg(ffmpeg.isEmpty() ? tr("not found") : ffmpeg);

    reportSection(tr("System"), lines);
}

void optionsDialog::on_bt_folders_clicked()
{
    const QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QStringList lines;
    lines << describePath(tr("Settings"), config + QStringLiteral("/xfb.conf"), true)
          << describePath(tr("Log"), config + QStringLiteral("/xfb.log"), true)
          << describePath(tr("Database"), databasePath(), true)
          << describePath(tr("Music"), ui->txt_musicPath->text(), false)
          << describePath(tr("Jingles"), ui->txt_jinglePath->text(), false)
          << describePath(tr("Programmes"), ui->txt_programsPath->text(), false)
          << describePath(tr("Recordings"), ui->txt_savePath->text(), false)
          << describePath(tr("FTP temporary"), ui->txt_FTPlocalTempFolder->text(), false)
          << describePath(tr("TakeOver temporary"), ui->txt_takeOverlocalTempFolder->text(), false);
    reportSection(tr("Folders and files"), lines);
}

void optionsDialog::on_bt_memory_clicked()
{
    // free(1) is Linux-only; macOS answers the same question with vm_stat.
#ifdef Q_OS_WIN
    reportCommand(tr("Memory"),
                  QStringLiteral("wmic OS get FreePhysicalMemory,TotalVisibleMemorySize /Value"));
#else
    reportCommand(tr("Memory"), QStringLiteral("free -mt 2>/dev/null || vm_stat"));
#endif
}

void optionsDialog::on_bt_disk_clicked()
{
    // -T (print the filesystem type) is a GNU extension; BSD df rejects it.
#ifdef Q_OS_WIN
    reportCommand(tr("Disk space"),
                  QStringLiteral("wmic logicaldisk get size,freespace,caption"));
#else
    reportCommand(tr("Disk space"), QStringLiteral("df -hT 2>/dev/null || df -h"));
#endif
}

void optionsDialog::on_bt_audio_clicked()
{
    QStringList lines;
    const QAudioDevice defaultOut = QMediaDevices::defaultAudioOutput();
    lines << tr("Outputs:");
    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty())
        lines << QStringLiteral("  ") + tr("none — this machine cannot make a sound");
    for (const QAudioDevice &device : outputs) {
        lines << QStringLiteral("  %1 (%2 ch)%3")
                     .arg(device.description())
                     .arg(device.maximumChannelCount())
                     .arg(device.id() == defaultOut.id() ? tr("  ← system default")
                                                         : QString());
    }
    lines << tr("Inputs:");
    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    if (inputs.isEmpty())
        lines << QStringLiteral("  ") + tr("none — nothing can be recorded");
    for (const QAudioDevice &device : inputs)
        lines << QStringLiteral("  ") + device.description();

    // What XFB is set to use, which is the half a device list never tells you.
    const auto chosen = [&](QComboBox *combo, const QString &fallback) {
        if (!combo)
            return fallback;
        return combo->currentData().toByteArray().isEmpty() ? fallback
                                                            : combo->currentText();
    };
    lines << tr("On air: %1").arg(chosen(m_mainOutputCombo, tr("system default")))
          << tr("Cue: %1").arg(chosen(m_cueOutputCombo, tr("off")))
          << tr("Recording: %1").arg(ui->cboxRecDev->currentText());
    reportSection(tr("Audio devices"), lines);
}

void optionsDialog::on_bt_network_clicked()
{
    QStringList lines;
    lines << tr("Host: %1").arg(QSysInfo::machineHostName());
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if (!interface.flags().testFlag(QNetworkInterface::IsUp)
            || interface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol)
                lines << QStringLiteral("%1: %2").arg(interface.humanReadableName(),
                                                      ip.toString());
        }
    }

    if (!ui->cbox_enableNetworking->isChecked()) {
        lines << tr("Station server: switched off");
        reportSection(tr("Network"), lines);
        return;
    }

    const QString host = QUrl(ui->txt_server->text()).host().isEmpty()
                             ? ui->txt_server->text().trimmed()
                             : QUrl(ui->txt_server->text()).host();
    const quint16 port = quint16(ui->txt_port->text().toUInt());
    lines << tr("Station server: %1:%2 as %3 (%4)")
                 .arg(host).arg(port)
                 .arg(ui->txt_user->text(), ui->cbox_role->currentText());
    if (host.isEmpty()) {
        lines << tr("No server address is set.");
    } else {
        // A three-second reach for the door. Blocking, like every other button
        // on this tab, but bounded — an unreachable server must not hang XFB.
        QTcpSocket socket;
        socket.connectToHost(host, port ? port : 21);
        lines << (socket.waitForConnected(3000)
                      ? tr("It answers.")
                      : tr("No answer: %1").arg(socket.errorString()));
        socket.abort();
    }
    reportSection(tr("Network"), lines);
}

void optionsDialog::on_bt_openLog_clicked()
{
    // Where the log really is: next to xfb.conf, which is what users are asked
    // for first when something goes wrong.
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                         + QStringLiteral("/xfb.log");
    if (!QFile::exists(path)) {
        reportSection(tr("Log"), { tr("There is no log file yet."), path });
        return;
    }
    reportSection(tr("Log"), { path, describePath(tr("size"), path, true) });
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        ui->txt_terminal->appendPlainText(tr("Could not open the log in an editor."));
}

void optionsDialog::on_bt_edit_settings_clicked()
{
    // The settings live in the writable config location, which is where every
    // other reader in XFB looks. This used to hand the editor ":/xfb.conf" —
    // a Qt resource path, which no text editor can open — so the button did
    // nothing at all.
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                         + QStringLiteral("/xfb.conf");
    if (!QFile::exists(path)) {
        reportSection(tr("Settings"),
                      { tr("No settings file yet — it is written the first time you save."),
                        path });
        return;
    }
    ui->txt_terminal->appendPlainText(path);
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        ui->txt_terminal->appendPlainText(tr("Could not open the settings file in an editor."));
}

void optionsDialog::on_bt_report_clicked()
{
    ui->txt_terminal->clear();
    ui->txt_terminal->appendPlainText(
        tr("XFB diagnostics — %1")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    ui->txt_terminal->appendPlainText(QString());
    QApplication::setOverrideCursor(Qt::WaitCursor);
    on_bt_system_clicked();
    on_bt_folders_clicked();
    on_bt_audio_clicked();
    on_bt_network_clicked();
    on_bt_memory_clicked();
    on_bt_disk_clicked();
    QApplication::restoreOverrideCursor();
    ui->txt_terminal->appendPlainText(
        tr("End of report. \"Copy\" or \"Save as…\" puts all of this where you "
           "can send it."));
    // Nothing above prints a password, but the server user and the machine's
    // addresses are in there, so say so before it is mailed anywhere.
    ui->txt_terminal->appendPlainText(
        tr("It names this machine, its addresses and your folders — no passwords."));
}

void optionsDialog::on_bt_copyOutput_clicked()
{
    if (QClipboard *clipboard = QApplication::clipboard())
        clipboard->setText(ui->txt_terminal->toPlainText());
}

void optionsDialog::on_bt_saveOutput_clicked()
{
    const QString suggestion =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QStringLiteral("/xfb-diagnostics-")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
        + QStringLiteral(".txt");
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save the diagnostics"), suggestion, tr("Text files (*.txt)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Could not save"), file.errorString());
        return;
    }
    QTextStream out(&file);
    out << ui->txt_terminal->toPlainText();
    file.close();
    ui->txt_terminal->appendPlainText(tr("Saved to %1").arg(path));
}

void optionsDialog::on_bt_clearOutput_clicked()
{
    ui->txt_terminal->clear();
}

// ---------------------------------------------------------------------------
// Database tab
// ---------------------------------------------------------------------------

QString optionsDialog::databasePath() const
{
    // Ask the open connection rather than the "Database" setting: that setting
    // still holds the relative path of a much older layout, while the
    // connection knows the file XFB is really reading and writing.
    const QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"), false);
    return db.isValid() ? db.databaseName() : QString();
}

void optionsDialog::refreshDatabaseCounts()
{
    ui->tbl_dbCounts->clearContents();
    ui->tbl_dbCounts->setColumnCount(2);
    ui->tbl_dbCounts->setHorizontalHeaderLabels({ tr("Table"), tr("Records") });
    ui->tbl_dbCounts->setRowCount(0);

    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
    if (!db.isOpen()) {
        ui->tbl_dbCounts->setRowCount(1);
        ui->tbl_dbCounts->setItem(0, 0, new QTableWidgetItem(tr("No database is open")));
        return;
    }

    // Whatever the database actually holds, rather than a list written here
    // that goes stale the next time a table is added.
    QSqlQuery tables(db);
    tables.exec(QStringLiteral("select name from sqlite_master where type='table' "
                               "and name not like 'sqlite_%' order by name"));
    while (tables.next()) {
        const QString name = tables.value(0).toString();
        QSqlQuery count(db);
        // The name comes from sqlite_master, so it is a real identifier; quote
        // it anyway so a table named after a keyword still counts.
        count.exec(QStringLiteral("select count(*) from \"%1\"")
                       .arg(QString(name).replace(QLatin1Char('"'), QLatin1String("\"\""))));
        const QString rows = count.next() ? QString::number(count.value(0).toLongLong())
                                          : tr("?");
        const int row = ui->tbl_dbCounts->rowCount();
        ui->tbl_dbCounts->insertRow(row);
        ui->tbl_dbCounts->setItem(row, 0, new QTableWidgetItem(name));
        auto *value = new QTableWidgetItem(rows);
        value->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->tbl_dbCounts->setItem(row, 1, value);
    }

    const QFileInfo info(databasePath());
    const int row = ui->tbl_dbCounts->rowCount();
    ui->tbl_dbCounts->insertRow(row);
    auto *label = new QTableWidgetItem(tr("File size"));
    QFont bold = label->font();
    bold.setBold(true);
    label->setFont(bold);
    ui->tbl_dbCounts->setItem(row, 0, label);
    auto *size = new QTableWidgetItem(
        info.exists() ? tr("%1 KB").arg(info.size() / 1024) : tr("unknown"));
    size->setFont(bold);
    size->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->tbl_dbCounts->setItem(row, 1, size);

    ui->tbl_dbCounts->horizontalHeader()->setStretchLastSection(false);
    ui->tbl_dbCounts->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tbl_dbCounts->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void optionsDialog::on_bt_dbRefresh_clicked()
{
    refreshDatabaseCounts();
}

void optionsDialog::on_bt_dbBackup_clicked()
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
    if (!db.isOpen()) {
        QMessageBox::warning(this, tr("No database"),
                             tr("There is no open database to back up."));
        return;
    }

    const QString suggestion =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QStringLiteral("/adb-")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
        + QStringLiteral(".db");
    QString path = QFileDialog::getSaveFileName(this, tr("Back up the database"),
                                                suggestion, tr("Database (*.db)"));
    if (path.isEmpty())
        return;
    // VACUUM INTO refuses to overwrite, which is the behaviour we want for a
    // backup — but the file dialog has already asked about replacing, so an
    // existing file is one the operator chose to lose.
    if (QFile::exists(path) && !QFile::remove(path)) {
        QMessageBox::warning(this, tr("Could not back up"),
                             tr("%1 is in the way and could not be removed.").arg(path));
        return;
    }

    // Taken through SQLite rather than by copying the file: XFB runs in WAL
    // mode, so the .db on disk is only part of the story at any moment and a
    // plain copy of it can be a database with today's music missing.
    QSqlQuery query(db);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = query.exec(
        QStringLiteral("VACUUM INTO '%1'")
            .arg(QString(path).replace(QLatin1Char('\''), QLatin1String("''"))));
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::critical(this, tr("Could not back up"), query.lastError().text());
        return;
    }
    QMessageBox::information(
        this, tr("Backed up"),
        tr("The database was copied to:\n%1\n\nIt is a complete database on its own — "
           "point XFB at it to go back to how things were.").arg(path));
}

void optionsDialog::on_bt_dbCheck_clicked()
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
    if (!db.isOpen()) {
        QMessageBox::warning(this, tr("No database"), tr("There is no open database."));
        return;
    }
    QSqlQuery query(db);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = query.exec(QStringLiteral("PRAGMA integrity_check"));
    QStringList findings;
    while (ok && query.next())
        findings << query.value(0).toString();
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::critical(this, tr("Could not check"), query.lastError().text());
        return;
    }
    if (findings.size() == 1 && findings.first().compare(QLatin1String("ok"),
                                                         Qt::CaseInsensitive) == 0) {
        QMessageBox::information(this, tr("The database is sound"),
                                 tr("SQLite walked the whole file and found nothing wrong."));
        return;
    }
    QMessageBox::warning(
        this, tr("The database is damaged"),
        tr("SQLite reported:\n\n%1\n\nRestore your most recent backup rather than "
           "carrying on with this file.").arg(findings.join(QLatin1Char('\n'))));
}

void optionsDialog::on_bt_dbCompact_clicked()
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
    if (!db.isOpen()) {
        QMessageBox::warning(this, tr("No database"), tr("There is no open database."));
        return;
    }
    const qint64 before = QFileInfo(databasePath()).size();
    QSqlQuery query(db);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = query.exec(QStringLiteral("VACUUM"));
    QApplication::restoreOverrideCursor();
    if (!ok) {
        QMessageBox::critical(this, tr("Could not compact"), query.lastError().text());
        return;
    }
    const qint64 after = QFileInfo(databasePath()).size();
    refreshDatabaseCounts();
    QMessageBox::information(this, tr("Compacted"),
                             tr("%1 KB became %2 KB. Nothing was lost.")
                                 .arg(before / 1024).arg(after / 1024));
}

void optionsDialog::on_bt_dbShow_clicked()
{
    const QString path = databasePath();
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox::warning(this, tr("No database file"),
                             tr("XFB could not work out where the database file is."));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

void optionsDialog::on_pushButton_3_clicked()
{
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    //delete all records in the music table
    QMessageBox::StandardButton go;
    go = QMessageBox::question(this,"Sure root?","Are you sure you want to delete ALL the tracks from the music table in the database?", QMessageBox::Yes|QMessageBox::No);
    if(go==QMessageBox::Yes){
        QSqlQuery sql(db);
        sql.prepare("delete from musics where 1");
        if(sql.exec()){
           QMessageBox::information(this,tr("Tracks removed"),tr("All the tracks were removed from the database!"));
           //update_music_table();
        } else {
            QMessageBox::critical(this,tr("Error"),sql.lastError().text());
            qDebug() << "last sql: " << sql.lastQuery();
        }
    }
}

void optionsDialog::on_f_bt_del_jingles_table_clicked()
{
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    //delete all records in the jingles table
    QMessageBox::StandardButton go;
    go = QMessageBox::question(this,"Sure root?","Are you sure you want to delete ALL the Jingles from the jingles table in the database?", QMessageBox::Yes|QMessageBox::No);
    if(go==QMessageBox::Yes){
        QSqlQuery sql(db);
        sql.prepare("delete from jingles where 1");
        if(sql.exec()){
           QMessageBox::information(this,tr("Jingles removed"),tr("All the jingles were removed from the database!"));
           //update_music_table();
        } else {
            QMessageBox::critical(this,tr("Error"),sql.lastError().text());
            qDebug() << "last sql: " << sql.lastQuery();
        }
    }
}

void optionsDialog::on_f_bt_del_pub_table_clicked()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    //delete all records in the pub table
    QMessageBox::StandardButton go;
    go = QMessageBox::question(this,"Sure root?","Are you sure you want to delete ALL the publicity from the pub table in the database? (Scheduler table will also be cleared)", QMessageBox::Yes|QMessageBox::No);
    if(go==QMessageBox::Yes){
        QSqlQuery sql(db);
        sql.prepare("delete from pub where 1");
        if(sql.exec()){
            QMessageBox::information(this,tr("Pubs removed"),tr("All the pubs were removed from the database!"));
            QSqlQuery q(db);
            q.prepare("delete from scheduler where 1");
           if(q.exec()){
               QMessageBox::information(this,tr("Scheduler cleared"),tr("Scheduler table was cleared!"));
           }

           //update_music_table();
        } else {
            QMessageBox::critical(this,tr("Error"),sql.lastError().text());
            qDebug() << "last sql: " << sql.lastQuery();
        }
    }
}

void optionsDialog::on_cbox_enableNetworking_toggled(bool checked)
{
    // The whole box, so its labels grey out with the fields rather than
    // leaving "Server URL:" reading as live next to a dead field.
    ui->grp_server->setEnabled(checked);
}

void optionsDialog::on_checkBox_enableTorrents_clicked(bool checked)
{
    // Only when the user turns the feature ON here, and only the first time
    // ever (guarded by TorrentPrivacyConsent). This is a clicked() handler, so
    // it does not fire for the programmatic setChecked() done while loading.
    if (!checked)
        return;

    QString configFilePath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                             + "/xfb.conf";
    QSettings settings(configFilePath, QSettings::IniFormat);
    if (settings.value("TorrentPrivacyConsent", false).toBool())
        return; // the disclosure has already been acknowledged

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Before you enable Torrents"));
    box.setText(tr("Please read how private this feature really is."));
    box.setInformativeText(tr(
        "• Searching is anonymised through the Tor network.\n\n"
        "• Downloading is NOT anonymous. BitTorrent transfers cannot be "
        "routed through Tor, so while a download runs your real IP address "
        "is visible to the tracker and to the other peers sharing that file.\n\n"
        "• XFB minimises this: it disables DHT/peer-exchange, injects no extra "
        "trackers, requires encryption and stops uploading as soon as a "
        "download finishes — but it cannot hide your IP during the transfer.\n\n"
        "• You are responsible for complying with the copyright law that "
        "applies to you. Only download content you are legally entitled to.\n\n"
        "Enable the Torrents feature with these limitations understood?"));
    QPushButton *accept = box.addButton(tr("I understand — enable"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();

    if (box.clickedButton() == accept)
        settings.setValue("TorrentPrivacyConsent", true);
    else
        ui->checkBox_enableTorrents->setChecked(false); // declined — revert
}

void optionsDialog::on_bt_browseSavePath_clicked()
{
    QString savePath = QFileDialog::getExistingDirectory(this, tr("Select a directory"));
    qDebug()<<"Default save path: "<<savePath;
    ui->txt_savePath->setText(savePath);
}

void optionsDialog::on_bt_browse_programPath_clicked()
{
    QString ProgramPath = QFileDialog::getExistingDirectory(this, tr("Select a directory"));
    qDebug()<<"Default Program path: "<<ProgramPath;
    ui->txt_programsPath->setText(ProgramPath);
}

void optionsDialog::on_bt_browseFTPlocalFolder_clicked()
{
    QString FTPPath = QFileDialog::getExistingDirectory(this, tr("Select a directory"));
    qDebug()<<"Default FTP path: "<<FTPPath;
    ui->txt_FTPlocalTempFolder->setText(FTPPath);
}

void optionsDialog::on_bt_browse_musicPath_clicked()
{
    QString MusicPath = QFileDialog::getExistingDirectory(this, tr("Select a directory"));
    qDebug()<<"Default Music path: "<<MusicPath;
    ui->txt_musicPath->setText(MusicPath);
}

void optionsDialog::on_bt_browse_jinglePath_clicked()
{
    QString JinglePath = QFileDialog::getExistingDirectory(this, tr("Select a directory"));
    qDebug()<<"Default Jingle path: "<<JinglePath;
    ui->txt_jinglePath->setText(JinglePath);
}

void optionsDialog::on_bt_browseTakeOverlocalFolder_clicked()
{
    QString TakeOverPath = QFileDialog::getExistingDirectory(this, tr("Select a directory"));
    qDebug()<<"Default TakeOver path: "<<TakeOverPath;
    ui->txt_takeOverlocalTempFolder->setText(TakeOverPath);
}
