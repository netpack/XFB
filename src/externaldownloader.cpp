#include "externaldownloader.h"
#include "ui_externaldownloader.h"
#include "QProcess"
#include <QMessageBox>
#include <QtSql>
#include <QDir>
#include "addgenre.h"
#include "player.h"
#include "services/DependencyChecker.h"
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

    // When a link is pasted (or typed), automatically scrape the video's
    // title/artist after a short debounce and pre-fill the fields.
    m_metaDebounce = new QTimer(this);
    m_metaDebounce->setSingleShot(true);
    m_metaDebounce->setInterval(600);
    connect(m_metaDebounce, &QTimer::timeout, this, &externaldownloader::fetchVideoDetails);
    connect(ui->txt_videoLink, &QLineEdit::textChanged, this,
            [this]() { m_metaDebounce->start(); });

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

void externaldownloader::showLoadingFrame(){
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

// Resolve an external executable robustly across platforms.
//
// On macOS a GUI app launched from Finder/.app does NOT inherit the user's
// shell PATH, so QStandardPaths::findExecutable() routinely fails to locate
// tools installed by Homebrew/MacPorts/pip (e.g. /opt/homebrew/bin) even when
// they are present. That is exactly why yt-dlp's postprocessing reports
// "ffprobe and ffmpeg not found": ffmpeg is installed, but neither XFB nor
// yt-dlp can see it on PATH. Check PATH first, then fall back to the same
// well-known locations XFB already probes for yt-dlp.
static QString resolveExecutable(const QString &name)
{
    QString path = QStandardPaths::findExecutable(name);
    if (!path.isEmpty()) {
        return path;
    }

#if defined(Q_OS_MACOS)
    const QStringList commonDirs = {
        "/opt/homebrew/bin",                 // Homebrew on Apple Silicon
        "/usr/local/bin",                    // Homebrew on Intel
        "/opt/local/bin",                    // MacPorts
        QDir::homePath() + "/.local/bin",    // pip user installs
        "/usr/bin"                           // system
    };
#elif defined(Q_OS_UNIX)
    const QStringList commonDirs = {
        "/usr/local/bin",
        "/usr/bin",
        QDir::homePath() + "/.local/bin"
    };
#else
    const QStringList commonDirs = {};
#endif

    for (const QString &dir : commonDirs) {
        const QString candidate = dir + "/" + name;
        const QFileInfo info(candidate);
        if (info.exists() && info.isExecutable()) {
            return candidate;
        }
    }
    return QString();
}

// Locate the yt-dlp executable. Prefer the self-updating binary XFB installs in
// ~/.local/bin (it stays current via "yt-dlp -U"); then fall back to PATH and,
// on macOS/Windows, to common install locations / filename variants. Shared by
// the single-video and playlist download paths.
static QString findYtDlpExecutable()
{
    {
#ifdef Q_OS_WIN
        const QString localYtDlp = QDir::homePath() + "/.local/bin/yt-dlp.exe";
#else
        const QString localYtDlp = QDir::homePath() + "/.local/bin/yt-dlp";
#endif
        const QFileInfo localInfo(localYtDlp);
        if (localInfo.exists() && localInfo.isExecutable()) {
            return localYtDlp;
        }
    }

    QString ytdlpPath = QStandardPaths::findExecutable("yt-dlp");

#ifdef Q_OS_WIN
    if (ytdlpPath.isEmpty()) ytdlpPath = QStandardPaths::findExecutable("yt-dlp.exe");
    if (ytdlpPath.isEmpty()) ytdlpPath = QStandardPaths::findExecutable("yt-dlp.cmd");
    if (ytdlpPath.isEmpty()) ytdlpPath = QStandardPaths::findExecutable("yt-dlp.bat");
#endif

#ifdef Q_OS_MACOS
    // GUI apps don't inherit the shell PATH; check common install locations.
    if (ytdlpPath.isEmpty()) {
        const QStringList commonPaths = {
            "/opt/homebrew/bin/yt-dlp",              // Homebrew on Apple Silicon
            "/usr/local/bin/yt-dlp",                 // Homebrew on Intel Macs
            "/opt/local/bin/yt-dlp",                 // MacPorts
            QDir::homePath() + "/.local/bin/yt-dlp", // pip user install
            "/usr/bin/yt-dlp"                        // System install
        };
        for (const QString &path : commonPaths) {
            if (QFile::exists(path)) {
                ytdlpPath = path;
                break;
            }
        }
    }
#endif

    return ytdlpPath;
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
    DatabaseCredentials dbCreds,
    bool skipUpdateCheck
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

    // 1. Sanitize Artist/Song for the filename. A "/" in a title (SoundCloud
    //    tracks like "Artist / Song") becomes a directory separator in the
    //    yt-dlp output template and sends the file into an unintended
    //    subdirectory with trailing/leading-space names; the rest of the set
    //    is illegal on Windows. The database keeps the original artist/song
    //    text — only the on-disk name is sanitized.
    const auto sanitizeFilenameComponent = [](QString s) {
        static const QRegularExpression illegalChars(
            QStringLiteral("[/\\\\:*?\"<>|]"));
        s.replace(illegalChars, QStringLiteral("-"));
        static const QRegularExpression spaceRuns(QStringLiteral("\\s{2,}"));
        s.replace(spaceRuns, QStringLiteral(" "));
        return s.trimmed();
    };
    QString safeArtist = sanitizeFilenameComponent(yartist);
    QString safeSong = sanitizeFilenameComponent(ysong);

    if (safeArtist.isEmpty() || safeSong.isEmpty()) {
        result.success = false;
        result.message = "Error: Artist and Song name cannot be empty.";
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }

    // 2. Determine Music Directory (must be user-writable).
    // Honor the user's configured MusicPath from xfb.conf (set in Options).
    // When it's not configured, default to the standard, writable Music
    // location in a dedicated XFB subfolder (e.g. ~/Music/XFB). The previous
    // executable-relative path resolved to /usr/music for installed packages,
    // which the user cannot write to.
    QString musicBaseDir;
    {
        const QString configFileName = "xfb.conf";
        const QString writableConfigPath =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QSettings settings(writableConfigPath + "/" + configFileName, QSettings::IniFormat);
        musicBaseDir = settings.value("MusicPath").toString().trimmed();
    }

    if (musicBaseDir.isEmpty()) {
        // Default: <Music>/XFB, with fallbacks if the Music location is undefined.
        QString base = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        if (base.isEmpty()) {
            const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
            base = home.isEmpty()
                       ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       : QDir(home).filePath("Music");
        }
        musicBaseDir = QDir(base).filePath("XFB");
    }

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

    // If this track was already downloaded (any supported audio extension),
    // skip the download entirely instead of re-fetching and re-encoding it.
    {
        const QStringList knownExts = {"mp3", "ogg", "opus", "m4a", "aac", "webm"};
        QString existingPath;
        for (const QString &ext : knownExts) {
            const QString candidate = musicDir.absoluteFilePath(targetBaseFilename + "." + ext);
            if (QFileInfo::exists(candidate)) {
                existingPath = candidate;
                break;
            }
        }

        if (!existingPath.isEmpty()) {
            appendOutput("Already downloaded — skipping: " + existingPath);

            bool inDb = false;
            {
                QSqlQuery q(db);
                q.prepare("SELECT 1 FROM musics WHERE path = ?");
                q.addBindValue(existingPath);
                if (q.exec() && q.next())
                    inDb = true;
            }

            if (inDb) {
                appendOutput("Track is already in the library — nothing to do.");
            } else {
                // The file exists on disk but the library lost track of it:
                // re-register it instead of downloading a duplicate.
                QString dur = "-";
                QString exifPath = QStandardPaths::findExecutable("exiftool");
#ifdef Q_OS_WIN
                if (exifPath.isEmpty()) exifPath = QStandardPaths::findExecutable("exiftool.exe");
#endif
                if (!exifPath.isEmpty()) {
                    QProcess p;
                    p.start(exifPath, {"-T", "-Duration", existingPath});
                    if (p.waitForFinished(15000)) {
                        const QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
                        if (!out.isEmpty())
                            dur = out;
                    } else {
                        p.kill();
                    }
                }
                QSqlQuery ins(db);
                ins.prepare("INSERT INTO musics (id, artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                            "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, 0, '-')");
                ins.addBindValue(yartist);
                ins.addBindValue(ysong);
                ins.addBindValue(g1);
                ins.addBindValue(g2);
                ins.addBindValue(country);
                ins.addBindValue(pub_date);
                ins.addBindValue(existingPath);
                ins.addBindValue(dur);
                if (ins.exec())
                    appendOutput("Existing file re-registered in the library.");
                else
                    appendOutput("Warning: could not register the existing file: " + ins.lastError().text());
            }

            result.success = true;
            result.message = "Skipped: \"" + targetBaseFilename + "\" is already downloaded.";
            db.close();
            QSqlDatabase::removeDatabase(workerConnectionName);
            result.consoleOutput = consoleLines.join("\n");
            return result;
        }
    }

    // 3. Find yt-dlp executable.
    QString ytdlpPath = findYtDlpExecutable();

    if (ytdlpPath.isEmpty()) {
        result.success = false;
        result.message = "Error: 'yt-dlp' executable not found in PATH or common installation locations. Please install yt-dlp and ensure it's accessible.";
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }
    appendOutput("Found yt-dlp at: " + ytdlpPath);

    // Best-effort update of yt-dlp before use. YouTube changes frequently and an
    // out-of-date yt-dlp commonly fails to extract media. This is non-fatal:
    // - For a self-contained/pip install, "yt-dlp -U" updates it in place.
    // - For a package-manager install it simply reports it can't self-update;
    //   we log that and continue with the installed version.
    // Skipped when the caller already updated yt-dlp (e.g. the playlist path
    // updates once up front rather than before every single video).
    if (!skipUpdateCheck) {
        appendOutput("Checking for yt-dlp updates...");
        QProcess updateProc;
        updateProc.setProcessChannelMode(QProcess::MergedChannels);
        updateProc.start(ytdlpPath, {"-U"});
        if (updateProc.waitForStarted(10000)) {
            // Cap the update so a slow/stuck network can't block the download.
            if (updateProc.waitForFinished(60000)) {
                const QString updateOut = QString::fromUtf8(updateProc.readAll()).trimmed();
                if (!updateOut.isEmpty()) {
                    appendOutput(updateOut);
                }
            } else {
                updateProc.kill();
                updateProc.waitForFinished(2000);
                appendOutput("yt-dlp update timed out; continuing with the installed version.");
            }
        } else {
            appendOutput("Could not run yt-dlp update; continuing with the installed version.");
        }
    }

    // Determine the desired output audio format and download options. Honor the
    // user's configuration from xfb.conf (Options → Downloads (yt-dlp)).
    QString audioFormat;
    bool keepVideo = false;
    bool embedThumbnail = false;
    bool embedMetadata = true;
    {
        const QString configFileName = "xfb.conf";
        const QString writableConfigPath =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QSettings settings(writableConfigPath + "/" + configFileName, QSettings::IniFormat);
        audioFormat = settings.value("MusicFormat", "opus").toString().trimmed().toLower();
        keepVideo = settings.value("MusicKeepVideo", false).toBool();
        embedThumbnail = settings.value("MusicEmbedThumbnail", false).toBool();
        embedMetadata = settings.value("MusicEmbedMetadata", true).toBool();
    }
    if (audioFormat != "mp3" && audioFormat != "ogg" && audioFormat != "opus") {
        audioFormat = "opus";
    }

    // 4. Prepare and run the yt-dlp command.
    // Resolve the actual encoder/format based on what the local ffmpeg can do.
    // yt-dlp's "opus" needs libopus and "vorbis" needs libvorbis; some ffmpeg
    // builds lack one or the other. We map the requested format to the best
    // available encoder, falling back through the open formats to mp3.
    //
    // Use resolveExecutable (not QStandardPaths::findExecutable) so ffmpeg is
    // found in Homebrew/MacPorts locations even when the macOS GUI app has no
    // shell PATH. When found, we pass --ffmpeg-location to yt-dlp below, which
    // also lets yt-dlp locate ffprobe in the same directory.
    const QString ffmpegPath = resolveExecutable("ffmpeg");

    // Gather available ffmpeg encoder names once.
    QString ffmpegEncoders;
    if (!ffmpegPath.isEmpty()) {
        QProcess encProc;
        encProc.start(ffmpegPath, {"-hide_banner", "-encoders"});
        if (encProc.waitForStarted(8000) && encProc.waitForFinished(15000)) {
            ffmpegEncoders = QString::fromUtf8(encProc.readAllStandardOutput());
        }
    }
    auto hasEncoder = [&](const QString &name) {
        // Encoder lines look like " A....D libopus   libopus Opus ..."
        return ffmpegEncoders.contains(" " + name + " ");
    };

    QString ytdlpAudioFormat;  // value for yt-dlp --audio-format
    QString effectiveExt;      // extension yt-dlp will produce
    if (audioFormat == "opus") {
        if (hasEncoder("libopus")) {
            ytdlpAudioFormat = "opus";
            effectiveExt = "opus";
        } else if (hasEncoder("libvorbis")) {
            ytdlpAudioFormat = "vorbis";
            effectiveExt = "ogg";
            appendOutput("Note: ffmpeg has no libopus encoder; using Vorbis (.ogg) instead.");
        } else {
            ytdlpAudioFormat = "mp3";
            effectiveExt = "mp3";
            appendOutput("Note: ffmpeg lacks Opus/Vorbis encoders; falling back to MP3.");
        }
    } else if (audioFormat == "ogg") {
        if (hasEncoder("libvorbis")) {
            ytdlpAudioFormat = "vorbis";
            effectiveExt = "ogg";
        } else if (hasEncoder("libopus")) {
            ytdlpAudioFormat = "opus";
            effectiveExt = "opus";
            appendOutput("Note: ffmpeg has no libvorbis encoder; using Opus (.opus) instead.");
        } else {
            ytdlpAudioFormat = "mp3";
            effectiveExt = "mp3";
            appendOutput("Note: ffmpeg lacks Ogg encoders; falling back to MP3.");
        }
    } else {
        ytdlpAudioFormat = "mp3";
        effectiveExt = "mp3";
    }

    // CBR for MP3 (Qt's FFmpeg backend can stall on VBR MP3s whose timestamps
    // don't advance cleanly); best-quality VBR for opus/vorbis.
    const QString audioQuality = (ytdlpAudioFormat == "mp3") ? QStringLiteral("192K")
                                                             : QStringLiteral("0");

    QStringList ytdlpArgs;
    ytdlpArgs << "--extract-audio"
              << "--audio-format" << ytdlpAudioFormat
              << "--audio-quality" << audioQuality
              << "-o" << outputPathTemplate;

    // Ask yt-dlp to print the final file path after post-processing/move. This
    // is authoritative and accounts for OS-specific filename sanitization we
    // cannot reliably predict (e.g. yt-dlp turns ':' into '#' on Windows).
    // "--print" implies "--simulate", so pair it with "--no-simulate" to still
    // perform the actual download.
    ytdlpArgs << "--no-simulate" << "--print" << "after_move:filepath";
    if (!ffmpegPath.isEmpty()) {
        ytdlpArgs << "--ffmpeg-location" << ffmpegPath;
    }

    if (ytdlpAudioFormat == "mp3") {
        // Omit the Xing/LAME header. For a CBR MP3 it isn't needed for seeking,
        // and without it the decoder won't attempt gapless encoder-delay
        // "skipped samples" handling — which is what stalls Qt's FFmpeg
        // multimedia backend right at the start of playback.
        ytdlpArgs << "--postprocessor-args" << "ffmpeg:-write_xing 0";
    }

    // Be resilient to transient network refusals from the media servers
    ytdlpArgs << "--retries" << "10" << "--fragment-retries" << "10";

    // Optional, user-configurable behaviours (Options → Downloads (yt-dlp)).
    if (keepVideo) {
        // Keep the original downloaded video file next to the extracted audio.
        ytdlpArgs << "--keep-video";
    } else {
        // Audio-only use: don't download the video stream at all. Besides
        // being much faster, the video media URLs are the ones YouTube most
        // often rejects with HTTP 403.
        ytdlpArgs << "-f" << "bestaudio/best";
    }
    if (embedThumbnail) {
        ytdlpArgs << "--embed-thumbnail";
    }

    // YouTube extraction needs a JavaScript runtime (yt-dlp EJS). deno is
    // enabled by default; if it isn't present but another supported runtime is,
    // tell yt-dlp to use it. The "--js-runtimes" option only exists in newer
    // yt-dlp builds, so probe --help first: passing it to an older yt-dlp fails
    // with "no such option: --js-runtimes". Older builds don't need the flag.
    bool ytdlpSupportsJsRuntimes = false;
    {
        QProcess helpProc;
        helpProc.start(ytdlpPath, {"--help"});
        if (helpProc.waitForFinished(15000)) {
            const QString helpText = QString::fromUtf8(helpProc.readAllStandardOutput())
                                   + QString::fromUtf8(helpProc.readAllStandardError());
            ytdlpSupportsJsRuntimes = helpText.contains("--js-runtimes");
        }
    }

    if (ytdlpSupportsJsRuntimes && resolveExecutable("deno").isEmpty()) {
        QString jsRuntimeArg;
        QString nodePath = resolveExecutable("node");
        if (nodePath.isEmpty()) {
            // Debian/Ubuntu/Mint historically install the binary as "nodejs".
            const QString nodejsPath = resolveExecutable("nodejs");
            if (!nodejsPath.isEmpty()) {
                // Point yt-dlp's "node" runtime at the actual binary path.
                jsRuntimeArg = "node:" + nodejsPath;
            }
        } else {
            jsRuntimeArg = "node";
        }
        if (jsRuntimeArg.isEmpty()) {
            const QString bunPath = resolveExecutable("bun");
            if (!bunPath.isEmpty()) {
                jsRuntimeArg = "bun";
            }
        }
        if (!jsRuntimeArg.isEmpty()) {
            ytdlpArgs << "--js-runtimes" << jsRuntimeArg;
            appendOutput("Using JavaScript runtime: " + jsRuntimeArg);
        } else {
            appendOutput("Warning: no JavaScript runtime (deno/node/bun) found. "
                         "Some YouTube formats may be unavailable.");
        }
    }

    ytdlpArgs << ylink; // The URL (positional argument, kept last)

    // YouTube's media servers intermittently reject the URLs handed out to
    // one player client with "HTTP Error 403: Forbidden" while another
    // client works fine. When that happens, retry with alternative clients
    // before giving up.
    const QList<QStringList> fallbackClientArgs = {
        {}, // first attempt: yt-dlp defaults
        {"--extractor-args", "youtube:player_client=default,web_safari,android"},
        {"--extractor-args", "youtube:player_client=android,web", "--force-ipv4"},
    };

    bool ytdlpOk = false;
    int lastExitCode = -1;
    QString successOutput; // captured output of the successful attempt
    for (int attempt = 0; attempt < fallbackClientArgs.size(); ++attempt) {
        QStringList attemptArgs = ytdlpArgs;
        for (const QString &extra : fallbackClientArgs[attempt])
            attemptArgs.insert(attemptArgs.size() - 1, extra); // keep URL last

        if (attempt > 0)
            appendOutput(QString("HTTP 403 from YouTube — retrying with an "
                                 "alternative player client (attempt %1 of %2)...")
                             .arg(attempt + 1).arg(fallbackClientArgs.size()));
        appendOutput("Executing: " + ytdlpPath + " " + attemptArgs.join(" "));

        QProcess ytdlpProcess;
        ytdlpProcess.setProcessChannelMode(QProcess::MergedChannels); // Combine stdout and stderr
        ytdlpProcess.setProgram(ytdlpPath);
        ytdlpProcess.setArguments(attemptArgs);

        QString attemptOutput;
        QEventLoop loopYtdlp;
        QObject::connect(&ytdlpProcess, &QProcess::finished, &loopYtdlp, &QEventLoop::quit);
        QObject::connect(&ytdlpProcess, &QProcess::readyRead, [&]() {
            const QString chunk = QString::fromUtf8(ytdlpProcess.readAll());
            attemptOutput += chunk;
            appendOutput(chunk); // Read output as it comes
        });

        ytdlpProcess.start();
        if (attempt == 0)
            appendOutput("Putting hamsters on the job... hold on... *a tribute to torrentz");
        loopYtdlp.exec(); // Wait for finished signal

        // Read any remaining output
        const QString tail = QString::fromUtf8(ytdlpProcess.readAll());
        attemptOutput += tail;
        appendOutput(tail);

        lastExitCode = ytdlpProcess.exitCode();
        if (ytdlpProcess.exitStatus() == QProcess::NormalExit && lastExitCode == 0) {
            ytdlpOk = true;
            successOutput = attemptOutput;
            break;
        }

        // Only the 403 rejection is worth retrying with another client;
        // other failures (bad URL, private video...) will fail again anyway.
        const bool was403 = attemptOutput.contains(QLatin1String("403"))
                            && attemptOutput.contains(QLatin1String("Forbidden"), Qt::CaseInsensitive);
        if (!was403)
            break;
    }

    if (!ytdlpOk) {
        result.success = false;
        result.message = "Error: yt-dlp failed (Exit code: " + QString::number(lastExitCode) + "). Check console output for details.";
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }

    appendOutput("yt-dlp finished successfully.");

    // Find the downloaded file. yt-dlp replaces %(ext)s with the actual
    // extension, which matches the requested audio format. Check the chosen
    // format first, then fall back to other common audio extensions.
    QStringList possibleExtensions = {effectiveExt, audioFormat, "mp3", "ogg", "opus", "m4a", "aac", "webm"};
    possibleExtensions.removeDuplicates();
    QString finalFilepath;
    QString finalFilename;
    QFileInfo fileInfo;
    bool fileFound = false;

    // 1) Preferred: use the exact path yt-dlp printed (--print after_move:filepath).
    //    Scan output lines from the end and take the last one that is an actual
    //    file on disk. This is robust to filename sanitization differences.
    {
        const QStringList outLines =
            successOutput.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
        for (int i = outLines.size() - 1; i >= 0 && !fileFound; --i) {
            const QString cand = outLines.at(i).trimmed();
            if (cand.isEmpty()) {
                continue;
            }
            QFileInfo fi(cand);
            if (fi.isFile()) {
                finalFilepath = fi.absoluteFilePath();
                finalFilename = fi.fileName();
                fileInfo.setFile(finalFilepath);
                fileFound = true;
            }
        }
    }

    // 2) Fallback: guess the filename from the template + a known extension.
    if (!fileFound) {
        for (const QString& ext : possibleExtensions) {
            finalFilename = targetBaseFilename + "." + ext;
            finalFilepath = musicDir.absoluteFilePath(finalFilename);
            fileInfo.setFile(finalFilepath);
            if (fileInfo.exists()) {
                fileFound = true;
                break;
            }
        }
    }

    if (!fileFound) {
        result.success = false;
        result.message = "Error: Expected output file not found after download. Checked extensions: " + possibleExtensions.join(", ");
        appendOutput(result.message);
        result.consoleOutput = consoleLines.join("\n");
        return result;
    }
    appendOutput("Downloaded file seems to be at: " + finalFilepath);

    // When "keep video" is enabled, yt-dlp's --keep-video leaves the separately
    // downloaded component streams (e.g. ".f137.mp4", ".f251.webm") on disk in
    // addition to the merged video and the extracted audio — cluttering the
    // music folder with several files. Keep only the single largest video file
    // (the merged one) and the audio; remove the leftover component streams.
    if (keepVideo) {
        const QStringList videoExts = {"mkv", "mp4", "webm", "m4v", "mov", "avi", "flv"};
        const QFileInfoList entries = musicDir.entryInfoList(QDir::Files);
        QList<QFileInfo> videoFiles;
        QString largestVideoPath;
        qint64 largestSize = -1;
        for (const QFileInfo &fi : entries) {
            if (fi.fileName().startsWith(targetBaseFilename) &&
                videoExts.contains(fi.suffix().toLower())) {
                videoFiles.append(fi);
                if (fi.size() > largestSize) {
                    largestSize = fi.size();
                    largestVideoPath = fi.absoluteFilePath();
                }
            }
        }
        for (const QFileInfo &fi : videoFiles) {
            if (fi.absoluteFilePath() != largestVideoPath) {
                if (QFile::remove(fi.absoluteFilePath())) {
                    appendOutput("Removed extra video stream: " + fi.fileName());
                }
            }
        }
        if (!largestVideoPath.isEmpty()) {
            appendOutput("Kept video file: " + QFileInfo(largestVideoPath).fileName());
        }
    }

    // Embed the user-entered metadata (artist/title, plus genre) into the file
    // itself so it is self-describing — e.g. for "Retrieve metadata from file".
    // yt-dlp downloads carry no tags by default. Use ffmpeg with stream copy
    // (no re-encode) to write Vorbis comments / ID3 tags, then swap the file in.
    if (embedMetadata && !ffmpegPath.isEmpty() && (!yartist.isEmpty() || !ysong.isEmpty())) {
        const QString taggedTmp = finalFilepath + ".tagging." + fileInfo.suffix();
        QStringList tagArgs;
        tagArgs << "-y" << "-i" << finalFilepath << "-map" << "0" << "-c" << "copy";
        if (!yartist.isEmpty()) tagArgs << "-metadata" << ("artist=" + yartist);
        if (!ysong.isEmpty())   tagArgs << "-metadata" << ("title=" + ysong);
        if (!g1.isEmpty() && g1 != "Genre")  tagArgs << "-metadata" << ("genre=" + g1);
        if (effectiveExt == "mp3") {
            // Preserve the no-Xing-header property (avoids Qt FFmpeg stall on
            // some MP3s) when re-muxing to add tags.
            tagArgs << "-write_xing" << "0";
        }
        tagArgs << taggedTmp;

        QProcess tagProc;
        tagProc.setProcessChannelMode(QProcess::MergedChannels);
        tagProc.start(ffmpegPath, tagArgs);
        if (tagProc.waitForStarted(8000) && tagProc.waitForFinished(60000) &&
            tagProc.exitStatus() == QProcess::NormalExit && tagProc.exitCode() == 0 &&
            QFileInfo(taggedTmp).size() > 0) {
            // Replace the original with the tagged version.
            if (QFile::remove(finalFilepath) && QFile::rename(taggedTmp, finalFilepath)) {
                appendOutput("Embedded metadata into the downloaded file.");
            } else {
                QFile::remove(taggedTmp); // keep original if swap failed
                appendOutput("Note: could not replace file with the tagged version; keeping original.");
            }
        } else {
            QFile::remove(taggedTmp);
            appendOutput("Note: embedding metadata failed; the file was kept untagged.");
        }
    }

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


// --- Playlist download ----------------------------------------------------

// A SoundCloud set (album/playlist), e.g. https://soundcloud.com/bandua/sets/bandua
static bool isSoundCloudSetUrl(const QString &url)
{
    return url.contains("soundcloud.com/", Qt::CaseInsensitive) &&
           url.contains("/sets/", Qt::CaseInsensitive);
}

// Aggregate result for downloading every entry of a playlist.
struct PlaylistResult {
    int total = 0;       // entries discovered in the playlist
    int succeeded = 0;   // newly downloaded and added
    int skipped = 0;     // already present in the library, or unusable entries
    int failed = 0;      // entries that errored out
    bool fatal = false;  // true if the whole operation could not start
    QString message;     // human-readable summary / error
    QString consoleOutput;
};

// Download every video in a playlist as audio and add each to the library.
//
// Strategy: enumerate the playlist cheaply with "--flat-playlist" (no media is
// fetched), then download each entry through the existing single-video pipeline
// (processDownloadTask) so all the format/ffmpeg/tagging/DB logic is reused.
// Per-video Artist/Song are guessed from the title — music titles are commonly
// "Artist - Song"; otherwise the channel name is used as the artist.
PlaylistResult processPlaylistDownloadTask(
    QString playlistUrl,
    QString g1,
    QString g2,
    QString country,
    QString pub_date,
    DatabaseCredentials dbCreds
    ) {
    PlaylistResult agg;
    QStringList consoleLines;

    auto appendOutput = [&](const QString &line) {
        qDebug().noquote() << "[Playlist]" << line;
        consoleLines.append(line);
    };

    appendOutput("Starting playlist download task in thread: "
                 + QString::number((quintptr)QThread::currentThreadId()));

    const QString ytdlpPath = findYtDlpExecutable();
    if (ytdlpPath.isEmpty()) {
        agg.fatal = true;
        agg.message = "Error: 'yt-dlp' executable not found. Please install yt-dlp and try again.";
        appendOutput(agg.message);
        agg.consoleOutput = consoleLines.join("\n");
        return agg;
    }
    appendOutput("Found yt-dlp at: " + ytdlpPath);

    // Update yt-dlp once for the entire playlist (not per video).
    appendOutput("Checking for yt-dlp updates...");
    {
        QProcess updateProc;
        updateProc.setProcessChannelMode(QProcess::MergedChannels);
        updateProc.start(ytdlpPath, {"-U"});
        if (updateProc.waitForStarted(10000)) {
            if (updateProc.waitForFinished(60000)) {
                const QString updateOut = QString::fromUtf8(updateProc.readAll()).trimmed();
                if (!updateOut.isEmpty()) appendOutput(updateOut);
            } else {
                updateProc.kill();
                updateProc.waitForFinished(2000);
                appendOutput("yt-dlp update timed out; continuing with the installed version.");
            }
        } else {
            appendOutput("Could not run yt-dlp update; continuing with the installed version.");
        }
    }

    // Enumerate the playlist entries without downloading any media. We print one
    // line per entry as "id<US>title<US>uploader<US>webpage_url<US>url", using
    // the ASCII Unit Separator (0x1F) as a delimiter so it can't collide with
    // text in titles.
    //
    // YouTube playlists are enumerated with "--flat-playlist" (cheap: one index
    // request, titles included). SoundCloud sets can NOT use the flat index —
    // its flat entries carry no title/uploader (both "NA") and some are bare
    // api-v2.soundcloud.com URLs. Without --flat-playlist yt-dlp fetches each
    // track's metadata (~1s per track, still no media download since --print
    // implies --simulate), which yields real titles and canonical permalinks.
    const bool soundcloudSet = isSoundCloudSetUrl(playlistUrl);
    const QChar US(0x1f);
    appendOutput(soundcloudSet ? "Fetching SoundCloud set entries (with metadata)..."
                               : "Fetching playlist entries...");
    QStringList entryLines;
    {
        QProcess listProc;
        QStringList listArgs;
        if (!soundcloudSet) listArgs << "--flat-playlist";
        listArgs << "--ignore-errors"
                 << "--no-warnings"
                 << "--print"
                 << QString("%(id)s%1%(title)s%1%(uploader)s%1%(webpage_url)s%1%(url)s").arg(US)
                 << playlistUrl;
        listProc.start(ytdlpPath, listArgs);
        if (!listProc.waitForStarted(15000)) {
            agg.fatal = true;
            agg.message = "Error: could not start yt-dlp to read the playlist.";
            appendOutput(agg.message);
            agg.consoleOutput = consoleLines.join("\n");
            return agg;
        }
        // Reading a playlist index is network-bound; cap it generously. The
        // per-track metadata pass for SoundCloud sets gets a higher cap.
        if (!listProc.waitForFinished(soundcloudSet ? 300000 : 180000)) {
            listProc.kill();
            listProc.waitForFinished(2000);
            appendOutput("Timed out while reading the playlist.");
        }
        const QString out = QString::fromUtf8(listProc.readAllStandardOutput());
        const QString err = QString::fromUtf8(listProc.readAllStandardError()).trimmed();
        if (!err.isEmpty()) appendOutput(err);
        entryLines = out.split('\n', Qt::SkipEmptyParts);
    }

    if (entryLines.isEmpty()) {
        agg.fatal = true;
        agg.message = "No playlist entries were found. Make sure the link points to a public "
                      "YouTube playlist (containing \"list=\") or SoundCloud set "
                      "(soundcloud.com/<artist>/sets/<set>).";
        appendOutput(agg.message);
        agg.consoleOutput = consoleLines.join("\n");
        return agg;
    }

    agg.total = entryLines.size();
    appendOutput(QString("Found %1 entries in the playlist.").arg(agg.total));

    int index = 0;
    for (const QString &line : entryLines) {
        ++index;
        const QStringList parts = line.split(US);
        const QString id = parts.value(0).trimmed();
        const QString title = parts.value(1).trimmed();
        const QString uploader = parts.value(2).trimmed();
        const QString webpageUrl = parts.value(3).trimmed();
        const QString entryUrl = parts.value(4).trimmed();

        if (id.isEmpty() || id == "NA") {
            appendOutput(QString("[%1/%2] Skipping entry with no video id.")
                             .arg(index).arg(agg.total));
            agg.skipped++;
            continue;
        }

        // Pick the URL to download. webpage_url is the canonical page (set by
        // the full-metadata pass; "NA" for flat entries). Flat entries put the
        // entry link in url instead. YouTube ids keep the historical watch-URL
        // construction as a last resort.
        QString videoUrl;
        if (webpageUrl.startsWith("http")) {
            videoUrl = webpageUrl;
        } else if (entryUrl.startsWith("http")) {
            videoUrl = entryUrl;
        } else {
            videoUrl = "https://www.youtube.com/watch?v=" + id;
        }

        // Guess Artist/Song from the title. "Artist - Song" is the common form
        // for music; otherwise fall back to the channel name (minus YouTube's
        // " - Topic" auto-channel suffix) as the artist.
        QString artist;
        QString song;
        const int sep = title.indexOf(" - ");
        if (sep > 0) {
            artist = title.left(sep).trimmed();
            song = title.mid(sep + 3).trimmed();
        } else {
            QString channel = uploader;
            channel.remove(QRegularExpression("\\s*-\\s*Topic$"));
            artist = channel.trimmed();
            song = title;
        }
        if (artist.isEmpty() || artist == "NA") artist = "Unknown Artist";
        if (song.isEmpty()   || song == "NA")   song = id;

        appendOutput(QString("[%1/%2] Downloading: %3 - %4")
                         .arg(index).arg(agg.total).arg(artist, song));

        // Reuse the single-video pipeline. yt-dlp was already updated above, so
        // skip the per-video update check.
        DownloadResult r = processDownloadTask(
            videoUrl, artist, song, g1, g2, country, pub_date, dbCreds, /*skipUpdateCheck=*/true);

        if (!r.consoleOutput.isEmpty()) consoleLines.append(r.consoleOutput);

        if (r.success) {
            if (r.message.contains("already in the database", Qt::CaseInsensitive)) {
                agg.skipped++;
            } else {
                agg.succeeded++;
            }
        } else {
            agg.failed++;
            appendOutput(QString("[%1/%2] Failed: %3").arg(index).arg(agg.total).arg(r.message));
        }
    }

    agg.message = QString("Playlist finished: %1 downloaded, %2 already present/skipped, "
                          "%3 failed (of %4 total).")
                      .arg(agg.succeeded).arg(agg.skipped).arg(agg.failed).arg(agg.total);
    appendOutput(agg.message);
    agg.consoleOutput = consoleLines.join("\n");
    return agg;
}


// Ensure yt-dlp and ffmpeg/ffprobe are installed BEFORE the download runs.
//
// Downloads execute in a background worker (processDownloadTask) that has no UI
// thread and therefore cannot prompt the user to install anything. If ffmpeg is
// missing there, yt-dlp's audio extraction aborts with
// "Postprocessing: ffprobe and ffmpeg not found". We do the provisioning here,
// on the UI thread, so XFB can install the dependencies on demand and with the
// user's consent (Homebrew on macOS, the system package manager elsewhere).
bool externaldownloader::ensureDownloadDependencies()
{
    DependencyChecker checker;

    // yt-dlp: provision the self-updating binary in ~/.local/bin if needed. The
    // worker locates it via the same path, so this keeps downloads working even
    // on a clean machine.
    checker.ensureYtDlp(this);

    // ffmpeg (and ffprobe, shipped in the same package) is what yt-dlp uses to
    // extract/convert audio. Without it the download completes but the final
    // audio file is never produced. On macOS ensureDependency will also bootstrap
    // Homebrew first if it isn't installed yet.
    const bool haveFfmpeg = checker.ensureDependency(
        "ffmpeg",
        tr("Downloading and converting audio needs FFmpeg (which also provides "
           "ffprobe). Without it yt-dlp can't produce the final audio file."),
        this);

    if (!haveFfmpeg) {
        ui->txt_teminal_yd1->appendPlainText(
            "FFmpeg is not available — audio extraction would fail. Download aborted.");
    }
    return haveFfmpeg;
}

// --- The method in your externaldownloader class ---
void externaldownloader::getFile() {
    ui->bt_youtube_getIt->setEnabled(false); // Disable button immediately
    ui->txt_teminal_yd1->clear(); // Clear previous output
    ui->txt_teminal_yd1->appendPlainText("Preparing download...");
    ui->frame_loading->show(); // Show loading indicator

    // Make sure the download toolchain (yt-dlp + ffmpeg/ffprobe) is present
    // before handing off to the background worker, which can't install anything.
    if (!ensureDownloadDependencies()) {
        ui->bt_youtube_getIt->setEnabled(true);
        ui->frame_loading->hide();
        return;
    }

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
            emit musicAdded(); // Emit signal to notify that music was added to database
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
        creds, // Pass the credentials struct
        false  // skipUpdateCheck: single downloads update yt-dlp first
        );

    // Set the future for the watcher to monitor
    watcher->setFuture(future);

    ui->txt_teminal_yd1->appendPlainText("Download task submitted to background thread...");
    // The function returns immediately, UI remains responsive
}

void externaldownloader::getPlaylist() {
    ui->bt_youtube_getIt->setEnabled(false);
    if (ui->bt_youtube_getPlaylist) ui->bt_youtube_getPlaylist->setEnabled(false);
    ui->txt_teminal_yd1->clear();
    ui->txt_teminal_yd1->appendPlainText("Preparing playlist download...");
    ui->frame_loading->show();

    // Same as single downloads: provision yt-dlp + ffmpeg on the UI thread before
    // the background worker starts, so audio extraction doesn't fail with
    // "ffprobe and ffmpeg not found".
    if (!ensureDownloadDependencies()) {
        ui->bt_youtube_getIt->setEnabled(true);
        if (ui->bt_youtube_getPlaylist) ui->bt_youtube_getPlaylist->setEnabled(true);
        ui->frame_loading->hide();
        return;
    }

    QString ylink = ui->txt_videoLink->text().trimmed();

    // A YouTube playlist link must carry a "list=" parameter (unlike single
    // downloads we must NOT strip query parameters after '&', since that is
    // where the list id lives in "watch?v=...&list=..." URLs). SoundCloud sets
    // are recognized by their .../sets/... path instead.
    if (ylink.isEmpty() || (!ylink.contains("list=") && !isSoundCloudSetUrl(ylink))) {
        QMessageBox::warning(this, tr("Not a Playlist"),
            tr("Please paste a playlist link in the Video Link field: a YouTube playlist "
               "(containing \"list=\", e.g. https://www.youtube.com/playlist?list=...) or "
               "a SoundCloud set (e.g. https://soundcloud.com/artist/sets/name)."));
        ui->bt_youtube_getIt->setEnabled(true);
        if (ui->bt_youtube_getPlaylist) ui->bt_youtube_getPlaylist->setEnabled(true);
        ui->frame_loading->hide();
        return;
    }

    const QString g1 = ui->cbox_g1->currentText();
    const QString g2 = ui->cbox_g2->currentText();
    const QString country = ui->checkBox_cplp->isChecked() ? "PT" : "Other country / language";
    const QString pub_date = ui->dateEdit_publishedDate->text();

    DatabaseCredentials creds;
    QSqlDatabase mainDb = QSqlDatabase::database("xfb_connection");
    if (!mainDb.isValid()) {
        QMessageBox::critical(this, tr("Database Error"),
            tr("Main database connection 'xfb_connection' is not valid. Cannot proceed."));
        ui->bt_youtube_getIt->setEnabled(true);
        if (ui->bt_youtube_getPlaylist) ui->bt_youtube_getPlaylist->setEnabled(true);
        ui->frame_loading->hide();
        return;
    }
    creds.driver = mainDb.driverName();
    creds.databaseName = mainDb.databaseName();
    creds.hostName = mainDb.hostName();
    creds.userName = mainDb.userName();
    creds.password = mainDb.password();
    creds.connectionName = mainDb.connectionName();

    QFutureWatcher<PlaylistResult> *watcher = new QFutureWatcher<PlaylistResult>(this);
    connect(watcher, &QFutureWatcher<PlaylistResult>::finished, this, [this, watcher]() {
        PlaylistResult result = watcher->result();

        ui->txt_teminal_yd1->appendPlainText("--- Playlist Task Finished ---");
        ui->txt_teminal_yd1->appendPlainText(result.consoleOutput);

        if (result.fatal) {
            QMessageBox::critical(this, tr("Playlist Downloader"), result.message);
        } else {
            QMessageBox::information(this, tr("Playlist Downloader"), result.message);
            if (result.succeeded > 0) {
                emit musicAdded(); // Refresh the library if anything was added
            }
        }

        ui->bt_youtube_getIt->setEnabled(true);
        if (ui->bt_youtube_getPlaylist) ui->bt_youtube_getPlaylist->setEnabled(true);
        ui->frame_loading->hide();
        watcher->deleteLater();
    });

    QFuture<PlaylistResult> future = QtConcurrent::run(
        processPlaylistDownloadTask,
        ylink, g1, g2, country, pub_date, creds);
    watcher->setFuture(future);

    ui->txt_teminal_yd1->appendPlainText("Playlist task submitted to background thread...");
}

void externaldownloader::on_bt_youtube_getPlaylist_clicked()
{
    const QString ylink = ui->txt_videoLink->text().trimmed();
    if (ylink.isEmpty() || (!ylink.contains("list=") && !isSoundCloudSetUrl(ylink))) {
        QMessageBox::information(this, tr("Playlist Downloader"),
            tr("Please paste a playlist link in the Video Link field: a YouTube playlist "
               "(containing \"list=\", e.g. https://www.youtube.com/playlist?list=...) or "
               "a SoundCloud set (e.g. https://soundcloud.com/artist/sets/name)."));
        return;
    }

    const auto reply = QMessageBox::question(this, tr("Download Whole Playlist?"),
        tr("This downloads every entry of the playlist as audio and adds them to your "
           "library. Artist and Song are guessed from each entry's title, and the genres "
           "selected above are applied to all of them.\n\nThis can take a while. Continue?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return;
    }

    getPlaylist();
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

    // Refresh the genre combos with any genre just added. One query object
    // per model: setQuery(std::move(...)) guts the source query, so reusing
    // it for the second combo dereferenced a null d-pointer (segfault).
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");

    QSqlQueryModel *model = new QSqlQueryModel(this);
    QSqlQuery qry(db);
    qry.prepare("select name from genres1 order by name");
    qry.exec();
    model->setQuery(std::move(qry));
    ui->cbox_g1->setModel(model);

    QSqlQueryModel *model2 = new QSqlQueryModel(this);
    QSqlQuery qry2(db);
    qry2.prepare("select name from genres1 order by name");
    qry2.exec();
    model2->setQuery(std::move(qry2));
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
    m_lastAutoArtist.clear();
    m_lastAutoSong.clear();

    // Add the tab stop distance setting here
    ui->txt_teminal_yd1->setTabStopDistance(80);
}

void externaldownloader::fetchVideoDetails()
{
    const QString url = ui->txt_videoLink->text().trimmed();
    if (!url.startsWith("http://", Qt::CaseInsensitive)
        && !url.startsWith("https://", Qt::CaseInsensitive)) {
        return;
    }
    // A playlist link has no single title to scrape
    if (url.contains("list=") && !url.contains("watch?v=") && !url.contains("youtu.be/"))
        return;

    const QString ytdlp = findYtDlpExecutable();
    if (ytdlp.isEmpty())
        return;

    // A newer link supersedes any fetch still in flight
    if (m_metaFetch) {
        m_metaFetch->disconnect(this);
        m_metaFetch->kill();
        m_metaFetch->deleteLater();
        m_metaFetch = nullptr;
    }

    ui->txt_teminal_yd1->appendPlainText(tr("Fetching video details..."));

    m_metaFetch = new QProcess(this);
    QProcess *proc = m_metaFetch;

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, proc](int exitCode, QProcess::ExitStatus status) {
        proc->deleteLater();
        if (m_metaFetch == proc)
            m_metaFetch = nullptr;

        if (status != QProcess::NormalExit || exitCode != 0) {
            ui->txt_teminal_yd1->appendPlainText(
                tr("Could not fetch video details — fill Artist/Song manually."));
            return;
        }

        const QString line =
            QString::fromUtf8(proc->readAllStandardOutput()).split('\n').value(0).trimmed();
        const QStringList parts = line.split('\t');
        auto field = [&parts](int i) {
            const QString v = parts.value(i).trimmed();
            return (v == QLatin1String("NA") || v == QLatin1String("none")) ? QString() : v;
        };

        QString artist = field(0);
        QString song = field(1); // the "track" field, when the site provides it
        const QString title = field(2);
        QString uploader = field(3);
        uploader.remove(QRegularExpression(QStringLiteral(" - Topic$")));

        if (artist.isEmpty() || song.isEmpty()) {
            // Common convention: "Artist - Title"
            const int sep = title.indexOf(QLatin1String(" - "));
            if (sep > 0) {
                if (artist.isEmpty())
                    artist = title.left(sep).trimmed();
                if (song.isEmpty())
                    song = title.mid(sep + 3).trimmed();
            } else if (song.isEmpty()) {
                song = title;
            }
        }
        if (artist.isEmpty())
            artist = uploader;

        // Strip the usual video-title noise: (Official Video), [4K Remaster],
        // (Lyric Video), (HD)... — meaningful qualifiers like (Live ...) or
        // (Remix) are kept.
        static const QRegularExpression titleNoise(QStringLiteral(
            "\\s*[\\(\\[](official\\s+)?(music\\s+)?(video|audio|visualizer|"
            "lyrics?(\\s+video)?|hd|hq|4k(\\s+remaster(ed)?)?|"
            "remaster(ed)?(\\s+\\d{4})?)[\\)\\]]"),
            QRegularExpression::CaseInsensitiveOption);
        song.remove(titleNoise);
        song = song.trimmed();
        if (artist.isEmpty() && song.isEmpty())
            return;

        // Only fill fields the user hasn't typed into (a previous auto-fill
        // may be overwritten by a newer link's details)
        bool filled = false;
        if (!artist.isEmpty()
            && (ui->txt_artist->text().trimmed().isEmpty()
                || ui->txt_artist->text() == m_lastAutoArtist)) {
            ui->txt_artist->setText(artist);
            m_lastAutoArtist = artist;
            filled = true;
        }
        if (!song.isEmpty()
            && (ui->txt_song->text().trimmed().isEmpty()
                || ui->txt_song->text() == m_lastAutoSong)) {
            ui->txt_song->setText(song);
            m_lastAutoSong = song;
            filled = true;
        }
        ui->txt_teminal_yd1->appendPlainText(
            (filled ? tr("Auto-filled: %1 — %2") : tr("Video details: %1 — %2"))
                .arg(artist, song));
    });

    // Metadata-only query: no download, one tab-separated line
    proc->start(ytdlp, {"--no-playlist", "--skip-download", "--no-warnings",
                        "--print", "%(artist)s\t%(track)s\t%(title)s\t%(uploader)s",
                        url});

    // Don't let a stuck fetch linger forever
    QTimer::singleShot(25000, proc, [proc]() {
        if (proc->state() != QProcess::NotRunning)
            proc->kill();
    });
}
