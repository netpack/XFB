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

    TorrentDownload dl;
    dl.magnetLink = magnetLink;
    dl.name = name;
    dl.downloadPath = QDir(m_downloadDirectory).filePath(name);
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
        // Step 1: Strip .onion tracker URLs — the torrent client can't reach them without Tor proxy
        // Also strip any tracker that's clearly unreachable (dead/invalid)
        QRegularExpression onionTrackerRe("&tr=[^&]*\\.onion[^&]*", QRegularExpression::CaseInsensitiveOption);
        int removed = 0;
        while (magnet.contains(onionTrackerRe)) {
            magnet.remove(onionTrackerRe);
            removed++;
        }
        if (removed > 0) {
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                             QString("Stripped %1 .onion tracker(s) from magnet link").arg(removed));
        }

        // Step 2: Inject well-known public trackers so the client can find peers.
        // Use both HTTP and UDP trackers — UDP may be blocked by some firewalls/NATs.
        QStringList publicTrackers = {
            // HTTP/HTTPS trackers (more reliable through NAT/firewalls)
            "http://tracker.opentrackr.org:1337/announce",
            "https://tracker.lilithraws.org:443/announce",
            "http://tracker.openbittorrent.com:80/announce",
            "https://opentracker.i2p.rocks:443/announce",
            "http://tracker.bt4g.com:2095/announce",
            "http://tracker.files.fm:6969/announce",
            "http://tracker.gbitt.info:80/announce",
            // UDP trackers
            "udp://tracker.opentrackr.org:1337/announce",
            "udp://open.stealth.si:80/announce",
            "udp://tracker.torrent.eu.org:451/announce",
            "udp://exodus.desync.com:6969/announce",
            "udp://open.demonii.com:1337/announce",
            "udp://tracker.openbittorrent.com:6969/announce",
            "udp://tracker.moeking.me:6969/announce",
            "udp://explodie.org:6969/announce",
            "udp://tracker.tiny-vps.com:6969/announce",
            "udp://tracker.pomf.se:80/announce",
            "udp://p4p.arenabg.com:1337/announce"
        };
        for (const QString &tracker : publicTrackers) {
            QString encoded = QUrl::toPercentEncoding(tracker);
            if (!magnet.contains(encoded) && !magnet.contains(tracker)) {
                magnet += "&tr=" + encoded;
            }
        }
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Prepared magnet link: stripped %1 onion trackers, injected %2 public trackers")
                         .arg(removed).arg(publicTrackers.size()));
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                         QString("Final magnet (first 200 chars): %1").arg(magnet.left(200)));
    }

    QStringList args;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    if (m_torrentClient.contains("transmission-cli")) {
        // Create a config directory with DHT/PEX/LPD enabled but NO incoming connections
        QString configDir = QDir(m_downloadDirectory).filePath(".transmission-config");
        QDir().mkpath(configDir);
        QString settingsPath = QDir(configDir).filePath("settings.json");
        QFile settingsFile(settingsPath);
        if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QJsonObject settings;
            settings["dht-enabled"] = true;
            settings["pex-enabled"] = true;
            settings["lpd-enabled"] = true;
            settings["port-forwarding-enabled"] = false;  // no UPnP/NAT-PMP
            settings["peer-port-random-on-start"] = false;
            settings["peer-port"] = 0;                    // disable listening port
            settings["encryption"] = 2;                   // require encrypted connections
            settings["download-dir"] = m_downloadDirectory;
        settingsFile.write(QJsonDocument(settings).toJson());
            settingsFile.close();
        }
        args << "-g" << configDir    // config dir
             << "-w" << m_downloadDirectory
             << magnet;
    } else if (m_torrentClient.contains("aria2c")) {
        // Use high ephemeral port range instead of port 0 (which disables listening
        // entirely and can cause exit code 28 timeouts on sparse swarms).
        // Ports 49152-65535 are ephemeral — outbound-initiated connections on these
        // won't trigger macOS firewall prompts.
        args << "--enable-dht=true"
             << "--dht-listen-port=49152-65535"
             << "--listen-port=49152-65535"
             << "--bt-enable-lpd=true"
             << "--enable-peer-exchange=true"
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

    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentDownloadService",
                     QString("Starting: %1 %2").arg(m_torrentClient, args.join(" ").left(300)));

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
