#include "TorrentDownloadService.h"
#include "ErrorHandler.h"
#include "Logger.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStandardItem>
#include <QDirIterator>
#include <QDateTime>
#include <QRegularExpression>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

TorrentDownloadService::TorrentDownloadService(QObject *parent)
    : BaseService(parent)
    , m_torService(nullptr)
    , m_statusTimer(new QTimer(this))
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_downloadsModel(new QStandardItemModel(this))
{
    QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_downloadDirectory = QDir(downloadsPath).filePath("XFB_Torrents");
    QDir().mkpath(m_downloadDirectory);

    // State file for persisting downloads across restarts
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    m_stateFilePath = QDir(appData).filePath("torrent_downloads.json");

    m_statusTimer->setInterval(2000);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() { updateDownloadModel(); });

    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, [](const QString &) { /* reserved */ });

    QStringList headers;
    headers << "Name" << "Progress" << "Status" << "Speed";
    m_downloadsModel->setHorizontalHeaderLabels(headers);

    findTorrentClient();
}

TorrentDownloadService::~TorrentDownloadService()
{
    stopAllProcesses();
}

void TorrentDownloadService::setTorService(TorNetworkService *torService)
{
    m_torService = torService;
}

bool TorrentDownloadService::findTorrentClient()
{
    // Prefer aria2c (better DHT/PEX/LPD support), fall back to transmission-cli
    QStringList candidates = {
#ifdef Q_OS_WIN
        QCoreApplication::applicationDirPath() + "/aria2c.exe",
        "C:/Program Files/aria2/aria2c.exe",
        QDir::homePath() + "/AppData/Local/Programs/aria2/aria2c.exe",
#else
        "/opt/homebrew/bin/aria2c",
        "/usr/local/bin/aria2c",
        "/opt/homebrew/bin/transmission-cli",
        "/usr/local/bin/transmission-cli",
#endif
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            m_torrentClient = path;
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                             QString("Found torrent client: %1").arg(path));
            return true;
        }
    }
    // Fallback to PATH search
    for (const QString &name : QStringList{"aria2c", "transmission-cli"}) {
        QString found = QStandardPaths::findExecutable(name);
        if (!found.isEmpty()) {
            m_torrentClient = found;
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                             QString("Found torrent client: %1").arg(found));
            return true;
        }
    }
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentDownloadService",
                           "No supported torrent client found. Install transmission-cli or aria2c.");
    return false;
}

// ── Download lifecycle ──────────────────────────────────────────────────────

QString TorrentDownloadService::startDownload(const QString &magnetLink, const QString &name)
{
    if (m_torrentClient.isEmpty()) {
        emit downloadError("", "No torrent client available. Install transmission-cli.");
        return {};
    }
    if (magnetLink.isEmpty()) {
        emit downloadError("", "No magnet link provided");
        return {};
    }
    // Require Tor to be connected
    if (!m_torService || !m_torService->isTorReady()) {
        emit downloadError("", "Tor is not connected. Connect to Tor before downloading.");
        return {};
    }

    QString downloadId = QString::number(QDateTime::currentMSecsSinceEpoch());

    // The torrent title is attacker-controlled metadata; never let it escape
    // the download directory. Reduce it to a single safe leaf name (dropping
    // any path separators or "..") before building a filesystem path — this
    // path drives the post-download audio scan, so a "../" title must not be
    // able to make us walk (and auto-import) an arbitrary directory.
    QString safeLeaf = QFileInfo(name).fileName();
    safeLeaf.remove(QRegularExpression("[/\\\\]"));
    while (safeLeaf.startsWith('.'))
        safeLeaf.remove(0, 1);
    if (safeLeaf.isEmpty())
        safeLeaf = downloadId;
    const QString baseDir = QDir(m_downloadDirectory).absolutePath();
    QString candidate = QDir::cleanPath(QDir(baseDir).filePath(safeLeaf));
    if (candidate != baseDir && !candidate.startsWith(baseDir + "/"))
        candidate = QDir(baseDir).filePath(downloadId); // paranoia fallback

    TorrentDownload dl;
    dl.magnetLink = magnetLink;
    dl.name = name;                 // original title kept for display only
    dl.downloadPath = candidate;
    dl.status = "starting";
    dl.progress = 0.0;
    dl.isStreaming = false;
    dl.process = nullptr;

    m_downloads[downloadId] = dl;
    m_downloadOrder.append(downloadId);

    if (startTorrentClientForDownload(downloadId)) {
        if (!m_statusTimer->isActive()) m_statusTimer->start();
        emit downloadStarted(downloadId);
        updateDownloadModel();
        saveDownloadState();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Started download: %1").arg(name));
    } else {
        m_downloads.remove(downloadId);
        m_downloadOrder.removeAll(downloadId);
        emit downloadError(downloadId, "Failed to start torrent client");
        return {};
    }
    return downloadId;
}

bool TorrentDownloadService::startTorrentClientForDownload(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return false;
    TorrentDownload &dl = m_downloads[downloadId];

    QProcess *proc = new QProcess(this);
    dl.process = proc;

    // ── Process finished ────────────────────────────────────────────────
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, downloadId](int exitCode, QProcess::ExitStatus) {
        if (!m_downloads.contains(downloadId)) return;
        auto &d = m_downloads[downloadId];
        d.process = nullptr;
        if (d.status == "cancelled") return;  // already handled
        if (exitCode == 0) {
            d.status = "completed";
            d.progress = 100.0;
            checkForCompletedFiles(downloadId);
        } else {
            d.status = "error";
            d.errorMessage = QString("Client exited with code %1").arg(exitCode);
            emit downloadError(downloadId, d.errorMessage);
        }
        updateDownloadModel();
        saveDownloadState();
        emit downloadsChanged();
    });

    // ── Process error ───────────────────────────────────────────────────
    connect(proc, &QProcess::errorOccurred, this, [this, downloadId](QProcess::ProcessError err) {
        if (!m_downloads.contains(downloadId)) return;
        auto &d = m_downloads[downloadId];
        if (d.status == "cancelled") return;
        d.status = "error";
        d.process = nullptr;
        d.errorMessage = QString("Process error: %1").arg(err);
        emit downloadError(downloadId, d.errorMessage);
        updateDownloadModel();
    });

    // ── stdout → progress parsing ───────────────────────────────────────
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, downloadId, proc]() {
        if (!m_downloads.contains(downloadId)) return;
        QString output = proc->readAllStandardOutput();
        auto &d = m_downloads[downloadId];

        // aria2c: "[#abc123 45MiB/120MiB(37%) CN:12 DL:1.2MiB ETA:1m2s]"
        // transmission-cli: "Progress: 45.2%, dl from 3 of 12 peers (234 kB/s)"
        QRegularExpression progressRe("(\\d+\\.?\\d*)%");
        QRegularExpressionMatch m = progressRe.match(output);
        if (m.hasMatch()) {
            double pct = m.captured(1).toDouble();
            d.progress = pct;
            d.status = (pct >= 100.0) ? "seeding" : "downloading";
            emit downloadProgress(downloadId, pct);
        }
        // Speed extraction: "DL:1.2MiB" (aria2c) or "234 kB/s" (transmission-cli)
        QRegularExpression speedRe("DL:(\\d+\\.?\\d*\\s*[kKmMgG]?i?B)");
        QRegularExpressionMatch sm = speedRe.match(output);
        if (sm.hasMatch()) {
            d.speed = sm.captured(1) + "/s";
        } else {
            // Fallback: transmission-cli style
            QRegularExpression speedRe2("(\\d+\\.?\\d*\\s*[kKmMgG]?B/s)");
            QRegularExpressionMatch sm2 = speedRe2.match(output);
            if (sm2.hasMatch()) {
                d.speed = sm2.captured(1);
            }
        }

        // aria2c completion: "Download complete:" or "(OK):download completed."
        if (output.contains("Download complete") || output.contains("download completed")) {
            d.progress = 100.0;
            d.status = "completed";
            emit downloadProgress(downloadId, 100.0);
        }
    });

    // Also capture stderr for error details
    connect(proc, &QProcess::readyReadStandardError, this, [this, downloadId, proc]() {
        if (!m_downloads.contains(downloadId)) return;
        QString output = proc->readAllStandardError().trimmed();
        if (!output.isEmpty()) {
            auto &d = m_downloads[downloadId];
            // Store last meaningful stderr line as error detail
            d.errorMessage = output.left(200);
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                             QString("stderr [%1]: %2").arg(downloadId, output.left(200)));
        }
    });

    // ── Build command args ─────────────────────────────────────────────────
    // NOTE: Search queries and magnet link fetching go through Tor (via TorNetworkService).
    // BitTorrent peer-to-peer traffic is NOT proxied through Tor because:
    //   1) transmission-cli doesn't support SOCKS5 for peer connections via env vars
    //   2) Tor Project explicitly advises against torrenting over Tor
    //   3) It overloads the Tor network and doesn't provide meaningful anonymity for P2P

    // Clean and prepare the magnet link for the torrent client
    QString magnet = dl.magnetLink;
    if (magnet.startsWith("magnet:")) {
        // Strip .onion tracker URLs — the torrent client can't reach them
        // without a Tor proxy (peer traffic is not proxied). We deliberately
        // do NOT inject public trackers: announcing to extra trackers only
        // broadcasts the user's real IP more widely. The download uses only
        // the trackers the magnet already carries (plus DHT/PEX if enabled,
        // which the privacy-hardened client settings below turn off).
        QRegularExpression onionTrackerRe("&tr=[^&]*\\.onion[^&]*", QRegularExpression::CaseInsensitiveOption);
        int removed = 0;
        while (magnet.contains(onionTrackerRe)) {
            magnet.remove(onionTrackerRe);
            removed++;
        }
        // Redacted: never log the magnet link or its infohash to disk.
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Prepared magnet: stripped %1 onion tracker(s); no public trackers injected").arg(removed));
    }

    QStringList args;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    if (m_torrentClient.contains("transmission-cli")) {
        // Privacy-hardened config: no peer-discovery broadcast (DHT/PEX/LPD
        // off), no incoming/listening port, encryption required, and stop
        // seeding as soon as the download completes so we don't keep
        // uploading (real IP is exposed to peers — minimise that window).
        QString configDir = QDir(m_downloadDirectory).filePath(".transmission-config");
        QDir().mkpath(configDir);
        QString settingsPath = QDir(configDir).filePath("settings.json");
        QFile settingsFile(settingsPath);
        if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QJsonObject settings;
            settings["dht-enabled"] = false;              // don't broadcast to the DHT
            settings["pex-enabled"] = false;              // no peer exchange
            settings["lpd-enabled"] = false;              // no local (LAN) peer discovery
            settings["port-forwarding-enabled"] = false;  // no UPnP/NAT-PMP
            settings["peer-port-random-on-start"] = false;
            settings["peer-port"] = 0;                    // disable listening port
            settings["encryption"] = 2;                   // require encrypted connections
            settings["ratio-limit"] = 0;                  // stop seeding at ratio 0…
            settings["ratio-limit-enabled"] = true;       // …i.e. as soon as complete
            settings["idle-seeding-limit"] = 1;           // and never idle-seed
            settings["idle-seeding-limit-enabled"] = true;
            settings["download-dir"] = m_downloadDirectory;
        settingsFile.write(QJsonDocument(settings).toJson());
            settingsFile.close();
        }
        args << "-g" << configDir    // config dir
             << "-w" << m_downloadDirectory
             << magnet;
    } else if (m_torrentClient.contains("aria2c")) {
        // Privacy-hardened: DHT/LPD/PEX all off so the client only talks to
        // the trackers the magnet already carries (no wider IP broadcast),
        // encryption required, and seeding stops immediately on completion.
        args << "--enable-dht=false"
             << "--bt-enable-lpd=false"
             << "--enable-peer-exchange=false"
             << "--bt-require-crypto=true"
             << "--bt-min-crypto-level=arc4"
             << "--bt-tracker-connect-timeout=30"
             << "--bt-tracker-timeout=30"
             << "--connect-timeout=60"
             << "--timeout=120"
             << "--bt-stop-timeout=0"            // never exit just because DL stalls
             << "--max-tries=5"
             << "--retry-wait=10"
             << "--seed-time=0"
             << "--follow-torrent=mem"
             << "--max-connection-per-server=16"
             << "--split=16"
             << "--summary-interval=5"
             << "--disable-ipv6=true"
             << "--bt-request-peer-speed-limit=0"
             << "--no-netrc=true"
             << "--dir=" + m_downloadDirectory
             << magnet;
    } else {
        return false;
    }

    proc->setProcessEnvironment(env);
    proc->setProcessChannelMode(QProcess::SeparateChannels);

    // Redacted: don't log the argument list (it ends with the magnet link).
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                     QString("Starting torrent client: %1").arg(QFileInfo(m_torrentClient).fileName()));

    proc->start(m_torrentClient, args);

    if (!proc->waitForStarted(5000)) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Error, "TorrentDownloadService",
                         QString("Failed to start %1").arg(m_torrentClient));
        dl.process = nullptr;
        proc->deleteLater();
        return false;
    }

    dl.status = "downloading";
    return true;
}

void TorrentDownloadService::cancelDownload(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return;
    auto &dl = m_downloads[downloadId];
    dl.status = "cancelled";
    if (dl.process && dl.process->state() == QProcess::Running) {
        dl.process->terminate();
        if (!dl.process->waitForFinished(2000)) dl.process->kill();
    }
    dl.process = nullptr;
    emit downloadCancelled(downloadId);
    updateDownloadModel();
    saveDownloadState();
    emit downloadsChanged();
}

void TorrentDownloadService::cancelAllDownloads()
{
    QStringList ids = m_downloadOrder;
    for (const QString &id : ids) {
        if (m_downloads.contains(id) && m_downloads[id].status == "downloading") {
            cancelDownload(id);
        }
    }
}

bool TorrentDownloadService::retryDownload(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return false;
    auto &dl = m_downloads[downloadId];
    if (dl.status != "error" && dl.status != "cancelled") return false;
    if (dl.magnetLink.isEmpty()) return false;

    dl.status = "starting";
    dl.progress = 0.0;
    dl.speed.clear();
    dl.errorMessage.clear();
    updateDownloadModel();

    if (startTorrentClientForDownload(downloadId)) {
        emit downloadStarted(downloadId);
        return true;
    } else {
        dl.status = "error";
        dl.errorMessage = "Failed to restart torrent client";
        updateDownloadModel();
        return false;
    }
}

void TorrentDownloadService::removeCompleted()
{
    QStringList toRemove;
    for (const QString &id : m_downloadOrder) {
        if (!m_downloads.contains(id)) continue;
        const auto &dl = m_downloads[id];
        if (dl.status == "completed" || dl.status == "error" || dl.status == "cancelled") {
            toRemove.append(id);
        }
    }
    for (const QString &id : toRemove) {
        m_downloads.remove(id);
        m_downloadOrder.removeAll(id);
    }
    updateDownloadModel();
    saveDownloadState();
    emit downloadsChanged();
}

void TorrentDownloadService::stopAllProcesses()
{
    m_statusTimer->stop();
    // Persist state before killing processes so downloads can be resumed
    saveDownloadState();
    for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it) {
        QProcess *p = it.value().process;
        if (p && p->state() == QProcess::Running) {
            p->terminate();
            if (!p->waitForFinished(2000)) p->kill();
        }
        it.value().process = nullptr;
    }
}

int TorrentDownloadService::activeDownloadCount() const
{
    int count = 0;
    for (auto it = m_downloads.constBegin(); it != m_downloads.constEnd(); ++it) {
        if (it.value().status == "downloading" || it.value().status == "starting" || it.value().status == "seeding")
            ++count;
    }
    return count;
}

QString TorrentDownloadService::downloadIdForRow(int row) const
{
    if (row >= 0 && row < m_downloadOrder.size())
        return m_downloadOrder[row];
    return {};
}

// ── Model update ────────────────────────────────────────────────────────────

void TorrentDownloadService::updateDownloadModel()
{
    m_downloadsModel->removeRows(0, m_downloadsModel->rowCount());

    for (const QString &id : m_downloadOrder) {
        if (!m_downloads.contains(id)) continue;
        const TorrentDownload &dl = m_downloads[id];

        auto *nameItem = new QStandardItem(dl.name);
        nameItem->setData(id, Qt::UserRole);  // store downloadId
        nameItem->setEditable(false);

        auto *progressItem = new QStandardItem(QString::number(dl.progress, 'f', 1) + "%");
        progressItem->setEditable(false);

        // Status with icon hint and error details
        QString statusText = dl.status;
        if (dl.status == "downloading") statusText = QString::fromUtf8("⬇ Downloading");
        else if (dl.status == "seeding")   statusText = QString::fromUtf8("⬆ Seeding");
        else if (dl.status == "completed") statusText = QString::fromUtf8("✓ Completed");
        else if (dl.status == "error") {
            statusText = QString::fromUtf8("✗ Error");
            if (!dl.errorMessage.isEmpty())
                statusText += ": " + dl.errorMessage.left(60);
        }
        else if (dl.status == "cancelled") statusText = QString::fromUtf8("⊘ Cancelled");
        else if (dl.status == "starting")  statusText = QString::fromUtf8("… Starting");
        else if (dl.status == "paused")    statusText = QString::fromUtf8("⏸ Paused");
        auto *statusItem = new QStandardItem(statusText);
        statusItem->setEditable(false);
        if (!dl.errorMessage.isEmpty())
            statusItem->setToolTip(dl.errorMessage);

        auto *speedItem = new QStandardItem(dl.speed.isEmpty() ? "-" : dl.speed);
        speedItem->setEditable(false);

        m_downloadsModel->appendRow({nameItem, progressItem, statusItem, speedItem});
    }

    // Keep headers
    m_downloadsModel->setHorizontalHeaderLabels({"Name", "Progress", "Status", "Speed"});
}

// ── Completion / audio file scanning ────────────────────────────────────────

void TorrentDownloadService::checkForCompletedFiles(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return;
    const TorrentDownload &dl = m_downloads[downloadId];
    QStringList audio = findAudioFiles(dl.downloadPath);
    if (audio.isEmpty()) {
        // Also scan the top-level download dir (transmission may not create a subfolder)
        audio = findAudioFiles(m_downloadDirectory);
    }
    if (!audio.isEmpty()) {
        emit downloadCompleted(downloadId, audio);
    }
}

QStringList TorrentDownloadService::findAudioFiles(const QString &directory)
{
    QStringList audioFiles;
    QStringList exts = {"*.mp3", "*.flac", "*.wav", "*.ogg", "*.m4a", "*.aac", "*.wma", "*.opus"};
    QDirIterator it(directory, exts, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) audioFiles << it.next();
    return audioFiles;
}

bool TorrentDownloadService::isAudioFile(const QString &filePath)
{
    static const QStringList exts = {".mp3",".flac",".wav",".ogg",".m4a",".aac",".wma",".opus"};
    return exts.contains("." + QFileInfo(filePath).suffix().toLower());
}

// ── Persistence — save/load/resume downloads across app restarts ─────────

void TorrentDownloadService::saveDownloadState()
{
    QJsonArray arr;
    for (const QString &id : m_downloadOrder) {
        if (!m_downloads.contains(id)) continue;
        const TorrentDownload &dl = m_downloads[id];
        // Only persist downloads that are worth resuming
        if (dl.status == "cancelled") continue;

        QJsonObject obj;
        obj["id"] = id;
        obj["magnetLink"] = dl.magnetLink;
        obj["name"] = dl.name;
        obj["downloadPath"] = dl.downloadPath;
        obj["status"] = dl.status;
        obj["progress"] = dl.progress;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile file(m_stateFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson(QJsonDocument::Compact));
        file.close();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Saved %1 download(s) to state file").arg(arr.size()));
    }
}

void TorrentDownloadService::loadDownloadState()
{
    QFile file(m_stateFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        QString id = obj["id"].toString();
        if (id.isEmpty() || m_downloads.contains(id)) continue;

        TorrentDownload dl;
        dl.magnetLink = obj["magnetLink"].toString();
        dl.name = obj["name"].toString();
        dl.downloadPath = obj["downloadPath"].toString();
        dl.status = obj["status"].toString();
        dl.progress = obj["progress"].toDouble();
        dl.isStreaming = false;
        dl.process = nullptr;

        // Mark incomplete downloads as paused so they can be resumed
        if (dl.status == "downloading" || dl.status == "starting" || dl.status == "seeding") {
            dl.status = "paused";
        }

        m_downloads[id] = dl;
        m_downloadOrder.append(id);
    }

    if (!m_downloads.isEmpty()) {
        updateDownloadModel();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Loaded %1 download(s) from previous session").arg(m_downloads.size()));
    }
}

void TorrentDownloadService::resumeDownloads()
{
    if (m_torrentClient.isEmpty()) return;

    int resumed = 0;
    for (const QString &id : m_downloadOrder) {
        if (!m_downloads.contains(id)) continue;
        TorrentDownload &dl = m_downloads[id];

        // Resume paused (previously active) downloads
        if (dl.status == "paused" && !dl.magnetLink.isEmpty()) {
            dl.status = "starting";
            if (startTorrentClientForDownload(id)) {
                resumed++;
                emit downloadStarted(id);
            } else {
                dl.status = "error";
            }
        }
    }

    if (resumed > 0) {
        if (!m_statusTimer->isActive()) m_statusTimer->start();
        updateDownloadModel();
        emit downloadsChanged();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Resumed %1 download(s)").arg(resumed));
    }
}

// ── Streaming ───────────────────────────────────────────────────────────────

void TorrentDownloadService::enableStreaming(const QString &downloadId)
{
    if (m_downloads.contains(downloadId)) {
        m_downloads[downloadId].isStreaming = true;
        setupStreaming(downloadId);
    }
}

void TorrentDownloadService::setupStreaming(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return;
    TorrentDownload &dl = m_downloads[downloadId];
    QString f = getFirstStreamableFile(downloadId);
    if (!f.isEmpty() && canStreamFile(f, dl.progress))
        emit streamingReady(downloadId, f);
}

QString TorrentDownloadService::getFirstStreamableFile(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return {};
    QStringList files = findAudioFiles(m_downloads[downloadId].downloadPath);
    return files.isEmpty() ? QString() : files.first();
}

bool TorrentDownloadService::canStreamFile(const QString &, double progress)
{
    return progress >= 10.0;
}

QStringList TorrentDownloadService::getStreamableFiles(const QString &downloadId)
{
    if (!m_downloads.contains(downloadId)) return {};
    return findAudioFiles(m_downloads[downloadId].downloadPath);
}
