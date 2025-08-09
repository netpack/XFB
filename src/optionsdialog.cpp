#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "QFile"
#include "QDebug"
#include "QTextStream"
#include "QProcess"
#include <QMessageBox>
#include <QtSql>
#include <commonFunctions.h>
#include <QDebug>
#include <player.h>
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
#include <QDateTime>
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
    // Language
    QString idioma = settings.value("Language", "en").toString(); // Default "en"
    if(idioma == "pt") { ui->cbox_lang->setCurrentText("Português"); }
    else if(idioma == "fr") { ui->cbox_lang->setCurrentText("Français"); }
    else { ui->cbox_lang->setCurrentText("English"); } // Default to English text

    // General Checkboxes
    ui->checkBox_disableSeekBar->setChecked(settings.value("Disable_Seek_Bar", false).toBool());
    ui->checkBox_disableVolume->setChecked(settings.value("Disable_Volume", false).toBool());
    ui->checkBox_Normalize_Soft->setChecked(settings.value("Normalize_Soft", false).toBool());
    ui->checkBox_fullScreen->setChecked(settings.value("FullScreen", false).toBool());
    ui->checkBox_darkMode->setChecked(settings.value("DarkMode", false).toBool());

    // -- Database Tab --
    txt_selected_db = settings.value("Database").toString(); // Load into member variable
    ui->txt_selected_db->setText(txt_selected_db.isEmpty() ? "[NO DATABASE SET]" : txt_selected_db); // Display

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
    ui->txt_password->setText(settings.value("Pass").toString());
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

    qDebug() << "Finished loading settings in options dialog.";

    // Re-apply direct styling AFTER loading dark mode setting
    bool finalDarkMode = ui->checkBox_darkMode->isChecked(); // Use the UI value now
    qDebug() << "[StyleFix] OptionsDialog applying direct style for dark mode:" << finalDarkMode;
    if (finalDarkMode) {
        this->setStyleSheet("QDialog { background-color: #353535; color: #bbbbbb; }");
    } else {
        this->setStyleSheet("QDialog { background-color: #ffffff; color: #333333; }");
    }


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
    settings.setValue("Normalize_Soft", ui->checkBox_Normalize_Soft->isChecked());
    settings.setValue("Disable_Volume", ui->checkBox_disableVolume->isChecked());
    settings.setValue("FullScreen", ui->checkBox_fullScreen->isChecked());
    settings.setValue("DarkMode", ui->checkBox_darkMode->isChecked());

    // Language
    QString langText = ui->cbox_lang->currentText();
    if (langText == "Português") settings.setValue("Language", "pt");
    else if (langText == "Français") settings.setValue("Language", "fr");
    else settings.setValue("Language", "en"); // Default

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

    // Networking
    bool networkingEnabled = ui->cbox_enableNetworking->isChecked();
    settings.setValue("Enable_Networking", networkingEnabled);
    if (networkingEnabled) {
        settings.setValue("Server_URL", ui->txt_server->text());
        settings.setValue("Port", ui->txt_port->text());
        settings.setValue("User", ui->txt_user->text());
        settings.setValue("Pass", ui->txt_password->text()); // Still insecure
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

    // QSettings automatically saves on destruction or explicitly via sync()
    settings.sync(); // Force save to file immediately
    qDebug() << "Settings save attempt finished for" << settings.fileName();
    if (settings.status() != QSettings::NoError) {
        qWarning() << "Error during QSettings sync:" << settings.status();
        QMessageBox::warning(this, tr("Settings Error"), tr("Could not save settings to configuration file."));
    } else {
        qInfo() << "Settings saved successfully.";
        // Optional: QMessageBox::information(this, tr("Settings Saved"), tr("Settings saved successfully."));
    }
}

void optionsDialog::on_bt_save_settings_clicked()
{
    saveSettings2Db();

}

void optionsDialog::on_pushButton_clicked()
{
    saveSettings2Db();

    this->hide();
}

void optionsDialog::on_pushButton_2_clicked()
{
    //update_music_table();
    this->hide();
}

void optionsDialog::on_bt_pwd_clicked()
{

    QProcess sh;
    sh.start("sh", QStringList() << "-c" << "pwd");

    sh.waitForFinished();
    QByteArray output = sh.readAll();
    qDebug() << output;
    ui->txt_terminal->appendPlainText(output);
    sh.close();

}

void optionsDialog::on_bt_uname_clicked()
{
    QProcess sh;
    sh.start("sh", QStringList() << "-c" << "uname -a");

    sh.waitForFinished();
    QByteArray output = sh.readAll();
    ui->txt_terminal->appendPlainText(output);
    sh.close();
}

void optionsDialog::on_bt_edit_settings_clicked()
{
    QProcess process;
    process.start("gedit", QStringList() << ":/xfb.conf");
    process.waitForFinished(-1);
}

void optionsDialog::on_bt_free_clicked()
{
    QProcess sh;
    sh.start("sh", QStringList() << "-c" << "free -mt");
    sh.waitForFinished();
    QByteArray output = sh.readAll();
    ui->txt_terminal->appendPlainText(output);
    sh.close();
}

void optionsDialog::on_bt_df_clicked()
{
    QProcess sh;
    sh.start("sh", QStringList() << "-c" << "df -hT");
    sh.waitForFinished();
    QByteArray output = sh.readAll();
    ui->txt_terminal->appendPlainText(output);
    sh.close();
}

void optionsDialog::on_bt_update_youtubedl_clicked()
{

    ui->txt_terminal->appendPlainText("If you are having problems downloading from External try this on a terminal:");
   ui->txt_terminal->appendPlainText("sudo easy_install -U youtube-dl");
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
    ui->txt_server->setEnabled(checked);
    ui->txt_port->setEnabled(checked);
    ui->txt_user->setEnabled(checked);
    ui->txt_password->setEnabled(checked);
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
