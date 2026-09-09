/*
Hi and welcome. Thank you for reading my code.
I've made all in a way that I hope makes is easy for anyone to understand.
Enjoy! . Frédéric Bogaerts 2015 @ Netpack - Online Solutions!.
*/

#include "player.h"
#include "commonFunctions.h"
#include "audioformats.h"
#include "ui_player.h"
#include "add_music_single.h"
#include "add_full_dir.h"
#include "addgenre.h"
#include "addjingle.h"
#include "add_pub.h"
#include "add_program.h"
#include "optionsdialog.h"
#include "externaldownloader.h"
#include "CoverArtDialog.h"
#include "aboutus.h"
#include "audio/AudioDeviceRouter.h"
#include "audio/BpmDetector.h"
#include "audio/BpmLibrary.h"
#include "audio/CueBus.h"
#include "audio/FxEngine.h"
#include "audio/IntroLibrary.h"
#include "audio/LoudnessScanner.h"
#include "audio/WaveformStore.h"
#include "audio/VoiceDuck.h"
#include "audio/VoiceRecorder.h"
#include "ArtworkStore.h"
#include "PadBoard.h"
#include "PlaylistWaveView.h"
#include "LevelMeter.h"
#include "ThemeManager.h"
#include "dialogs/AudioFxDialog.h"
#include "secretstore.h"
#include "services/NgrokTunnelService.h"
#include "services/UpdateCheckService.h"
#include "ui/DonationNotice.h"

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
#include <QNetworkInterface>
#include <QHostAddress>
#include <QHttpPart>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointF>
#include <QVector>
#include <QMovie>
#include <QProgressDialog>
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
#include <QBuffer>
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
#include <QScopedValueRollback>
#include <QSpacerItem>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QToolBox>
#include <QToolButton>
#include <QDockWidget>
#include <QMenu>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <QHelpEvent>

#ifdef XFB_HAS_WEBENGINE
#include <QtWebEngineQuick>
#endif
#include <QQuickWidget>
#include <QQuickItem>  // full definition needed for adBanner->rootObject()

#include "services/ServiceContainer.h"
#include "services/AccessibilityManager.h"
#include "services/AccessibilitySettingsService.h"
#include "services/BrailleDisplayService.h"
#include "dialogs/AccessibilityPreferencesDialog.h"
#include "dialogs/AirLogDialog.h"
#include "dialogs/ScheduleDialog.h"
#include "services/ProgrammeSchedule.h"
#include "dialogs/QuotaDialog.h"
#include "dialogs/DeadAirDialog.h"
#include "dialogs/RequestTrayDialog.h"
#include "services/DeadAirWatchdog.h"
#include "dialogs/MobileSyncDialog.h"
#include "dialogs/ProductionSyncDialog.h"
#include "dialogs/StationSyncDialog.h"
#include "dialogs/StreamDialog.h"
#include "dialogs/VoiceTrackDialog.h"
#include "dialogs/WatchedFoldersDialog.h"
#include "services/LibraryWatcher.h"
#include "services/StreamService.h"
#include "services/AirLog.h"
#include "services/MusicQuota.h"
#include "services/RotationRules.h"
#include "dialogs/RotationDialog.h"
#include "services/HourClock.h"
#include "dialogs/HourClockDialog.h"
#include "services/TimeSignal.h"
#include "dialogs/TimeSignalDialog.h"
#include "services/MobileSyncServer.h"
#include "services/RequestLine.h"
#include "services/ProductionSyncClient.h"
#include "services/StationSyncClient.h"
#include "dialogs/AccessibilityTutorialDialog.h"
#include "dialogs/SignInDialog.h"
#include "dialogs/UsersRolesDialog.h"
#include "services/AccessControl.h"
#include "services/AudioFeedbackService.h"
#include "services/LiveRegionManager.h"
#include "services/SystemStatusAnnouncer.h"
#include "services/TorNetworkService.h"
#include "services/TorrentSearchService.h"
#include "services/TorrentDownloadService.h"
#include "services/DependencyChecker.h"
#include "mediaduration.h"

// Static variable definition for recursion protection
int player::s_recursionDepth = 0;

// Version tag of the QMainWindow dock layout stored in xfb.conf — bump it
// when the set of docks changes so stale layouts are discarded.
// v2: the artwork panel moved from its own dock into the side panel.
static constexpr int kLayoutStateVersion = 2;

// How wide the volume slider is allowed to get, whatever room the row has.
static constexpr int kVolumeSliderMaxWidth = 360;

// The transport buttons and the Auto Mode button announce their state by
// filling with colour. Every one of these fills is a light one, and the label
// on top of it has to stay readable: left to the theme, a dark theme draws its
// own near-white foreground, which on this green all but disappears. Black is
// the higher-contrast choice on all four (6:1 on the darkest of them, better
// on the rest), so it is spelled out rather than inherited.
static const char *const kAutoModeOnStyle =
    "background-color: rgb(175, 227, 59); color: black;";
static const char *const kPlayingGreenDarkStyle =
    "background-color:#5e9604; color: black;";
static const char *const kPlayingGreenLightStyle =
    "background-color:#2CCD54; color: black;";
static const char *const kPlayingAmberStyle =
    "background-color:#F0DB1B; color: black;";

// The "support XFB" notice: a beat after the window is up, then once every
// two days for as long as the session lasts.
static constexpr int kDonationNoticeStartupDelayMs = 2500;
static constexpr int kDonationNoticeIntervalMs = 2 * 24 * 60 * 60 * 1000;

// Item data role holding the audio file of a history row (its text is a
// timestamped line, not a path), so its artwork icon can be found again.
static constexpr int kArtworkPathRole = Qt::UserRole + 103;

// Renders the library's BPM column. Two rows carry no number and they mean
// different things: NULL is "nobody has looked at this yet", 0 is "looked
// at, and this track holds no steady tempo" — a real answer about the
// music, not a failure. Showing the raw 0 reads as a broken value and an
// empty cell explains nothing, so each state says what it is, in muted
// italics so it reads as an annotation rather than as data.
class BpmCellDelegate : public QStyledItemDelegate {
    // Defined in a .cpp and not run through moc, so the inherited tr() would
    // look the strings up under QStyledItemDelegate while lupdate files them
    // under BpmCellDelegate — and nothing would ever translate. This declares
    // a tr() bound to the right context without needing Q_OBJECT.
    Q_DECLARE_TR_FUNCTIONS(BpmCellDelegate)
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // Which of the three states a cell is in.
    enum State { Measured, NoSteadyTempo, NotAnalysed };

    static State stateOf(const QVariant &value) {
        if (value.isNull())
            return NotAnalysed;
        bool ok = false;
        const double bpm = value.toDouble(&ok);
        if (!ok)
            return NotAnalysed;
        return bpm > 0.0 ? Measured : NoSteadyTempo;
    }

    QString displayText(const QVariant &value, const QLocale &locale) const override {
        switch (stateOf(value)) {
        case NoSteadyTempo:
            return tr("no steady beat");
        case NotAnalysed:
            return tr("not measured");
        case Measured:
            break;
        }
        return locale.toString(value.toDouble(), 'f', 1);
    }

protected:
    void initStyleOption(QStyleOptionViewItem *option,
                         const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        if (stateOf(index.data(Qt::EditRole)) == Measured)
            return;

        option->font.setItalic(true);
        // Only the unselected colour: leaving HighlightedText alone keeps
        // the label readable on a selected row, whatever the theme.
        option->palette.setColor(QPalette::Text,
                                 option->palette.color(QPalette::Disabled, QPalette::Text));
    }

    // The label alone does not say why, and the column is too narrow to
    // spell it out — so hovering a cell that has no number explains it.
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view,
                   const QStyleOptionViewItem &option,
                   const QModelIndex &index) override {
        if (event && event->type() == QEvent::ToolTip) {
            const State state = stateOf(index.data(Qt::EditRole));
            if (state != Measured) {
                QToolTip::showText(
                    event->globalPos(),
                    state == NoSteadyTempo
                        ? tr("This track was analysed and holds no steady beat — usual "
                             "for freely-played solo, live and jazz recordings. Auto Mode "
                             "can still play it, it just will not tempo-match it.\n\n"
                             "Double-click to type a tempo in yourself.")
                        : tr("This track has not been analysed yet. Run Database → "
                             "Measure the BPM of all music tracks in the database.\n\n"
                             "Double-click to type a tempo in yourself."),
                    view);
                return true;
            }
        }
        return QStyledItemDelegate::helpEvent(event, view, option, index);
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

    m_processUptime.start();

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

    // Rebuild the .ui grid into a customizable dock layout: the Playlist /
    // History / DJ tabs stay as the central area and every other section
    // becomes a panel the user can drag somewhere else, float as its own
    // window or hide (View menu). The arrangement is saved on exit and
    // restored on the next start; View → Reset the layout brings back the
    // classic arrangement below.
    if (ui->gridLayout_2) {
        QWidget *parentWidget = ui->widget;

        // Reparent widgets to a temporary holder so they survive layout deletion
        QWidget tempHolder;
        ui->frame_4->setParent(&tempHolder);
        ui->frame->setParent(&tempHolder);
        ui->tabWidget_2->setParent(&tempHolder);
        ui->pubWidget->setParent(&tempHolder);
        ui->page_FTP_Connection->setParent(&tempHolder);

        // Delete the old grid layout (and neutralize the dangling ui pointer)
        QLayout *oldLayout = parentWidget->layout();
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item;
        }
        delete oldLayout;
        ui->gridLayout_2 = nullptr;

        // Central area: the main tabs inside a thin splitter (kept as the
        // level meter's vertical docking spot — see updateConfig).
        QVBoxLayout *vbox = new QVBoxLayout(parentWidget);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);
        m_middleSplitter = new QSplitter(Qt::Horizontal, parentWidget);
        m_middleSplitter->setChildrenCollapsible(false);
        ui->tabWidget_2->setParent(m_middleSplitter);
        m_middleSplitter->addWidget(ui->tabWidget_2);
        vbox->addWidget(m_middleSplitter);

        ui->frame->setMinimumWidth(200);
        ui->frame->setMaximumWidth(16777215); // remove the 350 cap
        ui->page_FTP_Connection->setMaximumWidth(16777215); // remove the 350 cap
        ui->page_FTP_Connection->setMinimumWidth(150);

        // Dock panels. The object names are what saveState()/restoreState()
        // key the stored layout on — never rename them.
        auto makeDock = [this](const QString &objectName, const QString &title,
                               QWidget *content) {
            auto *dock = new QDockWidget(title, this);
            dock->setObjectName(objectName);
            dock->setWidget(content);
            return dock;
        };
        m_dockPlayer = makeDock(QStringLiteral("dockPlayer"),
                                tr("Player controls"), ui->frame_4);
        m_dockClock = makeDock(QStringLiteral("dockClock"),
                               tr("Clock"), ui->frame);
        m_dockLibrary = makeDock(QStringLiteral("dockLibrary"),
                                 tr("Library"), ui->pubWidget);

        // Track artwork: the store extracts covers in the background and
        // this panel shows the cover of the track on air. It sits at the
        // top of the side panel rather than in a dock of its own, so
        // collapsing the side panel reclaims the whole right column for
        // the tabs (a separate artwork dock kept the column open).
        m_artStore = new ArtworkStore(this);
        m_artPanel = new NowPlayingArtPanel(m_artStore, this);
        auto *sideContainer = new QWidget(this);
        auto *sideVbox = new QVBoxLayout(sideContainer);
        sideVbox->setContentsMargins(0, 0, 0, 0);
        sideVbox->setSpacing(0);
        sideVbox->addWidget(m_artPanel, 0);
        sideVbox->addWidget(ui->page_FTP_Connection, 1);
        m_dockSide = makeDock(QStringLiteral("dockSide"),
                              tr("Side panel"), sideContainer);

        // Top and bottom docks span the full window width so the default
        // arrangement mirrors the classic XFB layout.
        setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
        setDockNestingEnabled(true);

        addDockWidget(Qt::TopDockWidgetArea, m_dockPlayer);
        addDockWidget(Qt::TopDockWidgetArea, m_dockClock);
        addDockWidget(Qt::RightDockWidgetArea, m_dockSide);
        addDockWidget(Qt::BottomDockWidgetArea, m_dockLibrary);
        resizeDocks({m_dockPlayer, m_dockClock}, {1100, 360}, Qt::Horizontal);
        resizeDocks({m_dockSide}, {330}, Qt::Horizontal);
        resizeDocks({m_dockLibrary}, {260}, Qt::Vertical);

        // Always-visible toggle in the tab-bar corner to show/hide the side
        // panel (Search / Filters / Extras / Playlist) — kept from the
        // pre-dock days; it now drives the dock's visibility.
        m_sidePanelToggle = new QToolButton(ui->tabWidget_2);
        m_sidePanelToggle->setCheckable(true);
        m_sidePanelToggle->setChecked(true);
        m_sidePanelToggle->setAutoRaise(true);
        m_sidePanelToggle->setArrowType(Qt::RightArrow);
        m_sidePanelToggle->setToolTip(tr("Show or hide the side panel (Search, Filters, Extras, Playlist)"));
        ui->tabWidget_2->setCornerWidget(m_sidePanelToggle, Qt::TopRightCorner);
        connect(m_sidePanelToggle, &QToolButton::toggled, this, [this](bool on) {
            if (m_dockSide)
                m_dockSide->setVisible(on);
            if (m_sidePanelToggle)
                m_sidePanelToggle->setArrowType(on ? Qt::RightArrow : Qt::LeftArrow);
        });
        connect(m_dockSide, &QDockWidget::visibilityChanged, this, [this](bool visible) {
            if (!m_sidePanelToggle)
                return;
            QSignalBlocker blocker(m_sidePanelToggle);
            m_sidePanelToggle->setChecked(visible);
            m_sidePanelToggle->setArrowType(visible ? Qt::RightArrow : Qt::LeftArrow);
        });

        // View menu: per-panel visibility, plus locking and resetting the
        // whole arrangement.
        QMenu *viewMenu = new QMenu(tr("View"), this);
        // Every menu entry carries an icon, so a panel can be picked out of
        // the list by its picture rather than by reading all four names.
        const struct { QDockWidget *dock; const char *icon; } dockEntries[] = {
            {m_dockPlayer,  ":/icons/flat/Vertical Settings Mixer-32.png"},
            {m_dockClock,   ":/icons/player-time.png"},
            {m_dockSide,    ":/icons/ic_menu_search.png"},
            {m_dockLibrary, ":/icons/flat/Database-48.png"},
        };
        for (const auto &entry : dockEntries) {
            QAction *toggle = entry.dock->toggleViewAction();
            toggle->setIcon(QIcon(QLatin1String(entry.icon)));
            viewMenu->addAction(toggle);
        }
        // The artwork panel lives inside the side panel; this toggle only
        // shows/hides it there (persisted separately from the dock state).
        QAction *artworkAction = viewMenu->addAction(QIcon(":/icons/insert-image.png"),
                                                     tr("Artwork"));
        artworkAction->setCheckable(true);
        {
            QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + "/xfb.conf", QSettings::IniFormat);
            const bool showArt = settings.value("ShowArtworkPanel", true).toBool();
            artworkAction->setChecked(showArt);
            m_artPanel->setVisible(showArt);
        }
        connect(artworkAction, &QAction::toggled, this, [this](bool on) {
            if (m_artPanel)
                m_artPanel->setVisible(on);
            QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + "/xfb.conf", QSettings::IniFormat);
            settings.setValue("ShowArtworkPanel", on);
        });
        viewMenu->addSeparator();
        m_lockLayoutAction = viewMenu->addAction(QIcon(":/icons/lock.png"),
                                                 tr("Lock the layout"));
        m_lockLayoutAction->setCheckable(true);
        m_lockLayoutAction->setToolTip(tr("Hide the panel title bars and prevent the panels "
                                          "from being moved or closed"));
        connect(m_lockLayoutAction, &QAction::toggled, this, [this](bool locked) {
            setLayoutLocked(locked);
            QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + "/xfb.conf", QSettings::IniFormat);
            settings.setValue("LayoutLocked", locked);
        });
        QAction *resetLayoutAction = viewMenu->addAction(QIcon(":/icons/document-revert.png"),
                                                         tr("Reset the layout"));
        connect(resetLayoutAction, &QAction::triggered,
                this, &player::resetDockLayout);
        if (ui->menuHelp && ui->menuHelp->menuAction())
            ui->menuBar->insertMenu(ui->menuHelp->menuAction(), viewMenu);
        else
            ui->menuBar->addMenu(viewMenu);

        // The bottom logo strip joins the status bar so the dock panels get
        // the full bottom edge of the window: logo on the left (a normal
        // status-bar widget — Qt hides it briefly while a temporary status
        // message shows), info line permanent on the right.
        if (ui->txt_bottom_info && ui->NetpackLogo && ui->statusBar) {
            ui->statusBar->addWidget(ui->NetpackLogo);
            ui->statusBar->addPermanentWidget(ui->txt_bottom_info);
            if (ui->gridLayout && ui->horizontalLayout_11) {
                ui->gridLayout->removeItem(ui->horizontalLayout_11);
                delete ui->horizontalLayout_11;
                ui->horizontalLayout_11 = nullptr;
            }
        }

        // Remember this default arrangement (for View → Reset the layout),
        // then bring back whatever the user had last time.
        m_defaultLayoutState = saveState(kLayoutStateVersion);
        {
            QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + "/xfb.conf", QSettings::IniFormat);
            const QByteArray state = settings.value("MainWindowState").toByteArray();
            if (!state.isEmpty() && !restoreState(state, kLayoutStateVersion))
                qWarning() << "Could not restore the saved panel layout, using defaults";
            if (!settings.value("FullScreen", false).toBool()) {
                const QByteArray geometry = settings.value("MainWindowGeometry").toByteArray();
                if (!geometry.isEmpty())
                    restoreGeometry(geometry);
            }
            if (settings.value("LayoutLocked", false).toBool())
                m_lockLayoutAction->setChecked(true); // triggers setLayoutLocked
        }

        // The player panel's controls are placed by hand for whatever width
        // the panel ends up with — see relayoutPlayerFrame().
        ui->frame_4->installEventFilter(this);
        ui->frame_4->setMinimumHeight(121);
        // A volume slider is a coarse control — a couple of hundred pixels is
        // already finer than anyone needs, and a metre of it is just harder to
        // aim at. Capping it also means toggling the wave view (which takes
        // the progress slider off the row beside it) no longer changes the
        // size of the volume control under the operator's hand.
        ui->sliderVolume->setMaximumWidth(kVolumeSliderMaxWidth);
        // Their labels must not soak up the room the sliders give back, or
        // hiding the progress slider leaves "Volume" stretched across half the
        // panel with its slider stranded on the far right.
        ui->label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        ui->label_2->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        // With nothing able to grow, a QHBoxLayout centres what it has, which
        // would slide the volume control into the middle of the row whenever
        // the progress slider went away. This spacer claims the free room at
        // the right-hand end instead, but only while there is any.
        m_volumeRowSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed,
                                            QSizePolicy::Minimum);
        ui->horizontalLayout_7->addItem(m_volumeRowSpacer);
        relayoutPlayerFrame();
        ui->frame->installEventFilter(this);
        relayoutClockFrame();

        qDebug() << "Rebuilt the main window into a customizable dock layout";
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
        // Tempo comes out of the same decode the waveforms do, so the BPM
        // library shares the store rather than reading the files again.
        m_bpmLibrary = new BpmLibrary(m_waveStore, this);
        // Intro/outro times come off the very same decode, for the same
        // reason: nothing here reads an audio file that WaveformStore has
        // not already been asked for.
        m_introLibrary = new IntroLibrary(m_waveStore, this);
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

            // Auto-mix quietness threshold (% of a track's own max peak).
            // Config-only — written back once so the key is discoverable.
            const int autoMixThr =
                qBound(1, waveSettings.value("AutoMixThresholdPercent", 5).toInt(), 50);
            PlaylistWaveView::setAutoMixThresholdPercent(autoMixThr);
            // The intro detector trims the quiet edges with the same rule,
            // so it has to see the same number.
            m_introLibrary->setThresholdPercent(autoMixThr);
            if (!waveSettings.contains("AutoMixThresholdPercent"))
                waveSettings.setValue("AutoMixThresholdPercent", autoMixThr);

            // How much of a track has to agree on a tempo before BPM
            // reports one. Config-only, same as the threshold above:
            // lowering it fills in more of the library at the cost of
            // tempos the music does not actually hold to.
            const int bpmAgreement =
                qBound(10, waveSettings.value("BpmMinAgreementPercent", 35).toInt(), 100);
            BpmDetector::setMinAgreementPercent(bpmAgreement);
            if (!waveSettings.contains("BpmMinAgreementPercent"))
                waveSettings.setValue("BpmMinAgreementPercent", bpmAgreement);
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

        // Auto-mix: crossfade-prep the whole playlist in one click
        m_autoMixButton = new QToolButton(waveBar);
        m_autoMixButton->setText(tr("Auto-mix"));
        m_autoMixButton->setToolTip(tr("Analyze every transition in the playlist and set the "
                                       "crossfade overlaps automatically: each track starts "
                                       "where the previous one goes quiet. Existing overlaps "
                                       "are recomputed."));
        m_autoMixButton->setVisible(false); // shown when wave view turns on
        waveBarLayout->addWidget(m_autoMixButton);
        connect(m_autoMixButton, &QToolButton::clicked,
                this, [this]() { startAutoMix({}); });

        // Connected once here rather than in startAutoMix(): with every
        // waveform already cached, autoMix() finishes before it returns.
        connect(m_waveView, &PlaylistWaveView::autoMixProgress,
                this, [this](int done, int total) {
            if (m_autoMixProgress) {
                m_autoMixProgress->setMaximum(total);
                m_autoMixProgress->setValue(done);
            }
        });
        connect(m_waveView, &PlaylistWaveView::autoMixFinished,
                this, [this](int applied, int skipped, bool canceled) {
            if (m_autoMixProgress)
                m_autoMixProgress->deleteLater();
            if (m_autoMixButton)
                m_autoMixButton->setEnabled(true);
            ui->statusBar->showMessage(
                canceled
                    ? tr("Auto-mix canceled — %1 transition(s) set").arg(applied)
                    : tr("Auto-mix: %1 transition(s) set, %2 skipped")
                          .arg(applied).arg(skipped),
                5000);
        });

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

        // The operator dragged the intro marker: that value is theirs from
        // now on. It is written with the lock set, so neither the lazy
        // measurement nor a library sweep can put the detector's guess back.
        connect(m_nowPlayingWave, &NowPlayingWaveStrip::introEdited,
                this, [this](qint64 introMs) {
            const QString path = m_nowPlayingWave->track();
            m_currentIntroMs = introMs;
            m_currentIntroLocked = true;
            if (!m_introLibrary || path.isEmpty())
                return;
            if (m_introLibrary->setIntroByHand(path, introMs)) {
                announceAccessible(tr("Intro set to %1")
                                       .arg(spokenDuration(introMs)));
            } else {
                // A file dropped straight into the playlist is not in the
                // musics table, so there is no row to store it on. Say so
                // rather than letting the marker lie about being saved.
                ui->statusBar->showMessage(
                    tr("This track is not in the database, so its intro "
                       "cannot be saved"), 6000);
            }
        });

        // Seeking from the strip. Wave view hides the seek slider (the strip
        // draws the playhead instead), so without this there is no way to
        // move through the track on air at all while the view is on.
        connect(m_nowPlayingWave, &NowPlayingWaveStrip::seekRequested,
                this, [this](qint64 positionMs) {
            if (disableSeekBar || !Xplayer)
                return;
            Xplayer->setPosition(positionMs);
            ui->sliderProgress->setValue(int(positionMs)); // it is only hidden
            announceAccessible(tr("Seek to %1").arg(spokenDuration(positionMs)));
        });

        connect(m_nowPlayingWave, &NowPlayingWaveStrip::introResetRequested,
                this, [this]() {
            const QString path = m_nowPlayingWave->track();
            if (!m_introLibrary || path.isEmpty())
                return;
            m_introLibrary->clearLock(path);
            m_currentIntroMs = -1;
            m_currentOutroMs = -1;
            m_currentIntroLocked = false;
            m_introAnnouncedPath.clear();
            m_nowPlayingWave->setIntro(-1, -1, false);
            // The row now looks exactly like one that was never measured,
            // so the ordinary lazy path picks it straight back up.
            refreshIntroForCurrentTrack();
        });

        // A measurement that lands while the track is on air (the lazy
        // analysis of a track nobody had played before) updates the strip
        // and is spoken, since the operator was told there was no number.
        connect(m_introLibrary, &IntroLibrary::introMeasured, this,
                [this](const QString &path, const IntroTimes &times) {
            if (!m_nowPlayingWave || path != m_nowPlayingWave->track())
                return;
            applyIntroTimes(times);
            if (times.introMs >= 0 && m_introAnnouncedPath != path) {
                m_introAnnouncedPath = path;
                announceAccessible(introAnnouncement());
            }
        });

        connect(m_waveViewToggle, &QToolButton::toggled,
                this, &player::setPlaylistWaveView);

        // Track artwork: every playlist row gets its cover as an icon
        // (extracted in the background, cached on disk); history rows are
        // tagged with their file when they are added so they get one too.
        ui->playlist->setIconSize(QSize(28, 28));
        if (ui->historyList)
            ui->historyList->setIconSize(QSize(20, 20));
        connect(ui->playlist->model(), &QAbstractItemModel::rowsInserted, this,
                [this](const QModelIndex &, int first, int last) {
            for (int row = first; row <= last; ++row)
                requestItemArtwork(ui->playlist->item(row), QString());
        });
        connect(m_artStore, &ArtworkStore::artworkReady,
                this, &player::onArtworkReady);
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

    // Stereo output level meter next to the volume slider. Created hidden;
    // updateConfig() below applies the saved Options visibility.
    m_levelMeter = new LevelMeter(this);
    m_levelMeter->setVisible(false);
    ui->horizontalLayout_7->insertWidget(2, m_levelMeter);

    // Clicking the seek bar's groove (page/single steps) must also seek:
    // those actions emit neither sliderMoved nor sliderReleased. Deferred a
    // tick so the slider's value has settled before we read it.
    connect(ui->sliderProgress, &QAbstractSlider::actionTriggered, this,
            [this](int action) {
        if (action == QAbstractSlider::SliderPageStepAdd
            || action == QAbstractSlider::SliderPageStepSub
            || action == QAbstractSlider::SliderSingleStepAdd
            || action == QAbstractSlider::SliderSingleStepSub) {
            QTimer::singleShot(0, this, [this]() {
                if (!ui->sliderProgress->isSliderDown())
                    Xplayer->setPosition(ui->sliderProgress->value());
            });
        }
    });

    // Auto Auto-mix (Options): every track added to the playlist gets its
    // crossfade overlap computed right away, silently in the background —
    // same engine as the Auto-mix button, without the progress dialog.
    connect(ui->playlist->model(), &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex &, int first, int last) {
        if (!m_autoAutoMix || !m_waveView)
            return;
        QVector<int> rows;
        for (int r = first; r <= last; ++r)
            rows.append(r);
        // Deferred one tick: the inserter may still be filling item roles
        QTimer::singleShot(0, this, [this, rows]() {
            if (m_autoAutoMix && m_waveView && !m_waveView->autoMixActive())
                m_waveView->autoMix(rows);
        });
    });

    qDebug() << "Initializing database and UI components";

    // Initialize database connection with error handling
    try {
        updateConfig();
        if (!checkDbOpen()) {
            qCritical() << "Failed to open database - this may cause crashes";
            // Don't throw here, let the app try to continue
        } else {
            qDebug() << "Database opened successfully";
            seedDefaultGenres();
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
            m_tailFadeFactor = v.toDouble();
            applyTailVolume(m_tailPlayer ? m_tailPlayer->position() : 0);
        });
        connect(m_tailFade, &QVariantAnimation::finished, this, [this]() {
            if (m_tailPlayer)
                m_tailPlayer->stop();
        });
        // The tail's volume is the fade above TIMES the outgoing track's own
        // volume line, so a line the operator drew is still heard as the track
        // leaves. A voice-track segue sets the fade aside entirely and lets the
        // line alone carry it (see m_tailEnvelope).
        connect(m_tailPlayer, &FxPlayer::positionChanged,
                this, &player::onTailPositionChanged);
        connect(m_tailPlayer, &FxPlayer::mediaStatusChanged, this,
                [this](QMediaPlayer::MediaStatus status) {
            if (m_tailEnvelopeActive && status == QMediaPlayer::EndOfMedia)
                stopTailPlayer();
        });

        // Cue bus: its own pair of players, permanently bound to the cue
        // device. Created before the routing settings are applied below so
        // it never exists in an unrouted state.
        setupCueBus();

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
        // Every player now exists: push the configured on-air and cue output
        // devices into them (updateConfig() ran before they were built).
        applyOutputDeviceSettings();
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
        // Loudness normalisation: the gain for the track about to play is
        // computed here, before any audio comes out. sourceChanged covers
        // every route into playback (manual, auto mode, gapless handoff),
        // which is why it is hooked here rather than at each setSource().
        connect(Xplayer, &FxPlayer::sourceChanged,
                this, &player::applyLoudnessForSource);
        connect(Xplayer, &FxPlayer::levels, m_levelMeter, &LevelMeter::setLevels);
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
                // Lazy measurement: the intro of a track is worked out the
                // first time it goes to air, so an operator never has to
                // run the library sweep to get the feature at all.
                refreshIntroForCurrentTrack();
            }
            if (m_artPanel)
                m_artPanel->setTrack(url.isLocalFile() ? url.toLocalFile()
                                                       : QString());
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
            // Leaving PausedState by any route other than the pause button
            // (Play Next/Previous, stop, auto-advance) must clear the paused
            // flag, or the button stays yellow and the next click "resumes".
            if (state != QMediaPlayer::PausedState && playPause) {
                playPause = false;
                ui->bt_pause_play->setStyleSheet("");
                refreshTransportAccessibleState();
            }
            if (state == QMediaPlayer::StoppedState && PlayMode == "Playing_Segue" && !m_manualAdvancing) {
                // When playback stops, play the next media if in segue mode.
                // A 0 ms shot still defers through the event loop (no
                // re-entry) but adds no dead air between tracks.
                QTimer::singleShot(0, this, &player::playNextMedia);
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
        // Position stuck at 0 counts as a stall too: a wedged audio output
        // device leaves the FX engine "playing" at 0 with no error emitted,
        // so excluding 0 here made that failure completely silent. The first
        // tick after play() never strikes (m_lastKnownPosition starts at -1),
        // so a track still gets ~9 s to produce its first frames.
        if (currentPos == m_lastKnownPosition) {
            m_stallCount++;
            qWarning() << "Playback stall detected! Position stuck at" << currentPos << "ms (count:" << m_stallCount << ")";
            
            if (m_stallCount >= 2) {
                // Playback has been stuck for 6+ seconds — force recovery
                qWarning() << "Forcing player recovery after stall on:" << Xplayer->source().toString();
                ui->statusBar->showMessage(tr("Playback stalled — recovering..."), 5000);
                
                m_manualAdvancing = true;
                Xplayer->stop();
                Xplayer->setSource(QUrl()); // Release stuck AVFoundation session
                // Rebuild the FX engine's audio sink: a wedged output device
                // is the usual cause of a frozen position, and reusing the
                // same sink would stall every following track too, leaving
                // playback stuck in a recovery loop.
                Xplayer->resetAudioSink();
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

    // The scheduler runs whatever role this machine is in. It only ever reads
    // this installation's own scheduler table and queues into this
    // installation's own running order — there is nothing server-side about
    // it. Gating it on Role meant that on a default install (Role is Client)
    // every schedule an operator wrote in Add a publicity / Add a program was
    // silently never checked, which is not something the dialog that wrote
    // them gives any hint of.
    QTimer *schedulerTimer = new QTimer(this);
    connect(schedulerTimer, &QTimer::timeout, this, &player::run_scheduler);
    schedulerTimer->start(60000);

    if(Role=="Server"){

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

                applyMusicHeaderLabels(model);
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
    // As-run log: close whatever a crash left hanging and drop anything past
    // the retention window, before the first track of this session opens a row.
    AirLog::instance()->start();

    // The hour clock's fixed-item timer. Does nothing at all unless the
    // station has switched the feature on in the Hour Clocks window.
    setupHourClock();
    setupTimeSignals();

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
     
     // Accessibility improvements for keyboard navigation.
     // Tab key navigation stays OFF: with it on, Tab walks cell to cell inside
     // the table and focus can never leave it, which traps anyone working
     // without a mouse. Off, Tab and Shift+Tab move between panels (what a
     // screen-reader user expects) and the arrow keys move within the table.
     ui->musicView->setFocusPolicy(Qt::StrongFocus);
     ui->musicView->setTabKeyNavigation(false);
     ui->musicView->setAttribute(Qt::WA_KeyboardFocusChange, true);

     ui->musicView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->musicView, &QWidget::customContextMenuRequested,
         this, &player::musicViewContextMenu);

     ui->playlist->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->playlist, &QWidget::customContextMenuRequested,
             this, &player::playlistContextMenu);

     // Accessibility improvements for jinglesView
     ui->jinglesView->setFocusPolicy(Qt::StrongFocus);
     ui->jinglesView->setTabKeyNavigation(false);  // see musicView above
     ui->jinglesView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->jinglesView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->jinglesView, &QWidget::customContextMenuRequested,
         this, &player::jinglesViewContextMenu);

     // Accessibility improvements for pubView
     ui->pubView->setFocusPolicy(Qt::StrongFocus);
     ui->pubView->setTabKeyNavigation(false);  // see musicView above
     ui->pubView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->pubView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->pubView, &QWidget::customContextMenuRequested,
         this, &player::pubViewContextMenu);

     // Accessibility improvements for programsView
     ui->programsView->setFocusPolicy(Qt::StrongFocus);
     ui->programsView->setTabKeyNavigation(false);  // see musicView above
     ui->programsView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->programsView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->programsView, &QWidget::customContextMenuRequested,
         this, &player::programsViewContextMenu);

     // Accessibility improvements for torrentsView
     ui->torrentsView->setFocusPolicy(Qt::StrongFocus);
     ui->torrentsView->setTabKeyNavigation(false);  // see musicView above
     ui->torrentsView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->torrentsView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->torrentsView, &QWidget::customContextMenuRequested,
         this, &player::torrentsViewContextMenu);

     // Genre combo boxes will be populated by update_music_table()

    update_music_table();

    // Repair library rows with a missing duration once the window is up
    // (deferred so startup and the first paint aren't delayed by probes).
    QTimer::singleShot(5000, this, &player::startTimeBackfill);

   /*Bottom info — single line, it lives in the status bar now. The version is
     prepended at runtime from QCoreApplication (set once in main.cpp) rather
     than baked into the .ui: hardcoding it there meant the footer silently kept
     showing an old release, and it also made the string one more place to bump.*/
   QDir dir; QString cpath = dir.absolutePath();
   QString binfo = QStringLiteral("XFB v%1 ").arg(QCoreApplication::applicationVersion())
                   + ui->txt_bottom_info->text().replace('\n', QStringLiteral("  •  "))
                   + "  •  " + cpath + "  •  " + Role;
   ui->txt_bottom_info->setText(binfo);

   /*Default button states*/
   ui->bt_takeOver->setEnabled(false);
   ui->bt_pause_rec->setEnabled(false);



   if(Role=="Server"){

       autoMode = 1;
       qDebug()<<"Role is set to Server, so autoMode is ON by default";
       ui->bt_autoMode->setStyleSheet(kAutoModeOnStyle);
       ui->bt_takeOver->setHidden(true);
       ui->menuClient_3->setEnabled(false);

   }else{

       qDebug("XFB is now running in client mode!");

       autoMode = 0;
       qDebug()<<"autoMode is OFF";
       ui->bt_autoMode->setStyleSheet("");
       ui->menuServer->setEnabled(false);

   }


    // The reminder to support XFB used to be a modal dialog fired in front of
    // the splash screen — nothing could be done until it was dismissed. It is
    // now a small notice in the corner of the window: it appears shortly after
    // startup and again every two days for sessions that stay open (this app
    // is often left running for days), and closing it costs one click.
    // Scripted runs (--no-dialogs) never see it.
    if (!QApplication::arguments().contains("--no-dialogs")) {
        m_donationNotice = new DonationNotice(this);

        auto showNotice = [this]() {
            if (!m_donationNotice)
                return;
            qDebug() << "Showing the donation notice";
            m_donationNotice->popUp();
            announceAccessible(m_donationNotice->spokenText());
        };

        // Let the main window finish appearing before the notice fades in.
        QTimer::singleShot(kDonationNoticeStartupDelayMs, this, showNotice);

        m_donationNoticeTimer = new QTimer(this);
        m_donationNoticeTimer->setInterval(kDonationNoticeIntervalMs);
        connect(m_donationNoticeTimer, &QTimer::timeout, this, showNotice);
        m_donationNoticeTimer->start();
    } else {
        qDebug() << "Skipping the donation notice (--no-dialogs flag)";
    }

   // Theming (palette + stylesheet) is applied application-wide by
   // ThemeManager (main.cpp at startup, updateConfig on options changes) —
   // no per-widget style overrides needed here anymore.

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

           // Nothing is provisioned in advance any more. XFB used to fetch
           // yt-dlp and FFmpeg a second and a half after startup so the first
           // download would not have to wait; that put a download toolchain on
           // every desk, including the great majority whose operators cannot
           // see the downloader at all. Every path that needs a tool asks for
           // it where it is needed — ensureYtDlp() when the downloader opens,
           // ensureDependency("ffmpeg" / "sox" / "tor" / …) at the feature
           // that uses it — which is both later and honest about why.

           // Bring the Tor/torrent services up only when the feature is
           // enabled. A disabled feature creates no services, opens no ports
           // and never touches the network — this is the real kill-switch.
           // ensureTorrentServices() is idempotent and is also called when
           // the Options toggle turns the feature on at runtime.
           {
               QSettings torCfg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                    + "/xfb.conf", QSettings::IniFormat);
               // The role at the desk counts too: an operator without
               // "Support .torrent files" has no tab and must have no Tor
               // process either, or the kill-switch is only a hidden tab.
               // A later sign-in that does have it brings them up
               // (applyTorrentTabVisibility).
               if (torCfg.value("EnableTorrents", false).toBool()
                   && AccessControl::instance().allows(QStringLiteral("downloads.torrents")))
                   ensureTorrentServices();
           }
       } catch (const std::exception& e) {
           qWarning() << "Exception during torrent service initialization:" << e.what();
           m_torNetworkService = nullptr;
           m_torrentSearchService = nullptr;
           m_torrentDownloadService = nullptr;
       }
   } else {
       // Command-line / minimal mode: no torrent services at all
       qDebug() << "Skipping torrent services initialization";
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
           // Name the icon-only controls and give the transport keyboard
           // shortcuts + menu entries (the app previously had neither).
           setupAccessibleControls();
           setupPlaybackShortcuts();
           // The cue menu entry exists only now, so give it its status tip
           // (which is the "why cueing is off" text when there is no ear).
           applyOutputDeviceSettings();
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
       AccessControl::instance().guard(conv432All, QStringLiteral("library.retune"));
       connect(conv432All, &QAction::triggered, this, &player::convertAllMusicsTo432);
   }

   // Tempo analysis of the library, which is what auto mode's BPM matching
   // draws on. Only ever touches tracks that have never been measured.
   {
       QAction *analyzeBpm = new QAction(QIcon(":/icons/chronometer.png"),
                                         tr("Measure the BPM of all music tracks in the database"), this);
       analyzeBpm->setToolTip(tr("Measure the tempo of every track that does not have one yet, "
                                 "so Auto Mode can follow a track with one at a similar tempo. "
                                 "Each track is decoded once; the result is stored in the "
                                 "database."));
       ui->menuDatabase->addAction(analyzeBpm);
       AccessControl::instance().guard(analyzeBpm, QStringLiteral("library.analyse"));
       connect(analyzeBpm, &QAction::triggered, this, &player::analyzeLibraryBpm);
   }

   // Intro / outro times. A third sweep of the same shape as the two around
   // it, off the same waveform decode as the BPM one — what it feeds is the
   // countdown on the now-playing strip and the spoken ramp, so a presenter
   // knows exactly when to stop talking.
   {
       QAction *analyzeIntro = new QAction(QIcon(":/icons/chronometer.png"),
                                           tr("Measure the intro and outro times of the database"), this);
       analyzeIntro->setToolTip(tr("Work out, for every music track that has not been measured "
                                   "yet, how long its intro runs before the vocal and how long "
                                   "its run-out is. This is a level heuristic, not vocal "
                                   "detection: correct anything it gets wrong by dragging the "
                                   "marker on the now-playing wave strip, and your value is "
                                   "kept the next time this runs."));
       ui->menuDatabase->addAction(analyzeIntro);
       AccessControl::instance().guard(analyzeIntro, QStringLiteral("library.analyse"));
       connect(analyzeIntro, &QAction::triggered, this, &player::analyzeLibraryIntro);
   }

   // EBU R128 loudness. The measurement is a library sweep like the BPM one
   // above; what it feeds is the per-track playback gain, so a quiet
   // transfer no longer arrives many dB under the track before it. Nothing
   // is re-encoded — the correction is applied live, at playback.
   {
       m_loudnessScanner = new LoudnessScanner(this);
       connect(m_loudnessScanner, &LoudnessScanner::measured,
               this, [this](const LoudnessMeasurement &m) { storeLoudness(m); });

       QAction *scanLoudness = new QAction(QIcon(":/icons/chronometer.png"),
                                          tr("Measure the loudness (EBU R128) of the database"), this);
       scanLoudness->setToolTip(tr("Measure the integrated loudness and true peak of every track, "
                                   "jingle, commercial and programme that has not been measured "
                                   "yet, so playback can put them all on air at the same level. "
                                   "Files are never modified. Anything whose file changed since it "
                                   "was measured is measured again."));
       ui->menuDatabase->addAction(scanLoudness);
       AccessControl::instance().guard(scanLoudness, QStringLiteral("library.analyse"));
       connect(scanLoudness, &QAction::triggered, this, &player::scanLibraryLoudness);

       // updateConfig() ran before the players existed (it is called at the
       // top of this constructor), so the saved settings are pushed into
       // them here, now that they do.
       applyLoudnessSettings();
   }

   // Cover art for the downloads that predate XFB keeping one. Unlike the
   // sweeps above this one goes out to the internet and rewrites files, so it
   // is a window the operator drives rather than a sweep that just runs.
   {
       QAction *findCovers = new QAction(QIcon(":/icons/insert-image.png"),
                                         tr("Find the missing cover art…"), this);
       findCovers->setToolTip(tr("Look for a cover for every track in the library that has "
                                 "none, and — once you have seen what was found — write it "
                                 "into the file. Downloads from before XFB kept the cover "
                                 "have nothing to show on the deck, the phone's notification "
                                 "or the lock screen."));
       ui->menuDatabase->addAction(findCovers);
       AccessControl::instance().guard(findCovers, QStringLiteral("library.coverart"));
       connect(findCovers, &QAction::triggered, this, [this]() {
           // Parented to the window but not modal: finding covers for a whole
           // library is long, and nothing here stops the station playing.
           auto *dialog = new CoverArtDialog(this);
           dialog->setAttribute(Qt::WA_DeleteOnClose);
           dialog->show();
       });
   }

   // Folders that feed the library on their own. Sits with the other ways
   // material gets in rather than with the station-wide settings: an operator
   // looking for "how do songs get into XFB" looks in this menu.
   {
       QAction *watchedFolders = new QAction(QIcon(":/icons/flat/Import-48.png"),
                                             tr("Watched folders…"), this);
       watchedFolders->setMenuRole(QAction::NoRole);
       watchedFolders->setToolTip(tr("Folders XFB keeps an eye on: anything new that "
                                     "lands in one is added to the music, jingles, "
                                     "publicities or programs library on its own."));
       ui->menuDatabase->addAction(watchedFolders);
       AccessControl::instance().guard(watchedFolders,
                                       QStringLiteral("library.watchedfolders"));
       connect(watchedFolders, &QAction::triggered, this, [this]() {
           if (!m_watchedFoldersDialog) {
               m_watchedFoldersDialog = new WatchedFoldersDialog(libraryWatcher(), this);
               m_watchedFoldersDialog->setAttribute(Qt::WA_DeleteOnClose, false);
               connect(m_watchedFoldersDialog, &WatchedFoldersDialog::announcementRequested,
                       this, &player::announceAccessible);
           }
           m_watchedFoldersDialog->show();
           m_watchedFoldersDialog->raise();
           m_watchedFoldersDialog->activateWindow();
       });

       // Constructing it is what arms it, so folders an operator turned on
       // are being watched from startup without opening the window. With it
       // off nothing is constructed and no timer runs.
       if (LibraryWatcher::loadConfig().enabled)
           libraryWatcher();
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

   // Pads tab, right after the DJ tab (hide it via Options → ShowPadsTab).
   // A grid of labelled, coloured pads that fire a jingle, a stab or a bed
   // on a single press — built to be driven from a touch screen.
   {
       m_padBoard = new PadBoardWidget(this);
       // A pad can be auditioned before it goes to air; the cue bus decides
       // whether that is possible and says why when it is not.
       connect(m_padBoard, &PadBoardWidget::cueRequested, this,
               [this](const QString &path, const QString &label) {
           cueFile(path, label);
       });
       // A 4x6 grid of finger-sized pads insists on some 310 px of height, and
       // a QTabWidget is as tall as its tallest page whichever page is on
       // show — so without this the Pads tab alone kept the central area from
       // ever shrinking, and the library panel underneath had almost no travel
       // left to be dragged. In the scroll area the pads shrink first and
       // scroll after that.
       m_padBoard->setOutputDeviceId(m_mainOutputDeviceId); // pads go to air
       m_padBoardPage = wrapInScrollArea(m_padBoard, this);

       QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                              + "/xfb.conf", QSettings::IniFormat);
       if (settings.value("ShowPadsTab", true).toBool()) {
           const int djIndex = ui->tabWidget_2->indexOf(ui->tab_dj);
           ui->tabWidget_2->insertTab(djIndex + 1, m_padBoardPage,
                                      QIcon(":/icons/flat/Natural User Interface 2-48.png"),
                                      tr("Pads"));
       }
   }

   // Audio FX tab, right after the DJ/Pads group (hide it via Options → ShowFxTab)
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
           ui->tabWidget_2->insertTab(djGroupInsertIndex(), m_fxTabPage,
                                      QIcon(":/icons/flat/eq-fx-64.png"), tr("Audio FX"));
       }

       // The same settings are editable from the menu dialog too — refresh
       // the tab's controls whenever it becomes visible.
       connect(ui->tabWidget_2, &QTabWidget::currentChanged, this, [this](int idx) {
           if (m_fxTabPage && m_fxTabWidget && ui->tabWidget_2->widget(idx) == m_fxTabPage)
               m_fxTabWidget->reloadFromSettings();
       });
   }

   // Every pane that was drawn at a fixed size gets a scroll bar rather than
   // hiding its lower half on a laptop screen. Done last, once every page the
   // side toolbox is going to hold has been added to it.
   makeSidePanelScrollable();
   makeTabScrollable(ui->tabTorrents);


   // Who is at the desk, and what they may do with it. Last, because it
   // disables menu entries and buttons that everything above has just
   // finished creating.
   setupAccessControl();

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
        
        // Register SystemStatusAnnouncer as a singleton service using template method
        serviceContainer->registerSingleton<SystemStatusAnnouncer>();

        // Backs the Accessibility Preferences dialog (Options menu).
        serviceContainer->registerSingleton<AccessibilitySettingsService>();

        // Braille display support (via BrlTTY) is advertised in the package
        // description; register it so the service actually runs.
        serviceContainer->registerSingleton<BrailleDisplayService>();

        qDebug() << "Accessibility services registered successfully";

        // Registering only constructs a service on first resolve — it does NOT
        // run doInitialize(). Without this block the whole accessibility stack
        // (widget enhancer, AT-SPI bridge, audio feedback, live regions) stayed
        // dormant while still reporting success. Initialize explicitly, with
        // AccessibilityManager last because it consumes all the others.
        struct { const char *name; IService *svc; } a11yServices[] = {
            {"AudioFeedbackService",    serviceContainer->resolve<AudioFeedbackService>()},
            {"LiveRegionManager",       serviceContainer->resolve<LiveRegionManager>()},
            {"SystemStatusAnnouncer",   serviceContainer->resolve<SystemStatusAnnouncer>()},
            {"AccessibilitySettingsService", serviceContainer->resolve<AccessibilitySettingsService>()},
            {"BrailleDisplayService",   serviceContainer->resolve<BrailleDisplayService>()},
            {"AccessibilityManager",    serviceContainer->resolve<AccessibilityManager>()},
        };
        for (const auto &entry : a11yServices) {
            if (!entry.svc) {
                qWarning() << "Accessibility service could not be resolved:" << entry.name;
                continue;
            }
            if (entry.svc->state() != IService::ServiceState::Uninitialized)
                continue; // already running
            if (!entry.svc->initialize())
                qWarning() << "Accessibility service failed to initialize:" << entry.name;
        }
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
    // Persist the panel layout FIRST — the macOS branch below never returns.
    {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                               + "/xfb.conf", QSettings::IniFormat);
        settings.setValue("MainWindowState", saveState(kLayoutStateVersion));
        settings.setValue("MainWindowGeometry", saveGeometry());
        settings.sync();
    }

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

// Generic helper to get MediaInfo (ASYNC)
void player::getMediaInfoForFile(const QString& filePath) {
    QString mediaInfoPath = DependencyChecker::resolveExecutable("mediainfo");
    if (mediaInfoPath.isEmpty()) {
         DependencyChecker depChecker;
         if (!depChecker.ensureDependency("mediainfo",
                 tr("Reading a track's metadata uses the MediaInfo tool."), this)) {
             return;
         }
         mediaInfoPath = DependencyChecker::resolveExecutable("mediainfo");
         if (mediaInfoPath.isEmpty())
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


// The DJ tab heads a small group of performance tabs (DJ, Pads, Audio FX).
// Any of them can be hidden from the Options dialog, so the insertion point
// of a tab has to be worked out from the ones actually present rather than
// from a fixed index.
int player::djGroupInsertIndex() const
{
    if (!ui || !ui->tabWidget_2)
        return 0;
    if (m_padBoardPage) {
        const int padsIndex = ui->tabWidget_2->indexOf(m_padBoardPage);
        if (padsIndex != -1)
            return padsIndex + 1;
    }
    return ui->tabWidget_2->indexOf(ui->tab_dj) + 1;
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
    Disable_Volume = settings.value("Disable_Volume", false).toBool();
    // Re-apply the configured theme (palette + stylesheet) so options-dialog
    // changes take effect live; darkMode reflects the resolved theme.
    ThemeManager::apply(qobject_cast<QApplication *>(QApplication::instance()));
    darkMode = ThemeManager::currentIsDark();
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

    // Auto Auto-mix: overlaps computed automatically for new playlist items
    m_autoAutoMix = settings.value("AutoAutoMix", false).toBool();

    // EBU R128 loudness normalisation. Live-applied: the Options dialog's
    // finished() signal is wired to updateConfig(), so a change to the
    // target or the ceiling reaches the track already on air.
    m_loudnessEnabled = settings.value("LoudnessNormalize", false).toBool();
    m_loudnessTargetLufs = qBound(LoudnessScanner::kMinTargetLufs,
                                  settings.value("LoudnessTargetLufs",
                                                 LoudnessScanner::kDefaultTargetLufs).toDouble(),
                                  LoudnessScanner::kMaxTargetLufs);
    m_loudnessCeilingDbTp = qBound(-9.0,
                                   settings.value("LoudnessCeilingDbTp",
                                                  LoudnessScanner::kDefaultCeilingDbTp).toDouble(),
                                   0.0);
    applyLoudnessSettings();

    // --- Cue bus and output routing (Options -> "Cue and outputs") ---
    // Stored as QAudioDevice::id() values; empty means "system default".
    // Live-applied like everything else here, so changing the cue device
    // while a show is running takes effect on the next cue.
    m_mainOutputDeviceId = settings.value("Cue/MainOutputDevice").toByteArray();
    m_cueOutputDeviceId = settings.value("Cue/CueOutputDevice").toByteArray();
    m_cueSpeakAnnouncements = settings.value("Cue/SpeakAnnouncements", false).toBool();
    m_cueCountdown = settings.value("Cue/SpokenCountdown", false).toBool();
    m_cueVolume = qBound(0, settings.value("Cue/Volume", 80).toInt(), 100);
    applyOutputDeviceSettings();

    // Auto mode tempo matching: how far the next track's BPM may sit from
    // the previous one's before it stops being a smooth crossfade.
    m_bpmMatch = settings.value("AutoModeMatchBpm", false).toBool();
    m_bpmTolerance = qBound(1, settings.value("AutoModeBpmTolerance", 8).toInt(), 60);
    m_autoModeNoRepeat = qBound(0, settings.value("AutoModeNoRepeatCount", 10).toInt(), 500);

    // Stereo LED output meter: visibility and docking position. Horizontal
    // lives in the volume-slider strip; vertical docks between the main
    // tabs and the side panel. NEVER touch ui->gridLayout_2 here — the
    // startup revamp above deleted it, so that pointer dangles (this was a
    // startup segfault).
    if (m_levelMeter) {
        const bool showMeter = settings.value("ShowLevelMeter", false).toBool();
        const bool wantVertical = m_middleSplitter
            && settings.value("LevelMeterPlacement", "volume").toString() == "side";

        if (wantVertical != m_levelMeterVertical) {
            m_levelMeter->setParent(nullptr); // detach from the current host
            m_levelMeter->setMeterOrientation(wantVertical ? Qt::Vertical
                                                           : Qt::Horizontal);
            if (wantVertical)
                m_middleSplitter->insertWidget(1, m_levelMeter);
            else
                ui->horizontalLayout_7->insertWidget(2, m_levelMeter);
            m_levelMeterVertical = wantVertical;
        }

        if (!showMeter)
            m_levelMeter->clear();
        m_levelMeter->setVisible(showMeter);
    }

    // Example: Update UI elements based on loaded settings
    if (disableSeekBar) {
        ui->sliderProgress->setEnabled(false);
        qDebug() << "Disable Seek bar setting: true";
    } else {
        ui->sliderProgress->setEnabled(true);
        qDebug() << "Disable Seek bar setting: false";
    }
    // The wave strip's playhead is the seek control while wave view is on,
    // so the same switch has to reach it — otherwise turning the seek bar
    // off would leave the track scrubbable from the strip anyway.
    if (m_nowPlayingWave)
        m_nowPlayingWave->setSeekEnabled(!disableSeekBar);

    if (Disable_Volume) {
        ui->sliderVolume->setEnabled(false);
        qDebug() << "Disable Volume setting: true";
    } else {
        ui->sliderVolume->setEnabled(true);
        qDebug() << "Disable Volume setting: false";
    }

    // Log other settings
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

    // Show or hide the Torrents tab based on the EnableTorrents setting and on
    // whether this operator's role supports .torrent files at all.
    applyTorrentTabVisibility();
    qDebug() << "EnableTorrents setting:" << enableTorrents;

    // Show or hide the Pads tab (next to the DJ tab). Hiding it does not
    // discard anything: the pads stay in xfb.conf and come back with the tab.
    bool showPadsTab = settings.value("ShowPadsTab", true).toBool();
    if (ui && ui->tabWidget_2 && m_padBoardPage) {
        int padsTabIndex = ui->tabWidget_2->indexOf(m_padBoardPage);
        if (showPadsTab && padsTabIndex == -1) {
            const int djIndex = ui->tabWidget_2->indexOf(ui->tab_dj);
            ui->tabWidget_2->insertTab(djIndex + 1, m_padBoardPage,
                                       QIcon(":/icons/flat/Natural User Interface 2-48.png"),
                                       tr("Pads"));
        } else if (!showPadsTab && padsTabIndex != -1) {
            // Nothing may keep playing out of a tab the operator just hid.
            m_padBoard->stopAll();
            ui->tabWidget_2->removeTab(padsTabIndex);
            // removeTab() leaves the page parentless — hand it back to the
            // window so it is owned (and destroyed) with it.
            m_padBoardPage->setParent(this);
            m_padBoardPage->hide();
        }
        qDebug() << "ShowPadsTab setting:" << showPadsTab;
    }

    // Show or hide the Audio FX tab (next to the DJ and Pads tabs)
    bool showFxTab = settings.value("ShowFxTab", true).toBool();
    if (ui && ui->tabWidget_2 && m_fxTabPage) {
        int fxTabIndex = ui->tabWidget_2->indexOf(m_fxTabPage);
        if (showFxTab && fxTabIndex == -1) {
            ui->tabWidget_2->insertTab(djGroupInsertIndex(), m_fxTabPage,
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
    // Covers the case where the state did not change but the preference did.
    applyProgressBarVisibility();
    relayoutPlayerFrame();

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

void player::seedDefaultGenres()
{
    // One-shot top-up: brings installs whose database predates the
    // comprehensive default genre list up to date (fresh installs already
    // get it from the bundled adb.db). Runs once — genres the user deletes
    // afterwards are never re-added.
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/xfb.conf", QSettings::IniFormat);
    if (settings.value("DefaultGenresSeeded", false).toBool())
        return;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen())
        return; // flag stays unset: retried on the next launch

    static const char *const kDefaults[] = {
        "50s", "60s", "70s", "80s", "90s", "2000s", "2010s",
        "Acoustic", "Alternative", "Ambient", "Blues", "Bossa Nova",
        "Chillout", "Classical", "Country", "Dance", "Deep House", "Disco",
        "Downtempo", "Drum & Bass", "Dub", "Dubstep", "EDM", "Electronic",
        "Fado", "Folk", "Funk", "Garage", "Gospel", "Grunge", "Hip-Hop",
        "House", "Indie", "Instrumental", "Jazz", "Kizomba", "Latin",
        "Lo-Fi", "Lounge", "Metal", "New Age", "Oldies", "Opera", "Pop",
        "Psytrance", "Punk", "Rap", "Reggaeton", "RnB", "Rock",
        "Rockabilly", "Salsa", "Samba", "Ska", "Soul", "Soundtrack",
        "Swing", "Synthwave", "Techno", "Trance", "Trap", "Trip-Hop",
        "World",
    };

    QSqlQuery countQry(db);
    countQry.exec("select count(*) from genres1");
    const int before = countQry.next() ? countQry.value(0).toInt() : 0;

    QSqlQuery qry(db);
    for (const char *const g : kDefaults) {
        qry.prepare("insert into genres1 (name) select :n where not exists "
                    "(select 1 from genres1 where name = :n collate nocase)");
        qry.bindValue(":n", QString::fromUtf8(g));
        if (!qry.exec())
            qWarning() << "Genre top-up failed for" << g << ":" << qry.lastError().text();
    }

    countQry.exec("select count(*) from genres1");
    const int after = countQry.next() ? countQry.value(0).toInt() : before;
    settings.setValue("DefaultGenresSeeded", true);
    qInfo() << "Default genre top-up complete," << (after - before) << "genre(s) added";
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

    // The bundled adb.db is ONLY a first-run skeleton. It must never
    // replace an existing user database: the old "resource is newer" rule
    // here silently wiped a real music library the moment a rebuild
    // touched the bundled file (2026-07-17). Copy solely when the user
    // has no database yet.
    const bool copyRequired = !persistentFile.exists();

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

    // Daily safety net: keep dated copies of the user database (last 7
    // days). The library db is small, so this is cheap — and it makes any
    // future regression in this area recoverable instead of fatal.
    if (!copyRequired && persistentFile.exists()) {
        const QString backupDirPath = specificAppDataPath + "/backups";
        QDir().mkpath(backupDirPath);
        const QString backupPath = backupDirPath + "/adb-"
            + QDate::currentDate().toString("yyyyMMdd") + ".db";
        if (!QFile::exists(backupPath)) {
            if (QFile::copy(persistentDbPath, backupPath))
                qInfo() << "Database backup created:" << backupPath;
            QDir backupDir(backupDirPath, "adb-*.db", QDir::Name, QDir::Files);
            QStringList backups = backupDir.entryList();
            while (backups.size() > 7)
                QFile::remove(backupDirPath + "/" + backups.takeFirst());
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

    // SQLite's default rollback journal lets one unfinished read block every
    // write in the process. QSqlQueryModel hands out rows 256 at a time, so
    // the library view over a table bigger than that keeps a read transaction
    // open for as long as the window is — and once the library passed 256
    // tracks, every download failed at the end with "database is locked",
    // after the audio had already been fetched. WAL lets readers and writers
    // work at the same time, which is the shape of this app: one long-lived
    // reader in the UI, short writes from the download and sync workers.
    {
        QSqlQuery journalMode(adb);
        if (!journalMode.exec("PRAGMA journal_mode=WAL")) {
            qWarning() << "Could not switch the database to WAL:"
                       << journalMode.lastError().text();
        } else if (journalMode.next()) {
            const QString mode = journalMode.value(0).toString();
            qInfo() << "Database journal mode:" << mode;
            if (mode.compare("wal", Qt::CaseInsensitive) != 0) {
                qWarning() << "The database did not accept WAL and stays in" << mode
                           << "- writes may fail while a large list is open.";
            }
        }

        // Readers no longer block writers, but two writers still queue: wait
        // for the other one rather than failing outright.
        QSqlQuery busy(adb);
        if (!busy.exec("PRAGMA busy_timeout=15000"))
            qWarning() << "Could not set the busy timeout:" << busy.lastError().text();

        // Fold anything left in a -wal file (a previous run that ended badly)
        // back into the database itself, so the daily backup — a plain copy of
        // the .db, taken before this connection exists — is complete on its
        // own. PASSIVE because checkDbOpen() also runs with the library view
        // open, and a TRUNCATE checkpoint would wait for that reader instead,
        // freezing the window. Housekeeping only: while a list is being read
        // this reports the database as locked and does nothing, and SQLite
        // checkpoints on its own as the log grows and when the app closes.
        QSqlQuery checkpoint(adb);
        if (!checkpoint.exec("PRAGMA wal_checkpoint(PASSIVE)"))
            qDebug() << "Write-ahead log left for later:" << checkpoint.lastError().text();
    }

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

    // The as-run log. Created here rather than in a migration so an
    // install that predates it gets the table on its next launch.
    AirLog::ensureSchema(adb);

    // Which tracks count towards the national music quota, so the as-run log
    // can be measured against it. Same reasoning as the as-run log above:
    // created here rather than in a migration so an install that predates the
    // feature gets the table on its next launch. A track with no row is not
    // counted against the station — it is counted as unmarked, which is what
    // lets a library be classified a few albums at a time.
    MusicQuota::ensureSchema(adb);

    // Per-track rotation rules (category, daypart, date window, weight).
    // Same reasoning as the as-run log above: created here rather than in a
    // migration so an install that predates the feature gets the table on its
    // next launch. A track with no row here simply runs on the station's
    // defaults, so an existing library needs no conversion at all.
    RotationRules::ensureSchema(adb);

    // The hour clock: named clocks, their slots, and the weekday/hour they are
    // assigned to. Same reasoning again — created here rather than in a
    // migration so an install that predates the feature gets the tables on its
    // next launch. Three empty tables are all a station that never opens the
    // feature ever has: nothing reads them until HourClock/Enabled is set.
    HourClock::ensureSchema(adb);
    // The pips and the hour ident. One empty table until the operator adds a
    // signal; nothing reads it until TimeSignal/Enabled is set.
    TimeSignal::ensureSchema(adb);

    // musics.id is a plain INTEGER, not a primary key, and every importer in
    // XFB inserts with an explicit NULL for it — so tracks added through the
    // application have no id at all, while the rows that came with the shipped
    // database do. Anything that identifies a track by id (the rotation
    // editor's per-track rules, for one) simply cannot see the imported half
    // of a library. SQLite cannot promote a column to a primary key in place,
    // so instead: give the id-less rows their rowid, and keep new inserts
    // filled by a trigger. The rowid is only taken when nothing else already
    // claims that number, and the fallback sits above every id in use, so no
    // two tracks can end up sharing one.
    {
        QSqlQuery idFix(adb);
        idFix.exec(QStringLiteral(
            "UPDATE musics SET id = rowid WHERE id IS NULL"
            " AND rowid NOT IN (SELECT id FROM musics WHERE id IS NOT NULL)"));
        const int claimed = idFix.numRowsAffected();
        idFix.exec(QStringLiteral(
            "UPDATE musics SET id = (SELECT COALESCE(MAX(id), 0) FROM musics) + rowid"
            " WHERE id IS NULL"));
        const int shifted = idFix.numRowsAffected();
        if (claimed > 0 || shifted > 0)
            qInfo() << "musics: gave an id to" << (claimed + shifted)
                    << "track(s) that had none";
        if (!idFix.exec(QStringLiteral(
                "CREATE TRIGGER IF NOT EXISTS musics_fill_id"
                " AFTER INSERT ON musics WHEN NEW.id IS NULL BEGIN"
                "  UPDATE musics SET id = CASE"
                "    WHEN NOT EXISTS (SELECT 1 FROM musics WHERE id = NEW.rowid)"
                "      THEN NEW.rowid"
                "    ELSE (SELECT COALESCE(MAX(id), 0) + 1 FROM musics) END"
                "  WHERE rowid = NEW.rowid; END"))) {
            qWarning() << "musics: could not install the id trigger:"
                       << idFix.lastError().text();
        }
    }

    // Tempo column, added to libraries created before BPM existed. NULL
    // means "never analysed", 0 means "analysed, no steady tempo" — see
    // BpmLibrary. SQLite has no ADD COLUMN IF NOT EXISTS, so the presence
    // of the column is what decides whether to add it.
    if (!adb.record("musics").contains("bpm")) {
        QSqlQuery addBpmColumn(adb);
        if (!addBpmColumn.exec("ALTER TABLE musics ADD COLUMN bpm REAL"))
            qWarning() << "Failed to add the bpm column:" << addBpmColumn.lastError().text();
        else
            qInfo() << "Added the bpm column to the musics table";
    }

    // EBU R128 loudness, measured by LoudnessScanner and used by playback to
    // put every track on air at the same programme loudness. Same shape as
    // the bpm column above, and for the same reason — SQLite has no
    // ADD COLUMN IF NOT EXISTS, so the presence of the column is the test,
    // which makes this idempotent on every start for existing installs.
    //
    //   lufs           integrated loudness, LUFS (NULL = never measured)
    //   true_peak      inter-sample peak, dBTP  (NULL = never measured)
    //   loudness_mtime the file's modification time when it was measured,
    //                  so a re-scan can skip rows whose file has not
    //                  changed and re-do the ones that have.
    //
    // All four media tables carry a path, so all four get the columns: a
    // jingle or a commercial arriving 8 dB hotter than the music is exactly
    // the problem this feature exists to solve, and three extra REAL columns
    // on small tables cost nothing.
    for (const char *const table : {"musics", "jingles", "pub", "programs"}) {
        const QSqlRecord rec = adb.record(QString::fromLatin1(table));
        if (rec.isEmpty())
            continue; // table absent in this database: nothing to migrate
        struct { const char *name; const char *type; } columns[] = {
            {"lufs", "REAL"}, {"true_peak", "REAL"}, {"loudness_mtime", "INTEGER"}
        };
        for (const auto &col : columns) {
            if (rec.contains(QString::fromLatin1(col.name)))
                continue;
            QSqlQuery addColumn(adb);
            const QString sql = QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                                    .arg(QString::fromLatin1(table),
                                         QString::fromLatin1(col.name),
                                         QString::fromLatin1(col.type));
            if (!addColumn.exec(sql)) {
                qWarning() << "Failed to add the" << col.name << "column to" << table
                           << ":" << addColumn.lastError().text();
            } else {
                qInfo() << "Added the" << col.name << "column to the" << table << "table";
            }
        }
    }

    // Intro (ramp) and outro times, measured by IntroDetector off the
    // waveform and used by the now-playing strip and the announcements.
    // Same idempotent shape as the two blocks above.
    //
    //   intro_ms      position of the vocal entry, ms from the start of the
    //                 file (NULL = never measured)
    //   outro_ms      LENGTH of the run-out at the end, ms (NULL = ditto)
    //   intro_locked  1 when the operator dragged the marker themselves, in
    //                 which case no sweep may overwrite intro_ms again
    //
    // musics only, unlike the loudness columns: an intro is a number you
    // talk over, and nobody talks over a jingle or a commercial.
    {
        const QSqlRecord rec = adb.record(QStringLiteral("musics"));
        struct { const char *name; const char *type; } introColumns[] = {
            {"intro_ms", "INTEGER"}, {"outro_ms", "INTEGER"},
            {"intro_locked", "INTEGER DEFAULT 0"}
        };
        for (const auto &col : introColumns) {
            if (rec.isEmpty() || rec.contains(QString::fromLatin1(col.name)))
                continue;
            QSqlQuery addColumn(adb);
            const QString sql = QString("ALTER TABLE musics ADD COLUMN %1 %2")
                                    .arg(QString::fromLatin1(col.name),
                                         QString::fromLatin1(col.type));
            if (!addColumn.exec(sql)) {
                qWarning() << "Failed to add the" << col.name
                           << "column to musics:" << addColumn.lastError().text();
            } else {
                qInfo() << "Added the" << col.name << "column to the musics table";
            }
        }
    }

    // Where a downloaded track came from.
    //
    //   source_url    the link the operator pasted (NULL for anything added
    //                 from disk, which is most of an older library)
    //
    // Kept because a file is not the only thing a download produces: the cover
    // art can be fetched again from here when the file turned out not to carry
    // one, and there is no other way back to it once the download window has
    // been closed. Same idempotent shape as the blocks above.
    {
        const QSqlRecord rec = adb.record(QStringLiteral("musics"));
        if (!rec.isEmpty() && !rec.contains(QStringLiteral("source_url"))) {
            QSqlQuery addColumn(adb);
            if (!addColumn.exec(QStringLiteral("ALTER TABLE musics ADD COLUMN source_url TEXT"))) {
                qWarning() << "Failed to add the source_url column to musics:"
                           << addColumn.lastError().text();
            } else {
                qInfo() << "Added the source_url column to the musics table";
            }
        }
    }

    // Listener requests, left by whoever can reach the public now-playing
    // page. Created here for the same reason as the tables above: an install
    // that predates the feature gets it on its next launch, and nothing a
    // listener does can ever be the thing that creates it.
    //
    // Nothing in this table is ever acted on by itself. It is read by the
    // request tray and by nothing else; putting a request on air is something
    // the operator does with the mouse.
    RequestLine::ensureSchema(adb);
    RequestLine::purgeHandled();

    return true;
}

void player::on_actionOpen_triggered()
{

    qDebug() << "File -> Open file";

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    // Same set the library importer accepts (add_full_dir), so anything that
    // can be added to the database can also be dropped straight into the
    // running order — Opus in particular is what the downloader produces.
    // One shared list now, so the two can no longer drift apart.
    dialog.setNameFilters(AudioFormats::fileDialogFilters());
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if(dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        ui->playlist->addItems(fileNames);
        calculate_playlist_total_time();

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

void player::playlistContextMenu(const QPoint& pos){
    QPoint globalPos = ui->playlist->mapToGlobal(pos);
    QMenu thisMenu;
    QString remove = tr("Remove this track from the playlist");
    QString moveToTop = tr("Send this track to the top of the playlist");
    QString moveToBottom = tr("Send this track to the bottom of the playlist");
    QString addVolumeLine = tr("Add a volume line");
    QString resetVolumeLine = tr("Reset the volume line");
    QString removeVolumeLine = tr("Remove the volume line");
    QString autoMixThis = tr("Auto-mix the transition into this track");
    QString cueThis = tr("Cue this track in the headphones");


    QAction *entryCue = thisMenu.addAction(cueThis);
    thisMenu.addSeparator();
    QAction *entryRemove = thisMenu.addAction(remove);
    QAction *entryToTop = thisMenu.addAction(moveToTop);
    QAction *entryToBottom = thisMenu.addAction(moveToBottom);

    {
        const AccessControl &access = AccessControl::instance();
        const bool mayEditOrder = access.allows(QStringLiteral("playlist.edit"));
        entryCue->setEnabled(access.allows(QStringLiteral("playback.cue")));
        entryRemove->setEnabled(mayEditOrder);
        entryToTop->setEnabled(mayEditOrder);
        entryToBottom->setEnabled(mayEditOrder);
    }

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

    thisMenu.addSeparator();
    QAction *autoMixAction = thisMenu.addAction(autoMixThis);
    autoMixAction->setToolTip(tr("Compute this track's crossfade overlap from the sound "
                                 "waves: it will start where the previous track goes quiet."));

    QString voiceTrackHere = tr("Voice track over the join above this track...");
    QAction *voiceAction = thisMenu.addAction(voiceTrackHere);
    voiceAction->setToolTip(tr("Record your link across this join. XFB writes the "
                               "ducking onto both songs as an ordinary volume line "
                               "you can then edit by hand."));
    voiceAction->setEnabled(menuRow >= 1);

    {
        const AccessControl &access = AccessControl::instance();
        // The volume line and the auto-mix both change how the running order
        // sounds, so they go with editing it.
        const bool mayEditOrder = access.allows(QStringLiteral("playlist.edit"));
        volAction->setEnabled(mayEditOrder);
        autoMixAction->setEnabled(access.allows(QStringLiteral("playback.automix")));
        if (voiceAction->isEnabled())
            voiceAction->setEnabled(access.allows(QStringLiteral("playback.voicetrack")));
    }

    QAction* selectedItem = thisMenu.exec(globalPos);
    if(selectedItem){
        QString selectedListItem = selectedItem->text();
        int rowidx = ui->playlist->selectionModel()->currentIndex().row();
        estevalor = ui->playlist->model()->data(ui->playlist->model()->index(rowidx,0)).toString();

        if(selectedListItem==cueThis){
            // estevalor holds the clicked row's text, which for the playlist
            // is the file path itself.
            cueFile(estevalor, QFileInfo(estevalor).fileName());
        }
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
        if(selectedListItem==autoMixThis){
            // The result is inspected in the wave view, so switch it on
            if (m_waveViewToggle && !m_waveViewToggle->isChecked())
                m_waveViewToggle->setChecked(true);
            startAutoMix({rowidx});
        }
        if(selectedListItem==voiceTrackHere){
            openVoiceTrackDialog(rowidx);
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
    // Pre-fade listen: only ever offered for a single track, because there is
    // one pair of headphones and auditioning twelve tracks at once is not a
    // thing. Plays on the cue device alone (Options -> Cue and outputs).
    QAction *actCue = nullptr;
    if (!multiSelect) {
        actCue = thisMenu.addAction(QIcon(":/icons/ic_launcher_voicedial.png"),
                                    tr("Cue this track in the headphones"));
        // The key is named in the label rather than set as a shortcut on this
        // action: the Playback menu already owns Ctrl+Shift+C application-wide,
        // and a second action carrying the same sequence makes Qt call it
        // ambiguous while this menu is open — at which point NEITHER fires.
        actCue->setText(tr("Cue this track in the headphones\t%1")
                            .arg(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C)
                                     .toString(QKeySequence::NativeText)));
        actCue->setToolTip(tr("Listen to this track on the cue output only. "
                              "It never reaches the on-air output."));
        thisMenu.setToolTipsVisible(true);
    }
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

    // The quota marking, on the selection. Not in the batch submenu: it is
    // offered for one track as readily as for forty, and it is the entry an
    // operator reaches for while listening to something they have just
    // realised nobody has ever classified.
    QAction *actQuota = thisMenu.addAction(
        QIcon(":/icons/flat/Music Transcript-48.png"),
        multiSelect ? tr("Mark %1 tracks for the music quota...").arg(count)
                    : tr("Mark this track for the music quota..."));

    QAction *actRetune432 = thisMenu.addAction(
        QIcon(":/icons/flat/tuning-fork-64.png"),
        multiSelect ? tr("Retune %1 tracks to 432 Hz...").arg(count)
                    : tr("Retune this track to 432 Hz..."));
    thisMenu.addSeparator();

    // Sync to phone. The server cannot push to a phone, so these mark tracks
    // and the phone collects them as a playlist called "Marked for this phone".
    QAction *actSyncSelection = thisMenu.addAction(
        QIcon(":/icons/flat/Upload to Cloud-48.png"),
        multiSelect ? tr("Sync %1 tracks to the phone").arg(count)
                    : tr("Sync this track to the phone"));
    QAction *actSyncAll = thisMenu.addAction(
        QIcon(":/icons/flat/Connection Sync-48.png"),
        tr("Sync the whole music list to the phone"));
    QAction *actSyncClear = nullptr;
    if (m_mobileSyncServer && !m_mobileSyncServer->syncSet().isEmpty()) {
        actSyncClear = thisMenu.addAction(
            QIcon(":/icons/sync_off.png"),
            tr("Clear what is marked for the phone (%1)")
                .arg(m_mobileSyncServer->syncSet().size()));
    }
    thisMenu.addSeparator();

    QAction *actDelete = thisMenu.addAction(
        multiSelect ? tr("Delete %1 tracks from database").arg(count)
                    : tr("Delete this track from database"));
    thisMenu.addSeparator();
    QAction *actAudacity = thisMenu.addAction(tr("Open this in Audacity"));
    QAction *actInfo = thisMenu.addAction(tr("Retrieve metadata from file (mediainfo)"));

    // What this session may not do is greyed out rather than missing, so an
    // operator sees the entry is there and learns from the tooltip why it will
    // not open. Set here rather than through AccessControl::guard() because
    // this menu is built afresh on every right-click and thrown away after.
    {
        const AccessControl &access = AccessControl::instance();
        const bool mayQueue   = access.allows(QStringLiteral("playlist.edit"));
        const bool mayEdit    = access.allows(QStringLiteral("library.edit"));
        const bool mayDelete  = access.allows(QStringLiteral("library.delete"));
        actAddBottom->setEnabled(mayQueue);
        actAddTop->setEnabled(mayQueue);
        if (actCue)
            actCue->setEnabled(access.allows(QStringLiteral("playback.cue")));
        for (QAction *batch : {actSetGenre1, actSetGenre2, actSetArtist, actSetCountry}) {
            if (batch)
                batch->setEnabled(mayEdit);
        }
        actQuota->setEnabled(access.allows(QStringLiteral("programming.quota")));
        actRetune432->setEnabled(access.allows(QStringLiteral("library.retune")));
        const bool mayPair = access.allows(QStringLiteral("station.sync.mobile"));
        actSyncSelection->setEnabled(mayPair);
        actSyncAll->setEnabled(mayPair);
        if (actSyncClear)
            actSyncClear->setEnabled(mayPair);
        actDelete->setEnabled(mayDelete);
        actAudacity->setEnabled(access.allows(QStringLiteral("library.audacity")));
    }

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

    if (actCue && selectedItem == actCue) {
        const QStringList paths = getSelectedPaths();
        if (!paths.isEmpty())
            cueFile(paths.first(), QFileInfo(paths.first()).fileName());

    } else if (selectedItem == actAddBottom) {
        for (const QString &path : getSelectedPaths())
            ui->playlist->addItem(path);
        calculate_playlist_total_time();

    } else if (selectedItem == actQuota) {
        QList<qint64> ids;
        for (int id : getSelectedIds()) {
            if (id > 0 && !ids.contains(id))
                ids.append(id);
        }
        openMusicQuota(ids);

    } else if (selectedItem == actRetune432) {
        convertMusicsTo432(getSelectedPaths());

    } else if (selectedItem == actSyncSelection) {
        markForPhone(getSelectedPaths());

    } else if (selectedItem == actSyncAll) {
        markForPhone(allListedMusicPaths(), true);

    } else if (actSyncClear && selectedItem == actSyncClear) {
        mobileSyncServer()->clearSyncSet();
        ui->statusBar->showMessage(tr("Nothing is marked for the phone now."), 6000);
        announceAccessible(tr("Cleared what was marked for the phone"));

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
    const QString actionCue = tr("Cue this jingle in the headphones");

    // The entries are kept so what this session may not do can be greyed out
    // rather than removed; the menu is thrown away when it closes.
    QAction *entryAddBottom = thisMenu.addAction(actionAddToBottom);
    QAction *entryAddTop = thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    QAction *entryCue = thisMenu.addAction(actionCue);
    thisMenu.addSeparator();
    QAction *entryDelete = thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    QAction *entryAudacity = thisMenu.addAction(actionOpenAudacity);

    {
        const AccessControl &access = AccessControl::instance();
        const bool mayQueue = access.allows(QStringLiteral("playlist.edit"));
        entryAddBottom->setEnabled(mayQueue);
        entryAddTop->setEnabled(mayQueue);
        entryCue->setEnabled(access.allows(QStringLiteral("playback.cue")));
        entryDelete->setEnabled(access.allows(QStringLiteral("library.delete")));
        entryAudacity->setEnabled(access.allows(QStringLiteral("library.audacity")));
    }

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QModelIndexList selectedIndexes = ui->jinglesView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    int rowidx = selectedIndexes.first().row();
    QModelIndex pathIndex = ui->jinglesView->model()->index(rowidx, 1); // Assuming column 1 is path
    if (!pathIndex.isValid()) return;
    QString selectedFilePath = ui->jinglesView->model()->data(pathIndex).toString();

    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionCue) {
        cueFile(selectedFilePath, QFileInfo(selectedFilePath).fileName());
    } else if (selectedActionText == actionAddToBottom) {
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


void player::pubViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->pubView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this pub from the database");
    QString openWithAudacity = tr("Open this in Audacity");
    QString cueThisAdvert = tr("Cue this advert in the headphones");

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QAction *entryAddBottom = thisMenu.addAction(addToBottomOfPlaylist);
    QAction *entryAddTop = thisMenu.addAction(addtoTopOfPlaylist);
    thisMenu.addSeparator();
    QAction *entryCue = thisMenu.addAction(cueThisAdvert);
    thisMenu.addSeparator();
    QAction *entryDelete = thisMenu.addAction(deleteThisFromDB);
    QAction *entryAudacity = thisMenu.addAction(openWithAudacity);

    {
        const AccessControl &access = AccessControl::instance();
        const bool mayQueue = access.allows(QStringLiteral("playlist.edit"));
        entryAddBottom->setEnabled(mayQueue);
        entryAddTop->setEnabled(mayQueue);
        entryCue->setEnabled(access.allows(QStringLiteral("playback.cue")));
        entryDelete->setEnabled(access.allows(QStringLiteral("library.delete")));
        entryAudacity->setEnabled(access.allows(QStringLiteral("library.audacity")));
    }

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (selectedItem)
    {
        //qDebug()<<"selected item in context menu was: "<<selectedItem->text();
        QString selectedMenuItem = selectedItem->text();
        int rowidx = ui->pubView->selectionModel()->currentIndex().row();
        estevalor = ui->pubView->model()->data(ui->pubView->model()->index(rowidx,2)).toString();

        if(selectedMenuItem==cueThisAdvert){
            // estevalor is column 2 of the pub table: the file path.
            cueFile(estevalor, QFileInfo(estevalor).fileName());
        }
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
// Resolve one of the serverFtpCmds* helper scripts to an absolute path.
// Search order (first existing file wins):
//   1. "ServerScriptsPath" in xfb.conf (config-only override)
//   2. user/system data dirs under xfb/scripts — e.g. ~/.local/share/xfb/scripts
//      then /usr/share/xfb/scripts on Linux, ~/Library/Application Support/
//      xfb/scripts on macOS — so a station's edited copies override the
//      packaged templates without touching root-owned files
//   3. the macOS bundle's Resources/scripts
//   4. a "scripts" dir next to the binary or in any parent (dev checkouts)
QString player::serverScriptPath(const QString& scriptName) const {
    QStringList candidates;

    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!writableConfigPath.isEmpty()) {
        QSettings settings(writableConfigPath + "/xfb.conf", QSettings::IniFormat);
        const QString configured = settings.value("ServerScriptsPath").toString();
        if (!configured.isEmpty())
            candidates << QDir(configured).filePath(scriptName);
    }

    const QString located = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                   "xfb/scripts/" + scriptName);
    if (!located.isEmpty())
        candidates << located;

    const QString appDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    candidates << QDir(appDir + "/../Resources/scripts").filePath(scriptName);
#endif
    QDir walk(appDir);
    for (int i = 0; i < 6; ++i) {
        candidates << walk.filePath("scripts/" + scriptName);
        if (!walk.cdUp())
            break;
    }

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return QFileInfo(candidate).absoluteFilePath();
    }
    qWarning() << "Server script" << scriptName << "not found. Searched:" << candidates;
    return QString();
}

// The shipped serverFtpCmds* scripts are templates: the station's FTP host
// and port must replace the literal [IP] and [PORT] placeholders before the
// script can talk to a server.
bool player::serverScriptIsTemplate(const QString& scriptPath) const {
    QFile f(scriptPath);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return false;
    const QString head = QString::fromUtf8(f.read(4096));
    return head.contains(QLatin1String("[IP]")) || head.contains(QLatin1String("[PORT]"));
}

// Launch a server script through bash so the exec bit isn't required (the
// packaged copies are plain data files). The working directory matters: the
// scripts do "cd ../ftp/", so run from FTPPath (a folder the options dialog
// requires to be named "ftp" — ../ftp from inside it resolves back to it),
// falling back to the script's own directory so ../ftp is its sibling.
void player::startServerScript(QProcess* process, const QString& scriptPath) {
    QString workDir = FTPPath;
    if (workDir.isEmpty() || !QDir(workDir).exists())
        workDir = QFileInfo(scriptPath).absolutePath();
    process->setWorkingDirectory(workDir);
    process->start("/bin/bash", QStringList() << scriptPath);
}

// Same launch rules as startServerScript, but as a shell fragment for the
// legacy call sites that pipe script output through grep via "sh -c".
QString player::serverScriptShellCommand(const QString& scriptPath) const {
    QString quoted = scriptPath;
    quoted.replace(QLatin1String("'"), QLatin1String("'\\''"));
    QString workDir = FTPPath;
    if (workDir.isEmpty() || !QDir(workDir).exists())
        workDir = QFileInfo(scriptPath).absolutePath();
    QString quotedDir = workDir;
    quotedDir.replace(QLatin1String("'"), QLatin1String("'\\''"));
    return QStringLiteral("cd '%1' && bash '%2'").arg(quotedDir, quoted);
}

// Helper function to run a server script asynchronously
void player::runServerCheckScript(const QString& scriptName, const QString& fileToCheck, const QString& successMessage, const QString& failureMessage) {
    QString scriptPath = serverScriptPath(scriptName);

    qInfo() << "Attempting to execute check script:" << scriptName << "->" << scriptPath << "for file:" << fileToCheck;

    if (scriptPath.isEmpty()) {
        QMessageBox::critical(this, tr("Script Error"),
                              tr("The required check script '%1' was not found.\nInstall it under the application data directory (e.g. share/xfb/scripts) or set ServerScriptsPath in xfb.conf.").arg(scriptName));
        return;
    }
    if (serverScriptIsTemplate(scriptPath)) {
        QMessageBox::critical(this, tr("Script Not Configured"),
                              tr("The script '%1' still contains the [IP]/[PORT] placeholders.\nEdit it with your server's address and port:\n%2").arg(scriptName, scriptPath));
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

    // Run the script and check its full output in the finished handler.
    startServerScript(checkProcess, scriptPath);
}
// Helper function to run upload/put script asynchronously
void player::runServerUploadScript(const QString& scriptName, const QString& fileToUpload, const QString& successMessage, const QString& failureMessage, std::function<void(bool)> callback) {
    QString scriptPath = serverScriptPath(scriptName);

    qInfo() << "Attempting to execute upload script:" << scriptName << "->" << scriptPath << "for file:" << fileToUpload;
     qDebug() << "Dependencies: Script must exist, ~/.netrc configured.";

    if (scriptPath.isEmpty()) {
        QMessageBox::critical(this, tr("Script Error"),
                              tr("The required upload script '%1' was not found.\nInstall it under the application data directory (e.g. share/xfb/scripts) or set ServerScriptsPath in xfb.conf.").arg(scriptName));
        callback(false); // Indicate failure
        return;
    }
    if (serverScriptIsTemplate(scriptPath)) {
        QMessageBox::critical(this, tr("Script Not Configured"),
                              tr("The script '%1' still contains the [IP]/[PORT] placeholders.\nEdit it with your server's address and port:\n%2").arg(scriptName, scriptPath));
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
    startServerScript(uploadProcess, scriptPath);
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
    const QString actionCue = tr("Cue this program in the headphones");

    QAction *entryAddBottom = thisMenu.addAction(actionAddToBottom);
    QAction *entryAddTop = thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    QAction *entryCue = thisMenu.addAction(actionCue);
    thisMenu.addSeparator();
    QAction *entryDelete = thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    QAction *entryAudacity = thisMenu.addAction(actionOpenAudacity);
    thisMenu.addSeparator();
    QAction *entryCheckSent = thisMenu.addAction(actionCheckSent);
    QAction *entryResend = thisMenu.addAction(actionResendToServer);

    {
        const AccessControl &access = AccessControl::instance();
        const bool mayQueue = access.allows(QStringLiteral("playlist.edit"));
        entryAddBottom->setEnabled(mayQueue);
        entryAddTop->setEnabled(mayQueue);
        entryCue->setEnabled(access.allows(QStringLiteral("playback.cue")));
        entryDelete->setEnabled(access.allows(QStringLiteral("library.delete")));
        entryAudacity->setEnabled(access.allows(QStringLiteral("library.audacity")));
        // Both of these talk to the station's server, which is where the
        // server permissions live.
        const bool mayReachServer = access.allows(QStringLiteral("station.server.ftp"));
        entryCheckSent->setEnabled(mayReachServer);
        entryResend->setEnabled(mayReachServer);
    }

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

    if (selectedActionText == actionCue) {
        cueFile(selectedFilePath, selectedFileName);
    } else if (selectedActionText == actionAddToBottom) {
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

    QAction *entryDownload = thisMenu.addAction(actionDownload);
    QAction *entryDownloadStream = thisMenu.addAction(actionDownloadAndStream);
    thisMenu.addSeparator();
    QAction *entryMagnet = thisMenu.addAction(actionCopyMagnet);
    thisMenu.addAction(actionViewDetails);

    {
        // Viewing what a search found costs nothing; taking it does.
        const bool mayTorrent =
            AccessControl::instance().allows(QStringLiteral("downloads.torrents"));
        entryDownload->setEnabled(mayTorrent);
        entryDownloadStream->setEnabled(mayTorrent);
        entryMagnet->setEnabled(mayTorrent);
    }

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
            if (!confirmDownloadNetworkExposure()) {
                return; // no VPN and the user chose not to proceed
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
            if (!confirmDownloadNetworkExposure()) {
                return; // no VPN and the user chose not to proceed
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

    // Whatever the operator stopped, they have restarted: the watchdog is
    // allowed to care about silence again.
    if (m_deadAirWatchdog)
        m_deadAirWatchdog->noteOperatorPlay();

    if(PlayMode=="stopped"){
        // Auto Mode exists so the station never goes silent, so starting it on
        // an empty running order has to put a track up rather than refuse. Do
        // it before the transport flips to Playing_Segue: if the library has
        // nothing to offer, the button must stay in its stopped state instead
        // of going green over silence.
        if(autoMode==1 && ui->playlist->count()==0){
            qDebug()<<"Play pressed with an empty playlist and autoMode on — picking a track";
            if(!autoModeGetMoreSongs()){
                const QString why = tr("Auto Mode has no track to play — the music "
                                       "library is empty.");
                qDebug()<<"autoMode could not feed the playlist; staying stopped";
                ui->statusBar->showMessage(why, 8000);
                announceAccessible(why);
                return;
            }
        }

        if(darkMode){
            ui->btPlay->setStyleSheet(kPlayingGreenDarkStyle);
        }else{

            ui->btPlay->setStyleSheet(kPlayingGreenLightStyle);
        }
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";
        playNextSong();

    }  else if(PlayMode=="Playing_StopAtNextOne"){

        if(darkMode){
            ui->btPlay->setStyleSheet(kPlayingGreenDarkStyle);
        }else{

            ui->btPlay->setStyleSheet(kPlayingGreenLightStyle);
        }
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";

    } else if(PlayMode=="Playing_Segue"){

        ui->btPlay->setStyleSheet(kPlayingAmberStyle);
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
                const QUrl nextUrl = QUrl::fromLocalFile(itemDaPlaylist);
                XplaylistUrls.clear();
                XplaylistUrls.append(nextUrl);
                XplaylistIndex = 0;

                // Prevent playbackStateChanged from triggering playNextMedia
                // while we're changing the source
                m_manualAdvancing = true;
                onAbout2Finish = 0;  // Reset so playlistAboutToFinish can fire for the new track

                // Clear the previous source first to release any stuck
                // AVFoundation session — but not when this track was
                // preloaded: the clear would tear down the primed pipeline
                // and reintroduce the very gap the preload removes.
                if (!Xplayer->hasPreparedNext(nextUrl))
                    Xplayer->setSource(QUrl());

                // Set the media to play
                Xplayer->setSource(nextUrl);

                lastPlayedSong = itemDaPlaylist;

                // Capture the track's volume line before its item is deleted;
                // onPositionChanged applies it while this track plays
                m_activeEnvelope = PlaylistWaveView::parseEnvelope(
                    firstItem->data(PlaylistWaveView::VolumeEnvelopeRole).toString());
                m_activeEnvelopePath = itemDaPlaylist;
                // A recorded link is played out, never faded under what
                // follows it: startOverlapSegue() needs to know which this is.
                m_activeIsVoiceTrack =
                    firstItem->data(PlaylistWaveView::VoiceTrackRole).toBool();
                if (m_nowPlayingWave)
                    m_nowPlayingWave->setEnvelope(m_activeEnvelope);

                // Apply the level for position 0 right away (a track without
                // a line restores the plain slider volume) so the first
                // instants don't play at the previous track's envelope level
                if (XplayerOutput) {
                    const double base = ui->sliderVolume->value() / 100.0;
                    XplayerOutput->setVolume(float(base * m_timeSignalDuck
                        * PlaylistWaveView::envelopeGainAt(m_activeEnvelope, 0)));
                    m_envelopeApplied = !m_activeEnvelope.isEmpty();
                }

                int dotsNumInString = itemDaPlaylist.count(".");
                qDebug()<<"dotsNumInString has value: "<<dotsNumInString;

                QFileInfo fileName(itemDaPlaylist);
                QString baseName = fileName.fileName();
                ui->txtNowPlaying->setText(baseName);
                // Tell the stream what went to air. Does nothing unless the
                // built-in encoder is actually streaming.
                updateStreamNowPlaying(itemDaPlaylist);
                // Speak the new track: without this a blind operator has no
                // way to tell what went to air. The intro rides along in the
                // SAME utterance rather than following it — the announcement
                // queue delivers one message per timer tick, and a screen
                // reader given two in quick succession drops the first.
                // sourceChanged has already run refreshIntroForCurrentTrack()
                // by now; when the track has never been measured the number
                // arrives later and is spoken then, by the introMeasured
                // handler.
                QString nowPlayingSpeech = tr("Now playing: %1").arg(baseName);
                if (m_currentIntroMs > 0 && m_nowPlayingWave
                        && m_nowPlayingWave->track() == itemDaPlaylist) {
                    nowPlayingSpeech = tr("%1. Intro %2")
                        .arg(nowPlayingSpeech, spokenDuration(m_currentIntroMs));
                    m_introAnnouncedPath = itemDaPlaylist;
                }
                announceAccessible(nowPlayingSpeech);

                QDateTime now = QDateTime::currentDateTime();
                QString text = now.toString("yyyy-MM-dd || hh:mm:ss ||");
                QString historyNewLine = text + " " + baseName;
                {
                    auto *historyItem = new QListWidgetItem(historyNewLine);
                    // Tag the row with its file so it gets the cover icon
                    historyItem->setData(kArtworkPathRole, itemDaPlaylist);
                    ui->historyList->addItem(historyItem);
                    requestItemArtwork(historyItem, itemDaPlaylist);
                }
                int hlistcout = ui->historyList->count();
                qDebug()<<"historyList has "<<hlistcout<<" items";

                if(hlistcout > 99){
                    qDebug()<<"HistoryList is being cleaned because it's over 100 records now...";
                    QListWidgetItem* itemToDelete = ui->historyList->item(0);
                    if (itemToDelete) {
                        delete itemToDelete;
                    }
                }

                // As-run log: on the running order the outgoing item ends
                // the instant this one starts. An overlap segue has already
                // closed its own row (with the position it really reached),
                // so this is a no-op in that case. Closing is queued, so it
                // costs nothing here.
                closeAirLogEntry(m_airEndReason.isEmpty() ? QStringLiteral("end")
                                                          : m_airEndReason);
                m_airEndReason.clear();

                Xplayer->play();
                
                // Re-enable automatic advancement now that playback has started
                m_manualAdvancing = false;
                
                // Start the watchdog timer to detect stalled playback
                m_lastKnownPosition = -1;
                m_stallCount = 0;
                m_playbackWatchdog->start();

                // Opening the as-run row means looking the path up in four
                // tables, so it happens after play() has been issued rather
                // than in front of it.
                m_airHandle = AirLog::instance()->openPath(itemDaPlaylist, autoMode == 1);
                m_airPosition = 0;

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
    closeAirLogEntry(QStringLiteral("stopped"));

    // Silence the operator asked for is not dead air. Without this the
    // watchdog would rescue a station that was deliberately taken down.
    if (m_deadAirWatchdog)
        m_deadAirWatchdog->noteOperatorStop();

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
    m_nextPrepared = false;
    lastPlayedSong = "";  // Reset so the same song can be played again after stop
}

void player::on_sliderProgress_sliderReleased()
{
    // Seek once, where the user dropped the handle. Seeking on every
    // sliderMoved event restarted the FX decoder dozens of times per drag,
    // and the periodic position updates yanked the handle back mid-drag
    // (onPositionChanged now leaves the slider alone while it's held).
    Xplayer->setPosition(ui->sliderProgress->value());
}

void player::on_sliderVolume_sliderMoved(int position)
{
    //qDebug()<<"volume slider mooved "<<position;
    // m_timeSignalDuck is 1.0 unless a time signal is playing over the top:
    // moving the slider under one should move the ducked level, not cancel
    // the duck and come back up in the middle of the pips.
    XplayerOutput->setVolume(float(position / 100.0 * m_timeSignalDuck));

}

void player::onPositionChanged(qint64 position)
{
    // As-run log: remember where the on-air item got to, and leave a
    // breadcrumb in the database now and then. The write itself is queued —
    // this runs ten times a second while audio is going out.
    m_airPosition = position;
    if (m_airHandle > 0)
        AirLog::instance()->heartbeat(m_airHandle, position);

    // Never move the slider while the user is holding it — the playback
    // ticks would drag the handle back to the playing position mid-grab
    if (!ui->sliderProgress->isSliderDown())
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
        // The duck rides in the same sum: this runs ten times a second, so a
        // duck applied anywhere else would last one tick.
        XplayerOutput->setVolume(float(base * gain * m_timeSignalDuck));
        m_envelopeApplied = true;
    } else if (m_envelopeApplied) {
        // The line no longer applies (new track without one): restore
        m_envelopeApplied = false;
        if (XplayerOutput)
            XplayerOutput->setVolume(float(ui->sliderVolume->value() / 100.0
                                           * m_timeSignalDuck));
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

    // Spoken countdown in the private ear (Options -> Cue and outputs).
    // The wave strip shows the same numbers on screen, which is no use to an
    // operator working by ear; these are spoken into the cue headphones only,
    // so they never go to air. Marks, not a per-second count: rendering a
    // spoken phrase takes a moment, and "thirty ... twenty ... ten ... five"
    // is what a presenter actually uses to time a link.
    if (m_cueCountdown && m_cueBus
            && Xplayer && Xplayer->playbackState() == QMediaPlayer::PlayingState) {
        const qint64 remaining = trackTotalDuration - position;
        const int seconds = int(remaining / 1000);
        static const int kMarks[] = { 30, 20, 10, 5 };
        for (const int mark : kMarks) {
            if (seconds == mark && m_lastSpokenCountdown != mark) {
                m_lastSpokenCountdown = mark;
                m_cueBus->speak(tr("%n second(s)", "spoken countdown to the end of "
                                                   "the on-air track", mark));
                break;
            }
        }
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

    // Gapless: shortly before this track ends, hand the upcoming playlist
    // item to the player so the transition starts instantly (the FX engine
    // preloads its decoder and hands off without breaking the stream; plain
    // playback preloads the media on a standby player). When a crossfade
    // overlap is set, arm the preload before that segue fires instead.
    if (PlayMode == "Playing_Segue" && !m_manualAdvancing && !m_nextPrepared
            && position > 0 && trackTotalDuration > 0 && ui->playlist->count() > 0) {
        const qint64 overlapMs =
            ui->playlist->item(0)->data(PlaylistWaveView::OverlapRole).toLongLong();
        const qint64 horizon = qMax(qint64(5000), overlapMs + 2000);
        const qint64 remaining = trackTotalDuration - position;
        if (remaining > 0 && remaining <= horizon) {
            m_nextPrepared = true; // re-armed by the next durationChanged
            const QString nextPath = ui->playlist->item(0)->text();
            if (QFileInfo::exists(nextPath))
                Xplayer->prepareNext(QUrl::fromLocalFile(nextPath));

            // Warm up the tail player as well: auto-mix sets the overlap
            // asynchronously after the item is added, so the 80% preload in
            // playlistAboutToFinish usually ran too early to see it. Probing
            // the outgoing track now keeps the segue from paying that cost
            // as a hole in the crossfade.
            if (overlapMs > 0 && m_tailPlayer
                    && Xplayer->source().isLocalFile()
                    && m_tailPlayer->playbackState() != QMediaPlayer::PlayingState
                    && m_tailPlayer->source() != Xplayer->source())
                m_tailPlayer->setSource(Xplayer->source());
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
    // The library's stored time is a rounded guess; this is the real length.
    if (m_airHandle > 0)
        AirLog::instance()->setPlannedMs(m_airHandle, position);
    m_overlapSegueFired = false; // new media: re-arm the overlap segue
    m_lastSpokenCountdown = -1;  // ... and the spoken countdown marks
    m_nextPrepared = false;      // new media: re-arm the gapless preload

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

    // As-run log: the outgoing item ends here, at the position it really
    // reached, even though its tail keeps fading for another few seconds.
    closeAirLogEntry(QStringLiteral("segue"), endingPos);

    // When the FX engine drives playback and the next track is already
    // preloaded, the engine crossfades internally: the outgoing decoder
    // keeps feeding the same audio stream, faded out per sample — no cut,
    // no tail-player spin-up, sample-continuous. The tail player remains
    // the fallback for plain playback or when no preload is armed.
    QUrl nextUrl;
    bool nextIsVoiceTrack = false;
    if (ui->playlist->count() > 0) {
        nextUrl = QUrl::fromLocalFile(ui->playlist->item(0)->text());
        nextIsVoiceTrack =
            ui->playlist->item(0)->data(PlaylistWaveView::VoiceTrackRole).toBool();
    }

    // Voice tracking: a link is not a song, and neither edge of it wants the
    // linear crossfade. Going INTO a link, the song underneath must follow the
    // ducking that was generated (and possibly hand-edited) for it rather than
    // sliding to nothing; coming OUT of one, the link itself has to be heard
    // to its last word rather than faded under the incoming song. Both are the
    // same rule: drive the tail from the outgoing item's own volume line and
    // let it play out.
    //
    // m_activeIsVoiceTrack is trusted only when it still describes the track
    // that is actually ending — the same path guard onPositionChanged() uses
    // on m_activeEnvelope. A track started outside playNextSong() (the
    // library, a deck, a jingle) leaves both stale, and a stale flag here
    // would silently kill the crossfade on an ordinary join.
    const bool outgoingIsVoiceTrack =
        m_activeIsVoiceTrack && endingSource.toLocalFile() == m_activeEnvelopePath;
    const bool voiceSegue = nextIsVoiceTrack || outgoingIsVoiceTrack;

    // A volume line drawn on the outgoing track has to survive the segue. The
    // engine's internal crossfade fades the outgoing decoder per sample and
    // knows nothing about the line, so a track carrying one goes out through
    // the tail player instead, where the line and the fade can be multiplied
    // together. Sample-continuous mixing is the better join and it is still
    // what an ordinary track gets; correctness wins only where the operator
    // actually asked for something.
    const QVector<QPointF> endingEnvelope =
        (endingSource.toLocalFile() == m_activeEnvelopePath) ? m_activeEnvelope
                                                             : QVector<QPointF>();
    const bool endingHasVolumeLine = endingEnvelope.size() > 1;

    const bool engineMix = !voiceSegue && !endingHasVolumeLine
                           && Xplayer->fxEngineActive()
                           && !nextUrl.isEmpty()
                           && Xplayer->hasPreparedNext(nextUrl);

    if (engineMix) {
        Xplayer->setNextCrossfade(qBound(qint64(200), fadeMs, qint64(600000)));
    } else if (m_tailPlayer && endingSource.isLocalFile()) {
        if (m_tailFade->state() == QAbstractAnimation::Running)
            m_tailFade->stop();
        m_tailFadeFactor = 1.0;
        const float startVolume = XplayerOutput ? XplayerOutput->volume() : 1.0f;
        m_tailOutput->setVolume(startVolume);
        if (m_tailPlayer->source() != endingSource) // normally preloaded earlier
            m_tailPlayer->setSource(endingSource);
        // Seek BEFORE play: a stopped-state seek is only remembered, so the
        // decoder spawns once, already at the tail position. The previous
        // play-then-seek order spawned it twice (from 0, then again at the
        // seek), which opened an audible hole at the start of the crossfade.
        m_tailPlayer->setPosition(endingPos);
        m_tailPlayer->play();

        if (voiceSegue) {
            m_tailEnvelope = endingEnvelope;
            m_tailEnvelopeActive = true;
            m_tailBaseVolume = ui->sliderVolume
                ? float(ui->sliderVolume->value() / 100.0) : startVolume;
            applyTailVolume(endingPos);
            qDebug() << "Voice track segue: the tail follows its own volume line"
                     << (m_tailEnvelope.isEmpty() ? "(flat)" : "")
                     << "instead of fading out over" << fadeMs << "ms";
        } else {
            // An ordinary segue still fades — but if the outgoing track has a
            // volume line, the fade rides on top of it rather than replacing
            // it, so the shape the operator drew is what leaves the station.
            m_tailEnvelope = endingEnvelope;
            m_tailEnvelopeActive = endingHasVolumeLine;
            m_tailBaseVolume = startVolume;
            m_tailFade->setStartValue(1.0);
            m_tailFade->setEndValue(0.0);
            // Absolute sanity bound, not the UI window: saved playlists may
            // carry overlaps larger than the current "Max overlap" setting
            m_tailFade->setDuration(int(qBound(qint64(200), fadeMs, qint64(600000))));
            m_tailFade->start();
        }
    }

    // From the state machine's point of view this is just a manual advance;
    // playNextSong() guards the source switch with m_manualAdvancing itself.
    playNextSong();
}

void player::closeAirLogEntry(const QString &reason, qint64 playedMs)
{
    if (m_airHandle <= 0)
        return;
    AirLog::instance()->close(m_airHandle,
                              playedMs < 0 ? m_airPosition : playedMs, reason);
    m_airHandle = 0;
    m_airPosition = 0;
}

void player::stopTailPlayer()
{
    if (m_tailFade && m_tailFade->state() == QAbstractAnimation::Running)
        m_tailFade->stop();
    m_tailEnvelopeActive = false;
    m_tailEnvelope.clear();
    m_tailFadeFactor = 1.0;
    if (m_tailPlayer)
        m_tailPlayer->stop();
}

void player::onTailPositionChanged(qint64 positionMs)
{
    // Only worth recomputing while a volume line is in play; without one the
    // fade animation already writes every value the tail needs.
    if (!m_tailEnvelopeActive)
        return;
    applyTailVolume(positionMs);
}

// The one place the tail's volume is decided, so the fade and the outgoing
// track's own volume line can never end up fighting over it: they multiply.
// A voice-track segue runs no fade, so its factor stays at 1 and the line
// alone carries the track out; an ordinary segue with no line has no envelope
// to apply and is the plain fade it always was.
void player::applyTailVolume(qint64 positionMs)
{
    if (!m_tailOutput)
        return;
    const double shape = m_tailEnvelopeActive
        ? PlaylistWaveView::envelopeGainAt(m_tailEnvelope, positionMs)
        : 1.0;
    m_tailOutput->setVolume(float(qBound(0.0, m_tailBaseVolume * shape * m_tailFadeFactor, 1.0)));
}

// ---------------------------------------------------------------------------
// Voice tracking
//
// The presenter's link between two songs, recorded over the join and inserted
// as a track of its own. Everything it leaves behind is ordinary: a file, a
// playlist item, an overlap and two volume lines. The only new thing in the
// playlist is the VoiceTrackRole marker, which exists so the segue knows not
// to fade a link the way it fades a song (see startOverlapSegue()).
// ---------------------------------------------------------------------------

int player::voiceTrackTargetRow()
{
    if (ui->playlist->count() < 2) {
        const QString why = tr("A voice track sits between two tracks. Put at "
                               "least two in the playlist first.");
        announceAccessible(why);
        QMessageBox::information(this, tr("Voice track"), why);
        return -1;
    }

    int row = ui->playlist->currentRow();
    if (row < 1) {
        // The join above row 0 is with whatever is already on air, and that
        // track's playlist item is gone — there is nothing left to write a
        // volume line onto, so the ducking could only be half done. Offer the
        // first join XFB can actually prepare instead of half-doing it.
        row = 1;
        ui->playlist->setCurrentRow(row);
        announceAccessible(tr("The first join XFB can prepare is the one above "
                              "track 2; moved there."));
    }
    return row;
}

void player::openVoiceTrackDialog(int joinRow)
{
    if (joinRow < 1 || joinRow >= ui->playlist->count())
        return;
    if (!m_waveStore) {
        QMessageBox::information(this, tr("Voice track"),
                                 tr("The sound-wave store is not available, so "
                                    "XFB cannot analyse the join."));
        return;
    }

    QListWidgetItem *prevItem = ui->playlist->item(joinRow - 1);
    QListWidgetItem *nextItem = ui->playlist->item(joinRow);
    if (!prevItem || !nextItem)
        return;

    const QString prevPath = prevItem->text();
    const QString nextPath = nextItem->text();
    const qint64 joinOverlap =
        nextItem->data(PlaylistWaveView::OverlapRole).toLongLong();

    announceAccessible(tr("Voice track over the join between %1 and %2.")
                           .arg(QFileInfo(prevPath).fileName(),
                                QFileInfo(nextPath).fileName()));

    VoiceTrackDialog dlg(prevPath, nextPath, joinOverlap, m_waveStore, m_cueBus, this);
    connect(&dlg, &VoiceTrackDialog::announcementRequested,
            this, &player::announceAccessible);
    if (dlg.exec() != QDialog::Accepted) {
        announceAccessible(tr("Voice track discarded."));
        return;
    }

    const VoiceTrackDialog::Result r = dlg.result();
    if (r.takePath.isEmpty())
        return;

    // The dialog is modal but playback is not: auto mode and the segue both
    // consume row 0 while it is open, so the item pointers taken above may be
    // dangling by now. Re-find the join by path; if it has gone to air in the
    // meantime, say so rather than inserting the link somewhere else.
    int row = -1;
    for (int i = 1; i < ui->playlist->count(); ++i) {
        if (ui->playlist->item(i)->text() == nextPath
                && ui->playlist->item(i - 1)->text() == prevPath) {
            row = i;
            break;
        }
    }
    if (row < 0) {
        const QString why = tr("That join has already gone to air, so the link "
                               "was not inserted. The recording is kept at %1.")
                                .arg(r.takePath);
        announceAccessible(why);
        QMessageBox::information(this, tr("Voice track"), why);
        return;
    }
    prevItem = ui->playlist->item(row - 1);
    nextItem = ui->playlist->item(row);

    auto *voice = new QListWidgetItem(r.takePath);
    voice->setData(PlaylistWaveView::OverlapRole, r.leadMs);
    voice->setData(PlaylistWaveView::VoiceTrackRole, true);
    ui->playlist->insertItem(row, voice);

    // The incoming song keeps starting where it always did — its overlap is
    // now measured against the link rather than against the outgoing song.
    nextItem->setData(PlaylistWaveView::OverlapRole, r.nextOverlapMs);

    // The ducking. Ordinary volume lines, editable in the wave view and saved
    // with the playlist like any other.
    if (!r.prevEnvelope.isEmpty())
        prevItem->setData(PlaylistWaveView::VolumeEnvelopeRole, r.prevEnvelope);
    if (!r.nextEnvelope.isEmpty())
        nextItem->setData(PlaylistWaveView::VolumeEnvelopeRole, r.nextEnvelope);

    calculate_playlist_total_time();
    if (m_waveViewToggle && !m_waveViewToggle->isChecked())
        m_waveViewToggle->setChecked(true); // the duck is inspected there
    if (m_waveView)
        m_waveView->refresh();
    ui->playlist->setCurrentItem(voice);

    const bool ducked = !r.prevEnvelope.isEmpty() || !r.nextEnvelope.isEmpty();
    announceAccessible(ducked
        ? tr("Voice track added at position %1, starting %2 seconds before %3 "
             "ends. The ducking was written onto both songs.")
              .arg(row + 1)
              .arg(r.leadMs / 1000.0, 0, 'f', 1)
              .arg(QFileInfo(prevPath).fileName())
        : tr("Voice track added at position %1. No ducking was written — no "
             "speech was found in the take.").arg(row + 1));
    qInfo() << "Voice track inserted at row" << row << r.takePath
            << "lead" << r.leadMs << "ms, next overlap" << r.nextOverlapMs << "ms";
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
    if (m_autoMixButton)
        m_autoMixButton->setVisible(on);
    // The wave strip already draws a playhead, so the slider row can go.
    applyProgressBarVisibility();
    relayoutPlayerFrame();
    // A running auto-mix pass is deliberately not canceled here: the
    // overlaps it sets are honoured by playback whether the view is on or
    // off, and the progress dialog's Cancel button still works.

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/xfb.conf", QSettings::IniFormat);
    if (settings.value("PlaylistWaveView", false).toBool() != on)
        settings.setValue("PlaylistWaveView", on);
}

void player::startAutoMix(const QVector<int> &rows)
{
    if (!m_waveView || m_waveView->autoMixActive())
        return;

    m_waveView->autoMix(rows);
    if (!m_waveView->autoMixActive())
        return; // everything was cached: finished synchronously, no dialog

    if (m_autoMixButton)
        m_autoMixButton->setEnabled(false);
    m_autoMixProgress = new QProgressDialog(tr("Auto-mix: analyzing waveforms..."),
                                            tr("Cancel"), 0, 0, this);
    m_autoMixProgress->setWindowModality(Qt::WindowModal);
    connect(m_autoMixProgress, &QProgressDialog::canceled,
            m_waveView, &PlaylistWaveView::cancelAutoMix);
}

// Measure the tempo of every library track that has never been analysed.
// Each one costs a full decode, so this is an explicit operation rather
// than something that happens behind the operator's back — but it only
// ever runs once per file, and auto mode's tempo matching is only as good
// as the share of the library it has covered.
void player::analyzeLibraryBpm()
{
    if (!m_bpmLibrary)
        return;
    if (m_bpmLibrary->busy()) {
        ui->statusBar->showMessage(tr("BPM analysis is already running"), 5000);
        return;
    }

    QStringList pending = m_bpmLibrary->tracksMissingBpm();
    if (pending.isEmpty()) {
        // Nothing is missing, but a measurement is only as good as the
        // detector that made it — offer the re-run rather than turning the
        // operator away, which is the only way results from an older
        // version of XFB ever get refreshed.
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Analyze BPM"),
            tr("Every track in the database has already been measured.\n\n"
               "Measure them all again? This is worth doing after an update, "
               "and it is the way to refresh tracks whose tempo looks wrong. "
               "Any tempo you typed in by hand will be overwritten."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;

        pending = m_bpmLibrary->allTracks();
        if (pending.isEmpty())
            return;
    }

    m_bpmProgress = new QProgressDialog(tr("Measuring the tempo of %n track(s)...",
                                           nullptr, pending.size()),
                                        tr("Cancel"), 0, pending.size(), this);
    m_bpmProgress->setWindowModality(Qt::WindowModal);
    m_bpmProgress->setMinimumDuration(0);
    connect(m_bpmProgress, &QProgressDialog::canceled,
            m_bpmLibrary, &BpmLibrary::cancel);

    // Both connections are scoped to the dialog: a second run gets its own.
    connect(m_bpmLibrary, &BpmLibrary::progress, m_bpmProgress,
            [this](int done, int total) {
        if (m_bpmProgress) {
            m_bpmProgress->setMaximum(total);
            m_bpmProgress->setValue(done);
        }
    });
    connect(m_bpmLibrary, &BpmLibrary::finished, m_bpmProgress,
            [this](int analyzed, int skipped, bool canceled) {
        if (m_bpmProgress)
            m_bpmProgress->deleteLater();
        ui->statusBar->showMessage(
            canceled
                ? tr("BPM analysis canceled — %1 track(s) measured").arg(analyzed)
                : tr("BPM analysis: %1 track(s) measured, %2 without a steady tempo")
                      .arg(analyzed).arg(skipped),
            8000);
        update_music_table(); // show the new numbers in the BPM column
    });

    m_bpmLibrary->analyze(pending);
}

// ---------------------------------------------------------------------------
// Intro (ramp) and outro times
//
// Three ways a track gets its numbers:
//
//   1. lazily, the first time it goes to air (refreshIntroForCurrentTrack
//      below, off the sourceChanged handler),
//   2. in bulk, from Database -> "Measure the intro and outro times of the
//      database" (analyzeLibraryIntro), or
//   3. by hand, by dragging the marker on the now-playing wave strip —
//      which locks the row so neither of the other two can undo it.
//
// The measurement itself is IntroDetector's, over the 20 ms peak buckets
// WaveformStore extracts; see that header for what the heuristic can and
// cannot do. Nothing here decodes anything the wave view has not already
// paid for.
// ---------------------------------------------------------------------------

// Push one set of times into the strip and remember them for the speech.
void player::applyIntroTimes(const IntroTimes &times)
{
    m_currentIntroMs = times.introMs;
    m_currentOutroMs = times.outroMs;
    m_currentIntroLocked = times.locked;
    if (m_nowPlayingWave)
        m_nowPlayingWave->setIntro(times.introMs, times.outroMs, times.locked);

    // The number a presenter talks over: say it privately, once, as the
    // track starts. On screen the wave strip already counts it down.
    if (m_cueCountdown && m_cueBus && times.introMs > 0
            && Xplayer && Xplayer->playbackState() == QMediaPlayer::PlayingState
            && Xplayer->position() < times.introMs) {
        m_cueBus->speak(tr("Intro %1").arg(spokenDuration(times.introMs)));
    }
}

// The on-air track's intro: shown on the strip, and measured in the
// background when the library has never seen it.
void player::refreshIntroForCurrentTrack()
{
    if (!m_introLibrary || !m_nowPlayingWave)
        return;

    const QString path = m_nowPlayingWave->track();
    if (path.isEmpty()) {
        applyIntroTimes(IntroTimes{});
        m_introAnnouncedPath.clear();
        return;
    }

    const IntroTimes times = m_introLibrary->timesFor(path);
    applyIntroTimes(times);

    // Not in the library, or never measured: ask for it. analyzeQuietly()
    // is a no-op on a row that already has a number or is locked, so this
    // costs nothing on the tracks that matter most (the ones played often).
    if (times.known && times.introMs < 0)
        m_introLibrary->analyzeQuietly(path);
}

// Spoken form of the intro. Two shapes, because the useful number changes
// while the track runs: before the vocal it is the time LEFT of the ramp,
// after it there is nothing left to talk over.
QString player::introAnnouncement() const
{
    if (m_nowPlayingWave && m_nowPlayingWave->track().isEmpty())
        return tr("Nothing is playing");
    if (m_currentIntroMs < 0)
        return tr("The intro of this track has not been measured");
    if (m_currentIntroMs == 0)
        return tr("No intro: this track starts straight in");

    const qint64 position = Xplayer ? Xplayer->position() : 0;
    if (position < m_currentIntroMs) {
        return tr("%1 of intro left, of %2")
            .arg(spokenDuration(m_currentIntroMs - position),
                 spokenDuration(m_currentIntroMs));
    }
    return m_currentIntroLocked
        ? tr("Intro %1, set by hand, already passed")
              .arg(spokenDuration(m_currentIntroMs))
        : tr("Intro %1, already passed").arg(spokenDuration(m_currentIntroMs));
}

// Measure the intro of every library track that has never been analysed.
// Sibling of analyzeLibraryBpm() above and scanLibraryLoudness() below: a
// modal, cancellable progress dialog that owns the signal connections, so
// a second run gets a clean set of its own.
void player::analyzeLibraryIntro()
{
    if (!m_introLibrary)
        return;
    if (m_introLibrary->busy()) {
        ui->statusBar->showMessage(tr("Intro analysis is already running"), 5000);
        return;
    }

    QStringList pending = m_introLibrary->tracksMissingIntro();
    bool forced = false;
    if (pending.isEmpty()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Measure intros"),
            tr("Every track in the database already has an intro time.\n\n"
               "Measure them all again? Anything you corrected by hand is "
               "kept — only the values XFB worked out itself are replaced."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;

        pending = m_introLibrary->allTracks();
        if (pending.isEmpty())
            return;
        forced = true;
    }

    m_introProgress = new QProgressDialog(
        tr("Measuring the intro of %n track(s)...", nullptr, pending.size()),
        tr("Cancel"), 0, pending.size(), this);
    m_introProgress->setWindowModality(Qt::WindowModal);
    m_introProgress->setMinimumDuration(0);
    connect(m_introProgress, &QProgressDialog::canceled,
            m_introLibrary, &IntroLibrary::cancel);

    // Both connections are scoped to the dialog, so they die with it.
    connect(m_introLibrary, &IntroLibrary::progress, m_introProgress,
            [this](int done, int total) {
        if (m_introProgress) {
            m_introProgress->setMaximum(total);
            m_introProgress->setValue(done);
        }
    });
    connect(m_introLibrary, &IntroLibrary::finished, m_introProgress,
            [this, forced](int measured, int skipped, bool canceled) {
        if (m_introProgress)
            m_introProgress->deleteLater();
        ui->statusBar->showMessage(
            canceled
                ? tr("Intro analysis canceled - %1 track(s) measured").arg(measured)
                : (forced
                       ? tr("Intros: %1 track(s) measured, %2 left alone "
                            "(set by hand, silent or unreadable)")
                             .arg(measured).arg(skipped)
                       : tr("Intros: %1 track(s) measured, %2 could not be measured")
                             .arg(measured).arg(skipped)),
            8000);
        // The track on air may have been one of them.
        refreshIntroForCurrentTrack();
    });

    m_introLibrary->analyze(pending);
}

// ---------------------------------------------------------------------------
// EBU R128 loudness
//
// Two halves that meet in the database:
//
//   1. A sweep (Database -> "Measure the loudness (EBU R128) of the
//      database") runs LoudnessScanner over every row that has no
//      measurement, or whose file has been replaced since it was measured,
//      and stores integrated loudness / true peak / the file's mtime.
//
//   2. Playback reads those two numbers for the track being loaded and
//      hands the FX engine a gain of (target - integrated) dB, reduced so
//      the track's true peak cannot pass the ceiling.
//
// Nothing is re-encoded, ever. The library keeps its files exactly as they
// are, and a re-scan costs one ffmpeg pass per file.
//
// The gain lands in FxDsp's GainStage, at the head of the engine's chain.
// The playlist's volume envelope keeps riding the sink volume, where it
// always has. They are different quantities — the gain is DECIBELS, the
// envelope is a LINEAR 0..1 multiplier — and they multiply, so the
// operator's fader and volume lines keep their exact meaning.
// ---------------------------------------------------------------------------

namespace
{
// Every media table that holds a path and can be put to air.
const char *const kLoudnessTables[] = {"musics", "jingles", "pub", "programs"};

qint64 fileMtimeSecs(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists())
        return 0;
    return info.lastModified().toSecsSinceEpoch();
}
} // namespace

// Rows that still need measuring: never measured, or measured against a
// different version of the file. This is what makes the sweep resumable —
// cancel it half way and the next run picks up where it stopped.
QStringList player::tracksNeedingLoudness(int *alreadyMeasured) const
{
    QStringList pending;
    QSet<QString> seen;
    int skipped = 0;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        if (alreadyMeasured)
            *alreadyMeasured = 0;
        return pending;
    }

    for (const char *const table : kLoudnessTables) {
        const QSqlRecord rec = db.record(QString::fromLatin1(table));
        if (rec.isEmpty() || !rec.contains(QStringLiteral("lufs")))
            continue;

        QSqlQuery query(db);
        query.prepare(QString("SELECT path, lufs, loudness_mtime FROM %1")
                          .arg(QString::fromLatin1(table)));
        if (!query.exec()) {
            qWarning() << "Loudness: could not list" << table
                       << ":" << query.lastError().text();
            continue;
        }

        while (query.next()) {
            const QString path = query.value(0).toString();
            if (path.isEmpty() || seen.contains(path))
                continue;
            seen.insert(path);

            const bool measured = !query.value(1).isNull();
            const qint64 storedMtime = query.value(2).toLongLong();
            const qint64 currentMtime = fileMtimeSecs(path);

            // A file that is gone is not worth an ffmpeg run; leave the row
            // alone so the operator's own housekeeping decides its fate.
            if (currentMtime == 0)
                continue;

            if (measured && storedMtime == currentMtime) {
                ++skipped;
                continue;
            }
            pending.append(path);
        }
    }

    if (alreadyMeasured)
        *alreadyMeasured = skipped;
    return pending;
}

void player::storeLoudness(const LoudnessMeasurement &measurement)
{
    if (measurement.filePath.isEmpty())
        return;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen())
        return;

    const qint64 mtime = fileMtimeSecs(measurement.filePath);

    for (const char *const table : kLoudnessTables) {
        const QSqlRecord rec = db.record(QString::fromLatin1(table));
        if (rec.isEmpty() || !rec.contains(QStringLiteral("lufs")))
            continue;

        QSqlQuery query(db);
        if (measurement.valid) {
            query.prepare(QString("UPDATE %1 SET lufs = :lufs, true_peak = :peak, "
                                  "loudness_mtime = :mtime WHERE path = :path")
                              .arg(QString::fromLatin1(table)));
            query.bindValue(QStringLiteral(":lufs"), measurement.integratedLufs);
            query.bindValue(QStringLiteral(":peak"), measurement.truePeakDbtp);
        } else {
            // Failed or silent: stamp the mtime only, so the row is not
            // retried on every sweep, but leave lufs NULL so playback knows
            // it has no usable measurement and leaves the track alone.
            query.prepare(QString("UPDATE %1 SET loudness_mtime = :mtime "
                                  "WHERE path = :path")
                              .arg(QString::fromLatin1(table)));
        }
        query.bindValue(QStringLiteral(":mtime"), mtime);
        query.bindValue(QStringLiteral(":path"), measurement.filePath);
        if (!query.exec()) {
            qWarning() << "Loudness: could not store the measurement of"
                       << measurement.filePath << ":" << query.lastError().text();
        }
    }

    // The track on air may be the one that was just measured.
    if (Xplayer && Xplayer->source().isLocalFile()
            && Xplayer->source().toLocalFile() == measurement.filePath) {
        m_loudnessAppliedPath.clear();
        applyLoudnessForSource(Xplayer->source());
    }
}

// Menu action. Follows the same shape as the Opus/Ogg conversion actions
// and the BPM sweep: a modal, cancellable progress dialog that owns the
// signal connections, so a second run gets a clean set of its own.
void player::scanLibraryLoudness()
{
    if (!m_loudnessScanner)
        return;

    if (m_loudnessScanner->busy()) {
        ui->statusBar->showMessage(tr("A loudness scan is already running"), 5000);
        return;
    }

    if (!LoudnessScanner::available()) {
        QMessageBox::warning(this, tr("Measure loudness"),
            tr("ffmpeg was not found, so loudness cannot be measured.\n\n"
               "Install ffmpeg and try again."));
        return;
    }

    int alreadyMeasured = 0;
    QStringList pending = tracksNeedingLoudness(&alreadyMeasured);

    if (pending.isEmpty()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Measure loudness"),
            tr("Everything in the database has already been measured "
               "(%n item(s)).\n\nMeasure it all again?", nullptr, alreadyMeasured),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;

        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        QSet<QString> seen;
        for (const char *const table : kLoudnessTables) {
            const QSqlRecord rec = db.record(QString::fromLatin1(table));
            if (rec.isEmpty() || !rec.contains(QStringLiteral("lufs")))
                continue;
            QSqlQuery query(db);
            query.prepare(QString("SELECT path FROM %1").arg(QString::fromLatin1(table)));
            if (!query.exec())
                continue;
            while (query.next()) {
                const QString path = query.value(0).toString();
                if (!path.isEmpty() && !seen.contains(path)) {
                    seen.insert(path);
                    pending.append(path);
                }
            }
        }
        if (pending.isEmpty())
            return;
        alreadyMeasured = 0;
    }

    m_loudnessSkipped = alreadyMeasured;

    m_loudnessProgress = new QProgressDialog(
        tr("Measuring the loudness of %n item(s)...", nullptr, pending.size()),
        tr("Cancel"), 0, pending.size(), this);
    m_loudnessProgress->setWindowModality(Qt::WindowModal);
    m_loudnessProgress->setMinimumDuration(0);
    connect(m_loudnessProgress, &QProgressDialog::canceled,
            m_loudnessScanner, &LoudnessScanner::cancel);

    // Both connections are scoped to the dialog, so they die with it.
    connect(m_loudnessScanner, &LoudnessScanner::progress, m_loudnessProgress,
            [this](int done, int total) {
        if (m_loudnessProgress) {
            m_loudnessProgress->setMaximum(total);
            m_loudnessProgress->setValue(done);
        }
    });
    connect(m_loudnessScanner, &LoudnessScanner::finished, m_loudnessProgress,
            [this](int measured, int failed, bool canceled) {
        if (m_loudnessProgress)
            m_loudnessProgress->deleteLater();
        ui->statusBar->showMessage(
            canceled
                ? tr("Loudness scan canceled — %1 item(s) measured").arg(measured)
                : tr("Loudness: %1 item(s) measured, %2 skipped as unchanged, "
                     "%3 could not be measured")
                      .arg(measured).arg(m_loudnessSkipped).arg(failed),
            8000);
        update_music_table();
    });

    m_loudnessScanner->measure(pending);
}

// The measured numbers for a file, from whichever media table holds it.
bool player::loudnessForPath(const QString &path, double *lufs, double *truePeak) const
{
    if (path.isEmpty())
        return false;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen())
        return false;

    for (const char *const table : kLoudnessTables) {
        const QSqlRecord rec = db.record(QString::fromLatin1(table));
        if (rec.isEmpty() || !rec.contains(QStringLiteral("lufs")))
            continue;

        QSqlQuery query(db);
        query.prepare(QString("SELECT lufs, true_peak FROM %1 WHERE path = :path")
                          .arg(QString::fromLatin1(table)));
        query.bindValue(QStringLiteral(":path"), path);
        if (!query.exec() || !query.next())
            continue;
        if (query.value(0).isNull())
            continue; // row exists but was never measured

        if (lufs)
            *lufs = query.value(0).toDouble();
        if (truePeak) {
            // No true peak stored (an old row): claim no headroom at all,
            // which is the safe direction — the gain gets capped, not
            // waved through.
            *truePeak = query.value(1).isNull() ? 0.0 : query.value(1).toDouble();
        }
        return true;
    }

    return false;
}

// Compute and push the playback gain for the track being loaded. A track
// with no measurement plays at unity — never at a guess.
void player::applyLoudnessForSource(const QUrl &url)
{
    if (!Xplayer)
        return;

    const QString path = url.isLocalFile() ? url.toLocalFile() : QString();
    if (path == m_loudnessAppliedPath)
        return;
    m_loudnessAppliedPath = path;

    if (!m_loudnessEnabled || path.isEmpty()) {
        Xplayer->setLoudnessGainDb(0.0, true);
        return;
    }

    double lufs = 0.0;
    double truePeak = 0.0;
    if (!loudnessForPath(path, &lufs, &truePeak)) {
        Xplayer->setLoudnessGainDb(0.0, true);
        // Measure it in the background so the next play is corrected, as
        // long as a library sweep is not already using the process budget.
        if (m_loudnessScanner && !m_loudnessScanner->busy()
                && LoudnessScanner::available() && QFile::exists(path)) {
            m_loudnessScanner->measure({path});
        }
        return;
    }

    const double gainDb = LoudnessScanner::normalizationGainDb(
        lufs, truePeak, m_loudnessTargetLufs, m_loudnessCeilingDbTp);
    Xplayer->setLoudnessGainDb(gainDb, true);
    qDebug() << "Loudness:" << QFileInfo(path).fileName()
             << "I =" << lufs << "LUFS, TP =" << truePeak
             << "dBTP -> gain" << gainDb << "dB";
}

// Push the Options settings into the players. Called from updateConfig(),
// which the Options dialog's finished() signal is wired to, so a change
// takes effect on the track already playing.
void player::applyLoudnessSettings()
{
    const bool engineAvailable = FxPlayer::fxAvailable();
    const bool active = m_loudnessEnabled && engineAvailable;

    if (m_loudnessEnabled && !engineAvailable) {
        qWarning() << "Loudness normalisation is enabled but ffmpeg is missing; "
                      "playback stays uncorrected";
    }

    // The limiter guards the master output whenever normalisation is on: a
    // track measured wrong (or an operator's makeup gain) must not be able
    // to clip what leaves the building.
    for (FxPlayer *p : {Xplayer, m_tailPlayer, lp1_Xplayer, lp2_Xplayer}) {
        if (!p)
            continue;
        p->setLimiter(active, m_loudnessCeilingDbTp);
        p->setLoudnessActive(active);
    }

    // Recompute for whatever is on air right now.
    if (Xplayer) {
        m_loudnessAppliedPath.clear();
        applyLoudnessForSource(Xplayer->source());
    }
}


// ---------------------------------------------------------------------------
// Adding library tracks to the playlist without a mouse
// ---------------------------------------------------------------------------

// Which library view the operator is working in. Focus usually sits on a
// view's viewport rather than the view itself, so walk up the parent chain.
QTableView *player::focusedLibraryView() const
{
    QWidget *w = QApplication::focusWidget();
    while (w) {
        if (w == ui->musicView || w == ui->jinglesView || w == ui->programsView)
            return qobject_cast<QTableView *>(w);
        w = w->parentWidget();
    }
    return nullptr;
}

// Append (or prepend) the selected rows of a library view to the playlist.
// This is the same operation the context menu performs; it lives here so the
// Enter key and the menu entries can reach it too.
void player::addSelectionToPlaylist(QTableView *view, bool toTop)
{
    if (!view) {
        // Nothing focused: fall back to the music library, which is what the
        // operator almost always means.
        view = ui->musicView;
    }
    if (!view || !view->model() || !view->selectionModel()) {
        announceAccessible(tr("No track selected"));
        return;
    }

    // Path column differs per table: musics(id,artist,song,g1,g2,country,
    // published,path,...), jingles(name,path), programs(id,name,path).
    int pathColumn = -1;
    if (view == ui->musicView)          pathColumn = 7;
    else if (view == ui->jinglesView)   pathColumn = 1;
    else if (view == ui->programsView)  pathColumn = 2;
    if (pathColumn < 0)
        return;

    // selectedRows(0) would return nothing for the music view, whose column 0
    // is hidden, so ask for a column that is actually visible.
    const int probeColumn = (view == ui->musicView) ? 1 : 0;
    QModelIndexList rows = view->selectionModel()->selectedRows(probeColumn);
    if (rows.isEmpty() && view->currentIndex().isValid()) {
        // Keyboard users often just arrow to a row without extending a
        // selection; treat the current row as the target.
        rows << view->model()->index(view->currentIndex().row(), probeColumn);
    }
    if (rows.isEmpty()) {
        announceAccessible(tr("No track selected"));
        return;
    }

    QStringList paths;
    for (const QModelIndex &idx : rows) {
        const QModelIndex pathIdx = view->model()->index(idx.row(), pathColumn);
        if (pathIdx.isValid()) {
            const QString p = view->model()->data(pathIdx).toString();
            if (!p.isEmpty())
                paths << p;
        }
    }
    if (paths.isEmpty()) {
        announceAccessible(tr("No file path for the selected track"));
        return;
    }

    if (toTop) {
        for (int i = paths.size() - 1; i >= 0; --i)
            ui->playlist->insertItem(0, paths.at(i));
    } else {
        for (const QString &p : paths)
            ui->playlist->addItem(p);
    }
    calculate_playlist_total_time();

    // Confirm out loud — otherwise a blind operator has no way to tell the
    // track actually landed in the running order.
    const QString what = paths.size() == 1
                             ? QFileInfo(paths.first()).fileName()
                             : tr("%n tracks", "", paths.size());
    announceAccessible(toTop ? tr("Added %1 to the start of the playlist").arg(what)
                             : tr("Added %1 to the playlist").arg(what));
}

// ---------------------------------------------------------------------------
// Accessibility support
// ---------------------------------------------------------------------------

// Give every icon-only control a name. Qt derives a widget's accessible name
// from its text, so buttons that carry only an icon reach a screen reader as
// an anonymous "button". Tooltips are set to the same string so the
// information is available to sighted mouse users too.
void player::setupAccessibleControls()
{
    struct { QWidget *w; const char *name; const char *description; } controls[] = {
        {ui->btPlay,        QT_TR_NOOP("Play"),                 QT_TR_NOOP("Start playing the playlist")},
        {ui->btStop,        QT_TR_NOOP("Stop"),                 QT_TR_NOOP("Stop playback")},
        {ui->btPlayNext,    QT_TR_NOOP("Play next"),            QT_TR_NOOP("Skip to the next track in the playlist")},
        {ui->bt_pause_play, QT_TR_NOOP("Pause"),                QT_TR_NOOP("Pause or resume the track that is playing")},
        {ui->bt_pause_rec,  QT_TR_NOOP("Pause recording"),      QT_TR_NOOP("Pause or resume the recording")},
        {ui->lp_1_bt_play,  QT_TR_NOOP("Deck 1: play"),         QT_TR_NOOP("Play the file loaded in deck 1")},
        {ui->pushButton,    QT_TR_NOOP("Deck 1: stop"),         QT_TR_NOOP("Stop deck 1")},
        {ui->lp_1_bt_pause, QT_TR_NOOP("Deck 1: pause"),        QT_TR_NOOP("Pause or resume deck 1")},
        {ui->lp_1_bt_play_2,QT_TR_NOOP("Deck 2: play"),         QT_TR_NOOP("Play the file loaded in deck 2")},
        {ui->pushButton_2,  QT_TR_NOOP("Deck 2: stop"),         QT_TR_NOOP("Stop deck 2")},
        {ui->lp_2_bt_pause, QT_TR_NOOP("Deck 2: pause"),        QT_TR_NOOP("Pause or resume deck 2")},
        {ui->bt_rol_streaming_play, QT_TR_NOOP("Play the stream"), QT_TR_NOOP("Start playing the radio stream")},
        {ui->bt_rol_streaming_stop, QT_TR_NOOP("Stop the stream"), QT_TR_NOOP("Stop playing the radio stream")},
        {ui->bt_icecast,    QT_TR_NOOP("Icecast server"),       QT_TR_NOOP("Start or stop the Icecast streaming server")},
        {ui->bt_butt,       QT_TR_NOOP("BUTT broadcaster"),     QT_TR_NOOP("Start or stop the BUTT broadcasting tool")},
        {ui->bt_ddns,       QT_TR_NOOP("Update dynamic DNS"),   QT_TR_NOOP("Refresh the external address of this station")},
        {ui->bt_portTest,   QT_TR_NOOP("Test the streaming port"), QT_TR_NOOP("Check whether the streaming port is reachable")},
        {ui->bt_add_some_random_songs_from_genre, QT_TR_NOOP("Add random songs"),
                                                  QT_TR_NOOP("Add random songs of the selected genre to the playlist")},
        {ui->led_rec,       QT_TR_NOOP("Recording indicator"),  QT_TR_NOOP("Shows whether recording is active")},
    };

    for (const auto &c : controls) {
        if (!c.w)
            continue;
        c.w->setAccessibleName(tr(c.name));
        c.w->setAccessibleDescription(tr(c.description));
        if (c.w->toolTip().isEmpty())
            c.w->setToolTip(tr(c.description));
    }

    // The data views and the playlist are the main reading surfaces; name them
    // so a screen reader announces what a table is when focus lands on it.
    struct { QWidget *w; const char *name; } views[] = {
        {ui->musicView,    QT_TR_NOOP("Music library")},
        {ui->jinglesView,  QT_TR_NOOP("Jingles")},
        {ui->pubView,      QT_TR_NOOP("Adverts")},
        {ui->programsView, QT_TR_NOOP("Programs")},
        {ui->playlist,     QT_TR_NOOP("Playlist")},
        {ui->txt_search,   QT_TR_NOOP("Search the music library")},
    };
    for (const auto &v : views) {
        if (v.w)
            v.w->setAccessibleName(tr(v.name));
    }

    // Enter/Return (and double-click) on a library row puts it in the
    // playlist. Previously the only route was the right-click context menu,
    // which a Mac keyboard cannot even produce.
    for (QTableView *view : {ui->musicView, ui->jinglesView, ui->programsView}) {
        if (!view)
            continue;
        connect(view, &QAbstractItemView::activated, this, [this, view](const QModelIndex &) {
            addSelectionToPlaylist(view, false);
        });
        const QString hint = tr("Press Enter to add the selected track to the playlist.");
        view->setAccessibleDescription(
            view->accessibleDescription().isEmpty()
                ? hint
                : view->accessibleDescription() + QLatin1Char(' ') + hint);
    }

    refreshTransportAccessibleState();
}

// Build a Playback menu. Before this the transport existed only as on-screen
// buttons — there was not a single shortcut in the application and no menu
// entry for play/pause/stop/next, so the app could not be operated without a
// mouse. Shortcuts are application-wide so they work whatever has focus, and
// use Ctrl+Shift+<key> to stay clear of text entry and platform defaults.
void player::setupPlaybackShortcuts()
{
    if (!ui->menuBar)
        return;

    QMenu *playbackMenu = new QMenu(tr("&Playback"), this);

    // The icons are the ones on the transport buttons, so the menu entry and
    // the button an operator already knows read as the same command.
    struct { const char *text; const char *icon; QKeySequence key; void (player::*slot)(); } entries[] = {
        {QT_TR_NOOP("&Play / Segue"), ":/icons/flat/Play-64.png",
         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), &player::on_btPlay_clicked},
        {QT_TR_NOOP("Pause / &Resume"), ":/icons/flat/Pause-32.png",
         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Space), &player::on_bt_pause_play_clicked},
        {QT_TR_NOOP("&Stop"), ":/icons/flat/Stop Sign-32.png",
         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), &player::on_btStop_clicked},
        {QT_TR_NOOP("&Next track"), ":/icons/flat/Fast Forward-32.png",
         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), &player::on_btPlayNext_clicked},
        {QT_TR_NOOP("Pre&vious track"), ":/icons/flat/Previous-32.png",
         QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), &player::playPreviousMedia},
    };

    for (const auto &e : entries) {
        QAction *action = playbackMenu->addAction(QIcon(QLatin1String(e.icon)), tr(e.text));
        action->setShortcut(e.key);
        // Works no matter which panel currently holds focus.
        action->setShortcutContext(Qt::ApplicationShortcut);
        connect(action, &QAction::triggered, this, e.slot);
        addAction(action); // keep the shortcut alive even when the menu is closed
        AccessControl::instance().guard(action, QStringLiteral("playback.transport"));
    }

    playbackMenu->addSeparator();

    // Getting a track into the running order is the most common task in the
    // app and had no keyboard route at all. Menu entries make it discoverable
    // as well as reachable.
    QAction *addEnd = playbackMenu->addAction(QIcon(":/icons/align-vertical-bottom.png"),
                                              tr("Add selection to &end of playlist"));
    addEnd->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Return));
    addEnd->setShortcutContext(Qt::ApplicationShortcut);
    connect(addEnd, &QAction::triggered, this, [this]() {
        addSelectionToPlaylist(focusedLibraryView(), false);
    });
    addAction(addEnd);
    AccessControl::instance().guard(addEnd, QStringLiteral("playlist.edit"));

    QAction *addTop = playbackMenu->addAction(QIcon(":/icons/align-vertical-top.png"),
                                              tr("Add selection to &start of playlist"));
    addTop->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Return));
    addTop->setShortcutContext(Qt::ApplicationShortcut);
    connect(addTop, &QAction::triggered, this, [this]() {
        addSelectionToPlaylist(focusedLibraryView(), true);
    });
    addAction(addTop);
    AccessControl::instance().guard(addTop, QStringLiteral("playlist.edit"));

    playbackMenu->addSeparator();

    // Reordering the running order without a mouse. Until now the only way to
    // move a track was to drag it, which is no way at all for a keyboard-only
    // operator — the right-click menu offered only "move to top"/"move to
    // bottom". takeItem/insertItem move the same item object, so the crossfade
    // overlap and volume envelope stored on it survive the move (same idiom as
    // the context menu above). The new position is announced, because a screen
    // reader gives no feedback on a row that moves under the cursor.
    const auto movePlaylistItem = [this](int delta) {
        const int row = ui->playlist->currentRow();
        if (row < 0) {
            announceAccessible(tr("No playlist track is selected"));
            return;
        }
        const int target = row + delta;
        if (target < 0 || target >= ui->playlist->count()) {
            announceAccessible(delta < 0 ? tr("Already at the top of the playlist")
                                         : tr("Already at the bottom of the playlist"));
            return;
        }
        QListWidgetItem *moved = ui->playlist->takeItem(row);
        if (!moved)
            return;
        ui->playlist->insertItem(target, moved);
        ui->playlist->setCurrentItem(moved);
        announceAccessible(tr("%1 moved to position %2 of %3")
                               .arg(moved->text())
                               .arg(target + 1)
                               .arg(ui->playlist->count()));
    };

    QAction *moveUp = playbackMenu->addAction(QIcon(":/icons/align-vertical-top.png"),
                                              tr("Move playlist track &up"));
    moveUp->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up));
    moveUp->setShortcutContext(Qt::ApplicationShortcut);
    connect(moveUp, &QAction::triggered, this, [movePlaylistItem]() { movePlaylistItem(-1); });
    addAction(moveUp);
    AccessControl::instance().guard(moveUp, QStringLiteral("playlist.edit"));

    QAction *moveDown = playbackMenu->addAction(QIcon(":/icons/align-vertical-bottom.png"),
                                                tr("Move playlist track &down"));
    moveDown->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down));
    moveDown->setShortcutContext(Qt::ApplicationShortcut);
    connect(moveDown, &QAction::triggered, this, [movePlaylistItem]() { movePlaylistItem(1); });
    addAction(moveDown);
    AccessControl::instance().guard(moveDown, QStringLiteral("playlist.edit"));

    playbackMenu->addSeparator();

    // "What is playing?" — the single most useful thing for a blind operator.
    QAction *whatsPlaying = playbackMenu->addAction(QIcon(":/icons/ic_launcher_voicedial.png"),
                                                    tr("Announce &what is playing"));
    whatsPlaying->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
    whatsPlaying->setShortcutContext(Qt::ApplicationShortcut);
    connect(whatsPlaying, &QAction::triggered, this, [this]() {
        const QString title = ui->txtNowPlaying->text().trimmed();
        if (title.isEmpty()) {
            announceAccessible(tr("Nothing is playing"));
            return;
        }
        QString message = tr("Playing: %1").arg(title);
        // One utterance rather than two: the announcement queue delivers a
        // message per timer tick, and a screen reader given two in quick
        // succession routinely drops or truncates the first.
        if (announcesRemainingWithNowPlaying() && Xplayer
                && Xplayer->playbackState() != QMediaPlayer::StoppedState
                && trackTotalDuration > 0) {
            message = tr("%1. %2").arg(message, remainingTimeAnnouncement());
        }
        announceAccessible(message);
    });
    addAction(whatsPlaying);

    // The time left is what decides when to talk over the outro, so it also
    // gets a key of its own — the operator should not have to sit through the
    // track title to hear it, and it stays reachable when the combined
    // announcement above is switched off.
    QAction *timeRemaining = playbackMenu->addAction(QIcon(":/icons/player-time.png"),
                                                     tr("Announce time &remaining"));
    timeRemaining->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    timeRemaining->setShortcutContext(Qt::ApplicationShortcut);
    connect(timeRemaining, &QAction::triggered, this, [this]() {
        announceAccessible(remainingTimeAnnouncement());
    });
    addAction(timeRemaining);

    // The presenter's other clock: how long they can still talk before the
    // vocal. The countdown is on screen on the wave strip, which is no use
    // at all to an operator working by ear, so it gets a key of its own.
    // Ctrl+Shift+I was the free one — L, P, S, N, B, W, H, R, A, T, Space,
    // Up, Down, Left, Right and Return are all already spoken for.
    QAction *introCountdown = playbackMenu->addAction(QIcon(":/icons/player-time.png"),
                                                      tr("Announce the &intro countdown"));
    introCountdown->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    introCountdown->setShortcutContext(Qt::ApplicationShortcut);
    introCountdown->setStatusTip(tr("Say how much of the intro is left before the vocal"));
    connect(introCountdown, &QAction::triggered, this, [this]() {
        announceAccessible(introAnnouncement());
    });
    addAction(introCountdown);

    playbackMenu->addSeparator();

    // --- Cue bus (pre-fade listen) ---
    // Ctrl+Shift+C and Ctrl+Shift+X were the free pair; C for cue and X for
    // "cut it". Ctrl+Shift+ P, S, N, B, W, H, R, L, I, Up, Down, Return,
    // Space and Ctrl+Alt+ R, Return are all already spoken for.
    m_cueAction = playbackMenu->addAction(QIcon(":/icons/ic_launcher_voicedial.png"),
                                          tr("&Cue the selected track (headphones)"));
    m_cueAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    m_cueAction->setShortcutContext(Qt::ApplicationShortcut);
    m_cueAction->setStatusTip(tr("Listen to the selected track in the cue headphones only"));
    connect(m_cueAction, &QAction::triggered, this, [this]() { cueCurrentSelection(); });
    addAction(m_cueAction);
    AccessControl::instance().guard(m_cueAction, QStringLiteral("playback.cue"));

    // --- Voice tracking ---
    // Ctrl+Shift+V: V for voice, and the last obvious free letter. Ctrl+Shift+
    // P, S, N, B, W, H, R, L, I, C, X, Up, Down, Return, Space and Ctrl+Alt+ R,
    // Return are all already spoken for.
    QAction *voiceTrack = playbackMenu->addAction(QIcon(":/icons/ic_launcher_voicedial.png"),
                                                  tr("&Voice track over this join..."));
    voiceTrack->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    voiceTrack->setShortcutContext(Qt::ApplicationShortcut);
    voiceTrack->setStatusTip(tr("Record your link over the join above the selected "
                                "playlist track; XFB writes the ducking"));
    connect(voiceTrack, &QAction::triggered, this, [this]() {
        const int row = voiceTrackTargetRow();
        if (row >= 1)
            openVoiceTrackDialog(row);
    });
    addAction(voiceTrack);
    AccessControl::instance().guard(voiceTrack, QStringLiteral("playback.voicetrack"));

    m_cueStopAction = playbackMenu->addAction(QIcon(":/icons/flat/Stop Sign-32.png"),
                                              tr("Stop the c&ue"));
    m_cueStopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X));
    m_cueStopAction->setShortcutContext(Qt::ApplicationShortcut);
    m_cueStopAction->setEnabled(false);
    connect(m_cueStopAction, &QAction::triggered, this, [this]() {
        if (m_cueBus) {
            m_cueBus->stopCue();
            announceAccessible(tr("Cue stopped"));
        }
    });
    addAction(m_cueStopAction);
    AccessControl::instance().guard(m_cueStopAction, QStringLiteral("playback.cue"));

    // Insert before Options so File/Playlists keep their familiar positions.
    ui->menuBar->insertMenu(ui->menuXFB->menuAction(), playbackMenu);
    qInfo() << "Playback menu installed with" << playbackMenu->actions().count()
            << "keyboard-accessible actions";

    // Tutorial for operators who work by ear. Lives in Help, where someone
    // looking for guidance will go first, and has its own shortcut so it can
    // be reached without hunting through the menus.
    if (ui->menuHelp) {
        QAction *tutorial = new QAction(QIcon(":/icons/help-hint.png"),
                                        tr("&Tutorial for Blind Users"), this);
        tutorial->setMenuRole(QAction::NoRole);
        tutorial->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
        tutorial->setShortcutContext(Qt::ApplicationShortcut);
        tutorial->setStatusTip(tr("Learn how to run XFB with a screen reader and the keyboard"));
        connect(tutorial, &QAction::triggered, this, [this]() {
            // Non-modal and owned by the window, so the operator can try each
            // step in the main window while the tutorial stays open. Reuse a
            // single instance rather than stacking copies.
            if (!m_tutorialDialog) {
                m_tutorialDialog = new AccessibilityTutorialDialog(this);
                connect(m_tutorialDialog, &AccessibilityTutorialDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_tutorialDialog->show();
            m_tutorialDialog->raise();
            m_tutorialDialog->activateWindow();
            announceAccessible(tr("Tutorial for blind users opened"));
        });
        ui->menuHelp->addAction(tutorial);
        addAction(tutorial);
    }

    // The Accessibility Preferences dialog was fully implemented but had no
    // way in — no menu entry and no code path opened it — so the settings it
    // manages (verbosity, announcement timing) were unreachable.
    if (ui->menuXFB) {
        QAction *a11yPrefs = new QAction(QIcon(":/icons/text-speak.png"),
                                         tr("&Accessibility Preferences..."), this);
        // Without this, Qt's macOS text heuristic sees "Preferences" and moves
        // the item into the application menu, away from the Options menu where
        // the rest of XFB's settings live.
        a11yPrefs->setMenuRole(QAction::NoRole);
        a11yPrefs->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
        a11yPrefs->setShortcutContext(Qt::ApplicationShortcut);
        connect(a11yPrefs, &QAction::triggered, this, [this]() {
            auto *container = ServiceContainer::instance();
            auto *settings = container ? container->resolve<AccessibilitySettingsService>() : nullptr;
            if (!settings) {
                QMessageBox::warning(this, tr("Accessibility"),
                    tr("The accessibility settings service is not available."));
                return;
            }
            auto *braille = container->resolve<BrailleDisplayService>();
            AccessibilityPreferencesDialog dialog(settings, braille, this);
            dialog.exec();
        });
        ui->menuXFB->addSeparator();
        ui->menuXFB->addAction(a11yPrefs);
        addAction(a11yPrefs);
    }

    // Sync to phone. Lives next to the other station-wide settings because it
    // is a property of this installation, not of the playlist on air.
    if (ui->menuXFB) {
        QAction *mobileSync = new QAction(
            QIcon(":/icons/flat/Connection Sync-48.png"), tr("Sync to &Phone..."), this);
        mobileSync->setMenuRole(QAction::NoRole);
        connect(mobileSync, &QAction::triggered, this, [this]() {
            if (!m_mobileSyncDialog) {
                m_mobileSyncDialog = new MobileSyncDialog(mobileSyncServer(), this);
                m_mobileSyncDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_mobileSyncDialog, &MobileSyncDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_mobileSyncDialog->show();
            m_mobileSyncDialog->raise();
            m_mobileSyncDialog->activateWindow();
        });
        ui->menuXFB->addAction(mobileSync);
        addAction(mobileSync);
        AccessControl::instance().guard(mobileSync, QStringLiteral("station.sync.mobile"));

        // Constructing the server is what honours the auto-start setting, so
        // an operator who asked for it does not have to open the dialog first.
        QSettings syncSettings;
        if (syncSettings.value(QStringLiteral("MobileSync/AutoStart"), false).toBool())
            mobileSyncServer();

        // Broadcast redundancy: the same window is used on the machine that is
        // on air and on the one standing by, because when it is being set up
        // nobody yet knows which will turn out to be which.
        QAction *stationSync = new QAction(
            QIcon(":/icons/flat/Connection Sync-48.png"),
            tr("Broadcast &Redundancy..."), this);
        stationSync->setMenuRole(QAction::NoRole);
        connect(stationSync, &QAction::triggered, this, [this]() {
            if (!m_stationSyncDialog) {
                m_stationSyncDialog = new StationSyncDialog(mobileSyncServer(),
                                                            stationSyncClient(), this);
                m_stationSyncDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_stationSyncDialog, &StationSyncDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_stationSyncDialog->show();
            m_stationSyncDialog->raise();
            m_stationSyncDialog->activateWindow();
        });
        ui->menuXFB->addAction(stationSync);
        addAction(stationSync);
        AccessControl::instance().guard(stationSync, QStringLiteral("station.sync.station"));

        // Streaming from inside XFB. Sits with the other station-wide
        // settings for the same reason the sync windows do: it is a property
        // of this installation, not of the playlist on air.
        QAction *streamAction = new QAction(
            QIcon(":/icons/flat/Connection Sync-48.png"), tr("Stream to &Icecast..."), this);
        streamAction->setMenuRole(QAction::NoRole);
        connect(streamAction, &QAction::triggered, this, [this]() {
            if (!m_streamDialog) {
                m_streamDialog = new StreamDialog(streamService(), this);
                m_streamDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_streamDialog, &StreamDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_streamDialog->show();
            m_streamDialog->raise();
            m_streamDialog->activateWindow();
        });
        ui->menuXFB->addAction(streamAction);
        addAction(streamAction);
        AccessControl::instance().guard(streamAction, QStringLiteral("station.stream"));

        // An operator who asked to go on air at startup should not have to
        // open the window first; constructing the service is what honours it.
        if (StreamService::autoStartEnabled())
            streamService()->start();

        // The dead-air watchdog. Same shelf as the rest: a property of this
        // installation, not of what is on air right now.
        QAction *deadAir = new QAction(QIcon(":/icons/flat/Security Checked-48.png"),
                                       tr("&Dead-Air Watchdog..."), this);
        deadAir->setMenuRole(QAction::NoRole);
        deadAir->setStatusTip(tr("What to do when the station goes quiet"));
        connect(deadAir, &QAction::triggered, this, [this]() {
            if (!m_deadAirDialog) {
                m_deadAirDialog = new DeadAirDialog(deadAirWatchdog(), this);
                m_deadAirDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_deadAirDialog, &DeadAirDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_deadAirDialog->show();
            m_deadAirDialog->raise();
            m_deadAirDialog->activateWindow();
        });
        ui->menuXFB->addAction(deadAir);
        addAction(deadAir);
        AccessControl::instance().guard(deadAir, QStringLiteral("programming.deadair"));

        // Constructing it is what arms it, so an operator who turned it on
        // does not have to open the window every morning. Reading the setting
        // rather than building the object unconditionally is what keeps the
        // "off by default" promise literal: with it off, no timer runs.
        if (DeadAirWatchdog::loadConfig().enabled)
            deadAirWatchdog();
        // The as-run log. In the XFB menu with the other station-wide things:
        // it is a property of the installation, not of what is on air now.
        QAction *airLog = new QAction(QIcon(":/icons/flat/Music Transcript-48.png"),
                                      tr("As-Run &Log..."), this);
        airLog->setMenuRole(QAction::NoRole);
        airLog->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
        airLog->setShortcutContext(Qt::ApplicationShortcut);
        airLog->setStatusTip(tr("What actually went to air, and when"));
        connect(airLog, &QAction::triggered, this, [this]() {
            if (!m_airLogDialog) {
                m_airLogDialog = new AirLogDialog(this);
                m_airLogDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_airLogDialog, &AirLogDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_airLogDialog->show();
            m_airLogDialog->raise();
            m_airLogDialog->activateWindow();
            announceAccessible(tr("As-run log opened"));
        });
        ui->menuXFB->addAction(airLog);
        addAction(airLog);
        AccessControl::instance().guard(airLog, QStringLiteral("programming.airlog"));

        // The same question asked forwards. The scheduler table has been
        // written by "Add a publicity" and "Add a program" since the first
        // version and never once read back, so "what is on tomorrow?" had no
        // answer anywhere in XFB — and a booking whose file had moved aired
        // nothing without saying so. Next to the as-run log because they are
        // the same window in the two directions.
        QAction *schedule = new QAction(QIcon(":/icons/flat/Tasks-48.png"),
                                        tr("What Is &Scheduled..."), this);
        schedule->setMenuRole(QAction::NoRole);
        schedule->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
        schedule->setShortcutContext(Qt::ApplicationShortcut);
        schedule->setStatusTip(tr("What is booked to go to air, and when"));
        connect(schedule, &QAction::triggered, this, [this]() {
            if (!m_scheduleDialog) {
                m_scheduleDialog = new ScheduleDialog(this);
                m_scheduleDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_scheduleDialog, &ScheduleDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_scheduleDialog->show();
            m_scheduleDialog->raise();
            m_scheduleDialog->activateWindow();
            announceAccessible(tr("Schedule opened"));
        });
        ui->menuXFB->addAction(schedule);
        addAction(schedule);
        AccessControl::instance().guard(schedule, QStringLiteral("programming.schedule"));

        // Rotation rules. Next to the as-run log on purpose: the log is where
        // an operator notices the same artist coming round too often, and this
        // is where they do something about it.
        QAction *rotation = new QAction(QIcon(":/icons/flat/Shuffle-48.png"),
                                        tr("&Rotation Rules..."), this);
        rotation->setMenuRole(QAction::NoRole);
        // Ctrl+Shift+R is already the remaining-time toggle, so this takes the
        // Alt variant rather than quietly stealing a binding the operator has.
        rotation->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R));
        rotation->setShortcutContext(Qt::ApplicationShortcut);
        rotation->setStatusTip(tr("Artist and title separation, categories, "
                                  "dayparts and seasonal dates"));
        connect(rotation, &QAction::triggered, this, [this]() {
            // Whatever is highlighted in the music table is what the operator
            // means to program, so it arrives selected.
            QList<qint64> preselected;
            if (ui->musicView && ui->musicView->selectionModel()) {
                const QModelIndexList rows =
                    ui->musicView->selectionModel()->selectedRows(0);
                for (const QModelIndex &index : rows) {
                    const qint64 id = index.data().toLongLong();
                    if (id > 0 && !preselected.contains(id))
                        preselected.append(id);
                }
            }
            if (!m_rotationDialog) {
                m_rotationDialog = new RotationDialog(this, preselected);
                m_rotationDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_rotationDialog, &RotationDialog::announcementRequested,
                        this, &player::announceAccessible);
            } else {
                m_rotationDialog->preselect(preselected);
            }
            m_rotationDialog->show();
            m_rotationDialog->raise();
            m_rotationDialog->activateWindow();
            announceAccessible(tr("Rotation rules opened"));
        });
        ui->menuXFB->addAction(rotation);
        addAction(rotation);
        AccessControl::instance().guard(rotation, QStringLiteral("programming.rotation"));

        // The national music quota. Next to the rotation rules because it is
        // the same job seen from the other end: rotation decides what goes on
        // next, this says what the month has to add up to, and both are read
        // off the same as-run log.
        QAction *quota = new QAction(QIcon(":/icons/flat/Music Transcript-48.png"),
                                     tr("Music &Quota..."), this);
        quota->setMenuRole(QAction::NoRole);
        // Ctrl+Shift+Q would be quit on a few desktops; Ctrl+Alt+Q is free.
        quota->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Q));
        quota->setShortcutContext(Qt::ApplicationShortcut);
        quota->setStatusTip(tr("Mark the library for the national music quota, "
                               "and measure what went to air against it"));
        connect(quota, &QAction::triggered, this, [this]() {
            // Whatever is highlighted in the music table is what the operator
            // means to mark, so it arrives selected — the same as the rotation
            // editor above.
            QList<qint64> preselected;
            if (ui->musicView && ui->musicView->selectionModel()) {
                const QModelIndexList rows =
                    ui->musicView->selectionModel()->selectedRows(0);
                for (const QModelIndex &index : rows) {
                    const qint64 id = index.data().toLongLong();
                    if (id > 0 && !preselected.contains(id))
                        preselected.append(id);
                }
            }
            openMusicQuota(preselected);
        });
        ui->menuXFB->addAction(quota);
        addAction(quota);
        AccessControl::instance().guard(quota, QStringLiteral("programming.quota"));

        // The hour clock. It sits next to the rotation rules because the two
        // answer neighbouring questions: rotation says *which* record, the
        // clock says *what kind of thing* goes there and *when*.
        // The pie, not a clock face: chronometer.png already marks the three
        // analysis passes in the Database menu, and an icon that says two
        // different things is worse than none. A clock *is* a pie read
        // clockwise, which is what the wheel in the window draws.
        QAction *hourClock = new QAction(QIcon(":/icons/office-chart-pie.png"),
                                         tr("Hour &Clocks..."), this);
        hourClock->setMenuRole(QAction::NoRole);
        // Ctrl+Shift+K: the C, L and H of "clock" are all taken already.
        hourClock->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
        hourClock->setShortcutContext(Qt::ApplicationShortcut);
        hourClock->setStatusTip(tr("Programme the hour as a clock: sweeps, ad "
                                   "breaks, jingles and fixed-time items"));
        connect(hourClock, &QAction::triggered, this, [this]() {
            if (!m_hourClockDialog) {
                m_hourClockDialog = new HourClockDialog(this);
                m_hourClockDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_hourClockDialog, &HourClockDialog::announcementRequested,
                        this, &player::announceAccessible);
                // A clock or a setting changed: forget what has already fired
                // this hour, so an item the operator has just moved is not
                // held back by a key recorded against its old time.
                connect(m_hourClockDialog, &HourClockDialog::clocksChanged,
                        this, [this]() {
                            m_hourClockFired.clear();
                            setupHourClock();
                        });
            } else {
                m_hourClockDialog->reload();
            }
            m_hourClockDialog->show();
            m_hourClockDialog->raise();
            m_hourClockDialog->activateWindow();
            announceAccessible(tr("Hour clocks opened"));
        });
        ui->menuXFB->addAction(hourClock);
        addAction(hourClock);
        AccessControl::instance().guard(hourClock, QStringLiteral("programming.hourclock"));

        // Time signals. Under the hour clock because a reader looking for
        // "the thing that says the time" will look at the clock first, and
        // this is the entry that tells them it is its own window: the clock
        // shapes an hour, this puts one piece of audio on one second of it.
        // Not chronometer.png: that already marks the three analysis passes
        // in the Database menu, and an icon that says two different things is
        // worse than none.
        QAction *timeSignals = new QAction(QIcon(":/icons/view-time-schedule.png"),
                                           tr("&Time Signals..."), this);
        timeSignals->setMenuRole(QAction::NoRole);
        // Ctrl+Shift+T: T for time, and free on every platform XFB ships to.
        timeSignals->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
        timeSignals->setShortcutContext(Qt::ApplicationShortcut);
        timeSignals->setStatusTip(tr("The pips and the hour ident: what plays "
                                     "on the hour, in which hours, and how it "
                                     "reaches the air"));
        connect(timeSignals, &QAction::triggered, this, [this]() {
            if (!m_timeSignalDialog) {
                m_timeSignalDialog = new TimeSignalDialog(this);
                m_timeSignalDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_timeSignalDialog, &TimeSignalDialog::announcementRequested,
                        this, &player::announceAccessible);
                // A signal or a setting changed: forget what has already fired
                // this hour, so a time the operator has just moved is not held
                // back by a key recorded against the old one.
                connect(m_timeSignalDialog, &TimeSignalDialog::signalsChanged,
                        this, [this]() {
                            m_timeSignalFired.clear();
                            setupTimeSignals();
                        });
                // "Test now" goes to air by exactly the route the signal
                // asks for. A test that played somewhere else would prove
                // nothing about the thing being tested.
                connect(m_timeSignalDialog, &TimeSignalDialog::testRequested,
                        this, [this](const TimeSignal::Signal &signal) {
                            const int hour = QTime::currentTime().hour();
                            const QString path = TimeSignal::mediaPathFor(signal, hour);
                            if (!path.isEmpty())
                                fireTimeSignal(signal, path);
                        });
            } else {
                m_timeSignalDialog->reload();
            }
            m_timeSignalDialog->show();
            m_timeSignalDialog->raise();
            m_timeSignalDialog->activateWindow();
            announceAccessible(tr("Time signals opened"));
        });
        ui->menuXFB->addAction(timeSignals);
        addAction(timeSignals);
        AccessControl::instance().guard(timeSignals, QStringLiteral("programming.timesignal"));

        // Listener requests, and the switch that puts the public page on the
        // network at all. Same shelf as the rest: it is a property of this
        // installation, not of what is on air now. Opening the window is the
        // only way to turn the page on, and it ships off.
        QAction *requests = new QAction(QIcon(":/icons/im-message-new.png"),
                                        tr("Listener &Requests..."), this);
        requests->setMenuRole(QAction::NoRole);
        requests->setStatusTip(tr("The public now-playing page, and what "
                                  "listeners have asked for"));
        connect(requests, &QAction::triggered, this, [this]() {
            if (!m_requestTrayDialog) {
                m_requestTrayDialog = new RequestTrayDialog(mobileSyncServer(), this);
                m_requestTrayDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_requestTrayDialog, &RequestTrayDialog::announcementRequested,
                        this, &player::announceAccessible);
                // The one route from a listener's request to the air, and it
                // runs through the operator pressing a button in that window.
                connect(m_requestTrayDialog, &RequestTrayDialog::addToPlaylistRequested,
                        this, [this](const QString &path) {
                            if (path.isEmpty() || !ui || !ui->playlist)
                                return;
                            ui->playlist->addItem(path);
                            calculate_playlist_total_time();
                            ui->statusBar->showMessage(
                                tr("Requested track added to the running order: %1")
                                    .arg(QFileInfo(path).completeBaseName()), 8000);
                        });
            }
            m_requestTrayDialog->show();
            m_requestTrayDialog->raise();
            m_requestTrayDialog->activateWindow();
            announceAccessible(tr("Listener requests opened"));
        });
        ui->menuXFB->addAction(requests);
        addAction(requests);
        AccessControl::instance().guard(requests, QStringLiteral("programming.requests"));

        // A backup that only copies when somebody remembers to ask is not a
        // backup, so the client is built at startup whenever it has standing
        // orders — that is what starts its timer and its first pull.
        {
            QSettings stationSettings(
                QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                    + "/xfb.conf", QSettings::IniFormat);
            stationSettings.beginGroup(QStringLiteral("StationSync"));
            const bool onStart = stationSettings.value(QStringLiteral("SyncOnStart"),
                                                       false).toBool();
            const int every = stationSettings.value(QStringLiteral("AutoSyncMinutes"),
                                                    0).toInt();
            stationSettings.endGroup();
            if (onStart || every > 0) {
                StationSyncClient *client = stationSyncClient();
                if (onStart && client->paired()) {
                    // Not during the constructor: the library database and the
                    // rest of the window are not up yet, and the first thing a
                    // sync does is write to both.
                    QTimer::singleShot(15000, client, &StationSyncClient::sync);
                }
            }
        }

        // Production computers: the same window on both machines again, for
        // the same reason — the one on air lets a production machine in from
        // the top half, the production machine points itself at the station
        // from the bottom half.
        QAction *productionSync = new QAction(
            QIcon(":/icons/flat/Connection Sync-48.png"), tr("&Production Computers..."), this);
        productionSync->setMenuRole(QAction::NoRole);
        connect(productionSync, &QAction::triggered, this, [this]() {
            if (!m_productionSyncDialog) {
                m_productionSyncDialog = new ProductionSyncDialog(mobileSyncServer(),
                                                                  productionSyncClient(), this);
                m_productionSyncDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_productionSyncDialog, &ProductionSyncDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_productionSyncDialog->show();
            m_productionSyncDialog->raise();
            m_productionSyncDialog->activateWindow();
        });
        ui->menuXFB->addAction(productionSync);
        addAction(productionSync);
        AccessControl::instance().guard(productionSync, QStringLiteral("station.sync.production"));

        // A production machine with standing orders fetches without being
        // asked, the same way the backup does.
        {
            QSettings productionSettings(
                QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                    + "/xfb.conf", QSettings::IniFormat);
            productionSettings.beginGroup(QStringLiteral("ProductionSync"));
            const bool onStart = productionSettings.value(QStringLiteral("SyncOnStart"),
                                                          false).toBool();
            const int every = productionSettings.value(QStringLiteral("AutoSyncMinutes"),
                                                       0).toInt();
            const bool alsoPublish =
                productionSettings.value(QStringLiteral("PublishAutomatically"), true).toBool();
            productionSettings.endGroup();
            if (onStart || every > 0) {
                ProductionSyncClient *client = productionSyncClient();
                if (onStart && client->paired()) {
                    // Same reasoning as the backup's delay: nothing this
                    // touches — the database, the tables on screen — is up yet.
                    QTimer::singleShot(20000, client, [client, alsoPublish]() {
                        if (alsoPublish)
                            client->sync();
                        else
                            client->pull();
                    });
                }
            }
        }
    }
}

// Like the phone sync server, the backup client is built on demand and does
// nothing at all until it has been paired with a station.
namespace {

/**
 * Watches the application for signs of life, so the desk can lock itself
 * after a quiet spell.
 *
 * It is its own object rather than the window's event filter because it wants
 * every event the application sees, and player::eventFilter is written around
 * the handful of widgets it was installed on. All it does is notice input and
 * restart a timer; nothing here can swallow an event.
 */
class DeskIdleWatcher : public QObject
{
public:
    DeskIdleWatcher(QTimer *timer, QObject *parent)
        : QObject(parent), m_timer(timer)
    {
        qApp->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
            if (m_timer && m_timer->isActive())
                m_timer->start(); // restart from now
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QPointer<QTimer> m_timer;
};

} // namespace

void player::setupAccessControl()
{
    AccessControl &access = AccessControl::instance();

    // The menu entries that come from player.ui. The runtime-built ones are
    // bound where they are created, so the permission sits next to the entry
    // rather than in a table somebody has to remember to update.
    const struct { QAction *action; const char *permission; } bindings[] = {
        {ui->actionOpen,                      "playlist.open"},
        {ui->actionSave_Playlist,             "playlist.save"},
        {ui->actionLoad_Playlist,             "playlist.load"},
        {ui->actionClear_Playlist,            "playlist.clear"},
        {ui->actionAdd_a_single_song,         "library.add.single"},
        {ui->actionAdd_all_songs_in_a_folder, "library.add.folder"},
        {ui->actionAdd_Jingle,                "library.add.jingle"},
        {ui->actionAdd_a_publicity,           "library.add.publicity"},
        {ui->actionAdd_a_program,             "library.add.program"},
        {ui->actionManage_Genres,             "library.genres"},
        {ui->actionCheck_the_Database_records, "library.check"},
        {ui->actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation,
                                              "library.purge"},
        {ui->actionRemove_duplicate_songs,    "library.duplicates"},
        {ui->actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database,
                                              "library.autotrim"},
        {ui->actionConvert_all_musics_in_the_database_to_mp3, "library.convert"},
        {ui->actionConvert_all_musics_in_the_database_to_ogg, "library.convert"},
        {ui->actionConvert_all_musics_in_the_database_to_opus, "library.convert"},
        {ui->actionFullScreen,                "station.fullscreen"},
        {ui->actionOptions,                   "station.options"},
        {ui->actionRecord_a_new_Program,      "programming.record"},
        {ui->actionMake_a_program_from_this_playlist, "programming.makeprogram"},
        {ui->actionForce_an_FTP_Check,        "station.server.ftp"},
        {ui->actionForce_monitorization,      "station.server.monitor"},
        {ui->actionUpdate_Dinamic_Server_s_IP, "station.server.ip"},
        {ui->actionUpdate_System,             "station.update"},
        {ui->actionInstall_all_dependencies,  "station.dependencies"},
    };
    for (const auto &binding : bindings)
        access.guard(binding.action, QLatin1String(binding.permission));

    // The downloader is not merely allowed, it is switched on: no role has it
    // until an administrator ticks "Allow adding sources from external
    // sources", and a station that never takes music off the internet should
    // not carry the entry at all. Hidden rather than greyed out, therefore.
    access.guard(ui->actionAdd_a_song_from_Youtube_or_Other,
                 QStringLiteral("downloads.external"), AccessControl::WhenDenied::Hide);

    // Help and the accessibility preferences are deliberately not in that
    // list. A station that can lock somebody out of the screen reader
    // settings has built a trap, not a safeguard.

    if (ui->menuXFB) {
        ui->menuXFB->addSeparator();

        QAction *usersRoles = new QAction(QIcon(":/icons/flat/Security Checked-48.png"),
                                          tr("&Users and roles..."), this);
        usersRoles->setMenuRole(QAction::NoRole);
        usersRoles->setToolTip(tr("Who may sign in to this XFB, and what each of "
                                  "them is allowed to do with it."));
        connect(usersRoles, &QAction::triggered, this, [this]() {
            if (!m_usersRolesDialog) {
                m_usersRolesDialog = new UsersRolesDialog(this);
                m_usersRolesDialog->setAttribute(Qt::WA_DeleteOnClose, false);
                connect(m_usersRolesDialog, &UsersRolesDialog::announcementRequested,
                        this, &player::announceAccessible);
            }
            m_usersRolesDialog->show();
            m_usersRolesDialog->raise();
            m_usersRolesDialog->activateWindow();
        });
        ui->menuXFB->addAction(usersRoles);
        addAction(usersRoles);
        // Not guarded: on an unprotected installation this is the only way in,
        // and once there are accounts it refuses on its own — see below.
        connect(&access, &AccessControl::sessionChanged, usersRoles, [usersRoles]() {
            const AccessControl &control = AccessControl::instance();
            usersRoles->setEnabled(!control.isProtected() || control.isAdministrator());
        });
        usersRoles->setEnabled(!access.isProtected() || access.isAdministrator());

        m_lockDeskAction = new QAction(QIcon(":/icons/lock.png"),
                                       tr("&Lock the desk"), this);
        m_lockDeskAction->setMenuRole(QAction::NoRole);
        m_lockDeskAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
        m_lockDeskAction->setShortcutContext(Qt::ApplicationShortcut);
        m_lockDeskAction->setToolTip(tr("Ask for a password before the desk can be "
                                        "touched again. Whatever is on air stays "
                                        "on air."));
        connect(m_lockDeskAction, &QAction::triggered, this, &player::lockDesk);
        ui->menuXFB->addAction(m_lockDeskAction);
        addAction(m_lockDeskAction);
    }

    connect(&access, &AccessControl::sessionChanged, this, [this]() {
        refreshOperatorInTitle();
        applyAccessToControls();
    });

    // The desk locks itself only when an administrator asked it to, and the
    // timer is rebuilt whenever that setting changes.
    m_idleLockTimer = new QTimer(this);
    m_idleLockTimer->setSingleShot(true);
    connect(m_idleLockTimer, &QTimer::timeout, this, [this]() {
        if (AccessControl::instance().autoLockMinutes() > 0)
            lockDesk();
    });
    new DeskIdleWatcher(m_idleLockTimer, this);

    auto armIdleLock = [this]() {
        const int minutes = AccessControl::instance().autoLockMinutes();
        if (minutes > 0)
            m_idleLockTimer->start(minutes * 60 * 1000);
        else
            m_idleLockTimer->stop();
    };
    connect(&access, &AccessControl::sessionChanged, this, armIdleLock);
    armIdleLock();

    refreshOperatorInTitle();
    applyAccessToControls();
}

void player::refreshOperatorInTitle()
{
    AccessControl &access = AccessControl::instance();

    // The title is where an operator glances to be sure they are not about to
    // work as somebody else — which matters most on the machine three people
    // share across a day.
    QString title = QStringLiteral("XFB");
    if (access.isProtected()) {
        const AccessControl::User user = access.currentUser();
        const QString who = user.displayName.isEmpty() ? user.username : user.displayName;
        if (who.isEmpty())
            title += tr(" — nobody signed in");
        else
            title += QStringLiteral(" — %1 (%2)").arg(who, access.currentRoleName());
    }
    setWindowTitle(title);
}

void player::applyTorrentTabVisibility()
{
    if (!ui || !ui->pubWidget)
        return;

    // Two switches, both of which have to be on: the station has enabled the
    // feature in Options, and the role at the desk supports .torrent files.
    // Signing in as somebody without the permission takes the tab away for as
    // long as they are there, and stops whatever it was doing.
    QString configFilePath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                             + "/xfb.conf";
    QSettings settings(configFilePath, QSettings::IniFormat);
    const bool wanted = settings.value("EnableTorrents", false).toBool()
                        && AccessControl::instance().allows(QStringLiteral("downloads.torrents"));

    const int torrentsTabIndex = ui->pubWidget->indexOf(ui->tabTorrents);
    // The one-time privacy disclosure is handled where the user actually turns
    // the feature on (optionsDialog::on_checkBox_enableTorrents_clicked), not
    // here — this runs on every startup and must never prompt.
    if (wanted) {
        // Bring the services up on demand (idempotent). ensureTorrentServices
        // is what actually creates them — the tab is just the entry point.
        ensureTorrentServices();
        // Re-add the tab if it was previously removed
        if (torrentsTabIndex == -1) {
            ui->pubWidget->addTab(ui->tabTorrents,
                QIcon(":/icons/flat/pirate-32.png"), tr("Torrents"));
        }
    } else {
        // Turning the feature off is a real kill-switch: stop any Tor
        // connection and running downloads before hiding the tab.
        shutdownTorrentActivity();
        // Remove the tab (widget is not deleted, just hidden from the tab bar)
        if (torrentsTabIndex != -1) {
            ui->pubWidget->removeTab(torrentsTabIndex);
        }
    }
}

void player::applyAccessToControls()
{
    // Everything else this feature touches is a menu entry, which guard()
    // takes care of. These are the controls an operator actually uses, and
    // leaving them live would make the menu entry beside them a decoration.
    const bool mayDrive = AccessControl::instance().allows(QStringLiteral("playback.transport"));
    for (QWidget *button : {static_cast<QWidget *>(ui->btPlay),
                            static_cast<QWidget *>(ui->btStop),
                            static_cast<QWidget *>(ui->bt_pause_play),
                            static_cast<QWidget *>(ui->btPlayNext)}) {
        if (button)
            button->setEnabled(mayDrive);
    }

    const bool mayAutoMix = AccessControl::instance().allows(QStringLiteral("playback.automix"));
    if (ui->bt_autoMode)
        ui->bt_autoMode->setEnabled(mayAutoMix);

    // A whole tab rather than one button, and hidden rather than disabled: see
    // the note on WhenDenied::Hide.
    applyTorrentTabVisibility();
}

void player::lockDesk()
{
    AccessControl &access = AccessControl::instance();
    if (!access.isProtected()) {
        QMessageBox::information(
            this, tr("Nothing to lock"),
            tr("This installation has no accounts, so there is no password to ask "
               "for. Create one in Options ▸ Users and roles."));
        return;
    }
    if (m_deskLocked)
        return;

    m_deskLocked = true;
    m_idleLockTimer->stop();

    // The session goes before the question is asked: if XFB is killed while
    // the lock is up, it does not come back signed in as whoever left.
    access.signOut();
    announceAccessible(tr("The desk is locked. The station is still on air."));

    if (!SignInDialog::ask(SignInDialog::Mode::Lock, this)) {
        // Quit was chosen, and the question about going off air has already
        // been asked and answered inside the dialog.
        m_deskLocked = false;
        close();
        return;
    }

    m_deskLocked = false;
    announceAccessible(tr("Signed in as %1.").arg(access.currentUser().displayName));
}

StationSyncClient *player::stationSyncClient()
{
    if (m_stationSync)
        return m_stationSync;

    m_stationSync = new StationSyncClient(this);
    connect(m_stationSync, &StationSyncClient::finished, this,
            [this](const QString &summary) {
                ui->statusBar->showMessage(tr("Station backup: %1").arg(summary), 10000);
            });
    connect(m_stationSync, &StationSyncClient::failed, this,
            [this](const QString &reason) {
                ui->statusBar->showMessage(tr("Station backup: %1").arg(reason), 10000);
                qWarning() << "StationSyncClient:" << reason;
            });

    // The backup side of the dead-air story. The studio publishes a
    // heartbeat; this machine listens, and when the studio has been dark or
    // unreachable for the configured period it says so as loudly as a desktop
    // application can. It stops there on purpose — see monitorTakeOver() for
    // the manual flow that actually puts a machine on air, which stays a
    // decision a person makes.
    connect(m_stationSync, &StationSyncClient::studioWentDark, this,
            [this](const QString &reason) {
                qCritical() << "STUDIO DARK:" << reason;
                ui->statusBar->showMessage(tr("STUDIO DARK — %1").arg(reason), 0);
                announceAccessible(tr("Studio dark. %1").arg(reason));
                showAirAlert(tr("The studio is dark"),
                             tr("%1\n\nThis machine is the backup. Nothing has "
                                "been put on air automatically — check the "
                                "studio, and use Take Over when you have "
                                "decided to.").arg(reason),
                             true);
            });
    connect(m_stationSync, &StationSyncClient::studioCameBack, this,
            [this](const QString &detail) {
                qInfo() << "Studio back:" << detail;
                ui->statusBar->showMessage(detail, 15000);
                announceAccessible(detail);
            });
    return m_stationSync;
}

// Built on demand like the others, and equally inert until it has been paired
// with a station to produce for.
ProductionSyncClient *player::productionSyncClient()
{
    if (m_productionSync)
        return m_productionSync;

    m_productionSync = new ProductionSyncClient(this);
    connect(m_productionSync, &ProductionSyncClient::finished, this,
            [this](const QString &summary) {
                ui->statusBar->showMessage(tr("Production sync: %1").arg(summary), 10000);
                // A fetch writes rows straight into the library, so what is on
                // screen is out of date until the tables are read again.
                update_music_table();
            });
    connect(m_productionSync, &ProductionSyncClient::failed, this,
            [this](const QString &reason) {
                ui->statusBar->showMessage(tr("Production sync: %1").arg(reason), 10000);
                qWarning() << "ProductionSyncClient:" << reason;
            });
    return m_productionSync;
}

// ---------------------------------------------------------------- dead air

// Off by default, and this is the one place that decision is worth arguing.
// A watchdog that starts rescuing a station nobody has configured a fallback
// for would interrupt a working installation the first time somebody paused
// playback to answer the phone, and would start unrelated audio on a machine
// whose operator never asked for any. The failure it guards against is rare
// and catastrophic; the false positive is common and immediate. So it stays
// off until the operator has been through the dialog, seen the threshold, and
// pointed it at material they are willing to hear go out.
LibraryWatcher *player::libraryWatcher()
{
    if (m_libraryWatcher)
        return m_libraryWatcher;

    m_libraryWatcher = new LibraryWatcher(this);

    // The library views are showing rows that no longer say everything the
    // database does, so they are rebuilt — and said out loud, because an
    // import that happens on its own is exactly the kind of change a screen
    // reader user would otherwise never learn about.
    connect(m_libraryWatcher, &LibraryWatcher::imported, this,
            [this](int count, const QString &what) {
        Q_UNUSED(count)
        update_music_table();
        announceAccessible(what);
    });

    connect(m_libraryWatcher, &LibraryWatcher::logMessage, this,
            [](const QString &message) {
        qInfo() << "[WatchedFolders]" << message;
    });

    return m_libraryWatcher;
}

DeadAirWatchdog *player::deadAirWatchdog()
{
    if (m_deadAirWatchdog)
        return m_deadAirWatchdog;

    m_deadAirWatchdog = new DeadAirWatchdog(this);

    // The level meter's own feed: post-DSP master peaks, the same numbers the
    // LEDs draw. Emitted only while the FX engine is the active path, which
    // the watchdog knows and accounts for.
    if (Xplayer) {
        connect(Xplayer, &FxPlayer::levels,
                m_deadAirWatchdog, &DeadAirWatchdog::noteLevels);
    }

    // Everything else the watchdog needs is a poll of the transport, once a
    // second, on the main thread — cheap, and it cannot perturb the playback
    // state machine the way hooking playbackStateChanged would.
    m_deadAirFeedTimer = new QTimer(this);
    m_deadAirFeedTimer->setInterval(1000);
    connect(m_deadAirFeedTimer, &QTimer::timeout, this, [this]() {
        if (!m_deadAirWatchdog || !Xplayer)
            return;
        const bool playing =
            Xplayer->playbackState() == QMediaPlayer::PlayingState;
        const bool haveProgramme = ui->playlist->count() > 0
                                   || !Xplayer->source().isEmpty()
                                   || PlayMode == "Playing_Segue"
                                   || autoMode == 1;
        m_deadAirWatchdog->noteTransport(playing, Xplayer->position(),
                                         haveProgramme,
                                         Xplayer->fxEngineActive());
    });
    m_deadAirFeedTimer->start();

    connect(m_deadAirWatchdog, &DeadAirWatchdog::tripped, this,
            [this](DeadAirWatchdog::Reason reason, const QString &detail) {
        Q_UNUSED(reason)
        QString what;
        const bool ok = startDeadAirFallback(&what);
        m_deadAirWatchdog->noteFallbackStarted(ok, what);

        // A paired phone cannot be called — the sync server is pull-only — so
        // the incident is published where one can find it.
        // mobileSyncServer() only constructs the object; it does not open a
        // port. Recording the incident now means it is already there if the
        // operator starts the sync server later in the same outage.
        if (m_deadAirWatchdog->config().notifyPhone) {
            mobileSyncServer()->postIncident(
                QStringLiteral("deadair"),
                QStringLiteral("trip"),
                ok ? tr("%1 — fallback started: %2").arg(detail, what)
                   : tr("%1 — FALLBACK FAILED: %2").arg(detail, what));
        }
    });

    connect(m_deadAirWatchdog, &DeadAirWatchdog::recovered, this,
            [this](const QString &detail) {
        if (m_mobileSyncServer)
            m_mobileSyncServer->resolveIncident(QStringLiteral("deadair"), detail);
    });

    // One line per event in xfb.log, next to xfb.conf, plus the status bar.
    connect(m_deadAirWatchdog, &DeadAirWatchdog::logMessage, this,
            [this](const QString &message) {
        qWarning().noquote() << "Dead air:" << message;
        ui->statusBar->showMessage(message, 20000);
        announceAccessible(message);
    });

    connect(m_deadAirWatchdog, &DeadAirWatchdog::alert, this,
            &player::showAirAlert);

    return m_deadAirWatchdog;
}

// The rescue itself. The running order is a QListWidget of absolute paths and
// playNextSong() consumes row 0, so putting the fallback on air is a matter of
// inserting it at the top and asking for the next song — no new playback path,
// nothing for the m_manualAdvancing state machine to trip over.
bool player::startDeadAirFallback(QString *what)
{
    const DeadAirWatchdog::Config config =
        m_deadAirWatchdog ? m_deadAirWatchdog->config()
                          : DeadAirWatchdog::loadConfig();

    QStringList tracks = DeadAirWatchdog::resolveFallbackTracks(config);

    QString source;
    if (!tracks.isEmpty()) {
        source = !config.fallbackPlaylist.isEmpty()
                     && !DeadAirWatchdog::tracksFromPlaylistFile(
                            config.fallbackPlaylist).isEmpty()
                     ? QFileInfo(config.fallbackPlaylist).fileName()
                     : config.fallbackFolder;
    } else if (autoMode == 1) {
        // No fallback configured, but Auto Mode can pick from the library —
        // which beats silence by a distance.
        const int before = ui->playlist->count();
        autoModeGetMoreSongs();
        if (ui->playlist->count() > before) {
            if (what)
                *what = tr("Auto Mode picked %1 track(s) from the library")
                            .arg(ui->playlist->count() - before);
            PlayMode = "Playing_Segue";
            playNextSong();
            return true;
        }
    }

    if (tracks.isEmpty()) {
        if (what)
            *what = tr("no fallback playlist or folder is configured, and "
                       "nothing else could be found to play");
        return false;
    }

    // In at the top, in order, so row 0 is the first fallback track.
    for (int i = tracks.size() - 1; i >= 0; --i)
        ui->playlist->insertItem(0, tracks.at(i));
    calculate_playlist_total_time();

    // A stalled engine has to be let go of before anything else will come out
    // of it — the same release the per-track stall recovery does.
    m_manualAdvancing = true;
    Xplayer->stop();
    Xplayer->setSource(QUrl());
    Xplayer->resetAudioSink();
    m_manualAdvancing = false;

    // playNextSong() refuses to replay what it thinks is already on, and
    // after an outage that memory is exactly wrong.
    lastPlayedSong.clear();
    PlayMode = "Playing_Segue";
    if (darkMode)
        ui->btPlay->setStyleSheet(kPlayingGreenDarkStyle);
    else
        ui->btPlay->setStyleSheet(kPlayingGreenLightStyle);
    ui->btPlay->setText(tr("Play and Segue"));

    playNextSong();

    const bool started = Xplayer->playbackState() == QMediaPlayer::PlayingState;
    if (what) {
        *what = started
            ? tr("%n track(s) from %1", "", tracks.size())
                  .arg(source.isEmpty() ? tr("the fallback material") : source)
            : tr("queued %n track(s) but the player did not start", "",
                 tracks.size());
    }
    return started;
}

// Modeless on purpose: a modal box would stop the event loop that is trying to
// get the station back on air. Only one is kept up at a time — an outage that
// lasts an hour must not bury the screen in windows.
void player::showAirAlert(const QString &title, const QString &message,
                          bool critical)
{
    if (m_airAlertBox) {
        m_airAlertBox->setText(message);
        m_airAlertBox->setWindowTitle(title);
        m_airAlertBox->raise();
        return;
    }

    auto *box = new QMessageBox(critical ? QMessageBox::Critical
                                         : QMessageBox::Information,
                                title, message, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose, true);
    box->setModal(false);
    box->setWindowModality(Qt::NonModal);
    m_airAlertBox = box;
    box->show();
    box->raise();
    box->activateWindow();
}

// What a backup station is told when it asks whether this one is still on air.
QJsonObject player::stationHeartbeatState() const
{
    QJsonObject state;

    const bool playing = Xplayer
        && Xplayer->playbackState() == QMediaPlayer::PlayingState;

    QString word = playing ? QStringLiteral("playing") : QStringLiteral("silent");
    if (!playing && ui->playlist->count() == 0 && PlayMode == "stopped")
        word = QStringLiteral("off");

    if (m_deadAirWatchdog) {
        switch (m_deadAirWatchdog->airState()) {
        case DeadAirWatchdog::AirState::Off:      word = QStringLiteral("off"); break;
        case DeadAirWatchdog::AirState::Playing:  word = QStringLiteral("playing"); break;
        case DeadAirWatchdog::AirState::Silent:   word = QStringLiteral("silent"); break;
        case DeadAirWatchdog::AirState::Stalled:  word = QStringLiteral("stalled"); break;
        case DeadAirWatchdog::AirState::Fallback: word = QStringLiteral("fallback"); break;
        }
        state.insert(QStringLiteral("watchdog"), true);
        state.insert(QStringLiteral("watchdogEnabled"),
                     m_deadAirWatchdog->config().enabled);
        state.insert(QStringLiteral("levelDb"), m_deadAirWatchdog->lastLevelDb());
    } else {
        state.insert(QStringLiteral("watchdog"), false);
    }

    state.insert(QStringLiteral("state"), word);
    state.insert(QStringLiteral("track"), QFileInfo(lastPlayedSong).fileName());
    state.insert(QStringLiteral("path"), lastPlayedSong);
    state.insert(QStringLiteral("positionMs"), Xplayer ? Xplayer->position() : -1);
    state.insert(QStringLiteral("durationMs"), Xplayer ? Xplayer->duration() : -1);
    state.insert(QStringLiteral("playlistCount"), ui->playlist->count());
    state.insert(QStringLiteral("autoMode"), autoMode == 1);
    state.insert(QStringLiteral("playMode"), PlayMode);
    state.insert(QStringLiteral("uptime"),
                 m_processUptime.isValid() ? m_processUptime.elapsed() / 1000 : 0);
    return state;
}

// Everything the public listener page is told, and nothing else. There is no
// path here, no playlist depth, no Auto Mode flag and no watchdog state —
// compare stationHeartbeatState() above, which has all four and is only ever
// answered to a paired backup.
MobileSyncServer::NowPlaying player::publicNowPlaying()
{
    MobileSyncServer::NowPlaying now;

    now.onAir = Xplayer
        && Xplayer->playbackState() == QMediaPlayer::PlayingState
        && !lastPlayedSong.isEmpty();
    if (!now.onAir)
        return now;

    now.positionMs = Xplayer->position();
    now.durationMs = Xplayer->duration();

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (db.isOpen()) {
        QSqlQuery lookup(db);
        lookup.prepare(QStringLiteral(
            "SELECT artist, song FROM musics WHERE path = :path"));
        lookup.bindValue(QStringLiteral(":path"), lastPlayedSong);
        if (lookup.exec() && lookup.next()) {
            now.artist = lookup.value(0).toString().trimmed();
            now.title  = lookup.value(1).toString().trimmed();
        }
    }
    if (now.title.isEmpty()) {
        // "Artist - Title.mp3" is how most libraries on disk are named, and
        // splitting it beats putting a file name in front of listeners.
        const QString base = QFileInfo(lastPlayedSong).completeBaseName();
        const int dash = base.indexOf(QStringLiteral(" - "));
        if (now.artist.isEmpty() && dash > 0) {
            now.artist = base.left(dash).trimmed();
            now.title  = base.mid(dash + 3).trimmed();
        } else {
            now.title = base;
        }
    }

    // The cover, re-encoded small. The page never opens a file, so the only
    // way a picture reaches it is as bytes handed over here — and these are a
    // fresh JPEG made from the decoded image, not a copy of anything on disk.
    // Encoded once per track and then held, because this runs on every poll.
    if (m_publicArtPath != lastPlayedSong) {
        m_publicArtPath = lastPlayedSong;
        m_publicArtJpeg.clear();
        m_publicArtKey.clear();

        if (m_artStore) {
            const ArtworkData *art = m_artStore->fetch(lastPlayedSong);
            if (art && art->ready()) {
                const QImage image = art->pixmap.toImage().scaled(
                    220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QBuffer buffer(&m_publicArtJpeg);
                if (buffer.open(QIODevice::WriteOnly)
                    && image.save(&buffer, "JPEG", 72)) {
                    buffer.close();
                    m_publicArtKey = QString::fromLatin1(
                        QCryptographicHash::hash(m_publicArtJpeg,
                                                 QCryptographicHash::Sha256)
                            .toHex().left(16));
                } else {
                    m_publicArtJpeg.clear();
                }
            }
        }
    }
    now.artworkJpeg = m_publicArtJpeg;
    now.artworkKey  = m_publicArtKey;

    return now;
}

// Created on demand and inert until told to start: an operator who never
// opens the dialog gets exactly the behaviour XFB had before this existed.
StreamService *player::streamService()
{
    if (m_streamService)
        return m_streamService;

    m_streamService = new StreamService(this);

    // The tap lives in the main playlist player — that is what is on air.
    // Arming it is what makes FxPlayer route through the FX engine, so it
    // is armed only while the service is actually streaming.
    connect(m_streamService, &StreamService::activeChanged, this, [this](bool active) {
        if (Xplayer)
            Xplayer->setPcmTapEnabled(active);
        ui->statusBar->showMessage(active ? tr("Streaming: on air")
                                          : tr("Streaming: off air"), 10000);
        if (active) {
            // Whatever is playing right now, so a mount that comes up
            // mid-track still carries a title.
            updateStreamNowPlaying(lastPlayedSong);
        }
    });

    if (Xplayer) {
        connect(Xplayer, &FxPlayer::pcmTap,
                m_streamService, &StreamService::feedPcm);
    }

    connect(m_streamService, &StreamService::logMessage, this,
            [](const QString &message) { qInfo() << "Stream:" << message; });

    return m_streamService;
}

// Icecast carries the title out of band, so it has to be pushed as the track
// changes. The library knows the real artist and song; the file name is the
// fallback, and for a lot of stations it is all there is.
void player::updateStreamNowPlaying(const QString &filePath)
{
    if (!m_streamService || !m_streamService->isActive() || filePath.isEmpty())
        return;

    QString artist;
    QString title;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (db.isOpen()) {
        QSqlQuery lookup(db);
        lookup.prepare(QStringLiteral(
            "SELECT artist, song FROM musics WHERE path = :path"));
        lookup.bindValue(QStringLiteral(":path"), filePath);
        if (lookup.exec() && lookup.next()) {
            artist = lookup.value(0).toString().trimmed();
            title = lookup.value(1).toString().trimmed();
        }
    }

    if (title.isEmpty()) {
        // "Artist - Title.mp3" is how most libraries on disk are named, and
        // splitting it beats sending a file name to the listeners.
        const QString base = QFileInfo(filePath).completeBaseName();
        const int dash = base.indexOf(QStringLiteral(" - "));
        if (artist.isEmpty() && dash > 0) {
            artist = base.left(dash).trimmed();
            title = base.mid(dash + 3).trimmed();
        } else {
            title = base;
        }
    }

    m_streamService->setNowPlaying(artist, title);
}

// The sync server is created on demand and never listens until it is told to,
// so an operator who never opens the dialog has no port open.
MobileSyncServer *player::mobileSyncServer()
{
    if (m_mobileSyncServer)
        return m_mobileSyncServer;

    m_mobileSyncServer = new MobileSyncServer(this);

    // The heartbeat a backup station polls. Same arrangement as the playlist
    // provider below: only the player knows what is on air, and the server has
    // no business reaching into it.
    m_mobileSyncServer->setStationStateProvider(
        [this]() { return stationHeartbeatState(); });

    // What the public page is allowed to say. A provider of its own rather
    // than a slice of the heartbeat above: the heartbeat carries the file
    // path, and the whole point of keeping these apart is that a field added
    // there can never quietly turn up on a page served to the street.
    m_mobileSyncServer->setNowPlayingProvider(
        [this]() { return publicNowPlaying(); });

    // Only the player can read the playlist that is loaded right now, and that
    // is the one an operator most often wants to carry out of the studio.
    m_mobileSyncServer->setLivePlaylistProvider([this]() {
        QVector<MobileSyncServer::Track> tracks;
        if (!ui || !ui->playlist)
            return tracks;

        // The library lives on the named connection. This used to ask for the
        // default one, which XFB never opens, so every lookup failed silently
        // and the phone was handed file base names instead of artist, song and
        // duration for every track in the live playlist.
        QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
        QSqlQuery lookup(db);
        const bool haveDb = db.isOpen() && lookup.prepare(
            QStringLiteral("SELECT artist, song, time FROM musics WHERE path = :path"));

        const int count = ui->playlist->count();
        for (int row = 0; row < count; ++row) {
            QListWidgetItem *item = ui->playlist->item(row);
            if (!item)
                continue;

            MobileSyncServer::Track track;
            track.path = item->text();
            if (track.path.isEmpty())
                continue;

            track.id = MobileSyncServer::idForPath(track.path);
            track.overlapMs =
                item->data(PlaylistWaveView::OverlapRole).toLongLong();
            track.volumeEnvelope =
                item->data(PlaylistWaveView::VolumeEnvelopeRole).toString();

            const QFileInfo info(track.path);
            track.bytes = info.size();
            track.song = info.completeBaseName();

            if (haveDb) {
                lookup.bindValue(QStringLiteral(":path"), track.path);
                if (lookup.exec() && lookup.next()) {
                    const QString artist = lookup.value(0).toString();
                    const QString song = lookup.value(1).toString();
                    if (!artist.isEmpty()) track.artist = artist;
                    if (!song.isEmpty())   track.song = song;
                    track.duration = lookup.value(2).toString();
                }
            }

            tracks << track;
        }
        return tracks;
    });

    connect(m_mobileSyncServer, &MobileSyncServer::errorOccurred, this,
            [this](const QString &message) {
                ui->statusBar->showMessage(tr("Phone sync: %1").arg(message), 8000);
                qWarning() << "MobileSyncServer:" << message;
            });
    // A production computer has just added something to this station's library.
    // Auto Mode needs no telling — every pick is a fresh query, so the next one
    // can already choose what has just arrived — but the tables on screen are
    // showing what the library held a moment ago.
    connect(m_mobileSyncServer, &MobileSyncServer::catalogueChangedByPeer, this,
            [this](const QString &device, const QString &summary) {
                ui->statusBar->showMessage(tr("%1 published %2").arg(device, summary), 10000);
                announceAccessible(tr("%1 published %2 to this station").arg(device, summary));
                if (m_peerCatalogueRefreshQueued)
                    return;
                m_peerCatalogueRefreshQueued = true;
                QTimer::singleShot(2000, this, [this]() {
                    m_peerCatalogueRefreshQueued = false;
                    update_music_table();
                });
            });

    connect(m_mobileSyncServer, &MobileSyncServer::devicePaired, this,
            [this](const QString &name) {
                ui->statusBar->showMessage(tr("%1 paired with XFB").arg(name), 6000);
            });

    QSettings settings;
    if (settings.value(QStringLiteral("MobileSync/AutoStart"), false).toBool())
        m_mobileSyncServer->start();

    return m_mobileSyncServer;
}

QStringList player::allListedMusicPaths() const
{
    QStringList paths;
    QAbstractItemModel *model = ui->musicView ? ui->musicView->model() : nullptr;
    if (!model)
        return paths;

    // The music model loads rows as the view scrolls, so without this an
    // unscrolled list would only ever offer its first screenful.
    while (model->canFetchMore(QModelIndex()))
        model->fetchMore(QModelIndex());

    const int rows = model->rowCount();
    paths.reserve(rows);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex pathIdx = model->index(row, 7);   // musics.path
        if (!pathIdx.isValid())
            continue;
        const QString path = model->data(pathIdx).toString();
        if (!path.isEmpty())
            paths << path;
    }
    return paths;
}

void player::markForPhone(const QStringList &paths, bool confirmFirst)
{
    if (paths.isEmpty()) {
        ui->statusBar->showMessage(tr("There are no tracks to send to the phone."), 6000);
        return;
    }

    if (confirmFirst) {
        // Sending a whole library over Wi-Fi is worth a moment's thought, so
        // the size is shown before anything is marked.
        qint64 bytes = 0;
        int missing = 0;
        for (const QString &path : paths) {
            const QFileInfo info(path);
            if (info.exists())
                bytes += info.size();
            else
                ++missing;
        }

        QString question = tr("Mark %1 tracks (%2) for the phone to download?")
                               .arg(paths.size())
                               .arg(QLocale().formattedDataSize(bytes));
        if (missing > 0)
            question += tr("\n\n%1 of them are missing from disk and will be skipped.").arg(missing);

        if (QMessageBox::question(this, tr("Sync to Phone"), question,
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
    }

    MobileSyncServer *server = mobileSyncServer();
    const int added = server->addToSyncSet(paths);
    const int total = server->syncSet().size();

    QString message = (added == 0)
        ? tr("Already marked for the phone; %n track(s) waiting.", "", total)
        : tr("%n track(s) marked for the phone.", "", added);

    // Marking is useless if nothing is listening, and asking to sync is a clear
    // enough instruction to start serving on the operator's behalf.
    if (!server->isListening()) {
        if (server->start()) {
            message += tr(" Now serving on port %1 — open XFB on the phone and "
                          "download \"Marked for this phone\".").arg(server->port());
        } else {
            message += tr(" Could not start serving; check Options > Sync to Phone.");
        }
    } else {
        message += tr(" Open XFB on the phone and download \"Marked for this phone\".");
    }

    ui->statusBar->showMessage(message, 12000);
    announceAccessible(message);
}

// "00:02:35" is read out by a screen reader as a run of digits, which is slow
// to take in and easy to mishear while something is going to air. Words are
// unambiguous, and dropping the zero components keeps the message short.
QString player::spokenDuration(qint64 milliseconds) const
{
    const qint64 totalSeconds = (milliseconds + 500) / 1000; // nearest second
    const int hours   = int(totalSeconds / 3600);
    const int minutes = int((totalSeconds % 3600) / 60);
    const int seconds = int(totalSeconds % 60);

    QStringList parts;
    if (hours > 0)
        parts << tr("%n hour(s)", "spoken duration", hours);
    if (minutes > 0)
        parts << tr("%n minute(s)", "spoken duration", minutes);
    // Keep seconds when they are the only thing left to say, so a track with
    // 40 seconds to run does not announce an empty duration.
    if (seconds > 0 || parts.isEmpty())
        parts << tr("%n second(s)", "spoken duration", seconds);

    return parts.join(QLatin1Char(' '));
}

QString player::remainingTimeAnnouncement() const
{
    if (!Xplayer || Xplayer->playbackState() == QMediaPlayer::StoppedState)
        return tr("Nothing is playing");

    // Live streams, and formats whose length is not known until decoding has
    // run, report no duration — there is no time left to count down.
    if (trackTotalDuration <= 0)
        return tr("The length of this track is not known");

    const qint64 remaining = trackTotalDuration - Xplayer->position();
    if (remaining <= 0)
        return tr("Less than a second remaining");

    return tr("%1 remaining").arg(spokenDuration(remaining));
}

bool player::announcesRemainingWithNowPlaying() const
{
    auto *container = ServiceContainer::instance();
    auto *settings = container ? container->resolve<AccessibilitySettingsService>() : nullptr;
    // Default on when the service is unavailable, matching the stored default:
    // the time left is the more useful half of the announcement.
    return settings ? settings->settings().announceRemainingWithNowPlaying : true;
}

void player::announceAccessible(const QString &message)
{
    if (message.isEmpty())
        return;
    // Always surface it visually too — the status bar is useful for everyone.
    ui->statusBar->showMessage(message, 5000);

    // The private ear, when the operator asked for one. This is an *extra*
    // channel, never the only one: the screen reader still gets the message
    // below whether or not the cue device exists, and CueBus::speak() is a
    // no-op unless spoken cues are switched on and a cue device is live.
    if (m_cueBus)
        m_cueBus->speak(message);

    auto *container = ServiceContainer::instance();
    if (!container)
        return;
    if (auto *manager = container->resolve<AccessibilityManager>())
        manager->announceMessage(message, AccessibilityManager::Priority::Normal);
}

// The music quota window. Two ways in — the XFB menu and the music table's
// context menu — and both mean the same thing, so the window itself is built
// once and kept: the marking tab holds a selection and a half-typed search
// that reopening from scratch would throw away.
void player::openMusicQuota(const QList<qint64> &preselected)
{
    if (!m_quotaDialog) {
        m_quotaDialog = new QuotaDialog(this, preselected);
        m_quotaDialog->setAttribute(Qt::WA_DeleteOnClose, false);
        connect(m_quotaDialog, &QuotaDialog::announcementRequested,
                this, &player::announceAccessible);
    } else {
        m_quotaDialog->preselect(preselected);
    }
    m_quotaDialog->show();
    m_quotaDialog->raise();
    m_quotaDialog->activateWindow();
    announceAccessible(tr("Music quota opened"));
}

// ---------------------------------------------------------------------------
// Cue bus (pre-fade listen)
//
// Auditioning the next track while another one is on air needs a second
// output device, and the whole feature stands or falls on the audition never
// reaching the first one. That guarantee lives in CueBus/FxPlayer (see
// CueBus.h); what lives here is the operator's side of it: the settings, the
// menu entries, the shortcut and the on-screen indication of what is being
// cued.
// ---------------------------------------------------------------------------

void player::setupCueBus()
{
    if (m_cueBus)
        return;
    m_cueBus = new CueBus(this);

    // Permanent status-bar widget rather than a timed message: what is in the
    // headphones has to stay visible for as long as it is playing, and a
    // showMessage() would be wiped by the next status update.
    m_cueIndicator = new QLabel(this);
    m_cueIndicator->setObjectName(QStringLiteral("cueIndicator"));
    m_cueIndicator->setAccessibleName(tr("Cue monitor"));
    m_cueIndicator->setToolTip(tr("What is playing in the cue headphones. "
                                  "Cue audio never reaches the on-air output."));
    m_cueIndicator->hide();
    ui->statusBar->addPermanentWidget(m_cueIndicator);

    connect(m_cueBus, &CueBus::cueStateChanged, this,
            [this](bool cueing, const QString &label) {
        updateCueIndicator(cueing, label);
    });
    connect(m_cueBus, &CueBus::cueFailed, this, [this](const QString &reason) {
        // Refusals are the interesting case (no second device configured),
        // so they are spoken as well as shown.
        announceAccessible(reason);
        qWarning() << "Cue refused:" << reason;
    });

    updateCueIndicator(false, QString());
}

void player::applyOutputDeviceSettings()
{
    // On-air players. A device that has gone away falls back to the system
    // default inside FxPlayer, which logs it — silence on air is worse than
    // the wrong speaker.
    FxPlayer *const onAir[] = { Xplayer, lp1_Xplayer, lp2_Xplayer,
                                m_tailPlayer, RadioPlayer };
    for (FxPlayer *p : onAir) {
        if (p)
            p->setOutputDeviceId(m_mainOutputDeviceId);
    }
    // The pads are on air too.
    if (m_padBoard)
        m_padBoard->setOutputDeviceId(m_mainOutputDeviceId);

    if (m_cueBus) {
        m_cueBus->setVolume(m_cueVolume);
        m_cueBus->setMainDeviceId(m_mainOutputDeviceId);
        m_cueBus->setCueDeviceId(m_cueOutputDeviceId);
        m_cueBus->setSpeechEnabled(m_cueSpeakAnnouncements);
    }

    qInfo() << "Audio routing: on air ->"
            << AudioDeviceRouter::describe(m_mainOutputDeviceId)
            << "| cue ->" << AudioDeviceRouter::describe(m_cueOutputDeviceId);

    if (m_cueAction) {
        QString why;
        const bool ready = m_cueBus && m_cueBus->isAvailable(&why);
        m_cueAction->setEnabled(true); // still triggerable: it explains why not
        m_cueAction->setStatusTip(ready
            ? tr("Listen to the selected track in the cue headphones only")
            : why);
    }
}

void player::updateCueIndicator(bool cueing, const QString &label)
{
    if (!m_cueIndicator)
        return;
    if (cueing) {
        const QString text = tr("CUE: %1").arg(label);
        m_cueIndicator->setText(text);
        // Screen readers read the accessible name, not the styled text.
        m_cueIndicator->setAccessibleName(
            tr("Cue monitor: %1 is playing in the cue headphones").arg(label));
        m_cueIndicator->setStyleSheet(
            QStringLiteral("QLabel { padding: 1px 6px; border-radius: 3px; "
                           "background: #b8860b; color: white; font-weight: bold; }"));
        m_cueIndicator->show();
    } else {
        m_cueIndicator->setText(QString());
        m_cueIndicator->setAccessibleName(tr("Cue monitor: nothing is being cued"));
        m_cueIndicator->hide();
    }
    if (m_cueStopAction)
        m_cueStopAction->setEnabled(cueing);
}

void player::cueFile(const QString &path, const QString &label)
{
    if (!m_cueBus)
        return;
    if (path.trimmed().isEmpty()) {
        announceAccessible(tr("There is nothing to cue"));
        return;
    }
    const QUrl url = QUrl::fromLocalFile(path);
    const bool wasCueing = m_cueBus->isCueing() && m_cueBus->currentSource() == url;
    m_cueBus->toggleCue(url, label);
    if (wasCueing)
        announceAccessible(tr("Cue stopped"));
    else if (m_cueBus->isCueing())
        announceAccessible(tr("Cueing %1 in the headphones").arg(m_cueBus->currentLabel()));
    // A refusal has already been announced by the cueFailed connection.
}

void player::cueCurrentSelection()
{
    // The playlist and the library views are the two places an operator picks
    // a track from, so the shortcut follows the focus between them.
    if (ui->playlist && ui->playlist->hasFocus() && ui->playlist->currentItem()) {
        const QString path = ui->playlist->currentItem()->text();
        cueFile(path, QFileInfo(path).fileName());
        return;
    }

    QTableView *view = focusedLibraryView();
    if (!view || !view->selectionModel() || !view->model()) {
        announceAccessible(tr("Select a track in the library or the playlist first"));
        return;
    }
    const QModelIndex current = view->selectionModel()->currentIndex();
    if (!current.isValid()) {
        announceAccessible(tr("No track is selected"));
        return;
    }

    // The path column differs per view: musics keeps it at 7, the jingle,
    // advert and program tables at 1. Take the first column whose value looks
    // like an existing file rather than guessing per view.
    QString path;
    const int columns = view->model()->columnCount();
    for (int col = 0; col < columns && path.isEmpty(); ++col) {
        const QString value = view->model()->data(view->model()->index(current.row(), col))
                                  .toString();
        if (!value.isEmpty() && QFileInfo::exists(value))
            path = value;
    }
    if (path.isEmpty()) {
        announceAccessible(tr("No file path for the selected track"));
        return;
    }
    cueFile(path, QFileInfo(path).fileName());
}

// Playback and recording state used to be signalled only by a background
// colour (yellow pause button, red/green/yellow LED), which is invisible to a
// screen reader and ambiguous for colourblind users. Mirror it into the
// accessible name so the state is always readable as text.
void player::refreshTransportAccessibleState()
{
    if (ui->bt_pause_play) {
        ui->bt_pause_play->setAccessibleName(playPause ? tr("Resume (paused)")
                                                       : tr("Pause"));
        ui->bt_pause_play->setToolTip(playPause ? tr("Paused — click to resume")
                                                : tr("Pause the track that is playing"));
    }
    if (ui->bt_pause_rec) {
        ui->bt_pause_rec->setAccessibleName(recPause ? tr("Resume recording (paused)")
                                                     : tr("Pause recording"));
    }
    if (ui->lp_1_bt_pause)
        ui->lp_1_bt_pause->setAccessibleName(lp_1_paused ? tr("Deck 1: resume (paused)")
                                                         : tr("Deck 1: pause"));
    if (ui->lp_2_bt_pause)
        ui->lp_2_bt_pause->setAccessibleName(lp_2_paused ? tr("Deck 2: resume (paused)")
                                                         : tr("Deck 2: pause"));
}

// Replace the raw database column names shown in a table view's header
// ("published_date", "played_times", ...) with polished labels. Works for
// every model the views use: QSqlTableModel derives from QSqlQueryModel,
// and record().indexOf resolves the column regardless of query shape.
void player::applySqlHeaderLabels(QSqlQueryModel *model, const QList<QPair<QString, QString>> &labels) {
    if (!model)
        return;
    for (const auto &label : labels) {
        const int col = model->record().indexOf(label.first);
        if (col >= 0)
            model->setHeaderData(col, Qt::Horizontal, label.second);
    }
}

// The musics table is shown through several models (full table, search
// results, genre filters) — they all get the same labels.
void player::applyMusicHeaderLabels(QSqlQueryModel *model) {
    applySqlHeaderLabels(model, {
        {"artist",         tr("Artist")},
        {"song",           tr("Song")},
        {"genre1",         tr("Genre 1")},
        {"genre2",         tr("Genre 2")},
        {"country",        tr("Country")},
        {"published_date", tr("Published")},
        {"path",           tr("File")},
        {"time",           tr("Time")},
        {"played_times",   tr("Plays")},
        {"last_played",    tr("Last Played")},
        {"bpm",            tr("BPM")},
    });

    // Cells with no number say which of the two reasons applies rather than
    // sitting empty (see BpmCellDelegate). They stay editable either way, so
    // a tempo the detector will not commit to can be typed in by hand.
    const int bpmColumn = model->record().indexOf(QStringLiteral("bpm"));
    if (bpmColumn >= 0) {
        // Created on first use: this runs from the constructor's table setup
        // as well as from every later refresh.
        if (!m_bpmCellDelegate)
            m_bpmCellDelegate = new BpmCellDelegate(this);
        model->setHeaderData(bpmColumn, Qt::Horizontal,
                             tr("Measured tempo. \"no steady beat\" means the track was "
                                "analysed and holds no fixed tempo, which is usual for "
                                "freely-played solo, live and jazz recordings; those tracks "
                                "still play, they are just not tempo-matched. \"not "
                                "measured\" means it has not been analysed yet (Database "
                                "menu). Double-click a cell to set a tempo by hand."),
                             Qt::ToolTipRole);
        if (ui->musicView) {
            // The delegate is a property of the view and survives a model
            // swap; the column width does not. Every caller here installs
            // the labels *before* setModel(), and the header resets its
            // section sizes when a model arrives — so widening the column
            // has to wait until that has happened.
            ui->musicView->setItemDelegateForColumn(bpmColumn, m_bpmCellDelegate);
            QPointer<QTableView> view = ui->musicView;
            QTimer::singleShot(0, this, [view, bpmColumn]() {
                if (view && view->model() && bpmColumn < view->model()->columnCount()
                    && view->columnWidth(bpmColumn) < 130) {
                    view->setColumnWidth(bpmColumn, 130); // fits "no steady beat"
                }
            });
        }
    }
}

// One-time repair pass for library rows whose duration was never stored —
// the old add flows shelled out to a bare "exiftool", which fails silently
// when the tool is missing or (on macOS) not on the GUI app's minimal PATH,
// leaving the music list's time column empty. Runs shortly after startup and
// works through the rows one per timer tick so the UI stays responsive.
void player::startTimeBackfill() {
    if (!m_timeBackfillPending.isEmpty())
        return; // already running

    if (!MediaDuration::probeAvailable()) {
        qWarning() << "Time backfill skipped: neither exiftool nor ffmpeg is available.";
        return;
    }

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen())
        return;

    // Every valid stored time contains a colon; anything else (NULL, empty,
    // "-", the old parser's stray "s"/"(approx)" artifacts) needs a re-probe.
    QSqlQuery qry(db);
    if (!qry.exec("SELECT path FROM musics WHERE time IS NULL OR instr(time, ':') = 0")) {
        qWarning() << "Time backfill query failed:" << qry.lastError().text();
        return;
    }
    while (qry.next())
        m_timeBackfillPending << qry.value(0).toString();

    if (m_timeBackfillPending.isEmpty())
        return;

    qInfo() << "Backfilling missing track times for" << m_timeBackfillPending.size() << "library entries.";
    m_timeBackfillUpdated = 0;
    processNextTimeBackfill();
}

void player::processNextTimeBackfill() {
    if (m_timeBackfillPending.isEmpty()) {
        if (m_timeBackfillUpdated > 0) {
            qInfo() << "Time backfill finished:" << m_timeBackfillUpdated << "entries updated.";
            ui->statusBar->showMessage(tr("Updated the duration of %1 library tracks.").arg(m_timeBackfillUpdated), 8000);
            update_music_table();
        }
        return;
    }

    const QString path = m_timeBackfillPending.takeFirst();
    if (QFile::exists(path)) {
        const QString time = MediaDuration::forFile(path);
        if (!time.isEmpty()) {
            QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            QSqlQuery upd(db);
            upd.prepare("UPDATE musics SET time = :time WHERE path = :path");
            upd.bindValue(":time", time);
            upd.bindValue(":path", path);
            if (upd.exec())
                m_timeBackfillUpdated++;
            else
                qWarning() << "Time backfill update failed for" << path << ":" << upd.lastError().text();
        }
    }

    // Short breather between files so the probes never make the UI feel busy
    QTimer::singleShot(50, this, &player::processNextTimeBackfill);
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
        applyMusicHeaderLabels(model);
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
        applySqlHeaderLabels(jinglesmodel, {
            {"name", tr("Name")},
            {"path", tr("File")},
        });
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
        applySqlHeaderLabels(pubmodel, {
            {"name", tr("Name")},
            {"path", tr("File")},
        });
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
        applySqlHeaderLabels(programsmodel, {
            {"name", tr("Name")},
            {"path", tr("File")},
        });
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
        applySqlHeaderLabels(torrentsmodel, {
            {"name",       tr("Name")},
            {"path",       tr("File")},
            {"size",       tr("Size")},
            {"date_added", tr("Added")},
            {"status",     tr("Status")},
        });
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

        {
            const bool mayTorrent =
                AccessControl::instance().allows(QStringLiteral("downloads.torrents"));
            downloadAction->setEnabled(mayTorrent);
            downloadStreamAction->setEnabled(mayTorrent);
            copyMagnetAction->setEnabled(mayTorrent);
        }

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
                if (!confirmDownloadNetworkExposure()) {
                    return; // no VPN and the user chose not to proceed
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
        ui->bt_autoMode->setStyleSheet(kAutoModeOnStyle);
    } else {
        autoMode = 0;
        qDebug()<<"autoMode is OFF";
        ui->bt_autoMode->setStyleSheet("");
    }
}

// The track the next auto mode pick will follow: the tail of the running
// order, or — when nothing is queued — whatever is on air. Its tempo is
// what the crossfade into the new track has to work across.
// Auto Mode's memory of what it has put up lately. Bounded well above the
// window it is actually asked for, so raising AutoModeNoRepeatCount at runtime
// has something to look back on.
void player::rememberAutoModePick(const QString &path)
{
    if (path.isEmpty())
        return;
    m_recentAutoPicks.removeAll(path);
    m_recentAutoPicks.append(path);
    while (m_recentAutoPicks.size() > 500)
        m_recentAutoPicks.removeFirst();
}

QString player::autoModeReferenceTrack() const
{
    const int rows = ui->playlist->count();
    if (rows > 0)
        return ui->playlist->item(rows - 1)->text();
    if (Xplayer && Xplayer->source().isLocalFile())
        return Xplayer->source().toLocalFile();
    return lastPlayedSong;
}

bool player::autoModeGetMoreSongs()
{
    if (autoMode != 1)
        return false;

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


    // The hour clock, when the station has switched it on, says what kind of
    // music *this moment* of the hour wants — the sweep the clock is in, or
    // the next one due. It stands in for the hourgenre row and changes nothing
    // else: an empty answer (feature off, no clock on this hour, a sweep with
    // no genre of its own) leaves currentGenre exactly as the hour grid set
    // it, which is the old behaviour to the letter. A genre only ever narrows
    // passes 0 and 1 below; pass 2 is the whole library, so a clock can no
    // more empty the candidate pool than an hourgenre row can.
    const QString clockGenre = hourClockGenreNow();
    if (!clockGenre.isEmpty()) {
        qDebug() << "autoMode: the hour clock asks for" << clockGenre
                 << "in place of the hour grid's" << currentGenre;
        currentGenre = clockGenre;
    }

    // Tempo of the track the new one will follow. When it has never been
    // measured, the pick falls back to plain random and the measurement is
    // started in the background, so the following pick can use it.
    const QString referenceTrack = autoModeReferenceTrack();
    double referenceBpm = 0.0;
    if (m_bpmMatch && m_bpmLibrary) {
        referenceBpm = m_bpmLibrary->bpmFor(referenceTrack);
        if (referenceBpm <= 0.0)
            m_bpmLibrary->analyzeQuietly(referenceTrack);
    }

    // Three passes, each dropping one constraint: tempo-matched inside this
    // hour's genre, then anything in the genre, then anything at all. A
    // library where little has been analysed yet — or an hour whose
    // programmed genre holds nothing playable (the reference track may be the
    // only one in it) — must still keep the playlist fed, so a miss on an
    // earlier pass is not a failure. Only running out of library is.
    // The size of the library bounds the no-repeat window below.
    int libraryCount = 0;
    {
        QSqlQuery countQuery(db);
        if (countQuery.exec(QStringLiteral("select count(*) from musics"))
            && countQuery.next()) {
            libraryCount = countQuery.value(0).toInt();
        }
    }

    // What Auto Mode refuses to play again. Remembering only the track just
    // played is not enough: whenever the pool it draws from is narrow — an
    // hour's programmed genre, or the handful of tracks that happen to have a
    // measured BPM — two or three titles ping-pong for hours. The window never
    // covers more than half the library, so a small one cannot exclude
    // everything it has. This does not depend on which pass is running, so it
    // is worked out once.
    const int windowSize = qMin(m_autoModeNoRepeat, qMax(0, libraryCount / 2));
    QStringList excluded;
    for (int i = qMax(0, m_recentAutoPicks.size() - windowSize);
         i < m_recentAutoPicks.size(); ++i) {
        const QString &recent = m_recentAutoPicks.at(i);
        if (!recent.isEmpty() && !excluded.contains(recent))
            excluded << recent;
    }
    // These two are the immediate neighbours and are excluded whatever the
    // window is. An empty one is left out rather than bound: binding an empty
    // QString binds SQL NULL, "path <> NULL" is NULL rather than true, and
    // that is why Auto Mode could never pick its first track from a cold start
    // until it had already played something.
    for (const QString &neighbour : {lastPlayedSong, referenceTrack}) {
        if (!neighbour.isEmpty() && !excluded.contains(neighbour))
            excluded << neighbour;
    }

    const bool haveGenre = !currentGenre.isEmpty();
    const int lastPass = haveGenre ? 2 : 1; // no genre: pass 1 is already the wide one
    const int passCount = lastPass + 1;

    QStringList passNames;
    for (int pass = 0; pass <= lastPass; ++pass) {
        const bool matchBpm = (pass == 0 && referenceBpm > 0.0);
        const bool matchGenre = (pass < 2 && haveGenre);
        if (matchBpm && matchGenre)
            passNames << tr("tempo-matched, genre %1").arg(currentGenre);
        else if (matchBpm)
            passNames << tr("tempo-matched");
        else if (matchGenre)
            passNames << tr("genre %1").arg(currentGenre);
        else
            passNames << tr("whole library");
    }

    const RotationRules::Settings rotationSettings = RotationRules::settings();

    // One pass's worth of candidates. Identical SQL to the three-pass chooser
    // that was here before, except that it brings back a sample rather than a
    // single row: the rotation rules need something to choose between, and a
    // random sample of the pool is still a fair draw from it.
    auto candidatesForPass = [&](int pass) -> QList<RotationRules::Candidate> {
        QList<RotationRules::Candidate> out;
        const bool matchBpm = (pass == 0 && referenceBpm > 0.0);
        if (pass == 0 && !matchBpm)
            return out;             // nothing measured to match against
        const bool matchGenre = (pass < 2 && haveGenre);

        QString sql = QStringLiteral("select id, path, artist, song from musics where 1 = 1");
        if (!excluded.isEmpty()) {
            QStringList placeholders;
            for (int i = 0; i < excluded.size(); ++i)
                placeholders << QStringLiteral(":x%1").arg(i);
            sql += QStringLiteral(" and path not in (%1)")
                       .arg(placeholders.join(QStringLiteral(", ")));
        }
        if (matchGenre)
            sql += QStringLiteral(" and genre1 like :genre");
        if (matchBpm) {
            // Half and double time count as a match: a 140 BPM track drops
            // over a 70 BPM one beat for beat, which is exactly the kind of
            // transition this is meant to find.
            sql += QStringLiteral(" and bpm > 0"
                                  " and min(abs(bpm - :ref), abs(bpm - :refDouble),"
                                  " abs(bpm - :refHalf)) <= :tolerance");
        }
        sql += QStringLiteral(" order by random() limit :limit");

        QSqlQuery query(db);
        query.prepare(sql);
        for (int i = 0; i < excluded.size(); ++i)
            query.bindValue(QStringLiteral(":x%1").arg(i), excluded.at(i));
        if (matchGenre)
            query.bindValue(QStringLiteral(":genre"), currentGenre);
        if (matchBpm) {
            query.bindValue(QStringLiteral(":ref"), referenceBpm);
            query.bindValue(QStringLiteral(":refDouble"), referenceBpm * 2.0);
            query.bindValue(QStringLiteral(":refHalf"), referenceBpm / 2.0);
            query.bindValue(QStringLiteral(":tolerance"), m_bpmTolerance);
        }
        query.bindValue(QStringLiteral(":limit"), rotationSettings.candidateLimit);

        if (!query.exec()) {
            qDebug() << "SQL ERROR: " << query.lastError();
            qDebug() << "SQL was: " << query.lastQuery();
            return out;
        }
        while (query.next()) {
            RotationRules::Candidate c;
            c.musicId = query.value(0).toLongLong();
            c.path    = query.value(1).toString();
            c.artist  = query.value(2).toString();
            c.title   = query.value(3).toString();
            if (!c.path.isEmpty())
                out.append(c);
        }
        if (out.isEmpty()) {
            if (matchBpm) {
                qDebug() << "autoMode found nothing within" << m_bpmTolerance
                         << "BPM of" << referenceBpm << "— widening the search";
            } else if (matchGenre) {
                qDebug() << "autoMode found nothing in this hour's genre"
                         << currentGenre << "— falling back to the whole library";
            }
        }
        return out;
    };

    // Rotation: artist and title separation, dayparts, date windows and the
    // category weights, relaxed one rule at a time when the pool cannot
    // satisfy them all. Nothing in here can return "no track" for a library
    // that has one — only an empty pool can, and that is handled below.
    const RotationRules::Selection selection =
        RotationRules::instance()->select(candidatesForPass, passCount, passNames,
                                          now);
    if (selection.ok) {
        const QString path = selection.track.path;
        ui->playlist->addItem(path);
        rememberAutoModePick(path);

        const RotationRules::Decision &d = selection.decision;
        if (!d.rulesActive) {
            qDebug() << "autoMode (rotation off) adding:" << path
                     << "from the" << d.passName << "pass";
        } else if (d.relaxedRules.isEmpty()) {
            qDebug() << "autoMode adding:" << path << "—" << d.passName
                     << "pass," << d.survivors << "of" << d.poolSize
                     << "candidates passed every rotation rule; category"
                     << d.category << "weight" << d.weight;
        } else {
            // A rule that keeps having to be relaxed is a rule that is wrong
            // for this library, and the only way anybody finds that out is by
            // seeing it said plainly.
            qWarning() << "autoMode adding:" << path << "—" << d.passName
                       << "pass, but had to relax:"
                       << d.relaxedRules.join(QStringLiteral(", "))
                       << "(" << d.survivors << "of" << d.poolSize << "candidates)";
        }

        // Keep the chain going: the track just queued is the reference for
        // the next pick, so measure it now if nobody ever has.
        if (m_bpmMatch && m_bpmLibrary)
            m_bpmLibrary->analyzeQuietly(path);
        return true;
    }

    // Every pass came back empty, which means the exclusion list has eaten the
    // library rather than that a rotation rule bit. Last resort: a library too
    // small to offer anything *but* what was just played would otherwise leave
    // the station silent, which is the one thing Auto Mode exists to prevent.
    // Repeating is better than dead air.
    if (!excluded.isEmpty()) {
        QSqlQuery again(db);
        if (again.exec(QStringLiteral("select path from musics"
                                      " order by random() limit 1"))
            && again.next()) {
            const QString onlyChoice = again.value(0).toString();
            qDebug() << "autoMode has nothing it has not played recently;"
                     << "repeating" << onlyChoice << "rather than going silent";
            ui->playlist->addItem(onlyChoice);
            rememberAutoModePick(onlyChoice);
            if (m_bpmMatch && m_bpmLibrary)
                m_bpmLibrary->analyzeQuietly(onlyChoice);
            return true;
        }
    }
    qDebug() << "autoMode found no track to add (empty library)";
    return false;
}

// ---------------------------------------------------------------------------
// The hour clock
//
// Two jobs, and only two. hourClockGenreNow() tells Auto Mode what kind of
// music the clock wants filled at this second of the hour, and hourClockTick()
// puts the hour's non-music items — the news, the ad break at :20, the jingle
// between two sweeps — into the running order when they come due. Everything
// else about a clock is editing, and editing lives in HourClockDialog.
//
// "When they come due" is the timeline's answer, not the slot's: a hard-timed
// item is due at the second it is pinned to, and a floating one at the second
// the arithmetic lands it on. The difference between the two kinds is which
// of them the hour is allowed to push around, and that is settled in
// HourClock::resolve() before this ever sees it.
//
// Both are inert until HourClock/Enabled is set. That is deliberate: a station
// that never opens the feature must behave byte for byte as it did before, and
// the only way to be sure of that is for the new code to return early.
// ---------------------------------------------------------------------------

QString player::hourClockGenreNow() const
{
    const HourClock::Settings settings = HourClock::settings();
    if (!settings.enabled)
        return QString();
    const QDateTime now = QDateTime::currentDateTime();
    const int second = now.time().minute() * 60 + now.time().second();
    return HourClock::genreForMoment(now.date().dayOfWeek(), now.time().hour(),
                                     second);
}

void player::setupHourClock()
{
    const HourClock::Settings settings = HourClock::settings();
    if (!settings.enabled || !settings.fireHardTimed) {
        if (m_hourClockTimer)
            m_hourClockTimer->stop();
        return;
    }
    if (!m_hourClockTimer) {
        // Twenty seconds, against a firing window of sixty: an item is checked
        // three times inside its window, so one missed tick (a busy segue, a
        // dialog holding the event loop) still puts the news on air.
        m_hourClockTimer = new QTimer(this);
        m_hourClockTimer->setInterval(20000);
        connect(m_hourClockTimer, &QTimer::timeout, this, &player::hourClockTick);
    }
    if (!m_hourClockTimer->isActive())
        m_hourClockTimer->start();
    qInfo() << "Hour clock: following the programmed clocks; fixed items will "
               "be queued within" << settings.fireWindowSeconds << "s of their time";
}

void player::hourClockTick()
{
    const HourClock::Settings settings = HourClock::settings();
    if (!settings.enabled || !settings.fireHardTimed)
        return;
    // Only while XFB is driving. With the operator at the desk, nothing should
    // be pushing items into the running order behind their back.
    if (autoMode != 1)
        return;
    if (!ui || !ui->playlist)
        return;

    const QDateTime now = QDateTime::currentDateTime();
    const int day    = now.date().dayOfWeek();
    const int hour   = now.time().hour();
    const int second = now.time().minute() * 60 + now.time().second();

    const qint64 clockId = HourClock::clockIdFor(day, hour);
    if (clockId <= 0)
        return;
    const HourClock::Clock clock = HourClock::loadClock(clockId);
    if (clock.items.isEmpty())
        return;
    const HourClock::Timeline timeline = HourClock::resolve(clock);

    for (const HourClock::ResolvedSlot &r : timeline.items) {
        // Music sweeps are filled by Auto Mode, not fired: there is no one
        // file that *is* a sweep.
        if (r.slot.isMusic())
            continue;
        // Everything else in the hour is fired at the time the timeline puts
        // it at — which for a hard-timed item is the time it is pinned to
        // (resolve() never moves those), and for a floating one is where it
        // lands after the items before it. A floating ad break is still an ad
        // break: it was written into the hour to go on air, and only firing
        // the pinned ones left every unpinned item as a drawing of an hour
        // that never played.
        if (second < r.start || second >= r.start + settings.fireWindowSeconds)
            continue;

        const QString key = QStringLiteral("%1/%2/%3")
                                .arg(now.date().toString(Qt::ISODate))
                                .arg(hour)
                                .arg(r.slot.position);
        if (m_hourClockFired.contains(key))
            continue;
        m_hourClockFired.insert(key);

        const QString path = HourClock::mediaPathFor(r.slot);
        const QString what = r.slot.label.isEmpty()
                                 ? HourClock::typeLabel(r.slot.type)
                                 : r.slot.label;
        if (path.isEmpty()) {
            // A programmed item with nothing behind it is worth saying out
            // loud once, and worth not retrying every twenty seconds.
            qWarning() << "Hour clock:" << what << "is due at"
                       << HourClock::formatOffset(r.start)
                       << "but nothing in the" << HourClock::mediaTable(r.slot.type)
                       << "table matches" << r.slot.reference;
            announceAccessible(tr("%1 is due now but there is no audio for it.")
                                   .arg(what));
            continue;
        }

        // Next, not last: a fixed item that lands after the four tracks Auto
        // Mode has already queued is not a fixed item at all.
        ui->playlist->insertItem(0, path);
        calculate_playlist_total_time();
        qInfo() << "Hour clock: queued" << what << "for" << path;
        announceAccessible(tr("%1 is due at %2 past the hour and is next in "
                              "the running order.")
                               .arg(what, HourClock::formatOffset(r.start)));
    }

    // The key set only ever grows within a day. Bound it rather than let a
    // station left running for a month accumulate.
    if (m_hourClockFired.size() > 512)
        m_hourClockFired.clear();
}

// ---------------------------------------------------------------------------
// Time signals
//
// The pips, and the ident that names the hour. The hour clock could fire an
// ident, but only under Auto Mode, only one clock to an hour (so a different
// line for each hour of the day means twenty-four clocks), and it falls back
// to a *random* jingle when the name matches nothing — which for audio whose
// whole job is to say "it's ten o'clock" is the one wrong answer.
//
// So this is its own thing, and the three differences are the point: the days
// and hours ride on the signal itself, it fires with an operator at the desk,
// and it can reach the air three ways instead of only the queue.
// ---------------------------------------------------------------------------

void player::setupTimeSignals()
{
    const TimeSignal::Settings settings = TimeSignal::settings();
    if (!settings.enabled) {
        if (m_timeSignalTimer)
            m_timeSignalTimer->stop();
        qInfo() << "Time signals: off; nothing is fired";
        return;
    }
    if (!m_timeSignalTimer) {
        m_timeSignalTimer = new QTimer(this);
        connect(m_timeSignalTimer, &QTimer::timeout, this, &player::timeSignalTick);
    }
    // The tick *is* the accuracy: a signal cannot land closer to its second
    // than the interval it is looked for on.
    m_timeSignalTimer->setInterval(settings.tickMs);
    if (!m_timeSignalTimer->isActive())
        m_timeSignalTimer->start();
    qInfo() << "Time signals: on, checked every" << settings.tickMs
            << "ms;" << (settings.whenLive ? "with an operator driving too"
                                           : "under Auto Mode only");
}

void player::timeSignalTick()
{
    const TimeSignal::Settings settings = TimeSignal::settings();
    if (!settings.enabled)
        return;
    // Unlike the hour clock, this does not insist on Auto Mode — a station's
    // pips are the station's whoever is at the desk — but the operator can
    // still say otherwise.
    if (!settings.whenLive && autoMode != 1)
        return;
    if (!ui || !ui->playlist)
        return;

    const QDateTime now = QDateTime::currentDateTime();
    const int day    = now.date().dayOfWeek();
    const int hour   = now.time().hour();
    const int second = now.time().minute() * 60 + now.time().second();

    const QList<TimeSignal::Signal> due = TimeSignal::allSignals();
    for (const TimeSignal::Signal &signal : due) {
        if (!TimeSignal::isDue(signal, day, hour, second))
            continue;

        const QString key = QStringLiteral("%1/%2/%3")
                                .arg(now.date().toString(Qt::ISODate))
                                .arg(hour)
                                .arg(signal.id);
        if (m_timeSignalFired.contains(key))
            continue;
        m_timeSignalFired.insert(key);

        const QString what = signal.label.isEmpty() ? tr("The time signal")
                                                    : signal.label;
        const QString path = TimeSignal::mediaPathFor(signal, hour);
        if (path.isEmpty()) {
            // Said once, and not retried every second. A hole is bad; a
            // random jingle claiming the wrong hour is worse, so nothing
            // else is played in its place.
            qWarning() << "Time signals:" << what << "is due but nothing is called"
                       << TimeSignal::referenceFor(signal, hour);
            announceAccessible(tr("%1 is due now, but there is no audio called \"%2\".")
                                   .arg(what, TimeSignal::referenceFor(signal, hour)));
            continue;
        }
        fireTimeSignal(signal, path);
    }

    // The key set only ever grows within a day. Bound it rather than let a
    // station left running for a month accumulate.
    if (m_timeSignalFired.size() > 512)
        m_timeSignalFired.clear();
}

void player::fireTimeSignal(const TimeSignal::Signal &signal, const QString &path)
{
    const QString what = signal.label.isEmpty() ? tr("The time signal") : signal.label;

    switch (signal.placement) {

    case TimeSignal::Placement::Next:
        // The old behaviour, and still the right one for an ident on a music
        // station that would rather not chop a record: next, not last.
        ui->playlist->insertItem(0, path);
        calculate_playlist_total_time();
        qInfo() << "Time signals: queued" << what << "next —" << path;
        announceAccessible(tr("%1 is next in the running order.").arg(what));
        return;

    case TimeSignal::Placement::Interrupt: {
        ui->playlist->insertItem(0, path);
        calculate_playlist_total_time();

        // With the deck stopped there is nothing to interrupt, and
        // playNextSong() would refuse the advance anyway ("there's nothing to
        // play"), leaving the signal sitting in the running order while the
        // log claimed it had gone out. A station that is off air is off air:
        // the signal waits at the top of the order, and says so.
        const bool onAir = Xplayer
                        && Xplayer->playbackState() == QMediaPlayer::PlayingState;
        if (!onAir) {
            qInfo() << "Time signals: nothing was playing, so" << what
                    << "is next in the running order instead —" << path;
            announceAccessible(tr("%1 is next in the running order: nothing was "
                                  "playing to interrupt.").arg(what));
            return;
        }

        // Straight through playNextSong(), which is the path the skip button
        // and the segue already take: the as-run row for what was playing is
        // closed with a reason of its own, and the running order carries on
        // from the item after this one.
        m_airEndReason = QStringLiteral("time signal");
        m_manualAdvancing = true;
        playNextSong();
        m_airEndReason.clear();
        QTimer::singleShot(200, this, [this]() { m_manualAdvancing = false; });
        qInfo() << "Time signals: interrupted the deck for" << what << "—" << path;
        announceAccessible(tr("%1 is on air now.").arg(what));
        return;
    }

    case TimeSignal::Placement::OverTheTop:
        break;   // the long one, below
    }

    // --- over the top ---
    // Its own player on the on-air device, so the music underneath is left
    // exactly as it is and only its level moves.
    if (!m_timeSignalPlayer) {
        m_timeSignalOutput = new QAudioOutput(this);
        if (!m_mainOutputDeviceId.isEmpty()) {
            bool fellBack = false;
            const QAudioDevice device =
                AudioDeviceRouter::resolve(m_mainOutputDeviceId, &fellBack);
            if (fellBack)
                qWarning() << "Time signals: the on-air device is gone, using the default";
            if (!device.isNull())
                m_timeSignalOutput->setDevice(device);
        }
        m_timeSignalPlayer = new QMediaPlayer(this);
        m_timeSignalPlayer->setAudioOutput(m_timeSignalOutput);

        connect(m_timeSignalPlayer, &QMediaPlayer::mediaStatusChanged, this,
                [this](QMediaPlayer::MediaStatus status) {
                    if (status == QMediaPlayer::EndOfMedia)
                        endTimeSignalDuck();
                });
        connect(m_timeSignalPlayer, &QMediaPlayer::errorOccurred, this,
                [this, what](QMediaPlayer::Error, const QString &message) {
                    qWarning() << "Time signals: could not play it:" << message;
                    endTimeSignalDuck();
                });
        connect(m_timeSignalPlayer, &QMediaPlayer::positionChanged, this,
                [this](qint64 position) {
                    if (m_timeSignalAirHandle > 0)
                        AirLog::instance()->heartbeat(m_timeSignalAirHandle, position);
                });
    }

    // A signal that arrives while the last one is still going: the new one
    // wins, and the duck is not applied twice.
    if (m_timeSignalPlayer->playbackState() != QMediaPlayer::StoppedState) {
        m_timeSignalPlayer->stop();
        endTimeSignalDuck();
    }

    m_timeSignalDuck = qBound(0, signal.duckPercent, 100) / 100.0;
    if (XplayerOutput) {
        // Push the new factor through the same sum the envelope uses, so the
        // next position tick does not undo it.
        const double base = ui->sliderVolume->value() / 100.0;
        const double gain = m_activeEnvelope.isEmpty()
                                ? 1.0
                                : PlaylistWaveView::envelopeGainAt(m_activeEnvelope,
                                                                  m_airPosition);
        XplayerOutput->setVolume(float(base * gain * m_timeSignalDuck));
    }

    m_timeSignalOutput->setVolume(float(qBound(0, signal.volumePercent, 100) / 100.0));
    m_timeSignalPlayer->setSource(QUrl::fromLocalFile(path));
    m_timeSignalPlayer->play();

    // It went out, so it goes in the as-run log like anything else that did.
    m_timeSignalAirHandle = AirLog::instance()->openPath(path, autoMode == 1);

    qInfo() << "Time signals:" << what << "over the top, music at"
            << signal.duckPercent << "% —" << path;
    announceAccessible(tr("%1 is playing over the music.").arg(what));
}

void player::endTimeSignalDuck()
{
    if (m_timeSignalAirHandle > 0) {
        AirLog::instance()->close(m_timeSignalAirHandle,
                                  m_timeSignalPlayer ? m_timeSignalPlayer->position() : 0,
                                  QStringLiteral("end"));
        m_timeSignalAirHandle = 0;
    }

    if (qFuzzyCompare(m_timeSignalDuck, 1.0))
        return;
    m_timeSignalDuck = 1.0;

    // Put the music back where the slider and the envelope say it should be,
    // rather than at whatever the duck left it at.
    if (XplayerOutput) {
        const double base = ui->sliderVolume->value() / 100.0;
        const double gain = m_activeEnvelope.isEmpty()
                                ? 1.0
                                : PlaylistWaveView::envelopeGainAt(m_activeEnvelope,
                                                                  m_airPosition);
        XplayerOutput->setVolume(float(base * gain));
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

    // The operator cut it short — the log has to say so, not "played out".
    m_airEndReason = QStringLiteral("skipped");

    // Suppress the auto-advance that fires when the current source stops
    m_manualAdvancing = true;
    
    playNextSong();
    m_airEndReason.clear();
    
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
            // Was a hand-written mp3/mp4/ogg/wav/flac list, so a programme
            // delivered as Opus, m4a, aac or wma was never scheduled.
            const QStringList programFiles = AudioFormats::findAudioFiles(dir);
            for (const QString &file : programFiles){

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
                        sql_add.prepare("INSERT INTO programs (name,path) VALUES(?, ?)");
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
















    // NOTE: the block below is commented out and has been since long before
    // this change, so it imports nothing today. If it is ever revived it must
    // use AudioFormats::findAudioFiles() like the programme scan above — the
    // mp3/mp4/ogg/wav/flac list it still carries misses Opus, m4a, aac and wma.
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



                        QString time = MediaDuration::forFile(file);
                        qDebug()<<"Total track time is: "<<time;

                        // Must use the named connection: the app never opens
                        // Qt's default one, so a bare QSqlQuery silently fails
                        // and the downloaded program never reaches the library.
                        QSqlQuery sql_add(db);
                        int played = 0;
                        QString last = "-";
                        sql_add.prepare("INSERT INTO musics (artist,song,genre1,genre2,country,"
                                        "published_date,path,time,played_times,last_played) "
                                        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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








    // NOTE: the block below is commented out and has been since long before
    // this change, so it imports nothing today. If it is ever revived it must
    // use AudioFormats::findAudioFiles() like the programme scan above — the
    // mp3/mp4/ogg/wav/flac list it still carries misses Opus, m4a, aac and wma.
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



                QString time = MediaDuration::forFile(file);
                qDebug()<<"Total track time is: "<<time;


                // Named connection, not Qt's unopened default one (see above).
                QSqlQuery sql_add(db);
                int played = 0;
                QString last = "-";
                sql_add.prepare("INSERT INTO musics (artist,song,genre1,genre2,country,"
                                        "published_date,path,time,played_times,last_played) "
                                        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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
                    qry.prepare("INSERT INTO programs (name,path) VALUES(?, ?)");
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

    // The takeover XMLs live in the configured FTP folder (FTPPath): that is
    // where the upload script picks up *.xml from, and it is user-writable —
    // the old hardcoded /usr/share/xfb/ftp/ is neither on installed systems.
    QFile::remove(QDir(FTPPath).filePath("takeover.xml"));

    QString takeOverFile = QDir(FTPPath).filePath("returntakeover.xml");
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


        QProcess sh2;
        QByteArray output2;
        QString FTPCmdPath, xmls;

        QString putTakeOverScript = serverScriptPath("serverFtpCmdsPutTakeOver.sh");
        if (putTakeOverScript.isEmpty()) {
            qWarning() << "returnTakeOver: serverFtpCmdsPutTakeOver.sh not found — cannot notify the server.";
            return;
        }
        FTPCmdPath = serverScriptShellCommand(putTakeOverScript) + " | grep 'Transfer complete'";
        qDebug() << "running: " << FTPCmdPath;
        qDebug() << "If you get errors: chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";



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

            QFile::remove(QDir(FTPPath).filePath("takeover.xml"));

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



// Type 3 rows whose interval has run out. Collected here and acted on after
// the loop rather than inside it, because the loop is walking the same table.
// The rowid rides along as the last column so a row can be named exactly;
// columns 0..14 keep the positions the two older types read them from.
QList<QPair<QVariant, QString>> expiredIntervals; // rowid, advert id ("" = programme)

QSqlQuery sched_qry(db);
sched_qry.prepare("select *, rowid from scheduler");
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

            /* The day of the week, resolved rather than string-compared.
             *
             * This used to build today's name from English literals and
             * compare it with `==` against whatever the column held — and the
             * column held the *displayed* text of the combo box in "Add a
             * publicity" / "Add a program", which on a Portuguese or French
             * installation is "Segunda" or "Lundi". So every weekly booking
             * made in a translated XFB silently never went on air, with
             * nothing anywhere to say so. ProgrammeSchedule::weekDayFor()
             * understands both, so those bookings now fire; new rows are
             * written with the English name (see add_pub.cpp).
             *
             * The hour and minute are compared as numbers for the same class
             * of reason: "08" and "8" are the same time and were not the same
             * string. */

            const QString is_program = sched_qry.value(14).toString();

            const int today = QDate::currentDate().dayOfWeek();
            const int scheduledDay =
                ProgrammeSchedule::weekDayFor(sched_qry.value(7).toString());
            const QTime nowTime = QTime::currentTime();

            // A day nothing recognises is skipped in silence: this runs every
            // minute, and the schedule window is where an operator is told
            // about a booking that will never air. An empty hour or minute is
            // skipped for the same reason, and so that it does not read as
            // midnight now that these are compared as numbers.
            const bool hasTime = !sched_qry.value(4).isNull()
                                 && !sched_qry.value(5).isNull();

            if((scheduledDay != 0) && hasTime && (scheduledDay == today)
               && (nowTime.hour() == sched_qry.value(4).toInt())
               && (nowTime.minute() == sched_qry.value(5).toInt())){
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

        /*
         * 3 = an event to be played every day at a specific hour/min, between
         *     two dates, both included. A campaign: "this advert, at ten past
         *     eight, for the fortnight it is paid for".
         *
         * The six start_/end_ columns have been in the scheduler table since
         * it was written; until now nothing filled them and nothing read them.
         */
        if(tipo=="3"){

            const QDate today = QDate::currentDate();
            const QDate from(sched_qry.value(8).toInt(),
                             sched_qry.value(9).toInt(),
                             sched_qry.value(10).toInt());
            const QDate to(sched_qry.value(11).toInt(),
                           sched_qry.value(12).toInt(),
                           sched_qry.value(13).toInt());

            const QString schId = sched_qry.value(0).toString();
            const QString is_program = sched_qry.value(14).toString();

            if(!from.isValid() || !to.isValid()){
                qWarning() << "Scheduler: a date interval rule has no usable dates"
                           << "and was ignored (id" << schId << ")";
            } else if(today > to){

                // The campaign is over. Same ending as a one-off: the rule
                // goes, and an advert with no rules left goes with it.
                expiredIntervals.append(qMakePair(sched_qry.value(15),
                                                  is_program=="1" ? QString() : schId));

            } else if(today >= from){

                const QTime nowTime = QTime::currentTime();
                if((nowTime.hour()==sched_qry.value(4).toInt())
                   && (nowTime.minute()==sched_qry.value(5).toInt())){

                    qDebug() << "Scheduled event now fired (type 3)!";

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

// The campaigns that ran out, cleared now the walk over the table is done.
for(const QPair<QVariant, QString> &expired : std::as_const(expiredIntervals)){

    QSqlQuery del_qry(db);
    del_qry.prepare("DELETE FROM scheduler WHERE rowid = ?");
    del_qry.addBindValue(expired.first);
    if(!del_qry.exec()){
        qWarning() << "Scheduler: could not remove a finished date interval:"
                   << del_qry.lastError().text();
        continue;
    }
    qInfo() << "Scheduler: a date interval has run its course and was removed.";

    if(expired.second.isEmpty())
        continue;   // a programme: XFB never deletes those by itself

    QSqlQuery left_qry(db);
    left_qry.prepare("SELECT count(id) FROM scheduler WHERE id=?");
    left_qry.addBindValue(expired.second);
    if(left_qry.exec() && left_qry.next() && left_qry.value(0).toInt()==0){
        QSqlQuery drop_qry(db);
        drop_qry.prepare("DELETE FROM pub WHERE id=?");
        drop_qry.addBindValue(expired.second);
        if(drop_qry.exec()){
            qInfo() << "Scheduler: advert" << expired.second
                    << "has no schedules left and was removed.";
            update_music_table();
        }
    }
}

}







void player::on_actionOptions_triggered()
{
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("station.options"), this))
        return;

    // Use show() instead of exec() to avoid blocking the event loop
    // (exec() blocks QMediaPlayer signal processing and causes audio to stop updating)
    optionsDialog *opt = new optionsDialog(this);
    opt->setAttribute(Qt::WA_DeleteOnClose);
    opt->setModal(true);
    connect(opt, &QDialog::finished, this, &player::update_music_table);
    // Re-apply the saved settings immediately (tab visibility, seek bar,
    // volume lock, torrents tab, FX tab...) instead of requiring a restart
    connect(opt, &QDialog::finished, this, &player::updateConfig);
    // Tempo matching does nothing until the library has been measured, and
    // an empty BPM column gives the operator no clue why. Offer the sweep
    // right where the option is switched on (updateConfig, connected above,
    // has already refreshed m_bpmMatch by the time this runs).
    connect(opt, &QDialog::finished, this, &player::offerBpmAnalysis);
    opt->show();
}

// Ask about measuring the library's tempo, but only when it would change
// anything: the option is on, and tracks are still unmeasured. Declining is
// remembered, so this never becomes a dialog the operator learns to dismiss.
void player::offerBpmAnalysis()
{
    if (!m_bpmMatch || !m_bpmLibrary || m_bpmLibrary->busy())
        return;

    const int missing = m_bpmLibrary->tracksMissingBpm().size();
    if (missing == 0)
        return;

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/xfb.conf", QSettings::IniFormat);
    if (settings.value("AutoModeBpmAnalysisDeclined", false).toBool())
        return;

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Measure the BPM of the library?"),
        tr("Auto Mode can only follow a track with one at a similar tempo once "
           "the tempo of each track has been measured. %n track(s) have not been "
           "measured yet, and stay out of the matching until they are.\n\n"
           "Measure them now? Each track is decoded once, in the background; you "
           "can also start this later from the Database menu.", nullptr, missing),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (answer == QMessageBox::Yes)
        analyzeLibraryBpm();
    else
        settings.setValue("AutoModeBpmAnalysisDeclined", true);
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
    applyMusicHeaderLabels(model);
    ui->musicView->setModel(model);

    ui->musicView->setSortingEnabled(true);
    ui->musicView->hideColumn(0);
    ui->musicView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

void player::on_bt_reset_clicked()
{
    // Show the full list immediately. The old bare setQuery() ran on Qt's
    // default database connection — the app's data lives on the named
    // "xfb_connection", so the view stayed stale until a manual search
    // (which uses the right connection) refreshed it.
    ui->txt_search->clear();
    checkDbOpen();
    delete ui->musicView->model();
    QSqlQueryModel *modelo = new QSqlQueryModel();
    QSqlQuery all(QSqlDatabase::database("xfb_connection"));
    all.exec("select * from musics");
    modelo->setQuery(std::move(all));
    applyMusicHeaderLabels(modelo);
    ui->musicView->setModel(modelo);
    ui->musicView->setSortingEnabled(true);
    ui->musicView->hideColumn(0);
    ui->musicView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
        applyMusicHeaderLabels(model);
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
               // A recorded link is marked so it comes back as one; the
               // attribute is additive, so an older XFB reading this file
               // simply plays the link as an ordinary track.
               const bool isVoiceTrack = ui->playlist->item(i)
                   ->data(PlaylistWaveView::VoiceTrackRole).toBool();
               xmlWriter.writeStartElement("track");
               if (overlapMs > 0)
                   xmlWriter.writeAttribute("overlap", QString::number(overlapMs));
               if (!volumeLine.isEmpty())
                   xmlWriter.writeAttribute("volenv", volumeLine);
               if (isVoiceTrack)
                   xmlWriter.writeAttribute("voicetrack", QStringLiteral("1"));
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
                        // Absent in every playlist written before voice
                        // tracking existed, which is exactly what we want:
                        // those rows load as the music items they are.
                        const bool isVoiceTrack =
                            Rxml.attributes().value(QStringLiteral("voicetrack"))
                                == QLatin1String("1");
                        QString track = Rxml.readElementText();
                        qDebug()<<"Rxml.readElementText(): "<<track;
                        auto *item = new QListWidgetItem(track);
                        if (overlapMs > 0)
                            item->setData(PlaylistWaveView::OverlapRole, overlapMs);
                        if (isVoiceTrack)
                            item->setData(PlaylistWaveView::VoiceTrackRole, true);
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
    // Same reason as the voice-track dialog: without the operator's agreement
    // macOS hands back silence rather than refusing, and the programme would
    // be lost before anyone noticed. This sits before recMode changes so the
    // answer can re-enter here and start the recording properly.
    if (!ensureMicrophoneAccess(this, [this]() { on_bt_rec_clicked(); }))
        return;

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
        // The LED colour is meaningless to a screen reader, so state the
        // problem in text as well — a silent failed recording is the worst
        // outcome here.
        ui->led_rec->setAccessibleName(tr("Recording problem: nothing is being recorded"));
        ui->led_rec->setToolTip(tr("Recording problem: nothing is being recorded"));
        announceAccessible(tr("Warning: the recording is empty — check the input device"));
    } else {
        //green
         ui->led_rec->setStyleSheet("background-color:#B3FF57;border-radius:8px;");
         ui->led_rec->setAccessibleName(tr("Recording"));
         ui->led_rec->setToolTip(tr("Recording is running"));
    }



}

void player::RecT5(){

    //amarilo
    ui->led_rec->setStyleSheet("background-color:#FFFB00;border-radius:8px;");
    ui->led_rec->setAccessibleName(tr("Recording starting"));
    ui->led_rec->setToolTip(tr("Recording is starting"));
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

#ifdef Q_OS_LINUX
    // Pacman installs (Arch/AUR) can't use the .deb release asset: the package
    // manager owns the files, so point the user at the AUR instead
    {
        const QString pacman = QStandardPaths::findExecutable("pacman");
        if (!pacman.isEmpty()) {
            QProcess probe;
            probe.start(pacman, {"-Qo", QCoreApplication::applicationFilePath()});
            probe.waitForFinished(8000);
            if (probe.exitStatus() == QProcess::NormalExit && probe.exitCode() == 0) {
                QMessageBox::information(this, tr("Update with your package manager"),
                    tr("This copy of XFB is managed by pacman, so the update comes "
                       "from the AUR. Run your AUR helper, for example:\n\n"
                       "    yay -Syu xfb\n\n"
                       "then launch XFB again to run version %1.").arg(version));
                return;
            }
        }
        // The release asset is a .deb, which only dpkg-based systems can install
        if (QStandardPaths::findExecutable("dpkg").isEmpty()
            && QStandardPaths::findExecutable("apt").isEmpty()) {
            QDesktopServices::openUrl(releasePage);
            return;
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

                                QProcess sh2;
                                QByteArray output2;
                                QString FTPCmdPath, xmls;

                                QString putProgramScript = serverScriptPath("serverFtpCmdsPutProgram.sh");
                                if (putProgramScript.isEmpty()) {
                                    QMessageBox::critical(this, tr("Script Error"),
                                                          tr("The required upload script '%1' was not found.\nInstall it under the application data directory (e.g. share/xfb/scripts) or set ServerScriptsPath in xfb.conf.").arg("serverFtpCmdsPutProgram.sh"));
                                    return;
                                }
                                FTPCmdPath = serverScriptShellCommand(putProgramScript) + " | grep 'Transfer complete'";
                                qDebug() << "running: " << FTPCmdPath;
                                qDebug() << "If you get errors: chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";

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
                                            qry.prepare("INSERT INTO programs (name,path) VALUES(?, ?)");
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
              QProcess sh2;

              QByteArray output2;
              QString FTPCmdPath, xmls;
              QString putProgramScript = serverScriptPath("serverFtpCmdsPutProgram.sh");
              if (putProgramScript.isEmpty()) {
                  QMessageBox::critical(this, tr("Script Error"),
                                        tr("The required upload script '%1' was not found.\nInstall it under the application data directory (e.g. share/xfb/scripts) or set ServerScriptsPath in xfb.conf.").arg("serverFtpCmdsPutProgram.sh"));
                  return;
              }
              FTPCmdPath = serverScriptShellCommand(putProgramScript) + " | grep 'Transfer complete'";
              qDebug() << "running: " << FTPCmdPath;
              qDebug() << "If you get errors: chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
              sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
              sh2.waitForFinished(-1);
              output2 = sh2.readAll();
              xmls = output2;
              qDebug()<<output2;
              sh2.close();
              qDebug()<<"Checking if the file's intergrity was perserved...";
              QProcess shCHK2;
              QByteArray outputCHK2;
              QString FTPCmdPathCHK, xmlsCHK;
              // The check script ships both with and without the .sh suffix;
              // prefer the .sh copy and fall back to the legacy bare name.
              QString chkProgramScript = serverScriptPath("serverFtpCmdsCHKProgram.sh");
              if (chkProgramScript.isEmpty())
                  chkProgramScript = serverScriptPath("serverFtpCmdsCHKProgram");
              if (chkProgramScript.isEmpty()) {
                  qWarning() << "serverFtpCmdsCHKProgram(.sh) not found — skipping upload integrity check.";
              } else {
              FTPCmdPathCHK = serverScriptShellCommand(chkProgramScript) + " | grep "+NomeDestePrograma;
              qDebug() << "running: " << FTPCmdPathCHK;
              qDebug() << "If you get errors: chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
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
                          QMessageBox::information(this,tr("Integrity verification failed!"),tr("The file does not seem to have been sent to the server correctly since the size of the local file differs from the one on the FTP server. Please try to send the program again."));
                      }
                  } else {
                      qDebug()<<"It was not possible to get the size of the local file: "<<mmfile;
                  }
              }
              } // end integrity check (script found)
              qDebug()<<"Program uploaded to server!";
              QString fileToRemove = FTPPath+"/"+NomeDestePrograma+".ogg";
              if (QFile::remove(fileToRemove)) {
                  qDebug()<<"FTP temp file deleted:" << fileToRemove;
              } else {
                  qWarning()<<"Failed to delete FTP temp file:" << fileToRemove;
              }
              QMessageBox::StandardButton answer;
              answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program? (The program was successfully uploaded to the server)"), QMessageBox::Yes|QMessageBox::No);
              if(answer==QMessageBox::Yes){
                  if (QFile::remove(destino)) {
                      qDebug()<<"Local file deleted:" << destino;
                  } else {
                      qWarning()<<"Failed to delete local file:" << destino;
                  }
                  QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
              } else {
                          QSqlQuery qry(db);
                          qry.prepare("INSERT INTO programs (name,path) VALUES(?, ?)");
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
              qry.prepare("INSERT INTO programs (name,path) VALUES(?, ?)");
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

        // --- File seems OK, probe the duration (exiftool, ffmpeg fallback) ---
        qDebug() << "File seems OK. Probing duration...";

        QString durationValue = MediaDuration::forFile(path);

        if (durationValue.isEmpty()) {
            qWarning() << "Could not determine duration for:" << path;
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
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("library.purge"), this))
        return;



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
    // The player panel positions its controls by hand, so it has to be told
    // whenever the space it has to fit them into changes.
    if (watched == ui->frame_4 && event->type() == QEvent::Resize) {
        relayoutPlayerFrame();
        return false; // let the frame see its own resize too
    }
    if (watched == ui->frame && event->type() == QEvent::Resize) {
        relayoutClockFrame();
        return false;
    }

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
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("library.autotrim"), this))
        return;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- SoX: install it on first use, with the user's consent ---
    QString soxPath = DependencyChecker::resolveExecutable("sox");
    if (soxPath.isEmpty()) {
        DependencyChecker depChecker;
        if (!depChecker.ensureDependency("sox", tr("Trimming the silence from the start and the end of every "
                  "track in the database is done by SoX (Sound eXchange)."), this)) {
            return;
        }
        soxPath = DependencyChecker::resolveExecutable("sox");
        if (soxPath.isEmpty())
            return;
    }
    qInfo() << "Found sox executable at:" << soxPath;
    // --- End SoX check ---

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
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("station.update"), this))
        return;

    checkForUpdates();
}

void player::on_actionInstall_all_dependencies_triggered()
{
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("station.dependencies"), this))
        return;

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

          // --- SoX: install it on first use, with the user's consent ---
          QString soxPath = DependencyChecker::resolveExecutable("sox");
          if (soxPath.isEmpty()) {
              DependencyChecker depChecker;
              if (!depChecker.ensureDependency("sox", tr("Trimming the silence from the start and the end of the "
                            "selected tracks is done by SoX (Sound eXchange)."), this)) {
                  return;
              }
              soxPath = DependencyChecker::resolveExecutable("sox");
              if (soxPath.isEmpty())
                  return;
          }
          qInfo() << "Found sox executable at:" << soxPath;
          // --- End SoX check ---

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
          // --- SoX: install it on first use, with the user's consent ---
          QString soxPath = DependencyChecker::resolveExecutable("sox");
          if (soxPath.isEmpty()) {
              DependencyChecker depChecker;
              if (!depChecker.ensureDependency("sox", tr("Trimming the silence from the start and the end of the "
                            "selected tracks is done by SoX (Sound eXchange)."), this)) {
                  return;
              }
              soxPath = DependencyChecker::resolveExecutable("sox");
              if (soxPath.isEmpty())
                  return;
          }
          qInfo() << "Found sox executable at:" << soxPath;
          // --- End SoX check ---

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
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("library.convert"), this))
        return;

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
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("library.convert"), this))
        return;

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
void player::on_actionConvert_all_musics_in_the_database_to_opus_triggered()
{
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("library.convert"), this))
        return;

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

    // --- Check that this ffmpeg was built with libopus ---
    // Unlike MP3 and Vorbis, plenty of ffmpeg builds ship without an Opus
    // encoder. Finding that out now costs one process; finding it out inside
    // the loop costs one failed track per record in the database.
    {
        QProcess encoderProbe;
        encoderProbe.start(ffmpegPath, {"-hide_banner", "-encoders"});
        encoderProbe.waitForFinished(15000);
        const QString encoders = QString::fromUtf8(encoderProbe.readAllStandardOutput());
        if (!encoders.contains(" libopus ")) {
            qWarning() << "ffmpeg at" << ffmpegPath << "has no libopus encoder.";
            QMessageBox::critical(this, "Missing Encoder",
                                  "This ffmpeg build has no Opus encoder (libopus), so the tracks "
                                  "cannot be converted to .opus.\n\nInstall an ffmpeg built with "
                                  "libopus, or convert to ogg instead.");
            return;
        }
    }
    // --- End encoder check ---

    // --- Confirmation ---
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Full Conversion",
                                     "Convert ALL tracks in the database to Opus (128kbps VBR)?\n\n"
                                     "Original files will be replaced with the Opus version.\n"
                                     "This action cannot be undone and may take a very long time!\n\n"
                                     "Note: Only the audio stream will be kept.\n"
                                     "Tracks that are already .opus are left untouched.",
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

    QProgressDialog progressDialog("Converting all tracks to Opus...", "Cancel", 0, 0, this);
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
    int skippedCount = 0; // Already .opus — nothing to do, and re-encoding would only lose quality
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

        // Already Opus: transcoding it again would throw away quality for nothing.
        if (originalFileInfo.suffix().compare("opus", Qt::CaseInsensitive) == 0) {
            qInfo() << "Already Opus, leaving untouched:" << originalPath;
            skippedCount++;
            continue;
        }

        // --- Prepare Paths ---
        // completeBaseName() keeps everything up to the LAST dot, so a title
        // such as "Artist - Song feat. Bob.mp3" does not lose its tail.
        QString baseName = originalFileInfo.completeBaseName();
        QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
        QString tempOpusPath = tempDir.filePath(baseName + ".opus"); // Full path for temporary Opus
        QString finalOpusPath = QDir(originalDir).filePath(baseName + ".opus"); // Final path for Opus in original dir

        // A different file already sitting on the destination name would be
        // silently destroyed by the rename below, so refuse this record.
        if (QFile::exists(finalOpusPath)) {
            qWarning() << "Destination already exists, refusing to overwrite:" << finalOpusPath;
            failCount++;
            continue;
        }

        // --- Run FFMPEG ---
        QProcess ffmpegProcess;
        QStringList ffmpegArgs;
        // Args for Opus. "-vn" matters here: an attached cover image is a video
        // stream, and libopus in an Ogg container cannot carry it, so leaving it
        // mapped makes ffmpeg fail outright.
        ffmpegArgs << "-y"                       // Never stop on an "overwrite?" prompt (there is no stdin)
                   << "-i" << originalPath       // Input file
                   << "-vn"                      // No video output (drops cover art)
                   << "-c:a" << "libopus"        // Audio codec: libopus
                   << "-b:a" << "128k"           // Target bitrate (VBR by default)
                   << tempOpusPath;              // Output file path

        qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

        ffmpegProcess.start(ffmpegPath, ffmpegArgs);

        // Wait for ffmpeg (e.g., 10 min timeout)
        if (!ffmpegProcess.waitForFinished(600000)) {
            qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
            ffmpegProcess.kill();
            ffmpegProcess.waitForFinished(1000);
            QFile::remove(tempOpusPath); // Clean up incomplete temp file
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
            QFile::remove(tempOpusPath); // Clean up potentially failed temp file
            failCount++;
            continue;
        }

        // Check if temp Opus exists and has size
        QFileInfo tempOpusInfo(tempOpusPath);
        if (!tempOpusInfo.exists() || tempOpusInfo.size() == 0) {
            qWarning() << "ffmpeg finished successfully but the output Opus file is missing or empty:" << tempOpusPath;
            QFile::remove(tempOpusPath);
            failCount++;
            continue;
        }

        qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempOpusPath;

        // --- Delete Original File ---
        qInfo() << "Attempting to delete original file:" << originalPath;
        if (!QFile::remove(originalPath)) {
            qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
            QFile::remove(tempOpusPath); // Don't leave the conversion behind in temp
            failCount++;
            continue; // Skip rest of steps for this file
        }
        qInfo() << "Original file deleted successfully.";

        // --- Move Temporary Opus to Final Location ---
        qInfo() << "Attempting to move" << tempOpusPath << "to" << finalOpusPath;
        if (!QFile::rename(tempOpusPath, finalOpusPath)) {
            qWarning() << "Failed to move temporary Opus" << tempOpusPath << "to" << finalOpusPath << ". The original file was deleted! Opus remains in temp folder.";
            QMessageBox::warning(this, "Move Failed",
                                 QString("Failed to move the converted Opus file to its final destination:\n%1\n\n"
                                         "The original file was deleted, but the Opus file remains in the temporary folder:\n%2\n\n"
                                         "Please move it manually and check the database record.")
                                     .arg(finalOpusPath).arg(tempOpusPath));
            failCount++;
            continue; // Cannot update DB path if move failed
        }
        qInfo() << "Opus file moved successfully to:" << finalOpusPath;

        // --- Update Database ---
        qInfo() << "Updating database: set path =" << finalOpusPath << "where path =" << originalPath;
        queryUpdate.bindValue(":new_path", finalOpusPath);
        queryUpdate.bindValue(":old_path", originalPath);

        if (!queryUpdate.exec()) {
            qWarning() << "Failed to update database path for:" << originalPath << "->" << finalOpusPath;
            qWarning() << "DB Error:" << queryUpdate.lastError().text();
            qWarning() << "Last Query (Bound values might not show):" << queryUpdate.lastQuery();
            QMessageBox::warning(this, "Database Update Failed",
                                 QString("The file was successfully converted and moved to:\n%1\n\n"
                                         "However, updating the database record failed:\n%2\n\n"
                                         "Please check the database manually.")
                                     .arg(finalOpusPath).arg(queryUpdate.lastError().text()));
            failCount++; // Count as failure since DB update is critical
        } else {
            qInfo() << "Database path updated successfully.";
            successCount++;
        }

    } // End while loop

    // --- Final Cleanup & Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close progress dialog

    update_music_table(); // Update the table view once after all operations

    QString summaryMessage;
    if (cancelled) {
        summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Converted to Opus: %2\nAlready Opus (skipped): %3\nFailed/Skipped: %4")
                             .arg(processedCount - 1)
                             .arg(successCount)
                             .arg(skippedCount)
                             .arg(failCount);
    } else {
        summaryMessage = QString("Opus Conversion Complete.\n\nTotal Records: %1\nSuccessfully Converted: %2\nAlready Opus (skipped): %3\nFailed/Skipped: %4")
                             .arg(totalFiles) // Use the count obtained earlier
                             .arg(successCount)
                             .arg(skippedCount)
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
    applyMusicHeaderLabels(model);
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
    QString scriptName = "serverFtpCmdsPutTakeOver.sh";
    QString scriptPath = serverScriptPath(scriptName);

    qInfo() << "Attempting to execute upload script:" << scriptName << "->" << scriptPath;
     qDebug() << "Dependencies: Script must exist and ~/.netrc must be configured correctly.";


    if (scriptPath.isEmpty() || serverScriptIsTemplate(scriptPath)) {
         if (scriptPath.isEmpty()) {
             QMessageBox::critical(this, tr("Script Error"),
                                   tr("The required upload script '%1' was not found.\nInstall it under the application data directory (e.g. share/xfb/scripts) or set ServerScriptsPath in xfb.conf.").arg(scriptName));
         } else {
             QMessageBox::critical(this, tr("Script Not Configured"),
                                   tr("The script '%1' still contains the [IP]/[PORT] placeholders.\nEdit it with your server's address and port:\n%2").arg(scriptName, scriptPath));
         }
         // Leave the local XML file so the user can upload it manually.
         ui->bt_takeOver->setEnabled(true);
         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         return;
    }


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
    startServerScript(uploadProcess, scriptPath); // No arguments needed based on original code
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
    QString scriptName = "serverFtpCmdsCHKTakeOver.sh";
    QString scriptPath = serverScriptPath(scriptName);

    qInfo() << "Attempting to execute check script:" << scriptName << "->" << scriptPath;

    if (scriptPath.isEmpty()) {
        // Stop checking? Or keep retrying? Let's stop and warn.
        QMessageBox::critical(this, tr("Script Error"),
                              tr("The required check script '%1' was not found.\nInstall it under the application data directory (e.g. share/xfb/scripts) or set ServerScriptsPath in xfb.conf.\n\nTakeover verification stopped.").arg(scriptName));
        return;
    }
    if (serverScriptIsTemplate(scriptPath)) {
        QMessageBox::critical(this, tr("Script Not Configured"),
                              tr("The script '%1' still contains the [IP]/[PORT] placeholders.\nEdit it with your server's address and port:\n%2\n\nTakeover verification stopped.").arg(scriptName, scriptPath));
        return;
    }

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
    startServerScript(checkProcess, scriptPath);
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
        refreshTransportAccessibleState();
        announceAccessible(tr("Recording paused"));

    } else {
        recPause=false;
        ui->bt_pause_rec->setStyleSheet("");
        audioRecorder->record();
        recTimer->start();
        refreshTransportAccessibleState();
        announceAccessible(tr("Recording resumed"));
    }

}

void player::on_bt_pause_play_clicked()
{

    if(playPause==false){
        playPause=true;
        ui->bt_pause_play->setStyleSheet("background-color:yellow");

        Xplayer->pause();
        refreshTransportAccessibleState();
        announceAccessible(tr("Paused"));

    } else {
        playPause=false;
        ui->bt_pause_play->setStyleSheet("");
        Xplayer->play();
        refreshTransportAccessibleState();
        announceAccessible(tr("Resumed"));

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


// --------------------------------------------------------------- small screens
//
// The player panel (frame_4) was drawn in Qt Designer with every control at a
// fixed pixel position inside a 1060-px-wide frame. Docked next to the clock on
// a laptop screen there is nowhere near that much room, so the right-hand end
// of the panel — including the progress slider — simply disappeared under the
// clock beside it.
//
// The panel is not rebuilt out of nested layouts because three of its controls
// are deliberately drawn *on top of* others (the record LED and the record
// pause button sit inside the Record button; the play pause button sits inside
// the Play button) and three more are status banners that cover the program
// area while a program is being made. A layout cannot express that. Instead the
// hand-made positions are recomputed from the panel's real width every time it
// changes, keeping the same design at whatever size it is given.
void player::relayoutPlayerFrame()
{
    if (!ui || !ui->frame_4)
        return;

    // Moving a child can make the frame itself be measured again, and the
    // measurement arrives as another resize: without this guard the panel
    // lays itself out inside its own layout pass until the stack runs out.
    if (m_relayoutingPlayerFrame)
        return;
    QScopedValueRollback<bool> guard(m_relayoutingPlayerFrame, true);

    const int w = ui->frame_4->width();
    const int h = ui->frame_4->height();
    if (w <= 0 || h <= 0)
        return;

    constexpr int kMargin = 9;
    // The design geometry these ratios come from: the transport block runs
    // from x=9 to x=581, the program block from x=510 to x=1051.
    constexpr double kTransportSpan = 572.0;
    constexpr double kProgramSpan   = 541.0;

    auto place = [](QWidget *widget, int x, int y, int width, int height) {
        if (widget)
            widget->setGeometry(x, y, qMax(1, width), qMax(1, height));
    };

    // The transport keeps its designed size while the panel is wide enough for
    // it, is never allowed more than a bit over half the panel, and below that
    // shrinks as one block so the buttons keep their proportions.
    const double transportScale =
        qBound(0.42, qMin(1.0, ((w - 2 * kMargin) * 0.60) / kTransportSpan), 1.0);
    auto tx = [&](int designX) { return kMargin + int((designX - 9) * transportScale); };
    auto tw = [&](int designW) { return qMax(18, int(designW * transportScale)); };

    // Row 1, left: play / next / stop / record.
    const int playX = tx(9), playW = tw(221);
    place(ui->btPlay, playX, 10, playW, 55);
    place(ui->btPlayNext, tx(239), 10, tw(171), 25);
    place(ui->btStop, tx(239), 40, tw(171), 25);
    const int recX = tx(420), recW = tw(161);
    place(ui->bt_rec, recX, 40, recW, 25);
    place(ui->txt_recTime, recX, 10, tw(150), 20);

    // The three controls that ride on top of the two big buttons are anchored
    // to those buttons rather than scaled, so they stay legible and stay
    // inside their host at every size.
    place(ui->bt_pause_play, playX + playW - 29, 36, 25, 25);
    place(ui->led_rec, recX + int(45 * transportScale), 43, 22, 20);
    place(ui->bt_pause_rec, recX + recW - 31, 42, 25, 19);

    // Row 1, right: the program area gets whatever is left.
    const int programLeft  = qMin(tx(581) + 12, qMax(kMargin, w - 150));
    const int programWidth = qMax(110, w - kMargin - programLeft);
    auto px = [&](int designX) {
        return programLeft + int((designX - 510) * programWidth / kProgramSpan);
    };
    auto pw = [&](int designW) {
        return qMax(24, int(designW * programWidth / kProgramSpan));
    };
    place(ui->txt_ProgramName, programLeft, 10, programWidth, 37);
    place(ui->bt_ProgramStopandProcess, px(750), 10, pw(301), 34);
    place(ui->txtDuration, programLeft, 45, programWidth, 20);
    // Status banners, drawn over the program area while they are shown.
    place(ui->txt_uploadingPrograms, programLeft, 0, programWidth, 51);
    place(ui->txt_creatingPrograms, programLeft, 0, programWidth, 51);
    place(ui->txt_loading, programLeft, 0, programWidth, 56);

    // Row 2: the volume and progress sliders, full width — this is the row
    // that used to run off the right-hand edge.
    const int slidersY = (h >= 121) ? 66 : qBound(40, h - 55, 66);
    place(ui->layoutWidget1, kMargin + 1, slidersY, w - 2 * (kMargin + 1), 27);

    // Row 3: the now-playing line, full width.
    const int nowPlayingY = qBound(slidersY + 28, (h >= 121) ? 90 : h - 31, qMax(1, h - 20));
    place(ui->txtNowPlaying, kMargin + 11, nowPlayingY, w - 2 * (kMargin + 11), 31);
}

// The clock panel has the same hand-placed contents as the player panel, and
// the same problem with them: the 8-digit readout was drawn 301 px wide, so on
// a narrow panel the seconds fell off the right-hand edge. QLCDNumber scales
// its digits to whatever room it is given, so filling the panel is all this
// takes.
void player::relayoutClockFrame()
{
    if (!ui || !ui->frame)
        return;

    const int w = ui->frame->width();
    const int h = ui->frame->height();
    if (w <= 0 || h <= 0)
        return;

    constexpr int kMargin = 8;
    const int inner = qMax(40, w - 2 * kMargin);

    // The Auto Mode button keeps its designed height; the readout takes the
    // rest, so a shorter panel loses digit height rather than the button.
    const int buttonHeight = qMin(31, qMax(20, h - 30));
    const int clockHeight = qMax(24, h - buttonHeight - 3 * kMargin);

    if (ui->txt_horas)
        ui->txt_horas->setGeometry(kMargin, kMargin, inner, clockHeight);
    if (ui->bt_autoMode)
        ui->bt_autoMode->setGeometry(kMargin, kMargin + clockHeight + kMargin,
                                     inner, buttonHeight);
}

// The wave strip above the playlist draws the on-air track with a playhead
// running across it, which is the same information the progress slider carries
// — and on a small screen the slider's row is worth reclaiming. Hiding it
// leaves the volume slider the whole row. Config key HideProgressInWaveView
// (default true) turns the behaviour off for anyone who wants both.
void player::applyProgressBarVisibility()
{
    if (!ui || !ui->sliderProgress || !ui->label_2)
        return;

    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/xfb.conf", QSettings::IniFormat);
    const bool hideWithWave = settings.value("HideProgressInWaveView", true).toBool();
    const bool waveOn = m_waveViewToggle && m_waveViewToggle->isChecked();
    const bool show = !(waveOn && hideWithWave);

    ui->label_2->setVisible(show);
    ui->sliderProgress->setVisible(show);

    if (m_volumeRowSpacer) {
        m_volumeRowSpacer->changeSize(0, 0,
                                      show ? QSizePolicy::Fixed
                                           : QSizePolicy::Expanding,
                                      QSizePolicy::Minimum);
        ui->horizontalLayout_7->invalidate();
    }
}

QScrollArea *player::wrapInScrollArea(QWidget *content, QWidget *parent)
{
    auto *area = new QScrollArea(parent);
    area->setFrameShape(QFrame::NoFrame);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (content && !content->layout()) {
        // A widget laid out by hand reports no size of its own, so the scroll
        // area would squash it to nothing and never scroll. The box its
        // children actually occupy is the size it needs.
        //
        // childrenRect() would give that box, except it leaves out anything
        // currently hidden — and several of these panes keep controls hidden
        // until they are needed, which would shrink the page and strand them
        // again the moment they appeared.
        QRect used;
        const QObjectList children = content->children();
        for (QObject *child : children) {
            if (auto *widget = qobject_cast<QWidget *>(child))
                used |= widget->geometry();
        }
        if (used.isValid())
            content->setMinimumSize(used.right() + 8, used.bottom() + 8);

        // The hand-placed pane is parked inside a holder that *does* have a
        // layout, and the holder is what the scroll area resizes. Handing a
        // layout-less widget straight to a resizable scroll area sets up a
        // loop — the area resizes the widget, the widget's new size is
        // measured again, the area resizes it again — which ends as a stack
        // overflow rather than as a wrong-looking panel.
        auto *holder = new QWidget(area);
        auto *box = new QVBoxLayout(holder);
        box->setContentsMargins(0, 0, 0, 0);
        box->setSpacing(0);
        content->setParent(holder);
        box->addWidget(content, 0);
        box->addStretch(1);
        area->setWidgetResizable(true);
        area->setWidget(holder);
        return area;
    }

    area->setWidgetResizable(true);
    area->setWidget(content);
    return area;
}

// A QTabWidget is as tall and as wide as its most demanding page, whichever
// page happens to be on show. The Torrents tab — a toolbar, a search row, a
// results table and a downloads panel — is far and away the largest of the
// library tabs, and it was holding the whole library panel at a size the
// operator could barely drag: the Musics tab needs 66 px, Torrents insists on
// 226. Moving a tab's contents into a scroll area inside the tab lets the
// panel be dragged down to whatever the operator wants, and the tab that
// needs the room scrolls instead of dictating to the other four.
void player::makeTabScrollable(QWidget *tab)
{
    if (!tab)
        return;
    QLayout *inner = tab->layout();
    if (!inner || qobject_cast<QScrollArea *>(tab->findChild<QScrollArea *>()))
        return;

    // setLayout() takes the layout off its old parent widget, bringing the
    // laid-out children with it, so the tab is left free for a new one.
    auto *content = new QWidget;
    content->setLayout(inner);

    auto *outer = new QVBoxLayout(tab);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(wrapInScrollArea(content, tab));
}

// Every page of the side toolbox (Search / Filters / Extras / Playlist) is
// another hand-positioned pane, and the pages are taller than the panel is on
// a laptop: the controls at the bottom were simply off-screen, reachable only
// by collapsing the library underneath — which nobody would guess at. Each
// page goes into a scroll area, which both scrolls when it has to and lets the
// panel be dragged narrow without a fight.
void player::makeSidePanelScrollable()
{
    QToolBox *box = ui ? ui->page_FTP_Connection : nullptr;
    if (!box)
        return;

    const int pages = box->count();
    for (int index = 0; index < pages; ++index) {
        QWidget *page = box->widget(index);
        if (!page || qobject_cast<QScrollArea *>(page))
            continue;

        const QString label = box->itemText(index);
        const QIcon icon = box->itemIcon(index);
        const QString tip = box->itemToolTip(index);

        box->removeItem(index);
        QScrollArea *area = wrapInScrollArea(page, box);
        box->insertItem(index, area, icon, label);
        box->setItemToolTip(index, tip);
    }
    box->setCurrentIndex(0);
}

void player::setLayoutLocked(bool locked)
{
    m_layoutLocked = locked;
    const QList<QDockWidget *> docks = {m_dockPlayer, m_dockClock,
                                        m_dockSide, m_dockLibrary};
    for (QDockWidget *dock : docks) {
        if (!dock)
            continue;
        if (locked && !dock->isFloating()) {
            // An empty title-bar widget hides the bar entirely, so the
            // locked layout looks like one seamless window.
            if (!dock->titleBarWidget())
                dock->setTitleBarWidget(new QWidget(dock));
            dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
        } else {
            QWidget *old = dock->titleBarWidget();
            dock->setTitleBarWidget(nullptr);
            delete old;
            dock->setFeatures(QDockWidget::DockWidgetMovable
                              | QDockWidget::DockWidgetFloatable
                              | QDockWidget::DockWidgetClosable);
        }
    }
}

void player::resetDockLayout()
{
    if (m_defaultLayoutState.isEmpty())
        return;
    if (m_lockLayoutAction && m_lockLayoutAction->isChecked())
        m_lockLayoutAction->setChecked(false); // unlock so the reset is visible
    restoreState(m_defaultLayoutState, kLayoutStateVersion);
    const QList<QDockWidget *> docks = {m_dockPlayer, m_dockClock,
                                        m_dockSide, m_dockLibrary};
    for (QDockWidget *dock : docks) {
        if (dock) {
            dock->setFloating(false);
            dock->show();
        }
    }
}

void player::requestItemArtwork(QListWidgetItem *item, const QString &path)
{
    if (!item || !m_artStore)
        return;
    // Playlist rows carry the file path as their text; history rows carry
    // it in kArtworkPathRole (set when they are added).
    const QString file = !path.isEmpty()
                             ? path
                             : (item->data(kArtworkPathRole).isValid()
                                    ? item->data(kArtworkPathRole).toString()
                                    : item->text());
    if (file.isEmpty() || !QFileInfo::exists(file))
        return;
    if (const ArtworkData *art = m_artStore->fetch(file)) {
        if (art->ready())
            item->setIcon(QIcon(art->pixmap));
    }
    // Not cached yet: onArtworkReady() sets the icon when extraction ends
}

void player::onArtworkReady(const QString &path)
{
    const ArtworkData *art = m_artStore ? m_artStore->peek(path) : nullptr;
    if (!art || !art->ready() || !ui)
        return;
    const QIcon icon(art->pixmap);
    if (ui->playlist) {
        for (int i = 0; i < ui->playlist->count(); ++i) {
            QListWidgetItem *item = ui->playlist->item(i);
            if (item && item->text() == path)
                item->setIcon(icon);
        }
    }
    if (ui->historyList) {
        for (int i = 0; i < ui->historyList->count(); ++i) {
            QListWidgetItem *item = ui->historyList->item(i);
            if (item && item->data(kArtworkPathRole).toString() == path)
                item->setIcon(icon);
        }
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

    // Ensure Tor is available before attempting to connect. XFB no longer
    // ships a copy of Tor: the Torrents feature is off by default, so bundling
    // an unsigned tor.exe with pluggable transports made every install look
    // worse to Windows' reputation checks for a tool most operators never use.
    // TorNetworkService still finds a bundled, system or Tor Browser copy if
    // one is there; otherwise we offer to install it, once, with consent.
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
                   "Install it with your package manager — \"tor\" on Linux and "
                   "macOS, or the Tor Browser on Windows — and try again. XFB "
                   "picks up a system install, a Tor Browser install, or a copy "
                   "placed in its own \"tor\" folder."));
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

bool player::isVpnActive(QString *ifaceName) const
{
    // Heuristic: a tunnel-style interface that is up, running, not loopback,
    // and carries a real (non-link-local) IPv4 address. VPN detection can
    // never be certain, so we bias toward *not* reporting a VPN when unsure —
    // for a warn-only feature a missed VPN just shows an extra warning, while
    // a false positive would wrongly suppress it.
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)
                || !flags.testFlag(QNetworkInterface::IsRunning)
                || flags.testFlag(QNetworkInterface::IsLoopBack))
            continue;

        const QString name = iface.name();
        const QString human = iface.humanReadableName();
        const bool tunnelLike =
            name.startsWith("utun") ||   // macOS VPNs
            name.startsWith("tun")  ||   // OpenVPN / generic tun (Linux/mac)
            name.startsWith("tap")  ||
            name.startsWith("wg")   ||   // WireGuard
            name.startsWith("ppp")  ||   // PPTP / L2TP
            name.startsWith("ipsec")||
            human.contains("VPN", Qt::CaseInsensitive) ||
            human.contains("WireGuard", Qt::CaseInsensitive) ||
            human.contains("OpenVPN", Qt::CaseInsensitive) ||
            human.contains("TAP", Qt::CaseInsensitive);
        if (!tunnelLike)
            continue;

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol
                    && !ip.isLoopback() && !ip.isLinkLocal()) {
                if (ifaceName)
                    *ifaceName = name;
                return true;
            }
        }
    }
    return false;
}

bool player::confirmDownloadNetworkExposure()
{
    if (isVpnActive())
        return true; // a VPN is up — proceed without nagging

    // No VPN: warn, but never block (the user chose warn-only).
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("No VPN detected"));
    box.setText(tr("You are about to download a torrent without a VPN."));
    box.setInformativeText(tr(
        "BitTorrent transfers cannot be routed through Tor. Without a VPN, your "
        "real IP address is visible to the tracker and to the other peers sharing "
        "this file.\n\n"
        "For more privacy, connect a VPN that allows peer-to-peer traffic and then "
        "start the download again.\n\n"
        "Download anyway?"));
    QPushButton *proceed = box.addButton(tr("Download anyway"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    return box.clickedButton() == proceed;
}

void player::updateVpnStatusLabel()
{
    if (!ui || !ui->vpnStatusLabel)
        return;
    QString iface;
    if (isVpnActive(&iface)) {
        ui->vpnStatusLabel->setText(tr("VPN: on (%1)").arg(iface));
        ui->vpnStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        ui->vpnStatusLabel->setToolTip(tr("A VPN tunnel is active — torrent downloads egress through it."));
    } else {
        ui->vpnStatusLabel->setText(tr("VPN: off"));
        ui->vpnStatusLabel->setStyleSheet("color: #d08000; font-weight: bold;");
        ui->vpnStatusLabel->setToolTip(tr("No VPN detected. Downloading exposes your real IP to peers; "
                                          "connect a VPN that allows P2P for more privacy."));
    }
}

void player::ensureTorrentServices()
{
    // Idempotent: the services live for the rest of the session once created.
    // Toggling the feature off stops their activity (shutdownTorrentActivity)
    // but does not destroy them, so these signal/button connections are made
    // exactly once and never double-fire.
    if (m_torNetworkService)
        return;

    m_torNetworkService = new TorNetworkService(this);
    m_torrentSearchService = new TorrentSearchService(this);
    m_torrentDownloadService = new TorrentDownloadService(this);

    if (m_torNetworkService && !m_torNetworkService->initialize())
        qWarning() << "Failed to initialize TorNetworkService";
    if (m_torrentSearchService && !m_torrentSearchService->initialize())
        qWarning() << "Failed to initialize TorrentSearchService";
    if (m_torrentDownloadService && !m_torrentDownloadService->initialize())
        qWarning() << "Failed to initialize TorrentDownloadService";

    if (m_torrentSearchService && m_torNetworkService)
        m_torrentSearchService->setTorService(m_torNetworkService);
    if (m_torrentDownloadService && m_torNetworkService)
        m_torrentDownloadService->setTorService(m_torNetworkService);

    // Tor network signals
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

    // Torrent search signals
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

    // Torrent download signals
    if (m_torrentDownloadService) {
        connect(m_torrentDownloadService, &TorrentDownloadService::downloadCompleted,
                this, &player::onTorrentDownloadCompleted);
        connect(m_torrentDownloadService, &TorrentDownloadService::streamingReady,
                this, &player::onTorrentStreamingReady);
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

        ui->downloadsTableView->setModel(m_torrentDownloadService->getDownloadsModel());
        ui->downloadsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        ui->downloadsTableView->horizontalHeader()->setStretchLastSection(true);
        ui->downloadsTableView->setColumnWidth(0, 300);  // Name
        ui->downloadsTableView->setColumnWidth(1, 80);   // Progress
        ui->downloadsTableView->setColumnWidth(2, 120);  // Status

        // Downloads-panel buttons
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
            if (m_torrentDownloadService->activeDownloadCount() == 0)
                ui->downloadsPanel->setVisible(false);
        });
        connect(ui->downloadOpenFolderButton, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_torrentDownloadService->downloadDirectory()));
        });
        connect(ui->downloadRetryButton, &QPushButton::clicked, this, [this]() {
            QModelIndex idx = ui->downloadsTableView->currentIndex();
            if (idx.isValid()) {
                QString dlId = m_torrentDownloadService->downloadIdForRow(idx.row());
                if (!dlId.isEmpty()) {
                    if (m_torrentDownloadService->retryDownload(dlId))
                        ui->statusBar->showMessage(tr("Retrying download..."), 3000);
                    else
                        ui->statusBar->showMessage(tr("Cannot retry this download"), 3000);
                }
            }
        });
    }

    if (m_torrentSearchService)
        ui->torrentsView->setModel(m_torrentSearchService->getResultsModel());

    // Live VPN indicator: refresh the status label every few seconds so it
    // tracks the user connecting/dropping a VPN while the tab is open.
    if (!m_vpnStatusTimer) {
        m_vpnStatusTimer = new QTimer(this);
        m_vpnStatusTimer->setInterval(4000);
        connect(m_vpnStatusTimer, &QTimer::timeout, this, &player::updateVpnStatusLabel);
    }
    m_vpnStatusTimer->start();
    updateVpnStatusLabel();
}

void player::shutdownTorrentActivity()
{
    // Turning the feature off must stop all torrent network activity: cancel
    // any running downloads (kills the aria2c/transmission processes) and drop
    // the Tor connection. The service objects stay alive but idle — they bind
    // no ports and make no requests until explicitly driven again.
    if (m_torrentDownloadService)
        m_torrentDownloadService->cancelAllDownloads();
    if (m_torNetworkService)
        m_torNetworkService->disconnectFromTor();
    if (m_vpnStatusTimer)
        m_vpnStatusTimer->stop();
    updateTorConnectionUI(false);
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
            // Be precise: Tor anonymises SEARCH only, not the download.
            ui->torConnectionStatus->setText(tr("Status: Search anonymised via Tor — downloads are NOT anonymous"));
            ui->torConnectionStatus->setStyleSheet("color: green; font-weight: bold;");
        }
        if (ui->torrentWarningLabel) {
            ui->torrentWarningLabel->setText(tr("⚠️ Search is routed through Tor. Downloading is NOT: while a download runs your real IP is visible to the tracker and peers. Only download content you are legally entitled to."));
        }
    } else {
        if (ui->torConnectButton) ui->torConnectButton->setText(tr("Connect to Tor"));
        if (ui->torConnectionStatus) {
            ui->torConnectionStatus->setText(tr("Status: Disconnected"));
            ui->torConnectionStatus->setStyleSheet("color: red; font-weight: bold;");
        }
        if (ui->torrentWarningLabel) {
            ui->torrentWarningLabel->setText(tr("⚠️ Only download content you are legally entitled to. Click \"Connect to Tor\" to anonymise searching (note: downloading is never anonymous)."));
        }
    }
}

void player::onTorReady()
{
    updateTorConnectionUI(true);
    QMessageBox::information(this, tr("Tor Connected"),
                            tr("Connected to the Tor network. Your torrent SEARCHES are now anonymised.\n\n"
                               "Downloading a torrent is still not anonymous — during the transfer your "
                               "real IP is visible to the tracker and peers."));
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
                                 "Your search traffic will still be routed through the Tor network, "
                                 "but will exit through a Tor exit node to reach the clearnet site. "
                                 "(Downloading remains non-anonymous either way.)"));
    
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


// A song that sits in the database twice is a song that can be drawn twice in
// the same hour, that counts twice towards the rotation separation rules, and
// that ends up half-measured — a loudness or intro sweep updates whichever of
// the two rows it walked into. This collapses every artist + song name pair
// down to a single record.
//
// Records only. The audio files are never touched: an operator who keeps two
// encodes of the same song on purpose still has both on disk, and deleting
// audio is not a thing a menu item should do quietly.
void player::on_actionRemove_duplicate_songs_triggered()
{
    // The menu entry is disabled for a session without this, but the slot
    // is also reachable from a shortcut and from other code, and what it
    // does cannot be taken back.
    if (!AccessControl::instance().demand(QStringLiteral("library.duplicates"), this))
        return;

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Database connection is not open."));
        return;
    }

    // What is worth carrying off a record before it is deleted. Taken from the
    // live schema rather than hard-coded, because every one of these arrived as
    // a migration and a database that predates one of them must not turn this
    // into a failed SELECT.
    static const char *const kMergeableColumns[] = {
        "bpm", "lufs", "true_peak", "loudness_mtime", "intro_ms", "outro_ms"
    };
    const QSqlRecord schema = db.record(QStringLiteral("musics"));
    QStringList mergeColumns;
    for (const char *name : kMergeableColumns) {
        if (schema.contains(QString::fromLatin1(name)))
            mergeColumns << QString::fromLatin1(name);
    }
    const bool haveIntroLocked = schema.contains(QStringLiteral("intro_locked"));

    struct Record {
        qint64 rowid = 0;
        qint64 id = 0;
        QString artist;
        QString song;
        QString path;
        int playedTimes = 0;
        QString lastPlayed;
        int introLocked = 0;
        QVariantList extras;      // parallel to mergeColumns
        bool fileExists = false;
    };

    QStringList columns{QStringLiteral("rowid"), QStringLiteral("id"),
                        QStringLiteral("artist"), QStringLiteral("song"),
                        QStringLiteral("path"), QStringLiteral("played_times"),
                        QStringLiteral("last_played")};
    const int extrasOffset = columns.size();
    columns += mergeColumns;
    if (haveIntroLocked)
        columns << QStringLiteral("intro_locked");

    QSqlQuery select(db);
    if (!select.exec(QStringLiteral("SELECT %1 FROM musics ORDER BY rowid")
                         .arg(columns.join(QStringLiteral(", "))))) {
        qWarning() << "Failed to read the musics table:" << select.lastError().text();
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to read the musics table."));
        return;
    }

    // Grouped in C++ rather than with SQL's GROUP BY: SQLite's LOWER() folds
    // ASCII and nothing else, so "ÁGUA" and "Água" would walk out of a query
    // as two different songs. QString::toLower() knows the rest of Unicode,
    // and simplified() collapses the stray double spaces that come with
    // metadata somebody typed.
    QMap<QString, QList<Record>> groups;
    while (select.next()) {
        Record r;
        r.rowid       = select.value(0).toLongLong();
        r.id          = select.value(1).toLongLong();
        r.artist      = select.value(2).toString();
        r.song        = select.value(3).toString();
        r.path        = select.value(4).toString();
        r.playedTimes = select.value(5).toInt();
        r.lastPlayed  = select.value(6).toString();
        for (int i = 0; i < mergeColumns.size(); ++i)
            r.extras << select.value(extrasOffset + i);
        if (haveIntroLocked)
            r.introLocked = select.value(extrasOffset + mergeColumns.size()).toInt();
        r.fileExists = !r.path.isEmpty() && QFile::exists(r.path);

        const QString key = r.artist.simplified().toLower()
                            + QChar(0x1f)          // never occurs in a title
                            + r.song.simplified().toLower();
        groups[key].append(r);
    }

    // --- Work out what would go, and show it before anything moves ---
    struct Removal {
        Record keep;
        QList<Record> drop;
    };
    QList<Removal> removals;
    int recordsToRemove = 0;
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        const QList<Record> &group = it.value();
        if (group.size() < 2)
            continue;

        // The keeper is the oldest record whose file is still on disk. Oldest,
        // because the rest of XFB has been referring to it for longer — the
        // as-run log, the rotation rules and any pending request all point at
        // that row. On disk, because a record whose file is gone cannot be the
        // one survivor of a song. If none of them still has a file, the oldest
        // wins anyway: this removes duplicates, not missing files, and the
        // database check beside it in the menu is what clears those out.
        int keepIndex = 0;
        for (int i = 1; i < group.size(); ++i) {
            if (group.at(i).fileExists && !group.at(keepIndex).fileExists) {
                keepIndex = i;
                break;
            }
        }

        Removal removal;
        removal.keep = group.at(keepIndex);
        for (int i = 0; i < group.size(); ++i) {
            if (i != keepIndex)
                removal.drop.append(group.at(i));
        }
        recordsToRemove += removal.drop.size();
        removals.append(removal);
    }

    if (removals.isEmpty()) {
        QMessageBox::information(this, tr("Remove Duplicates"),
                                 tr("No duplicates found — every artist and song "
                                    "name in the database appears exactly once."));
        return;
    }

    // The whole list would be a wall of text on a library that has drifted for
    // years, so the dialog shows a readable slice and the log keeps all of it.
    const int kPreviewGroups = 40;
    QStringList preview;
    for (int i = 0; i < removals.size(); ++i) {
        const Removal &r = removals.at(i);
        qInfo().noquote() << "Duplicate:" << r.keep.artist << "-" << r.keep.song;
        qInfo().noquote() << "    keep" << r.keep.path;
        if (i < kPreviewGroups)
            preview << QStringLiteral("%1 — %2\n    %3 %4")
                           .arg(r.keep.artist, r.keep.song, tr("keep"), r.keep.path);
        for (const Record &d : r.drop) {
            qInfo().noquote() << "    drop" << d.path;
            if (i < kPreviewGroups)
                preview << QStringLiteral("    %1 %2").arg(tr("drop"), d.path);
        }
    }
    if (removals.size() > kPreviewGroups) {
        preview << QString()
                << tr("… and %n more song(s), all of them listed in xfb.log.",
                      nullptr, removals.size() - kPreviewGroups);
    }

    QMessageBox confirm(this);
    confirm.setIcon(QMessageBox::Question);
    confirm.setWindowTitle(tr("Remove Duplicates"));
    confirm.setText(tr("%n song(s) appear more than once in the database.",
                       nullptr, removals.size()));
    confirm.setInformativeText(
        tr("Remove %n duplicate record(s), keeping one of each song?\n\n"
           "Play counts and any measured BPM, loudness and intro times are "
           "merged onto the record that stays. The audio files themselves are "
           "not deleted — only the database records.",
           nullptr, recordsToRemove));
    confirm.setDetailedText(preview.join(QLatin1Char('\n')));
    confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirm.setDefaultButton(QMessageBox::No);
    if (confirm.exec() != QMessageBox::Yes)
        return;

    // --- Prepare the statements once, outside the loop ---
    QStringList assignments{QStringLiteral("played_times = :played_times"),
                            QStringLiteral("last_played = :last_played")};
    for (const QString &column : mergeColumns)
        assignments << QStringLiteral("%1 = :%1").arg(column);
    if (haveIntroLocked)
        assignments << QStringLiteral("intro_locked = :intro_locked");

    QSqlQuery mergeInto(db);
    QSqlQuery deleteRecord(db);
    if (!mergeInto.prepare(QStringLiteral("UPDATE musics SET %1 WHERE rowid = :rowid")
                               .arg(assignments.join(QStringLiteral(", "))))
        || !deleteRecord.prepare(QStringLiteral("DELETE FROM musics WHERE rowid = :rowid"))) {
        qWarning() << "Failed to prepare the duplicate statements:"
                   << mergeInto.lastError().text() << deleteRecord.lastError().text();
        QMessageBox::critical(this, tr("Database Error"),
                              tr("Failed to prepare the database queries."));
        return;
    }

    // Rotation rules are keyed to musics.id, pending requests to musics.rowid.
    // A record that is about to stop existing hands both over to the one that
    // stays, so a song does not fall out of its category for having been added
    // twice. UPDATE OR IGNORE is what keeps the rotation move honest: music_id
    // is the primary key there, so a keeper that already has rules of its own
    // simply keeps them.
    const QStringList tables = db.tables();
    const bool haveRotation = tables.contains(QStringLiteral("rotation"));
    const bool haveRequests = tables.contains(QStringLiteral("requests"));
    QSqlQuery moveRotation(db), dropRotation(db), moveRequests(db);
    if (haveRotation) {
        moveRotation.prepare(QStringLiteral(
            "UPDATE OR IGNORE rotation SET music_id = :keep WHERE music_id = :drop"));
        dropRotation.prepare(QStringLiteral("DELETE FROM rotation WHERE music_id = :drop"));
    }
    if (haveRequests) {
        moveRequests.prepare(QStringLiteral(
            "UPDATE requests SET music_id = :keep WHERE music_id = :drop"));
    }

    // The three stamps XFB has written into musics.last_played over the years.
    auto parseLastPlayed = [](const QString &text) -> QDateTime {
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty() || trimmed == QLatin1String("-"))
            return QDateTime();
        QDateTime dt = QDateTime::fromString(trimmed,
                                             QStringLiteral("yyyy-MM-dd || hh:mm:ss"));
        if (!dt.isValid())
            dt = QDateTime::fromString(trimmed, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        if (!dt.isValid())
            dt = QDateTime::fromString(trimmed, Qt::ISODate);
        return dt;
    };

    QProgressDialog progress(tr("Removing duplicate songs…"), tr("Cancel"),
                             0, removals.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(0);

    // One transaction for the lot: a cancel or a failure half way through
    // would otherwise leave a library with rotation rules pointing at records
    // that are gone.
    const bool inTransaction = db.transaction();
    if (!inTransaction)
        qWarning() << "Could not open a transaction; removing duplicates one by one.";

    int removedCount = 0;
    int mergedCount = 0;
    int failedCount = 0;
    bool cancelled = false;

    for (int i = 0; i < removals.size(); ++i) {
        progress.setValue(i);
        qApp->processEvents();
        if (progress.wasCanceled()) {
            cancelled = true;
            break;
        }

        const Removal &removal = removals.at(i);
        const Record &keep = removal.keep;

        // Merge first, so that nothing measured is lost with the record that
        // carried it.
        int playedTimes = keep.playedTimes;
        QString lastPlayed = keep.lastPlayed;
        QDateTime lastPlayedAt = parseLastPlayed(keep.lastPlayed);
        QVariantList extras = keep.extras;
        int introLocked = keep.introLocked;

        for (const Record &drop : removal.drop) {
            playedTimes = qMax(playedTimes, drop.playedTimes);
            const QDateTime dropAt = parseLastPlayed(drop.lastPlayed);
            if (dropAt.isValid() && (!lastPlayedAt.isValid() || dropAt > lastPlayedAt)) {
                lastPlayedAt = dropAt;
                lastPlayed = drop.lastPlayed;
            }
            for (int c = 0; c < mergeColumns.size(); ++c) {
                // NULL is "never measured" for all of these. BPM counts zero as
                // never measured too — the auto-mix chooser only ever matches
                // on bpm > 0, so a stored 0 is not a tempo, it is an absence.
                const bool missing = extras.at(c).isNull()
                                     || (mergeColumns.at(c) == QLatin1String("bpm")
                                         && extras.at(c).toDouble() <= 0.0);
                if (!missing || drop.extras.at(c).isNull())
                    continue;
                extras[c] = drop.extras.at(c);
                // intro_locked belongs to intro_ms: it says the marker beside
                // it was placed by hand rather than measured. Taking one
                // without the other would either freeze a number nobody chose
                // or unlock one somebody did, so the pair travels together.
                if (mergeColumns.at(c) == QLatin1String("intro_ms"))
                    introLocked = drop.introLocked;
            }
        }

        mergeInto.bindValue(QStringLiteral(":played_times"), playedTimes);
        mergeInto.bindValue(QStringLiteral(":last_played"), lastPlayed);
        for (int c = 0; c < mergeColumns.size(); ++c)
            mergeInto.bindValue(QStringLiteral(":%1").arg(mergeColumns.at(c)), extras.at(c));
        if (haveIntroLocked)
            mergeInto.bindValue(QStringLiteral(":intro_locked"), introLocked);
        mergeInto.bindValue(QStringLiteral(":rowid"), keep.rowid);
        if (!mergeInto.exec()) {
            qWarning() << "Failed to merge onto" << keep.path << ":"
                       << mergeInto.lastError().text();
            failedCount += removal.drop.size();
            continue;               // leave the duplicates rather than lose data
        }
        mergedCount++;

        for (const Record &drop : removal.drop) {
            if (haveRotation) {
                moveRotation.bindValue(QStringLiteral(":keep"), keep.id);
                moveRotation.bindValue(QStringLiteral(":drop"), drop.id);
                if (!moveRotation.exec())
                    qWarning() << "Failed to move rotation rules:" << moveRotation.lastError().text();
                dropRotation.bindValue(QStringLiteral(":drop"), drop.id);
                if (!dropRotation.exec())
                    qWarning() << "Failed to drop rotation rules:" << dropRotation.lastError().text();
            }
            if (haveRequests) {
                moveRequests.bindValue(QStringLiteral(":keep"), keep.rowid);
                moveRequests.bindValue(QStringLiteral(":drop"), drop.rowid);
                if (!moveRequests.exec())
                    qWarning() << "Failed to move requests:" << moveRequests.lastError().text();
            }

            deleteRecord.bindValue(QStringLiteral(":rowid"), drop.rowid);
            if (deleteRecord.exec()) {
                qInfo().noquote() << "Removed duplicate record:" << drop.path;
                removedCount++;
            } else {
                qWarning() << "Failed to remove" << drop.path << ":"
                           << deleteRecord.lastError().text();
                failedCount++;
            }
        }
    }

    progress.setValue(removals.size());

    if (inTransaction) {
        // A cancel keeps what has already been merged and removed rather than
        // throwing it away: every group is finished before the next one
        // starts, so the half that ran is as consistent as the whole would be.
        if (!db.commit()) {
            qWarning() << "Failed to commit the duplicate removal:" << db.lastError().text();
            db.rollback();
            QMessageBox::critical(this, tr("Database Error"),
                                  tr("The duplicates could not be removed — the "
                                     "database was left untouched.\n\n%1")
                                      .arg(db.lastError().text()));
            return;
        }
    }

    update_music_table();

    QString summary = tr("Removed %n duplicate record(s).", nullptr, removedCount)
                      + QLatin1Char('\n')
                      + tr("Songs merged: %1").arg(mergedCount);
    if (failedCount > 0)
        summary += QLatin1Char('\n') + tr("Failed: %1 (see xfb.log)").arg(failedCount);
    if (cancelled)
        summary += QLatin1Char('\n') + tr("Cancelled — the rest were left alone.");
    summary += QStringLiteral("\n\n") + tr("No audio files were deleted.");

    qInfo().noquote() << "Duplicate removal:" << QString(summary).replace('\n', QLatin1String(" | "));
    QMessageBox::information(this, tr("Remove Duplicates"), summary);
}


// End of player.cpp
