#ifndef PLAYER_H
#define PLAYER_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QtSql>
#include <QMediaRecorder>
#include <QAudioRecorder>
#include <QUrl>
#include <QAudioInput>
#include <QByteArray>
#include <QComboBox>
#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QWidget>
#include <QTime>
#include <QAudioProbe>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>
#include <phonon/audiooutput.h>
#include <phonon/seekslider.h>
#include <phonon/mediaobject.h>
#include <phonon/volumeslider.h>
#include <phonon/backendcapabilities.h>
#include <phonon/audiodataoutput.h>
#include <QtMultimedia/QAudioDeviceInfo>
#include <QtMultimedia/QAudioInput>
#include <QtDataVisualization>


namespace Ui {
class player;
}

class QAudioLevel;

class player : public QMainWindow
{
    Q_OBJECT

public:
    explicit player(QWidget *parent = 0);
    ~player();
    int onAbout2Finish;
    QSqlDatabase adb;
    QString saveFile;
    QTimer *recTimer = new QTimer(this);
    int recSecs;
    int recMins;
    int recHours;
    int server_this_day_of_the_week;
    QString lp1_total_time;
    int lp1_total_time_int;
    QString lp2_total_time;
    int lp2_total_time_int;
    QTimer *stimer = new QTimer(this);
    QTimer *icetimer = new QTimer(this);
    QTimer *butt_timer = new QTimer(this);
    int jingleCadaNumMusicas;

public slots:
    void checkDbOpen();
    void update_music_table();
    void checkForUpdates();
    void processBuffer(const QAudioBuffer& buffer);

private slots:
    void on_actionOpen_triggered();
    void on_btPlay_clicked();
    void on_btStop_clicked();
    void onPositionChanged(qint64 position);
    void durationChanged(qint64 position);
    void currentMediaChanged(const QMediaContent & content);
    void volumeChanged(int volume);
    void lp1_onPositionChanged(qint64 position);
    void lp1_durationChanged(qint64 position);
    void lp1_currentMediaChanged(const QMediaContent & content);
    void lp1_volumeChanged(int volume);
    void lp2_onPositionChanged(qint64 position);
    void lp2_durationChanged(qint64 position);
    void lp2_currentMediaChanged(const QMediaContent & content);
    void lp2_volumeChanged(int volume);
    void playlistAboutToFinish();
    void playNextSong();
    void showTime();
    void playlistContextMenu(const QPoint&);
    void musicViewContextMenu(const QPoint&);
    void jinglesViewContextMenu(const QPoint&);
    void pubViewContextMenu(const QPoint&);
    void programsViewContextMenu(const QPoint&);
    void dropEvent(QDropEvent *);
    void dragEnterEvent(QDragEnterEvent *);
    void on_musicView_pressed(const QModelIndex &index);
    void on_jinglesView_pressed(const QModelIndex &index);
    void on_pubView_pressed(const QModelIndex &index);
    void on_programsView_pressed(const QModelIndex &index);
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
    void on_bt_apply_multi_selection_clicked();
    void on_actionConvert_all_musics_in_the_database_to_mp3_triggered();
    void on_actionConvert_all_musics_in_the_database_to_ogg_triggered();
    void on_bt_start_streaming_clicked();
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
    void monitorTakeOver();
    void on_bt_pause_rec_clicked();
    void on_bt_pause_play_clicked();
    void on_actionUpdate_Configuration_triggered();
    void on_bt_purgeCache_clicked();

    void dataReceived(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >);
    void dataReceivedLp1(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >);
    void dataReceivedLp2(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >);

    void on_actionForce_to_open_default_database_triggered();
    void aboutToFinish();

    void sourceChanged(Phonon::MediaSource);

    void setXMarginTxtPlay2();
void trocaDeMusica();
private:
    Ui::player *ui;
    QMediaPlayer *Xplayer;
    QMediaPlaylist *Xplaylist;
    QMediaPlayer *lp1_Xplayer;
    QMediaPlaylist *lp1_Xplaylist;
    QMediaPlayer *lp2_Xplayer;
    QMediaPlaylist *lp2_Xplaylist;
    qint64 trackTotalDuration;
    int autoMode,recMode,indexJust3rdDropEvt,lastTrackPercentage,Port,tmpFullScreen;
    QString aExtencaoDesteCoiso, txt_selected_db, disableSeekBar,normalization_soft, Disable_Volume,ask_normalize_new_files,estevalor,xaction,text,txtDuration,lastPlayedSong,Role,recDevice,SavePath,NomeDestePrograma,ProgramsPath,MusicPath,JinglePath,Server_URL,User,Pass,destinationProgram,FTPPath,TakeOverPath,PlayMode,genrehour,ComHour,fullScreen,codec,contentamento;
    QAudioRecorder *audioRecorder;
    QAudioProbe *probe;
    QMediaPlayer RadioPlayer;
    QProcess radio1;
    QMovie* movie;
    QMovie* movie2;
    bool lp_1_paused = false;
    bool lp_2_paused = false;
    int indexcanal, indexcanalLp1;
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
    QString AutoStartStop = "false";
    QString AutoStartTime, AutoStopTime;
    QString featuredTxt, cpath, binfo;



    Phonon::MediaObject *mediaObject;
    Phonon::MediaObject *mediaObjectLp1;
    Phonon::MediaObject *mediaObjectLp2;
    Phonon::MediaObject *mediaObjectIN;

    Phonon::MediaObject *metaInformationResolver;

    Phonon::AudioOutput *audioOutput;
    Phonon::AudioOutput *audioOutputLp1;
    Phonon::AudioOutput *audioOutputLp2;


    Phonon::AudioDataOutput *dataout;
    Phonon::AudioDataOutput *dataoutLp1;
    Phonon::AudioDataOutput *dataoutLp2;

    Phonon::VolumeSlider *volumeSlider;
    Phonon::VolumeSlider *volumeSlider_LP1;
    Phonon::VolumeSlider *volumeSlider_LP2;

    Phonon::SeekSlider *seekSlider;
    Phonon::SeekSlider *seekSlider_LP1;
    Phonon::SeekSlider *seekSlider_LP2;

    QList<Phonon::MediaSource> sources;




    QAudioDeviceInfo m_device;
    player *m_audioInfo;
    QAudioFormat m_format;
    QAudioInput *m_audioInput;
    QIODevice *m_input;
    bool m_pullMode;
    QByteArray m_buffer;
    QList<QAudioLevel*> audioLevels;

};



#endif // PLAYER_H
