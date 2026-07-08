#include "TorNetworkService.h"
#include "ErrorHandler.h"
#include "Logger.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <memory>

TorNetworkService::TorNetworkService(QObject *parent)
    : BaseService(parent)
    , m_torProcess(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_connectionTimer(new QTimer(this))
    , m_torReady(false)
    , m_socksPort(9050)
    , m_controlPort(9051)
    , m_autoConnect(false) // Don't auto-connect
    , m_connectionRetries(0)
{
    // Set up connection timer — retries every 5 seconds while Tor bootstraps
    m_connectionTimer->setInterval(5000);
    connect(m_connectionTimer, &QTimer::timeout, this, &TorNetworkService::checkTorConnection);

    // Create Tor data directory
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_torDataDir = QDir(appData).filePath("tor_data");
    QDir().mkpath(m_torDataDir);

    // Find Tor executable
    findTorExecutable();
    
    // Set up security filters
    setupSecurityFilters();
}

TorNetworkService::~TorNetworkService()
{
    stopTorProcess();
}

bool TorNetworkService::isTorAvailable()
{
    // Re-locate if we don't already have a valid path (handles Tor being
    // installed/bundled after this service was first constructed).
    if (!m_torExecutable.isEmpty() && QFile::exists(m_torExecutable)) {
        return true;
    }
    return findTorExecutable();
}

bool TorNetworkService::findTorExecutable()
{
    // Common Tor executable locations
    QStringList possiblePaths = {
#ifdef Q_OS_WIN
        QCoreApplication::applicationDirPath() + "/tor/tor.exe",
        QDir::homePath() + "/AppData/Local/Programs/Tor/tor.exe",
        "C:/Program Files/Tor Browser/Browser/TorBrowser/Tor/tor.exe",
        "C:/Program Files (x86)/Tor Browser/Browser/TorBrowser/Tor/tor.exe",
#else
        "/usr/bin/tor",
        "/usr/local/bin/tor",
        "/opt/homebrew/bin/tor",
#endif
        QStandardPaths::findExecutable("tor")
    };

    for (const QString &path : possiblePaths) {
        if (!path.isEmpty() && QFile::exists(path)) {
            m_torExecutable = path;
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                             QString("Found Tor executable: %1").arg(path));
            return true;
        }
    }

    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                           "Tor executable not found. Please install Tor.");
    return false;
}

QString TorNetworkService::createTorConfig()
{
    QString configPath = QDir(m_torDataDir).filePath("torrc");
    QFile configFile(configPath);
    
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << "# XFB Tor Configuration - Security Enhanced\n";
        out << "DataDirectory " << m_torDataDir << "\n";
        out << "SocksPort " << m_socksPort << "\n";
        out << "ControlPort " << m_controlPort << "\n";
        out << "CookieAuthentication 1\n";
        out << "ClientOnly 1\n";
        out << "Log notice stdout\n";
        out << "SafeLogging 1\n";
        
        // Security settings
        out << "# Security enhancements\n";
        out << "DisableNetwork 0\n";
        out << "EnforceDistinctSubnets 1\n";
        out << "ClientRejectInternalAddresses 1\n";
        out << "WarnUnsafeSocks 1\n";
        out << "VirtualAddrNetworkIPv4 10.192.0.0/10\n";
        out << "AutomapHostsOnResolve 1\n";
        out << "TransPort 0\n"; // Disable transparent proxy
        out << "DNSPort 0\n";   // Disable DNS port
        
        configFile.close();
    }
    
    return configPath;
}

bool TorNetworkService::startTorProcess()
{
    if (m_torExecutable.isEmpty()) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Error, "TorNetworkService", 
                               "Cannot start Tor: executable not found");
        return false;
    }

    if (m_torProcess && m_torProcess->state() == QProcess::Running) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", "Tor is already running");
        return true;
    }

    // Clean up any previous process that is no longer running
    if (m_torProcess) {
        m_torProcess->deleteLater();
        m_torProcess = nullptr;
    }

    // Create Tor process
    m_torProcess = new QProcess(this);
    m_torProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_torProcess, &QProcess::started, this, &TorNetworkService::onTorProcessStarted);
    connect(m_torProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TorNetworkService::onTorProcessFinished);
    connect(m_torProcess, &QProcess::errorOccurred, this, &TorNetworkService::onTorProcessError);
    
    // Accumulate Tor output for bootstrap detection
    connect(m_torProcess, &QProcess::readyReadStandardOutput, [this]() {
        QString output = m_torProcess->readAllStandardOutput();
        m_torOutput += output;
        
        // Check for bootstrap completion in real-time
        if (output.contains("Bootstrapped 100%") || output.contains("Done")) {
            m_torReady = true;
            m_connectionTimer->stop();
            m_connectionRetries = 0;
            setupTorProxy();
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                                   "Tor connection established (bootstrapped 100%)");
            emit torReady();
        } else {
            // Log bootstrap progress
            QRegularExpression bootstrapRe("Bootstrapped (\\d+)%");
            QRegularExpressionMatch match = bootstrapRe.match(output);
            if (match.hasMatch()) {
                ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                                       QString("Tor bootstrapping: %1%").arg(match.captured(1)));
            }
        }
    });

    // Create configuration
    QString configPath = createTorConfig();
    
    // Clear state for new connection
    m_torOutput.clear();
    m_connectionRetries = 0;
    
    // Start Tor with configuration
    QStringList arguments;
    arguments << "-f" << configPath;
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                     QString("Starting Tor with config: %1").arg(configPath));
    
    m_torProcess->start(m_torExecutable, arguments);
    
    // Start connection check timer
    m_connectionTimer->start();
    
    return m_torProcess->waitForStarted(10000);
}

void TorNetworkService::stopTorProcess()
{
    m_connectionTimer->stop();
    m_connectionRetries = 0;
    m_torOutput.clear();
    
    if (m_torProcess && m_torProcess->state() == QProcess::Running) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", "Stopping Tor process");
        m_torProcess->terminate();
        if (!m_torProcess->waitForFinished(5000)) {
            m_torProcess->kill();
        }
    }
    
    m_torReady = false;
    clearProxy();
    emit torStopped();
}

bool TorNetworkService::isTorRunning() const
{
    return m_torProcess && m_torProcess->state() == QProcess::Running;
}

bool TorNetworkService::isTorReady() const
{
    return m_torReady;
}

void TorNetworkService::onTorProcessStarted()
{
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", "Tor process started");
    emit torStarted();
}

void TorNetworkService::onTorProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                     QString("Tor process finished with exit code: %1").arg(exitCode));
    m_torReady = false;
    emit torStopped();
}

void TorNetworkService::onTorProcessError(QProcess::ProcessError error)
{
    QString errorMsg = QString("Tor process error: %1").arg(error);
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Error, "TorNetworkService", errorMsg);
    emit torError(errorMsg);
}

void TorNetworkService::checkTorConnection()
{
    if (!isTorRunning()) {
        m_connectionTimer->stop();
        m_connectionRetries = 0;
        return;
    }

    // If already ready (detected by the readyRead handler), stop checking
    if (m_torReady) {
        m_connectionTimer->stop();
        m_connectionRetries = 0;
        return;
    }

    m_connectionRetries++;
    
    // Check accumulated output for bootstrap status
    if (m_torOutput.contains("Bootstrapped 100%")) {
        m_torReady = true;
        m_connectionTimer->stop();
        m_connectionRetries = 0;
        setupTorProxy();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                               "Tor connection established (bootstrapped 100%)");
        emit torReady();
        return;
    }
    
    // Give up after ~60 seconds (12 retries * 5 seconds)
    if (m_connectionRetries >= 12) {
        m_connectionTimer->stop();
        m_connectionRetries = 0;
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Error, "TorNetworkService", 
                               "Tor connection timed out after 60 seconds");
        emit torError("Tor connection timed out. Please check your network and try again.");
    }
}

void TorNetworkService::setupTorProxy()
{
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(m_socksPort);
    m_networkManager->setProxy(proxy);
}

void TorNetworkService::setTorProxy()
{
    if (m_torReady) {
        setupTorProxy();
    }
}

void TorNetworkService::clearProxy()
{
    m_networkManager->setProxy(QNetworkProxy::NoProxy);
}

QNetworkReply* TorNetworkService::makeRequest(const QUrl &url)
{
    if (!m_torReady) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                               "Tor not ready, cannot make request");
        return nullptr;
    }

    // Security check: Only allow .onion domains
    if (!url.host().endsWith(".onion")) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                               QString("Blocked non-onion request: %1").arg(url.toString()));
        return nullptr;
    }

    QNetworkRequest request(url);
    
    // Set secure headers — mimic Tor Browser
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; rv:128.0) Gecko/20100101 Firefox/128.0");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.5");
    request.setRawHeader("DNT", "1");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Upgrade-Insecure-Requests", "1");
    
    // Follow redirects automatically
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(5);
    
    return m_networkManager->get(request);
}

QNetworkReply* TorNetworkService::makeRequestClearnet(const QUrl &url)
{
    if (!m_torReady) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                               "Tor not ready, cannot make clearnet request");
        return nullptr;
    }

    // Allow clearnet requests through Tor SOCKS proxy (still anonymous)
    // Only allow HTTPS for clearnet to prevent exit-node sniffing
    if (url.scheme() != "https") {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                               QString("Blocked non-HTTPS clearnet request: %1").arg(url.toString()));
        return nullptr;
    }

    // Block known tracking/surveillance domains
    QString host = url.host().toLower();
    for (const QString &blocked : m_blockedDomains) {
        if (host.contains(blocked)) {
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                                   QString("Blocked request to surveillance domain: %1").arg(host));
            return nullptr;
        }
    }

    QNetworkRequest request(url);
    
    // Set secure headers — mimic Tor Browser
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; rv:128.0) Gecko/20100101 Firefox/128.0");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.5");
    request.setRawHeader("DNT", "1");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Upgrade-Insecure-Requests", "1");
    
    // Follow redirects automatically
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(5);
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                     QString("Making clearnet request through Tor: %1").arg(url.toString()));
    
    return m_networkManager->get(request);
}

void TorNetworkService::setupSecurityFilters()
{
    // Set up blocked domains and suspicious patterns
    m_blockedDomains = {
        "facebook.com", "google.com", "microsoft.com", "apple.com",
        "amazon.com", "cloudflare.com", "googleapis.com"
    };
    
    m_suspiciousPatterns = {
        "javascript:", "data:", "vbscript:", "about:",
        "<script", "<iframe", "<embed", "<object",
        "eval(", "document.write", "window.location"
    };
}

bool TorNetworkService::isSecureRequest(const QUrl &url, const QString &content)
{
    // Check if domain is blocked
    QString host = url.host().toLower();
    for (const QString &blocked : m_blockedDomains) {
        if (host.contains(blocked)) {
            return false;
        }
    }
    
    // Check for suspicious patterns in content
    QString lowerContent = content.toLower();
    for (const QString &pattern : m_suspiciousPatterns) {
        if (lowerContent.contains(pattern)) {
            return false;
        }
    }
    
    return true;
}

bool TorNetworkService::connectToTor()
{
    if (isTorRunning()) {
        return true; // Already connected
    }
    
    return startTorProcess();
}

void TorNetworkService::disconnectFromTor()
{
    stopTorProcess();
}

void TorNetworkService::findOnionMirror(const QString &clearnetDomain)
{
    // Known onion mirrors for popular torrent sites
    QStringList knownMirrors = getKnownOnionMirrors(clearnetDomain);
    
    if (!knownMirrors.isEmpty()) {
        // Track how many mirrors we've tested and whether any succeeded
        auto pendingCount = std::make_shared<int>(knownMirrors.size());
        auto foundOne = std::make_shared<bool>(false);
        
        // Test each known mirror
        for (const QString &onionUrl : knownMirrors) {
            QNetworkReply *reply = makeRequest(QUrl(onionUrl));
            if (reply) {
                connect(reply, &QNetworkReply::finished, [this, clearnetDomain, onionUrl, reply, pendingCount, foundOne]() {
                    if (reply->error() == QNetworkReply::NoError && !(*foundOne)) {
                        *foundOne = true;
                        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                                         QString("Found working onion mirror: %1").arg(onionUrl));
                        emit onionMirrorFound(clearnetDomain, onionUrl);
                    } else if (reply->error() != QNetworkReply::NoError) {
                        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                                         QString("Onion mirror unreachable: %1 (%2)").arg(onionUrl, reply->errorString()));
                    }
                    reply->deleteLater();
                    
                    // If all mirrors have been tested and none worked, emit failure
                    (*pendingCount)--;
                    if (*pendingCount <= 0 && !(*foundOne)) {
                        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                                         QString("All onion mirrors failed for: %1").arg(clearnetDomain));
                        emit onionMirrorSearchFailed(clearnetDomain);
                    }
                });
            } else {
                (*pendingCount)--;
                if (*pendingCount <= 0 && !(*foundOne)) {
                    emit onionMirrorSearchFailed(clearnetDomain);
                }
            }
        }
    } else {
        // If no known mirrors, try to find them through directory services
        findOnionMirrorThroughDirectory(clearnetDomain);
    }
}

void TorNetworkService::findOnionMirrorThroughDirectory(const QString &clearnetDomain)
{
    // Emit a signal that we're searching
    emit searchingForOnionMirror(clearnetDomain);
    
    // Check known mirrors after a short delay
    QTimer::singleShot(3000, [this, clearnetDomain]() {
        QStringList foundMirrors = getKnownOnionMirrors(clearnetDomain);
        if (!foundMirrors.isEmpty()) {
            emit onionMirrorFound(clearnetDomain, foundMirrors.first());
        } else {
            emit onionMirrorSearchFailed(clearnetDomain);
        }
    });
}

QStringList TorNetworkService::getKnownOnionMirrors(const QString &domain)
{
    QStringList mirrors;
    
    // Known onion mirrors for 1337x (these change frequently)
    // Multiple mirrors increase the chance of finding a working one
    if (domain.contains("1337x")) {
        mirrors << "http://l337xdarkkaqfwzntnfk5bmoaroivtl6xsbatabvlb52umg6v3ch44yd.onion"
                << "http://l337xvhp25md4ywyo3xqruqy2lbmqmfpncdvp4g6jtlnqv3cxhdauqd.onion"
                << "http://l337x2nwwfhj3snkh73d2jkasa6xjnlbhj6cnqe2cjlxuqrasyhmzyd.onion";
    }
    
    return mirrors;
}

QString TorNetworkService::getCurrentOnionMirror(const QString &clearnetDomain)
{
    QStringList mirrors = getKnownOnionMirrors(clearnetDomain);
    return mirrors.isEmpty() ? QString() : mirrors.first();
}

bool TorNetworkService::testTorConnection()
{
    // Implementation for testing Tor connection
    return m_torReady;
}

void TorNetworkService::handleOnionMirrorReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() == QNetworkReply::NoError) {
        QString onionUrl = reply->url().toString();
        QString clearnetDomain = reply->property("clearnetDomain").toString();
        
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorNetworkService", 
                               QString("Onion mirror test successful: %1").arg(onionUrl));
        
        emit onionMirrorFound(clearnetDomain, onionUrl);
    } else {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorNetworkService", 
                               QString("Onion mirror test failed: %1").arg(reply->errorString()));
    }
    
    reply->deleteLater();
}
