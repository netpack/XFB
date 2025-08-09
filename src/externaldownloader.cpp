#include "externaldownloader.h"
#include "ui_externaldownloader.h"
#include "QProcess"
#include <QMessageBox>
#include <QtSql>
#include <QDir>
#include <addgenre.h>
#include <player.h>
#include <QtConcurrent>

//#include "permission_utils.h"


externaldownloader::externaldownloader(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::externaldownloader)
{
    ui->setupUi(this);

    /*
    // --- Check Permissions Early ---
    MicrophonePermissionStatus micStatus = checkMicrophonePermission();
    if (micStatus == MicrophonePermissionStatus::Denied || micStatus == MicrophonePermissionStatus::Error) {
        showMicrophonePermissionWarning(micStatus, this);
        return;
    } else if (micStatus == MicrophonePermissionStatus::Undetermined) {
        // Show the info message *once* perhaps? Or every time options are opened?
        showMicrophonePermissionWarning(micStatus, this);
    }*/



    ui->frame_loading->hide();


    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = writableConfigPath + "/" + configFileName;
    QSettings settingsnew(configFilePath, QSettings::IniFormat);
    bool darkMode = settingsnew.value("DarkMode", false).toBool();
    qDebug() << "[StyleFix] OptionsDialog checking dark mode:" << darkMode;


    // 3. Apply DIRECT stylesheet for background color
    // This overrides the default Fusion background drawing
    if (darkMode) {
        this->setStyleSheet("QDialog { background-color: #353535; color: #bbbbbb; }");
        // Optional: force tab page background again if needed, though attributes should work
        // if (theTabWidget) theTabWidget->setStyleSheet("QWidget { background-color: #353535; color: #bbbbbb; }");
    } else {
        this->setStyleSheet("QDialog { background-color: #ffffff; color: #333333; }");
        // Optional: force tab page background again if needed
        // if (theTabWidget) theTabWidget->setStyleSheet("QWidget { background-color: #ffffff; color: #333333; }");
    }
    // --- END C++ BACKGROUND FIX & DIRECT STYLING ---



    // --- Inside externaldownloader constructor ---

    qDebug() << "ED Construct: Attempting DB access...";

    // Test Step 1: Check if getting the handle crashes
    QSqlDatabase db; // Default invalid handle
    try {
        qDebug() << "ED Construct: Calling QSqlDatabase::database()...";
        // Use the 'false' parameter to PREVENT it from adding if it doesn't exist
        db = QSqlDatabase::database("xfb_connection", false);
        qDebug() << "ED Construct: QSqlDatabase::database() returned. isValid():" << db.isValid();
    } catch (const std::exception& e) {
        qCritical() << "ED Construct: *** CRASH/EXCEPTION during QSqlDatabase::database():" << e.what();
        return; // Exit constructor if this crashes
    } catch (...) {
        qCritical() << "ED Construct: *** CRASH/UNKNOWN EXCEPTION during QSqlDatabase::database()";
        return; // Exit constructor if this crashes
    }

    // If we reach here, getting the handle didn't crash. Now check validity.
    if (!db.isValid()) {
        qCritical() << "ED Construct: Database connection 'xfb_connection' is NOT VALID.";
        qWarning() << "Available connections:" << QSqlDatabase::connectionNames();
        qWarning() << "Did initializeDatabase() run successfully with this name?";
        // Maybe show a user error message here?
        return; // Exit if handle is invalid
    }
    qDebug() << "ED Construct: Connection handle is valid.";

    // Test Step 2: Check if isOpen() crashes
    bool isOpen = false;
    try {
        qDebug() << "ED Construct: Calling db.isOpen()...";
        isOpen = db.isOpen();
        qDebug() << "ED Construct: db.isOpen() returned:" << isOpen;
    } catch (const std::exception& e) {
        qCritical() << "ED Construct: *** CRASH/EXCEPTION during db.isOpen():" << e.what();
        return;
    } catch (...) {
        qCritical() << "ED Construct: *** CRASH/UNKNOWN EXCEPTION during db.isOpen()";
        return;
    }

    // If we reach here, isOpen() didn't crash. Check the result.
    if (!isOpen) {
        qWarning() << "ED Construct: Database connection 'xfb_connection' not open!";
        // Attempt to re-initialize or show error
        // if (!initializeDatabase()) { // Assuming initializeDatabase is accessible
        //    qCritical() << "ED Construct: Failed to re-initialize database.";
        //    return;
        // }
        // db = QSqlDatabase::database("xfb_connection", false); // Re-fetch after init
        // if (!db.isOpen()) {
        //     qCritical() << "ED Construct: Still not open after re-init.";
        //     return;
        // }
        return; // Exit if not open and not re-initialized
    } else {
        qDebug() << "ED Construct: Database is open. Proceeding with query.";

        // Test Step 3: Query execution (keep your original logic here)
        // Wrap in try-catch just in case, although crashes here are less common than DB errors
        try {
            QSqlQueryModel * model=new QSqlQueryModel();
            QSqlQueryModel * model2=new QSqlQueryModel();
            // Use stack allocation for QSqlQuery unless you have a strong reason for heap
            QSqlQuery qry(db); // Pass the valid, open handle

            qDebug() << "ED Construct: Preparing/executing query 1...";
            if (!qry.prepare("select name from genres1 order by name")) { // Added ORDER BY
                qWarning() << "ED Construct: Failed to prepare query 1:" << qry.lastError().text();
            } else if (!qry.exec()) {
                qWarning() << "ED Construct: Failed to execute query 1:" << qry.lastError().text();
            } else {
                qDebug() << "ED Construct: Query 1 success. Setting model...";
                model->setQuery(std::move(qry)); // Set after successful exec
                if(model->lastError().isValid()) qWarning() << "Error setting query 1:" << model->lastError();
                ui->cbox_g1->setModel(model);
                qDebug() << "ED Construct: Model 1 set.";
            }

            // Re-use the query object if possible, or create a new one on stack
            QSqlQuery qry2(db);
            qDebug() << "ED Construct: Preparing/executing query 2...";
            if (!qry2.prepare("select name from genres1 order by name")) { // Re-prepare
                qWarning() << "ED Construct: Failed to prepare query 2:" << qry2.lastError().text();
            } else if (!qry2.exec()) {
                qWarning() << "ED Construct: Failed to execute query 2:" << qry2.lastError().text();
            } else {
                qDebug() << "ED Construct: Query 2 success. Setting model...";
                model2->setQuery(std::move(qry2)); // Set after successful exec
                if(model2->lastError().isValid()) qWarning() << "Error setting query 2:" << model2->lastError();
                ui->cbox_g2->setModel(model2);
                qDebug() << "ED Construct: Model 2 set.";
            }

            // Delete the QSqlQuery if you used 'new' (but stack is preferred)
            // delete qry;

        } catch (const std::exception& e) {
            qCritical() << "ED Construct: *** CRASH/EXCEPTION during query execution:" << e.what();
            return;
        } catch (...) {
            qCritical() << "ED Construct: *** CRASH/UNKNOWN EXCEPTION during query execution";
            return;
        }
        qDebug() << "ED Construct: Finished DB setup.";
    }

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

// --- Helper Function (Consider placing in a utility class or namespace) ---
// Basic sanitization for potentially problematic filename characters *if needed*
// beyond what yt-dlp does. Often, relying on yt-dlp is sufficient.
QString sanitizeFilenameComponent(QString component) {
    // Remove characters problematic for *most* filesystems. Adjust as needed.
    // Using QRegularExpression for cleaner removal.
    component.remove(QRegularExpression(R"([\\/:*?"<>|])")); // Common Windows/Unix problematic chars
    component.replace(".", ""); // Keep the dot removal if specifically required
    component.replace("'", "");
    component.replace("\"", "");
    return component.trimmed(); // Remove leading/trailing whitespace
}
// --- Database Parameters Struct (Optional but good practice) ---
struct DatabaseCredentials {
    QString driver;        // e.g., "QSQLITE", "QPSQL"
    QString databaseName; // e.g., path for SQLite, name for PostgreSQL
    QString hostName;      // Optional, depends on driver
    QString userName;      // Optional
    QString password;      // Optional
    QString connectionName; // The *original* main connection name
};

// --- Function to run in a separate thread ---
struct DownloadResult {
    bool success = false;
    QString message;
    QString consoleOutput;
};

DownloadResult processDownloadTask(
    QString ylink,
    QString yartist,
    QString ysong,
    QString g1,
    QString g2,
    QString country,
    QString pub_date,
    DatabaseCredentials dbCreds
    ) {
    DownloadResult result;
    QStringList consoleLines; // Accumulate console output

    auto appendOutput = [&](const QString& line) {
        qDebug().noquote() << "[Worker]" << line; // Log worker output
        consoleLines.append(line);
    };

    appendOutput("Starting download task in thread: " + QString::number((quintptr)QThread::currentThreadId()));

    // Generate a unique connection name for this worker thread
    QString workerConnectionName = QString("worker_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    // --- Database Connection Handling ---
    QSqlDatabase db; // Declare here to manage scope for cleanup

    { // Scope to ensure db object is accessible for cleanup if addDatabase fails
        db = QSqlDatabase::addDatabase(dbCreds.driver, workerConnectionName);
        if (!db.isValid()) {
            result.success = false;
            result.message = "Error: Failed to add database driver '" + dbCreds.driver + "' in worker thread.";
            appendOutput(result.message);
            result.consoleOutput = consoleLines.join("\n");
            // No need to removeDatabase yet as it wasn't successfully added
            return result;
        }

        db.setDatabaseName(dbCreds.databaseName);
        if (!dbCreds.hostName.isEmpty()) db.setHostName(dbCreds.hostName);
        if (!dbCreds.userName.isEmpty()) db.setUserName(dbCreds.userName);
        if (!dbCreds.password.isEmpty()) db.setPassword(dbCreds.password);
        // Set other parameters like port if needed

        if (!db.open()) {
            result.success = false;
            result.message = "Error: Could not open database connection '" + workerConnectionName + "' in worker thread. Error: " + db.lastError().text();
            appendOutput(result.message);
            db.close(); // Attempt to close
            QSqlDatabase::removeDatabase(workerConnectionName); // Clean up
            result.consoleOutput = consoleLines.join("\n");
            return result;
        }
        appendOutput("Database connection '" + workerConnectionName + "' opened successfully in worker thread.");

    } // End of initial scope, db object persists

    // 1. Sanitize Artist/Song for Filename (Optional - yt-dlp's template often handles this)
    // If you still want manual sanitization:
    // QString safeArtist = sanitizeFilenameComponent(yartist);
    // QString safeSong = sanitizeFilenameComponent(ysong);
    // If relying on yt-dlp:
    QString safeArtist = yartist; // Use original for metadata, yt-dlp handles filename
    QString safeSong = ysong;

    if (safeArtist.isEmpty() || safeSong.isEmpty()) {
        result.success = false;
        result.message = "Error: Artist and Song name cannot be empty.";
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }

    // 2. Determine Music Directory (More Robustly)
    // Option A: Relative to executable
    QString musicBaseDir = QCoreApplication::applicationDirPath() + "/../music";
    // Option B: Standard Music Location (Recommended for user data)
    // QString musicBaseDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + QCoreApplication::applicationName() + "/music";
    // Option C: Make it configurable

    QDir musicDir(musicBaseDir);
    if (!musicDir.exists()) {
        if (!musicDir.mkpath(".")) {
            result.success = false;
            result.message = "Error: Cannot create music directory: " + musicDir.absolutePath();
            appendOutput(result.message);
            result.consoleOutput = consoleLines.join("\n");
            return result;
        }
        appendOutput("Created music directory: " + musicDir.absolutePath());
    }
    QString targetBaseFilename = safeArtist + " - " + safeSong;
    // Let yt-dlp add the extension based on format
    QString outputPathTemplate = musicDir.absoluteFilePath(targetBaseFilename + ".%(ext)s");


    // 3. Find yt-dlp executable
    QString ytdlpPath = QStandardPaths::findExecutable("yt-dlp");
#ifdef Q_OS_WIN
        // On Windows, check common extensions if findExecutable fails without them
    if (ytdlpPath.isEmpty()) ytdlpPath = QStandardPaths::findExecutable("yt-dlp.exe");
    if (ytdlpPath.isEmpty()) ytdlpPath = QStandardPaths::findExecutable("yt-dlp.cmd");
    if (ytdlpPath.isEmpty()) ytdlpPath = QStandardPaths::findExecutable("yt-dlp.bat");
#endif

    if (ytdlpPath.isEmpty()) {
        result.success = false;
        result.message = "Error: 'yt-dlp' executable not found in PATH. Please install yt-dlp and ensure it's accessible.";
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }
    appendOutput("Found yt-dlp at: " + ytdlpPath);

    // 4. Prepare and Run yt-dlp command
    QStringList ytdlpArgs;
    ytdlpArgs << "--extract-audio"
              << "--audio-format" << "vorbis" // ogg container with vorbis audio
              << "-o" << outputPathTemplate // Output template
              << "--ffmpeg-location" << QStandardPaths::findExecutable("ffmpeg") // Explicitly provide ffmpeg path if needed
              << ylink;                        // The URL

    appendOutput("Executing: " + ytdlpPath + " " + ytdlpArgs.join(" "));

    QProcess ytdlpProcess;
    ytdlpProcess.setProcessChannelMode(QProcess::MergedChannels); // Combine stdout and stderr
    ytdlpProcess.setProgram(ytdlpPath);
    ytdlpProcess.setArguments(ytdlpArgs);

    QEventLoop loopYtdlp;
    QObject::connect(&ytdlpProcess, &QProcess::finished, &loopYtdlp, &QEventLoop::quit);
    QObject::connect(&ytdlpProcess, &QProcess::readyRead, [&]() {
        appendOutput(QString::fromUtf8(ytdlpProcess.readAll())); // Read output as it comes
    });

    ytdlpProcess.start();
    appendOutput("Putting hamsters on the job... hold on... *a tribute to torrentz");
    loopYtdlp.exec(); // Wait for finished signal

    // Read any remaining output
    appendOutput(QString::fromUtf8(ytdlpProcess.readAll()));

    if (ytdlpProcess.exitStatus() != QProcess::NormalExit || ytdlpProcess.exitCode() != 0) {
        result.success = false;
        result.message = "Error: yt-dlp failed (Exit code: " + QString::number(ytdlpProcess.exitCode()) + "). Check console output for details.";
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }

    appendOutput("yt-dlp finished successfully.");

    // Construct the expected final filename (yt-dlp replaces %(ext)s)
    QString finalFilename = targetBaseFilename + ".ogg"; // Assuming vorbis -> ogg
    QString finalFilepath = musicDir.absoluteFilePath(finalFilename);
    QFileInfo fileInfo(finalFilepath);

    if (!fileInfo.exists()) {
        // Sometimes yt-dlp might use .opus instead of .ogg for vorbis, check common alternatives
        finalFilename = targetBaseFilename + ".opus";
        finalFilepath = musicDir.absoluteFilePath(finalFilename);
        fileInfo.setFile(finalFilepath);
        if(!fileInfo.exists()) {
            result.success = false;
            result.message = "Error: Expected output file not found after download: " + finalFilepath + " or .ogg";
            appendOutput(result.message);
            result.consoleOutput = consoleLines.join("\n");
            return result;
        }
    }
    appendOutput("Downloaded file seems to be at: " + finalFilepath);

    bool dbhasmusic = false;
    { // Scope for QSqlQuery
        QSqlQuery queryCheck(db);
        queryCheck.prepare("SELECT id FROM musics WHERE path = :path");
        queryCheck.bindValue(":path", finalFilepath);
        if (!queryCheck.exec()) {
            result.success = false;
            result.message = "Error checking database: " + queryCheck.lastError().text();
            appendOutput(result.message);
            appendOutput("Failing query: " + queryCheck.lastQuery());
            result.consoleOutput = consoleLines.join("\n");
            db.close();
            return result;
        }
        if (queryCheck.next()) {
            dbhasmusic = true;
            appendOutput("Skipping: " + finalFilepath + " (Already in DB)");
        }
    } // QSqlQuery queryCheck goes out of scope


    if (dbhasmusic) {
        result.success = true; // Not an error, just already exists
        result.message = "This song is already in the database.";
        result.consoleOutput = consoleLines.join("\n");
        db.close();
        QSqlDatabase::removeDatabase(workerConnectionName);
        appendOutput("Database connection '" + workerConnectionName + "' closed and removed.");
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }

    // 6. Get Duration using exiftool (if not already in DB)
    QString trackDuration = "-"; // Default value

    QString exiftoolPath = QStandardPaths::findExecutable("exiftool");
#ifdef Q_OS_WIN
    if (exiftoolPath.isEmpty()) exiftoolPath = QStandardPaths::findExecutable("exiftool.exe");
#endif

    if (exiftoolPath.isEmpty()) {
        appendOutput("Warning: 'exiftool' not found. Cannot get track duration.");
    } else {
        appendOutput("Found exiftool at: " + exiftoolPath);
        QStringList exifArgs;
        exifArgs << "-T" << "-Duration" << finalFilepath; // -T for tab-separated, -Duration for just duration

        appendOutput("Executing: " + exiftoolPath + " " + exifArgs.join(" "));

        QProcess exiftoolProcess;
        exiftoolProcess.setProcessChannelMode(QProcess::MergedChannels);
        exiftoolProcess.setProgram(exiftoolPath);
        exiftoolProcess.setArguments(exifArgs);

        QEventLoop loopExif;
        QString exifOutput;
        QObject::connect(&exiftoolProcess, &QProcess::finished, &loopExif, &QEventLoop::quit);
        QObject::connect(&exiftoolProcess, &QProcess::readyRead, [&]() {
            exifOutput += QString::fromUtf8(exiftoolProcess.readAllStandardOutput());
        });

        exiftoolProcess.start();
        loopExif.exec(); // Wait for finished

        exifOutput += QString::fromUtf8(exiftoolProcess.readAllStandardOutput()); // Get remaining

        if (exiftoolProcess.exitStatus() == QProcess::NormalExit && exiftoolProcess.exitCode() == 0 && !exifOutput.trimmed().isEmpty()) {
            // Expected output is just the duration string, e.g., "0:03:45.67" or "3.5 s"
            trackDuration = exifOutput.trimmed();
                // Optional: Normalize duration format here if needed (e.g., always to HH:MM:SS)
            appendOutput("Track duration found: " + trackDuration);
        } else {
            appendOutput("Warning: exiftool failed or returned no duration. Exit code: " + QString::number(exiftoolProcess.exitCode()));
            appendOutput("exiftool output: " + exifOutput);
        }
    }


    // 7. Add to Database
    { // Scope for QSqlQuery
        QSqlQuery queryInsert(db);
        queryInsert.prepare("INSERT INTO musics (id, artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                            "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        queryInsert.addBindValue(yartist); // Use original non-sanitized for DB metadata
        queryInsert.addBindValue(ysong);
        queryInsert.addBindValue(g1);
        queryInsert.addBindValue(g2);
        queryInsert.addBindValue(country);
        queryInsert.addBindValue(pub_date);
        queryInsert.addBindValue(finalFilepath);
        queryInsert.addBindValue(trackDuration); // Use fetched or default duration
        queryInsert.addBindValue(0);       // Bind the integer 0
        queryInsert.addBindValue("-");    // Bind the string "-"

        // Add extra debug output right before exec()
        appendOutput("--- Binding Values (Positional) ---");
        appendOutput("SQL: " + queryInsert.lastQuery()); // See the prepared query
        QList<QVariant> boundValues = queryInsert.boundValues();
        appendOutput("Bound Value Count: " + QString::number(boundValues.size()));
        for(int i = 0; i < boundValues.size(); ++i) {
            appendOutput(QString("Bound Value %1: %2").arg(i).arg(boundValues.at(i).toString()));
        }
        appendOutput("--- Attempting exec() ---");


        if (!queryInsert.exec()) {
            result.success = false;
            result.message = "Error adding to database: " + queryInsert.lastError().text();
            appendOutput(result.message);
            appendOutput("Failing query: " + queryInsert.lastQuery());
            db.close();
            QSqlDatabase::removeDatabase(workerConnectionName);
            appendOutput("Database connection '" + workerConnectionName + "' closed and removed.");
            result.consoleOutput = consoleLines.join("\n");
            return result;
        } else {
            appendOutput("Successfully added to database.");
            result.success = true;
            result.message = "Video downloaded, converted, and added to database!";
        }
    } // QSqlQuery queryInsert goes out of scope

    appendOutput("All Done in worker thread!");
    db.close();
    QSqlDatabase::removeDatabase(workerConnectionName);
    appendOutput("Database connection '" + workerConnectionName + "' closed and removed.");
    result.consoleOutput = consoleLines.join("\n");
    return result;
}


// --- The method in your externaldownloader class ---
void externaldownloader::getFile() {
    ui->bt_youtube_getIt->setEnabled(false); // Disable button immediately
    ui->txt_teminal_yd1->clear(); // Clear previous output
    ui->txt_teminal_yd1->appendPlainText("Preparing download...");
    ui->frame_loading->show(); // Show loading indicator

    // 1. Gather UI Data (in the UI thread)
    QString ylink = ui->txt_videoLink->text().trimmed();
    QStringList ylinkparts = ylink.split('&'); // Basic cleaning of tracking params
    ylink = ylinkparts[0];

    QString yartist = ui->txt_artist->text().trimmed();
    QString ysong = ui->txt_song->text().trimmed();
    QString g1 = ui->cbox_g1->currentText();
    QString g2 = ui->cbox_g2->currentText();
    QString country = ui->checkBox_cplp->isChecked() ? "PT" : "Other country / language";
    QString pub_date = ui->dateEdit_publishedDate->text(); // Assumes QDateEdit format is suitable

    if (ylink.isEmpty() || yartist.isEmpty() || ysong.isEmpty()) {
        QMessageBox::warning(this, tr("Input Missing"), tr("Please provide a Video Link, Artist, and Song title."));
        ui->bt_youtube_getIt->setEnabled(true);
        ui->frame_loading->hide();
        ui->txt_teminal_yd1->appendPlainText("Operation cancelled: Missing input.");
        return;
    }


    // 2. DB
    DatabaseCredentials creds;
    QSqlDatabase mainDb = QSqlDatabase::database("xfb_connection"); // Get handle to main connection
    if (!mainDb.isValid()) {
        QMessageBox::critical(this, tr("Database Error"), tr("Main database connection 'xfb_connection' is not valid. Cannot proceed."));
        ui->bt_youtube_getIt->setEnabled(true);
        ui->frame_loading->hide();
        return;
    }
    creds.driver = mainDb.driverName();
    creds.databaseName = mainDb.databaseName();
    creds.hostName = mainDb.hostName();     // May be empty depending on driver
    creds.userName = mainDb.userName();     // May be empty
    creds.password = mainDb.password();     // May be empty
    creds.connectionName = mainDb.connectionName(); // Store original

    // Create a watcher to monitor the background task
    QFutureWatcher<DownloadResult> *watcher = new QFutureWatcher<DownloadResult>(this);

    // Connect the watcher's finished signal to a slot in this class (the UI thread)
    connect(watcher, &QFutureWatcher<DownloadResult>::finished, this, [this, watcher]() {
        DownloadResult result = watcher->result(); // Get the result from the future

        // Update UI safely from the main thread
        ui->txt_teminal_yd1->appendPlainText("--- Task Finished ---");
        ui->txt_teminal_yd1->appendPlainText(result.consoleOutput); // Append all console output at the end

        if (result.success) {
            QMessageBox::information(this, tr("yt-dlp Downloader"), result.message);
        } else {
            QMessageBox::critical(this, tr("yt-dlp Downloader Error"), result.message);
        }

        ui->bt_youtube_getIt->setEnabled(true); // Re-enable button
        ui->frame_loading->hide(); // Hide loading indicator
        watcher->deleteLater(); // Clean up the watcher
    });

    // 3. Run the task in a separate thread from Qt's global thread pool
    QFuture<DownloadResult> future = QtConcurrent::run(
        processDownloadTask,
        ylink, yartist, ysong, g1, g2, country, pub_date,
        creds // Pass the credentials struct
        );

    // Set the future for the watcher to monitor
    watcher->setFuture(future);

    ui->txt_teminal_yd1->appendPlainText("Download task submitted to background thread...");
    // The function returns immediately, UI remains responsive
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

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQueryModel * model=new QSqlQueryModel();
    QSqlQueryModel * model2=new QSqlQueryModel();
    QSqlQuery* qry=new QSqlQuery(db);

    qry->prepare("select name from genres1");
    qry->exec();
    model->setQuery(std::move(*qry));
    ui->cbox_g1->setModel(model);

    qry->prepare("select name from genres1");
    qry->exec();
    model2->setQuery(std::move(*qry));
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
    
    // Add the tab stop distance setting here
    ui->txt_teminal_yd1->setTabStopDistance(80);
}
