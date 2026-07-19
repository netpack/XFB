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
class QNetworkAccessManager;
class QProgressDialog;
class QQuickWidget;
class QToolButton;
class QSplitter;
class QMovie;

// Project forward declarations
class TorNetworkService;
class TorrentSearchService;
class TorrentDownloadService;
class NgrokTunnelService;
class UpdateCheckService;
class AudioFxWidget;
class WaveformStore;
class PlaylistWaveView;
class NowPlayingWaveStrip;
class LevelMeter;
class ArtworkStore;
class NowPlayingArtPanel;

#include "services/TorrentTypes.h"
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
    void autoModeGetMoreSongs();
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

    // Audio FX tab (inserted after the DJ tab; visibility via ShowFxTab)
    AudioFxWidget *m_fxTabWidget = nullptr;
    QWidget *m_fxTabPage = nullptr;

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

    // LP deck scratching state (index 0 = deck 1, 1 = deck 2)
    QElapsedTimer m_scratchClock;
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
    
    // Watchdog timer to detect stalled playback
    QTimer *m_playbackWatchdog = nullptr;
    qint64 m_lastKnownPosition = -1;
    int m_stallCount = 0;
    
    void launchExternalApplication(const QString &appName, const QString &filePath);
    void getMediaInfoForFile(const QString &filePath);
    void runServerCheckScript(const QString &scriptName, const QString &fileToCheck, const QString &successMessage, const QString &failureMessage);
    void runServerUploadScript(const QString &scriptName, const QString &fileToUpload, const QString &successMessage, const QString &failureMessage, std::function<void (bool)> callback);
    void getDurationForFile(const QString &filePath, std::function<void (const QString &, const QString &)> callback);
};



#endif // PLAYER_H
