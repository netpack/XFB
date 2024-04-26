/*
Hi and welcome. Thank you for reading my code.
I've made all in a way that I hope makes is easy for anyone to understand.
Enjoy! . Frédéric Bogaerts 2015 @ Netpack - Online Solutions!.
*/

#include "player.h"
#include "ui_player.h"
#include "add_music_single.h"
#include "add_full_dir.h"
#include "addgenre.h"
#include "addjingle.h"
#include "add_pub.h"
#include "add_program.h"
#include "optionsdialog.h"
#include "aboutus.h"
#include "externaldownloader.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QtSql>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QAudio>
#include <QDebug>
#include <QMultimedia>
#include <QAudioProbe>
#include <QTableWidgetItem>
#include <QList>
#include <QDateTime>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QSqlTableModel>
#include <QFileInfo>
#include <QAudioRecorder>
#include <QAudioDeviceInfo>
#include <QAudioDecoder>
#include <QMediaRecorder>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QHttpPart>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointF>
#include <QVector>
#include <QMovie>
#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QTextBrowser>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPixmap>

class ClickableTextBrowser : public QTextBrowser {
public:
    explicit ClickableTextBrowser(QWidget* parent = nullptr) : QTextBrowser(parent) {}

protected:
    void mousePressEvent(QMouseEvent* event) override {
        QTextBrowser::mousePressEvent(event);
        if (event->button() == Qt::LeftButton) {
            QTextCursor cursor = cursorForPosition(event->pos());
            cursor.select(QTextCursor::WordUnderCursor);
            QString href = cursor.selectedText();
            if (href.startsWith("http://") || href.startsWith("https://")) {
                QDesktopServices::openUrl(QUrl(href));
                event->accept(); // Indicate that the event has been handled
            }
        }
    }
};

class CustomMessageBox : public QDialog {
public:
    CustomMessageBox(const QString& title, const QString& message, const QPixmap& pixmap, QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(title);

        QIcon icon(":/48x48.png");

        setWindowIcon(icon);
        qreal level(0.86);
        setWindowOpacity(level);
        setMinimumWidth(586);



        QVBoxLayout* layout = new QVBoxLayout(this);

        // Add image
        QLabel* imageLabel = new QLabel(this);
        imageLabel->setPixmap(pixmap);
        imageLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(imageLabel);


        // Add message (as a clickable QTextBrowser)
        ClickableTextBrowser* messageBrowser = new ClickableTextBrowser(this);
        messageBrowser->setOpenExternalLinks(true);
        messageBrowser->setHtml(message);
        layout->addWidget(messageBrowser);


        // Add OK button
        QPushButton* okButton = new QPushButton(tr("I'll donate if I can and if I find XFB useful!"), this);
        layout->addWidget(okButton);
        connect(okButton, &QPushButton::clicked, this, &CustomMessageBox::accept);
    }
};


player::player(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::player)
{

    qDebug()<<"\nStarting XFB :: Developed by Frédéric Bogaerts @ Netpack - Online Solutions! www.netpack.pt";

    ui->setupUi(this);

    updateConfig();
    checkDbOpen();
    //on_actionUpdate_Dinamic_Server_s_IP_triggered();

    Xplayer = new QMediaPlayer(this);
    Xplaylist = new QMediaPlaylist();
    Xplaylist->setCurrentIndex(1);
    Xplayer->setPlaylist(Xplaylist);

    lp1_Xplayer = new QMediaPlayer(this);
    lp1_Xplaylist = new QMediaPlaylist();
    lp1_Xplaylist->setCurrentIndex(1);
    lp1_Xplayer->setPlaylist(lp1_Xplaylist);

    lp2_Xplayer = new QMediaPlayer(this);
    lp2_Xplaylist = new QMediaPlaylist();
    lp2_Xplaylist->setCurrentIndex(1);
    lp2_Xplayer->setPlaylist(lp2_Xplaylist);

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

    audioRecorder = new QAudioRecorder(this);

    //Audio devices
    foreach (const QString &device, audioRecorder->audioInputs()) {
            //ui->audioDeviceBox->addItem(device, QVariant(device));
            qDebug()<<"Audio Hardware on this system: "<<QVariant(QString(device));

        }

    //Audio codecs
    foreach (const QString &codecName, audioRecorder->supportedAudioCodecs()) {
            //ui->audioCodecBox->addItem(codecName, QVariant(codecName));
            qDebug()<<"Audio Codecs on this system: "<<QVariant(QString(codecName));
        }

    //Containers
    foreach (const QString &containerName, audioRecorder->supportedContainers()) {
           // ui->containerBox->addItem(containerName, QVariant(containerName));
            qDebug()<<"Audio Containers on this system: "<<QVariant(QString(containerName));
        }

    //Sample rates
    foreach (int sampleRate, audioRecorder->supportedAudioSampleRates()) {
       // ui->sampleRateBox->addItem(QString::number(sampleRate), QVariant(sampleRate));
        qDebug()<<"Audio Sample Rates suported: "<<QVariant(int(sampleRate));
    }

    connect(Xplayer, &QMediaPlayer::positionChanged, this, &player::onPositionChanged);
    connect(Xplayer, &QMediaPlayer::durationChanged, this, &player::durationChanged);
    connect(Xplayer, &QMediaPlayer::currentMediaChanged, this, &player::currentMediaChanged);
    connect(Xplayer, &QMediaPlayer::volumeChanged, this, &player::volumeChanged);

    connect(lp1_Xplayer, &QMediaPlayer::positionChanged, this, &player::lp1_onPositionChanged);
    connect(lp1_Xplayer, &QMediaPlayer::durationChanged, this, &player::lp1_durationChanged);
    connect(lp1_Xplayer, &QMediaPlayer::currentMediaChanged, this, &player::lp1_currentMediaChanged);
    connect(lp1_Xplayer, &QMediaPlayer::volumeChanged, this, &player::lp1_volumeChanged);

    connect(lp2_Xplayer, &QMediaPlayer::positionChanged, this, &player::lp2_onPositionChanged);
    connect(lp2_Xplayer, &QMediaPlayer::durationChanged, this, &player::lp2_durationChanged);
    connect(lp2_Xplayer, &QMediaPlayer::currentMediaChanged, this, &player::lp2_currentMediaChanged);
    connect(lp2_Xplayer, &QMediaPlayer::volumeChanged, this, &player::lp2_volumeChanged);

    connect(recTimer,SIGNAL(timeout()),this,SLOT(run_recTimer()));

    /* main clock signals and slots */
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(showTime()));
    timer->start(1000);
    showTime();

    if(Role=="Server"){

        QTimer *schedulerTimer = new QTimer(this);
        connect(schedulerTimer,SIGNAL(timeout()),this,SLOT(run_scheduler()));
        schedulerTimer->start(60000);

        run_server_scheduler(); //run at startup

        server_this_day_of_the_week = QDate::currentDate().dayOfWeek();
        QTimer *schedulerTimerh = new QTimer(this);
        connect(schedulerTimerh,SIGNAL(timeout()),this,SLOT(run_server_scheduler()));
        schedulerTimerh->start(3600000); //once per hour

        QTimer *schedulerTimerMT = new QTimer(this);
        connect(schedulerTimerMT,SIGNAL(timeout()),this,SLOT(monitorTakeOver()));
        schedulerTimerMT->start(25000);

    }

    /*Populate music table with an editable table field on double-click*/

    QSqlTableModel *model = new QSqlTableModel(this);
    model->setTable("musics");
    model->select();

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

    /*Populate jingles table with an editable table field on double-click*/
    QSqlTableModel * jinglesmodel = new QSqlTableModel(this);
    jinglesmodel->setTable("jingles");
    jinglesmodel->select();
    ui->jinglesView->setModel(jinglesmodel);
    ui->jinglesView->setSortingEnabled(true);
    ui->jinglesView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Pub table*/

    QSqlTableModel *pubmodel = new QSqlTableModel(this);
    pubmodel->setTable("pub");
    pubmodel->select();
    ui->pubView->setModel(pubmodel);
    ui->pubView->setSortingEnabled(true);
    ui->pubView->hideColumn(0);
    ui->pubView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Programs table*/

    QSqlTableModel *programsmodel = new QSqlTableModel(this);
    programsmodel->setTable("programs");
    programsmodel->select();
    ui->programsView->setModel(programsmodel);
    ui->programsView->setSortingEnabled(true);
    ui->programsView->hideColumn(0);
    ui->programsView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

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
    /*Music list*/
     ui->musicView->setSelectionMode(QAbstractItemView::ExtendedSelection);
     ui->musicView->setDragEnabled(true);
     ui->musicView->viewport()->setAcceptDrops(false);
     ui->musicView->setAcceptDrops(false);
     ui->musicView->setDropIndicatorShown(true);
     ui->musicView->setDragDropMode(QAbstractItemView::DragOnly);
     ui->musicView->setSelectionBehavior(QAbstractItemView::SelectRows);

     ui->musicView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->musicView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(musicViewContextMenu(const QPoint&)));

     ui->playlist->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->playlist, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(playlistContextMenu(const QPoint&)));

     ui->jinglesView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->jinglesView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(jinglesViewContextMenu(const QPoint&)));

     ui->pubView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->pubView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(pubViewContextMenu(const QPoint&)));

     ui->programsView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->programsView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(programsViewContextMenu(const QPoint&)));

     /*Populate genre1 and 2 filters*/
     QSqlQueryModel * model_genre1=new QSqlQueryModel();
     QSqlQuery* qry=new QSqlQuery();
     QString sqlq = "select name from genres1";
     qry->exec(sqlq);
     model_genre1->setQuery(*qry);
     ui->cBoxGenre1->setModel(model_genre1);
     ui->cBoxGenre2->setModel(model_genre1);

    update_music_table();

   /*Bottom info*/
   QDir dir; QString cpath = dir.absolutePath();
   QString binfo = ui->txt_bottom_info->text()+"\n"+cpath+"\n"+Role;
   ui->txt_bottom_info->setText(binfo);

   /*Default button states*/
   ui->bt_takeOver->setEnabled(false);
   ui->bt_pause_rec->setEnabled(false);



   if(Role=="Server"){

       autoMode = 1;
       qDebug()<<"Role is set to Server, so autoMode is ON by default";
       ui->bt_autoMode->setStyleSheet("background-color: rgb(175, 227, 59)");
       ui->bt_takeOver->setHidden(true);
       ui->menuClient_3->setEnabled(false);

   }else{

       qDebug("XFB is now running in client mode!");

       autoMode = 0;
       qDebug()<<"autoMode is OFF";
       ui->bt_autoMode->setStyleSheet("");
       ui->menuServer->setEnabled(false);

   }


   QPixmap pixmap(":/images/donate.png");
   CustomMessageBox msgBox(tr("Donate to the Developer!"), tr("Please support the development of XFB!<br>If you appreciate this software, kindly consider making a donation to support the developer!<br><a href=\"https://www.paypal.com/donate/?hosted_button_id=TFDSZU78WLMC6\">Donate via PayPal!</a><br>Contact for professional support and custom development!<br><br>Why did the computer get a little emotional when using WinRAR?<br>_Because even after all the \"evaluation\", it still felt unzipped!"), pixmap);
   msgBox.exec();




}




player::~player()
{
    delete ui;
    delete audioRecorder;
    delete probe;
}




void player::updateConfig(){


    /* get db settings */
    QFile settings ("/etc/xfb/xfb.conf");
    if (!settings.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "/etc/xfb/xfb.conf could not be loaded. Please check that it exists";
        return;
    }

    QTextStream in(&settings);
    qDebug() << "Opening /etc/xfb/xfb.conf";
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList results = line.split(" = ");

        //for a switch we need a hash function.. todo later..
        if(results[0]=="SavePath"){
            SavePath = results[1];
        }
        if(results[0]=="Server_URL"){
            Server_URL = results[1];
        }
        if(results[0]=="Port"){
            Port = results[1].toInt();
        }
        if(results[0]=="User"){
            User = results[1];
        }
        if(results[0]=="Pass"){
            Pass = results[1];
        }
        if(results[0]=="ProgramsPath"){
            ProgramsPath = results[1];
        }
        if(results[0]=="MusicPath"){
            MusicPath = results[1];
        }
        if(results[0]=="JinglePath"){
            JinglePath = results[1];
        }
        if(results[0]=="FTPPath"){
            FTPPath = results[1];
        }
        if(results[0]=="TakeOverPath"){
            TakeOverPath = results[1];
        }
        if(results[0]=="ComHour"){
            ComHour = results[1];
        }
        if(results[0]=="fullScreen"){
            fullScreen = results[1];
            if(fullScreen=="true"){

            }
        }
        if(results[0]=="RecDevice"){
            recDevice = results[1];
        }
        if(results[0]=="RecCodec"){
            codec = results[1];
        }
        if(results[0]=="RecContainer"){
            contentamento = results[1];
        }
        if(results[0]=="Database"){
            txt_selected_db = results[1];
        }
        if(results[0]=="Disable_Seek_Bar"){
            disableSeekBar = results[1];
            if(disableSeekBar=="true")
                ui->sliderProgress->setEnabled(0);
            qDebug() << "Disable Seek bar settings: " << disableSeekBar;
        }
        if(results[0]=="Normalize_Soft"){
            normalization_soft = results[1];
            qDebug() << "normalization_soft settings: " << normalization_soft;
        }
        if(results[0]=="Disable_Volume"){
            Disable_Volume = results[1];
            if(Disable_Volume=="true")
                ui->sliderVolume->setEnabled(0);
            qDebug() << "Disable_Volume settings: " << Disable_Volume;
        }
        if(results[0]=="Role"){
            Role = results[1];
            qDebug() << "Role settings: " << Role;
            if(Role=="Server"){
                qDebug("XFB is now running in server mode!");
            } else{
                qDebug("XFB is now running in client mode!");
            }

        }

    }



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

void player::checkDbOpen(){

        if(!adb.isOpen()){

            qDebug()<<"Opening db from checkDbOpen";

                adb=QSqlDatabase::addDatabase("QSQLITE");
                adb.setDatabaseName("/usr/share/xfb/config/adb.db");
                if (!adb.open()) {
                    qDebug() << "Error opening database:" << adb.lastError().text();
                }
        }

}

void player::on_actionOpen_triggered()
{

    qDebug() << "File -> Open file";

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(tr("Audio Files (*.ogg *.mp3 *.flac *.wav)"));
    dialog.setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if(dialog.exec())
    {
        fileNames = dialog.selectedFiles();
        ui->playlist->addItems(fileNames);
    }

}

void::player::playlistContextMenu(const QPoint& pos){
    QPoint globalPos = ui->playlist->mapToGlobal(pos);
    QMenu thisMenu;
    QString remove = tr("Remove this track from the playlist");
    QString moveToTop = tr("Send this track to the top of the playlist");
    QString moveToBottom = tr("Send this track to the bottom of the playlist");


    thisMenu.addAction(remove);
    thisMenu.addAction(moveToTop);
    thisMenu.addAction(moveToBottom);


    QAction* selectedItem = thisMenu.exec(globalPos);
    if(selectedItem){
        QString selectedListItem = selectedItem->text();
        int rowidx = ui->playlist->selectionModel()->currentIndex().row();
        estevalor = ui->playlist->model()->data(ui->playlist->model()->index(rowidx,0)).toString();

        if(selectedListItem==remove){
            delete ui->playlist->item(rowidx);
            calculate_playlist_total_time();
        }
        if(selectedListItem==moveToTop){
            delete ui->playlist->item(rowidx);
            ui->playlist->insertItem(0,estevalor);
        }
        if(selectedListItem==moveToBottom){
            delete ui->playlist->item(rowidx);
            ui->playlist->addItem(estevalor);
        }


    }
}

void::player::musicViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->musicView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this track from database");
    QString openWithAudacity = tr("Open this in Audacity");
    QString getInfoFromMediaInfo = tr("Retrieve meta information from file");



    thisMenu.addAction(addToBottomOfPlaylist);
    thisMenu.addAction(addtoTopOfPlaylist);
    thisMenu.addAction(deleteThisFromDB);
    thisMenu.addAction(openWithAudacity);
    //thisMenu.addAction(getInfoFromMediaInfo);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (selectedItem)
    {
        //qDebug()<<"selected item in context menu was: "<<selectedItem->text();
        QString selectedMenuItem = selectedItem->text();
        int rowidx = ui->musicView->selectionModel()->currentIndex().row();
        estevalor = ui->musicView->model()->data(ui->musicView->model()->index(rowidx,7)).toString();

        if(selectedMenuItem==addToBottomOfPlaylist){
            qDebug()<<"Launch add this to bottom of playlist";
            ui->playlist->addItem(estevalor);
            calculate_playlist_total_time();
        }
        if(selectedMenuItem==addtoTopOfPlaylist){
            qDebug()<<"Launch add this to top of playlist";
         ui->playlist->insertItem(0,estevalor);
         calculate_playlist_total_time();
        }
        if(selectedMenuItem==deleteThisFromDB){

            QMessageBox::StandardButton go;
            go = QMessageBox::question(this,tr("Sure?"),tr("Are you sure you want to delete the track from the database?"), QMessageBox::Yes|QMessageBox::No);
            if(go==QMessageBox::Yes){

                QMessageBox::StandardButton rm;
                rm = QMessageBox::question(this,tr("Erase the file also?"),tr("Do you want to delete the file from the hard drive also?"), QMessageBox::Yes|QMessageBox::No);
                if(rm==QMessageBox::Yes){


                    QProcess sh;
                    sh.startDetached("sh",QStringList()<<"-c"<<"rm \""+estevalor+"\"");

                    qDebug()<<"File DELETED :: "<<estevalor;


                }

                QSqlQuery sql;
                sql.prepare("delete from musics where path=:path");
                sql.bindValue(":path",estevalor);
                if(sql.exec()){
                    //QMessageBox::information(this,tr("Track removed"),tr("The track was removed"));
                    update_music_table();
                } else {
                    QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                    qDebug() << "last sql: " << sql.lastQuery();
                }
            }
           }

       if(selectedMenuItem==openWithAudacity){
           QProcess sh;
           sh.startDetached("sh",QStringList()<<"-c"<<"audacity \""+estevalor+"\"");
       }
       if(selectedMenuItem==getInfoFromMediaInfo){
           delete ui->playlist->item(rowidx);
           qDebug()<<"setInfoFromMediaInfo: "<<estevalor;
           QProcess shi;
           shi.start("sh", QStringList() << "-c" << "mediainfo \""+estevalor+"\"");
           shi.waitForFinished();
           
                   QString shiout = shi.readAll();
   
                   QStringList pieces = shiout.split( "\n" );
                   QString format = pieces.value( 2 );
                   QString size = pieces.value( 3 );
                   QString duration = pieces.value( 4 );
                   QString bitrate = pieces.value( 6 );
                   QString album = pieces.value( 7 );
                   QString artist = pieces.value( 8 );
                   QString song = pieces.value( 9 );
                   QString genre = pieces.value( 14 );

                   QStringList artist_arr = artist.split(":");
                   QString artist_name = artist_arr[1].trimmed();

                   QStringList song_arr = song.split(":");
                   QString song_name = song_arr[1].trimmed();

                   QStringList format_arr = format.split(":");
                   QString format_name = format_arr[1].trimmed();

                   QStringList album_arr = album.split(":");
                   QString album_name = album_arr[1].trimmed();

                   QStringList bitrate_arr = bitrate.split(":");
                   QString bitrate_name = bitrate_arr[1].trimmed();

                   QStringList duration_arr = duration.split(":");
                   QString duration_name = duration_arr[1].trimmed();

                   QStringList genre_arr = genre.split(":");
                   QString genre_name = genre_arr[1].trimmed();

                   QStringList size_arr = size.split(":");
                   QString size_name = size_arr[1].trimmed();

                   QString msg4box = tr("Artist: ")+artist_name+tr("; Song: ")+song_name+tr("; Album: ")+album_name+tr("; Genre: ")+genre_name+tr(" Bitrate: ")+bitrate_name+tr("; Duration:")+duration_name+tr("; size: ")+size_name+tr("; Format: ")+format_name;

                   QMessageBox::StandardButton rpl;
                   rpl = QMessageBox::question(this,tr("Apply this info to the database?"),msg4box,QMessageBox::Yes|QMessageBox::No);

                   if(rpl==QMessageBox::Yes){
                       qDebug()<<"Applying this info to the db: "<<msg4box;


                       QSqlQuery qr;
                       QString tmpqr = "update musics set artist ='"+artist_name+"', song = '"+song_name+"', genre1 = '"+genre_name+"', genre2 = '"+genre_name+"' where path = '"+estevalor+"'";

                       qr.prepare(tmpqr);
                               
                       if(qr.exec()){
                           qDebug()<<"Database updated; Query was: "<<qr.lastQuery();
                           update_music_table();
                       } else {
                           qDebug()<<"Error updating database; Query was: "<<qr.lastQuery();
                       }



                   }

       }


    }

}




void::player::jinglesViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->jinglesView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this jingle from the database");
    QString openWithAudacity = tr("Open this in Audacity");

    thisMenu.addAction(addToBottomOfPlaylist);
    thisMenu.addAction(addtoTopOfPlaylist);
    thisMenu.addAction(deleteThisFromDB);
    thisMenu.addAction(openWithAudacity);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (selectedItem)
    {
        //qDebug()<<"selected item in context menu was: "<<selectedItem->text();
        QString selectedMenuItem = selectedItem->text();
        int rowidx = ui->jinglesView->selectionModel()->currentIndex().row();
        estevalor = ui->jinglesView->model()->data(ui->jinglesView->model()->index(rowidx,1)).toString();

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
            go = QMessageBox::question(this,tr("Sure?"),tr("Are you sure you want to delete this jingle from the database?"), QMessageBox::Yes|QMessageBox::No);
            if(go==QMessageBox::Yes){
                QSqlQuery sql;
                sql.prepare("delete from jingles where path=:path");
                sql.bindValue(":path",estevalor);
                if(sql.exec()){
                    QMessageBox::information(this,tr("Jingle removed"),tr("The jingle was removed from the database!"));
                    update_music_table();
                } else {
                    QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                    qDebug() << "last sql: " << sql.lastQuery();
                }
            }
           }

       if(selectedMenuItem==openWithAudacity){
           QProcess sh;
           sh.startDetached("sh",QStringList()<<"-c"<<"audacity \""+estevalor+"\"");
       }


    }

}



void::player::pubViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->pubView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this pub from the database");
    QString openWithAudacity = tr("Open this in Audacity");

    thisMenu.addAction(addToBottomOfPlaylist);
    thisMenu.addAction(addtoTopOfPlaylist);
    thisMenu.addAction(deleteThisFromDB);
    thisMenu.addAction(openWithAudacity);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (selectedItem)
    {
        //qDebug()<<"selected item in context menu was: "<<selectedItem->text();
        QString selectedMenuItem = selectedItem->text();
        int rowidx = ui->pubView->selectionModel()->currentIndex().row();
        estevalor = ui->pubView->model()->data(ui->pubView->model()->index(rowidx,2)).toString();

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
                QSqlQuery sql;
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
           QProcess sh;
           sh.startDetached("sh",QStringList()<<"-c"<<"audacity \""+estevalor+"\"");
       }


    }

}


void::player::programsViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->programsView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this program from the database");
    QString openWithAudacity = tr("Open this in Audacity");
    QString resendtoserver = tr("(Re)Send this program to the server");
    QString checkSent = tr("Verify that the program is in the server");

    thisMenu.addAction(addToBottomOfPlaylist);
    thisMenu.addAction(addtoTopOfPlaylist);
    thisMenu.addAction(deleteThisFromDB);
    thisMenu.addAction(openWithAudacity);
    thisMenu.addAction(checkSent);
    thisMenu.addAction(resendtoserver);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (selectedItem)
    {
        //qDebug()<<"selected item in context menu was: "<<selectedItem->text();
        QString selectedMenuItem = selectedItem->text();
        int rowidx = ui->programsView->selectionModel()->currentIndex().row();
        estevalor = ui->programsView->model()->data(ui->programsView->model()->index(rowidx,2)).toString();

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
            go = QMessageBox::question(this,tr("Sure?"),tr("Are you sure you want to delete this program from the database?"), QMessageBox::Yes|QMessageBox::No);
            if(go==QMessageBox::Yes){
                QSqlQuery sql;
                sql.prepare("delete from programs where path=:path");
                sql.bindValue(":path",estevalor);
                if(sql.exec()){
                    QMessageBox::information(this,tr("Jingle removed"),tr("The program was removed from the database!"));
                    update_music_table();
                } else {
                    QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                    qDebug() << "last sql: " << sql.lastQuery();
                }
            }
           }

       if(selectedMenuItem==openWithAudacity){
           QProcess sh;
           sh.startDetached("sh",QStringList()<<"-c"<<"audacity \""+estevalor+"\"");
       }

       if(selectedMenuItem==checkSent){

           QStringList array_de_nomes = estevalor.split("/");
           QString nome = array_de_nomes.last();

           QProcess sh;
           QString shcmd = "/usr/share/xfb/scripts/serverFtpCmdsCHKProgram.sh | grep "+nome;
           qDebug()<<"Running: "<<shcmd;

           sh.start("sh",QStringList()<<"-c"<<shcmd);
           sh.waitForFinished(120000);

           QString shout = sh.readAll();

           qDebug()<<"Check file on the server output result: "<<shout;

           QString msgbtxt = tr("The program is in the server!")+"\n\n"+shout;

           if(shout != ""){
               QMessageBox::information(this,tr("OK"),msgbtxt);
           } else {
               QMessageBox::critical(this,tr("ERROR"),tr("The program is NOT in the server. Please try to upload again."));
           }


       }

       if(selectedMenuItem==resendtoserver){

           ui->txt_uploadingPrograms->show();
           QMessageBox::StandardButton sendToServer;
           sendToServer = QMessageBox::question(this,tr("(Re)Send to server?"),tr("Send programs to the server (and rewrite if it's already there)?"),QMessageBox::Yes|QMessageBox::No);
           if(sendToServer==QMessageBox::Yes){

               QFileInfo fileName(estevalor);
               QString filename = fileName.fileName();

               qDebug() << "(Re)Send to server :: Start \nProgram path: "<<estevalor
                        << "\nFile name: "<<filename<<"\nCopying to tmp FTP local folder";


               QString cp2ftp = "cp "+estevalor+" "+FTPPath+"/"+filename;
               qDebug()<<"Running: "<<cp2ftp;
               QProcess cmd;
               cmd.startDetached("sh",QStringList()<<"-c"<<cp2ftp);
               cmd.waitForFinished(-1);
               cmd.close();


                   qDebug()<<"File copied to temp FTP folder.\nSending program to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutProgram).";


                   QProcess sh,sh2;
                   QByteArray output, output2;
                   QString outPath, FTPCmdPath, xmls;


                   sh.start("sh", QStringList() << "-c" << "pwd");
                   sh.waitForFinished();
                   output = sh.readAll();
                   outPath = output;
                   QStringList path_arry = outPath.split("\n");
                   FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutProgram.sh | grep 'Transfer complete'";
                   qDebug() << "running: " << FTPCmdPath;
                   qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsPutProgram.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
                   sh.close();



                   sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
                   sh2.waitForFinished(-1);
                   output2 = sh2.readAll();
                   xmls = output2;
                   qDebug()<<output2;
                   sh2.close();


                   qDebug()<<"Program upload finished!";

                   QProcess bashDelThis;
                   QString fileToRemove = "rm "+FTPPath+"/"+filename;
                   bashDelThis.start("sh",QStringList()<<"-c"<<fileToRemove);
                   bashDelThis.waitForFinished();
                   bashDelThis.close();

                   qDebug()<<"FTP temp file deleted";

                   ui->txt_uploadingPrograms->hide();
           }



       }


    }

}





void player::on_btPlay_clicked(){
    qDebug()<<"Play button clicked";

    if(PlayMode=="stopped"){

        ui->btPlay->setStyleSheet("background-color:#2CCD54"); //green
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";
        playNextSong();

    }  else if(PlayMode=="Playing_StopAtNextOne"){

        ui->btPlay->setStyleSheet("background-color:#2CCD54"); //green
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";

    } else if(PlayMode=="Playing_Segue"){

        ui->btPlay->setStyleSheet("background-color:#F0DB1B"); //amarillo
        PlayMode = "Playing_StopAtNextOne";
        ui->btPlay->setText(tr("Play and Stop"));


    }


}

void player::playNextSong(){


    if(PlayMode=="Playing_Segue"){
        qDebug()<<"The white rabit is Playing_segue";


        if(ui->playlist->count()>0){


            QString itemDaPlaylist = ui->playlist->item(0)->text();

            qDebug()<<"itemDaPlaylist has value "<<itemDaPlaylist;

            if((lastPlayedSong!=itemDaPlaylist)||(autoMode==0)){
                qDebug()<<"lastplayesong != itemdaplaylist";

                Xplaylist->removeMedia(0);
                Xplaylist->addMedia(QUrl::fromLocalFile(itemDaPlaylist));

                lastPlayedSong = itemDaPlaylist;

                int dotsNumInString = itemDaPlaylist.count(".");

                qDebug()<<"dotsNumInString has value: "<<dotsNumInString;

                QFileInfo fileName(itemDaPlaylist);
                QString baseName = fileName.fileName();
                ui->txtNowPlaying->setText(baseName);

                QDateTime now = QDateTime::currentDateTime();
                QString text = now.toString("yyyy-MM-dd || hh:mm:ss ||");
                QString historyNewLine = text + " " + baseName;
                ui->historyList->addItem(historyNewLine);
                int hlistcout = ui->historyList->count();
                qDebug()<<"historyList has "<<hlistcout<<" items";

                if(hlistcout > 99){
                    qDebug()<<"HistoryList is beeing cleaned beacuse it's over 100 records now...";
                    delete ui->historyList->item(0);
                }

                Xplayer->play();

                delete ui->playlist->item(0);

                if(ui->checkBox_update_last_played_values->isChecked()){

                    QSqlQuery* qry = new QSqlQuery();
                    QString qrystr = "update musics set last_played = '"+now.toString("yyyy-MM-dd || hh:mm:ss")+"' where path = \""+lastPlayedSong+"\"";
                    qry->exec(qrystr);

                    qrystr = "update musics set played_times = played_times+1 where path = \""+lastPlayedSong+"\"";
                    qry->exec(qrystr);

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

                        int num = 1;
                        QSqlQuery query;
                        query.prepare("select path from jingles order by random() limit :num");
                        query.bindValue(":num", num);
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
            qDebug()<<"Almost giving up dude.. there's nothing to play.. but trying again since we are in autoMode..";
            playlistAboutToFinish();
            playNextSong();
            return;
        }

        qDebug()<<"I'm giving up dude.. there's nothing to play..";
        Xplayer->stop();
        ui->btPlay->setStyleSheet("");
        ui->btPlay->setText(tr("Play"));
        PlayMode = "stopped";

    }




} else if(PlayMode=="Playing_StopAtNextOne"){
    qDebug()<<"The white rabit is Playing_StopAtNextOne";
    Xplayer->stop();
    ui->btPlay->setStyleSheet("");
    ui->btPlay->setText(tr("Play"));
    PlayMode = "stopped";

} else if(PlayMode=="stopped"){
    qDebug()<<"The white rabit is stopped";
}

calculate_playlist_total_time();

}

void player::on_btStop_clicked()
{
    Xplayer->stop();
    ui->btPlay->setStyleSheet("");
    ui->btPlay->setText(tr("Play"));
    PlayMode = "stopped";
    Xplaylist->removeMedia(0);
}

void player::on_sliderProgress_sliderMoved(int position)
{
    //qDebug()<<"progreess slider mooved "<<position;
    Xplayer->setPosition(position);
}

void player::on_sliderVolume_sliderMoved(int position)
{
    //qDebug()<<"volume slider mooved "<<position;
    Xplayer->setVolume(position);

}

void player::onPositionChanged(qint64 position)
{

     ui->sliderProgress->setValue(position);

    int valor = (position*100)/trackTotalDuration;

    QString tamanhoDoValor = QString::number(valor);
    //qDebug()<<"Tamanho do valor: "<<tamanhoDoValor;

    QStringList segundoValorTamanhoDoValor = tamanhoDoValor.split("");
      //qDebug() << "O segundo valor de TamanhoDoValor é: " << segundoValorTamanhoDoValor;


    if(segundoValorTamanhoDoValor[2]=="0" && valor != lastTrackPercentage)
    {
        qDebug()<<"trackPercentage: "<<valor;
        lastTrackPercentage = valor;
    }

    if(valor >= 80 && onAbout2Finish == 0){
         playlistAboutToFinish();
     }

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
    QString txtElapsedTimeLable =  xhoras+":"+xminutos+":"+xsegundos+" of "+txtDuration;
    ui->txtDuration->setText(txtElapsedTimeLable);

}

void player::durationChanged(qint64 position)
{
    qDebug()<<"Xplayer durationChanged changed to "<<position;
    ui->sliderProgress->setMaximum(position);
    trackTotalDuration = position;

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

void player::currentMediaChanged(const QMediaContent &content)
{
    //qDebug()<<"On currentMediaChanged with content: "<<content;
    if(onAbout2Finish==1)
    {
        onAbout2Finish = 0;
        Xplaylist->removeMedia(Xplaylist->currentIndex());
        playNextSong();
    }

}

void player::volumeChanged(int volume){
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
        movie->stop();
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

void player::lp1_currentMediaChanged(const QMediaContent &content)
{

   qDebug()<<"LP 1 Current Media Changed..";

}

void player::lp1_volumeChanged(int volume){
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
        movie->stop();
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

void player::lp2_currentMediaChanged(const QMediaContent &content)
{

   qDebug()<<"LP 2 Current Media Changed..";

}

void player::lp2_volumeChanged(int volume){
    qDebug()<<"LP 2 Volume: "<<volume;
}

void player::playlistAboutToFinish()
{
    qDebug()<<"Launched playlistAboutToFinish";
    onAbout2Finish = 1;

    int numItemsInPlaylist = ui->playlist->count();
    if(numItemsInPlaylist==0)
        autoModeGetMoreSongs();

}


void player::update_music_table(){
    checkDbOpen();
    /*Populate music table with an editable table field on double-click*/

    QSqlTableModel * model = new QSqlTableModel(this);
    model->setTable("musics");
    model->select();
    ui->musicView->setModel(model);


    /*Populate jingles table with an editable table field on double-click*/
    QSqlTableModel * jinglesmodel = new QSqlTableModel(this);
    jinglesmodel->setTable("jingles");
    jinglesmodel->select();
    ui->jinglesView->setModel(jinglesmodel);


    /*Populate Pub table*/
    QSqlTableModel *pubmodel = new QSqlTableModel(this);
    pubmodel->setTable("pub");
    pubmodel->select();
    ui->pubView->setModel(pubmodel);

    /*Populate Programs table*/
    QSqlTableModel *programsmodel = new QSqlTableModel(this);
    programsmodel->setTable("programs");
    programsmodel->select();
    ui->programsView->setModel(programsmodel);


    /*Populate genre1 and 2 filters*/
    QSqlQueryModel * model_genre1=new QSqlQueryModel();
    //QSqlQueryModel * model_genre2=new QSqlQueryModel();

    QSqlQuery* qry=new QSqlQuery();

    QString sqlq = "select name from genres1";
    qry->exec(sqlq);
    model_genre1->setQuery(*qry);
    ui->cBoxGenre1->setModel(model_genre1);
    ui->cBoxGenre2->setModel(model_genre1);

    ui->comboBox_random_add_genre->setModel(model_genre1);

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

    ui->musicView->selectRow(index.row());
    int rowidx = ui->musicView->selectionModel()->currentIndex().row();
    estevalor = ui->musicView->model()->data(ui->musicView->model()->index(rowidx,7)).toString();
    qDebug () << "Music path: [" << estevalor << "]";

            xaction = "drag_to_music_playlist";
            //check if file exists and avoid adding if it does not
            bool ha = QFile::exists (estevalor);
            if(!ha){

                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, "The file does NOT exist?", "It seams like the file does NOT exist on the hard drive... Or there is a problem reading it. Should it be deleted from the database?",
                                              QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                  qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path was changed)";
                      QSqlQuery* qry=new QSqlQuery();
                      qry->prepare("delete from musics where path = :thpath");
                      qry->bindValue(":thpath",estevalor);

                     if(qry->exec()){
                          qDebug() << "Music Deleted from database! last query was:"<< qry->lastQuery();
                          update_music_table();
                     } else {
                         qDebug() << "There was an error deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                     }
                } else {
                  qDebug() << "keeping invalid record in db... please fix path manually..";
                }

            } else {

                //the file DOES exist .. :-);

                QDrag *drag = new QDrag(this);
                QMimeData *mimeData = new QMimeData;

                mimeData->setData(text, "drag_to_music_playlist");
                drag->setMimeData(mimeData);
                Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
                qDebug() << "DropAction near 1081 has: " << dropAction;


            }

}

void player::on_jinglesView_pressed(const QModelIndex &index)
{
    indexJust3rdDropEvt=0;

        //int thisid = index.row()+1;
        ui->jinglesView->selectRow(index.row());
        int rowidx = ui->jinglesView->selectionModel()->currentIndex().row();

        QSqlTableModel * model = new QSqlTableModel(this);
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
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery();
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

        //int thisid = index.row()+1;
        ui->pubView->selectRow(index.row());
        int rowidx = ui->pubView->selectionModel()->currentIndex().row();

        QSqlTableModel * model = new QSqlTableModel(this);
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
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery();
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

        //int thisid = index.row()+1;
        ui->programsView->selectRow(index.row());
        int rowidx = ui->programsView->selectionModel()->currentIndex().row();

        QSqlTableModel * model = new QSqlTableModel(this);
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
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery();
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

void player::on_bt_autoMode_clicked()
{
    if(autoMode == 0){
        autoMode = 1;
        qDebug()<<"autoMode is ON";
        ui->bt_autoMode->setStyleSheet("background-color: rgb(175, 227, 59)");
    } else {
        autoMode = 0;
        qDebug()<<"autoMode is OFF";
        ui->bt_autoMode->setStyleSheet("");
    }
}

void player::autoModeGetMoreSongs()
{

    //check if there's a programed genre for this hour in the hourgenre table

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

    QString qry="select genre from hourgenre where hour="+currentHour+" and day="+dw;
    qDebug()<<"hourgenre query is: "<<qry;

    QSqlQuery runQry;
    QString currentGenre = "";

    if(runQry.exec(qry)){

        qDebug()<<"The query was successfull: "<<qry;

        while(runQry.next()){
            currentGenre = runQry.value(0).toString();
            qDebug()<<"We now have selected the following genre for this hour, based on the data from the hourgenre table in the database: "<<currentGenre;
        }

    } else {

        qDebug()<<"Oh dear, what now? "<<runQry.lastError();


    }


    if(currentGenre.isEmpty()){

           if(autoMode==1){
               //randomly select music from db
               int numMusics = 1;
               QSqlQuery query;
               query.prepare("select path from musics order by random() limit :numMusics");
               query.bindValue(":numMusics", numMusics);
               if(query.exec())
               {
                   qDebug() << "SQL query executed: " << query.lastQuery();

                   while(query.next()){
                       QString path = query.value(0).toString();

                       if(path!=lastPlayedSong){
                           ui->playlist->addItem(path);
                           qDebug() << "autoMode random music chooser adding: " << path;

                       } else {

                           QSqlQuery numOfItemsInDB;
                           numOfItemsInDB.prepare("select count(path) from music where 1");
                           numOfItemsInDB.exec();
                           if(numOfItemsInDB.value(0).toInt()>1){
                               qDebug()<<"autoMode picked the same song that was played before... Small DB or a Big Coincidence? Quering again for a 'random' new song..";
                               autoModeGetMoreSongs();
                           }

                       }

                   }

               } else {
                   qDebug() << "SQL ERROR: " << query.lastError();
                   qDebug() << "SQL was: " << query.lastQuery();


               }
           }

    } else {

        if(autoMode==1){
            //randomly select music from db based on genre for this hour

            QString querystr = "select path from musics where genre1 like '"+currentGenre+"' order by random() limit 1";
            QSqlQuery query;
            if(query.exec(querystr))
            {
                qDebug() << "SQL query executed: " << query.lastQuery();

                while(query.next()){
                    QString path = query.value(0).toString();

                    if(path!=lastPlayedSong){
                        ui->playlist->addItem(path);
                        qDebug() << "autoMode genre based random music chooser adding: " << path;

                    } else {

                        QSqlQuery numOfItemsInDB;
                        numOfItemsInDB.prepare("select count(path) from music where 1");
                        numOfItemsInDB.exec();
                        if(numOfItemsInDB.value(0).toInt()>1){
                            qDebug()<<"autoMode picked the same song that was played before... Small DB or a Big Coincidence? Quering again for a 'random' new song..";
                            autoModeGetMoreSongs();
                        }

                    }

                }

            } else {
                qDebug() << "SQL ERROR: " << query.lastError();
                qDebug() << "SQL was: " << query.lastQuery();


            }
            currentGenre = "";
        }

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

    Xplaylist->removeMedia(0);
    playNextSong();

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

                        if (Rxml.name() == "XFBClientTakeOver") {
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

                        if(Rxml.name()=="www.netpack.pt"){
                            qDebug()<<"Token element: "<<Rxml.name();
                            Rxml.readNext();
                        }

                        if(Rxml.name()=="ip"){
                            takeOverIP = Rxml.readElementText();
                            qDebug()<<"takeOverIP: "<<takeOverIP;
                        }

                        if(Rxml.name()=="stream"){
                            takeOverStream = Rxml.readElementText();
                            qDebug()<<"TakeOverStream: "<<takeOverStream;



                            //play



                            radio1str = "mplayer -volume 100 -playlist "+takeOverStream;

                            qDebug()<<"Full cmd is: "<<radio1str;

                            radio1.start("sh",QStringList()<<"-c"<<radio1str);
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

                            connect(stimer, SIGNAL(timeout()), this, SLOT(pingTakeOverClient()));
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

                        if (Rxml.name() == "XFBClientTakeOver") {
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

                        if(Rxml.name()=="www.netpack.pt"){
                            qDebug()<<"Token element: "<<Rxml.name();
                            Rxml.readNext();
                        }

                        if(Rxml.name()=="ip"){
                            returnTakeOverIP = Rxml.readElementText();
                            qDebug()<<"returnTakeOverIP: "<<returnTakeOverIP;
                        }

                        if(Rxml.name()=="cmd"){
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
            QDirIterator fit(dir,QStringList()<< "*.mp3"<<"*.mp4"<<"*.ogg"<<"*.wav"<<"*.flac",QDir::Files,QDirIterator::Subdirectories);
            while (fit.hasNext()){

                QString file = fit.next();

                qDebug()<<"Found: "<<file;
                
                

                
                //for every file in this folder check if it exist in DB
                
                
                
                QSqlQuery sql;
                QString qry = "SELECT path from programs where path='"+file+"'";
                
                if(sql.exec(qry)){
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



                        QSqlQuery sql_add;
                        QString qry_add = "insert into programs values(NULL,'"+filename+"','"+file+"')";
                        if(sql_add.exec(qry_add)){
                            qDebug()<<"Query OK. Program localy added to programs table";

                             //schelule it





                            QSqlQuery qryid;
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

                                        QSqlQuery qhm;

                                        if(qhm.exec(qryhourmin)){

                                            QString def = "def";

                                            while(qhm.next()){
                                                def = "not";
                                                QString pHora = qhm.value(0).toString();
                                                QString pMin = qhm.value(1).toString();

                                                QSqlQuery addsch;
                                                QString addstr = "insert into scheduler values ('"+pID+"','"+pAno+"','"+pMes+"','"+pDia+"','"+pHora+"','"+pMin+"','1',NULL,NULL,NULL,NULL,NULL,NULL,NULL,'1')";

                                                if(addsch.exec(addstr)){
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



                QSqlQuery sql;
                QString qry = "SELECT path from musics where path='"+file+"'";

                if(sql.exec(qry)){
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



                        QProcess cmd;
                        QString time;
                        QString cmdtmpstr = "exiftool \""+file+"\" | grep Duration";
                        cmd.start("sh",QStringList()<<"-c"<<cmdtmpstr);
                        cmd.waitForFinished();
                        QString cmdOut = cmd.readAll();
                        qDebug()<<"Output of exiftool: "<<cmdOut;
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

                        QSqlQuery sql_add;
                        int played = 0;
                        QString last = "-";
                        QString qry_add = "insert into musics values(NULL,'"+artist+"','"+song+"','"+g1+"','"+g2+"','"+country+"','"+pub_date+"','"+file+"','"+time+"','"+played+"','"+last+"')";
                        if(sql_add.exec(qry_add)){
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








    qDebug()<<"Looking for musics on the root folder... [BUG FIX :: NOT LOOKING BECAUSE IT HAS TO BE MOOVED TO ANOTHER THREAD FIST]";
    /*
    QDirIterator rfmit(MusicPath,QStringList()<< "*.mp3"<<"*.mp4"<<"*.ogg"<<"*.wav"<<"*.flac",QDir::Files);
    while (rfmit.hasNext()){

        QString file = rfmit.next();

        qDebug()<<"Found: "<<file;



        //for every file in this folder check if it exist in DB



        QSqlQuery sql;
        QString qry = "SELECT path from musics where path='"+file+"'";

        if(sql.exec(qry)){
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



                QProcess cmd;
                QString time;
                QString cmdtmpstr = "exiftool \""+file+"\" | grep Duration";
                cmd.start("sh",QStringList()<<"-c"<<cmdtmpstr);
                cmd.waitForFinished();
                QString cmdOut = cmd.readAll();
                qDebug()<<"Output of exiftool: "<<cmdOut;
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


                QSqlQuery sql_add;
                int played = 0;
                QString last = "-";
                QString qry_add = "insert into musics values(NULL,'"+artist+"','"+song+"','"+g1+"','"+g2+"','"+country+"','"+pub_date+"','"+file+"','"+time+"','"+played+"','"+last+"')";
                if(sql_add.exec(qry_add)){
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

                    QSqlQuery qry;
                    QString thisquery = "insert into programs values(NULL,'"+nomeDoPrograma+"','"+fileNameWPath+"')";
                    if(qry.exec(thisquery)){
                        qDebug()<<"server_ftp_check() :: Query OK. Program added to programs table";
                    } else {
                         qDebug()<<"server_ftp_check() :: Query was not ok while atempting to add to the programs table";
                    }

                    QSqlQuery qryid;
                    QString thisqueryid = "select * from programs where path like '"+fileNameWPath+"'";
                    qDebug()<<"server_ftp_check() :: Select id query is: "<<thisqueryid;
                    if(qryid.exec(thisqueryid)){
                        while(qryid.next()){
                            QString pID = qryid.value(0).toString();
                            qDebug()<<"server_ftp_check() :: Query OK. This id is: "<<pID;

                            QStringList dataarr = dataDoPrograma.split("-");
                            QString pAno = dataarr[0];
                            QString pMes = dataarr[1];
                            QString pDia = dataarr[2];

                            if(!pAno.isEmpty() && !pMes.isEmpty() && !pDia.isEmpty()){

                                QString qryhourmin = "select hour, min from hourprograms where name like '"+nomeDoPrograma+"'";

                                QSqlQuery qhm;

                                if(qhm.exec(qryhourmin)){

                                    while(qhm.next()){
                                        QString pHora = qhm.value(0).toString();
                                        QString pMin = qhm.value(1).toString();

                                        QSqlQuery addsch;
                                        QString addstr = "insert into scheduler values ('"+pID+"','"+pAno+"','"+pMes+"','"+pDia+"','"+pHora+"','"+pMin+"','1',NULL,NULL,NULL,NULL,NULL,NULL,NULL,'1')";

                                        if(addsch.exec(addstr)){
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

    QFile::remove("/usr/share/xfb/ftp/takeover.xml");

    QString takeOverFile = "/usr/share/xfb/ftp/returntakeover.xml";
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


        QProcess sh,sh2;
        QByteArray output, output2;
        QString outPath, FTPCmdPath, xmls;


        sh.start("sh", QStringList() << "-c" << "pwd");
        sh.waitForFinished();
        output = sh.readAll();
        outPath = output;
        QStringList path_arry = outPath.split("\n");
        FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutTakeOver.sh | grep 'Transfer complete'";
        qDebug() << "running: " << FTPCmdPath;
        qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsPutTakeOver.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
        sh.close();



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

            QFile::remove("/usr/share/xfb/ftp/takeover.xml");

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
    kb.startDetached("sh",QStringList()<<"-c"<<"killall mplayer");
    kb.waitForFinished();

    qDebug()<<"All instances of mplayer were closed";

}


void player::run_scheduler(){

    checkDbOpen();

    /*check if there is any thing to go on now*/

    qDebug()<<"\nThe scheduller is now monitoring the database (the scheduler table) to check if there is something to go on at "<<QDateTime::currentDateTime().toString()<<"\n";



QSqlQuery sched_qry;
sched_qry.prepare("select * from scheduler");
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
                    QSqlQuery getPath;
                    if(is_program=="1"){
                        getPath.prepare("select path from programs where id='"+schId+"'");
                    } else {
                        getPath.prepare("select path from pub where id='"+schId+"'");
                    }

                    if(getPath.exec()){
                        while(getPath.next()){
                            QString pubPath = getPath.value(0).toString();
                            ui->playlist->insertItem(0,pubPath);
                            qDebug()<<"Scheduled event added to the top of the playlist: "<<pubPath;
                        }

                    }

                    //delete scheduler row cause its a type 1

                    QSqlQuery del_qry;
                    del_qry.prepare("delete from scheduler where id='"+schId+"' and ano='"+ano1+"' and mes='"+mes1+"' and dia='"+dia1+"' and ano='"+ano1+"' and hora='"+hora1+"' and min='"+min1+"'");
                    if(del_qry.exec()){
                        qDebug () << "Scheduled rule was deleted!";
                    } else {
                        qDebug()<<"exeption deleting scheduled rule with qry: "<<del_qry.lastQuery()<<" we got: "<<del_qry.lastError();
                    }

                    //check if pub still has other scheduler rules and delete from pub if not
                    QSqlQuery sq;
                    sq.prepare("select count(id) from scheduler where id='"+schId+"'");
                    if(sq.exec()){

                        while(sq.next()){
                            qDebug()<<"We have a total of "<<sq.value(0).toString()<<" records in the scheduler table with the schId "<<schId;
                            if(sq.value(0).toString()=="0"){
                                //we can delete it from pub cause no more scheduled rules apply

                                QSqlQuery sd;
                                sd.prepare("delete from pub where id='"+schId+"'");
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
            int dia = QDate::currentDate().dayOfWeek();

            QString dw;
            if(dia == 1) dw = "Monday";
            if(dia == 2) dw = "Tuesday";
            if(dia == 3) dw = "Wednesday";
            if(dia == 4) dw = "Thursday";
            if(dia == 5) dw = "Friday";
            if(dia == 6) dw = "Saturday";
            if(dia == 7) dw = "Sunday";

            //qDebug()<<"Today it's "<< dw << "!";


            QString is_program = sched_qry.value(14).toString();



            QDateTime now = QDateTime::currentDateTime();

            QString hora1 = now.toString("h");
            QString min1 = now.toString("m");



            QString dw2 = sched_qry.value(7).toString();
            QString hora2 = sched_qry.value(4).toString();
            QString min2 = sched_qry.value(5).toString();



            //so:
            //
            //dw has today's day of the week
            //hora1 has the current hour
            //min1 has the current minute
            //dw2 has the scheduled day of the week
            //hora2 has the scheduled hour
            //min2 has the scheduled minute
            //
            // /////////////// //

            if((dw==dw2)&&(hora1==hora2)&&(min1==min2)){
                qDebug() << "Scheduled event now fired (type 2)!";
                QString schId = sched_qry.value(0).toString();

                //add to playlist
                QSqlQuery getPath;
                if(is_program=="1"){
                    getPath.prepare("select path from programs where id='"+schId+"'");
                } else {
                    getPath.prepare("select path from pub where id='"+schId+"'");
                }

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







void player::on_actionOptions_triggered()
{


    optionsDialog opt;
    opt.setModal(true);
    opt.exec();
    update_music_table();

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
    widget->show();

}

void player::on_bt_search_clicked()
{
    //search
    qDebug()<<"Staring a new search!";
    QString term = ui->txt_search->text();

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("select * from musics where artist like '%"+term+"%' or song like '%"+term+"%'");
    ui->musicView->setModel(model);

    ui->musicView->setSortingEnabled(true);
    ui->musicView->hideColumn(0);
    ui->musicView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}

void player::on_bt_reset_clicked()
{
    QSqlQueryModel *modelo = new QSqlQueryModel();
    modelo->setQuery("select * from musics");
    ui->musicView->setModel(modelo);
}

void player::on_bt_apply_filter_clicked()
{
    //filter by genres

    QString addG1 = "";
    QString addG2 = "";

    bool g1_checked = ui->checkBox_filter_genre1->checkState(); //true or false
    bool g2_checked = ui->checkBox_filter_genre2->checkState();

    qDebug () << "132426032015 " << g1_checked << " : " << g2_checked;

    if(g1_checked == true){
        qDebug()<<"g1 is checked";
        QString selectedGenre1 = ui->cBoxGenre1->currentText();
        addG1 = " genre1='"+selectedGenre1+"' ";










        QSqlQuery sql;
        QString qry = "select count(*) from musics where genre1 like '"+selectedGenre1+"'";

        if(sql.exec(qry)){
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
        QString selectedGenre2 = ui->cBoxGenre2->currentText();
        if(g1_checked==true){
             qDebug()<<"both are checked...";
             addG2 = "and genre2='"+selectedGenre2+"' ";
        } else{
            qDebug()<<"Only g2 is checked";
            addG2 = " genre2='"+selectedGenre2+"' ";
        }

    }
    qDebug()<<"addG1 is "<<addG1<<" and addG2 is "<<addG2;
    if(addG1 != "" || addG2 != ""){
        qDebug() << "making a new table to show with the results...";
        QSqlQueryModel * model = new QSqlQueryModel();
        model->setQuery("select * from musics where "+addG1+addG2);
        ui->musicView->setModel(model);
    }




}

void player::on_bt_updateTables_clicked()
{
    update_music_table();
}
/*
void player::on_bt_youtubeDL_clicked()
{
    externaldownloader* widget = new externaldownloader;
    widget->setAttribute(Qt::WA_DeleteOnClose);
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

               xmlWriter.writeTextElement("track",txtItem);
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

                    if (Rxml.name() == "XFBPlaylist") {
                        qDebug()<<"Valid XFB Playlist Found!";
                        Rxml.readNext();

                        if(Rxml.isEndElement()){
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

                    if(Rxml.name()=="www.netpack.pt"){
                        qDebug()<<"Token element: "<<Rxml.name();
                        Rxml.readNext();
                    }

                    if(Rxml.name()=="track"){
                        QString track = Rxml.readElementText();
                        qDebug()<<"Rxml.readElementText(): "<<track;
                        ui->playlist->addItem(track);
                    }



                }
    }
    file.close();
}



void player::on_bt_rec_clicked()
{

 if(recMode == 0){
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
    } else {
        //green
         ui->led_rec->setStyleSheet("background-color:#B3FF57;border-radius:8px;");
    }



}

void player::RecT5(){

    //amarilo
    ui->led_rec->setStyleSheet("background-color:#FFFB00;border-radius:8px;");
    ui->led_rec->show();

    QProcess sh;
    sh.startDetached("sh",QStringList()<<"-c"<<"rm -f \""+saveFile+"\"");

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


    audioRecorder->setAudioInput(recDevice);

    qDebug()<<"Selecting this audio input device: "<<recDevice;





        QAudioEncoderSettings settings;
        //settings.setCodec(codec);
        //settings.setChannelCount(2);
        settings.setQuality(QMultimedia::EncodingQuality(int(QMultimedia::VeryHighQuality)));
        settings.setEncodingMode(QMultimedia::ConstantBitRateEncoding);


        //QString container = "ogg";
        aExtencaoDesteCoiso = "ogg";

        if(contentamento=="matroska")
            aExtencaoDesteCoiso = "mp3";
        if(contentamento=="ogg")
            aExtencaoDesteCoiso = "ogg";
        if(contentamento=="mp4")
            aExtencaoDesteCoiso = "mp4";
        if(contentamento=="wav")
            aExtencaoDesteCoiso = "wav";
        if(contentamento=="quicktime")
            aExtencaoDesteCoiso = "mov";
        if(contentamento=="avi")
            aExtencaoDesteCoiso = "avi";
        if(contentamento=="3gpp")
            aExtencaoDesteCoiso = "3gpp";
        if(contentamento=="flv")
            aExtencaoDesteCoiso = "fvl";
        if(contentamento=="amr")
            aExtencaoDesteCoiso = "amr";
        if(contentamento=="asf")
            aExtencaoDesteCoiso = "asf";
        if(contentamento=="dv")
            aExtencaoDesteCoiso = "dv";
        if(contentamento=="mpeg")
            aExtencaoDesteCoiso = "mpg";
        if(contentamento=="vob")
            aExtencaoDesteCoiso = "vob";
        if(contentamento=="mpegts")
            aExtencaoDesteCoiso = "mpeg";
        if(contentamento=="3g2")
            aExtencaoDesteCoiso = "3g2";
        if(contentamento=="3gp")
            aExtencaoDesteCoiso = "3gp";
        if(contentamento=="raw")
            aExtencaoDesteCoiso = "raw";


    QAudioEncoderSettings audioSettings;
    audioSettings.setQuality(QMultimedia::HighQuality);

    audioRecorder->setEncodingSettings(audioSettings, QVideoEncoderSettings(), contentamento);
    audioRecorder->setOutputLocation(QUrl(saveFile));
    audioRecorder->record();

    qDebug()<<"Audio Rec Channel Count is: "<<settings.channelCount();

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
    QMessageBox::information(this,tr("Check for updates"),tr("Please update with 'yay' under Arch-based distros, or clone from XFB github."));
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

    saveProgram = QMessageBox::question(this,tr("Save Program?"),tr("Save this program?"),QMessageBox::Yes|QMessageBox::No);
    if(saveProgram==QMessageBox::Yes){
            qDebug()<<"Saving the program";


            if(saveFile.isEmpty()){
                QMessageBox::information(this,tr("No program set or recorded"),tr("There is no program set or recorded to send"));
            } else {
                qDebug()<<"Processing file: "<<saveFile;

                if(Role=="Client"){

                        ui->txt_uploadingPrograms->show();
                        QProcess cmd;
                        destinationProgram = ProgramsPath+"/"+NomeDestePrograma+"."+aExtencaoDesteCoiso;
                        QString mvcmd = "cp "+saveFile+" "+destinationProgram;
                        qDebug()<<"Running: "<<mvcmd;
                        cmd.startDetached("sh",QStringList()<<"-c"<<mvcmd);
                        cmd.waitForFinished(-1);
                        cmd.close();

                        QMessageBox::StandardButton sendToServer;

                        sendToServer = QMessageBox::question(this,tr("Send to server?"),tr("Send programs to the server?"),QMessageBox::Yes|QMessageBox::No);
                        if(sendToServer==QMessageBox::Yes){

                            QString cp2ftp = "cp "+saveFile+" "+FTPPath+"/"+NomeDestePrograma+"."+aExtencaoDesteCoiso;
                            qDebug()<<"Running: "<<cp2ftp;
                            cmd.startDetached("sh",QStringList()<<"-c"<<cp2ftp);
                            cmd.waitForFinished(-1);
                            cmd.close();

                                qDebug()<<"Sending program to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutProgram).";

                                QProcess sh,sh2;
                                QByteArray output, output2;
                                QString outPath, FTPCmdPath, xmls;

                                sh.start("sh", QStringList() << "-c" << "pwd");
                                sh.waitForFinished();
                                output = sh.readAll();
                                outPath = output;
                                QStringList path_arry = outPath.split("\n");
                                FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutProgram.sh | grep 'Transfer complete'";
                                qDebug() << "running: " << FTPCmdPath;
                                qDebug() << "If you get errors: cd scripts && chmod +x serverFtpCmdsPutProgram.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
                                sh.close();

                                sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
                                sh2.waitForFinished(-1);
                                output2 = sh2.readAll();
                                xmls = output2;
                                qDebug()<<output2;
                                sh2.close();

                                qDebug()<<"Program upload finished!";

                                QProcess bashDelThis;
                                QString fileToRemove = "rm "+FTPPath+"/"+NomeDestePrograma+".ogg";
                                bashDelThis.start("sh",QStringList()<<"-c"<<fileToRemove);
                                bashDelThis.waitForFinished();
                                bashDelThis.close();

                                qDebug()<<"FTP temp file deleted";

                                QMessageBox::StandardButton answer;
                                answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program?"), QMessageBox::Yes|QMessageBox::No);

                                if(answer==QMessageBox::Yes){

                                    QProcess bashDoThis;
                                    QString fileToRemoveStr = "rm "+destinationProgram;
                                    bashDoThis.start("sh",QStringList()<<"-c"<<fileToRemoveStr);
                                    bashDoThis.waitForFinished();
                                    bashDoThis.close();
                                    QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
                                    ui->txt_ProgramName->hide();
                                    ui->bt_ProgramStopandProcess->hide();

                                } else {

                                            QSqlQuery qry;
                                            QString thisquery = "insert into programs values(NULL,'"+NomeDestePrograma+"','"+destinationProgram+"')";
                                            if(qry.exec(thisquery)){
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
    ui->txt_creatingPrograms->show();
      qDebug()<<"Running Make_a_program_with_the_current_playlist";


      QString cmdcp;
      cmdcp= "rm ../ProgramGenerator/*.ogg";
      QProcess cmddelP;
      cmddelP.start("sh",QStringList()<<"-c"<<cmdcp);
      cmddelP.waitForFinished(-1);
      cmddelP.close();
       qDebug()<<"temp files from ProgramGenerator folder removed";

      cmdcp= "rm inputs.txt";
      cmddelP.start("sh",QStringList()<<"-c"<<cmdcp);
      cmddelP.waitForFinished(-1);
      cmddelP.close();
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
              QString cmdcp = "cp \""+txtItem+"\" ../ProgramGenerator/"+nome;
              qDebug()<<">> >> >> >> >> Adding audio: ";
              qDebug()<<cmdcp;
              QProcess cmd;
              cmd.start("sh",QStringList()<<"-c"<<cmdcp);
              cmd.waitForFinished(-1);
              cmd.close();

              contatstr = "../ProgramGenerator/"+nome;
              QProcess cmd34;
              cmd34.start("sh",QStringList()<<"-c"<<"echo \"file '"+contatstr+"'\" >> inputs.txt");
              cmd34.waitForFinished(-1);
              cmd34.close();

          }

      }
      QString cmdconcat = "ffmpeg -f concat -i inputs.txt "+destino;
      qDebug()<<">> >> >> >> >> Concatenating audio.. the full command is: >> >> >> >> >> >>";
      qDebug()<<cmdconcat;
      QProcess cmd;
      cmd.start("sh",QStringList()<<"-c"<<cmdconcat);
      cmd.waitForFinished(300000); //600000 10 minutos



      cmd.close();
      ui->txt_creatingPrograms->hide();
      QString cmddel = "rm ../ProgramGenerator/*.ogg";
      qDebug()<<">> >> >> >> >> Removing tmp audio: ";
      qDebug()<<cmddel;
      cmd.start("sh",QStringList()<<"-c"<<cmddel);
      cmd.waitForFinished(-1);
      cmd.close();
      ui->txt_uploadingPrograms->show();
      
      
      
      
      QMessageBox::StandardButton sendToServer;
      sendToServer = QMessageBox::question(this,tr("Send to server?"),tr("Send programs to the server?"),QMessageBox::Yes|QMessageBox::No);
      if(sendToServer==QMessageBox::Yes){
          QString cp2ftp = "cp "+destino+" "+FTPPath+"/"+NomeDestePrograma+".ogg";
          qDebug()<<"Running: "<<cp2ftp;
          QProcess cmd;
          cmd.startDetached("sh",QStringList()<<"-c"<<cp2ftp);
          cmd.waitForFinished(-1);
          cmd.close();

              qDebug()<<"Sending program to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutProgram).";
              QProcess sh,sh2;

              QByteArray output, output2;
              QString outPath, FTPCmdPath, xmls;
              sh.start("sh", QStringList() << "-c" << "pwd");
              sh.waitForFinished();
              output = sh.readAll();
              outPath = output;
              QStringList path_arry = outPath.split("\n");
              FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutProgram.sh | grep 'Transfer complete'";
              qDebug() << "running: " << FTPCmdPath;
              qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsPutProgram.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
              sh.close();
              sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
              sh2.waitForFinished(-1);
              output2 = sh2.readAll();
              xmls = output2;
              qDebug()<<output2;
              sh2.close();
              qDebug()<<"Checking if the file's intergrity was perserved...";
              QProcess shCHK,shCHK2;
              QByteArray outputCHK, outputCHK2;
              QString outPathCHK, FTPCmdPathCHK, xmlsCHK;
              shCHK.start("sh", QStringList() << "-c" << "pwd");
              shCHK.waitForFinished();
              outputCHK = shCHK.readAll();
              outPathCHK = outputCHK;
              QStringList path_arryCHK = outPathCHK.split("\n");
              FTPCmdPathCHK = path_arryCHK[0]+"/usr/share/xfb/scripts/serverFtpCmdsCHKProgram | grep "+NomeDestePrograma;
              qDebug() << "running: " << FTPCmdPathCHK;
              qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsCHKProgram && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
              shCHK.close();
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
                          QMessageBox::information(this,tr("Interity verification faild!"),tr("The file does not seam to have been sent to the server correctly since the size of the local file differs from the one on the FTP server. Please try to send the program again."));
                      }
                  } else {
                      qDebug()<<"It was not possible to get the size of the local file: "<<mmfile;
                  }
              }
              qDebug()<<"Program uploaded to server!";
              QProcess bashDelThis;
              QString fileToRemove = "rm "+FTPPath+"/"+NomeDestePrograma+".ogg";
              bashDelThis.start("sh",QStringList()<<"-c"<<fileToRemove);
              bashDelThis.waitForFinished();
              bashDelThis.close();
              qDebug()<<"FTP temp file deleted";
              QMessageBox::StandardButton answer;
              answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program? (The program was sucessfuly uploaded to the server)"), QMessageBox::Yes|QMessageBox::No);
              if(answer==QMessageBox::Yes){
                  QProcess bashDoThis;
                  QString fileToRemoveStr = "rm "+destino;
                  bashDoThis.start("sh",QStringList()<<"-c"<<fileToRemoveStr);
                  bashDoThis.waitForFinished();
                  bashDoThis.close();
                  QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
              } else {
                          QSqlQuery qry;
                          QString thisquery = "insert into programs values(NULL,'"+NomeDestePrograma+"','"+destino+"')";
                          if(qry.exec(thisquery)){
                              qDebug()<<"Query OK. Program localy added to programs table";
                          } else {
                               qDebug()<<"Query was not ok while atempting to localy add to the programs table";
                          }
              }
      } else {
          QMessageBox::StandardButton answer;
          answer = QMessageBox::question(this,tr("Delete local copy?"),tr("Delete the local copy of the program?"), QMessageBox::Yes|QMessageBox::No);
          if(answer==QMessageBox::Yes){
              QProcess bashDoThis;
              QString fileToRemoveStr = "rm "+destino;
              bashDoThis.start("sh",QStringList()<<"-c"<<fileToRemoveStr);
              bashDoThis.waitForFinished();
              bashDoThis.close();
              QMessageBox::information(this,tr("Local file deleted"),tr("The local copy of the file was deleted."));
          } else {
              QSqlQuery qry;
              QString thisquery = "insert into programs values(NULL,'"+NomeDestePrograma+"','"+destino+"')";
              if(qry.exec(thisquery)){
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

    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: Run a check on all the records in the music table; For each check if it exist and update the time info of the song",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){


        QSqlQuery qr;
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"Processing: "<<path;

                bool ha = QFile::exists(path);
                if(!ha){
                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(this, "The file does NOT exist?", "It seams like the file does NOT exist on the hard drive... Or there is a problem reading it. Should it be deleted from the database?",
                                                  QMessageBox::Yes|QMessageBox::No);
                    if (reply == QMessageBox::Yes) {
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path was changed)";
                          QSqlQuery* qry=new QSqlQuery();
                          qry->prepare("delete from musics where path = :thpath");
                          qry->bindValue(":thpath",path);

                         if(qry->exec()){
                              qDebug() << "Music Deleted from database! last query was:"<< qry->lastQuery();
                              update_music_table();
                         } else {
                             qDebug() << "There was an error deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                         }
                    } else {
                      qDebug() << "keeping invalid record in db... please fix path manually..";
                    }
                } else {

                    QFile myFile(path);

                    qDebug()<<"File.size() is now: "<<myFile.size();

                    if(myFile.size()==0){

                        QMessageBox::StandardButton reply;
                        reply = QMessageBox::question(this, "The file does exist but the it's empty?", "It seams like the file does exist on the hard drive BUT HAS 0B !... Or there is a problem reading it. Should it be deleted from the database and the hard-drive?",
                                                      QMessageBox::Yes|QMessageBox::No);
                        if (reply == QMessageBox::Yes) {
                              QSqlQuery* qry=new QSqlQuery();
                              qry->prepare("delete from musics where path = :thpath");
                              qry->bindValue(":thpath",path);

                             if(qry->exec()){
                                  qDebug() << "Music Deleted from database and HD! last query was:"<< qry->lastQuery();

                                  QProcess rmthis;
                                  QString rmthistr = "rm "+path;
                                  rmthis.start("sh",QStringList()<<"-c"<<rmthistr);
                                  rmthis.waitForFinished(-1);



                                  update_music_table();
                             } else {
                                 qDebug() << "There was an error deleting the music from the database"<< qry->lastError() << qry->lastQuery();
                             }
                        } else {
                          qDebug() << "keeping invalid record in db... please fix path manually..";
                        }


                    } else {


                     qDebug()<<"The file seams to be OK. Checking it's time...";

                     QProcess cmd;
                     QString cmdtmpstr = "exiftool \""+path+"\" | grep Duration";
                     cmd.start("sh",QStringList()<<"-c"<<cmdtmpstr);
                     cmd.waitForFinished(-1);
                     QString cmdOut = cmd.readAll();
                     qDebug()<<"Output of exiftool: "<<cmdOut;
                     cmd.close();

                     QStringList arraycmd = cmdOut.split(" ");

                     if(arraycmd.count()>25){
                     //qDebug()<<"Total track time is: "<<arraycmd[25];

                         QStringList splitarray = arraycmd[25].split("\n");

                         qDebug()<<"Total track time is: "<<splitarray[0];

                         QSqlQuery* qry = new QSqlQuery();
                         QString qrystr = "update musics set time = '"+splitarray[0]+"' where path = \""+path+"\"";
                         qry->exec(qrystr);


                         update_music_table();

                     } else {
                         qDebug()<<"-------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>     !!!!!!!!    An exception happend !? ... outputing details of this track: ";
                         for(int i=0;i<arraycmd.count();i++){
                             qDebug()<< "The array position "<<i<<" has: "<<arraycmd[i];
                         }
                     }

                     QSqlQuery* qry = new QSqlQuery();
                     QString qrystr = "update musics set played_times = 0 where path = \""+path+"\"";
                     qry->exec(qrystr);


                    }
                }

            }
        }


    }



}

void player::calculate_playlist_total_time(){


    int ch = 0;
    int cm = 0;
    int cs = 0;

    if(ui->playlist->count()==0){
        QString finalTimeString = "Total time of the playlist: 00:00:00";

        ui->txt_playlistTotalTime->setText(finalTimeString);
    }

    for(int i=0;i<ui->playlist->count();i++){

        QString musica = ui->playlist->item(i)->text();

        QProcess cmd;
        QString cmdtmpstr = "exiftool \""+musica+"\" | grep Duration";
        cmd.start("sh",QStringList()<<"-c"<<cmdtmpstr);
        cmd.waitForFinished();
        QString cmdOut = cmd.readAll();
        qDebug()<<"Output of exiftool: "<<cmdOut;
        cmd.close();

        QStringList arraycmd = cmdOut.split(" ");
        if(arraycmd.count()>25){
            QStringList splitarray = arraycmd[25].split("\n");
            qDebug()<<"Total track time is: "<<splitarray[0];

            QStringList hmsArray = splitarray[0].split(":");

            if(hmsArray.count()>=2){

                qDebug()<<"H: "<<hmsArray[0];
                 qDebug()<<"M: "<<hmsArray[1];
                  qDebug()<<"S: "<<hmsArray[2];

                  int h = hmsArray[0].toInt();
                  int m = hmsArray[1].toInt();
                  int s = hmsArray[2].toInt();

                  int fh = ch+h;
                  int fm = cm+m;
                  int fs = cs+s;

                  while(fs>=60){

                      fm+=1;
                      fs-=60;

                  }

                while(fm>=60){

                    fh+=1;
                    fm-=60;

                }

                QString sfs,sfm,sfh;


                if(fs<10){
                    sfs = "0"+QString::number(fs);
                } else {
                    sfs = QString::number(fs);
                }

                if(fm<10){
                    sfm = "0"+QString::number(fm);
                } else {
                    sfm = QString::number(fm);
                }

                if(fh<10){
                    sfh = "0"+QString::number(fh);
                } else {
                    sfh = QString::number(fh);
                }

                QString finalTimeString = "Total time of the playlist: "+sfh+":"+sfm+":"+sfs;

                ui->txt_playlistTotalTime->setText(finalTimeString);

                ch = fh;
                cm = fm;
                cs = fs;


            } else {

                hmsArray = splitarray[0].split(".");

                //qDebug()<<"H: "<<hmsArray[0];
                //qDebug()<<"M: "<<hmsArray[1];
                qDebug()<<"S: "<<hmsArray[0];

                  int h = 0;
                  int m = 0;
                  int s = hmsArray[0].toInt();

                  int fh = ch+h;
                  int fm = cm+m;
                  int fs = cs+s;


                  while(fs>=60){

                      fm+=1;
                      fs-=60;

                  }



                while(fm>=60){

                    fh+=1;
                    fm-=60;

                }

                QString sfs,sfm,sfh;


                if(fs<10){
                    sfs = "0"+QString::number(fs);
                } else {
                    sfs = QString::number(fs);
                }

                if(fm<10){
                    sfm = "0"+QString::number(fm);
                } else {
                    sfm = QString::number(fm);
                }

                if(fh<10){
                    sfh = "0"+QString::number(fh);
                } else {
                    sfh = QString::number(fh);
                }

                QString finalTimeString = "Total time of the playlist: "+sfh+":"+sfm+":"+sfs;

                ui->txt_playlistTotalTime->setText(finalTimeString);

                ch = fh;
                cm = fm;
                cs = fs;
            }




        } else {
            qDebug()<<"-------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>     !!!!!!!!    An exception happend !? ... outputing details of this track: ";
            for(int i=0;i<arraycmd.count();i++){
                qDebug()<< "The array position "<<i<<" has: "<<arraycmd[i];
            }
        }


    }



}

void player::on_actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation_triggered()
{


    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: Run a check on all the records in the music table; For each check if it exist AND DELETE WITHOUT ASKING FOR CONFIRMATION and update the time info of the song",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){



        QSqlQuery qr;
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"Processing: "<<path;

                bool ha = QFile::exists(path);
                if(!ha){


                    qDebug()<<"Deleting "<<path;

                          QSqlQuery* qry=new QSqlQuery();
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

                              QSqlQuery* qry=new QSqlQuery();
                              qry->prepare("delete from musics where path = :thpath");
                              qry->bindValue(":thpath",path);

                             if(qry->exec()){
                                  qDebug() << "Music Deleted from database and HD! last query was:"<< qry->lastQuery();

                                  QProcess rmthis;
                                  QString rmthistr = "rm "+path;
                                  rmthis.start("sh",QStringList()<<"-c"<<rmthistr);
                                  rmthis.waitForFinished(-1);



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


    QString c = "curl [ip]/XFB/Config/ftpupdate.txt";
    QProcess pc;
    pc.start("sh",QStringList()<<"-c"<<c);
    pc.waitForFinished();

    QString cOut = pc.readAll();

    QStringList ServerIP = cOut.split("\n");

    qDebug()<<"Server's IP is now:"<<ServerIP[0];




    QString radio1str = "mplayer -playlist http://"+ServerIP[0]+":8000/stream.m3u";

    qDebug()<<"Full cmd is: "<<radio1str;

    radio1.start("sh",QStringList()<<"-c"<<radio1str);
    radio1.waitForStarted(-1);
    radio1.closeReadChannel(QProcess::StandardOutput);
    radio1.closeReadChannel(QProcess::StandardError);


    ui->bt_rol_streaming_play->setStyleSheet("background-color:#C8EE72");

}

void player::on_bt_rol_streaming_stop_clicked()
{
    radio1.close();
    radio1.kill();
    ui->bt_rol_streaming_play->setStyleSheet("");
}

void player::on_lp_1_bt_play_clicked()
{
    ui->lp_1_bt_play->setDisabled(true);

    lp1_Xplaylist->removeMedia(0);
    lp1_Xplaylist->addMedia(QUrl::fromLocalFile(ui->lp_1_txt_file->text()));
    lp1_Xplayer->play();


    movie = new QMovie(":/images/lp_anim1.gif");
    ui->lp_1->setMovie(movie);
    movie->start();


}

void player::on_pushButton_clicked()
{
    lp1_Xplayer->stop();
    ui->lp_1_bt_play->setDisabled(false);
    ui->lp_1->setPixmap(QPixmap(":/images/lp_player_p0.png"));
    movie->stop();
    lp_1_paused = false;
    ui->lp_1_bt_pause->setStyleSheet("");
}

void player::on_lp_1_bt_play_2_clicked()
{
    ui->lp_1_bt_play_2->setDisabled(true);
    ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p1.png"));
    lp2_Xplaylist->removeMedia(0);
    lp2_Xplaylist->addMedia(QUrl::fromLocalFile(ui->lp_2_txt_file->text()));
    lp2_Xplayer->play();

    movie2 = new QMovie(":/images/lp_anim1.gif");
    ui->lp_2->setMovie(movie2);
    movie2->start();



}

void player::on_pushButton_2_clicked()
{
    lp2_Xplayer->stop();
    ui->lp_1_bt_play_2->setDisabled(false);
    ui->lp_2->setPixmap(QPixmap(":/images/lp_player_p0.png"));
    movie2->stop();
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
    QProcess snd;
    QString sndcmd = "soundconverter";
    snd.startDetached("sh",QStringList()<<"-c"<<sndcmd);


}


void player::on_actionAutoTrim_the_silence_from_the_start_and_the_end_of_all_music_tracks_in_the_database_triggered()
{

    movie = new QMovie(":/images/loading.gif");
    ui->txt_loading->setMovie(movie);
    ui->txt_loading->setScaledContents(true);

    ui->txt_loading->show();

    movie->start();


    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: Trim the silence below 0.1% of sound from the start and the end of every music in the database?",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){




        QSqlQuery qr;
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"AutoTrim Processing: "<<path;

                QFileInfo musicfile(path);
                QString fileName(musicfile.fileName());

                QString dest = "../tmp/"+fileName;



                    QProcess sox;
                    QString soxcmd = "sox \""+path+"\" \""+dest+"\" silence 1 0.1 1%";
                    qDebug()<<"Running cmd: "<<soxcmd;
                    sox.start("sh",QStringList()<<"-c"<<soxcmd);
                    sox.waitForFinished(-1);
                    //qDebug()<<"Sox is done with this file..";

                    QProcess Fmv;
                    QString Fmvcmd = "mv \""+dest+"\" \""+path+"\"";
                    Fmv.start("sh",QStringList()<<"-c"<<Fmvcmd);
                    Fmv.waitForFinished(-1);
                    qDebug()<<"File Trimmed!";







            }
        }




    }

    movie->stop();
    ui->txt_loading->hide();


}

void player::on_actionUpdate_System_triggered()
{
    checkForUpdates();
}

void player::on_bt_apply_multi_selection_clicked()
{

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




                               QString qry = "delete from musics where path='"+thisfilename+"'";
                               QSqlQuery sql;

                               if(sql.exec(qry)){
                                   qDebug()<<"Track removed from the database: "<<thisfilename;
                               } else {
                                   QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                                   qDebug() << "last sql: " << sql.lastQuery();
                               }


                               if(rm==QMessageBox::Yes){


                                   qDebug()<<"Deleting file from HD: "<<thisfilename;
                                   QProcess sh;
                                   sh.start("sh",QStringList()<<"-c"<<"rm \""+thisfilename+"\"");
                                   sh.waitForFinished(-1);

                                   qDebug()<<"File DELETED!";


                               }







                           }

                       }


                       update_music_table();





                   }
                  }










   }



   if(accao==3){

       //convert 2 mp3

       qDebug()<<"running 3---";

       QModelIndexList indexlist = ui->musicView->selectionModel()->selectedIndexes();
       int row = -1;
       foreach(QModelIndex index, indexlist){
          // qDebug()<<"index.row(): "<<index.row()<<" ;; var row: "<<row;
           if(index.row()!=row){
               row = index.row();

               QString thisfilename = ui->musicView->model()->data(ui->musicView->model()->index(row,7)).toString(); //file path
               qDebug()<<"File to convert: "<<thisfilename;

               QProcess runcmd;
               QFileInfo fileinfo = thisfilename;
               QString filewoext = fileinfo.baseName();
               QString filepath = fileinfo.absolutePath();

               QString cmd = "ffmpeg -i \""+thisfilename+"\" -vn -ar 44100 -ac 2 -ab 192k -f mp3 \"../tmp/"+filewoext+".mp3\"";


               qDebug()<<"base name: "<<filewoext;
               qDebug()<<"file path: "<<filepath;
               qDebug()<<"ffmpeg cmd string: "<<cmd;



               runcmd.start("sh",QStringList()<<"-c"<<cmd);
               runcmd.waitForFinished(-1);

               qDebug()<<"File converted to tmp folder..";

               cmd = "rm \""+thisfilename+"\"";
               runcmd.start("sh",QStringList()<<"-c"<<cmd);
               runcmd.waitForFinished(-1);

               qDebug()<<"Original file deteled..";

               cmd = "mv \"../tmp/"+filewoext+".mp3\" \""+filepath+"/"+filewoext+".mp3\"";
               runcmd.start("sh",QStringList()<<"-c"<<cmd);
               runcmd.waitForFinished(-1);

               qDebug()<<"Converted file moved to the folder of the original file..";


               QSqlQuery *q = new QSqlQuery();
               QString qrs = "update musics set path = \""+filepath+"/"+filewoext+".mp3\" where path = \""+thisfilename+"\"";

               if(q->exec(qrs)){
                   qDebug()<<"Path updated in database records!";
                   update_music_table();
               } else {
                   qDebug()<<"Error updating path in the database:";
                   qDebug()<<q->lastError();
                   qDebug()<<"The command executed was:";
                   qDebug()<<q->lastQuery();
               }


               QMessageBox::information(this,tr("Done"),tr("File(s) converted!"));

           }

       }


   }




   if(accao==4){

       //convert 2 mp3

       qDebug()<<"running 4---";

       QModelIndexList indexlist = ui->musicView->selectionModel()->selectedIndexes();
       int row = -1;
       foreach(QModelIndex index, indexlist){
           if(index.row()!=row){
               row = index.row();

               QString thisfilename = ui->musicView->model()->data(ui->musicView->model()->index(row,7)).toString(); //file path
               qDebug()<<"File to convert: "<<thisfilename;

               QProcess runcmd;
               QFileInfo fileinfo = thisfilename;
               QString filewoext = fileinfo.baseName();
               QString filepath = fileinfo.absolutePath();

               QString cmd = "ffmpeg -i \""+thisfilename+"\" -c:a libvorbis -qscale:a 7 \"../tmp/"+filewoext+".ogg\"";



               qDebug()<<"base name: "<<filewoext;
               qDebug()<<"file path: "<<filepath;
               qDebug()<<"ffmpeg cmd string: "<<cmd;



               runcmd.start("sh",QStringList()<<"-c"<<cmd);
               runcmd.waitForFinished(-1);

               qDebug()<<"File converted to tmp folder..";

               cmd = "rm \""+thisfilename+"\"";
               runcmd.start("sh",QStringList()<<"-c"<<cmd);
               runcmd.waitForFinished(-1);

               qDebug()<<"Original file deteled..";

               cmd = "mv \"../tmp/"+filewoext+".ogg\" \""+filepath+"/"+filewoext+".ogg\"";
               runcmd.start("sh",QStringList()<<"-c"<<cmd);
               runcmd.waitForFinished(-1);

               qDebug()<<"Converted file moved to the folder of the original file..";


               QSqlQuery *q = new QSqlQuery();
               QString qrs = "update musics set path = '"+filepath+"/"+filewoext+".ogg' where path = '"+thisfilename+"'";

               if(q->exec(qrs)){
                   qDebug()<<"Path updated in database recordsposition!";
                   update_music_table();
               } else {
                   qDebug()<<"Error updating path in the database:";
                   qDebug()<<q->lastError();
                   qDebug()<<"The command executed was:";
                   qDebug()<<q->lastQuery();
               }


               QMessageBox::information(this,tr("Done"),tr("File(s) converted!"));

           }

       }


   }



   if(accao==5){

       //trim

       QModelIndexList indexlist = ui->musicView->selectionModel()->selectedIndexes();
       int row;
       foreach(QModelIndex index, indexlist){
           if(index.row()!=row){
               row = index.row();
               //qDebug()<<"This row is selected: "<<row;

               QString path = ui->musicView->model()->data(ui->musicView->model()->index(row,7)).toString();

               qDebug()<<"AutoTrim Processing: "<<path;

               QFileInfo musicfile(path);
               QString fileName(musicfile.fileName());

               QString dest = "../tmp/"+fileName;



                   QProcess sox;
                   QString soxcmd = "sox \""+path+"\" \""+dest+"\" silence 1 0.1 1%";
                   qDebug()<<"Running cmd: "<<soxcmd;
                   sox.start("sh",QStringList()<<"-c"<<soxcmd);
                   sox.waitForFinished(-1);
                   //qDebug()<<"Sox is done with this file..";

                   QProcess Fmv;
                   QString Fmvcmd = "mv \""+dest+"\" \""+path+"\"";
                   Fmv.start("sh",QStringList()<<"-c"<<Fmvcmd);
                   Fmv.waitForFinished(-1);
                   qDebug()<<"File Trimmed!";






           }

       }


   }



   if(accao==6){

       //trim eXtreme 1

       QModelIndexList indexlist = ui->musicView->selectionModel()->selectedIndexes();
       int row;
       foreach(QModelIndex index, indexlist){
           if(index.row()!=row){
               row = index.row();
               //qDebug()<<"This row is selected: "<<row;

               QString path = ui->musicView->model()->data(ui->musicView->model()->index(row,7)).toString();

               qDebug()<<"AutoTrim Processing: "<<path;

               QFileInfo musicfile(path);
               QString fileName(musicfile.fileName());

               QString dest = "../tmp/"+fileName;



                   QProcess sox;
                   QString soxcmd = "sox \""+path+"\" \""+dest+"\" silence 1 0.2 1%";
                   qDebug()<<"Running cmd: "<<soxcmd;
                   sox.start("sh",QStringList()<<"-c"<<soxcmd);
                   sox.waitForFinished(-1);
                   //qDebug()<<"Sox is done with this file..";

                   QProcess Fmv;
                   QString Fmvcmd = "mv \""+dest+"\" \""+path+"\"";
                   Fmv.start("sh",QStringList()<<"-c"<<Fmvcmd);
                   Fmv.waitForFinished(-1);
                   qDebug()<<"File Trimmed!";






           }

       }


   }





}

void player::on_actionConvert_all_musics_in_the_database_to_mp3_triggered()
{

    movie = new QMovie(":/images/loading.gif");
    ui->txt_loading->setMovie(movie);
    ui->txt_loading->setScaledContents(true);

    ui->txt_loading->show();

    movie->start();


    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: convert all the musics in the database to MP3 (Defaults: Stereo 192kbs 44100)?",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){




        QSqlQuery qr;
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"AutoTrim Processing: "<<path;

                QFileInfo musicfile(path);
                QString fileName(musicfile.baseName());
                QString filepath(musicfile.absolutePath());



                QProcess runcmd;

                QString cmd = "ffmpeg -i \""+path+"\" -vn -ar 44100 -ac 2 -ab 192k -f mp3 \"../tmp/"+fileName+".mp3\"";


                qDebug()<<"base name: "<<fileName;
                qDebug()<<"file path: "<<filepath;
                qDebug()<<"ffmpeg cmd string: "<<cmd;



                runcmd.start("sh",QStringList()<<"-c"<<cmd);
                runcmd.waitForFinished(-1);

                qDebug()<<"File converted to tmp folder..";

                cmd = "rm \""+path+"\"";
                runcmd.start("sh",QStringList()<<"-c"<<cmd);
                runcmd.waitForFinished(-1);

                qDebug()<<"Original file deteled..";

                cmd = "mv \"../tmp/"+fileName+".mp3\" \""+filepath+"/"+fileName+".mp3\"";
                runcmd.start("sh",QStringList()<<"-c"<<cmd);
                runcmd.waitForFinished(-1);

                qDebug()<<"Converted file moved to the folder of the original file..";


                QSqlQuery *q = new QSqlQuery();
                QString qrs = "update musics set path = '"+filepath+"/"+fileName+".mp3' where path = '"+path+"'";

                if(q->exec(qrs)){
                    qDebug()<<"Path updated in database records!";
                    update_music_table();
                } else {
                    qDebug()<<"Error updating path in the database:";
                    qDebug()<<q->lastError();
                    qDebug()<<"The command executed was:";
                    qDebug()<<q->lastQuery();
                }





            }
        }




    }


    QMessageBox::information(this,tr("Done"),tr("File(s) converted!"));

    movie->stop();
    ui->txt_loading->hide();


}

void player::on_actionConvert_all_musics_in_the_database_to_ogg_triggered()
{

    movie = new QMovie(":/images/loading.gif");
    ui->txt_loading->setMovie(movie);
    ui->txt_loading->setScaledContents(true);

    ui->txt_loading->show();

    movie->start();


    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: convert all the musics in the database to OGG (Defaults: Stereo ScaleQuality 7)?",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){




        QSqlQuery qr;
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"AutoTrim Processing: "<<path;

                QFileInfo musicfile(path);
                QString fileName(musicfile.baseName());
                QString filepath(musicfile.absolutePath());



                QProcess runcmd;

                QString cmd = "ffmpeg -i \""+path+"\" -c:a libvorbis -qscale:a 7 \"../tmp/"+fileName+".ogg\"";


                qDebug()<<"base name: "<<fileName;
                qDebug()<<"file path: "<<filepath;
                qDebug()<<"ffmpeg cmd string: "<<cmd;



                runcmd.start("sh",QStringList()<<"-c"<<cmd);
                runcmd.waitForFinished(-1);

                qDebug()<<"File converted to tmp folder..";

                cmd = "rm \""+path+"\"";
                runcmd.start("sh",QStringList()<<"-c"<<cmd);
                runcmd.waitForFinished(-1);

                qDebug()<<"Original file deteled..";

                cmd = "mv \"../tmp/"+fileName+".ogg\" \""+filepath+"/"+fileName+".ogg\"";
                runcmd.start("sh",QStringList()<<"-c"<<cmd);
                runcmd.waitForFinished(-1);

                qDebug()<<"Converted file moved to the folder of the original file..";


                QSqlQuery *q = new QSqlQuery();
                QString qrs = "update musics set path = '"+filepath+"/"+fileName+".ogg' where path = '"+path+"'";

                if(q->exec(qrs)){
                    qDebug()<<"Path updated in database records!";
                    update_music_table();
                } else {
                    qDebug()<<"Error updating path in the database:";
                    qDebug()<<q->lastError();
                    qDebug()<<"The command executed was:";
                    qDebug()<<q->lastQuery();
                }





            }
        }




    }


    QMessageBox::information(this,tr("Done"),tr("File(s) converted!"));

    movie->stop();
    ui->txt_loading->hide();


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
    stimer->stop();
    icetimer->stop();
    butt_timer->stop();



    QProcess kb;
    kb.startDetached("sh",QStringList()<<"-c"<<"killall icecast");
    kb.waitForFinished();
    ui->lbl_icecast->setText("Stopped");
    ui->lbl_icecast->setStyleSheet("color:blue");
    ui->bt_icecast->setStyleSheet("");

    kb.start("sh",QStringList()<<"-c"<<"killall butt");
    kb.waitForFinished();
    ui->lbl_butt->setText("Stopped");
    ui->lbl_butt->setStyleSheet("color:blue");
    ui->bt_butt->setStyleSheet("");

    ui->lbl_port->setText("Stopped");
    ui->lbl_port->setStyleSheet("color:blue");

    ui->bt_takeOver->setStyleSheet("");
    ui->bt_takeOver->setEnabled(false);

    ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
    ui->txt_ProgramName->hide();

    piscaLive = false;

}


void player::streaming_timmer(){

    qDebug()<<"Running streaming_timmer (checking external processes...)";


    QProcess chk;
    chk.start("sh",QStringList()<<"-c"<<"ps -e | grep icecast");
    chk.waitForFinished();
    QString found = chk.readAll();

    if(found!=""){
        ui->lbl_icecast->setText("Running");
        ui->lbl_icecast->setStyleSheet("color:green");
    } else {
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:red");
         ui->bt_takeOver->setEnabled(false);
    }


    chk.start("sh",QStringList()<<"-c"<<"ps -e | grep butt");
    chk.waitForFinished();
    found = chk.readAll();

    if(found!=""){
        ui->lbl_butt->setText("Running");
        ui->lbl_butt->setStyleSheet("color:green");
    } else {
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:red");
        ui->bt_takeOver->setEnabled(false);
    }




}

void player::ddnsUpdate(){
    QProcess cmd;
    cmd.start("sh",QStringList()<<"-c"<<"curl ipv4.icanhazip.com");
    cmd.waitForFinished();
    QString cmdout = cmd.readAll().trimmed();

    qDebug()<<"The current external ip determined was: "<<cmdout;

    ui->lbl_ddns->setText(cmdout);







}

void player::on_horizontalSlider_lps_vol_sliderMoved(int position)
{
    qDebug()<<"LPS Horisontal Slider moved to: "<<position;


    if(position==100){
        lp1_Xplayer->setVolume(100);
        lp2_Xplayer->setVolume(100);

        ui->lbl_lp1_vol_level->setText("100");
        ui->lbl_lp2_vol_level->setText("100");
    }


    if(position<=99){

        lp1_Xplayer->setVolume(100);
        lp2_Xplayer->setVolume(position);

        ui->lbl_lp1_vol_level->setText("100");

        QString np = QString::number(position);

        ui->lbl_lp2_vol_level->setText(np);


    }

    if(position>=101){

        lp2_Xplayer->setVolume(100);

        int lp1_volume = 200-position;
        lp1_Xplayer->setVolume(lp1_volume);

        QString lp1_lbl_vol = QString::number(lp1_volume);
        ui->lbl_lp1_vol_level->setText(lp1_lbl_vol);

        ui->lbl_lp2_vol_level->setText("100");


    }

}

void player::on_bt_center_lps_vol_clicked()
{
    lp1_Xplayer->setVolume(100);
    lp2_Xplayer->setVolume(100);

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
    model->setQuery("select * from musics where artist like '%"+term+"%' or song like '%"+term+"%'");
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

    QFile settings_ftp("~/.netrc");

    QTextStream outF(&settings_ftp);

    QString c2 = "curl "+Server_URL+"/XFB/Config/ftpupdate.txt";
    qDebug()<<"Full cmd is: "<<c2;
    QProcess pc2;
    pc2.start("sh",QStringList()<<"-c"<<c2);
    pc2.waitForFinished();

    QString cOut2 = pc2.readAll();

    QStringList ServerIP = cOut2.split("\n");

    qDebug()<<"Server's IP is now:"<<ServerIP[0];


    outF<<"machine "<<ServerIP[0]<<" login "<<User<<" password "<<Pass<<"\n";


    settings_ftp.close();
}

void player::on_bt_add_some_random_songs_from_genre_clicked()
{

    QString selectedGenre = ui->comboBox_random_add_genre->currentText();


    QString num = ui->spinBox_num_of_songs_to_add_random->text();

    QString querystr = "select path from musics where genre1 like '"+selectedGenre+"' order by random() limit "+num;
    QSqlQuery query;
    if(query.exec(querystr))
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

    if(icecastrunning==false){
        QProcess processo;

        qDebug()<<"Forcing all instences of icecast to close...";
        processo.start("sh", QStringList()<<"-c"<<"killall icecast");
        processo.waitForFinished();

        qDebug()<<"Starting icecast...";
        processo.startDetached("sh",QStringList()<<"-c"<<"xvfb-run -a icecast -c /usr/local/etc/icecast.xml");

        processo.waitForStarted();

        qDebug()<<"icecast started!";

        icecastrunning = true;

        ui->bt_icecast->setStyleSheet("background-color:#C8EE72");

        connect(icetimer, SIGNAL(timeout()), this, SLOT(ice_timmer()));
        icetimer->start(5000);
        ice_timmer();
    } else {
        icetimer->stop();
        QProcess processo;

        qDebug()<<"Forcing all instences of icecast to close...";
        processo.start("sh", QStringList()<<"-c"<<"killall icecast");
        processo.waitForFinished();

         icecastrunning = false;
         ui->bt_icecast->setStyleSheet("");
         ui->lbl_icecast->setText("Stopped");
         ui->lbl_icecast->setStyleSheet("color:blue");

         ui->bt_takeOver->setStyleSheet("");
         ui->bt_takeOver->setEnabled(false);

         ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
         ui->txt_ProgramName->hide();

         piscaLive = false;
    }



}

void player::ice_timmer(){

    QProcess chk;
    chk.start("sh",QStringList()<<"-c"<<"ps -e | grep icecast");
    chk.waitForFinished();
    QString found = chk.readAll();

    if(found!=""){
        ui->lbl_icecast->setText("Running");
        ui->lbl_icecast->setStyleSheet("color:green");
    } else {
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:red");

        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);

        ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
        ui->txt_ProgramName->hide();

        piscaLive = false;
    }


}

void player::on_bt_butt_clicked()
{

    if(buttrunning==false){
        QProcess processo;

        qDebug()<<"Forcing all instences of butt to close...";
        processo.start("sh", QStringList()<<"-c"<<"killall butt");
        processo.waitForFinished();

        qDebug()<<"Starting butt...";
        processo.startDetached("sh",QStringList()<<"-c"<<"xvfb-run -a butt");

        processo.waitForStarted();

        qDebug()<<"butt started!";

        buttrunning = true;

        ui->bt_butt->setStyleSheet("background-color:#C8EE72");

        connect(butt_timer, SIGNAL(timeout()), this, SLOT(butt_timmer()));
        butt_timer->start(5000);
        butt_timmer();
    } else {
        butt_timer->stop();
        QProcess processo;

        qDebug()<<"Forcing all instences of icecast to close...";
        processo.start("sh", QStringList()<<"-c"<<"killall icecast");
        processo.waitForFinished();

         buttrunning = false;
         ui->bt_butt->setStyleSheet("");
         ui->lbl_butt->setText("Stopped");
         ui->lbl_butt->setStyleSheet("color:blue");

         ui->bt_takeOver->setStyleSheet("");
         ui->bt_takeOver->setEnabled(false);

         ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
         ui->txt_ProgramName->hide();

         piscaLive = false;
    }



}
void player::butt_timmer(){

    QProcess chk;
    chk.start("sh",QStringList()<<"-c"<<"ps -e | grep butt");
    chk.waitForFinished();
    QString found = chk.readAll();

    if(found!=""){
        ui->lbl_butt->setText("Running");
        ui->lbl_butt->setStyleSheet("color:green");
    } else {
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:red");

    }


}

void player::on_bt_ddns_clicked()
{
    ddnsUpdate();
}

void player::on_bt_portTest_clicked()
{
    ui->lbl_port->setText("Checking...");
    ui->lbl_port->setStyleSheet("color:#FBEA23");


//
    QString ptest = "ss -tl4 '( sport = :8888 )' | grep LISTEN";
    QString nctest = "nc -z "+ui->lbl_ddns->text()+" 8888; echo $?";


    qDebug()<<"The netcat command to check the port is: "<<ptest;


    QProcess cmd;
    cmd.start("sh",QStringList()<<"-c"<<ptest);
    cmd.waitForFinished();
    QString cmdout = cmd.readAll().trimmed();

    cmd.start("sh",QStringList()<<"-c"<<nctest);
    cmd.waitForFinished();
    QString ncmdout = cmd.readAll().trimmed();

    qDebug()<<"ss output: "<<cmdout;
    qDebug()<<"nc output: "<<ncmdout;

    if(cmdout != ""){

        ui->lbl_port->setText("OPEN IN");
        ui->lbl_port->setStyleSheet("color:yellow");

        if(ncmdout != ""){

            ui->lbl_port->setText("OPEN IN + OUT");
            ui->lbl_port->setStyleSheet("color:green");

            QString lbl_streamURL = "<a href=\"http://"+ui->lbl_ddns->text()+":8888/stream.m3u\">http://"+ui->lbl_ddns->text()+":8888/stream.m3u</a>";

            ui->lbl_streamURL->setText(lbl_streamURL);
            ui->lbl_streamURL->setTextFormat(Qt::RichText);
            ui->lbl_streamURL->setTextInteractionFlags(Qt::TextBrowserInteraction);
            ui->lbl_streamURL->setOpenExternalLinks(false);

            ui->bt_takeOver->setEnabled(true);
        } else {
            QMessageBox::information(this,tr("Error"),tr("It was not possible to determine if port 8888 is open/forwarded :-( \n"
                                                         "Most probable cause is that the port is NOT OPEN / NOT FORWARDED in the Router or firewall.\n"
                                                         "This can also be due to the dependencie with the netcat command\n"
                                                         "(try running the nc command). Or an error with the network?"));
            ui->lbl_port->setText("ERROR");
            ui->lbl_port->setStyleSheet("color:red");
        }



    } else {
        QMessageBox::information(this,tr("Error"),tr("It was not possible to determine if port 8888 is LISTENING :-( \n"
                                                     "Most probable cause is that the port is NOT OPEN in the firewall.\n"
                                                     "This can also be due to the dependencie with the netcat command\n"
                                                     "(try running the nc command). Or an error with the network?"));
        ui->lbl_port->setText("ERROR");
        ui->lbl_port->setStyleSheet("color:red");
    }


}

void player::on_bt_takeOver_clicked()
{
    ui->bt_takeOver->setStyleSheet("background-color:yellow");

    QString rmthis = "/usr/share/xfb/ftp/*.xml";
    QFile::remove(rmthis);

    rmthis = "/usr/share/xfb/TakeOver/*.xml";
    QFile::remove(rmthis);


    //just to be sure ...

    rmthis = TakeOverPath+"/*.xml";
    QFile::remove(rmthis);

    rmthis = FTPPath+"/*.xml";
    QFile::remove(rmthis);



    QString takeOverFile = "/usr/share/xfb/ftp/takeover.xml";
    QFile file(takeOverFile);

    if(takeOver == false){

        takeOver = true;

        file.open(QIODevice::WriteOnly);
        QXmlStreamWriter xmlWriter(&file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("XFBClientTakeOver");
        xmlWriter.writeStartElement("www.netpack.pt");

        QString ip = ui->lbl_ddns->text();
        QString stream = "http://"+ip+":8888/stream.m3u";



        xmlWriter.writeTextElement("stream",stream);
        xmlWriter.writeTextElement("ip",ip);


        xmlWriter.writeEndElement();
        xmlWriter.writeEndDocument();
        file.close();








        qDebug()<<"Sending takeOver to server. This requires ~/.netrc to be configured with the ftp options and FTP Path in the options to point to a folder called 'ftp' that MUST be located in the parent directory of XFB (due to the code of config/serverFtpCmdsPutTakeOver.sh).";


        QProcess sh,sh2;
        QByteArray output, output2;
        QString outPath, FTPCmdPath, xmls;


        sh.start("sh", QStringList() << "-c" << "pwd");
        sh.waitForFinished();
        output = sh.readAll();
        outPath = output;
        QStringList path_arry = outPath.split("\n");
        FTPCmdPath = path_arry[0]+"/usr/share/xfb/scripts/serverFtpCmdsPutTakeOver.sh | grep 'Transfer complete'";
        qDebug() << "running: " << FTPCmdPath;
        qDebug() << "If you get errors: cd config && chmod +x serverFtpCmdsPutTakeOver.sh && chmod 600 ~/.netrc (the ftp is configured in .netrc correct?)";
        sh.close();



        sh2.start("sh", QStringList() << "-c" << FTPCmdPath);
        sh2.waitForFinished(-1);
        output2 = sh2.readAll().trimmed();
        xmls = output2;
        qDebug()<<"The output of serverFtpCmdsPutTakeOver.sh is:\n"<<output2;
        sh2.close();



        if(output2 == "226 Transfer complete."){

            ui->bt_takeOver->setStyleSheet("background-color:blue");
            ui->bt_takeOver->setText(tr("Connecting..."));

            QFile::remove("/usr/share/xfb/ftp/takeover.xml");

            QTimer::singleShot(5000, this, SLOT(checkTakeOver()));


        }










    } else {
        takeOver = false;
        returnTakeOver();
        QFile::remove(takeOverFile) ;
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));

        ui->txt_ProgramName->setStyleSheet("background-color:#FFE329;padding:5px;");
        ui->txt_ProgramName->hide();

        piscaLive = false;


    }



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


void player::checkTakeOver(){
    qDebug()<<"Checking the TakeOver...";

    QProcess sh;
    QString shcmd = "/usr/share/xfb/scripts/serverFtpCmdsCHKTakeOver.sh | grep confirmtakeover.xml";
    sh.start("sh",QStringList()<<"-c"<<shcmd);
    sh.waitForFinished();
    QString shout = sh.readAll().trimmed();
    qDebug()<<"The output of: "<<shcmd<<":\n"<<shout;

    if(shout!=""){

        QStringList shoutarray = shout.split(".");

        if(shoutarray.count()>=1){
            if(shoutarray[1]=="xml"){

                qDebug()<<"      -------------------         [TakeOver CONFIRMED]      ---------------------       ";

                ui->bt_takeOver->setStyleSheet("background-color:green");
                ui->bt_takeOver->setText(tr("BROADCASTING LIVE!!!"));


                ui->txt_ProgramName->setText(tr("BROADCASTING LIVE!!!"));
                ui->txt_ProgramName->setStyleSheet("background-color:red;color:#FFF;text-align:center !important;font-size:28px;font-weight:bolder;");
                ui->txt_ProgramName->setAlignment(Qt::AlignHCenter);
                ui->txt_ProgramName->show();

                if(piscaLive==false){
                    piscaLive=true;
                    livePiscaStart();
                }



            } else {

                qDebug()<<"      -------------------         [TakeOver FAILD :: shoutarray[1] is not 'xml']         ---------------------       \n"
                          "      -------------------     [Trying again in 10 sec]     ---------------------";
                QTimer::singleShot(10000, this, SLOT(checkTakeOver()));

            }
        } else {
            qDebug()<<"      -------------------         [TakeOver FAILD :: shoutarray.count is less than 1]         ---------------------       \n"
                      "      -------------------     [Trying again in 10 sec]     ---------------------";
            QTimer::singleShot(10000, this, SLOT(checkTakeOver()));

        }



    } else {
        qDebug()<<"      -------------------         [TakeOver FAILD :: shout is empty]         ---------------------       \n"
                  "      -------------------     [Trying again in 10 sec]     ---------------------";
        QTimer::singleShot(10000, this, SLOT(checkTakeOver()));

    }

}

void player::MainsetVol100(){
    Xplayer->setVolume(100);
}

void player::MainsetVol80(){
    Xplayer->setVolume(80);
}
void player::MainsetVol60(){
    Xplayer->setVolume(60);
}
void player::MainsetVol40(){
    Xplayer->setVolume(40);
}
void player::MainsetVol20(){
    Xplayer->setVolume(20);
}
void player::MainsetVol10(){
    Xplayer->setVolume(10);
}
void player::MainsetVol5(){
    Xplayer->setVolume(5);
}
void player::MainStop(){
    Xplayer->stop();
    ui->btPlay->setStyleSheet("");
    ui->btPlay->setText(tr("Play"));
    PlayMode = "stopped";
}

void player::pingTakeOverClient(){

    qDebug()<<"Pinging the TakeOver Client ->| "<<takeOverIP;

    QProcess ping;
    QString pingcmd = "ping -c 1 "+takeOverIP+" | grep '1 received'";

    ping.start("sh",QStringList()<<"-c"<<pingcmd);
    ping.waitForFinished(8000);

    QString cmdout = ping.readAll();

    QStringList pingArray = cmdout.split(",");

    if(pingArray[1]==" 1 received"){

        qDebug()<<"TakeOver Client pinged OK!! |<- "<<takeOverIP;

    } else {
        qDebug()<<"ERROR pinging TakeOver Client!!\nKilling all streams and starting to play localy..";

        //stop mplayer and start xplayer

        QProcess kb;
        kb.startDetached("sh",QStringList()<<"-c"<<"killall mplayer");

        on_btPlay_clicked();

        QTimer::singleShot(3500, this, SLOT(MainsetVol100()));
        QTimer::singleShot(3000, this, SLOT(MainsetVol80()));
        QTimer::singleShot(2500, this, SLOT(MainsetVol60()));
        QTimer::singleShot(2000, this, SLOT(MainsetVol40()));
        QTimer::singleShot(1500, this, SLOT(MainsetVol20()));
        QTimer::singleShot(1000, this, SLOT(MainsetVol10()));
        QTimer::singleShot(500, this, SLOT(MainsetVol5()));




        QTimer::singleShot(30000,this,SLOT(recoveryStreamTakeOverPlay()));

    }


}


void player::recoveryStreamTakeOverPlay(){

    qDebug()<<"Connection lost. Recovery routines running...";

    stopMplayer();

    radio1str = "mplayer -volume 100 -playlist "+takeOverStream;

    qDebug()<<"Full cmd is: "<<radio1str;

    radio1.start("sh",QStringList()<<"-c"<<radio1str);
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


}

void player::on_bt_pause_rec_clicked()
{

    if(recPause==false){
        recPause=true;
        ui->bt_pause_rec->setStyleSheet("background-color:yellow");

        audioRecorder->pause();
        recTimer->stop();

    } else {
        recPause=false;
        ui->bt_pause_rec->setStyleSheet("");
        audioRecorder->record();
        recTimer->start();
    }

}

void player::on_bt_pause_play_clicked()
{

    if(playPause==false){
        playPause=true;
        ui->bt_pause_play->setStyleSheet("background-color:yellow");

        Xplayer->pause();


    } else {
        playPause=false;
        ui->bt_pause_play->setStyleSheet("");
        Xplayer->play();

    }
}
