/*
Hi and welcome. Thank you for reading my code.
I've made all in a way that I hope makes is easy for anyone to understand.
Enjoy! . Frédéric Bogaerts 2015 @ Netpack - Online Solutions!.
*/

#include "player.h"
#include "ui_player.h"
#include "add_music_single.h"
#include "add_full_dir.h"
#include "addgenre.h"
#include "addjingle.h"
#include "add_pub.h"
#include "add_program.h"
#include "optionsdialog.h"
#include "externaldownloader.h"
#include "aboutus.h"
#include "audio/FxEngine.h"
#include "audio/WaveformStore.h"
#include "PlaylistWaveView.h"
#include "dialogs/AudioFxDialog.h"
#include "secretstore.h"
#include "services/NgrokTunnelService.h"
#include "services/UpdateCheckService.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QtSql>
#include <QMediaPlayer>
#include <QAudio>
#include <QDebug>
#include <QtMultimedia>
#include <QTableWidgetItem>
#include <QList>
#include <QDateTime>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QSqlTableModel>
#include <QFileInfo>
#include <QMediaDevices>
#include <QAudioInput>
#include <QAudioOutput>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QHttpPart>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointF>
#include <QVector>
#include <QMovie>
#include <QProgressDialog>
#include <QTextBrowser>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QDesktopServices>

#include <cstdlib> // _exit()
#ifdef Q_OS_MAC
#include <unistd.h>
#endif
#include <QUrl>
#include <QClipboard>
#include <QHeaderView>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDialog>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QPushButton>
#include <QPixmap>
#include <QtCore>
#include <QtGlobal>
#include <QSizeGrip>
#include <QScopeGuard>
#include <QProcess>
#include <QComboBox>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QToolBox>
#include <QToolButton>

#ifdef XFB_HAS_WEBENGINE
#include <QtWebEngineQuick>
#endif
#include <QQuickWidget>
#include <QQuickItem>  // full definition needed for adBanner->rootObject()

#include "services/ServiceContainer.h"
#include "services/AccessibilityManager.h"
#include "services/AudioFeedbackService.h"
#include "services/LiveRegionManager.h"
#include "services/PlaybackStatusAnnouncer.h"
#include "services/SystemStatusAnnouncer.h"
#include "services/TorNetworkService.h"
#include "services/TorrentSearchService.h"
#include "services/TorrentDownloadService.h"
#include "services/DependencyChecker.h"

// Static variable definition for recursion protection
int player::s_recursionDepth = 0;

class ClickableTextBrowser : public QTextBrowser {
public:
    explicit ClickableTextBrowser(QWidget* parent = nullptr) : QTextBrowser(parent) {}

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QTextBrowser::mousePressEvent(event);
        if (event->button() == Qt::LeftButton) {
            QTextCursor cursor = cursorForPosition(event->pos());
            cursor.select(QTextCursor::WordUnderCursor);
            QString href = cursor.selectedText();
            if (href.startsWith("http://") || href.startsWith("https://")) {
                QDesktopServices::openUrl(QUrl(href));
                event->accept(); // Indicate that the event has been handled
            }
        }
    }
};

class CustomMessageBox : public QDialog {
public:
    CustomMessageBox(const QString& title, const QString& message, const QPixmap& pixmap, QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(title);

        QIcon icon(":/48x48.png");

        setWindowIcon(icon);
        qreal level(0.86);
        setWindowOpacity(level);
        setMinimumWidth(586);



        QVBoxLayout* layout = new QVBoxLayout(this);

        // Add image
        QLabel* imageLabel = new QLabel(this);
        imageLabel->setPixmap(pixmap);
        imageLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(imageLabel);


        // Add message (as a clickable QTextBrowser)
        ClickableTextBrowser* messageBrowser = new ClickableTextBrowser(this);
        messageBrowser->setOpenExternalLinks(true);
        messageBrowser->setHtml(message);
        layout->addWidget(messageBrowser);


        // Add OK button
        QPushButton* okButton = new QPushButton(tr("I'll donate if I can"), this);
        layout->addWidget(okButton);
        connect(okButton, &QPushButton::clicked, this, &CustomMessageBox::accept);
    }
};

player::player(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::player)
{
    // Ensure we're in the main thread
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        qCritical() << "Player constructor called from wrong thread!";
        qCritical() << "Current thread:" << QThread::currentThread();
        qCritical() << "Main thread:" << QApplication::instance()->thread();
        throw std::runtime_error("Player must be created in main thread");
    }

    qDebug()<<"\nStarting XFB :: Developed by Frédéric Bogaerts @ Netpack - Online Solutions! www.netpack.pt";

    qDebug() << "About to call ui->setupUi(this)...";
    
    try {
        ui->setupUi(this);
        qDebug() << "ui->setupUi(this) completed successfully!";
    } catch (const std::exception& e) {
        qCritical() << "Exception during ui->setupUi():" << e.what();
        throw;
    } catch (...) {
        qCritical() << "Unknown exception during ui->setupUi()";
        throw;
    }

    // Fix grid layout: the UI file's QGridLayout has 25 rows but only 3 have widgets.
    // Empty rows consume space that creates a visible gap. Replace with a VBoxLayout
    // and use QSplitter for user-resizable side panels.
    if (ui->gridLayout_2) {
        QWidget *parentWidget = ui->widget;
        
        // Reparent widgets to a temporary holder so they survive layout deletion
        QWidget tempHolder;
        ui->frame_4->setParent(&tempHolder);
        ui->frame->setParent(&tempHolder);
        ui->tabWidget_2->setParent(&tempHolder);
        ui->pubWidget->setParent(&tempHolder);
        ui->page_FTP_Connection->setParent(&tempHolder);
        
        // Delete the old grid layout
        QLayout *oldLayout = parentWidget->layout();
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item;
        }
        delete oldLayout;
        
        // Build a clean VBoxLayout
        QVBoxLayout *vbox = new QVBoxLayout(parentWidget);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);
        
        // Top row: player controls + clock — resizable splitter
        QSplitter *topSplitter = new QSplitter(Qt::Horizontal, parentWidget);
        topSplitter->setChildrenCollapsible(false);
        ui->frame_4->setParent(topSplitter);
        ui->frame->setParent(topSplitter);
        ui->frame->setMinimumWidth(200);
        ui->frame->setMaximumWidth(16777215); // remove the 350 cap
        topSplitter->addWidget(ui->frame_4);
        topSplitter->addWidget(ui->frame);
        topSplitter->setStretchFactor(0, 1); // frame_4 stretches
        topSplitter->setStretchFactor(1, 0); // frame stays compact
        topSplitter->setSizes({1000, 350});
        vbox->addWidget(topSplitter, 0);
        
        // Middle row: Playlist/History/DJ/FX tabs + the side toolbox —
        // resizable splitter. The toolbox lives up here (rather than beside the
        // music lists) so that collapsing the bottom music tabs hands their
        // vertical space to this area — the DJ and Audio FX tabs.
        QSplitter *middleSplitter = new QSplitter(Qt::Horizontal, parentWidget);
        middleSplitter->setChildrenCollapsible(false);
        ui->tabWidget_2->setParent(middleSplitter);
        ui->page_FTP_Connection->setParent(middleSplitter);
        ui->page_FTP_Connection->setMaximumWidth(16777215); // remove the 350 cap
        ui->page_FTP_Connection->setMinimumWidth(150);
        middleSplitter->addWidget(ui->tabWidget_2);
        middleSplitter->addWidget(ui->page_FTP_Connection);
        middleSplitter->setStretchFactor(0, 1); // the tabs stretch
        middleSplitter->setStretchFactor(1, 0); // toolbox stays compact
        middleSplitter->setSizes({1000, 350});
        middleSplitter->setCollapsible(0, false);
        // Don't allow dragging the side panel down to nothing — that left it
        // impossible to restore. Hiding/showing is done with the toggle below.
        middleSplitter->setCollapsible(1, false);
        vbox->addWidget(middleSplitter, 1);

        // Always-visible toggle in the tab-bar corner to show/hide the side
        // panel (Search / Filters / Extras / Playlist). Hiding a QSplitter child
        // reclaims its space for the tabs, and the corner button guarantees the
        // panel can always be brought back.
        m_sidePanelToggle = new QToolButton(ui->tabWidget_2);
        m_sidePanelToggle->setCheckable(true);
        m_sidePanelToggle->setChecked(true);
        m_sidePanelToggle->setAutoRaise(true);
        m_sidePanelToggle->setArrowType(Qt::RightArrow);
        m_sidePanelToggle->setToolTip(tr("Show or hide the side panel (Search, Filters, Extras, Playlist)"));
        ui->tabWidget_2->setCornerWidget(m_sidePanelToggle, Qt::TopRightCorner);
        connect(m_sidePanelToggle, &QToolButton::toggled, this, [this](bool on) {
            if (ui->page_FTP_Connection)
                ui->page_FTP_Connection->setVisible(on);
            if (m_sidePanelToggle)
                m_sidePanelToggle->setArrowType(on ? Qt::RightArrow : Qt::LeftArrow);
        });

        // Bottom row: Music/Jingles/Pub/Programs/Torrents tabs — full width,
        // pinned to the bottom of the window. Collapsing them (click the active
        // tab) folds this row down to just the tab bar, giving all of its space
        // to the middle area above.
        ui->pubWidget->setParent(parentWidget);
        vbox->addWidget(ui->pubWidget, 1);
        
        qDebug() << "Replaced gridLayout_2 with VBoxLayout + resizable QSplitters";
    }

    // Make the Music/Jingles/Pub/Programs/Torrents tab area collapsible:
    // clicking the currently-selected tab folds the content pane down to just
    // the tab bar, freeing vertical space for the playlist above.
    setupCollapsibleTabs();

    // The playlist controls panel (frame_2: Total time / Sum to Playlist /
    // Update last played / Random) moves into the Search/Filters/Extras toolbox
    // as a new "Playlist" page, so all side controls live in one toggle-able
    // panel. The Playlist tab then shows just the playlist, full width.
    if (ui->tabPlaylist && ui->playlist) {
        QWidget *playlistTab = ui->tabPlaylist;

        // Move frame_2 into the side toolbox (reparents it out of this tab).
        if (ui->frame_2 && ui->page_FTP_Connection &&
            ui->page_FTP_Connection->indexOf(ui->frame_2) < 0) {
            ui->frame_2->setMinimumWidth(0);
            ui->frame_2->setMaximumWidth(16777215);
            ui->page_FTP_Connection->addItem(ui->frame_2,
                QIcon(QStringLiteral(":/icons/format-list-unordered.png")),
                tr("Playlist"));
        }

        // Reparent the playlist out of the old grid layout, then delete it.
        QWidget tempHolder2;
        ui->playlist->setParent(&tempHolder2);

        QLayout *oldPlaylistLayout = playlistTab->layout();
        if (oldPlaylistLayout) {
            QLayoutItem *item;
            while ((item = oldPlaylistLayout->takeAt(0)) != nullptr) {
                delete item;
            }
            delete oldPlaylistLayout;
        }

        // Playlist fills the whole tab now.
        QVBoxLayout *playlistVbox = new QVBoxLayout(playlistTab);
        playlistVbox->setContentsMargins(0, 0, 0, 0);
        playlistVbox->setSpacing(0);
        ui->playlist->setParent(playlistTab);
        playlistVbox->addWidget(ui->playlist);
        qDebug() << "Playlist tab now full-width; controls moved into side toolbox";

        // Sound-wave view of the playlist (crossfade preparation). The
        // toggle lives in a thin bar above the playlist; its state persists
        // in xfb.conf (PlaylistWaveView) and is restored by updateConfig().
        m_waveStore = new WaveformStore(this);
        m_waveView = new PlaylistWaveView(ui->playlist, m_waveStore, this);
        m_waveView->setNowPlayingProvider([this]() {
            return (Xplayer && Xplayer->source().isLocalFile())
                       ? Xplayer->source().toLocalFile() : QString();
        });

        auto *waveBar = new QWidget(playlistTab);
        auto *waveBarLayout = new QHBoxLayout(waveBar);
        waveBarLayout->setContentsMargins(4, 2, 4, 2);
        waveBarLayout->setSpacing(4);
        m_waveViewToggle = new QToolButton(waveBar);
        m_waveViewToggle->setText(tr("Wave view"));
        m_waveViewToggle->setCheckable(true);
        m_waveViewToggle->setToolTip(tr("Show each track's sound wave and prepare crossfades: "
                                        "drag a track's wave left to start it before the previous "
                                        "track ends, use the round button to preview the transition."));
        waveBarLayout->addWidget(m_waveViewToggle);

        // Max overlap window (how early the next track can be dragged to
        // start before the previous one ends). Persisted in xfb.conf;
        // shown only while the wave view is active.
        m_maxOverlapBox = new QWidget(waveBar);
        auto *maxOverlapLayout = new QHBoxLayout(m_maxOverlapBox);
        maxOverlapLayout->setContentsMargins(12, 0, 0, 0);
        maxOverlapLayout->setSpacing(4);
        auto *maxOverlapLabel = new QLabel(tr("Max overlap:"), m_maxOverlapBox);
        m_maxOverlapSpin = new QSpinBox(m_maxOverlapBox);
        m_maxOverlapSpin->setRange(5, 180);
        m_maxOverlapSpin->setSuffix(tr(" s"));
        m_maxOverlapSpin->setToolTip(tr("How early the next track can be dragged to start "
                                        "before the previous one ends. Increase this when a "
                                        "track has a long quiet tail you want the next one "
                                        "to start over."));
        {
            QSettings waveSettings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                       + "/xfb.conf", QSettings::IniFormat);
            const int maxOverlapSecs =
                qBound(5, waveSettings.value("MaxOverlapSeconds", 25).toInt(), 180);
            PlaylistWaveView::setMaxOverlapMs(qint64(maxOverlapSecs) * 1000);
            m_maxOverlapSpin->setValue(maxOverlapSecs);
        }
        connect(m_maxOverlapSpin, qOverload<int>(&QSpinBox::valueChanged),
                this, [this](int secs) {
            PlaylistWaveView::setMaxOverlapMs(qint64(secs) * 1000);
            QSettings waveSettings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                       + "/xfb.conf", QSettings::IniFormat);
            waveSettings.setValue("MaxOverlapSeconds", secs);
            if (m_waveView)
                m_waveView->refresh(); // the strips rescale to the new window
        });
        maxOverlapLayout->addWidget(maxOverlapLabel);
        maxOverlapLayout->addWidget(m_maxOverlapSpin);
        m_maxOverlapBox->setVisible(false); // shown when wave view turns on
        waveBarLayout->addWidget(m_maxOverlapBox);

        waveBarLayout->addStretch();
        playlistVbox->insertWidget(0, waveBar);

        // The playing track leaves the playlist when it starts, so its wave
        // and volume line live on in this strip (shown while wave view is on)
        m_nowPlayingWave = new NowPlayingWaveStrip(m_waveStore, playlistTab);
        playlistVbox->insertWidget(1, m_nowPlayingWave);
        connect(m_nowPlayingWave, &NowPlayingWaveStrip::envelopeEdited,
                this, [this](const QVector<QPointF> &points) {
            // Live edit of the on-air track's line: onPositionChanged
            // applies it on the next tick
            m_activeEnvelope = points;
            m_activeEnvelopePath = m_nowPlayingWave->track();
        });

        connect(m_waveViewToggle, &QToolButton::toggled,
                this, &player::setPlaylistWaveView);
    }
    
    // Verify UI was properly initialized
    qDebug() << "Checking UI elements...";
    if (!ui) {
        qCritical() << "UI object is null!";
        throw std::runtime_error("UI object is null");
    }
    
    qDebug() << "Checking sliderProgress...";
    if (!ui->sliderProgress) {
        qCritical() << "sliderProgress is null!";
        throw std::runtime_error("sliderProgress is null");
    }
    
    qDebug() << "Checking sliderVolume...";
    if (!ui->sliderVolume) {
        qCritical() << "sliderVolume is null!";
        throw std::runtime_error("sliderVolume is null");
    }
    
    qDebug() << "Checking playlist...";
    if (!ui->playlist) {
        qCritical() << "playlist is null!";
        throw std::runtime_error("playlist is null");
    }
    
    qDebug() << "Checking musicView...";
    if (!ui->musicView) {
        qCritical() << "musicView is null!";
        throw std::runtime_error("musicView is null");
    }
    
    qDebug() << "UI validation passed!";

    // Initialize timers (must be created in the constructor, not in the header)
    stimer = new QTimer(this);
    icetimer = new QTimer(this);
    butt_timer = new QTimer(this);
    adRefreshTimer = new QTimer(this);

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    networkManager = new QNetworkAccessManager(this);
/*
    // Only needs to be done once per application run
    QtWebEngineQuick::initialize();

    adBanner = new QQuickWidget(this);
    adBanner->setFixedHeight(90);
    // Set a minimum width based on Google's requirement (or your layout needs)
    adBanner->setMinimumWidth(728);
    adBanner->setResizeMode(QQuickWidget::SizeRootObjectToView);
    adBanner->setSource(QUrl("qrc:/AdView.qml")); // Load from resource

    ui->gridLayout_2->addWidget(adBanner, 25, 0, 1, 1);

    // Keep refresh logic if needed, but call the QML function
    adRefreshTimer = new QTimer(this); // Assuming adRefreshTimer is a member
    adRefreshTimer->setInterval(60000); // Refresh less often? e.g., 60 seconds
    connect(adRefreshTimer, &QTimer::timeout, this, &player::refreshAdBanner);
    adRefreshTimer->start();*/

    qDebug() << "Initializing database and UI components";
    
    // Initialize database connection with error handling
    try {
        updateConfig();
        if (!checkDbOpen()) {
            qCritical() << "Failed to open database - this may cause crashes";
            // Don't throw here, let the app try to continue
        } else {
            qDebug() << "Database opened successfully";
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception during database initialization:" << e.what();
        // Continue without database for now
    }
    //on_actionUpdate_Dinamic_Server_s_IP_triggered();

    // Initialize audio outputs for Qt6 - ensure they're created in the main thread
    qDebug() << "Initializing audio outputs in thread:" << QThread::currentThread();
    
    try {
        XplayerOutput = new QAudioOutput(this);
        lp1_XplayerOutput = new QAudioOutput(this);
        lp2_XplayerOutput = new QAudioOutput(this);
        qDebug() << "Audio outputs created successfully";
    } catch (const std::exception& e) {
        qCritical() << "Exception creating audio outputs:" << e.what();
        throw;
    }

    // Initialize media players with audio outputs - ensure main thread
    try {
        Xplayer = new FxPlayer(this);
        Xplayer->setAudioOutput(XplayerOutput);
        XplaylistIndex = 0;
        qDebug() << "Main player created successfully";

        lp1_Xplayer = new FxPlayer(this);
        lp1_Xplayer->setAudioOutput(lp1_XplayerOutput);
        lp1_XplaylistIndex = 0;
        qDebug() << "LP1 player created successfully";

        lp2_Xplayer = new FxPlayer(this);
        lp2_Xplayer->setAudioOutput(lp2_XplayerOutput);
        lp2_XplaylistIndex = 0;
        qDebug() << "LP2 player created successfully";

        // Overlap segue tail player: plays out (and fades) the end of the
        // outgoing track while the next one starts on the main player.
        m_tailOutput = new QAudioOutput(this);
        m_tailPlayer = new FxPlayer(this);
        m_tailPlayer->setAudioOutput(m_tailOutput);
        m_tailFade = new QVariantAnimation(this);
        connect(m_tailFade, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            if (m_tailOutput)
                m_tailOutput->setVolume(v.toFloat());
        });
        connect(m_tailFade, &QVariantAnimation::finished, this, [this]() {
            if (m_tailPlayer)
                m_tailPlayer->stop();
        });

        // Restore persisted FX settings (EQ / compressor / 432 Hz retune)
        applyStoredFxSettings();

        // LP decks always route through the FX engine so scratching and the
        // DJ effects work even with EQ/compressor off (falls back to plain
        // playback automatically when ffmpeg is missing)
        lp1_Xplayer->setPreferEngineAlways(true);
        lp2_Xplayer->setPreferEngineAlways(true);

        // Streaming client: FxPlayer routes http(s) URLs through the
        // ffmpeg-CLI engine (plain QMediaPlayer cannot play live streams).
        RadioPlayerOutput = new QAudioOutput(this);
        RadioPlayer = new FxPlayer(this);
        RadioPlayer->setAudioOutput(RadioPlayerOutput);
        RadioPlayerOutput->setVolume(ui->slider_rol_volume->value() / 100.0);
        connect(ui->slider_rol_volume, &QSlider::valueChanged, this, [this](int v) {
            if (RadioPlayerOutput)
                RadioPlayerOutput->setVolume(v / 100.0);
        });
        connect(RadioPlayer, &FxPlayer::playbackStateChanged, this,
                [this](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::PlayingState) {
                ui->bt_rol_streaming_play->setStyleSheet("background-color:#C8EE72");
                ui->lbl_rol_streaming_status->setText(tr("Playing: %1")
                    .arg(RadioPlayer->source().toDisplayString()));
            } else if (state == QMediaPlayer::StoppedState) {
                ui->bt_rol_streaming_play->setStyleSheet("");
                ui->lbl_rol_streaming_status->setText(tr("Stopped"));
            }
        });
        connect(RadioPlayer, &FxPlayer::mediaStatusChanged, this,
                [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::BufferingMedia || status == QMediaPlayer::LoadingMedia)
                ui->lbl_rol_streaming_status->setText(tr("Connecting / buffering..."));
            else if (status == QMediaPlayer::StalledMedia)
                ui->lbl_rol_streaming_status->setText(tr("Stream stalled — rebuffering..."));
            else if (status == QMediaPlayer::InvalidMedia)
                ui->lbl_rol_streaming_status->setText(tr("Invalid stream"));
        });
        connect(RadioPlayer, &FxPlayer::errorOccurred, this,
                [this](QMediaPlayer::Error, const QString &errorString) {
            ui->bt_rol_streaming_play->setStyleSheet("");
            ui->lbl_rol_streaming_status->setText(tr("Stream error: %1").arg(errorString));
        });
    } catch (const std::exception& e) {
        qCritical() << "Exception creating media players:" << e.what();
        throw;
    }

    indexcanal = 4;
    onAbout2Finish = 0;
    autoMode=1;
    recMode=0;
    PlayMode="stopped";
    tmpFullScreen=0;
    jingleCadaNumMusicas = 0;

    ui->txt_ProgramName->hide();
    ui->bt_ProgramStopandProcess->hide();
    ui->txt_uploadingPrograms->hide();
    ui->txt_creatingPrograms->hide();
    ui->led_rec->hide();
    ui->txt_loading->hide();

    // Initialize Qt6 media recording components with error handling
    qDebug() << "Initializing media recording components";
    try {
        captureSession = new QMediaCaptureSession(this);
        audioRecorder = new QMediaRecorder(this);
        audioInput = new QAudioInput(this);

        captureSession->setRecorder(audioRecorder);
        captureSession->setAudioInput(audioInput);
        qDebug() << "Media recording components initialized successfully";
    } catch (const std::exception& e) {
        qCritical() << "Exception initializing media recording:" << e.what();
        // Don't throw here, recording is not critical for basic functionality
        captureSession = nullptr;
        audioRecorder = nullptr;
        audioInput = nullptr;
    }

    // List available audio input devices
    const QList<QAudioDevice> inputDevices = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : inputDevices) {
        qDebug() << "Audio Hardware on this system: " << device.description();
    }

    // Get supported audio codecs using mediaFormat() in Qt6
    const QList<QMediaFormat::AudioCodec> supportedCodecs = audioRecorder->mediaFormat().supportedAudioCodecs(QMediaFormat::Encode);
    for (const QMediaFormat::AudioCodec &codec : supportedCodecs) {
        qDebug() << "Audio Codecs on this system: " << QMediaFormat::audioCodecName(codec);
    }

    // Get supported containers (file formats) in Qt6
    const QList<QMediaFormat::FileFormat> supportedContainers = audioRecorder->mediaFormat().supportedFileFormats(QMediaFormat::Encode);
    for (const QMediaFormat::FileFormat &format : supportedContainers) {
        qDebug() << "Audio Containers on this system: " << QMediaFormat::fileFormatName(format);
    }

    // Get supported sample rates - in Qt6 we need to check available quality settings
    QMediaRecorder::EncodingMode mode = audioRecorder->encodingMode();
    QMediaRecorder::Quality quality = audioRecorder->quality();
    qDebug() << "Current encoding mode: " << mode << ", quality: " << quality;
    qDebug() << "Audio Sample Rates are handled through quality settings in Qt6";

    // Connect media player signals with error handling
    qDebug() << "Connecting media player signals";
    try {
        connect(Xplayer, &FxPlayer::positionChanged, this, &player::onPositionChanged);
        connect(Xplayer, &FxPlayer::durationChanged, this, &player::durationChanged);
        connect(Xplayer, &FxPlayer::sourceChanged, this, &player::currentMediaChanged);
        connect(Xplayer->audioOutput(), &QAudioOutput::volumeChanged, this, &player::volumeChanged);
        // The wave view ghosts the playing track behind the first playlist row
        connect(Xplayer, &FxPlayer::sourceChanged, this, [this](const QUrl &url) {
            if (m_waveView)
                m_waveView->refresh();
            if (m_nowPlayingWave) {
                const bool local = url.isLocalFile();
                m_nowPlayingWave->setTrack(local ? url.toLocalFile() : QString());
                m_nowPlayingWave->setVisible(local && m_waveView
                                             && m_waveView->isActive());
            }
        });
        qDebug() << "Main player signals connected";

        // Handle media player errors (e.g., codec issues, corrupt files)
        connect(Xplayer, &FxPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString) {
            qWarning() << "Media player error:" << error << "-" << errorString;
            qWarning() << "Current source:" << Xplayer->source().toString();
            
            // Don't hang — recover gracefully
            if (PlayMode == "Playing_Segue" && !m_manualAdvancing) {
                qWarning() << "Error during segue playback, advancing to next track...";
                ui->statusBar->showMessage(tr("Playback error: %1 — skipping track").arg(errorString), 5000);
                // Use a timer to avoid re-entry issues
                QTimer::singleShot(200, this, &player::playNextMedia);
            } else {
                ui->statusBar->showMessage(tr("Playback error: %1").arg(errorString), 5000);
            }
        });

        // Handle media status changes (detect stalled/invalid media)
        connect(Xplayer, &FxPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
            qDebug() << "Media status changed:" << status;
            if (status == QMediaPlayer::InvalidMedia) {
                qWarning() << "Invalid media detected:" << Xplayer->source().toString();
                if (PlayMode == "Playing_Segue" && !m_manualAdvancing) {
                    ui->statusBar->showMessage(tr("Invalid media file — skipping"), 5000);
                    QTimer::singleShot(200, this, &player::playNextMedia);
                }
            } else if (status == QMediaPlayer::StalledMedia) {
                qWarning() << "Media playback stalled:" << Xplayer->source().toString();
                ui->statusBar->showMessage(tr("Playback stalled — buffering..."), 3000);
            }
        });

        // Connect media player signals for playlist management
        connect(Xplayer, &FxPlayer::playbackStateChanged, [this](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState && PlayMode == "Playing_Segue" && !m_manualAdvancing) {
                // When playback stops, play the next media if in segue mode
                QTimer::singleShot(100, this, &player::playNextMedia);
            }
        });
        qDebug() << "Main player playlist signals connected";

        connect(lp1_Xplayer, &FxPlayer::positionChanged, this, &player::lp1_onPositionChanged);
        connect(lp1_Xplayer, &FxPlayer::durationChanged, this, &player::lp1_durationChanged);
        connect(lp1_Xplayer, &FxPlayer::sourceChanged, this, &player::lp1_currentMediaChanged);
        connect(lp1_Xplayer->audioOutput(), &QAudioOutput::volumeChanged, this, &player::lp1_volumeChanged);
        qDebug() << "LP1 player signals connected";

        connect(lp2_Xplayer, &FxPlayer::positionChanged, this, &player::lp2_onPositionChanged);
        connect(lp2_Xplayer, &FxPlayer::durationChanged, this, &player::lp2_durationChanged);
        connect(lp2_Xplayer, &FxPlayer::sourceChanged, this, &player::lp2_currentMediaChanged);
        connect(lp2_Xplayer->audioOutput(), &QAudioOutput::volumeChanged, this, &player::lp2_volumeChanged);
        qDebug() << "LP2 player signals connected";
    } catch (const std::exception& e) {
        qCritical() << "Exception connecting media player signals:" << e.what();
        throw; // This is critical for functionality
    }

    // Watchdog timer: detects when playback stalls (position stops updating)
    m_playbackWatchdog = new QTimer(this);
    m_playbackWatchdog->setInterval(3000); // Check every 3 seconds
    connect(m_playbackWatchdog, &QTimer::timeout, this, [this]() {
        if (Xplayer->playbackState() != QMediaPlayer::PlayingState) {
            m_stallCount = 0;
            m_playbackWatchdog->stop();
            return;
        }
        
        qint64 currentPos = Xplayer->position();
        if (currentPos == m_lastKnownPosition && currentPos > 0) {
            m_stallCount++;
            qWarning() << "Playback stall detected! Position stuck at" << currentPos << "ms (count:" << m_stallCount << ")";
            
            if (m_stallCount >= 2) {
                // Playback has been stuck for 6+ seconds — force recovery
                qWarning() << "Forcing player recovery after stall on:" << Xplayer->source().toString();
                ui->statusBar->showMessage(tr("Playback stalled — recovering..."), 5000);
                
                m_manualAdvancing = true;
                Xplayer->stop();
                Xplayer->setSource(QUrl()); // Release stuck AVFoundation session
                m_manualAdvancing = false;
                m_stallCount = 0;
                m_playbackWatchdog->stop();
                
                // If in segue mode, advance to next track
                if (PlayMode == "Playing_Segue") {
                    QTimer::singleShot(300, this, &player::playNextMedia);
                } else {
                    PlayMode = "stopped";
                    ui->btPlay->setStyleSheet("");
                    ui->btPlay->setText(tr("Play"));
                }
            }
        } else {
            m_stallCount = 0;
        }
        m_lastKnownPosition = currentPos;
    });

    // Initialize recTimer before connecting it
    recTimer = new QTimer(this);
    connect(recTimer, &QTimer::timeout, this, &player::run_recTimer);

    /* main clock signals and slots */
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &player::showTime);
    timer->start(1000);
    showTime();

    if(Role=="Server"){

        QTimer *schedulerTimer = new QTimer(this);
        connect(schedulerTimer, &QTimer::timeout, this, &player::run_scheduler);
        schedulerTimer->start(60000);

        run_server_scheduler(); //run at startup

        server_this_day_of_the_week = QDate::currentDate().dayOfWeek();
        QTimer *schedulerTimerh = new QTimer(this);
        connect(schedulerTimerh, &QTimer::timeout, this, &player::run_server_scheduler);
        schedulerTimerh->start(3600000); //once per hour

        QTimer *schedulerTimerMT = new QTimer(this);
        connect(schedulerTimerMT, &QTimer::timeout, this, &player::monitorTakeOver);
        schedulerTimerMT->start(25000);

    }

    /*Populate music table with an editable table field on double-click*/
    qDebug() << "Initializing music table model";
    bool dbAvailable = false;
    try {
        if (!checkDbOpen()) {
            qWarning() << "Database not available for music table";
        } else {
            QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            if (!db.isValid() || !db.isOpen()) {
                qCritical() << "Database connection invalid for music table";
            } else {
                dbAvailable = true;
                QSqlTableModel *model = new QSqlTableModel(this, db);
                model->setTable("musics");
                if (!model->select()) {
                    qWarning() << "Failed to select from musics table:" << model->lastError().text();
                } else {
                    qDebug() << "Music table model created successfully";
                }

                ui->musicView->setModel(model);
                ui->musicView->setSortingEnabled(true);
                ui->musicView->hideColumn(0);
                ui->musicView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
                ui->musicView->setColumnWidth(1,150);
                ui->musicView->setColumnWidth(2,150);
                ui->musicView->setColumnWidth(3,80);
                ui->musicView->setColumnWidth(4,80);
                ui->musicView->setColumnWidth(5,60);
                ui->musicView->setColumnWidth(6,100);
                ui->musicView->setColumnWidth(7,300);
                ui->musicView->setColumnWidth(8,50);
                ui->musicView->setColumnWidth(9,80);
                ui->musicView->setColumnWidth(10,100);
                qDebug() << "Music table view configured successfully";
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception initializing music table:" << e.what();
        // Continue without music table — don't abort the entire constructor
    }
checkDbOpen();
    /*Populate jingles table with an editable table field on double-click*/
    if (dbAvailable) {
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlTableModel * jinglesmodel = new QSqlTableModel(this,db);
    jinglesmodel->setTable("jingles");
    jinglesmodel->select();
    ui->jinglesView->setModel(jinglesmodel);
    ui->jinglesView->setSortingEnabled(true);
    ui->jinglesView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Pub table*/

    QSqlTableModel *pubmodel = new QSqlTableModel(this,db);
    pubmodel->setTable("pub");
    pubmodel->select();
    ui->pubView->setModel(pubmodel);
    ui->pubView->setSortingEnabled(true);
    ui->pubView->hideColumn(0);
    ui->pubView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Programs table*/

    QSqlTableModel *programsmodel = new QSqlTableModel(this,db);
    programsmodel->setTable("programs");
    programsmodel->select();
    ui->programsView->setModel(programsmodel);
    ui->programsView->setSortingEnabled(true);
    ui->programsView->hideColumn(0);
    ui->programsView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Torrents table*/

    QSqlTableModel *torrentsmodel = new QSqlTableModel(this,db);
    torrentsmodel->setTable("torrents");
    torrentsmodel->select();
    ui->torrentsView->setModel(torrentsmodel);
    ui->torrentsView->setSortingEnabled(true);
    ui->torrentsView->hideColumn(0);
    ui->torrentsView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    } // end if (dbAvailable)

    /*Drag & Drop Set*/

    /*player*/
     this->setAcceptDrops(true);
    /*playlist*/
     ui->playlist->setSelectionMode(QAbstractItemView::SingleSelection);
     ui->playlist->setDragEnabled(true);
     ui->playlist->viewport()->setAcceptDrops(true);
     ui->playlist->setAcceptDrops(true);
     ui->playlist->setDropIndicatorShown(false);
     ui->playlist->setDragDropMode(QAbstractItemView::InternalMove);
     
     // Accessibility improvements for playlist
     ui->playlist->setFocusPolicy(Qt::StrongFocus);
     ui->playlist->setAttribute(Qt::WA_KeyboardFocusChange, true);
    /*Music list*/
     ui->musicView->setSelectionMode(QAbstractItemView::ExtendedSelection);
     ui->musicView->setDragEnabled(true);
     ui->musicView->viewport()->setAcceptDrops(false);
     ui->musicView->setAcceptDrops(false);
     ui->musicView->setDropIndicatorShown(true);
     ui->musicView->setDragDropMode(QAbstractItemView::DragOnly);
     ui->musicView->setSelectionBehavior(QAbstractItemView::SelectRows);
     
     // Accessibility improvements for keyboard navigation
     ui->musicView->setFocusPolicy(Qt::StrongFocus);
     ui->musicView->setTabKeyNavigation(true);
     ui->musicView->setAttribute(Qt::WA_KeyboardFocusChange, true);

     ui->musicView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->musicView, &QWidget::customContextMenuRequested,
         this, &player::musicViewContextMenu);

     ui->playlist->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->playlist, &QWidget::customContextMenuRequested,
             this, &player::playlistContextMenu);

     // Accessibility improvements for jinglesView
     ui->jinglesView->setFocusPolicy(Qt::StrongFocus);
     ui->jinglesView->setTabKeyNavigation(true);
     ui->jinglesView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->jinglesView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->jinglesView, &QWidget::customContextMenuRequested,
         this, &player::jinglesViewContextMenu);

     // Accessibility improvements for pubView
     ui->pubView->setFocusPolicy(Qt::StrongFocus);
     ui->pubView->setTabKeyNavigation(true);
     ui->pubView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->pubView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->pubView, &QWidget::customContextMenuRequested,
         this, &player::pubViewContextMenu);

     // Accessibility improvements for programsView
     ui->programsView->setFocusPolicy(Qt::StrongFocus);
     ui->programsView->setTabKeyNavigation(true);
     ui->programsView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->programsView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->programsView, &QWidget::customContextMenuRequested,
         this, &player::programsViewContextMenu);

     // Accessibility improvements for torrentsView
     ui->torrentsView->setFocusPolicy(Qt::StrongFocus);
     ui->torrentsView->setTabKeyNavigation(true);
     ui->torrentsView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->torrentsView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->torrentsView, &QWidget::customContextMenuRequested,
         this, &player::torrentsViewContextMenu);

     // Genre combo boxes will be populated by update_music_table()

    update_music_table();

   /*Bottom info*/
   QDir dir; QString cpath = dir.absolutePath();
   QString binfo = ui->txt_bottom_info->text()+"\n"+cpath+"\n"+Role;
   ui->txt_bottom_info->setText(binfo);

   /*Default button states*/
   ui->bt_takeOver->setEnabled(false);
   ui->bt_pause_rec->setEnabled(false);



   if(Role=="Server"){

       autoMode = 1;
       qDebug()<<"Role is set to Server, so autoMode is ON by default";
       ui->bt_autoMode->setStyleSheet("background-color: rgb(175, 227, 59)");
       ui->bt_takeOver->setHidden(true);
       ui->menuClient_3->setEnabled(false);

   }else{

       qDebug("XFB is now running in client mode!");

       autoMode = 0;
       qDebug()<<"autoMode is OFF";
       ui->bt_autoMode->setStyleSheet("");
       ui->menuServer->setEnabled(false);

   }


    // Show the donation dialog on every startup (skipped only for scripted
    // runs via --no-dialogs)
    if (!QApplication::arguments().contains("--no-dialogs")) {
        try {
            QPixmap pixmap(":/images/donate.png");
            CustomMessageBox msgBox(tr("Donate to the Developer!"), tr("Please support the development of XFB!<br>If you appreciate this software, kindly consider making a donation to support the developer!<br><a href=\"https://www.paypal.com/donate/?hosted_button_id=TFDSZU78WLMC6\">Donate via PayPal!</a><br>Contact for professional support and custom development!<br><br>Why did the computer get a little emotional when using WinRAR?<br>_Because even after all the \"evaluation\", it still felt unzipped!"), pixmap);
            msgBox.exec();
        } catch (const std::exception& e) {
            qWarning() << "Exception showing donation dialog:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception showing donation dialog";
        }
    } else {
        qDebug() << "Skipping donation dialog (--no-dialogs flag)";
    }

   // Directly set style on the status bar
   if(darkMode){
       qDebug("Loading darkmode");
       ui->statusBar->setStyleSheet("background-color: #353535 !important; color: #ffffff; border: none; margin: 0; padding: 0;");

   } else {
       qDebug("Loading lightmode");
       this->setStyleSheet("background-color: #ffffff");
       ui->statusBar->setStyleSheet("background-color: #ffffff !important; color: #303030; border: none; margin: 0; padding: 0;");


   }

   // Initialize torrent services with error handling (only if GUI is available)
   if (!QApplication::arguments().contains("--version") && 
       !QApplication::arguments().contains("--help") &&
       !QApplication::arguments().contains("-v") &&
       !QApplication::arguments().contains("-h") &&
       !QApplication::arguments().contains("--minimal")) {
       
       try {
           // Note: external tools (Tor, a torrent client, etc.) are NOT installed
           // at startup. They are installed on-demand, with the user's explicit
           // consent, the first time a feature actually needs them (see
           // DependencyChecker::ensureDependency). Here we only log what's missing.
           {
               DependencyChecker depChecker;
               QList<DependencyInfo> missing = depChecker.checkDependencies();
               if (!missing.isEmpty()) {
                   QStringList names;
                   for (const auto &d : missing) names << d.name;
                   qInfo() << "Optional dependencies not installed:" << names.join(", ")
                           << "— will be offered on first use.";
               }
           }

           // Proactively provision the core download toolchain (yt-dlp + ffmpeg)
           // shortly after startup, so the FIRST download doesn't stall waiting
           // for an install. We defer with a single-shot timer so the main
           // window is visible before any consent dialog appears (running the
           // modal prompts in the constructor would block before the UI shows).
           //
           // This is done once (guarded by a config flag): if the tools are
           // already present it's a no-op, and if the user declines we don't
           // nag on every launch — the on-demand prompts (opening the downloader
           // or starting a download) still cover them.
           {
               const QString cfgPath =
                   QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                   + "/xfb.conf";
               QSettings depSettings(cfgPath, QSettings::IniFormat);
               const bool alreadyPrompted =
                   depSettings.value("StartupDepsProvisioned", false).toBool();

               const bool haveFfmpeg = DependencyChecker::isAvailable("ffmpeg");
               const bool haveYtdlp =
                   QFileInfo(DependencyChecker::localYtDlpPath()).isExecutable() ||
                   DependencyChecker::isAvailable("yt-dlp");

               if (!alreadyPrompted && !(haveFfmpeg && haveYtdlp)) {
                   QTimer::singleShot(1500, this, [this]() {
                       DependencyChecker depChecker;
                       // Self-updating yt-dlp in ~/.local/bin (no admin needed).
                       depChecker.ensureYtDlp(this);
                       // ffmpeg also provides ffprobe; on macOS this bootstraps
                       // Homebrew first if it isn't installed.
                       depChecker.ensureDependency("ffmpeg",
                           tr("Downloading audio needs FFmpeg (which also provides ffprobe). "
                              "Installing it now means your first download won't have to wait."),
                           this);

                       const QString cfgPath =
                           QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/xfb.conf";
                       QSettings s(cfgPath, QSettings::IniFormat);
                       s.setValue("StartupDepsProvisioned", true);
                   });
               }
           }

           m_torNetworkService = new TorNetworkService(this);
           m_torrentSearchService = new TorrentSearchService(this);
           m_torrentDownloadService = new TorrentDownloadService(this);
           
           // Initialize the services safely
           if (m_torNetworkService && !m_torNetworkService->initialize()) {
               qWarning() << "Failed to initialize TorNetworkService";
           }
           if (m_torrentSearchService && !m_torrentSearchService->initialize()) {
               qWarning() << "Failed to initialize TorrentSearchService";
           }
           if (m_torrentDownloadService && !m_torrentDownloadService->initialize()) {
               qWarning() << "Failed to initialize TorrentDownloadService";
           }
       } catch (const std::exception& e) {
           qWarning() << "Exception during torrent service initialization:" << e.what();
           // Set services to nullptr if initialization fails
           m_torNetworkService = nullptr;
           m_torrentSearchService = nullptr;
           m_torrentDownloadService = nullptr;
       }
   } else {
       // Set services to nullptr for command line usage or minimal mode
       qDebug() << "Skipping torrent services initialization";
       m_torNetworkService = nullptr;
       m_torrentSearchService = nullptr;
       m_torrentDownloadService = nullptr;
   }
   
   // Connect torrent services with null checks
   if (m_torrentSearchService && m_torNetworkService) {
       m_torrentSearchService->setTorService(m_torNetworkService);
   }
   if (m_torrentDownloadService && m_torNetworkService) {
       m_torrentDownloadService->setTorService(m_torNetworkService);
   }
   
   // Connect torrent network signals with null checks
   if (m_torNetworkService) {
       connect(m_torNetworkService, &TorNetworkService::torReady,
               this, &player::onTorReady);
       connect(m_torNetworkService, &TorNetworkService::torStopped,
               this, &player::onTorDisconnected);
       connect(m_torNetworkService, &TorNetworkService::torError,
               this, &player::onTorError);
       connect(m_torNetworkService, &TorNetworkService::onionMirrorFound,
               this, &player::onOnionMirrorFound);
       connect(m_torNetworkService, &TorNetworkService::searchingForOnionMirror,
               this, &player::onSearchingForOnionMirror);
       connect(m_torNetworkService, &TorNetworkService::onionMirrorSearchFailed,
               this, &player::onOnionMirrorSearchFailed);
   }
   
   // Connect torrent search signals with null checks
   if (m_torrentSearchService) {
       connect(m_torrentSearchService, &TorrentSearchService::resultsReady,
               this, &player::onTorrentSearchResults);
       connect(m_torrentSearchService, &TorrentSearchService::searchError,
               this, &player::onTorrentSearchError);
       connect(m_torrentSearchService, &TorrentSearchService::searchProgress,
               ui->torrentSearchProgress, &QProgressBar::setValue);
       connect(m_torrentSearchService, &TorrentSearchService::searchStarted,
               [this]() { ui->torrentSearchProgress->setVisible(true); });
       connect(m_torrentSearchService, &TorrentSearchService::searchFinished,
               [this]() { ui->torrentSearchProgress->setVisible(false); });
       connect(m_torrentSearchService, &TorrentSearchService::onionSitesUnavailable,
               this, &player::onOnionSitesUnavailable);
   }
   
   // Connect torrent download signals with null checks
   if (m_torrentDownloadService) {
       connect(m_torrentDownloadService, &TorrentDownloadService::downloadCompleted,
               this, &player::onTorrentDownloadCompleted);
       connect(m_torrentDownloadService, &TorrentDownloadService::streamingReady,
               this, &player::onTorrentStreamingReady);

       // Show downloads panel whenever a download starts
       connect(m_torrentDownloadService, &TorrentDownloadService::downloadStarted,
               this, [this](const QString &) {
           ui->downloadsPanel->setVisible(true);
           updateDownloadsCountLabel();
       });
       connect(m_torrentDownloadService, &TorrentDownloadService::downloadProgress,
               this, [this](const QString &, double) {
           updateDownloadsCountLabel();
       });
       connect(m_torrentDownloadService, &TorrentDownloadService::downloadError,
               this, [this](const QString &, const QString &error) {
           ui->statusBar->showMessage(tr("Download error: %1").arg(error), 5000);
           updateDownloadsCountLabel();
       });
       connect(m_torrentDownloadService, &TorrentDownloadService::downloadCancelled,
               this, [this](const QString &) {
           updateDownloadsCountLabel();
       });
       connect(m_torrentDownloadService, &TorrentDownloadService::downloadsChanged,
               this, [this]() {
           updateDownloadsCountLabel();
       });

       // Wire the downloads table view to the model
       ui->downloadsTableView->setModel(m_torrentDownloadService->getDownloadsModel());
       // Allow user to resize all columns freely; last column stretches to fill
       ui->downloadsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
       ui->downloadsTableView->horizontalHeader()->setStretchLastSection(true);
       ui->downloadsTableView->setColumnWidth(0, 300);  // Name
       ui->downloadsTableView->setColumnWidth(1, 80);   // Progress
       ui->downloadsTableView->setColumnWidth(2, 120);  // Status
   }

   // Downloads panel buttons
   if (m_torrentDownloadService) {
       connect(ui->downloadCancelSelectedButton, &QPushButton::clicked, this, [this]() {
           QModelIndex idx = ui->downloadsTableView->currentIndex();
           if (idx.isValid()) {
               QString dlId = m_torrentDownloadService->downloadIdForRow(idx.row());
               if (!dlId.isEmpty()) {
                   m_torrentDownloadService->cancelDownload(dlId);
                   ui->statusBar->showMessage(tr("Download cancelled"), 3000);
               }
           }
       });
       connect(ui->downloadCancelAllButton, &QPushButton::clicked, this, [this]() {
           m_torrentDownloadService->cancelAllDownloads();
           ui->statusBar->showMessage(tr("All downloads cancelled"), 3000);
       });
       connect(ui->downloadClearFinishedButton, &QPushButton::clicked, this, [this]() {
           m_torrentDownloadService->removeCompleted();
           if (m_torrentDownloadService->activeDownloadCount() == 0) {
               ui->downloadsPanel->setVisible(false);
           }
       });
       connect(ui->downloadOpenFolderButton, &QPushButton::clicked, this, [this]() {
           QDesktopServices::openUrl(QUrl::fromLocalFile(m_torrentDownloadService->downloadDirectory()));
       });
       connect(ui->downloadRetryButton, &QPushButton::clicked, this, [this]() {
           QModelIndex idx = ui->downloadsTableView->currentIndex();
           if (idx.isValid()) {
               QString dlId = m_torrentDownloadService->downloadIdForRow(idx.row());
               if (!dlId.isEmpty()) {
                   if (m_torrentDownloadService->retryDownload(dlId)) {
                       ui->statusBar->showMessage(tr("Retrying download..."), 3000);
                   } else {
                       ui->statusBar->showMessage(tr("Cannot retry this download"), 3000);
                   }
               }
           }
       });
   }
   
   // Set torrents view model with null check
   if (m_torrentSearchService) {
       ui->torrentsView->setModel(m_torrentSearchService->getResultsModel());
   }

   // --- Make torrentsView and downloadsPanel resizable via QSplitter ---
   {
       auto *splitter = new QSplitter(Qt::Vertical, ui->torrentsView->parentWidget());
       splitter->setChildrenCollapsible(false);

       // Reparent torrentsView and downloadsPanel into the splitter
       auto *parentLayout = qobject_cast<QVBoxLayout*>(ui->torrentsView->parentWidget()->layout());
       if (parentLayout) {
           // Find and remove torrentsView + downloadsPanel from the layout
           parentLayout->removeWidget(ui->torrentsView);
           parentLayout->removeWidget(ui->downloadsPanel);

           // Add them to the splitter instead
           splitter->addWidget(ui->torrentsView);
           splitter->addWidget(ui->downloadsPanel);

           // Search results get most space, downloads panel gets less
           splitter->setStretchFactor(0, 3);
           splitter->setStretchFactor(1, 1);
           splitter->setSizes({400, 150});

           // Insert splitter where torrentsView was (after search progress bar)
           parentLayout->addWidget(splitter);
       }

       // Wire collapse/expand toggle for downloads table
       connect(ui->downloadsCollapseButton, &QPushButton::clicked, this, [this]() {
           bool visible = ui->downloadsTableView->isVisible();
           ui->downloadsTableView->setVisible(!visible);
           ui->downloadsCollapseButton->setText(visible ? QStringLiteral("▶") : QStringLiteral("▼"));
       });
   }

   // Note: The main layout uses QGridLayout with absolute-positioned children
   // in the right panel, so only the torrents tab internal splitter is used.
   
   // --- Make top tabs and bottom panels resizable via vertical QSplitter ---
   {
       auto *mainLayout = qobject_cast<QGridLayout*>(ui->widget->layout());
       if (mainLayout && ui->tabWidget_2 && ui->pubWidget) {
           auto *vSplitter = new QSplitter(Qt::Vertical, ui->widget);
           vSplitter->setChildrenCollapsible(false);

           // Remove tabWidget_2 and pubWidget from the grid
           mainLayout->removeWidget(ui->tabWidget_2);
           mainLayout->removeWidget(ui->pubWidget);

           vSplitter->addWidget(ui->tabWidget_2);
           vSplitter->addWidget(ui->pubWidget);

           vSplitter->setStretchFactor(0, 3);
           vSplitter->setStretchFactor(1, 1);

           // Place splitter in column 0
           mainLayout->addWidget(vSplitter, 20, 0, 5, 1);

           // Move frame contents (clock stays on top, auto mode + playlist controls go to Controls tab)
           if (ui->frame && ui->page_FTP_Connection) {
               // Create a wrapper that holds the clock on top and the toolbox below
               auto *rightPanel = new QWidget();
               rightPanel->setMinimumWidth(350);
               rightPanel->setMaximumWidth(350);
               auto *rightLayout = new QVBoxLayout(rightPanel);
               rightLayout->setContentsMargins(0, 0, 0, 0);
               rightLayout->setSpacing(0);

               // Keep the clock on top (outside the toolbox)
               ui->txt_horas->setParent(rightPanel);
               ui->txt_horas->setMinimumHeight(70);
               ui->txt_horas->setMaximumHeight(90);
               rightLayout->addWidget(ui->txt_horas);

               // Small margin between clock and toolbox
               rightLayout->addSpacing(6);

               // Remove page_FTP_Connection from the grid before reparenting
               mainLayout->removeWidget(ui->page_FTP_Connection);

               // Move the toolbox below the clock
               rightLayout->addWidget(ui->page_FTP_Connection, 1);

               // Create the Controls page content
               auto *controlsPage = new QWidget();
               auto *controlsLayout = new QVBoxLayout(controlsPage);
               controlsLayout->setContentsMargins(5, 5, 5, 5);
               controlsLayout->setSpacing(6);

               // Auto Mode button
               ui->bt_autoMode->setParent(controlsPage);
               ui->bt_autoMode->setMinimumHeight(24);
               ui->bt_autoMode->setMaximumHeight(28);
               controlsLayout->addWidget(ui->bt_autoMode);

               // Total time label from playlist panel
               ui->txt_playlistTotalTime->setParent(controlsPage);
               controlsLayout->addWidget(ui->txt_playlistTotalTime);

               // Checkboxes and controls from frame_2's layoutWidget
               ui->checkBox_sum_to_playlist_time->setParent(controlsPage);
               ui->checkBox_sum_to_playlist_time->setMaximumWidth(16777215);
               controlsLayout->addWidget(ui->checkBox_sum_to_playlist_time);

               ui->checkBox_update_last_played_values->setParent(controlsPage);
               controlsLayout->addWidget(ui->checkBox_update_last_played_values);

               // Random jingle row
               auto *jingleRow = new QHBoxLayout();
               ui->checkBox_random_jingles->setParent(controlsPage);
               jingleRow->addWidget(ui->checkBox_random_jingles);
               ui->spinBox_random_jingles_interval->setParent(controlsPage);
               jingleRow->addWidget(ui->spinBox_random_jingles_interval);
               ui->label_9->setParent(controlsPage);
               jingleRow->addWidget(ui->label_9);
               jingleRow->addStretch();
               controlsLayout->addLayout(jingleRow);

               // Random add songs row
               auto *randomRow = new QHBoxLayout();
               ui->label_3->setParent(controlsPage);
               randomRow->addWidget(ui->label_3);
               ui->spinBox_num_of_songs_to_add_random->setParent(controlsPage);
               randomRow->addWidget(ui->spinBox_num_of_songs_to_add_random);
               ui->comboBox_random_add_genre->setParent(controlsPage);
               randomRow->addWidget(ui->comboBox_random_add_genre);
               ui->label_10->setParent(controlsPage);
               randomRow->addWidget(ui->label_10);
               ui->bt_add_some_random_songs_from_genre->setParent(controlsPage);
               randomRow->addWidget(ui->bt_add_some_random_songs_from_genre);
               randomRow->addStretch();
               controlsLayout->addLayout(randomRow);

               controlsLayout->addStretch();

               // Insert Controls as the first page in the toolbox
               ui->page_FTP_Connection->insertItem(0, controlsPage,
                   QIcon(QStringLiteral(":/icons/flat/Einstein-48.png")),
                   tr("Controls"));
               ui->page_FTP_Connection->setCurrentIndex(0);

               // Hide the now-empty frame and frame_2
               ui->frame->setVisible(false);
               mainLayout->removeWidget(ui->frame);
               ui->frame_2->setVisible(false);

               // Place the right panel wrapper in the grid
               mainLayout->addWidget(rightPanel, 0, 1, 25, 1);
           }
       }
   }

   // Load persisted download state and resume incomplete downloads
   if (m_torrentDownloadService) {
       m_torrentDownloadService->loadDownloadState();
       m_torrentDownloadService->resumeDownloads();
   }
   
   // Initialize UI state for Tor controls after constructor completes
   QTimer::singleShot(0, [this]() {
       updateTorConnectionUI(false);
   });

   // DO NOT auto-start Tor - user must manually connect

   // Register accessibility services with the service container
   if (!QApplication::arguments().contains("--minimal")) {
       try {
           registerAccessibilityServices();
           initializeAccessibility();
       } catch (const std::exception& e) {
           qWarning() << "Exception during accessibility initialization:" << e.what();
       }
   } else {
       qDebug() << "Skipping accessibility initialization (--minimal mode)";
   }

   // Start with the Musics tab selected in the bottom panel
   ui->pubWidget->setCurrentWidget(ui->tabMusic);

   // ngrok tunnel: public share link for the local streaming server
   {
       m_ngrokService = new NgrokTunnelService(this);

       connect(m_ngrokService, &NgrokTunnelService::tunnelStarted, this,
               [this](const QString &publicUrl) {
           ui->txt_ngrok_url->setText(publicUrl);
           ui->bt_ngrok->setText(tr("Stop"));
           ui->bt_ngrok->setStyleSheet("background-color:#C8EE72");
           ui->lbl_ngrok_status->setText(tr("Public link active! Share it with your listeners "
                                            "(they may need to append your mount point, e.g. /stream)."));
       });
       connect(m_ngrokService, &NgrokTunnelService::tunnelStopped, this, [this]() {
           ui->txt_ngrok_url->clear();
           ui->bt_ngrok->setText(tr("Share"));
           ui->bt_ngrok->setStyleSheet("");
           ui->lbl_ngrok_status->setText(tr("Stopped"));
       });
       connect(m_ngrokService, &NgrokTunnelService::tunnelError, this,
               [this](const QString &message) {
           ui->txt_ngrok_url->clear();
           ui->bt_ngrok->setText(tr("Share"));
           ui->bt_ngrok->setStyleSheet("");
           ui->lbl_ngrok_status->setText(tr("ngrok error: %1").arg(message));
       });
       connect(m_ngrokService, &NgrokTunnelService::authTokenConfigured, this,
               [this](bool ok, const QString &message) {
           ui->lbl_ngrok_status->setText(message);
           if (ok) {
               QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                      + "/xfb.conf", QSettings::IniFormat);
               settings.setValue("NgrokConfigured", true);
           }
       });

       // Restore the last used tunnel port
       QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                              + "/xfb.conf", QSettings::IniFormat);
       const int ngrokPort = settings.value("NgrokLocalPort", 8888).toInt();
       ui->txt_ngrok_port->setText(QString::number(ngrokPort));
   }

   // Update notifications: check GitHub releases shortly after startup and
   // then once a day; the user can also check manually (XFB → Update System)
   {
       m_updateService = new UpdateCheckService(networkManager, this);

       connect(m_updateService, &UpdateCheckService::updateAvailable, this,
               [this](const QString &version, const QUrl &releasePage,
                      const QUrl &downloadUrl, const QString & /*notes*/) {
           QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                  + "/xfb.conf", QSettings::IniFormat);
           if (!m_updateCheckManual
               && settings.value("SkipUpdateVersion").toString() == version) {
               qInfo() << "Update" << version << "available but skipped by user preference";
               return;
           }
           notifyUpdateAvailable(version, releasePage, downloadUrl);
       });
       connect(m_updateService, &UpdateCheckService::upToDate, this,
               [this](const QString &current) {
           QMessageBox::information(this, tr("Check for updates"),
               tr("You are running XFB %1 — this is the latest version.").arg(current));
       });
       connect(m_updateService, &UpdateCheckService::checkFailed, this,
               [this](const QString &error) {
           QMessageBox::warning(this, tr("Check for updates"),
               tr("Could not check for updates:\n%1").arg(error));
       });

       if (!QApplication::arguments().contains("--no-dialogs")) {
           QTimer::singleShot(20000, this, [this]() {
               m_updateCheckManual = false;
               m_updateService->checkNow(false);
           });
           QTimer *dailyUpdateTimer = new QTimer(this);
           connect(dailyUpdateTimer, &QTimer::timeout, this, [this]() {
               m_updateCheckManual = false;
               m_updateService->checkNow(false);
           });
           dailyUpdateTimer->start(24 * 60 * 60 * 1000);
       }
   }

   // 432 Hz conversion menu entry (the live EQ/compressor controls live in
   // the Audio FX tab next to the DJ tab)
   {
       QAction *conv432All = new QAction(QIcon(":/icons/flat/tuning-fork-64.png"),
                                         tr("Convert all musics in the database to 432 Hz tuning"), this);
       ui->menuDatabase->addAction(conv432All);
       connect(conv432All, &QAction::triggered, this, &player::convertAllMusicsTo432);
   }

   // DJ decks: scratchable platters + performance FX
   {
       m_scratchClock.start();
       ui->lp_1->installEventFilter(this);
       ui->lp_2->installEventFilter(this);
       ui->lp_1->setCursor(Qt::OpenHandCursor);
       ui->lp_2->setCursor(Qt::OpenHandCursor);
       ui->lp_1->setToolTip(tr("Grab the platter to scratch while playing"));
       ui->lp_2->setToolTip(tr("Grab the platter to scratch while playing"));

       auto wireDeckFx = [this](QDial *filterDial, QDial *echoDial, QPushButton *brakeBtn,
                                QPushButton *backspinBtn, FxPlayer *deck, int deckIdx) {
           auto applyFx = [filterDial, echoDial, deck]() {
               deck->setDjFx(filterDial->value() / 100.0, echoDial->value() / 100.0);
           };
           connect(filterDial, &QDial::valueChanged, this, applyFx);
           connect(echoDial, &QDial::valueChanged, this, applyFx);
           connect(brakeBtn, &QPushButton::clicked, this, [this, deck, deckIdx]() {
               startPlatterEffectAnimation(deckIdx, false);
               deck->djBrake();
           });
           connect(backspinBtn, &QPushButton::clicked, this, [this, deck, deckIdx]() {
               startPlatterEffectAnimation(deckIdx, true);
               deck->djBackspin();
           });
       };
       wireDeckFx(ui->lp1_dial_filter, ui->lp1_dial_echo,
                  ui->lp1_bt_brake, ui->lp1_bt_backspin, lp1_Xplayer, 0);
       wireDeckFx(ui->lp2_dial_filter, ui->lp2_dial_echo,
                  ui->lp2_bt_brake, ui->lp2_bt_backspin, lp2_Xplayer, 1);

       // When a deck stops on its own (brake, backspin, natural end),
       // restore its play button and platter artwork
       connect(lp1_Xplayer, &FxPlayer::playbackStateChanged, this,
               [this](QMediaPlayer::PlaybackState s) {
           if (s == QMediaPlayer::StoppedState) {
               if (m_lpPlatterAnim[0])
                   m_lpPlatterAnim[0]->stop();
               ui->lp_1_bt_play->setDisabled(false);
               ui->lp_1->setPixmap(QPixmap(":/images/lp_player_p0.png"));
               if (movie) movie->stop();
               lp_1_paused = false;
               ui->lp_1_bt_pause->setStyleSheet("");
           }
       });
       connect(lp2_Xplayer, &FxPlayer::playbackStateChanged, this,
               [this](QMediaPlayer::PlaybackState s) {
           if (s == QMediaPlayer::StoppedState) {
               if (m_lpPlatterAnim[1])
                   m_lpPlatterAnim[1]->stop();
               ui->lp_1_bt_play_2->setDisabled(false);
               ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p0.png"));
               if (movie2) movie2->stop();
               lp_2_paused = false;
               ui->lp_2_bt_pause->setStyleSheet("");
           }
       });
   }

   // Audio FX tab, right after the DJ tab (hide it via Options → ShowFxTab)
   {
       m_fxTabWidget = new AudioFxWidget(Xplayer, lp1_Xplayer, lp2_Xplayer, this);
       auto *fxScroll = new QScrollArea(this);
       fxScroll->setWidgetResizable(true);
       fxScroll->setFrameShape(QFrame::NoFrame);
       fxScroll->setWidget(m_fxTabWidget);
       m_fxTabPage = fxScroll;

       QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                              + "/xfb.conf", QSettings::IniFormat);
       if (settings.value("ShowFxTab", true).toBool()) {
           const int djIndex = ui->tabWidget_2->indexOf(ui->tab_dj);
           ui->tabWidget_2->insertTab(djIndex + 1, m_fxTabPage,
                                      QIcon(":/icons/flat/eq-fx-64.png"), tr("Audio FX"));
       }

       // The same settings are editable from the menu dialog too — refresh
       // the tab's controls whenever it becomes visible.
       connect(ui->tabWidget_2, &QTabWidget::currentChanged, this, [this](int idx) {
           if (m_fxTabPage && m_fxTabWidget && ui->tabWidget_2->widget(idx) == m_fxTabPage)
               m_fxTabWidget->reloadFromSettings();
       });
   }

   qDebug() << "Player constructor completed successfully";


}

void player::registerAccessibilityServices()
{
    try {
        auto* serviceContainer = ServiceContainer::instance();
        if (!serviceContainer) {
            qWarning() << "ServiceContainer not available - cannot register accessibility services";
            return;
        }
        
        // Register AccessibilityManager as a singleton service using template method
        serviceContainer->registerSingleton<AccessibilityManager>();
        
        // Register AudioFeedbackService as a singleton service using template method
        serviceContainer->registerSingleton<AudioFeedbackService>();
        
        // Register LiveRegionManager as a singleton service using template method
        serviceContainer->registerSingleton<LiveRegionManager>();
        
        // Register PlaybackStatusAnnouncer as a singleton service using template method
        serviceContainer->registerSingleton<PlaybackStatusAnnouncer>();
        
        // Register SystemStatusAnnouncer as a singleton service using template method
        serviceContainer->registerSingleton<SystemStatusAnnouncer>();
        
        qDebug() << "Accessibility services registered successfully";
    } catch (const std::exception& e) {
        qCritical() << "Exception registering accessibility services:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception registering accessibility services";
    }
}

void player::initializeAccessibility()
{
    try {
        // Get the accessibility manager from the service container
        auto* serviceContainer = ServiceContainer::instance();
        if (!serviceContainer) {
            qWarning() << "ServiceContainer not available - accessibility features disabled";
            return;
        }
        
        auto* accessibilityManager = serviceContainer->resolve<AccessibilityManager>();
        
        if (accessibilityManager) {
            // Initialize player-specific accessibility enhancements
            if (accessibilityManager->initializePlayerAccessibility(this)) {
                qDebug() << "Player accessibility initialized successfully";
            } else {
                qWarning() << "Failed to initialize player accessibility";
            }
        } else {
            qWarning() << "AccessibilityManager not available - accessibility features disabled";
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception during accessibility initialization:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception during accessibility initialization";
    }
}

void player::closeEvent(QCloseEvent *event)
{
    // Force immediate clean exit on macOS to prevent crash-on-close.
    // All cleanup that matters (Tor process, database) is handled by the OS
    // when the process terminates. The Tor child process will receive SIGHUP.
#ifdef Q_OS_MAC
    // Stop Tor process — it's a child process that should be terminated explicitly
    if (m_torNetworkService) {
        m_torNetworkService->stopTorProcess();
    }
    // Stop external radio process
    if (radio1.state() == QProcess::Running) {
        radio1.kill();
    }
    // Stop the ngrok tunnel — _exit() below would orphan the agent process
    if (m_ngrokService) {
        m_ngrokService->stop();
    }
    // Force exit — bypasses all Qt destruction which causes the SIGSEGV
    _exit(0);
#endif

    // Non-macOS: do graceful shutdown
    if (QApplication* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->disconnect(this);
    }
    
    if (ui) {
        if (ui->musicView) ui->musicView->setModel(nullptr);
        if (ui->jinglesView) ui->jinglesView->setModel(nullptr);
        if (ui->pubView) ui->pubView->setModel(nullptr);
        if (ui->programsView) ui->programsView->setModel(nullptr);
        if (ui->torrentsView) ui->torrentsView->setModel(nullptr);
        if (ui->downloadsTableView) ui->downloadsTableView->setModel(nullptr);
        if (ui->cBoxGenre1) ui->cBoxGenre1->setModel(nullptr);
        if (ui->cBoxGenre2) ui->cBoxGenre2->setModel(nullptr);
        if (ui->comboBox_random_add_genre) ui->comboBox_random_add_genre->setModel(nullptr);
    }
    
    if (m_torNetworkService) {
        m_torNetworkService->stopTorProcess();
    }
    if (m_torrentSearchService) {
        m_torrentSearchService->cancelSearch();
    }
    if (m_ngrokService) {
        m_ngrokService->stop();
    }

    if (stimer) stimer->stop();
    if (icetimer) icetimer->stop();
    if (butt_timer) butt_timer->stop();
    if (recTimer) recTimer->stop();
    if (adRefreshTimer) adRefreshTimer->stop();
    
    if (Xplayer) Xplayer->stop();
    if (lp1_Xplayer) lp1_Xplayer->stop();
    if (lp2_Xplayer) lp2_Xplayer->stop();
    if (RadioPlayer) RadioPlayer->stop();
    
    if (radio1.state() == QProcess::Running) {
        radio1.terminate();
        radio1.waitForFinished(2000);
    }
    
    if (audioRecorder && audioRecorder->recorderState() == QMediaRecorder::RecordingState) {
        audioRecorder->stop();
    }
    
    if (ServiceContainer::instance()) {
        ServiceContainer::instance()->shutdownServices();
    }
    
    if (adb.isOpen()) {
        adb.close();
    }
    
    event->accept();

    // After the graceful cleanup above (Tor/processes stopped, models detached,
    // services shut down, database closed), bypass Qt's automatic widget/member
    // destruction. That teardown crashes (SIGSEGV) on exit due to ordering
    // issues between the media players, services and the singleton
    // ServiceContainer — the destructor would also shut services down a second
    // time. macOS already force-exits above for the same reason; we do the same
    // here now that all meaningful resources have been released explicitly.
    std::_Exit(0);
}

player::~player()
{
    // Stop Tor process first — must happen before Qt's event loop winds down
    if (m_torNetworkService) {
        m_torNetworkService->stopTorProcess();
    }
    
    // Shutdown services to prevent access to destroyed objects
    if (ServiceContainer::instance()) {
        ServiceContainer::instance()->shutdownServices();
    }
    
    // Stop all timers to prevent callbacks during destruction
    if (stimer) stimer->stop();
    if (icetimer) icetimer->stop();
    if (butt_timer) butt_timer->stop();
    if (recTimer) recTimer->stop();
    if (adRefreshTimer) adRefreshTimer->stop();
    
    // Stop any ongoing operations
    if (Xplayer && Xplayer->playbackState() == QMediaPlayer::PlayingState) {
        Xplayer->stop();
    }
    if (lp1_Xplayer && lp1_Xplayer->playbackState() == QMediaPlayer::PlayingState) {
        lp1_Xplayer->stop();
    }
    if (lp2_Xplayer && lp2_Xplayer->playbackState() == QMediaPlayer::PlayingState) {
        lp2_Xplayer->stop();
    }
    
    // Stop value-member media player and process (these are NOT pointers, so they
    // will be destroyed when `player` is destroyed — make sure they're stopped first)
    if (RadioPlayer) RadioPlayer->stop();
    
    if (radio1.state() == QProcess::Running) {
        radio1.terminate();
        if (!radio1.waitForFinished(2000)) {
            radio1.kill();
            radio1.waitForFinished(1000);
        }
    }
    
    // Clean up audio recording safely
    if (audioRecorder) {
        if (audioRecorder->recorderState() == QMediaRecorder::RecordingState) {
            audioRecorder->stop();
        }
        audioRecorder->deleteLater();
        audioRecorder = nullptr;
    }
    
    // Clean up capture session safely
    if (captureSession) {
        captureSession->deleteLater();
        captureSession = nullptr;
    }
    
    // Clean up audio input safely
    if (audioInput) {
        audioInput->deleteLater();
        audioInput = nullptr;
    }
    
    // Don't manually delete audio outputs - they are managed by Qt's parent-child system
    // The QMediaPlayer objects and their audio outputs will be cleaned up automatically
    // when this QWidget (player) is destroyed, since they were created with 'this' as parent
    
    // Clean up ad banner safely
    if (adBanner) {
        adBanner->deleteLater();
        adBanner = nullptr;
    }
    
    // UI is automatically cleaned up by Qt's parent-child system
    // Don't manually delete ui as it's managed by QWidget's destructor
}

// Generic helper to launch an external GUI application safely
void player::launchExternalApplication(const QString& appName, const QString& filePath) {
    QString appPath = QStandardPaths::findExecutable(appName);
    
#ifdef Q_OS_MAC
    // On macOS, also check /Applications for .app bundles
    if (appPath.isEmpty()) {
        QString macAppPath = "/Applications/" + appName.left(1).toUpper() + appName.mid(1) + ".app";
        if (QFile::exists(macAppPath)) {
            // Use 'open -a' to launch macOS apps
            QStringList args;
            args << "-a" << macAppPath << filePath;
            if (QProcess::startDetached("open", args)) {
                qInfo() << "Launched" << macAppPath << "with file:" << filePath;
                return;
            }
        }
    }
#endif
    
    if (appPath.isEmpty()) {
        qWarning() << "Cannot find executable for" << appName << "in system PATH.";
        // Try opening with default handler as fallback
        bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        if (!opened) {
             QMessageBox::warning(this, tr("Application Not Found"), tr("Could not find '%1' in your system's PATH, and could not open the file '%2' with the default application.").arg(appName, QFileInfo(filePath).fileName()));
        }
        return;
    }

    qInfo() << "Launching" << appName << " (" << appPath << ") with file:" << filePath;
    QStringList args;
    args << filePath;

    if (!QProcess::startDetached(appPath, args)) {
        qWarning() << "Failed to start detached process:" << appPath << args;
        QMessageBox::critical(this, tr("Launch Error"), tr("Failed to launch '%1' with the specified file.").arg(appName));
    }
}

// Generic helper to get MediaInfo (ASYNC) - Requires modification
// TODO: Implement proper parsing of MediaInfo output (JSON is best)
void player::getMediaInfoForFile(const QString& filePath) {
    QString mediaInfoPath = QStandardPaths::findExecutable("mediainfo");
    if (mediaInfoPath.isEmpty()) {
         QMessageBox::warning(this, tr("Dependency Missing"), tr("'mediainfo' command not found. Please install it to retrieve metadata."));
         return;
    }

    QProcess *mediaInfoProcess = new QProcess(this);
    QStringList args;
    // Use JSON output for robust parsing regardless of which tags are present.
    args << "--Output=JSON" << filePath;

    qInfo() << "Getting MediaInfo:" << mediaInfoPath << args;

    connect(mediaInfoProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, mediaInfoProcess, filePath](int exitCode, QProcess::ExitStatus exitStatus){

        QString output = QString::fromLocal8Bit(mediaInfoProcess->readAllStandardOutput()).trimmed();
        QString errorOutput = QString::fromLocal8Bit(mediaInfoProcess->readAllStandardError()).trimmed();

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qWarning() << "mediainfo failed for" << filePath << "Exit:" << exitCode;
            if (!errorOutput.isEmpty()) qWarning() << "mediainfo STDERR:" << errorOutput;
             QMessageBox::warning(this, tr("Metadata Error"), tr("Failed to get metadata using 'mediainfo'.\n%1").arg(errorOutput));
        } else {
            qDebug() << "mediainfo output:\n" << output;

            // Parse mediainfo's JSON. Structure:
            //   { "media": { "track": [ { "@type": "General", ... }, ... ] } }
            QJsonObject general;
            QJsonParseError perr;
            const QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &perr);
            if (perr.error == QJsonParseError::NoError && doc.isObject()) {
                const QJsonArray tracks = doc.object().value("media").toObject()
                                              .value("track").toArray();
                for (const QJsonValue &tv : tracks) {
                    const QJsonObject t = tv.toObject();
                    if (t.value("@type").toString() == QLatin1String("General")) {
                        general = t;
                        break;
                    }
                }
            }

            if (general.isEmpty()) {
                qWarning() << "Could not parse mediainfo output:" << output;
                QMessageBox::warning(this, tr("Metadata Error"), tr("Could not parse the metadata received from 'mediainfo'."));
            } else {
                // Helper: return the first non-empty value among candidate keys.
                auto field = [&general](std::initializer_list<const char*> keys) -> QString {
                    for (const char *k : keys) {
                        const QString v = general.value(QLatin1String(k)).toString().trimmed();
                        if (!v.isEmpty()) return v;
                    }
                    return QString();
                };

                const QString artist   = field({"Performer", "Artist", "Album_Performer"});
                const QString song     = field({"Title", "Track"});
                const QString album    = field({"Album"});
                const QString genre    = field({"Genre"});
                const QString duration = field({"Duration_String3", "Duration_String", "Duration"});
                const QString size     = field({"FileSize_String", "FileSize"});
                const QString format   = field({"Format"});
                const QString bitrate  = field({"OverallBitRate_String", "OverallBitRate"});

                auto orDash = [](const QString &s) { return s.isEmpty() ? QStringLiteral("-") : s; };

                const QString msg4box = tr("Artist: %1\nSong: %2\nAlbum: %3\nGenre: %4\nDuration: %5\nSize: %6\nFormat: %7\nBitrate: %8")
                    .arg(orDash(artist), orDash(song), orDash(album), orDash(genre),
                         orDash(duration), orDash(size), orDash(format), orDash(bitrate));

                const QMessageBox::StandardButton rpl = QMessageBox::question(this,
                    tr("Apply this info to the database?"), msg4box,
                    QMessageBox::Yes | QMessageBox::No);

                if (rpl == QMessageBox::Yes) {
                    // Only update fields we actually found, so we don't overwrite
                    // existing good data with blanks.
                    QStringList sets;
                    if (!artist.isEmpty()) sets << "artist = :artist";
                    if (!song.isEmpty())   sets << "song = :song";
                    if (!genre.isEmpty())  sets << "genre1 = :genre, genre2 = :genre";

                    if (sets.isEmpty()) {
                        QMessageBox::information(this, tr("Nothing to Update"),
                            tr("The file did not contain artist, title or genre tags to apply."));
                    } else {
                        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
                        QSqlQuery query(db);
                        query.prepare("UPDATE musics SET " + sets.join(", ") + " WHERE path = :path");
                        if (!artist.isEmpty()) query.bindValue(":artist", artist);
                        if (!song.isEmpty())   query.bindValue(":song", song);
                        if (!genre.isEmpty())  query.bindValue(":genre", genre);
                        query.bindValue(":path", filePath);

                        if (!query.exec()) {
                            qWarning() << "Failed to update metadata in DB:" << query.lastError().text();
                            QMessageBox::warning(this, tr("Database Error"), tr("Failed to update metadata in the database."));
                        } else {
                            qInfo() << "Metadata updated in DB for:" << filePath;
                            update_music_table();
                        }
                    }
                }
            }
        }
        mediaInfoProcess->deleteLater();
    });

     connect(mediaInfoProcess, &QProcess::errorOccurred, this, [this, mediaInfoProcess](QProcess::ProcessError error){
         qWarning() << "Failed to start mediainfo. Error:" << error << "-" << mediaInfoProcess->errorString();
         QMessageBox::critical(this, tr("Process Error"), tr("Could not start the 'mediainfo' process."));
         mediaInfoProcess->deleteLater();
     });

    mediaInfoProcess->start(mediaInfoPath, args);
}


void player::updateConfig() {
    qDebug() << "Updating player configuration using QSettings...";

    // --- Use QSettings with the WRITABLE configuration file path ---
    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (writableConfigPath.isEmpty()) {
        qCritical() << "Could not determine writable config location in updateConfig!";
        return; // Cannot proceed without path
    }
    QString configFilePath = writableConfigPath + "/" + configFileName;

    QSettings settings(configFilePath, QSettings::IniFormat);
    qDebug() << "Reading configuration from:" << settings.fileName();

    // --- Read values using settings.value() and assign to member variables ---

    // Keep the config private to the owning user: it can contain credentials
    SecretStore::restrictFile(configFilePath);

    SavePath = settings.value("SavePath").toString();
    Server_URL = settings.value("Server_URL").toString();
    Port = settings.value("Port", 0).toInt(); // Provide default, convert to int
    User = settings.value("User").toString();
    Pass = SecretStore::open(settings.value("Pass").toString()); // stored obfuscated (see secretstore.h)
    ProgramsPath = settings.value("ProgramsPath").toString();
    MusicPath = settings.value("MusicPath").toString();
    JinglePath = settings.value("JinglePath").toString();
    FTPPath = settings.value("FTPPath").toString();
    TakeOverPath = settings.value("TakeOverPath").toString();
    ComHour = settings.value("ComHour", "00:00:00").toString(); // Provide default

    // Read boolean values directly
    fullScreen = settings.value("FullScreen", false).toBool();
    disableSeekBar = settings.value("Disable_Seek_Bar", false).toBool();
    normalization_soft = settings.value("Normalize_Soft", false).toBool();
    Disable_Volume = settings.value("Disable_Volume", false).toBool();
    darkMode = settings.value("DarkMode", false).toBool();
    bool enableTorrents = settings.value("EnableTorrents", false).toBool();

    // Read recording info (description and potentially enum data)
    recDevice = settings.value("RecDevice").toString(); // Store description
    // Read enum values if you saved them that way from optionsDialog
    recCodec = settings.value("RecCodec", QVariant::fromValue(QMediaFormat::AudioCodec::Unspecified)).value<QMediaFormat::AudioCodec>();
    recContainer = settings.value("RecContainer", QVariant::fromValue(QMediaFormat::FileFormat())).value<QMediaFormat::FileFormat>();

    // Database path
    txt_selected_db = settings.value("Database").toString();

    // Streaming client: restore the last used stream URL
    if (ui && ui->txt_rol_stream_url) {
        const QString streamUrl = settings.value("StreamClientURL").toString();
        if (!streamUrl.isEmpty())
            ui->txt_rol_stream_url->setText(streamUrl);
    }

    // Role
    Role = settings.value("Role", "Client").toString(); // Default "Client"

    // --- Apply settings to UI or internal state AFTER reading ALL settings ---
    qDebug() << "Applying loaded configuration settings...";

    // Example: Update UI elements based on loaded settings
    if (disableSeekBar) {
        ui->sliderProgress->setEnabled(false);
        qDebug() << "Disable Seek bar setting: true";
    } else {
        ui->sliderProgress->setEnabled(true);
        qDebug() << "Disable Seek bar setting: false";
    }

    if (Disable_Volume) {
        ui->sliderVolume->setEnabled(false);
        qDebug() << "Disable Volume setting: true";
    } else {
        ui->sliderVolume->setEnabled(true);
        qDebug() << "Disable Volume setting: false";
    }

    // Log other settings
    qDebug() << "Normalization Soft setting:" << normalization_soft;
    qDebug() << "Role setting:" << Role;
    if (Role == "Server") {
        qDebug("XFB Role: Server mode actions can be taken now.");
        // Add any specific logic needed when running as server
    } else {
        qDebug("XFB Role: Client mode actions can be taken now.");
        // Add any specific logic needed when running as client
    }
    qDebug() << "DarkMode setting:" << darkMode;
    // Note: Applying dark mode often requires more than just setting the variable.
    // It usually involves reapplying palettes/stylesheets, potentially restarting parts of the UI.
    // Consider how dark mode changes are triggered and applied application-wide.

    // Log paths etc.
    qDebug() << "SavePath:" << SavePath;
    qDebug() << "ProgramsPath:" << ProgramsPath;
    // ... log other variables as needed ...

    // Show or hide the Torrents tab based on the EnableTorrents setting
    if (ui && ui->pubWidget) {
        int torrentsTabIndex = ui->pubWidget->indexOf(ui->tabTorrents);
        if (enableTorrents) {
            // Re-add the tab if it was previously removed
            if (torrentsTabIndex == -1) {
                ui->pubWidget->addTab(ui->tabTorrents,
                    QIcon(":/icons/flat/pirate-32.png"), tr("Torrents"));
            }
        } else {
            // Remove the tab (widget is not deleted, just hidden from the tab bar)
            if (torrentsTabIndex != -1) {
                ui->pubWidget->removeTab(torrentsTabIndex);
            }
        }
        qDebug() << "EnableTorrents setting:" << enableTorrents;
    }

    // Show or hide the Audio FX tab (next to the DJ tab)
    bool showFxTab = settings.value("ShowFxTab", true).toBool();
    if (ui && ui->tabWidget_2 && m_fxTabPage) {
        int fxTabIndex = ui->tabWidget_2->indexOf(m_fxTabPage);
        if (showFxTab && fxTabIndex == -1) {
            const int djIndex = ui->tabWidget_2->indexOf(ui->tab_dj);
            ui->tabWidget_2->insertTab(djIndex + 1, m_fxTabPage,
                                       QIcon(":/icons/flat/eq-fx-64.png"), tr("Audio FX"));
        } else if (!showFxTab && fxTabIndex != -1) {
            ui->tabWidget_2->removeTab(fxTabIndex);
        }
        qDebug() << "ShowFxTab setting:" << showFxTab;
    }

    // Restore the playlist sound-wave view (crossfade preparation) state
    bool waveView = settings.value("PlaylistWaveView", false).toBool();
    if (m_waveViewToggle && m_waveViewToggle->isChecked() != waveView)
        m_waveViewToggle->setChecked(waveView); // toggled() applies it

    // Re-apply the FX chain settings (covers the 432 Hz switch in the
    // Options dialog; no-op during construction, before the players exist)
    applyStoredFxSettings();

    qDebug() << "Finished updating player configuration.";
}


void player::showTime()
{
    QTime time = QTime::currentTime();
    QString text = time.toString("hh:mm");
    QString segundos = time.toString(":ss");
    if ((time.second() % 2) == 0)
        segundos[0] = ' ';
    ui->txt_horas->display(text+segundos);
}

bool player::checkDbOpen() {

    QString resourceDbPath = ":/adb.db";
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
         qWarning() << "Could not determine writable application data location.";
         return false;
    }

    QString appDirName = QCoreApplication::applicationName(); // Or your specific app identifier "XFB"
    if (appDirName.isEmpty()) {
        appDirName = "XFB"; // Fallback if not set via QCoreApplication
        qWarning() << "QCoreApplication::applicationName() is empty, using fallback:" << appDirName;
    }
    QString specificAppDataPath = appDataPath + "/" + appDirName;
    QString persistentDbPath = specificAppDataPath + "/adb.db"; // Target path using app-specific subfolder
    const QString connectionName = "xfb_connection"; // Use a distinct connection name

    // --- Ensure Application Data Sub-Directory Exists ---
    QDir dir(specificAppDataPath); // Use the specific path
    if (!dir.exists()) {
        qInfo() << "Creating application data directory:" << specificAppDataPath;
        if (!dir.mkpath(".")) { // mkpath creates parent directories if needed
            qWarning() << "Failed to create application data directory:" << specificAppDataPath;
            return false;
        }
    }

    // --- Check and Copy Database File ---
    QFile persistentFile(persistentDbPath);
    QFileInfo resourceInfo(resourceDbPath);

    if (!resourceInfo.exists()) {
        qCritical() << "CRITICAL: Database resource file not found:" << resourceDbPath;
        qCritical() << "Make sure adb.db is added to your .qrc file and the .qrc is in RESOURCES in your .pro file.";
        return false;
    }

    bool copyRequired = !persistentFile.exists();
    if (persistentFile.exists() && resourceInfo.lastModified() > QFileInfo(persistentFile).lastModified()) {
        qInfo() << "Resource database is newer. Removing old persistent copy.";
        if (!persistentFile.remove()) {
             qWarning() << "Could not remove existing persistent database file:" << persistentFile.errorString() << "- Check if it's locked by another process.";
             // Decide if you want to proceed or fail here. Maybe try opening anyway?
        }
        copyRequired = true;
    }

    if (copyRequired) {
        qInfo() << "Attempting to copy database from resource to:" << persistentDbPath;
        if (!QFile::copy(resourceDbPath, persistentDbPath)) {
            QFile resFile(resourceDbPath); // For error checking
            qCritical() << "Failed to copy database from resource (" << resourceDbPath << ", exists:" << resFile.exists() << ") to"
                       << persistentDbPath << "- Error:" << persistentFile.errorString() << "(Check write permissions for " << specificAppDataPath << ")";
            return false;
        } else {
            // Explicitly set permissions after successful copy
            if (!persistentFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther)) {
                 qWarning() << "Could not set permissions on the copied database file:" << persistentDbPath << "Error:" << persistentFile.errorString();
            } else {
                 qInfo() << "Database copied successfully and permissions set for" << persistentDbPath;
            }
        }
    }

    // --- Verify Readability Before Connecting ---
    // Re-instantiate QFile object to ensure fresh state check
    QFile checkFile(persistentDbPath);
    if (!checkFile.exists()) {
        qCritical() << "Database file disappeared after check/copy? Path:" << persistentDbPath;
        return false;
    }
    if (!checkFile.permissions().testFlag(QFileDevice::ReadOwner)) { // Check owner read permission specifically
         qWarning() << "Persistent database file lacks read permission for owner:" << persistentDbPath;
         qWarning() << "Attempting to grant read permission...";
         if (!checkFile.setPermissions(checkFile.permissions() | QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
              qCritical() << "Failed to grant read/write permission to database file:" << persistentDbPath << "Error:" << checkFile.errorString();
              qCritical() << "Manual permission change might be required. Check user/group ownership and permissions.";
              return false; // Fail if we can't ensure readability
         } else {
              qInfo() << "Read/Write permission granted successfully.";
         }
    }


    // --- Setup the QSqlDatabase connection ---
    if (QSqlDatabase::contains(connectionName)) {
         // Even if reusing, check if it's actually usable / still open correctly
         if (adb.isOpen() && !adb.databaseName().isEmpty() ) {
             // Optional: Ping the database to be sure it's responsive
             QSqlQuery pingQuery(adb);
             if (!pingQuery.exec("SELECT 1")) { // Simple query
                 qWarning() << "Ping query failed on existing open connection. Error:" << pingQuery.lastError().text();
                 qWarning() << "Connection state might be stale. Closing it.";
                 adb.close(); // Close the potentially stale connection
             } else {
                 qDebug() << "Existing connection ping successful.";
             }
         } else {
              qDebug() << "Existing connection handle was found but not open or configured. Will proceed to open.";
         }

    } else {
        adb = QSqlDatabase::addDatabase("QSQLITE", connectionName);
         qDebug() << "Adding new database connection:" << connectionName;
        if (!adb.isValid()) {
            qCritical() << "Failed to add database connection. QSQLITE driver possibly missing or invalid.";
            qCritical() << "Available drivers:" << QSqlDatabase::drivers();
            return false;
        }
    }

    // Set the database file path *after* getting a valid handle
    // This is important even for existing connections if they were closed or became invalid
    if (adb.databaseName() != persistentDbPath) {
         qDebug() << "Setting database name for connection" << connectionName << "to" << persistentDbPath;
         adb.setDatabaseName(persistentDbPath);
    }


    // Only try to open if it's not already open (or if we closed it above)
    if (!adb.isOpen()) {
        qInfo() << "Attempting to open database connection:" << connectionName << " Path:" << adb.databaseName();
        if (!adb.open()) {
            qCritical() << "Database Error: Failed to open database:" << adb.lastError().text();
            qCritical() << "Database path was:" << adb.databaseName();
            qCritical() << "Underlying driver error:" << adb.lastError().driverText();
             qCritical() << "Available drivers:" << QSqlDatabase::drivers();
             if (adb.lastError().text().contains("Driver not loaded")) {
                 qCritical() << ">>> Potential deployment issue: Ensure the QSQLITE plugin is deployed correctly.";
             }
             // Specifically check for file permission related errors if possible
             if (adb.lastError().text().contains("unable to open", Qt::CaseInsensitive)) {
                 qCritical() << ">>> Error indicates file access problem. Verify permissions and path again.";
                 qCritical() << ">>> Path:" << persistentDbPath << " Exists:" << QFile::exists(persistentDbPath) << " Readable:" << QFile(persistentDbPath).isReadable();
             }
            return false;
        } else {
            qInfo() << "Database connection opened successfully:" << adb.connectionName() << " Path:" << adb.databaseName();
        }
    } else {
         qDebug() << "Database connection" << connectionName << "was already open and seems valid.";
    }

    // Final check
    if (!adb.isOpen()) {
        qCritical() << "Database connection is unexpectedly not open after initialization sequence.";
        return false;
    }

    qInfo() << "Database initialization successful. Connection '" << connectionName << "' is open.";
    
    // Create torrents table if it doesn't exist
    QSqlQuery createTorrentsTable(adb);
    QString createTorrentsTableSql = R"(
        CREATE TABLE IF NOT EXISTS torrents (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            path TEXT NOT NULL,
            size INTEGER,
            date_added DATETIME DEFAULT CURRENT_TIMESTAMP,
            status TEXT DEFAULT 'active'
        )
    )";
    
    if (!createTorrentsTable.exec(createTorrentsTableSql)) {
        qWarning() << "Failed to create torrents table:" << createTorrentsTable.lastError().text();
    } else {
        qDebug() << "Torrents table created or already exists";
    }
    
    return true;
}

void player::on_actionOpen_triggered()
{

    qDebug() << "File -> Open file";

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(tr("Audio Files (*.ogg *.mp3 *.flac *.wav)"));
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if(dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        ui->playlist->addItems(fileNames);

        // If we're currently playing, we don't need to do anything else
        // The files will be played when the current track finishes
        // If we're not playing, we could start playing the first file
        if (PlayMode == "stopped" && !fileNames.isEmpty()) {
            // Add the first file to the playlist and play it
            XplaylistUrls.clear();
            XplaylistUrls.append(QUrl::fromLocalFile(fileNames.first()));
            XplaylistIndex = 0;

            // This will trigger playback if the user clicks Play
            // We don't auto-start playback here to maintain the original behavior
        }
    }
}

void::player::playlistContextMenu(const QPoint& pos){
    QPoint globalPos = ui->playlist->mapToGlobal(pos);
    QMenu thisMenu;
    QString remove = tr("Remove this track from the playlist");
    QString moveToTop = tr("Send this track to the top of the playlist");
    QString moveToBottom = tr("Send this track to the bottom of the playlist");
    QString addVolumeLine = tr("Add a volume line");
    QString resetVolumeLine = tr("Reset the volume line");
    QString removeVolumeLine = tr("Remove the volume line");


    thisMenu.addAction(remove);
    thisMenu.addAction(moveToTop);
    thisMenu.addAction(moveToBottom);

    // Volume line (Sonar-style envelope drawn over the track's waveform in
    // the wave view): offer Add or Remove depending on the clicked track
    int menuRow = ui->playlist->selectionModel()->currentIndex().row();
    QListWidgetItem *menuItem = ui->playlist->item(menuRow);
    const bool hasVolumeLine = menuItem
        && !menuItem->data(PlaylistWaveView::VolumeEnvelopeRole).toString().isEmpty();
    thisMenu.addSeparator();
    thisMenu.setToolTipsVisible(true);
    QAction *volAction = thisMenu.addAction(hasVolumeLine ? removeVolumeLine
                                                          : addVolumeLine);
    volAction->setToolTip(tr("A volume line controls the track's volume over time: "
                             "double-click the line to add points, drag points (or "
                             "the line between them) to shape the volume, "
                             "double-click a point to reset it to 0 dB, "
                             "right-click a point to remove it."));
    if (hasVolumeLine)
        thisMenu.addAction(resetVolumeLine); // back to a flat 0 dB line


    QAction* selectedItem = thisMenu.exec(globalPos);
    if(selectedItem){
        QString selectedListItem = selectedItem->text();
        int rowidx = ui->playlist->selectionModel()->currentIndex().row();
        estevalor = ui->playlist->model()->data(ui->playlist->model()->index(rowidx,0)).toString();

        if(selectedListItem==remove){
            delete ui->playlist->item(rowidx);
            calculate_playlist_total_time();
        }
        // takeItem/insertItem move the same item object, so the crossfade
        // overlap stored on it (wave view) survives the move
        if(selectedListItem==moveToTop){
            QListWidgetItem *moved = ui->playlist->takeItem(rowidx);
            if (moved)
                ui->playlist->insertItem(0, moved);
        }
        if(selectedListItem==moveToBottom){
            QListWidgetItem *moved = ui->playlist->takeItem(rowidx);
            if (moved)
                ui->playlist->addItem(moved);
        }
        if(selectedListItem==addVolumeLine){
            if (QListWidgetItem *it = ui->playlist->item(rowidx)) {
                // A flat 100% line (like Sonar's freshly added envelope);
                // the user double-clicks it in the wave view to add points
                it->setData(PlaylistWaveView::VolumeEnvelopeRole,
                            PlaylistWaveView::encodeEnvelope({QPointF(0.0, 1.0)}));
                // The line is edited in the wave view, so switch it on
                if (m_waveViewToggle && !m_waveViewToggle->isChecked())
                    m_waveViewToggle->setChecked(true);
            }
        }
        if(selectedListItem==resetVolumeLine){
            if (QListWidgetItem *it = ui->playlist->item(rowidx))
                it->setData(PlaylistWaveView::VolumeEnvelopeRole,
                            PlaylistWaveView::encodeEnvelope({QPointF(0.0, 1.0)}));
        }
        if(selectedListItem==removeVolumeLine){
            if (QListWidgetItem *it = ui->playlist->item(rowidx))
                it->setData(PlaylistWaveView::VolumeEnvelopeRole, QVariant());
        }


    }
}

void player::musicViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->musicView->mapToGlobal(pos);

    QModelIndexList selectedIndexes = ui->musicView->selectionModel()->selectedRows(1); // column 1 (artist) — column 0 is hidden
    if (selectedIndexes.isEmpty()) return;

    bool multiSelect = selectedIndexes.size() > 1;
    int count = selectedIndexes.size();

    QMenu thisMenu;

    // Playlist actions
    QAction *actAddBottom = thisMenu.addAction(tr("Add to the bottom of playlist"));
    QAction *actAddTop = thisMenu.addAction(tr("Add to the top of the playlist"));
    thisMenu.addSeparator();

    // Batch edit actions (show submenu when multi-selected)
    QAction *actSetGenre1 = nullptr;
    QAction *actSetGenre2 = nullptr;
    QAction *actSetArtist = nullptr;
    QAction *actSetCountry = nullptr;

    if (multiSelect) {
        QMenu *batchMenu = thisMenu.addMenu(tr("Batch Edit (%1 tracks)").arg(count));
        actSetGenre1 = batchMenu->addAction(tr("Set Genre 1..."));
        actSetGenre2 = batchMenu->addAction(tr("Set Genre 2..."));
        actSetArtist = batchMenu->addAction(tr("Set Artist..."));
        actSetCountry = batchMenu->addAction(tr("Set Country..."));
        thisMenu.addSeparator();
    }

    QAction *actRetune432 = thisMenu.addAction(
        QIcon(":/icons/flat/tuning-fork-64.png"),
        multiSelect ? tr("Retune %1 tracks to 432 Hz...").arg(count)
                    : tr("Retune this track to 432 Hz..."));
    thisMenu.addSeparator();

    QAction *actDelete = thisMenu.addAction(
        multiSelect ? tr("Delete %1 tracks from database").arg(count)
                    : tr("Delete this track from database"));
    thisMenu.addSeparator();
    QAction *actAudacity = thisMenu.addAction(tr("Open this in Audacity"));
    QAction *actInfo = thisMenu.addAction(tr("Retrieve metadata from file (mediainfo)"));

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");

    // Helper lambdas
    auto getSelectedPaths = [&]() -> QStringList {
        QStringList paths;
        for (const QModelIndex &idx : selectedIndexes) {
            QModelIndex pathIdx = ui->musicView->model()->index(idx.row(), 7);
            if (pathIdx.isValid()) {
                QString p = ui->musicView->model()->data(pathIdx).toString();
                if (!p.isEmpty()) paths << p;
            }
        }
        return paths;
    };
    auto getSelectedIds = [&]() -> QList<int> {
        QList<int> ids;
        for (const QModelIndex &idx : selectedIndexes) {
            QModelIndex idIdx = ui->musicView->model()->index(idx.row(), 0);
            if (idIdx.isValid()) ids << ui->musicView->model()->data(idIdx).toInt();
        }
        return ids;
    };

    if (selectedItem == actAddBottom) {
        for (const QString &path : getSelectedPaths())
            ui->playlist->addItem(path);
        calculate_playlist_total_time();

    } else if (selectedItem == actRetune432) {
        convertMusicsTo432(getSelectedPaths());

    } else if (selectedItem == actAddTop) {
        QStringList paths = getSelectedPaths();
        for (int i = paths.size() - 1; i >= 0; --i)
            ui->playlist->insertItem(0, paths[i]);
        calculate_playlist_total_time();

    } else if (selectedItem == actSetGenre1) {
        bool ok;
        QString genre = QInputDialog::getText(this, tr("Set Genre 1"),
            tr("New Genre 1 for %1 tracks:").arg(count), QLineEdit::Normal, "", &ok);
        if (ok && !genre.isEmpty()) {
            QSqlQuery q(db);
            QList<int> ids = getSelectedIds();
            qDebug() << "Batch Set Genre 1: updating" << ids.size() << "tracks, IDs:" << ids;
            for (int id : ids) {
                q.prepare("UPDATE musics SET genre1 = :g WHERE id = :id");
                q.bindValue(":g", genre);
                q.bindValue(":id", id);
                if (!q.exec()) {
                    qWarning() << "Failed to update genre1 for id" << id << ":" << q.lastError().text();
                }
            }
            update_music_table();
            ui->statusBar->showMessage(tr("Genre 1 set to '%1' for %2 tracks").arg(genre).arg(ids.size()), 5000);
        }

    } else if (selectedItem == actSetGenre2) {
        bool ok;
        QString genre = QInputDialog::getText(this, tr("Set Genre 2"),
            tr("New Genre 2 for %1 tracks:").arg(count), QLineEdit::Normal, "", &ok);
        if (ok && !genre.isEmpty()) {
            QSqlQuery q(db);
            for (int id : getSelectedIds()) {
                q.prepare("UPDATE musics SET genre2 = :g WHERE id = :id");
                q.bindValue(":g", genre);
                q.bindValue(":id", id);
                q.exec();
            }
            update_music_table();
            ui->statusBar->showMessage(tr("Genre 2 set to '%1' for %2 tracks").arg(genre).arg(count), 5000);
        }

    } else if (selectedItem == actSetArtist) {
        bool ok;
        QString artist = QInputDialog::getText(this, tr("Set Artist"),
            tr("New Artist for %1 tracks:").arg(count), QLineEdit::Normal, "", &ok);
        if (ok && !artist.isEmpty()) {
            QSqlQuery q(db);
            for (int id : getSelectedIds()) {
                q.prepare("UPDATE musics SET artist = :a WHERE id = :id");
                q.bindValue(":a", artist);
                q.bindValue(":id", id);
                q.exec();
            }
            update_music_table();
            ui->statusBar->showMessage(tr("Artist set to '%1' for %2 tracks").arg(artist).arg(count), 5000);
        }

    } else if (selectedItem == actSetCountry) {
        bool ok;
        QString country = QInputDialog::getText(this, tr("Set Country"),
            tr("New Country for %1 tracks:").arg(count), QLineEdit::Normal, "", &ok);
        if (ok && !country.isEmpty()) {
            QSqlQuery q(db);
            for (int id : getSelectedIds()) {
                q.prepare("UPDATE musics SET country = :c WHERE id = :id");
                q.bindValue(":c", country);
                q.bindValue(":id", id);
                q.exec();
            }
            update_music_table();
            ui->statusBar->showMessage(tr("Country set to '%1' for %2 tracks").arg(country).arg(count), 5000);
        }

    } else if (selectedItem == actDelete) {
        QStringList paths = getSelectedPaths();
        QMessageBox::StandardButton go = QMessageBox::question(this, tr("Confirm Deletion"),
            tr("Are you sure you want to delete %1 track(s) from the database?").arg(paths.size()),
            QMessageBox::Yes | QMessageBox::No);
        if (go != QMessageBox::Yes) return;

        QMessageBox::StandardButton rm = QMessageBox::question(this, tr("Delete Files?"),
            tr("Also delete the %1 file(s) from the hard drive?").arg(paths.size()),
            QMessageBox::Yes | QMessageBox::No);
        bool deleteFiles = (rm == QMessageBox::Yes);

        int deleted = 0;
        QSqlQuery q(db);
        for (const QString &path : paths) {
            if (deleteFiles && QFile::exists(path)) {
                if (!QFile::remove(path))
                    qWarning() << "Failed to delete file:" << path;
            }
            q.prepare("DELETE FROM musics WHERE path = :path");
            q.bindValue(":path", path);
            if (q.exec()) deleted++;
        }
        update_music_table();
        ui->statusBar->showMessage(tr("Deleted %1 track(s)").arg(deleted), 5000);

    } else if (selectedItem == actAudacity) {
        QStringList paths = getSelectedPaths();
        if (!paths.isEmpty())
            launchExternalApplication("audacity", paths.first());

    } else if (selectedItem == actInfo) {
        QStringList paths = getSelectedPaths();
        if (!paths.isEmpty())
            getMediaInfoForFile(paths.first());
    }
}


void player::jinglesViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->jinglesView->mapToGlobal(pos);
    QMenu thisMenu;
    const QString actionAddToBottom = tr("Add to the bottom of playlist");
    const QString actionAddToTop = tr("Add to the top of the playlist");
    const QString actionDeleteFromDB = tr("Delete this jingle from the database");
    const QString actionOpenAudacity = tr("Open this in Audacity");

    thisMenu.addAction(actionAddToBottom);
    thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    thisMenu.addAction(actionOpenAudacity);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QModelIndexList selectedIndexes = ui->jinglesView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    int rowidx = selectedIndexes.first().row();
    QModelIndex pathIndex = ui->jinglesView->model()->index(rowidx, 1); // Assuming column 1 is path
    if (!pathIndex.isValid()) return;
    QString selectedFilePath = ui->jinglesView->model()->data(pathIndex).toString();

    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionAddToBottom) {
        ui->playlist->addItem(selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionAddToTop) {
        ui->playlist->insertItem(0, selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionDeleteFromDB) {
        QMessageBox::StandardButton go = QMessageBox::question(this, tr("Confirm Deletion"), tr("Are you sure you want to delete this jingle from the database?\n(File on disk will NOT be deleted)"), QMessageBox::Yes | QMessageBox::No);
        if (go == QMessageBox::Yes) {
            QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            QSqlQuery sql(db);
            sql.prepare("DELETE FROM jingles WHERE path = :path");
            sql.bindValue(":path", selectedFilePath);
            if (sql.exec()) {
                qInfo() << "Jingle removed from database:" << selectedFilePath;
                update_music_table(); // This reloads all tables, including jingles
                QMessageBox::information(this, tr("Jingle Removed"), tr("The jingle was removed from the database."));
            } else {
                 qCritical() << "Database Error deleting jingle:" << sql.lastError().text() << sql.lastQuery();
                 QMessageBox::critical(this, tr("Database Error"), tr("Failed to remove the jingle from the database:\n%1").arg(sql.lastError().text()));
            }
        }
    } else if (selectedActionText == actionOpenAudacity) {
        launchExternalApplication("audacity", selectedFilePath);
    }
}


void::player::pubViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->pubView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this pub from the database");
    QString openWithAudacity = tr("Open this in Audacity");

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    thisMenu.addAction(addToBottomOfPlaylist);
    thisMenu.addAction(addtoTopOfPlaylist);
    thisMenu.addAction(deleteThisFromDB);
    thisMenu.addAction(openWithAudacity);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (selectedItem)
    {
        //qDebug()<<"selected item in context menu was: "<<selectedItem->text();
        QString selectedMenuItem = selectedItem->text();
        int rowidx = ui->pubView->selectionModel()->currentIndex().row();
        estevalor = ui->pubView->model()->data(ui->pubView->model()->index(rowidx,2)).toString();

        if(selectedMenuItem==addToBottomOfPlaylist){
            qDebug()<<"Launch add this to bottom of playlist";
            ui->playlist->addItem(estevalor);
        }
        if(selectedMenuItem==addtoTopOfPlaylist){
            qDebug()<<"Launch add this to top of playlist";
         ui->playlist->insertItem(0,estevalor);
        }
        if(selectedMenuItem==deleteThisFromDB){

            QMessageBox::StandardButton go;
            go = QMessageBox::question(this,tr("Sure?"),tr("Are you sure you want to delete this pub from the database?"), QMessageBox::Yes|QMessageBox::No);
            if(go==QMessageBox::Yes){
                checkDbOpen();
                QSqlQuery sql(db);
                sql.prepare("delete from pub where path=:path");
                sql.bindValue(":path",estevalor);
                if(sql.exec()){
                    QMessageBox::information(this,tr("Jingle removed"),tr("The pub was removed from the database!"));
                    update_music_table();
                } else {
                    QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                    qDebug() << "last sql: " << sql.lastQuery();
                }
            }
           }

       if(selectedMenuItem==openWithAudacity){
           // Argument list instead of a shell string: file names must never
           // be interpreted by a shell.
           launchExternalApplication("audacity", estevalor);
       }


    }

}
// Helper function to run a server script asynchronously
void player::runServerCheckScript(const QString& scriptName, const QString& fileToCheck, const QString& successMessage, const QString& failureMessage) {
    // TODO: Replace this with a robust way to find the script
    QString scriptBaseName = scriptName; // e.g., "serverFtpCmdsCHKProgram.sh"
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path
    QString scriptPath = QDir(scriptDir).filePath(scriptBaseName);

    qInfo() << "Attempting to execute check script:" << scriptPath << "for file:" << fileToCheck;

    if (!QFileInfo::exists(scriptPath)) {
        qWarning() << "Check script not found at:" << scriptPath;
        QMessageBox::critical(this, "Script Error", QString("The required check script was not found:\n%1").arg(scriptPath));
        return;
    }

    QProgressDialog progress(tr("Checking server..."), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    qApp->processEvents();

    QProcess *checkProcess = new QProcess(this);
    QStringList args;
    // Pass filename as argument instead of relying on grep? Modify script if possible.
    // For now, mimic original grep:
    // args << "| grep" << fileToCheck; // NO! Don't pipe in C++ QProcess args

    connect(checkProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, checkProcess, fileToCheck, successMessage, failureMessage, &progress](int exitCode, QProcess::ExitStatus exitStatus) {

        progress.cancel(); // Close progress dialog
        QString stdOutput = QString::fromLocal8Bit(checkProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(checkProcess->readAllStandardError()).trimmed();
        bool found = false;

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            qDebug() << "Check script STDOUT:\n" << stdOutput;
            // Check if the output contains the filename we are looking for
            if (stdOutput.contains(fileToCheck, Qt::CaseInsensitive)) {
                found = true;
            } else {
                 if (!stdError.isEmpty()) qWarning() << "Check script STDERR (exit 0):\n" << stdError;
            }
        } else {
            qWarning() << "Check script failed or exited with error. ExitCode:" << exitCode;
            if (!stdError.isEmpty()) qWarning() << "Check script STDERR:\n" << stdError;
        }

        if (found) {
            QMessageBox::information(this, tr("Check Successful"), successMessage + "\n\nServer Output:\n" + stdOutput.left(300));
        } else {
            QMessageBox::critical(this, tr("Check Failed"), failureMessage);
        }

        checkProcess->deleteLater();
    });

    connect(checkProcess, &QProcess::errorOccurred, this, [this, checkProcess, scriptPath, &progress](QProcess::ProcessError error){
         qWarning() << "Failed to start check script (" << scriptPath << "). Error:" << error << "-" << checkProcess->errorString();
         progress.cancel();
         QMessageBox::critical(this, "Script Error", QString("Could not start the check script:\n%1\n\nError: %2").arg(scriptPath).arg(checkProcess->errorString()));
         checkProcess->deleteLater();
     });

    // Start the script - How to handle the grep part robustly?
    // Option 1 (Best): Modify script to take filename as arg and return exit code 0 if found.
    // checkProcess->start(scriptPath, QStringList() << fileToCheck); // If script modified
    // Option 2 (Current): Run script and check output in C++.
    checkProcess->start(scriptPath); // Run script, check full output in finished signal
}
// Helper function to run upload/put script asynchronously
void player::runServerUploadScript(const QString& scriptName, const QString& fileToUpload, const QString& successMessage, const QString& failureMessage, std::function<void(bool)> callback) {
    // TODO: Replace this with a robust way to find the script
    QString scriptBaseName = scriptName; // e.g., "serverFtpCmdsPutProgram.sh"
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path
    QString scriptPath = QDir(scriptDir).filePath(scriptBaseName);

    qInfo() << "Attempting to execute upload script:" << scriptPath << "for file:" << fileToUpload;
     qDebug() << "Dependencies: Script must exist, be executable, ~/.netrc configured.";

    if (!QFileInfo::exists(scriptPath)) {
        qWarning() << "Upload script not found at:" << scriptPath;
        QMessageBox::critical(this, "Script Error", QString("The required upload script was not found:\n%1").arg(scriptPath));
        callback(false); // Indicate failure
        return;
    }

    QProgressDialog progress(tr("Uploading to server..."), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    qApp->processEvents();


    QProcess *uploadProcess = new QProcess(this);
    // Does the script need the filename as argument? Assume not for now based on original.

    connect(uploadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, uploadProcess, successMessage, failureMessage, &progress, callback](int exitCode, QProcess::ExitStatus exitStatus) {

        progress.cancel();
        QString stdOutput = QString::fromLocal8Bit(uploadProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(uploadProcess->readAllStandardError()).trimmed();
        bool success = false;

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
             qDebug() << "Upload script STDOUT:\n" << stdOutput;
            if (stdOutput.contains("Transfer complete", Qt::CaseInsensitive)) {
                 qInfo() << "Upload script reported success.";
                 success = true;
             } else {
                  qWarning() << "Upload script finished (exit 0) but success message not found in output.";
                  if (!stdError.isEmpty()) qWarning() << "Upload script STDERR:\n" << stdError;
             }
        } else {
            qWarning() << "Upload script failed or exited with error. ExitCode:" << exitCode;
            if (!stdError.isEmpty()) qWarning() << "Upload script STDERR:\n" << stdError;
            else if (!stdOutput.isEmpty()) qWarning() << "Upload script STDOUT (check errors):\n" << stdOutput;
        }

        if (success) {
             QMessageBox::information(this, tr("Upload Successful"), successMessage);
             callback(true); // Indicate success
        } else {
             QMessageBox::critical(this, "Upload Failed", failureMessage + tr("\nCheck ~/.netrc, script, network, server status.\nOutput:\n%1\n%2")
                                   .arg(stdOutput.left(200)).arg(stdError.left(200)));
             callback(false); // Indicate failure
        }
        uploadProcess->deleteLater();
    });

     connect(uploadProcess, &QProcess::errorOccurred, this, [this, uploadProcess, scriptPath, failureMessage, &progress, callback](QProcess::ProcessError error){
         qWarning() << "Failed to start upload script (" << scriptPath << "). Error:" << error << "-" << uploadProcess->errorString();
         progress.cancel();
         QMessageBox::critical(this, "Script Error", QString("Could not start the upload script:\n%1\n\nError: %2").arg(scriptPath).arg(uploadProcess->errorString()) + "\n" + failureMessage);
         callback(false); // Indicate failure
         uploadProcess->deleteLater();
     });

    // Start the script
    uploadProcess->start(scriptPath);
}


void player::programsViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->programsView->mapToGlobal(pos);
    QMenu thisMenu;
    const QString actionAddToBottom = tr("Add to the bottom of playlist");
    const QString actionAddToTop = tr("Add to the top of the playlist");
    const QString actionDeleteFromDB = tr("Delete this program from the database");
    const QString actionOpenAudacity = tr("Open this in Audacity");
    const QString actionResendToServer = tr("(Re)Send this program to the server");
    const QString actionCheckSent = tr("Verify that the program is in the server");

    thisMenu.addAction(actionAddToBottom);
    thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    thisMenu.addAction(actionOpenAudacity);
    thisMenu.addSeparator();
    thisMenu.addAction(actionCheckSent);
    thisMenu.addAction(actionResendToServer);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QModelIndexList selectedIndexes = ui->programsView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    int rowidx = selectedIndexes.first().row();
    QModelIndex pathIndex = ui->programsView->model()->index(rowidx, 2); // Assuming column 2 is path
    if (!pathIndex.isValid()) return;
    QString selectedFilePath = ui->programsView->model()->data(pathIndex).toString();
    QFileInfo fileInfo(selectedFilePath);
    QString selectedFileName = fileInfo.fileName();


    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionAddToBottom) {
        ui->playlist->addItem(selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionAddToTop) {
        ui->playlist->insertItem(0, selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionDeleteFromDB) {
         QMessageBox::StandardButton go = QMessageBox::question(this, tr("Confirm Deletion"), tr("Are you sure you want to delete this program from the database?\n(File on disk will NOT be deleted)"), QMessageBox::Yes | QMessageBox::No);
         if (go == QMessageBox::Yes) {
             QSqlDatabase db = QSqlDatabase::database("xfb_connection");
             QSqlQuery sql(db);
             sql.prepare("DELETE FROM programs WHERE path = :path");
             sql.bindValue(":path", selectedFilePath);
             if (sql.exec()) {
                 qInfo() << "Program removed from database:" << selectedFilePath;
                 update_music_table();
                 QMessageBox::information(this, tr("Program Removed"), tr("The program was removed from the database."));
             } else {
                 qCritical() << "Database Error deleting program:" << sql.lastError().text() << sql.lastQuery();
                 QMessageBox::critical(this, tr("Database Error"), tr("Failed to remove the program from the database:\n%1").arg(sql.lastError().text()));
             }
         }
    } else if (selectedActionText == actionOpenAudacity) {
         launchExternalApplication("audacity", selectedFilePath);
    } else if (selectedActionText == actionCheckSent) {
        runServerCheckScript("serverFtpCmdsCHKProgram.sh", selectedFileName,
                             tr("The program '%1' is present on the server!").arg(selectedFileName),
                             tr("The program '%1' was NOT found on the server.").arg(selectedFileName));
    } else if (selectedActionText == actionResendToServer) {
         qInfo() << "(Re)Sending program to server:" << selectedFilePath;
         ui->txt_uploadingPrograms->show(); // Show indicator

         // 1. Copy file to temporary FTP location (FTPPath)
         QString ftpTempPath = QDir(FTPPath).filePath(selectedFileName);
         qInfo() << "Copying" << selectedFilePath << "to" << ftpTempPath;
         QFile::remove(ftpTempPath); // Remove existing temp file first
         if (!QFile::copy(selectedFilePath, ftpTempPath)) {
             qWarning() << "Failed to copy program to temporary FTP folder:" << ftpTempPath;
             QMessageBox::critical(this, tr("Copy Error"), tr("Failed to copy the program file to the temporary upload folder.\nCheck permissions for '%1'.").arg(FTPPath));
             ui->txt_uploadingPrograms->hide();
             return;
         }

         // 2. Run the upload script asynchronously
         runServerUploadScript("serverFtpCmdsPutProgram.sh", ftpTempPath,
                               tr("Program '%1' uploaded successfully.").arg(selectedFileName),
                               tr("Failed to upload program '%1'.").arg(selectedFileName),
                               // Callback function after upload attempt:
                               [this, ftpTempPath](bool uploadSuccess) {
                                    // 3. Clean up temporary file
                                    qInfo() << "Cleaning up temporary FTP file:" << ftpTempPath;
                                    if (QFile::remove(ftpTempPath)) {
                                        qDebug() << "Removed temporary FTP file:" << ftpTempPath;
                                    } else {
                                        qWarning() << "Failed to remove temporary FTP file:" << ftpTempPath;
                                    }
                                    ui->txt_uploadingPrograms->hide(); // Hide indicator
                                    // No further action needed here based on original code after upload attempt
                               });
     }
}

void player::torrentsViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->torrentsView->mapToGlobal(pos);
    QMenu thisMenu;
    const QString actionDownload = tr("Download Torrent");
    const QString actionDownloadAndStream = tr("Download and Stream");
    const QString actionCopyMagnet = tr("Copy Magnet Link");
    const QString actionViewDetails = tr("View Details");

    thisMenu.addAction(actionDownload);
    thisMenu.addAction(actionDownloadAndStream);
    thisMenu.addSeparator();
    thisMenu.addAction(actionCopyMagnet);
    thisMenu.addAction(actionViewDetails);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QModelIndexList selectedIndexes = ui->torrentsView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    int rowidx = selectedIndexes.first().row();
    QModelIndex nameIndex = ui->torrentsView->model()->index(rowidx, 0);
    if (!nameIndex.isValid()) return;
    
    // Get the torrent result data
    QVariant resultData = nameIndex.data(Qt::UserRole);
    if (!resultData.isValid()) return;
    
    TorrentSearchResult result = resultData.value<TorrentSearchResult>();
    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionDownload) {
        // Show legal warning before download
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            tr("Download Confirmation"), 
            tr("⚠️ LEGAL RESPONSIBILITY ⚠️\n\n"
               "You are about to download: %1\n\n"
               "By proceeding, you confirm that:\n"
               "• You have the legal right to download this content\n"
               "• The content is not copyrighted or you own the rights\n"
               "• You comply with all applicable laws\n\n"
               "XFB is not responsible for any illegal downloads.\n\n"
               "Do you want to proceed?").arg(result.name),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes && m_torrentDownloadService) {
            if (!ensureTorrentClient()) {
                return;
            }
            QString downloadId = m_torrentDownloadService->startDownload(result.magnetLink, result.name);
            if (!downloadId.isEmpty()) {
                QMessageBox::information(this, tr("Download Started"), 
                                        tr("Download started for: %1").arg(result.name));
            }
        }
    } else if (selectedActionText == actionDownloadAndStream) {
        // Show legal warning before download
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            tr("Stream Confirmation"), 
            tr("⚠️ LEGAL RESPONSIBILITY ⚠️\n\n"
               "You are about to download and stream: %1\n\n"
               "By proceeding, you confirm that:\n"
               "• You have the legal right to access this content\n"
               "• The content is not copyrighted or you own the rights\n"
               "• You comply with all applicable laws\n\n"
               "Streaming will begin once enough data is downloaded.\n\n"
               "Do you want to proceed?").arg(result.name),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes && m_torrentDownloadService) {
            if (!ensureTorrentClient()) {
                return;
            }
            QString downloadId = m_torrentDownloadService->startDownload(result.magnetLink, result.name);
            if (!downloadId.isEmpty()) {
                m_torrentDownloadService->enableStreaming(downloadId);
                QMessageBox::information(this, tr("Streaming Started"), 
                                        tr("Download and streaming started for: %1\n"
                                           "Playback will begin automatically once ready.").arg(result.name));
            }
        }
    } else if (selectedActionText == actionCopyMagnet) {
        if (!result.magnetLink.isEmpty()) {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(result.magnetLink);
            QMessageBox::information(this, tr("Copied"), tr("Magnet link copied to clipboard."));
        } else {
            QMessageBox::warning(this, tr("Error"), tr("No magnet link available for this torrent."));
        }
    } else if (selectedActionText == actionViewDetails) {
        QString details = tr("Torrent Details:\n\n"
                           "Name: %1\n"
                           "Size: %2\n"
                           "Seeders: %3\n"
                           "Leechers: %4\n"
                           "Upload Date: %5\n"
                           "Uploader: %6\n"
                           "Category: %7")
                         .arg(result.name)
                         .arg(result.size)
                         .arg(result.seeders)
                         .arg(result.leechers)
                         .arg(result.uploadDate)
                         .arg(result.uploader)
                         .arg(result.category);
        
        QMessageBox::information(this, tr("Torrent Details"), details);
    }
}


// Helper to get duration using exiftool (async)
void player::getDurationForFile(const QString& filePath, std::function<void(const QString&, const QString&)> callback) {
    QString exiftoolPath = QStandardPaths::findExecutable("exiftool");
    if (exiftoolPath.isEmpty()) {
        qWarning() << "exiftool not found for duration check.";
        callback(filePath, ""); // Return empty duration on failure
        return;
    }

    QProcess *process = new QProcess(this);
    QStringList arguments;
    arguments << "-T" << "-Duration" << filePath; // Use -T for direct value output

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [process, filePath, callback](int exitCode, QProcess::ExitStatus exitStatus){
        QString durationValue = "";
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            durationValue = QString::fromLocal8Bit(process->readAllStandardOutput()).trimmed();
             if (durationValue == "-") durationValue = ""; // Handle case where tag not found
        } else {
            qWarning() << "exiftool failed for duration check on" << filePath;
            // Log error output?
        }
        callback(filePath, durationValue); // Call the callback with result
        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this, [process, filePath, callback](QProcess::ProcessError error){
         qWarning() << "Failed to start exiftool for duration check. Error:" << error;
         callback(filePath, ""); // Return empty duration
         process->deleteLater();
    });

    process->start(exiftoolPath, arguments);
}

void player::on_btPlay_clicked(){
    qDebug()<<"Play button clicked";

    if(PlayMode=="stopped"){
        if(darkMode){
            ui->btPlay->setStyleSheet("background-color:#5e9604"); //green
        }else{

            ui->btPlay->setStyleSheet("background-color:#2CCD54"); //green
        }
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";
        playNextSong();

    }  else if(PlayMode=="Playing_StopAtNextOne"){

        if(darkMode){
            ui->btPlay->setStyleSheet("background-color:#5e9604"); //green
        }else{

            ui->btPlay->setStyleSheet("background-color:#2CCD54"); //green
        }
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";

    } else if(PlayMode=="Playing_Segue"){

        ui->btPlay->setStyleSheet("background-color:#F0DB1B"); //amarillo
        PlayMode = "Playing_StopAtNextOne";
        ui->btPlay->setText(tr("Play and Stop"));


    }


}

void player::playNextMedia() {
    // Check if we're already in a playNextMedia operation to prevent re-entry
    static thread_local bool inPlayNextMedia = false;
    if (inPlayNextMedia) {
        qDebug() << "playNextMedia already in progress, skipping to prevent deadlock";
        return;
    }
    
    // Set the flag to prevent re-entry
    inPlayNextMedia = true;
    
    // Ensure cleanup on function exit
    auto guard = [&]() { inPlayNextMedia = false; };
    QScopeGuard scopeGuard(guard);
    
    // Validate UI components
    if (!ui || !ui->playlist) {
        qCritical() << "UI or playlist widget is null in playNextMedia!";
        return;
    }
    
    // All playlist operations run on the main thread — no mutex needed.
    
    // Increment the playlist index and play the next media
    if (XplaylistUrls.isEmpty()) {
        qDebug() << "Playlist is empty, cannot play next media";
        return;
    }

    XplaylistIndex++;
    if (XplaylistIndex >= XplaylistUrls.size()) {
        // Reached the end of the playlist
        XplaylistIndex = 0;
        if (PlayMode == "Playing_Segue") {
            // In segue mode, try to get the next song from the playlist widget
            playNextSong();
            return;
        } else {
            // Otherwise, stop playback
            m_manualAdvancing = true;
            Xplayer->stop();
            m_manualAdvancing = false;
            ui->btPlay->setStyleSheet("");
            ui->btPlay->setText(tr("Play"));
            PlayMode = "stopped";
            return;
        }
    } else {
        // Play the next media in the playlist
        m_manualAdvancing = true;
        Xplayer->setSource(XplaylistUrls[XplaylistIndex]);
        Xplayer->play();
        m_manualAdvancing = false;
    }
}

void player::playPreviousMedia() {
    // Decrement the playlist index and play the previous media
    if (XplaylistUrls.isEmpty()) {
        qDebug() << "Playlist is empty, cannot play previous media";
        return;
    }

    XplaylistIndex--;
    if (XplaylistIndex < 0) {
        // Reached the beginning of the playlist, wrap around to the end
        XplaylistIndex = XplaylistUrls.size() - 1;
    }

    m_manualAdvancing = true;
    Xplayer->setSource(XplaylistUrls[XplaylistIndex]);
    Xplayer->play();
    m_manualAdvancing = false;
}

void player::playNextSong(){
    // Check if we're already in a playNextSong operation to prevent deadlock
    static thread_local bool inPlayNextSong = false;
    if (inPlayNextSong) {
        qDebug() << "playNextSong already in progress, skipping to prevent deadlock";
        return;
    }
    
    // Set the flag to prevent re-entry
    inPlayNextSong = true;
    
    // Ensure cleanup on function exit
    auto guard = [&]() { inPlayNextSong = false; };
    QScopeGuard scopeGuard(guard);
    
    // Recursion protection
    if (s_recursionDepth >= MAX_RECURSION_DEPTH) {
        qWarning() << "Maximum recursion depth reached in playNextSong, stopping to prevent stack overflow";
        return;
    }
    s_recursionDepth++;
    
    // Ensure cleanup on function exit
    auto recursionGuard = [this]() { s_recursionDepth--; };
    QScopeGuard recursionScopeGuard(recursionGuard);

    // Validate UI components (no mutex needed for read-only checks)
    if (!ui || !ui->playlist) {
        qCritical() << "UI or playlist widget is null in playNextSong!";
        return;
    }

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open in playNextSong!";
        checkDbOpen(); // Try to reopen the database
        db = QSqlDatabase::database("xfb_connection");
        if (!db.isOpen()) {
            qCritical() << "Failed to open database connection in playNextSong!";
            return;
        }
    }
    
    if(PlayMode=="Playing_Segue"){
        qDebug()<<"The white rabit is Playing_segue";

        // All playlist operations run on the main thread — no mutex needed.
        // The re-entry guard above prevents recursive calls.
        
            // Check if playlist has items and validate first item
            if(ui->playlist->count() <= 0){
                qDebug()<<"Playlist is empty, cannot play next song";
                if(autoMode==1){
                    qDebug()<<"Trying to get more songs since we are in autoMode...";
                    int currentPlaylistCount = ui->playlist->count();
                    playlistAboutToFinish();
                    if(ui->playlist->count() <= currentPlaylistCount) {
                        qDebug()<<"No new items added to playlist, stopping playback";
                        Xplayer->stop();
                        ui->btPlay->setStyleSheet("");
                        ui->btPlay->setText(tr("Play"));
                        PlayMode = "stopped";
                        return;
                    }
                    // Songs were added — fall through to play the first one
                } else {
                    // Auto Mode is off and the playlist is exhausted: stop
                    // cleanly and return the Play button to its stopped state.
                    qDebug()<<"Playlist empty and autoMode off — stopping playback";
                    Xplayer->stop();
                    ui->btPlay->setStyleSheet("");
                    ui->btPlay->setText(tr("Play"));
                    PlayMode = "stopped";
                    return;
                }
            }

            // Safely get the first playlist item
            QListWidgetItem* firstItem = ui->playlist->item(0);
            if (!firstItem) {
                qCritical() << "First playlist item is null despite count > 0!";
                return;
            }

            QString itemDaPlaylist = firstItem->text();
            qDebug()<<"itemDaPlaylist has value "<<itemDaPlaylist;

            if((lastPlayedSong!=itemDaPlaylist)||(autoMode==0)){
                qDebug()<<"lastplayesong != itemdaplaylist";

                // Clear current playlist and add new media
                XplaylistUrls.clear();
                XplaylistUrls.append(QUrl::fromLocalFile(itemDaPlaylist));
                XplaylistIndex = 0;

                // Prevent playbackStateChanged from triggering playNextMedia
                // while we're changing the source
                m_manualAdvancing = true;
                onAbout2Finish = 0;  // Reset so playlistAboutToFinish can fire for the new track

                // Clear previous source first to release any stuck AVFoundation session
                Xplayer->setSource(QUrl());

                // Set the media to play
                Xplayer->setSource(XplaylistUrls[XplaylistIndex]);

                lastPlayedSong = itemDaPlaylist;

                // Capture the track's volume line before its item is deleted;
                // onPositionChanged applies it while this track plays
                m_activeEnvelope = PlaylistWaveView::parseEnvelope(
                    firstItem->data(PlaylistWaveView::VolumeEnvelopeRole).toString());
                m_activeEnvelopePath = itemDaPlaylist;
                if (m_nowPlayingWave)
                    m_nowPlayingWave->setEnvelope(m_activeEnvelope);

                // Apply the level for position 0 right away (a track without
                // a line restores the plain slider volume) so the first
                // instants don't play at the previous track's envelope level
                if (XplayerOutput) {
                    const double base = ui->sliderVolume->value() / 100.0;
                    XplayerOutput->setVolume(float(base
                        * PlaylistWaveView::envelopeGainAt(m_activeEnvelope, 0)));
                    m_envelopeApplied = !m_activeEnvelope.isEmpty();
                }

                int dotsNumInString = itemDaPlaylist.count(".");
                qDebug()<<"dotsNumInString has value: "<<dotsNumInString;

                QFileInfo fileName(itemDaPlaylist);
                QString baseName = fileName.fileName();
                ui->txtNowPlaying->setText(baseName);

                QDateTime now = QDateTime::currentDateTime();
                QString text = now.toString("yyyy-MM-dd || hh:mm:ss ||");
                QString historyNewLine = text + " " + baseName;
                ui->historyList->addItem(historyNewLine);
                int hlistcout = ui->historyList->count();
                qDebug()<<"historyList has "<<hlistcout<<" items";

                if(hlistcout > 99){
                    qDebug()<<"HistoryList is being cleaned because it's over 100 records now...";
                    QListWidgetItem* itemToDelete = ui->historyList->item(0);
                    if (itemToDelete) {
                        delete itemToDelete;
                    }
                }

                Xplayer->play();
                
                // Re-enable automatic advancement now that playback has started
                m_manualAdvancing = false;
                
                // Start the watchdog timer to detect stalled playback
                m_lastKnownPosition = -1;
                m_stallCount = 0;
                m_playbackWatchdog->start();

                // Safely delete the first playlist item
                QListWidgetItem* itemToDelete = ui->playlist->item(0);
                if (itemToDelete) {
                    delete itemToDelete;
                }

                if(ui->checkBox_update_last_played_values->isChecked()){
                    // Prepared statements: file paths may contain quotes
                    QSqlQuery qry(db);
                    qry.prepare("update musics set last_played = :ts where path = :path");
                    qry.bindValue(":ts", now.toString("yyyy-MM-dd || hh:mm:ss"));
                    qry.bindValue(":path", lastPlayedSong);
                    if (!qry.exec()) {
                        qWarning() << "Failed to update last_played:" << qry.lastError().text();
                    }

                    qry.prepare("update musics set played_times = played_times+1 where path = :path");
                    qry.bindValue(":path", lastPlayedSong);
                    if (!qry.exec()) {
                        qWarning() << "Failed to update played_times:" << qry.lastError().text();
                    }
                }

                if(ui->checkBox_random_jingles->isChecked()){
                    int num = ui->spinBox_random_jingles_interval->value();
                    qDebug()<<"Adding a new jingle every "<<num<<" songs.. (setting checkbox to false if value is zero..)";

                    if(num==0){
                        ui->checkBox_random_jingles->setChecked(false);
                    } else {
                        if(jingleCadaNumMusicas==num){
                            jingleCadaNumMusicas = 0;
                            qDebug()<<"Adding a jingle..";

                            int jingleNum = 1;
                            checkDbOpen();
                            QSqlQuery query(db);
                            query.prepare("select path from jingles order by random() limit :num");
                            query.bindValue(":num", jingleNum);
                            if(query.exec())
                            {
                                qDebug() << "SQL query executed: " << query.lastQuery();

                                while(query.next()){
                                    QString path = query.value(0).toString();
                                    ui->playlist->insertItem(0,path);
                                    qDebug() << "autoMode random jingle chooser adding: " << path;
                                }
                            } else {
                                qDebug() << "SQL ERROR: " << query.lastError();
                                qDebug() << "SQL was: " << query.lastQuery();
                            }
                        } else {
                            jingleCadaNumMusicas++;
                            qDebug()<<"jingleCadaNumMusicas incremented to "<<jingleCadaNumMusicas;
                        }
                    }
                }
            } else {
                qDebug()<<"lastplayesong has the same value that itemdaplaylist...";
            }
    } else {
        if(autoMode==1){
            qDebug()<<"PlayMode is not Playing_Segue but autoMode is on.. trying to get songs..";
            int currentPlaylistCount = ui->playlist->count();
            playlistAboutToFinish();
            if(ui->playlist->count() <= currentPlaylistCount) {
                qDebug()<<"No new items added to playlist, stopping";
                Xplayer->stop();
                ui->btPlay->setStyleSheet("");
                ui->btPlay->setText(tr("Play"));
                PlayMode = "stopped";
                return;
            }
            // Songs were added but PlayMode isn't segue — just return, user needs to click play
        }

        qDebug()<<"I'm giving up dude.. there's nothing to play..";
    }
}

void player::on_btStop_clicked()
{
    m_manualAdvancing = true;  // Prevent playbackStateChanged from triggering playNextMedia

    stopTailPlayer(); // silence a crossfade tail that may still be fading out

    // Forcefully reset the media player to recover from any stuck state
    // (AVFoundation on macOS can hang on certain OGG files)
    Xplayer->stop();
    Xplayer->setSource(QUrl());  // Clear the source to fully release AVFoundation resources
    
    m_manualAdvancing = false;
    m_playbackWatchdog->stop();
    m_stallCount = 0;
    ui->btPlay->setStyleSheet("");
    ui->btPlay->setText(tr("Play"));
    PlayMode = "stopped";
    XplaylistUrls.clear();
    XplaylistIndex = 0;
    trackTotalDuration = 0;
    onAbout2Finish = 0;
    lastPlayedSong = "";  // Reset so the same song can be played again after stop
}

void player::on_sliderProgress_sliderMoved(int position)
{
    //qDebug()<<"progreess slider mooved "<<position;
    Xplayer->setPosition(position);
}

void player::on_sliderVolume_sliderMoved(int position)
{
    //qDebug()<<"volume slider mooved "<<position;
    XplayerOutput->setVolume(position / 100.0);

}

void player::onPositionChanged(qint64 position)
{
     ui->sliderProgress->setValue(position);

    if (m_nowPlayingWave)
        m_nowPlayingWave->setPlayhead(position);

    // Volume line: shape the playing track's volume along its envelope
    // (slider volume stays the reference level the line scales from)
    if (!m_activeEnvelope.isEmpty() && Xplayer && XplayerOutput
            && Xplayer->source().isLocalFile()
            && Xplayer->source().toLocalFile() == m_activeEnvelopePath) {
        const double base = ui->sliderVolume->value() / 100.0;
        const double gain = PlaylistWaveView::envelopeGainAt(m_activeEnvelope, position);
        XplayerOutput->setVolume(float(base * gain));
        m_envelopeApplied = true;
    } else if (m_envelopeApplied) {
        // The line no longer applies (new track without one): restore
        m_envelopeApplied = false;
        if (XplayerOutput)
            XplayerOutput->setVolume(ui->sliderVolume->value() / 100.0);
    }

    // Guard against division by zero (duration may not be known yet for some formats)
    if (trackTotalDuration <= 0) {
        // Just update the elapsed time display without percentage calculations
        int segundos = position / 1000;
        int minutos = segundos / 60;
        segundos = segundos % 60;
        int horas = minutos / 60;
        minutos = minutos % 60;
        QString txtElapsedTimeLable = QString("%1:%2:%3")
            .arg(horas, 2, 10, QChar('0'))
            .arg(minutos, 2, 10, QChar('0'))
            .arg(segundos, 2, 10, QChar('0'));
        ui->txtDuration->setText(txtElapsedTimeLable);
        return;
    }

    int valor = (int)((position * 100) / trackTotalDuration);

    if(valor >= 80 && onAbout2Finish == 0){
         playlistAboutToFinish();
     }

    // Overlap segue: when the next playlist item defines a crossfade
    // overlap (set by dragging its wave in the playlist wave view), start
    // it that many ms before the current track ends.
    if (PlayMode == "Playing_Segue" && !m_manualAdvancing && !m_overlapSegueFired
            && position > 0 && ui->playlist->count() > 0) {
        const qint64 overlapMs =
            ui->playlist->item(0)->data(PlaylistWaveView::OverlapRole).toLongLong();
        if (overlapMs > 0) {
            const qint64 remaining = trackTotalDuration - position;
            if (remaining > 0 && remaining <= overlapMs) {
                m_overlapSegueFired = true; // re-armed by the next durationChanged
                // Deferred: don't switch sources from inside a player signal
                QTimer::singleShot(0, this, [this, remaining]() {
                    startOverlapSegue(remaining);
                });
                return;
            }
        }
    }

    int segundos = position / 1000;
    int minutos = segundos / 60;
    segundos = segundos % 60;
    int horas = minutos / 60;
    minutos = minutos % 60;

    QString txtElapsedTimeLable = QString("%1:%2:%3 of %4")
        .arg(horas, 2, 10, QChar('0'))
        .arg(minutos, 2, 10, QChar('0'))
        .arg(segundos, 2, 10, QChar('0'))
        .arg(txtDuration);
    ui->txtDuration->setText(txtElapsedTimeLable);
}

void player::durationChanged(qint64 position)
{
    qDebug()<<"Xplayer durationChanged changed to "<<position;
    ui->sliderProgress->setMaximum(position);
    trackTotalDuration = position;
    m_overlapSegueFired = false; // new media: re-arm the overlap segue

    int segundos = position / 1000;
    int minutos = 0;
    int horas = 0;
    QString xsegundos;
    QString xminutos;
    QString xhoras;
    while(segundos>60)
    {
        ++minutos;
        segundos-=60;
    }
    while(minutos>60)
    {
        ++horas;
        minutos-=60;
    }

    if(segundos<10)
    {
        xsegundos = "0"+QString::number(segundos);
    } else {
        xsegundos = QString::number(segundos);
    }
    if(minutos<10)
    {
        xminutos = "0"+QString::number(minutos);
    } else {
        xminutos = QString::number(minutos);
    }
    if(horas<10)
    {
        xhoras = "0"+QString::number(horas);
    } else {
        xhoras = QString::number(horas);
    }

    txtDuration = xhoras+":"+xminutos+":"+xsegundos;

}

void player::currentMediaChanged(const QUrl &content)
{
    // Unused parameter 'content'
    Q_UNUSED(content);

    //qDebug()<<"On currentMediaChanged with content: "<<content;
    if(onAbout2Finish==1)
    {
        onAbout2Finish = 0;
        // Using XplaylistUrls instead of deprecated QMediaPlaylist
        if (XplaylistIndex < XplaylistUrls.size()) {
            XplaylistUrls.removeAt(XplaylistIndex);
        }
        playNextSong();
    }

}

void player::volumeChanged(float volume){
    qDebug()<<"Volume: "<<volume;
}


void player::lp1_onPositionChanged(qint64 position)
{
    int segundos = position/1000;
    int h = 0;
    int m = 0;

    while (segundos>=60){
        m+=1;
        segundos-=60;
    }

    while (m>=60){
        h+=1;
        m-=60;
    }

    QString fs,fm,fh;


    if(segundos<10){
        fs = "0"+QString::number(segundos);
    } else {
        fs = QString::number(segundos);
    }

    if(m<10){
        fm = "0"+QString::number(m);
    } else {
        fm = QString::number(m);
    }

    if(h<10){
        fh = "0"+QString::number(h);
    } else {
        fh = QString::number(h);
    }

    QString time = fh+":"+fm+":"+fs+tr(" of ")+lp1_total_time;

    ui->lbl_total_time_lp1->setText(time);


    int timeLeft = lp1_total_time_int - position;

    segundos = timeLeft/1000;
    h = 0;
    m = 0;

    while (segundos>=60){
        m+=1;
        segundos-=60;
    }

    while (m>=60){
        h+=1;
        m-=60;
    }

    if(segundos<10){
        fs = "0"+QString::number(segundos);
    } else {
        fs = QString::number(segundos);
    }

    if(m<10){
        fm = "0"+QString::number(m);
    } else {
        fm = QString::number(m);
    }

    if(h<10){
        fh = "0"+QString::number(h);
    } else {
        fh = QString::number(h);
    }

    time = fh+":"+fm+":"+fs;

    ui->lbl_lp1_remaining->setText(time);

    if(segundos==1&&m==0&&h==0){

        lp1_Xplayer->stop();
        ui->lp_1_bt_play->setDisabled(false);
        ui->lp_1->setPixmap(QPixmap(":/images/lp_player_p0.png"));
        if (movie) movie->stop();
        lp_1_paused = false;
        ui->lp_1_bt_pause->setStyleSheet("");
        ui->lbl_lp1_remaining->setText("");
    }


}

void player::lp1_durationChanged(qint64 position)
{

    qDebug()<<"LP 1 Duration Changed changed to "<<position;

    lp1_total_time_int = position;

    int segundos = position/1000;
    int h = 0;
    int m = 0;

    while (segundos>=60){
        m+=1;
        segundos-=60;
    }

    while (m>=60){
        h+=1;
        m-=60;
    }

    QString fs,fm,fh;


    if(segundos<10){
        fs = "0"+QString::number(segundos);
    } else {
        fs = QString::number(segundos);
    }

    if(m<10){
        fm = "0"+QString::number(m);
    } else {
        fm = QString::number(m);
    }

    if(h<10){
        fh = "0"+QString::number(h);
    } else {
        fh = QString::number(h);
    }

    QString time = fh+":"+fm+":"+fs;

   // ui->lbl_total_time_lp1->setText(time);
    lp1_total_time = time;



}

void player::lp1_currentMediaChanged(const QUrl &content)
{
   // Unused parameter 'content'
   Q_UNUSED(content);

   qDebug()<<"LP 1 Current Media Changed..";
}

void player::lp1_volumeChanged(float volume){
    qDebug()<<"LP 1 Volume: "<<volume;
}


void player::lp2_onPositionChanged(qint64 position)
{

    int segundos = position/1000;
    int h = 0;
    int m = 0;

    while (segundos>=60){
        m+=1;
        segundos-=60;
    }

    while (m>=60){
        h+=1;
        m-=60;
    }

    QString fs,fm,fh;


    if(segundos<10){
        fs = "0"+QString::number(segundos);
    } else {
        fs = QString::number(segundos);
    }

    if(m<10){
        fm = "0"+QString::number(m);
    } else {
        fm = QString::number(m);
    }

    if(h<10){
        fh = "0"+QString::number(h);
    } else {
        fh = QString::number(h);
    }

    QString time = fh+":"+fm+":"+fs+tr(" of ")+lp2_total_time;

    ui->lbl_total_time_lp2->setText(time);


    int timeLeft = lp2_total_time_int - position;

    segundos = timeLeft/1000;
    h = 0;
    m = 0;

    while (segundos>=60){
        m+=1;
        segundos-=60;
    }

    while (m>=60){
        h+=1;
        m-=60;
    }

    if(segundos<10){
        fs = "0"+QString::number(segundos);
    } else {
        fs = QString::number(segundos);
    }

    if(m<10){
        fm = "0"+QString::number(m);
    } else {
        fm = QString::number(m);
    }

    if(h<10){
        fh = "0"+QString::number(h);
    } else {
        fh = QString::number(h);
    }

    time = fh+":"+fm+":"+fs;

    ui->lbl_lp2_remaining->setText(time);


    if(segundos==1&&m==0&&h==0){

        lp2_Xplayer->stop();
        ui->lp_1_bt_play_2->setDisabled(false);
        ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p0.png"));
        if (movie2) movie2->stop();  // was movie (LP1's animation) — wrong deck
        lp_2_paused = false;
        ui->lp_2_bt_pause->setStyleSheet("");
        ui->lbl_lp2_remaining->setText("");
    }


}

void player::lp2_durationChanged(qint64 position)
{

    qDebug()<<"LP 2 Duration Changed changed to "<<position;

    lp2_total_time_int = position;

    int segundos = position/1000;
    int h = 0;
    int m = 0;

    while (segundos>=60){
        m+=1;
        segundos-=60;
    }

    while (m>=60){
        h+=1;
        m-=60;
    }

    QString fs,fm,fh;


    if(segundos<10){
        fs = "0"+QString::number(segundos);
    } else {
        fs = QString::number(segundos);
    }

    if(m<10){
        fm = "0"+QString::number(m);
    } else {
        fm = QString::number(m);
    }

    if(h<10){
        fh = "0"+QString::number(h);
    } else {
        fh = QString::number(h);
    }

    QString time = fh+":"+fm+":"+fs;

   // ui->lbl_total_time_lp1->setText(time);
    lp2_total_time = time;
}

void player::lp2_currentMediaChanged(const QUrl &content)
{
    // Unused parameter 'content', but keeping method for signal connection
    Q_UNUSED(content);

    qDebug()<<"LP 2 Current Media Changed..";
}

void player::lp2_volumeChanged(float volume){
    qDebug()<<"LP 2 Volume: "<<volume;
}

void player::playlistAboutToFinish()
{
    qDebug()<<"Launched playlistAboutToFinish";
    onAbout2Finish = 1;

    int numItemsInPlaylist = ui->playlist->count();
    if(numItemsInPlaylist==0)
        autoModeGetMoreSongs();

    // If the next track defines a crossfade overlap, preload the current
    // track into the tail player now so the handoff at the segue point is
    // instantaneous (this fires once per track, at ~80% of its duration).
    if (m_tailPlayer && ui->playlist->count() > 0
            && ui->playlist->item(0)->data(PlaylistWaveView::OverlapRole).toLongLong() > 0
            && Xplayer && Xplayer->source().isLocalFile()
            && m_tailPlayer->playbackState() != QMediaPlayer::PlayingState
            && m_tailPlayer->source() != Xplayer->source()) {
        qDebug() << "Overlap segue: preloading tail player with" << Xplayer->source();
        m_tailPlayer->setSource(Xplayer->source());
    }
}

void player::startOverlapSegue(qint64 fadeMs)
{
    qDebug() << "Overlap segue: starting the next track" << fadeMs
             << "ms before the current one ends";

    // Snapshot the outgoing track BEFORE playNextSong() switches the source
    const QUrl endingSource = Xplayer->source();
    const qint64 endingPos = Xplayer->position();

    if (m_tailPlayer && endingSource.isLocalFile()) {
        if (m_tailFade->state() == QAbstractAnimation::Running)
            m_tailFade->stop();
        const float startVolume = XplayerOutput ? XplayerOutput->volume() : 1.0f;
        m_tailOutput->setVolume(startVolume);
        if (m_tailPlayer->source() != endingSource) // normally preloaded at 80%
            m_tailPlayer->setSource(endingSource);
        m_tailPlayer->play();
        m_tailPlayer->setPosition(endingPos);
        m_tailFade->setStartValue(double(startVolume));
        m_tailFade->setEndValue(0.0);
        // Absolute sanity bound, not the UI window: saved playlists may
        // carry overlaps larger than the current "Max overlap" setting
        m_tailFade->setDuration(int(qBound(qint64(200), fadeMs, qint64(600000))));
        m_tailFade->start();
    }

    // From the state machine's point of view this is just a manual advance;
    // playNextSong() guards the source switch with m_manualAdvancing itself.
    playNextSong();
}

void player::stopTailPlayer()
{
    if (m_tailFade && m_tailFade->state() == QAbstractAnimation::Running)
        m_tailFade->stop();
    if (m_tailPlayer)
        m_tailPlayer->stop();
}

void player::setPlaylistWaveView(bool on)
{
    if (m_waveView)
        m_waveView->setActive(on);
    if (m_waveViewToggle && m_waveViewToggle->isChecked() != on)
        m_waveViewToggle->setChecked(on);
    if (m_nowPlayingWave)
        m_nowPlayingWave->setVisible(on && Xplayer
                                     && Xplayer->source().isLocalFile());
    if (m_maxOverlapBox)
        m_maxOverlapBox->setVisible(on);

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/xfb.conf", QSettings::IniFormat);
    if (settings.value("PlaylistWaveView", false).toBool() != on)
        settings.setValue("PlaylistWaveView", on);
}


void player::update_music_table() {

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    checkDbOpen();

    qInfo() << "Updating tables using connection:" << db.connectionName() << "DB Name:" << db.databaseName();

    // --- Populate music table ---
    // Always delete previous model if reloading to prevent memory leaks and issues
    delete ui->musicView->model(); // Delete old model first
    QSqlTableModel *model = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    model->setTable("musics");
    // Optional: Set edit strategy if needed (BEFORE select)
    // model->setEditStrategy(QSqlTableModel::OnFieldChange);
    if (!model->select()) {
        qWarning() << "Failed to select 'musics' table:" << model->lastError().text();
        delete model; // Clean up failed model
    } else {
        ui->musicView->setModel(model);
        // Configure view AFTER setting model (if needed)
         qDebug() << "'musics' table model set.";
         // Re-apply view settings if necessary as they might reset with new model
         ui->musicView->setSortingEnabled(true);
         ui->musicView->hideColumn(0);
         // ... other ui->musicView settings ...
    }


    // --- Populate jingles table ---
    delete ui->jinglesView->model(); // Delete old model
    QSqlTableModel *jinglesmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    jinglesmodel->setTable("jingles");
    if (!jinglesmodel->select()) {
        qWarning() << "Failed to select 'jingles' table:" << jinglesmodel->lastError().text();
        delete jinglesmodel;
    } else {
        ui->jinglesView->setModel(jinglesmodel);
         qDebug() << "'jingles' table model set.";
         // Configure view if needed
    }


    // --- Populate Pub table ---
    delete ui->pubView->model(); // Delete old model
    QSqlTableModel *pubmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    pubmodel->setTable("pub");
    if (!pubmodel->select()) {
        qWarning() << "Failed to select 'pub' table:" << pubmodel->lastError().text();
        delete pubmodel;
    } else {
        ui->pubView->setModel(pubmodel);
         qDebug() << "'pub' table model set.";
        // Configure view if needed
    }

    // --- Populate Programs table ---
    delete ui->programsView->model(); // Delete old model
    QSqlTableModel *programsmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    programsmodel->setTable("programs");
    if (!programsmodel->select()) {
        qWarning() << "Failed to select 'programs' table:" << programsmodel->lastError().text();
        delete programsmodel;
    } else {
        ui->programsView->setModel(programsmodel);
         qDebug() << "'programs' table model set.";
        // Configure view if needed
    }

    // --- Populate Torrents table ---
    delete ui->torrentsView->model(); // Delete old model
    QSqlTableModel *torrentsmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    torrentsmodel->setTable("torrents");
    if (!torrentsmodel->select()) {
        qWarning() << "Failed to select 'torrents' table:" << torrentsmodel->lastError().text();
        delete torrentsmodel;
    } else {
        ui->torrentsView->setModel(torrentsmodel);
         qDebug() << "'torrents' table model set.";
        // Configure view if needed
    }


    // --- Populate genre1 and 2 filters ---
    // No need to delete QComboBox models usually unless you are replacing them frequently
    // Create model ONCE, maybe in constructor, and just update query if needed?
    // Or if updating here, manage previous model memory:
    delete ui->cBoxGenre1->model(); // Delete previous model data if reloading fully
    // Note: cBoxGenre2 shares the same model, deleting above is sufficient

    // Create model associated with the correct DB connection
    QSqlQueryModel *model_genre1 = new QSqlQueryModel(this); // Parent 'this' manages memory somewhat

    // Use a stack-based QSqlQuery - safer memory management
    QSqlQuery qry(db); // Pass the CORRECT db handle

    QString sqlq = "SELECT name FROM genres1 ORDER BY name"; // Added ORDER BY
    if (!qry.prepare(sqlq)) { // Prepare is safer
         qWarning() << "Failed to prepare genre query:" << qry.lastError().text();
         delete model_genre1; // Clean up model
         return; // Exit if query prep fails
    }

    if (!qry.exec()) { // Execute the prepared query
        qWarning() << "Failed to execute genre query:" << qry.lastError().text();
        delete model_genre1; // Clean up model
        return; // Exit if query exec fails
    }

    model_genre1->setQuery(std::move(qry)); // Set query AFTER successful execution
    if(model_genre1->lastError().isValid()){
         qWarning() << "Error setting query on genre model:" << model_genre1->lastError().text();
    }

    // Set model for BOTH combo boxes
    ui->cBoxGenre1->setModel(model_genre1);
    ui->cBoxGenre1->setModelColumn(0); // Display the first column ('name')

    ui->cBoxGenre2->setModel(model_genre1); // Share the same model
    ui->cBoxGenre2->setModelColumn(0);

    // Also update the random add genre combo box if it uses the same list
    delete ui->comboBox_random_add_genre->model(); // Assuming it should also be updated
    ui->comboBox_random_add_genre->setModel(model_genre1);
    ui->comboBox_random_add_genre->setModelColumn(0);

    qInfo() << "Genre combo boxes updated.";

    qInfo() << "Finished updating tables.";
}



void player::dropEvent(QDropEvent *event)
{

    player *source =
            qobject_cast<player *>(event->source());

    qDebug()<<"::::DROP::::";


        if(xaction == "drag_to_music_playlist"){
            qDebug() << "drop MUSIC event! " << estevalor << " xaction: " << xaction << "evnt source: " << source->objectName();
            //qDebug () << "event mime data" << event->mimeData();





            if(source->objectName()=="player"){
            //sources.append(estevalor);








            int tab_index = ui->tabWidget_2->currentIndex();


            if(tab_index==0){



                ui->playlist->addItem(estevalor);

                if(ui->checkBox_sum_to_playlist_time->isChecked()){
                    calculate_playlist_total_time();
                }




            }


            if(tab_index==2){





                if(ui->lp_1->underMouse()){
                     qDebug()<<"Add to DJ tab :: LP 1 ::"<<estevalor;

                     ui->lp_1_txt_file->setText(estevalor);
                     ui->lp_1->setPixmap(QPixmap(":/images/lp_player_p1.png"));

                }



                if(ui->lp_2->underMouse()){
                     qDebug()<<"Add to DJ tab :: LP 2 ::"<<estevalor;

                        ui->lp_2_txt_file->setText(estevalor);
                        ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p1.png"));

                }







            }





            }
            xaction = "";




        } else {
            qDebug () << "xaction is not defined or is not 'drag_to_music_playlist' .. its content is: " << xaction;
        }


       event->acceptProposedAction();


}

void player::dragEnterEvent(QDragEnterEvent *event)
 {
    //qDebug() << "drag enter event " << event << event->mimeData();
    if(indexJust3rdDropEvt==1){
        //qDebug () << "This is the 2nd interaction and we now accepted the proposed action.";
        event->acceptProposedAction();
        indexJust3rdDropEvt = 0;
    } else{
        indexJust3rdDropEvt++;
    }




 }

void player::on_musicView_pressed(const QModelIndex &index)
{
    indexJust3rdDropEvt=0;
    int rowidx = index.row();
    estevalor = ui->musicView->model()->data(ui->musicView->model()->index(rowidx,7)).toString();
    qDebug () << "Music path: [" << estevalor << "]";
    xaction = "drag_to_music_playlist";

    // Create manual QDrag so the main window's dropEvent receives it
    QByteArray text = estevalor.toUtf8();
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("drag_to_music_playlist", text);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

void player::on_jinglesView_pressed(const QModelIndex &index)
{
    indexJust3rdDropEvt=0;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        //int thisid = index.row()+1;
        ui->jinglesView->selectRow(index.row());
        int rowidx = ui->jinglesView->selectionModel()->currentIndex().row();
        checkDbOpen();
        QSqlTableModel * model = new QSqlTableModel(this,db);
        model->setTable("jingles");
        model->select();
        QString sqlPath = model->index(rowidx , 1).data().toString();
        qDebug()<< sqlPath;
        xaction = "drag_to_music_playlist";
        estevalor = sqlPath;
                //check if file exists and avoid adding if it does not
                bool ha = QFile::exists (sqlPath);
                if(!ha){

                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(this, "The file does NOT exist?", "It seams like the file does NOT exist on the hard drive... Should it be deleted from the database?",
                                                  QMessageBox::Yes|QMessageBox::No);
                    if (reply == QMessageBox::Yes) {
                        checkDbOpen();
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery(db);
                          qry->prepare("delete from jingles where path = :thpath");
                          qry->bindValue(":thpath",sqlPath);

                         if(qry->exec()){
                              qDebug() << "Music Deleted form database! last query was:"<< qry->lastQuery();
                              update_music_table();
                         } else {
                             qDebug() << "There was an error deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                         }
                    } else {
                      qDebug() << "keeping invalid record in db... please fix path..";
                    }

                }


        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setData(text, "drag_to_music_playlist");
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);

}

void player::on_pubView_pressed(const QModelIndex &index)
{
    indexJust3rdDropEvt=0;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        //int thisid = index.row()+1;
        ui->pubView->selectRow(index.row());
        int rowidx = ui->pubView->selectionModel()->currentIndex().row();
checkDbOpen();
        QSqlTableModel * model = new QSqlTableModel(this,db);
        model->setTable("pub");
        model->select();
        QString sqlPath = model->index(rowidx , 2).data().toString();
        qDebug()<< sqlPath;
        xaction = "drag_to_music_playlist";
        estevalor = sqlPath;
                //check if file exists and avoid adding if it does not
                bool ha = QFile::exists (sqlPath);
                if(!ha){

                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(this, "The file does NOT exist?", "It seams like the file does NOT exist on the hard drive... Should it be deleted from the database?",
                                                  QMessageBox::Yes|QMessageBox::No);
                    if (reply == QMessageBox::Yes) {
                        checkDbOpen();
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery(db);
                          qry->prepare("delete from pub where path = :thpath");
                          qry->bindValue(":thpath",sqlPath);

                         if(qry->exec()){
                              qDebug() << "Pub deleted form database! last query was:"<< qry->lastQuery();
                              update_music_table();
                         } else {
                             qDebug() << "There was an error deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                         }
                    } else {
                      qDebug() << "keeping invalid record in db... please fix path..";
                    }

                }





        //qDebug() << "Pressed row is: " << index.row() << " and QVariant valor is: " << valor.toString();
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setData(text, "drag_to_music_playlist");
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);

}

void player::on_programsView_pressed(const QModelIndex &index)
{
    indexJust3rdDropEvt=0;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        //int thisid = index.row()+1;
        ui->programsView->selectRow(index.row());
        int rowidx = ui->programsView->selectionModel()->currentIndex().row();
checkDbOpen();
        QSqlTableModel * model = new QSqlTableModel(this,db);
        model->setTable("programs");
        model->select();
        QString sqlPath = model->index(rowidx , 2).data().toString();
        qDebug()<< sqlPath;
        xaction = "drag_to_music_playlist";
        estevalor = sqlPath;
                //check if file exists and avoid adding if it does not
                bool ha = QFile::exists (sqlPath);
                if(!ha){

                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(this, "The file does NOT exist?", "It seams like the file does NOT exist on the hard drive... Should it be deleted from the database?",
                                                  QMessageBox::Yes|QMessageBox::No);
                    if (reply == QMessageBox::Yes) {
                        checkDbOpen();
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery(db);
                          qry->prepare("delete from programs where path = :thpath");
                          qry->bindValue(":thpath",sqlPath);

                         if(qry->exec()){
                              qDebug() << "Program deleted form database! last query was:"<< qry->lastQuery();
                              update_music_table();
                         } else {
                             qDebug() << "There was an error deleting the program from the database"<< qry->lastError() << qry->lastQuery();
                         }
                    } else {
                      qDebug() << "keeping invalid record in db... please fix path.. ";
                    }

                }





        //qDebug() << "Pressed row is: " << index.row() << " and QVariant valor is: " << valor.toString();
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setData(text, "drag_to_music_playlist");
        drag->setMimeData(mimeData);
        drag->exec(Qt::CopyAction);

}

void player::on_torrentsView_pressed(const QModelIndex &index)
{
    indexJust3rdDropEvt=0;

    ui->torrentsView->selectRow(index.row());
    int rowidx = ui->torrentsView->selectionModel()->currentIndex().row();
    
    // Check if this is a torrent search result (QStandardItemModel) or a database record (QSqlTableModel)
    QModelIndex nameIndex = ui->torrentsView->model()->index(rowidx, 0);
    QVariant resultData = nameIndex.data(Qt::UserRole);
    
    if (resultData.isValid() && resultData.canConvert<TorrentSearchResult>()) {
        // This is a search result — show download context menu instead of treating as a local file
        TorrentSearchResult result = resultData.value<TorrentSearchResult>();
        
        QMenu menu(this);
        QAction *downloadAction = menu.addAction(tr("Download Torrent"));
        QAction *downloadStreamAction = menu.addAction(tr("Download and Stream"));
        menu.addSeparator();
        QAction *copyMagnetAction = menu.addAction(tr("Copy Magnet Link"));
        
        QAction *selected = menu.exec(QCursor::pos());
        if (!selected) return;
        
        if (selected == copyMagnetAction) {
            if (!result.magnetLink.isEmpty()) {
                QApplication::clipboard()->setText(result.magnetLink);
                ui->statusBar->showMessage(tr("Magnet link copied to clipboard"), 3000);
            } else {
                // Fetch magnet link from the detail page first
                ui->statusBar->showMessage(tr("Fetching magnet link..."), 0);
                m_torrentSearchService->fetchMagnetLink(result.torrentUrl, [this](const QString &magnet) {
                    if (!magnet.isEmpty()) {
                        QApplication::clipboard()->setText(magnet);
                        ui->statusBar->showMessage(tr("Magnet link copied to clipboard"), 3000);
                    } else {
                        ui->statusBar->showMessage(tr("Could not retrieve magnet link"), 3000);
                    }
                });
            }
        } else if (selected == downloadAction || selected == downloadStreamAction) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, 
                tr("Download Confirmation"), 
                tr("You are about to download: %1\n\n"
                   "By proceeding, you confirm that you have the legal right "
                   "to download this content.\n\n"
                   "Proceed?").arg(result.name),
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes && m_torrentDownloadService) {
                if (!ensureTorrentClient()) {
                    return;
                }
                bool wantStream = (selected == downloadStreamAction);
                QString torrentName = result.name;
                
                if (!result.magnetLink.isEmpty()) {
                    // Already have a magnet link — start download directly
                    QString downloadId = m_torrentDownloadService->startDownload(result.magnetLink, torrentName);
                    if (!downloadId.isEmpty()) {
                        if (wantStream) {
                            m_torrentDownloadService->enableStreaming(downloadId);
                        }
                        ui->statusBar->showMessage(tr("Download started: %1").arg(torrentName), 5000);
                    } else {
                        QMessageBox::warning(this, tr("Download Error"), 
                                            tr("Failed to start download."));
                    }
                } else {
                    // Need to fetch the magnet link from the torrent detail page first
                    ui->statusBar->showMessage(tr("Fetching magnet link for: %1").arg(torrentName), 0);
                    ui->downloadsPanel->setVisible(true);
                    m_torrentSearchService->fetchMagnetLink(result.torrentUrl, 
                        [this, torrentName, wantStream](const QString &magnet) {
                        if (magnet.isEmpty()) {
                            QMessageBox::warning(this, tr("Download Error"), 
                                                tr("Could not retrieve magnet link for this torrent."));
                            ui->statusBar->showMessage(tr("Failed to get magnet link"), 3000);
                            return;
                        }
                        QString downloadId = m_torrentDownloadService->startDownload(magnet, torrentName);
                        if (!downloadId.isEmpty()) {
                            if (wantStream) {
                                m_torrentDownloadService->enableStreaming(downloadId);
                            }
                            ui->statusBar->showMessage(tr("Download started: %1").arg(torrentName), 5000);
                        } else {
                            QMessageBox::warning(this, tr("Download Error"), 
                                                tr("Failed to start download."));
                        }
                    });
                }
            }
        }
        return;
    }
    
    // Original database record handling
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    checkDbOpen();
    QSqlTableModel *model = new QSqlTableModel(this, db);
    model->setTable("torrents");
    model->select();
    QString sqlPath = model->index(rowidx, 2).data().toString();
    qDebug() << sqlPath;
    xaction = "drag_to_music_playlist";
    estevalor = sqlPath;
    
    bool ha = QFile::exists(sqlPath);
    if (!ha) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "The file does NOT exist?", 
                                      "It seams like the file does NOT exist on the hard drive... Should it be deleted from the database?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            checkDbOpen();
            qDebug() << "the file should be deleted from the database cause it does not exist in the hd";
            QSqlQuery *qry = new QSqlQuery(db);
            qry->prepare("delete from torrents where path = :thpath");
            qry->bindValue(":thpath", sqlPath);
            if (qry->exec()) {
                qDebug() << "Torrent deleted from database!";
                update_music_table();
            } else {
                qDebug() << "Error deleting torrent:" << qry->lastError();
            }
            delete qry;
        }
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData(text, "drag_to_music_playlist");
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
    
    delete model;
}

void player::on_bt_autoMode_clicked()
{
    if(autoMode == 0){
        autoMode = 1;
        qDebug()<<"autoMode is ON";
        ui->bt_autoMode->setStyleSheet("background-color: rgb(175, 227, 59)");
    } else {
        autoMode = 0;
        qDebug()<<"autoMode is OFF";
        ui->bt_autoMode->setStyleSheet("");
    }
}

void player::autoModeGetMoreSongs()
{

    //check if there's a programed genre for this hour in the hourgenre table

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QDateTime now = QDateTime::currentDateTime();
    QString currentHour = now.toString("h");
    int dia = QDate::currentDate().dayOfWeek();

    QString dw;
    if(dia == 1) dw = "1";
    if(dia == 2) dw = "2";
    if(dia == 3) dw = "3";
    if(dia == 4) dw = "4";
    if(dia == 5) dw = "5";
    if(dia == 6) dw = "6";
    if(dia == 7) dw = "7";

checkDbOpen();
    QSqlQuery runQry(db);
    runQry.prepare("select genre from hourgenre where hour=:hour and day=:day");
    runQry.bindValue(":hour", currentHour);
    runQry.bindValue(":day", dw);
    QString currentGenre = "";

    if(runQry.exec()){

        qDebug()<<"The query was successfull: "<<runQry.lastQuery();

        while(runQry.next()){
            currentGenre = runQry.value(0).toString();
            qDebug()<<"We now have selected the following genre for this hour, based on the data from the hourgenre table in the database: "<<currentGenre;
        }

    } else {

        qDebug()<<"Oh dear, what now? "<<runQry.lastError();


    }


    if(currentGenre.isEmpty()){

           if(autoMode==1){
               //randomly select music from db
               int numMusics = 1;
               QSqlQuery query(db);
               query.prepare("select path from musics order by random() limit :numMusics");
               query.bindValue(":numMusics", numMusics);
               if(query.exec())
               {
                   qDebug() << "SQL query executed: " << query.lastQuery();

                   while(query.next()){
                       QString path = query.value(0).toString();

                       if(path!=lastPlayedSong){
                           ui->playlist->addItem(path);
                           qDebug() << "autoMode random music chooser adding: " << path;

                       } else {

                           QSqlQuery numOfItemsInDB(db);
                           numOfItemsInDB.prepare("select count(path) from music where 1");
                           numOfItemsInDB.exec();
                           if(numOfItemsInDB.value(0).toInt()>1){
                               qDebug()<<"autoMode picked the same song that was played before... Small DB or a Big Coincidence? Quering again for a 'random' new song..";
                               autoModeGetMoreSongs();
                           }

                       }

                   }

               } else {
                   qDebug() << "SQL ERROR: " << query.lastError();
                   qDebug() << "SQL was: " << query.lastQuery();


               }
           }

    } else {

        if(autoMode==1){
            //randomly select music from db based on genre for this hour

            QSqlQuery query(db);
            query.prepare("select path from musics where genre1 like :genre order by random() limit 1");
            query.bindValue(":genre", currentGenre);
            if(query.exec())
            {
                qDebug() << "SQL query executed: " << query.lastQuery();

                while(query.next()){
                    QString path = query.value(0).toString();

                    if(path!=lastPlayedSong){
                        ui->playlist->addItem(path);
                        qDebug() << "autoMode genre based random music chooser adding: " << path;

                    } else {

                        QSqlQuery numOfItemsInDB(db);
                        numOfItemsInDB.prepare("select count(path) from music where 1");
                        numOfItemsInDB.exec();
                        if(numOfItemsInDB.value(0).toInt()>1){
                            qDebug()<<"autoMode picked the same song that was played before... Small DB or a Big Coincidence? Quering again for a 'random' new song..";
                            autoModeGetMoreSongs();
                        }

                    }

                }

            } else {
                qDebug() << "SQL ERROR: " << query.lastError();
                qDebug() << "SQL was: " << query.lastQuery();


            }
            currentGenre = "";
        }

    }



}

void player::on_actionAdd_a_single_song_triggered()
{
    add_music_single add_music_single;
    add_music_single.setModal(true);
    add_music_single.exec();
    update_music_table();
}

void player::on_btPlayNext_clicked()
{
    qDebug() << "Play Next button clicked";
    
    // Suppress the auto-advance that fires when the current source stops
    m_manualAdvancing = true;
    
    playNextSong();
    
    // Re-enable auto-advance after the event loop processes the state change
    QTimer::singleShot(200, this, [this]() { m_manualAdvancing = false; });
}

void player::on_actionAdd_all_songs_in_a_folder_triggered()
{
     qDebug()<<"Add a full dir";
     add_full_dir add_full_dir;
     add_full_dir.setModal(true);
     add_full_dir.exec();
     update_music_table();

}

void player::on_actionManage_Genres_triggered()
{
    addgenre addgenre;
    addgenre.setModal(true);
    addgenre.exec();

}

void player::on_actionAdd_Jingle_triggered()
{
    addJingle addjingle;
    addjingle.setModal(true);
    addjingle.exec();
    update_music_table();
}

void player::on_actionAdd_a_publicity_triggered()
{
    add_pub addp;
    addp.setModal(true);
    addp.exec();
    update_music_table();
}


void player::monitorTakeOver(){


    QString hora = QDateTime::currentDateTime().toString("hh:mm:ss");

    QString tkOut = "";

    tkOut += hora+"-----> TakeOver Monitoring ";



    QString takeoverfile = TakeOverPath+"/takeover.xml";
    QFile file(takeoverfile);

    tkOut += takeoverfile;


    if(!file.open(QFile::ReadOnly | QFile::Text)){
         tkOut += ": nothing found. ";
         qDebug()<<tkOut;
    } else {


        QXmlStreamReader Rxml;
        Rxml.setDevice(&file);
        Rxml.readNext();

        while(!Rxml.atEnd()){

            if (Rxml.isStartElement()) {

                        if (Rxml.name() == QStringLiteral("XFBClientTakeOver")) {
                            qDebug()<<"Valid XFB TakeOver Found!";
                            Rxml.readNext();

                            if(Rxml.isEndElement()){
                                qDebug()<<"Found the last element of the XML file after StarElement, leaving the while loop";
                                Rxml.readNext();
                                break;
                            }

                        } else {
                            Rxml.raiseError(QObject::tr("Not an XFB TakeOver file"));
                        }
                    } else {
                        Rxml.readNext();

                        if(Rxml.name()==QStringLiteral("www.netpack.pt")){
                            qDebug()<<"Token element: "<<Rxml.name();
                            Rxml.readNext();
                        }

                        if(Rxml.name()==QStringLiteral("ip")){
                            takeOverIP = Rxml.readElementText();
                            qDebug()<<"takeOverIP: "<<takeOverIP;
                        }

                        if(Rxml.name()==QStringLiteral("stream")){
                            takeOverStream = Rxml.readElementText();
                            qDebug()<<"TakeOverStream: "<<takeOverStream;



                            //play



                            qDebug()<<"Starting mplayer for takeover stream:"<<takeOverStream;

                            radio1.start("mplayer", QStringList()
                                         << "-volume" << "100"
                                         << "-playlist" << takeOverStream);
                            radio1.waitForStarted(-1);
                            radio1.closeReadChannel(QProcess::StandardOutput);
                            radio1.closeReadChannel(QProcess::StandardError);


                            ui->txtNowPlaying->setText(takeOverStream);

                            QDateTime now = QDateTime::currentDateTime();
                            QString text = now.toString("yyyy-MM-dd || hh:mm:ss ||");
                            QString historyNewLine = text + " " + takeOverStream;
                            ui->historyList->addItem(historyNewLine);

                            //stop the main player...

                            QTimer::singleShot(500, this, SLOT(MainsetVol100()));
                            QTimer::singleShot(1000, this, SLOT(MainsetVol80()));
                            QTimer::singleShot(1500, this, SLOT(MainsetVol60()));
                            QTimer::singleShot(2000, this, SLOT(MainsetVol40()));
                            QTimer::singleShot(2500, this, SLOT(MainsetVol20()));
                            QTimer::singleShot(3000, this, SLOT(MainsetVol10()));
                            QTimer::singleShot(3500, this, SLOT(MainsetVol5()));
                            QTimer::singleShot(4000, this, SLOT(MainStop()));


                            //rename the takeover xml

                            qDebug()<<"Converting TakeOver file into confirmation..";
                            QString confirmtakeover = TakeOverPath+"/confirmtakeover.xml";

                            QFile::rename(takeoverfile,confirmtakeover);
                            qDebug()<<"TakeOver file converted into confirmation file!";


                            QTimer::singleShot(120000, this, SLOT(rmConfirmTakeOver()));

                            //ping the client

                            connect(stimer, &QTimer::timeout, this, &player::pingTakeOverClient);
                            stimer->start(30000);



                        }



                    }
        }
        file.close();

    }

    QString ret = TakeOverPath+"/returntakeover.xml";
    QFile rfile(ret);

    tkOut = "";

    tkOut += hora+"-----> ReturnTakeOver Monitoring "+ret;


    if(!rfile.open(QFile::ReadOnly | QFile::Text)){
       tkOut += ": nothing found.";
       qDebug()<<tkOut;
    } else {

        stimer->stop();


        QXmlStreamReader Rxml;
        Rxml.setDevice(&rfile);
        Rxml.readNext();

        while(!Rxml.atEnd()){

            if (Rxml.isStartElement()) {

                        if (Rxml.name() == QStringLiteral("XFBClientTakeOver")) {
                            qDebug()<<"Valid XFB returnTakeOver Found!";
                            Rxml.readNext();

                            if(Rxml.isEndElement()){
                                qDebug()<<"Found the last element of the XML file after StarElement, leaving the while loop";
                                Rxml.readNext();
                                break;
                            }

                        } else {
                            Rxml.raiseError(QObject::tr("Not an XFB returnTakeOver file"));
                        }
                    } else {
                        Rxml.readNext();

                        if(Rxml.name()==QStringLiteral("www.netpack.pt")){
                            qDebug()<<"Token element: "<<Rxml.name();
                            Rxml.readNext();
                        }

                        if(Rxml.name()==QStringLiteral("ip")){
                            returnTakeOverIP = Rxml.readElementText();
                            qDebug()<<"returnTakeOverIP: "<<returnTakeOverIP;
                        }

                        if(Rxml.name()==QStringLiteral("cmd")){
                            takeOverStream = Rxml.readElementText();
                            qDebug()<<"ReturnTakeOver :: "<<takeOverStream;

                            if(takeOverStream=="returnTakeOver" && returnTakeOverIP==takeOverIP){

                                qDebug()<<"Closing takeOver from: "<<returnTakeOverIP;

                                QTimer::singleShot(3000, this, SLOT(stopMplayer()));

                                //ensure stop

                                on_btStop_clicked();



                                //Start the main player...

                                QTimer::singleShot(250, this, SLOT(on_btPlay_clicked()));


                                QTimer::singleShot(500, this, SLOT(MainsetVol5()));
                                QTimer::singleShot(1000, this, SLOT(MainsetVol10()));
                                QTimer::singleShot(1500, this, SLOT(MainsetVol20()));
                                QTimer::singleShot(2000, this, SLOT(MainsetVol40()));
                                QTimer::singleShot(2500, this, SLOT(MainsetVol60()));
                                QTimer::singleShot(3000, this, SLOT(MainsetVol80()));
                                QTimer::singleShot(3500, this, SLOT(MainsetVol100()));






                            } else {

                                qDebug()<<"The returnTakeOver IP: "<<returnTakeOverIP<<" tried to close a takeOver connection created by: "<<takeOverIP;

                            }





                        }



                    }
        }
        rfile.close();
        QString rmthis = TakeOverPath+"/returntakeover.xml";
        QFile::remove(rmthis);

    }









}

void player::run_server_scheduler(){


    QString hora = QDateTime::currentDateTime().toString("hh:mm");



    qDebug()<<"-------------------------------> Running Server Scheduler <-------------------------------------\n"
              "----------------------------------> "<<hora<<" <------------------------------------------------";



server_check_and_schedule_new_programs();

}


void player::server_check_and_schedule_new_programs(){


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    //check the programs folder and get the name of the programs/folders

    qDebug()<<"Monitoring ProgramsPath var that is set to: "<<ProgramsPath    ;

    QDirIterator folder_it(ProgramsPath, QStringList() << "*", QDir::Files);

    while(folder_it.hasNext()){
        QString fit = folder_it.next();
        qDebug()<<"Folder Iterator fodler_it found the value: "<<fit;

        QStringList fit_array = fit.split("/");
        QString fit_name = fit_array.last();

        QStringList fit_array2 = fit_name.split("_");

        QString fit_prog_name = fit_array2[0];

        qDebug()<<"NEW UPLOADED FILE FOUND: "<<fit_prog_name;

        QString mv2folder = ProgramsPath+"/"+fit_prog_name+"/"+fit_name;



        QDir dir;

        bool check = dir.rename(fit,mv2folder);

        if(check==true){
            qDebug()<<"Uploaded program was moved to "<<mv2folder;
        } else {
            qDebug()<<"There was an error moving the uploaded file to "<<mv2folder;
        }


    }

    QDirIterator it(ProgramsPath, QStringList() << "*", QDir::Dirs, QDirIterator::Subdirectories);
    while (it.hasNext()){




        QString dir = it.next();
        QStringList lst = dir.split("/");
        QString ls = lst.last();



        if((ls!=".") && (ls!="..")){
            qDebug() << ls;

            //check every folder


            qDebug()<<"Looking for programs...";
            QDirIterator fit(dir,QStringList()<< "*.mp3"<<"*.mp4"<<"*.ogg"<<"*.wav"<<"*.flac",QDir::Files,QDirIterator::Subdirectories);
            while (fit.hasNext()){

                QString file = fit.next();

                qDebug()<<"Found: "<<file;




                //for every file in this folder check if it exist in DB



                QSqlQuery sql(db);
                sql.prepare("SELECT path from programs where path=:path");
                sql.bindValue(":path", file);

                if(sql.exec()){
                    qDebug()<<"Query ran fine: "<<sql.lastQuery();

                    QString ha = "default";

                    while(sql.next()){

                        ha = sql.value(0).toString();

                        qDebug()<<"Query returned: "<<ha;

                    }

                    if(ha=="default"){
                        qDebug()<<"Query didn't return any rows.. so adding it..";
                        //add if if not

                        QFileInfo info(file);
                        QString filename(info.fileName());



                        QSqlQuery sql_add(db);
                        sql_add.prepare("INSERT INTO programs VALUES(NULL, ?, ?)");
                        sql_add.addBindValue(filename);
                        sql_add.addBindValue(file);
                        if(sql_add.exec()){
                            qDebug()<<"Query OK. Program localy added to programs table";

                             //schelule it





                            QSqlQuery qryid(db);
                            QString thisqueryid = "select * from programs where path like '"+file+"'";
                            qDebug()<<"server programs monitorization :: Select id query is: "<<thisqueryid;
                            if(qryid.exec(thisqueryid)){
                                while(qryid.next()){
                                    QString pID = qryid.value(0).toString();
                                    qDebug()<<"server programs monitorization :: Query OK. This id is: "<<pID;

                                    QStringList divide_filename = filename.split("_");
                                    QString nomeDoPrograma = divide_filename[0];

                                    QStringList divide_ext = divide_filename[1].split(".");
                                    QString dataDoPrograma = divide_ext[0];

                                    QStringList dataarr = dataDoPrograma.split("-");
                                    QString pAno = dataarr[0];
                                    QString pMes = dataarr[1];
                                    QString pDia = dataarr[2];

                                    if(!pAno.isEmpty() && !pMes.isEmpty() && !pDia.isEmpty()){

                                        QString qryhourmin = "select hour, min from hourprograms where name like '"+nomeDoPrograma+"'";

                                        QSqlQuery qhm(db);

                                        if(qhm.exec(qryhourmin)){

                                            QString def = "def";

                                            while(qhm.next()){
                                                def = "not";
                                                QString pHora = qhm.value(0).toString();
                                                QString pMin = qhm.value(1).toString();

                                                QSqlQuery addsch(db);
                                                addsch.prepare("INSERT INTO scheduler VALUES (?, ?, ?, ?, ?, ?, '1', NULL, NULL, NULL, NULL, NULL, NULL, NULL, '1')");
                                                addsch.addBindValue(pID);
                                                addsch.addBindValue(pAno);
                                                addsch.addBindValue(pMes);
                                                addsch.addBindValue(pDia);
                                                addsch.addBindValue(pHora);
                                                addsch.addBindValue(pMin);

                                                if(addsch.exec()){
                                                    qDebug()<<"server programs monitorization :: Program scheduled correctly.";
                                                    qDebug()<<nomeDoPrograma<< " :: "<<pAno<<"-"<<pMes<<"-"<<pDia<<" at "<<pHora<<":"<<pMin;
                                                } else {
                                                    qDebug()<<"server programs monitorization :: It was not possible to add program to scheduler: "<<addsch.lastError();
                                                }
                                            }

                                            if(def=="def"){

                                                qDebug()<<"The program "<<nomeDoPrograma<<" hasn't got an hour and minute extablished in the hourprograms table so XFB can't add it by itself..";

                                            }



                                        } else {

                                            qDebug()<<"server programs monitorization :: It was not possible to figure out the hour and minute for this program: "<<nomeDoPrograma;
                                            qDebug()<<"server programs monitorization :: This should be in the 'hourprograms' table.";

                                        }


                                    } else {

                                        qDebug()<<"server programs monitorization :: We got a program but there was an error adding it beacuse one value of the data is empty. Please check the programs are named like 'name_YYYY-mm-dd.ogg'";

                                    }
                                }




                            } else {
                                 qDebug()<<"server programs monitorization :: Query was not ok while atempting to get ID from the programs table"<<qryid.lastError();
                            }


                        } else {
                             qDebug()<<"Query was not ok while atempting to localy add to the programs table: "<<sql_add.lastError();
                        }


                    }


                } else {
                     qDebug()<<"Error running query: "<<sql.lastError();
                }
            }





        }
}










    //do the same for songs






    qDebug()<<"Monitoring MusicPath var that is set to: "<<MusicPath;
















    qDebug()<<"Looking for musics in the subdirectories of Music Path... [BUG FIX :: NOT LOOKING BECAUSE IT HAS TO BE MOOVED TO ANOTHER THREAD FIST]";
/*

    QDirIterator mit(MusicPath,QStringList()<<"*", QDir::Dirs,QDirIterator::Subdirectories);
    while(mit.hasNext()){




        QString dir = mit.next();
        QStringList lst = dir.split("/");
        QString ls = lst.last();



        if((ls!=".") && (ls!="..")){
            qDebug() << "Processing: "<<dir;

            qDebug()<<"Looking for musics...";
            QDirIterator fmit(dir,QStringList()<< "*.mp3"<<"*.mp4"<<"*.ogg"<<"*.wav"<<"*.flac",QDir::Files,QDirIterator::Subdirectories);
            while (fmit.hasNext()){

                QString file = fmit.next();

                qDebug()<<"Found: "<<file;



                //for every file in this folder check if it exist in DB



                QSqlQuery sql(db);
                sql.prepare("SELECT path from musics where path=:path");
                sql.bindValue(":path", file);

                if(sql.exec()){
                    qDebug()<<"Query ran fine: "<<sql.lastQuery();

                    QString ha = "default";

                    while(sql.next()){

                        ha = sql.value(0).toString();

                        qDebug()<<"Query returned: "<<ha;

                    }

                    if(ha=="default"){
                        qDebug()<<"Query didn't return any rows ------------> So adding..      ";
                        //add if if not






                        QFileInfo info(file);
                        QString filename(info.fileName());







                        QString artist = filename;
                        QString song = "";

                        QStringList splitFileName = filename.split("-");
                        if(splitFileName.count()>1){
                            artist = splitFileName[0];
                            song = splitFileName[1];
                        }


                        QStringList genre_split = dir.split("/");


                        QString genre = genre_split.last();

                        qDebug()<<"------------------>>>>>>>>>>>>>>>>>>>>  GENRE: "<<genre;


                        QString g1 = genre, g2 = genre;

                        QString country = "Other country / language";

                        QString pub_date = "2016/01/01";



                        QProcess cmd;
                        QString time;
                        // Use QProcess argument list to avoid shell injection via filenames
                        cmd.start("exiftool", QStringList() << file);
                        cmd.waitForFinished();
                        QString cmdOut = cmd.readAll();
                        // Filter for Duration line
                        QString durationLine;
                        for (const QString &line : cmdOut.split("\n")) {
                            if (line.contains("Duration", Qt::CaseInsensitive)) {
                                durationLine = line;
                                break;
                            }
                        }
                        qDebug()<<"Output of exiftool: "<<durationLine;
                        cmd.close();

                        QStringList arraycmd = durationLine.split(" ");
                        if(arraycmd.count()>1){
                            time = arraycmd.last().trimmed();
                            qDebug()<<"Total track time is: "<<time;

                        } else {
                            qDebug()<<"-------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>     !!!!!!!!    An exception happend !? ... outputing details of this track: ";
                            for(int i=0;i<arraycmd.count();i++){
                                qDebug()<< "The array position "<<i<<" has: "<<arraycmd[i];
                               }

                           }

                        QSqlQuery sql_add;
                        int played = 0;
                        QString last = "-";
                        sql_add.prepare("INSERT INTO musics VALUES(NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                        sql_add.addBindValue(artist);
                        sql_add.addBindValue(song);
                        sql_add.addBindValue(g1);
                        sql_add.addBindValue(g2);
                        sql_add.addBindValue(country);
                        sql_add.addBindValue(pub_date);
                        sql_add.addBindValue(file);
                        sql_add.addBindValue(time);
                        sql_add.addBindValue(played);
                        sql_add.addBindValue(last);
                        if(sql_add.exec()){
                            qDebug()<<"Query OK. Music localy added to musics table :: "<<sql_add.lastQuery();


                        } else {
                             qDebug()<<"Query was not ok while atempting to localy add to the musics table: "<<sql_add.lastError();
                        }


                    }


                } else {
                     qDebug()<<"Error running query: "<<sql.lastError();
                }




}




            }








        }
*/








    qDebug()<<"Looking for musics on the root folder... [BUG FIX :: NOT LOOKING BECAUSE IT HAS TO BE MOOVED TO ANOTHER THREAD FIST]";
    /*
    QDirIterator rfmit(MusicPath,QStringList()<< "*.mp3"<<"*.mp4"<<"*.ogg"<<"*.wav"<<"*.flac",QDir::Files);
    while (rfmit.hasNext()){

        QString file = rfmit.next();

        qDebug()<<"Found: "<<file;



        //for every file in this folder check if it exist in DB



        QSqlQuery sql(db);
        sql.prepare("SELECT path from musics where path=:path");
        sql.bindValue(":path", file);

        if(sql.exec()){
            qDebug()<<"Query ran fine: "<<sql.lastQuery();

            QString ha = "default";

            while(sql.next()){

                ha = sql.value(0).toString();

                qDebug()<<"Query returned: "<<ha;

            }

            if(ha=="default"){
                qDebug()<<"Query didn't return any rows";
                //add if if not






                QFileInfo info(file);
                QString filename(info.fileName());







                QString artist = filename;
                QString song = "";

                QStringList splitFileName = filename.split("-");
                if(splitFileName.count()>1){
                    artist = splitFileName[0];
                    song = splitFileName[1];
                }


                QString g1 = "Pop", g2 = "Pop";

                QString country = "Other country / language";

                QString pub_date = "2016/01/01";



                QProcess cmd;
                QString time;
                // Use QProcess argument list to avoid shell injection via filenames
                cmd.start("exiftool", QStringList() << file);
                cmd.waitForFinished();
                QString cmdOut = cmd.readAll();
                // Filter for Duration line
                QString durationLine;
                for (const QString &line : cmdOut.split("\n")) {
                    if (line.contains("Duration", Qt::CaseInsensitive)) {
                        durationLine = line;
                        break;
                    }
                }
                qDebug()<<"Output of exiftool: "<<durationLine;
                cmd.close();

                QStringList arraycmd = durationLine.split(" ");
                if(arraycmd.count()>1){
                    time = arraycmd.last().trimmed();
                    qDebug()<<"Total track time is: "<<time;

                } else {
                    qDebug()<<"-------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>     !!!!!!!!    An exception happend !? ... outputing details of this track: ";
                    for(int i=0;i<arraycmd.count();i++){
                        qDebug()<< "The array position "<<i<<" has: "<<arraycmd[i];
                       }

                   }


                QSqlQuery sql_add;
                int played = 0;
                QString last = "-";
                sql_add.prepare("INSERT INTO musics VALUES(NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                sql_add.addBindValue(artist);
                sql_add.addBindValue(song);
                sql_add.addBindValue(g1);
                sql_add.addBindValue(g2);
                sql_add.addBindValue(country);
                sql_add.addBindValue(pub_date);
                sql_add.addBindValue(file);
                sql_add.addBindValue(time);
                sql_add.addBindValue(played);
                sql_add.addBindValue(last);
                if(sql_add.exec()){
                    qDebug()<<"Query OK. Music localy added to musics table :: "<<sql_add.lastQuery();


                } else {
                     qDebug()<<"Query was not ok while atempting to localy add to the musics table: "<<sql_add.lastError();
                }


            }


        } else {
             qDebug()<<"Error running query: "<<sql.lastError();
        }




}





*/

















    //do the same for jingles?





    update_music_table();

}




void player::server_ftp_check(){


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            qDebug()<<"server_ftp_check() :: Looking for new programs in the FTP server to download";
            QProcess sh,sh2, sh3;
            QByteArray output, output2;
            QString outPath, FTPCmdPath, xmls;

            sh.start("sh", QStringList() << "-c" << "pwd");
            sh.waitForFinished(-1);
            output = sh.readAll();
            outPath = output;
            QStringList path_arry = outPath.split("\n");
            FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsGetPrograms";
            qDebug() << "server_ftp_check() :: running: " << FTPCmdPath;
            qDebug() << "server_ftp_check() :: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!   -> If you get errors: cd config && chmod +x serverFtpCmdsGetPrograms && chmod 600 ~/.netrc  <- !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
            sh.close();

            sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
            sh2.waitForFinished(-1);
            output2 = sh2.readAll();
            xmls = output2;
            qDebug()<<output2;
            sh2.close();

            QString mvcmdstr = "ls *.ogg && mv *.ogg "+ProgramsPath;

            qDebug()<<"server_ftp_check() :: Running: "<<mvcmdstr;

            sh3.start("sh", QStringList() << "-c" << mvcmdstr);
            sh3.waitForFinished(-1);
            QString sh3Out = sh3.readAll();
            QStringList filearr = sh3Out.split("\n");

            for(int i=0;i<filearr.length();i++){

                QString fileName = filearr[i];

                qDebug()<<"server_ftp_check() :: Found this one to add:"<<fileName;

                if(!fileName.isEmpty()){

                    QString fileNameWPath = ProgramsPath+"/"+fileName;

                    QStringList splitstr = fileName.split("_");
                    QString nomeDoPrograma = splitstr[0];

                    QString splitstr2 = splitstr[1];
                    QStringList split2 = splitstr2.split(".");
                    QString dataDoPrograma = split2[0];

                    qDebug()<<"server_ftp_check() :: This programs name is: "<<nomeDoPrograma;
                    qDebug()<<"server_ftp_check() :: The programs date is: "<<dataDoPrograma;

                    QSqlQuery qry(db);
                    qry.prepare("INSERT INTO programs VALUES(NULL, ?, ?)");
                    qry.addBindValue(nomeDoPrograma);
                    qry.addBindValue(fileNameWPath);
                    if(qry.exec()){
                        qDebug()<<"server_ftp_check() :: Query OK. Program added to programs table";
                    } else {
                         qDebug()<<"server_ftp_check() :: Query was not ok while atempting to add to the programs table";
                    }

                    QSqlQuery qryid(db);
                    qryid.prepare("SELECT * FROM programs WHERE path LIKE ?");
                    qryid.addBindValue(fileNameWPath);
                    qDebug()<<"server_ftp_check() :: Select id query for path:" << fileNameWPath;
                    if(qryid.exec()){
                        while(qryid.next()){
                            QString pID = qryid.value(0).toString();
                            qDebug()<<"server_ftp_check() :: Query OK. This id is: "<<pID;

                            QStringList dataarr = dataDoPrograma.split("-");
                            QString pAno = dataarr[0];
                            QString pMes = dataarr[1];
                            QString pDia = dataarr[2];

                            if(!pAno.isEmpty() && !pMes.isEmpty() && !pDia.isEmpty()){

                                QSqlQuery qhm(db);
                                qhm.prepare("SELECT hour, min FROM hourprograms WHERE name LIKE ?");
                                qhm.addBindValue(nomeDoPrograma);

                                if(qhm.exec()){

                                    while(qhm.next()){
                                        QString pHora = qhm.value(0).toString();
                                        QString pMin = qhm.value(1).toString();

                                        QSqlQuery addsch(db);
                                        addsch.prepare("INSERT INTO scheduler VALUES (?, ?, ?, ?, ?, ?, '1', NULL, NULL, NULL, NULL, NULL, NULL, NULL, '1')");
                                        addsch.addBindValue(pID);
                                        addsch.addBindValue(pAno);
                                        addsch.addBindValue(pMes);
                                        addsch.addBindValue(pDia);
                                        addsch.addBindValue(pHora);
                                        addsch.addBindValue(pMin);

                                        if(addsch.exec()){
                                            qDebug()<<"server_ftp_check() :: Program scheduled correctly.";
                                            qDebug()<<nomeDoPrograma<< " :: "<<pAno<<"-"<<pMes<<"-"<<pDia<<" at "<<pHora<<":"<<pMin;
                                        } else {
                                            qDebug()<<"server_ftp_check() :: It was not possible to add program to scheduler: "<<addsch.lastError();
                                        }
                                    }



                                } else {

                                    qDebug()<<"server_ftp_check() :: It was not possible to figure out the hour and minute for this program: "<<nomeDoPrograma;
                                    qDebug()<<"server_ftp_check() :: This should be in the 'hourprograms' table.";

                                }


                            } else {

                                qDebug()<<"server_ftp_check() :: We got a program but there was an error adding it beacuse one value of the data is empty. Please check the programs are named like 'name_YYYY-mm-dd.ogg'";

                            }
                        }




                    } else {
                         qDebug()<<"server_ftp_check() :: Query was not ok while atempting to get ID from the programs table"<<qryid.lastError();
                    }

                } else {
                    qDebug()<<"server_ftp_check() :: SERVER: Finished looking for programs on the FTP";
                }

            }


}


void player::rmConfirmTakeOver(){
    QString rmthis = TakeOverPath+"/confirmtakeover.xml";
    QFile::remove(rmthis);
}

void player::returnTakeOver(){

    QFile::remove("/usr/share/xfb/ftp/takeover.xml");

    QString takeOverFile = "/usr/share/xfb/ftp/returntakeover.xml";
    QFile file(takeOverFile);

    if(returntakeOver == false){

        returntakeOver = true;

        file.open(QIODevice::WriteOnly);
        QXmlStreamWriter xmlWriter(&file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("XFBClientTakeOver");
        xmlWriter.writeStartElement("www.netpack.pt");
        QString ip = ui->lbl_ddns->text();

        xmlWriter.writeTextElement("ip",ip);
        xmlWriter.writeTextElement("cmd","returnTakeOver");


        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();
        file.close();








        qDebug()<<"Sending returntakeOver to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutTakeOver.sh).";


        QProcess sh,sh2;
        QByteArray output, output2;
        QString outPath, FTPCmdPath, xmls;


        sh.start("sh", QStringList() << "-c" << "pwd");
        sh.waitForFinished();
        output = sh.readAll();
        outPath = output;
        QStringList path_arry = outPath.split("\n");
        FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutTakeOver.sh | grep 'Transfer complete'";
        qDebug() << "running: " << FTPCmdPath;
        qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsPutTakeOver.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
        sh.close();



        sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
        sh2.waitForFinished(-1);
        output2 = sh2.readAll().trimmed();
        xmls = output2;
        qDebug()<<"The output of serverFtpCmdsPutTakeOver.sh is:\n"<<output2;
        sh2.close();



        if(output2 == "226 Transfer complete."){

            ui->bt_takeOver->setStyleSheet("");
            ui->bt_takeOver->setText(tr("Broadcast LIVE"));

            ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
            ui->txt_ProgramName->hide();

            piscaLive = false;

            QFile::remove("/usr/share/xfb/ftp/takeover.xml");

        }










    } else {
        returntakeOver = false;
        QFile::remove(takeOverFile) ;
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));

        ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
        ui->txt_ProgramName->hide();

        piscaLive = false;

    }



}

void player::stopMplayer(){

    QProcess kb;
    kb.startDetached("killall", QStringList() << "mplayer");
    kb.waitForFinished();

    qDebug()<<"All instances of mplayer were closed";

}


void player::run_scheduler(){

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    checkDbOpen();

    /*check if there is any thing to go on now*/

    qDebug()<<"\nThe scheduller is now monitoring the database (the scheduler table) to check if there is something to go on at "<<QDateTime::currentDateTime().toString()<<"\n";



QSqlQuery sched_qry(db);
sched_qry.prepare("select * from scheduler");
if(sched_qry.exec()){
    while(sched_qry.next()){
        QString tipo = sched_qry.value(6).toString();
        //qDebug() << "Scheduler got a type "<<tipo<<" event rule.";

        /*
         *
         * Var tipo determines the type of shedule
         * 1 = an event to be played only once at a specific minute in time
         *
         * 2 = an event to be played every specific day of the week at a specific hour/min
         *
         * */

        if(tipo=="1"){
            QDateTime now = QDateTime::currentDateTime();

                QString ano1 = now.toString("yyyy");
                QString mes1 = now.toString("M");
                QString dia1 = now.toString("d");
                QString hora1 = now.toString("h");
                QString min1 = now.toString("m");

                QString ano2 = sched_qry.value(1).toString();
                QString mes2 = sched_qry.value(2).toString();
                QString dia2 = sched_qry.value(3).toString();
                QString hora2 = sched_qry.value(4).toString();
                QString min2 = sched_qry.value(5).toString();

                QString is_program = sched_qry.value(14).toString();

                /*
                qDebug()<<"Scheduler is comparing values: ";
                qDebug()<<ano1<<" should be equal to "<<ano2<<" -> ano";
                qDebug()<<mes1<<" should be equal to "<<mes2<<" -> mes";
                qDebug()<<dia1<<" should be equal to "<<dia2<<" -> dia";
                qDebug()<<hora1<<" should be equal to "<<hora2<<" -> hora";
                qDebug()<<min1<<" should be equal to "<<min2<<" -> minuto";
                 */


            //if agora values == este qry values
                if((ano1==ano2) && (mes1==mes2) && (dia1==dia2) && (hora1==hora2) && (min1==min2)){
                    qDebug() << "Scheduled event now fired (type 1)!";

                    QString schId = sched_qry.value(0).toString();

                    //add to playlist
                    QSqlQuery getPath(db);
                    if(is_program=="1"){
                        getPath.prepare("SELECT path FROM programs WHERE id=?");
                    } else {
                        getPath.prepare("SELECT path FROM pub WHERE id=?");
                    }
                    getPath.addBindValue(schId);

                    if(getPath.exec()){
                        while(getPath.next()){
                            QString pubPath = getPath.value(0).toString();
                            ui->playlist->insertItem(0,pubPath);
                            qDebug()<<"Scheduled event added to the top of the playlist: "<<pubPath;
                        }

                    }

                    //delete scheduler row cause its a type 1

                    QSqlQuery del_qry(db);
                    del_qry.prepare("DELETE FROM scheduler WHERE id=? AND ano=? AND mes=? AND dia=? AND hora=? AND min=?");
                    del_qry.addBindValue(schId);
                    del_qry.addBindValue(ano1);
                    del_qry.addBindValue(mes1);
                    del_qry.addBindValue(dia1);
                    del_qry.addBindValue(hora1);
                    del_qry.addBindValue(min1);
                    if(del_qry.exec()){
                        qDebug () << "Scheduled rule was deleted!";
                    } else {
                        qDebug()<<"exeption deleting scheduled rule with qry: "<<del_qry.lastQuery()<<" we got: "<<del_qry.lastError();
                    }

                    //check if pub still has other scheduler rules and delete from pub if not
                    QSqlQuery sq(db);
                    sq.prepare("SELECT count(id) FROM scheduler WHERE id=?");
                    sq.addBindValue(schId);
                    if(sq.exec()){

                        while(sq.next()){
                            qDebug()<<"We have a total of "<<sq.value(0).toString()<<" records in the scheduler table with the schId "<<schId;
                            if(sq.value(0).toString()=="0"){
                                //we can delete it from pub cause no more scheduled rules apply

                                QSqlQuery sd(db);
                                sd.prepare("DELETE FROM pub WHERE id=?");
                                sd.addBindValue(schId);
                                if(sd.exec()){
                                    qDebug () << "Pub rule was deleted!";
                                    update_music_table();
                                } else {
                                    qDebug()<<"exeption deleting pub rule with qry: "<<del_qry.lastQuery()<<" we got: "<<del_qry.lastError();
                                }

                            }
                        }


                    }

                }

        }

        if(tipo=="2"){
            int dia = QDate::currentDate().dayOfWeek();

            QString dw;
            if(dia == 1) dw = "Monday";
            if(dia == 2) dw = "Tuesday";
            if(dia == 3) dw = "Wednesday";
            if(dia == 4) dw = "Thursday";
            if(dia == 5) dw = "Friday";
            if(dia == 6) dw = "Saturday";
            if(dia == 7) dw = "Sunday";

            //qDebug()<<"Today it's "<< dw << "!";


            QString is_program = sched_qry.value(14).toString();



            QDateTime now = QDateTime::currentDateTime();

            QString hora1 = now.toString("h");
            QString min1 = now.toString("m");



            QString dw2 = sched_qry.value(7).toString();
            QString hora2 = sched_qry.value(4).toString();
            QString min2 = sched_qry.value(5).toString();



            //so:
            //
            //dw has today's day of the week
            //hora1 has the current hour
            //min1 has the current minute
            //dw2 has the scheduled day of the week
            //hora2 has the scheduled hour
            //min2 has the scheduled minute
            //
            // /////////////// //

            if((dw==dw2)&&(hora1==hora2)&&(min1==min2)){
                qDebug() << "Scheduled event now fired (type 2)!";
                QString schId = sched_qry.value(0).toString();

                //add to playlist
                QSqlQuery getPath(db);
                if(is_program=="1"){
                    getPath.prepare("SELECT path FROM programs WHERE id=?");
                } else {
                    getPath.prepare("SELECT path FROM pub WHERE id=?");
                }
                getPath.addBindValue(schId);

                if(getPath.exec()){
                    while(getPath.next()){
                        QString pubPath = getPath.value(0).toString();
                        ui->playlist->insertItem(0,pubPath);
                        qDebug()<<"Scheduled event added to the top of the playlist: "<<pubPath;
                    }

                }

            }


        }

    }
}

}







void player::on_actionOptions_triggered()
{
    // Use show() instead of exec() to avoid blocking the event loop
    // (exec() blocks QMediaPlayer signal processing and causes audio to stop updating)
    optionsDialog *opt = new optionsDialog(this);
    opt->setAttribute(Qt::WA_DeleteOnClose);
    opt->setModal(true);
    connect(opt, &QDialog::finished, this, &player::update_music_table);
    // Re-apply the saved settings immediately (tab visibility, seek bar,
    // volume lock, torrents tab, FX tab...) instead of requiring a restart
    connect(opt, &QDialog::finished, this, &player::updateConfig);
    opt->show();
}

void player::on_actionAbout_triggered()
{
    aboutUs ab;
    ab.setModal(true);
    ab.exec();
}

void player::on_actionAdd_a_song_from_Youtube_or_Other_triggered()
{
    externaldownloader* widget = new externaldownloader;
    widget->setAttribute(Qt::WA_DeleteOnClose);

    // Refresh the music table when a song is successfully added.
    connect(widget, &externaldownloader::musicAdded, this, &player::update_music_table);

    widget->show();

    // The external downloader relies on several command-line tools. Offer to
    // install any that are missing, on demand and with the user's consent. The
    // window is shown either way so the user can still fill in details.
    DependencyChecker depChecker;
    // yt-dlp is provisioned as a self-updating binary in ~/.local/bin so it can
    // keep current with YouTube changes via "yt-dlp -U".
    if (!depChecker.ensureYtDlp(widget).isEmpty()) {
        // FFmpeg is needed to extract/convert the audio to the chosen format.
        depChecker.ensureDependency("ffmpeg",
            tr("yt-dlp needs FFmpeg to extract and convert the downloaded audio to a "
               "playable format (mp3/ogg)."),
            widget);
        // A JavaScript runtime is required by yt-dlp for reliable YouTube
        // extraction; without it some formats may be missing. Accept either
        // "node" or "nodejs" (Debian/Ubuntu/Mint historically named it nodejs).
        depChecker.ensureAnyOf({"node", "nodejs"},
            tr("YouTube downloads need a JavaScript runtime (Node.js) so yt-dlp can "
               "reliably extract the available audio formats."),
            widget);
        // ExifTool is used to read the track duration when adding it to the library.
        depChecker.ensureDependency("exiftool",
            tr("ExifTool is used to read the track's duration when adding it to your "
               "music library."),
            widget);
    }
}

void player::on_bt_search_clicked()
{
    //search
    qDebug()<<"Staring a new search!";
    QString term = ui->txt_search->text();

    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery searchQuery(QSqlDatabase::database("xfb_connection"));
    searchQuery.prepare("select * from musics where artist like :t1 or song like :t2");
    searchQuery.bindValue(":t1", "%" + term + "%");
    searchQuery.bindValue(":t2", "%" + term + "%");
    searchQuery.exec();
    model->setQuery(std::move(searchQuery));
    ui->musicView->setModel(model);

    ui->musicView->setSortingEnabled(true);
    ui->musicView->hideColumn(0);
    ui->musicView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

void player::on_bt_reset_clicked()
{
    QSqlQueryModel *modelo = new QSqlQueryModel();
    modelo->setQuery("select * from musics");
    ui->musicView->setModel(modelo);
}

void player::on_bt_apply_filter_clicked()
{
    //filter by genres

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QString addG1 = "";
    QString addG2 = "";

    bool g1_checked = ui->checkBox_filter_genre1->checkState(); //true or false
    bool g2_checked = ui->checkBox_filter_genre2->checkState();

    qDebug () << "132426032015 " << g1_checked << " : " << g2_checked;

    QString selectedGenre1;
    QString selectedGenre2;

    if(g1_checked == true){
        qDebug()<<"g1 is checked";
        selectedGenre1 = ui->cBoxGenre1->currentText();
        addG1 = " genre1=:g1 ";










        QSqlQuery sql(db);
        sql.prepare("select count(*) from musics where genre1 like :genre");
        sql.bindValue(":genre", selectedGenre1);

        if(sql.exec()){
            qDebug()<<"Query ran fine: "<<sql.lastQuery();



            while(sql.next()){

                QString num_of_songs_with_this_genre = sql.value(0).toString();

                qDebug()<<"This genre has "<<num_of_songs_with_this_genre<<" songs";

                QString lbl = "The genre '"+selectedGenre1+"' has "+num_of_songs_with_this_genre+" songs";
                ui->txt_bottom_info->setText(lbl);

            }




        }







    }
    if(g2_checked == true){
        qDebug()<<"g2 is checked";
        selectedGenre2 = ui->cBoxGenre2->currentText();
        if(g1_checked==true){
             qDebug()<<"both are checked...";
             addG2 = "and genre2=:g2 ";
        } else{
            qDebug()<<"Only g2 is checked";
            addG2 = " genre2=:g2 ";
        }

    }
    qDebug()<<"addG1 is "<<addG1<<" and addG2 is "<<addG2;
    if(addG1 != "" || addG2 != ""){
        qDebug() << "making a new table to show with the results...";
        QSqlQueryModel * model = new QSqlQueryModel();
        QSqlQuery filterQuery(db);
        filterQuery.prepare("select * from musics where "+addG1+addG2);
        if(g1_checked) filterQuery.bindValue(":g1", selectedGenre1);
        if(g2_checked) filterQuery.bindValue(":g2", selectedGenre2);
        filterQuery.exec();
        model->setQuery(std::move(filterQuery));
        ui->musicView->setModel(model);
    }




}

void player::on_bt_updateTables_clicked()
{
    ui->statusBar->showMessage(tr("Refreshing tables..."), 2000);
    update_music_table();
    ui->statusBar->showMessage(tr("Tables updated successfully"), 3000);
}
/*
void player::on_bt_youtubeDL_clicked()
{
    externaldownloader* widget = new externaldownloader;
    widget->setAttribute(Qt::WA_DeleteOnClose);
    
    // Connect the musicAdded signal to update the music table
    connect(widget, &externaldownloader::musicAdded, this, &player::update_music_table);
    
    widget->show();
}
*/
void player::on_actionSave_Playlist_triggered()
{
    qDebug()<<"Saving the playlist...";

    QString filename = QFileDialog::getSaveFileName(this,"Save playlist","../playlists/","XML files (*.xml)");

    if(!filename.isEmpty()){
        qDebug()<<"saving "<<filename;

        QStringList farray = filename.split(".");
        if(farray.count()==1){
           filename = filename+".xml";
        }

        QFile file(filename);
        file.open(QIODevice::WriteOnly);

        QXmlStreamWriter xmlWriter(&file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("XFBPlaylist");
        xmlWriter.writeStartElement("www.netpack.pt");


        //loop playlist and save every line into xml
            int numItems = ui->playlist->count();
            for(int i=0;i<numItems;i++){
               //qDebug()<<"i is: "<<i;
               QString txtItem = ui->playlist->item(i)->text();
               qDebug()<<"Xml adding file "<<txtItem;

               // The crossfade overlap and volume line set in the wave view
               // travel with the track as attributes (older XFB versions
               // simply ignore them)
               const qint64 overlapMs =
                   ui->playlist->item(i)->data(PlaylistWaveView::OverlapRole).toLongLong();
               const QString volumeLine = ui->playlist->item(i)
                   ->data(PlaylistWaveView::VolumeEnvelopeRole).toString();
               xmlWriter.writeStartElement("track");
               if (overlapMs > 0)
                   xmlWriter.writeAttribute("overlap", QString::number(overlapMs));
               if (!volumeLine.isEmpty())
                   xmlWriter.writeAttribute("volenv", volumeLine);
               xmlWriter.writeCharacters(txtItem);
               xmlWriter.writeEndElement();
            }

        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();
        file.close();
        QMessageBox::information(this,"Playlist Saved","The playlist was saved!");
    }


}

void player::on_actionClear_Playlist_triggered()
{
    QMessageBox::StandardButton reply;
    reply=QMessageBox::question(this,"Sure?","Are you sure? This will clear all tracks listed in the playlist.",QMessageBox::Yes|QMessageBox::No);
    if(reply==QMessageBox::Yes){
        ui->playlist->clear();
        calculate_playlist_total_time();
    }
}

void player::on_actionLoad_Playlist_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,"Load Playlist","../playlists/",tr("Xml files(*.xml)"));
    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        QMessageBox::information(this,"Bad input","There was an error importing the file.. sorry.");
    }

    QXmlStreamReader Rxml;
    Rxml.setDevice(&file);
    Rxml.readNext();

    while(!Rxml.atEnd()){

        if (Rxml.isStartElement()) {

                    if (Rxml.name() == QStringLiteral("XFBPlaylist")) {
                        qDebug()<<"Valid XFB Playlist Found!";
                        Rxml.readNext();

                        if(Rxml.isEndElement() && Rxml.name() == QLatin1String("XFBPlaylist")){
                            qDebug()<<"Found the last element of the XML file after StarElement, leaving the while loop";
                            Rxml.readNext();
                            break;
                        }

                    } else {
                        Rxml.raiseError(QObject::tr("Not an XFB playlist file"));
                    }
                } else {
                    Rxml.readNext();

                    //qDebug()<<"This Rxml.name() is "<<Rxml.name();

                    if(Rxml.name()==QStringLiteral("www.netpack.pt")){
                        qDebug()<<"Token element: "<<Rxml.name();
                        Rxml.readNext();
                    }

                    if(Rxml.name()==QStringLiteral("track")){
                        // Attributes must be read before readElementText()
                        // Sanity-bound only (10 min): the overlap must survive
                        // loading even when it exceeds the current Max overlap
                        // setting, so the saved crossfade point is not lost
                        const qint64 overlapMs = qBound(qint64(0),
                            qint64(Rxml.attributes().value(QStringLiteral("overlap")).toLongLong()),
                            qint64(600000));
                        const QString volumeLine =
                            Rxml.attributes().value(QStringLiteral("volenv")).toString();
                        QString track = Rxml.readElementText();
                        qDebug()<<"Rxml.readElementText(): "<<track;
                        auto *item = new QListWidgetItem(track);
                        if (overlapMs > 0)
                            item->setData(PlaylistWaveView::OverlapRole, overlapMs);
                        if (!volumeLine.isEmpty()) {
                            // Re-encode through the parser to sanitize the input
                            const QVector<QPointF> env =
                                PlaylistWaveView::parseEnvelope(volumeLine);
                            if (!env.isEmpty())
                                item->setData(PlaylistWaveView::VolumeEnvelopeRole,
                                              PlaylistWaveView::encodeEnvelope(env));
                        }
                        ui->playlist->addItem(item);
                    }



                }
    }
    file.close();
    calculate_playlist_total_time();
    if (m_waveView)
        m_waveView->refresh();
}



void player::on_bt_rec_clicked()
{

 if(recMode == 0){
    recMode = 1;
    ui->bt_rec->hide();
    updateConfig();


   // qDebug()<<"The current recording device is: "<<recDevice;

        saveFile = SavePath+"/XFB.ogg";





        qDebug()<<"Removing old file..";
        QFile::remove(saveFile);
        qDebug()<<"Removed!";



  QTimer::singleShot(5000, this, SLOT(RectimerDone()));
  RecT5();
  QTimer::singleShot(1000, this, SLOT(RecT4()));
  QTimer::singleShot(2000, this, SLOT(RecT3()));
  QTimer::singleShot(3000, this, SLOT(RecT2()));
  QTimer::singleShot(4000, this, SLOT(RecT1()));

  QTimer::singleShot(10000, this, SLOT(RecCHK()));


    } else {
        recMode = 0;
        qDebug()<<"STOP RECORDING!";
        ui->bt_rec->setStyleSheet("");
        ui->bt_pause_rec->setStyleSheet("");
        setRecTimeToDefaults();
        audioRecorder->stop();
        ui->led_rec->hide();
        ui->bt_pause_rec->setEnabled(false);
        recPause = false;
    }

}

void player::RecCHK(){

    QFile myFile(saveFile);

    qDebug()<<"File.size() is now: "<<myFile.size();

    if(myFile.size()==0){
        //red
        ui->led_rec->setStyleSheet("background-color:#FF0010;border-radius:8px;");
    } else {
        //green
         ui->led_rec->setStyleSheet("background-color:#B3FF57;border-radius:8px;");
    }



}

void player::RecT5(){

    //amarilo
    ui->led_rec->setStyleSheet("background-color:#FFFB00;border-radius:8px;");
    ui->led_rec->show();

    if (!saveFile.isEmpty())
        QFile::remove(saveFile);

    ui->txt_recTime->setText("5");
    ui->txt_recTime->show();

}
void player::RecT4(){

    ui->txt_recTime->setText("4");

}
void player::RecT3(){

    ui->txt_recTime->setText("3");

}
void player::RecT2(){

    ui->txt_recTime->setText("2");

}
void player::RecT1(){

    ui->txt_recTime->setText("1");

}

void player::RectimerDone(){

    qDebug()<<"RECORDING!";
    ui->bt_rec->setStyleSheet("background-color: rgb(245, 101, 101)");


    ui->bt_pause_rec->setEnabled(true);


    //QTimer *recTimer = new QTimer(this);
    setRecTimeToDefaults();
    ui->txt_recTime->show();

    recTimer->start(1000);




    qDebug()<<"---> NEW Recording to: "<<saveFile;

    // Find the requested audio input device
    const QList<QAudioDevice> inputDevices = QMediaDevices::audioInputs();
    QAudioDevice selectedDevice;

    for (const QAudioDevice &device : inputDevices) {
        if (device.description() == recDevice) {
            selectedDevice = device;
            break;
        }
    }

    if (selectedDevice.isNull()) {
        // If the specific device wasn't found, use the default device
        selectedDevice = QMediaDevices::defaultAudioInput();
    }

    // Set the audio input device
    audioInput->setDevice(selectedDevice);
    qDebug() << "Selecting this audio input device: " << selectedDevice.description();

    // Configure media recorder settings
    QMediaFormat format;

    // Set container format
    aExtencaoDesteCoiso = "ogg";

    if(contentamento=="matroska") {
        format.setFileFormat(QMediaFormat::FileFormat::Matroska);
        aExtencaoDesteCoiso = "mp3";
    } else if(contentamento=="ogg") {
        format.setFileFormat(QMediaFormat::FileFormat::Ogg);
        aExtencaoDesteCoiso = "ogg";
    } else if(contentamento=="mp4") {
        format.setFileFormat(QMediaFormat::FileFormat::MPEG4);
        aExtencaoDesteCoiso = "mp4";
    } else if(contentamento=="wav") {
        format.setFileFormat(QMediaFormat::FileFormat::Wave);
        aExtencaoDesteCoiso = "wav";
    } else if(contentamento=="quicktime") {
        format.setFileFormat(QMediaFormat::FileFormat::QuickTime);
        aExtencaoDesteCoiso = "mov";
    } else {
        // Default to Ogg if format not recognized
        format.setFileFormat(QMediaFormat::FileFormat::Ogg);
        aExtencaoDesteCoiso = "ogg";
    }

    // Set audio codec (if needed)
    if (!codec.isEmpty()) {
        if (codec == "audio/vorbis") {
            format.setAudioCodec(QMediaFormat::AudioCodec::Vorbis);
        } else if (codec == "audio/opus") {
            format.setAudioCodec(QMediaFormat::AudioCodec::Opus);
        } else if (codec == "audio/mp3") {
            format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
        } else if (codec == "audio/aac") {
            format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
        }
    }

    // Set quality
    audioRecorder->setQuality(QMediaRecorder::HighQuality);
    audioRecorder->setMediaFormat(format);
    audioRecorder->setOutputLocation(QUrl::fromLocalFile(saveFile));

    // Connect signals for recording state changes
    connect(audioRecorder, &QMediaRecorder::recorderStateChanged, this, [](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::RecordingState) {
            qDebug() << "Recording state: Recording";
        } else if (state == QMediaRecorder::PausedState) {
            qDebug() << "Recording state: Paused";
        } else if (state == QMediaRecorder::StoppedState) {
            qDebug() << "Recording state: Stopped";
        }
    });

    connect(audioRecorder, &QMediaRecorder::errorOccurred, this, [this](QMediaRecorder::Error error, const QString &errorString) {
        // Unused parameter 'error'
        Q_UNUSED(error);

        qDebug() << "Recording error: " << errorString;
        QMessageBox::warning(this, tr("Recording Error"), errorString);
    });

    // Start recording
    audioRecorder->record();

    qDebug() << "Recording started with format: " << format.fileFormat();

    ui->bt_rec->show();
}


void player::run_recTimer(){

    QString xsegundos;
    QString xminutos;
    QString xhoras;

    if(recSecs<60){
        recSecs++;
    }

    if(recSecs==60)
    {
        recMins++;
        recSecs-=60;
    }
    if(recMins>59)
    {
        ++recHours;
        recMins-=60;
    }

    if(recSecs<10)
    {
        xsegundos = "0"+QString::number(recSecs);
    } else {
        xsegundos = QString::number(recSecs);
    }
    if(recMins<10)
    {
        xminutos = "0"+QString::number(recMins);
    } else {
        xminutos = QString::number(recMins);
    }
    if(recHours<10)
    {
        xhoras = "0"+QString::number(recHours);
    } else {
        xhoras = QString::number(recHours);
    }
   QString txtElapsedTimeLable =  xhoras+":"+xminutos+":"+xsegundos;
   ui->txt_recTime->setText(txtElapsedTimeLable);




}

void player::setRecTimeToDefaults(){
    recTimer->stop();
    recSecs = 0;
    recMins = 0;
    recHours = 0;
    ui->txt_recTime->setText("");
    ui->txt_recTime->hide();

}


void player::checkForUpdates(){
    if (!m_updateService)
        return;
    m_updateCheckManual = true;
    m_updateService->checkNow(true);
}

void player::notifyUpdateAvailable(const QString &version, const QUrl &releasePage,
                                   const QUrl &downloadUrl)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Update available"));
    box.setIcon(QMessageBox::Information);
    box.setText(tr("XFB %1 is available — you are running %2.")
                    .arg(version, UpdateCheckService::currentVersion()));
    box.setInformativeText(tr("\"Install now\" downloads the update and opens the installer; "
                              "XFB will then close so you can relaunch the new version."));
    QPushButton *installBtn = box.addButton(tr("Install now"), QMessageBox::AcceptRole);
    QPushButton *notesBtn = box.addButton(tr("Release notes"), QMessageBox::ActionRole);
    QPushButton *skipBtn = box.addButton(tr("Skip this version"), QMessageBox::DestructiveRole);
    box.addButton(tr("Later"), QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == notesBtn) {
        QDesktopServices::openUrl(releasePage);
        return;
    }
    if (box.clickedButton() == skipBtn) {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                               + "/xfb.conf", QSettings::IniFormat);
        settings.setValue("SkipUpdateVersion", version);
        return;
    }
    if (box.clickedButton() != installBtn)
        return;

#ifdef Q_OS_MAC
    // Homebrew installs upgrade through brew so the cask stays in sync
    {
        const QString brew = QStandardPaths::findExecutable(
            "brew", {"/opt/homebrew/bin", "/usr/local/bin"});
        if (!brew.isEmpty()) {
            QProcess probe;
            probe.start(brew, {"list", "--cask", "xfb"});
            probe.waitForFinished(8000);
            if (probe.exitStatus() == QProcess::NormalExit && probe.exitCode() == 0) {
                QProgressDialog progress(tr("Upgrading XFB with Homebrew..."), QString(), 0, 0, this);
                progress.setWindowModality(Qt::WindowModal);
                progress.show();

                QProcess upgrade;
                upgrade.start(brew, {"upgrade", "--cask", "xfb"});
                while (upgrade.state() != QProcess::NotRunning) {
                    upgrade.waitForFinished(200);
                    qApp->processEvents();
                }
                progress.close();

                if (upgrade.exitStatus() == QProcess::NormalExit && upgrade.exitCode() == 0) {
                    QMessageBox::information(this, tr("Update installed"),
                        tr("XFB %1 was installed. XFB will now close — "
                           "launch it again to run the new version.").arg(version));
                    close();
                } else {
                    QMessageBox::warning(this, tr("Update failed"),
                        tr("Homebrew could not upgrade XFB:\n%1")
                            .arg(QString::fromLocal8Bit(upgrade.readAllStandardError()).trimmed()));
                }
                return;
            }
        }
    }
#endif

    downloadAndOpenUpdate(downloadUrl, releasePage, version);
}

void player::downloadAndOpenUpdate(const QUrl &downloadUrl, const QUrl &releasePage,
                                   const QString &version)
{
    // No platform artifact attached to the release: hand over to the browser
    if (downloadUrl.isEmpty()) {
        QDesktopServices::openUrl(releasePage);
        return;
    }

    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString fileName = downloadUrl.fileName().isEmpty()
                                 ? QStringLiteral("XFB-%1-update").arg(version)
                                 : downloadUrl.fileName();
    const QString targetPath = downloads + "/" + fileName;

    auto *progress = new QProgressDialog(tr("Downloading XFB %1 ...").arg(version),
                                         tr("Cancel"), 0, 100, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setAutoClose(false);
    progress->show();

    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = networkManager->get(request);

    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::downloadProgress, progress,
            [progress](qint64 received, qint64 total) {
        if (total > 0)
            progress->setValue(static_cast<int>(received * 100 / total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, progress, targetPath, version]() {
        reply->deleteLater();
        progress->deleteLater();
        progress->close();

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() != QNetworkReply::OperationCanceledError) {
                QMessageBox::warning(this, tr("Update failed"),
                    tr("Could not download the update:\n%1").arg(reply->errorString()));
            }
            return;
        }

        QFile out(targetPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("Update failed"),
                tr("Could not save the update to %1").arg(targetPath));
            return;
        }
        out.write(reply->readAll());
        out.close();

        QMessageBox::information(this, tr("Update downloaded"),
            tr("XFB %1 was downloaded to:\n%2\n\nThe installer will open now. "
               "XFB will close — install the update and launch XFB again.")
                .arg(version, targetPath));
        QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
        close();
    });
}

void player::on_actionRecord_a_new_Program_triggered()
{
    bool ok;
    NomeDestePrograma = QInputDialog::getText(0, tr("Program name"),tr("What is the program's name? \n\nFormat MUST be: NAME_YYYY-MM-DD \nEx: program_2016-02-07\n\n"), QLineEdit::Normal,tr("Program_2016-02-07"), &ok);
    if (ok && !NomeDestePrograma.isEmpty()) {
        qDebug()<<"Program name is: "<<NomeDestePrograma;
        ui->txt_ProgramName->setText(NomeDestePrograma);
        ui->txt_ProgramName->show();
        ui->bt_ProgramStopandProcess->show();
    }
}

void player::on_bt_ProgramStopandProcess_clicked()
{
    QMessageBox::StandardButton saveProgram;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    saveProgram = QMessageBox::question(this,tr("Save Program?"),tr("Save this program?"),QMessageBox::Yes|QMessageBox::No);
    if(saveProgram==QMessageBox::Yes){
            qDebug()<<"Saving the program";


            if(saveFile.isEmpty()){
                QMessageBox::information(this,tr("No program set or recorded"),tr("There is no program set or recorded to send"));
            } else {
                qDebug()<<"Processing file: "<<saveFile;

                if(Role=="Client"){

                        ui->txt_uploadingPrograms->show();
                        destinationProgram = ProgramsPath+"/"+NomeDestePrograma+"."+aExtencaoDesteCoiso;
                        qDebug()<<"Copying"<<saveFile<<"to"<<destinationProgram;
                        QFile::remove(destinationProgram);
                        if (!QFile::copy(saveFile, destinationProgram)) {
                            qWarning()<<"Failed to copy program to"<<destinationProgram;
                        }

                        QMessageBox::StandardButton sendToServer;

                        sendToServer = QMessageBox::question(this,tr("Send to server?"),tr("Send programs to the server?"),QMessageBox::Yes|QMessageBox::No);
                        if(sendToServer==QMessageBox::Yes){

                            const QString ftpDestination = FTPPath+"/"+NomeDestePrograma+"."+aExtencaoDesteCoiso;
                            qDebug()<<"Copying"<<saveFile<<"to"<<ftpDestination;
                            QFile::remove(ftpDestination);
                            if (!QFile::copy(saveFile, ftpDestination)) {
                                qWarning()<<"Failed to copy program to"<<ftpDestination;
                            }

                                qDebug()<<"Sending program to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutProgram).";

                                QProcess sh,sh2;
                                QByteArray output, output2;
                                QString outPath, FTPCmdPath, xmls;

                                sh.start("sh", QStringList() << "-c" << "pwd");
                                sh.waitForFinished();
                                output = sh.readAll();
                                outPath = output;
                                QStringList path_arry = outPath.split("\n");
                                FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutProgram.sh | grep 'Transfer complete'";
                                qDebug() << "running: " << FTPCmdPath;
                                qDebug() << "If you get errors: cd scripts && chmod +x serverFtpCmdsPutProgram.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
                                sh.close();

                                sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
                                sh2.waitForFinished(-1);
                                output2 = sh2.readAll();
                                xmls = output2;
                                qDebug()<<output2;
                                sh2.close();

                                qDebug()<<"Program upload finished!";

                                QString fileToRemove = FTPPath+"/"+NomeDestePrograma+".ogg";
                                if (QFile::remove(fileToRemove)) {
                                    qDebug()<<"FTP temp file deleted:" << fileToRemove;
                                } else {
                                    qWarning()<<"Failed to delete FTP temp file:" << fileToRemove;
                                }

                                QMessageBox::StandardButton answer;
                                answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program?"), QMessageBox::Yes|QMessageBox::No);

                                if(answer==QMessageBox::Yes){

                                    if (QFile::remove(destinationProgram)) {
                                        qDebug()<<"Local program file deleted:" << destinationProgram;
                                    } else {
                                        qWarning()<<"Failed to delete local program file:" << destinationProgram;
                                    }
                                    QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
                                    ui->txt_ProgramName->hide();
                                    ui->bt_ProgramStopandProcess->hide();

                                } else {

                                            QSqlQuery qry(db);
                                            qry.prepare("INSERT INTO programs VALUES(NULL, ?, ?)");
                                            qry.addBindValue(NomeDestePrograma);
                                            qry.addBindValue(destinationProgram);
                                            if(qry.exec()){
                                                qDebug()<<"Query OK. Program localy added to programs table";
                                            } else {
                                                 qDebug()<<"Query was not ok while atempting to localy add to the programs table";
                                            }

                                    ui->txt_ProgramName->hide();
                                    ui->bt_ProgramStopandProcess->hide();

                                }


                            ui->txt_uploadingPrograms->hide();


                        } else {
                            ui->txt_ProgramName->hide();
                            ui->bt_ProgramStopandProcess->hide();

                            ui->txt_uploadingPrograms->hide();
                        }


                } else {
                    QMessageBox::information(this,tr("Cannot Send"),tr("It's not possible to send the Program to the server if XFB role is set to Server"));
                }


            }



    } else {
        qDebug()<<"Sending program to server was cancelled";
        ui->txt_ProgramName->hide();
        ui->bt_ProgramStopandProcess->hide();
    }
}

void player::on_actionAdd_a_program_triggered()
{
    add_program addp;
    addp.setModal(true);
    addp.exec();
    update_music_table();
}





void player::on_actionFullScreen_triggered()
{
    if(tmpFullScreen==0){
        QMainWindow::showFullScreen();
        tmpFullScreen=1;
    } else{
        QMainWindow::showMaximized();
        tmpFullScreen=0;
    }
}


void player::pingServer()
{
    QMessageBox::information(this,tr("Ping Server"),tr("Option not coded. Can you buy me a coffee? (Paypal: fred@netpack.pt ;))"));
}

void player::on_actionForce_an_FTP_Check_triggered()
{
    server_ftp_check();
}

void player::on_actionMake_a_program_from_this_playlist_triggered()
{

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    ui->txt_creatingPrograms->show();
      qDebug()<<"Running Make_a_program_with_the_current_playlist";


      QDir programGenDir("../ProgramGenerator");
      const QStringList staleOggs = programGenDir.entryList(QStringList() << "*.ogg", QDir::Files);
      for (const QString &stale : staleOggs)
          programGenDir.remove(stale);
       qDebug()<<"temp files from ProgramGenerator folder removed";

      QFile::remove("inputs.txt");
       qDebug()<<"inputs.txt from bin forlder removed";



      bool ok;
      NomeDestePrograma = QInputDialog::getText(0, tr("Program name"),tr("What is the program's name? \n\nFormat MUST be: NAME_YYYY-MM-DD \nEx: program_2016-02-07\n\n"), QLineEdit::Normal,tr("Program_2016-02-07"), &ok);
      if (ok && !NomeDestePrograma.isEmpty()) {
          qDebug()<<"Program name is: "<<NomeDestePrograma;
      }

      //get the curent items of the playlist
      QString contatstr;
      QString destino = ProgramsPath+"/"+NomeDestePrograma+".ogg";
      int numItems = ui->playlist->count();
      for(int i=0;i<numItems;i++){
          QString nome = "music"+QString::number(i)+".ogg";
          qDebug()<<"NOME: "<<nome;
          qDebug()<<"Processing "<<i;
          QString txtItem = ui->playlist->item(i)->text();
          qDebug()<<"Processing file "<<txtItem;



          QStringList array_ext_item = txtItem.split(".");
          qDebug()<<" ------------>>>>>>>>>>>>>>>>>>>> EXT :::::::::::::: "<<array_ext_item.last();
          if(array_ext_item.last()!="ogg"){
              QMessageBox::warning(this,tr("File not in ogg..."),"The file "+txtItem+" is not in OGG and at this moment we can only concatenate the files if they are.. please convert this file (or all?) to ogg for now... sorry :-( ");
              ui->txt_creatingPrograms->hide();
              return;

          } else {
              contatstr = "../ProgramGenerator/"+nome;
              qDebug()<<">> >> >> >> >> Adding audio:"<<txtItem<<"->"<<contatstr;
              QFile::remove(contatstr);
              if (!QFile::copy(txtItem, contatstr)) {
                  qWarning()<<"Failed to stage file for program:"<<txtItem;
              }

              // ffmpeg concat list entry; embedded single quotes are escaped
              // per the concat demuxer's quoting rules
              QFile inputsFile("inputs.txt");
              if (inputsFile.open(QIODevice::Append | QIODevice::Text)) {
                  QString escaped = contatstr;
                  escaped.replace("'", "'\\''");
                  QTextStream(&inputsFile) << "file '" << escaped << "'\n";
              } else {
                  qWarning()<<"Could not append to inputs.txt";
              }

          }

      }
      qDebug()<<">> >> >> >> >> Concatenating audio into:"<<destino;
      QString concatFfmpeg = FxEngine::ffmpegExecutable();
      if (concatFfmpeg.isEmpty())
          concatFfmpeg = "ffmpeg";
      QProcess cmd;
      cmd.start(concatFfmpeg, QStringList()
                << "-y" << "-f" << "concat" << "-safe" << "0"
                << "-i" << "inputs.txt" << destino);
      cmd.waitForFinished(300000); //600000 10 minutos



      cmd.close();
      ui->txt_creatingPrograms->hide();
      qDebug()<<">> >> >> >> >> Removing tmp audio";
      const QStringList stagedOggs = programGenDir.entryList(QStringList() << "*.ogg", QDir::Files);
      for (const QString &staged : stagedOggs)
          programGenDir.remove(staged);
      ui->txt_uploadingPrograms->show();




      QMessageBox::StandardButton sendToServer;
      sendToServer = QMessageBox::question(this,tr("Send to server?"),tr("Send programs to the server?"),QMessageBox::Yes|QMessageBox::No);
      if(sendToServer==QMessageBox::Yes){
          const QString ftpProgramCopy = FTPPath+"/"+NomeDestePrograma+".ogg";
          qDebug()<<"Copying"<<destino<<"to"<<ftpProgramCopy;
          QFile::remove(ftpProgramCopy);
          if (!QFile::copy(destino, ftpProgramCopy)) {
              qWarning()<<"Failed to copy program to"<<ftpProgramCopy;
          }

              qDebug()<<"Sending program to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutProgram).";
              QProcess sh,sh2;

              QByteArray output, output2;
              QString outPath, FTPCmdPath, xmls;
              sh.start("sh", QStringList() << "-c" << "pwd");
              sh.waitForFinished();
              output = sh.readAll();
              outPath = output;
              QStringList path_arry = outPath.split("\n");
              FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutProgram.sh | grep 'Transfer complete'";
              qDebug() << "running: " << FTPCmdPath;
              qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsPutProgram.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
              sh.close();
              sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
              sh2.waitForFinished(-1);
              output2 = sh2.readAll();
              xmls = output2;
              qDebug()<<output2;
              sh2.close();
              qDebug()<<"Checking if the file's intergrity was perserved...";
              QProcess shCHK,shCHK2;
              QByteArray outputCHK, outputCHK2;
              QString outPathCHK, FTPCmdPathCHK, xmlsCHK;
              shCHK.start("sh", QStringList() << "-c" << "pwd");
              shCHK.waitForFinished();
              outputCHK = shCHK.readAll();
              outPathCHK = outputCHK;
              QStringList path_arryCHK = outPathCHK.split("\n");
              FTPCmdPathCHK = path_arryCHK[0]+"/usr/share/xfb/scripts/serverFtpCmdsCHKProgram | grep "+NomeDestePrograma;
              qDebug() << "running: " << FTPCmdPathCHK;
              qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsCHKProgram && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
              shCHK.close();
              shCHK2.start("sh", QStringList() << "-c" << FTPCmdPathCHK);
              shCHK2.waitForFinished();
              outputCHK2 = shCHK2.readAll();
              xmlsCHK = outputCHK2;
              qDebug()<<outputCHK2;
              shCHK2.close();
              if(!xmlsCHK.isEmpty()){
                  QStringList splitCHKout = xmlsCHK.split(" ");
                  for(int i=0;i<splitCHKout.count();i++){
                      qDebug()<<"In position "<<i<<" of the array the value is: "<<splitCHKout[i];
                  }
                  QString ftpSizeStr = splitCHKout[9];

                  for(int i=10;i<99;i++){
                      if(ftpSizeStr.isEmpty() || ftpSizeStr == "0")
                          ftpSizeStr = splitCHKout[i];
                  }

                  qDebug()<<"Size value in array: "<<ftpSizeStr;
                  int size = 0;
                  QString mmfile = FTPPath+"/"+NomeDestePrograma+".ogg";
                  QFile myFile(mmfile);
                  if (myFile.open(QIODevice::ReadOnly)){
                      size = myFile.size();  //when file does open.
                      QString sizeStr = QString::number(size);
                      qDebug()<<"Size value of local file: "<<sizeStr;
                      myFile.close();
                      if(ftpSizeStr==sizeStr){
                          qDebug()<<"The file's integrity on the FTP server was verified correctly!";
                      } else {
                          qDebug()<<"Failed to verify the integrity of the file in the FTP server. Size of Local and remote files do NOT match...";
                          QMessageBox::information(this,tr("Interity verification faild!"),tr("The file does not seam to have been sent to the server correctly since the size of the local file differs from the one on the FTP server. Please try to send the program again."));
                      }
                  } else {
                      qDebug()<<"It was not possible to get the size of the local file: "<<mmfile;
                  }
              }
              qDebug()<<"Program uploaded to server!";
              QString fileToRemove = FTPPath+"/"+NomeDestePrograma+".ogg";
              if (QFile::remove(fileToRemove)) {
                  qDebug()<<"FTP temp file deleted:" << fileToRemove;
              } else {
                  qWarning()<<"Failed to delete FTP temp file:" << fileToRemove;
              }
              QMessageBox::StandardButton answer;
              answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program? (The program was sucessfuly uploaded to the server)"), QMessageBox::Yes|QMessageBox::No);
              if(answer==QMessageBox::Yes){
                  if (QFile::remove(destino)) {
                      qDebug()<<"Local file deleted:" << destino;
                  } else {
                      qWarning()<<"Failed to delete local file:" << destino;
                  }
                  QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
              } else {
                          QSqlQuery qry(db);
                          qry.prepare("INSERT INTO programs VALUES(NULL, ?, ?)");
                          qry.addBindValue(NomeDestePrograma);
                          qry.addBindValue(destino);
                          if(qry.exec()){
                              qDebug()<<"Query OK. Program localy added to programs table";
                          } else {
                               qDebug()<<"Query was not ok while atempting to localy add to the programs table";
                          }
              }
      } else {
          QMessageBox::StandardButton answer;
          answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program?"), QMessageBox::Yes|QMessageBox::No);
          if(answer==QMessageBox::Yes){
              if (QFile::remove(destino)) {
                  qDebug()<<"Local file deleted:" << destino;
              } else {
                  qWarning()<<"Failed to delete local file:" << destino;
              }
              QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
          } else {
              QSqlQuery qry(db);
              qry.prepare("INSERT INTO programs VALUES(NULL, ?, ?)");
              qry.addBindValue(NomeDestePrograma);
              qry.addBindValue(destino);
              if(qry.exec()){
                  qDebug()<<"Query OK. Program localy added to programs table";
              } else {
                   qDebug()<<"Query was not ok while atempting to localy add to the programs table";
              }
  }
      }
  ui->txt_uploadingPrograms->hide();
}

void player::on_actionCheck_the_Database_records_triggered()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    QMessageBox::StandardButton run = QMessageBox::question(this, "Run Database Check?",
                                                            "Run a check on all music records?\n"
                                                            "- Verify file existence\n"
                                                            "- Check for empty files\n"
                                                            "- Update duration from file metadata\n"
                                                            "- Reset play count (if file OK)\n\n"
                                                            "This may take some time.",
                                                            QMessageBox::Yes | QMessageBox::No);
    if (run == QMessageBox::No) {
        return;
    }

    // Use stack-allocated QSqlQuery objects within the loop or prepare outside
    QSqlQuery querySelect(db);
    QSqlQuery queryDelete(db); // For delete operations
    QSqlQuery queryUpdate(db); // For update operations

    // Prepare statements outside the loop for efficiency
    if (!queryDelete.prepare("DELETE FROM musics WHERE path = :path")) {
         qWarning() << "Failed to prepare DELETE statement:" << queryDelete.lastError();
         QMessageBox::critical(this, "Database Error", "Failed to prepare database query (DELETE).");
         return;
    }
    if (!queryUpdate.prepare("UPDATE musics SET time = :time WHERE path = :path")) {
        qWarning() << "Failed to prepare time UPDATE statement:" << queryUpdate.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query (UPDATE time).");
        return;
    }
    // Separate prepare for played_times update
    QSqlQuery queryUpdatePlayedTimes(db);
     if (!queryUpdatePlayedTimes.prepare("UPDATE musics SET played_times = 0 WHERE path = :path")) {
        qWarning() << "Failed to prepare played_times UPDATE statement:" << queryUpdatePlayedTimes.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query (UPDATE played_times).");
        return;
    }


    QString selectStr = "SELECT path, time FROM musics"; // Select only needed columns
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table.");
        return;
    }

    int processedCount = 0;
    int errorsCount = 0;
    int deletedCount = 0;
    int updatedCount = 0;

    while (querySelect.next()) {
        QString path = querySelect.value(0).toString();
        QString currentDbTime = querySelect.value(1).toString(); // Get current time for comparison later if needed

        qInfo().noquote() << "Processing [" << ++processedCount << "]:" << path; // Use qInfo for less verbose default logging

        if (!QFile::exists(path)) {
            qWarning() << "File does not exist:" << path;
            QMessageBox::StandardButton reply = QMessageBox::question(this, "File Missing",
                                                                      QString("File not found:\n%1\n\nDelete this record from the database?")
                                                                          .arg(path),
                                                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (reply == QMessageBox::Cancel) {
                qInfo() << "User cancelled operation.";
                break; // Exit the loop
            } else if (reply == QMessageBox::Yes) {
                queryDelete.bindValue(":path", path);
                if (queryDelete.exec()) {
                    qInfo() << "Record deleted from database (file missing):" << path;
                    deletedCount++;
                    update_music_table(); // Update view immediately
                } else {
                    qWarning() << "Failed to delete record for missing file:" << queryDelete.lastError() << queryDelete.lastQuery();
                    errorsCount++;
                     QMessageBox::warning(this, "Database Error", QString("Failed to delete record for missing file:\n%1\n\nError: %2").arg(path, queryDelete.lastError().text()));
                }
            } else {
                qInfo() << "Skipping missing file record, keeping in DB:" << path;
                errorsCount++; // Count as an issue to resolve
            }
            continue; // Move to the next record
        }

        // File exists, check size
        QFile file(path);
        qint64 fileSize = file.size(); // Check size directly
        qDebug() << "File exists, size:" << fileSize;

        if (fileSize == 0) {
            qWarning() << "File exists but is empty (0 bytes):" << path;
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Empty File",
                                                                      QString("File exists but is empty (0 bytes):\n%1\n\nDelete record from database AND delete the empty file from disk?")
                                                                          .arg(path),
                                                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
             if (reply == QMessageBox::Cancel) {
                qInfo() << "User cancelled operation.";
                break; // Exit the loop
            } else if (reply == QMessageBox::Yes) {
                // 1. Delete from DB first
                queryDelete.bindValue(":path", path);
                if (queryDelete.exec()) {
                    qInfo() << "Record deleted from database (empty file):" << path;
                    deletedCount++;
                    // 2. If DB delete successful, delete file from disk
                    if (QFile::remove(path)) {
                        qInfo() << "Empty file deleted from disk:" << path;
                    } else {
                        qWarning() << "Failed to delete empty file from disk (record was deleted from DB):" << path << file.errorString();
                         QMessageBox::warning(this, "File System Error", QString("Record deleted from DB, but failed to delete empty file from disk:\n%1\n\nError: %2").arg(path, file.errorString()));
                        // Don't increment errorsCount here as the primary goal (DB cleanup) succeeded.
                    }
                    update_music_table(); // Update view immediately
                } else {
                    qWarning() << "Failed to delete record for empty file:" << queryDelete.lastError() << queryDelete.lastQuery();
                    errorsCount++;
                    QMessageBox::warning(this, "Database Error", QString("Failed to delete record for empty file:\n%1\n\nError: %2").arg(path, queryDelete.lastError().text()));
                }
            } else {
                qInfo() << "Skipping empty file record, keeping in DB and on disk:" << path;
                errorsCount++; // Count as an issue to resolve
            }
            continue; // Move to the next record
        }

        // --- File seems OK, get duration using Exiftool (C++ Parsing Method) ---
        qDebug() << "File seems OK. Checking metadata with exiftool...";

        QProcess process;
        QString command = "exiftool"; // Just the command
        QStringList arguments;
        arguments << "-Duration"; // Ask only for the Duration tag for efficiency
        arguments << path;        // QProcess handles quoting arguments

        qDebug() << "Running command:" << command << arguments;

        process.start(command, arguments);

        // Wait for the process to finish (e.g., 10 seconds timeout)
        if (!process.waitForFinished(10000)) {
            qWarning() << "Exiftool process timed out for:" << path << process.errorString();
            process.kill();
            process.waitForFinished(1000); // Wait a bit after killing
            errorsCount++;
            continue; // Skip this file
        }

        // Check for exiftool execution errors
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            qWarning() << "Exiftool process failed for:" << path
                       << "Exit code:" << process.exitCode()
                       << "Exit status:" << process.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(process.readAllStandardError()); // Or fromUtf8
            qWarning() << "Exiftool Standard Error:\n" << errorOutput;
            errorsCount++;
             // Optionally show a non-blocking notification or log this error prominently
            continue; // Skip this file
        }

        // Read ALL standard output from exiftool
        QString exiftoolOutput = QString::fromLocal8Bit(process.readAllStandardOutput()); // Or fromUtf8
        qDebug() << "Exiftool raw output:\n" << exiftoolOutput;

        QString durationValue;
        // Find the line containing "Duration" and extract the value
        QStringList lines = exiftoolOutput.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.simplified().startsWith("Duration", Qt::CaseInsensitive)) {
                int colonPos = line.indexOf(':');
                if (colonPos != -1) {
                    durationValue = line.mid(colonPos + 1).trimmed();
                    qInfo() << "Found Duration:" << durationValue << "for" << path;
                    break; // Found it
                }
            }
        }

        if (durationValue.isEmpty()) {
            qWarning() << "Could not find or parse 'Duration' tag from exiftool output for:" << path;
             QString errorOutput = QString::fromLocal8Bit(process.readAllStandardError()); // Check stderr again
             if (!errorOutput.trimmed().isEmpty()) {
                 qWarning() << "Exiftool stderr for non-duration file:" << errorOutput;
             }
            errorsCount++;
            // Maybe update time to NULL or a special value? For now, just skip update.
        } else {
            // Update the database only if the time is different or needs setting
            if (currentDbTime != durationValue) { // Optional: Avoid unnecessary updates
                 qInfo() << "Updating time in DB from" << currentDbTime << "to" << durationValue << "for" << path;
                 queryUpdate.bindValue(":time", durationValue);
                 queryUpdate.bindValue(":path", path);
                 if (queryUpdate.exec()) {
                     qDebug() << "Time updated successfully in DB.";
                     updatedCount++;
                     update_music_table(); // Update view
                 } else {
                     qWarning() << "Failed to update time in DB:" << queryUpdate.lastError() << queryUpdate.lastQuery();
                     errorsCount++;
                     QMessageBox::warning(this, "Database Error", QString("Failed to update time for:\n%1\n\nError: %2").arg(path, queryUpdate.lastError().text()));
                 }
            } else {
                 qDebug() << "Duration in DB already matches exiftool output. No time update needed.";
            }

             // Reset played_times (original logic) - Do this only if time update was successful or not needed?
             // Let's assume reset should happen if the file is valid, regardless of time update necessity.
            qInfo() << "Resetting played_times to 0 for:" << path;
            queryUpdatePlayedTimes.bindValue(":path", path);
             if (!queryUpdatePlayedTimes.exec()) {
                 qWarning() << "Failed to reset played_times:" << queryUpdatePlayedTimes.lastError() << queryUpdatePlayedTimes.lastQuery();
                 errorsCount++;
                 // Non-critical error, maybe don't show a message box here.
             }
        }
        // --- End Exiftool Processing ---

    } // End while loop

    qInfo() << "-------------------------------------";
    qInfo() << "Database check complete.";
    qInfo() << "Processed:" << processedCount << "records.";
    qInfo() << "Deleted:" << deletedCount << "records.";
    qInfo() << "Updated:" << updatedCount << "times.";
    qInfo() << "Errors/Skipped:" << errorsCount << "records (check warnings above).";
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Check Complete",
                             QString("Database check finished.\n\nProcessed: %1\nDeleted: %2\nTime Updated: %3\nErrors/Skipped: %4\n\nSee application output log for details.")
                                 .arg(processedCount)
                                 .arg(deletedCount)
                                 .arg(updatedCount)
                                 .arg(errorsCount));

    // Final table update might be redundant if called after every change, but safe to have.
    update_music_table();
}

void player::calculate_playlist_total_time() {
    qint64 totalSeconds = 0; // Use qint64 for total seconds to prevent overflow
    int failedFiles = 0;
    int playlistCount = ui->playlist->count();

    qInfo() << "Calculating total time for" << playlistCount << "items in playlist...";

    if (playlistCount == 0) {
        ui->txt_playlistTotalTime->setText("Total time: 00:00:00");
        return;
    }

    // Resolve a duration-reading tool. Prefer ffprobe (bundled with XFB on
    // Windows, ships with ffmpeg elsewhere); fall back to exiftool. GUI apps
    // don't always inherit the shell PATH, so also probe common locations and
    // the application directory (where the bundled ffprobe.exe lives).
    auto resolveTool = [](const QString &name) -> QString {
        QString p = QStandardPaths::findExecutable(name);
        if (!p.isEmpty()) return p;
        QStringList dirs;
#if defined(Q_OS_WIN)
        dirs << QCoreApplication::applicationDirPath();
#elif defined(Q_OS_MACOS)
        dirs << "/opt/homebrew/bin" << "/usr/local/bin" << "/opt/local/bin"
             << (QDir::homePath() + "/.local/bin") << "/usr/bin"
             << QCoreApplication::applicationDirPath();
#else
        dirs << "/usr/local/bin" << "/usr/bin" << (QDir::homePath() + "/.local/bin")
             << QCoreApplication::applicationDirPath();
#endif
        for (const QString &d : dirs) {
            QString cand = d + "/" + name;
#ifdef Q_OS_WIN
            if (!cand.endsWith(".exe", Qt::CaseInsensitive)) cand += ".exe";
#endif
            if (QFile::exists(cand)) return cand;
        }
        return QString();
    };
    const QString ffprobePath = resolveTool("ffprobe");
    const QString exiftoolPath = ffprobePath.isEmpty() ? resolveTool("exiftool") : QString();
    if (ffprobePath.isEmpty() && exiftoolPath.isEmpty()) {
        qWarning() << "Neither ffprobe nor exiftool found; cannot compute playlist duration.";
    }

    // Parse a duration string ("HH:MM:SS[.ss]", "SS[.ss]", "N/A", "-") into
    // whole seconds, or -1 if it isn't a usable duration.
    auto parseDurationSeconds = [](QString s) -> qint64 {
        s = s.trimmed();
        const int paren = s.indexOf('(');
        if (paren > 0) s = s.left(paren).trimmed();
        if (s.isEmpty() || s == "-" || s.compare("N/A", Qt::CaseInsensitive) == 0)
            return -1;
        if (s.contains(':')) {
            const QStringList parts = s.split(':');
            if (parts.size() == 3) {
                bool hOk, mOk, sOk;
                const int h = parts[0].toInt(&hOk);
                const int m = parts[1].toInt(&mOk);
                const double sec = parts[2].toDouble(&sOk);
                if (hOk && mOk && sOk && h >= 0 && m >= 0 && sec >= 0)
                    return qint64(h) * 3600 + qint64(m) * 60 + static_cast<qint64>(sec);
            }
            return -1;
        }
        bool ok;
        const double d = s.toDouble(&ok);
        return (ok && d >= 0) ? static_cast<qint64>(d) : -1;
    };

    // Fall back to the duration XFB stored in the database when the media tool
    // can't read a file (offline file, unusual container, missing metadata...).
    QSqlDatabase timeDb = QSqlDatabase::database("xfb_connection");
    auto dbDurationSeconds = [&](const QString &path) -> qint64 {
        if (!timeDb.isValid() || !timeDb.isOpen()) return -1;
        QSqlQuery q(timeDb);
        q.prepare("SELECT time FROM musics WHERE path = :p");
        q.bindValue(":p", path);
        if (q.exec() && q.next())
            return parseDurationSeconds(q.value(0).toString());
        return -1;
    };

    for (int i = 0; i < playlistCount; ++i) {
        QListWidgetItem* item = ui->playlist->item(i);
        if (!item) continue; // Should not happen, but safety check

        QString filePath = item->text(); // Assuming the item text is the full path
        qInfo().noquote() << "Processing playlist item [" << i+1 << "/" << playlistCount << "]:" << filePath;

        qint64 trackSeconds = -1;

        // 1) Use the duration XFB already stored in the library database. This is
        //    a fast, indexed lookup and covers the overwhelmingly common case:
        //    tracks dragged in from the library were probed once at import time
        //    and their duration is cached in "musics.time". Doing this first
        //    avoids spawning ffprobe per item — which, over a network share,
        //    blocked the UI thread for 10-15s while the playlist appeared to
        //    hang after a drop.
        trackSeconds = dbDurationSeconds(filePath);

        // 2) Only when the database has no usable duration (files not in the
        //    library, or imported before durations were stored) do we fall back
        //    to the media tool (ffprobe preferred, exiftool fallback), and only
        //    if the file is actually present on disk.
        if (trackSeconds < 0 &&
            (!ffprobePath.isEmpty() || !exiftoolPath.isEmpty()) && QFile::exists(filePath)) {
            QProcess process;
            QString command;
            QStringList arguments;
            if (!ffprobePath.isEmpty()) {
                // ffprobe prints the duration in seconds (e.g. "215.093333").
                command = ffprobePath;
                arguments << "-v" << "error"
                          << "-show_entries" << "format=duration"
                          << "-of" << "default=noprint_wrappers=1:nokey=1"
                          << filePath;
            } else {
                // exiftool with -T prints just the Duration value (HH:MM:SS[.ss]).
                command = exiftoolPath;
                arguments << "-T" << "-Duration" << filePath;
            }

            // Read stdout only so any tool warnings on stderr can't corrupt the
            // parsed value.
            process.start(command, arguments);
            if (process.waitForStarted(5000) && process.waitForFinished(15000) &&
                process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
                const QString out = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
                trackSeconds = parseDurationSeconds(out);
            } else {
                process.kill();
                process.waitForFinished(1000);
            }
        }

        if (trackSeconds >= 0) {
            totalSeconds += trackSeconds;
        } else {
            qWarning() << "Could not determine duration for:" << filePath;
            failedFiles++;
        }

    } // End for loop

    // --- Format total time ---
    qint64 finalHours = totalSeconds / 3600;
    qint64 finalMinutes = (totalSeconds % 3600) / 60;
    qint64 finalSeconds = totalSeconds % 60;

    // Format as HH:MM:SS with leading zeros
    QString finalTimeString = QStringLiteral("Total time: %1:%2:%3")
                                  .arg(finalHours, 2, 10, QChar('0'))   // At least 2 digits, base 10, pad with '0'
                                  .arg(finalMinutes, 2, 10, QChar('0'))
                                  .arg(finalSeconds, 2, 10, QChar('0'));

    if (failedFiles > 0) {
        finalTimeString += QString(" (%1 item(s) failed)").arg(failedFiles);
         qWarning() << "Finished calculating playlist time, but failed to get duration for" << failedFiles << "items.";
    }

    qInfo() << "Final calculated playlist time string:" << finalTimeString;
    ui->txt_playlistTotalTime->setText(finalTimeString);
}
void player::on_actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation_triggered()
{


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: Run a check on all the records in the music table; For each check if it exist AND DELETE WITHOUT ASKING FOR CONFIRMATION and update the time info of the song",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){



        QSqlQuery qr(db);
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"Processing: "<<path;

                bool ha = QFile::exists(path);
                if(!ha){


                    qDebug()<<"Deleting "<<path;

                          QSqlQuery* qry=new QSqlQuery(db);
                          qry->prepare("delete from musics where path = :thpath");
                          qry->bindValue(":thpath",path);

                         if(qry->exec()){
                              qDebug() << "Music Deleted from database! last query was:"<< qry->lastQuery();
                              update_music_table();
                         } else {
                             qDebug() << "There was an error deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                         }

                } else {

                    QFile myFile(path);

                    qDebug()<<"File.size() is now: "<<myFile.size();

                    if(myFile.size()==0){

                              QSqlQuery* qry=new QSqlQuery(db);
                              qry->prepare("delete from musics where path = :thpath");
                              qry->bindValue(":thpath",path);

                             if(qry->exec()){
                                  qDebug() << "Music Deleted from database and HD! last query was:"<< qry->lastQuery();

                                  if (QFile::remove(path)) {
                                      qDebug() << "File deleted:" << path;
                                  } else {
                                      qWarning() << "Failed to delete file:" << path;
                                  }

                                  update_music_table();
                             } else {
                                 qDebug() << "There was an erropositionr deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                             }




                    }




                }


            }
        }



    }




}

void player::on_bt_rol_streaming_play_clicked()
{
    QString urlText = ui->txt_rol_stream_url->text().trimmed();
    if (urlText.isEmpty()) {
        ui->lbl_rol_streaming_status->setText(tr("Enter a stream URL first (e.g. http://server:8000/stream)"));
        return;
    }

    // Convenience: allow "server:8000/stream" without a scheme
    if (!urlText.contains(QStringLiteral("://")))
        urlText.prepend(QStringLiteral("http://"));

    const QUrl streamUrl = QUrl::fromUserInput(urlText);
    if (!streamUrl.isValid() || streamUrl.host().isEmpty()
        || (streamUrl.scheme() != "http" && streamUrl.scheme() != "https")) {
        ui->lbl_rol_streaming_status->setText(tr("Invalid stream URL: %1").arg(urlText));
        return;
    }

    // Remember the URL for the next session
    {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                               + "/xfb.conf", QSettings::IniFormat);
        settings.setValue("StreamClientURL", urlText);
    }

    // Playlist URLs (.m3u/.pls) are resolved to their first stream entry;
    // everything else (direct Icecast/Shoutcast mounts, HLS .m3u8) plays
    // natively through QMediaPlayer.
    const QString path = streamUrl.path().toLower();
    if ((path.endsWith(QStringLiteral(".m3u")) || path.endsWith(QStringLiteral(".pls")))
        && !path.endsWith(QStringLiteral(".m3u8"))) {
        resolveAndPlayStreamPlaylist(streamUrl);
    } else {
        startRadioStream(streamUrl);
    }
}

void player::startRadioStream(const QUrl &streamUrl)
{
    qInfo() << "Streaming client: playing" << streamUrl.toDisplayString();
    ui->lbl_rol_streaming_status->setText(tr("Connecting to %1 ...").arg(streamUrl.toDisplayString()));
    RadioPlayer->stop();
    RadioPlayer->setSource(streamUrl);
    RadioPlayer->play();
}

void player::resolveAndPlayStreamPlaylist(const QUrl &playlistUrl)
{
    ui->lbl_rol_streaming_status->setText(tr("Fetching playlist %1 ...").arg(playlistUrl.toDisplayString()));

    QNetworkRequest request(playlistUrl);
    request.setTransferTimeout(10000);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, playlistUrl]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            ui->lbl_rol_streaming_status->setText(tr("Could not fetch playlist: %1").arg(reply->errorString()));
            return;
        }

        // Cap what we parse: a stream playlist is tiny, anything huge is bogus
        const QString body = QString::fromUtf8(reply->read(64 * 1024));
        QUrl firstEntry;

        for (const QString &rawLine : body.split('\n')) {
            const QString line = rawLine.trimmed();
            if (line.isEmpty())
                continue;
            if (playlistUrl.path().toLower().endsWith(QStringLiteral(".pls"))) {
                // PLS format: File1=http://...
                if (line.startsWith(QStringLiteral("File"), Qt::CaseInsensitive) && line.contains('=')) {
                    firstEntry = QUrl(line.section('=', 1).trimmed());
                    break;
                }
            } else {
                // M3U format: first non-comment line
                if (!line.startsWith('#')) {
                    firstEntry = QUrl(line);
                    break;
                }
            }
        }

        // Relative entries are resolved against the playlist location
        if (firstEntry.isRelative())
            firstEntry = playlistUrl.resolved(firstEntry);

        if (!firstEntry.isValid() || firstEntry.host().isEmpty()
            || (firstEntry.scheme() != "http" && firstEntry.scheme() != "https")) {
            ui->lbl_rol_streaming_status->setText(tr("Playlist contains no playable stream URL"));
            return;
        }

        startRadioStream(firstEntry);
    });
}

/* ============================= DJ deck scratching ============================= */

void player::setPlatterRotation(int deck, double degrees)
{
    QLabel *platter = (deck == 0) ? ui->lp_1 : ui->lp_2;
    const QPixmap &base = m_lpPlatterBase[deck];
    if (base.isNull())
        return;

    // Compose in the artwork's native size — the label has scaledContents,
    // so the result is displayed exactly like the original pixmap/movie.
    // Only the vinyl disc rotates: a rotated copy of the frame is painted
    // clipped to the record's circle (radius chosen to stay inside the
    // tonearm needle), while the turntable base and arm stay static.
    QPixmap frame = base;
    {
        QPainter p(&frame);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::Antialiasing);
        const QPointF center(base.width() / 2.0, base.height() / 2.0);
        const double radius = base.width() * (85.0 / 241.0);
        QPainterPath discClip;
        discClip.addEllipse(center, radius, radius);
        p.setClipPath(discClip);
        p.translate(center);
        p.rotate(degrees);
        p.translate(-center);
        p.drawPixmap(0, 0, base);
    }
    platter->setPixmap(frame);
}

void player::grabPlatterFrame(int deck)
{
    QLabel *platter = (deck == 0) ? ui->lp_1 : ui->lp_2;
    QMovie *anim = (deck == 0) ? movie : movie2;

    if (m_lpPlatterAnim[deck])
        m_lpPlatterAnim[deck]->stop();

    if (anim && anim->state() != QMovie::NotRunning) {
        anim->setPaused(true);
        m_lpPlatterBase[deck] = anim->currentPixmap();
    } else {
        m_lpPlatterBase[deck] = platter->pixmap();
    }
    if (m_lpPlatterBase[deck].isNull())
        m_lpPlatterBase[deck] = QPixmap(":/images/lp_player_p0.png");
    m_lpPlatterRotation[deck] = 0.0;
}

void player::restorePlatterMotion(int deck)
{
    QLabel *platter = (deck == 0) ? ui->lp_1 : ui->lp_2;
    QMovie *anim = (deck == 0) ? movie : movie2;
    if (anim) {
        platter->setMovie(anim); // setPixmap() during the scratch detached it
        anim->setPaused(false);
    } else {
        platter->setPixmap(QPixmap(":/images/lp_player_p0.png"));
    }
}

void player::startPlatterEffectAnimation(int deck, bool backspin)
{
    FxPlayer *lp = (deck == 0) ? lp1_Xplayer : lp2_Xplayer;
    if (!lp || lp->playbackState() != QMediaPlayer::PlayingState || !lp->fxEngineActive())
        return; // the audio effect will not run either

    grabPlatterFrame(deck);

    auto *anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    if (backspin) {
        // Fast whip backwards, matching the ~0.8 s audio backspin
        anim->setEndValue(-1900.0);
        anim->setDuration(850);
        anim->setEasingCurve(QEasingCurve::Linear);
    } else {
        // Platter decelerating to a stop over ~1.1 s
        anim->setEndValue(150.0);
        anim->setDuration(1100);
        anim->setEasingCurve(QEasingCurve::OutCubic);
    }
    connect(anim, &QVariantAnimation::valueChanged, this, [this, deck](const QVariant &v) {
        setPlatterRotation(deck, v.toDouble());
    });
    m_lpPlatterAnim[deck] = anim;
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

bool player::eventFilter(QObject *watched, QEvent *event)
{
    int deck = -1;
    if (watched == ui->lp_1)
        deck = 0;
    else if (watched == ui->lp_2)
        deck = 1;

    if (deck >= 0) {
        FxPlayer *lp = (deck == 0) ? lp1_Xplayer : lp2_Xplayer;
        QLabel *platter = (deck == 0) ? ui->lp_1 : ui->lp_2;

        auto angleAt = [platter](const QPointF &pos) {
            const QPointF c = platter->rect().center();
            constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;
            return std::atan2(pos.y() - c.y(), pos.x() - c.x()) * kRadToDeg;
        };

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton
                && lp && lp->playbackState() == QMediaPlayer::PlayingState
                && lp->fxEngineActive()) {
                m_lpScratching[deck] = true;
                m_lpLastAngleDeg[deck] = angleAt(me->position());
                m_lpLastMoveMs[deck] = m_scratchClock.elapsed();
                lp->scratchBegin();
                grabPlatterFrame(deck); // freeze the artwork; rotation follows the hand
                platter->setCursor(Qt::ClosedHandCursor);
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            if (!m_lpScratching[deck])
                break;
            auto *me = static_cast<QMouseEvent *>(event);
            const qint64 nowMs = m_scratchClock.elapsed();
            const qint64 dtMs = std::max<qint64>(1, nowMs - m_lpLastMoveMs[deck]);
            double dAngle = angleAt(me->position()) - m_lpLastAngleDeg[deck];
            while (dAngle > 180.0) dAngle -= 360.0;
            while (dAngle < -180.0) dAngle += 360.0;
            // A 33 1/3 rpm platter turns 200 degrees per second at speed 1.0
            const double rate = (dAngle * 1000.0 / dtMs) / 200.0;
            lp->scratchMove(rate);
            m_lpLastAngleDeg[deck] += dAngle;
            m_lpLastMoveMs[deck] = nowMs;

            // The record follows the hand
            m_lpPlatterRotation[deck] += dAngle;
            setPlatterRotation(deck, m_lpPlatterRotation[deck]);
            return true;
        }
        case QEvent::MouseButtonRelease: {
            if (!m_lpScratching[deck])
                break;
            m_lpScratching[deck] = false;
            lp->scratchEnd();
            restorePlatterMotion(deck);
            platter->setCursor(Qt::OpenHandCursor);
            return true;
        }
        default:
            break;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

/* =========================== ngrok public share link =========================== */

void player::on_bt_ngrok_setup_clicked()
{
    if (!NgrokTunnelService::available()) {
        QMessageBox box(this);
        box.setWindowTitle(tr("ngrok not installed"));
        box.setIcon(QMessageBox::Information);
        box.setText(tr("The 'ngrok' command was not found on this system."));
        box.setInformativeText(tr("ngrok creates a public link to your streaming server without "
                                  "any router configuration.\n\n"
                                  "1) Download it from https://ngrok.com/download\n"
                                  "   (macOS: brew install ngrok, Linux: snap install ngrok)\n"
                                  "2) Restart XFB and click Setup again."));
        QPushButton *openBtn = box.addButton(tr("Open download page"), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Close);
        box.exec();
        if (box.clickedButton() == openBtn)
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://ngrok.com/download")));
        return;
    }

    bool ok = false;
    const QString token = QInputDialog::getText(this, tr("ngrok account setup"),
        tr("Connect your (free) ngrok account:\n\n"
           "1) Create an account at https://dashboard.ngrok.com\n"
           "2) Open \"Your Authtoken\" and copy the token\n"
           "3) Paste it below — it is stored by ngrok itself, not by XFB\n"),
        QLineEdit::Password, QString(), &ok);
    if (!ok)
        return;

    ui->lbl_ngrok_status->setText(tr("Saving authtoken..."));
    m_ngrokService->configureAuthToken(token);
}

void player::on_bt_ngrok_clicked()
{
    if (m_ngrokService->isRunning()) {
        m_ngrokService->stop();
        return;
    }

    if (!NgrokTunnelService::available()) {
        on_bt_ngrok_setup_clicked(); // shows the install instructions
        return;
    }

    bool portOk = false;
    const int port = ui->txt_ngrok_port->text().trimmed().toInt(&portOk);
    if (!portOk || port < 1 || port > 65535) {
        ui->lbl_ngrok_status->setText(tr("Invalid local port: %1").arg(ui->txt_ngrok_port->text()));
        return;
    }

    {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                               + "/xfb.conf", QSettings::IniFormat);
        settings.setValue("NgrokLocalPort", port);
    }

    ui->lbl_ngrok_status->setText(tr("Creating public link for localhost:%1 ...").arg(port));
    m_ngrokService->start(port);
}

void player::on_bt_ngrok_copy_clicked()
{
    const QString url = ui->txt_ngrok_url->text();
    if (url.isEmpty()) {
        ui->lbl_ngrok_status->setText(tr("No public link to copy yet — click Share first."));
        return;
    }
    QApplication::clipboard()->setText(url);
    ui->statusBar->showMessage(tr("Public stream link copied to the clipboard"), 5000);
}

void player::on_bt_rol_streaming_stop_clicked()
{
    RadioPlayer->stop();
    RadioPlayer->setSource(QUrl()); // drop the network connection immediately
    ui->bt_rol_streaming_play->setStyleSheet("");
    ui->lbl_rol_streaming_status->setText(tr("Stopped"));
}

void player::on_lp_1_bt_play_clicked()
{
    ui->lp_1_bt_play->setDisabled(true);

    // Using lp1_XplaylistUrls instead of deprecated QMediaPlaylist
    lp1_XplaylistUrls.clear();
    lp1_XplaylistUrls.append(QUrl::fromLocalFile(ui->lp_1_txt_file->text()));
    lp1_Xplayer->setSource(lp1_XplaylistUrls.first());
    lp1_Xplayer->play();


    if (movie) movie->deleteLater(); // don't leak the previous animation
    movie = new QMovie(":/images/lp_anim1.gif");
    ui->lp_1->setMovie(movie);
    movie->start();


}

void player::on_pushButton_clicked()
{
    lp1_Xplayer->stop();
    ui->lp_1_bt_play->setDisabled(false);
    ui->lp_1->setPixmap(QPixmap(":/images/lp_player_p0.png"));
    if (movie) movie->stop(); // null until the first play — avoid crash on early Stop
    lp_1_paused = false;
    ui->lp_1_bt_pause->setStyleSheet("");
}

void player::on_lp_1_bt_play_2_clicked()
{
    ui->lp_1_bt_play_2->setDisabled(true);
    ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p1.png"));
    // Using lp2_XplaylistUrls instead of deprecated QMediaPlaylist
    lp2_XplaylistUrls.clear();
    lp2_XplaylistUrls.append(QUrl::fromLocalFile(ui->lp_2_txt_file->text()));
    lp2_Xplayer->setSource(lp2_XplaylistUrls.first());
    lp2_Xplayer->play();

    if (movie2) movie2->deleteLater(); // don't leak the previous animation
    movie2 = new QMovie(":/images/lp_anim1.gif");
    ui->lp_2->setMovie(movie2);
    movie2->start();



}

void player::on_pushButton_2_clicked()
{
    lp2_Xplayer->stop();
    ui->lp_1_bt_play_2->setDisabled(false);
    ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p0.png"));
    if (movie2) movie2->stop(); // null until the first play — avoid crash on early Stop
    lp_2_paused = false;
    ui->lp_2_bt_pause->setStyleSheet("");
}

void player::on_lp_1_bt_pause_clicked()
{
    if(lp_1_paused){
        lp1_Xplayer->play();
        //movie->start();
        lp_1_paused = false;
        ui->lp_1_bt_pause->setStyleSheet("");
    }else{
        lp1_Xplayer->pause();
        //movie->stop();
        lp_1_paused = true;
        ui->lp_1_bt_pause->setStyleSheet("background-color:#EDE635");
    }

}

void player::on_lp_2_bt_pause_clicked()
{
    if(lp_2_paused){
        lp2_Xplayer->play();
        //movie->start();
        lp_2_paused = false;
        ui->lp_2_bt_pause->setStyleSheet("");
    }else{
        lp2_Xplayer->pause();
        //movie->stop();
        lp_2_paused = true;
        ui->lp_2_bt_pause->setStyleSheet("background-color:#EDE635");
    }

}

void player::on_bt_sndconv_clicked()
{
#ifdef Q_OS_MAC
    // On macOS, try to open a sound converter app
    // Check for common audio converter apps
    QStringList macApps = {
        "/Applications/XLD.app",
        "/Applications/Switch Audio Converter.app",
        "/Applications/MediaHuman Audio Converter.app"
    };
    for (const QString &app : macApps) {
        if (QFile::exists(app)) {
            QProcess::startDetached("open", QStringList() << "-a" << app);
            return;
        }
    }
    // Fallback: open App Store search for audio converter
    QMessageBox::information(this, tr("Sound Converter"),
        tr("No audio converter application found.\n\n"
           "Recommended free options for macOS:\n"
           "• XLD (X Lossless Decoder)\n"
           "• FFmpeg (command line: brew install ffmpeg)\n\n"
           "You can also use: ffmpeg -i input.ogg output.mp3"));
#else
    // Linux: try soundconverter, then gnome-sound-converter
    QString converter = QStandardPaths::findExecutable("soundconverter");
    if (converter.isEmpty()) {
        converter = QStandardPaths::findExecutable("gnome-sound-converter");
    }
    if (!converter.isEmpty()) {
        QProcess::startDetached(converter, QStringList());
    } else {
        // Offer to install soundconverter on demand, with consent and progress.
        DependencyChecker depChecker;
        if (depChecker.ensureDependency("soundconverter",
                tr("The Sound Converter lets you convert audio files between formats. "
                   "It is provided by the \"soundconverter\" program."),
                this)) {
            converter = QStandardPaths::findExecutable("soundconverter");
            if (!converter.isEmpty()) {
                QProcess::startDetached(converter, QStringList());
            }
        }
    }
#endif
}


void player::on_actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database_triggered()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- Check for SOX executable ---
    QString soxPath = QStandardPaths::findExecutable("sox");
    if (soxPath.isEmpty()) {
        qWarning() << "'sox' command not found in system PATH.";
        QMessageBox::critical(this, "Missing Dependency",
                              "The 'sox' command (Sound eXchange) is required for this feature "
                              "but was not found in the system's PATH.\n\nPlease install SoX and ensure it's accessible.");
        return;
    }
    qInfo() << "Found sox executable at:" << soxPath;
    // --- End SOX check ---

    QMessageBox::StandardButton run = QMessageBox::question(this, "Confirm Auto-Trim",
                                                            "This will attempt to trim silence (below 1% threshold) from the start and end of every track in the database using 'sox'.\n\n"
                                                            "Original files will be overwritten!\n\n"
                                                            "This process can take a long time and is irreversible.\n"
                                                            "Are you sure you want to proceed?",
                                                            QMessageBox::Yes | QMessageBox::No);
    if (run == QMessageBox::No) {
        return;
    }

    // --- Setup Loading Indicator & Progress Dialog ---
    QMovie loadingMovie(":/images/loading.gif"); // Use stack variable if movie is temporary
    if(!loadingMovie.isValid()){
         qWarning() << "Loading GIF not valid:" << ":/images/loading.gif";
         // Continue without movie, maybe show text?
         ui->txt_loading->setText("Processing...");
         ui->txt_loading->show();
    } else {
        ui->txt_loading->setMovie(&loadingMovie);
        ui->txt_loading->setScaledContents(true);
        ui->txt_loading->show();
        loadingMovie.start();
    }
    qApp->processEvents(); // Allow UI to update

    QProgressDialog progressDialog("Trimming silence...", "Cancel", 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();
    qApp->processEvents();
    // --- End Setup ---


    QSqlQuery querySelect(db);
    QString selectStr = "SELECT path FROM musics"; // Only need the path
    int totalFiles = 0;

    // First count the files for the progress bar
    if (querySelect.exec("SELECT COUNT(*) FROM musics")) {
        if (querySelect.next()) {
            totalFiles = querySelect.value(0).toInt();
        }
    } else {
         qWarning() << "Failed to count records:" << querySelect.lastError();
         // Proceed without accurate progress max? Or abort? Let's try proceeding.
    }
    progressDialog.setMaximum(totalFiles > 0 ? totalFiles : 100); // Set max for progress bar
    qApp->processEvents();


    // Now get the paths
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT paths from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table for paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;
    bool cancelled = false;

    QTemporaryDir tempDir; // Creates a unique temporary directory
    if (!tempDir.isValid()) {
        qWarning() << "Failed to create temporary directory:" << tempDir.errorString();
        QMessageBox::critical(this, "File System Error", "Could not create a temporary directory to process files.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }
    qInfo() << "Using temporary directory:" << tempDir.path();


    while (querySelect.next() && !cancelled) {
        processedCount++;
        progressDialog.setValue(processedCount);
        QString originalPath = querySelect.value(0).toString();
        QFileInfo originalFileInfo(originalPath);

        progressDialog.setLabelText(QString("Processing [%1/%2]:\n%3")
                                        .arg(processedCount)
                                        .arg(totalFiles)
                                        .arg(originalFileInfo.fileName()));
        qApp->processEvents(); // Keep UI responsive and check for Cancel button

        if (progressDialog.wasCanceled()) {
            qInfo() << "User cancelled the operation.";
            cancelled = true;
            break;
        }

        qInfo().noquote() << "AutoTrim Processing [" << processedCount << "/" << totalFiles << "]:" << originalPath;

        if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
            qWarning() << "File does not exist or is not a regular file:" << originalPath;
            failCount++;
            continue; // Skip to next file
        }

        // --- Prepare temporary output path ---
        // Use original filename but in the temporary directory
        QString tempOutputPath = tempDir.filePath(originalFileInfo.fileName());
        qDebug() << "Temporary output path:" << tempOutputPath;

        // --- Run SOX ---
        QProcess soxProcess;
        QStringList soxArgs;
        soxArgs << originalPath                   // Input file
                << tempOutputPath                 // Output file
                << "silence"                      // Effect name
                << "1"                            // Detect silence around the file (mode)
                << "0.1"                          // Stop point (duration in seconds)
                << "1%";                          // Threshold (e.g., 0.1% or 1%) - NOTE: You had 0.1% in question, 1% in code. Using 1% as per code.

        qDebug() << "Running command:" << soxPath << soxArgs;

        soxProcess.start(soxPath, soxArgs);

        // Wait for SOX to finish (adjust timeout if needed, -1 means wait indefinitely)
        // Use a reasonable timeout, e.g., 5 minutes (300000 ms) for potentially large files
        if (!soxProcess.waitForFinished(300000)) {
            qWarning() << "Sox process timed out for:" << originalPath << soxProcess.errorString();
            soxProcess.kill();
            soxProcess.waitForFinished(1000); // Wait a bit after killing
            failCount++;
            continue; // Skip to next file
        }

        // Check SOX execution result
        if (soxProcess.exitStatus() != QProcess::NormalExit || soxProcess.exitCode() != 0) {
            qWarning() << "Sox process failed for:" << originalPath
                       << "Exit code:" << soxProcess.exitCode()
                       << "Exit status:" << soxProcess.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(soxProcess.readAllStandardError());
            qWarning() << "Sox Standard Error:\n" << errorOutput;
            // Clean up potentially incomplete temp file before continuing
            QFile::remove(tempOutputPath);
            failCount++;
            continue; // Skip to next file
        }

        // --- Check if temp file was created and is valid ---
        QFileInfo tempFileInfo(tempOutputPath);
        if (!tempFileInfo.exists() || tempFileInfo.size() == 0) {
             qWarning() << "Sox finished successfully but the output file is missing or empty:" << tempOutputPath;
             // Clean up potentially empty temp file
             QFile::remove(tempOutputPath);
             failCount++;
             continue; // Skip to next file
        }

        qDebug() << "Sox completed successfully. Temp file size:" << tempFileInfo.size();

        // --- Replace original file with trimmed version ---
        // QFile::rename will overwrite the destination if it exists on most platforms.
        // This is atomic on many filesystems (safer than delete then move).
        bool renamed = QFile::rename(tempOutputPath, originalPath);

        if (!renamed) {
            // Rename failed. Maybe permissions issue, file lock, cross-device move?
            // Try copy & delete as a fallback (less safe, not atomic)
            qWarning() << "QFile::rename failed for" << tempOutputPath << "to" << originalPath << ". Attempting copy and delete.";
            if (QFile::copy(tempOutputPath, originalPath)) {
                 qDebug() << "Copy successful. Now removing temporary file.";
                 if (!QFile::remove(tempOutputPath)) {
                      qWarning() << "Failed to remove temporary file after successful copy:" << tempOutputPath;
                      // Continue, main goal achieved, but temp file left behind.
                 }
                 successCount++;
                 qInfo() << "File trimmed and replaced (using copy/delete):" << originalPath;
            } else {
                qWarning() << "Fallback copy failed for:" << tempOutputPath << "to" << originalPath;
                // Clean up temp file if copy failed
                QFile::remove(tempOutputPath);
                failCount++;
                continue; // Skip to next file
            }
        } else {
            // Rename successful! The temp file is now gone (moved).
            successCount++;
            qInfo() << "File trimmed and replaced (using rename):" << originalPath;
        }
        // --- End Replace ---

    } // End while loop

    // --- Cleanup and Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close the progress dialog cleanly

    // Temporary directory (`tempDir`) is automatically removed here when it goes out of scope.

    QString summaryMessage;
    if (cancelled) {
         summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                             .arg(processedCount -1) // Don't count the one being processed when cancelled
                             .arg(successCount)
                             .arg(failCount);
    } else {
         summaryMessage = QString("Auto-Trim Complete.\n\nTotal Records: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                             .arg(totalFiles)
                             .arg(successCount)
                             .arg(failCount);
    }

    qInfo() << "-------------------------------------";
    qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Operation Summary", summaryMessage);
}

void player::on_actionUpdate_System_triggered()
{
    checkForUpdates();
}

void player::on_actionInstall_all_dependencies_triggered()
{
    DependencyChecker depChecker;
    depChecker.installAllInteractive(this);
}

void player::on_bt_apply_multi_selection_clicked()
{

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    int accao = ui->cbox_multi_select->currentIndex();

    qDebug()<<"ComboBox action: "<<accao;

   if(accao==1){

       //name them in debugger

       QModelIndexList indexlist = ui->musicView->selectionModel()->selectedIndexes();
       int row;
       foreach(QModelIndex index, indexlist){
           if(index.row()!=row){
               row = index.row();
               qDebug()<<"This row is selected: "<<row;
               qDebug()<<ui->musicView->model()->data(ui->musicView->model()->index(row,7)).toString();

           }

       }


   }

   if(accao==2){
       //rm them




       {

                   QMessageBox::StandardButton go;
                   go = QMessageBox::question(this,tr("Sure?"),tr("Are you sure you want to delete all the selected track from the database?"), QMessageBox::Yes|QMessageBox::No);
                   if(go==QMessageBox::Yes){



                       QMessageBox::StandardButton rm;
                       rm = QMessageBox::question(this,tr("Erase the files also?"),tr("Do you want to delete all the selected files from the hard drive also?"), QMessageBox::Yes|QMessageBox::No);




                       QModelIndexList indexlist = ui->musicView->selectionModel()->selectedIndexes();
                       int row;
                       foreach(QModelIndex index, indexlist){
                           if(index.row()!=row){
                               row = index.row();
                               qDebug()<<"This row is selected: "<<row;
                               QString thisfilename = ui->musicView->model()->data(ui->musicView->model()->index(row,7)).toString();




                               QSqlQuery sql(db);
                               sql.prepare("delete from musics where path=:path");
                               sql.bindValue(":path", thisfilename);

                               if(sql.exec()){
                                   qDebug()<<"Track removed from the database: "<<thisfilename;
                               } else {
                                   QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                                   qDebug() << "last sql: " << sql.lastQuery();
                               }


                               if (rm == QMessageBox::Yes) {
                                   qInfo() << "User confirmed deletion. Attempting to delete file from disk:" << thisfilename;

                                   // --- Use Qt's QFile::remove for platform-independent deletion ---

                                   // Optional: Check existence first, though remove() handles non-existent files gracefully (returns false)
                                   if (!QFile::exists(thisfilename)) {
                                       qWarning() << "File no longer exists or path is incorrect:" << thisfilename;
                                       // You might want to inform the user or just proceed as if deleted
                                       QMessageBox::information(this, "File Not Found",
                                                                QString("The file\n%1\ncould not be found. It might have been deleted already.")
                                                                    .arg(thisfilename));
                                   } else {
                                       // Attempt the deletion
                                       bool success = QFile::remove(thisfilename);

                                       if (success) {
                                           qInfo() << "File successfully deleted from disk:" << thisfilename;
                                           // Optional: A small confirmation message if desired, often logging is enough.
                                           // QMessageBox::information(this, "Success", QString("File deleted:\n%1").arg(thisfilename));
                                       } else {
                                           // Deletion failed - Log the error and inform the user
                                           // Note: QFile::errorString() isn't always set reliably after remove(),
                                           //       so checking the return value is the primary way.
                                           qWarning() << "Failed to delete file from disk:" << thisfilename;
                                           QMessageBox::warning(this, "Deletion Failed",
                                                                QString("Could not delete the file:\n%1\n\nPlease check file permissions or if the file is currently in use.")
                                                                    .arg(thisfilename));
                                       }
                                   }
                                   // --- End of QFile::remove usage ---

                               } else {
                                   qDebug() << "User cancelled file deletion for:" << thisfilename;
                               }







                           }

                       }


                       update_music_table();





                   }
                  }










   }



   if(accao==3){


       QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
       if (!db.isOpen()) {
           qWarning() << "Database connection 'xfb_connection' is not open!";
           QMessageBox::critical(this, "Database Error", "Database connection is not open.");
           return;
       }

       // --- Check for FFMPEG executable ---
       QString ffmpegPath = FxEngine::ffmpegExecutable();
       if (ffmpegPath.isEmpty()) {
           qWarning() << "'ffmpeg' command not found in system PATH.";
           QMessageBox::critical(this, "Missing Dependency",
                                 "The 'ffmpeg' command is required for audio conversion "
                                 "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
           return;
       }
       qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
       // --- End FFMPEG check ---


       // --- Get Selected Files ---
       QModelIndexList selectedIndexes = ui->musicView->selectionModel()->selectedIndexes();
       QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
       for (const QModelIndex &index : selectedIndexes) {
           uniqueRows.insert(index.row());
       }

       if (uniqueRows.isEmpty()) {
           QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to convert.");
           return;
       }

       int totalFilesToProcess = uniqueRows.size();
       qInfo() << "Found" << totalFilesToProcess << "unique rows selected for conversion.";

       // --- Confirmation ---
       QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Conversion",
                                        QString("Convert %1 selected track(s) to MP3 (192kbps)?\n\n"
                                                "Original files will be replaced with the MP3 version.\n"
                                                "This action cannot be undone.\n\n"
                                                "Note: Only the audio stream will be kept.")
                                            .arg(totalFilesToProcess),
                                        QMessageBox::Yes | QMessageBox::No);

       if (confirm == QMessageBox::No) {
           return;
       }

       // --- Setup Progress Dialog ---
       QProgressDialog progressDialog("Converting to MP3...", "Cancel", 0, totalFilesToProcess, this);
       progressDialog.setWindowModality(Qt::WindowModal);
       progressDialog.setValue(0);
       progressDialog.show();
       qApp->processEvents(); // Allow UI to update

       // --- Prepare Database Query (outside loop) ---
       QSqlQuery queryUpdate(db);
       if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
           qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
           QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
           progressDialog.cancel();
           return;
       }

       // --- Get Temporary Directory ---
       QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
       if (tempDirPath.isEmpty()) {
            qWarning() << "Could not find a writable temporary location.";
            QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
            progressDialog.cancel();
            return;
       }
       QDir tempDir(tempDirPath);
       qInfo() << "Using temporary directory:" << tempDirPath;


       // --- Process Files ---
       int successCount = 0;
       int failCount = 0;
       bool cancelled = false;

       // Convert QSet to QList to iterate in a defined order (optional, but often useful)
       QList<int> rowsToProcess = uniqueRows.values();
       std::sort(rowsToProcess.begin(), rowsToProcess.end()); // Sort rows numerically

       for (int row : rowsToProcess) {
           if (progressDialog.wasCanceled()) {
               cancelled = true;
               break;
           }

           progressDialog.setValue(successCount + failCount); // Update progress
           qApp->processEvents();

           // Get original file path from model (assuming column 7)
           QModelIndex pathIndex = ui->musicView->model()->index(row, 7);
           QString originalPath = ui->musicView->model()->data(pathIndex).toString();
           QFileInfo originalFileInfo(originalPath);

           progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                           .arg(successCount + failCount + 1)
                                           .arg(totalFilesToProcess)
                                           .arg(originalFileInfo.fileName()));
           qApp->processEvents();

           qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

           if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
               qWarning() << "Original file does not exist or is not a file:" << originalPath;
               failCount++;
               continue; // Skip to next file
           }

           // --- Prepare Paths ---
           QString baseName = originalFileInfo.baseName(); // Name without extension
           QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
           QString tempMp3Path = tempDir.filePath(baseName + ".mp3"); // Full path for temporary MP3
           QString finalMp3Path = QDir(originalDir).filePath(baseName + ".mp3"); // Final path for MP3 in original dir

           // --- Run FFMPEG ---
           QProcess ffmpegProcess;
           QStringList ffmpegArgs;
           ffmpegArgs << "-i" << originalPath // Input file
                      << "-vn"               // No video output
                      << "-ar" << "44100"    // Audio sample rate
                      << "-ac" << "2"        // Audio channels (stereo)
                      << "-b:a" << "192k"    // Audio bitrate (use -b:a for modern ffmpeg) - was -ab
                      << "-f" << "mp3"       // Force output format (usually optional)
                      << tempMp3Path;        // Output file path

           qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

           ffmpegProcess.start(ffmpegPath, ffmpegArgs);

           // Wait for ffmpeg (e.g., 10 min timeout for potentially large files/slow conversion)
           if (!ffmpegProcess.waitForFinished(600000)) {
               qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
               ffmpegProcess.kill();
               ffmpegProcess.waitForFinished(1000);
               QFile::remove(tempMp3Path); // Clean up incomplete temp file
               failCount++;
               continue;
           }

           // Check ffmpeg result
           if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
               qWarning() << "ffmpeg process failed for:" << originalPath
                          << "Exit code:" << ffmpegProcess.exitCode()
                          << "Exit status:" << ffmpegProcess.exitStatus();
               QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
               qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
                QFile::remove(tempMp3Path); // Clean up potentially failed temp file
               failCount++;
               continue;
           }

           // Check if temp MP3 exists and has size
            QFileInfo tempMp3Info(tempMp3Path);
            if (!tempMp3Info.exists() || tempMp3Info.size() == 0) {
                qWarning() << "ffmpeg finished successfully but the output MP3 file is missing or empty:" << tempMp3Path;
                QFile::remove(tempMp3Path);
                failCount++;
                continue;
            }

           qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempMp3Path;

           // --- Delete Original File ---
           qInfo() << "Attempting to delete original file:" << originalPath;
           if (!QFile::remove(originalPath)) {
               qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
               // Don't delete the temp MP3 - user might want it
               // Consider moving temp MP3 to a known 'failed' folder? For now, just leave it in temp.
               failCount++;
               continue; // Skip rest of steps for this file
           }
           qInfo() << "Original file deleted successfully.";

           // --- Move Temporary MP3 to Final Location ---
           qInfo() << "Attempting to move" << tempMp3Path << "to" << finalMp3Path;
           if (!QFile::rename(tempMp3Path, finalMp3Path)) {
               qWarning() << "Failed to move temporary MP3" << tempMp3Path << "to" << finalMp3Path << ". The original file was deleted! MP3 remains in temp folder.";
               // This is a problematic state - original is gone, MP3 is stuck in temp.
               // Inform the user strongly.
               QMessageBox::warning(this, "Move Failed",
                                    QString("Failed to move the converted MP3 to its final destination:\n%1\n\n"
                                            "The original file was deleted, but the MP3 remains in the temporary folder:\n%2\n\n"
                                            "Please move it manually and check the database record.")
                                        .arg(finalMp3Path).arg(tempMp3Path));
               failCount++;
               continue; // Cannot update DB path if move failed
           }
           qInfo() << "MP3 moved successfully to:" << finalMp3Path;

           // --- Update Database ---
           qInfo() << "Updating database: set path =" << finalMp3Path << "where path =" << originalPath;
           queryUpdate.bindValue(":new_path", finalMp3Path);
           queryUpdate.bindValue(":old_path", originalPath);

           if (!queryUpdate.exec()) {
               qWarning() << "Failed to update database path for:" << originalPath << "->" << finalMp3Path;
               qWarning() << "DB Error:" << queryUpdate.lastError().text();
               qWarning() << "Last Query:" << queryUpdate.lastQuery(); // See bound values if driver supports it
               // The file was converted and moved, but the DB is out of sync.
                QMessageBox::warning(this, "Database Update Failed",
                                    QString("The file was successfully converted and moved to:\n%1\n\n"
                                            "However, updating the database record failed:\n%2\n\n"
                                            "Please check the database manually.")
                                        .arg(finalMp3Path).arg(queryUpdate.lastError().text()));
               failCount++; // Count as failure since DB update is critical
           } else {
               qInfo() << "Database path updated successfully.";
               successCount++;
               // update_music_table(); // Consider updating only once at the end for performance
           }

       } // End for loop

       // --- Final Cleanup & Summary ---
       progressDialog.cancel(); // Close progress dialog

       update_music_table(); // Update the table view once after all operations

       QString summaryMessage;
       if (cancelled) {
           summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Converted: %1\nFailed/Skipped: %2")
                                .arg(successCount)
                                .arg(failCount);
       } else {
           summaryMessage = QString("Conversion Complete.\n\nTotal Selected: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                                .arg(totalFilesToProcess)
                                .arg(successCount)
                                .arg(failCount);
       }

       qInfo() << "-------------------------------------";
       qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
       qInfo() << "-------------------------------------";

       QMessageBox::information(this, "Conversion Summary", summaryMessage);
   }




   if(accao==4){

       QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
          if (!db.isOpen()) {
              qWarning() << "Database connection 'xfb_connection' is not open!";
              QMessageBox::critical(this, "Database Error", "Database connection is not open.");
              return;
          }

          // --- Check for FFMPEG executable ---
          QString ffmpegPath = FxEngine::ffmpegExecutable();
          if (ffmpegPath.isEmpty()) {
              qWarning() << "'ffmpeg' command not found in system PATH.";
              QMessageBox::critical(this, "Missing Dependency",
                                    "The 'ffmpeg' command is required for audio conversion "
                                    "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
              return;
          }
          qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
          // --- End FFMPEG check ---


          // --- Get Selected Files ---
          QModelIndexList selectedIndexes = ui->musicView->selectionModel()->selectedIndexes();
          QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
          for (const QModelIndex &index : selectedIndexes) {
              uniqueRows.insert(index.row());
          }

          if (uniqueRows.isEmpty()) {
              QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to convert.");
              return;
          }

          int totalFilesToProcess = uniqueRows.size();
          qInfo() << "Found" << totalFilesToProcess << "unique rows selected for conversion to Ogg Vorbis.";

          // --- Confirmation ---
          // Adjusted confirmation message for Ogg Vorbis (quality scale 7)
          QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Conversion",
                                           QString("Convert %1 selected track(s) to Ogg Vorbis (Quality ~7)?\n\n"
                                                   "Original files will be replaced with the Ogg version.\n"
                                                   "This action cannot be undone.\n\n"
                                                   "Note: Only the audio stream will be kept.")
                                               .arg(totalFilesToProcess),
                                           QMessageBox::Yes | QMessageBox::No);

          if (confirm == QMessageBox::No) {
              return;
          }

          // --- Setup Progress Dialog ---
          // Adjusted progress dialog title
          QProgressDialog progressDialog("Converting to Ogg Vorbis...", "Cancel", 0, totalFilesToProcess, this);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setValue(0);
          progressDialog.show();
          qApp->processEvents(); // Allow UI to update

          // --- Prepare Database Query (outside loop) ---
          QSqlQuery queryUpdate(db);
          // Using single quotes for string literals in SQL might be less portable
          // but stick to original if required by specific DB. Prepared statements handle this better.
          if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
              qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
              QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
              progressDialog.cancel();
              return;
          }

          // --- Get Temporary Directory ---
          QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
          if (tempDirPath.isEmpty()) {
               qWarning() << "Could not find a writable temporary location.";
               QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
               progressDialog.cancel();
               return;
          }
          QDir tempDir(tempDirPath);
          qInfo() << "Using temporary directory:" << tempDirPath;


          // --- Process Files ---
          int successCount = 0;
          int failCount = 0;
          bool cancelled = false;

          QList<int> rowsToProcess = uniqueRows.values();
          std::sort(rowsToProcess.begin(), rowsToProcess.end());

          for (int row : rowsToProcess) {
              if (progressDialog.wasCanceled()) {
                  cancelled = true;
                  break;
              }

              progressDialog.setValue(successCount + failCount); // Update progress
              qApp->processEvents();

              // Get original file path from model (assuming column 7)
              QModelIndex pathIndex = ui->musicView->model()->index(row, 7);
              QString originalPath = ui->musicView->model()->data(pathIndex).toString();
              QFileInfo originalFileInfo(originalPath);

              // Adjust progress label text
              progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                              .arg(successCount + failCount + 1)
                                              .arg(totalFilesToProcess)
                                              .arg(originalFileInfo.fileName()));
              qApp->processEvents();

              qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

              if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
                  qWarning() << "Original file does not exist or is not a file:" << originalPath;
                  failCount++;
                  continue; // Skip to next file
              }

              // --- Prepare Paths ---
              QString baseName = originalFileInfo.baseName();
              QString originalDir = originalFileInfo.absolutePath();
              // Construct Ogg paths
              QString tempOggPath = tempDir.filePath(baseName + ".ogg");
              QString finalOggPath = QDir(originalDir).filePath(baseName + ".ogg");

              // --- Run FFMPEG ---
              QProcess ffmpegProcess;
              QStringList ffmpegArgs;
              // Adjusted ffmpeg arguments for Ogg Vorbis
              ffmpegArgs << "-i" << originalPath           // Input file
                         << "-c:a" << "libvorbis"        // Audio codec: libvorbis
                         << "-qscale:a" << "7"           // Quality scale (adjust as needed, 5-7 is common)
                         << tempOggPath;                 // Output file path

              qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

              ffmpegProcess.start(ffmpegPath, ffmpegArgs);

              // Wait for ffmpeg (adjust timeout if needed)
              if (!ffmpegProcess.waitForFinished(600000)) { // 10 min timeout
                  qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
                  ffmpegProcess.kill();
                  ffmpegProcess.waitForFinished(1000);
                  QFile::remove(tempOggPath); // Clean up incomplete temp file
                  failCount++;
                  continue;
              }

              // Check ffmpeg result
              if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
                  qWarning() << "ffmpeg process failed for:" << originalPath
                             << "Exit code:" << ffmpegProcess.exitCode()
                             << "Exit status:" << ffmpegProcess.exitStatus();
                  QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
                  qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
                   QFile::remove(tempOggPath); // Clean up potentially failed temp file
                  failCount++;
                  continue;
              }

              // Check if temp Ogg exists and has size
               QFileInfo tempOggInfo(tempOggPath);
               if (!tempOggInfo.exists() || tempOggInfo.size() == 0) {
                   qWarning() << "ffmpeg finished successfully but the output Ogg file is missing or empty:" << tempOggPath;
                   QFile::remove(tempOggPath);
                   failCount++;
                   continue;
               }

              qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempOggPath;

              // --- Delete Original File ---
              qInfo() << "Attempting to delete original file:" << originalPath;
              if (!QFile::remove(originalPath)) {
                  qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
                  failCount++;
                  continue; // Skip rest of steps for this file
              }
              qInfo() << "Original file deleted successfully.";

              // --- Move Temporary Ogg to Final Location ---
              qInfo() << "Attempting to move" << tempOggPath << "to" << finalOggPath;
              if (!QFile::rename(tempOggPath, finalOggPath)) {
                  qWarning() << "Failed to move temporary Ogg" << tempOggPath << "to" << finalOggPath << ". The original file was deleted! Ogg remains in temp folder.";
                  QMessageBox::warning(this, "Move Failed",
                                       QString("Failed to move the converted Ogg file to its final destination:\n%1\n\n"
                                               "The original file was deleted, but the Ogg file remains in the temporary folder:\n%2\n\n"
                                               "Please move it manually and check the database record.")
                                           .arg(finalOggPath).arg(tempOggPath));
                  failCount++;
                  continue; // Cannot update DB path if move failed
              }
              qInfo() << "Ogg file moved successfully to:" << finalOggPath;

              // --- Update Database ---
              qInfo() << "Updating database: set path =" << finalOggPath << "where path =" << originalPath;
              queryUpdate.bindValue(":new_path", finalOggPath);
              queryUpdate.bindValue(":old_path", originalPath);

              if (!queryUpdate.exec()) {
                  qWarning() << "Failed to update database path for:" << originalPath << "->" << finalOggPath;
                  qWarning() << "DB Error:" << queryUpdate.lastError().text();
                  qWarning() << "Last Query:" << queryUpdate.lastQuery();
                  QMessageBox::warning(this, "Database Update Failed",
                                       QString("The file was successfully converted and moved to:\n%1\n\n"
                                               "However, updating the database record failed:\n%2\n\n"
                                               "Please check the database manually.")
                                           .arg(finalOggPath).arg(queryUpdate.lastError().text()));
                  failCount++; // Count as failure since DB update is critical
              } else {
                  qInfo() << "Database path updated successfully.";
                  successCount++;
              }

          } // End for loop

          // --- Final Cleanup & Summary ---
          progressDialog.cancel(); // Close progress dialog

          update_music_table(); // Update the table view once after all operations

          // Adjust summary message
          QString summaryMessage;
          if (cancelled) {
              summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Converted to Ogg: %1\nFailed/Skipped: %2")
                                   .arg(successCount)
                                   .arg(failCount);
          } else {
              summaryMessage = QString("Ogg Conversion Complete.\n\nTotal Selected: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                                   .arg(totalFilesToProcess)
                                   .arg(successCount)
                                   .arg(failCount);
          }

          qInfo() << "-------------------------------------";
          qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
          qInfo() << "-------------------------------------";

          QMessageBox::information(this, "Conversion Summary", summaryMessage);


   }



   if(accao==5){

       //trim

       // --- Check for SOX executable ---
          QString soxPath = QStandardPaths::findExecutable("sox");
          if (soxPath.isEmpty()) {
              qWarning() << "'sox' command not found in system PATH.";
              QMessageBox::critical(this, "Missing Dependency",
                                    "The 'sox' command (Sound eXchange) is required for this feature "
                                    "but was not found in the system's PATH.\n\nPlease install SoX and ensure it's accessible.");
              return;
          }
          qInfo() << "Found sox executable at:" << soxPath;
          // --- End SOX check ---

          // --- Get Selected Files ---
          QItemSelectionModel *selectionModel = ui->musicView->selectionModel();
          if (!selectionModel) {
              qWarning() << "Music view selection model is null.";
              return;
          }
          QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
          QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
          for (const QModelIndex &index : selectedIndexes) {
              uniqueRows.insert(index.row());
          }

          if (uniqueRows.isEmpty()) {
              QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to trim silence.");
              return;
          }

          int totalFilesToProcess = uniqueRows.size();
          qInfo() << "Found" << totalFilesToProcess << "unique rows selected for silence trimming.";

          // --- Confirmation ---
          QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Silence Trim",
                                           QString("Trim silence (below 1% threshold) from the start and end of %1 selected track(s)?\n\n"
                                                   "Original files will be overwritten!\n\n"
                                                   "This action cannot be undone.")
                                               .arg(totalFilesToProcess),
                                           QMessageBox::Yes | QMessageBox::No);

          if (confirm == QMessageBox::No) {
              return;
          }

          // --- Setup Progress Dialog ---
          QProgressDialog progressDialog("Trimming silence...", "Cancel", 0, totalFilesToProcess, this);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setValue(0);
          progressDialog.show();
          qApp->processEvents(); // Allow UI to update

          // --- Get Temporary Directory ---
          // Use QTemporaryDir for automatic cleanup if preferred, or standard temp location
          QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
          if (tempDirPath.isEmpty()) {
               qWarning() << "Could not find a writable temporary location.";
               QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for processing.");
               progressDialog.cancel();
               return;
          }
          QDir tempDir(tempDirPath);
          qInfo() << "Using temporary directory:" << tempDirPath;

          // --- Process Files ---
          int successCount = 0;
          int failCount = 0;
          bool cancelled = false;
          QAbstractItemModel *model = ui->musicView->model();
          if (!model) {
              qWarning() << "Music view model is null.";
              QMessageBox::critical(this, "Internal Error", "Cannot access the data model.");
              progressDialog.cancel();
              return;
          }


          // Convert QSet to QList to iterate in a defined order (optional)
          QList<int> rowsToProcess = uniqueRows.values();
          std::sort(rowsToProcess.begin(), rowsToProcess.end()); // Sort rows numerically

          for (int row : rowsToProcess) {
              if (progressDialog.wasCanceled()) {
                  cancelled = true;
                  break;
              }

              progressDialog.setValue(successCount + failCount); // Update progress
              qApp->processEvents();

              // Get original file path from model (assuming column 7)
              QModelIndex pathIndex = model->index(row, 7); // Assuming column 7 holds the path
              if (!pathIndex.isValid()) {
                   qWarning() << "Invalid model index for row" << row << ", column 7.";
                   failCount++;
                   continue;
              }
              QString originalPath = model->data(pathIndex).toString();
              QFileInfo originalFileInfo(originalPath);

              progressDialog.setLabelText(QString("Trimming [%1/%2]:\n%3")
                                              .arg(successCount + failCount + 1)
                                              .arg(totalFilesToProcess)
                                              .arg(originalFileInfo.fileName()));
              qApp->processEvents();

              qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

              if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
                  qWarning() << "Original file does not exist or is not a file:" << originalPath;
                  failCount++;
                  continue; // Skip to next file
              }

              // --- Prepare Paths ---
              QString baseName = originalFileInfo.fileName(); // Keep full filename for temp
              QString tempOutputPath = tempDir.filePath(baseName); // Construct full path in temp dir
              // Ensure temp filename is unique in case multiple selections have the same filename (though unlikely if paths are unique)
              // A more robust approach might add a unique ID or use QTemporaryFile, but simple temp path often suffices here.
              qDebug() << "Temporary output path:" << tempOutputPath;

              // --- Run SOX ---
              QProcess soxProcess;
              QStringList soxArgs;
              soxArgs << originalPath                   // Input file
                      << tempOutputPath                 // Output file
                      << "silence"                      // Effect name
                      << "1"                            // Detect silence around the file (mode)
                      << "0.1"                          // Stop point (duration in seconds) - from original code
                      << "1%";                          // Threshold (e.g., 0.1% or 1%) - from original code

              qDebug() << "Running command:" << soxPath << soxArgs;

              soxProcess.start(soxPath, soxArgs);

              // Wait for SOX (adjust timeout if needed, -1 means wait indefinitely)
              if (!soxProcess.waitForFinished(300000)) { // 5 min timeout
                  qWarning() << "Sox process timed out for:" << originalPath << soxProcess.errorString();
                  soxProcess.kill();
                  soxProcess.waitForFinished(1000);
                  QFile::remove(tempOutputPath); // Clean up incomplete temp file
                  failCount++;
                  continue;
              }

              // Check SOX execution result
              if (soxProcess.exitStatus() != QProcess::NormalExit || soxProcess.exitCode() != 0) {
                  qWarning() << "Sox process failed for:" << originalPath
                             << "Exit code:" << soxProcess.exitCode()
                             << "Exit status:" << soxProcess.exitStatus();
                  QString errorOutput = QString::fromLocal8Bit(soxProcess.readAllStandardError());
                  qWarning() << "Sox Standard Error:\n" << errorOutput;
                  QFile::remove(tempOutputPath); // Clean up potentially failed temp file
                  failCount++;
                  continue;
              }

              // --- Check if temp file was created and is valid ---
              QFileInfo tempFileInfo(tempOutputPath);
              if (!tempFileInfo.exists() || tempFileInfo.size() == 0) {
                   qWarning() << "Sox finished successfully but the output file is missing or empty:" << tempOutputPath;
                   QFile::remove(tempOutputPath);
                   failCount++;
                   continue;
              }
              // Optional: Check if temp file size is drastically different? Maybe not reliable.

              qInfo() << "Sox completed successfully for:" << originalPath << " -> " << tempOutputPath;

              // --- Replace original file with trimmed version ---
              qInfo() << "Attempting to replace" << originalPath << "with" << tempOutputPath;
              if (!QFile::rename(tempOutputPath, originalPath)) {
                   qWarning() << "Failed to replace original file using rename:" << originalPath << "with temp file:" << tempOutputPath;
                   // Attempt fallback copy/delete might be risky if original perms are the issue
                   // Best to leave the temp file and warn the user.
                   QMessageBox::warning(this, "Replacement Failed",
                                       QString("Could not replace the original file:\n%1\n\n"
                                               "The trimmed version remains in the temporary folder:\n%2\n\n"
                                               "Please check file permissions and replace it manually if desired.")
                                           .arg(originalPath).arg(tempOutputPath));
                  failCount++;
                  // Do NOT delete the temp file in this case - user might want it.
              } else {
                  qInfo() << "File trimmed and replaced successfully:" << originalPath;
                  successCount++;
                  // Temp file is gone (renamed)
              }
              // --- End Replace ---

          } // End for loop

          // --- Final Cleanup & Summary ---
          progressDialog.cancel(); // Close progress dialog

          // Note: If using QTemporaryDir, it cleans up here. If using standard temp location,
          // successfully moved files are gone, failed temp files might remain as per logic above.

          QString summaryMessage;
          if (cancelled) {
              summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Trimmed: %1\nFailed/Skipped: %2")
                                   .arg(successCount)
                                   .arg(failCount);
          } else {
              summaryMessage = QString("Silence Trim Complete.\n\nTotal Selected: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                                   .arg(totalFilesToProcess)
                                   .arg(successCount)
                                   .arg(failCount);
          }

          qInfo() << "-------------------------------------";
          qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
          qInfo() << "-------------------------------------";

          QMessageBox::information(this, "Operation Summary", summaryMessage);


   }



   if(accao==6){

       //trim eXtreme 1
       // --- Check for SOX executable ---
          QString soxPath = QStandardPaths::findExecutable("sox");
          if (soxPath.isEmpty()) {
              qWarning() << "'sox' command not found in system PATH.";
              QMessageBox::critical(this, "Missing Dependency",
                                    "The 'sox' command (Sound eXchange) is required for this feature "
                                    "but was not found in the system's PATH.\n\nPlease install SoX and ensure it's accessible.");
              return;
          }
          qInfo() << "Found sox executable at:" << soxPath;
          // --- End SOX check ---

          // --- Get Selected Files ---
          QItemSelectionModel *selectionModel = ui->musicView->selectionModel();
          if (!selectionModel) {
              qWarning() << "Music view selection model is null.";
              return;
          }
          QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
          QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
          for (const QModelIndex &index : selectedIndexes) {
              uniqueRows.insert(index.row());
          }

          if (uniqueRows.isEmpty()) {
              QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to trim silence.");
              return;
          }

          int totalFilesToProcess = uniqueRows.size();
          qInfo() << "Found" << totalFilesToProcess << "unique rows selected for silence trimming (0.2s threshold).";

          // --- Confirmation ---
          // Updated confirmation message slightly
          QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Silence Trim",
                                           QString("Trim silence (0.2s duration, below 1% threshold) from the start and end of %1 selected track(s)?\n\n"
                                                   "Original files will be overwritten!\n\n"
                                                   "This action cannot be undone.")
                                               .arg(totalFilesToProcess),
                                           QMessageBox::Yes | QMessageBox::No);

          if (confirm == QMessageBox::No) {
              return;
          }

          // --- Setup Progress Dialog ---
          QProgressDialog progressDialog("Trimming silence (0.2s threshold)...", "Cancel", 0, totalFilesToProcess, this);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setValue(0);
          progressDialog.show();
          qApp->processEvents(); // Allow UI to update

          // --- Get Temporary Directory ---
          QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
          if (tempDirPath.isEmpty()) {
               qWarning() << "Could not find a writable temporary location.";
               QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for processing.");
               progressDialog.cancel();
               return;
          }
          QDir tempDir(tempDirPath);
          qInfo() << "Using temporary directory:" << tempDirPath;

          // --- Process Files ---
          int successCount = 0;
          int failCount = 0;
          bool cancelled = false;
          QAbstractItemModel *model = ui->musicView->model();
          if (!model) {
              qWarning() << "Music view model is null.";
              QMessageBox::critical(this, "Internal Error", "Cannot access the data model.");
              progressDialog.cancel();
              return;
          }

          QList<int> rowsToProcess = uniqueRows.values();
          std::sort(rowsToProcess.begin(), rowsToProcess.end());

          for (int row : rowsToProcess) {
              if (progressDialog.wasCanceled()) {
                  cancelled = true;
                  break;
              }

              progressDialog.setValue(successCount + failCount);
              qApp->processEvents();

              QModelIndex pathIndex = model->index(row, 7); // Assuming column 7 holds the path
              if (!pathIndex.isValid()) {
                   qWarning() << "Invalid model index for row" << row << ", column 7.";
                   failCount++;
                   continue;
              }
              QString originalPath = model->data(pathIndex).toString();
              QFileInfo originalFileInfo(originalPath);

              progressDialog.setLabelText(QString("Trimming [%1/%2]:\n%3")
                                              .arg(successCount + failCount + 1)
                                              .arg(totalFilesToProcess)
                                              .arg(originalFileInfo.fileName()));
              qApp->processEvents();

              qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

              if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
                  qWarning() << "Original file does not exist or is not a file:" << originalPath;
                  failCount++;
                  continue;
              }

              // --- Prepare Paths ---
              QString baseName = originalFileInfo.fileName();
              QString tempOutputPath = tempDir.filePath(baseName);
              qDebug() << "Temporary output path:" << tempOutputPath;

              // --- Run SOX ---
              QProcess soxProcess;
              QStringList soxArgs;
              // ***** THE ONLY CHANGE IS HERE *****
              soxArgs << originalPath                   // Input file
                      << tempOutputPath                 // Output file
                      << "silence"                      // Effect name
                      << "1"                            // Detect silence around the file (mode)
                      << "0.2"                          // Stop point (duration in seconds) - MODIFIED
                      << "1%";                          // Threshold (e.g., 0.1% or 1%)

              qDebug() << "Running command:" << soxPath << soxArgs;

              soxProcess.start(soxPath, soxArgs);

              if (!soxProcess.waitForFinished(300000)) { // 5 min timeout
                  qWarning() << "Sox process timed out for:" << originalPath << soxProcess.errorString();
                  soxProcess.kill();
                  soxProcess.waitForFinished(1000);
                  QFile::remove(tempOutputPath);
                  failCount++;
                  continue;
              }

              if (soxProcess.exitStatus() != QProcess::NormalExit || soxProcess.exitCode() != 0) {
                  qWarning() << "Sox process failed for:" << originalPath
                             << "Exit code:" << soxProcess.exitCode()
                             << "Exit status:" << soxProcess.exitStatus();
                  QString errorOutput = QString::fromLocal8Bit(soxProcess.readAllStandardError());
                  qWarning() << "Sox Standard Error:\n" << errorOutput;
                  QFile::remove(tempOutputPath);
                  failCount++;
                  continue;
              }

              // --- Check if temp file was created and is valid ---
              QFileInfo tempFileInfo(tempOutputPath);
              if (!tempFileInfo.exists() || tempFileInfo.size() == 0) {
                   qWarning() << "Sox finished successfully but the output file is missing or empty:" << tempOutputPath;
                   QFile::remove(tempOutputPath);
                   failCount++;
                   continue;
              }

              qInfo() << "Sox completed successfully for:" << originalPath << " -> " << tempOutputPath;

              // --- Replace original file with trimmed version ---
              qInfo() << "Attempting to replace" << originalPath << "with" << tempOutputPath;
              if (!QFile::rename(tempOutputPath, originalPath)) {
                   qWarning() << "Failed to replace original file using rename:" << originalPath << "with temp file:" << tempOutputPath;
                   QMessageBox::warning(this, "Replacement Failed",
                                       QString("Could not replace the original file:\n%1\n\n"
                                               "The trimmed version remains in the temporary folder:\n%2\n\n"
                                               "Please check file permissions and replace it manually if desired.")
                                           .arg(originalPath).arg(tempOutputPath));
                  failCount++;
                  // Do NOT delete the temp file in this case
              } else {
                  qInfo() << "File trimmed and replaced successfully:" << originalPath;
                  successCount++;
                  // Temp file is gone (renamed)
              }
              // --- End Replace ---

          } // End for loop

          // --- Final Cleanup & Summary ---
          progressDialog.cancel(); // Close progress dialog

          // Adjust summary message slightly
          QString summaryMessage;
          if (cancelled) {
              summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Trimmed (0.2s): %1\nFailed/Skipped: %2")
                                   .arg(successCount)
                                   .arg(failCount);
          } else {
              summaryMessage = QString("Silence Trim Complete (0.2s threshold).\n\nTotal Selected: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                                   .arg(totalFilesToProcess)
                                   .arg(successCount)
                                   .arg(failCount);
          }

          qInfo() << "-------------------------------------";
          qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
          qInfo() << "-------------------------------------";

          QMessageBox::information(this, "Operation Summary", summaryMessage);


   }





}

void player::on_actionConvert_all_musics_in_the_database_to_mp3_triggered()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- Check for FFMPEG executable ---
    QString ffmpegPath = FxEngine::ffmpegExecutable();
    if (ffmpegPath.isEmpty()) {
        qWarning() << "'ffmpeg' command not found in system PATH.";
        QMessageBox::critical(this, "Missing Dependency",
                              "The 'ffmpeg' command is required for audio conversion "
                              "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
        return;
    }
    qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
    // --- End FFMPEG check ---

    // --- Confirmation ---
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Full Conversion",
                                     "Convert ALL tracks in the database to MP3 (192kbps)?\n\n"
                                     "Original files will be replaced with the MP3 version.\n"
                                     "This action cannot be undone and may take a very long time!\n\n"
                                     "Note: Only the audio stream will be kept.",
                                     QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::No) {
        return;
    }

    // --- Setup Loading Indicator & Progress Dialog ---
    // Use stack variable for QMovie if its lifetime is limited to this function
    QMovie loadingMovie(":/images/loading.gif");
    if(!loadingMovie.isValid()){
         qWarning() << "Loading GIF not valid:" << ":/images/loading.gif";
         ui->txt_loading->setText("Processing..."); // Fallback text
         ui->txt_loading->show();
    } else {
        ui->txt_loading->setMovie(&loadingMovie);
        ui->txt_loading->setScaledContents(true);
        ui->txt_loading->show();
        loadingMovie.start();
    }
    qApp->processEvents(); // Allow UI to update

    QProgressDialog progressDialog("Converting all tracks to MP3...", "Cancel", 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();
    qApp->processEvents();
    // --- End Setup ---

    // --- Prepare Database Queries (outside loop) ---
    QSqlQuery querySelect(db);
    QSqlQuery queryUpdate(db);
    QSqlQuery queryCount(db); // For counting total records

    // Prepare UPDATE statement
    if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
        qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // Count total files for progress bar
    int totalFiles = 0;
    if (queryCount.exec("SELECT COUNT(*) FROM musics")) {
        if (queryCount.next()) {
            totalFiles = queryCount.value(0).toInt();
            progressDialog.setMaximum(totalFiles);
        }
    } else {
        qWarning() << "Failed to count records:" << queryCount.lastError();
        // Proceed without accurate progress max? Or abort? Let's proceed.
        progressDialog.setMaximum(100); // Set an arbitrary max
    }
    qApp->processEvents();


    // Select only the path
    QString selectStr = "SELECT path FROM musics";
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT paths from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table for paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // --- Get Temporary Directory ---
    QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDirPath.isEmpty()) {
         qWarning() << "Could not find a writable temporary location.";
         QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
         loadingMovie.stop();
         ui->txt_loading->hide();
         progressDialog.cancel();
         return;
    }
    QDir tempDir(tempDirPath);
    qInfo() << "Using temporary directory:" << tempDirPath;


    // --- Process Files ---
    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;
    bool cancelled = false;

    while (querySelect.next() && !cancelled) {
        processedCount++;
        progressDialog.setValue(processedCount);
        qApp->processEvents(); // Keep UI responsive and check for Cancel button

        if (progressDialog.wasCanceled()) {
            qInfo() << "User cancelled the operation.";
            cancelled = true;
            break;
        }

        QString originalPath = querySelect.value(0).toString();
        QFileInfo originalFileInfo(originalPath);

        progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                        .arg(processedCount)
                                        .arg(totalFiles > 0 ? totalFiles : processedCount) // Show total if known
                                        .arg(originalFileInfo.fileName()));
        qApp->processEvents();

        qInfo().noquote() << "Processing [" << processedCount << "/" << (totalFiles > 0 ? QString::number(totalFiles): "?") << "]:" << originalPath;

        if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
            qWarning() << "Original file does not exist or is not a file:" << originalPath;
            failCount++;
            continue; // Skip to next file
        }

        // --- Prepare Paths ---
        QString baseName = originalFileInfo.baseName(); // Name without extension
        QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
        QString tempMp3Path = tempDir.filePath(baseName + ".mp3"); // Full path for temporary MP3
        QString finalMp3Path = QDir(originalDir).filePath(baseName + ".mp3"); // Final path for MP3 in original dir

        // --- Run FFMPEG ---
        QProcess ffmpegProcess;
        QStringList ffmpegArgs;
        ffmpegArgs << "-i" << originalPath // Input file
                   << "-vn"               // No video output
                   << "-ar" << "44100"    // Audio sample rate
                   << "-ac" << "2"        // Audio channels (stereo)
                   << "-b:a" << "192k"    // Audio bitrate (use -b:a)
                   << "-f" << "mp3"       // Force output format (optional)
                   << tempMp3Path;        // Output file path

        qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

        ffmpegProcess.start(ffmpegPath, ffmpegArgs);

        // Wait for ffmpeg (e.g., 10 min timeout)
        if (!ffmpegProcess.waitForFinished(600000)) {
            qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
            ffmpegProcess.kill();
            ffmpegProcess.waitForFinished(1000);
            QFile::remove(tempMp3Path); // Clean up incomplete temp file
            failCount++;
            continue;
        }

        // Check ffmpeg result
        if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
            qWarning() << "ffmpeg process failed for:" << originalPath
                       << "Exit code:" << ffmpegProcess.exitCode()
                       << "Exit status:" << ffmpegProcess.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
            qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
             QFile::remove(tempMp3Path); // Clean up potentially failed temp file
            failCount++;
            continue;
        }

        // Check if temp MP3 exists and has size
         QFileInfo tempMp3Info(tempMp3Path);
         if (!tempMp3Info.exists() || tempMp3Info.size() == 0) {
             qWarning() << "ffmpeg finished successfully but the output MP3 file is missing or empty:" << tempMp3Path;
             QFile::remove(tempMp3Path);
             failCount++;
             continue;
         }

        qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempMp3Path;

        // --- Delete Original File ---
        qInfo() << "Attempting to delete original file:" << originalPath;
        if (!QFile::remove(originalPath)) {
            qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
            failCount++;
            continue; // Skip rest of steps for this file
        }
        qInfo() << "Original file deleted successfully.";

        // --- Move Temporary MP3 to Final Location ---
        qInfo() << "Attempting to move" << tempMp3Path << "to" << finalMp3Path;
        if (!QFile::rename(tempMp3Path, finalMp3Path)) {
            qWarning() << "Failed to move temporary MP3" << tempMp3Path << "to" << finalMp3Path << ". The original file was deleted! MP3 remains in temp folder.";
            QMessageBox::warning(this, "Move Failed",
                                 QString("Failed to move the converted MP3 to its final destination:\n%1\n\n"
                                         "The original file was deleted, but the MP3 remains in the temporary folder:\n%2\n\n"
                                         "Please move it manually and check the database record.")
                                     .arg(finalMp3Path).arg(tempMp3Path));
            failCount++;
            continue; // Cannot update DB path if move failed
        }
        qInfo() << "MP3 moved successfully to:" << finalMp3Path;

        // --- Update Database ---
        qInfo() << "Updating database: set path =" << finalMp3Path << "where path =" << originalPath;
        queryUpdate.bindValue(":new_path", finalMp3Path);
        queryUpdate.bindValue(":old_path", originalPath);

        if (!queryUpdate.exec()) {
            qWarning() << "Failed to update database path for:" << originalPath << "->" << finalMp3Path;
            qWarning() << "DB Error:" << queryUpdate.lastError().text();
            qWarning() << "Last Query (Bound values might not show):" << queryUpdate.lastQuery();
            QMessageBox::warning(this, "Database Update Failed",
                                 QString("The file was successfully converted and moved to:\n%1\n\n"
                                         "However, updating the database record failed:\n%2\n\n"
                                         "Please check the database manually.")
                                     .arg(finalMp3Path).arg(queryUpdate.lastError().text()));
            failCount++; // Count as failure since DB update is critical
        } else {
            qInfo() << "Database path updated successfully.";
            successCount++;
            // update_music_table(); // Update only once at the end
        }

    } // End while loop

    // --- Final Cleanup & Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close progress dialog

    update_music_table(); // Update the table view once after all operations

    QString summaryMessage;
    if (cancelled) {
        summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                             .arg(processedCount -1) // Don't count the one being processed when cancelled
                             .arg(successCount)
                             .arg(failCount);
    } else {
        summaryMessage = QString("MP3 Conversion Complete.\n\nTotal Records: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                             .arg(totalFiles) // Use the count obtained earlier
                             .arg(successCount)
                             .arg(failCount);
    }

    qInfo() << "-------------------------------------";
    qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Conversion Summary", summaryMessage);
}
void player::on_actionConvert_all_musics_in_the_database_to_ogg_triggered()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- Check for FFMPEG executable ---
    QString ffmpegPath = FxEngine::ffmpegExecutable();
    if (ffmpegPath.isEmpty()) {
        qWarning() << "'ffmpeg' command not found in system PATH.";
        QMessageBox::critical(this, "Missing Dependency",
                              "The 'ffmpeg' command is required for audio conversion "
                              "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
        return;
    }
    qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
    // --- End FFMPEG check ---

    // --- Confirmation ---
    // Updated confirmation message for Ogg
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Full Conversion",
                                     "Convert ALL tracks in the database to Ogg Vorbis (Quality ~7)?\n\n"
                                     "Original files will be replaced with the Ogg version.\n"
                                     "This action cannot be undone and may take a very long time!\n\n"
                                     "Note: Only the audio stream will be kept.",
                                     QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::No) {
        return;
    }

    // --- Setup Loading Indicator & Progress Dialog ---
    QMovie loadingMovie(":/images/loading.gif");
    if(!loadingMovie.isValid()){
         qWarning() << "Loading GIF not valid:" << ":/images/loading.gif";
         ui->txt_loading->setText("Processing..."); // Fallback text
         ui->txt_loading->show();
    } else {
        ui->txt_loading->setMovie(&loadingMovie);
        ui->txt_loading->setScaledContents(true);
        ui->txt_loading->show();
        loadingMovie.start();
    }
    qApp->processEvents(); // Allow UI to update

    // Updated progress dialog title
    QProgressDialog progressDialog("Converting all tracks to Ogg Vorbis...", "Cancel", 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();
    qApp->processEvents();
    // --- End Setup ---

    // --- Prepare Database Queries (outside loop) ---
    QSqlQuery querySelect(db);
    QSqlQuery queryUpdate(db);
    QSqlQuery queryCount(db); // For counting total records

    // Prepare UPDATE statement
    if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
        qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // Count total files for progress bar
    int totalFiles = 0;
    if (queryCount.exec("SELECT COUNT(*) FROM musics")) {
        if (queryCount.next()) {
            totalFiles = queryCount.value(0).toInt();
            progressDialog.setMaximum(totalFiles);
        }
    } else {
        qWarning() << "Failed to count records:" << queryCount.lastError();
        progressDialog.setMaximum(100); // Set an arbitrary max
    }
    qApp->processEvents();


    // Select only the path
    QString selectStr = "SELECT path FROM musics";
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT paths from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table for paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // --- Get Temporary Directory ---
    QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDirPath.isEmpty()) {
         qWarning() << "Could not find a writable temporary location.";
         QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
         loadingMovie.stop();
         ui->txt_loading->hide();
         progressDialog.cancel();
         return;
    }
    QDir tempDir(tempDirPath);
    qInfo() << "Using temporary directory:" << tempDirPath;


    // --- Process Files ---
    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;
    bool cancelled = false;

    while (querySelect.next() && !cancelled) {
        processedCount++;
        progressDialog.setValue(processedCount);
        qApp->processEvents(); // Keep UI responsive and check for Cancel button

        if (progressDialog.wasCanceled()) {
            qInfo() << "User cancelled the operation.";
            cancelled = true;
            break;
        }

        QString originalPath = querySelect.value(0).toString();
        QFileInfo originalFileInfo(originalPath);

        // Updated progress label
        progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                        .arg(processedCount)
                                        .arg(totalFiles > 0 ? totalFiles : processedCount) // Show total if known
                                        .arg(originalFileInfo.fileName()));
        qApp->processEvents();

        qInfo().noquote() << "Processing [" << processedCount << "/" << (totalFiles > 0 ? QString::number(totalFiles): "?") << "]:" << originalPath;

        if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
            qWarning() << "Original file does not exist or is not a file:" << originalPath;
            failCount++;
            continue; // Skip to next file
        }

        // --- Prepare Paths ---
        QString baseName = originalFileInfo.baseName(); // Name without extension
        QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
        // Ogg paths
        QString tempOggPath = tempDir.filePath(baseName + ".ogg"); // Full path for temporary Ogg
        QString finalOggPath = QDir(originalDir).filePath(baseName + ".ogg"); // Final path for Ogg in original dir

        // --- Run FFMPEG ---
        QProcess ffmpegProcess;
        QStringList ffmpegArgs;
        // Args for Ogg Vorbis
        ffmpegArgs << "-i" << originalPath           // Input file
                   << "-c:a" << "libvorbis"        // Audio codec: libvorbis
                   << "-qscale:a" << "7"           // Quality scale (adjust as needed)
                   << tempOggPath;                 // Output file path

        qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

        ffmpegProcess.start(ffmpegPath, ffmpegArgs);

        // Wait for ffmpeg (e.g., 10 min timeout)
        if (!ffmpegProcess.waitForFinished(600000)) {
            qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
            ffmpegProcess.kill();
            ffmpegProcess.waitForFinished(1000);
            QFile::remove(tempOggPath); // Clean up incomplete temp file
            failCount++;
            continue;
        }

        // Check ffmpeg result
        if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
            qWarning() << "ffmpeg process failed for:" << originalPath
                       << "Exit code:" << ffmpegProcess.exitCode()
                       << "Exit status:" << ffmpegProcess.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
            qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
             QFile::remove(tempOggPath); // Clean up potentially failed temp file
            failCount++;
            continue;
        }

        // Check if temp Ogg exists and has size
         QFileInfo tempOggInfo(tempOggPath);
         if (!tempOggInfo.exists() || tempOggInfo.size() == 0) {
             qWarning() << "ffmpeg finished successfully but the output Ogg file is missing or empty:" << tempOggPath;
             QFile::remove(tempOggPath);
             failCount++;
             continue;
         }

        qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempOggPath;

        // --- Delete Original File ---
        qInfo() << "Attempting to delete original file:" << originalPath;
        if (!QFile::remove(originalPath)) {
            qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
            failCount++;
            continue; // Skip rest of steps for this file
        }
        qInfo() << "Original file deleted successfully.";

        // --- Move Temporary Ogg to Final Location ---
        qInfo() << "Attempting to move" << tempOggPath << "to" << finalOggPath;
        if (!QFile::rename(tempOggPath, finalOggPath)) {
            qWarning() << "Failed to move temporary Ogg" << tempOggPath << "to" << finalOggPath << ". The original file was deleted! Ogg remains in temp folder.";
            QMessageBox::warning(this, "Move Failed",
                                 QString("Failed to move the converted Ogg file to its final destination:\n%1\n\n"
                                         "The original file was deleted, but the Ogg file remains in the temporary folder:\n%2\n\n"
                                         "Please move it manually and check the database record.")
                                     .arg(finalOggPath).arg(tempOggPath));
            failCount++;
            continue; // Cannot update DB path if move failed
        }
        qInfo() << "Ogg file moved successfully to:" << finalOggPath;

        // --- Update Database ---
        qInfo() << "Updating database: set path =" << finalOggPath << "where path =" << originalPath;
        queryUpdate.bindValue(":new_path", finalOggPath);
        queryUpdate.bindValue(":old_path", originalPath);

        if (!queryUpdate.exec()) {
            qWarning() << "Failed to update database path for:" << originalPath << "->" << finalOggPath;
            qWarning() << "DB Error:" << queryUpdate.lastError().text();
            qWarning() << "Last Query (Bound values might not show):" << queryUpdate.lastQuery();
            QMessageBox::warning(this, "Database Update Failed",
                                 QString("The file was successfully converted and moved to:\n%1\n\n"
                                         "However, updating the database record failed:\n%2\n\n"
                                         "Please check the database manually.")
                                     .arg(finalOggPath).arg(queryUpdate.lastError().text()));
            failCount++; // Count as failure since DB update is critical
        } else {
            qInfo() << "Database path updated successfully.";
            successCount++;
            // update_music_table(); // Update only once at the end
        }

    } // End while loop

    // --- Final Cleanup & Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close progress dialog

    update_music_table(); // Update the table view once after all operations

    // Updated summary message
    QString summaryMessage;
    if (cancelled) {
        summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Converted to Ogg: %2\nFailed/Skipped: %3")
                             .arg(processedCount -1)
                             .arg(successCount)
                             .arg(failCount);
    } else {
        summaryMessage = QString("Ogg Conversion Complete.\n\nTotal Records: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                             .arg(totalFiles) // Use the count obtained earlier
                             .arg(successCount)
                             .arg(failCount);
    }

    qInfo() << "-------------------------------------";
    qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Conversion Summary", summaryMessage);
}
void player::on_bt_start_streaming_clicked()
{
    qDebug()<<"Starting the streaming!";

    on_bt_icecast_clicked();

    on_bt_butt_clicked();

    ddnsUpdate();

    on_bt_portTest_clicked();
butt_timmer();
ice_timmer();

/*
    connect(stimer, SIGNAL(timeout()), this, SLOT(streaming_timmer()));
    stimer->start(5000);
*/
}
void player::on_bt_stop_streaming_clicked()
{
    if(stimer) stimer->stop();
    if(icetimer) icetimer->stop();
    if(butt_timer) butt_timer->stop();

    qInfo() << "Attempting to stop streaming processes...";

    // --- Stop Icecast ---
    QProcess killer_icecast;
    QString icecast_cmd;
    QStringList icecast_args;
    bool kill_success_icecast = false;

#ifdef Q_OS_WIN
    icecast_cmd = "taskkill";
    icecast_args << "/F" << "/IM" << "icecast.exe"; // Adjust executable name if needed
#else // Linux, macOS, other Unix-like
    // Prefer pkill if available, fallback to killall might be needed if pkill isn't standard
    icecast_cmd = "pkill"; // Or "killall" as a fallback
    icecast_args << "icecast"; // Process name
#endif

    qDebug() << "Running kill command for icecast:" << icecast_cmd << icecast_args;
    killer_icecast.start(icecast_cmd, icecast_args);
    if (killer_icecast.waitForFinished(3000)) { // Wait up to 3 seconds
        if (killer_icecast.exitStatus() == QProcess::NormalExit && killer_icecast.exitCode() == 0) {
            qInfo() << "Icecast process killed successfully (or was not running).";
            kill_success_icecast = true; // Assume success if command ran ok
        } else {
            qWarning() << "Kill command for icecast failed or process not found. Exit code:" << killer_icecast.exitCode();
            // Might log stderr: QString err = killer_icecast.readAllStandardError(); qDebug() << err;
        }
    } else {
        qWarning() << "Kill command for icecast timed out.";
        killer_icecast.kill(); // Kill the killer process itself
    }

    // Update UI regardless of success? Or only on success? Let's update anyway.
    ui->lbl_icecast->setText("Stopped");
    ui->lbl_icecast->setStyleSheet("color:blue;"); // Ensure CSS syntax is correct
    ui->bt_icecast->setStyleSheet("");


    // --- Stop Butt ---
    QProcess killer_butt;
    QString butt_cmd;
    QStringList butt_args;
    bool kill_success_butt = false;

#ifdef Q_OS_WIN
    butt_cmd = "taskkill";
    butt_args << "/F" << "/IM" << "butt.exe"; // Adjust executable name if needed
#else // Linux, macOS, other Unix-like
    butt_cmd = "pkill"; // Or "killall"
    butt_args << "butt"; // Process name
#endif

    qDebug() << "Running kill command for butt:" << butt_cmd << butt_args;
    killer_butt.start(butt_cmd, butt_args);
    if (killer_butt.waitForFinished(3000)) { // Wait up to 3 seconds
        if (killer_butt.exitStatus() == QProcess::NormalExit && killer_butt.exitCode() == 0) {
            qInfo() << "Butt process killed successfully (or was not running).";
            kill_success_butt = true;
        } else {
            qWarning() << "Kill command for butt failed or process not found. Exit code:" << killer_butt.exitCode();
        }
    } else {
        qWarning() << "Kill command for butt timed out.";
        killer_butt.kill();
    }

    ui->lbl_butt->setText("Stopped");
    ui->lbl_butt->setStyleSheet("color:blue;");
    ui->bt_butt->setStyleSheet("");

    // --- Update Other UI ---
    ui->lbl_port->setText("Stopped");
    ui->lbl_port->setStyleSheet("color:blue;");

    ui->bt_takeOver->setStyleSheet("");
    ui->bt_takeOver->setEnabled(false);

    // Assuming txt_ProgramName is meant to be hidden when stopped
    ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
    ui->txt_ProgramName->hide();

    piscaLive = false; // Assuming piscaLive is a member variable bool
}
void player::streaming_timmer(){

    qDebug()<<"Running streaming_timmer (checking external processes...)";

    // --- Check Icecast ---
    QProcess check_icecast;
    QString icecast_check_cmd;
    QStringList icecast_check_args;
    bool icecast_running = false;

#ifdef Q_OS_WIN
    icecast_check_cmd = "tasklist";
    icecast_check_args << "/NH" << "/FI" << "IMAGENAME eq icecast.exe"; // Adjust image name
#else // Linux, macOS, other Unix-like
    icecast_check_cmd = "pgrep";
    icecast_check_args << "-x" << "icecast"; // Match exact process name
#endif

    check_icecast.start(icecast_check_cmd, icecast_check_args);
    if (check_icecast.waitForFinished(1000)) { // Short timeout
#ifdef Q_OS_WIN
        // Tasklist often returns 0 even if not found, check output
        QString output = check_icecast.readAllStandardOutput();
        if (output.contains("icecast.exe", Qt::CaseInsensitive)) {
            icecast_running = true;
        }
#else // pgrep check
        if (check_icecast.exitStatus() == QProcess::NormalExit && check_icecast.exitCode() == 0) {
            // pgrep returns 0 if process(es) found
            icecast_running = true;
        }
#endif
    } else {
         qWarning() << "Check command for icecast timed out or failed.";
    }

    // Update Icecast UI
    if(icecast_running){
        ui->lbl_icecast->setText("Running");
        ui->lbl_icecast->setStyleSheet("color:green;");
    } else {
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:red;");
         ui->bt_takeOver->setEnabled(false); // Disable takeover if icecast stopped
    }


    // --- Check Butt ---
    QProcess check_butt;
    QString butt_check_cmd;
    QStringList butt_check_args;
    bool butt_running = false;

#ifdef Q_OS_WIN
    butt_check_cmd = "tasklist";
    butt_check_args << "/NH" << "/FI" << "IMAGENAME eq butt.exe"; // Adjust image name
#else // Linux, macOS, other Unix-like
    butt_check_cmd = "pgrep";
    butt_check_args << "-x" << "butt"; // Match exact process name
#endif

    check_butt.start(butt_check_cmd, butt_check_args);
     if (check_butt.waitForFinished(1000)) { // Short timeout
 #ifdef Q_OS_WIN
        QString output = check_butt.readAllStandardOutput();
        if (output.contains("butt.exe", Qt::CaseInsensitive)) {
             butt_running = true;
         }
 #else // pgrep check
         if (check_butt.exitStatus() == QProcess::NormalExit && check_butt.exitCode() == 0) {
             butt_running = true;
         }
 #endif
    } else {
         qWarning() << "Check command for butt timed out or failed.";
     }

    // Update Butt UI
    if(butt_running){
        ui->lbl_butt->setText("Running");
        ui->lbl_butt->setStyleSheet("color:green;");
    } else {
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:red;");
        ui->bt_takeOver->setEnabled(false); // Also disable if butt stopped? Decide based on logic.
    }
}
void player::ddnsUpdate() {
    qDebug() << "Requesting external IP address...";

    // Ensure networkManager is initialized (e.g., in constructor)
    if (!networkManager) {
         qWarning() << "Network manager not initialized!";
         networkManager = new QNetworkAccessManager(this); // Lazy init? Or handle error better.
    }

    // Use HTTPS for security. icanhazip supports it.
    QUrl url("https://ipv4.icanhazip.com");
    QNetworkRequest request(url);

    // Set a timeout for the request (e.g., 10 seconds)
    request.setTransferTimeout(10000);

    QNetworkReply *reply = networkManager->get(request);

    // Connect the finished signal to a lambda or a dedicated slot
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString externalIp = "Error"; // Default in case of failure

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            externalIp = QString::fromUtf8(responseData).trimmed(); // Assume UTF-8, trim whitespace
            qInfo() << "External IP determined:" << externalIp;
        } else {
            qWarning() << "Failed to get external IP:" << reply->errorString();
            externalIp = "Error: " + reply->errorString(); // Provide error info in UI
        }

        // Update the UI label
        ui->lbl_ddns->setText(externalIp);
        // Consider styling based on success/error?
        // if (reply->error() != QNetworkReply::NoError) {
        //     ui->lbl_ddns->setStyleSheet("color: red;");
        // } else {
        //     ui->lbl_ddns->setStyleSheet(""); // Clear style
        // }


        // IMPORTANT: Clean up the reply object to prevent memory leaks
        reply->deleteLater();
    });

    // Optional: Connect errorOccurred signal for more immediate feedback?
    // connect(reply, &QNetworkReply::errorOccurred, this, ...);

    // Optional: Connect sslErrors signal if needed for debugging HTTPS issues
    // connect(reply, &QNetworkReply::sslErrors, this, ...);
}

void player::on_horizontalSlider_lps_vol_sliderMoved(int position)
{
    qDebug()<<"LPS Horisontal Slider moved to: "<<position;


    if(position==100){
        lp1_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
        lp2_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume

        ui->lbl_lp1_vol_level->setText("100");
        ui->lbl_lp2_vol_level->setText("100");
    }


    if(position<=99){

        lp1_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
        lp2_XplayerOutput->setVolume(position / 100.0); // Qt6 uses 0.0-1.0 range for volume

        ui->lbl_lp1_vol_level->setText("100");

        QString np = QString::number(position);

        ui->lbl_lp2_vol_level->setText(np);


    }

    if(position>=101){

        lp2_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume

        int lp1_volume = 200-position;
        lp1_XplayerOutput->setVolume(lp1_volume / 100.0); // Qt6 uses 0.0-1.0 range for volume

        QString lp1_lbl_vol = QString::number(lp1_volume);
        ui->lbl_lp1_vol_level->setText(lp1_lbl_vol);

        ui->lbl_lp2_vol_level->setText("100");


    }

}

void player::on_bt_center_lps_vol_clicked()
{
    lp1_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
    lp2_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume

    ui->lbl_lp1_vol_level->setText("100");
    ui->lbl_lp2_vol_level->setText("100");

    ui->horizontalSlider_lps_vol->setSliderPosition(100);
}


void player::on_txt_search_returnPressed()
{
    //search
    qDebug()<<"Staring a new search!";
    QString term = ui->txt_search->text();

    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery searchQuery(QSqlDatabase::database("xfb_connection"));
    searchQuery.prepare("select * from musics where artist like :t1 or song like :t2");
    searchQuery.bindValue(":t1", "%" + term + "%");
    searchQuery.bindValue(":t2", "%" + term + "%");
    searchQuery.exec();
    model->setQuery(std::move(searchQuery));
    ui->musicView->setModel(model);

    ui->musicView->setSortingEnabled(true);
    ui->musicView->hideColumn(0);
    ui->musicView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

void player::on_actionForce_monitorization_triggered()
{
    server_check_and_schedule_new_programs();
    run_server_scheduler();
}



void player::on_actionUpdate_Dinamic_Server_s_IP_triggered()
{
    // --- Confirmation ---
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm .netrc Update",
                                     QString("This will fetch the server IP from:\n%1\n\n"
                                             "It will then **overwrite** your ~/.netrc file with credentials for this server:\n"
                                             "machine [Server IP]\nlogin %2\npassword [Your Password]\n\n"
                                             "Any other entries in ~/.netrc will be lost! Proceed?")
                                         .arg(Server_URL + "/XFB/Config/ftpupdate.txt") // Show the full URL
                                         .arg(User), // Show the username being written
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (confirm != QMessageBox::Yes) {
        qInfo() << ".netrc update cancelled by user.";
        return;
    }


    // --- Network Request ---
    qInfo() << "Requesting server IP configuration...";

    if (!networkManager) {
         qWarning() << "Network manager not initialized!";
         // Initialize it here if necessary, or ensure it's done in the constructor
         networkManager = new QNetworkAccessManager(this);
    }

    // Construct the full URL
    QUrl configUrl(Server_URL); // Assuming Server_URL is like "http://example.com"
    if (!configUrl.isValid() || Server_URL.isEmpty()) {
        qWarning() << "Server_URL is invalid or empty:" << Server_URL;
        QMessageBox::critical(this, "Configuration Error", "The Server URL configured in the application is invalid.");
        return;
    }
    configUrl.setPath("/XFB/Config/ftpupdate.txt"); // Append the path

    QNetworkRequest request(configUrl);
    request.setTransferTimeout(15000); // 15 second timeout

    qDebug() << "Fetching configuration from:" << request.url().toString();

    QNetworkReply *reply = networkManager->get(request);

    // Connect the finished signal (asynchronous handling)
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // --- Handle Network Response ---
        QString serverIp; // Variable to store the successfully retrieved IP

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QString responseString = QString::fromUtf8(responseData).trimmed(); // Assume UTF-8

            // Basic parsing: Assume IP is the first non-empty line
            QStringList lines = responseString.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

            if (!lines.isEmpty()) {
                serverIp = lines.first().trimmed(); // Take the first line and trim whitespace
                if (serverIp.isEmpty()) {
                     qWarning() << "Received empty first line from config URL:" << reply->url().toString();
                     QMessageBox::warning(this, "Update Failed", "Received empty data from the server configuration URL.");
                } else {
                     qInfo() << "Server IP determined:" << serverIp;
                }
            } else {
                qWarning() << "Received empty response from config URL:" << reply->url().toString();
                QMessageBox::warning(this, "Update Failed", "Received empty response from the server configuration URL.");
            }
        } else {
            qWarning() << "Failed to fetch server config:" << reply->errorString() << "(" << reply->error() << ")";
            QMessageBox::critical(this, "Network Error", QString("Failed to fetch server configuration:\n%1").arg(reply->errorString()));
            reply->deleteLater(); // MUST cleanup reply object
            return; // Stop processing on network error
        }

        // --- Proceed only if IP was found ---
        if (serverIp.isEmpty()) {
            reply->deleteLater(); // MUST cleanup reply object
            return; // Stop if we couldn't get a valid IP
        }


        // --- Write to .netrc ---
        QString netrcPath = QDir::homePath() + "/.netrc";
        qInfo() << "Attempting to write to:" << netrcPath;

        QFile netrcFile(netrcPath);

        // Open for writing, overwrite existing content (Truncate)
        if (!netrcFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qWarning() << "Failed to open" << netrcPath << "for writing:" << netrcFile.errorString();
            QMessageBox::critical(this, "File Error", QString("Could not open the .netrc file for writing:\n%1\n\nPlease check permissions.").arg(netrcPath));
            reply->deleteLater();
            return;
        }

        QTextStream outStream(&netrcFile);
        QString line = QString("machine %1 login %2 password %3\n").arg(serverIp, User, Pass);
        outStream << line;
        outStream.flush(); // Ensure data is written

        if (outStream.status() != QTextStream::Ok) {
             qWarning() << "Error writing to .netrc file stream.";
             // File might be partially written
        }

        // Set strict permissions (Owner Read/Write only - 600)
        if (!netrcFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner)) {
             qWarning() << "Failed to set permissions (600) on" << netrcPath;
             // Not critical enough to fail the whole operation, but log it.
        }

        netrcFile.close(); // QTextStream destructor also closes it, but explicit is okay.

        qInfo() << ".netrc file updated successfully.";
        QMessageBox::information(this, "Update Successful", QString(".netrc file updated successfully for machine: %1").arg(serverIp));

        // --- Cleanup ---
        reply->deleteLater(); // MUST cleanup reply object
    });
}

void player::on_bt_add_some_random_songs_from_genre_clicked()
{

    QString selectedGenre = ui->comboBox_random_add_genre->currentText();

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QString num = ui->spinBox_num_of_songs_to_add_random->text();
checkDbOpen();
    QSqlQuery query(db);
    query.prepare("select path from musics where genre1 like :genre order by random() limit :num");
    query.bindValue(":genre", selectedGenre);
    query.bindValue(":num", num.toInt());
    if(query.exec())
    {
        qDebug() << "SQL query executed from 201606181026: " << query.lastQuery();

        while(query.next()){
            QString path = query.value(0).toString();


                ui->playlist->addItem(path);
                qDebug() << "autoMode genre based random music chooser from on_bt_add_some_random_songs_from_genre_clicked() adding: " << path;


            }



    } else {
        qDebug() << "SQL ERROR: " << query.lastError();
        qDebug() << "SQL was: " << query.lastQuery();


    }


}

void player::on_bt_icecast_clicked()
{
    // Use a local variable to track the intended state change during this function
    bool shouldBeRunning = !icecastrunning; // If currently false, we want it true, and vice-versa

    if (shouldBeRunning) {
        // --- Try to START Icecast ---
        qInfo() << "Attempting to start Icecast...";

        // 1. Force kill any existing instances first (as per original logic)
        qDebug() << "Ensuring no other icecast instances are running...";
        killProcessByName("icecast"); // Call helper function

        // 2. Find xvfb-run
        QString xvfbRunPath = QStandardPaths::findExecutable("xvfb-run");
        if (xvfbRunPath.isEmpty()) {
             qWarning() << "xvfb-run command not found in PATH. Cannot start icecast.";
             QMessageBox::critical(this, "Error", "Required command 'xvfb-run' not found. Cannot start Icecast.");
             return; // Cannot proceed
        }
        qDebug() << "Found xvfb-run at:" << xvfbRunPath;
        // Optional: Check if icecast executable itself exists?
        // QString icecastPath = QStandardPaths::findExecutable("icecast");
        // if (icecastPath.isEmpty()) { ... error ... }


        // 3. Prepare and start the process detached
        QProcess *icecastProcess = new QProcess(this); // Create on heap if needed elsewhere, else stack
        QStringList args;
        QString configPath = "/usr/local/etc/icecast.xml"; // Consider making this configurable
        args << "-a" << "icecast" << "-c" << configPath;

        qDebug() << "Starting detached process:" << xvfbRunPath << args;

        bool started = icecastProcess->startDetached(xvfbRunPath, args);
        // Note: startDetached only tells you if Qt could *invoke* the process,
        // not if xvfb-run or icecast started successfully internally.
        // The timer check (ice_timmer) is the real verification.

        if (started) {
            qInfo() << "Icecast process launch initiated.";
            icecastrunning = true; // Update state flag

            // Update UI to "Starting" or "Running" - timer will confirm "Running"
            ui->lbl_icecast->setText("Starting..."); // More accurate initial state
            ui->lbl_icecast->setStyleSheet("color:orange;"); // Indicate intermediate state
            ui->bt_icecast->setStyleSheet("background-color:#C8EE72;"); // Indicate "on" state

            // Connect and start the timer to check status
            if (icetimer) {
                // Disconnect first to avoid multiple connections if clicked rapidly
                disconnect(icetimer, &QTimer::timeout, this, &player::ice_timmer);
                connect(icetimer, &QTimer::timeout, this, &player::ice_timmer);
                icetimer->start(5000); // Check every 5 seconds
                QTimer::singleShot(500, this, &player::ice_timmer); // Check quickly once after starting
            } else {
                 qWarning() << "icetimer is null!";
            }
        } else {
            qWarning() << "Failed to initiate icecast process launch.";
            QMessageBox::critical(this, "Error", "Failed to start the Icecast process using xvfb-run.");
            // Don't change icecastrunning state or UI if launch failed
            delete icecastProcess; // Clean up if created on heap
        }

    } else {
        // --- Try to STOP Icecast ---
        qInfo() << "Attempting to stop Icecast...";

        // 1. Stop the status check timer
        if (icetimer) {
            icetimer->stop();
            // Optional: disconnect(icetimer, &QTimer::timeout, this, &player::ice_timmer);
        }

        // 2. Kill the process
        bool killed = killProcessByName("icecast"); // Call helper function

        // 3. Update state and UI regardless of kill success, as the intention is to stop
        icecastrunning = false;
        ui->bt_icecast->setStyleSheet(""); // Default style
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:blue;"); // Use correct CSS

        // Reset dependent UI elements (as per original logic)
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);
        ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
        ui->txt_ProgramName->hide();
        piscaLive = false;

        if (!killed) {
             qWarning() << "Kill command for icecast may have failed, but UI is set to stopped state.";
             // User might need to manually check if process actually stopped
        }
    }
}
void player::ice_timmer() {
    qDebug() << "Running ice_timmer (checking icecast process status...)";

    QProcess check_icecast;
    QString check_cmd;
    QStringList check_args;
    bool is_running = false; // Assume not running initially

#ifdef Q_OS_WIN
    // Windows: Use tasklist with image name filter
    check_cmd = "tasklist";
    // /NH for no header, /FI for filter. Adjust "icecast.exe" if needed.
    check_args << "/NH" << "/FI" << "IMAGENAME eq icecast.exe";
#else
    // Unix-like (Linux, macOS): Use pgrep with exact name match
    check_cmd = "pgrep";
    check_args << "-x" << "icecast"; // -x matches the name exactly
#endif

    qDebug() << "Executing check:" << check_cmd << check_args;
    check_icecast.start(check_cmd, check_args);

    // Wait for the check command to finish (short timeout)
    if (check_icecast.waitForFinished(1500)) { // 1.5 second timeout

#ifdef Q_OS_WIN
        // On Windows, tasklist usually exits with 0 even if not found when filtering.
        // We need to check if the output contains the process name.
        QByteArray output = check_icecast.readAllStandardOutput();
        // QByteArray::contains has no case-sensitivity overload; lowercase the
        // (ASCII) tasklist output and match the lowercase literal.
        if (output.toLower().contains("icecast.exe")) {
            is_running = true;
            qDebug() << "tasklist output indicates icecast is running.";
        } else {
             qDebug() << "tasklist output does not contain icecast.exe.";
             is_running = false;
        }
#else
        // On Unix-like, pgrep exits with 0 if found, non-zero otherwise.
        if (check_icecast.exitStatus() == QProcess::NormalExit && check_icecast.exitCode() == 0) {
            is_running = true;
            qDebug() << "pgrep found running icecast process (exit code 0).";
        } else {
            // Exit code 1 usually means not found, other codes indicate errors.
             qDebug() << "pgrep did not find running icecast process (exit code" << check_icecast.exitCode() << ").";
            is_running = false;
        }
#endif
    } else {
        // Timeout or other error running the check command itself
        qWarning() << "Check command (" << check_cmd << ") timed out or failed to start.";
        is_running = false; // Assume not running if check failed
        // Consider setting UI to an "Unknown" or "Error" state? For now, treat as stopped.
    }

    // --- Update UI based on the is_running state ---
    if (is_running) {
        // Update state variable if needed (though might be redundant if only used here)
        if (!icecastrunning) { // Update internal state if it changed
             qInfo() << "Icecast detected as running (was previously considered stopped).";
             icecastrunning = true;
        }
        // Set UI to "Running" state
        ui->lbl_icecast->setText("Running");
        ui->lbl_icecast->setStyleSheet("color:green;"); // Ensure semicolon
        // Optionally ensure other UI elements are in the correct "running" state if needed
    } else {
        // Update state variable if needed
        if (icecastrunning) { // Update internal state if it changed
            qInfo() << "Icecast detected as stopped (was previously considered running).";
            icecastrunning = false;
        }
        // Set UI to "Stopped" state and reset related elements
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:red;"); // Ensure semicolon

        // Reset other UI elements as per original logic when stopped
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);
        ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
        ui->txt_ProgramName->hide();
        piscaLive = false; // Reset live indicator flag
    }
}
void player::on_bt_butt_clicked()
{
    // Determine the intended state
    bool shouldBeRunning = !buttrunning;

    if (shouldBeRunning) {
        // --- Try to START Butt ---
        qInfo() << "Attempting to start Butt...";

        // 1. Force kill any existing instances first
        qDebug() << "Ensuring no other butt instances are running...";
        killProcessByName("butt"); // Kill 'butt' process

        // 2. Find required executables
        QString xvfbRunPath = QStandardPaths::findExecutable("xvfb-run");
        if (xvfbRunPath.isEmpty()) {
             qWarning() << "xvfb-run command not found in PATH. Cannot start butt.";
             QMessageBox::critical(this, "Error", "Required command 'xvfb-run' not found. Cannot start Butt.");
             return; // Cannot proceed
        }
        QString buttPath = QStandardPaths::findExecutable("butt");
         if (buttPath.isEmpty()) {
             qWarning() << "butt command not found in PATH. Cannot start butt.";
             QMessageBox::critical(this, "Error", "Required command 'butt' not found. Cannot start Butt.");
             return; // Cannot proceed
         }
        qDebug() << "Found xvfb-run at:" << xvfbRunPath;
        qDebug() << "Found butt at:" << buttPath; // We found it, but xvfb-run will call it by name


        // 3. Prepare and start the process detached
        QProcess *buttProcess = new QProcess(this); // Optional: manage this pointer if needed later
        QStringList args;
        args << "-a" << "butt"; // Arguments for xvfb-run

        qDebug() << "Starting detached process:" << xvfbRunPath << args;

        bool started = buttProcess->startDetached(xvfbRunPath, args);

        if (started) {
            qInfo() << "Butt process launch initiated.";
            buttrunning = true; // Update state flag

            // Update UI to "Starting..." - timer will confirm "Running"
            ui->lbl_butt->setText("Starting..."); // More accurate initial state
            ui->lbl_butt->setStyleSheet("color:orange;"); // Indicate intermediate state
            ui->bt_butt->setStyleSheet("background-color:#C8EE72;"); // Indicate "on" state

            // Connect and start the timer to check status
            if (butt_timer) {
                // Ensure clean connection
                disconnect(butt_timer, &QTimer::timeout, this, &player::butt_timmer);
                connect(butt_timer, &QTimer::timeout, this, &player::butt_timmer);
                butt_timer->start(5000); // Check every 5 seconds
                QTimer::singleShot(500, this, &player::butt_timmer); // Check quickly once after starting
            } else {
                 qWarning() << "butt_timer is null!";
            }
        } else {
            qWarning() << "Failed to initiate butt process launch.";
            QMessageBox::critical(this, "Error", "Failed to start the Butt process using xvfb-run.");
            // Clean up if necessary
            delete buttProcess;
        }

    } else {
        // --- Try to STOP Butt ---
        qInfo() << "Attempting to stop Butt...";

        // 1. Stop the status check timer
        if (butt_timer) {
            butt_timer->stop();
            // Optional: disconnect(...)
        }

        // 2. Kill the 'butt' process (FIXED: was killing icecast in original code)
        bool killed = killProcessByName("butt");

        // 3. Update state and UI regardless of kill success
        buttrunning = false;
        ui->bt_butt->setStyleSheet(""); // Default style
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:blue;");

        // Reset dependent UI elements only if Butt stopping should affect them
        // Note: Original code reset these based on stopping *icecast*. Decide if that's correct.
        // If stopping BUTT should stop the 'takeover', etc., keep these lines.
        // Otherwise, remove them or move them to the icecast stop logic.
        // Assuming stopping Butt SHOULD reset these for now:
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);
        ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
        ui->txt_ProgramName->hide();
        piscaLive = false;

        if (!killed) {
             qWarning() << "Kill command for butt may have failed, but UI is set to stopped state.";
        }
    }
}

// --- Refactored butt_timmer ---

void player::butt_timmer() {
    qDebug() << "Running butt_timmer (checking butt process status...)";

    QProcess check_butt;
    QString check_cmd;
    QStringList check_args;
    bool is_running = false; // Assume not running initially

#ifdef Q_OS_WIN
    // Windows: Use tasklist with image name filter
    check_cmd = "tasklist";
    // Adjust "butt.exe" if the actual executable name differs
    check_args << "/NH" << "/FI" << "IMAGENAME eq butt.exe";
#else
    // Unix-like (Linux, macOS): Use pgrep with exact name match
    check_cmd = "pgrep";
    check_args << "-x" << "butt"; // -x matches the name exactly
#endif

    qDebug() << "Executing check:" << check_cmd << check_args;
    check_butt.start(check_cmd, check_args);

    // Wait for the check command to finish (short timeout)
    if (check_butt.waitForFinished(1500)) { // 1.5 second timeout

#ifdef Q_OS_WIN
        QByteArray output = check_butt.readAllStandardOutput();
        // QByteArray::contains has no case-sensitivity overload; lowercase the
        // (ASCII) tasklist output and match the lowercase literal.
        if (output.toLower().contains("butt.exe")) {
            is_running = true;
            qDebug() << "tasklist output indicates butt is running.";
        } else {
            qDebug() << "tasklist output does not contain butt.exe.";
            is_running = false;
        }
#else
        if (check_butt.exitStatus() == QProcess::NormalExit && check_butt.exitCode() == 0) {
            is_running = true;
            qDebug() << "pgrep found running butt process (exit code 0).";
        } else {
            qDebug() << "pgrep did not find running butt process (exit code" << check_butt.exitCode() << ").";
            is_running = false;
        }
#endif
    } else {
        qWarning() << "Check command (" << check_cmd << ") timed out or failed to start.";
        is_running = false; // Assume not running if check failed
    }

    // --- Update UI and state based on the is_running result ---
    if (is_running) {
        // Update state if changed
        if (!buttrunning) {
             qInfo() << "Butt detected as running (was previously considered stopped).";
             buttrunning = true;
        }
        // Set UI to "Running" state
        ui->lbl_butt->setText("Running");
        ui->lbl_butt->setStyleSheet("color:green;");
    } else {
        // Update state if changed
        if (buttrunning) {
            qInfo() << "Butt detected as stopped (was previously considered running).";
            buttrunning = false;
        }
        // Set UI to "Stopped" state
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:red;");
        // Decide if stopping Butt should affect other UI like bt_takeOver.
        // Original butt_timmer didn't reset other UI, only ice_timmer did.
        // Stick to that logic unless intended otherwise.
    }
}

void player::on_bt_ddns_clicked()
{
    ddnsUpdate();
}
void player::on_bt_portTest_clicked()
{
    // --- Configuration ---
    const quint16 portToCheck = 8888;
    const int connectionTimeoutMs = 5000; // 5 seconds timeout for connection attempt

    // --- Get External IP ---
    QString externalIpStr = ui->lbl_ddns->text();

    // --- Basic Validation ---
    if (externalIpStr.isEmpty() || externalIpStr.startsWith("Error")) {
        QMessageBox::warning(this, tr("Missing IP"), tr("Could not determine the external IP address. Please update DDNS first."));
        ui->lbl_port->setText("No IP");
        ui->lbl_port->setStyleSheet("color:orange;");
        return;
    }
    // Optional: More robust IP validation
    QHostAddress addr(externalIpStr);
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) {
         qWarning() << "External IP from label is not a valid IPv4 address:" << externalIpStr;
         // Decide whether to proceed or warn the user further
         // QMessageBox::warning(this, tr("Invalid IP"), tr("The determined external IP address (%1) does not appear valid.").arg(externalIpStr));
         // return; // Uncomment to be stricter
    }


    // --- Start Check ---
    qInfo() << "Checking external reachability for" << externalIpStr << ":" << portToCheck;
    ui->lbl_port->setText("Checking...");
    ui->lbl_port->setStyleSheet("color:#FBEA23;"); // Yellowish color for checking
    // Disable button during check to prevent multiple clicks?
    // ui->bt_portTest->setEnabled(false); // Re-enable in results handling


    // --- Use QTcpSocket for the check ---
    QTcpSocket *socket = new QTcpSocket(this);
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);

    // Lambda to handle cleanup for both success and failure
    auto cleanup = [socket, timeoutTimer, this]() {
        // ui->bt_portTest->setEnabled(true); // Re-enable button
        if (timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }
        // Ensure socket is disconnected before deleting if necessary
        if(socket->state() != QAbstractSocket::UnconnectedState){
            socket->abort(); // Force close immediately
        }
        socket->deleteLater();
        timeoutTimer->deleteLater();
        qDebug() << "Cleanup after port check complete.";
    };

    // Connection successful
    connect(socket, &QTcpSocket::connected, this, [this, externalIpStr, portToCheck, cleanup]() {
        qInfo() << "Connection successful to" << externalIpStr << ":" << portToCheck;
        ui->lbl_port->setText("OPEN"); // Simplified from "OPEN IN + OUT"
        ui->lbl_port->setStyleSheet("color:green;");

        // Format the clickable link
        QString urlString = QString("http://%1:%2/stream.m3u").arg(externalIpStr).arg(portToCheck);
        QString linkHtml = QString("<a href=\"%1\">%1</a>").arg(urlString);

        ui->lbl_streamURL->setText(linkHtml);
        ui->lbl_streamURL->setTextFormat(Qt::RichText);
        ui->lbl_streamURL->setTextInteractionFlags(Qt::TextBrowserInteraction);
        // Note: setOpenExternalLinks(false) is default for QLabel, use true if you want direct clicking
        ui->lbl_streamURL->setOpenExternalLinks(true);

        ui->bt_takeOver->setEnabled(true);

        cleanup(); // Call cleanup lambda
    });

    // Connection error (includes refusal, host not found, etc.)
    connect(socket, &QTcpSocket::errorOccurred, this, [this, socket, cleanup](QAbstractSocket::SocketError socketError) {
        QString errorMsg;
        switch(socketError) {
            case QAbstractSocket::ConnectionRefusedError:
                errorMsg = tr("Connection Refused: Port is likely closed or not listening.");
                break;
            case QAbstractSocket::RemoteHostClosedError:
                errorMsg = tr("Connection Closed Unexpectedly."); // Less likely during initial check
                 break;
            case QAbstractSocket::HostNotFoundError:
                 errorMsg = tr("Host Not Found: The IP address might be incorrect or unreachable.");
                 break;
             case QAbstractSocket::SocketTimeoutError: // Should be caught by our timer, but handle anyway
                 errorMsg = tr("Connection Timed Out (Socket Error).");
                 break;
             case QAbstractSocket::NetworkError:
                 errorMsg = tr("Network Error: Check your connection or firewall.");
                 break;
             default:
                 errorMsg = tr("Connection Failed: %1").arg(socket->errorString());
                 break;
        }
        qWarning() << "Connection error:" << errorMsg << "(" << socketError << ")";
        ui->lbl_port->setText("CLOSED/ERROR");
        ui->lbl_port->setStyleSheet("color:red;");
        QMessageBox::warning(this, tr("Port Check Failed"), errorMsg + "\n\n" + tr("Ensure the service (e.g., Icecast) is running locally and the port is correctly forwarded in your router/firewall."));

        ui->lbl_streamURL->setText(tr("N/A")); // Clear stream URL
        ui->lbl_streamURL->setOpenExternalLinks(false);
        ui->bt_takeOver->setEnabled(false);

        cleanup(); // Call cleanup lambda
    });

    // Connection timeout timer
    connect(timeoutTimer, &QTimer::timeout, this, [this, socket, cleanup]() {
        qWarning() << "Connection attempt timed out.";
        ui->lbl_port->setText("TIMEOUT");
        ui->lbl_port->setStyleSheet("color:red;");
        QMessageBox::warning(this, tr("Port Check Timed Out"), tr("Could not connect to the server within the time limit.\n\nCheck if the server is running, the IP address is correct, and the port is open/forwarded. Network latency or firewalls could also be the cause."));

        ui->lbl_streamURL->setText(tr("N/A"));
        ui->lbl_streamURL->setOpenExternalLinks(false);
        ui->bt_takeOver->setEnabled(false);

        // Abort the socket connection attempt before cleanup
        socket->abort();
        cleanup(); // Call cleanup lambda
    });

    // --- Initiate Connection ---
    socket->connectToHost(externalIpStr, portToCheck);
    timeoutTimer->start(connectionTimeoutMs); // Start the timeout timer
}


void player::deleteFilesByPattern(const QString &dirPath, const QString &pattern) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist, cannot delete files:" << dirPath;
        return;
    }

    // Ensure pattern is treated as a wildcard pattern
    QStringList nameFilters;
    nameFilters << pattern; // e.g., "*.xml"

    // List files matching the pattern
    QStringList filesToDelete = dir.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);

    int deletedCount = 0;
    if (filesToDelete.isEmpty()) {
        qDebug() << "No files matching pattern" << pattern << "found in" << dirPath;
        return;
    }

    qInfo() << "Attempting to delete" << filesToDelete.count() << "files matching" << pattern << "in" << dirPath;
    for (const QString &filename : filesToDelete) {
        QString filePath = dir.filePath(filename);
        if (QFile::remove(filePath)) {
            qDebug() << "Deleted:" << filePath;
            deletedCount++;
        } else {
            qWarning() << "Failed to delete:" << filePath;
            // Log QFile::errorString() if needed for more details
        }
    }
    qInfo() << "Successfully deleted" << deletedCount << "out of" << filesToDelete.count() << "matching files.";
    return;
}

void player::on_bt_takeOver_clicked()
{
    // --- State Toggle Logic ---
    if (takeOver) {
        // Currently in takeover mode, user wants to cancel
        qInfo() << "Cancelling Takeover...";
        takeOver = false; // Update state first

        // Call the function responsible for reversing the takeover on the server side
        returnTakeOver(); // This likely needs its own robust implementation (maybe another script?)

        // Clean up local indicator file (use configured FTPPath)
        QString takeOverFilePath = QDir(FTPPath).filePath("takeover.xml");
        if (QFile::exists(takeOverFilePath)) {
            if (QFile::remove(takeOverFilePath)) {
                qInfo() << "Removed local takeover indicator file:" << takeOverFilePath;
            } else {
                qWarning() << "Failed to remove local takeover indicator file:" << takeOverFilePath;
            }
        }

        // Reset UI
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));
        ui->txt_ProgramName->setStyleSheet(""); // Clear style
        ui->txt_ProgramName->hide();
        piscaLive = false;
        return; // Finished cancelling
    }

    // --- Initiate Takeover ---
    qInfo() << "Initiating Takeover...";
    ui->bt_takeOver->setEnabled(false); // Disable button during operation
    ui->bt_takeOver->setText(tr("Processing..."));
    ui->bt_takeOver->setStyleSheet("background-color:yellow;"); // Indicate processing
    qApp->processEvents(); // Update UI immediately

    // --- 1. Clean Up Old Files ---
    qInfo() << "Cleaning up previous XML files...";
    // Use helper function with configured paths
    deleteFilesByPattern(FTPPath, "*.xml");
    deleteFilesByPattern(TakeOverPath, "*.xml");
    // Add checks here if deletion failure is critical

    // --- 2. Get Required Data ---
    QString externalIp = ui->lbl_ddns->text();
    if (externalIp.isEmpty() || externalIp.startsWith("Error")) {
        QMessageBox::warning(this, tr("Missing IP"), tr("Cannot initiate takeover without a valid external IP address. Please update DDNS first."));
        ui->bt_takeOver->setEnabled(true); // Re-enable button
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));
        ui->bt_takeOver->setStyleSheet("");
        return;
    }
    QString streamUrl = QString("http://%1:8888/stream.m3u").arg(externalIp); // Hardcoded port 8888

    // --- 3. Create Takeover XML ---
    QString takeOverFilePath = QDir(FTPPath).filePath("takeover.xml"); // Create in FTPPath
    qInfo() << "Creating takeover file:" << takeOverFilePath;
    QFile file(takeOverFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "Failed to open takeover file for writing:" << file.errorString();
        QMessageBox::critical(this, "File Error", QString("Could not create the takeover XML file:\n%1").arg(takeOverFilePath));
        ui->bt_takeOver->setEnabled(true);
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));
        ui->bt_takeOver->setStyleSheet("");
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("XFBClientTakeOver");
    xmlWriter.writeStartElement("www.netpack.pt"); // Consider making this configurable
    xmlWriter.writeTextElement("stream", streamUrl);
    xmlWriter.writeTextElement("ip", externalIp);
    xmlWriter.writeEndElement(); // www.netpack.pt
    xmlWriter.writeEndElement(); // XFBClientTakeOver
    xmlWriter.writeEndDocument();

    if (xmlWriter.hasError()) {
         qWarning() << "Error writing XML to takeover file.";
         // File might be corrupt or incomplete
         file.close(); // Try to close
         QFile::remove(takeOverFilePath); // Attempt cleanup
         QMessageBox::critical(this, "XML Error", "Failed to generate the takeover XML file correctly.");
         ui->bt_takeOver->setEnabled(true);
         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         return;
    }
    file.close(); // XML writing finished successfully
    qInfo() << "Takeover XML created successfully.";

    // --- 4. Execute Upload Script Asynchronously ---
    // Determine script path (needs configuration - example assumes it's relative to app dir)
    // TODO: Replace this with a robust way to find the script (e.g., settings, known install path)
    QString scriptName = "serverFtpCmdsPutTakeOver.sh";
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path relative to app binary
    QString scriptPath = QDir(scriptDir).filePath(scriptName);

    qInfo() << "Attempting to execute upload script:" << scriptPath;
     qDebug() << "Dependencies: Script must exist, be executable, and ~/.netrc configured correctly.";


    if (!QFileInfo::exists(scriptPath)) {
         qWarning() << "Upload script not found at:" << scriptPath;
         QMessageBox::critical(this, "Script Error", QString("The required upload script was not found:\n%1").arg(scriptPath));
         // Clean up local XML file? Maybe not, user might want to upload manually.
         ui->bt_takeOver->setEnabled(true);
         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         return;
    }
     // Optional: Check if script is executable on Linux/macOS
     // QFileInfo scriptInfo(scriptPath);
     // if (!scriptInfo.isExecutable()) { ... error ... }


    ui->bt_takeOver->setText(tr("Uploading..."));
    qApp->processEvents();

    QProcess *uploadProcess = new QProcess(this); // Create on heap for async handling

    // Connect signals *before* starting
    connect(uploadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, uploadProcess, takeOverFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Upload script finished. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;

        QString stdOutput = QString::fromLocal8Bit(uploadProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(uploadProcess->readAllStandardError()).trimmed();

        bool success = false;
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Script exited normally, check output for confirmation
             qDebug() << "Upload script STDOUT:\n" << stdOutput;
             // Make check more robust - case-insensitive contains?
            if (stdOutput.contains("Transfer complete", Qt::CaseInsensitive)) {
                 qInfo() << "Upload script reported success.";
                 success = true;
             } else {
                  qWarning() << "Upload script finished (exit 0) but success message not found in output.";
                  if (!stdError.isEmpty()) {
                       qWarning() << "Upload script STDERR:\n" << stdError;
                  }
             }
        } else {
            qWarning() << "Upload script failed to execute or exited with an error.";
             if (!stdError.isEmpty()) {
                  qWarning() << "Upload script STDERR:\n" << stdError;
             } else if (!stdOutput.isEmpty()) {
                 // Sometimes errors go to stdout
                 qWarning() << "Upload script STDOUT (check for errors):\n" << stdOutput;
             }
        }

        // --- Handle result ---
        if (success) {
            ui->bt_takeOver->setStyleSheet("background-color:blue;"); // Indicate active takeover
            ui->bt_takeOver->setText(tr("Verifying...")); // Or "Broadcasting (Click to Stop)"
            takeOver = true; // Update state only on confirmed success

            // Clean up local file *after* successful upload
            if (QFile::remove(takeOverFilePath)) {
                 qInfo() << "Removed local takeover XML after successful upload.";
            } else {
                 qWarning() << "Could not remove local takeover XML after successful upload:" << takeOverFilePath;
            }

            // Start the verification check
            QTimer::singleShot(5000, this, &player::checkTakeOver);

        } else {
            // Upload failed or confirmation missing
            QMessageBox::critical(this, "Upload Failed", tr("Failed to upload the takeover file to the server or confirmation was not received.\nPlease check ~/.netrc, script permissions, network connection, and server status.\n\nScript output (if any):\n%1\n%2")
                                  .arg(stdOutput.left(200)) // Show some output
                                  .arg(stdError.left(200))); // Show some error output
            // Reset UI to pre-takeover state
            ui->bt_takeOver->setText(tr("Broadcast LIVE"));
            ui->bt_takeOver->setStyleSheet("");
            // Leave local XML file for debugging? Or remove it? Let's leave it for now.
            // QFile::remove(takeOverFilePath);
        }

        // Re-enable button and clean up process object
        ui->bt_takeOver->setEnabled(true);
        uploadProcess->deleteLater();
    });

    // Handle process startup errors
     connect(uploadProcess, &QProcess::errorOccurred, this, [this, uploadProcess, scriptPath](QProcess::ProcessError error){
         qWarning() << "Failed to start upload script (" << scriptPath << "). Error:" << error;
         QMessageBox::critical(this, "Script Error", QString("Could not start the upload script:\n%1\n\nError: %2").arg(scriptPath).arg(uploadProcess->errorString()));

         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         ui->bt_takeOver->setEnabled(true);
         uploadProcess->deleteLater(); // Cleanup
     });

    // Start the script
    uploadProcess->start(scriptPath); // No arguments needed based on original code
}
void player::livePiscaStart(){

    if(piscaLive==true){
        ui->txt_ProgramName->setStyleSheet("background-color:#FF6549;color:#FFF;text-align:center !important;font-size:28px;font-weight:bolder;");
        QTimer::singleShot(1000,this,SLOT(livePiscaStop()));
    }



}
void player::livePiscaStop(){

                    ui->txt_ProgramName->setStyleSheet("background-color:red;color:#FFF;text-align:center !important;font-size:28px;font-weight:bolder;");
                    QTimer::singleShot(1000,this,SLOT(livePiscaStart()));

}


void player::checkTakeOver() {
    qDebug() << "Checking Takeover status asynchronously...";

    // --- Determine Script Path ---
    // TODO: Replace this with a robust way to find the script
    QString scriptName = "serverFtpCmdsCHKTakeOver.sh";
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path
    QString scriptPath = QDir(scriptDir).filePath(scriptName);

    qInfo() << "Attempting to execute check script:" << scriptPath;

    if (!QFileInfo::exists(scriptPath)) {
        qWarning() << "Check script not found at:" << scriptPath;
        // Stop checking? Or keep retrying? Let's stop and warn.
        QMessageBox::critical(this, "Script Error", QString("The required check script was not found:\n%1\n\nTakeover verification stopped.").arg(scriptPath));
        // Reset UI to non-live state? Depends on desired behavior on error.
        // resetToNonLiveState(); // Call a hypothetical function to reset UI
        return;
    }
    // Optional: Check script executability on Linux/macOS

    // --- Execute Script Asynchronously ---
    QProcess *checkProcess = new QProcess(this); // Create on heap for async

    // Connection to handle when the process finishes
    connect(checkProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, checkProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Check script finished. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;

        bool takeoverConfirmed = false;
        QString stdOutput = QString::fromLocal8Bit(checkProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(checkProcess->readAllStandardError()).trimmed();

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Script finished successfully, now check its output
            qDebug() << "Check script STDOUT:\n" << stdOutput;
            if (stdOutput.contains("confirmtakeover.xml", Qt::CaseInsensitive)) {
                // Found the confirmation filename in the output
                takeoverConfirmed = true;
                qInfo() << "Takeover confirmation found in script output.";
            } else {
                qInfo() << "Takeover confirmation filename not found in script output.";
                // Log stderr just in case it has info
                if (!stdError.isEmpty()) {
                    qWarning() << "Check script STDERR (though exit code was 0):\n" << stdError;
                }
            }
        } else {
            // Script failed to execute properly or returned an error code
            qWarning() << "Check script failed or exited with error.";
            if (!stdError.isEmpty()) {
                qWarning() << "Check script STDERR:\n" << stdError;
            } else if (!stdOutput.isEmpty()) {
                // Sometimes errors go to stdout
                qWarning() << "Check script STDOUT (check for errors):\n" << stdOutput;
            }
        }

        // --- Update UI and State ---
        if (takeoverConfirmed) {
            qInfo() << "      -------------------         [TakeOver CONFIRMED]      ---------------------       ";
            // Check if already in the live state to avoid redundant updates/starts
            if (ui->bt_takeOver->text() != tr("BROADCASTING LIVE!!!")) {
                 ui->bt_takeOver->setStyleSheet("background-color:green;"); // Use semicolon
                 ui->bt_takeOver->setText(tr("BROADCASTING LIVE!!!"));

                 ui->txt_ProgramName->setText(tr("BROADCASTING LIVE!!!"));
                 ui->txt_ProgramName->setStyleSheet("background-color:red;color:#FFF;text-align:center !important;font-size:28px;font-weight:bolder;"); // Added semicolon
                 ui->txt_ProgramName->setAlignment(Qt::AlignHCenter);
                 ui->txt_ProgramName->show();

                 if (piscaLive == false) {
                     piscaLive = true;
                     livePiscaStart(); // Assuming this starts the blinking animation
                 }
            } else {
                 qDebug() << "Takeover already confirmed, UI state unchanged.";
            }
            // Successfully confirmed, do NOT schedule another check.

        } else {
            // Takeover not confirmed, schedule retry
            qWarning() << "      -------------------         [Takeover Check FAILED]         ---------------------       ";
            qWarning() << "      -------------------     [Scheduling retry in 10 sec]     ---------------------";

            // Optional: Update UI to show "Verification Failed" or similar temporarily?
            // ui->bt_takeOver->setText(tr("Verification Failed"));
            // ui->bt_takeOver->setStyleSheet("background-color:orange;"); // Indicate temporary failure?

            QTimer::singleShot(10000, this, &player::checkTakeOver); // Schedule retry
        }

        // Cleanup the process object
        checkProcess->deleteLater();
    });

    // Connection to handle process startup errors
    connect(checkProcess, &QProcess::errorOccurred, this, [this, checkProcess, scriptPath](QProcess::ProcessError error) {
        qWarning() << "Failed to start check script (" << scriptPath << "). Error:" << error << "-" << checkProcess->errorString();

        // Decide how to handle startup failure. Retry? Show critical error?
        qWarning() << "      -------------------         [Takeover Check FAILED - Could not start script]         ---------------------       ";
        qWarning() << "      -------------------     [Scheduling retry in 10 sec]     ---------------------";

        // Optional: Update UI to show error state
        // ui->bt_takeOver->setText(tr("Check Script Error"));
        // ui->bt_takeOver->setStyleSheet("background-color:purple;");

        QTimer::singleShot(10000, this, &player::checkTakeOver); // Schedule retry even on startup error
        checkProcess->deleteLater(); // Cleanup
    });

    // --- Start the Check ---
    // Execute the script directly, do not pipe to grep here
    checkProcess->start(scriptPath);
}
void player::MainsetVol100(){
    XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
}

void player::MainsetVol80(){
    XplayerOutput->setVolume(0.8); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol60(){
    XplayerOutput->setVolume(0.6); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol40(){
    XplayerOutput->setVolume(0.4); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol20(){
    XplayerOutput->setVolume(0.2); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol10(){
    XplayerOutput->setVolume(0.1); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol5(){
    XplayerOutput->setVolume(0.05); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainStop(){
    stopTailPlayer();
    Xplayer->stop();
    ui->btPlay->setStyleSheet("");
    ui->btPlay->setText(tr("Play"));
    PlayMode = "stopped";
}

bool player::killProcessByName(const QString &processName) {
    QProcess killer;
    QString cmd;
    QStringList args;
    bool success = false;

#ifdef Q_OS_WIN
    // Windows: Use taskkill
    cmd = "taskkill";
    args << "/F" << "/IM" << processName + "*";
    qWarning() << "Windows taskkill pattern matching is basic. Ensure '" << processName << "*' matches correctly, or use exact name.";
#else
    // Unix-like (Linux, macOS): Prefer pkill
    cmd = "pkill";
    args << "-f" << processName; // -f matches against the entire command line
#endif

    qInfo() << "Attempting to kill processes matching:" << processName << "using command:" << cmd << args;
    killer.start(cmd, args);
    if (killer.waitForFinished(3000)) { // Wait up to 3 seconds
        if (killer.exitStatus() == QProcess::NormalExit && killer.exitCode() == 0) {
            qInfo() << "Kill command executed successfully for" << processName << "(process may or may not have been running).";
            success = true;
        } else {
            qWarning() << "Kill command for" << processName << "failed or reported an error. Exit code:" << killer.exitCode() << "Status:" << killer.exitStatus();
        }
    } else {
        qWarning() << "Kill command for" << processName << "timed out.";
        killer.kill();
        killer.waitForFinished(500);
    }
    return success;
}

void player::pingTakeOverClient() {

    if (takeOverIP.isEmpty()) {
        qWarning() << "Cannot ping TakeOver Client: takeOverIP is empty.";
        // Optionally trigger the failure actions immediately? Or just return?
        // triggerPingFailureActions(); // Call a helper containing the failure steps
        return;
    }

    qInfo() << "Pinging TakeOver Client asynchronously ->| " << takeOverIP;

    // --- Prepare Ping Command (Platform Specific) ---
    QProcess *pingProcess = new QProcess(this); // Create on heap for async handling
    QString pingCmd;
    QStringList pingArgs;
    int timeoutMs = 5000; // Timeout for the ping command itself (e.g., 5 seconds) - was 8s wait

#ifdef Q_OS_WIN
    pingCmd = "ping";
    // -n 1: Send 1 echo request
    // -w timeout: Wait 'timeout' milliseconds for reply (use slightly less than waitForFinished timeout)
    pingArgs << "-n" << "1" << "-w" << QString::number(timeoutMs - 500) << takeOverIP;
#elif defined(Q_OS_MACOS)
    pingCmd = "ping";
    // -c 1: Send 1 echo request
    // -t timeout: Specify timeout in seconds for the command
    pingArgs << "-c" << "1" << "-t" << QString::number(timeoutMs / 1000) << takeOverIP;
#else // Linux and other Unix-like
    pingCmd = "ping";
    // -c 1: Send 1 echo request
    // -W timeout: Wait 'timeout' seconds for a reply
    // -q: Quiet output (optional, we only care about exit code)
    pingArgs << "-c" << "1" << "-W" << QString::number(timeoutMs / 1000) << "-q" << takeOverIP;
#endif

    qDebug() << "Executing ping command:" << pingCmd << pingArgs;

    // --- Connect Signals BEFORE Starting ---

    // Handle process finished
    connect(pingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, pingProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Ping process finished. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;

        bool success = false;
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Exit code 0 typically means success (at least one reply received)
            success = true;
        } else {
            // Non-zero exit code or crash indicates failure
            qWarning() << "Ping command failed or host unreachable.";
            // Read stderr/stdout for potential clues (optional)
            // QString stdErr = QString::fromLocal8Bit(pingProcess->readAllStandardError());
            // if (!stdErr.isEmpty()) qDebug() << "Ping STDERR:" << stdErr;
            // QString stdOut = QString::fromLocal8Bit(pingProcess->readAllStandardOutput());
            // if (!stdOut.isEmpty()) qDebug() << "Ping STDOUT:" << stdOut;
        }

        // --- Take Action Based on Result ---
        if (success) {
            qInfo() << "TakeOver Client pinged OK!! |<- " << takeOverIP;
            // No further action needed on success according to original logic
        } else {
            qWarning() << "ERROR pinging TakeOver Client!! Taking recovery actions...";
            triggerPingFailureActions(); // Encapsulate failure actions in a helper
        }

        // Cleanup
        pingProcess->deleteLater();
    });

    // Handle process startup errors
    connect(pingProcess, &QProcess::errorOccurred, this, [this, pingProcess](QProcess::ProcessError error) {
        qWarning() << "Failed to start ping process for" << takeOverIP << ". Error:" << error << "-" << pingProcess->errorString();
        qWarning() << "Treating as ping failure and taking recovery actions...";

        triggerPingFailureActions(); // Trigger failure actions if ping command itself fails

        // Cleanup
        pingProcess->deleteLater();
    });

    // --- Start the Ping ---
    pingProcess->start(pingCmd, pingArgs);
    // We don't use waitForFinished here, rely on signals
}

// --- Helper Function for Failure Actions ---
// Encapsulating these makes the main logic cleaner
void player::triggerPingFailureActions() {

    qWarning() << "Killing all streams (mplayer) and starting local playback...";

    // Stop mplayer using the robust helper
    killProcessByName("mplayer");

    // Start local playback
    on_btPlay_clicked(); // Assuming this starts the local player

    // Schedule volume fade-in (Original logic)
    // Consider if these delays/steps are still appropriate
    QTimer::singleShot(3500, this, &player::MainsetVol100);
    QTimer::singleShot(3000, this, &player::MainsetVol80);
    QTimer::singleShot(2500, this, &player::MainsetVol60);
    QTimer::singleShot(2000, this, &player::MainsetVol40);
    QTimer::singleShot(1500, this, &player::MainsetVol20);
    QTimer::singleShot(1000, this, &player::MainsetVol10);
    QTimer::singleShot(500, this, &player::MainsetVol5); // Note: Original code didn't have 0? Start low.

    // Schedule recovery attempt
    qInfo() << "Scheduling recovery stream attempt in 30 seconds.";
    QTimer::singleShot(30000, this, &player::recoveryStreamTakeOverPlay);
}

void player::recoveryStreamTakeOverPlay() {

    qInfo() << "Recovery initiated: Attempting to play takeover stream...";

    // --- 1. Stop Existing Player ---
    qInfo() << "Stopping any existing mplayer instance...";
    stopMplayer(); // Call the assumed function to stop the player

    // --- 2. Validate Stream URL/Path ---
    if (takeOverStream.isEmpty()) {
        qWarning() << "Cannot start recovery stream: takeOverStream variable is empty.";
        QMessageBox::critical(this, tr("Recovery Error"), tr("The recovery stream URL is not configured. Cannot proceed."));
        // Optionally trigger different recovery or stop?
        return;
    }
    // Optional: If it's a local file path, check if it exists
    // QUrl streamUrl(takeOverStream);
    // if (streamUrl.isLocalFile()) {
    //     QFileInfo playlistInfo(streamUrl.toLocalFile());
    //     if (!playlistInfo.exists()) {
    //         qWarning() << "Recovery playlist file not found:" << playlistInfo.filePath();
    //         QMessageBox::critical(this, tr("Recovery Error"), tr("Recovery playlist file not found:\n%1").arg(playlistInfo.filePath()));
    //         return;
    //     }
    // }
    qInfo() << "Attempting to play stream/playlist:" << takeOverStream;

    // --- 3. Find mplayer Executable ---
    QString mplayerPath = QStandardPaths::findExecutable("mplayer");
    if (mplayerPath.isEmpty()) {
        qWarning() << "mplayer executable not found in system PATH.";
        QMessageBox::critical(this, tr("Dependency Error"), tr("'mplayer' command not found. Please install mplayer and ensure it is in your PATH."));
        // Cannot proceed without mplayer
        return;
    }
    qInfo() << "Found mplayer at:" << mplayerPath;


    // --- 4. Prepare and Start mplayer Asynchronously ---
    QStringList mplayerArgs;
    // Add arguments BEFORE the playlist/URL
    mplayerArgs << "-volume" << "100"; // Start at full volume? Or maybe lower and fade in?
    mplayerArgs << "-playlist" << takeOverStream; // Add playlist/URL last

    qDebug() << "Executing:" << mplayerPath << mplayerArgs;

    // Disconnect any previous signal connections from the 'radio1' process object
    // to avoid duplicate handlers if this function is called again rapidly.
    radio1.disconnect(); // Disconnects all signals from this object

    // Connect error handling signal *before* starting
    connect(&radio1, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        qWarning() << "Failed to start mplayer for recovery stream. Error:" << error << "-" << radio1.errorString();
        QMessageBox::warning(this, tr("Playback Error"), tr("Failed to start the recovery stream player (mplayer).\nError: %1").arg(radio1.errorString()));
        // Optionally, schedule another recovery attempt?
        // QTimer::singleShot(15000, this, &player::recoveryStreamTakeOverPlay);
    });

    // Optionally connect started signal for logging confirmation
     connect(&radio1, &QProcess::started, this, [this](){
         qInfo() << "mplayer process started successfully for recovery stream.";
     });

     // Optionally connect finished signal for cleanup or restart logic
     connect(&radio1, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
             this, [this](int exitCode, QProcess::ExitStatus exitStatus){
         qWarning() << "mplayer recovery stream finished unexpectedly. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;
         // Decide what to do: retry? stop? Assume user interaction needed?
         // Maybe just log it if recoveryStreamTakeOverPlay is the only way to restart.
         // If the stream *should* run indefinitely, maybe schedule another recovery:
         // QTimer::singleShot(5000, this, &player::recoveryStreamTakeOverPlay);
     });


    // Start the process
    radio1.start(mplayerPath, mplayerArgs);

    // Close communication channels *after* starting if not needed (as per original)
    // Note: Keeping stderr open can be useful for debugging mplayer errors.
    radio1.closeReadChannel(QProcess::StandardOutput);
    radio1.closeReadChannel(QProcess::StandardError);
    // radio1.closeWriteChannel(); // Also close input if definitely not needed


    // --- 5. Update UI Immediately ---
    // (Original logic updated UI before confirming mplayer started)
    ui->txtNowPlaying->setText(QString("RECOVERY: %1").arg(takeOverStream)); // Add prefix for clarity

    QDateTime now = QDateTime::currentDateTime();
    QString text = now.toString("yyyy-MM-dd || hh:mm:ss ||");
    // Add context to history entry
    QString historyNewLine = text + " RECOVERY STARTED - " + takeOverStream;
    ui->historyList->addItem(historyNewLine);
    ui->historyList->scrollToBottom(); // Ensure latest entry is visible


    // --- 6. Stop the Main Local Player (Delayed Fade) ---
    // This logic assumes MainsetVol/MainStop control a *different* player/UI element
    qInfo() << "Scheduling fade-out and stop for the main local player...";
    QTimer::singleShot(500, this, &player::MainsetVol100); // Start fade from 100? Or 0? This sequence looks like fade *out*.
    QTimer::singleShot(1000, this, &player::MainsetVol80);
    QTimer::singleShot(1500, this, &player::MainsetVol60);
    QTimer::singleShot(2000, this, &player::MainsetVol40);
    QTimer::singleShot(2500, this, &player::MainsetVol20);
    QTimer::singleShot(3000, this, &player::MainsetVol10);
    QTimer::singleShot(3500, this, &player::MainsetVol5);
    QTimer::singleShot(4000, this, &player::MainStop); // Final stop
}
void player::on_bt_pause_rec_clicked()
{

    if(recPause==false){
        recPause=true;
        ui->bt_pause_rec->setStyleSheet("background-color:yellow");

        audioRecorder->pause();
        recTimer->stop();

    } else {
        recPause=false;
        ui->bt_pause_rec->setStyleSheet("");
        audioRecorder->record();
        recTimer->start();
    }

}

void player::on_bt_pause_play_clicked()
{

    if(playPause==false){
        playPause=true;
        ui->bt_pause_play->setStyleSheet("background-color:yellow");

        Xplayer->pause();


    } else {
        playPause=false;
        ui->bt_pause_play->setStyleSheet("");
        Xplayer->play();

    }
}

void player::refreshAdBanner() {
    if (adBanner && adBanner->rootObject()) {
        // Call the QML function to reload the ad
        QMetaObject::invokeMethod(adBanner->rootObject(), "loadAd");
    }
}

// Make the bottom Music/Jingles/Pub/Programs/Torrents tab widget collapsible:
// clicking the already-selected tab folds the content pane away (leaving the
// tab bar), and clicking it again — or selecting another tab — restores it.
void player::setupCollapsibleTabs()
{
    if (!ui || !ui->pubWidget)
        return;

    QTabWidget *tw = ui->pubWidget;
    QTabBar *bar = tw->tabBar();
    if (!bar)
        return;

    bar->setToolTip(tr("Click the selected tab again to collapse or expand this panel"));

    connect(bar, &QTabBar::tabBarClicked, this, [this, tw](int index) {
        if (index < 0)
            return;
        if (index == tw->currentIndex()) {
            // Clicking the active tab toggles the content pane.
            setPubTabsCollapsed(!m_pubTabsCollapsed);
        } else if (m_pubTabsCollapsed) {
            // Selecting a different tab while collapsed expands to show it.
            setPubTabsCollapsed(false);
        }
    });
}

void player::setPubTabsCollapsed(bool collapsed)
{
    if (!ui || !ui->pubWidget)
        return;

    QTabWidget *tw = ui->pubWidget;
    m_pubTabsCollapsed = collapsed;

    if (collapsed) {
        // Fold the widget down to just the tab bar so the area above it (the
        // playlist) gets the freed space. TabPosition is South, so the bar
        // stays visible.
        const int barHeight = tw->tabBar() ? tw->tabBar()->sizeHint().height() : 24;
        tw->setMaximumHeight(barHeight + 4);
    } else {
        tw->setMaximumHeight(QWIDGETSIZE_MAX);
    }
}

// UI accessor methods for controllers
QTableView* player::getMusicView() const {
    return ui->musicView;
}

QPushButton* player::getPlayButton() const {
    return ui->btPlay;
}

QPushButton* player::getStopButton() const {
    return ui->btStop;
}

QSlider* player::getProgressSlider() const {
    return ui->sliderProgress;
}

QSlider* player::getVolumeSlider() const {
    return ui->sliderVolume;
}

// Torrent functionality implementations

bool player::ensureTorrentClient()
{
    DependencyChecker depChecker;
    return depChecker.ensureAnyOf({"aria2c", "transmission-cli"},
        tr("Downloading torrents requires a torrent client. XFB can use aria2 "
           "(recommended) or transmission-cli to fetch the files."),
        this);
}

void player::on_torConnectButton_clicked()
{
    if (!ui || !m_torNetworkService) {
        QMessageBox::warning(this, tr("Tor Connection"), tr("Tor network service is not available."));
        return;
    }
    
    if (m_torNetworkService->isTorReady()) {
        QMessageBox::information(this, tr("Tor Connection"), tr("Tor is already connected."));
        return;
    }

    // Ensure Tor is available before attempting to connect. Prefer the Tor
    // that ships with XFB (bundled at <app>/tor/tor.exe on Windows) or one
    // found on the system; only if none is present do we try to install it via
    // a package manager (works on Linux/macOS).
    {
        bool torAvailable = m_torNetworkService->isTorAvailable();
        if (!torAvailable) {
            DependencyChecker depChecker;
            depChecker.ensureDependency("tor",
                tr("XFB uses the Tor network to anonymously search for and reach .onion "
                   "torrent mirrors. The \"tor\" program provides this connection."),
                this);
            torAvailable = m_torNetworkService->isTorAvailable();
        }
        if (!torAvailable) {
            updateTorConnectionUI(false);
            QMessageBox::warning(this, tr("Tor Not Available"),
                tr("XFB could not find the Tor program needed to connect.\n\n"
                   "On Linux/macOS, install the \"tor\" package with your package "
                   "manager. On Windows, reinstall XFB so the bundled Tor is present."));
            return;
        }
    }

    // Show security warning
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Tor Connection"), 
        tr("⚠️ SECURITY NOTICE ⚠️\n\n"
           "You are about to connect to the Tor network. This will:\n"
           "• Route your traffic through the Tor network for anonymity\n"
           "• Only allow connections to .onion sites for security\n"
           "• Block suspicious scripts and external connections\n"
           "• Enable secure torrent searching\n\n"
           "Do you want to connect to Tor?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    if (ui->torConnectButton) {
        ui->torConnectButton->setEnabled(false);
        ui->torConnectButton->setText(tr("Connecting..."));
    }
    if (ui->torConnectionStatus) {
        ui->torConnectionStatus->setText(tr("Status: Connecting..."));
        ui->torConnectionStatus->setStyleSheet("color: orange; font-weight: bold;");
    }
    
    if (m_torNetworkService->connectToTor()) {
        // Connection started, wait for ready signal
    } else {
        QMessageBox::critical(this, tr("Connection Error"), 
                             tr("Failed to start Tor connection. Please check if Tor is installed."));
        updateTorConnectionUI(false);
    }
}

void player::on_torDisconnectButton_clicked()
{
    if (!m_torNetworkService) {
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Disconnect Tor"), 
        tr("Are you sure you want to disconnect from Tor?\n\n"
           "This will disable torrent searching and stop all secure connections."),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_torNetworkService->disconnectFromTor();
        updateTorConnectionUI(false);
    }
}

void player::on_findOnionButton_clicked()
{
    if (!ui || !m_torNetworkService || !m_torNetworkService->isTorReady()) {
        QMessageBox::warning(this, tr("Tor Required"), 
                            tr("Please connect to Tor first before searching for onion mirrors."));
        return;
    }
    
    if (ui->findOnionButton) {
        ui->findOnionButton->setEnabled(false);
        ui->findOnionButton->setText(tr("Searching..."));
    }
    
    // Search for 1337x.to onion mirror
    m_torNetworkService->findOnionMirror("1337x.to");
}

void player::on_reloadPageButton_clicked()
{
    if (!m_torNetworkService || !m_torNetworkService->isTorReady()) {
        QMessageBox::warning(this, tr("Tor Required"), 
                            tr("Please connect to Tor first."));
        return;
    }
    
    QString query = ui->torrentSearchEdit->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::warning(this, tr("No Search Query"), 
                            tr("Please enter a search query first."));
        return;
    }
    
    // Clear current results and search again
    if (m_torrentSearchService) {
        m_torrentSearchService->cancelSearch();
        m_torrentSearchService->clearResults();
        
        // Start new search
        m_torrentSearchService->searchTorrents(query);
    }
}

void player::updateTorConnectionUI(bool connected)
{
    // Add null checks for UI elements to prevent crashes
    if (!ui) return;
    
    if (ui->torConnectButton) ui->torConnectButton->setEnabled(!connected);
    if (ui->torDisconnectButton) ui->torDisconnectButton->setEnabled(connected);
    if (ui->torrentSearchEdit) ui->torrentSearchEdit->setEnabled(connected);
    if (ui->torrentSearchButton) ui->torrentSearchButton->setEnabled(connected);
    if (ui->findOnionButton) ui->findOnionButton->setEnabled(connected);
    if (ui->reloadPageButton) ui->reloadPageButton->setEnabled(connected);
    
    if (connected) {
        if (ui->torConnectButton) ui->torConnectButton->setText(tr("Connect to Tor"));
        if (ui->torConnectionStatus) {
            ui->torConnectionStatus->setText(tr("Status: Connected & Secure"));
            ui->torConnectionStatus->setStyleSheet("color: green; font-weight: bold;");
        }
        if (ui->torrentWarningLabel) {
            ui->torrentWarningLabel->setText(tr("⚠️ WARNING: Only download legal content. Tor Status: Connected - Secure browsing enabled."));
        }
    } else {
        if (ui->torConnectButton) ui->torConnectButton->setText(tr("Connect to Tor"));
        if (ui->torConnectionStatus) {
            ui->torConnectionStatus->setText(tr("Status: Disconnected"));
            ui->torConnectionStatus->setStyleSheet("color: red; font-weight: bold;");
        }
        if (ui->torrentWarningLabel) {
            ui->torrentWarningLabel->setText(tr("⚠️ WARNING: Only download legal content. Tor Status: Disconnected - Click \"Connect to Tor\" to enable secure browsing."));
        }
    }
}

void player::onTorReady()
{
    updateTorConnectionUI(true);
    QMessageBox::information(this, tr("Tor Connected"), 
                            tr("Successfully connected to Tor network. You can now search for torrents securely."));
}

void player::onTorDisconnected()
{
    updateTorConnectionUI(false);
    ui->statusBar->showMessage(tr("Disconnected from Tor network"), 3000);
}

void player::updateDownloadsCountLabel()
{
    if (!m_torrentDownloadService || !ui) return;
    int active = m_torrentDownloadService->activeDownloadCount();
    ui->downloadsCountLabel->setText(QString("(%1 active)").arg(active));
    // Auto-hide panel when nothing left
    if (active == 0 && m_torrentDownloadService->getDownloadsModel()->rowCount() == 0) {
        ui->downloadsPanel->setVisible(false);
    }
}

void player::onTorError(const QString &error)
{
    updateTorConnectionUI(false);
    QMessageBox::critical(this, tr("Tor Connection Error"), 
                         tr("Tor connection failed: %1").arg(error));
}

void player::onOnionMirrorFound(const QString &clearnetDomain, const QString &onionUrl)
{
    Q_UNUSED(clearnetDomain)
    
    if (ui && ui->findOnionButton) {
        ui->findOnionButton->setEnabled(true);
        ui->findOnionButton->setText(tr("Find 1337x.to Onion"));
    }
    
    QMessageBox::information(this, tr("Onion Mirror Found"), 
                            tr("Found working onion mirror:\n%1\n\nThis will be used for secure searches.").arg(onionUrl));
}

void player::onSearchingForOnionMirror(const QString &clearnetDomain)
{
    Q_UNUSED(clearnetDomain)
    if (ui && ui->statusBar) {
        ui->statusBar->showMessage(tr("Searching for secure onion mirror..."), 0);
    }
}

void player::onOnionMirrorSearchFailed(const QString &clearnetDomain)
{
    Q_UNUSED(clearnetDomain)
    
    if (ui && ui->findOnionButton) {
        ui->findOnionButton->setEnabled(true);
        ui->findOnionButton->setText(tr("Find 1337x.to Onion"));
    }
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Onion Mirror Unavailable"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(tr("Could not find a working .onion mirror for 1337x.to.\n\n"
                      "The onion site may be temporarily down or the address may have changed."));
    msgBox.setInformativeText(tr("Would you like to use the main site (1337x.to) through Tor instead?\n\n"
                                 "Your traffic will still be routed through the Tor network for anonymity, "
                                 "but will exit through a Tor exit node to reach the clearnet site."));
    
    QPushButton *useClearnetBtn = msgBox.addButton(tr("Use Main Site via Tor"), QMessageBox::AcceptRole);
    QPushButton *retryBtn = msgBox.addButton(tr("Retry Onion"), QMessageBox::RejectRole);
    msgBox.addButton(QMessageBox::Cancel);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == useClearnetBtn) {
        if (m_torrentSearchService) {
            m_torrentSearchService->enableClearnetFallback(true);
            if (ui && ui->statusBar) {
                ui->statusBar->showMessage(tr("Clearnet fallback enabled — searches will use 1337x.to through Tor"), 5000);
            }
        }
    } else if (msgBox.clickedButton() == retryBtn) {
        // Retry the onion mirror search
        on_findOnionButton_clicked();
    }
}

void player::onOnionSitesUnavailable()
{
    // This is triggered during a search when onion sites can't be reached
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Onion Sites Unavailable"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(tr("The .onion mirror for 1337x could not be reached."));
    msgBox.setInformativeText(tr("Would you like to search using the main site (1337x.to) through Tor?\n\n"
                                 "Your connection will still be anonymous (routed through Tor), "
                                 "but will use a Tor exit node instead of staying within the onion network."));
    
    QPushButton *useClearnetBtn = msgBox.addButton(tr("Use Main Site via Tor"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel Search"), QMessageBox::RejectRole);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == useClearnetBtn) {
        if (m_torrentSearchService) {
            m_torrentSearchService->enableClearnetFallback(true);
            // Re-trigger the search with clearnet fallback now enabled
            QString query = ui->torrentSearchEdit ? ui->torrentSearchEdit->text().trimmed() : QString();
            if (!query.isEmpty()) {
                m_torrentSearchService->searchTorrents(query);
            }
            if (ui && ui->statusBar) {
                ui->statusBar->showMessage(tr("Searching 1337x.to through Tor..."), 0);
            }
        }
    }
}

void player::on_torrentSearchButton_clicked()
{
    if (!m_torrentSearchService) {
        QMessageBox::warning(this, tr("Torrent Search"), tr("Torrent search service is not available."));
        return;
    }
    
    if (!m_torNetworkService || !m_torNetworkService->isTorReady()) {
        QMessageBox::warning(this, tr("Tor Required"), 
                            tr("Please connect to Tor first before searching for torrents.\n\n"
                               "Click the \"Connect to Tor\" button to establish a secure connection."));
        return;
    }
    
    QString query = ui->torrentSearchEdit->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::warning(this, tr("Search Error"), tr("Please enter a search query."));
        return;
    }
    
    // Show legal warning
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        tr("Legal Notice"), 
        tr("⚠️ IMPORTANT LEGAL NOTICE ⚠️\n\n"
           "You are about to search for torrents. Please ensure you only download content that:\n"
           "• You have legal rights to access\n"
           "• Is not copyrighted or you own the copyright\n"
           "• Complies with your local laws\n\n"
           "XFB is not responsible for any illegal use of this feature.\n\n"
           "Do you understand and agree to use this feature responsibly?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    m_torrentSearchService->searchTorrents(query);
}

void player::on_torrentClearButton_clicked()
{
    ui->torrentSearchEdit->clear();
    
    if (m_torrentSearchService) {
        m_torrentSearchService->cancelSearch();
        
        // Clear results
        m_torrentSearchService->clearResults();
    }
}

void player::on_torrentSearchEdit_returnPressed()
{
    on_torrentSearchButton_clicked();
}

void player::onTorrentSearchResults(const QList<TorrentSearchResult> &results)
{
    if (results.isEmpty()) {
        QMessageBox::information(this, tr("Search Results"), 
                                tr("No audio torrents found for your search query."));
    } else {
        QString message = tr("Found %1 audio torrent(s) matching your search.").arg(results.size());
        ui->statusBar->showMessage(message, 5000);
    }
}

void player::onTorrentSearchError(const QString &error)
{
    QMessageBox::warning(this, tr("Search Error"), 
                        tr("Torrent search failed: %1").arg(error));
}

void player::onTorrentDownloadCompleted(const QString &downloadId, const QStringList &audioFiles)
{
    Q_UNUSED(downloadId)
    
    updateDownloadsCountLabel();
    
    // Add completed audio files to the music database
    int added = 0;
    for (const QString &filePath : audioFiles) {
        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        checkDbOpen();
        
        // Check if file already exists in database
        QSqlQuery checkQuery(db);
        checkQuery.prepare("SELECT COUNT(*) FROM musics WHERE path = ?");
        checkQuery.addBindValue(filePath);
        if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            continue;  // already in database
        }
        
        QFileInfo fileInfo(filePath);
        QSqlQuery query(db);
        query.prepare("INSERT INTO musics (artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue("Unknown");                          // artist
        query.addBindValue(fileInfo.completeBaseName());        // song
        query.addBindValue("Torrent");                          // genre1
        query.addBindValue("");                                 // genre2
        query.addBindValue("");                                 // country
        query.addBindValue(QDate::currentDate().toString("yyyy-MM-dd")); // published_date
        query.addBindValue(filePath);                           // path
        query.addBindValue("");                                 // time
        query.addBindValue(0);                                  // played_times
        query.addBindValue("-");                                // last_played
        
        if (query.exec()) {
            added++;
            qInfo() << "Added torrent audio file to database:" << filePath;
        } else {
            qWarning() << "Failed to add torrent file to database:" << query.lastError().text();
        }
    }
    
    // Update music table
    update_music_table();
    
    // Persist updated download state
    if (m_torrentDownloadService) {
        m_torrentDownloadService->saveDownloadState();
    }
    
    // Show notification
    QString message = tr("Downloaded %1 audio file(s) — %2 added to music library.")
                        .arg(audioFiles.size()).arg(added);
    QMessageBox::information(this, tr("Download Complete"), message);
}

void player::onTorrentStreamingReady(const QString &downloadId, const QString &filePath)
{
    Q_UNUSED(downloadId)
    
    // Add to playlist and start playing
    ui->playlist->addItem(filePath);
    calculate_playlist_total_time();
    
    // If not currently playing, start playing this file
    if (PlayMode == "stopped") {
        XplaylistUrls.clear();
        XplaylistUrls.append(QUrl::fromLocalFile(filePath));
        XplaylistIndex = 0;
        
        // Start playback
        on_btPlay_clicked();
    }
    
    QString message = tr("Streaming started for: %1").arg(QFileInfo(filePath).baseName());
    ui->statusBar->showMessage(message, 5000);
}

/* =========================== Audio FX (EQ / Compressor / 432 Hz) =========================== */

void player::applyStoredFxSettings()
{
    if (Xplayer)
        Xplayer->setFxParams(FxSettings::loadChannel(QStringLiteral("Main")));
    // The overlap-segue tail player carries the end of a track that started
    // on the main channel, so it must sound identical (432 Hz retune, EQ...)
    if (m_tailPlayer)
        m_tailPlayer->setFxParams(FxSettings::loadChannel(QStringLiteral("Main")));
    if (lp1_Xplayer)
        lp1_Xplayer->setFxParams(FxSettings::loadChannel(QStringLiteral("LP1")));
    if (lp2_Xplayer)
        lp2_Xplayer->setFxParams(FxSettings::loadChannel(QStringLiteral("LP2")));
}

void player::openAudioFxDialog()
{
    // When the Audio FX tab is visible, jump to it instead of opening a
    // second control surface.
    if (m_fxTabPage && ui->tabWidget_2->indexOf(m_fxTabPage) != -1) {
        ui->tabWidget_2->setCurrentWidget(m_fxTabPage);
        if (m_fxTabWidget)
            m_fxTabWidget->reloadFromSettings();
        return;
    }

    AudioFxDialog dialog(Xplayer, lp1_Xplayer, lp2_Xplayer, this);
    dialog.exec();
}

void player::convertAllMusicsTo432()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Database connection is not open."));
        return;
    }

    QStringList paths;
    QSqlQuery query(db);
    if (!query.exec("SELECT path FROM musics")) {
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to query the musics table: %1").arg(query.lastError().text()));
        return;
    }
    while (query.next())
        paths << query.value(0).toString();

    convertMusicsTo432(paths);
}

void player::convertMusicsTo432(const QStringList &paths)
{
    if (paths.isEmpty()) {
        QMessageBox::information(this, tr("Convert to 432 Hz"), tr("No tracks to convert."));
        return;
    }

    const QString ffmpegPath = FxEngine::ffmpegExecutable();
    if (ffmpegPath.isEmpty()) {
        QMessageBox::critical(this, tr("Missing Dependency"),
                              tr("The 'ffmpeg' command is required for audio conversion "
                                 "but was not found.\n\nPlease install ffmpeg and ensure it's accessible."));
        return;
    }
    const QString ffprobePath = FxEngine::ffprobeExecutable();

    // Ask how to convert
    QMessageBox choice(this);
    choice.setWindowTitle(tr("Convert to 432 Hz"));
    choice.setIcon(QMessageBox::Question);
    choice.setText(tr("Retune %n track(s) from A=440 Hz to A=432 Hz?", "", paths.size()));
    choice.setInformativeText(tr("\"Replace originals\" overwrites the audio files in place (the "
                                 "database stays unchanged).\n\n"
                                 "\"Keep originals\" writes new files with a \"_432Hz\" suffix and "
                                 "points the database at them, leaving the original files untouched.\n\n"
                                 "Files already retuned by XFB are detected (via an embedded tag) "
                                 "and skipped automatically, so a track can never be converted twice.\n\n"
                                 "Tip: for a non-destructive alternative, enable live 432 Hz playback "
                                 "in Options instead."));
    QPushButton *replaceBtn = choice.addButton(tr("Replace originals"), QMessageBox::DestructiveRole);
    QPushButton *copyBtn = choice.addButton(tr("Keep originals (make _432Hz copies)"), QMessageBox::AcceptRole);
    choice.addButton(QMessageBox::Cancel);
    choice.exec();

    const bool replaceMode = (choice.clickedButton() == replaceBtn);
    if (!replaceMode && choice.clickedButton() != copyBtn)
        return;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery queryUpdate(db);
    if (!replaceMode) {
        if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("Failed to prepare the database update query."));
            return;
        }
    }

    // A track is "already 432" when it carries the tag we embed during
    // conversion, or (copy mode) when its file name has the _432Hz suffix.
    auto isAlready432 = [&ffprobePath](const QString &path) {
        if (QFileInfo(path).completeBaseName().endsWith(QStringLiteral("_432Hz")))
            return true;
        if (ffprobePath.isEmpty())
            return false;
        QProcess probe;
        probe.start(ffprobePath, {"-v", "error",
                                  "-show_entries", "format_tags:stream_tags",
                                  "-of", "default=noprint_wrappers=1", path});
        if (!probe.waitForFinished(10000)) {
            probe.kill();
            return false;
        }
        const QString tags = QString::fromLocal8Bit(probe.readAllStandardOutput());
        return tags.contains(QStringLiteral("xfb_tuning=432"), Qt::CaseInsensitive)
               || tags.contains(QStringLiteral("XFB-432Hz"), Qt::CaseInsensitive);
    };

    QProgressDialog progressDialog(tr("Retuning tracks to 432 Hz..."), tr("Cancel"), 0,
                                   paths.size(), this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();

    int successCount = 0;
    int failCount = 0;
    int skippedCount = 0;
    int processed = 0;

    for (const QString &originalPath : paths) {
        if (progressDialog.wasCanceled())
            break;
        progressDialog.setValue(++processed);
        progressDialog.setLabelText(tr("Retuning %1 of %2:\n%3")
                                        .arg(processed).arg(paths.size())
                                        .arg(QFileInfo(originalPath).fileName()));
        qApp->processEvents();

        const QFileInfo fi(originalPath);
        if (!fi.exists() || !fi.isFile()) {
            qWarning() << "432Hz conversion: file not found, skipping:" << originalPath;
            failCount++;
            continue;
        }
        // Never convert twice: suffix (copy mode) or embedded tag (any mode)
        if (isAlready432(originalPath)) {
            skippedCount++;
            continue;
        }

        // Determine the source sample rate (default to 44100 when unknown)
        int sampleRate = 44100;
        if (!ffprobePath.isEmpty()) {
            QProcess probe;
            probe.start(ffprobePath, {"-v", "error", "-select_streams", "a:0",
                                      "-show_entries", "stream=sample_rate",
                                      "-of", "csv=p=0", originalPath});
            if (probe.waitForFinished(10000)) {
                bool ok = false;
                const int sr = QString::fromLatin1(probe.readAllStandardOutput()).trimmed().toInt(&ok);
                if (ok && sr > 0)
                    sampleRate = sr;
            }
        }
        const int retunedRate = qRound(sampleRate * 432.0 / 440.0);
        // atempo compensates the slowdown introduced by asetrate, so the
        // converted file keeps its original duration and BPM
        const double tempoComp = static_cast<double>(sampleRate) / retunedRate;

        const QString ext = fi.suffix().toLower();
        const QString tempOut = fi.absolutePath() + "/." + fi.completeBaseName()
                                + "_432tmp." + fi.suffix();

        QStringList args;
        args << "-y" << "-nostdin" << "-loglevel" << "error"
             << "-i" << originalPath
             << "-vn"
             << "-map_metadata" << "0"
             // Durable marker so this file is never converted twice and the
             // live 432 Hz mode knows not to retune it again
             << "-metadata" << "XFB_TUNING=432"
             << "-af" << QString("asetrate=%1,aresample=%2,atempo=%3")
                             .arg(retunedRate).arg(sampleRate)
                             .arg(tempoComp, 0, 'f', 8);
        if (ext == "wav") // WAV drops custom keys; use the standard comment tag
            args << "-metadata" << "comment=XFB-432Hz";
        // Sensible encoder quality for lossy targets; lossless formats ignore this
        if (ext == "mp3")
            args << "-b:a" << "320k";
        else if (ext == "ogg" || ext == "oga" || ext == "opus" || ext == "m4a" || ext == "aac")
            args << "-b:a" << "256k";
        args << tempOut;

        QProcess ffmpegProcess;
        ffmpegProcess.start(ffmpegPath, args);
        if (!ffmpegProcess.waitForFinished(600000)) {
            qWarning() << "432Hz conversion timed out for:" << originalPath;
            ffmpegProcess.kill();
            ffmpegProcess.waitForFinished(1000);
            QFile::remove(tempOut);
            failCount++;
            continue;
        }
        if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0
            || !QFileInfo::exists(tempOut) || QFileInfo(tempOut).size() == 0) {
            qWarning() << "432Hz conversion failed for:" << originalPath
                       << QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
            QFile::remove(tempOut);
            failCount++;
            continue;
        }

        if (replaceMode) {
            if (!QFile::remove(originalPath) || !QFile::rename(tempOut, originalPath)) {
                qWarning() << "432Hz conversion: could not replace original:" << originalPath;
                QFile::remove(tempOut);
                failCount++;
                continue;
            }
        } else {
            const QString newPath = fi.absolutePath() + "/" + fi.completeBaseName()
                                    + "_432Hz." + fi.suffix();
            QFile::remove(newPath); // allow re-running the conversion
            if (!QFile::rename(tempOut, newPath)) {
                qWarning() << "432Hz conversion: could not create:" << newPath;
                QFile::remove(tempOut);
                failCount++;
                continue;
            }
            queryUpdate.bindValue(":new_path", newPath);
            queryUpdate.bindValue(":old_path", originalPath);
            if (!queryUpdate.exec()) {
                qWarning() << "432Hz conversion: DB update failed for:" << originalPath
                           << queryUpdate.lastError().text();
                failCount++;
                continue;
            }
        }
        successCount++;
    }

    progressDialog.setValue(paths.size());
    update_music_table();

    QString summary = tr("432 Hz conversion finished.\n\nConverted: %1\nFailed: %2")
                          .arg(successCount).arg(failCount);
    if (skippedCount > 0)
        summary += tr("\nSkipped (already 432 Hz): %1").arg(skippedCount);
    QMessageBox::information(this, tr("Convert to 432 Hz"), summary);
}


// End of player.cpp
