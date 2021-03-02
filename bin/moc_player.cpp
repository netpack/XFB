/****************************************************************************
** Meta object code from reading C++ file 'player.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/player.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'player.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_player_t {
    QByteArrayData data[137];
    char stringdata0[3189];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_player_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_player_t qt_meta_stringdata_player = {
    {
QT_MOC_LITERAL(0, 0, 6), // "player"
QT_MOC_LITERAL(1, 7, 11), // "checkDbOpen"
QT_MOC_LITERAL(2, 19, 0), // ""
QT_MOC_LITERAL(3, 20, 18), // "update_music_table"
QT_MOC_LITERAL(4, 39, 15), // "checkForUpdates"
QT_MOC_LITERAL(5, 55, 23), // "on_actionOpen_triggered"
QT_MOC_LITERAL(6, 79, 17), // "on_btPlay_clicked"
QT_MOC_LITERAL(7, 97, 29), // "on_sliderProgress_sliderMoved"
QT_MOC_LITERAL(8, 127, 8), // "position"
QT_MOC_LITERAL(9, 136, 27), // "on_sliderVolume_sliderMoved"
QT_MOC_LITERAL(10, 164, 17), // "on_btStop_clicked"
QT_MOC_LITERAL(11, 182, 17), // "onPositionChanged"
QT_MOC_LITERAL(12, 200, 15), // "durationChanged"
QT_MOC_LITERAL(13, 216, 19), // "currentMediaChanged"
QT_MOC_LITERAL(14, 236, 13), // "QMediaContent"
QT_MOC_LITERAL(15, 250, 7), // "content"
QT_MOC_LITERAL(16, 258, 13), // "volumeChanged"
QT_MOC_LITERAL(17, 272, 6), // "volume"
QT_MOC_LITERAL(18, 279, 21), // "lp1_onPositionChanged"
QT_MOC_LITERAL(19, 301, 19), // "lp1_durationChanged"
QT_MOC_LITERAL(20, 321, 23), // "lp1_currentMediaChanged"
QT_MOC_LITERAL(21, 345, 17), // "lp1_volumeChanged"
QT_MOC_LITERAL(22, 363, 21), // "lp2_onPositionChanged"
QT_MOC_LITERAL(23, 385, 19), // "lp2_durationChanged"
QT_MOC_LITERAL(24, 405, 23), // "lp2_currentMediaChanged"
QT_MOC_LITERAL(25, 429, 17), // "lp2_volumeChanged"
QT_MOC_LITERAL(26, 447, 21), // "playlistAboutToFinish"
QT_MOC_LITERAL(27, 469, 12), // "playNextSong"
QT_MOC_LITERAL(28, 482, 8), // "showTime"
QT_MOC_LITERAL(29, 491, 19), // "playlistContextMenu"
QT_MOC_LITERAL(30, 511, 20), // "musicViewContextMenu"
QT_MOC_LITERAL(31, 532, 22), // "jinglesViewContextMenu"
QT_MOC_LITERAL(32, 555, 18), // "pubViewContextMenu"
QT_MOC_LITERAL(33, 574, 23), // "programsViewContextMenu"
QT_MOC_LITERAL(34, 598, 9), // "dropEvent"
QT_MOC_LITERAL(35, 608, 11), // "QDropEvent*"
QT_MOC_LITERAL(36, 620, 14), // "dragEnterEvent"
QT_MOC_LITERAL(37, 635, 16), // "QDragEnterEvent*"
QT_MOC_LITERAL(38, 652, 20), // "on_musicView_pressed"
QT_MOC_LITERAL(39, 673, 11), // "QModelIndex"
QT_MOC_LITERAL(40, 685, 5), // "index"
QT_MOC_LITERAL(41, 691, 22), // "on_jinglesView_pressed"
QT_MOC_LITERAL(42, 714, 18), // "on_pubView_pressed"
QT_MOC_LITERAL(43, 733, 23), // "on_programsView_pressed"
QT_MOC_LITERAL(44, 757, 20), // "autoModeGetMoreSongs"
QT_MOC_LITERAL(45, 778, 22), // "on_bt_autoMode_clicked"
QT_MOC_LITERAL(46, 801, 36), // "on_actionAdd_a_single_song_tr..."
QT_MOC_LITERAL(47, 838, 21), // "on_btPlayNext_clicked"
QT_MOC_LITERAL(48, 860, 44), // "on_actionAdd_all_songs_in_a_f..."
QT_MOC_LITERAL(49, 905, 32), // "on_actionManage_Genres_triggered"
QT_MOC_LITERAL(50, 938, 29), // "on_actionAdd_Jingle_triggered"
QT_MOC_LITERAL(51, 968, 34), // "on_actionAdd_a_publicity_trig..."
QT_MOC_LITERAL(52, 1003, 13), // "run_scheduler"
QT_MOC_LITERAL(53, 1017, 26), // "on_actionOptions_triggered"
QT_MOC_LITERAL(54, 1044, 24), // "on_actionAbout_triggered"
QT_MOC_LITERAL(55, 1069, 51), // "on_actionAdd_a_song_from_Yout..."
QT_MOC_LITERAL(56, 1121, 20), // "on_bt_search_clicked"
QT_MOC_LITERAL(57, 1142, 19), // "on_bt_reset_clicked"
QT_MOC_LITERAL(58, 1162, 26), // "on_bt_apply_filter_clicked"
QT_MOC_LITERAL(59, 1189, 26), // "on_bt_updateTables_clicked"
QT_MOC_LITERAL(60, 1216, 32), // "on_actionSave_Playlist_triggered"
QT_MOC_LITERAL(61, 1249, 33), // "on_actionClear_Playlist_trigg..."
QT_MOC_LITERAL(62, 1283, 32), // "on_actionLoad_Playlist_triggered"
QT_MOC_LITERAL(63, 1316, 20), // "run_server_scheduler"
QT_MOC_LITERAL(64, 1337, 17), // "on_bt_rec_clicked"
QT_MOC_LITERAL(65, 1355, 39), // "on_actionRecord_a_new_Program..."
QT_MOC_LITERAL(66, 1395, 35), // "on_bt_ProgramStopandProcess_c..."
QT_MOC_LITERAL(67, 1431, 32), // "on_actionAdd_a_program_triggered"
QT_MOC_LITERAL(68, 1464, 12), // "run_recTimer"
QT_MOC_LITERAL(69, 1477, 20), // "setRecTimeToDefaults"
QT_MOC_LITERAL(70, 1498, 12), // "updateConfig"
QT_MOC_LITERAL(71, 1511, 10), // "pingServer"
QT_MOC_LITERAL(72, 1522, 16), // "server_ftp_check"
QT_MOC_LITERAL(73, 1539, 29), // "on_actionFullScreen_triggered"
QT_MOC_LITERAL(74, 1569, 37), // "on_actionForce_an_FTP_Check_t..."
QT_MOC_LITERAL(75, 1607, 52), // "on_actionMake_a_program_from_..."
QT_MOC_LITERAL(76, 1660, 45), // "on_actionCheck_the_Database_r..."
QT_MOC_LITERAL(77, 1706, 29), // "calculate_playlist_total_time"
QT_MOC_LITERAL(78, 1736, 12), // "RectimerDone"
QT_MOC_LITERAL(79, 1749, 5), // "RecT5"
QT_MOC_LITERAL(80, 1755, 5), // "RecT4"
QT_MOC_LITERAL(81, 1761, 5), // "RecT3"
QT_MOC_LITERAL(82, 1767, 5), // "RecT2"
QT_MOC_LITERAL(83, 1773, 5), // "RecT1"
QT_MOC_LITERAL(84, 1779, 6), // "RecCHK"
QT_MOC_LITERAL(85, 1786, 90), // "on_actionCheck_Database_Data_..."
QT_MOC_LITERAL(86, 1877, 32), // "on_bt_rol_streaming_play_clicked"
QT_MOC_LITERAL(87, 1910, 32), // "on_bt_rol_streaming_stop_clicked"
QT_MOC_LITERAL(88, 1943, 23), // "on_lp_1_bt_play_clicked"
QT_MOC_LITERAL(89, 1967, 21), // "on_pushButton_clicked"
QT_MOC_LITERAL(90, 1989, 25), // "on_lp_1_bt_play_2_clicked"
QT_MOC_LITERAL(91, 2015, 23), // "on_pushButton_2_clicked"
QT_MOC_LITERAL(92, 2039, 24), // "on_lp_1_bt_pause_clicked"
QT_MOC_LITERAL(93, 2064, 24), // "on_lp_2_bt_pause_clicked"
QT_MOC_LITERAL(94, 2089, 21), // "on_bt_sndconv_clicked"
QT_MOC_LITERAL(95, 2111, 102), // "on_actionAutoTrim_the_silence..."
QT_MOC_LITERAL(96, 2214, 32), // "on_actionUpdate_System_triggered"
QT_MOC_LITERAL(97, 2247, 35), // "on_bt_apply_multi_selection_c..."
QT_MOC_LITERAL(98, 2283, 60), // "on_actionConvert_all_musics_i..."
QT_MOC_LITERAL(99, 2344, 60), // "on_actionConvert_all_musics_i..."
QT_MOC_LITERAL(100, 2405, 29), // "on_bt_start_streaming_clicked"
QT_MOC_LITERAL(101, 2435, 39), // "on_horizontalSlider_lps_vol_s..."
QT_MOC_LITERAL(102, 2475, 28), // "on_bt_center_lps_vol_clicked"
QT_MOC_LITERAL(103, 2504, 16), // "streaming_timmer"
QT_MOC_LITERAL(104, 2521, 28), // "on_bt_stop_streaming_clicked"
QT_MOC_LITERAL(105, 2550, 27), // "on_txt_search_returnPressed"
QT_MOC_LITERAL(106, 2578, 38), // "server_check_and_schedule_new..."
QT_MOC_LITERAL(107, 2617, 39), // "on_actionForce_monitorization..."
QT_MOC_LITERAL(108, 2657, 45), // "on_actionUpdate_Dinamic_Serve..."
QT_MOC_LITERAL(109, 2703, 46), // "on_bt_add_some_random_songs_f..."
QT_MOC_LITERAL(110, 2750, 10), // "ddnsUpdate"
QT_MOC_LITERAL(111, 2761, 10), // "ice_timmer"
QT_MOC_LITERAL(112, 2772, 11), // "butt_timmer"
QT_MOC_LITERAL(113, 2784, 21), // "on_bt_icecast_clicked"
QT_MOC_LITERAL(114, 2806, 18), // "on_bt_butt_clicked"
QT_MOC_LITERAL(115, 2825, 18), // "on_bt_ddns_clicked"
QT_MOC_LITERAL(116, 2844, 22), // "on_bt_portTest_clicked"
QT_MOC_LITERAL(117, 2867, 22), // "on_bt_takeOver_clicked"
QT_MOC_LITERAL(118, 2890, 13), // "MainsetVol100"
QT_MOC_LITERAL(119, 2904, 12), // "MainsetVol80"
QT_MOC_LITERAL(120, 2917, 12), // "MainsetVol60"
QT_MOC_LITERAL(121, 2930, 12), // "MainsetVol40"
QT_MOC_LITERAL(122, 2943, 12), // "MainsetVol20"
QT_MOC_LITERAL(123, 2956, 12), // "MainsetVol10"
QT_MOC_LITERAL(124, 2969, 11), // "MainsetVol5"
QT_MOC_LITERAL(125, 2981, 8), // "MainStop"
QT_MOC_LITERAL(126, 2990, 18), // "pingTakeOverClient"
QT_MOC_LITERAL(127, 3009, 26), // "recoveryStreamTakeOverPlay"
QT_MOC_LITERAL(128, 3036, 13), // "checkTakeOver"
QT_MOC_LITERAL(129, 3050, 14), // "livePiscaStart"
QT_MOC_LITERAL(130, 3065, 13), // "livePiscaStop"
QT_MOC_LITERAL(131, 3079, 17), // "rmConfirmTakeOver"
QT_MOC_LITERAL(132, 3097, 14), // "returnTakeOver"
QT_MOC_LITERAL(133, 3112, 11), // "stopMplayer"
QT_MOC_LITERAL(134, 3124, 15), // "monitorTakeOver"
QT_MOC_LITERAL(135, 3140, 23), // "on_bt_pause_rec_clicked"
QT_MOC_LITERAL(136, 3164, 24) // "on_bt_pause_play_clicked"

    },
    "player\0checkDbOpen\0\0update_music_table\0"
    "checkForUpdates\0on_actionOpen_triggered\0"
    "on_btPlay_clicked\0on_sliderProgress_sliderMoved\0"
    "position\0on_sliderVolume_sliderMoved\0"
    "on_btStop_clicked\0onPositionChanged\0"
    "durationChanged\0currentMediaChanged\0"
    "QMediaContent\0content\0volumeChanged\0"
    "volume\0lp1_onPositionChanged\0"
    "lp1_durationChanged\0lp1_currentMediaChanged\0"
    "lp1_volumeChanged\0lp2_onPositionChanged\0"
    "lp2_durationChanged\0lp2_currentMediaChanged\0"
    "lp2_volumeChanged\0playlistAboutToFinish\0"
    "playNextSong\0showTime\0playlistContextMenu\0"
    "musicViewContextMenu\0jinglesViewContextMenu\0"
    "pubViewContextMenu\0programsViewContextMenu\0"
    "dropEvent\0QDropEvent*\0dragEnterEvent\0"
    "QDragEnterEvent*\0on_musicView_pressed\0"
    "QModelIndex\0index\0on_jinglesView_pressed\0"
    "on_pubView_pressed\0on_programsView_pressed\0"
    "autoModeGetMoreSongs\0on_bt_autoMode_clicked\0"
    "on_actionAdd_a_single_song_triggered\0"
    "on_btPlayNext_clicked\0"
    "on_actionAdd_all_songs_in_a_folder_triggered\0"
    "on_actionManage_Genres_triggered\0"
    "on_actionAdd_Jingle_triggered\0"
    "on_actionAdd_a_publicity_triggered\0"
    "run_scheduler\0on_actionOptions_triggered\0"
    "on_actionAbout_triggered\0"
    "on_actionAdd_a_song_from_Youtube_or_Other_triggered\0"
    "on_bt_search_clicked\0on_bt_reset_clicked\0"
    "on_bt_apply_filter_clicked\0"
    "on_bt_updateTables_clicked\0"
    "on_actionSave_Playlist_triggered\0"
    "on_actionClear_Playlist_triggered\0"
    "on_actionLoad_Playlist_triggered\0"
    "run_server_scheduler\0on_bt_rec_clicked\0"
    "on_actionRecord_a_new_Program_triggered\0"
    "on_bt_ProgramStopandProcess_clicked\0"
    "on_actionAdd_a_program_triggered\0"
    "run_recTimer\0setRecTimeToDefaults\0"
    "updateConfig\0pingServer\0server_ftp_check\0"
    "on_actionFullScreen_triggered\0"
    "on_actionForce_an_FTP_Check_triggered\0"
    "on_actionMake_a_program_from_this_playlist_triggered\0"
    "on_actionCheck_the_Database_records_triggered\0"
    "calculate_playlist_total_time\0"
    "RectimerDone\0RecT5\0RecT4\0RecT3\0RecT2\0"
    "RecT1\0RecCHK\0"
    "on_actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_co"
    "nfirmation_triggered\0"
    "on_bt_rol_streaming_play_clicked\0"
    "on_bt_rol_streaming_stop_clicked\0"
    "on_lp_1_bt_play_clicked\0on_pushButton_clicked\0"
    "on_lp_1_bt_play_2_clicked\0"
    "on_pushButton_2_clicked\0"
    "on_lp_1_bt_pause_clicked\0"
    "on_lp_2_bt_pause_clicked\0on_bt_sndconv_clicked\0"
    "on_actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_"
    "tracks_in_the_database_triggered\0"
    "on_actionUpdate_System_triggered\0"
    "on_bt_apply_multi_selection_clicked\0"
    "on_actionConvert_all_musics_in_the_database_to_mp3_triggered\0"
    "on_actionConvert_all_musics_in_the_database_to_ogg_triggered\0"
    "on_bt_start_streaming_clicked\0"
    "on_horizontalSlider_lps_vol_sliderMoved\0"
    "on_bt_center_lps_vol_clicked\0"
    "streaming_timmer\0on_bt_stop_streaming_clicked\0"
    "on_txt_search_returnPressed\0"
    "server_check_and_schedule_new_programs\0"
    "on_actionForce_monitorization_triggered\0"
    "on_actionUpdate_Dinamic_Server_s_IP_triggered\0"
    "on_bt_add_some_random_songs_from_genre_clicked\0"
    "ddnsUpdate\0ice_timmer\0butt_timmer\0"
    "on_bt_icecast_clicked\0on_bt_butt_clicked\0"
    "on_bt_ddns_clicked\0on_bt_portTest_clicked\0"
    "on_bt_takeOver_clicked\0MainsetVol100\0"
    "MainsetVol80\0MainsetVol60\0MainsetVol40\0"
    "MainsetVol20\0MainsetVol10\0MainsetVol5\0"
    "MainStop\0pingTakeOverClient\0"
    "recoveryStreamTakeOverPlay\0checkTakeOver\0"
    "livePiscaStart\0livePiscaStop\0"
    "rmConfirmTakeOver\0returnTakeOver\0"
    "stopMplayer\0monitorTakeOver\0"
    "on_bt_pause_rec_clicked\0"
    "on_bt_pause_play_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_player[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
     127,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  649,    2, 0x0a /* Public */,
       3,    0,  650,    2, 0x0a /* Public */,
       4,    0,  651,    2, 0x0a /* Public */,
       5,    0,  652,    2, 0x08 /* Private */,
       6,    0,  653,    2, 0x08 /* Private */,
       7,    1,  654,    2, 0x08 /* Private */,
       9,    1,  657,    2, 0x08 /* Private */,
      10,    0,  660,    2, 0x08 /* Private */,
      11,    1,  661,    2, 0x08 /* Private */,
      12,    1,  664,    2, 0x08 /* Private */,
      13,    1,  667,    2, 0x08 /* Private */,
      16,    1,  670,    2, 0x08 /* Private */,
      18,    1,  673,    2, 0x08 /* Private */,
      19,    1,  676,    2, 0x08 /* Private */,
      20,    1,  679,    2, 0x08 /* Private */,
      21,    1,  682,    2, 0x08 /* Private */,
      22,    1,  685,    2, 0x08 /* Private */,
      23,    1,  688,    2, 0x08 /* Private */,
      24,    1,  691,    2, 0x08 /* Private */,
      25,    1,  694,    2, 0x08 /* Private */,
      26,    0,  697,    2, 0x08 /* Private */,
      27,    0,  698,    2, 0x08 /* Private */,
      28,    0,  699,    2, 0x08 /* Private */,
      29,    1,  700,    2, 0x08 /* Private */,
      30,    1,  703,    2, 0x08 /* Private */,
      31,    1,  706,    2, 0x08 /* Private */,
      32,    1,  709,    2, 0x08 /* Private */,
      33,    1,  712,    2, 0x08 /* Private */,
      34,    1,  715,    2, 0x08 /* Private */,
      36,    1,  718,    2, 0x08 /* Private */,
      38,    1,  721,    2, 0x08 /* Private */,
      41,    1,  724,    2, 0x08 /* Private */,
      42,    1,  727,    2, 0x08 /* Private */,
      43,    1,  730,    2, 0x08 /* Private */,
      44,    0,  733,    2, 0x08 /* Private */,
      45,    0,  734,    2, 0x08 /* Private */,
      46,    0,  735,    2, 0x08 /* Private */,
      47,    0,  736,    2, 0x08 /* Private */,
      48,    0,  737,    2, 0x08 /* Private */,
      49,    0,  738,    2, 0x08 /* Private */,
      50,    0,  739,    2, 0x08 /* Private */,
      51,    0,  740,    2, 0x08 /* Private */,
      52,    0,  741,    2, 0x08 /* Private */,
      53,    0,  742,    2, 0x08 /* Private */,
      54,    0,  743,    2, 0x08 /* Private */,
      55,    0,  744,    2, 0x08 /* Private */,
      56,    0,  745,    2, 0x08 /* Private */,
      57,    0,  746,    2, 0x08 /* Private */,
      58,    0,  747,    2, 0x08 /* Private */,
      59,    0,  748,    2, 0x08 /* Private */,
      60,    0,  749,    2, 0x08 /* Private */,
      61,    0,  750,    2, 0x08 /* Private */,
      62,    0,  751,    2, 0x08 /* Private */,
      63,    0,  752,    2, 0x08 /* Private */,
      64,    0,  753,    2, 0x08 /* Private */,
      65,    0,  754,    2, 0x08 /* Private */,
      66,    0,  755,    2, 0x08 /* Private */,
      67,    0,  756,    2, 0x08 /* Private */,
      68,    0,  757,    2, 0x08 /* Private */,
      69,    0,  758,    2, 0x08 /* Private */,
      70,    0,  759,    2, 0x08 /* Private */,
      71,    0,  760,    2, 0x08 /* Private */,
      72,    0,  761,    2, 0x08 /* Private */,
      73,    0,  762,    2, 0x08 /* Private */,
      74,    0,  763,    2, 0x08 /* Private */,
      75,    0,  764,    2, 0x08 /* Private */,
      76,    0,  765,    2, 0x08 /* Private */,
      77,    0,  766,    2, 0x08 /* Private */,
      78,    0,  767,    2, 0x08 /* Private */,
      79,    0,  768,    2, 0x08 /* Private */,
      80,    0,  769,    2, 0x08 /* Private */,
      81,    0,  770,    2, 0x08 /* Private */,
      82,    0,  771,    2, 0x08 /* Private */,
      83,    0,  772,    2, 0x08 /* Private */,
      84,    0,  773,    2, 0x08 /* Private */,
      85,    0,  774,    2, 0x08 /* Private */,
      86,    0,  775,    2, 0x08 /* Private */,
      87,    0,  776,    2, 0x08 /* Private */,
      88,    0,  777,    2, 0x08 /* Private */,
      89,    0,  778,    2, 0x08 /* Private */,
      90,    0,  779,    2, 0x08 /* Private */,
      91,    0,  780,    2, 0x08 /* Private */,
      92,    0,  781,    2, 0x08 /* Private */,
      93,    0,  782,    2, 0x08 /* Private */,
      94,    0,  783,    2, 0x08 /* Private */,
      95,    0,  784,    2, 0x08 /* Private */,
      96,    0,  785,    2, 0x08 /* Private */,
      97,    0,  786,    2, 0x08 /* Private */,
      98,    0,  787,    2, 0x08 /* Private */,
      99,    0,  788,    2, 0x08 /* Private */,
     100,    0,  789,    2, 0x08 /* Private */,
     101,    1,  790,    2, 0x08 /* Private */,
     102,    0,  793,    2, 0x08 /* Private */,
     103,    0,  794,    2, 0x08 /* Private */,
     104,    0,  795,    2, 0x08 /* Private */,
     105,    0,  796,    2, 0x08 /* Private */,
     106,    0,  797,    2, 0x08 /* Private */,
     107,    0,  798,    2, 0x08 /* Private */,
     108,    0,  799,    2, 0x08 /* Private */,
     109,    0,  800,    2, 0x08 /* Private */,
     110,    0,  801,    2, 0x08 /* Private */,
     111,    0,  802,    2, 0x08 /* Private */,
     112,    0,  803,    2, 0x08 /* Private */,
     113,    0,  804,    2, 0x08 /* Private */,
     114,    0,  805,    2, 0x08 /* Private */,
     115,    0,  806,    2, 0x08 /* Private */,
     116,    0,  807,    2, 0x08 /* Private */,
     117,    0,  808,    2, 0x08 /* Private */,
     118,    0,  809,    2, 0x08 /* Private */,
     119,    0,  810,    2, 0x08 /* Private */,
     120,    0,  811,    2, 0x08 /* Private */,
     121,    0,  812,    2, 0x08 /* Private */,
     122,    0,  813,    2, 0x08 /* Private */,
     123,    0,  814,    2, 0x08 /* Private */,
     124,    0,  815,    2, 0x08 /* Private */,
     125,    0,  816,    2, 0x08 /* Private */,
     126,    0,  817,    2, 0x08 /* Private */,
     127,    0,  818,    2, 0x08 /* Private */,
     128,    0,  819,    2, 0x08 /* Private */,
     129,    0,  820,    2, 0x08 /* Private */,
     130,    0,  821,    2, 0x08 /* Private */,
     131,    0,  822,    2, 0x08 /* Private */,
     132,    0,  823,    2, 0x08 /* Private */,
     133,    0,  824,    2, 0x08 /* Private */,
     134,    0,  825,    2, 0x08 /* Private */,
     135,    0,  826,    2, 0x08 /* Private */,
     136,    0,  827,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::LongLong,    8,
    QMetaType::Void, QMetaType::LongLong,    8,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, QMetaType::LongLong,    8,
    QMetaType::Void, QMetaType::LongLong,    8,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, QMetaType::LongLong,    8,
    QMetaType::Void, QMetaType::LongLong,    8,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,    2,
    QMetaType::Void, QMetaType::QPoint,    2,
    QMetaType::Void, QMetaType::QPoint,    2,
    QMetaType::Void, QMetaType::QPoint,    2,
    QMetaType::Void, QMetaType::QPoint,    2,
    QMetaType::Void, 0x80000000 | 35,    2,
    QMetaType::Void, 0x80000000 | 37,    2,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void player::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<player *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->checkDbOpen(); break;
        case 1: _t->update_music_table(); break;
        case 2: _t->checkForUpdates(); break;
        case 3: _t->on_actionOpen_triggered(); break;
        case 4: _t->on_btPlay_clicked(); break;
        case 5: _t->on_sliderProgress_sliderMoved((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->on_sliderVolume_sliderMoved((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->on_btStop_clicked(); break;
        case 8: _t->onPositionChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 9: _t->durationChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 10: _t->currentMediaChanged((*reinterpret_cast< const QMediaContent(*)>(_a[1]))); break;
        case 11: _t->volumeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->lp1_onPositionChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 13: _t->lp1_durationChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 14: _t->lp1_currentMediaChanged((*reinterpret_cast< const QMediaContent(*)>(_a[1]))); break;
        case 15: _t->lp1_volumeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 16: _t->lp2_onPositionChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 17: _t->lp2_durationChanged((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 18: _t->lp2_currentMediaChanged((*reinterpret_cast< const QMediaContent(*)>(_a[1]))); break;
        case 19: _t->lp2_volumeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 20: _t->playlistAboutToFinish(); break;
        case 21: _t->playNextSong(); break;
        case 22: _t->showTime(); break;
        case 23: _t->playlistContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 24: _t->musicViewContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 25: _t->jinglesViewContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 26: _t->pubViewContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 27: _t->programsViewContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 28: _t->dropEvent((*reinterpret_cast< QDropEvent*(*)>(_a[1]))); break;
        case 29: _t->dragEnterEvent((*reinterpret_cast< QDragEnterEvent*(*)>(_a[1]))); break;
        case 30: _t->on_musicView_pressed((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 31: _t->on_jinglesView_pressed((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 32: _t->on_pubView_pressed((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 33: _t->on_programsView_pressed((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 34: _t->autoModeGetMoreSongs(); break;
        case 35: _t->on_bt_autoMode_clicked(); break;
        case 36: _t->on_actionAdd_a_single_song_triggered(); break;
        case 37: _t->on_btPlayNext_clicked(); break;
        case 38: _t->on_actionAdd_all_songs_in_a_folder_triggered(); break;
        case 39: _t->on_actionManage_Genres_triggered(); break;
        case 40: _t->on_actionAdd_Jingle_triggered(); break;
        case 41: _t->on_actionAdd_a_publicity_triggered(); break;
        case 42: _t->run_scheduler(); break;
        case 43: _t->on_actionOptions_triggered(); break;
        case 44: _t->on_actionAbout_triggered(); break;
        case 45: _t->on_actionAdd_a_song_from_Youtube_or_Other_triggered(); break;
        case 46: _t->on_bt_search_clicked(); break;
        case 47: _t->on_bt_reset_clicked(); break;
        case 48: _t->on_bt_apply_filter_clicked(); break;
        case 49: _t->on_bt_updateTables_clicked(); break;
        case 50: _t->on_actionSave_Playlist_triggered(); break;
        case 51: _t->on_actionClear_Playlist_triggered(); break;
        case 52: _t->on_actionLoad_Playlist_triggered(); break;
        case 53: _t->run_server_scheduler(); break;
        case 54: _t->on_bt_rec_clicked(); break;
        case 55: _t->on_actionRecord_a_new_Program_triggered(); break;
        case 56: _t->on_bt_ProgramStopandProcess_clicked(); break;
        case 57: _t->on_actionAdd_a_program_triggered(); break;
        case 58: _t->run_recTimer(); break;
        case 59: _t->setRecTimeToDefaults(); break;
        case 60: _t->updateConfig(); break;
        case 61: _t->pingServer(); break;
        case 62: _t->server_ftp_check(); break;
        case 63: _t->on_actionFullScreen_triggered(); break;
        case 64: _t->on_actionForce_an_FTP_Check_triggered(); break;
        case 65: _t->on_actionMake_a_program_from_this_playlist_triggered(); break;
        case 66: _t->on_actionCheck_the_Database_records_triggered(); break;
        case 67: _t->calculate_playlist_total_time(); break;
        case 68: _t->RectimerDone(); break;
        case 69: _t->RecT5(); break;
        case 70: _t->RecT4(); break;
        case 71: _t->RecT3(); break;
        case 72: _t->RecT2(); break;
        case 73: _t->RecT1(); break;
        case 74: _t->RecCHK(); break;
        case 75: _t->on_actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation_triggered(); break;
        case 76: _t->on_bt_rol_streaming_play_clicked(); break;
        case 77: _t->on_bt_rol_streaming_stop_clicked(); break;
        case 78: _t->on_lp_1_bt_play_clicked(); break;
        case 79: _t->on_pushButton_clicked(); break;
        case 80: _t->on_lp_1_bt_play_2_clicked(); break;
        case 81: _t->on_pushButton_2_clicked(); break;
        case 82: _t->on_lp_1_bt_pause_clicked(); break;
        case 83: _t->on_lp_2_bt_pause_clicked(); break;
        case 84: _t->on_bt_sndconv_clicked(); break;
        case 85: _t->on_actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database_triggered(); break;
        case 86: _t->on_actionUpdate_System_triggered(); break;
        case 87: _t->on_bt_apply_multi_selection_clicked(); break;
        case 88: _t->on_actionConvert_all_musics_in_the_database_to_mp3_triggered(); break;
        case 89: _t->on_actionConvert_all_musics_in_the_database_to_ogg_triggered(); break;
        case 90: _t->on_bt_start_streaming_clicked(); break;
        case 91: _t->on_horizontalSlider_lps_vol_sliderMoved((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 92: _t->on_bt_center_lps_vol_clicked(); break;
        case 93: _t->streaming_timmer(); break;
        case 94: _t->on_bt_stop_streaming_clicked(); break;
        case 95: _t->on_txt_search_returnPressed(); break;
        case 96: _t->server_check_and_schedule_new_programs(); break;
        case 97: _t->on_actionForce_monitorization_triggered(); break;
        case 98: _t->on_actionUpdate_Dinamic_Server_s_IP_triggered(); break;
        case 99: _t->on_bt_add_some_random_songs_from_genre_clicked(); break;
        case 100: _t->ddnsUpdate(); break;
        case 101: _t->ice_timmer(); break;
        case 102: _t->butt_timmer(); break;
        case 103: _t->on_bt_icecast_clicked(); break;
        case 104: _t->on_bt_butt_clicked(); break;
        case 105: _t->on_bt_ddns_clicked(); break;
        case 106: _t->on_bt_portTest_clicked(); break;
        case 107: _t->on_bt_takeOver_clicked(); break;
        case 108: _t->MainsetVol100(); break;
        case 109: _t->MainsetVol80(); break;
        case 110: _t->MainsetVol60(); break;
        case 111: _t->MainsetVol40(); break;
        case 112: _t->MainsetVol20(); break;
        case 113: _t->MainsetVol10(); break;
        case 114: _t->MainsetVol5(); break;
        case 115: _t->MainStop(); break;
        case 116: _t->pingTakeOverClient(); break;
        case 117: _t->recoveryStreamTakeOverPlay(); break;
        case 118: _t->checkTakeOver(); break;
        case 119: _t->livePiscaStart(); break;
        case 120: _t->livePiscaStop(); break;
        case 121: _t->rmConfirmTakeOver(); break;
        case 122: _t->returnTakeOver(); break;
        case 123: _t->stopMplayer(); break;
        case 124: _t->monitorTakeOver(); break;
        case 125: _t->on_bt_pause_rec_clicked(); break;
        case 126: _t->on_bt_pause_play_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMediaContent >(); break;
            }
            break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMediaContent >(); break;
            }
            break;
        case 18:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QMediaContent >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject player::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_player.data,
    qt_meta_data_player,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *player::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *player::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_player.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int player::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 127)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 127;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 127)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 127;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
