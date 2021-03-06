/********************************************************************************
** Form generated from reading UI file 'player.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PLAYER_H
#define UI_PLAYER_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_player
{
public:
    QAction *actionOpen;
    QAction *actionAdd_a_single_song;
    QAction *actionAdd_all_songs_in_a_folder;
    QAction *actionManage_Genres;
    QAction *actionAdd_Jingle;
    QAction *actionAdd_a_publicity;
    QAction *actionOptions;
    QAction *actionAbout;
    QAction *actionAdd_a_song_from_Youtube_or_Other;
    QAction *actionSave_Playlist;
    QAction *actionClear_Playlist;
    QAction *actionLoad_Playlist;
    QAction *actionRecord_a_new_Program;
    QAction *action123;
    QAction *actionAdd_a_program;
    QAction *actionFullScreen;
    QAction *actionForce_an_FTP_Check;
    QAction *actionMake_a_program_from_this_playlist;
    QAction *actionCheck_the_Database_records;
    QAction *actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation;
    QAction *actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database;
    QAction *actionUpdate_System;
    QAction *actionConvert_all_musics_in_the_database_to_mp3;
    QAction *actionConvert_all_musics_in_the_database_to_ogg;
    QAction *actionForce_monitorization;
    QAction *actionUpdate_Dinamic_Server_s_IP;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QWidget *widget;
    QGridLayout *gridLayout_2;
    QFrame *frame;
    QLCDNumber *txt_horas;
    QPushButton *bt_autoMode;
    QTabWidget *tabWidget_2;
    QWidget *tabPlaylist;
    QGridLayout *gridLayout_3;
    QFrame *frame_2;
    QLabel *txt_playlistTotalTime;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_10;
    QCheckBox *checkBox_sum_to_playlist_time;
    QCheckBox *checkBox_update_last_played_values;
    QHBoxLayout *horizontalLayout_9;
    QCheckBox *checkBox_random_jingles;
    QSpinBox *spinBox_random_jingles_interval;
    QLabel *label_9;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_3;
    QSpinBox *spinBox_num_of_songs_to_add_random;
    QComboBox *comboBox_random_add_genre;
    QLabel *label_10;
    QPushButton *bt_add_some_random_songs_from_genre;
    QListWidget *playlist;
    QWidget *tabHistorylist;
    QHBoxLayout *horizontalLayout_6;
    QListWidget *historyList;
    QWidget *tab_dj;
    QLabel *lp_1;
    QLabel *lp_2;
    QLabel *lp_1_txt_file;
    QLabel *lp_2_txt_file;
    QPushButton *lp_1_bt_play;
    QPushButton *lp_1_bt_play_2;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *lp_1_bt_pause;
    QPushButton *lp_2_bt_pause;
    QSlider *horizontalSlider_lps_vol;
    QLabel *lbl_total_time_lp1;
    QLabel *lbl_total_time_lp2;
    QLabel *lbl_lp1_remaining;
    QLabel *lbl_lp2_remaining;
    QLabel *lbl_lp1_vol_level;
    QLabel *lbl_lp2_vol_level;
    QToolButton *bt_center_lps_vol;
    QTabWidget *pubWidget;
    QWidget *tabMusic;
    QHBoxLayout *horizontalLayout_2;
    QTableView *musicView;
    QWidget *tabJingles;
    QHBoxLayout *horizontalLayout_3;
    QTableView *jinglesView;
    QWidget *tabPub;
    QHBoxLayout *horizontalLayout_4;
    QTableView *pubView;
    QWidget *tabPrograms;
    QHBoxLayout *horizontalLayout_5;
    QTableView *programsView;
    QToolBox *page_FTP_Connection;
    QWidget *page;
    QWidget *layoutWidget1;
    QHBoxLayout *horizontalLayout;
    QLineEdit *txt_search;
    QPushButton *bt_reset;
    QPushButton *bt_search;
    QWidget *page_2;
    QPushButton *bt_apply_filter;
    QCheckBox *checkBox_filter_genre1;
    QComboBox *cBoxGenre1;
    QComboBox *cBoxGenre2;
    QCheckBox *checkBox_filter_genre2;
    QWidget *page_3;
    QPushButton *bt_updateTables;
    QPushButton *bt_sndconv;
    QComboBox *cbox_multi_select;
    QPushButton *bt_apply_multi_selection;
    QWidget *Radio;
    QPushButton *bt_rol_streaming_play;
    QLabel *label_4;
    QPushButton *bt_rol_streaming_stop;
    QWidget *page_4;
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_7;
    QPushButton *bt_start_streaming;
    QLabel *lbl_butt;
    QLabel *lbl_ddns;
    QLabel *lbl_port;
    QPushButton *bt_takeOver;
    QPushButton *bt_stop_streaming;
    QLabel *lbl_icecast;
    QLabel *label_8;
    QToolButton *bt_icecast;
    QToolButton *bt_butt;
    QToolButton *bt_ddns;
    QToolButton *bt_portTest;
    QFrame *line;
    QFrame *line_2;
    QFrame *line_3;
    QLabel *lbl_streamURL;
    QFrame *frame_4;
    QPushButton *btPlay;
    QPushButton *btStop;
    QPushButton *btPlayNext;
    QPushButton *bt_rec;
    QLabel *txt_recTime;
    QLabel *txtNowPlaying;
    QLabel *txt_ProgramName;
    QPushButton *bt_ProgramStopandProcess;
    QLabel *txt_uploadingPrograms;
    QLabel *txtDuration;
    QWidget *layoutWidget2;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label;
    QSlider *sliderVolume;
    QLabel *label_2;
    QSlider *sliderProgress;
    QLabel *txt_creatingPrograms;
    QLabel *led_rec;
    QLabel *txt_loading;
    QToolButton *bt_pause_rec;
    QToolButton *bt_pause_play;
    QHBoxLayout *horizontalLayout_11;
    QLabel *NetpackLogo;
    QLabel *txt_bottom_info;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuPlaylists;
    QMenu *menuDatabase;
    QMenu *menuHelp;
    QMenu *menuXFB;
    QMenu *menuClient_3;
    QMenu *menuServer;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *player)
    {
        if (player->objectName().isEmpty())
            player->setObjectName(QString::fromUtf8("player"));
        player->resize(1478, 820);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(255);
        sizePolicy.setVerticalStretch(255);
        sizePolicy.setHeightForWidth(player->sizePolicy().hasHeightForWidth());
        player->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/48x48.png"), QSize(), QIcon::Normal, QIcon::Off);
        player->setWindowIcon(icon);
        player->setWindowOpacity(1.000000000000000);
        player->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        player->setIconSize(QSize(48, 48));
        player->setDockOptions(QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks|QMainWindow::AnimatedDocks);
        player->setUnifiedTitleAndToolBarOnMac(true);
        actionOpen = new QAction(player);
        actionOpen->setObjectName(QString::fromUtf8("actionOpen"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/flat/Open Folder-32.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpen->setIcon(icon1);
        actionAdd_a_single_song = new QAction(player);
        actionAdd_a_single_song->setObjectName(QString::fromUtf8("actionAdd_a_single_song"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/flat/Treble Clef-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAdd_a_single_song->setIcon(icon2);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        actionAdd_a_single_song->setFont(font);
        actionAdd_all_songs_in_a_folder = new QAction(player);
        actionAdd_all_songs_in_a_folder->setObjectName(QString::fromUtf8("actionAdd_all_songs_in_a_folder"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/ic_menu_archive.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAdd_all_songs_in_a_folder->setIcon(icon3);
        actionAdd_all_songs_in_a_folder->setFont(font);
        actionManage_Genres = new QAction(player);
        actionManage_Genres->setObjectName(QString::fromUtf8("actionManage_Genres"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/flat/Radio-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionManage_Genres->setIcon(icon4);
        actionManage_Genres->setFont(font);
        actionAdd_Jingle = new QAction(player);
        actionAdd_Jingle->setObjectName(QString::fromUtf8("actionAdd_Jingle"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/flat/Alto Saxophone-52.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAdd_Jingle->setIcon(icon5);
        actionAdd_Jingle->setFont(font);
        actionAdd_a_publicity = new QAction(player);
        actionAdd_a_publicity->setObjectName(QString::fromUtf8("actionAdd_a_publicity"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/flat/Audio Wave 2-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAdd_a_publicity->setIcon(icon6);
        actionAdd_a_publicity->setFont(font);
        actionOptions = new QAction(player);
        actionOptions->setObjectName(QString::fromUtf8("actionOptions"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/ic_sysbar_quicksettings.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOptions->setIcon(icon7);
        actionOptions->setFont(font);
        actionAbout = new QAction(player);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/ic_menu_help.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAbout->setIcon(icon8);
        actionAbout->setFont(font);
        actionAdd_a_song_from_Youtube_or_Other = new QAction(player);
        actionAdd_a_song_from_Youtube_or_Other->setObjectName(QString::fromUtf8("actionAdd_a_song_from_Youtube_or_Other"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/icon48x48.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAdd_a_song_from_Youtube_or_Other->setIcon(icon9);
        actionAdd_a_song_from_Youtube_or_Other->setFont(font);
        actionSave_Playlist = new QAction(player);
        actionSave_Playlist->setObjectName(QString::fromUtf8("actionSave_Playlist"));
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/ic_menu_save.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave_Playlist->setIcon(icon10);
        actionSave_Playlist->setFont(font);
        actionClear_Playlist = new QAction(player);
        actionClear_Playlist->setObjectName(QString::fromUtf8("actionClear_Playlist"));
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/icons/ic_menu_close_clear_cancel.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionClear_Playlist->setIcon(icon11);
        actionClear_Playlist->setFont(font);
        actionLoad_Playlist = new QAction(player);
        actionLoad_Playlist->setObjectName(QString::fromUtf8("actionLoad_Playlist"));
        actionLoad_Playlist->setIcon(icon1);
        actionLoad_Playlist->setFont(font);
        actionRecord_a_new_Program = new QAction(player);
        actionRecord_a_new_Program->setObjectName(QString::fromUtf8("actionRecord_a_new_Program"));
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/icons/ic_voice_search_api_holo_light.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRecord_a_new_Program->setIcon(icon12);
        actionRecord_a_new_Program->setFont(font);
        action123 = new QAction(player);
        action123->setObjectName(QString::fromUtf8("action123"));
        actionAdd_a_program = new QAction(player);
        actionAdd_a_program->setObjectName(QString::fromUtf8("actionAdd_a_program"));
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/icons/flat/Microphone-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAdd_a_program->setIcon(icon13);
        actionAdd_a_program->setFont(font);
        actionFullScreen = new QAction(player);
        actionFullScreen->setObjectName(QString::fromUtf8("actionFullScreen"));
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/icons/view-fullscreen.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionFullScreen->setIcon(icon14);
        actionForce_an_FTP_Check = new QAction(player);
        actionForce_an_FTP_Check->setObjectName(QString::fromUtf8("actionForce_an_FTP_Check"));
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/icons/edit-text-frame-update.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionForce_an_FTP_Check->setIcon(icon15);
        actionMake_a_program_from_this_playlist = new QAction(player);
        actionMake_a_program_from_this_playlist->setObjectName(QString::fromUtf8("actionMake_a_program_from_this_playlist"));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/icons/distribute-vertical-margin.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionMake_a_program_from_this_playlist->setIcon(icon16);
        actionMake_a_program_from_this_playlist->setFont(font);
        actionCheck_the_Database_records = new QAction(player);
        actionCheck_the_Database_records->setObjectName(QString::fromUtf8("actionCheck_the_Database_records"));
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/icons/ic_menu_view.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionCheck_the_Database_records->setIcon(icon17);
        actionCheck_the_Database_records->setFont(font);
        actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation = new QAction(player);
        actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation->setObjectName(QString::fromUtf8("actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation"));
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/icons/edit-table-delete-row.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation->setIcon(icon18);
        actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation->setFont(font);
        actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database = new QAction(player);
        actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database->setObjectName(QString::fromUtf8("actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database"));
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/icons/distribute-horizontal-equal.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database->setIcon(icon19);
        actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database->setFont(font);
        actionUpdate_System = new QAction(player);
        actionUpdate_System->setObjectName(QString::fromUtf8("actionUpdate_System"));
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/icons/download.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionUpdate_System->setIcon(icon20);
        actionConvert_all_musics_in_the_database_to_mp3 = new QAction(player);
        actionConvert_all_musics_in_the_database_to_mp3->setObjectName(QString::fromUtf8("actionConvert_all_musics_in_the_database_to_mp3"));
        QIcon icon21;
        icon21.addFile(QString::fromUtf8(":/icons/mp3b.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionConvert_all_musics_in_the_database_to_mp3->setIcon(icon21);
        actionConvert_all_musics_in_the_database_to_mp3->setFont(font);
        actionConvert_all_musics_in_the_database_to_ogg = new QAction(player);
        actionConvert_all_musics_in_the_database_to_ogg->setObjectName(QString::fromUtf8("actionConvert_all_musics_in_the_database_to_ogg"));
        QIcon icon22;
        icon22.addFile(QString::fromUtf8(":/icons/ogg3.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionConvert_all_musics_in_the_database_to_ogg->setIcon(icon22);
        actionConvert_all_musics_in_the_database_to_ogg->setFont(font);
        actionForce_monitorization = new QAction(player);
        actionForce_monitorization->setObjectName(QString::fromUtf8("actionForce_monitorization"));
        QIcon icon23;
        icon23.addFile(QString::fromUtf8(":/icons/games-solve.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionForce_monitorization->setIcon(icon23);
        actionUpdate_Dinamic_Server_s_IP = new QAction(player);
        actionUpdate_Dinamic_Server_s_IP->setObjectName(QString::fromUtf8("actionUpdate_Dinamic_Server_s_IP"));
        QIcon icon24;
        icon24.addFile(QString::fromUtf8(":/icons/ic_menu_compass.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionUpdate_Dinamic_Server_s_IP->setIcon(icon24);
        centralWidget = new QWidget(player);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(255);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy1);
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(0);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        widget = new QWidget(centralWidget);
        widget->setObjectName(QString::fromUtf8("widget"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(255);
        sizePolicy2.setVerticalStretch(100);
        sizePolicy2.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy2);
        widget->setStyleSheet(QString::fromUtf8(""));
        gridLayout_2 = new QGridLayout(widget);
        gridLayout_2->setSpacing(0);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        frame = new QFrame(widget);
        frame->setObjectName(QString::fromUtf8("frame"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(frame->sizePolicy().hasHeightForWidth());
        frame->setSizePolicy(sizePolicy3);
        frame->setMinimumSize(QSize(350, 0));
        frame->setMaximumSize(QSize(251, 16777215));
        QFont font1;
        font1.setKerning(true);
        frame->setFont(font1);
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Raised);
        txt_horas = new QLCDNumber(frame);
        txt_horas->setObjectName(QString::fromUtf8("txt_horas"));
        txt_horas->setGeometry(QRect(30, 20, 301, 61));
        txt_horas->setStyleSheet(QString::fromUtf8("color: rgb(129, 129, 194);"));
        txt_horas->setFrameShape(QFrame::NoFrame);
        txt_horas->setDigitCount(8);
        txt_horas->setMode(QLCDNumber::Dec);
        bt_autoMode = new QPushButton(frame);
        bt_autoMode->setObjectName(QString::fromUtf8("bt_autoMode"));
        bt_autoMode->setGeometry(QRect(30, 90, 301, 31));
        QSizePolicy sizePolicy4(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(bt_autoMode->sizePolicy().hasHeightForWidth());
        bt_autoMode->setSizePolicy(sizePolicy4);
        bt_autoMode->setAutoFillBackground(false);
        bt_autoMode->setStyleSheet(QString::fromUtf8("background-color: rgb(175, 227, 59);"));
        QIcon icon25;
        icon25.addFile(QString::fromUtf8(":/icons/flat/Einstein-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_autoMode->setIcon(icon25);
        bt_autoMode->setIconSize(QSize(32, 32));
        bt_autoMode->setAutoDefault(false);
        bt_autoMode->setFlat(false);

        gridLayout_2->addWidget(frame, 0, 1, 5, 1);

        tabWidget_2 = new QTabWidget(widget);
        tabWidget_2->setObjectName(QString::fromUtf8("tabWidget_2"));
        sizePolicy1.setHeightForWidth(tabWidget_2->sizePolicy().hasHeightForWidth());
        tabWidget_2->setSizePolicy(sizePolicy1);
        QFont font2;
        font2.setKerning(false);
        tabWidget_2->setFont(font2);
        tabWidget_2->setIconSize(QSize(22, 22));
        tabWidget_2->setDocumentMode(true);
        tabWidget_2->setTabBarAutoHide(false);
        tabPlaylist = new QWidget();
        tabPlaylist->setObjectName(QString::fromUtf8("tabPlaylist"));
        sizePolicy3.setHeightForWidth(tabPlaylist->sizePolicy().hasHeightForWidth());
        tabPlaylist->setSizePolicy(sizePolicy3);
        gridLayout_3 = new QGridLayout(tabPlaylist);
        gridLayout_3->setSpacing(0);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        frame_2 = new QFrame(tabPlaylist);
        frame_2->setObjectName(QString::fromUtf8("frame_2"));
        sizePolicy3.setHeightForWidth(frame_2->sizePolicy().hasHeightForWidth());
        frame_2->setSizePolicy(sizePolicy3);
        frame_2->setMaximumSize(QSize(350, 16777215));
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        txt_playlistTotalTime = new QLabel(frame_2);
        txt_playlistTotalTime->setObjectName(QString::fromUtf8("txt_playlistTotalTime"));
        txt_playlistTotalTime->setGeometry(QRect(10, 10, 231, 31));
        QFont font3;
        font3.setPointSize(10);
        txt_playlistTotalTime->setFont(font3);
        layoutWidget = new QWidget(frame_2);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 40, 339, 96));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setSpacing(6);
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        checkBox_sum_to_playlist_time = new QCheckBox(layoutWidget);
        checkBox_sum_to_playlist_time->setObjectName(QString::fromUtf8("checkBox_sum_to_playlist_time"));
        checkBox_sum_to_playlist_time->setChecked(true);

        horizontalLayout_10->addWidget(checkBox_sum_to_playlist_time);

        checkBox_update_last_played_values = new QCheckBox(layoutWidget);
        checkBox_update_last_played_values->setObjectName(QString::fromUtf8("checkBox_update_last_played_values"));
        checkBox_update_last_played_values->setChecked(true);

        horizontalLayout_10->addWidget(checkBox_update_last_played_values);


        verticalLayout->addLayout(horizontalLayout_10);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        checkBox_random_jingles = new QCheckBox(layoutWidget);
        checkBox_random_jingles->setObjectName(QString::fromUtf8("checkBox_random_jingles"));

        horizontalLayout_9->addWidget(checkBox_random_jingles);

        spinBox_random_jingles_interval = new QSpinBox(layoutWidget);
        spinBox_random_jingles_interval->setObjectName(QString::fromUtf8("spinBox_random_jingles_interval"));

        horizontalLayout_9->addWidget(spinBox_random_jingles_interval);

        label_9 = new QLabel(layoutWidget);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        horizontalLayout_9->addWidget(label_9);


        verticalLayout->addLayout(horizontalLayout_9);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setSpacing(6);
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        label_3 = new QLabel(layoutWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_8->addWidget(label_3);

        spinBox_num_of_songs_to_add_random = new QSpinBox(layoutWidget);
        spinBox_num_of_songs_to_add_random->setObjectName(QString::fromUtf8("spinBox_num_of_songs_to_add_random"));

        horizontalLayout_8->addWidget(spinBox_num_of_songs_to_add_random);

        comboBox_random_add_genre = new QComboBox(layoutWidget);
        comboBox_random_add_genre->setObjectName(QString::fromUtf8("comboBox_random_add_genre"));

        horizontalLayout_8->addWidget(comboBox_random_add_genre);

        label_10 = new QLabel(layoutWidget);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        horizontalLayout_8->addWidget(label_10);

        bt_add_some_random_songs_from_genre = new QPushButton(layoutWidget);
        bt_add_some_random_songs_from_genre->setObjectName(QString::fromUtf8("bt_add_some_random_songs_from_genre"));
        QIcon icon26;
        icon26.addFile(QString::fromUtf8(":/icons/flat/Quick Mode On-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_add_some_random_songs_from_genre->setIcon(icon26);

        horizontalLayout_8->addWidget(bt_add_some_random_songs_from_genre);


        verticalLayout->addLayout(horizontalLayout_8);


        gridLayout_3->addWidget(frame_2, 2, 1, 1, 1);

        playlist = new QListWidget(tabPlaylist);
        playlist->setObjectName(QString::fromUtf8("playlist"));
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(10);
        sizePolicy5.setHeightForWidth(playlist->sizePolicy().hasHeightForWidth());
        playlist->setSizePolicy(sizePolicy5);
        playlist->setFont(font2);
        playlist->setAcceptDrops(true);
        playlist->setFrameShape(QFrame::NoFrame);
        playlist->setFrameShadow(QFrame::Plain);
        playlist->setLineWidth(1);
        playlist->setAutoScrollMargin(16);
        playlist->setAlternatingRowColors(true);
        playlist->setSelectionBehavior(QAbstractItemView::SelectRows);
        playlist->setIconSize(QSize(32, 32));
        playlist->setResizeMode(QListView::Adjust);
        playlist->setViewMode(QListView::ListMode);

        gridLayout_3->addWidget(playlist, 2, 0, 1, 1);

        QIcon icon27;
        icon27.addFile(QString::fromUtf8(":/icons/align-vertical-top.png"), QSize(), QIcon::Normal, QIcon::Off);
        tabWidget_2->addTab(tabPlaylist, icon27, QString());
        tabHistorylist = new QWidget();
        tabHistorylist->setObjectName(QString::fromUtf8("tabHistorylist"));
        sizePolicy3.setHeightForWidth(tabHistorylist->sizePolicy().hasHeightForWidth());
        tabHistorylist->setSizePolicy(sizePolicy3);
        tabHistorylist->setFont(font2);
        horizontalLayout_6 = new QHBoxLayout(tabHistorylist);
        horizontalLayout_6->setSpacing(0);
        horizontalLayout_6->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        historyList = new QListWidget(tabHistorylist);
        historyList->setObjectName(QString::fromUtf8("historyList"));
        historyList->setStyleSheet(QString::fromUtf8("background-color: rgb(206, 217, 255);"));
        historyList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        historyList->setProperty("showDropIndicator", QVariant(false));
        historyList->setSelectionBehavior(QAbstractItemView::SelectRows);

        horizontalLayout_6->addWidget(historyList);

        QIcon icon28;
        icon28.addFile(QString::fromUtf8(":/icons/align-vertical-bottom.png"), QSize(), QIcon::Normal, QIcon::Off);
        tabWidget_2->addTab(tabHistorylist, icon28, QString());
        tab_dj = new QWidget();
        tab_dj->setObjectName(QString::fromUtf8("tab_dj"));
        lp_1 = new QLabel(tab_dj);
        lp_1->setObjectName(QString::fromUtf8("lp_1"));
        lp_1->setGeometry(QRect(260, 30, 191, 161));
        lp_1->setAcceptDrops(true);
        lp_1->setPixmap(QPixmap(QString::fromUtf8(":/images/lp_player_p0.png")));
        lp_1->setScaledContents(true);
        lp_2 = new QLabel(tab_dj);
        lp_2->setObjectName(QString::fromUtf8("lp_2"));
        lp_2->setGeometry(QRect(620, 30, 191, 161));
        lp_2->setAcceptDrops(true);
        lp_2->setPixmap(QPixmap(QString::fromUtf8(":/images/lp_player_p0.png")));
        lp_2->setScaledContents(true);
        lp_1_txt_file = new QLabel(tab_dj);
        lp_1_txt_file->setObjectName(QString::fromUtf8("lp_1_txt_file"));
        lp_1_txt_file->setGeometry(QRect(10, 0, 611, 21));
        QFont font4;
        font4.setPointSize(7);
        lp_1_txt_file->setFont(font4);
        lp_2_txt_file = new QLabel(tab_dj);
        lp_2_txt_file->setObjectName(QString::fromUtf8("lp_2_txt_file"));
        lp_2_txt_file->setGeometry(QRect(640, 0, 831, 21));
        lp_2_txt_file->setFont(font4);
        lp_2_txt_file->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        lp_1_bt_play = new QPushButton(tab_dj);
        lp_1_bt_play->setObjectName(QString::fromUtf8("lp_1_bt_play"));
        lp_1_bt_play->setGeometry(QRect(10, 50, 111, 31));
        QFont font5;
        font5.setPointSize(8);
        lp_1_bt_play->setFont(font5);
        QIcon icon29;
        icon29.addFile(QString::fromUtf8(":/icons/ic_menu_play_clip.png"), QSize(), QIcon::Normal, QIcon::Off);
        lp_1_bt_play->setIcon(icon29);
        lp_1_bt_play_2 = new QPushButton(tab_dj);
        lp_1_bt_play_2->setObjectName(QString::fromUtf8("lp_1_bt_play_2"));
        lp_1_bt_play_2->setGeometry(QRect(950, 50, 111, 31));
        lp_1_bt_play_2->setFont(font5);
        lp_1_bt_play_2->setIcon(icon29);
        pushButton = new QPushButton(tab_dj);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(10, 130, 111, 31));
        QIcon icon30;
        icon30.addFile(QString::fromUtf8(":/icons/draw-rectangle.png"), QSize(), QIcon::Normal, QIcon::Off);
        pushButton->setIcon(icon30);
        pushButton_2 = new QPushButton(tab_dj);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(950, 130, 111, 31));
        pushButton_2->setIcon(icon30);
        lp_1_bt_pause = new QPushButton(tab_dj);
        lp_1_bt_pause->setObjectName(QString::fromUtf8("lp_1_bt_pause"));
        lp_1_bt_pause->setGeometry(QRect(10, 90, 111, 31));
        lp_1_bt_pause->setFont(font5);
        QIcon icon31;
        icon31.addFile(QString::fromUtf8(":/icons/ic_pause_light.png"), QSize(), QIcon::Normal, QIcon::Off);
        lp_1_bt_pause->setIcon(icon31);
        lp_2_bt_pause = new QPushButton(tab_dj);
        lp_2_bt_pause->setObjectName(QString::fromUtf8("lp_2_bt_pause"));
        lp_2_bt_pause->setGeometry(QRect(950, 90, 111, 31));
        lp_2_bt_pause->setFont(font5);
        lp_2_bt_pause->setIcon(icon31);
        horizontalSlider_lps_vol = new QSlider(tab_dj);
        horizontalSlider_lps_vol->setObjectName(QString::fromUtf8("horizontalSlider_lps_vol"));
        horizontalSlider_lps_vol->setGeometry(QRect(460, 145, 160, 41));
        QFont font6;
        font6.setBold(false);
        font6.setWeight(50);
        horizontalSlider_lps_vol->setFont(font6);
        horizontalSlider_lps_vol->setStyleSheet(QString::fromUtf8("selection-background-color: rgb(255, 255, 255);\n"
"background-color: rgb(255, 255, 255);"));
        horizontalSlider_lps_vol->setMaximum(200);
        horizontalSlider_lps_vol->setValue(100);
        horizontalSlider_lps_vol->setSliderPosition(100);
        horizontalSlider_lps_vol->setOrientation(Qt::Horizontal);
        horizontalSlider_lps_vol->setInvertedAppearance(false);
        horizontalSlider_lps_vol->setInvertedControls(false);
        lbl_total_time_lp1 = new QLabel(tab_dj);
        lbl_total_time_lp1->setObjectName(QString::fromUtf8("lbl_total_time_lp1"));
        lbl_total_time_lp1->setGeometry(QRect(130, 50, 121, 21));
        QFont font7;
        font7.setPointSize(9);
        lbl_total_time_lp1->setFont(font7);
        lbl_total_time_lp2 = new QLabel(tab_dj);
        lbl_total_time_lp2->setObjectName(QString::fromUtf8("lbl_total_time_lp2"));
        lbl_total_time_lp2->setGeometry(QRect(820, 50, 121, 20));
        lbl_total_time_lp2->setFont(font7);
        lbl_lp1_remaining = new QLabel(tab_dj);
        lbl_lp1_remaining->setObjectName(QString::fromUtf8("lbl_lp1_remaining"));
        lbl_lp1_remaining->setGeometry(QRect(130, 80, 121, 21));
        QFont font8;
        font8.setPointSize(13);
        font8.setBold(true);
        font8.setWeight(75);
        lbl_lp1_remaining->setFont(font8);
        lbl_lp1_remaining->setStyleSheet(QString::fromUtf8("color: rgb(255, 0, 0);"));
        lbl_lp2_remaining = new QLabel(tab_dj);
        lbl_lp2_remaining->setObjectName(QString::fromUtf8("lbl_lp2_remaining"));
        lbl_lp2_remaining->setGeometry(QRect(820, 70, 121, 21));
        lbl_lp2_remaining->setFont(font8);
        lbl_lp2_remaining->setStyleSheet(QString::fromUtf8("color: rgb(255, 0, 0);"));
        lbl_lp1_vol_level = new QLabel(tab_dj);
        lbl_lp1_vol_level->setObjectName(QString::fromUtf8("lbl_lp1_vol_level"));
        lbl_lp1_vol_level->setGeometry(QRect(460, 130, 31, 17));
        QFont font9;
        font9.setPointSize(8);
        font9.setBold(false);
        font9.setItalic(true);
        font9.setWeight(50);
        lbl_lp1_vol_level->setFont(font9);
        lbl_lp2_vol_level = new QLabel(tab_dj);
        lbl_lp2_vol_level->setObjectName(QString::fromUtf8("lbl_lp2_vol_level"));
        lbl_lp2_vol_level->setGeometry(QRect(580, 130, 31, 17));
        lbl_lp2_vol_level->setFont(font9);
        lbl_lp2_vol_level->setLayoutDirection(Qt::LeftToRight);
        lbl_lp2_vol_level->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        bt_center_lps_vol = new QToolButton(tab_dj);
        bt_center_lps_vol->setObjectName(QString::fromUtf8("bt_center_lps_vol"));
        bt_center_lps_vol->setGeometry(QRect(535, 124, 11, 23));
        QIcon icon32;
        icon32.addFile(QString::fromUtf8(":/icons/record.png"), QSize(), QIcon::Normal, QIcon::Off);
        tabWidget_2->addTab(tab_dj, icon32, QString());

        gridLayout_2->addWidget(tabWidget_2, 20, 0, 4, 2);

        pubWidget = new QTabWidget(widget);
        pubWidget->setObjectName(QString::fromUtf8("pubWidget"));
        sizePolicy1.setHeightForWidth(pubWidget->sizePolicy().hasHeightForWidth());
        pubWidget->setSizePolicy(sizePolicy1);
        pubWidget->setMinimumSize(QSize(0, 0));
        pubWidget->setSizeIncrement(QSize(0, 0));
        pubWidget->setBaseSize(QSize(0, 0));
        pubWidget->setFont(font2);
        pubWidget->setTabPosition(QTabWidget::South);
        pubWidget->setIconSize(QSize(24, 24));
        pubWidget->setElideMode(Qt::ElideNone);
        pubWidget->setDocumentMode(true);
        pubWidget->setTabBarAutoHide(true);
        tabMusic = new QWidget();
        tabMusic->setObjectName(QString::fromUtf8("tabMusic"));
        sizePolicy3.setHeightForWidth(tabMusic->sizePolicy().hasHeightForWidth());
        tabMusic->setSizePolicy(sizePolicy3);
        horizontalLayout_2 = new QHBoxLayout(tabMusic);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        musicView = new QTableView(tabMusic);
        musicView->setObjectName(QString::fromUtf8("musicView"));
        musicView->setStyleSheet(QString::fromUtf8("text-align:center;"));
        musicView->setFrameShape(QFrame::StyledPanel);
        musicView->setFrameShadow(QFrame::Plain);
        musicView->setLineWidth(3);
        musicView->setEditTriggers(QAbstractItemView::DoubleClicked);
        musicView->setDragEnabled(true);
        musicView->setDragDropOverwriteMode(true);
        musicView->setDragDropMode(QAbstractItemView::DragOnly);
        musicView->setDefaultDropAction(Qt::CopyAction);
        musicView->setAlternatingRowColors(true);
        musicView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        musicView->setSelectionBehavior(QAbstractItemView::SelectItems);
        musicView->setIconSize(QSize(48, 48));
        musicView->setShowGrid(true);
        musicView->setGridStyle(Qt::SolidLine);
        musicView->setSortingEnabled(true);
        musicView->setWordWrap(true);
        musicView->setCornerButtonEnabled(true);
        musicView->horizontalHeader()->setCascadingSectionResizes(true);
        musicView->horizontalHeader()->setMinimumSectionSize(40);
        musicView->horizontalHeader()->setDefaultSectionSize(200);
        musicView->horizontalHeader()->setStretchLastSection(true);
        musicView->verticalHeader()->setVisible(false);
        musicView->verticalHeader()->setCascadingSectionResizes(false);
        musicView->verticalHeader()->setMinimumSectionSize(20);
        musicView->verticalHeader()->setDefaultSectionSize(35);

        horizontalLayout_2->addWidget(musicView);

        pubWidget->addTab(tabMusic, icon2, QString());
        tabJingles = new QWidget();
        tabJingles->setObjectName(QString::fromUtf8("tabJingles"));
        sizePolicy3.setHeightForWidth(tabJingles->sizePolicy().hasHeightForWidth());
        tabJingles->setSizePolicy(sizePolicy3);
        horizontalLayout_3 = new QHBoxLayout(tabJingles);
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        jinglesView = new QTableView(tabJingles);
        jinglesView->setObjectName(QString::fromUtf8("jinglesView"));
        jinglesView->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);
        jinglesView->setTabKeyNavigation(false);
        jinglesView->setProperty("showDropIndicator", QVariant(true));
        jinglesView->setDragDropMode(QAbstractItemView::DragOnly);
        jinglesView->setAlternatingRowColors(true);
        jinglesView->setSelectionMode(QAbstractItemView::SingleSelection);
        jinglesView->setSelectionBehavior(QAbstractItemView::SelectRows);
        jinglesView->horizontalHeader()->setMinimumSectionSize(48);
        jinglesView->horizontalHeader()->setDefaultSectionSize(600);
        jinglesView->verticalHeader()->setDefaultSectionSize(48);

        horizontalLayout_3->addWidget(jinglesView);

        pubWidget->addTab(tabJingles, icon5, QString());
        tabPub = new QWidget();
        tabPub->setObjectName(QString::fromUtf8("tabPub"));
        sizePolicy3.setHeightForWidth(tabPub->sizePolicy().hasHeightForWidth());
        tabPub->setSizePolicy(sizePolicy3);
        horizontalLayout_4 = new QHBoxLayout(tabPub);
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        pubView = new QTableView(tabPub);
        pubView->setObjectName(QString::fromUtf8("pubView"));
        pubView->setDragDropMode(QAbstractItemView::DragOnly);
        pubView->setAlternatingRowColors(true);
        pubView->setSelectionMode(QAbstractItemView::SingleSelection);
        pubView->setSelectionBehavior(QAbstractItemView::SelectRows);
        pubView->horizontalHeader()->setMinimumSectionSize(150);
        pubView->horizontalHeader()->setDefaultSectionSize(600);
        pubView->verticalHeader()->setVisible(false);
        pubView->verticalHeader()->setDefaultSectionSize(48);

        horizontalLayout_4->addWidget(pubView);

        pubWidget->addTab(tabPub, icon6, QString());
        tabPrograms = new QWidget();
        tabPrograms->setObjectName(QString::fromUtf8("tabPrograms"));
        sizePolicy3.setHeightForWidth(tabPrograms->sizePolicy().hasHeightForWidth());
        tabPrograms->setSizePolicy(sizePolicy3);
        horizontalLayout_5 = new QHBoxLayout(tabPrograms);
        horizontalLayout_5->setSpacing(0);
        horizontalLayout_5->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        programsView = new QTableView(tabPrograms);
        programsView->setObjectName(QString::fromUtf8("programsView"));
        programsView->setDragDropMode(QAbstractItemView::DragOnly);
        programsView->setAlternatingRowColors(true);
        programsView->setSelectionMode(QAbstractItemView::SingleSelection);
        programsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        programsView->horizontalHeader()->setMinimumSectionSize(150);
        programsView->horizontalHeader()->setDefaultSectionSize(600);
        programsView->verticalHeader()->setVisible(false);
        programsView->verticalHeader()->setDefaultSectionSize(48);

        horizontalLayout_5->addWidget(programsView);

        pubWidget->addTab(tabPrograms, icon13, QString());

        gridLayout_2->addWidget(pubWidget, 24, 0, 1, 1);

        page_FTP_Connection = new QToolBox(widget);
        page_FTP_Connection->setObjectName(QString::fromUtf8("page_FTP_Connection"));
        sizePolicy3.setHeightForWidth(page_FTP_Connection->sizePolicy().hasHeightForWidth());
        page_FTP_Connection->setSizePolicy(sizePolicy3);
        page_FTP_Connection->setMinimumSize(QSize(0, 207));
        page_FTP_Connection->setMaximumSize(QSize(350, 16777212));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        page->setGeometry(QRect(0, 0, 350, 203));
        layoutWidget1 = new QWidget(page);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(10, 0, 281, 28));
        horizontalLayout = new QHBoxLayout(layoutWidget1);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        txt_search = new QLineEdit(layoutWidget1);
        txt_search->setObjectName(QString::fromUtf8("txt_search"));

        horizontalLayout->addWidget(txt_search);

        bt_reset = new QPushButton(page);
        bt_reset->setObjectName(QString::fromUtf8("bt_reset"));
        bt_reset->setGeometry(QRect(180, 30, 111, 25));
        bt_search = new QPushButton(page);
        bt_search->setObjectName(QString::fromUtf8("bt_search"));
        bt_search->setGeometry(QRect(10, 30, 111, 25));
        QIcon icon33;
        icon33.addFile(QString::fromUtf8(":/icons/ic_menu_search.png"), QSize(), QIcon::Normal, QIcon::Off);
        page_FTP_Connection->addItem(page, icon33, QString::fromUtf8("Search"));
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        page_2->setGeometry(QRect(0, 0, 350, 203));
        bt_apply_filter = new QPushButton(page_2);
        bt_apply_filter->setObjectName(QString::fromUtf8("bt_apply_filter"));
        bt_apply_filter->setGeometry(QRect(250, 40, 81, 41));
        checkBox_filter_genre1 = new QCheckBox(page_2);
        checkBox_filter_genre1->setObjectName(QString::fromUtf8("checkBox_filter_genre1"));
        checkBox_filter_genre1->setGeometry(QRect(20, 0, 141, 22));
        cBoxGenre1 = new QComboBox(page_2);
        cBoxGenre1->setObjectName(QString::fromUtf8("cBoxGenre1"));
        cBoxGenre1->setGeometry(QRect(40, 20, 201, 27));
        cBoxGenre1->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));
        cBoxGenre2 = new QComboBox(page_2);
        cBoxGenre2->setObjectName(QString::fromUtf8("cBoxGenre2"));
        cBoxGenre2->setGeometry(QRect(40, 72, 201, 31));
        cBoxGenre2->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));
        checkBox_filter_genre2 = new QCheckBox(page_2);
        checkBox_filter_genre2->setObjectName(QString::fromUtf8("checkBox_filter_genre2"));
        checkBox_filter_genre2->setGeometry(QRect(20, 50, 151, 22));
        QIcon icon34;
        icon34.addFile(QString::fromUtf8(":/icons/view-filter.png"), QSize(), QIcon::Normal, QIcon::Off);
        page_FTP_Connection->addItem(page_2, icon34, QString::fromUtf8("fillters"));
        page_3 = new QWidget();
        page_3->setObjectName(QString::fromUtf8("page_3"));
        page_3->setGeometry(QRect(0, 0, 350, 203));
        bt_updateTables = new QPushButton(page_3);
        bt_updateTables->setObjectName(QString::fromUtf8("bt_updateTables"));
        bt_updateTables->setGeometry(QRect(10, 0, 121, 21));
        QIcon icon35;
        icon35.addFile(QString::fromUtf8(":/icons/ic_menu_refresh.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_updateTables->setIcon(icon35);
        bt_sndconv = new QPushButton(page_3);
        bt_sndconv->setObjectName(QString::fromUtf8("bt_sndconv"));
        bt_sndconv->setGeometry(QRect(140, 0, 121, 21));
        QIcon icon36;
        icon36.addFile(QString::fromUtf8(":/icons/soundconverter.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_sndconv->setIcon(icon36);
        cbox_multi_select = new QComboBox(page_3);
        cbox_multi_select->addItem(QString());
        cbox_multi_select->addItem(QString());
        cbox_multi_select->addItem(QString());
        cbox_multi_select->addItem(QString());
        cbox_multi_select->addItem(QString());
        cbox_multi_select->addItem(QString());
        cbox_multi_select->addItem(QString());
        cbox_multi_select->setObjectName(QString::fromUtf8("cbox_multi_select"));
        cbox_multi_select->setGeometry(QRect(10, 40, 211, 21));
        QSizePolicy sizePolicy6(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(cbox_multi_select->sizePolicy().hasHeightForWidth());
        cbox_multi_select->setSizePolicy(sizePolicy6);
        cbox_multi_select->setMinimumSize(QSize(200, 0));
        cbox_multi_select->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));
        bt_apply_multi_selection = new QPushButton(page_3);
        bt_apply_multi_selection->setObjectName(QString::fromUtf8("bt_apply_multi_selection"));
        bt_apply_multi_selection->setGeometry(QRect(230, 40, 50, 21));
        QSizePolicy sizePolicy7(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy7.setHorizontalStretch(0);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(bt_apply_multi_selection->sizePolicy().hasHeightForWidth());
        bt_apply_multi_selection->setSizePolicy(sizePolicy7);
        bt_apply_multi_selection->setMaximumSize(QSize(50, 16777215));
        QIcon icon37;
        icon37.addFile(QString::fromUtf8(":/icons/linux-logo.png"), QSize(), QIcon::Normal, QIcon::Off);
        page_FTP_Connection->addItem(page_3, icon37, QString::fromUtf8("Extras"));
        Radio = new QWidget();
        Radio->setObjectName(QString::fromUtf8("Radio"));
        Radio->setGeometry(QRect(0, 0, 350, 203));
        Radio->setStyleSheet(QString::fromUtf8(""));
        bt_rol_streaming_play = new QPushButton(Radio);
        bt_rol_streaming_play->setObjectName(QString::fromUtf8("bt_rol_streaming_play"));
        bt_rol_streaming_play->setEnabled(false);
        bt_rol_streaming_play->setGeometry(QRect(230, 10, 31, 21));
        QIcon icon38;
        icon38.addFile(QString::fromUtf8(":/icons/player_play.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_rol_streaming_play->setIcon(icon38);
        label_4 = new QLabel(Radio);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(10, 10, 191, 21));
        bt_rol_streaming_stop = new QPushButton(Radio);
        bt_rol_streaming_stop->setObjectName(QString::fromUtf8("bt_rol_streaming_stop"));
        bt_rol_streaming_stop->setEnabled(false);
        bt_rol_streaming_stop->setGeometry(QRect(290, 10, 31, 21));
        QIcon icon39;
        icon39.addFile(QString::fromUtf8(":/icons/media-playback-stop.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_rol_streaming_stop->setIcon(icon39);
        page_FTP_Connection->addItem(Radio, icon, QString::fromUtf8("Streaming Client"));
        page_4 = new QWidget();
        page_4->setObjectName(QString::fromUtf8("page_4"));
        page_4->setEnabled(true);
        page_4->setGeometry(QRect(0, 0, 350, 203));
        label_5 = new QLabel(page_4);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(60, 51, 141, 17));
        label_6 = new QLabel(page_4);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(60, 79, 141, 17));
        label_7 = new QLabel(page_4);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(60, 101, 141, 17));
        bt_start_streaming = new QPushButton(page_4);
        bt_start_streaming->setObjectName(QString::fromUtf8("bt_start_streaming"));
        bt_start_streaming->setGeometry(QRect(20, 0, 71, 21));
        lbl_butt = new QLabel(page_4);
        lbl_butt->setObjectName(QString::fromUtf8("lbl_butt"));
        lbl_butt->setGeometry(QRect(220, 52, 121, 17));
        lbl_butt->setStyleSheet(QString::fromUtf8("color: rgb(98, 135, 255);"));
        lbl_ddns = new QLabel(page_4);
        lbl_ddns->setObjectName(QString::fromUtf8("lbl_ddns"));
        lbl_ddns->setGeometry(QRect(220, 78, 121, 17));
        lbl_ddns->setStyleSheet(QString::fromUtf8("color: rgb(98, 135, 255);"));
        lbl_port = new QLabel(page_4);
        lbl_port->setObjectName(QString::fromUtf8("lbl_port"));
        lbl_port->setGeometry(QRect(220, 100, 121, 17));
        lbl_port->setStyleSheet(QString::fromUtf8("color: rgb(98, 135, 255);"));
        bt_takeOver = new QPushButton(page_4);
        bt_takeOver->setObjectName(QString::fromUtf8("bt_takeOver"));
        bt_takeOver->setGeometry(QRect(180, 0, 141, 21));
        bt_stop_streaming = new QPushButton(page_4);
        bt_stop_streaming->setObjectName(QString::fromUtf8("bt_stop_streaming"));
        bt_stop_streaming->setGeometry(QRect(100, 0, 71, 21));
        lbl_icecast = new QLabel(page_4);
        lbl_icecast->setObjectName(QString::fromUtf8("lbl_icecast"));
        lbl_icecast->setGeometry(QRect(220, 30, 121, 17));
        lbl_icecast->setStyleSheet(QString::fromUtf8("color: rgb(98, 135, 255);"));
        label_8 = new QLabel(page_4);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(60, 30, 141, 17));
        bt_icecast = new QToolButton(page_4);
        bt_icecast->setObjectName(QString::fromUtf8("bt_icecast"));
        bt_icecast->setGeometry(QRect(20, 28, 24, 21));
        QIcon icon40;
        icon40.addFile(QString::fromUtf8(":/icons/quickopen-function.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_icecast->setIcon(icon40);
        bt_butt = new QToolButton(page_4);
        bt_butt->setObjectName(QString::fromUtf8("bt_butt"));
        bt_butt->setGeometry(QRect(20, 50, 24, 21));
        bt_butt->setIcon(icon40);
        bt_ddns = new QToolButton(page_4);
        bt_ddns->setObjectName(QString::fromUtf8("bt_ddns"));
        bt_ddns->setGeometry(QRect(20, 74, 24, 21));
        bt_ddns->setIcon(icon40);
        bt_portTest = new QToolButton(page_4);
        bt_portTest->setObjectName(QString::fromUtf8("bt_portTest"));
        bt_portTest->setGeometry(QRect(20, 100, 24, 21));
        bt_portTest->setIcon(icon40);
        line = new QFrame(page_4);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(10, 43, 311, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line_2 = new QFrame(page_4);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setGeometry(QRect(10, 65, 311, 16));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);
        line_3 = new QFrame(page_4);
        line_3->setObjectName(QString::fromUtf8("line_3"));
        line_3->setGeometry(QRect(10, 90, 311, 16));
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);
        lbl_streamURL = new QLabel(page_4);
        lbl_streamURL->setObjectName(QString::fromUtf8("lbl_streamURL"));
        lbl_streamURL->setGeometry(QRect(20, 130, 301, 17));
        QIcon icon41;
        icon41.addFile(QString::fromUtf8(":/icons/earth.png"), QSize(), QIcon::Normal, QIcon::Off);
        page_FTP_Connection->addItem(page_4, icon41, QString::fromUtf8("Streaming Server"));
        line_3->raise();
        line_2->raise();
        line->raise();
        label_5->raise();
        label_6->raise();
        label_7->raise();
        bt_start_streaming->raise();
        lbl_butt->raise();
        lbl_ddns->raise();
        lbl_port->raise();
        bt_takeOver->raise();
        bt_stop_streaming->raise();
        lbl_icecast->raise();
        label_8->raise();
        bt_icecast->raise();
        bt_butt->raise();
        bt_ddns->raise();
        bt_portTest->raise();
        lbl_streamURL->raise();

        gridLayout_2->addWidget(page_FTP_Connection, 24, 1, 1, 1);

        frame_4 = new QFrame(widget);
        frame_4->setObjectName(QString::fromUtf8("frame_4"));
        sizePolicy3.setHeightForWidth(frame_4->sizePolicy().hasHeightForWidth());
        frame_4->setSizePolicy(sizePolicy3);
        frame_4->setMinimumSize(QSize(0, 129));
        frame_4->setFrameShape(QFrame::NoFrame);
        frame_4->setFrameShadow(QFrame::Raised);
        btPlay = new QPushButton(frame_4);
        btPlay->setObjectName(QString::fromUtf8("btPlay"));
        btPlay->setGeometry(QRect(9, 10, 221, 55));
        QSizePolicy sizePolicy8(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy8.setHorizontalStretch(0);
        sizePolicy8.setVerticalStretch(30);
        sizePolicy8.setHeightForWidth(btPlay->sizePolicy().hasHeightForWidth());
        btPlay->setSizePolicy(sizePolicy8);
        btPlay->setMinimumSize(QSize(0, 55));
        QIcon icon42;
        icon42.addFile(QString::fromUtf8(":/icons/flat/Play-64.png"), QSize(), QIcon::Normal, QIcon::Off);
        btPlay->setIcon(icon42);
        btPlay->setIconSize(QSize(32, 32));
        btStop = new QPushButton(frame_4);
        btStop->setObjectName(QString::fromUtf8("btStop"));
        btStop->setGeometry(QRect(239, 40, 171, 25));
        QIcon icon43;
        icon43.addFile(QString::fromUtf8(":/icons/flat/Stop Sign-32.png"), QSize(), QIcon::Normal, QIcon::Off);
        btStop->setIcon(icon43);
        btStop->setIconSize(QSize(32, 32));
        btPlayNext = new QPushButton(frame_4);
        btPlayNext->setObjectName(QString::fromUtf8("btPlayNext"));
        btPlayNext->setGeometry(QRect(239, 10, 171, 25));
        QIcon icon44;
        icon44.addFile(QString::fromUtf8(":/icons/flat/Fast Forward-32.png"), QSize(), QIcon::Normal, QIcon::Off);
        btPlayNext->setIcon(icon44);
        btPlayNext->setIconSize(QSize(32, 32));
        bt_rec = new QPushButton(frame_4);
        bt_rec->setObjectName(QString::fromUtf8("bt_rec"));
        bt_rec->setGeometry(QRect(420, 40, 161, 25));
        QIcon icon45;
        icon45.addFile(QString::fromUtf8(":/icons/flat/Record-32.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_rec->setIcon(icon45);
        bt_rec->setIconSize(QSize(32, 32));
        txt_recTime = new QLabel(frame_4);
        txt_recTime->setObjectName(QString::fromUtf8("txt_recTime"));
        txt_recTime->setGeometry(QRect(420, 10, 150, 20));
        QFont font10;
        font10.setPointSize(11);
        font10.setBold(true);
        font10.setItalic(false);
        font10.setWeight(75);
        txt_recTime->setFont(font10);
        txt_recTime->setStyleSheet(QString::fromUtf8("color: rgb(255, 0, 0);"));
        txt_recTime->setAlignment(Qt::AlignCenter);
        txtNowPlaying = new QLabel(frame_4);
        txtNowPlaying->setObjectName(QString::fromUtf8("txtNowPlaying"));
        txtNowPlaying->setGeometry(QRect(20, 90, 1161, 31));
        QFont font11;
        font11.setPointSize(12);
        font11.setBold(true);
        font11.setUnderline(false);
        font11.setWeight(75);
        txtNowPlaying->setFont(font11);
        txtNowPlaying->setStyleSheet(QString::fromUtf8("color: rgb(124, 124, 186);border-radius:8px;"));
        txt_ProgramName = new QLabel(frame_4);
        txt_ProgramName->setObjectName(QString::fromUtf8("txt_ProgramName"));
        txt_ProgramName->setGeometry(QRect(510, 10, 541, 37));
        QFont font12;
        font12.setBold(true);
        font12.setItalic(true);
        font12.setUnderline(false);
        font12.setWeight(75);
        txt_ProgramName->setFont(font12);
        txt_ProgramName->setStyleSheet(QString::fromUtf8("background-color:#FFE329;padding:5px;"));
        txt_ProgramName->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        bt_ProgramStopandProcess = new QPushButton(frame_4);
        bt_ProgramStopandProcess->setObjectName(QString::fromUtf8("bt_ProgramStopandProcess"));
        bt_ProgramStopandProcess->setGeometry(QRect(750, 10, 301, 34));
        bt_ProgramStopandProcess->setMinimumSize(QSize(0, 34));
        QIcon icon46;
        icon46.addFile(QString::fromUtf8(":/icons/googlemusicframe.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_ProgramStopandProcess->setIcon(icon46);
        bt_ProgramStopandProcess->setIconSize(QSize(24, 24));
        txt_uploadingPrograms = new QLabel(frame_4);
        txt_uploadingPrograms->setObjectName(QString::fromUtf8("txt_uploadingPrograms"));
        txt_uploadingPrograms->setGeometry(QRect(510, 0, 541, 51));
        QFont font13;
        font13.setFamily(QString::fromUtf8("Noto Sans [monotype]"));
        font13.setPointSize(20);
        font13.setBold(false);
        font13.setItalic(true);
        font13.setWeight(9);
        txt_uploadingPrograms->setFont(font13);
        txt_uploadingPrograms->setStyleSheet(QString::fromUtf8("background-color:rgba(255,255,255,.86);\n"
"font: 75 italic 20pt \"Noto Sans [monotype]\";"));
        txt_uploadingPrograms->setAlignment(Qt::AlignCenter);
        txtDuration = new QLabel(frame_4);
        txtDuration->setObjectName(QString::fromUtf8("txtDuration"));
        txtDuration->setGeometry(QRect(510, 45, 581, 25));
        sizePolicy3.setHeightForWidth(txtDuration->sizePolicy().hasHeightForWidth());
        txtDuration->setSizePolicy(sizePolicy3);
        QFont font14;
        font14.setPointSize(14);
        font14.setBold(true);
        font14.setItalic(false);
        font14.setWeight(75);
        txtDuration->setFont(font14);
        txtDuration->setStyleSheet(QString::fromUtf8("color: rgb(0, 75, 167);"));
        txtDuration->setAlignment(Qt::AlignCenter);
        layoutWidget2 = new QWidget(frame_4);
        layoutWidget2->setObjectName(QString::fromUtf8("layoutWidget2"));
        layoutWidget2->setGeometry(QRect(10, 66, 1041, 20));
        horizontalLayout_7 = new QHBoxLayout(layoutWidget2);
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(layoutWidget2);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_7->addWidget(label);

        sliderVolume = new QSlider(layoutWidget2);
        sliderVolume->setObjectName(QString::fromUtf8("sliderVolume"));
        sliderVolume->setValue(99);
        sliderVolume->setOrientation(Qt::Horizontal);
        sliderVolume->setTickInterval(1);

        horizontalLayout_7->addWidget(sliderVolume);

        label_2 = new QLabel(layoutWidget2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_7->addWidget(label_2);

        sliderProgress = new QSlider(layoutWidget2);
        sliderProgress->setObjectName(QString::fromUtf8("sliderProgress"));
        sliderProgress->setStyleSheet(QString::fromUtf8(""));
        sliderProgress->setOrientation(Qt::Horizontal);

        horizontalLayout_7->addWidget(sliderProgress);

        txt_creatingPrograms = new QLabel(frame_4);
        txt_creatingPrograms->setObjectName(QString::fromUtf8("txt_creatingPrograms"));
        txt_creatingPrograms->setGeometry(QRect(510, 0, 541, 51));
        txt_creatingPrograms->setFont(font13);
        txt_creatingPrograms->setStyleSheet(QString::fromUtf8("background-color:rgba(255,255,255,.86);\n"
"font: 75 italic 20pt \"Noto Sans [monotype]\";"));
        txt_creatingPrograms->setAlignment(Qt::AlignCenter);
        led_rec = new QLabel(frame_4);
        led_rec->setObjectName(QString::fromUtf8("led_rec"));
        led_rec->setGeometry(QRect(465, 43, 22, 20));
        led_rec->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 247, 0); border-radius:8px;"));
        txt_loading = new QLabel(frame_4);
        txt_loading->setObjectName(QString::fromUtf8("txt_loading"));
        txt_loading->setGeometry(QRect(510, 0, 541, 56));
        txt_loading->setFont(font13);
        txt_loading->setStyleSheet(QString::fromUtf8("background-color:rgba(255,255,255,.86);\n"
"font: 75 italic 20pt \"Noto Sans [monotype]\";"));
        txt_loading->setAlignment(Qt::AlignCenter);
        bt_pause_rec = new QToolButton(frame_4);
        bt_pause_rec->setObjectName(QString::fromUtf8("bt_pause_rec"));
        bt_pause_rec->setGeometry(QRect(550, 42, 25, 19));
        QIcon icon47;
        icon47.addFile(QString::fromUtf8(":/icons/flat/Pause-32.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_pause_rec->setIcon(icon47);
        bt_pause_play = new QToolButton(frame_4);
        bt_pause_play->setObjectName(QString::fromUtf8("bt_pause_play"));
        bt_pause_play->setGeometry(QRect(200, 35, 25, 25));
        bt_pause_play->setIcon(icon47);
        txt_ProgramName->raise();
        btPlay->raise();
        btStop->raise();
        btPlayNext->raise();
        txtNowPlaying->raise();
        bt_ProgramStopandProcess->raise();
        txtDuration->raise();
        layoutWidget->raise();
        txt_creatingPrograms->raise();
        txt_loading->raise();
        txt_uploadingPrograms->raise();
        bt_pause_play->raise();
        bt_rec->raise();
        led_rec->raise();
        txt_recTime->raise();
        bt_pause_rec->raise();

        gridLayout_2->addWidget(frame_4, 0, 0, 5, 1);


        gridLayout->addWidget(widget, 0, 0, 1, 1);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setSpacing(0);
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        horizontalLayout_11->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalLayout_11->setContentsMargins(9, 0, -1, 0);
        NetpackLogo = new QLabel(centralWidget);
        NetpackLogo->setObjectName(QString::fromUtf8("NetpackLogo"));
        sizePolicy6.setHeightForWidth(NetpackLogo->sizePolicy().hasHeightForWidth());
        NetpackLogo->setSizePolicy(sizePolicy6);
        NetpackLogo->setMinimumSize(QSize(100, 50));
        NetpackLogo->setMaximumSize(QSize(100, 50));
        NetpackLogo->setSizeIncrement(QSize(0, 0));
        NetpackLogo->setLayoutDirection(Qt::RightToLeft);
        NetpackLogo->setAutoFillBackground(false);
        NetpackLogo->setStyleSheet(QString::fromUtf8("padding-top:10px;"));
        NetpackLogo->setPixmap(QPixmap(QString::fromUtf8(":/images/novo logo_12_cfundo2_900.png")));
        NetpackLogo->setScaledContents(true);
        NetpackLogo->setAlignment(Qt::AlignCenter);
        NetpackLogo->setMargin(2);
        NetpackLogo->setIndent(10);
        NetpackLogo->setOpenExternalLinks(false);

        horizontalLayout_11->addWidget(NetpackLogo);

        txt_bottom_info = new QLabel(centralWidget);
        txt_bottom_info->setObjectName(QString::fromUtf8("txt_bottom_info"));
        txt_bottom_info->setMinimumSize(QSize(0, 32));
        QFont font15;
        font15.setFamily(QString::fromUtf8("Ubuntu"));
        font15.setPointSize(7);
        font15.setBold(false);
        font15.setItalic(true);
        font15.setWeight(50);
        txt_bottom_info->setFont(font15);
        txt_bottom_info->setLayoutDirection(Qt::LeftToRight);
        txt_bottom_info->setStyleSheet(QString::fromUtf8("margin-right:15px;"));
        txt_bottom_info->setLineWidth(2);
        txt_bottom_info->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        txt_bottom_info->setWordWrap(true);

        horizontalLayout_11->addWidget(txt_bottom_info);


        gridLayout->addLayout(horizontalLayout_11, 3, 0, 2, 1);

        player->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(player);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1478, 23));
        menuBar->setFont(font);
        menuBar->setStyleSheet(QString::fromUtf8("color: rgb(117, 117, 175);"));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        QFont font16;
        font16.setBold(true);
        font16.setWeight(75);
        font16.setStrikeOut(false);
        menuFile->setFont(font16);
        menuFile->setStyleSheet(QString::fromUtf8("color: rgb(117, 117, 175); border-right: 1px solid rgb(124, 124, 186); border-left: 1px solid rgb(124, 124, 186); border-bottom: 1px solid rgb(124, 124, 186);"));
        menuPlaylists = new QMenu(menuFile);
        menuPlaylists->setObjectName(QString::fromUtf8("menuPlaylists"));
        QIcon icon48;
        icon48.addFile(QString::fromUtf8(":/icons/align-vertical-center.png"), QSize(), QIcon::Normal, QIcon::Off);
        menuPlaylists->setIcon(icon48);
        menuDatabase = new QMenu(menuBar);
        menuDatabase->setObjectName(QString::fromUtf8("menuDatabase"));
        menuDatabase->setStyleSheet(QString::fromUtf8("color: rgb(117, 117, 175); border-right: 1px solid rgb(124, 124, 186); border-left: 1px solid rgb(124, 124, 186); border-bottom: 1px solid rgb(124, 124, 186);"));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        menuHelp->setStyleSheet(QString::fromUtf8("color: rgb(117, 117, 175); border-right: 1px solid rgb(124, 124, 186); border-left: 1px solid rgb(124, 124, 186); border-bottom: 1px solid rgb(124, 124, 186);"));
        menuXFB = new QMenu(menuBar);
        menuXFB->setObjectName(QString::fromUtf8("menuXFB"));
        menuXFB->setFont(font);
        menuXFB->setStyleSheet(QString::fromUtf8("color: rgb(117, 117, 175); border-right: 1px solid rgb(124, 124, 186); border-left: 1px solid rgb(124, 124, 186); border-bottom: 1px solid rgb(124, 124, 186);"));
        menuClient_3 = new QMenu(menuXFB);
        menuClient_3->setObjectName(QString::fromUtf8("menuClient_3"));
        QIcon icon49;
        icon49.addFile(QString::fromUtf8(":/icons/view-media-artist.png"), QSize(), QIcon::Normal, QIcon::Off);
        menuClient_3->setIcon(icon49);
        menuServer = new QMenu(menuXFB);
        menuServer->setObjectName(QString::fromUtf8("menuServer"));
        menuServer->setFont(font);
        QIcon icon50;
        icon50.addFile(QString::fromUtf8(":/usr/share/icons/Mint-X/apps/48/radio.png"), QSize(), QIcon::Normal, QIcon::Off);
        menuServer->setIcon(icon50);
        player->setMenuBar(menuBar);
        statusBar = new QStatusBar(player);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        player->setStatusBar(statusBar);
#if QT_CONFIG(shortcut)
#endif // QT_CONFIG(shortcut)

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuDatabase->menuAction());
        menuBar->addAction(menuXFB->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionOpen);
        menuFile->addAction(menuPlaylists->menuAction());
        menuPlaylists->addAction(actionSave_Playlist);
        menuPlaylists->addAction(actionLoad_Playlist);
        menuPlaylists->addAction(actionClear_Playlist);
        menuDatabase->addAction(actionAdd_a_single_song);
        menuDatabase->addAction(actionAdd_a_song_from_Youtube_or_Other);
        menuDatabase->addAction(actionAdd_all_songs_in_a_folder);
        menuDatabase->addAction(actionAdd_Jingle);
        menuDatabase->addAction(actionAdd_a_publicity);
        menuDatabase->addAction(actionAdd_a_program);
        menuDatabase->addAction(actionManage_Genres);
        menuDatabase->addSeparator();
        menuDatabase->addAction(actionCheck_the_Database_records);
        menuDatabase->addAction(actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation);
        menuDatabase->addSeparator();
        menuDatabase->addAction(actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database);
        menuDatabase->addSeparator();
        menuDatabase->addAction(actionConvert_all_musics_in_the_database_to_mp3);
        menuDatabase->addAction(actionConvert_all_musics_in_the_database_to_ogg);
        menuHelp->addAction(actionAbout);
        menuXFB->addAction(actionFullScreen);
        menuXFB->addAction(actionOptions);
        menuXFB->addAction(menuClient_3->menuAction());
        menuXFB->addAction(menuServer->menuAction());
        menuXFB->addSeparator();
        menuXFB->addAction(actionUpdate_System);
        menuXFB->addAction(actionUpdate_Dinamic_Server_s_IP);
        menuClient_3->addAction(actionRecord_a_new_Program);
        menuClient_3->addAction(actionMake_a_program_from_this_playlist);
        menuServer->addAction(actionForce_an_FTP_Check);
        menuServer->addAction(actionForce_monitorization);

        retranslateUi(player);

        bt_autoMode->setDefault(false);
        tabWidget_2->setCurrentIndex(0);
        playlist->setCurrentRow(-1);
        pubWidget->setCurrentIndex(0);
        page_FTP_Connection->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(player);
    } // setupUi

    void retranslateUi(QMainWindow *player)
    {
        player->setWindowTitle(QCoreApplication::translate("player", "XFB", nullptr));
        actionOpen->setText(QCoreApplication::translate("player", "Open", nullptr));
        actionAdd_a_single_song->setText(QCoreApplication::translate("player", "Add a single song", nullptr));
        actionAdd_all_songs_in_a_folder->setText(QCoreApplication::translate("player", "Add all songs in a folder", nullptr));
        actionManage_Genres->setText(QCoreApplication::translate("player", "Manage genres", nullptr));
        actionAdd_Jingle->setText(QCoreApplication::translate("player", "Add a jingle", nullptr));
        actionAdd_a_publicity->setText(QCoreApplication::translate("player", "Add a publicity", nullptr));
        actionOptions->setText(QCoreApplication::translate("player", "Options", nullptr));
        actionAbout->setText(QCoreApplication::translate("player", "About", nullptr));
        actionAdd_a_song_from_Youtube_or_Other->setText(QCoreApplication::translate("player", "Add a song from Youtube", nullptr));
        actionSave_Playlist->setText(QCoreApplication::translate("player", "Save Playlist", nullptr));
        actionClear_Playlist->setText(QCoreApplication::translate("player", "Clear Playlist", nullptr));
        actionLoad_Playlist->setText(QCoreApplication::translate("player", "Load Playlist", nullptr));
        actionRecord_a_new_Program->setText(QCoreApplication::translate("player", "Record a new Program", nullptr));
        action123->setText(QCoreApplication::translate("player", "123", nullptr));
        actionAdd_a_program->setText(QCoreApplication::translate("player", "Add a program", nullptr));
        actionFullScreen->setText(QCoreApplication::translate("player", "FullScreen", nullptr));
        actionForce_an_FTP_Check->setText(QCoreApplication::translate("player", "Force FTP check", nullptr));
        actionForce_an_FTP_Check->setIconText(QCoreApplication::translate("player", "Force FTP Check", nullptr));
        actionMake_a_program_from_this_playlist->setText(QCoreApplication::translate("player", "Make a program from this playlist", nullptr));
        actionCheck_the_Database_records->setText(QCoreApplication::translate("player", "Check & Update Music Table Records", nullptr));
        actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation->setText(QCoreApplication::translate("player", "Check database data and DELETE all invalid records without confirmation", nullptr));
        actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database->setText(QCoreApplication::translate("player", "AutoTrim the silence from the start and the end of all music tracks in the database", nullptr));
        actionUpdate_System->setText(QCoreApplication::translate("player", "Check for XFB and System updates", nullptr));
        actionConvert_all_musics_in_the_database_to_mp3->setText(QCoreApplication::translate("player", "Convert all musics in the database to mp3", nullptr));
        actionConvert_all_musics_in_the_database_to_ogg->setText(QCoreApplication::translate("player", "Convert all musics in the database to ogg", nullptr));
        actionForce_monitorization->setText(QCoreApplication::translate("player", "Force monitorization", nullptr));
        actionUpdate_Dinamic_Server_s_IP->setText(QCoreApplication::translate("player", "Update Dinamic Server's IP", nullptr));
        bt_autoMode->setText(QCoreApplication::translate("player", "Auto Mode", nullptr));
        txt_playlistTotalTime->setText(QCoreApplication::translate("player", "Total time of the playlist: 00:00:00", nullptr));
        checkBox_sum_to_playlist_time->setText(QCoreApplication::translate("player", "Sum to Playlist Time", nullptr));
        checkBox_update_last_played_values->setText(QCoreApplication::translate("player", "Update last played values", nullptr));
        checkBox_random_jingles->setText(QCoreApplication::translate("player", "Random Jingle every:", nullptr));
        label_9->setText(QCoreApplication::translate("player", "songs.", nullptr));
        label_3->setText(QCoreApplication::translate("player", "Randomly add:", nullptr));
        spinBox_num_of_songs_to_add_random->setSuffix(QString());
        spinBox_num_of_songs_to_add_random->setPrefix(QString());
        label_10->setText(QCoreApplication::translate("player", "songs", nullptr));
        bt_add_some_random_songs_from_genre->setText(QString());
        tabWidget_2->setTabText(tabWidget_2->indexOf(tabPlaylist), QCoreApplication::translate("player", "PlayList", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tabHistorylist), QCoreApplication::translate("player", "HistoryList", nullptr));
        lp_1->setText(QString());
        lp_2->setText(QString());
        lp_1_txt_file->setText(QCoreApplication::translate("player", "Netpack - Online Solutions!", nullptr));
        lp_2_txt_file->setText(QCoreApplication::translate("player", "Netpack - Online Solutions!", nullptr));
        lp_1_bt_play->setText(QString());
        lp_1_bt_play_2->setText(QString());
        pushButton->setText(QString());
        pushButton_2->setText(QString());
        lp_1_bt_pause->setText(QString());
        lp_2_bt_pause->setText(QString());
        lbl_total_time_lp1->setText(QString());
        lbl_total_time_lp2->setText(QString());
        lbl_lp1_remaining->setText(QString());
        lbl_lp2_remaining->setText(QString());
        lbl_lp1_vol_level->setText(QCoreApplication::translate("player", "100", nullptr));
        lbl_lp2_vol_level->setText(QCoreApplication::translate("player", "100", nullptr));
        bt_center_lps_vol->setText(QCoreApplication::translate("player", "|", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_dj), QCoreApplication::translate("player", "DJ", nullptr));
        pubWidget->setTabText(pubWidget->indexOf(tabMusic), QCoreApplication::translate("player", "Musics", nullptr));
        pubWidget->setTabText(pubWidget->indexOf(tabJingles), QCoreApplication::translate("player", "Jingles", nullptr));
        pubWidget->setTabText(pubWidget->indexOf(tabPub), QCoreApplication::translate("player", "Pub", nullptr));
        pubWidget->setTabText(pubWidget->indexOf(tabPrograms), QCoreApplication::translate("player", "Programs", nullptr));
#if QT_CONFIG(whatsthis)
        page_FTP_Connection->setWhatsThis(QCoreApplication::translate("player", "Search (only music table)", nullptr));
#endif // QT_CONFIG(whatsthis)
        txt_search->setText(QString());
        bt_reset->setText(QCoreApplication::translate("player", "Reset", nullptr));
        bt_search->setText(QCoreApplication::translate("player", "Search", nullptr));
        page_FTP_Connection->setItemText(page_FTP_Connection->indexOf(page), QCoreApplication::translate("player", "Search", nullptr));
        bt_apply_filter->setText(QCoreApplication::translate("player", "Filter", nullptr));
        checkBox_filter_genre1->setText(QCoreApplication::translate("player", "Where genre1 is:", nullptr));
        checkBox_filter_genre2->setText(QCoreApplication::translate("player", "Where genre2 is:", nullptr));
        page_FTP_Connection->setItemText(page_FTP_Connection->indexOf(page_2), QCoreApplication::translate("player", "fillters", nullptr));
        bt_updateTables->setText(QCoreApplication::translate("player", "Update tables", nullptr));
        bt_sndconv->setText(QCoreApplication::translate("player", "Sound Converter", nullptr));
        cbox_multi_select->setItemText(0, QCoreApplication::translate("player", "With the Selected...", nullptr));
        cbox_multi_select->setItemText(1, QCoreApplication::translate("player", "Show PATH for selected file(s) in the Debugger or Shell", nullptr));
        cbox_multi_select->setItemText(2, QCoreApplication::translate("player", "DELETE  selected file(s)!", nullptr));
        cbox_multi_select->setItemText(3, QCoreApplication::translate("player", "Convert to mp3", nullptr));
        cbox_multi_select->setItemText(4, QCoreApplication::translate("player", "Convert to ogg", nullptr));
        cbox_multi_select->setItemText(5, QCoreApplication::translate("player", "Trim Silence from Start and End below 0.1% (remove absolute silence)", nullptr));
        cbox_multi_select->setItemText(6, QCoreApplication::translate("player", "Trim Sillence from Start and End below 0.2% (remove noise also)", nullptr));

        cbox_multi_select->setCurrentText(QCoreApplication::translate("player", "With the Selected...", nullptr));
        bt_apply_multi_selection->setText(QCoreApplication::translate("player", "GO", nullptr));
        page_FTP_Connection->setItemText(page_FTP_Connection->indexOf(page_3), QCoreApplication::translate("player", "Extras", nullptr));
        bt_rol_streaming_play->setText(QString());
        label_4->setText(QCoreApplication::translate("player", "[comming soon]", nullptr));
        bt_rol_streaming_stop->setText(QString());
        page_FTP_Connection->setItemText(page_FTP_Connection->indexOf(Radio), QCoreApplication::translate("player", "Streaming Client", nullptr));
        label_5->setText(QCoreApplication::translate("player", "StreamServer (butt):", nullptr));
        label_6->setText(QCoreApplication::translate("player", "DDNS Update:", nullptr));
        label_7->setText(QCoreApplication::translate("player", "Port 8888 Test:", nullptr));
        bt_start_streaming->setText(QCoreApplication::translate("player", "Start All", nullptr));
        lbl_butt->setText(QCoreApplication::translate("player", "Stopped", nullptr));
        lbl_ddns->setText(QCoreApplication::translate("player", "Stopped", nullptr));
        lbl_port->setText(QCoreApplication::translate("player", "Stopped", nullptr));
        bt_takeOver->setText(QCoreApplication::translate("player", "Broadcast LIVE", nullptr));
        bt_stop_streaming->setText(QCoreApplication::translate("player", "Stop All", nullptr));
        lbl_icecast->setText(QCoreApplication::translate("player", "Stopped", nullptr));
        label_8->setText(QCoreApplication::translate("player", "WebServer (icecast2):", nullptr));
        bt_icecast->setText(QString());
        bt_butt->setText(QString());
        bt_ddns->setText(QString());
        bt_portTest->setText(QString());
        lbl_streamURL->setText(QString());
        page_FTP_Connection->setItemText(page_FTP_Connection->indexOf(page_4), QCoreApplication::translate("player", "Streaming Server", nullptr));
        btPlay->setText(QCoreApplication::translate("player", "Play", nullptr));
        btStop->setText(QCoreApplication::translate("player", "Stop", nullptr));
        btPlayNext->setText(QCoreApplication::translate("player", "Play Next", nullptr));
        bt_rec->setText(QCoreApplication::translate("player", "Record", nullptr));
        txt_recTime->setText(QString());
        txtNowPlaying->setText(QCoreApplication::translate("player", "XFB - OpenSource Radio Automation Software", nullptr));
        txt_ProgramName->setText(QString());
        bt_ProgramStopandProcess->setText(QCoreApplication::translate("player", "Stop and process", nullptr));
        txt_uploadingPrograms->setText(QCoreApplication::translate("player", "Uploading programs, plase wait...", nullptr));
        txtDuration->setText(QString());
        label->setText(QCoreApplication::translate("player", "Volume", nullptr));
        label_2->setText(QCoreApplication::translate("player", "Progress", nullptr));
        txt_creatingPrograms->setText(QCoreApplication::translate("player", "Creating programs, please wait...", nullptr));
        led_rec->setText(QString());
        txt_loading->setText(QCoreApplication::translate("player", "loading...", nullptr));
        bt_pause_rec->setText(QString());
        bt_pause_play->setText(QString());
#if QT_CONFIG(tooltip)
        NetpackLogo->setToolTip(QCoreApplication::translate("player", "<html><head/><body><center><p>Netpack - Online Solutions! </p><p>www.netpack.pt</p><p> :: Coding your Dreams! ::</p><p>\n"
"\n"
"<table align=\"center\" cellpadding=\"10\">\n"
"<tbody><tr bgcolor=\"WHITE\"><td>\n"
"\n"
"<!-- IMAGE BEGINS HERE -->\n"
"<font size=\"-3\">\n"
"<pre><font color=\"white\">1100010111011100000001001111011010110100011001100000000101111000011011101111001010100010100011001100111001011111111011110110011000</font><br><font color=\"white\">1010110001100101001000111011110101010010011000101111110011010000000001100001101011000110000111100101001110111101101111101110101000</font><br><font color=\"white\">1010111011011</font><font color=\"#fcfdfd\">1</font><font color=\"#e7eaef\">1</font><font color=\"#cdd3de\">1</font><font color=\"#b3bccf\">1</font><font color=\"#a3afc5\">0</font><font color=\"#9dabc2\">0</font><font color=\"#a6b2c7\">0</font><font color=\"#c0c6d5\">1</font><font color=\"#eae7ed\">1</font><font color=\"#fcfcfd\">0</font><font color=\"white\">011100011000010011101101101001000100"
                        "01010101101001110001000110000001101000001110010110010101100010111</font><font color=\"#fdfdfd\">0</font><font color=\"#f4f4f4\">0</font><font color=\"#f7f7f7\">1</font><font color=\"white\">010</font><br><font color=\"white\">111010010101</font><font color=\"#cdd1d9\">1</font><font color=\"#646f8d\">0</font><font color=\"#33416a\">1</font><font color=\"#22325f\">0</font><font color=\"#223462\">1</font><font color=\"#233868\">0</font><font color=\"#243d6f\">1</font><font color=\"#44547d\">0</font><font color=\"#78728e\">0</font><font color=\"#90809c\">0</font><font color=\"#b3a5bb\">1</font><font color=\"#eeebf0\">0</font><font color=\"white\">001001100100110110000010110001001111010111001111010111010010001100111111111011101110000110100110111</font><font color=\"#d3d3d3\">1</font><font color=\"#c1c1c1\">0</font><font color=\"#b2b2b2\">1</font><font color=\"#afafaf\">1</font><font color=\"#c6c6c6\">1</font><font color=\"#f4f4f4\">1</font><font color=\"white\">0</font><br><font color=\"white\">0001110011</font><fo"
                        "nt color=\"#fdfdfd\">0</font><font color=\"#9fa2b3\">1</font><font color=\"#262c52\">0</font><font color=\"#1d1f44\">1</font><font color=\"#1c1c40\">10</font><font color=\"#1e1f45\">1</font><font color=\"#1e254c\">1</font><font color=\"#43486c\">1</font><font color=\"#6f6b82\">1</font><font color=\"#68677c\">1</font><font color=\"#6b697f\">0</font><font color=\"#787189\">0</font><font color=\"#91849e\">0</font><font color=\"#dedae1\">1</font><font color=\"white\">00010100</font><font color=\"#e9e8ee\">0</font><font color=\"#a19ab6\">1</font><font color=\"#958aa9\">1</font><font color=\"#9b90a9\">1</font><font color=\"#9f98aa\">1</font><font color=\"#aca8b7\">0</font><font color=\"#cac5d0\">0</font><font color=\"#edebef\">0</font><font color=\"#fdfdfd\">0</font><font color=\"white\">1</font><font color=\"#fefefe\">1</font><font color=\"#e7e7e7\">0</font><font color=\"#dfdfdf\">0</font><font color=\"#e8e8e8\">0</font><font color=\"#f5f5f5\">1</font><font color=\"white\">00001100</font><font color=\"#f0f0f0\">0</"
                        "font><font color=\"#f3f3f3\">0</font><font color=\"white\">0100101110101010</font><font color=\"#f7f7f7\">1</font><font color=\"#dbdbdb\">1</font><font color=\"white\">1010</font><font color=\"#fafafa\">0</font><font color=\"#dcdcdc\">0</font><font color=\"#dfdfdf\">1</font><font color=\"#e1e1e1\">10</font><font color=\"#e0e0e0\">1</font><font color=\"#dedede\">1</font><font color=\"#dadada\">1</font><font color=\"#d7d7d7\">11</font><font color=\"#e1e1e1\">0</font><font color=\"#efefef\">1</font><font color=\"#fefefe\">1</font><font color=\"white\">10001011011010110100011</font><font color=\"#909090\">1</font><font color=\"#858585\">1</font><font color=\"#d4d4d4\">0</font><font color=\"white\">1100</font><font color=\"#c3c3c3\">1</font><font color=\"#bfbfbf\">1</font><font color=\"#a7a7a7\">0</font><font color=\"#a1a1a1\">0</font><font color=\"#bbbbbb\">1</font><font color=\"#ececec\">1</font><font color=\"white\">0</font><br><font color=\"white\">0010011101</font><font color=\"#a0a7b9\">1</font><font color=\""
                        "#222952\">0</font><font color=\"#1d2044\">0</font><font color=\"#1d193d\">1</font><font color=\"#1b1536\">1</font><font color=\"#1b1436\">1</font><font color=\"#1d193d\">1</font><font color=\"#2d2f51\">0</font><font color=\"#d3d2da\">0</font><font color=\"#888397\">1</font><font color=\"#636478\">0</font><font color=\"#525c6c\">1</font><font color=\"#525b6b\">1</font><font color=\"#636579\">1</font><font color=\"#7f798f\">0</font><font color=\"#dbd8df\">1</font><font color=\"white\">011101</font><font color=\"#c1c1d2\">0</font><font color=\"#4c487b\">1</font><font color=\"#3b336d\">1</font><font color=\"#3e346d\">1</font><font color=\"#44396f\">0</font><font color=\"#797491\">1</font><font color=\"#636879\">1</font><font color=\"#5f6375\">1</font><font color=\"#777289\">0</font><font color=\"#a9a0b3\">1</font><font color=\"#eae7ec\">0</font><font color=\"white\">1</font><font color=\"#dfdfdf\">0</font><font color=\"#7e7e7e\">1</font><font color=\"#151515\">1</font><font color=\"#323232\">0</font><font color=\""
                        "#6a6a6a\">1</font><font color=\"#b4b4b4\">1</font><font color=\"#f2f2f2\">0</font><font color=\"white\">01001</font><font color=\"#939393\">0</font><font color=\"#2d2d2d\">0</font><font color=\"#adadad\">1</font><font color=\"white\">00101111100101</font><font color=\"#cecece\">0</font><font color=\"#484848\">0</font><font color=\"#6e6e6e\">0</font><font color=\"white\">0101</font><font color=\"#fafafa\">1</font><font color=\"#9f9f9f\">0</font><font color=\"#4e4e4e\">0</font><font color=\"#171717\">0</font><font color=\"#555555\">1</font><font color=\"#7d7d7d\">0</font><font color=\"#818181\">1</font><font color=\"#828282\">0</font><font color=\"#818181\">1</font><font color=\"#696969\">1</font><font color=\"#0d0d0d\">1</font><font color=\"#232323\">0</font><font color=\"#717171\">0</font><font color=\"#f8f8f8\">1</font><font color=\"white\">1010011101011101011111</font><font color=\"#5a5a5a\">0</font><font color=\"black\">0</font><font color=\"#474747\">1</font><font color=\"white\">11011</font><font color=\""
                        "#f5f5f5\">0</font><font color=\"#e7e7e7\">0</font><font color=\"#e9e9e9\">0</font><font color=\"#fdfdfd\">1</font><font color=\"white\">11</font><br><font color=\"white\">001000001</font><font color=\"#c3cbd9\">1</font><font color=\"#2a4070\">0</font><font color=\"#22325e\">1</font><font color=\"#202a54\">0</font><font color=\"#1f254d\">0</font><font color=\"#1f2249\">0</font><font color=\"#1f2349\">1</font><font color=\"#1f254d\">0</font><font color=\"#84889e\">0</font><font color=\"white\">1</font><font color=\"#efedf1\">1</font><font color=\"#8f8a9d\">1</font><font color=\"#626478\">0</font><font color=\"#495765\">1</font><font color=\"#445460\">1</font><font color=\"#575e70\">0</font><font color=\"#777388\">1</font><font color=\"#dbd9df\">0</font><font color=\"white\">000</font><font color=\"#fcfcfc\">1</font><font color=\"#9899a8\">1</font><font color=\"#242652\">0</font><font color=\"#25275a\">0</font><font color=\"#2e2f68\">0</font><font color=\"#36336d\">0</font><font color=\"#66608d\">1</font><font co"
                        "lor=\"#f1f0f4\">1</font><font color=\"#bdc1c6\">1</font><font color=\"#4a5765\">1</font><font color=\"#515b6b\">1</font><font color=\"#6b697f\">0</font><font color=\"#8d829a\">0</font><font color=\"#e5e1e8\">1</font><font color=\"white\">1</font><font color=\"#eeeeee\">1</font><font color=\"#050505\">1</font><font color=\"#020202\">0</font><font color=\"#212121\">1</font><font color=\"#060606\">1</font><font color=\"#1c1c1c\">1</font><font color=\"#696969\">1</font><font color=\"#c3c3c3\">0</font><font color=\"#f3f3f3\">0</font><font color=\"white\">00</font><font color=\"#979797\">0</font><font color=\"black\">0</font><font color=\"#676767\">0</font><font color=\"white\">00</font><font color=\"#e3e3e3\">0</font><font color=\"#8f8f8f\">0</font><font color=\"#c9c9c9\">1</font><font color=\"#f6f6f6\">1</font><font color=\"#ededed\">0</font><font color=\"#767676\">0</font><font color=\"#4b4b4b\">0</font><font color=\"#5f5f5f\">1</font><font color=\"#9a9a9a\">1</font><font color=\"#f8f8f8\">0</font><font color=\"#"
                        "f4f4f4\">0</font><font color=\"#929292\">0</font><font color=\"#313131\">1</font><font color=\"black\">0</font><font color=\"#505050\">0</font><font color=\"#9c9c9c\">1</font><font color=\"#9a9a9a\">1</font><font color=\"#9b9b9b\">0</font><font color=\"#d6d6d6\">1</font><font color=\"white\">10</font><font color=\"#a8a8a8\">0</font><font color=\"black\">0</font><font color=\"#111111\">1</font><font color=\"#e8e8e8\">0</font><font color=\"#dddddd\">0</font><font color=\"#c7c7c7\">0</font><font color=\"#a3a3a3\">0</font><font color=\"#606060\">0</font><font color=\"#030303\">1</font><font color=\"black\">0</font><font color=\"#151515\">1</font><font color=\"#e8e8e8\">1</font><font color=\"#f4f4f4\">1</font><font color=\"silver\">1</font><font color=\"#797979\">1</font><font color=\"#575757\">0</font><font color=\"#787878\">1</font><font color=\"#c5c5c5\">0</font><font color=\"#f6f6f6\">1</font><font color=\"white\">1011</font><font color=\"#f6f6f6\">0</font><font color=\"#b6b6b6\">0</font><font color=\"#b7b7b7\""
                        ">0</font><font color=\"#cccccc\">0</font><font color=\"#4c4c4c\">1</font><font color=\"#343434\">0</font><font color=\"#454545\">1</font><font color=\"#a3a3a3\">0</font><font color=\"white\">010</font><font color=\"#555555\">1</font><font color=\"#030303\">0</font><font color=\"#b5b5b5\">0</font><font color=\"#f9f9f9\">0</font><font color=\"#e5e5e5\">1</font><font color=\"#b2b2b2\">0</font><font color=\"#7e7e7e\">0</font><font color=\"#5a5a5a\">1</font><font color=\"#545454\">1</font><font color=\"#c9c9c9\">1</font><font color=\"white\">0110</font><br><font color=\"white\">01111001</font><font color=\"#eff2f5\">0</font><font color=\"#47608d\">0</font><font color=\"#304c7e\">1</font><font color=\"#2f4a7b\">1</font><font color=\"#2e4777\">1</font><font color=\"#2d4472\">1</font><font color=\"#2d4371\">10</font><font color=\"#2e4473\">1</font><font color=\"#c1c8d5\">1</font><font color=\"white\">10</font><font color=\"#f4f3f5\">0</font><font color=\"#a19aad\">1</font><font color=\"#6e6b82\">0</font><font color=\""
                        "#5a6072\">1</font><font color=\"#535c6c\">1</font><font color=\"#5d6174\">0</font><font color=\"#777489\">1</font><font color=\"#d3d1d9\">1</font><font color=\"white\">0</font><font color=\"#fafafa\">1</font><font color=\"#7c7e8e\">0</font><font color=\"#080a22\">1</font><font color=\"#0c0e2d\">1</font><font color=\"#171a43\">1</font><font color=\"#25275b\">0</font><font color=\"#505081\">0</font><font color=\"#e8e8ee\">0</font><font color=\"white\">1</font><font color=\"#fafafb\">1</font><font color=\"#7b738b\">0</font><font color=\"#6d6b81\">1</font><font color=\"#706d83\">0</font><font color=\"#7f768e\">0</font><font color=\"#a495ae\">0</font><font color=\"#fdfdfd\">0</font><font color=\"#e9e9e9\">1</font><font color=\"#010101\">0</font><font color=\"#070707\">1</font><font color=\"#e5e5e5\">0</font><font color=\"#9e9e9e\">0</font><font color=\"#353535\">0</font><font color=\"#010101\">10</font><font color=\"#282828\">0</font><font color=\"#6a6a6a\">0</font><font color=\"#919191\">0</font><font color=\"#3e3"
                        "e3e\">0</font><font color=\"black\">0</font><font color=\"#707070\">0</font><font color=\"white\">0</font><font color=\"#f6f6f6\">0</font><font color=\"#202020\">1</font><font color=\"black\">0</font><font color=\"#252525\">1</font><font color=\"#909090\">0</font><font color=\"#acacac\">1</font><font color=\"#9b9b9b\">1</font><font color=\"#7a7a7a\">01</font><font color=\"#777777\">1</font><font color=\"#c5c5c5\">1</font><font color=\"#fafafa\">0</font><font color=\"white\">0</font><font color=\"#909090\">0</font><font color=\"black\">1</font><font color=\"#6e6e6e\">0</font><font color=\"#f1f1f1\">1</font><font color=\"#ececec\">0</font><font color=\"#cfcfcf\">1</font><font color=\"#b7b7b7\">1</font><font color=\"#fbfbfb\">0</font><font color=\"white\">1</font><font color=\"#a8a8a8\">0</font><font color=\"black\">1</font><font color=\"#030303\">1</font><font color=\"#e3e3e3\">1</font><font color=\"#b0b0b0\">0</font><font color=\"#5c5c5c\">0</font><font color=\"#303030\">0</font><font color=\"#272727\">1</font>"
                        "<font color=\"#3b3b3b\">0</font><font color=\"#757575\">1</font><font color=\"#d8d8d8\">0</font><font color=\"#f9f9f9\">0</font><font color=\"#a6a6a6\">1</font><font color=\"#6d6d6d\">1</font><font color=\"#5d5d5d\">0</font><font color=\"#353535\">0</font><font color=\"#090909\">0</font><font color=\"#020202\">1</font><font color=\"#323232\">0</font><font color=\"#919191\">0</font><font color=\"#e7e7e7\">1</font><font color=\"white\">1</font><font color=\"#fdfdfd\">1</font><font color=\"#4a4a4a\">1</font><font color=\"#010101\">0</font><font color=\"#555555\">1</font><font color=\"#cdcdcd\">1</font><font color=\"#b7b7b7\">0</font><font color=\"#a9a9a9\">0</font><font color=\"#b2b2b2\">1</font><font color=\"#e2e2e2\">0</font><font color=\"#f8f8f8\">0</font><font color=\"#f0f0f0\">0</font><font color=\"white\">1</font><font color=\"#323232\">0</font><font color=\"#2e2e2e\">0</font><font color=\"#838383\">0</font><font color=\"#3d3d3d\">1</font><font color=\"#0b0b0b\">1</font><font color=\"#060606\">1</font><font"
                        " color=\"#303030\">1</font><font color=\"#5c5c5c\">0</font><font color=\"#818181\">0</font><font color=\"#c6c6c6\">0</font><font color=\"white\">1001</font><br><font color=\"white\">00010101</font><font color=\"#fafbfb\">1</font><font color=\"#e8ebf1\">10101100</font><font color=\"#f9fafb\">0</font><font color=\"white\">110</font><font color=\"#f9f9fa\">0</font><font color=\"#bcb4c3\">0</font><font color=\"#837992\">0</font><font color=\"#736e85\">1</font><font color=\"#6b697f\">0</font><font color=\"#6f6b81\">1</font><font color=\"#7e778e\">0</font><font color=\"#b7b1c2\">0</font><font color=\"#727297\">0</font><font color=\"#242657\">0</font><font color=\"#1f2151\">1</font><font color=\"#222455\">0</font><font color=\"#292b5f\">0</font><font color=\"#5c5c88\">0</font><font color=\"#e4e4eb\">0</font><font color=\"white\">10</font><font color=\"#fefefe\">0</font><font color=\"#e6e2e9\">1</font><font color=\"#e3e0e6\">0</font><font color=\"#e2dfe5\">0</font><font color=\"#e3e0e6\">0</font><font color=\"#e7e2e9\""
                        ">1</font><font color=\"#fdfcfd\">1</font><font color=\"#efefef\">0</font><font color=\"#111111\">1</font><font color=\"#070707\">0</font><font color=\"#efefef\">1</font><font color=\"white\">1</font><font color=\"#f5f5f5\">1</font><font color=\"#b9b9b9\">0</font><font color=\"#575757\">0</font><font color=\"#131313\">1</font><font color=\"#050505\">0</font><font color=\"black\">10</font><font color=\"#080808\">1</font><font color=\"#b7b7b7\">1</font><font color=\"white\">1</font><font color=\"#fdfdfd\">0</font><font color=\"#888888\">0</font><font color=\"#191919\">1</font><font color=\"#070707\">1</font><font color=\"#151515\">1</font><font color=\"#303030\">0</font><font color=\"#404040\">11</font><font color=\"#3b3b3b\">11</font><font color=\"#9c9c9c\">1</font><font color=\"#fcfcfc\">0</font><font color=\"white\">0</font><font color=\"#acacac\">0</font><font color=\"#080808\">10</font><font color=\"#1a1a1a\">0</font><font color=\"#1b1b1b\">0</font><font color=\"#444444\">1</font><font color=\"#dbdbdb\">0</f"
                        "ont><font color=\"white\">00</font><font color=\"#b7b7b7\">0</font><font color=\"#080808\">0</font><font color=\"#030303\">1</font><font color=\"#e7e7e7\">0</font><font color=\"white\">0</font><font color=\"#fefefe\">1</font><font color=\"#f4f4f4\">1</font><font color=\"#f0f0f0\">0</font><font color=\"#f9f9f9\">1</font><font color=\"white\">00</font><font color=\"#919191\">0</font><font color=\"#1d1d1d\">1</font><font color=\"#171717\">1</font><font color=\"#222222\">1</font><font color=\"#7c7c7c\">0</font><font color=\"#dcdcdc\">1</font><font color=\"#979797\">1</font><font color=\"#444444\">1</font><font color=\"#0d0d0d\">0</font><font color=\"#1a1a1a\">1</font><font color=\"#7d7d7d\">0</font><font color=\"#efefef\">1</font><font color=\"#717171\">0</font><font color=\"#0f0f0f\">0</font><font color=\"#040404\">0</font><font color=\"#0f0f0f\">1</font><font color=\"#202020\">0</font><font color=\"#2d2d2d\">1</font><font color=\"#303030\">1</font><font color=\"#2c2c2c\">1</font><font color=\"#4b4b4b\">1</font><"
                        "font color=\"#d9d9d9\">1</font><font color=\"#f0f0f0\">1</font><font color=\"#131313\">1</font><font color=\"#040404\">1</font><font color=\"#0e0e0e\">1</font><font color=\"#4a4a4a\">1</font><font color=\"#4f4f4f\">1</font><font color=\"#191919\">0</font><font color=\"#484848\">0</font><font color=\"#898989\">1</font><font color=\"#b9b9b9\">0</font><font color=\"#d4d4d4\">1</font><font color=\"#fafafa\">0</font><font color=\"white\">110</font><br><font color=\"white\">11011000010100100101110</font><font color=\"#dfd9e2\">1</font><font color=\"#a597af\">1</font><font color=\"#7e718f\">1</font><font color=\"#6d6284\">1</font><font color=\"#594f7b\">1</font><font color=\"#423870\">0</font><font color=\"#3b336d\">0</font><font color=\"#39336d\">0</font><font color=\"#3a346d\">1</font><font color=\"#504c7e\">0</font><font color=\"#9a98b5\">0</font><font color=\"#f6f6f8\">0</font><font color=\"white\">01100000100</font><font color=\"#d9d9d9\">0</font><font color=\"#979797\">0</font><font color=\"#f2f2f2\">1</font><f"
                        "ont color=\"white\">0010</font><font color=\"#f0f0f0\">0</font><font color=\"silver\">0</font><font color=\"#a1a1a1\">1</font><font color=\"#9f9f9f\">0</font><font color=\"#cdcdcd\">0</font><font color=\"white\">1101</font><font color=\"#f5f5f5\">0</font><font color=\"#cacaca\">1</font><font color=\"#acacac\">1</font><font color=\"#9f9f9f\">1</font><font color=\"#9b9b9b\">0</font><font color=\"#a6a6a6\">1</font><font color=\"#c4c4c4\">1</font><font color=\"#f3f3f3\">1</font><font color=\"white\">111</font><font color=\"#fefefe\">1</font><font color=\"#d1d1d1\">0</font><font color=\"#a7a7a7\">0</font><font color=\"#a2a2a2\">1</font><font color=\"#bcbcbc\">0</font><font color=\"#f9f9f9\">0</font><font color=\"white\">010</font><font color=\"#fefefe\">0</font><font color=\"#d0d0d0\">1</font><font color=\"#a2a2a2\">0</font><font color=\"#f4f4f4\">0</font><font color=\"white\">1101011</font><font color=\"#f6f6f6\">1</font><font color=\"#c1c1c1\">1</font><font color=\"#a8a8a8\">0</font><font color=\"#a9a9a9\">0</fon"
                        "t><font color=\"#dcdcdc\">1</font><font color=\"white\">100</font><font color=\"#e3e3e3\">0</font><font color=\"#aaaaaa\">0</font><font color=\"#c9c9c9\">0</font><font color=\"#f7f7f7\">1</font><font color=\"white\">0</font><font color=\"#eaeaea\">0</font><font color=\"#bebebe\">1</font><font color=\"#a5a5a5\">0</font><font color=\"#969696\">1</font><font color=\"#949494\">1</font><font color=\"#9e9e9e\">0</font><font color=\"#bcbcbc\">0</font><font color=\"#f7f7f7\">1</font><font color=\"white\">01</font><font color=\"#dcdcdc\">0</font><font color=\"#ababab\">1</font><font color=\"#e7e7e7\">0</font><font color=\"white\">00</font><font color=\"#f5f5f5\">1</font><font color=\"#c9c9c9\">0</font><font color=\"#a7a7a7\">1</font><font color=\"#adadad\">0</font><font color=\"#e0e0e0\">0</font><font color=\"white\">0000</font><br><font color=\"white\">101001111110100111101000</font><font color=\"#f9f8fa\">0</font><font color=\"#dddae4\">0</font><font color=\"#bdb9cc\">0</font><font color=\"#aca7bf\">0</font><font col"
                        "or=\"#a6a1bb\">1</font><font color=\"#afaac1\">1</font><font color=\"#bebace\">0</font><font color=\"#d4d2de\">1</font><font color=\"#efeef3\">0</font><font color=\"#fffffe\">1</font><font color=\"white\">0001010110110101001000111100001</font><font color=\"#fefefe\">1</font><font color=\"#aeaeae\">0</font><font color=\"#e5e5e5\">0</font><font color=\"#c9c9c9\">0</font><font color=\"#c4c4c4\">0</font><font color=\"#a8a8a8\">0</font><font color=\"#efefef\">0</font><font color=\"#f3f3f3\">1</font><font color=\"#f1f1f1\">0</font><font color=\"#dfdfdf\">1</font><font color=\"#f6f6f6\">1</font><font color=\"#727272\">1</font><font color=\"#fdfdfd\">1</font><font color=\"#bebebe\">0</font><font color=\"#d6d6d6\">0</font><font color=\"#fefefe\">0</font><font color=\"#efefef\">1</font><font color=\"#fafafa\">1</font><font color=\"#e1e1e1\">0</font><font color=\"#f1f1f1\">0</font><font color=\"#f6f6f6\">0</font><font color=\"#f8f8f8\">0</font><font color=\"#dbdbdb\">1</font><font color=\"#f8f8f8\">0</font><font color=\""
                        "white\">00</font><font color=\"#acacac\">0</font><font color=\"#949494\">1</font><font color=\"#d8d8d8\">1</font><font color=\"#b3b3b3\">1</font><font color=\"#f1f1f1\">0</font><font color=\"#f4f4f4\">0</font><font color=\"#e4e4e4\">1</font><font color=\"#eaeaea\">1</font><font color=\"#dedede\">0</font><font color=\"#898989\">0</font><font color=\"#fefefe\">1</font><font color=\"#f4f4f4\">1</font><font color=\"#fdfdfd\">0</font><font color=\"#f9f9f9\">0</font><font color=\"#ebebeb\">1</font><font color=\"#cbcbcb\">0</font><font color=\"#b5b5b5\">1</font><font color=\"#efefef\">0</font><font color=\"#b0b0b0\">0</font><font color=\"#e7e7e7\">0</font><font color=\"#fefefe\">1</font><font color=\"#f3f3f3\">0</font><font color=\"#f0f0f0\">1</font><font color=\"#e0e0e0\">1</font><font color=\"#fbfbfb\">0</font><font color=\"#eeeeee\">0</font><font color=\"#f8f8f8\">1</font><font color=\"#dedede\">1</font><font color=\"#f7f7f7\">0</font><font color=\"#f4f4f4\">1</font><font color=\"#e5e5e5\">0</font><font color=\"#d"
                        "ddddd\">0</font><font color=\"#fefefe\">1</font><font color=\"#9d9d9d\">0</font><font color=\"#d6d6d6\">0</font><font color=\"white\">1101</font><br><font color=\"white\">11100001110101111001011111000100101101100100001001110101110010011</font><font color=\"#fcfcfc\">0</font><font color=\"#6a6a6a\">0</font><font color=\"#6d6d6d\">0</font><font color=\"#7e7e7e\">0</font><font color=\"#737373\">1</font><font color=\"#6d6d6d\">1</font><font color=\"#d1d1d1\">0</font><font color=\"#5a5a5a\">0</font><font color=\"#878787\">0</font><font color=\"#787878\">1</font><font color=\"#e8e8e8\">1</font><font color=\"#717171\">0</font><font color=\"#979797\">1</font><font color=\"#9f9f9f\">0</font><font color=\"#858585\">1</font><font color=\"#c3c3c3\">0</font><font color=\"#7f7f7f\">0</font><font color=\"#787878\">0</font><font color=\"#a2a2a2\">1</font><font color=\"#8d8d8d\">1</font><font color=\"#969696\">0</font><font color=\"#737373\">0</font><font color=\"#6f6f6f\">1</font><font color=\"#b9b9b9\">1</font><font color=\""
                        "white\">1</font><font color=\"#fafafa\">0</font><font color=\"#a5a5a5\">1</font><font color=\"#7f7f7f\">1</font><font color=\"#6b6b6b\">1</font><font color=\"#4d4d4d\">0</font><font color=\"#bababa\">0</font><font color=\"#6f6f6f\">0</font><font color=\"#757575\">1</font><font color=\"#6f6f6f\">1</font><font color=\"#dcdcdc\">0</font><font color=\"#777777\">1</font><font color=\"#a4a4a4\">0</font><font color=\"#b2b2b2\">0</font><font color=\"#8e8e8e\">1</font><font color=\"#7e7e7e\">1</font><font color=\"#747474\">1</font><font color=\"#bdbdbd\">0</font><font color=\"#6b6b6b\">1</font><font color=\"#bababa\">0</font><font color=\"#8e8e8e\">1</font><font color=\"#939393\">1</font><font color=\"#cccccc\">0</font><font color=\"#767676\">0</font><font color=\"#828282\">1</font><font color=\"#6e6e6e\">1</font><font color=\"#c2c2c2\">1</font><font color=\"#696969\">0</font><font color=\"#7b7b7b\">0</font><font color=\"#9c9c9c\">1</font><font color=\"#a6a6a6\">1</font><font color=\"#939393\">1</font><font color=\"#7a"
                        "7a7a\">0</font><font color=\"#636363\">0</font><font color=\"#e0e0e0\">0</font><font color=\"#a7a7a7\">1</font><font color=\"#cccccc\">1</font><font color=\"white\">1101</font><br><font color=\"white\">0110101010000110110001011111111001111110110000011001100000100011110</font><font color=\"#f7f7f7\">0</font><font color=\"#f2f2f2\">1</font><font color=\"#f7f7f7\">0</font><font color=\"white\">11</font><font color=\"#fafafa\">0</font><font color=\"white\">1</font><font color=\"#fcfcfc\">1</font><font color=\"#fefefe\">0</font><font color=\"#fafafa\">00</font><font color=\"#fdfdfd\">0</font><font color=\"#f4f4f4\">0</font><font color=\"#fefefe\">1</font><font color=\"#fbfbfb\">1</font><font color=\"#fdfdfd\">1</font><font color=\"white\">1</font><font color=\"#fbfbfb\">1</font><font color=\"white\">0</font><font color=\"#f5f5f5\">1</font><font color=\"#f4f4f4\">0</font><font color=\"#fefefe\">1</font><font color=\"white\">00</font><font color=\"#fefefe\">0</font><font color=\"#f6f6f6\">0</font><font color=\"#f2f2f"
                        "2\">0</font><font color=\"#fafafa\">1</font><font color=\"white\">1</font><font color=\"#fafafa\">1</font><font color=\"#f2f2f2\">1</font><font color=\"#fcfcfc\">0</font><font color=\"white\">0</font><font color=\"#f9f9f9\">1</font><font color=\"#fbfbfb\">0</font><font color=\"#fefefe\">0</font><font color=\"#f7f7f7\">1</font><font color=\"white\">0</font><font color=\"#fafafa\">1</font><font color=\"#fefefe\">1</font><font color=\"#f4f4f4\">1</font><font color=\"#fdfdfd\">0</font><font color=\"#fcfcfc\">1</font><font color=\"#f4f4f4\">0</font><font color=\"white\">0</font><font color=\"#fdfdfd\">0</font><font color=\"#f3f3f3\">1</font><font color=\"#f7f7f7\">1</font><font color=\"white\">1</font><font color=\"#fafafa\">1</font><font color=\"white\">00</font><font color=\"#fbfbfb\">1</font><font color=\"#f8f8f8\">0</font><font color=\"#fafafa\">0</font><font color=\"#f7f7f7\">0</font><font color=\"white\">0</font><font color=\"#f9f9f9\">1</font><font color=\"#fcfcfc\">0</font><font color=\"white\">1011</font><"
                        "br>\n"
"</pre></font>\n"
"</td></tr>\n"
"		</tbody></table>\n"
"\n"
"</p></center></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        NetpackLogo->setText(QString());
        txt_bottom_info->setText(QCoreApplication::translate("player", "XFB Vers\303\243o 1.23 by Netpack - Online Solutions! :: www.netpack.pt", nullptr));
        menuFile->setTitle(QCoreApplication::translate("player", "File", nullptr));
        menuPlaylists->setTitle(QCoreApplication::translate("player", "Playlists", nullptr));
        menuDatabase->setTitle(QCoreApplication::translate("player", "Database", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("player", "Help", nullptr));
        menuXFB->setTitle(QCoreApplication::translate("player", "XFB", nullptr));
        menuClient_3->setTitle(QCoreApplication::translate("player", "Client", nullptr));
        menuServer->setTitle(QCoreApplication::translate("player", "Server", nullptr));
    } // retranslateUi

};

namespace Ui {
    class player: public Ui_player {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PLAYER_H
