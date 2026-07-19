#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QtSql>
// Forward declaration to avoid circular dependency
class player;

#include <QtMultimedia/QAudioInput>
#include <QByteArray>
#include <QColor>
#include <QComboBox>
#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QWidget>
#include <QtMultimedia/QMediaRecorder>

namespace Ui {
class optionsDialog;
}

class optionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit optionsDialog(QWidget *parent = 0);
    ~optionsDialog();
    QSqlDatabase adb;


private slots:


    void on_checkBox_disableSeekBar_toggled(bool checked);
    void saveSettings2Db();
    void on_bt_save_settings_clicked();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_bt_pwd_clicked();
    void on_bt_uname_clicked();
    void on_bt_edit_settings_clicked();
    void on_bt_free_clicked();
    void on_bt_df_clicked();
    void on_bt_update_youtubedl_clicked();
    void on_pushButton_3_clicked();
    void on_f_bt_del_jingles_table_clicked();
    void on_f_bt_del_pub_table_clicked();
    void on_cbox_enableNetworking_toggled(bool checked);
    // Shown once, only when the user ticks the Torrents checkbox here — never
    // at startup. Uses clicked() (user action) not toggled() (also fires on the
    // programmatic setChecked at load).
    void on_checkBox_enableTorrents_clicked(bool checked);
    void on_bt_browseSavePath_clicked();
    void on_bt_browse_programPath_clicked();
    void on_bt_browseFTPlocalFolder_clicked();

    void on_bt_browse_musicPath_clicked();

    void on_bt_browse_jinglePath_clicked();

    void on_bt_browseTakeOverlocalFolder_clicked();

    // Theme accent color (empty/invalid = the theme's own default)
    void on_bt_accentColor_clicked();
    void on_bt_accentReset_clicked();

private:
    void updateAccentButton();

    Ui::optionsDialog *ui;
    QColor m_accentColor; // invalid when using the theme default
    QString txt_selected_db;
    QString disableSeekBar;
    QString Normalize_Soft;
    QString Disable_Volume;
    QString Enable_Networking;
    QString Server_URL;
    QString Port;
    QString User;
    QString Pass;
    QString Role;
    QMediaRecorder *audioRecorder;
    QString FTPPath;
    QString ComHour;
    QString fullScreen;

};




#endif // OPTIONSDIALOG_H
