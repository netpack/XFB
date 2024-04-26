#include "externaldownloader.h"
#include "ui_externaldownloader.h"
#include "QProcess"
#include <QMessageBox>
#include <QtSql>
#include <QDir>
#include <addgenre.h>
#include <player.h>


externaldownloader::externaldownloader(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::externaldownloader)
{
    ui->setupUi(this);
    ui->frame_loading->hide();

    QSqlQueryModel * model=new QSqlQueryModel();
    QSqlQueryModel * model2=new QSqlQueryModel();

    QSqlQuery* qry=new QSqlQuery();

    qry->prepare("select name from genres1");
    qry->exec();
    model->setQuery(*qry);
    ui->cbox_g1->setModel(model);

    qry->prepare("select name from genres1");
    qry->exec();
    model2->setQuery(*qry);
    ui->cbox_g2->setModel(model2);

}

externaldownloader::~externaldownloader()
{
    delete ui;
}

void::externaldownloader::showLoadingFrame(){
    ui->frame_loading->show();
    //this down here allows us to show the splash sreen
    QTime dieTime= QTime::currentTime().addSecs(1);
       while( QTime::currentTime() < dieTime )
       QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    getFile();
}

void::externaldownloader::getFile(){

/*download the song, normalize, add to db*/

    ui->bt_youtube_getIt->blockSignals(true);
    ui->txt_teminal_yd1->appendPlainText("yt-dlt starting...");

    QString ylink;
    ylink = ui->txt_videoLink->text();
    QStringList ylinkparts = ylink.split("&");
    ylink = ylinkparts[0];

    QString yartist = ui->txt_artist->text();
    QString ysong = ui->txt_song->text();

    //remove problematic chars from the string ;-)
    yartist = yartist.replace(".","");
    ysong = ysong.replace(".","");
    yartist = yartist.replace("\"","");
    ysong = ysong.replace("\"","");
    yartist = yartist.replace("'","");
    ysong = ysong.replace("'","");
    yartist = yartist.replace("\\","");
    ysong = ysong.replace("\\","");
    yartist = yartist.replace("/","");
    ysong = ysong.replace("/","");
    yartist = yartist.replace("(","");
    ysong = ysong.replace("(","");
    yartist = yartist.replace(")","");
    ysong = ysong.replace(")","");

    QString cmd;
    cmd = "yt-dlp --extract-audio --audio-format vorbis -o '../music/"+yartist+" - "+ysong+".%(ext)s' " + ylink;
    qDebug()<<"Running: "<<cmd;
    ui->txt_teminal_yd1->appendPlainText(cmd);
    ui->txt_teminal_yd1->appendPlainText("Putting hamsters on the job... hold on... *a tribute to torrentz");

    QProcess sh;
    sh.setProcessChannelMode(QProcess::SeparateChannels);
    sh.start("sh", QStringList() << "-c" << cmd);
    //sh.waitForBytesWritten();
    QByteArray output = sh.readAll();
    //ui->txt_teminal_yd1->appendPlainText(output);
    sh.waitForFinished(-1);
    //output = sh.readAll();
    ui->txt_teminal_yd1->appendPlainText(output);
    sh.close();



        QString thisfilename = yartist+" - "+ysong+".ogg";
        qDebug()<<"External Downloader is now processing "<<thisfilename;
        qDebug()<<"The artist name is: "<<yartist;
        qDebug()<<"The name of the song is: "<<ysong;

        QString g1 = ui->cbox_g1->currentText();
        QString g2 = ui->cbox_g2->currentText();

        QString country;

        if(ui->checkBox_cplp->isChecked()){
            country = "PT";
        } else {
            country = "Other country / language";
        }


        QString pub_date = ui->dateEdit_publishedDate->text();


        QProcess shy;
        shy.start("sh", QStringList()<<"-c"<<"pwd");
        shy.waitForFinished();
        QByteArray outArray = shy.readAll();
        QString outStr = outArray;
        QStringList pathArray = outStr.split("\n");
        QString foldersStr = pathArray[0];
        QStringList folderArray = foldersStr.split("/");
        folderArray.removeLast();
        QString pwd;
        for(int i=0;i<folderArray.length();i++){
            pwd += folderArray[i]+"/";

        }

        QString file = pwd+"music/"+thisfilename;

        qDebug()<<"path for file is: "<<file;


        /*check if it's already there..*/

        int dbhasmusic=0;
        QSqlQuery query;
        QString seQuery = "SELECT id FROM musics WHERE path="+file;
        query.exec(seQuery);
        while (query.next()) {
               QString path = query.value(0).toString();
               qDebug() << "Skipping: "<<path<<" (Already in DB)";
               dbhasmusic=1;
           }
        qDebug()<<"dbhasmusic value is: "<<dbhasmusic;

        if(dbhasmusic==0){
            //add to db



            QProcess cmd;
            QString time;
            QString cmdtmpstr = "exiftool \""+file+"\" | grep Duration";
            cmd.start("sh",QStringList()<<"-c"<<cmdtmpstr);
            cmd.waitForFinished();
            QString cmdOut = cmd.readAll();
            qDebug()<<"Output of exiftool from externaldownloader: "<<cmdOut;
            cmd.close();

            QStringList arraycmd = cmdOut.split(" ");
            if(arraycmd.count()>25){
                QStringList splitarray = arraycmd[25].split("\n");
                qDebug()<<"Total track time is: "<<splitarray[0];
                time = splitarray[0];

            } else {
                qDebug()<<"-------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>     !!!!!!!!    An exception happend !? ... outputing details of this track: ";
                for(int i=0;i<arraycmd.count();i++){
                    qDebug()<< "The array position "<<i<<" has: "<<arraycmd[i];
                }
            }


            QSqlQuery sql;

            QString sqlquery = "insert into musics values(NULL,'"+yartist+"','"+ysong+"','"+g1+"','"+g2+"','"+country+"','"+pub_date+"','"+file+"','"+time+"',0,'-')";


            if(sql.exec(sqlquery))
            {
                qDebug() << "last sql: " << sql.lastQuery();
            } else {
                QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                qDebug() << "last sql: " << sql.lastQuery();
            }

            QMessageBox::information(this,tr("yt-dlp Downloader"),tr("Video downloaded, converted to ogg, moved to the music folder and added to database!\nSweet isn't it?"));
            //this->hide();
        } else {
            QMessageBox::information(this,tr("yt-dlp Downloader"),tr("This song was already in the database..."));

        }

   // } //eof for loop

    /*
     *
     * EOF add to db
     *
     * */



    ui->txt_teminal_yd1->appendPlainText("Developers NOTE: Won't work if executed from debug!");


    ui->txt_teminal_yd1->appendPlainText("All Done!");
    ui->bt_youtube_getIt->blockSignals(false);

    ui->frame_loading->hide();
}


void externaldownloader::on_bt_youtube_getIt_clicked()
{
    QString ylink = ui->txt_videoLink->text();



    QStringList ylistparts = ylink.split("&");

    /*
    for(int i=0;i<ylistparts.count();i++){
        qDebug()<<"ylistpart"<<i<<" has the value: "<<ylistparts[i];
    }*/

    ylink = ylistparts[0];

    qDebug()<<"ylink is now: "<<ylink;

    QString yartist = ui->txt_artist->text();
    QString ysong = ui->txt_song->text();

    if(ylink.isEmpty() || yartist.isEmpty() || ysong.isEmpty()){
        QMessageBox::information(this,tr("yt-dlp Downloader"),tr("The link, the artist name and the song title are mandatory..."));
    } else {
        showLoadingFrame();
    }



}
void externaldownloader::on_pushButton_clicked()
{
//manage genres

    addgenre addgenre;
    addgenre.setModal(true);
    addgenre.exec();

    QSqlQueryModel * model=new QSqlQueryModel();
    QSqlQueryModel * model2=new QSqlQueryModel();

    QSqlQuery* qry=new QSqlQuery();

    qry->prepare("select name from genres1");
    qry->exec();
    model->setQuery(*qry);
    ui->cbox_g1->setModel(model);

    qry->prepare("select name from genres1");
    qry->exec();
    model2->setQuery(*qry);
    ui->cbox_g2->setModel(model2);

}

void externaldownloader::on_bt_close_clicked()
{
    this->hide();

}

void externaldownloader::on_bt_clear_clicked()
{
    ui->txt_artist->setText("");
    ui->txt_song->setText("");
    ui->txt_teminal_yd1->clear();
    ui->txt_videoLink->setText("");
}
