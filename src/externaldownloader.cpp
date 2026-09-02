#include "externaldownloader.h"
#include "ui_externaldownloader.h"
#include "QProcess"
#include <QMessageBox>
#include <QtSql>
#include <QDir>
#include <QUrl>
#include <QUrlQuery>
#include <QPushButton>
#include "addgenre.h"
#include "player.h"
#include "services/DependencyChecker.h"
#include "mediaduration.h"
#include "streamingcatalog.h"
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

    // The field takes more than YouTube links now, so say so where the user
    // looks. Kept in code rather than the .ui so it stays translatable and the
    // generated header can't go stale.
    ui->txt_videoLink->setPlaceholderText(
        tr("YouTube, SoundCloud, Bandcamp, Spotify or Apple Music link"));
    ui->bt_youtube_getIt->setToolTip(
        tr("Download one track: a YouTube, SoundCloud or Bandcamp link, or a Spotify "
           "/ Apple Music track (fetched from YouTube, since neither service serves "
           "its own audio)"));
    ui->bt_youtube_getPlaylist->setToolTip(
        tr("Download every track of a YouTube playlist (link containing \"list=\"), "
           "a SoundCloud set, a Bandcamp album, or a Spotify / Apple Music album or "
           "playlist"));

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

// Ogg/Opus files carry their cover art as a METADATA_BLOCK_PICTURE Vorbis
// comment — a base64-encoded FLAC picture block — and not as a stream of their
// own. ffmpeg's ogg demuxer hands that comment back as a synthetic MJPEG
// "video" stream, so a plain "-map 0 -c copy" tagging pass tries to re-mux it
// as real video and the ogg/opus muxer refuses with "Unsupported codec id in
// stream 1", leaving the download untagged. Read the picture back out and
// rebuild the block so the tagging pass can write the cover as a comment
// again. Returns the base64 block ready for a METADATA_BLOCK_PICTURE tag, or
// an empty array — with *whyNot* set only when the file does have a cover we
// failed to rebuild, so the caller can tell "no cover" from "cover lost".
static QByteArray extractCoverPictureBlock(const QString &ffmpegPath,
                                           const QString &filePath,
                                           QString *whyNot)
{
    auto fail = [&](const QString &reason) {
        if (whyNot) *whyNot = reason;
        return QByteArray();
    };

    // ffprobe normally sits next to ffmpeg, which we already resolved past the
    // missing GUI PATH.
    QString ffprobePath = resolveExecutable("ffprobe");
    if (ffprobePath.isEmpty() && !ffmpegPath.isEmpty()) {
        const QString sibling = QFileInfo(ffmpegPath).absolutePath() + "/ffprobe";
        if (QFileInfo(sibling).isExecutable()) ffprobePath = sibling;
    }
    if (ffprobePath.isEmpty()) return fail("ffprobe was not found");

    QProcess probe;
    probe.start(ffprobePath, {"-v", "error", "-select_streams", "v:0",
                              "-show_entries", "stream=codec_name,width,height",
                              "-of", "default=noprint_wrappers=1", filePath});
    if (!probe.waitForStarted(8000) || !probe.waitForFinished(20000) ||
        probe.exitStatus() != QProcess::NormalExit) {
        return fail("ffprobe could not read the file");
    }

    QString codec;
    int width = 0, height = 0;
    const QStringList probeLines =
        QString::fromUtf8(probe.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : probeLines) {
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == "codec_name") codec = value;
        else if (key == "width")  width = value.toInt();
        else if (key == "height") height = value.toInt();
    }

    // No picture at all: nothing to carry over, and nothing went wrong.
    if (codec.isEmpty()) return QByteArray();

    QByteArray mime;
    QString pictureExt;
    if (codec == "mjpeg") { mime = "image/jpeg"; pictureExt = "jpg"; }
    else if (codec == "png") { mime = "image/png"; pictureExt = "png"; }
    else return fail("unsupported cover image format \"" + codec + "\"");

    // Copy the picture out untouched (no re-encode) so the bytes we put back
    // are the ones yt-dlp embedded.
    const QString picturePath = filePath + ".cover." + pictureExt;
    QProcess extract;
    extract.setProcessChannelMode(QProcess::MergedChannels);
    extract.start(ffmpegPath, {"-y", "-v", "error", "-i", filePath,
                               "-map", "0:v:0", "-frames:v", "1", "-c", "copy",
                               picturePath});
    const bool extracted = extract.waitForStarted(8000) && extract.waitForFinished(30000) &&
                           extract.exitStatus() == QProcess::NormalExit && extract.exitCode() == 0;
    QFile pictureFile(picturePath);
    if (!extracted || !pictureFile.open(QIODevice::ReadOnly)) {
        QFile::remove(picturePath);
        return fail("the cover image could not be extracted");
    }
    const QByteArray imageData = pictureFile.readAll();
    pictureFile.close();
    QFile::remove(picturePath);
    if (imageData.isEmpty()) return fail("the extracted cover image was empty");

    // FLAC picture block (as referenced by the Vorbis comment spec): all
    // fields big-endian, with length-prefixed MIME type and description.
    QByteArray block;
    auto appendBE32 = [&block](quint32 value) {
        block.append(char((value >> 24) & 0xFF));
        block.append(char((value >> 16) & 0xFF));
        block.append(char((value >> 8) & 0xFF));
        block.append(char(value & 0xFF));
    };
    appendBE32(3);                      // picture type: front cover
    appendBE32(quint32(mime.size()));
    block.append(mime);
    appendBE32(0);                      // empty description
    appendBE32(quint32(width));
    appendBE32(quint32(height));
    appendBE32(24);                     // colour depth in bits per pixel
    appendBE32(0);                      // 0 = not a palette-indexed image
    appendBE32(quint32(imageData.size()));
    block.append(imageData);

    return block.toBase64();
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
        // This connection writes while the UI thread is reading. WAL keeps the
        // reader out of the way; the timeout covers the other writers (a sync
        // running, or the next track of the same playlist) so a busy moment
        // waits instead of losing the track that was just downloaded.
        if (dbCreds.driver == QLatin1String("QSQLITE"))
            db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=15000"));
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
                QString dur = MediaDuration::forFile(existingPath);
                if (dur.isEmpty())
                    dur = "-";
                QSqlQuery ins(db);
                // source_url is added by the migration at startup, so a
                // database this build has never opened will not have it yet.
                // Naming the columns is what lets the same code write to both.
                const bool keepsSource =
                    db.record(QStringLiteral("musics")).contains(QStringLiteral("source_url"));
                ins.prepare(keepsSource
                    ? "INSERT INTO musics (id, artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played, source_url) "
                      "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, 0, '-', ?)"
                    : "INSERT INTO musics (id, artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
                      "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, 0, '-')");
                ins.addBindValue(yartist);
                ins.addBindValue(ysong);
                ins.addBindValue(g1);
                ins.addBindValue(g2);
                ins.addBindValue(country);
                ins.addBindValue(pub_date);
                ins.addBindValue(existingPath);
                ins.addBindValue(dur);
                if (keepsSource)
                    ins.addBindValue(ylink);
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

    // A Spotify or Apple Music link names a track but never serves its audio.
    // Turn it into the search that finds the same song on YouTube, which is
    // where the rest of this function can actually download from.
    if (StreamingCatalog::serviceOf(ylink) != StreamingCatalog::Service::None) {
        const StreamingCatalog::Listing listing =
            StreamingCatalog::resolve(ylink, appendOutput);
        if (listing.tracks.isEmpty()) {
            result.success = false;
            result.message = listing.error.isEmpty()
                                 ? QString("Error: that link holds no track XFB can read.")
                                 : listing.error;
            appendOutput(result.message);
            db.close();
            QSqlDatabase::removeDatabase(workerConnectionName);
            result.consoleOutput = consoleLines.join("\n");
            return result;
        }
        ylink = StreamingCatalog::searchUrlFor(listing.tracks.first());
        appendOutput("Searching for: " + listing.tracks.first().artist + " - "
                     + listing.tracks.first().title);
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

    // One call must produce exactly one track. A link copied out of the YouTube
    // app usually still carries the playlist (or the auto-generated radio Mix)
    // it was playing inside — "watch?v=...&list=RD...&start_radio=1" — and
    // without this yt-dlp happily follows it and downloads the whole thing.
    // Search queries are exempt: "ytsearchN:" IS a playlist of results, and
    // --no-playlist would leave yt-dlp nothing to fetch.
    if (!ylink.startsWith(QLatin1String("ytsearch"), Qt::CaseInsensitive))
        ytdlpArgs << "--no-playlist";

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
        const QString suffix = fileInfo.suffix().toLower();
        // MP3 and M4A hold their cover art as a real (attached-picture) stream
        // and re-mux it happily, so only the Ogg family needs the picture
        // moved back into a Vorbis comment.
        const bool oggFamily = (suffix == "opus" || suffix == "ogg" || suffix == "oga");

        QByteArray coverBlock;
        QString coverIssue;
        if (oggFamily) {
            coverBlock = extractCoverPictureBlock(ffmpegPath, finalFilepath, &coverIssue);
        }

        // The picture block is far too big for a command line once the cover is
        // a few hundred KB, so hand it to ffmpeg in a metadata file instead.
        QString metaFile;
        if (!coverBlock.isEmpty()) {
            metaFile = finalFilepath + ".tagging.ffmeta";
            QFile meta(metaFile);
            if (meta.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QByteArray escaped = coverBlock;
                escaped.replace('=', "\\=");   // '=' is a separator in ffmetadata
                meta.write(";FFMETADATA1\n");
                meta.write("METADATA_BLOCK_PICTURE=");
                meta.write(escaped);
                meta.write("\n");
                meta.close();
            } else {
                metaFile.clear();
                coverIssue = "a temporary metadata file could not be written";
            }
        }

        QStringList tagArgs;
        tagArgs << "-y" << "-i" << finalFilepath;
        if (!metaFile.isEmpty()) tagArgs << "-f" << "ffmetadata" << "-i" << metaFile;
        // Take only the audio when a cover has to travel as a comment: mapping
        // the demuxer's synthetic picture stream is exactly what the ogg/opus
        // muxer rejects.
        if (oggFamily && (!coverBlock.isEmpty() || !coverIssue.isEmpty()))
            tagArgs << "-map" << "0:a";
        else
            tagArgs << "-map" << "0";
        tagArgs << "-c" << "copy";
        if (!metaFile.isEmpty()) tagArgs << "-map_metadata" << "1";
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
        const bool tagged = tagProc.waitForStarted(8000) && tagProc.waitForFinished(60000) &&
                            tagProc.exitStatus() == QProcess::NormalExit && tagProc.exitCode() == 0 &&
                            QFileInfo(taggedTmp).size() > 0;
        const QString tagProcOutput = QString::fromUtf8(tagProc.readAll()).trimmed();
        if (!metaFile.isEmpty()) QFile::remove(metaFile);

        if (tagged) {
            // Replace the original with the tagged version.
            if (QFile::remove(finalFilepath) && QFile::rename(taggedTmp, finalFilepath)) {
                if (!coverBlock.isEmpty())
                    appendOutput("Embedded metadata into the downloaded file (cover art kept).");
                else if (!coverIssue.isEmpty())
                    appendOutput("Embedded metadata into the downloaded file, but the embedded "
                                 "cover art was dropped: " + coverIssue + ".");
                else
                    appendOutput("Embedded metadata into the downloaded file.");
            } else {
                QFile::remove(taggedTmp); // keep original if swap failed
                appendOutput("Note: could not replace file with the tagged version; keeping original.");
            }
        } else {
            QFile::remove(taggedTmp);
            QString why = "Note: embedding metadata failed; the file was kept untagged.";
            if (oggFamily && (!coverBlock.isEmpty() || !coverIssue.isEmpty())) {
                why += " The embedded cover art is the likely cause — an ." + suffix +
                       " file cannot hold it as a stream. Turning off \"Embed thumbnail\" in "
                       "Options → Downloads (yt-dlp) will let the tags through.";
            }
            appendOutput(why);
            if (!tagProcOutput.isEmpty()) appendOutput("ffmpeg said: " + tagProcOutput);
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

    // 6. Get Duration (exiftool with ffmpeg fallback, if not already in DB)
    QString trackDuration = MediaDuration::forFile(finalFilepath);
    if (trackDuration.isEmpty()) {
        trackDuration = "-";
        appendOutput("Warning: could not determine the track duration.");
    } else {
        appendOutput("Track duration found: " + trackDuration);
    }


    // 7. Add to Database
    { // Scope for QSqlQuery
        QSqlQuery queryInsert(db);
        // The link is kept so the cover art can be fetched again later: a
        // download whose file carries no picture has nothing else to go on.
        // See the note above about a database the migration has not reached.
        const bool keepsSource =
            db.record(QStringLiteral("musics")).contains(QStringLiteral("source_url"));
        queryInsert.prepare(keepsSource
            ? "INSERT INTO musics (id, artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played, source_url) "
              "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
            : "INSERT INTO musics (id, artist, song, genre1, genre2, country, published_date, path, time, played_times, last_played) "
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
        if (keepsSource)
            queryInsert.addBindValue(ylink);

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

// A Bandcamp page: the site itself or, as is usual, an artist's/label's
// subdomain (n5md.bandcamp.com). Artists who put their page on their own
// domain look like any other site from the URL alone, so those links take the
// ordinary path — yt-dlp still recognises them when it fetches them.
static bool isBandcampUrl(const QString &url)
{
    const QString host = QUrl(url).host().toLower();
    return host == QLatin1String("bandcamp.com")
           || host.endsWith(QLatin1String(".bandcamp.com"));
}

// A Bandcamp album, e.g. https://n5md.bandcamp.com/album/light-as-a-feather
// (one song lives under /track/ instead).
static bool isBandcampAlbumUrl(const QString &url)
{
    return isBandcampUrl(url)
           && QUrl(url).path().startsWith(QLatin1String("/album/"));
}

// Everything after the "?" in a Bandcamp link is tracking picked up on the way
// from wherever it was shared (fbclid, sfnsn, from=...). A Bandcamp page is
// named by its path alone, so dropping the rest keeps the console log readable
// and makes the same album pasted from two places the same link.
static QString stripBandcampTracking(const QString &url)
{
    QUrl u(url);
    u.setQuery(QString());
    u.setFragment(QString());
    return u.toString();
}

// How much of an endless YouTube Mix is worth downloading when the user asks
// for one anyway. Radios have no last track, so this is the stop.
static const int kMixEntryLimit = 25;

// A YouTube auto-generated Mix / radio rather than a playlist somebody made.
// Their ids start with RD (radio), and YouTube keeps extending them as you
// listen, so they have no fixed end.
static bool isYouTubeMixId(const QString &listId)
{
    return listId.startsWith(QLatin1String("RD"), Qt::CaseInsensitive);
}

// The playlist id of a YouTube link, or an empty string.
static QString youTubeListId(const QString &url)
{
    return QUrlQuery(QUrl(url)).queryItemValue(QStringLiteral("list"));
}

// Reduce a pasted YouTube link to the playlist it names.
//
// Sharing from the YouTube app copies a link like
//   watch?v=<video>&list=<id>&start_radio=1&index=3&pp=<tracking>
// Keeping only "list=" — dropping the video, the radio flag, the position and
// the tracking parameters — asks yt-dlp for exactly that playlist and nothing
// else. Mixes are the exception: YouTube refuses to serve an RD id as a
// playlist page ("This playlist type is unviewable"), so those keep the URL
// they arrived on and are capped by the caller instead. Links carrying no list
// id are returned untouched, save for a Bandcamp album, which is reduced to
// the album page itself.
static QString normalizePlaylistUrl(const QString &url)
{
    if (isBandcampUrl(url))
        return stripBandcampTracking(url);

    const QString listId = youTubeListId(url);
    if (listId.isEmpty() || isYouTubeMixId(listId))
        return url;

    return QStringLiteral("https://www.youtube.com/playlist?list=") + listId;
}

// The video a "watch?v=..." link is built around, without the playlist that
// happens to be attached to it.
static QString youTubeVideoOnlyUrl(const QString &url)
{
    const QString videoId = QUrlQuery(QUrl(url)).queryItemValue(QStringLiteral("v"));
    if (videoId.isEmpty())
        return QString();
    return QStringLiteral("https://www.youtube.com/watch?v=") + videoId;
}

// Reduce a pasted link to the one track it names, for a single download.
// A YouTube watch link keeps only its video id, which drops the playlist, the
// radio flag and the tracking parameters that ride along with a shared link; a
// Bandcamp link keeps only its path, for the same reason. Everything else —
// youtu.be, SoundCloud, Spotify, Apple Music — is handed over as pasted, since
// their parameters are part of the address.
static QString normalizeSingleUrl(const QString &url)
{
    const QString trimmed = url.trimmed();
    if (StreamingCatalog::serviceOf(trimmed) != StreamingCatalog::Service::None)
        return trimmed;

    if (isBandcampUrl(trimmed))
        return stripBandcampTracking(trimmed);

    const QString videoOnly = youTubeVideoOnlyUrl(trimmed);
    return videoOnly.isEmpty() ? trimmed : videoOnly;
}

// A link that names a whole album or playlist rather than one track: a YouTube
// playlist, a SoundCloud set, a Bandcamp album, or a Spotify / Apple Music
// collection. Decided from the URL shape alone, no network access.
static bool isCollectionLink(const QString &url)
{
    return url.contains(QLatin1String("list=")) || isSoundCloudSetUrl(url)
           || isBandcampAlbumUrl(url) || StreamingCatalog::isCollectionUrl(url);
}

// Aggregate result for downloading every entry of a playlist.
struct PlaylistResult {
    int total = 0;       // entries discovered in the playlist
    int succeeded = 0;   // newly downloaded and added
    int skipped = 0;     // already present in the library, or unusable entries
    int failed = 0;      // entries that errored out
    bool fatal = false;  // true if the whole operation could not start
    QString message;     // human-readable summary / error
    QString warning;     // e.g. the source only gave up part of a long playlist
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
    DatabaseCredentials dbCreds,
    int maxEntries
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

    // What the playlist is made of: the link to fetch, and the artist/song to
    // file the result under. Filled either from the streaming service's own
    // track list (Spotify, Apple Music) or from yt-dlp's playlist index.
    struct PlaylistEntry {
        QString url;
        QString artist;
        QString song;
    };
    QList<PlaylistEntry> entries;

    const StreamingCatalog::Service service = StreamingCatalog::serviceOf(playlistUrl);
    if (service != StreamingCatalog::Service::None) {
        // Spotify and Apple Music stream DRM-protected audio no downloader can
        // extract. What they do publish is the track list, so XFB reads that
        // and fetches each track the way it fetches everything else: the best
        // YouTube match for the artist and title.
        const StreamingCatalog::Listing listing =
            StreamingCatalog::resolve(playlistUrl, appendOutput);
        if (!listing.error.isEmpty()) {
            agg.fatal = true;
            agg.message = listing.error;
            appendOutput(agg.message);
            agg.consoleOutput = consoleLines.join("\n");
            return agg;
        }

        // A long playlist that came back short must not be downloaded as if it
        // were whole — carry the reason through to the summary the user sees.
        if (listing.truncated) {
            agg.warning = listing.truncationNote;
            appendOutput("WARNING: " + agg.warning);
        }

        for (const StreamingCatalog::Track &track : listing.tracks) {
            PlaylistEntry entry;
            entry.url = StreamingCatalog::searchUrlFor(track);
            entry.artist = track.artist.isEmpty() ? QStringLiteral("Unknown Artist")
                                                  : track.artist;
            entry.song = track.title;
            entries.append(entry);
        }
    } else {
        // Enumerate the playlist entries without downloading any media. We print
        // one line per entry as "id<US>title<US>uploader<US>webpage_url<US>url",
        // using the ASCII Unit Separator (0x1F) as a delimiter so it can't
        // collide with text in titles.
        //
        // YouTube playlists are enumerated with "--flat-playlist" (cheap: one
        // index request, titles included). SoundCloud sets and Bandcamp albums
        // can NOT use the flat index: SoundCloud's flat entries carry no
        // title/uploader (both "NA") and some are bare api-v2.soundcloud.com
        // URLs, while Bandcamp's carry the song title but no artist at all,
        // which would file a whole album under "Unknown Artist". Without
        // --flat-playlist yt-dlp fetches each track's metadata (~1s per track,
        // still no media download since --print implies --simulate), which
        // yields real titles, artists and canonical permalinks.
        const bool bandcampAlbum = isBandcampAlbumUrl(playlistUrl);
        const bool fullMetadata = isSoundCloudSetUrl(playlistUrl) || bandcampAlbum;
        const QChar US(0x1f);
        if (bandcampAlbum)
            appendOutput("Fetching Bandcamp album entries (with metadata)...");
        else if (fullMetadata)
            appendOutput("Fetching SoundCloud set entries (with metadata)...");
        else
            appendOutput("Fetching playlist entries...");
        QStringList entryLines;
        {
            QProcess listProc;
            QStringList listArgs;
            if (!fullMetadata) listArgs << "--flat-playlist";
            // Reading only the first N entries of an endless YouTube Mix keeps
            // the index request from walking a thousand-track radio.
            if (maxEntries > 0)
                listArgs << "--playlist-items" << QString("1:%1").arg(maxEntries);
            listArgs << "--ignore-errors"
                     << "--no-warnings"
                     << "--print"
                     << QString("%(id)s%1%(title)s%1%(uploader)s%1%(webpage_url)s"
                                "%1%(url)s%1%(playlist_count)s%1%(artist)s"
                                "%1%(track)s").arg(US)
                     << playlistUrl;
            listProc.start(ytdlpPath, listArgs);
            if (!listProc.waitForStarted(15000)) {
                agg.fatal = true;
                agg.message = "Error: could not start yt-dlp to read the playlist.";
                appendOutput(agg.message);
                agg.consoleOutput = consoleLines.join("\n");
                return agg;
            }

            // Reading a playlist index is network-bound, and a long one simply
            // takes long — a fixed overall deadline would cut a big playlist
            // off in the middle and hand back half a list as if it were whole.
            // yt-dlp streams entries as it finds them, so wait on *silence*
            // instead: only a source that has stopped producing is stuck.
            const int idleTimeoutMs = fullMetadata ? 120000 : 60000;
            QString out;
            QString err;
            bool stalled = false;
            forever {
                if (listProc.waitForReadyRead(idleTimeoutMs)) {
                    out += QString::fromUtf8(listProc.readAllStandardOutput());
                    err += QString::fromUtf8(listProc.readAllStandardError());
                    continue;
                }
                // Nothing arrived: either it finished, or it went quiet.
                if (listProc.waitForFinished(1000)
                        || listProc.state() == QProcess::NotRunning) {
                    break;
                }
                listProc.kill();
                listProc.waitForFinished(2000);
                stalled = true;
                break;
            }
            out += QString::fromUtf8(listProc.readAllStandardOutput());
            err += QString::fromUtf8(listProc.readAllStandardError());
            if (!err.trimmed().isEmpty()) appendOutput(err.trimmed());

            if (stalled) {
                // Downloading the part that was listed would be exactly the
                // half-finished playlist this is meant to prevent.
                agg.fatal = true;
                agg.message = QString("Reading the playlist stopped responding after "
                                      "%1 entries, so the list is incomplete. Nothing "
                                      "was downloaded — try again.")
                                  .arg(out.count('\n'));
                appendOutput(agg.message);
                agg.consoleOutput = consoleLines.join("\n");
                return agg;
            }

            entryLines = out.split('\n', Qt::SkipEmptyParts);
        }

        int declaredCount = 0; // what the source says the playlist holds
        for (const QString &line : std::as_const(entryLines)) {
            const QStringList parts = line.split(US);
            const QString id = parts.value(0).trimmed();
            const QString title = parts.value(1).trimmed();
            const QString uploader = parts.value(2).trimmed();
            const QString webpageUrl = parts.value(3).trimmed();
            const QString entryUrl = parts.value(4).trimmed();
            declaredCount = qMax(declaredCount, parts.value(5).trimmed().toInt());
            const QString metaArtist = parts.value(6).trimmed();
            const QString metaTrack = parts.value(7).trimmed();

            if (id.isEmpty() || id == "NA") {
                appendOutput("Skipping a playlist entry with no video id.");
                agg.skipped++;
                continue;
            }

            PlaylistEntry entry;
            // Pick the URL to download. webpage_url is the canonical page (set
            // by the full-metadata pass; "NA" for flat entries). Flat entries
            // put the entry link in url instead. YouTube ids keep the
            // historical watch-URL construction as a last resort.
            if (webpageUrl.startsWith("http")) {
                entry.url = webpageUrl;
            } else if (entryUrl.startsWith("http")) {
                entry.url = entryUrl;
            } else {
                entry.url = "https://www.youtube.com/watch?v=" + id;
            }

            // Bandcamp tags every track with its artist and its song name, so
            // there is nothing to guess: take them as given. This is only asked
            // of Bandcamp — SoundCloud fills its "track" field with the whole
            // title, artist and all, and YouTube's flat index has no tags at
            // all.
            const auto tagged = [](const QString &v) {
                return !v.isEmpty() && v != QLatin1String("NA");
            };
            if (bandcampAlbum && tagged(metaArtist) && tagged(metaTrack)) {
                entry.artist = metaArtist;
                entry.song = metaTrack;
            } else {
                // Guess Artist/Song from the title. "Artist - Song" is the common
                // form for music; otherwise fall back to the channel name (minus
                // YouTube's " - Topic" auto-channel suffix) as the artist.
                const int sep = title.indexOf(" - ");
                if (sep > 0) {
                    entry.artist = title.left(sep).trimmed();
                    entry.song = title.mid(sep + 3).trimmed();
                } else {
                    QString channel = uploader;
                    channel.remove(QRegularExpression("\\s*-\\s*Topic$"));
                    entry.artist = channel.trimmed();
                    entry.song = title;
                }
            }
            if (entry.artist.isEmpty() || entry.artist == "NA") entry.artist = "Unknown Artist";
            if (entry.song.isEmpty()   || entry.song == "NA")   entry.song = id;

            entries.append(entry);
        }

        // yt-dlp reports the playlist's own length on every entry, so a short
        // index can be caught rather than mistaken for the whole thing. The
        // deliberate Mix cap is not a surprise, so it doesn't warn.
        const int listed = entries.size() + agg.skipped;
        if (maxEntries == 0 && declaredCount > listed) {
            agg.warning = QString("The playlist says it holds %1 entries but only %2 "
                                  "could be listed; %3 will be downloaded.")
                              .arg(declaredCount).arg(listed).arg(entries.size());
            appendOutput("WARNING: " + agg.warning);
        }
    }

    if (entries.isEmpty()) {
        agg.fatal = true;
        agg.message = "No playlist entries were found. Make sure the link points to a public "
                      "YouTube playlist (containing \"list=\"), SoundCloud set "
                      "(soundcloud.com/<artist>/sets/<set>), Bandcamp album "
                      "(<artist>.bandcamp.com/album/<album>), or Spotify / Apple Music "
                      "album or playlist.";
        appendOutput(agg.message);
        agg.consoleOutput = consoleLines.join("\n");
        return agg;
    }

    // The streaming services hand over the whole list at once, so their cap is
    // applied here rather than during enumeration.
    if (maxEntries > 0 && entries.size() > maxEntries) {
        appendOutput(QString("Limiting the download to the first %1 of %2 entries.")
                         .arg(maxEntries).arg(entries.size()));
        entries = entries.mid(0, maxEntries);
    }

    agg.total = entries.size() + agg.skipped;
    appendOutput(QString("Found %1 entries to download.").arg(entries.size()));

    int index = 0;
    for (const PlaylistEntry &entry : std::as_const(entries)) {
        ++index;
        const QString artist = entry.artist;
        const QString song = entry.song;

        appendOutput(QString("[%1/%2] Downloading: %3 - %4")
                         .arg(index).arg(entries.size()).arg(artist, song));

        // Reuse the single-video pipeline. yt-dlp was already updated above, so
        // skip the per-video update check.
        DownloadResult r = processDownloadTask(
            entry.url, artist, song, g1, g2, country, pub_date, dbCreds, /*skipUpdateCheck=*/true);

        if (!r.consoleOutput.isEmpty()) consoleLines.append(r.consoleOutput);

        if (r.success) {
            if (r.message.contains("already in the database", Qt::CaseInsensitive)) {
                agg.skipped++;
            } else {
                agg.succeeded++;
            }
        } else {
            agg.failed++;
            appendOutput(QString("[%1/%2] Failed: %3")
                             .arg(index).arg(entries.size()).arg(r.message));
        }
    }

    agg.message = QString("Playlist finished: %1 downloaded, %2 already present/skipped, "
                          "%3 failed (of %4 total).")
                      .arg(agg.succeeded).arg(agg.skipped).arg(agg.failed).arg(agg.total);
    if (!agg.warning.isEmpty())
        agg.message += "\n\nNote: the track list was incomplete.\n" + agg.warning;
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
    QString ylink = normalizeSingleUrl(ui->txt_videoLink->text());

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

    const QString pasted = ui->txt_videoLink->text().trimmed();

    // A YouTube playlist link must carry a "list=" parameter (unlike single
    // downloads we must NOT strip query parameters after '&', since that is
    // where the list id lives in "watch?v=...&list=..." URLs). SoundCloud sets
    // are recognized by their .../sets/... path, Bandcamp albums by /album/,
    // Spotify and Apple Music albums/playlists by their own URL shape.
    if (pasted.isEmpty() || !isCollectionLink(pasted)) {
        QMessageBox::warning(this, tr("Not a Playlist"),
            tr("Please paste a playlist link in the Video Link field: a YouTube playlist "
               "(containing \"list=\"), a SoundCloud set "
               "(soundcloud.com/artist/sets/name), a Bandcamp album "
               "(artist.bandcamp.com/album/name), or a Spotify or Apple Music album "
               "or playlist."));
        ui->bt_youtube_getIt->setEnabled(true);
        if (ui->bt_youtube_getPlaylist) ui->bt_youtube_getPlaylist->setEnabled(true);
        ui->frame_loading->hide();
        return;
    }

    // Drop the video, the radio flag and the tracking parameters a shared
    // YouTube link carries, so only the playlist itself is fetched.
    const QString ylink = normalizePlaylistUrl(pasted);
    if (ylink != pasted)
        ui->txt_teminal_yd1->appendPlainText(tr("Reading the playlist as: %1").arg(ylink));

    // An auto-generated Mix never ends — YouTube keeps extending it — so it is
    // downloaded only up to a fixed number of tracks.
    const int maxEntries = isYouTubeMixId(youTubeListId(ylink)) ? kMixEntryLimit : 0;

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
        } else if (!result.warning.isEmpty()) {
            // A partial list is not a success to be waved through.
            QMessageBox::warning(this, tr("Playlist Downloader"), result.message);
            if (result.succeeded > 0)
                emit musicAdded();
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
        ylink, g1, g2, country, pub_date, creds, maxEntries);
    watcher->setFuture(future);

    ui->txt_teminal_yd1->appendPlainText("Playlist task submitted to background thread...");
}

void externaldownloader::on_bt_youtube_getPlaylist_clicked()
{
    const QString ylink = ui->txt_videoLink->text().trimmed();
    if (ylink.isEmpty() || !isCollectionLink(ylink)) {
        QMessageBox::information(this, tr("Playlist Downloader"),
            tr("Please paste a playlist link in the Video Link field: a YouTube playlist "
               "(containing \"list=\"), a SoundCloud set "
               "(soundcloud.com/artist/sets/name), a Bandcamp album "
               "(artist.bandcamp.com/album/name), or a Spotify or Apple Music album "
               "or playlist."));
        return;
    }

    // A link shared out of a YouTube Mix ("start_radio=1", list id "RD...") is
    // not a playlist at all: it is an endless radio YouTube builds around one
    // video, and asking for all of it means a thousand tracks nobody chose.
    // Offer the track it was started from instead.
    const QString listId = youTubeListId(ylink);
    if (isYouTubeMixId(listId)) {
        const QString videoUrl = youTubeVideoOnlyUrl(ylink);
        QMessageBox box(this);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(tr("That is a YouTube Mix"));
        box.setText(tr("This link is an auto-generated YouTube Mix (radio), not a "
                       "playlist somebody put together. It has no end — YouTube keeps "
                       "adding tracks to it as it plays."));
        box.setInformativeText(videoUrl.isEmpty()
            ? tr("XFB will download only its first %1 tracks.").arg(kMixEntryLimit)
            : tr("You probably want just the track the Mix was started from.\n\n"
                 "Fill in the Artist and Song fields and use \"Get it!\" for that one "
                 "track, or download the first %1 tracks of the Mix.").arg(kMixEntryLimit));
        QPushButton *mixButton =
            box.addButton(tr("Download %1 from the Mix").arg(kMixEntryLimit),
                          QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() != mixButton) {
            // Leave the single track in the field, ready for "Get it!".
            if (!videoUrl.isEmpty()) {
                ui->txt_videoLink->setText(videoUrl);
                ui->txt_teminal_yd1->appendPlainText(
                    tr("Kept only the track the Mix started from: %1").arg(videoUrl));
            }
            return;
        }
        getPlaylist();
        return;
    }

    const StreamingCatalog::Service service = StreamingCatalog::serviceOf(ylink);
    QString what;
    if (service != StreamingCatalog::Service::None) {
        what = tr("%1 does not hand out its audio, so XFB reads the track list and then "
                  "downloads each song from YouTube. Artist and Song come from %1 itself, "
                  "and the genres selected above are applied to all of them.")
                   .arg(StreamingCatalog::serviceName(service));
    } else if (isBandcampAlbumUrl(ylink)) {
        what = tr("This downloads every track of the Bandcamp album as audio and adds "
                  "them to your library. Artist and Song come from Bandcamp's own track "
                  "information, and the genres selected above are applied to all of "
                  "them.");
    } else {
        what = tr("This downloads every entry of the playlist as audio and adds them to "
                  "your library. Artist and Song are guessed from each entry's title, "
                  "and the genres selected above are applied to all of them.");
    }

    const auto reply = QMessageBox::question(this, tr("Download Whole Playlist?"),
        what + tr("\n\nThis can take a while. Continue?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return;
    }

    getPlaylist();
}

void externaldownloader::on_bt_youtube_getIt_clicked()
{
    const QString ylink = normalizeSingleUrl(ui->txt_videoLink->text());
    qDebug()<<"ylink is now: "<<ylink;

    const QString yartist = ui->txt_artist->text().trimmed();
    const QString ysong = ui->txt_song->text().trimmed();

    if(ylink.isEmpty() || yartist.isEmpty() || ysong.isEmpty()){
        QMessageBox::information(this,tr("Downloader"),tr("The link, the artist name and the song title are mandatory..."));
        return;
    }

    // A whole album or playlist pasted into a single download would fetch only
    // its first track and file it under the artist/song typed above — worse on
    // Bandcamp, where yt-dlp downloads every track of the album over one
    // another, since --no-playlist does not apply to an album page. Point the
    // user at the button that does what they meant.
    if (isBandcampAlbumUrl(ylink)) {
        QMessageBox::information(this, tr("Downloader"),
            tr("That is a Bandcamp album, not a single track. Use \"Get Playlist!\" to "
               "download all of it, or paste the link of one track "
               "(artist.bandcamp.com/track/name)."));
        return;
    }

    if (StreamingCatalog::isCollectionUrl(ylink)) {
        QMessageBox::information(this, tr("Downloader"),
            tr("That is a %1 album or playlist, not a single track. Use "
               "\"Get Playlist!\" to download all of it.")
                .arg(StreamingCatalog::serviceName(StreamingCatalog::serviceOf(ylink))));
        return;
    }

    showLoadingFrame();
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
    if (StreamingCatalog::isCollectionUrl(url))
        return;
    // A Bandcamp album link stands for every track on it, so there is no one
    // title to put in the fields; the playlist path reads them per track.
    if (isBandcampAlbumUrl(url))
        return;

    // yt-dlp cannot read Spotify or Apple Music, so those links get their
    // artist/title from the service's own catalogue instead.
    if (StreamingCatalog::serviceOf(url) != StreamingCatalog::Service::None) {
        fetchStreamingDetails(url);
        return;
    }

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

        applyAutoFill(artist, song);
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

void externaldownloader::fetchStreamingDetails(const QString &url)
{
    const StreamingCatalog::Service service = StreamingCatalog::serviceOf(url);
    ui->txt_teminal_yd1->appendPlainText(
        tr("Fetching the track details from %1...")
            .arg(StreamingCatalog::serviceName(service)));

    // Resolution does blocking network I/O, so it runs off the UI thread.
    auto *watcher = new QFutureWatcher<StreamingCatalog::Listing>(this);
    m_streamingFetch = watcher; // a newer link supersedes any fetch in flight

    connect(watcher, &QFutureWatcher<StreamingCatalog::Listing>::finished, this,
            [this, watcher]() {
        const bool current = (m_streamingFetch == watcher);
        if (current)
            m_streamingFetch = nullptr;
        const StreamingCatalog::Listing listing = watcher->result();
        watcher->deleteLater();
        if (!current)
            return; // the user has pasted something else since

        if (listing.tracks.isEmpty()) {
            ui->txt_teminal_yd1->appendPlainText(
                listing.error.isEmpty()
                    ? tr("Could not fetch the track details — fill Artist/Song manually.")
                    : listing.error);
            return;
        }
        applyAutoFill(listing.tracks.first().artist, listing.tracks.first().title);
    });

    watcher->setFuture(QtConcurrent::run([url]() {
        return StreamingCatalog::resolve(url);
    }));
}

void externaldownloader::applyAutoFill(const QString &artist, const QString &song)
{
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
        (filled ? tr("Auto-filled: %1 — %2") : tr("Track details: %1 — %2"))
            .arg(artist, song));
}
