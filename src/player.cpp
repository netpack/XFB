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
// QMediaPlaylist removed - using QList<QUrl> instead
#include <QAudio>
#include <QDebug>
#include <QtMultimedia>
// QAudioProbe is deprecated in Qt6
#include <QTableWidgetItem>
#include <QList>
#include <QDateTime>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QSqlTableModel>
#include <QFileInfo>
// QAudioRecorder is deprecated in Qt6
#include <QMediaDevices> // Qt6 replacement for QAudioDeviceInfo
#include <QAudioInput> // Qt6 replacement for audio recording
#include <QAudioOutput> // Qt6 for audio output
#include <QNetworkAccessManager>
#include <QNetworkInformation> // Qt6 replacement for QNetworkConfigurationManager
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

#include <QtCore>
#include <QtGlobal>
#include <QSizeGrip>

#include <QtWebEngineQuick>
#include <QQuickWidget>
#include "services/ServiceContainer.h"
#include "services/AccessibilityManager.h"

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
        QPushButton* okButton = new QPushButton(tr("I'll donate if I can"), this);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    networkManager = new QNetworkAccessManager(this);
/*
    // Only needs to be done once per application run
    QtWebEngineQuick::initialize();

    adBanner = new QQuickWidget(this);
    adBanner->setFixedHeight(90);
    // Set a minimum width based on Google's requirement (or your layout needs)
    adBanner->setMinimumWidth(728);
    adBanner->setResizeMode(QQuickWidget::SizeRootObjectToView);
    adBanner->setSource(QUrl("qrc:/AdView.qml")); // Load from resource

    ui->gridLayout_2->addWidget(adBanner, 25, 0, 1, 1);

    // Keep refresh logic if needed, but call the QML function
    adRefreshTimer = new QTimer(this); // Assuming adRefreshTimer is a member
    adRefreshTimer->setInterval(60000); // Refresh less often? e.g., 60 seconds
    connect(adRefreshTimer, &QTimer::timeout, this, &player::refreshAdBanner);
    adRefreshTimer->start();*/

    updateConfig();
    checkDbOpen();
    //on_actionUpdate_Dinamic_Server_s_IP_triggered();

    // Initialize audio outputs for Qt6
    XplayerOutput = new QAudioOutput(this);
    lp1_XplayerOutput = new QAudioOutput(this);
    lp2_XplayerOutput = new QAudioOutput(this);

    // Initialize media players with audio outputs
    Xplayer = new QMediaPlayer(this);
    Xplayer->setAudioOutput(XplayerOutput);
    XplaylistIndex = 0;

    lp1_Xplayer = new QMediaPlayer(this);
    lp1_Xplayer->setAudioOutput(lp1_XplayerOutput);
    lp1_XplaylistIndex = 0;

    lp2_Xplayer = new QMediaPlayer(this);
    lp2_Xplayer->setAudioOutput(lp2_XplayerOutput);
    lp2_XplaylistIndex = 0;

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

    // Initialize Qt6 media recording components
    captureSession = new QMediaCaptureSession(this);
    audioRecorder = new QMediaRecorder(this);
    audioInput = new QAudioInput(this);

    captureSession->setRecorder(audioRecorder);
    captureSession->setAudioInput(audioInput);

    // List available audio input devices
    const QList<QAudioDevice> inputDevices = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : inputDevices) {
        qDebug() << "Audio Hardware on this system: " << device.description();
    }

    // Get supported audio codecs using mediaFormat() in Qt6
    const QList<QMediaFormat::AudioCodec> supportedCodecs = audioRecorder->mediaFormat().supportedAudioCodecs(QMediaFormat::Encode);
    for (const QMediaFormat::AudioCodec &codec : supportedCodecs) {
        qDebug() << "Audio Codecs on this system: " << QMediaFormat::audioCodecName(codec);
    }

    // Get supported containers (file formats) in Qt6
    const QList<QMediaFormat::FileFormat> supportedContainers = audioRecorder->mediaFormat().supportedFileFormats(QMediaFormat::Encode);
    for (const QMediaFormat::FileFormat &format : supportedContainers) {
        qDebug() << "Audio Containers on this system: " << QMediaFormat::fileFormatName(format);
    }

    // Get supported sample rates - in Qt6 we need to check available quality settings
    QMediaRecorder::EncodingMode mode = audioRecorder->encodingMode();
    QMediaRecorder::Quality quality = audioRecorder->quality();
    qDebug() << "Current encoding mode: " << mode << ", quality: " << quality;
    qDebug() << "Audio Sample Rates are handled through quality settings in Qt6";

    connect(Xplayer, &QMediaPlayer::positionChanged, this, &player::onPositionChanged);
    connect(Xplayer, &QMediaPlayer::durationChanged, this, &player::durationChanged);
    connect(Xplayer, &QMediaPlayer::sourceChanged, this, &player::currentMediaChanged);
    connect(Xplayer->audioOutput(), &QAudioOutput::volumeChanged, this, &player::volumeChanged);

    // Connect media player signals for playlist management
    connect(Xplayer, &QMediaPlayer::playbackStateChanged, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState && PlayMode == "Playing_Segue") {
            // When playback stops, play the next media if in segue mode
            QTimer::singleShot(100, this, &player::playNextMedia);
        }
    });

    connect(lp1_Xplayer, &QMediaPlayer::positionChanged, this, &player::lp1_onPositionChanged);
    connect(lp1_Xplayer, &QMediaPlayer::durationChanged, this, &player::lp1_durationChanged);
    connect(lp1_Xplayer, &QMediaPlayer::sourceChanged, this, &player::lp1_currentMediaChanged);
    connect(lp1_Xplayer->audioOutput(), &QAudioOutput::volumeChanged, this, &player::lp1_volumeChanged);

    connect(lp2_Xplayer, &QMediaPlayer::positionChanged, this, &player::lp2_onPositionChanged);
    connect(lp2_Xplayer, &QMediaPlayer::durationChanged, this, &player::lp2_durationChanged);
    connect(lp2_Xplayer, &QMediaPlayer::sourceChanged, this, &player::lp2_currentMediaChanged);
    connect(lp2_Xplayer->audioOutput(), &QAudioOutput::volumeChanged, this, &player::lp2_volumeChanged);

    // Initialize recTimer before connecting it
    recTimer = new QTimer(this);
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
    checkDbOpen();
    QSqlTableModel *model = new QSqlTableModel(this,db);
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
checkDbOpen();
    /*Populate jingles table with an editable table field on double-click*/
    QSqlTableModel * jinglesmodel = new QSqlTableModel(this,db);
    jinglesmodel->setTable("jingles");
    jinglesmodel->select();
    ui->jinglesView->setModel(jinglesmodel);
    ui->jinglesView->setSortingEnabled(true);
    ui->jinglesView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Pub table*/

    QSqlTableModel *pubmodel = new QSqlTableModel(this,db);
    pubmodel->setTable("pub");
    pubmodel->select();
    ui->pubView->setModel(pubmodel);
    ui->pubView->setSortingEnabled(true);
    ui->pubView->hideColumn(0);
    ui->pubView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    /*Populate Programs table*/

    QSqlTableModel *programsmodel = new QSqlTableModel(this,db);
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
     
     // Accessibility improvements for playlist
     ui->playlist->setFocusPolicy(Qt::StrongFocus);
     ui->playlist->setAttribute(Qt::WA_KeyboardFocusChange, true);
    /*Music list*/
     ui->musicView->setSelectionMode(QAbstractItemView::ExtendedSelection);
     ui->musicView->setDragEnabled(true);
     ui->musicView->viewport()->setAcceptDrops(false);
     ui->musicView->setAcceptDrops(false);
     ui->musicView->setDropIndicatorShown(true);
     ui->musicView->setDragDropMode(QAbstractItemView::DragOnly);
     ui->musicView->setSelectionBehavior(QAbstractItemView::SelectRows);
     
     // Accessibility improvements for keyboard navigation
     ui->musicView->setFocusPolicy(Qt::StrongFocus);
     ui->musicView->setTabKeyNavigation(true);
     ui->musicView->setAttribute(Qt::WA_KeyboardFocusChange, true);

     ui->musicView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->musicView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(musicViewContextMenu(const QPoint&)));

     ui->playlist->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->playlist, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(playlistContextMenu(const QPoint&)));

     // Accessibility improvements for jinglesView
     ui->jinglesView->setFocusPolicy(Qt::StrongFocus);
     ui->jinglesView->setTabKeyNavigation(true);
     ui->jinglesView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->jinglesView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->jinglesView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(jinglesViewContextMenu(const QPoint&)));

     // Accessibility improvements for pubView
     ui->pubView->setFocusPolicy(Qt::StrongFocus);
     ui->pubView->setTabKeyNavigation(true);
     ui->pubView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->pubView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->pubView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(pubViewContextMenu(const QPoint&)));

     // Accessibility improvements for programsView
     ui->programsView->setFocusPolicy(Qt::StrongFocus);
     ui->programsView->setTabKeyNavigation(true);
     ui->programsView->setAttribute(Qt::WA_KeyboardFocusChange, true);
     ui->programsView->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(ui->programsView, SIGNAL(customContextMenuRequested(const QPoint&)),
         this, SLOT(programsViewContextMenu(const QPoint&)));

     /*Populate genre1 and 2 filters*/

     QSqlQueryModel * model_genre1=new QSqlQueryModel();
     QSqlQuery* qry=new QSqlQuery(db);
     QString sqlq = "select name from genres1";
     qry->exec(sqlq);
     model_genre1->setQuery(std::move(*qry));
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

   // Directly set style on the status bar
   if(darkMode){
       qDebug("Loading darkmode");
       ui->statusBar->setStyleSheet("background-color: #353535 !important; color: #ffffff; border: none; margin: 0; padding: 0;");

   } else {
       qDebug("Loading lightmode");
       this->setStyleSheet("background-color: #ffffff");
       ui->statusBar->setStyleSheet("background-color: #ffffff !important; color: #303030; border: none; margin: 0; padding: 0;");


   }

   // Initialize accessibility features for the player interface
   initializeAccessibility();

}

void player::initializeAccessibility()
{
    try {
        // Get the accessibility manager from the service container
        auto* serviceContainer = ServiceContainer::instance();
        auto* accessibilityManager = serviceContainer->resolve<AccessibilityManager>();
        
        if (accessibilityManager) {
            // Initialize player-specific accessibility enhancements
            if (accessibilityManager->initializePlayerAccessibility(this)) {
                qDebug() << "Player accessibility initialized successfully";
            } else {
                qWarning() << "Failed to initialize player accessibility";
            }
        } else {
            qWarning() << "AccessibilityManager not available - accessibility features disabled";
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception during accessibility initialization:" << e.what();
    }
}

player::~player()
{
    delete ui;
    delete audioRecorder;
    delete captureSession;
    delete audioInput;
    delete XplayerOutput;
    delete lp1_XplayerOutput;
    delete lp2_XplayerOutput;
    delete adBanner;
}

// Generic helper to launch an external GUI application safely
void player::launchExternalApplication(const QString& appName, const QString& filePath) {
    QString appPath = QStandardPaths::findExecutable(appName);
    if (appPath.isEmpty()) {
        qWarning() << "Cannot find executable for" << appName << "in system PATH.";
        // Try opening with default handler as fallback?
        bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        if (!opened) {
             QMessageBox::warning(this, tr("Application Not Found"), tr("Could not find '%1' in your system's PATH, and could not open the file '%2' with the default application.").arg(appName, QFileInfo(filePath).fileName()));
        } else {
             QMessageBox::information(this, tr("Opening File"), tr("'%1' not found. Opening '%2' with the default application instead.").arg(appName, QFileInfo(filePath).fileName()));
        }
        return;
    }

    qInfo() << "Launching" << appName << " (" << appPath << ") with file:" << filePath;
    QStringList args;
    // Most GUI apps take the file path as a direct argument
    args << filePath;

    if (!QProcess::startDetached(appPath, args)) {
        qWarning() << "Failed to start detached process:" << appPath << args;
        QMessageBox::critical(this, tr("Launch Error"), tr("Failed to launch '%1' with the specified file.").arg(appName));
    }
}

// Generic helper to get MediaInfo (ASYNC) - Requires modification
// TODO: Implement proper parsing of MediaInfo output (JSON is best)
void player::getMediaInfoForFile(const QString& filePath) {
    QString mediaInfoPath = QStandardPaths::findExecutable("mediainfo");
    if (mediaInfoPath.isEmpty()) {
         QMessageBox::warning(this, tr("Dependency Missing"), tr("'mediainfo' command not found. Please install it to retrieve metadata."));
         return;
    }

    QProcess *mediaInfoProcess = new QProcess(this);
    QStringList args;
    // Request specific tags for easier parsing, or use --Output=JSON
    args << "--Inform=General;%Artist%\\n%Title%\\n%Album%\\n%Genre%\\n%Duration/String3%\\n%FileSize/String%\\n%Format%\\n%OverallBitRate/String%"
         << filePath;

    qInfo() << "Getting MediaInfo:" << mediaInfoPath << args;

    connect(mediaInfoProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, mediaInfoProcess, filePath](int exitCode, QProcess::ExitStatus exitStatus){

        QString output = QString::fromLocal8Bit(mediaInfoProcess->readAllStandardOutput()).trimmed();
        QString errorOutput = QString::fromLocal8Bit(mediaInfoProcess->readAllStandardError()).trimmed();

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qWarning() << "mediainfo failed for" << filePath << "Exit:" << exitCode;
            if (!errorOutput.isEmpty()) qWarning() << "mediainfo STDERR:" << errorOutput;
             QMessageBox::warning(this, tr("Metadata Error"), tr("Failed to get metadata using 'mediainfo'.\n%1").arg(errorOutput));
        } else {
            qDebug() << "mediainfo output:\n" << output;
            // ** TODO: Parse the 'output' string here! **
            // The original parsing was extremely fragile (fixed indices).
            // Need to parse based on the requested format or, better, use JSON.
            // Example (NEEDS PROPER IMPLEMENTATION):
            QStringList lines = output.split('\n');
            if (lines.count() >= 8) { // Based on the requested format
                 QString artist = lines[0];
                 QString song = lines[1];
                 QString album = lines[2];
                 QString genre = lines[3];
                 QString duration = lines[4];
                 QString size = lines[5];
                 QString format = lines[6];
                 QString bitrate = lines[7];

                 QString msg4box = tr("Artist: %1\nSong: %2\nAlbum: %3\nGenre: %4\nDuration: %5\nSize: %6\nFormat: %7\nBitrate: %8")
                                     .arg(artist, song, album, genre, duration, size, format, bitrate);

                 QMessageBox::StandardButton rpl = QMessageBox::question(this, tr("Apply this info to the database?"), msg4box, QMessageBox::Yes | QMessageBox::No);

                 if (rpl == QMessageBox::Yes) {
                     // Update Database (Use Prepared Statements!)
                     QSqlDatabase db = QSqlDatabase::database("xfb_connection");
                     QSqlQuery query(db);
                     query.prepare("UPDATE musics SET artist = :artist, song = :song, genre1 = :genre, genre2 = :genre " // Assuming genre1/2 same here
                                     "WHERE path = :path");
                     query.bindValue(":artist", artist);
                     query.bindValue(":song", song);
                     query.bindValue(":genre", genre); // Or more complex genre mapping
                     query.bindValue(":path", filePath);

                     if (!query.exec()) {
                          qWarning() << "Failed to update metadata in DB:" << query.lastError().text();
                          QMessageBox::warning(this, tr("Database Error"), tr("Failed to update metadata in the database."));
                     } else {
                          qInfo() << "Metadata updated in DB for:" << filePath;
                          update_music_table();
                     }
                 }
            } else {
                qWarning() << "Could not parse mediainfo output:" << output;
                 QMessageBox::warning(this, tr("Metadata Error"), tr("Could not parse the metadata received from 'mediainfo'."));
            }
        }
        mediaInfoProcess->deleteLater();
    });

     connect(mediaInfoProcess, &QProcess::errorOccurred, this, [this, mediaInfoProcess](QProcess::ProcessError error){
         qWarning() << "Failed to start mediainfo. Error:" << error << "-" << mediaInfoProcess->errorString();
         QMessageBox::critical(this, tr("Process Error"), tr("Could not start the 'mediainfo' process."));
         mediaInfoProcess->deleteLater();
     });

    mediaInfoProcess->start(mediaInfoPath, args);
}


void player::updateConfig() {
    qDebug() << "Updating player configuration using QSettings...";

    // --- Use QSettings with the WRITABLE configuration file path ---
    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (writableConfigPath.isEmpty()) {
        qCritical() << "Could not determine writable config location in updateConfig!";
        return; // Cannot proceed without path
    }
    QString configFilePath = writableConfigPath + "/" + configFileName;

    QSettings settings(configFilePath, QSettings::IniFormat);
    qDebug() << "Reading configuration from:" << settings.fileName();

    // --- Read values using settings.value() and assign to member variables ---

    SavePath = settings.value("SavePath").toString();
    Server_URL = settings.value("Server_URL").toString();
    Port = settings.value("Port", 0).toInt(); // Provide default, convert to int
    User = settings.value("User").toString();
    Pass = settings.value("Pass").toString(); // Still insecure storage
    ProgramsPath = settings.value("ProgramsPath").toString();
    MusicPath = settings.value("MusicPath").toString();
    JinglePath = settings.value("JinglePath").toString();
    FTPPath = settings.value("FTPPath").toString();
    TakeOverPath = settings.value("TakeOverPath").toString();
    ComHour = settings.value("ComHour", "00:00:00").toString(); // Provide default

    // Read boolean values directly
    fullScreen = settings.value("FullScreen", false).toBool();
    disableSeekBar = settings.value("Disable_Seek_Bar", false).toBool();
    normalization_soft = settings.value("Normalize_Soft", false).toBool();
    Disable_Volume = settings.value("Disable_Volume", false).toBool();
    darkMode = settings.value("DarkMode", false).toBool();

    // Read recording info (description and potentially enum data)
    recDevice = settings.value("RecDevice").toString(); // Store description
    // Read enum values if you saved them that way from optionsDialog
    recCodec = settings.value("RecCodec", QVariant::fromValue(QMediaFormat::AudioCodec::Unspecified)).value<QMediaFormat::AudioCodec>();
    recContainer = settings.value("RecContainer", QVariant::fromValue(QMediaFormat::FileFormat())).value<QMediaFormat::FileFormat>();

    // Database path
    txt_selected_db = settings.value("Database").toString();

    // Role
    Role = settings.value("Role", "Client").toString(); // Default "Client"

    // --- Apply settings to UI or internal state AFTER reading ALL settings ---
    qDebug() << "Applying loaded configuration settings...";

    // Example: Update UI elements based on loaded settings
    if (disableSeekBar) {
        ui->sliderProgress->setEnabled(false);
        qDebug() << "Disable Seek bar setting: true";
    } else {
        ui->sliderProgress->setEnabled(true);
        qDebug() << "Disable Seek bar setting: false";
    }

    if (Disable_Volume) {
        ui->sliderVolume->setEnabled(false);
        qDebug() << "Disable Volume setting: true";
    } else {
        ui->sliderVolume->setEnabled(true);
        qDebug() << "Disable Volume setting: false";
    }

    // Log other settings
    qDebug() << "Normalization Soft setting:" << normalization_soft;
    qDebug() << "Role setting:" << Role;
    if (Role == "Server") {
        qDebug("XFB Role: Server mode actions can be taken now.");
        // Add any specific logic needed when running as server
    } else {
        qDebug("XFB Role: Client mode actions can be taken now.");
        // Add any specific logic needed when running as client
    }
    qDebug() << "DarkMode setting:" << darkMode;
    // Note: Applying dark mode often requires more than just setting the variable.
    // It usually involves reapplying palettes/stylesheets, potentially restarting parts of the UI.
    // Consider how dark mode changes are triggered and applied application-wide.

    // Log paths etc.
    qDebug() << "SavePath:" << SavePath;
    qDebug() << "ProgramsPath:" << ProgramsPath;
    // ... log other variables as needed ...

    qDebug() << "Finished updating player configuration.";
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

bool player::checkDbOpen() {

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QString resourceDbPath = ":/adb.db";
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
         qWarning() << "Could not determine writable application data location.";
         return false;
    }

    QString appDirName = QCoreApplication::applicationName(); // Or your specific app identifier "XFB"
    if (appDirName.isEmpty()) {
        appDirName = "XFB"; // Fallback if not set via QCoreApplication
        qWarning() << "QCoreApplication::applicationName() is empty, using fallback:" << appDirName;
    }
    QString specificAppDataPath = appDataPath + "/" + appDirName;
    QString persistentDbPath = specificAppDataPath + "/adb.db"; // Target path using app-specific subfolder
    const QString connectionName = "xfb_connection"; // Use a distinct connection name

    // --- Ensure Application Data Sub-Directory Exists ---
    QDir dir(specificAppDataPath); // Use the specific path
    if (!dir.exists()) {
        qInfo() << "Creating application data directory:" << specificAppDataPath;
        if (!dir.mkpath(".")) { // mkpath creates parent directories if needed
            qWarning() << "Failed to create application data directory:" << specificAppDataPath;
            return false;
        }
    }

    // --- Check and Copy Database File ---
    QFile persistentFile(persistentDbPath);
    QFileInfo resourceInfo(resourceDbPath);

    if (!resourceInfo.exists()) {
        qCritical() << "CRITICAL: Database resource file not found:" << resourceDbPath;
        qCritical() << "Make sure adb.db is added to your .qrc file and the .qrc is in RESOURCES in your .pro file.";
        return false;
    }

    bool copyRequired = !persistentFile.exists();
    if (persistentFile.exists() && resourceInfo.lastModified() > QFileInfo(persistentFile).lastModified()) {
        qInfo() << "Resource database is newer. Removing old persistent copy.";
        if (!persistentFile.remove()) {
             qWarning() << "Could not remove existing persistent database file:" << persistentFile.errorString() << "- Check if it's locked by another process.";
             // Decide if you want to proceed or fail here. Maybe try opening anyway?
        }
        copyRequired = true;
    }

    if (copyRequired) {
        qInfo() << "Attempting to copy database from resource to:" << persistentDbPath;
        if (!QFile::copy(resourceDbPath, persistentDbPath)) {
            QFile resFile(resourceDbPath); // For error checking
            qCritical() << "Failed to copy database from resource (" << resourceDbPath << ", exists:" << resFile.exists() << ") to"
                       << persistentDbPath << "- Error:" << persistentFile.errorString() << "(Check write permissions for " << specificAppDataPath << ")";
            return false;
        } else {
            // Explicitly set permissions after successful copy
            if (!persistentFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther)) {
                 qWarning() << "Could not set permissions on the copied database file:" << persistentDbPath << "Error:" << persistentFile.errorString();
            } else {
                 qInfo() << "Database copied successfully and permissions set for" << persistentDbPath;
            }
        }
    }

    // --- Verify Readability Before Connecting ---
    // Re-instantiate QFile object to ensure fresh state check
    QFile checkFile(persistentDbPath);
    if (!checkFile.exists()) {
        qCritical() << "Database file disappeared after check/copy? Path:" << persistentDbPath;
        return false;
    }
    if (!checkFile.permissions().testFlag(QFileDevice::ReadOwner)) { // Check owner read permission specifically
         qWarning() << "Persistent database file lacks read permission for owner:" << persistentDbPath;
         qWarning() << "Attempting to grant read permission...";
         if (!checkFile.setPermissions(checkFile.permissions() | QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
              qCritical() << "Failed to grant read/write permission to database file:" << persistentDbPath << "Error:" << checkFile.errorString();
              qCritical() << "Manual permission change might be required. Check user/group ownership and permissions.";
              return false; // Fail if we can't ensure readability
         } else {
              qInfo() << "Read/Write permission granted successfully.";
         }
    }


    // --- Setup the QSqlDatabase connection ---
    if (QSqlDatabase::contains(connectionName)) {
         // Even if reusing, check if it's actually usable / still open correctly
         if (db.isOpen() && !db.databaseName().isEmpty() ) {
             // Optional: Ping the database to be sure it's responsive
             QSqlQuery pingQuery(db);
             if (!pingQuery.exec("SELECT 1")) { // Simple query
                 qWarning() << "Ping query failed on existing open connection. Error:" << pingQuery.lastError().text();
                 qWarning() << "Connection state might be stale. Closing it.";
                 adb.close(); // Close the potentially stale connection
             } else {
                 qDebug() << "Existing connection ping successful.";
             }
         } else {
              qDebug() << "Existing connection handle was found but not open or configured. Will proceed to open.";
         }

    } else {
        adb = QSqlDatabase::addDatabase("QSQLITE", connectionName);
         qDebug() << "Adding new database connection:" << connectionName;
        if (!adb.isValid()) {
            qCritical() << "Failed to add database connection. QSQLITE driver possibly missing or invalid.";
            qCritical() << "Available drivers:" << QSqlDatabase::drivers();
            return false;
        }
    }

    // Set the database file path *after* getting a valid handle
    // This is important even for existing connections if they were closed or became invalid
    if (adb.databaseName() != persistentDbPath) {
         qDebug() << "Setting database name for connection" << connectionName << "to" << persistentDbPath;
         adb.setDatabaseName(persistentDbPath);
    }


    // Only try to open if it's not already open (or if we closed it above)
    if (!adb.isOpen()) {
        qInfo() << "Attempting to open database connection:" << connectionName << " Path:" << adb.databaseName();
        if (!adb.open()) {
            qCritical() << "Database Error: Failed to open database:" << adb.lastError().text();
            qCritical() << "Database path was:" << adb.databaseName();
            qCritical() << "Underlying driver error:" << adb.lastError().driverText();
             qCritical() << "Available drivers:" << QSqlDatabase::drivers();
             if (adb.lastError().text().contains("Driver not loaded")) {
                 qCritical() << ">>> Potential deployment issue: Ensure the QSQLITE plugin is deployed correctly.";
             }
             // Specifically check for file permission related errors if possible
             if (adb.lastError().text().contains("unable to open", Qt::CaseInsensitive)) {
                 qCritical() << ">>> Error indicates file access problem. Verify permissions and path again.";
                 qCritical() << ">>> Path:" << persistentDbPath << " Exists:" << QFile::exists(persistentDbPath) << " Readable:" << QFile(persistentDbPath).isReadable();
             }
            return false;
        } else {
            qInfo() << "Database connection opened successfully:" << adb.connectionName() << " Path:" << adb.databaseName();
        }
    } else {
         qDebug() << "Database connection" << connectionName << "was already open and seems valid.";
    }

    // Final check
    if (!adb.isOpen()) {
        qCritical() << "Database connection is unexpectedly not open after initialization sequence.";
        return false;
    }

    qInfo() << "Database initialization successful. Connection '" << connectionName << "' is open.";
    return true;
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

        // If we're currently playing, we don't need to do anything else
        // The files will be played when the current track finishes
        // If we're not playing, we could start playing the first file
        if (PlayMode == "stopped" && !fileNames.isEmpty()) {
            // Add the first file to the playlist and play it
            XplaylistUrls.clear();
            XplaylistUrls.append(QUrl::fromLocalFile(fileNames.first()));
            XplaylistIndex = 0;

            // This will trigger playback if the user clicks Play
            // We don't auto-start playback here to maintain the original behavior
        }
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

void player::musicViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->musicView->mapToGlobal(pos);
    QMenu thisMenu;
    // Use constants or enums instead of strings for actions if preferred
    const QString actionAddToBottom = tr("Add to the bottom of playlist");
    const QString actionAddToTop = tr("Add to the top of the playlist");
    const QString actionDeleteFromDB = tr("Delete this track from database");
    const QString actionOpenAudacity = tr("Open this in Audacity");
    const QString actionGetInfo = tr("Retrieve metadata from file (mediainfo)");

    thisMenu.addAction(actionAddToBottom);
    thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    thisMenu.addAction(actionOpenAudacity);
    thisMenu.addAction(actionGetInfo);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return; // No action selected

    QModelIndexList selectedIndexes = ui->musicView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return; // No row selected

    int rowidx = selectedIndexes.first().row(); // Use first selected index
    QModelIndex pathIndex = ui->musicView->model()->index(rowidx, 7); // Assuming column 7 is path
    if (!pathIndex.isValid()) return;
    QString selectedFilePath = ui->musicView->model()->data(pathIndex).toString();

    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionAddToBottom) {
        qDebug() << "Adding to bottom of playlist:" << selectedFilePath;
        ui->playlist->addItem(selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionAddToTop) {
        qDebug() << "Adding to top of playlist:" << selectedFilePath;
        ui->playlist->insertItem(0, selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionDeleteFromDB) {
        QMessageBox::StandardButton go = QMessageBox::question(this, tr("Confirm Deletion"), tr("Are you sure you want to delete the track '%1' from the database?").arg(QFileInfo(selectedFilePath).fileName()), QMessageBox::Yes | QMessageBox::No);
        if (go == QMessageBox::Yes) {
            QMessageBox::StandardButton rm = QMessageBox::question(this, tr("Delete File?"), tr("Also delete the file '%1' from the hard drive?").arg(QFileInfo(selectedFilePath).fileName()), QMessageBox::Yes | QMessageBox::No);

            bool deleteFile = (rm == QMessageBox::Yes);
            bool fileDeleted = false;

            if (deleteFile) {
                qInfo() << "Attempting to delete file from disk:" << selectedFilePath;
                if (QFile::exists(selectedFilePath)) {
                    if (QFile::remove(selectedFilePath)) {
                        qInfo() << "File successfully deleted from disk.";
                        fileDeleted = true;
                    } else {
                        qWarning() << "Failed to delete file from disk:" << selectedFilePath;
                        QMessageBox::warning(this, "File Deletion Failed", tr("Could not delete the file:\n%1\nPlease check permissions.").arg(selectedFilePath));
                        // Ask user if they still want to delete DB record? Maybe not. Let's abort.
                         return;
                    }
                } else {
                     qWarning() << "File not found for deletion, proceeding with DB removal.";
                     fileDeleted = true; // Treat as success if not found
                }
            }

            // Proceed with DB deletion if physical file deleted or not requested
            QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            QSqlQuery sql(db);
            sql.prepare("DELETE FROM musics WHERE path = :path");
            sql.bindValue(":path", selectedFilePath);
            if (sql.exec()) {
                qInfo() << "Track removed from database:" << selectedFilePath;
                update_music_table();
                QMessageBox::information(this, tr("Success"), tr("Track removed from database.") + (deleteFile && fileDeleted ? tr("\nFile also deleted from disk.") : ""));
            } else {
                qCritical() << "Database Error deleting track:" << sql.lastError().text() << sql.lastQuery();
                QMessageBox::critical(this, tr("Database Error"), tr("Failed to remove the track from the database:\n%1").arg(sql.lastError().text()));
                // If DB delete failed after physical delete, we have an orphan file record potentially
                if(deleteFile && fileDeleted){
                     QMessageBox::warning(this, tr("Potential Issue"), tr("The file was deleted from disk, but removing the database record failed. Please check the database."));
                }
            }
        }
    } else if (selectedActionText == actionOpenAudacity) {
        launchExternalApplication("audacity", selectedFilePath);
    } else if (selectedActionText == actionGetInfo) {
        getMediaInfoForFile(selectedFilePath);
    }
}


void player::jinglesViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->jinglesView->mapToGlobal(pos);
    QMenu thisMenu;
    const QString actionAddToBottom = tr("Add to the bottom of playlist");
    const QString actionAddToTop = tr("Add to the top of the playlist");
    const QString actionDeleteFromDB = tr("Delete this jingle from the database");
    const QString actionOpenAudacity = tr("Open this in Audacity");

    thisMenu.addAction(actionAddToBottom);
    thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    thisMenu.addAction(actionOpenAudacity);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QModelIndexList selectedIndexes = ui->jinglesView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    int rowidx = selectedIndexes.first().row();
    QModelIndex pathIndex = ui->jinglesView->model()->index(rowidx, 1); // Assuming column 1 is path
    if (!pathIndex.isValid()) return;
    QString selectedFilePath = ui->jinglesView->model()->data(pathIndex).toString();

    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionAddToBottom) {
        ui->playlist->addItem(selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionAddToTop) {
        ui->playlist->insertItem(0, selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionDeleteFromDB) {
        QMessageBox::StandardButton go = QMessageBox::question(this, tr("Confirm Deletion"), tr("Are you sure you want to delete this jingle from the database?\n(File on disk will NOT be deleted)"), QMessageBox::Yes | QMessageBox::No);
        if (go == QMessageBox::Yes) {
            QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            QSqlQuery sql(db);
            sql.prepare("DELETE FROM jingles WHERE path = :path");
            sql.bindValue(":path", selectedFilePath);
            if (sql.exec()) {
                qInfo() << "Jingle removed from database:" << selectedFilePath;
                update_music_table(); // This reloads all tables, including jingles
                QMessageBox::information(this, tr("Jingle Removed"), tr("The jingle was removed from the database."));
            } else {
                 qCritical() << "Database Error deleting jingle:" << sql.lastError().text() << sql.lastQuery();
                 QMessageBox::critical(this, tr("Database Error"), tr("Failed to remove the jingle from the database:\n%1").arg(sql.lastError().text()));
            }
        }
    } else if (selectedActionText == actionOpenAudacity) {
        launchExternalApplication("audacity", selectedFilePath);
    }
}


void::player::pubViewContextMenu(const QPoint& pos){
    QPoint globalPos = ui->pubView->mapToGlobal(pos);
    QMenu thisMenu;
    QString addToBottomOfPlaylist = tr("Add to the bottom of playlist");
    QString addtoTopOfPlaylist = tr("Add to the top of the playlist");
    QString deleteThisFromDB = tr("Delete this pub from the database");
    QString openWithAudacity = tr("Open this in Audacity");

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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
                checkDbOpen();
                QSqlQuery sql(db);
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
// Helper function to run a server script asynchronously
void player::runServerCheckScript(const QString& scriptName, const QString& fileToCheck, const QString& successMessage, const QString& failureMessage) {
    // TODO: Replace this with a robust way to find the script
    QString scriptBaseName = scriptName; // e.g., "serverFtpCmdsCHKProgram.sh"
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path
    QString scriptPath = QDir(scriptDir).filePath(scriptBaseName);

    qInfo() << "Attempting to execute check script:" << scriptPath << "for file:" << fileToCheck;

    if (!QFileInfo::exists(scriptPath)) {
        qWarning() << "Check script not found at:" << scriptPath;
        QMessageBox::critical(this, "Script Error", QString("The required check script was not found:\n%1").arg(scriptPath));
        return;
    }

    QProgressDialog progress(tr("Checking server..."), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    qApp->processEvents();

    QProcess *checkProcess = new QProcess(this);
    QStringList args;
    // Pass filename as argument instead of relying on grep? Modify script if possible.
    // For now, mimic original grep:
    // args << "| grep" << fileToCheck; // NO! Don't pipe in C++ QProcess args

    connect(checkProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, checkProcess, fileToCheck, successMessage, failureMessage, &progress](int exitCode, QProcess::ExitStatus exitStatus) {

        progress.cancel(); // Close progress dialog
        QString stdOutput = QString::fromLocal8Bit(checkProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(checkProcess->readAllStandardError()).trimmed();
        bool found = false;

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            qDebug() << "Check script STDOUT:\n" << stdOutput;
            // Check if the output contains the filename we are looking for
            if (stdOutput.contains(fileToCheck, Qt::CaseInsensitive)) {
                found = true;
            } else {
                 if (!stdError.isEmpty()) qWarning() << "Check script STDERR (exit 0):\n" << stdError;
            }
        } else {
            qWarning() << "Check script failed or exited with error. ExitCode:" << exitCode;
            if (!stdError.isEmpty()) qWarning() << "Check script STDERR:\n" << stdError;
        }

        if (found) {
            QMessageBox::information(this, tr("Check Successful"), successMessage + "\n\nServer Output:\n" + stdOutput.left(300));
        } else {
            QMessageBox::critical(this, tr("Check Failed"), failureMessage);
        }

        checkProcess->deleteLater();
    });

    connect(checkProcess, &QProcess::errorOccurred, this, [this, checkProcess, scriptPath, &progress](QProcess::ProcessError error){
         qWarning() << "Failed to start check script (" << scriptPath << "). Error:" << error << "-" << checkProcess->errorString();
         progress.cancel();
         QMessageBox::critical(this, "Script Error", QString("Could not start the check script:\n%1\n\nError: %2").arg(scriptPath).arg(checkProcess->errorString()));
         checkProcess->deleteLater();
     });

    // Start the script - How to handle the grep part robustly?
    // Option 1 (Best): Modify script to take filename as arg and return exit code 0 if found.
    // checkProcess->start(scriptPath, QStringList() << fileToCheck); // If script modified
    // Option 2 (Current): Run script and check output in C++.
    checkProcess->start(scriptPath); // Run script, check full output in finished signal
}
// Helper function to run upload/put script asynchronously
void player::runServerUploadScript(const QString& scriptName, const QString& fileToUpload, const QString& successMessage, const QString& failureMessage, std::function<void(bool)> callback) {
    // TODO: Replace this with a robust way to find the script
    QString scriptBaseName = scriptName; // e.g., "serverFtpCmdsPutProgram.sh"
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path
    QString scriptPath = QDir(scriptDir).filePath(scriptBaseName);

    qInfo() << "Attempting to execute upload script:" << scriptPath << "for file:" << fileToUpload;
     qDebug() << "Dependencies: Script must exist, be executable, ~/.netrc configured.";

    if (!QFileInfo::exists(scriptPath)) {
        qWarning() << "Upload script not found at:" << scriptPath;
        QMessageBox::critical(this, "Script Error", QString("The required upload script was not found:\n%1").arg(scriptPath));
        callback(false); // Indicate failure
        return;
    }

    QProgressDialog progress(tr("Uploading to server..."), tr("Cancel"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    qApp->processEvents();


    QProcess *uploadProcess = new QProcess(this);
    // Does the script need the filename as argument? Assume not for now based on original.

    connect(uploadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, uploadProcess, successMessage, failureMessage, &progress, callback](int exitCode, QProcess::ExitStatus exitStatus) {

        progress.cancel();
        QString stdOutput = QString::fromLocal8Bit(uploadProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(uploadProcess->readAllStandardError()).trimmed();
        bool success = false;

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
             qDebug() << "Upload script STDOUT:\n" << stdOutput;
            if (stdOutput.contains("Transfer complete", Qt::CaseInsensitive)) {
                 qInfo() << "Upload script reported success.";
                 success = true;
             } else {
                  qWarning() << "Upload script finished (exit 0) but success message not found in output.";
                  if (!stdError.isEmpty()) qWarning() << "Upload script STDERR:\n" << stdError;
             }
        } else {
            qWarning() << "Upload script failed or exited with error. ExitCode:" << exitCode;
            if (!stdError.isEmpty()) qWarning() << "Upload script STDERR:\n" << stdError;
            else if (!stdOutput.isEmpty()) qWarning() << "Upload script STDOUT (check errors):\n" << stdOutput;
        }

        if (success) {
             QMessageBox::information(this, tr("Upload Successful"), successMessage);
             callback(true); // Indicate success
        } else {
             QMessageBox::critical(this, "Upload Failed", failureMessage + tr("\nCheck ~/.netrc, script, network, server status.\nOutput:\n%1\n%2")
                                   .arg(stdOutput.left(200)).arg(stdError.left(200)));
             callback(false); // Indicate failure
        }
        uploadProcess->deleteLater();
    });

     connect(uploadProcess, &QProcess::errorOccurred, this, [this, uploadProcess, scriptPath, failureMessage, &progress, callback](QProcess::ProcessError error){
         qWarning() << "Failed to start upload script (" << scriptPath << "). Error:" << error << "-" << uploadProcess->errorString();
         progress.cancel();
         QMessageBox::critical(this, "Script Error", QString("Could not start the upload script:\n%1\n\nError: %2").arg(scriptPath).arg(uploadProcess->errorString()) + "\n" + failureMessage);
         callback(false); // Indicate failure
         uploadProcess->deleteLater();
     });

    // Start the script
    uploadProcess->start(scriptPath);
}


void player::programsViewContextMenu(const QPoint& pos) {
    QPoint globalPos = ui->programsView->mapToGlobal(pos);
    QMenu thisMenu;
    const QString actionAddToBottom = tr("Add to the bottom of playlist");
    const QString actionAddToTop = tr("Add to the top of the playlist");
    const QString actionDeleteFromDB = tr("Delete this program from the database");
    const QString actionOpenAudacity = tr("Open this in Audacity");
    const QString actionResendToServer = tr("(Re)Send this program to the server");
    const QString actionCheckSent = tr("Verify that the program is in the server");

    thisMenu.addAction(actionAddToBottom);
    thisMenu.addAction(actionAddToTop);
    thisMenu.addSeparator();
    thisMenu.addAction(actionDeleteFromDB);
    thisMenu.addSeparator();
    thisMenu.addAction(actionOpenAudacity);
    thisMenu.addSeparator();
    thisMenu.addAction(actionCheckSent);
    thisMenu.addAction(actionResendToServer);

    QAction* selectedItem = thisMenu.exec(globalPos);
    if (!selectedItem) return;

    QModelIndexList selectedIndexes = ui->programsView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;

    int rowidx = selectedIndexes.first().row();
    QModelIndex pathIndex = ui->programsView->model()->index(rowidx, 2); // Assuming column 2 is path
    if (!pathIndex.isValid()) return;
    QString selectedFilePath = ui->programsView->model()->data(pathIndex).toString();
    QFileInfo fileInfo(selectedFilePath);
    QString selectedFileName = fileInfo.fileName();


    QString selectedActionText = selectedItem->text();

    if (selectedActionText == actionAddToBottom) {
        ui->playlist->addItem(selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionAddToTop) {
        ui->playlist->insertItem(0, selectedFilePath);
        calculate_playlist_total_time();
    } else if (selectedActionText == actionDeleteFromDB) {
         QMessageBox::StandardButton go = QMessageBox::question(this, tr("Confirm Deletion"), tr("Are you sure you want to delete this program from the database?\n(File on disk will NOT be deleted)"), QMessageBox::Yes | QMessageBox::No);
         if (go == QMessageBox::Yes) {
             QSqlDatabase db = QSqlDatabase::database("xfb_connection");
             QSqlQuery sql(db);
             sql.prepare("DELETE FROM programs WHERE path = :path");
             sql.bindValue(":path", selectedFilePath);
             if (sql.exec()) {
                 qInfo() << "Program removed from database:" << selectedFilePath;
                 update_music_table();
                 QMessageBox::information(this, tr("Program Removed"), tr("The program was removed from the database."));
             } else {
                 qCritical() << "Database Error deleting program:" << sql.lastError().text() << sql.lastQuery();
                 QMessageBox::critical(this, tr("Database Error"), tr("Failed to remove the program from the database:\n%1").arg(sql.lastError().text()));
             }
         }
    } else if (selectedActionText == actionOpenAudacity) {
         launchExternalApplication("audacity", selectedFilePath);
    } else if (selectedActionText == actionCheckSent) {
        runServerCheckScript("serverFtpCmdsCHKProgram.sh", selectedFileName,
                             tr("The program '%1' is present on the server!").arg(selectedFileName),
                             tr("The program '%1' was NOT found on the server.").arg(selectedFileName));
    } else if (selectedActionText == actionResendToServer) {
         qInfo() << "(Re)Sending program to server:" << selectedFilePath;
         ui->txt_uploadingPrograms->show(); // Show indicator

         // 1. Copy file to temporary FTP location (FTPPath)
         QString ftpTempPath = QDir(FTPPath).filePath(selectedFileName);
         qInfo() << "Copying" << selectedFilePath << "to" << ftpTempPath;
         QFile::remove(ftpTempPath); // Remove existing temp file first
         if (!QFile::copy(selectedFilePath, ftpTempPath)) {
             qWarning() << "Failed to copy program to temporary FTP folder:" << ftpTempPath;
             QMessageBox::critical(this, tr("Copy Error"), tr("Failed to copy the program file to the temporary upload folder.\nCheck permissions for '%1'.").arg(FTPPath));
             ui->txt_uploadingPrograms->hide();
             return;
         }

         // 2. Run the upload script asynchronously
         runServerUploadScript("serverFtpCmdsPutProgram.sh", ftpTempPath,
                               tr("Program '%1' uploaded successfully.").arg(selectedFileName),
                               tr("Failed to upload program '%1'.").arg(selectedFileName),
                               // Callback function after upload attempt:
                               [this, ftpTempPath](bool uploadSuccess) {
                                    // 3. Clean up temporary file
                                    qInfo() << "Cleaning up temporary FTP file:" << ftpTempPath;
                                    if (QFile::remove(ftpTempPath)) {
                                        qDebug() << "Removed temporary FTP file:" << ftpTempPath;
                                    } else {
                                        qWarning() << "Failed to remove temporary FTP file:" << ftpTempPath;
                                    }
                                    ui->txt_uploadingPrograms->hide(); // Hide indicator
                                    // No further action needed here based on original code after upload attempt
                               });
     }
}


// Helper to get duration using exiftool (async)
void player::getDurationForFile(const QString& filePath, std::function<void(const QString&, const QString&)> callback) {
    QString exiftoolPath = QStandardPaths::findExecutable("exiftool");
    if (exiftoolPath.isEmpty()) {
        qWarning() << "exiftool not found for duration check.";
        callback(filePath, ""); // Return empty duration on failure
        return;
    }

    QProcess *process = new QProcess(this);
    QStringList arguments;
    arguments << "-T" << "-Duration" << filePath; // Use -T for direct value output

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [process, filePath, callback](int exitCode, QProcess::ExitStatus exitStatus){
        QString durationValue = "";
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            durationValue = QString::fromLocal8Bit(process->readAllStandardOutput()).trimmed();
             if (durationValue == "-") durationValue = ""; // Handle case where tag not found
        } else {
            qWarning() << "exiftool failed for duration check on" << filePath;
            // Log error output?
        }
        callback(filePath, durationValue); // Call the callback with result
        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this, [process, filePath, callback](QProcess::ProcessError error){
         qWarning() << "Failed to start exiftool for duration check. Error:" << error;
         callback(filePath, ""); // Return empty duration
         process->deleteLater();
    });

    process->start(exiftoolPath, arguments);
}

void player::on_btPlay_clicked(){
    qDebug()<<"Play button clicked";

    if(PlayMode=="stopped"){
        if(darkMode){
            ui->btPlay->setStyleSheet("background-color:#5e9604"); //green
        }else{

            ui->btPlay->setStyleSheet("background-color:#2CCD54"); //green
        }
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";
        playNextSong();

    }  else if(PlayMode=="Playing_StopAtNextOne"){

        if(darkMode){
            ui->btPlay->setStyleSheet("background-color:#5e9604"); //green
        }else{

            ui->btPlay->setStyleSheet("background-color:#2CCD54"); //green
        }
        ui->btPlay->setText(tr("Play and Segue"));
        PlayMode = "Playing_Segue";

    } else if(PlayMode=="Playing_Segue"){

        ui->btPlay->setStyleSheet("background-color:#F0DB1B"); //amarillo
        PlayMode = "Playing_StopAtNextOne";
        ui->btPlay->setText(tr("Play and Stop"));


    }


}

void player::playNextMedia() {
    // Increment the playlist index and play the next media
    if (XplaylistUrls.isEmpty()) {
        qDebug() << "Playlist is empty, cannot play next media";
        return;
    }

    XplaylistIndex++;
    if (XplaylistIndex >= XplaylistUrls.size()) {
        // Reached the end of the playlist
        XplaylistIndex = 0;
        if (PlayMode == "Playing_Segue") {
            // In segue mode, try to get the next song from the playlist widget
            playNextSong();
        } else {
            // Otherwise, stop playback
            Xplayer->stop();
            ui->btPlay->setStyleSheet("");
            ui->btPlay->setText(tr("Play"));
            PlayMode = "stopped";
        }
    } else {
        // Play the next media in the playlist
        Xplayer->setSource(XplaylistUrls[XplaylistIndex]);
        Xplayer->play();
    }
}

void player::playPreviousMedia() {
    // Decrement the playlist index and play the previous media
    if (XplaylistUrls.isEmpty()) {
        qDebug() << "Playlist is empty, cannot play previous media";
        return;
    }

    XplaylistIndex--;
    if (XplaylistIndex < 0) {
        // Reached the beginning of the playlist, wrap around to the end
        XplaylistIndex = XplaylistUrls.size() - 1;
    }

    Xplayer->setSource(XplaylistUrls[XplaylistIndex]);
    Xplayer->play();
}

void player::playNextSong(){


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open in playNextSong!";
        checkDbOpen(); // Try to reopen the database
        db = QSqlDatabase::database("xfb_connection");
        if (!db.isOpen()) {
            qCritical() << "Failed to open database connection in playNextSong!";
            return;
        }
    }
    
    if(PlayMode=="Playing_Segue"){
        qDebug()<<"The white rabit is Playing_segue";


        if(ui->playlist->count()>0){


            QString itemDaPlaylist = ui->playlist->item(0)->text();

            qDebug()<<"itemDaPlaylist has value "<<itemDaPlaylist;

            if((lastPlayedSong!=itemDaPlaylist)||(autoMode==0)){
                qDebug()<<"lastplayesong != itemdaplaylist";

                // Clear current playlist and add new media
                XplaylistUrls.clear();
                XplaylistUrls.append(QUrl::fromLocalFile(itemDaPlaylist));
                XplaylistIndex = 0;

                // Set the media to play
                Xplayer->setSource(XplaylistUrls[XplaylistIndex]);

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

                    QSqlQuery* qry = new QSqlQuery(db);
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
                        checkDbOpen();
                        QSqlQuery query(db);
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
            // Prevent infinite recursion by checking if playlist is still empty after playlistAboutToFinish
            int currentPlaylistCount = ui->playlist->count();
            playlistAboutToFinish();
            if(ui->playlist->count() > currentPlaylistCount) {
                // Only recurse if new items were added to the playlist
                playNextSong();
            } else {
                qDebug()<<"No new items added to playlist, stopping to prevent infinite recursion";
                Xplayer->stop();
                ui->btPlay->setStyleSheet("");
                ui->btPlay->setText(tr("Play"));
                PlayMode = "stopped";
            }
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
    XplaylistUrls.clear();
    XplaylistIndex = 0;
}

void player::on_sliderProgress_sliderMoved(int position)
{
    //qDebug()<<"progreess slider mooved "<<position;
    Xplayer->setPosition(position);
}

void player::on_sliderVolume_sliderMoved(int position)
{
    //qDebug()<<"volume slider mooved "<<position;
    XplayerOutput->setVolume(position / 100.0);

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

void player::currentMediaChanged(const QUrl &content)
{
    // Unused parameter 'content'
    Q_UNUSED(content);

    //qDebug()<<"On currentMediaChanged with content: "<<content;
    if(onAbout2Finish==1)
    {
        onAbout2Finish = 0;
        // Using XplaylistUrls instead of deprecated QMediaPlaylist
        if (XplaylistIndex < XplaylistUrls.size()) {
            XplaylistUrls.removeAt(XplaylistIndex);
        }
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

void player::lp1_currentMediaChanged(const QUrl &content)
{
   // Unused parameter 'content'
   Q_UNUSED(content);

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

void player::lp2_currentMediaChanged(const QUrl &content)
{
    // Unused parameter 'content', but keeping method for signal connection
    Q_UNUSED(content);

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


void player::update_music_table() {

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    checkDbOpen();

    qInfo() << "Updating tables using connection:" << db.connectionName() << "DB Name:" << db.databaseName();

    // --- Populate music table ---
    // Always delete previous model if reloading to prevent memory leaks and issues
    delete ui->musicView->model(); // Delete old model first
    QSqlTableModel *model = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    model->setTable("musics");
    // Optional: Set edit strategy if needed (BEFORE select)
    // model->setEditStrategy(QSqlTableModel::OnFieldChange);
    if (!model->select()) {
        qWarning() << "Failed to select 'musics' table:" << model->lastError().text();
        delete model; // Clean up failed model
    } else {
        ui->musicView->setModel(model);
        // Configure view AFTER setting model (if needed)
         qDebug() << "'musics' table model set.";
         // Re-apply view settings if necessary as they might reset with new model
         ui->musicView->setSortingEnabled(true);
         ui->musicView->hideColumn(0);
         // ... other ui->musicView settings ...
    }


    // --- Populate jingles table ---
    delete ui->jinglesView->model(); // Delete old model
    QSqlTableModel *jinglesmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    jinglesmodel->setTable("jingles");
    if (!jinglesmodel->select()) {
        qWarning() << "Failed to select 'jingles' table:" << jinglesmodel->lastError().text();
        delete jinglesmodel;
    } else {
        ui->jinglesView->setModel(jinglesmodel);
         qDebug() << "'jingles' table model set.";
         // Configure view if needed
    }


    // --- Populate Pub table ---
    delete ui->pubView->model(); // Delete old model
    QSqlTableModel *pubmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    pubmodel->setTable("pub");
    if (!pubmodel->select()) {
        qWarning() << "Failed to select 'pub' table:" << pubmodel->lastError().text();
        delete pubmodel;
    } else {
        ui->pubView->setModel(pubmodel);
         qDebug() << "'pub' table model set.";
        // Configure view if needed
    }

    // --- Populate Programs table ---
    delete ui->programsView->model(); // Delete old model
    QSqlTableModel *programsmodel = new QSqlTableModel(this, db); // Pass the CORRECT db handle
    programsmodel->setTable("programs");
    if (!programsmodel->select()) {
        qWarning() << "Failed to select 'programs' table:" << programsmodel->lastError().text();
        delete programsmodel;
    } else {
        ui->programsView->setModel(programsmodel);
         qDebug() << "'programs' table model set.";
        // Configure view if needed
    }


    // --- Populate genre1 and 2 filters ---
    // No need to delete QComboBox models usually unless you are replacing them frequently
    // Create model ONCE, maybe in constructor, and just update query if needed?
    // Or if updating here, manage previous model memory:
    delete ui->cBoxGenre1->model(); // Delete previous model data if reloading fully
    // Note: cBoxGenre2 shares the same model, deleting above is sufficient

    // Create model associated with the correct DB connection
    QSqlQueryModel *model_genre1 = new QSqlQueryModel(this); // Parent 'this' manages memory somewhat

    // Use a stack-based QSqlQuery - safer memory management
    QSqlQuery qry(db); // Pass the CORRECT db handle

    QString sqlq = "SELECT name FROM genres1 ORDER BY name"; // Added ORDER BY
    if (!qry.prepare(sqlq)) { // Prepare is safer
         qWarning() << "Failed to prepare genre query:" << qry.lastError().text();
         delete model_genre1; // Clean up model
         return; // Exit if query prep fails
    }

    if (!qry.exec()) { // Execute the prepared query
        qWarning() << "Failed to execute genre query:" << qry.lastError().text();
        delete model_genre1; // Clean up model
        return; // Exit if query exec fails
    }

    model_genre1->setQuery(std::move(qry)); // Set query AFTER successful execution
    if(model_genre1->lastError().isValid()){
         qWarning() << "Error setting query on genre model:" << model_genre1->lastError().text();
    }

    // Set model for BOTH combo boxes
    ui->cBoxGenre1->setModel(model_genre1);
    ui->cBoxGenre1->setModelColumn(0); // Display the first column ('name')

    ui->cBoxGenre2->setModel(model_genre1); // Share the same model
    ui->cBoxGenre2->setModelColumn(0);

    // Also update the random add genre combo box if it uses the same list
    delete ui->comboBox_random_add_genre->model(); // Assuming it should also be updated
    ui->comboBox_random_add_genre->setModel(model_genre1);
    ui->comboBox_random_add_genre->setModelColumn(0);

    qInfo() << "Genre combo boxes updated.";

    qInfo() << "Finished updating tables.";
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

QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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
                    checkDbOpen();
                    QSqlQuery* qry=new QSqlQuery(db);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        //int thisid = index.row()+1;
        ui->jinglesView->selectRow(index.row());
        int rowidx = ui->jinglesView->selectionModel()->currentIndex().row();
        checkDbOpen();
        QSqlTableModel * model = new QSqlTableModel(this,db);
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
                        checkDbOpen();
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery(db);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        //int thisid = index.row()+1;
        ui->pubView->selectRow(index.row());
        int rowidx = ui->pubView->selectionModel()->currentIndex().row();
checkDbOpen();
        QSqlTableModel * model = new QSqlTableModel(this,db);
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
                        checkDbOpen();
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery(db);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        //int thisid = index.row()+1;
        ui->programsView->selectRow(index.row());
        int rowidx = ui->programsView->selectionModel()->currentIndex().row();
checkDbOpen();
        QSqlTableModel * model = new QSqlTableModel(this,db);
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
                        checkDbOpen();
                      qDebug() << "the file should be deleted from the database cause it does not exist in the hd (or path should change)";
                          QSqlQuery* qry=new QSqlQuery(db);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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
checkDbOpen();
    QSqlQuery runQry(db);
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
               QSqlQuery query(db);
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

                           QSqlQuery numOfItemsInDB(db);
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
            QSqlQuery query(db);
            if(query.exec(querystr))
            {
                qDebug() << "SQL query executed: " << query.lastQuery();

                while(query.next()){
                    QString path = query.value(0).toString();

                    if(path!=lastPlayedSong){
                        ui->playlist->addItem(path);
                        qDebug() << "autoMode genre based random music chooser adding: " << path;

                    } else {

                        QSqlQuery numOfItemsInDB(db);
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

    // Using XplaylistUrls instead of deprecated QMediaPlaylist
    if (!XplaylistUrls.isEmpty()) {
        XplaylistUrls.removeAt(0);
    }
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

                        if (Rxml.name() == QStringLiteral("XFBClientTakeOver")) {
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

                        if(Rxml.name()==QStringLiteral("www.netpack.pt")){
                            qDebug()<<"Token element: "<<Rxml.name();
                            Rxml.readNext();
                        }

                        if(Rxml.name()==QStringLiteral("ip")){
                            takeOverIP = Rxml.readElementText();
                            qDebug()<<"takeOverIP: "<<takeOverIP;
                        }

                        if(Rxml.name()==QStringLiteral("stream")){
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

                        if (Rxml.name() == QStringLiteral("XFBClientTakeOver")) {
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

                        if(Rxml.name()==QStringLiteral("www.netpack.pt")){
                            qDebug()<<"Token element: "<<Rxml.name();
                            Rxml.readNext();
                        }

                        if(Rxml.name()==QStringLiteral("ip")){
                            returnTakeOverIP = Rxml.readElementText();
                            qDebug()<<"returnTakeOverIP: "<<returnTakeOverIP;
                        }

                        if(Rxml.name()==QStringLiteral("cmd")){
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


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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



                QSqlQuery sql(db);
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



                        QSqlQuery sql_add(db);
                        QString qry_add = "insert into programs values(NULL,'"+filename+"','"+file+"')";
                        if(sql_add.exec(qry_add)){
                            qDebug()<<"Query OK. Program localy added to programs table";

                             //schelule it





                            QSqlQuery qryid(db);
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

                                        QSqlQuery qhm(db);

                                        if(qhm.exec(qryhourmin)){

                                            QString def = "def";

                                            while(qhm.next()){
                                                def = "not";
                                                QString pHora = qhm.value(0).toString();
                                                QString pMin = qhm.value(1).toString();

                                                QSqlQuery addsch(db);
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


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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

                    QSqlQuery qry(db);
                    QString thisquery = "insert into programs values(NULL,'"+nomeDoPrograma+"','"+fileNameWPath+"')";
                    if(qry.exec(thisquery)){
                        qDebug()<<"server_ftp_check() :: Query OK. Program added to programs table";
                    } else {
                         qDebug()<<"server_ftp_check() :: Query was not ok while atempting to add to the programs table";
                    }

                    QSqlQuery qryid(db);
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

                                QSqlQuery qhm(db);

                                if(qhm.exec(qryhourmin)){

                                    while(qhm.next()){
                                        QString pHora = qhm.value(0).toString();
                                        QString pMin = qhm.value(1).toString();

                                        QSqlQuery addsch(db);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    checkDbOpen();

    /*check if there is any thing to go on now*/

    qDebug()<<"\nThe scheduller is now monitoring the database (the scheduler table) to check if there is something to go on at "<<QDateTime::currentDateTime().toString()<<"\n";



QSqlQuery sched_qry(db);
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
                    QSqlQuery getPath(db);
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

                    QSqlQuery del_qry(db);
                    del_qry.prepare("delete from scheduler where id='"+schId+"' and ano='"+ano1+"' and mes='"+mes1+"' and dia='"+dia1+"' and ano='"+ano1+"' and hora='"+hora1+"' and min='"+min1+"'");
                    if(del_qry.exec()){
                        qDebug () << "Scheduled rule was deleted!";
                    } else {
                        qDebug()<<"exeption deleting scheduled rule with qry: "<<del_qry.lastQuery()<<" we got: "<<del_qry.lastError();
                    }

                    //check if pub still has other scheduler rules and delete from pub if not
                    QSqlQuery sq(db);
                    sq.prepare("select count(id) from scheduler where id='"+schId+"'");
                    if(sq.exec()){

                        while(sq.next()){
                            qDebug()<<"We have a total of "<<sq.value(0).toString()<<" records in the scheduler table with the schId "<<schId;
                            if(sq.value(0).toString()=="0"){
                                //we can delete it from pub cause no more scheduled rules apply

                                QSqlQuery sd(db);
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
                QSqlQuery getPath(db);
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
    
    // Connect the musicAdded signal to update the music table
    connect(widget, &externaldownloader::musicAdded, this, &player::update_music_table);
    
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QString addG1 = "";
    QString addG2 = "";

    bool g1_checked = ui->checkBox_filter_genre1->checkState(); //true or false
    bool g2_checked = ui->checkBox_filter_genre2->checkState();

    qDebug () << "132426032015 " << g1_checked << " : " << g2_checked;

    if(g1_checked == true){
        qDebug()<<"g1 is checked";
        QString selectedGenre1 = ui->cBoxGenre1->currentText();
        addG1 = " genre1='"+selectedGenre1+"' ";










        QSqlQuery sql(db);
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
    
    // Connect the musicAdded signal to update the music table
    connect(widget, &externaldownloader::musicAdded, this, &player::update_music_table);
    
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

                    if (Rxml.name() == QStringLiteral("XFBPlaylist")) {
                        qDebug()<<"Valid XFB Playlist Found!";
                        Rxml.readNext();

                        if(Rxml.isEndElement() && Rxml.name() == QLatin1String("XFBPlaylist")){
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

                    if(Rxml.name()==QStringLiteral("www.netpack.pt")){
                        qDebug()<<"Token element: "<<Rxml.name();
                        Rxml.readNext();
                    }

                    if(Rxml.name()==QStringLiteral("track")){
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

    // Find the requested audio input device
    const QList<QAudioDevice> inputDevices = QMediaDevices::audioInputs();
    QAudioDevice selectedDevice;

    for (const QAudioDevice &device : inputDevices) {
        if (device.description() == recDevice) {
            selectedDevice = device;
            break;
        }
    }

    if (selectedDevice.isNull()) {
        // If the specific device wasn't found, use the default device
        selectedDevice = QMediaDevices::defaultAudioInput();
    }

    // Set the audio input device
    audioInput->setDevice(selectedDevice);
    qDebug() << "Selecting this audio input device: " << selectedDevice.description();

    // Configure media recorder settings
    QMediaFormat format;

    // Set container format
    aExtencaoDesteCoiso = "ogg";

    if(contentamento=="matroska") {
        format.setFileFormat(QMediaFormat::FileFormat::Matroska);
        aExtencaoDesteCoiso = "mp3";
    } else if(contentamento=="ogg") {
        format.setFileFormat(QMediaFormat::FileFormat::Ogg);
        aExtencaoDesteCoiso = "ogg";
    } else if(contentamento=="mp4") {
        format.setFileFormat(QMediaFormat::FileFormat::MPEG4);
        aExtencaoDesteCoiso = "mp4";
    } else if(contentamento=="wav") {
        format.setFileFormat(QMediaFormat::FileFormat::Wave);
        aExtencaoDesteCoiso = "wav";
    } else if(contentamento=="quicktime") {
        format.setFileFormat(QMediaFormat::FileFormat::QuickTime);
        aExtencaoDesteCoiso = "mov";
    } else {
        // Default to Ogg if format not recognized
        format.setFileFormat(QMediaFormat::FileFormat::Ogg);
        aExtencaoDesteCoiso = "ogg";
    }

    // Set audio codec (if needed)
    if (!codec.isEmpty()) {
        if (codec == "audio/vorbis") {
            format.setAudioCodec(QMediaFormat::AudioCodec::Vorbis);
        } else if (codec == "audio/opus") {
            format.setAudioCodec(QMediaFormat::AudioCodec::Opus);
        } else if (codec == "audio/mp3") {
            format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
        } else if (codec == "audio/aac") {
            format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
        }
    }

    // Set quality
    audioRecorder->setQuality(QMediaRecorder::HighQuality);
    audioRecorder->setMediaFormat(format);
    audioRecorder->setOutputLocation(QUrl::fromLocalFile(saveFile));

    // Connect signals for recording state changes
    connect(audioRecorder, &QMediaRecorder::recorderStateChanged, this, [](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::RecordingState) {
            qDebug() << "Recording state: Recording";
        } else if (state == QMediaRecorder::PausedState) {
            qDebug() << "Recording state: Paused";
        } else if (state == QMediaRecorder::StoppedState) {
            qDebug() << "Recording state: Stopped";
        }
    });

    connect(audioRecorder, &QMediaRecorder::errorOccurred, this, [this](QMediaRecorder::Error error, const QString &errorString) {
        // Unused parameter 'error'
        Q_UNUSED(error);

        qDebug() << "Recording error: " << errorString;
        QMessageBox::warning(this, tr("Recording Error"), errorString);
    });

    // Start recording
    audioRecorder->record();

    qDebug() << "Recording started with format: " << format.fileFormat();

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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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

                                            QSqlQuery qry(db);
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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
                          QSqlQuery qry(db);
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
              QSqlQuery qry(db);
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
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    QMessageBox::StandardButton run = QMessageBox::question(this, "Run Database Check?",
                                                            "Run a check on all music records?\n"
                                                            "- Verify file existence\n"
                                                            "- Check for empty files\n"
                                                            "- Update duration from file metadata\n"
                                                            "- Reset play count (if file OK)\n\n"
                                                            "This may take some time.",
                                                            QMessageBox::Yes | QMessageBox::No);
    if (run == QMessageBox::No) {
        return;
    }

    // Use stack-allocated QSqlQuery objects within the loop or prepare outside
    QSqlQuery querySelect(db);
    QSqlQuery queryDelete(db); // For delete operations
    QSqlQuery queryUpdate(db); // For update operations

    // Prepare statements outside the loop for efficiency
    if (!queryDelete.prepare("DELETE FROM musics WHERE path = :path")) {
         qWarning() << "Failed to prepare DELETE statement:" << queryDelete.lastError();
         QMessageBox::critical(this, "Database Error", "Failed to prepare database query (DELETE).");
         return;
    }
    if (!queryUpdate.prepare("UPDATE musics SET time = :time WHERE path = :path")) {
        qWarning() << "Failed to prepare time UPDATE statement:" << queryUpdate.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query (UPDATE time).");
        return;
    }
    // Separate prepare for played_times update
    QSqlQuery queryUpdatePlayedTimes(db);
     if (!queryUpdatePlayedTimes.prepare("UPDATE musics SET played_times = 0 WHERE path = :path")) {
        qWarning() << "Failed to prepare played_times UPDATE statement:" << queryUpdatePlayedTimes.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query (UPDATE played_times).");
        return;
    }


    QString selectStr = "SELECT path, time FROM musics"; // Select only needed columns
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table.");
        return;
    }

    int processedCount = 0;
    int errorsCount = 0;
    int deletedCount = 0;
    int updatedCount = 0;

    while (querySelect.next()) {
        QString path = querySelect.value(0).toString();
        QString currentDbTime = querySelect.value(1).toString(); // Get current time for comparison later if needed

        qInfo().noquote() << "Processing [" << ++processedCount << "]:" << path; // Use qInfo for less verbose default logging

        if (!QFile::exists(path)) {
            qWarning() << "File does not exist:" << path;
            QMessageBox::StandardButton reply = QMessageBox::question(this, "File Missing",
                                                                      QString("File not found:\n%1\n\nDelete this record from the database?")
                                                                          .arg(path),
                                                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (reply == QMessageBox::Cancel) {
                qInfo() << "User cancelled operation.";
                break; // Exit the loop
            } else if (reply == QMessageBox::Yes) {
                queryDelete.bindValue(":path", path);
                if (queryDelete.exec()) {
                    qInfo() << "Record deleted from database (file missing):" << path;
                    deletedCount++;
                    update_music_table(); // Update view immediately
                } else {
                    qWarning() << "Failed to delete record for missing file:" << queryDelete.lastError() << queryDelete.lastQuery();
                    errorsCount++;
                     QMessageBox::warning(this, "Database Error", QString("Failed to delete record for missing file:\n%1\n\nError: %2").arg(path, queryDelete.lastError().text()));
                }
            } else {
                qInfo() << "Skipping missing file record, keeping in DB:" << path;
                errorsCount++; // Count as an issue to resolve
            }
            continue; // Move to the next record
        }

        // File exists, check size
        QFile file(path);
        qint64 fileSize = file.size(); // Check size directly
        qDebug() << "File exists, size:" << fileSize;

        if (fileSize == 0) {
            qWarning() << "File exists but is empty (0 bytes):" << path;
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Empty File",
                                                                      QString("File exists but is empty (0 bytes):\n%1\n\nDelete record from database AND delete the empty file from disk?")
                                                                          .arg(path),
                                                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
             if (reply == QMessageBox::Cancel) {
                qInfo() << "User cancelled operation.";
                break; // Exit the loop
            } else if (reply == QMessageBox::Yes) {
                // 1. Delete from DB first
                queryDelete.bindValue(":path", path);
                if (queryDelete.exec()) {
                    qInfo() << "Record deleted from database (empty file):" << path;
                    deletedCount++;
                    // 2. If DB delete successful, delete file from disk
                    if (QFile::remove(path)) {
                        qInfo() << "Empty file deleted from disk:" << path;
                    } else {
                        qWarning() << "Failed to delete empty file from disk (record was deleted from DB):" << path << file.errorString();
                         QMessageBox::warning(this, "File System Error", QString("Record deleted from DB, but failed to delete empty file from disk:\n%1\n\nError: %2").arg(path, file.errorString()));
                        // Don't increment errorsCount here as the primary goal (DB cleanup) succeeded.
                    }
                    update_music_table(); // Update view immediately
                } else {
                    qWarning() << "Failed to delete record for empty file:" << queryDelete.lastError() << queryDelete.lastQuery();
                    errorsCount++;
                    QMessageBox::warning(this, "Database Error", QString("Failed to delete record for empty file:\n%1\n\nError: %2").arg(path, queryDelete.lastError().text()));
                }
            } else {
                qInfo() << "Skipping empty file record, keeping in DB and on disk:" << path;
                errorsCount++; // Count as an issue to resolve
            }
            continue; // Move to the next record
        }

        // --- File seems OK, get duration using Exiftool (C++ Parsing Method) ---
        qDebug() << "File seems OK. Checking metadata with exiftool...";

        QProcess process;
        QString command = "exiftool"; // Just the command
        QStringList arguments;
        arguments << "-Duration"; // Ask only for the Duration tag for efficiency
        arguments << path;        // QProcess handles quoting arguments

        qDebug() << "Running command:" << command << arguments;

        process.start(command, arguments);

        // Wait for the process to finish (e.g., 10 seconds timeout)
        if (!process.waitForFinished(10000)) {
            qWarning() << "Exiftool process timed out for:" << path << process.errorString();
            process.kill();
            process.waitForFinished(1000); // Wait a bit after killing
            errorsCount++;
            continue; // Skip this file
        }

        // Check for exiftool execution errors
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            qWarning() << "Exiftool process failed for:" << path
                       << "Exit code:" << process.exitCode()
                       << "Exit status:" << process.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(process.readAllStandardError()); // Or fromUtf8
            qWarning() << "Exiftool Standard Error:\n" << errorOutput;
            errorsCount++;
             // Optionally show a non-blocking notification or log this error prominently
            continue; // Skip this file
        }

        // Read ALL standard output from exiftool
        QString exiftoolOutput = QString::fromLocal8Bit(process.readAllStandardOutput()); // Or fromUtf8
        qDebug() << "Exiftool raw output:\n" << exiftoolOutput;

        QString durationValue;
        // Find the line containing "Duration" and extract the value
        QStringList lines = exiftoolOutput.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.simplified().startsWith("Duration", Qt::CaseInsensitive)) {
                int colonPos = line.indexOf(':');
                if (colonPos != -1) {
                    durationValue = line.mid(colonPos + 1).trimmed();
                    qInfo() << "Found Duration:" << durationValue << "for" << path;
                    break; // Found it
                }
            }
        }

        if (durationValue.isEmpty()) {
            qWarning() << "Could not find or parse 'Duration' tag from exiftool output for:" << path;
             QString errorOutput = QString::fromLocal8Bit(process.readAllStandardError()); // Check stderr again
             if (!errorOutput.trimmed().isEmpty()) {
                 qWarning() << "Exiftool stderr for non-duration file:" << errorOutput;
             }
            errorsCount++;
            // Maybe update time to NULL or a special value? For now, just skip update.
        } else {
            // Update the database only if the time is different or needs setting
            if (currentDbTime != durationValue) { // Optional: Avoid unnecessary updates
                 qInfo() << "Updating time in DB from" << currentDbTime << "to" << durationValue << "for" << path;
                 queryUpdate.bindValue(":time", durationValue);
                 queryUpdate.bindValue(":path", path);
                 if (queryUpdate.exec()) {
                     qDebug() << "Time updated successfully in DB.";
                     updatedCount++;
                     update_music_table(); // Update view
                 } else {
                     qWarning() << "Failed to update time in DB:" << queryUpdate.lastError() << queryUpdate.lastQuery();
                     errorsCount++;
                     QMessageBox::warning(this, "Database Error", QString("Failed to update time for:\n%1\n\nError: %2").arg(path, queryUpdate.lastError().text()));
                 }
            } else {
                 qDebug() << "Duration in DB already matches exiftool output. No time update needed.";
            }

             // Reset played_times (original logic) - Do this only if time update was successful or not needed?
             // Let's assume reset should happen if the file is valid, regardless of time update necessity.
            qInfo() << "Resetting played_times to 0 for:" << path;
            queryUpdatePlayedTimes.bindValue(":path", path);
             if (!queryUpdatePlayedTimes.exec()) {
                 qWarning() << "Failed to reset played_times:" << queryUpdatePlayedTimes.lastError() << queryUpdatePlayedTimes.lastQuery();
                 errorsCount++;
                 // Non-critical error, maybe don't show a message box here.
             }
        }
        // --- End Exiftool Processing ---

    } // End while loop

    qInfo() << "-------------------------------------";
    qInfo() << "Database check complete.";
    qInfo() << "Processed:" << processedCount << "records.";
    qInfo() << "Deleted:" << deletedCount << "records.";
    qInfo() << "Updated:" << updatedCount << "times.";
    qInfo() << "Errors/Skipped:" << errorsCount << "records (check warnings above).";
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Check Complete",
                             QString("Database check finished.\n\nProcessed: %1\nDeleted: %2\nTime Updated: %3\nErrors/Skipped: %4\n\nSee application output log for details.")
                                 .arg(processedCount)
                                 .arg(deletedCount)
                                 .arg(updatedCount)
                                 .arg(errorsCount));

    // Final table update might be redundant if called after every change, but safe to have.
    update_music_table();
}

void player::calculate_playlist_total_time() {
    qint64 totalSeconds = 0; // Use qint64 for total seconds to prevent overflow
    int failedFiles = 0;
    int playlistCount = ui->playlist->count();

    qInfo() << "Calculating total time for" << playlistCount << "items in playlist...";

    if (playlistCount == 0) {
        ui->txt_playlistTotalTime->setText("Total time: 00:00:00");
        return;
    }

    for (int i = 0; i < playlistCount; ++i) {
        QListWidgetItem* item = ui->playlist->item(i);
        if (!item) continue; // Should not happen, but safety check

        QString filePath = item->text(); // Assuming the item text is the full path
        qInfo().noquote() << "Processing playlist item [" << i+1 << "/" << playlistCount << "]:" << filePath;

        // --- Optional but recommended: Check if file exists ---
        if (!QFile::exists(filePath)) {
            qWarning() << "File listed in playlist does not exist:" << filePath;
            failedFiles++;
            // Optional: Visually mark the item in the playlist?
            // item->setForeground(Qt::red); // Example
            continue; // Skip to the next item
        }
        // --- End existence check ---


        QProcess process;
        QString command = "exiftool";
        QStringList arguments;
        // Use -T for Tab-separated values (easier parsing) and request only Duration
        // This should output *just* the duration value if found.
        arguments << "-T" << "-Duration" << filePath;

        qDebug() << "Running command:" << command << arguments;

        process.setProcessChannelMode(QProcess::MergedChannels); // Combine stdout and stderr for simpler reading if needed
        process.start(command, arguments);

        // Wait for the process to finish (e.g., 5 seconds timeout per file)
        if (!process.waitForFinished(5000)) {
            qWarning() << "Exiftool process timed out for:" << filePath << process.errorString();
            process.kill();
            process.waitForFinished(1000);
            failedFiles++;
            continue; // Skip this file
        }

        // Check for exiftool execution errors
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            qWarning() << "Exiftool process failed for:" << filePath
                       << "Exit code:" << process.exitCode()
                       << "Exit status:" << process.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(process.readAll()); // Read combined output/error
             qWarning() << "Exiftool output/error:\n" << errorOutput;
            failedFiles++;
            continue; // Skip this file
        }

        // Read the output (should be just the duration value now because of -T)
        QString durationString = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed(); // Or fromUtf8

        if (durationString.isEmpty() || durationString == "-") { // Exiftool might output "-" if tag not found
            qWarning() << "Could not find 'Duration' tag via exiftool for:" << filePath;
            failedFiles++;
            continue; // Skip this file
        }

        qDebug() << "Found Duration string:" << durationString << "for" << filePath;

        // --- Parse the duration string (HH:MM:SS.ss or SS.ss) into seconds ---
        qint64 currentTrackSeconds = 0;
        bool parseOk = false;

        if (durationString.contains(':')) {
            // Format HH:MM:SS.ss
            QStringList parts = durationString.split(':');
            if (parts.size() == 3) {
                bool hOk, mOk, sOk;
                int hours = parts[0].toInt(&hOk);
                int minutes = parts[1].toInt(&mOk);
                // Seconds might have fraction, take integer part
                double secondsDouble = parts[2].toDouble(&sOk);
                int seconds = static_cast<int>(secondsDouble); // Truncate fraction

                if (hOk && mOk && sOk && hours >= 0 && minutes >= 0 && minutes < 60 && seconds >= 0 && seconds < 60) {
                    currentTrackSeconds = (hours * 3600) + (minutes * 60) + seconds;
                    parseOk = true;
                }
            }
        } else {
            // Format SS.ss (seconds only)
            bool sOk;
            double secondsDouble = durationString.toDouble(&sOk);
            if (sOk && secondsDouble >= 0) {
                currentTrackSeconds = static_cast<qint64>(secondsDouble); // Truncate fraction
                parseOk = true;
            }
        }

        if (parseOk) {
            qDebug() << "Parsed seconds:" << currentTrackSeconds;
            totalSeconds += currentTrackSeconds;
        } else {
            qWarning() << "Failed to parse duration string:" << durationString << "for file:" << filePath;
            failedFiles++;
        }
        // --- End duration parsing ---

    } // End for loop

    // --- Format total time ---
    qint64 finalHours = totalSeconds / 3600;
    qint64 finalMinutes = (totalSeconds % 3600) / 60;
    qint64 finalSeconds = totalSeconds % 60;

    // Format as HH:MM:SS with leading zeros
    QString finalTimeString = QStringLiteral("Total time: %1:%2:%3")
                                  .arg(finalHours, 2, 10, QChar('0'))   // At least 2 digits, base 10, pad with '0'
                                  .arg(finalMinutes, 2, 10, QChar('0'))
                                  .arg(finalSeconds, 2, 10, QChar('0'));

    if (failedFiles > 0) {
        finalTimeString += QString(" (%1 item(s) failed)").arg(failedFiles);
         qWarning() << "Finished calculating playlist time, but failed to get duration for" << failedFiles << "items.";
    }

    qInfo() << "Final calculated playlist time string:" << finalTimeString;
    ui->txt_playlistTotalTime->setText(finalTimeString);
}
void player::on_actionCheck_Database_Data_and_DELETE_all_invalid_records_witouth_confirmation_triggered()
{


    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QMessageBox::StandardButton run;
    run = QMessageBox::question(this,"Sure?","Are you sure you want to: Run a check on all the records in the music table; For each check if it exist AND DELETE WITHOUT ASKING FOR CONFIRMATION and update the time info of the song",
                                QMessageBox::Yes|QMessageBox::No);
    if(run==QMessageBox::Yes){



        QSqlQuery qr(db);
        QString qrstr = "select * from musics";
        if(qr.exec(qrstr)){
            while(qr.next()){

                QString path = qr.value(7).toString();
                qDebug()<<"Processing: "<<path;

                bool ha = QFile::exists(path);
                if(!ha){


                    qDebug()<<"Deleting "<<path;

                          QSqlQuery* qry=new QSqlQuery(db);
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

                              QSqlQuery* qry=new QSqlQuery(db);
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

    // Using lp1_XplaylistUrls instead of deprecated QMediaPlaylist
    lp1_XplaylistUrls.clear();
    lp1_XplaylistUrls.append(QUrl::fromLocalFile(ui->lp_1_txt_file->text()));
    lp1_Xplayer->setSource(lp1_XplaylistUrls.first());
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
    // Using lp2_XplaylistUrls instead of deprecated QMediaPlaylist
    lp2_XplaylistUrls.clear();
    lp2_XplaylistUrls.append(QUrl::fromLocalFile(ui->lp_2_txt_file->text()));
    lp2_Xplayer->setSource(lp2_XplaylistUrls.first());
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
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- Check for SOX executable ---
    QString soxPath = QStandardPaths::findExecutable("sox");
    if (soxPath.isEmpty()) {
        qWarning() << "'sox' command not found in system PATH.";
        QMessageBox::critical(this, "Missing Dependency",
                              "The 'sox' command (Sound eXchange) is required for this feature "
                              "but was not found in the system's PATH.\n\nPlease install SoX and ensure it's accessible.");
        return;
    }
    qInfo() << "Found sox executable at:" << soxPath;
    // --- End SOX check ---

    QMessageBox::StandardButton run = QMessageBox::question(this, "Confirm Auto-Trim",
                                                            "This will attempt to trim silence (below 1% threshold) from the start and end of every track in the database using 'sox'.\n\n"
                                                            "Original files will be overwritten!\n\n"
                                                            "This process can take a long time and is irreversible.\n"
                                                            "Are you sure you want to proceed?",
                                                            QMessageBox::Yes | QMessageBox::No);
    if (run == QMessageBox::No) {
        return;
    }

    // --- Setup Loading Indicator & Progress Dialog ---
    QMovie loadingMovie(":/images/loading.gif"); // Use stack variable if movie is temporary
    if(!loadingMovie.isValid()){
         qWarning() << "Loading GIF not valid:" << ":/images/loading.gif";
         // Continue without movie, maybe show text?
         ui->txt_loading->setText("Processing...");
         ui->txt_loading->show();
    } else {
        ui->txt_loading->setMovie(&loadingMovie);
        ui->txt_loading->setScaledContents(true);
        ui->txt_loading->show();
        loadingMovie.start();
    }
    qApp->processEvents(); // Allow UI to update

    QProgressDialog progressDialog("Trimming silence...", "Cancel", 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();
    qApp->processEvents();
    // --- End Setup ---


    QSqlQuery querySelect(db);
    QString selectStr = "SELECT path FROM musics"; // Only need the path
    int totalFiles = 0;

    // First count the files for the progress bar
    if (querySelect.exec("SELECT COUNT(*) FROM musics")) {
        if (querySelect.next()) {
            totalFiles = querySelect.value(0).toInt();
        }
    } else {
         qWarning() << "Failed to count records:" << querySelect.lastError();
         // Proceed without accurate progress max? Or abort? Let's try proceeding.
    }
    progressDialog.setMaximum(totalFiles > 0 ? totalFiles : 100); // Set max for progress bar
    qApp->processEvents();


    // Now get the paths
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT paths from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table for paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;
    bool cancelled = false;

    QTemporaryDir tempDir; // Creates a unique temporary directory
    if (!tempDir.isValid()) {
        qWarning() << "Failed to create temporary directory:" << tempDir.errorString();
        QMessageBox::critical(this, "File System Error", "Could not create a temporary directory to process files.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }
    qInfo() << "Using temporary directory:" << tempDir.path();


    while (querySelect.next() && !cancelled) {
        processedCount++;
        progressDialog.setValue(processedCount);
        QString originalPath = querySelect.value(0).toString();
        QFileInfo originalFileInfo(originalPath);

        progressDialog.setLabelText(QString("Processing [%1/%2]:\n%3")
                                        .arg(processedCount)
                                        .arg(totalFiles)
                                        .arg(originalFileInfo.fileName()));
        qApp->processEvents(); // Keep UI responsive and check for Cancel button

        if (progressDialog.wasCanceled()) {
            qInfo() << "User cancelled the operation.";
            cancelled = true;
            break;
        }

        qInfo().noquote() << "AutoTrim Processing [" << processedCount << "/" << totalFiles << "]:" << originalPath;

        if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
            qWarning() << "File does not exist or is not a regular file:" << originalPath;
            failCount++;
            continue; // Skip to next file
        }

        // --- Prepare temporary output path ---
        // Use original filename but in the temporary directory
        QString tempOutputPath = tempDir.filePath(originalFileInfo.fileName());
        qDebug() << "Temporary output path:" << tempOutputPath;

        // --- Run SOX ---
        QProcess soxProcess;
        QStringList soxArgs;
        soxArgs << originalPath                   // Input file
                << tempOutputPath                 // Output file
                << "silence"                      // Effect name
                << "1"                            // Detect silence around the file (mode)
                << "0.1"                          // Stop point (duration in seconds)
                << "1%";                          // Threshold (e.g., 0.1% or 1%) - NOTE: You had 0.1% in question, 1% in code. Using 1% as per code.

        qDebug() << "Running command:" << soxPath << soxArgs;

        soxProcess.start(soxPath, soxArgs);

        // Wait for SOX to finish (adjust timeout if needed, -1 means wait indefinitely)
        // Use a reasonable timeout, e.g., 5 minutes (300000 ms) for potentially large files
        if (!soxProcess.waitForFinished(300000)) {
            qWarning() << "Sox process timed out for:" << originalPath << soxProcess.errorString();
            soxProcess.kill();
            soxProcess.waitForFinished(1000); // Wait a bit after killing
            failCount++;
            continue; // Skip to next file
        }

        // Check SOX execution result
        if (soxProcess.exitStatus() != QProcess::NormalExit || soxProcess.exitCode() != 0) {
            qWarning() << "Sox process failed for:" << originalPath
                       << "Exit code:" << soxProcess.exitCode()
                       << "Exit status:" << soxProcess.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(soxProcess.readAllStandardError());
            qWarning() << "Sox Standard Error:\n" << errorOutput;
            // Clean up potentially incomplete temp file before continuing
            QFile::remove(tempOutputPath);
            failCount++;
            continue; // Skip to next file
        }

        // --- Check if temp file was created and is valid ---
        QFileInfo tempFileInfo(tempOutputPath);
        if (!tempFileInfo.exists() || tempFileInfo.size() == 0) {
             qWarning() << "Sox finished successfully but the output file is missing or empty:" << tempOutputPath;
             // Clean up potentially empty temp file
             QFile::remove(tempOutputPath);
             failCount++;
             continue; // Skip to next file
        }

        qDebug() << "Sox completed successfully. Temp file size:" << tempFileInfo.size();

        // --- Replace original file with trimmed version ---
        // QFile::rename will overwrite the destination if it exists on most platforms.
        // This is atomic on many filesystems (safer than delete then move).
        bool renamed = QFile::rename(tempOutputPath, originalPath);

        if (!renamed) {
            // Rename failed. Maybe permissions issue, file lock, cross-device move?
            // Try copy & delete as a fallback (less safe, not atomic)
            qWarning() << "QFile::rename failed for" << tempOutputPath << "to" << originalPath << ". Attempting copy and delete.";
            if (QFile::copy(tempOutputPath, originalPath)) {
                 qDebug() << "Copy successful. Now removing temporary file.";
                 if (!QFile::remove(tempOutputPath)) {
                      qWarning() << "Failed to remove temporary file after successful copy:" << tempOutputPath;
                      // Continue, main goal achieved, but temp file left behind.
                 }
                 successCount++;
                 qInfo() << "File trimmed and replaced (using copy/delete):" << originalPath;
            } else {
                qWarning() << "Fallback copy failed for:" << tempOutputPath << "to" << originalPath;
                // Clean up temp file if copy failed
                QFile::remove(tempOutputPath);
                failCount++;
                continue; // Skip to next file
            }
        } else {
            // Rename successful! The temp file is now gone (moved).
            successCount++;
            qInfo() << "File trimmed and replaced (using rename):" << originalPath;
        }
        // --- End Replace ---

    } // End while loop

    // --- Cleanup and Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close the progress dialog cleanly

    // Temporary directory (`tempDir`) is automatically removed here when it goes out of scope.

    QString summaryMessage;
    if (cancelled) {
         summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                             .arg(processedCount -1) // Don't count the one being processed when cancelled
                             .arg(successCount)
                             .arg(failCount);
    } else {
         summaryMessage = QString("Auto-Trim Complete.\n\nTotal Records: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                             .arg(totalFiles)
                             .arg(successCount)
                             .arg(failCount);
    }

    qInfo() << "-------------------------------------";
    qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Operation Summary", summaryMessage);
}

void player::on_actionUpdate_System_triggered()
{
    checkForUpdates();
}

void player::on_bt_apply_multi_selection_clicked()
{

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
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
                               QSqlQuery sql(db);

                               if(sql.exec(qry)){
                                   qDebug()<<"Track removed from the database: "<<thisfilename;
                               } else {
                                   QMessageBox::critical(this,tr("Error"),sql.lastError().text());
                                   qDebug() << "last sql: " << sql.lastQuery();
                               }


                               if (rm == QMessageBox::Yes) {
                                   qInfo() << "User confirmed deletion. Attempting to delete file from disk:" << thisfilename;

                                   // --- Use Qt's QFile::remove for platform-independent deletion ---

                                   // Optional: Check existence first, though remove() handles non-existent files gracefully (returns false)
                                   if (!QFile::exists(thisfilename)) {
                                       qWarning() << "File no longer exists or path is incorrect:" << thisfilename;
                                       // You might want to inform the user or just proceed as if deleted
                                       QMessageBox::information(this, "File Not Found",
                                                                QString("The file\n%1\ncould not be found. It might have been deleted already.")
                                                                    .arg(thisfilename));
                                   } else {
                                       // Attempt the deletion
                                       bool success = QFile::remove(thisfilename);

                                       if (success) {
                                           qInfo() << "File successfully deleted from disk:" << thisfilename;
                                           // Optional: A small confirmation message if desired, often logging is enough.
                                           // QMessageBox::information(this, "Success", QString("File deleted:\n%1").arg(thisfilename));
                                       } else {
                                           // Deletion failed - Log the error and inform the user
                                           // Note: QFile::errorString() isn't always set reliably after remove(),
                                           //       so checking the return value is the primary way.
                                           qWarning() << "Failed to delete file from disk:" << thisfilename;
                                           QMessageBox::warning(this, "Deletion Failed",
                                                                QString("Could not delete the file:\n%1\n\nPlease check file permissions or if the file is currently in use.")
                                                                    .arg(thisfilename));
                                       }
                                   }
                                   // --- End of QFile::remove usage ---

                               } else {
                                   qDebug() << "User cancelled file deletion for:" << thisfilename;
                               }







                           }

                       }


                       update_music_table();





                   }
                  }










   }



   if(accao==3){


       QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
       if (!db.isOpen()) {
           qWarning() << "Database connection 'xfb_connection' is not open!";
           QMessageBox::critical(this, "Database Error", "Database connection is not open.");
           return;
       }

       // --- Check for FFMPEG executable ---
       QString ffmpegPath = QStandardPaths::findExecutable("ffmpeg");
       if (ffmpegPath.isEmpty()) {
           qWarning() << "'ffmpeg' command not found in system PATH.";
           QMessageBox::critical(this, "Missing Dependency",
                                 "The 'ffmpeg' command is required for audio conversion "
                                 "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
           return;
       }
       qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
       // --- End FFMPEG check ---


       // --- Get Selected Files ---
       QModelIndexList selectedIndexes = ui->musicView->selectionModel()->selectedIndexes();
       QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
       for (const QModelIndex &index : selectedIndexes) {
           uniqueRows.insert(index.row());
       }

       if (uniqueRows.isEmpty()) {
           QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to convert.");
           return;
       }

       int totalFilesToProcess = uniqueRows.size();
       qInfo() << "Found" << totalFilesToProcess << "unique rows selected for conversion.";

       // --- Confirmation ---
       QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Conversion",
                                        QString("Convert %1 selected track(s) to MP3 (192kbps)?\n\n"
                                                "Original files will be replaced with the MP3 version.\n"
                                                "This action cannot be undone.\n\n"
                                                "Note: Only the audio stream will be kept.")
                                            .arg(totalFilesToProcess),
                                        QMessageBox::Yes | QMessageBox::No);

       if (confirm == QMessageBox::No) {
           return;
       }

       // --- Setup Progress Dialog ---
       QProgressDialog progressDialog("Converting to MP3...", "Cancel", 0, totalFilesToProcess, this);
       progressDialog.setWindowModality(Qt::WindowModal);
       progressDialog.setValue(0);
       progressDialog.show();
       qApp->processEvents(); // Allow UI to update

       // --- Prepare Database Query (outside loop) ---
       QSqlQuery queryUpdate(db);
       if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
           qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
           QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
           progressDialog.cancel();
           return;
       }

       // --- Get Temporary Directory ---
       QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
       if (tempDirPath.isEmpty()) {
            qWarning() << "Could not find a writable temporary location.";
            QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
            progressDialog.cancel();
            return;
       }
       QDir tempDir(tempDirPath);
       qInfo() << "Using temporary directory:" << tempDirPath;


       // --- Process Files ---
       int successCount = 0;
       int failCount = 0;
       bool cancelled = false;

       // Convert QSet to QList to iterate in a defined order (optional, but often useful)
       QList<int> rowsToProcess = uniqueRows.values();
       std::sort(rowsToProcess.begin(), rowsToProcess.end()); // Sort rows numerically

       for (int row : rowsToProcess) {
           if (progressDialog.wasCanceled()) {
               cancelled = true;
               break;
           }

           progressDialog.setValue(successCount + failCount); // Update progress
           qApp->processEvents();

           // Get original file path from model (assuming column 7)
           QModelIndex pathIndex = ui->musicView->model()->index(row, 7);
           QString originalPath = ui->musicView->model()->data(pathIndex).toString();
           QFileInfo originalFileInfo(originalPath);

           progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                           .arg(successCount + failCount + 1)
                                           .arg(totalFilesToProcess)
                                           .arg(originalFileInfo.fileName()));
           qApp->processEvents();

           qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

           if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
               qWarning() << "Original file does not exist or is not a file:" << originalPath;
               failCount++;
               continue; // Skip to next file
           }

           // --- Prepare Paths ---
           QString baseName = originalFileInfo.baseName(); // Name without extension
           QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
           QString tempMp3Path = tempDir.filePath(baseName + ".mp3"); // Full path for temporary MP3
           QString finalMp3Path = QDir(originalDir).filePath(baseName + ".mp3"); // Final path for MP3 in original dir

           // --- Run FFMPEG ---
           QProcess ffmpegProcess;
           QStringList ffmpegArgs;
           ffmpegArgs << "-i" << originalPath // Input file
                      << "-vn"               // No video output
                      << "-ar" << "44100"    // Audio sample rate
                      << "-ac" << "2"        // Audio channels (stereo)
                      << "-b:a" << "192k"    // Audio bitrate (use -b:a for modern ffmpeg) - was -ab
                      << "-f" << "mp3"       // Force output format (usually optional)
                      << tempMp3Path;        // Output file path

           qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

           ffmpegProcess.start(ffmpegPath, ffmpegArgs);

           // Wait for ffmpeg (e.g., 10 min timeout for potentially large files/slow conversion)
           if (!ffmpegProcess.waitForFinished(600000)) {
               qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
               ffmpegProcess.kill();
               ffmpegProcess.waitForFinished(1000);
               QFile::remove(tempMp3Path); // Clean up incomplete temp file
               failCount++;
               continue;
           }

           // Check ffmpeg result
           if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
               qWarning() << "ffmpeg process failed for:" << originalPath
                          << "Exit code:" << ffmpegProcess.exitCode()
                          << "Exit status:" << ffmpegProcess.exitStatus();
               QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
               qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
                QFile::remove(tempMp3Path); // Clean up potentially failed temp file
               failCount++;
               continue;
           }

           // Check if temp MP3 exists and has size
            QFileInfo tempMp3Info(tempMp3Path);
            if (!tempMp3Info.exists() || tempMp3Info.size() == 0) {
                qWarning() << "ffmpeg finished successfully but the output MP3 file is missing or empty:" << tempMp3Path;
                QFile::remove(tempMp3Path);
                failCount++;
                continue;
            }

           qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempMp3Path;

           // --- Delete Original File ---
           qInfo() << "Attempting to delete original file:" << originalPath;
           if (!QFile::remove(originalPath)) {
               qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
               // Don't delete the temp MP3 - user might want it
               // Consider moving temp MP3 to a known 'failed' folder? For now, just leave it in temp.
               failCount++;
               continue; // Skip rest of steps for this file
           }
           qInfo() << "Original file deleted successfully.";

           // --- Move Temporary MP3 to Final Location ---
           qInfo() << "Attempting to move" << tempMp3Path << "to" << finalMp3Path;
           if (!QFile::rename(tempMp3Path, finalMp3Path)) {
               qWarning() << "Failed to move temporary MP3" << tempMp3Path << "to" << finalMp3Path << ". The original file was deleted! MP3 remains in temp folder.";
               // This is a problematic state - original is gone, MP3 is stuck in temp.
               // Inform the user strongly.
               QMessageBox::warning(this, "Move Failed",
                                    QString("Failed to move the converted MP3 to its final destination:\n%1\n\n"
                                            "The original file was deleted, but the MP3 remains in the temporary folder:\n%2\n\n"
                                            "Please move it manually and check the database record.")
                                        .arg(finalMp3Path).arg(tempMp3Path));
               failCount++;
               continue; // Cannot update DB path if move failed
           }
           qInfo() << "MP3 moved successfully to:" << finalMp3Path;

           // --- Update Database ---
           qInfo() << "Updating database: set path =" << finalMp3Path << "where path =" << originalPath;
           queryUpdate.bindValue(":new_path", finalMp3Path);
           queryUpdate.bindValue(":old_path", originalPath);

           if (!queryUpdate.exec()) {
               qWarning() << "Failed to update database path for:" << originalPath << "->" << finalMp3Path;
               qWarning() << "DB Error:" << queryUpdate.lastError().text();
               qWarning() << "Last Query:" << queryUpdate.lastQuery(); // See bound values if driver supports it
               // The file was converted and moved, but the DB is out of sync.
                QMessageBox::warning(this, "Database Update Failed",
                                    QString("The file was successfully converted and moved to:\n%1\n\n"
                                            "However, updating the database record failed:\n%2\n\n"
                                            "Please check the database manually.")
                                        .arg(finalMp3Path).arg(queryUpdate.lastError().text()));
               failCount++; // Count as failure since DB update is critical
           } else {
               qInfo() << "Database path updated successfully.";
               successCount++;
               // update_music_table(); // Consider updating only once at the end for performance
           }

       } // End for loop

       // --- Final Cleanup & Summary ---
       progressDialog.cancel(); // Close progress dialog

       update_music_table(); // Update the table view once after all operations

       QString summaryMessage;
       if (cancelled) {
           summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Converted: %1\nFailed/Skipped: %2")
                                .arg(successCount)
                                .arg(failCount);
       } else {
           summaryMessage = QString("Conversion Complete.\n\nTotal Selected: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                                .arg(totalFilesToProcess)
                                .arg(successCount)
                                .arg(failCount);
       }

       qInfo() << "-------------------------------------";
       qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
       qInfo() << "-------------------------------------";

       QMessageBox::information(this, "Conversion Summary", summaryMessage);
   }




   if(accao==4){

       QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
          if (!db.isOpen()) {
              qWarning() << "Database connection 'xfb_connection' is not open!";
              QMessageBox::critical(this, "Database Error", "Database connection is not open.");
              return;
          }

          // --- Check for FFMPEG executable ---
          QString ffmpegPath = QStandardPaths::findExecutable("ffmpeg");
          if (ffmpegPath.isEmpty()) {
              qWarning() << "'ffmpeg' command not found in system PATH.";
              QMessageBox::critical(this, "Missing Dependency",
                                    "The 'ffmpeg' command is required for audio conversion "
                                    "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
              return;
          }
          qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
          // --- End FFMPEG check ---


          // --- Get Selected Files ---
          QModelIndexList selectedIndexes = ui->musicView->selectionModel()->selectedIndexes();
          QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
          for (const QModelIndex &index : selectedIndexes) {
              uniqueRows.insert(index.row());
          }

          if (uniqueRows.isEmpty()) {
              QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to convert.");
              return;
          }

          int totalFilesToProcess = uniqueRows.size();
          qInfo() << "Found" << totalFilesToProcess << "unique rows selected for conversion to Ogg Vorbis.";

          // --- Confirmation ---
          // Adjusted confirmation message for Ogg Vorbis (quality scale 7)
          QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Conversion",
                                           QString("Convert %1 selected track(s) to Ogg Vorbis (Quality ~7)?\n\n"
                                                   "Original files will be replaced with the Ogg version.\n"
                                                   "This action cannot be undone.\n\n"
                                                   "Note: Only the audio stream will be kept.")
                                               .arg(totalFilesToProcess),
                                           QMessageBox::Yes | QMessageBox::No);

          if (confirm == QMessageBox::No) {
              return;
          }

          // --- Setup Progress Dialog ---
          // Adjusted progress dialog title
          QProgressDialog progressDialog("Converting to Ogg Vorbis...", "Cancel", 0, totalFilesToProcess, this);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setValue(0);
          progressDialog.show();
          qApp->processEvents(); // Allow UI to update

          // --- Prepare Database Query (outside loop) ---
          QSqlQuery queryUpdate(db);
          // Using single quotes for string literals in SQL might be less portable
          // but stick to original if required by specific DB. Prepared statements handle this better.
          if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
              qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
              QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
              progressDialog.cancel();
              return;
          }

          // --- Get Temporary Directory ---
          QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
          if (tempDirPath.isEmpty()) {
               qWarning() << "Could not find a writable temporary location.";
               QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
               progressDialog.cancel();
               return;
          }
          QDir tempDir(tempDirPath);
          qInfo() << "Using temporary directory:" << tempDirPath;


          // --- Process Files ---
          int successCount = 0;
          int failCount = 0;
          bool cancelled = false;

          QList<int> rowsToProcess = uniqueRows.values();
          std::sort(rowsToProcess.begin(), rowsToProcess.end());

          for (int row : rowsToProcess) {
              if (progressDialog.wasCanceled()) {
                  cancelled = true;
                  break;
              }

              progressDialog.setValue(successCount + failCount); // Update progress
              qApp->processEvents();

              // Get original file path from model (assuming column 7)
              QModelIndex pathIndex = ui->musicView->model()->index(row, 7);
              QString originalPath = ui->musicView->model()->data(pathIndex).toString();
              QFileInfo originalFileInfo(originalPath);

              // Adjust progress label text
              progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                              .arg(successCount + failCount + 1)
                                              .arg(totalFilesToProcess)
                                              .arg(originalFileInfo.fileName()));
              qApp->processEvents();

              qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

              if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
                  qWarning() << "Original file does not exist or is not a file:" << originalPath;
                  failCount++;
                  continue; // Skip to next file
              }

              // --- Prepare Paths ---
              QString baseName = originalFileInfo.baseName();
              QString originalDir = originalFileInfo.absolutePath();
              // Construct Ogg paths
              QString tempOggPath = tempDir.filePath(baseName + ".ogg");
              QString finalOggPath = QDir(originalDir).filePath(baseName + ".ogg");

              // --- Run FFMPEG ---
              QProcess ffmpegProcess;
              QStringList ffmpegArgs;
              // Adjusted ffmpeg arguments for Ogg Vorbis
              ffmpegArgs << "-i" << originalPath           // Input file
                         << "-c:a" << "libvorbis"        // Audio codec: libvorbis
                         << "-qscale:a" << "7"           // Quality scale (adjust as needed, 5-7 is common)
                         << tempOggPath;                 // Output file path

              qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

              ffmpegProcess.start(ffmpegPath, ffmpegArgs);

              // Wait for ffmpeg (adjust timeout if needed)
              if (!ffmpegProcess.waitForFinished(600000)) { // 10 min timeout
                  qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
                  ffmpegProcess.kill();
                  ffmpegProcess.waitForFinished(1000);
                  QFile::remove(tempOggPath); // Clean up incomplete temp file
                  failCount++;
                  continue;
              }

              // Check ffmpeg result
              if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
                  qWarning() << "ffmpeg process failed for:" << originalPath
                             << "Exit code:" << ffmpegProcess.exitCode()
                             << "Exit status:" << ffmpegProcess.exitStatus();
                  QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
                  qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
                   QFile::remove(tempOggPath); // Clean up potentially failed temp file
                  failCount++;
                  continue;
              }

              // Check if temp Ogg exists and has size
               QFileInfo tempOggInfo(tempOggPath);
               if (!tempOggInfo.exists() || tempOggInfo.size() == 0) {
                   qWarning() << "ffmpeg finished successfully but the output Ogg file is missing or empty:" << tempOggPath;
                   QFile::remove(tempOggPath);
                   failCount++;
                   continue;
               }

              qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempOggPath;

              // --- Delete Original File ---
              qInfo() << "Attempting to delete original file:" << originalPath;
              if (!QFile::remove(originalPath)) {
                  qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
                  failCount++;
                  continue; // Skip rest of steps for this file
              }
              qInfo() << "Original file deleted successfully.";

              // --- Move Temporary Ogg to Final Location ---
              qInfo() << "Attempting to move" << tempOggPath << "to" << finalOggPath;
              if (!QFile::rename(tempOggPath, finalOggPath)) {
                  qWarning() << "Failed to move temporary Ogg" << tempOggPath << "to" << finalOggPath << ". The original file was deleted! Ogg remains in temp folder.";
                  QMessageBox::warning(this, "Move Failed",
                                       QString("Failed to move the converted Ogg file to its final destination:\n%1\n\n"
                                               "The original file was deleted, but the Ogg file remains in the temporary folder:\n%2\n\n"
                                               "Please move it manually and check the database record.")
                                           .arg(finalOggPath).arg(tempOggPath));
                  failCount++;
                  continue; // Cannot update DB path if move failed
              }
              qInfo() << "Ogg file moved successfully to:" << finalOggPath;

              // --- Update Database ---
              qInfo() << "Updating database: set path =" << finalOggPath << "where path =" << originalPath;
              queryUpdate.bindValue(":new_path", finalOggPath);
              queryUpdate.bindValue(":old_path", originalPath);

              if (!queryUpdate.exec()) {
                  qWarning() << "Failed to update database path for:" << originalPath << "->" << finalOggPath;
                  qWarning() << "DB Error:" << queryUpdate.lastError().text();
                  qWarning() << "Last Query:" << queryUpdate.lastQuery();
                  QMessageBox::warning(this, "Database Update Failed",
                                       QString("The file was successfully converted and moved to:\n%1\n\n"
                                               "However, updating the database record failed:\n%2\n\n"
                                               "Please check the database manually.")
                                           .arg(finalOggPath).arg(queryUpdate.lastError().text()));
                  failCount++; // Count as failure since DB update is critical
              } else {
                  qInfo() << "Database path updated successfully.";
                  successCount++;
              }

          } // End for loop

          // --- Final Cleanup & Summary ---
          progressDialog.cancel(); // Close progress dialog

          update_music_table(); // Update the table view once after all operations

          // Adjust summary message
          QString summaryMessage;
          if (cancelled) {
              summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Converted to Ogg: %1\nFailed/Skipped: %2")
                                   .arg(successCount)
                                   .arg(failCount);
          } else {
              summaryMessage = QString("Ogg Conversion Complete.\n\nTotal Selected: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                                   .arg(totalFilesToProcess)
                                   .arg(successCount)
                                   .arg(failCount);
          }

          qInfo() << "-------------------------------------";
          qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
          qInfo() << "-------------------------------------";

          QMessageBox::information(this, "Conversion Summary", summaryMessage);


   }



   if(accao==5){

       //trim

       // --- Check for SOX executable ---
          QString soxPath = QStandardPaths::findExecutable("sox");
          if (soxPath.isEmpty()) {
              qWarning() << "'sox' command not found in system PATH.";
              QMessageBox::critical(this, "Missing Dependency",
                                    "The 'sox' command (Sound eXchange) is required for this feature "
                                    "but was not found in the system's PATH.\n\nPlease install SoX and ensure it's accessible.");
              return;
          }
          qInfo() << "Found sox executable at:" << soxPath;
          // --- End SOX check ---

          // --- Get Selected Files ---
          QItemSelectionModel *selectionModel = ui->musicView->selectionModel();
          if (!selectionModel) {
              qWarning() << "Music view selection model is null.";
              return;
          }
          QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
          QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
          for (const QModelIndex &index : selectedIndexes) {
              uniqueRows.insert(index.row());
          }

          if (uniqueRows.isEmpty()) {
              QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to trim silence.");
              return;
          }

          int totalFilesToProcess = uniqueRows.size();
          qInfo() << "Found" << totalFilesToProcess << "unique rows selected for silence trimming.";

          // --- Confirmation ---
          QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Silence Trim",
                                           QString("Trim silence (below 1% threshold) from the start and end of %1 selected track(s)?\n\n"
                                                   "Original files will be overwritten!\n\n"
                                                   "This action cannot be undone.")
                                               .arg(totalFilesToProcess),
                                           QMessageBox::Yes | QMessageBox::No);

          if (confirm == QMessageBox::No) {
              return;
          }

          // --- Setup Progress Dialog ---
          QProgressDialog progressDialog("Trimming silence...", "Cancel", 0, totalFilesToProcess, this);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setValue(0);
          progressDialog.show();
          qApp->processEvents(); // Allow UI to update

          // --- Get Temporary Directory ---
          // Use QTemporaryDir for automatic cleanup if preferred, or standard temp location
          QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
          if (tempDirPath.isEmpty()) {
               qWarning() << "Could not find a writable temporary location.";
               QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for processing.");
               progressDialog.cancel();
               return;
          }
          QDir tempDir(tempDirPath);
          qInfo() << "Using temporary directory:" << tempDirPath;

          // --- Process Files ---
          int successCount = 0;
          int failCount = 0;
          bool cancelled = false;
          QAbstractItemModel *model = ui->musicView->model();
          if (!model) {
              qWarning() << "Music view model is null.";
              QMessageBox::critical(this, "Internal Error", "Cannot access the data model.");
              progressDialog.cancel();
              return;
          }


          // Convert QSet to QList to iterate in a defined order (optional)
          QList<int> rowsToProcess = uniqueRows.values();
          std::sort(rowsToProcess.begin(), rowsToProcess.end()); // Sort rows numerically

          for (int row : rowsToProcess) {
              if (progressDialog.wasCanceled()) {
                  cancelled = true;
                  break;
              }

              progressDialog.setValue(successCount + failCount); // Update progress
              qApp->processEvents();

              // Get original file path from model (assuming column 7)
              QModelIndex pathIndex = model->index(row, 7); // Assuming column 7 holds the path
              if (!pathIndex.isValid()) {
                   qWarning() << "Invalid model index for row" << row << ", column 7.";
                   failCount++;
                   continue;
              }
              QString originalPath = model->data(pathIndex).toString();
              QFileInfo originalFileInfo(originalPath);

              progressDialog.setLabelText(QString("Trimming [%1/%2]:\n%3")
                                              .arg(successCount + failCount + 1)
                                              .arg(totalFilesToProcess)
                                              .arg(originalFileInfo.fileName()));
              qApp->processEvents();

              qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

              if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
                  qWarning() << "Original file does not exist or is not a file:" << originalPath;
                  failCount++;
                  continue; // Skip to next file
              }

              // --- Prepare Paths ---
              QString baseName = originalFileInfo.fileName(); // Keep full filename for temp
              QString tempOutputPath = tempDir.filePath(baseName); // Construct full path in temp dir
              // Ensure temp filename is unique in case multiple selections have the same filename (though unlikely if paths are unique)
              // A more robust approach might add a unique ID or use QTemporaryFile, but simple temp path often suffices here.
              qDebug() << "Temporary output path:" << tempOutputPath;

              // --- Run SOX ---
              QProcess soxProcess;
              QStringList soxArgs;
              soxArgs << originalPath                   // Input file
                      << tempOutputPath                 // Output file
                      << "silence"                      // Effect name
                      << "1"                            // Detect silence around the file (mode)
                      << "0.1"                          // Stop point (duration in seconds) - from original code
                      << "1%";                          // Threshold (e.g., 0.1% or 1%) - from original code

              qDebug() << "Running command:" << soxPath << soxArgs;

              soxProcess.start(soxPath, soxArgs);

              // Wait for SOX (adjust timeout if needed, -1 means wait indefinitely)
              if (!soxProcess.waitForFinished(300000)) { // 5 min timeout
                  qWarning() << "Sox process timed out for:" << originalPath << soxProcess.errorString();
                  soxProcess.kill();
                  soxProcess.waitForFinished(1000);
                  QFile::remove(tempOutputPath); // Clean up incomplete temp file
                  failCount++;
                  continue;
              }

              // Check SOX execution result
              if (soxProcess.exitStatus() != QProcess::NormalExit || soxProcess.exitCode() != 0) {
                  qWarning() << "Sox process failed for:" << originalPath
                             << "Exit code:" << soxProcess.exitCode()
                             << "Exit status:" << soxProcess.exitStatus();
                  QString errorOutput = QString::fromLocal8Bit(soxProcess.readAllStandardError());
                  qWarning() << "Sox Standard Error:\n" << errorOutput;
                  QFile::remove(tempOutputPath); // Clean up potentially failed temp file
                  failCount++;
                  continue;
              }

              // --- Check if temp file was created and is valid ---
              QFileInfo tempFileInfo(tempOutputPath);
              if (!tempFileInfo.exists() || tempFileInfo.size() == 0) {
                   qWarning() << "Sox finished successfully but the output file is missing or empty:" << tempOutputPath;
                   QFile::remove(tempOutputPath);
                   failCount++;
                   continue;
              }
              // Optional: Check if temp file size is drastically different? Maybe not reliable.

              qInfo() << "Sox completed successfully for:" << originalPath << " -> " << tempOutputPath;

              // --- Replace original file with trimmed version ---
              qInfo() << "Attempting to replace" << originalPath << "with" << tempOutputPath;
              if (!QFile::rename(tempOutputPath, originalPath)) {
                   qWarning() << "Failed to replace original file using rename:" << originalPath << "with temp file:" << tempOutputPath;
                   // Attempt fallback copy/delete might be risky if original perms are the issue
                   // Best to leave the temp file and warn the user.
                   QMessageBox::warning(this, "Replacement Failed",
                                       QString("Could not replace the original file:\n%1\n\n"
                                               "The trimmed version remains in the temporary folder:\n%2\n\n"
                                               "Please check file permissions and replace it manually if desired.")
                                           .arg(originalPath).arg(tempOutputPath));
                  failCount++;
                  // Do NOT delete the temp file in this case - user might want it.
              } else {
                  qInfo() << "File trimmed and replaced successfully:" << originalPath;
                  successCount++;
                  // Temp file is gone (renamed)
              }
              // --- End Replace ---

          } // End for loop

          // --- Final Cleanup & Summary ---
          progressDialog.cancel(); // Close progress dialog

          // Note: If using QTemporaryDir, it cleans up here. If using standard temp location,
          // successfully moved files are gone, failed temp files might remain as per logic above.

          QString summaryMessage;
          if (cancelled) {
              summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Trimmed: %1\nFailed/Skipped: %2")
                                   .arg(successCount)
                                   .arg(failCount);
          } else {
              summaryMessage = QString("Silence Trim Complete.\n\nTotal Selected: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                                   .arg(totalFilesToProcess)
                                   .arg(successCount)
                                   .arg(failCount);
          }

          qInfo() << "-------------------------------------";
          qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
          qInfo() << "-------------------------------------";

          QMessageBox::information(this, "Operation Summary", summaryMessage);


   }



   if(accao==6){

       //trim eXtreme 1
       // --- Check for SOX executable ---
          QString soxPath = QStandardPaths::findExecutable("sox");
          if (soxPath.isEmpty()) {
              qWarning() << "'sox' command not found in system PATH.";
              QMessageBox::critical(this, "Missing Dependency",
                                    "The 'sox' command (Sound eXchange) is required for this feature "
                                    "but was not found in the system's PATH.\n\nPlease install SoX and ensure it's accessible.");
              return;
          }
          qInfo() << "Found sox executable at:" << soxPath;
          // --- End SOX check ---

          // --- Get Selected Files ---
          QItemSelectionModel *selectionModel = ui->musicView->selectionModel();
          if (!selectionModel) {
              qWarning() << "Music view selection model is null.";
              return;
          }
          QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
          QSet<int> uniqueRows; // Use a QSet to get unique row numbers easily
          for (const QModelIndex &index : selectedIndexes) {
              uniqueRows.insert(index.row());
          }

          if (uniqueRows.isEmpty()) {
              QMessageBox::information(this, "No Selection", "Please select one or more tracks in the list to trim silence.");
              return;
          }

          int totalFilesToProcess = uniqueRows.size();
          qInfo() << "Found" << totalFilesToProcess << "unique rows selected for silence trimming (0.2s threshold).";

          // --- Confirmation ---
          // Updated confirmation message slightly
          QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Silence Trim",
                                           QString("Trim silence (0.2s duration, below 1% threshold) from the start and end of %1 selected track(s)?\n\n"
                                                   "Original files will be overwritten!\n\n"
                                                   "This action cannot be undone.")
                                               .arg(totalFilesToProcess),
                                           QMessageBox::Yes | QMessageBox::No);

          if (confirm == QMessageBox::No) {
              return;
          }

          // --- Setup Progress Dialog ---
          QProgressDialog progressDialog("Trimming silence (0.2s threshold)...", "Cancel", 0, totalFilesToProcess, this);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setValue(0);
          progressDialog.show();
          qApp->processEvents(); // Allow UI to update

          // --- Get Temporary Directory ---
          QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
          if (tempDirPath.isEmpty()) {
               qWarning() << "Could not find a writable temporary location.";
               QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for processing.");
               progressDialog.cancel();
               return;
          }
          QDir tempDir(tempDirPath);
          qInfo() << "Using temporary directory:" << tempDirPath;

          // --- Process Files ---
          int successCount = 0;
          int failCount = 0;
          bool cancelled = false;
          QAbstractItemModel *model = ui->musicView->model();
          if (!model) {
              qWarning() << "Music view model is null.";
              QMessageBox::critical(this, "Internal Error", "Cannot access the data model.");
              progressDialog.cancel();
              return;
          }

          QList<int> rowsToProcess = uniqueRows.values();
          std::sort(rowsToProcess.begin(), rowsToProcess.end());

          for (int row : rowsToProcess) {
              if (progressDialog.wasCanceled()) {
                  cancelled = true;
                  break;
              }

              progressDialog.setValue(successCount + failCount);
              qApp->processEvents();

              QModelIndex pathIndex = model->index(row, 7); // Assuming column 7 holds the path
              if (!pathIndex.isValid()) {
                   qWarning() << "Invalid model index for row" << row << ", column 7.";
                   failCount++;
                   continue;
              }
              QString originalPath = model->data(pathIndex).toString();
              QFileInfo originalFileInfo(originalPath);

              progressDialog.setLabelText(QString("Trimming [%1/%2]:\n%3")
                                              .arg(successCount + failCount + 1)
                                              .arg(totalFilesToProcess)
                                              .arg(originalFileInfo.fileName()));
              qApp->processEvents();

              qInfo().noquote() << "Processing [" << (successCount + failCount + 1) << "/" << totalFilesToProcess << "]:" << originalPath;

              if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
                  qWarning() << "Original file does not exist or is not a file:" << originalPath;
                  failCount++;
                  continue;
              }

              // --- Prepare Paths ---
              QString baseName = originalFileInfo.fileName();
              QString tempOutputPath = tempDir.filePath(baseName);
              qDebug() << "Temporary output path:" << tempOutputPath;

              // --- Run SOX ---
              QProcess soxProcess;
              QStringList soxArgs;
              // ***** THE ONLY CHANGE IS HERE *****
              soxArgs << originalPath                   // Input file
                      << tempOutputPath                 // Output file
                      << "silence"                      // Effect name
                      << "1"                            // Detect silence around the file (mode)
                      << "0.2"                          // Stop point (duration in seconds) - MODIFIED
                      << "1%";                          // Threshold (e.g., 0.1% or 1%)

              qDebug() << "Running command:" << soxPath << soxArgs;

              soxProcess.start(soxPath, soxArgs);

              if (!soxProcess.waitForFinished(300000)) { // 5 min timeout
                  qWarning() << "Sox process timed out for:" << originalPath << soxProcess.errorString();
                  soxProcess.kill();
                  soxProcess.waitForFinished(1000);
                  QFile::remove(tempOutputPath);
                  failCount++;
                  continue;
              }

              if (soxProcess.exitStatus() != QProcess::NormalExit || soxProcess.exitCode() != 0) {
                  qWarning() << "Sox process failed for:" << originalPath
                             << "Exit code:" << soxProcess.exitCode()
                             << "Exit status:" << soxProcess.exitStatus();
                  QString errorOutput = QString::fromLocal8Bit(soxProcess.readAllStandardError());
                  qWarning() << "Sox Standard Error:\n" << errorOutput;
                  QFile::remove(tempOutputPath);
                  failCount++;
                  continue;
              }

              // --- Check if temp file was created and is valid ---
              QFileInfo tempFileInfo(tempOutputPath);
              if (!tempFileInfo.exists() || tempFileInfo.size() == 0) {
                   qWarning() << "Sox finished successfully but the output file is missing or empty:" << tempOutputPath;
                   QFile::remove(tempOutputPath);
                   failCount++;
                   continue;
              }

              qInfo() << "Sox completed successfully for:" << originalPath << " -> " << tempOutputPath;

              // --- Replace original file with trimmed version ---
              qInfo() << "Attempting to replace" << originalPath << "with" << tempOutputPath;
              if (!QFile::rename(tempOutputPath, originalPath)) {
                   qWarning() << "Failed to replace original file using rename:" << originalPath << "with temp file:" << tempOutputPath;
                   QMessageBox::warning(this, "Replacement Failed",
                                       QString("Could not replace the original file:\n%1\n\n"
                                               "The trimmed version remains in the temporary folder:\n%2\n\n"
                                               "Please check file permissions and replace it manually if desired.")
                                           .arg(originalPath).arg(tempOutputPath));
                  failCount++;
                  // Do NOT delete the temp file in this case
              } else {
                  qInfo() << "File trimmed and replaced successfully:" << originalPath;
                  successCount++;
                  // Temp file is gone (renamed)
              }
              // --- End Replace ---

          } // End for loop

          // --- Final Cleanup & Summary ---
          progressDialog.cancel(); // Close progress dialog

          // Adjust summary message slightly
          QString summaryMessage;
          if (cancelled) {
              summaryMessage = QString("Operation Cancelled.\n\nSuccessfully Trimmed (0.2s): %1\nFailed/Skipped: %2")
                                   .arg(successCount)
                                   .arg(failCount);
          } else {
              summaryMessage = QString("Silence Trim Complete (0.2s threshold).\n\nTotal Selected: %1\nSuccessfully Trimmed: %2\nFailed/Skipped: %3")
                                   .arg(totalFilesToProcess)
                                   .arg(successCount)
                                   .arg(failCount);
          }

          qInfo() << "-------------------------------------";
          qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
          qInfo() << "-------------------------------------";

          QMessageBox::information(this, "Operation Summary", summaryMessage);


   }





}

void player::on_actionConvert_all_musics_in_the_database_to_mp3_triggered()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- Check for FFMPEG executable ---
    QString ffmpegPath = QStandardPaths::findExecutable("ffmpeg");
    if (ffmpegPath.isEmpty()) {
        qWarning() << "'ffmpeg' command not found in system PATH.";
        QMessageBox::critical(this, "Missing Dependency",
                              "The 'ffmpeg' command is required for audio conversion "
                              "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
        return;
    }
    qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
    // --- End FFMPEG check ---

    // --- Confirmation ---
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Full Conversion",
                                     "Convert ALL tracks in the database to MP3 (192kbps)?\n\n"
                                     "Original files will be replaced with the MP3 version.\n"
                                     "This action cannot be undone and may take a very long time!\n\n"
                                     "Note: Only the audio stream will be kept.",
                                     QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::No) {
        return;
    }

    // --- Setup Loading Indicator & Progress Dialog ---
    // Use stack variable for QMovie if its lifetime is limited to this function
    QMovie loadingMovie(":/images/loading.gif");
    if(!loadingMovie.isValid()){
         qWarning() << "Loading GIF not valid:" << ":/images/loading.gif";
         ui->txt_loading->setText("Processing..."); // Fallback text
         ui->txt_loading->show();
    } else {
        ui->txt_loading->setMovie(&loadingMovie);
        ui->txt_loading->setScaledContents(true);
        ui->txt_loading->show();
        loadingMovie.start();
    }
    qApp->processEvents(); // Allow UI to update

    QProgressDialog progressDialog("Converting all tracks to MP3...", "Cancel", 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();
    qApp->processEvents();
    // --- End Setup ---

    // --- Prepare Database Queries (outside loop) ---
    QSqlQuery querySelect(db);
    QSqlQuery queryUpdate(db);
    QSqlQuery queryCount(db); // For counting total records

    // Prepare UPDATE statement
    if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
        qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // Count total files for progress bar
    int totalFiles = 0;
    if (queryCount.exec("SELECT COUNT(*) FROM musics")) {
        if (queryCount.next()) {
            totalFiles = queryCount.value(0).toInt();
            progressDialog.setMaximum(totalFiles);
        }
    } else {
        qWarning() << "Failed to count records:" << queryCount.lastError();
        // Proceed without accurate progress max? Or abort? Let's proceed.
        progressDialog.setMaximum(100); // Set an arbitrary max
    }
    qApp->processEvents();


    // Select only the path
    QString selectStr = "SELECT path FROM musics";
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT paths from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table for paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // --- Get Temporary Directory ---
    QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDirPath.isEmpty()) {
         qWarning() << "Could not find a writable temporary location.";
         QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
         loadingMovie.stop();
         ui->txt_loading->hide();
         progressDialog.cancel();
         return;
    }
    QDir tempDir(tempDirPath);
    qInfo() << "Using temporary directory:" << tempDirPath;


    // --- Process Files ---
    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;
    bool cancelled = false;

    while (querySelect.next() && !cancelled) {
        processedCount++;
        progressDialog.setValue(processedCount);
        qApp->processEvents(); // Keep UI responsive and check for Cancel button

        if (progressDialog.wasCanceled()) {
            qInfo() << "User cancelled the operation.";
            cancelled = true;
            break;
        }

        QString originalPath = querySelect.value(0).toString();
        QFileInfo originalFileInfo(originalPath);

        progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                        .arg(processedCount)
                                        .arg(totalFiles > 0 ? totalFiles : processedCount) // Show total if known
                                        .arg(originalFileInfo.fileName()));
        qApp->processEvents();

        qInfo().noquote() << "Processing [" << processedCount << "/" << (totalFiles > 0 ? QString::number(totalFiles): "?") << "]:" << originalPath;

        if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
            qWarning() << "Original file does not exist or is not a file:" << originalPath;
            failCount++;
            continue; // Skip to next file
        }

        // --- Prepare Paths ---
        QString baseName = originalFileInfo.baseName(); // Name without extension
        QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
        QString tempMp3Path = tempDir.filePath(baseName + ".mp3"); // Full path for temporary MP3
        QString finalMp3Path = QDir(originalDir).filePath(baseName + ".mp3"); // Final path for MP3 in original dir

        // --- Run FFMPEG ---
        QProcess ffmpegProcess;
        QStringList ffmpegArgs;
        ffmpegArgs << "-i" << originalPath // Input file
                   << "-vn"               // No video output
                   << "-ar" << "44100"    // Audio sample rate
                   << "-ac" << "2"        // Audio channels (stereo)
                   << "-b:a" << "192k"    // Audio bitrate (use -b:a)
                   << "-f" << "mp3"       // Force output format (optional)
                   << tempMp3Path;        // Output file path

        qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

        ffmpegProcess.start(ffmpegPath, ffmpegArgs);

        // Wait for ffmpeg (e.g., 10 min timeout)
        if (!ffmpegProcess.waitForFinished(600000)) {
            qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
            ffmpegProcess.kill();
            ffmpegProcess.waitForFinished(1000);
            QFile::remove(tempMp3Path); // Clean up incomplete temp file
            failCount++;
            continue;
        }

        // Check ffmpeg result
        if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
            qWarning() << "ffmpeg process failed for:" << originalPath
                       << "Exit code:" << ffmpegProcess.exitCode()
                       << "Exit status:" << ffmpegProcess.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
            qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
             QFile::remove(tempMp3Path); // Clean up potentially failed temp file
            failCount++;
            continue;
        }

        // Check if temp MP3 exists and has size
         QFileInfo tempMp3Info(tempMp3Path);
         if (!tempMp3Info.exists() || tempMp3Info.size() == 0) {
             qWarning() << "ffmpeg finished successfully but the output MP3 file is missing or empty:" << tempMp3Path;
             QFile::remove(tempMp3Path);
             failCount++;
             continue;
         }

        qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempMp3Path;

        // --- Delete Original File ---
        qInfo() << "Attempting to delete original file:" << originalPath;
        if (!QFile::remove(originalPath)) {
            qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
            failCount++;
            continue; // Skip rest of steps for this file
        }
        qInfo() << "Original file deleted successfully.";

        // --- Move Temporary MP3 to Final Location ---
        qInfo() << "Attempting to move" << tempMp3Path << "to" << finalMp3Path;
        if (!QFile::rename(tempMp3Path, finalMp3Path)) {
            qWarning() << "Failed to move temporary MP3" << tempMp3Path << "to" << finalMp3Path << ". The original file was deleted! MP3 remains in temp folder.";
            QMessageBox::warning(this, "Move Failed",
                                 QString("Failed to move the converted MP3 to its final destination:\n%1\n\n"
                                         "The original file was deleted, but the MP3 remains in the temporary folder:\n%2\n\n"
                                         "Please move it manually and check the database record.")
                                     .arg(finalMp3Path).arg(tempMp3Path));
            failCount++;
            continue; // Cannot update DB path if move failed
        }
        qInfo() << "MP3 moved successfully to:" << finalMp3Path;

        // --- Update Database ---
        qInfo() << "Updating database: set path =" << finalMp3Path << "where path =" << originalPath;
        queryUpdate.bindValue(":new_path", finalMp3Path);
        queryUpdate.bindValue(":old_path", originalPath);

        if (!queryUpdate.exec()) {
            qWarning() << "Failed to update database path for:" << originalPath << "->" << finalMp3Path;
            qWarning() << "DB Error:" << queryUpdate.lastError().text();
            qWarning() << "Last Query (Bound values might not show):" << queryUpdate.lastQuery();
            QMessageBox::warning(this, "Database Update Failed",
                                 QString("The file was successfully converted and moved to:\n%1\n\n"
                                         "However, updating the database record failed:\n%2\n\n"
                                         "Please check the database manually.")
                                     .arg(finalMp3Path).arg(queryUpdate.lastError().text()));
            failCount++; // Count as failure since DB update is critical
        } else {
            qInfo() << "Database path updated successfully.";
            successCount++;
            // update_music_table(); // Update only once at the end
        }

    } // End while loop

    // --- Final Cleanup & Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close progress dialog

    update_music_table(); // Update the table view once after all operations

    QString summaryMessage;
    if (cancelled) {
        summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                             .arg(processedCount -1) // Don't count the one being processed when cancelled
                             .arg(successCount)
                             .arg(failCount);
    } else {
        summaryMessage = QString("MP3 Conversion Complete.\n\nTotal Records: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                             .arg(totalFiles) // Use the count obtained earlier
                             .arg(successCount)
                             .arg(failCount);
    }

    qInfo() << "-------------------------------------";
    qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Conversion Summary", summaryMessage);
}
void player::on_actionConvert_all_musics_in_the_database_to_ogg_triggered()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection"); // Or pass it in
    if (!db.isOpen()) {
        qWarning() << "Database connection 'xfb_connection' is not open!";
        QMessageBox::critical(this, "Database Error", "Database connection is not open.");
        return;
    }

    // --- Check for FFMPEG executable ---
    QString ffmpegPath = QStandardPaths::findExecutable("ffmpeg");
    if (ffmpegPath.isEmpty()) {
        qWarning() << "'ffmpeg' command not found in system PATH.";
        QMessageBox::critical(this, "Missing Dependency",
                              "The 'ffmpeg' command is required for audio conversion "
                              "but was not found in the system's PATH.\n\nPlease install ffmpeg and ensure it's accessible.");
        return;
    }
    qInfo() << "Found ffmpeg executable at:" << ffmpegPath;
    // --- End FFMPEG check ---

    // --- Confirmation ---
    // Updated confirmation message for Ogg
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm Full Conversion",
                                     "Convert ALL tracks in the database to Ogg Vorbis (Quality ~7)?\n\n"
                                     "Original files will be replaced with the Ogg version.\n"
                                     "This action cannot be undone and may take a very long time!\n\n"
                                     "Note: Only the audio stream will be kept.",
                                     QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::No) {
        return;
    }

    // --- Setup Loading Indicator & Progress Dialog ---
    QMovie loadingMovie(":/images/loading.gif");
    if(!loadingMovie.isValid()){
         qWarning() << "Loading GIF not valid:" << ":/images/loading.gif";
         ui->txt_loading->setText("Processing..."); // Fallback text
         ui->txt_loading->show();
    } else {
        ui->txt_loading->setMovie(&loadingMovie);
        ui->txt_loading->setScaledContents(true);
        ui->txt_loading->show();
        loadingMovie.start();
    }
    qApp->processEvents(); // Allow UI to update

    // Updated progress dialog title
    QProgressDialog progressDialog("Converting all tracks to Ogg Vorbis...", "Cancel", 0, 0, this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setValue(0);
    progressDialog.show();
    qApp->processEvents();
    // --- End Setup ---

    // --- Prepare Database Queries (outside loop) ---
    QSqlQuery querySelect(db);
    QSqlQuery queryUpdate(db);
    QSqlQuery queryCount(db); // For counting total records

    // Prepare UPDATE statement
    if (!queryUpdate.prepare("UPDATE musics SET path = :new_path WHERE path = :old_path")) {
        qWarning() << "Failed to prepare database UPDATE statement:" << queryUpdate.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to prepare database query for updating paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // Count total files for progress bar
    int totalFiles = 0;
    if (queryCount.exec("SELECT COUNT(*) FROM musics")) {
        if (queryCount.next()) {
            totalFiles = queryCount.value(0).toInt();
            progressDialog.setMaximum(totalFiles);
        }
    } else {
        qWarning() << "Failed to count records:" << queryCount.lastError();
        progressDialog.setMaximum(100); // Set an arbitrary max
    }
    qApp->processEvents();


    // Select only the path
    QString selectStr = "SELECT path FROM musics";
    if (!querySelect.exec(selectStr)) {
        qWarning() << "Failed to SELECT paths from musics:" << querySelect.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to query the musics table for paths.");
        loadingMovie.stop();
        ui->txt_loading->hide();
        progressDialog.cancel();
        return;
    }

    // --- Get Temporary Directory ---
    QString tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDirPath.isEmpty()) {
         qWarning() << "Could not find a writable temporary location.";
         QMessageBox::critical(this, "File System Error", "Cannot find a suitable temporary directory for conversion.");
         loadingMovie.stop();
         ui->txt_loading->hide();
         progressDialog.cancel();
         return;
    }
    QDir tempDir(tempDirPath);
    qInfo() << "Using temporary directory:" << tempDirPath;


    // --- Process Files ---
    int processedCount = 0;
    int successCount = 0;
    int failCount = 0;
    bool cancelled = false;

    while (querySelect.next() && !cancelled) {
        processedCount++;
        progressDialog.setValue(processedCount);
        qApp->processEvents(); // Keep UI responsive and check for Cancel button

        if (progressDialog.wasCanceled()) {
            qInfo() << "User cancelled the operation.";
            cancelled = true;
            break;
        }

        QString originalPath = querySelect.value(0).toString();
        QFileInfo originalFileInfo(originalPath);

        // Updated progress label
        progressDialog.setLabelText(QString("Converting [%1/%2]:\n%3")
                                        .arg(processedCount)
                                        .arg(totalFiles > 0 ? totalFiles : processedCount) // Show total if known
                                        .arg(originalFileInfo.fileName()));
        qApp->processEvents();

        qInfo().noquote() << "Processing [" << processedCount << "/" << (totalFiles > 0 ? QString::number(totalFiles): "?") << "]:" << originalPath;

        if (!originalFileInfo.exists() || !originalFileInfo.isFile()) {
            qWarning() << "Original file does not exist or is not a file:" << originalPath;
            failCount++;
            continue; // Skip to next file
        }

        // --- Prepare Paths ---
        QString baseName = originalFileInfo.baseName(); // Name without extension
        QString originalDir = originalFileInfo.absolutePath(); // Directory of original file
        // Ogg paths
        QString tempOggPath = tempDir.filePath(baseName + ".ogg"); // Full path for temporary Ogg
        QString finalOggPath = QDir(originalDir).filePath(baseName + ".ogg"); // Final path for Ogg in original dir

        // --- Run FFMPEG ---
        QProcess ffmpegProcess;
        QStringList ffmpegArgs;
        // Args for Ogg Vorbis
        ffmpegArgs << "-i" << originalPath           // Input file
                   << "-c:a" << "libvorbis"        // Audio codec: libvorbis
                   << "-qscale:a" << "7"           // Quality scale (adjust as needed)
                   << tempOggPath;                 // Output file path

        qDebug() << "Running command:" << ffmpegPath << ffmpegArgs;

        ffmpegProcess.start(ffmpegPath, ffmpegArgs);

        // Wait for ffmpeg (e.g., 10 min timeout)
        if (!ffmpegProcess.waitForFinished(600000)) {
            qWarning() << "ffmpeg process timed out for:" << originalPath << ffmpegProcess.errorString();
            ffmpegProcess.kill();
            ffmpegProcess.waitForFinished(1000);
            QFile::remove(tempOggPath); // Clean up incomplete temp file
            failCount++;
            continue;
        }

        // Check ffmpeg result
        if (ffmpegProcess.exitStatus() != QProcess::NormalExit || ffmpegProcess.exitCode() != 0) {
            qWarning() << "ffmpeg process failed for:" << originalPath
                       << "Exit code:" << ffmpegProcess.exitCode()
                       << "Exit status:" << ffmpegProcess.exitStatus();
            QString errorOutput = QString::fromLocal8Bit(ffmpegProcess.readAllStandardError());
            qWarning() << "ffmpeg Standard Error:\n" << errorOutput;
             QFile::remove(tempOggPath); // Clean up potentially failed temp file
            failCount++;
            continue;
        }

        // Check if temp Ogg exists and has size
         QFileInfo tempOggInfo(tempOggPath);
         if (!tempOggInfo.exists() || tempOggInfo.size() == 0) {
             qWarning() << "ffmpeg finished successfully but the output Ogg file is missing or empty:" << tempOggPath;
             QFile::remove(tempOggPath);
             failCount++;
             continue;
         }

        qInfo() << "ffmpeg conversion successful for:" << originalPath << " -> " << tempOggPath;

        // --- Delete Original File ---
        qInfo() << "Attempting to delete original file:" << originalPath;
        if (!QFile::remove(originalPath)) {
            qWarning() << "Failed to delete original file:" << originalPath << ". Skipping move and database update.";
            failCount++;
            continue; // Skip rest of steps for this file
        }
        qInfo() << "Original file deleted successfully.";

        // --- Move Temporary Ogg to Final Location ---
        qInfo() << "Attempting to move" << tempOggPath << "to" << finalOggPath;
        if (!QFile::rename(tempOggPath, finalOggPath)) {
            qWarning() << "Failed to move temporary Ogg" << tempOggPath << "to" << finalOggPath << ". The original file was deleted! Ogg remains in temp folder.";
            QMessageBox::warning(this, "Move Failed",
                                 QString("Failed to move the converted Ogg file to its final destination:\n%1\n\n"
                                         "The original file was deleted, but the Ogg file remains in the temporary folder:\n%2\n\n"
                                         "Please move it manually and check the database record.")
                                     .arg(finalOggPath).arg(tempOggPath));
            failCount++;
            continue; // Cannot update DB path if move failed
        }
        qInfo() << "Ogg file moved successfully to:" << finalOggPath;

        // --- Update Database ---
        qInfo() << "Updating database: set path =" << finalOggPath << "where path =" << originalPath;
        queryUpdate.bindValue(":new_path", finalOggPath);
        queryUpdate.bindValue(":old_path", originalPath);

        if (!queryUpdate.exec()) {
            qWarning() << "Failed to update database path for:" << originalPath << "->" << finalOggPath;
            qWarning() << "DB Error:" << queryUpdate.lastError().text();
            qWarning() << "Last Query (Bound values might not show):" << queryUpdate.lastQuery();
            QMessageBox::warning(this, "Database Update Failed",
                                 QString("The file was successfully converted and moved to:\n%1\n\n"
                                         "However, updating the database record failed:\n%2\n\n"
                                         "Please check the database manually.")
                                     .arg(finalOggPath).arg(queryUpdate.lastError().text()));
            failCount++; // Count as failure since DB update is critical
        } else {
            qInfo() << "Database path updated successfully.";
            successCount++;
            // update_music_table(); // Update only once at the end
        }

    } // End while loop

    // --- Final Cleanup & Summary ---
    loadingMovie.stop();
    ui->txt_loading->hide();
    progressDialog.cancel(); // Close progress dialog

    update_music_table(); // Update the table view once after all operations

    // Updated summary message
    QString summaryMessage;
    if (cancelled) {
        summaryMessage = QString("Operation Cancelled.\n\nProcessed: %1\nSuccessfully Converted to Ogg: %2\nFailed/Skipped: %3")
                             .arg(processedCount -1)
                             .arg(successCount)
                             .arg(failCount);
    } else {
        summaryMessage = QString("Ogg Conversion Complete.\n\nTotal Records: %1\nSuccessfully Converted: %2\nFailed/Skipped: %3")
                             .arg(totalFiles) // Use the count obtained earlier
                             .arg(successCount)
                             .arg(failCount);
    }

    qInfo() << "-------------------------------------";
    qInfo() << summaryMessage.replace("\n\n", " | "); // Log summary concisely
    qInfo() << "-------------------------------------";

    QMessageBox::information(this, "Conversion Summary", summaryMessage);
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
    if(stimer) stimer->stop();
    if(icetimer) icetimer->stop();
    if(butt_timer) butt_timer->stop();

    qInfo() << "Attempting to stop streaming processes...";

    // --- Stop Icecast ---
    QProcess killer_icecast;
    QString icecast_cmd;
    QStringList icecast_args;
    bool kill_success_icecast = false;

#ifdef Q_OS_WIN
    icecast_cmd = "taskkill";
    icecast_args << "/F" << "/IM" << "icecast.exe"; // Adjust executable name if needed
#else // Linux, macOS, other Unix-like
    // Prefer pkill if available, fallback to killall might be needed if pkill isn't standard
    icecast_cmd = "pkill"; // Or "killall" as a fallback
    icecast_args << "icecast"; // Process name
#endif

    qDebug() << "Running kill command for icecast:" << icecast_cmd << icecast_args;
    killer_icecast.start(icecast_cmd, icecast_args);
    if (killer_icecast.waitForFinished(3000)) { // Wait up to 3 seconds
        if (killer_icecast.exitStatus() == QProcess::NormalExit && killer_icecast.exitCode() == 0) {
            qInfo() << "Icecast process killed successfully (or was not running).";
            kill_success_icecast = true; // Assume success if command ran ok
        } else {
            qWarning() << "Kill command for icecast failed or process not found. Exit code:" << killer_icecast.exitCode();
            // Might log stderr: QString err = killer_icecast.readAllStandardError(); qDebug() << err;
        }
    } else {
        qWarning() << "Kill command for icecast timed out.";
        killer_icecast.kill(); // Kill the killer process itself
    }

    // Update UI regardless of success? Or only on success? Let's update anyway.
    ui->lbl_icecast->setText("Stopped");
    ui->lbl_icecast->setStyleSheet("color:blue;"); // Ensure CSS syntax is correct
    ui->bt_icecast->setStyleSheet("");


    // --- Stop Butt ---
    QProcess killer_butt;
    QString butt_cmd;
    QStringList butt_args;
    bool kill_success_butt = false;

#ifdef Q_OS_WIN
    butt_cmd = "taskkill";
    butt_args << "/F" << "/IM" << "butt.exe"; // Adjust executable name if needed
#else // Linux, macOS, other Unix-like
    butt_cmd = "pkill"; // Or "killall"
    butt_args << "butt"; // Process name
#endif

    qDebug() << "Running kill command for butt:" << butt_cmd << butt_args;
    killer_butt.start(butt_cmd, butt_args);
    if (killer_butt.waitForFinished(3000)) { // Wait up to 3 seconds
        if (killer_butt.exitStatus() == QProcess::NormalExit && killer_butt.exitCode() == 0) {
            qInfo() << "Butt process killed successfully (or was not running).";
            kill_success_butt = true;
        } else {
            qWarning() << "Kill command for butt failed or process not found. Exit code:" << killer_butt.exitCode();
        }
    } else {
        qWarning() << "Kill command for butt timed out.";
        killer_butt.kill();
    }

    ui->lbl_butt->setText("Stopped");
    ui->lbl_butt->setStyleSheet("color:blue;");
    ui->bt_butt->setStyleSheet("");

    // --- Update Other UI ---
    ui->lbl_port->setText("Stopped");
    ui->lbl_port->setStyleSheet("color:blue;");

    ui->bt_takeOver->setStyleSheet("");
    ui->bt_takeOver->setEnabled(false);

    // Assuming txt_ProgramName is meant to be hidden when stopped
    ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
    ui->txt_ProgramName->hide();

    piscaLive = false; // Assuming piscaLive is a member variable bool
}
void player::streaming_timmer(){

    qDebug()<<"Running streaming_timmer (checking external processes...)";

    // --- Check Icecast ---
    QProcess check_icecast;
    QString icecast_check_cmd;
    QStringList icecast_check_args;
    bool icecast_running = false;

#ifdef Q_OS_WIN
    icecast_check_cmd = "tasklist";
    icecast_check_args << "/NH" << "/FI" << "IMAGENAME eq icecast.exe"; // Adjust image name
#else // Linux, macOS, other Unix-like
    icecast_check_cmd = "pgrep";
    icecast_check_args << "-x" << "icecast"; // Match exact process name
#endif

    check_icecast.start(icecast_check_cmd, icecast_check_args);
    if (check_icecast.waitForFinished(1000)) { // Short timeout
#ifdef Q_OS_WIN
        // Tasklist often returns 0 even if not found, check output
        QString output = check_icecast.readAllStandardOutput();
        if (output.contains("icecast.exe", Qt::CaseInsensitive)) {
            icecast_running = true;
        }
#else // pgrep check
        if (check_icecast.exitStatus() == QProcess::NormalExit && check_icecast.exitCode() == 0) {
            // pgrep returns 0 if process(es) found
            icecast_running = true;
        }
#endif
    } else {
         qWarning() << "Check command for icecast timed out or failed.";
    }

    // Update Icecast UI
    if(icecast_running){
        ui->lbl_icecast->setText("Running");
        ui->lbl_icecast->setStyleSheet("color:green;");
    } else {
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:red;");
         ui->bt_takeOver->setEnabled(false); // Disable takeover if icecast stopped
    }


    // --- Check Butt ---
    QProcess check_butt;
    QString butt_check_cmd;
    QStringList butt_check_args;
    bool butt_running = false;

#ifdef Q_OS_WIN
    butt_check_cmd = "tasklist";
    butt_check_args << "/NH" << "/FI" << "IMAGENAME eq butt.exe"; // Adjust image name
#else // Linux, macOS, other Unix-like
    butt_check_cmd = "pgrep";
    butt_check_args << "-x" << "butt"; // Match exact process name
#endif

    check_butt.start(butt_check_cmd, butt_check_args);
     if (check_butt.waitForFinished(1000)) { // Short timeout
 #ifdef Q_OS_WIN
        QString output = check_butt.readAllStandardOutput();
        if (output.contains("butt.exe", Qt::CaseInsensitive)) {
             butt_running = true;
         }
 #else // pgrep check
         if (check_butt.exitStatus() == QProcess::NormalExit && check_butt.exitCode() == 0) {
             butt_running = true;
         }
 #endif
    } else {
         qWarning() << "Check command for butt timed out or failed.";
     }

    // Update Butt UI
    if(butt_running){
        ui->lbl_butt->setText("Running");
        ui->lbl_butt->setStyleSheet("color:green;");
    } else {
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:red;");
        ui->bt_takeOver->setEnabled(false); // Also disable if butt stopped? Decide based on logic.
    }
}
void player::ddnsUpdate() {
    qDebug() << "Requesting external IP address...";

    // Ensure networkManager is initialized (e.g., in constructor)
    if (!networkManager) {
         qWarning() << "Network manager not initialized!";
         networkManager = new QNetworkAccessManager(this); // Lazy init? Or handle error better.
    }

    // Use HTTPS for security. icanhazip supports it.
    QUrl url("https://ipv4.icanhazip.com");
    QNetworkRequest request(url);

    // Set a timeout for the request (e.g., 10 seconds)
    request.setTransferTimeout(10000);

    QNetworkReply *reply = networkManager->get(request);

    // Connect the finished signal to a lambda or a dedicated slot
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString externalIp = "Error"; // Default in case of failure

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            externalIp = QString::fromUtf8(responseData).trimmed(); // Assume UTF-8, trim whitespace
            qInfo() << "External IP determined:" << externalIp;
        } else {
            qWarning() << "Failed to get external IP:" << reply->errorString();
            externalIp = "Error: " + reply->errorString(); // Provide error info in UI
        }

        // Update the UI label
        ui->lbl_ddns->setText(externalIp);
        // Consider styling based on success/error?
        // if (reply->error() != QNetworkReply::NoError) {
        //     ui->lbl_ddns->setStyleSheet("color: red;");
        // } else {
        //     ui->lbl_ddns->setStyleSheet(""); // Clear style
        // }


        // IMPORTANT: Clean up the reply object to prevent memory leaks
        reply->deleteLater();
    });

    // Optional: Connect errorOccurred signal for more immediate feedback?
    // connect(reply, &QNetworkReply::errorOccurred, this, ...);

    // Optional: Connect sslErrors signal if needed for debugging HTTPS issues
    // connect(reply, &QNetworkReply::sslErrors, this, ...);
}

void player::on_horizontalSlider_lps_vol_sliderMoved(int position)
{
    qDebug()<<"LPS Horisontal Slider moved to: "<<position;


    if(position==100){
        lp1_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
        lp2_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume

        ui->lbl_lp1_vol_level->setText("100");
        ui->lbl_lp2_vol_level->setText("100");
    }


    if(position<=99){

        lp1_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
        lp2_XplayerOutput->setVolume(position / 100.0); // Qt6 uses 0.0-1.0 range for volume

        ui->lbl_lp1_vol_level->setText("100");

        QString np = QString::number(position);

        ui->lbl_lp2_vol_level->setText(np);


    }

    if(position>=101){

        lp2_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume

        int lp1_volume = 200-position;
        lp1_XplayerOutput->setVolume(lp1_volume / 100.0); // Qt6 uses 0.0-1.0 range for volume

        QString lp1_lbl_vol = QString::number(lp1_volume);
        ui->lbl_lp1_vol_level->setText(lp1_lbl_vol);

        ui->lbl_lp2_vol_level->setText("100");


    }

}

void player::on_bt_center_lps_vol_clicked()
{
    lp1_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
    lp2_XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume

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
    // --- Confirmation ---
    QMessageBox::StandardButton confirm = QMessageBox::question(this, "Confirm .netrc Update",
                                     QString("This will fetch the server IP from:\n%1\n\n"
                                             "It will then **overwrite** your ~/.netrc file with credentials for this server:\n"
                                             "machine [Server IP]\nlogin %2\npassword [Your Password]\n\n"
                                             "Any other entries in ~/.netrc will be lost! Proceed?")
                                         .arg(Server_URL + "/XFB/Config/ftpupdate.txt") // Show the full URL
                                         .arg(User), // Show the username being written
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (confirm != QMessageBox::Yes) {
        qInfo() << ".netrc update cancelled by user.";
        return;
    }


    // --- Network Request ---
    qInfo() << "Requesting server IP configuration...";

    if (!networkManager) {
         qWarning() << "Network manager not initialized!";
         // Initialize it here if necessary, or ensure it's done in the constructor
         networkManager = new QNetworkAccessManager(this);
    }

    // Construct the full URL
    QUrl configUrl(Server_URL); // Assuming Server_URL is like "http://example.com"
    if (!configUrl.isValid() || Server_URL.isEmpty()) {
        qWarning() << "Server_URL is invalid or empty:" << Server_URL;
        QMessageBox::critical(this, "Configuration Error", "The Server URL configured in the application is invalid.");
        return;
    }
    configUrl.setPath("/XFB/Config/ftpupdate.txt"); // Append the path

    QNetworkRequest request(configUrl);
    request.setTransferTimeout(15000); // 15 second timeout

    qDebug() << "Fetching configuration from:" << request.url().toString();

    QNetworkReply *reply = networkManager->get(request);

    // Connect the finished signal (asynchronous handling)
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // --- Handle Network Response ---
        QString serverIp; // Variable to store the successfully retrieved IP

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QString responseString = QString::fromUtf8(responseData).trimmed(); // Assume UTF-8

            // Basic parsing: Assume IP is the first non-empty line
            QStringList lines = responseString.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

            if (!lines.isEmpty()) {
                serverIp = lines.first().trimmed(); // Take the first line and trim whitespace
                if (serverIp.isEmpty()) {
                     qWarning() << "Received empty first line from config URL:" << reply->url().toString();
                     QMessageBox::warning(this, "Update Failed", "Received empty data from the server configuration URL.");
                } else {
                     qInfo() << "Server IP determined:" << serverIp;
                }
            } else {
                qWarning() << "Received empty response from config URL:" << reply->url().toString();
                QMessageBox::warning(this, "Update Failed", "Received empty response from the server configuration URL.");
            }
        } else {
            qWarning() << "Failed to fetch server config:" << reply->errorString() << "(" << reply->error() << ")";
            QMessageBox::critical(this, "Network Error", QString("Failed to fetch server configuration:\n%1").arg(reply->errorString()));
            reply->deleteLater(); // MUST cleanup reply object
            return; // Stop processing on network error
        }

        // --- Proceed only if IP was found ---
        if (serverIp.isEmpty()) {
            reply->deleteLater(); // MUST cleanup reply object
            return; // Stop if we couldn't get a valid IP
        }


        // --- Write to .netrc ---
        QString netrcPath = QDir::homePath() + "/.netrc";
        qInfo() << "Attempting to write to:" << netrcPath;

        QFile netrcFile(netrcPath);

        // Open for writing, overwrite existing content (Truncate)
        if (!netrcFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            qWarning() << "Failed to open" << netrcPath << "for writing:" << netrcFile.errorString();
            QMessageBox::critical(this, "File Error", QString("Could not open the .netrc file for writing:\n%1\n\nPlease check permissions.").arg(netrcPath));
            reply->deleteLater();
            return;
        }

        QTextStream outStream(&netrcFile);
        QString line = QString("machine %1 login %2 password %3\n").arg(serverIp, User, Pass);
        outStream << line;
        outStream.flush(); // Ensure data is written

        if (outStream.status() != QTextStream::Ok) {
             qWarning() << "Error writing to .netrc file stream.";
             // File might be partially written
        }

        // Set strict permissions (Owner Read/Write only - 600)
        if (!netrcFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner)) {
             qWarning() << "Failed to set permissions (600) on" << netrcPath;
             // Not critical enough to fail the whole operation, but log it.
        }

        netrcFile.close(); // QTextStream destructor also closes it, but explicit is okay.

        qInfo() << ".netrc file updated successfully.";
        QMessageBox::information(this, "Update Successful", QString(".netrc file updated successfully for machine: %1").arg(serverIp));

        // --- Cleanup ---
        reply->deleteLater(); // MUST cleanup reply object
    });
}

void player::on_bt_add_some_random_songs_from_genre_clicked()
{

    QString selectedGenre = ui->comboBox_random_add_genre->currentText();

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QString num = ui->spinBox_num_of_songs_to_add_random->text();
checkDbOpen();
    QString querystr = "select path from musics where genre1 like '"+selectedGenre+"' order by random() limit "+num;
    QSqlQuery query(db);
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
    // Use a local variable to track the intended state change during this function
    bool shouldBeRunning = !icecastrunning; // If currently false, we want it true, and vice-versa

    if (shouldBeRunning) {
        // --- Try to START Icecast ---
        qInfo() << "Attempting to start Icecast...";

        // 1. Force kill any existing instances first (as per original logic)
        qDebug() << "Ensuring no other icecast instances are running...";
        killProcessByName("icecast"); // Call helper function

        // 2. Find xvfb-run
        QString xvfbRunPath = QStandardPaths::findExecutable("xvfb-run");
        if (xvfbRunPath.isEmpty()) {
             qWarning() << "xvfb-run command not found in PATH. Cannot start icecast.";
             QMessageBox::critical(this, "Error", "Required command 'xvfb-run' not found. Cannot start Icecast.");
             return; // Cannot proceed
        }
        qDebug() << "Found xvfb-run at:" << xvfbRunPath;
        // Optional: Check if icecast executable itself exists?
        // QString icecastPath = QStandardPaths::findExecutable("icecast");
        // if (icecastPath.isEmpty()) { ... error ... }


        // 3. Prepare and start the process detached
        QProcess *icecastProcess = new QProcess(this); // Create on heap if needed elsewhere, else stack
        QStringList args;
        QString configPath = "/usr/local/etc/icecast.xml"; // Consider making this configurable
        args << "-a" << "icecast" << "-c" << configPath;

        qDebug() << "Starting detached process:" << xvfbRunPath << args;

        bool started = icecastProcess->startDetached(xvfbRunPath, args);
        // Note: startDetached only tells you if Qt could *invoke* the process,
        // not if xvfb-run or icecast started successfully internally.
        // The timer check (ice_timmer) is the real verification.

        if (started) {
            qInfo() << "Icecast process launch initiated.";
            icecastrunning = true; // Update state flag

            // Update UI to "Starting" or "Running" - timer will confirm "Running"
            ui->lbl_icecast->setText("Starting..."); // More accurate initial state
            ui->lbl_icecast->setStyleSheet("color:orange;"); // Indicate intermediate state
            ui->bt_icecast->setStyleSheet("background-color:#C8EE72;"); // Indicate "on" state

            // Connect and start the timer to check status
            if (icetimer) {
                // Disconnect first to avoid multiple connections if clicked rapidly
                disconnect(icetimer, &QTimer::timeout, this, &player::ice_timmer);
                connect(icetimer, &QTimer::timeout, this, &player::ice_timmer);
                icetimer->start(5000); // Check every 5 seconds
                QTimer::singleShot(500, this, &player::ice_timmer); // Check quickly once after starting
            } else {
                 qWarning() << "icetimer is null!";
            }
        } else {
            qWarning() << "Failed to initiate icecast process launch.";
            QMessageBox::critical(this, "Error", "Failed to start the Icecast process using xvfb-run.");
            // Don't change icecastrunning state or UI if launch failed
            delete icecastProcess; // Clean up if created on heap
        }

    } else {
        // --- Try to STOP Icecast ---
        qInfo() << "Attempting to stop Icecast...";

        // 1. Stop the status check timer
        if (icetimer) {
            icetimer->stop();
            // Optional: disconnect(icetimer, &QTimer::timeout, this, &player::ice_timmer);
        }

        // 2. Kill the process
        bool killed = killProcessByName("icecast"); // Call helper function

        // 3. Update state and UI regardless of kill success, as the intention is to stop
        icecastrunning = false;
        ui->bt_icecast->setStyleSheet(""); // Default style
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:blue;"); // Use correct CSS

        // Reset dependent UI elements (as per original logic)
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);
        ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
        ui->txt_ProgramName->hide();
        piscaLive = false;

        if (!killed) {
             qWarning() << "Kill command for icecast may have failed, but UI is set to stopped state.";
             // User might need to manually check if process actually stopped
        }
    }
}
void player::ice_timmer() {
    qDebug() << "Running ice_timmer (checking icecast process status...)";

    QProcess check_icecast;
    QString check_cmd;
    QStringList check_args;
    bool is_running = false; // Assume not running initially

#ifdef Q_OS_WIN
    // Windows: Use tasklist with image name filter
    check_cmd = "tasklist";
    // /NH for no header, /FI for filter. Adjust "icecast.exe" if needed.
    check_args << "/NH" << "/FI" << "IMAGENAME eq icecast.exe";
#else
    // Unix-like (Linux, macOS): Use pgrep with exact name match
    check_cmd = "pgrep";
    check_args << "-x" << "icecast"; // -x matches the name exactly
#endif

    qDebug() << "Executing check:" << check_cmd << check_args;
    check_icecast.start(check_cmd, check_args);

    // Wait for the check command to finish (short timeout)
    if (check_icecast.waitForFinished(1500)) { // 1.5 second timeout

#ifdef Q_OS_WIN
        // On Windows, tasklist usually exits with 0 even if not found when filtering.
        // We need to check if the output contains the process name.
        QByteArray output = check_icecast.readAllStandardOutput();
        if (output.contains("icecast.exe", Qt::CaseInsensitive)) {
            is_running = true;
            qDebug() << "tasklist output indicates icecast is running.";
        } else {
             qDebug() << "tasklist output does not contain icecast.exe.";
             is_running = false;
        }
#else
        // On Unix-like, pgrep exits with 0 if found, non-zero otherwise.
        if (check_icecast.exitStatus() == QProcess::NormalExit && check_icecast.exitCode() == 0) {
            is_running = true;
            qDebug() << "pgrep found running icecast process (exit code 0).";
        } else {
            // Exit code 1 usually means not found, other codes indicate errors.
             qDebug() << "pgrep did not find running icecast process (exit code" << check_icecast.exitCode() << ").";
            is_running = false;
        }
#endif
    } else {
        // Timeout or other error running the check command itself
        qWarning() << "Check command (" << check_cmd << ") timed out or failed to start.";
        is_running = false; // Assume not running if check failed
        // Consider setting UI to an "Unknown" or "Error" state? For now, treat as stopped.
    }

    // --- Update UI based on the is_running state ---
    if (is_running) {
        // Update state variable if needed (though might be redundant if only used here)
        if (!icecastrunning) { // Update internal state if it changed
             qInfo() << "Icecast detected as running (was previously considered stopped).";
             icecastrunning = true;
        }
        // Set UI to "Running" state
        ui->lbl_icecast->setText("Running");
        ui->lbl_icecast->setStyleSheet("color:green;"); // Ensure semicolon
        // Optionally ensure other UI elements are in the correct "running" state if needed
    } else {
        // Update state variable if needed
        if (icecastrunning) { // Update internal state if it changed
            qInfo() << "Icecast detected as stopped (was previously considered running).";
            icecastrunning = false;
        }
        // Set UI to "Stopped" state and reset related elements
        ui->lbl_icecast->setText("Stopped");
        ui->lbl_icecast->setStyleSheet("color:red;"); // Ensure semicolon

        // Reset other UI elements as per original logic when stopped
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);
        ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
        ui->txt_ProgramName->hide();
        piscaLive = false; // Reset live indicator flag
    }
}
void player::on_bt_butt_clicked()
{
    // Determine the intended state
    bool shouldBeRunning = !buttrunning;

    if (shouldBeRunning) {
        // --- Try to START Butt ---
        qInfo() << "Attempting to start Butt...";

        // 1. Force kill any existing instances first
        qDebug() << "Ensuring no other butt instances are running...";
        killProcessByName("butt"); // Kill 'butt' process

        // 2. Find required executables
        QString xvfbRunPath = QStandardPaths::findExecutable("xvfb-run");
        if (xvfbRunPath.isEmpty()) {
             qWarning() << "xvfb-run command not found in PATH. Cannot start butt.";
             QMessageBox::critical(this, "Error", "Required command 'xvfb-run' not found. Cannot start Butt.");
             return; // Cannot proceed
        }
        QString buttPath = QStandardPaths::findExecutable("butt");
         if (buttPath.isEmpty()) {
             qWarning() << "butt command not found in PATH. Cannot start butt.";
             QMessageBox::critical(this, "Error", "Required command 'butt' not found. Cannot start Butt.");
             return; // Cannot proceed
         }
        qDebug() << "Found xvfb-run at:" << xvfbRunPath;
        qDebug() << "Found butt at:" << buttPath; // We found it, but xvfb-run will call it by name


        // 3. Prepare and start the process detached
        QProcess *buttProcess = new QProcess(this); // Optional: manage this pointer if needed later
        QStringList args;
        args << "-a" << "butt"; // Arguments for xvfb-run

        qDebug() << "Starting detached process:" << xvfbRunPath << args;

        bool started = buttProcess->startDetached(xvfbRunPath, args);

        if (started) {
            qInfo() << "Butt process launch initiated.";
            buttrunning = true; // Update state flag

            // Update UI to "Starting..." - timer will confirm "Running"
            ui->lbl_butt->setText("Starting..."); // More accurate initial state
            ui->lbl_butt->setStyleSheet("color:orange;"); // Indicate intermediate state
            ui->bt_butt->setStyleSheet("background-color:#C8EE72;"); // Indicate "on" state

            // Connect and start the timer to check status
            if (butt_timer) {
                // Ensure clean connection
                disconnect(butt_timer, &QTimer::timeout, this, &player::butt_timmer);
                connect(butt_timer, &QTimer::timeout, this, &player::butt_timmer);
                butt_timer->start(5000); // Check every 5 seconds
                QTimer::singleShot(500, this, &player::butt_timmer); // Check quickly once after starting
            } else {
                 qWarning() << "butt_timer is null!";
            }
        } else {
            qWarning() << "Failed to initiate butt process launch.";
            QMessageBox::critical(this, "Error", "Failed to start the Butt process using xvfb-run.");
            // Clean up if necessary
            delete buttProcess;
        }

    } else {
        // --- Try to STOP Butt ---
        qInfo() << "Attempting to stop Butt...";

        // 1. Stop the status check timer
        if (butt_timer) {
            butt_timer->stop();
            // Optional: disconnect(...)
        }

        // 2. Kill the 'butt' process (FIXED: was killing icecast in original code)
        bool killed = killProcessByName("butt");

        // 3. Update state and UI regardless of kill success
        buttrunning = false;
        ui->bt_butt->setStyleSheet(""); // Default style
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:blue;");

        // Reset dependent UI elements only if Butt stopping should affect them
        // Note: Original code reset these based on stopping *icecast*. Decide if that's correct.
        // If stopping BUTT should stop the 'takeover', etc., keep these lines.
        // Otherwise, remove them or move them to the icecast stop logic.
        // Assuming stopping Butt SHOULD reset these for now:
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setEnabled(false);
        ui->txt_ProgramName->setStyleSheet(""); // Clear specific style
        ui->txt_ProgramName->hide();
        piscaLive = false;

        if (!killed) {
             qWarning() << "Kill command for butt may have failed, but UI is set to stopped state.";
        }
    }
}

// --- Refactored butt_timmer ---

void player::butt_timmer() {
    qDebug() << "Running butt_timmer (checking butt process status...)";

    QProcess check_butt;
    QString check_cmd;
    QStringList check_args;
    bool is_running = false; // Assume not running initially

#ifdef Q_OS_WIN
    // Windows: Use tasklist with image name filter
    check_cmd = "tasklist";
    // Adjust "butt.exe" if the actual executable name differs
    check_args << "/NH" << "/FI" << "IMAGENAME eq butt.exe";
#else
    // Unix-like (Linux, macOS): Use pgrep with exact name match
    check_cmd = "pgrep";
    check_args << "-x" << "butt"; // -x matches the name exactly
#endif

    qDebug() << "Executing check:" << check_cmd << check_args;
    check_butt.start(check_cmd, check_args);

    // Wait for the check command to finish (short timeout)
    if (check_butt.waitForFinished(1500)) { // 1.5 second timeout

#ifdef Q_OS_WIN
        QByteArray output = check_butt.readAllStandardOutput();
        if (output.contains("butt.exe", Qt::CaseInsensitive)) {
            is_running = true;
            qDebug() << "tasklist output indicates butt is running.";
        } else {
            qDebug() << "tasklist output does not contain butt.exe.";
            is_running = false;
        }
#else
        if (check_butt.exitStatus() == QProcess::NormalExit && check_butt.exitCode() == 0) {
            is_running = true;
            qDebug() << "pgrep found running butt process (exit code 0).";
        } else {
            qDebug() << "pgrep did not find running butt process (exit code" << check_butt.exitCode() << ").";
            is_running = false;
        }
#endif
    } else {
        qWarning() << "Check command (" << check_cmd << ") timed out or failed to start.";
        is_running = false; // Assume not running if check failed
    }

    // --- Update UI and state based on the is_running result ---
    if (is_running) {
        // Update state if changed
        if (!buttrunning) {
             qInfo() << "Butt detected as running (was previously considered stopped).";
             buttrunning = true;
        }
        // Set UI to "Running" state
        ui->lbl_butt->setText("Running");
        ui->lbl_butt->setStyleSheet("color:green;");
    } else {
        // Update state if changed
        if (buttrunning) {
            qInfo() << "Butt detected as stopped (was previously considered running).";
            buttrunning = false;
        }
        // Set UI to "Stopped" state
        ui->lbl_butt->setText("Stopped");
        ui->lbl_butt->setStyleSheet("color:red;");
        // Decide if stopping Butt should affect other UI like bt_takeOver.
        // Original butt_timmer didn't reset other UI, only ice_timmer did.
        // Stick to that logic unless intended otherwise.
    }
}

void player::on_bt_ddns_clicked()
{
    ddnsUpdate();
}
void player::on_bt_portTest_clicked()
{
    // --- Configuration ---
    const quint16 portToCheck = 8888;
    const int connectionTimeoutMs = 5000; // 5 seconds timeout for connection attempt

    // --- Get External IP ---
    QString externalIpStr = ui->lbl_ddns->text();

    // --- Basic Validation ---
    if (externalIpStr.isEmpty() || externalIpStr.startsWith("Error")) {
        QMessageBox::warning(this, tr("Missing IP"), tr("Could not determine the external IP address. Please update DDNS first."));
        ui->lbl_port->setText("No IP");
        ui->lbl_port->setStyleSheet("color:orange;");
        return;
    }
    // Optional: More robust IP validation
    QHostAddress addr(externalIpStr);
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) {
         qWarning() << "External IP from label is not a valid IPv4 address:" << externalIpStr;
         // Decide whether to proceed or warn the user further
         // QMessageBox::warning(this, tr("Invalid IP"), tr("The determined external IP address (%1) does not appear valid.").arg(externalIpStr));
         // return; // Uncomment to be stricter
    }


    // --- Start Check ---
    qInfo() << "Checking external reachability for" << externalIpStr << ":" << portToCheck;
    ui->lbl_port->setText("Checking...");
    ui->lbl_port->setStyleSheet("color:#FBEA23;"); // Yellowish color for checking
    // Disable button during check to prevent multiple clicks?
    // ui->bt_portTest->setEnabled(false); // Re-enable in results handling


    // --- Use QTcpSocket for the check ---
    QTcpSocket *socket = new QTcpSocket(this);
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);

    // Lambda to handle cleanup for both success and failure
    auto cleanup = [socket, timeoutTimer, this]() {
        // ui->bt_portTest->setEnabled(true); // Re-enable button
        if (timeoutTimer->isActive()) {
            timeoutTimer->stop();
        }
        // Ensure socket is disconnected before deleting if necessary
        if(socket->state() != QAbstractSocket::UnconnectedState){
            socket->abort(); // Force close immediately
        }
        socket->deleteLater();
        timeoutTimer->deleteLater();
        qDebug() << "Cleanup after port check complete.";
    };

    // Connection successful
    connect(socket, &QTcpSocket::connected, this, [this, externalIpStr, portToCheck, cleanup]() {
        qInfo() << "Connection successful to" << externalIpStr << ":" << portToCheck;
        ui->lbl_port->setText("OPEN"); // Simplified from "OPEN IN + OUT"
        ui->lbl_port->setStyleSheet("color:green;");

        // Format the clickable link
        QString urlString = QString("http://%1:%2/stream.m3u").arg(externalIpStr).arg(portToCheck);
        QString linkHtml = QString("<a href=\"%1\">%1</a>").arg(urlString);

        ui->lbl_streamURL->setText(linkHtml);
        ui->lbl_streamURL->setTextFormat(Qt::RichText);
        ui->lbl_streamURL->setTextInteractionFlags(Qt::TextBrowserInteraction);
        // Note: setOpenExternalLinks(false) is default for QLabel, use true if you want direct clicking
        ui->lbl_streamURL->setOpenExternalLinks(true);

        ui->bt_takeOver->setEnabled(true);

        cleanup(); // Call cleanup lambda
    });

    // Connection error (includes refusal, host not found, etc.)
    connect(socket, &QTcpSocket::errorOccurred, this, [this, socket, cleanup](QAbstractSocket::SocketError socketError) {
        QString errorMsg;
        switch(socketError) {
            case QAbstractSocket::ConnectionRefusedError:
                errorMsg = tr("Connection Refused: Port is likely closed or not listening.");
                break;
            case QAbstractSocket::RemoteHostClosedError:
                errorMsg = tr("Connection Closed Unexpectedly."); // Less likely during initial check
                 break;
            case QAbstractSocket::HostNotFoundError:
                 errorMsg = tr("Host Not Found: The IP address might be incorrect or unreachable.");
                 break;
             case QAbstractSocket::SocketTimeoutError: // Should be caught by our timer, but handle anyway
                 errorMsg = tr("Connection Timed Out (Socket Error).");
                 break;
             case QAbstractSocket::NetworkError:
                 errorMsg = tr("Network Error: Check your connection or firewall.");
                 break;
             default:
                 errorMsg = tr("Connection Failed: %1").arg(socket->errorString());
                 break;
        }
        qWarning() << "Connection error:" << errorMsg << "(" << socketError << ")";
        ui->lbl_port->setText("CLOSED/ERROR");
        ui->lbl_port->setStyleSheet("color:red;");
        QMessageBox::warning(this, tr("Port Check Failed"), errorMsg + "\n\n" + tr("Ensure the service (e.g., Icecast) is running locally and the port is correctly forwarded in your router/firewall."));

        ui->lbl_streamURL->setText(tr("N/A")); // Clear stream URL
        ui->lbl_streamURL->setOpenExternalLinks(false);
        ui->bt_takeOver->setEnabled(false);

        cleanup(); // Call cleanup lambda
    });

    // Connection timeout timer
    connect(timeoutTimer, &QTimer::timeout, this, [this, socket, cleanup]() {
        qWarning() << "Connection attempt timed out.";
        ui->lbl_port->setText("TIMEOUT");
        ui->lbl_port->setStyleSheet("color:red;");
        QMessageBox::warning(this, tr("Port Check Timed Out"), tr("Could not connect to the server within the time limit.\n\nCheck if the server is running, the IP address is correct, and the port is open/forwarded. Network latency or firewalls could also be the cause."));

        ui->lbl_streamURL->setText(tr("N/A"));
        ui->lbl_streamURL->setOpenExternalLinks(false);
        ui->bt_takeOver->setEnabled(false);

        // Abort the socket connection attempt before cleanup
        socket->abort();
        cleanup(); // Call cleanup lambda
    });

    // --- Initiate Connection ---
    socket->connectToHost(externalIpStr, portToCheck);
    timeoutTimer->start(connectionTimeoutMs); // Start the timeout timer
}


void player::deleteFilesByPattern(const QString &dirPath, const QString &pattern) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist, cannot delete files:" << dirPath;
        return;
    }

    // Ensure pattern is treated as a wildcard pattern
    QStringList nameFilters;
    nameFilters << pattern; // e.g., "*.xml"

    // List files matching the pattern
    QStringList filesToDelete = dir.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);

    int deletedCount = 0;
    if (filesToDelete.isEmpty()) {
        qDebug() << "No files matching pattern" << pattern << "found in" << dirPath;
        return;
    }

    qInfo() << "Attempting to delete" << filesToDelete.count() << "files matching" << pattern << "in" << dirPath;
    for (const QString &filename : filesToDelete) {
        QString filePath = dir.filePath(filename);
        if (QFile::remove(filePath)) {
            qDebug() << "Deleted:" << filePath;
            deletedCount++;
        } else {
            qWarning() << "Failed to delete:" << filePath;
            // Log QFile::errorString() if needed for more details
        }
    }
    qInfo() << "Successfully deleted" << deletedCount << "out of" << filesToDelete.count() << "matching files.";
    return;
}

void player::on_bt_takeOver_clicked()
{
    // --- State Toggle Logic ---
    if (takeOver) {
        // Currently in takeover mode, user wants to cancel
        qInfo() << "Cancelling Takeover...";
        takeOver = false; // Update state first

        // Call the function responsible for reversing the takeover on the server side
        returnTakeOver(); // This likely needs its own robust implementation (maybe another script?)

        // Clean up local indicator file (use configured FTPPath)
        QString takeOverFilePath = QDir(FTPPath).filePath("takeover.xml");
        if (QFile::exists(takeOverFilePath)) {
            if (QFile::remove(takeOverFilePath)) {
                qInfo() << "Removed local takeover indicator file:" << takeOverFilePath;
            } else {
                qWarning() << "Failed to remove local takeover indicator file:" << takeOverFilePath;
            }
        }

        // Reset UI
        ui->bt_takeOver->setStyleSheet("");
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));
        ui->txt_ProgramName->setStyleSheet(""); // Clear style
        ui->txt_ProgramName->hide();
        piscaLive = false;
        return; // Finished cancelling
    }

    // --- Initiate Takeover ---
    qInfo() << "Initiating Takeover...";
    ui->bt_takeOver->setEnabled(false); // Disable button during operation
    ui->bt_takeOver->setText(tr("Processing..."));
    ui->bt_takeOver->setStyleSheet("background-color:yellow;"); // Indicate processing
    qApp->processEvents(); // Update UI immediately

    // --- 1. Clean Up Old Files ---
    qInfo() << "Cleaning up previous XML files...";
    // Use helper function with configured paths
    deleteFilesByPattern(FTPPath, "*.xml");
    deleteFilesByPattern(TakeOverPath, "*.xml");
    // Add checks here if deletion failure is critical

    // --- 2. Get Required Data ---
    QString externalIp = ui->lbl_ddns->text();
    if (externalIp.isEmpty() || externalIp.startsWith("Error")) {
        QMessageBox::warning(this, tr("Missing IP"), tr("Cannot initiate takeover without a valid external IP address. Please update DDNS first."));
        ui->bt_takeOver->setEnabled(true); // Re-enable button
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));
        ui->bt_takeOver->setStyleSheet("");
        return;
    }
    QString streamUrl = QString("http://%1:8888/stream.m3u").arg(externalIp); // Hardcoded port 8888

    // --- 3. Create Takeover XML ---
    QString takeOverFilePath = QDir(FTPPath).filePath("takeover.xml"); // Create in FTPPath
    qInfo() << "Creating takeover file:" << takeOverFilePath;
    QFile file(takeOverFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "Failed to open takeover file for writing:" << file.errorString();
        QMessageBox::critical(this, "File Error", QString("Could not create the takeover XML file:\n%1").arg(takeOverFilePath));
        ui->bt_takeOver->setEnabled(true);
        ui->bt_takeOver->setText(tr("Broadcast LIVE"));
        ui->bt_takeOver->setStyleSheet("");
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("XFBClientTakeOver");
    xmlWriter.writeStartElement("www.netpack.pt"); // Consider making this configurable
    xmlWriter.writeTextElement("stream", streamUrl);
    xmlWriter.writeTextElement("ip", externalIp);
    xmlWriter.writeEndElement(); // www.netpack.pt
    xmlWriter.writeEndElement(); // XFBClientTakeOver
    xmlWriter.writeEndDocument();

    if (xmlWriter.hasError()) {
         qWarning() << "Error writing XML to takeover file.";
         // File might be corrupt or incomplete
         file.close(); // Try to close
         QFile::remove(takeOverFilePath); // Attempt cleanup
         QMessageBox::critical(this, "XML Error", "Failed to generate the takeover XML file correctly.");
         ui->bt_takeOver->setEnabled(true);
         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         return;
    }
    file.close(); // XML writing finished successfully
    qInfo() << "Takeover XML created successfully.";

    // --- 4. Execute Upload Script Asynchronously ---
    // Determine script path (needs configuration - example assumes it's relative to app dir)
    // TODO: Replace this with a robust way to find the script (e.g., settings, known install path)
    QString scriptName = "serverFtpCmdsPutTakeOver.sh";
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path relative to app binary
    QString scriptPath = QDir(scriptDir).filePath(scriptName);

    qInfo() << "Attempting to execute upload script:" << scriptPath;
     qDebug() << "Dependencies: Script must exist, be executable, and ~/.netrc configured correctly.";


    if (!QFileInfo::exists(scriptPath)) {
         qWarning() << "Upload script not found at:" << scriptPath;
         QMessageBox::critical(this, "Script Error", QString("The required upload script was not found:\n%1").arg(scriptPath));
         // Clean up local XML file? Maybe not, user might want to upload manually.
         ui->bt_takeOver->setEnabled(true);
         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         return;
    }
     // Optional: Check if script is executable on Linux/macOS
     // QFileInfo scriptInfo(scriptPath);
     // if (!scriptInfo.isExecutable()) { ... error ... }


    ui->bt_takeOver->setText(tr("Uploading..."));
    qApp->processEvents();

    QProcess *uploadProcess = new QProcess(this); // Create on heap for async handling

    // Connect signals *before* starting
    connect(uploadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, uploadProcess, takeOverFilePath](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Upload script finished. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;

        QString stdOutput = QString::fromLocal8Bit(uploadProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(uploadProcess->readAllStandardError()).trimmed();

        bool success = false;
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Script exited normally, check output for confirmation
             qDebug() << "Upload script STDOUT:\n" << stdOutput;
             // Make check more robust - case-insensitive contains?
            if (stdOutput.contains("Transfer complete", Qt::CaseInsensitive)) {
                 qInfo() << "Upload script reported success.";
                 success = true;
             } else {
                  qWarning() << "Upload script finished (exit 0) but success message not found in output.";
                  if (!stdError.isEmpty()) {
                       qWarning() << "Upload script STDERR:\n" << stdError;
                  }
             }
        } else {
            qWarning() << "Upload script failed to execute or exited with an error.";
             if (!stdError.isEmpty()) {
                  qWarning() << "Upload script STDERR:\n" << stdError;
             } else if (!stdOutput.isEmpty()) {
                 // Sometimes errors go to stdout
                 qWarning() << "Upload script STDOUT (check for errors):\n" << stdOutput;
             }
        }

        // --- Handle result ---
        if (success) {
            ui->bt_takeOver->setStyleSheet("background-color:blue;"); // Indicate active takeover
            ui->bt_takeOver->setText(tr("Verifying...")); // Or "Broadcasting (Click to Stop)"
            takeOver = true; // Update state only on confirmed success

            // Clean up local file *after* successful upload
            if (QFile::remove(takeOverFilePath)) {
                 qInfo() << "Removed local takeover XML after successful upload.";
            } else {
                 qWarning() << "Could not remove local takeover XML after successful upload:" << takeOverFilePath;
            }

            // Start the verification check
            QTimer::singleShot(5000, this, &player::checkTakeOver);

        } else {
            // Upload failed or confirmation missing
            QMessageBox::critical(this, "Upload Failed", tr("Failed to upload the takeover file to the server or confirmation was not received.\nPlease check ~/.netrc, script permissions, network connection, and server status.\n\nScript output (if any):\n%1\n%2")
                                  .arg(stdOutput.left(200)) // Show some output
                                  .arg(stdError.left(200))); // Show some error output
            // Reset UI to pre-takeover state
            ui->bt_takeOver->setText(tr("Broadcast LIVE"));
            ui->bt_takeOver->setStyleSheet("");
            // Leave local XML file for debugging? Or remove it? Let's leave it for now.
            // QFile::remove(takeOverFilePath);
        }

        // Re-enable button and clean up process object
        ui->bt_takeOver->setEnabled(true);
        uploadProcess->deleteLater();
    });

    // Handle process startup errors
     connect(uploadProcess, &QProcess::errorOccurred, this, [this, uploadProcess, scriptPath](QProcess::ProcessError error){
         qWarning() << "Failed to start upload script (" << scriptPath << "). Error:" << error;
         QMessageBox::critical(this, "Script Error", QString("Could not start the upload script:\n%1\n\nError: %2").arg(scriptPath).arg(uploadProcess->errorString()));

         ui->bt_takeOver->setText(tr("Broadcast LIVE"));
         ui->bt_takeOver->setStyleSheet("");
         ui->bt_takeOver->setEnabled(true);
         uploadProcess->deleteLater(); // Cleanup
     });

    // Start the script
    uploadProcess->start(scriptPath); // No arguments needed based on original code
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


void player::checkTakeOver() {
    qDebug() << "Checking Takeover status asynchronously...";

    // --- Determine Script Path ---
    // TODO: Replace this with a robust way to find the script
    QString scriptName = "serverFtpCmdsCHKTakeOver.sh";
    QString scriptDir = QCoreApplication::applicationDirPath() + "/usr/share/xfb/scripts"; // Example path
    QString scriptPath = QDir(scriptDir).filePath(scriptName);

    qInfo() << "Attempting to execute check script:" << scriptPath;

    if (!QFileInfo::exists(scriptPath)) {
        qWarning() << "Check script not found at:" << scriptPath;
        // Stop checking? Or keep retrying? Let's stop and warn.
        QMessageBox::critical(this, "Script Error", QString("The required check script was not found:\n%1\n\nTakeover verification stopped.").arg(scriptPath));
        // Reset UI to non-live state? Depends on desired behavior on error.
        // resetToNonLiveState(); // Call a hypothetical function to reset UI
        return;
    }
    // Optional: Check script executability on Linux/macOS

    // --- Execute Script Asynchronously ---
    QProcess *checkProcess = new QProcess(this); // Create on heap for async

    // Connection to handle when the process finishes
    connect(checkProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, checkProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Check script finished. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;

        bool takeoverConfirmed = false;
        QString stdOutput = QString::fromLocal8Bit(checkProcess->readAllStandardOutput()).trimmed();
        QString stdError = QString::fromLocal8Bit(checkProcess->readAllStandardError()).trimmed();

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Script finished successfully, now check its output
            qDebug() << "Check script STDOUT:\n" << stdOutput;
            if (stdOutput.contains("confirmtakeover.xml", Qt::CaseInsensitive)) {
                // Found the confirmation filename in the output
                takeoverConfirmed = true;
                qInfo() << "Takeover confirmation found in script output.";
            } else {
                qInfo() << "Takeover confirmation filename not found in script output.";
                // Log stderr just in case it has info
                if (!stdError.isEmpty()) {
                    qWarning() << "Check script STDERR (though exit code was 0):\n" << stdError;
                }
            }
        } else {
            // Script failed to execute properly or returned an error code
            qWarning() << "Check script failed or exited with error.";
            if (!stdError.isEmpty()) {
                qWarning() << "Check script STDERR:\n" << stdError;
            } else if (!stdOutput.isEmpty()) {
                // Sometimes errors go to stdout
                qWarning() << "Check script STDOUT (check for errors):\n" << stdOutput;
            }
        }

        // --- Update UI and State ---
        if (takeoverConfirmed) {
            qInfo() << "      -------------------         [TakeOver CONFIRMED]      ---------------------       ";
            // Check if already in the live state to avoid redundant updates/starts
            if (ui->bt_takeOver->text() != tr("BROADCASTING LIVE!!!")) {
                 ui->bt_takeOver->setStyleSheet("background-color:green;"); // Use semicolon
                 ui->bt_takeOver->setText(tr("BROADCASTING LIVE!!!"));

                 ui->txt_ProgramName->setText(tr("BROADCASTING LIVE!!!"));
                 ui->txt_ProgramName->setStyleSheet("background-color:red;color:#FFF;text-align:center !important;font-size:28px;font-weight:bolder;"); // Added semicolon
                 ui->txt_ProgramName->setAlignment(Qt::AlignHCenter);
                 ui->txt_ProgramName->show();

                 if (piscaLive == false) {
                     piscaLive = true;
                     livePiscaStart(); // Assuming this starts the blinking animation
                 }
            } else {
                 qDebug() << "Takeover already confirmed, UI state unchanged.";
            }
            // Successfully confirmed, do NOT schedule another check.

        } else {
            // Takeover not confirmed, schedule retry
            qWarning() << "      -------------------         [Takeover Check FAILED]         ---------------------       ";
            qWarning() << "      -------------------     [Scheduling retry in 10 sec]     ---------------------";

            // Optional: Update UI to show "Verification Failed" or similar temporarily?
            // ui->bt_takeOver->setText(tr("Verification Failed"));
            // ui->bt_takeOver->setStyleSheet("background-color:orange;"); // Indicate temporary failure?

            QTimer::singleShot(10000, this, &player::checkTakeOver); // Schedule retry
        }

        // Cleanup the process object
        checkProcess->deleteLater();
    });

    // Connection to handle process startup errors
    connect(checkProcess, &QProcess::errorOccurred, this, [this, checkProcess, scriptPath](QProcess::ProcessError error) {
        qWarning() << "Failed to start check script (" << scriptPath << "). Error:" << error << "-" << checkProcess->errorString();

        // Decide how to handle startup failure. Retry? Show critical error?
        qWarning() << "      -------------------         [Takeover Check FAILED - Could not start script]         ---------------------       ";
        qWarning() << "      -------------------     [Scheduling retry in 10 sec]     ---------------------";

        // Optional: Update UI to show error state
        // ui->bt_takeOver->setText(tr("Check Script Error"));
        // ui->bt_takeOver->setStyleSheet("background-color:purple;");

        QTimer::singleShot(10000, this, &player::checkTakeOver); // Schedule retry even on startup error
        checkProcess->deleteLater(); // Cleanup
    });

    // --- Start the Check ---
    // Execute the script directly, do not pipe to grep here
    checkProcess->start(scriptPath);
}
void player::MainsetVol100(){
    XplayerOutput->setVolume(1.0); // Qt6 uses 0.0-1.0 range for volume
}

void player::MainsetVol80(){
    XplayerOutput->setVolume(0.8); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol60(){
    XplayerOutput->setVolume(0.6); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol40(){
    XplayerOutput->setVolume(0.4); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol20(){
    XplayerOutput->setVolume(0.2); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol10(){
    XplayerOutput->setVolume(0.1); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainsetVol5(){
    XplayerOutput->setVolume(0.05); // Qt6 uses 0.0-1.0 range for volume
}
void player::MainStop(){
    Xplayer->stop();
    ui->btPlay->setStyleSheet("");
    ui->btPlay->setText(tr("Play"));
    PlayMode = "stopped";
}

bool player::killProcessByName(const QString &processName) {
    QProcess killer;
    QString cmd;
    QStringList args;
    bool success = false;

#ifdef Q_OS_WIN
    // Windows: Use taskkill
    cmd = "taskkill";
    args << "/F" << "/IM" << processName + "*";
    qWarning() << "Windows taskkill pattern matching is basic. Ensure '" << processName << "*' matches correctly, or use exact name.";
#else
    // Unix-like (Linux, macOS): Prefer pkill
    cmd = "pkill";
    args << "-f" << processName; // -f matches against the entire command line
#endif

    qInfo() << "Attempting to kill processes matching:" << processName << "using command:" << cmd << args;
    killer.start(cmd, args);
    if (killer.waitForFinished(3000)) { // Wait up to 3 seconds
        if (killer.exitStatus() == QProcess::NormalExit && killer.exitCode() == 0) {
            qInfo() << "Kill command executed successfully for" << processName << "(process may or may not have been running).";
            success = true;
        } else {
            qWarning() << "Kill command for" << processName << "failed or reported an error. Exit code:" << killer.exitCode() << "Status:" << killer.exitStatus();
        }
    } else {
        qWarning() << "Kill command for" << processName << "timed out.";
        killer.kill();
        killer.waitForFinished(500);
    }
    return success;
}

void player::pingTakeOverClient() {

    if (takeOverIP.isEmpty()) {
        qWarning() << "Cannot ping TakeOver Client: takeOverIP is empty.";
        // Optionally trigger the failure actions immediately? Or just return?
        // triggerPingFailureActions(); // Call a helper containing the failure steps
        return;
    }

    qInfo() << "Pinging TakeOver Client asynchronously ->| " << takeOverIP;

    // --- Prepare Ping Command (Platform Specific) ---
    QProcess *pingProcess = new QProcess(this); // Create on heap for async handling
    QString pingCmd;
    QStringList pingArgs;
    int timeoutMs = 5000; // Timeout for the ping command itself (e.g., 5 seconds) - was 8s wait

#ifdef Q_OS_WIN
    pingCmd = "ping";
    // -n 1: Send 1 echo request
    // -w timeout: Wait 'timeout' milliseconds for reply (use slightly less than waitForFinished timeout)
    pingArgs << "-n" << "1" << "-w" << QString::number(timeoutMs - 500) << takeOverIP;
#elif defined(Q_OS_MACOS)
    pingCmd = "ping";
    // -c 1: Send 1 echo request
    // -t timeout: Specify timeout in seconds for the command
    pingArgs << "-c" << "1" << "-t" << QString::number(timeoutMs / 1000) << takeOverIP;
#else // Linux and other Unix-like
    pingCmd = "ping";
    // -c 1: Send 1 echo request
    // -W timeout: Wait 'timeout' seconds for a reply
    // -q: Quiet output (optional, we only care about exit code)
    pingArgs << "-c" << "1" << "-W" << QString::number(timeoutMs / 1000) << "-q" << takeOverIP;
#endif

    qDebug() << "Executing ping command:" << pingCmd << pingArgs;

    // --- Connect Signals BEFORE Starting ---

    // Handle process finished
    connect(pingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, pingProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Ping process finished. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;

        bool success = false;
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            // Exit code 0 typically means success (at least one reply received)
            success = true;
        } else {
            // Non-zero exit code or crash indicates failure
            qWarning() << "Ping command failed or host unreachable.";
            // Read stderr/stdout for potential clues (optional)
            // QString stdErr = QString::fromLocal8Bit(pingProcess->readAllStandardError());
            // if (!stdErr.isEmpty()) qDebug() << "Ping STDERR:" << stdErr;
            // QString stdOut = QString::fromLocal8Bit(pingProcess->readAllStandardOutput());
            // if (!stdOut.isEmpty()) qDebug() << "Ping STDOUT:" << stdOut;
        }

        // --- Take Action Based on Result ---
        if (success) {
            qInfo() << "TakeOver Client pinged OK!! |<- " << takeOverIP;
            // No further action needed on success according to original logic
        } else {
            qWarning() << "ERROR pinging TakeOver Client!! Taking recovery actions...";
            triggerPingFailureActions(); // Encapsulate failure actions in a helper
        }

        // Cleanup
        pingProcess->deleteLater();
    });

    // Handle process startup errors
    connect(pingProcess, &QProcess::errorOccurred, this, [this, pingProcess](QProcess::ProcessError error) {
        qWarning() << "Failed to start ping process for" << takeOverIP << ". Error:" << error << "-" << pingProcess->errorString();
        qWarning() << "Treating as ping failure and taking recovery actions...";

        triggerPingFailureActions(); // Trigger failure actions if ping command itself fails

        // Cleanup
        pingProcess->deleteLater();
    });

    // --- Start the Ping ---
    pingProcess->start(pingCmd, pingArgs);
    // We don't use waitForFinished here, rely on signals
}

// --- Helper Function for Failure Actions ---
// Encapsulating these makes the main logic cleaner
void player::triggerPingFailureActions() {

    qWarning() << "Killing all streams (mplayer) and starting local playback...";

    // Stop mplayer using the robust helper
    killProcessByName("mplayer");

    // Start local playback
    on_btPlay_clicked(); // Assuming this starts the local player

    // Schedule volume fade-in (Original logic)
    // Consider if these delays/steps are still appropriate
    QTimer::singleShot(3500, this, &player::MainsetVol100);
    QTimer::singleShot(3000, this, &player::MainsetVol80);
    QTimer::singleShot(2500, this, &player::MainsetVol60);
    QTimer::singleShot(2000, this, &player::MainsetVol40);
    QTimer::singleShot(1500, this, &player::MainsetVol20);
    QTimer::singleShot(1000, this, &player::MainsetVol10);
    QTimer::singleShot(500, this, &player::MainsetVol5); // Note: Original code didn't have 0? Start low.

    // Schedule recovery attempt
    qInfo() << "Scheduling recovery stream attempt in 30 seconds.";
    QTimer::singleShot(30000, this, &player::recoveryStreamTakeOverPlay);
}

void player::recoveryStreamTakeOverPlay() {

    qInfo() << "Recovery initiated: Attempting to play takeover stream...";

    // --- 1. Stop Existing Player ---
    qInfo() << "Stopping any existing mplayer instance...";
    stopMplayer(); // Call the assumed function to stop the player

    // --- 2. Validate Stream URL/Path ---
    if (takeOverStream.isEmpty()) {
        qWarning() << "Cannot start recovery stream: takeOverStream variable is empty.";
        QMessageBox::critical(this, tr("Recovery Error"), tr("The recovery stream URL is not configured. Cannot proceed."));
        // Optionally trigger different recovery or stop?
        return;
    }
    // Optional: If it's a local file path, check if it exists
    // QUrl streamUrl(takeOverStream);
    // if (streamUrl.isLocalFile()) {
    //     QFileInfo playlistInfo(streamUrl.toLocalFile());
    //     if (!playlistInfo.exists()) {
    //         qWarning() << "Recovery playlist file not found:" << playlistInfo.filePath();
    //         QMessageBox::critical(this, tr("Recovery Error"), tr("Recovery playlist file not found:\n%1").arg(playlistInfo.filePath()));
    //         return;
    //     }
    // }
    qInfo() << "Attempting to play stream/playlist:" << takeOverStream;

    // --- 3. Find mplayer Executable ---
    QString mplayerPath = QStandardPaths::findExecutable("mplayer");
    if (mplayerPath.isEmpty()) {
        qWarning() << "mplayer executable not found in system PATH.";
        QMessageBox::critical(this, tr("Dependency Error"), tr("'mplayer' command not found. Please install mplayer and ensure it is in your PATH."));
        // Cannot proceed without mplayer
        return;
    }
    qInfo() << "Found mplayer at:" << mplayerPath;


    // --- 4. Prepare and Start mplayer Asynchronously ---
    QStringList mplayerArgs;
    // Add arguments BEFORE the playlist/URL
    mplayerArgs << "-volume" << "100"; // Start at full volume? Or maybe lower and fade in?
    mplayerArgs << "-playlist" << takeOverStream; // Add playlist/URL last

    qDebug() << "Executing:" << mplayerPath << mplayerArgs;

    // Disconnect any previous signal connections from the 'radio1' process object
    // to avoid duplicate handlers if this function is called again rapidly.
    radio1.disconnect(); // Disconnects all signals from this object

    // Connect error handling signal *before* starting
    connect(&radio1, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        qWarning() << "Failed to start mplayer for recovery stream. Error:" << error << "-" << radio1.errorString();
        QMessageBox::warning(this, tr("Playback Error"), tr("Failed to start the recovery stream player (mplayer).\nError: %1").arg(radio1.errorString()));
        // Optionally, schedule another recovery attempt?
        // QTimer::singleShot(15000, this, &player::recoveryStreamTakeOverPlay);
    });

    // Optionally connect started signal for logging confirmation
     connect(&radio1, &QProcess::started, this, [this](){
         qInfo() << "mplayer process started successfully for recovery stream.";
     });

     // Optionally connect finished signal for cleanup or restart logic
     connect(&radio1, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
             this, [this](int exitCode, QProcess::ExitStatus exitStatus){
         qWarning() << "mplayer recovery stream finished unexpectedly. ExitCode:" << exitCode << "ExitStatus:" << exitStatus;
         // Decide what to do: retry? stop? Assume user interaction needed?
         // Maybe just log it if recoveryStreamTakeOverPlay is the only way to restart.
         // If the stream *should* run indefinitely, maybe schedule another recovery:
         // QTimer::singleShot(5000, this, &player::recoveryStreamTakeOverPlay);
     });


    // Start the process
    radio1.start(mplayerPath, mplayerArgs);

    // Close communication channels *after* starting if not needed (as per original)
    // Note: Keeping stderr open can be useful for debugging mplayer errors.
    radio1.closeReadChannel(QProcess::StandardOutput);
    radio1.closeReadChannel(QProcess::StandardError);
    // radio1.closeWriteChannel(); // Also close input if definitely not needed


    // --- 5. Update UI Immediately ---
    // (Original logic updated UI before confirming mplayer started)
    ui->txtNowPlaying->setText(QString("RECOVERY: %1").arg(takeOverStream)); // Add prefix for clarity

    QDateTime now = QDateTime::currentDateTime();
    QString text = now.toString("yyyy-MM-dd || hh:mm:ss ||");
    // Add context to history entry
    QString historyNewLine = text + " RECOVERY STARTED - " + takeOverStream;
    ui->historyList->addItem(historyNewLine);
    ui->historyList->scrollToBottom(); // Ensure latest entry is visible


    // --- 6. Stop the Main Local Player (Delayed Fade) ---
    // This logic assumes MainsetVol/MainStop control a *different* player/UI element
    qInfo() << "Scheduling fade-out and stop for the main local player...";
    QTimer::singleShot(500, this, &player::MainsetVol100); // Start fade from 100? Or 0? This sequence looks like fade *out*.
    QTimer::singleShot(1000, this, &player::MainsetVol80);
    QTimer::singleShot(1500, this, &player::MainsetVol60);
    QTimer::singleShot(2000, this, &player::MainsetVol40);
    QTimer::singleShot(2500, this, &player::MainsetVol20);
    QTimer::singleShot(3000, this, &player::MainsetVol10);
    QTimer::singleShot(3500, this, &player::MainsetVol5);
    QTimer::singleShot(4000, this, &player::MainStop); // Final stop
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

void player::refreshAdBanner() {
    if (adBanner && adBanner->rootObject()) {
        // Call the QML function to reload the ad
        QMetaObject::invokeMethod(adBanner->rootObject(), "loadAd");
    }
}

// UI accessor methods for controllers
QTableView* player::getMusicView() const {
    return ui->musicView;
}

QPushButton* player::getPlayButton() const {
    return ui->btPlay;
}

QPushButton* player::getStopButton() const {
    return ui->btStop;
}

QSlider* player::getProgressSlider() const {
    return ui->sliderProgress;
}

QSlider* player::getVolumeSlider() const {
    return ui->sliderVolume;
}