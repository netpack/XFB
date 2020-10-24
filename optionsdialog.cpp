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
#include <QAudioRecorder>
#include <QAudioDeviceInfo>
#include <QAudioDecoder>
#include <QMediaRecorder>
#include <QAudioProbe>
#include <QMultimedia>
#include <QFileDialog>
#include <stdlib.h>
#include <math.h>
#include <QPainter>
#include <QVBoxLayout>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QDateTime>
#include <QtWidgets>

optionsDialog::optionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::optionsDialog)
{
    ui->setupUi(this);





   //ui->txt_selected_db->setText("Default DB adb.db selected");

    audioRecorder = new QAudioRecorder(this);

    //Audio devices
    foreach (const QString &device, audioRecorder->audioInputs()) {
            ui->cboxRecDev->addItem(device, QVariant(device));
            qDebug()<<"Audio Hardware detected on this system (optionsdialog.cpp): "<<QVariant(QString(device));

        }

    //Audio codecs
    foreach (const QString &codecName, audioRecorder->supportedAudioCodecs()) {

            ui->comboBox_codec->addItem(codecName, QVariant(codecName));
            qDebug()<<"Audio Codecs on this system (optionsdialog.cpp): "<<QVariant(QString(codecName));
        }

    //Containers
    foreach (const QString &containerName, audioRecorder->supportedContainers()) {
            ui->comboBox_container->addItem(containerName, QVariant(containerName));
            qDebug()<<"Audio Containers on this system (optionsdialog.cpp): "<<QVariant(QString(containerName));
        }


    /* get db settings */

    QFile settings ("../config/settings.conf");
    if (!settings.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "../config/settings.conf could not be opened for read only ..???.. does it exist?";
        return;
    }


        QTextStream in(&settings);
        qDebug() << "Opening settings.conf";
        while (!in.atEnd()) {
            QString line = in.readLine();
            //txt_selected_db = line;

            /*
             * settings are in the format setting=value
             * so we split the curent line into the results array
             *
             * */

            QStringList results = line.split(" = ");
            //qDebug() << "showing results: " << results[1];



            if(results[0]=="Language"){

                if(results[1]=="pt"){
                    ui->cbox_lang->setCurrentText("Português");
                }
                if(results[1]=="fr"){
                    ui->cbox_lang->setCurrentText("Français");
                }
                if(results[1]=="en"){
                    ui->cbox_lang->setCurrentText("English");
                }


            }

            if(results[0]=="RecDevice"){

                ui->cboxRecDev->setCurrentText(results[1]);

            }

            if(results[0]=="RecCodec"){

                ui->comboBox_codec->setCurrentText(results[1]);

            }

            if(results[0]=="RecContainer"){

                ui->comboBox_container->setCurrentText(results[1]);

            }

            if(results[0]=="SavePath"){

                ui->txt_savePath->setText(results[1]);

            }

            if(results[0]=="ProgramsPath"){

                ui->txt_programsPath->setText(results[1]);

            }

            if(results[0]=="MusicPath"){

                ui->txt_musicPath->setText(results[1]);

            }

            if(results[0]=="JinglePath"){

                ui->txt_jinglePath->setText(results[1]);

            }

            if(results[0]=="FTPPath"){

                ui->txt_FTPlocalTempFolder->setText(results[1]);

            }

            if(results[0]=="TakeOverPath"){

                ui->txt_takeOverlocalTempFolder->setText(results[1]);

            }
            if(results[0]=="Database"){
                txt_selected_db = results[1];
                ui->txt_selected_db->setText(txt_selected_db);
            }
            if(results[0]=="Disable_Seek_Bar"){
                disableSeekBar = results[1];
                qDebug() << "Disable Seek bar settings: " << disableSeekBar;

                if(disableSeekBar=="true"){
                    ui->checkBox_disableSeekBar->setChecked(true);
                } else {
                    ui->checkBox_disableSeekBar->setChecked(false);
                }
            }

            if(results[0]=="Normalize_Soft"){
                Normalize_Soft = results[1];
                qDebug() << "Normalize_Soft settings: " << Normalize_Soft;

                if(Normalize_Soft=="true"){
                    ui->checkBox_Normalize_Soft->setChecked(true);
                } else {
                    ui->checkBox_Normalize_Soft->setChecked(false);
                }
            }

            if(results[0]=="Disable_Volume"){
                Disable_Volume = results[1];
                qDebug() << "Disable Seek bar settings: " << Disable_Volume;

                if(Disable_Volume=="true"){
                    ui->checkBox_disableVolume->setChecked(true);
                } else {
                    ui->checkBox_disableVolume->setChecked(false);
                }
            }

            if(results[0]=="Enable_Networking"){
                Enable_Networking = results[1];
                qDebug() << "Enable Networking settings: " << Enable_Networking;

                if(Enable_Networking=="true"){
                    ui->cbox_enableNetworking->setChecked(true);




                } else {
                    ui->cbox_enableNetworking->setChecked(false);
                    ui->txt_server->setEnabled(false);
                    ui->txt_port->setEnabled(false);
                    ui->txt_user->setEnabled(false);
                    ui->txt_password->setEnabled(false);
                }
            }


            if(results[0]=="Server_URL"){
                Server_URL = results[1];
                qDebug() << "Server URL settings: " << Server_URL;
                if(!Server_URL.isEmpty()){
                    ui->txt_server->setText(Server_URL);
                }
            }

            if(results[0]=="Port"){
                Port = results[1];
                qDebug() << "Port settings: " << Port;
                if(!Port.isEmpty()){
                    ui->txt_port->setText(Port);
                }
            }

            if(results[0]=="User"){
                User = results[1];
                qDebug() << "User settings: " << User;
                if(!User.isEmpty()){
                    ui->txt_user->setText(User);
                }
            }

            if(results[0]=="Pass"){
                Pass = results[1];
                qDebug() << "Pass settings: " << Pass;
                if(!Pass.isEmpty()){
                    ui->txt_password->setText(Pass);
                }
            }

            if(results[0]=="Role"){
                Role = results[1];
                qDebug() << "Role settings: " << Role;

                ui->cbox_role->setCurrentText(Role);

            }

            if(results[0]=="ComHour"){
                ComHour = results[1];
                qDebug() << "ComHour settings: " << ComHour;

                QTime time = QTime::fromString(results[1]);
                ui->cboxComHour->setTime(time);
            }

            if(results[0]=="FullScreen"){
                fullScreen = results[1];
                qDebug() << "fullscreen settings: " << fullScreen;
                if(fullScreen=="true"){
                    ui->checkBox_fullScreen->setChecked(true);
                }

            }

            if(results[0]=="AutoStartStop"){
                AutoStartStop = results[1];
                qDebug() << "AutoStartStop settings: " << AutoStartStop;

                if(AutoStartStop == "true"){
                    ui->checkBox_enableStartStopTime->setChecked(true);
                }

            }

            if(results[0]=="AutoStartTime"){
                AutoStartTime = results[1];
                qDebug() << "AutoStartTime settings: " << AutoStartTime;

                ui->timeEdit_autoStartTime->setTime(QTime::fromString(AutoStartTime));


            }


            if(results[0]=="AutoStopTime"){
                AutoStopTime = results[1];
                qDebug() << "AutoStopTime settings: " << AutoStopTime;



                ui->timeEdit_autoStopTime->setTime(QTime::fromString(AutoStopTime));


            }








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
    qDebug() << "Saving... ";

    QFile settings_file("../config/settings.conf");
    if(!settings_file.open(QIODevice::WriteOnly)){
        qDebug() << "../config/settings.conf is NOT writable... check the file permitions.";
        return;
    }
    QTextStream out(&settings_file);


    //check the ui and save

    out << "Database = " << ui->txt_selected_db->text() << "\n";

    if(ui->checkBox_disableSeekBar->isChecked()){
        out << "Disable_Seek_Bar = true\n";
    } else {
        out << "Disable_Seek_Bar = false\n";
    }

    if(ui->checkBox_Normalize_Soft->isChecked()){
        out << "Normalize_Soft = true\n";
    } else {
        out << "Normalize_Soft = false\n";
    }

    if(ui->checkBox_disableVolume->isChecked()){
        out << "Disable_Volume = true\n";
    } else {
        out << "Disable_Volume = false\n";
    }

    if(ui->checkBox_fullScreen->isChecked()){
        out << "FullScreen = true\n";
    } else {
        out << "FullScreen = false\n";
    }


    if(ui->checkBox_enableStartStopTime->isChecked()){
        out << "AutoStartStop = true\n";
        out << "AutoStartTime = "<<ui->timeEdit_autoStartTime->text()<<"\n";
        out << "AutoStopTime = "<<ui->timeEdit_autoStopTime->text()<<"\n";
    } else {
        out << "AutoStartStop = false\n";
        out << "AutoStartTime = 00:00\n";
        out << "AutoStopTime = 00:00\n";
    }




    /*
            if(results[0]=="RecCodec"){

                ui->comboBox_codec->setCurrentText(results[1]);

            }

            if(results[0]=="RecContainer"){

                ui->comboBox_container->setCurrentText(results[1]);

            }*/


    if(ui->cbox_lang->currentText()=="Portugûes"){
        out<<"Language = pt\n";
    }
    if(ui->cbox_lang->currentText()=="English"){
        out<<"Language = en\n";
    }
    if(ui->cbox_lang->currentText()=="Français"){
        out<<"Language = fr\n";
    }

    QString RecDevice = ui->cboxRecDev->currentText();
    out << "RecDevice = "<<RecDevice<<"\n";

    QString RecCodec = ui->comboBox_codec->currentText();
    out << "RecCodec = "<<RecCodec<<"\n";

    QString RecContainer = ui->comboBox_container->currentText();
    out << "RecContainer = "<<RecContainer<<"\n";

    QString SavePath = ui->txt_savePath->text();
    out << "SavePath = "<<SavePath<<"\n";

    QString ProgramsPath = ui->txt_programsPath->text();
    out << "ProgramsPath = "<<ProgramsPath<<"\n";

    QString MusicPath = ui->txt_musicPath->text();
    out << "MusicPath = "<<MusicPath<<"\n";

    QString JinglePath = ui->txt_jinglePath->text();
    out << "JinglePath = "<<JinglePath<<"\n";

    QString FTPPath = ui->txt_FTPlocalTempFolder->text();
    out << "FTPPath = " << FTPPath<<"\n";

    QString TakeOverPath = ui->txt_takeOverlocalTempFolder->text();
    out << "TakeOverPath = " << TakeOverPath<<"\n";

    QString ComHour = ui->cboxComHour->time().toString();
    out << "ComHour = " << ComHour<<"\n";




    if(ui->cbox_enableNetworking->isChecked()){
        out << "Enable_Networking = true\n";
        out << "Server_URL = "<<ui->txt_server->text()<<"\n";
        out << "Port = "<<ui->txt_port->text()<<"\n";
        out << "User = "<<ui->txt_user->text()<<"\n";
        out << "Pass = "<<ui->txt_password->text()<<"\n";
        out << "Role = "<<ui->cbox_role->currentText()<<"\n";

        QFile settings_ftp("~/.netrc");
/*
        if(!settings_ftp.open(QIODevice::WriteOnly)){
            qDebug() << "~/.netrc is NOT writable... trying to change the file permitions..";
            QString c = "chmod +x ~/.netrc";
            QProcess pc;
            pc.start("sh",QStringList()<<"-c"<<c);
            pc.waitForFinished();

            if(!settings_ftp.open(QIODevice::WriteOnly)){
                qDebug() << "It was NOT possible to change the file permitions automatically.. maybe if you try again as root or sudoer";
                return;
            } else {
                qDebug()<<"File permitions sucessfuly changed!";
            }
        }
*/
        QTextStream outF(&settings_ftp);

        QString c2 = "curl "+ui->txt_server->text()+"/XFB/Config/ftpupdate.txt";
        qDebug()<<"Full cmd is: "<<c2;
        QProcess pc2;
        pc2.start("sh",QStringList()<<"-c"<<c2);
        pc2.waitForFinished();

        QString cOut2 = pc2.readAll();

        QStringList ServerIP = cOut2.split("\n");

        qDebug()<<"Server's IP is now:"<<ServerIP[0];


        outF<<"machine "<<ServerIP[0]<<" login "<<ui->txt_user->text()<<" password "<<ui->txt_password->text()<<"\n";
/*
        QString c = "sudo chmod 600 ~/.netrc";
        QProcess pc;
        pc.start("sh",QStringList()<<"-c"<<c);
        pc.waitForFinished();
        QString cOut = pc.readAll();
        qDebug()<<"Running :: sudo chmod 600 ~/.netrc :: "<<cOut;
*/
        settings_ftp.close();


    } else {
        out << "Enable_Networking = false\n";
        out << "Server_URL = \n";
        out << "Port = \n";
        out << "User = \n";
        out << "Pass = \n";
        out << "Role = "<<ui->cbox_role->currentText()<<"\n";
    }



    settings_file.close();

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
    /*
    QProcess process;
    process.start("pwd");
    qDebug () << "pid: " << process.pid();
    process.waitForFinished(-1);
    if(process.exitCode()!=0){
        qDebug () << " Error " << process.exitCode();
    }
    else{
        qDebug () << " Ok " << process.pid();
    }
    */


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
    process.start("gedit", QStringList() << "../config/settings.conf");
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

    ui->txt_terminal->appendPlainText("If you are having problems downloading from youtube try this on a terminal:");
   ui->txt_terminal->appendPlainText("sudo easy_install -U youtube-dl");
}

void optionsDialog::on_pushButton_3_clicked()
{

    //delete all records in the music table
    QMessageBox::StandardButton go;
    go = QMessageBox::question(this,"Sure root?","Are you sure you want to delete ALL the tracks from the music table in the database?", QMessageBox::Yes|QMessageBox::No);
    if(go==QMessageBox::Yes){
        QSqlQuery sql;
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

    //delete all records in the jingles table
    QMessageBox::StandardButton go;
    go = QMessageBox::question(this,"Sure root?","Are you sure you want to delete ALL the Jingles from the jingles table in the database?", QMessageBox::Yes|QMessageBox::No);
    if(go==QMessageBox::Yes){
        QSqlQuery sql;
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
    //delete all records in the pub table
    QMessageBox::StandardButton go;
    go = QMessageBox::question(this,"Sure root?","Are you sure you want to delete ALL the publicity from the pub table in the database? (Scheduler table will also be cleared)", QMessageBox::Yes|QMessageBox::No);
    if(go==QMessageBox::Yes){
        QSqlQuery sql;
        sql.prepare("delete from pub where 1");
        if(sql.exec()){
            QMessageBox::information(this,tr("Pubs removed"),tr("All the pubs were removed from the database!"));
            QSqlQuery q;
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
