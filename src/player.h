#ifndef PLAYER_H
#define PLAYER_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaRecorder>
#include <QMediaFormat>
#include <QMediaCaptureSession>
#include <QAudioOutput>
#include <QAudioInput>
#include <QSqlDatabase>
#include <QUrl>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QSet>
#include <QProcess>
#include <QVariantAnimation>
#include <QVector>

// Forward declarations (prefer these over heavy includes in headers)
class QAction;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QListWidgetItem;
class QPushButton;
class QSlider;
class QSpinBox;
class QTableView;
class QLabel;
class QFrame;
class QNetworkReply;
class QSqlQueryModel;
class QNetworkAccessManager;
class QProgressDialog;
class QQuickWidget;
class QToolButton;
class QSplitter;
class QScrollArea;
class QSpacerItem;
class QMovie;
class QJsonObject;

// Project forward declarations
class TorNetworkService;
class TorrentSearchService;
class TorrentDownloadService;
class NgrokTunnelService;
class UpdateCheckService;
class DonationNotice;
class AudioFxWidget;
class WaveformStore;
class IntroLibrary;
struct IntroTimes;
class PlaylistWaveView;
class BpmLibrary;
class CueBus;
class LoudnessScanner;
struct LoudnessMeasurement;
class QStyledItemDelegate;
class NowPlayingWaveStrip;
class LevelMeter;
class ArtworkStore;
class NowPlayingArtPanel;
class PadBoardWidget;

#include "services/TorrentTypes.h"
// For MobileSyncServer::NowPlaying, the struct publicNowPlaying() returns.
#include "services/MobileSyncServer.h"
#include "audio/FxPlayer.h"

namespace Ui {
class player;
}

class player : public QMainWindow
{
    Q_OBJECT
    friend class PlayerUIController;

public:
    explicit player(QWidget *parent = nullptr);
    ~player();

protected:
    void closeEvent(QCloseEvent *event) override;
    /** Turns the LP platter labels into scratchable jog wheels. */
    bool eventFilter(QObject *watched, QEvent *event) override;

public:
    // Public state
    int onAbout2Finish = 0;
    QSqlDatabase adb;
    QString saveFile;
    QTimer *recTimer = nullptr;
    int recSecs = 0;
    int recMins = 0;
    int recHours = 0;
    int server_this_day_of_the_week = 0;
    QString lp1_total_time;
    int lp1_total_time_int = 0;
    QString lp2_total_time;
    int lp2_total_time_int = 0;
    QTimer *stimer = nullptr;
    QTimer *icetimer = nullptr;
    QTimer *butt_timer = nullptr;
    QTimer *adRefreshTimer = nullptr;
    
    // UI accessor methods for controllers
    QTableView* getMusicView() const;
    QPushButton* getPlayButton() const;
    QPushButton* getStopButton() const;
    QSlider* getProgressSlider() const;
    QSlider* getVolumeSlider() const;
    int jingleCadaNumMusicas;

public slots:
    bool checkDbOpen();
    void update_music_table();
    void checkForUpdates();
    void refreshAdBanner(); // New slot to refresh the ad banner

private slots:
    void on_actionOpen_triggered();
    void on_btPlay_clicked();
    void on_sliderProgress_sliderReleased();
    void on_sliderVolume_sliderMoved(int position);
    void on_btStop_clicked();
    // Update these method signatures to work with Qt6
    void onPositionChanged(qint64 position);
    void durationChanged(qint64 position);
    void currentMediaChanged(const QUrl &content); // Changed from QMediaContent
    void volumeChanged(float volume);
    void lp1_onPositionChanged(qint64 position);
    void lp1_durationChanged(qint64 position);
    void lp1_currentMediaChanged(const QUrl &content); // Changed from QMediaContent
    void lp1_volumeChanged(float volume);
    void lp2_onPositionChanged(qint64 position);
    void lp2_durationChanged(qint64 position);
    void lp2_currentMediaChanged(const QUrl &content); // Changed from QMediaContent
    void lp2_volumeChanged(float volume);

    // Add new methods for playlist management
    void playNextMedia(); // New method to handle playlist progression
    void playPreviousMedia(); // New method to handle playlist progression
    void playlistAboutToFinish();
    void playNextSong();
    void showTime();
    void playlistContextMenu(const QPoint&);
    void musicViewContextMenu(const QPoint&);
    void jinglesViewContextMenu(const QPoint&);
    void pubViewContextMenu(const QPoint&);
    void programsViewContextMenu(const QPoint&);
    void torrentsViewContextMenu(const QPoint&);
    void dropEvent(QDropEvent *);
    void dragEnterEvent(QDragEnterEvent *);
    void on_musicView_pressed(const QModelIndex &index);
    void on_jinglesView_pressed(const QModelIndex &index);
    void on_pubView_pressed(const QModelIndex &index);
    void on_programsView_pressed(const QModelIndex &index);
    void on_torrentsView_pressed(const QModelIndex &index);
    // Queue one more track. Returns true when something was actually added,
    // so a caller that needs a track to start on can tell the difference
    // between "Auto Mode topped the list up" and "there was nothing to add".
    bool autoModeGetMoreSongs();
    void on_bt_autoMode_clicked();
    void on_actionAdd_a_single_song_triggered();
    void on_btPlayNext_clicked();
    void on_actionAdd_all_songs_in_a_folder_triggered();
    void on_actionManage_Genres_triggered();
    void on_actionAdd_Jingle_triggered();
    void on_actionAdd_a_publicity_triggered();
    void run_scheduler();
    void on_actionOptions_triggered();
    void on_actionAbout_triggered();
    void on_actionAdd_a_song_from_Youtube_or_Other_triggered();
    void on_bt_search_clicked();
    void on_bt_reset_clicked();
    void on_bt_apply_filter_clicked();
    void on_bt_updateTables_clicked();
    void on_actionSave_Playlist_triggered();
    void on_actionClear_Playlist_triggered();
    void on_actionLoad_Playlist_triggered();
    void run_server_scheduler();
    void on_bt_rec_clicked();
    void on_actionRecord_a_new_Program_triggered();
    void on_bt_ProgramStopandProcess_clicked();
    void on_actionAdd_a_program_triggered();
    void run_recTimer();
    void setRecTimeToDefaults();
    void updateConfig();
    void pingServer();
    void server_ftp_check();
    void on_actionFullScreen_triggered();
    void on_actionForce_an_FTP_Check_triggered();
    void on_actionMake_a_program_from_this_playlist_triggered();
    void on_actionCheck_the_Database_records_triggered();
    void calculate_playlist_total_time();
    void RectimerDone();
    void RecT5();
    void RecT4();
    void RecT3();
    void RecT2();
    void RecT1();
    void RecCHK();
    void on_actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation_triggered();
    void on_bt_rol_streaming_play_clicked();
    void on_bt_rol_streaming_stop_clicked();
    void on_lp_1_bt_play_clicked();
    void on_pushButton_clicked();
    void on_lp_1_bt_play_2_clicked();
    void on_pushButton_2_clicked();
    void on_lp_1_bt_pause_clicked();
    void on_lp_2_bt_pause_clicked();
    void on_bt_sndconv_clicked();
    void on_actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database_triggered();
    void on_actionUpdate_System_triggered();
    void on_actionInstall_all_dependencies_triggered();
    void on_bt_apply_multi_selection_clicked();
    void on_actionConvert_all_musics_in_the_database_to_mp3_triggered();
    void on_actionConvert_all_musics_in_the_database_to_ogg_triggered();
    void on_bt_start_streaming_clicked();
    void on_horizontalSlider_lps_vol_sliderMoved(int position);
    void on_bt_center_lps_vol_clicked();
    void streaming_timmer();
    void on_bt_stop_streaming_clicked();
    void on_txt_search_returnPressed();
    void server_check_and_schedule_new_programs();
    void on_actionForce_monitorization_triggered();
    void on_actionUpdate_Dinamic_Server_s_IP_triggered();
    void on_bt_add_some_random_songs_from_genre_clicked();
    void ddnsUpdate();
    void ice_timmer();
    void butt_timmer();
    void on_bt_icecast_clicked();
    void on_bt_butt_clicked();
    void on_bt_ddns_clicked();
    void on_bt_portTest_clicked();
    void on_bt_takeOver_clicked();
    void MainsetVol100();
    void MainsetVol80();
    void MainsetVol60();
    void MainsetVol40();
    void MainsetVol20();
    void MainsetVol10();
    void MainsetVol5();
    void MainStop();
    void pingTakeOverClient();
    void recoveryStreamTakeOverPlay();
    void checkTakeOver();
    void livePiscaStart();
    void livePiscaStop();
    void rmConfirmTakeOver();
    void returnTakeOver();
    void stopMplayer();
    void seedDefaultGenres();
    void monitorTakeOver();
    void on_bt_pause_rec_clicked();
    void deleteFilesByPattern(const QString &dirPath, const QString &pattern);
    void on_bt_pause_play_clicked();
    void triggerPingFailureActions();
    bool killProcessByName(const QString &processName);
    
    // Torrent functionality
    void on_torConnectButton_clicked();
    void on_torDisconnectButton_clicked();
    void on_torrentSearchButton_clicked();
    void on_torrentClearButton_clicked();
    void on_torrentSearchEdit_returnPressed();
    void on_findOnionButton_clicked();
    void on_reloadPageButton_clicked();
    void onTorrentSearchResults(const QList<TorrentSearchResult> &results);
    void onTorrentSearchError(const QString &error);
    void onTorrentDownloadCompleted(const QString &downloadId, const QStringList &audioFiles);
    void onTorrentStreamingReady(const QString &downloadId, const QString &filePath);

    // Ensures a torrent client (aria2 or transmission-cli) is installed,
    // prompting the user to install one on demand. Returns true if available.
    bool ensureTorrentClient();
    
    // Tor connection management
    void onTorReady();
    void onTorDisconnected();
    void onTorError(const QString &error);
    void onOnionMirrorFound(const QString &clearnetDomain, const QString &onionUrl);
    void onSearchingForOnionMirror(const QString &clearnetDomain);
    void onOnionMirrorSearchFailed(const QString &clearnetDomain);
    void onOnionSitesUnavailable();
    void updateTorConnectionUI(bool connected);
    void updateDownloadsCountLabel();
    // Real kill-switch for the torrent feature: services are created lazily
    // only when EnableTorrents is on (ensureTorrentServices, idempotent), and
    // all their network activity is stopped when it is turned off.
    void ensureTorrentServices();
    void shutdownTorrentActivity();
    // BitTorrent peer traffic can't go through Tor, so the download exposes the
    // real IP. XFB can't route it for the user, but it detects whether a VPN is
    // active and warns (never blocks) before downloading without one.
    bool isVpnActive(QString *ifaceName = nullptr) const;
    bool confirmDownloadNetworkExposure();
    void updateVpnStatusLabel();

    // Accessibility initialization
    void registerAccessibilityServices();
    void initializeAccessibility();

    // Audio FX (EQ / compressor / 432 Hz)
    void openAudioFxDialog();
    void applyStoredFxSettings();
    void convertAllMusicsTo432();
    void convertMusicsTo432(const QStringList &paths);

    // Streaming client (listen to a network stream)
    void startRadioStream(const QUrl &streamUrl);
    void resolveAndPlayStreamPlaylist(const QUrl &playlistUrl);

    // ngrok public share link for the streaming server
    void on_bt_ngrok_setup_clicked();
    void on_bt_ngrok_clicked();
    void on_bt_ngrok_copy_clicked();

    // Update notifications
    void notifyUpdateAvailable(const QString &version, const QUrl &releasePage,
                               const QUrl &downloadUrl);
    void downloadAndOpenUpdate(const QUrl &downloadUrl, const QUrl &releasePage,
                               const QString &version);

  private:
    Ui::player *ui = nullptr;

    // Media players (FxPlayer mirrors the QMediaPlayer API and adds the
    // optional FX chain: 432 Hz retune, equalizer and compressor)
    FxPlayer *Xplayer = nullptr;
    QList<QUrl> XplaylistUrls;
    int XplaylistIndex = 0;

    FxPlayer *lp1_Xplayer = nullptr;
    QList<QUrl> lp1_XplaylistUrls;
    int lp1_XplaylistIndex = 0;

    FxPlayer *lp2_Xplayer = nullptr;
    QList<QUrl> lp2_XplaylistUrls;
    int lp2_XplaylistIndex = 0;

    // Audio outputs
    QAudioOutput *XplayerOutput = nullptr;
    QAudioOutput *lp1_XplayerOutput = nullptr;
    QAudioOutput *lp2_XplayerOutput = nullptr;

    // Media recording
    QMediaCaptureSession *captureSession = nullptr;
    QMediaRecorder *audioRecorder = nullptr;
    QAudioInput *audioInput = nullptr;

    // Ad banner
    QQuickWidget *adBanner = nullptr;

    // Collapsible bottom tab area (Music/Jingles/Pub/Programs/Torrents)
    void setupCollapsibleTabs();
    void setPubTabsCollapsed(bool collapsed);
    bool m_pubTabsCollapsed = false;

    // Toggle button (tab-bar corner) that shows/hides the side panel
    // (Search/Filters/Extras toolbox + the moved Playlist controls).
    QToolButton *m_sidePanelToggle = nullptr;

    // Customizable dock layout: every main section except the central tabs
    // is a movable/floatable/closable panel. The arrangement persists in
    // xfb.conf (MainWindowState/MainWindowGeometry/LayoutLocked).
    QDockWidget *m_dockPlayer = nullptr;  // transport + now playing (frame_4)
    QDockWidget *m_dockClock = nullptr;   // clock + auto mode (frame)
    QDockWidget *m_dockLibrary = nullptr; // Music/Jingles/... tabs (pubWidget)
    QDockWidget *m_dockSide = nullptr;    // artwork + side toolbox
    QAction *m_lockLayoutAction = nullptr;
    QByteArray m_defaultLayoutState; // for View → Reset the layout
    bool m_layoutLocked = false;
    void setLayoutLocked(bool locked);
    void resetDockLayout();

    // Fitting the window onto a small screen. Three of the panels came out of
    // Qt Designer with every control at a hand-picked pixel position, which
    // only ever fitted the wide window they were drawn in: on a laptop the
    // player panel's progress slider ran underneath the clock beside it, and
    // the side panel's lower controls sat below the bottom edge with no way to
    // scroll down to them.
    void relayoutPlayerFrame();          ///< re-places frame_4's children for its real width
    /// Takes up the room the progress slider leaves behind, so the volume
    /// slider stays where it was instead of drifting into the middle.
    QSpacerItem *m_volumeRowSpacer = nullptr;
    void relayoutClockFrame();           ///< the same for the clock panel
    bool m_relayoutingPlayerFrame = false;
    void applyProgressBarVisibility();   ///< the wave view already shows the playhead
    void makeSidePanelScrollable();      ///< scroll bars for the side toolbox pages
    void makeTabScrollable(QWidget *tab);///< moves a tab's contents into a scroll area
    /** Puts @p content inside a scroll area so it can shrink below its own
     *  minimum size instead of forcing the whole window to stay tall. */
    static QScrollArea *wrapInScrollArea(QWidget *content, QWidget *parent);
    /// Scroll area holding m_padBoard; this, not m_padBoard, is the tab page.
    QWidget *m_padBoardPage = nullptr;

    // Track artwork (cover icons + the Artwork panel)
    ArtworkStore *m_artStore = nullptr;
    NowPlayingArtPanel *m_artPanel = nullptr;
    void requestItemArtwork(QListWidgetItem *item, const QString &path);
    void onArtworkReady(const QString &path);

    // State variables
    int indexcanal = 0;
    qint64 trackTotalDuration = 0;
    int autoMode = 0;
    int recMode = 0;
    int indexJust3rdDropEvt = 0;
    int lastTrackPercentage = 0;
    int Port = 0;
    int tmpFullScreen = 0;
    QString aExtencaoDesteCoiso, txt_selected_db, ask_normalize_new_files, estevalor, xaction, text, txtDuration, lastPlayedSong, recDevice, SavePath, NomeDestePrograma, ProgramsPath, MusicPath, JinglePath, Server_URL, User, Pass, destinationProgram, FTPPath, TakeOverPath, genrehour, ComHour, codec, contentamento;
    QString Role = "Client"; // Default to Client mode
    QString PlayMode = "stopped"; // Default playback state
    // Streaming client player. Uses FxPlayer because the plain QMediaPlayer
    // ffmpeg backend cannot play endless Icecast/Shoutcast streams (it stays
    // in LoadingMedia forever); FxPlayer routes stream URLs through the
    // ffmpeg-CLI engine, which handles them robustly (with reconnect).
    FxPlayer *RadioPlayer = nullptr;
    QAudioOutput *RadioPlayerOutput = nullptr; // audio sink for the streaming client
    QProcess radio1;
    QMovie *movie = nullptr;
    QMovie *movie2 = nullptr;
    bool Disable_Volume = false;
    bool normalization_soft = false;
    bool fullScreen = false;
    bool lp_1_paused = false;
    bool lp_2_paused = false;
    bool icecastrunning = false;
    bool buttrunning = false;
    bool takeOver = false;
    bool returntakeOver = false;
    QString takeOverStream;
    QString takeOverIP;
    QString returnTakeOverIP;
    QString radio1str;
    bool piscaLive = false;
    bool recPause = false;
    bool playPause = false;
    bool darkMode = false;
    bool disableSeekBar = false;
    QString recDeviceDesc;
    QMediaFormat::AudioCodec recCodec = QMediaFormat::AudioCodec::Unspecified;
    QMediaFormat::FileFormat recContainer = QMediaFormat::FileFormat();
    QNetworkAccessManager *networkManager = nullptr;
    
    // Torrent services
    TorNetworkService *m_torNetworkService = nullptr;
    TorrentSearchService *m_torrentSearchService = nullptr;
    TorrentDownloadService *m_torrentDownloadService = nullptr;
    QTimer *m_vpnStatusTimer = nullptr; // refreshes the Torrents-tab VPN indicator

    // ngrok tunnel for the public streaming link
    NgrokTunnelService *m_ngrokService = nullptr;

    // Update notifications
    UpdateCheckService *m_updateService = nullptr;
    bool m_updateCheckManual = false;

    // The corner "support XFB" notice, and the two-day timer that brings it
    // back for sessions that are never closed.
    DonationNotice *m_donationNotice = nullptr;
    QTimer *m_donationNoticeTimer = nullptr;

    // Audio FX tab (inserted after the Pads tab; visibility via ShowFxTab)
    AudioFxWidget *m_fxTabWidget = nullptr;
    QWidget *m_fxTabPage = nullptr;

    // Pads tab: the touch-friendly grid of instant-play pads (cart wall),
    // inserted right after the DJ tab; visibility via ShowPadsTab
    PadBoardWidget *m_padBoard = nullptr;
    /** Where a tab of the DJ group goes: after the pads, else after the DJ tab. */
    int djGroupInsertIndex() const;

    // Playlist sound-wave view (crossfade preparation). The store extracts
    // waveforms with ffmpeg; the view paints them into ui->playlist and
    // lets the user drag each track's wave to set its overlap with the
    // previous one (persisted in playlist XML files).
    WaveformStore *m_waveStore = nullptr;
    PlaylistWaveView *m_waveView = nullptr;
    QToolButton *m_waveViewToggle = nullptr;
    // Wave + volume line of the track on air (its playlist item is gone)
    NowPlayingWaveStrip *m_nowPlayingWave = nullptr;
    // "Max overlap" control: how early the next track can be dragged to
    // start (tracks with long silent tails need more than the default)
    QWidget *m_maxOverlapBox = nullptr;
    QSpinBox *m_maxOverlapSpin = nullptr;
    void setPlaylistWaveView(bool on);
    // Auto-mix: one click computes every transition's crossfade overlap
    // from the waveforms (quiet tail of one track + quiet head of the next)
    QToolButton *m_autoMixButton = nullptr;
    QPointer<QProgressDialog> m_autoMixProgress;
    void startAutoMix(const QVector<int> &rows);

    // Track tempo: measured off the same waveform decode, kept in the
    // musics table. Auto mode uses it to follow a track with one of a
    // similar tempo, so the crossfade between them does not fight itself.
    BpmLibrary *m_bpmLibrary = nullptr;
    QPointer<QProgressDialog> m_bpmProgress;
    // Blanks the BPM cells of tracks with no measurable tempo (stored as 0)
    QStyledItemDelegate *m_bpmCellDelegate = nullptr;
    void analyzeLibraryBpm();
    void offerBpmAnalysis();
    QString autoModeReferenceTrack() const;
    // Options: pick the next auto mode track near the previous one's tempo
    bool m_bpmMatch = false;
    int m_bpmTolerance = 8; // BPM, either side
    /// How many recently played tracks Auto Mode refuses to play again.
    int m_autoModeNoRepeat = 10;
    void rememberAutoModePick(const QString &path);
    /// What it has put up lately, newest last — its no-repeat window.
    QStringList m_recentAutoPicks;

    // ----------------------------------------------------------------
    // Intro (ramp) and outro times
    //
    // The intro is the number a presenter works to: how long they can talk
    // over the top of a record before the vocal arrives. It is measured
    // from the same 20 ms waveform buckets the wave view and the auto-mix
    // already use (audio/IntroDetector.h), stored on the musics table, and
    // shown — with a live countdown — on the now-playing wave strip.
    //
    // It is a LEVEL heuristic and it is wrong on some records, so the
    // marker is draggable and a corrected value is locked against being
    // overwritten by a later sweep. See IntroDetector's header comment for
    // exactly what it cannot do.
    // ----------------------------------------------------------------
    IntroLibrary *m_introLibrary = nullptr;
    QPointer<QProgressDialog> m_introProgress;
    /// Library sweep: measure the intro/outro of everything unmeasured.
    void analyzeLibraryIntro();
    /// Look the on-air track up, push it into the strip, measure if needed.
    void refreshIntroForCurrentTrack();
    /// Push one set of times into the strip and cache them for the speech.
    void applyIntroTimes(const IntroTimes &times);
    /// Spoken form: the ramp, or the time left of it while it is running.
    QString introAnnouncement() const;
    /// What the strip is currently showing (-1 = nothing measured).
    qint64 m_currentIntroMs = -1;
    qint64 m_currentOutroMs = -1;
    bool m_currentIntroLocked = false;
    /// Track whose intro has already been spoken, so it is said once.
    QString m_introAnnouncedPath;

    // ----------------------------------------------------------------
    // EBU R128 loudness normalisation
    //
    // Every library row can carry an integrated loudness (LUFS) and a true
    // peak (dBTP), measured once per file by LoudnessScanner. At playback
    // the difference between the configured target and the track's own
    // loudness becomes a gain in the FX engine's gain stage — a
    // NON-destructive correction; the files themselves are never touched.
    //
    // CAREFUL: that gain is in DECIBELS. The playlist volume envelope
    // (m_activeEnvelope / PlaylistWaveView::envelopeGainAt) is a LINEAR
    // 0..1 multiplier that lands on XplayerOutput->setVolume(). The two
    // ride in different places and multiply; they are never added.
    // ----------------------------------------------------------------
    LoudnessScanner *m_loudnessScanner = nullptr;
    QPointer<QProgressDialog> m_loudnessProgress;
    bool m_loudnessEnabled = false;
    double m_loudnessTargetLufs = -16.0;   // LUFS
    double m_loudnessCeilingDbTp = -1.0;   // dBTP
    /// Path the current loudness gain was computed for (avoids re-querying).
    QString m_loudnessAppliedPath;
    /// How many tracks the last sweep found already measured and skipped.
    int m_loudnessSkipped = 0;
    void scanLibraryLoudness();
    /// Look the loudness of a file up in whichever media table holds it.
    bool loudnessForPath(const QString &path, double *lufs, double *truePeak) const;
    /// Compute and push the playback gain for the track now being loaded.
    void applyLoudnessForSource(const QUrl &url);
    /// Push the enable flag, limiter and current gain into every player.
    void applyLoudnessSettings();
    void storeLoudness(const LoudnessMeasurement &measurement);
    /// Library rows with no measurement, or one taken before the file changed.
    QStringList tracksNeedingLoudness(int *alreadyMeasured) const;

    // --- Cue bus (pre-fade listen) ---
    // Auditions a track on a SECOND output device while something else is on
    // air. The bus owns its own players and is bound to the cue device only;
    // nothing here shares an output with the on-air players. See CueBus.h for
    // why cueing is refused outright when there is no distinct second device.
    CueBus *m_cueBus = nullptr;
    QLabel *m_cueIndicator = nullptr;  // permanent status-bar "CUE: ..." label
    QAction *m_cueAction = nullptr;    // Playback menu entry (Ctrl+Shift+C)
    QAction *m_cueStopAction = nullptr;
    QByteArray m_mainOutputDeviceId;   // Options: on-air output (empty = default)
    QByteArray m_cueOutputDeviceId;    // Options: the private ear
    bool m_cueSpeakAnnouncements = false; // route XFB's own speech to the ear
    bool m_cueCountdown = false;          // spoken outro countdown in the ear
    int  m_cueVolume = 80;                // Options: cue monitor level, 0..100
    int  m_lastSpokenCountdown = -1;      // last countdown mark already spoken
    /** Creates the cue bus and its status-bar indicator (called once). */
    void setupCueBus();
    /** Pushes the configured output devices into every player and the bus. */
    void applyOutputDeviceSettings();
    /** Cue whatever is selected in the focused library view or the playlist. */
    void cueCurrentSelection();
    /** Start (or stop) a cue of one file, reporting refusals to the operator. */
    void cueFile(const QString &path, const QString &label);
    /** Keeps the status bar, the menu entry and the accessible name in step. */
    void updateCueIndicator(bool cueing, const QString &label);

    // Overlap segue: when the next playlist item defines an overlap, the
    // dying tail of the current track is handed to this dedicated player
    // (faded out by m_tailFade) while the next track starts on Xplayer —
    // the main playback state machine never notices the difference.
    FxPlayer *m_tailPlayer = nullptr;
    QAudioOutput *m_tailOutput = nullptr;
    QVariantAnimation *m_tailFade = nullptr;
    bool m_overlapSegueFired = false;
    // Gapless: the next playlist item was handed to Xplayer->prepareNext()
    // for the currently playing track (re-armed on each durationChanged)
    bool m_nextPrepared = false;
    // Options: auto-mix every track as it is added to the playlist
    bool m_autoAutoMix = false;
    // Options: stereo LED output meter (horizontal in the volume strip, or
    // vertical between the main tabs and the side panel)
    LevelMeter *m_levelMeter = nullptr;
    bool m_levelMeterVertical = false; // current docking (starts horizontal)
    QSplitter *m_middleSplitter = nullptr; // main tabs | side panel splitter
    void startOverlapSegue(qint64 fadeMs);
    void stopTailPlayer();

    // Volume line (envelope) of the track playing on Xplayer, captured from
    // its playlist item when playback starts. Applied on every position
    // tick as sliderVolume * envelopeGainAt(position); m_envelopeApplied
    // makes sure the slider volume is restored when the line stops applying.
    QVector<QPointF> m_activeEnvelope;
    QString m_activeEnvelopePath;
    bool m_envelopeApplied = false;
    /** The item now on Xplayer is a recorded voice track (a presenter's
     *  link), not music. Set from PlaylistWaveView::VoiceTrackRole when the
     *  track starts; read by startOverlapSegue(). */
    bool m_activeIsVoiceTrack = false;

    // --- Voice tracking -----------------------------------------------------
    //
    // A segue normally fades the outgoing track linearly to nothing over the
    // overlap, and that is right for one song running under the next. It is
    // wrong at both edges of a voice track: fading the song out linearly
    // under the link throws away the ducking the operator just generated (and
    // possibly edited by hand), and fading the LINK out under the incoming
    // song chops the presenter off mid-word.
    //
    // So when either side of a segue is a voice track, the tail player is
    // driven by the outgoing item's own volume line instead of by m_tailFade,
    // and it plays out to its natural end. Nothing else about the segue
    // changes, and a join with no voice track in it behaves exactly as before.
    QVector<QPointF> m_tailEnvelope;
    bool m_tailEnvelopeActive = false;
    float m_tailBaseVolume = 1.0f;
    void onTailPositionChanged(qint64 positionMs);
    /**
     * Record a link over the join above playlist row @a joinRow (so between
     * rows joinRow-1 and joinRow), then insert it as a track of its own.
     */
    void openVoiceTrackDialog(int joinRow);
    /** The row whose join a voice track would go into, or -1 with the reason
     *  announced. Row 0 has no join above it that XFB can duck. */
    int voiceTrackTargetRow();

    // LP deck scratching state (index 0 = deck 1, 1 = deck 2)
    QElapsedTimer m_scratchClock;
    /// Started in the constructor: how long this XFB has been up, which is
    /// what a backup station's heartbeat asks for.
    QElapsedTimer m_processUptime;
    bool m_lpScratching[2] = {false, false};
    double m_lpLastAngleDeg[2] = {0.0, 0.0};
    qint64 m_lpLastMoveMs[2] = {0, 0};

    // Platter rotation rendering (scratch follows the hand; brake/backspin
    // play a matching deceleration/backspin animation)
    QPixmap m_lpPlatterBase[2];
    double m_lpPlatterRotation[2] = {0.0, 0.0};
    QPointer<QVariantAnimation> m_lpPlatterAnim[2];
    void setPlatterRotation(int deck, double degrees);
    void grabPlatterFrame(int deck);
    void restorePlatterMotion(int deck);
    void startPlatterEffectAnimation(int deck, bool backspin);
    
    // Thread safety for playlist operations
    mutable QMutex m_playlistMutex;
    static int s_recursionDepth;
    static const int MAX_RECURSION_DEPTH = 5;
    bool m_manualAdvancing = false;

    // --- As-run log ---
    // The airlog row for whatever is on air, plus how the item it replaces
    // ended: the segue, the skip and the stop all funnel through
    // playNextSong(), so the reason has to be handed in from the call site.
    qint64  m_airHandle = 0;
    qint64  m_airPosition = 0;   ///< last position reported for that row
    QString m_airEndReason;      ///< empty means the track simply ran out
    QPointer<class AirLogDialog> m_airLogDialog;
    /** The rotation editor and the "why did it pick that?" view. */
    QPointer<class RotationDialog> m_rotationDialog;

    // --- the hour clock -----------------------------------------------
    // The format as radio writes it: an ordered, timed hour of sweeps, ad
    // breaks, jingles and hard-timed items, assigned to weekday and hour.
    // Entirely off until HourClock/Enabled is set, at which point the current
    // sweep's genre stands in for the hourgenre row and the fixed items are
    // queued when they come due. See services/HourClock.h.
    QPointer<class HourClockDialog> m_hourClockDialog;
    /** Fires the hour's hard-timed items. Only created when the feature is on. */
    QTimer *m_hourClockTimer = nullptr;
    /** "date/hour/slot" keys already put on air this hour, so a 20-second
     *  tick inside a 60-second window cannot queue the news three times. */
    QSet<QString> m_hourClockFired;
    void setupHourClock();
    void hourClockTick();
    /** The genre the clock wants filled now, or empty for "use the hour grid".
     *  Split out so autoModeGetMoreSongs() stays one readable function and so
     *  the fallback is impossible to miss. */
    QString hourClockGenreNow() const;
    /** Closes the open as-run row, if any. playedMs < 0: use m_airPosition. */
    void closeAirLogEntry(const QString &reason, qint64 playedMs = -1);
    
    // Watchdog timer to detect stalled playback
    QTimer *m_playbackWatchdog = nullptr;
    qint64 m_lastKnownPosition = -1;
    int m_stallCount = 0;
    
    void launchExternalApplication(const QString &appName, const QString &filePath);
    void getMediaInfoForFile(const QString &filePath);
    void runServerCheckScript(const QString &scriptName, const QString &fileToCheck, const QString &successMessage, const QString &failureMessage);
    void runServerUploadScript(const QString &scriptName, const QString &fileToUpload, const QString &successMessage, const QString &failureMessage, std::function<void (bool)> callback);
    // Kept alive between openings so the tutorial reopens on the topic the
    // operator was last reading.
    QPointer<class AccessibilityTutorialDialog> m_tutorialDialog;

    // --- Sync to phone ---
    // Serves the library and playlists to the XFB companion app over the local
    // network. Created lazily, and only listens once the operator says so.
    class MobileSyncServer *mobileSyncServer();
    /** Every path currently listed in the Music List, filter and all. */
    QStringList allListedMusicPaths() const;
    /** Marks tracks for the phone to collect; confirms first for big batches. */
    void markForPhone(const QStringList &paths, bool confirmFirst = false);
    QPointer<class MobileSyncServer> m_mobileSyncServer;
    QPointer<class MobileSyncDialog> m_mobileSyncDialog;

    // --- Station backup ---
    // Mirrors another XFB on the network onto this one — the whole catalogue,
    // its media, the schedule and the saved playlists — so a station whose
    // studio machine dies is one launch away from being back on air. Uses the
    // same server as the phone sync above, with a role of its own.
    class StationSyncClient *stationSyncClient();
    QPointer<class StationSyncClient> m_stationSync;
    QPointer<class StationSyncDialog> m_stationSyncDialog;

    // --- Production computers ---
    // The other direction: another XFB where the programme is prepared, which
    // reads this station's catalogue and publishes back what it made, so the
    // machine on air is never the machine being worked on. Same server, same
    // pairing, a role of its own — the only one allowed to write here.
    class ProductionSyncClient *productionSyncClient();
    QPointer<class ProductionSyncClient> m_productionSync;
    QPointer<class ProductionSyncDialog> m_productionSyncDialog;
    /// A batch of published entries arrives as several requests; the tables on
    /// screen are reloaded once when it settles, not once per request.
    bool m_peerCatalogueRefreshQueued = false;
    // --- Streaming from inside XFB ---
    // Taps the player's own post-DSP master output, encodes it with ffmpeg
    // and feeds one or more Icecast mounts, pushing the on-air title as the
    // track changes. An alternative to the external Icecast + butt buttons,
    // which stay exactly as they were.
    class StreamService *streamService();
    /** Tell the stream what just went to air; cheap when not streaming. */
    void updateStreamNowPlaying(const QString &filePath);
    QPointer<class StreamService> m_streamService;
    QPointer<class StreamDialog> m_streamDialog;

    // --- Dead air ---
    // Watches the master level and the transport, and puts evergreen material
    // on when the station goes quiet. Created at startup only when the
    // operator has turned it on — a watchdog nobody asked for is a timer
    // nobody asked for.
    class DeadAirWatchdog *deadAirWatchdog();
    /** Queues the configured fallback material and starts it playing.
     *  @param what receives a description of what went on, or why nothing did.
     *  @return true when audio was actually started. */
    bool startDeadAirFallback(QString *what);
    /** Everything GET /api/station/heartbeat reports about this machine. */
    QJsonObject stationHeartbeatState() const;
    /** The alert an operator cannot miss, without blocking the event loop. */
    void showAirAlert(const QString &title, const QString &message, bool critical);
    QPointer<class DeadAirWatchdog> m_deadAirWatchdog;
    QPointer<class DeadAirDialog> m_deadAirDialog;

    // --- The public now-playing page and its request line ---
    // A page served to whoever can reach this machine, off until the operator
    // switches it on in the window below. What it may say about the air comes
    // from publicNowPlaying() and from nowhere else — deliberately not from
    // stationHeartbeatState(), which carries the file path.
    MobileSyncServer::NowPlaying publicNowPlaying();
    QPointer<class RequestTrayDialog> m_requestTrayDialog;
    /// The on-air cover, re-encoded small, held so the poll costs nothing.
    QString    m_publicArtPath;
    QByteArray m_publicArtJpeg;
    QString    m_publicArtKey;
    QPointer<class QMessageBox> m_airAlertBox;
    /// Samples the transport for the watchdog once a second.
    QTimer *m_deadAirFeedTimer = nullptr;

    // --- Adding library tracks to the playlist from the keyboard ---
    // The library views' only route into the playlist used to be the
    // right-click context menu, which is unreachable on a Mac keyboard (no
    // Menu key). These provide Enter/Return activation plus menu entries.
    QTableView *focusedLibraryView() const;
    void addSelectionToPlaylist(QTableView *view, bool toTop);

    // --- Accessibility (screen reader / keyboard support) ---
    // Names every icon-only control so a screen reader can identify it.
    void setupAccessibleControls();
    // Builds the Playback menu and its application-wide shortcuts, so the
    // transport can be driven entirely from the keyboard.
    void setupPlaybackShortcuts();
    // Speaks a message through the AccessibilityManager (no-op when no
    // assistive technology is attached).
    void announceAccessible(const QString &message);
    // A duration in words ("2 minutes 35 seconds") rather than digits, which
    // is what a screen reader can convey in one pass.
    QString spokenDuration(qint64 milliseconds) const;
    // How much of the on-air track is left, or why that cannot be said.
    QString remainingTimeAnnouncement() const;
    // True when the operator asked for the time left to follow the
    // "what is playing" announcement (Accessibility Preferences).
    bool announcesRemainingWithNowPlaying() const;
    // Mirrors playback/recording state into text a screen reader can read,
    // so state is never conveyed by colour alone.
    void refreshTransportAccessibleState();
    void applySqlHeaderLabels(QSqlQueryModel *model, const QList<QPair<QString, QString>> &labels);
    void applyMusicHeaderLabels(QSqlQueryModel *model);
    void startTimeBackfill();
    void processNextTimeBackfill();
    QStringList m_timeBackfillPending;
    int m_timeBackfillUpdated = 0;
    QString serverScriptPath(const QString &scriptName) const;
    bool serverScriptIsTemplate(const QString &scriptPath) const;
    void startServerScript(QProcess *process, const QString &scriptPath);
    QString serverScriptShellCommand(const QString &scriptPath) const;
    void getDurationForFile(const QString &filePath, std::function<void (const QString &, const QString &)> callback);
};



#endif // PLAYER_H
