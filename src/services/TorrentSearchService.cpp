#include "TorrentSearchService.h"
#include "ErrorHandler.h"
#include "Logger.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStandardItem>
#include <QUrl>
#include <QTextDocument>

TorrentSearchService::TorrentSearchService(QObject *parent)
    : BaseService(parent)
    , m_torService(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_searchTimeout(new QTimer(this))
    , m_resultsModel(new QStandardItemModel(this))
    , m_sortProxy(new TorrentSortProxyModel(this))
    , m_currentSiteIndex(0)
    , m_waitingForMirror(false)
{
    m_searchTimeout->setSingleShot(true);
    m_searchTimeout->setInterval(180000);  // 3 minutes — Tor can be slow
    connect(m_searchTimeout, &QTimer::timeout, this, &TorrentSearchService::onSearchTimeout);
    
    // Set up results model headers
    QStringList headers;
    headers << "Name" << "Size" << "Seeders" << "Leechers" << "Upload Date" << "Category";
    m_resultsModel->setHorizontalHeaderLabels(headers);
    m_sortProxy->setSourceModel(m_resultsModel);
}

TorrentSearchService::~TorrentSearchService()
{
    cancelSearch();
}

void TorrentSearchService::setTorService(TorNetworkService *torService)
{
    m_torService = torService;
    if (m_torService) {
        connect(m_torService, &TorNetworkService::torReady, this, &TorrentSearchService::onTorReady);
        connect(m_torService, &TorNetworkService::onionMirrorFound, 
                this, [this](const QString &clearnet, const QString &onion) {
            Q_UNUSED(clearnet);
            // Store the discovered onion URL
            if (!m_onionSearchSites.contains(onion)) {
                m_onionSearchSites.append(onion);
                ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                                 QString("Added onion search site: %1").arg(onion));
            }
            // If we were waiting for a mirror before searching, start now
            if (m_waitingForMirror && !m_currentQuery.isEmpty()) {
                m_waitingForMirror = false;
                m_currentSiteIndex = 0;
                searchNextSite();
            }
        });
    }
}

void TorrentSearchService::searchTorrents(const QString &query)
{
    if (query.trimmed().isEmpty()) {
        emit searchError("Search query cannot be empty");
        return;
    }
    
    if (!m_torService || !m_torService->isTorReady()) {
        emit searchError("Tor network is not ready. Please wait for Tor to connect.");
        return;
    }
    
    cancelSearch();
    
    m_currentQuery = query.trimmed();
    m_currentSiteIndex = 0;
    m_retryCount = 0;
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                     QString("Starting torrent search for: %1").arg(m_currentQuery));
    
    clearResults();
    emit searchStarted();
    m_searchTimeout->start();
    
    // If we already have onion sites, search immediately
    if (!m_onionSearchSites.isEmpty()) {
        searchNextSite();
    } else {
        // Request onion mirrors and wait for them
        m_waitingForMirror = true;
        findOnionMirrors();
        
        // Fallback: if no mirror found within 45 seconds, try clearnet or report error
        QTimer::singleShot(45000, this, [this]() {
            if (m_waitingForMirror) {
                m_waitingForMirror = false;
                if (m_onionSearchSites.isEmpty()) {
                    if (m_useClearnetFallback) {
                        // Use clearnet fallback directly
                        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                                         "No onion mirrors found, using clearnet fallback");
                        emit searchError("Onion sites unavailable — searching via clearnet through Tor...");
                        m_currentSiteIndex = 0;
                        searchNextSite();
                    } else {
                        emit onionSitesUnavailable();
                        emit searchError("No onion search sites available. You can enable clearnet fallback to search via 1337x.to through Tor.");
                        emit searchFinished();
                    }
                }
            }
        });
    }
}

void TorrentSearchService::cancelSearch()
{
    m_searchTimeout->stop();
    m_waitingForMirror = false;
    
    for (QNetworkReply *reply : m_activeReplies) {
        if (reply) {
            if (reply->isRunning()) {
                reply->abort();
            }
            reply->deleteLater();
        }
    }
    m_activeReplies.clear();
}

void TorrentSearchService::onTorReady()
{
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                           "Tor network is ready for searches");
}

void TorrentSearchService::onSearchTimeout()
{
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService", 
                           "Search timeout reached");
    cancelSearch();

    // Auto-retry once before giving up
    if (m_retryCount < 1 && !m_currentQuery.isEmpty()) {
        m_retryCount++;
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService",
                         QString("Auto-retrying search (attempt %1)").arg(m_retryCount + 1));
        emit searchError("Search timed out — retrying automatically...");
        m_currentSiteIndex = 0;
        m_searchTimeout->start();
        searchNextSite();
        return;
    }

    m_retryCount = 0;
    emit searchError("Search timeout after retry. The onion site may be slow — please try again.");
    emit searchFinished();
}

void TorrentSearchService::searchNextSite()
{
    if (m_currentSiteIndex >= m_onionSearchSites.size()) {
        // All onion sites exhausted — try clearnet fallback if enabled
        if (m_useClearnetFallback && !m_clearnetUrl.isEmpty()) {
            QString encodedQuery = QUrl::toPercentEncoding(m_currentQuery);
            QUrl searchUrl(m_clearnetUrl + "/search/" + encodedQuery + "/1/");
            
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                             QString("Onion sites exhausted, using clearnet fallback: %1").arg(searchUrl.toString()));
            
            QNetworkReply *reply = m_torService->makeRequestClearnet(searchUrl);
            if (reply) {
                m_activeReplies.append(reply);
                connect(reply, &QNetworkReply::finished, this, &TorrentSearchService::handleSearchReply);
                // Prevent re-entering this branch on the next call
                m_currentSiteIndex = m_onionSearchSites.size() + 1;
                emit searchProgress(90);
                return;
            } else {
                ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService", 
                                 "Clearnet fallback request failed");
            }
        }
        
        emit searchFinished();
        return;
    }
    
    QString siteUrl = m_onionSearchSites[m_currentSiteIndex];
    if (siteUrl.isEmpty()) {
        m_currentSiteIndex++;
        searchNextSite();
        return;
    }
    
    // Build search URL — 1337x uses /search/<query>/<page>/
    QString encodedQuery = QUrl::toPercentEncoding(m_currentQuery);
    QUrl searchUrl(siteUrl + "/search/" + encodedQuery + "/1/");
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                     QString("Searching: %1").arg(searchUrl.toString()));
    
    QNetworkReply *reply = m_torService->makeRequest(searchUrl);
    if (reply) {
        m_activeReplies.append(reply);
        connect(reply, &QNetworkReply::finished, this, &TorrentSearchService::handleSearchReply);
    } else {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService", 
                         "makeRequest returned null — URL may not be .onion");
        m_currentSiteIndex++;
        searchNextSite();
    }
    
    int progress = ((m_currentSiteIndex + 1) * 100) / qMax(1, m_onionSearchSites.size());
    emit searchProgress(progress);
}

QString TorrentSearchService::getCurrentSiteUrl()
{
    if (m_currentSiteIndex < m_onionSearchSites.size()) {
        return m_onionSearchSites[m_currentSiteIndex];
    }
    return QString();
}

void TorrentSearchService::findOnionMirrors()
{
    if (m_torService) {
        m_torService->findOnionMirror("1337x");
    }
}

void TorrentSearchService::handleSearchReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    m_activeReplies.removeAll(reply);
    
    if (reply->error() != QNetworkReply::NoError) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService", 
                         QString("Search request failed: %1").arg(reply->errorString()));
    } else {
        QString html = QString::fromUtf8(reply->readAll());
        
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                         QString("Received %1 bytes from %2").arg(html.size()).arg(reply->url().toString()));
        
        // Debug: log first 500 chars to diagnose what the site is returning
        QString preview = html.left(500).simplified();
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                         QString("Response preview: %1").arg(preview));
        
        // Parse results based on the site
        QList<TorrentSearchResult> results = parse1337xResults(html);
        
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                         QString("Parsed %1 results").arg(results.size()));
        
        if (!results.isEmpty()) {
            updateResultsModel(results);
            emit resultsReady(results);
        } else {
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                             "No results found on this site");
        }
    }
    
    reply->deleteLater();
    
    m_currentSiteIndex++;
    searchNextSite();
}

QList<TorrentSearchResult> TorrentSearchService::parse1337xResults(const QString &html)
{
    QList<TorrentSearchResult> results;
    
    // 1337x search results are in a <table class="table-list table-list-condensed">
    // Each result row: <td class="coll-1 name"><a href="/sub/...">Category</a><a href="/torrent/...">Name</a></td>
    //                  <td class="coll-2 seeds">N</td>
    //                  <td class="coll-3 leeches">N</td>
    //                  <td class="coll-date">Date</td>
    //                  <td class="coll-4 size ...">Size<span>unit</span></td>
    //                  <td class="coll-5 uploader">Name</td>
    
    // Extract each table row from the results table body
    QRegularExpression rowRe(
        "<tr>\\s*<td[^>]*class=\"[^\"]*name[^\"]*\"[^>]*>(.*?)</td>"
        "\\s*<td[^>]*class=\"[^\"]*seeds[^\"]*\"[^>]*>(\\d+)</td>"
        "\\s*<td[^>]*class=\"[^\"]*leeches[^\"]*\"[^>]*>(\\d+)</td>"
        "\\s*<td[^>]*class=\"[^\"]*date[^\"]*\"[^>]*>([^<]+)</td>"
        "\\s*<td[^>]*class=\"[^\"]*size[^\"]*\"[^>]*>([^<]+)<",
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption
    );
    
    QRegularExpressionMatchIterator rowMatches = rowRe.globalMatch(html);
    
    while (rowMatches.hasNext()) {
        QRegularExpressionMatch row = rowMatches.next();
        
        QString nameCell = row.captured(1);
        
        // Extract the torrent name and URL from the name cell
        // The second <a> tag in the name cell contains the actual torrent name/link
        QRegularExpression linkRe("<a\\s+href=\"(/torrent/[^\"]+)\"[^>]*>([^<]+)</a>",
                                  QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch linkMatch = linkRe.match(nameCell);
        
        if (!linkMatch.hasMatch()) continue;
        
        TorrentSearchResult result;
        result.torrentUrl = linkMatch.captured(1);
        result.name = linkMatch.captured(2).trimmed();
        result.seeders = row.captured(2).trimmed();
        result.leechers = row.captured(3).trimmed();
        result.uploadDate = row.captured(4).trimmed();
        result.size = row.captured(5).trimmed();
        result.isAudio = isAudioTorrent(result.name, result.category);
        result.isTrusted = false;
        
        // Extract category from the first <a> in the name cell (before the torrent link)
        QRegularExpression catRe("<a\\s+href=\"/sub/[^\"]+\"[^>]*>([^<]+)</a>",
                                 QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch catMatch = catRe.match(nameCell);
        if (catMatch.hasMatch()) {
            result.category = catMatch.captured(1).trimmed();
            result.isAudio = isAudioTorrent(result.name, result.category);
        }
        
        if (isValidTorrent(result)) {
            results.append(result);
        }
    }
    
    // If the table regex didn't match, try a simpler fallback approach
    if (results.isEmpty()) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                         "Table regex didn't match, trying fallback parser");
        results = parseFallback(html);
    }
    
    return results;
}

QList<TorrentSearchResult> TorrentSearchService::parseFallback(const QString &html)
{
    QList<TorrentSearchResult> results;
    
    // Fallback: find all torrent links and extract what we can
    QRegularExpression torrentLinkRe(
        "<a\\s+href=\"(/torrent/\\d+/[^\"]+)\"[^>]*>([^<]+)</a>",
        QRegularExpression::CaseInsensitiveOption
    );
    
    QRegularExpressionMatchIterator matches = torrentLinkRe.globalMatch(html);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        
        TorrentSearchResult result;
        result.torrentUrl = match.captured(1);
        result.name = match.captured(2).trimmed();
        result.isAudio = isAudioTorrent(result.name, result.category);
        result.isTrusted = false;
        
        if (!result.name.isEmpty() && result.name.length() > 3) {
            results.append(result);
        }
    }
    
    return results;
}

QList<TorrentSearchResult> TorrentSearchService::parseGenericResults(const QString &html)
{
    Q_UNUSED(html);
    return {};
}

bool TorrentSearchService::isAudioTorrent(const QString &name, const QString &category)
{
    // Check category first
    QStringList audioCategories = {"Music", "Audio", "Audiobooks", "Podcasts", "FLAC", "Lossless"};
    for (const QString &cat : audioCategories) {
        if (category.contains(cat, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Check name for audio indicators
    QStringList audioKeywords = {
        "album", "music", "song", "track", "audio", "mp3", "flac", "wav", "ogg",
        "m4a", "aac", "wma", "discography", "soundtrack", "podcast", "audiobook",
        "lossless", "320kbps", "256kbps", "192kbps", "vinyl", "remaster"
    };
    QString lowerName = name.toLower();
    for (const QString &keyword : audioKeywords) {
        if (lowerName.contains(keyword)) {
            return true;
        }
    }
    
    return false;
}

bool TorrentSearchService::isValidTorrent(const TorrentSearchResult &result)
{
    if (result.name.isEmpty() || result.name.length() < 3) {
        return false;
    }
    
    // Block obviously dangerous content
    QStringList blocked = {"crack", "keygen", "patch.exe", "loader.exe", "activator"};
    QString lowerName = result.name.toLower();
    for (const QString &b : blocked) {
        if (lowerName.contains(b)) {
            return false;
        }
    }
    
    return true;
}

bool TorrentSearchService::isSecureTorrent(const TorrentSearchResult &result)
{
    // Check magnet link format if present
    if (!result.magnetLink.isEmpty()) {
        if (!result.magnetLink.startsWith("magnet:?xt=urn:btih:")) {
            return false;
        }
    }
    return true;
}

QString TorrentSearchService::sanitizeHtml(const QString &html)
{
    QString sanitized = html;
    QRegularExpression scriptRe("<script[^>]*>.*?</script>", 
                                QRegularExpression::CaseInsensitiveOption | 
                                QRegularExpression::DotMatchesEverythingOption);
    sanitized.remove(scriptRe);
    return sanitized;
}

QString TorrentSearchService::extractMagnetLink(const QString &html, const QString &torrentName)
{
    Q_UNUSED(torrentName);
    QRegularExpression magnetRe("magnet:\\?xt=urn:btih:[^\"'\\s]+");
    QRegularExpressionMatch match = magnetRe.match(html);
    return match.hasMatch() ? match.captured(0) : QString();
}

bool TorrentSearchService::containsMaliciousContent(const QString &content)
{
    Q_UNUSED(content);
    // Simplified — the old version blocked all real web pages
    return false;
}

QString TorrentSearchService::removeAds(const QString &html)
{
    // Just strip iframes and embeds, leave the rest alone
    QString cleaned = html;
    QRegularExpression iframeRe("<iframe[^>]*>.*?</iframe>", 
                                QRegularExpression::CaseInsensitiveOption | 
                                QRegularExpression::DotMatchesEverythingOption);
    cleaned.remove(iframeRe);
    return cleaned;
}

static qint64 parseSizeToBytes(const QString &sizeStr)
{
    // Parse strings like "853 MB", "1.2 GB", "450 KB", "853MB"
    QRegularExpression re("([\\d.]+)\\s*(KB|MB|GB|TB|B)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sizeStr.trimmed());
    if (!m.hasMatch()) return 0;

    double value = m.captured(1).toDouble();
    QString unit = m.captured(2).toUpper();

    if (unit == "TB") return static_cast<qint64>(value * 1099511627776.0);
    if (unit == "GB") return static_cast<qint64>(value * 1073741824.0);
    if (unit == "MB") return static_cast<qint64>(value * 1048576.0);
    if (unit == "KB") return static_cast<qint64>(value * 1024.0);
    return static_cast<qint64>(value);
}

void TorrentSearchService::updateResultsModel(const QList<TorrentSearchResult> &results)
{
    for (const auto &result : results) {
        QList<QStandardItem*> row;
        row << new QStandardItem(result.name);

        // Size item: store numeric bytes in UserRole for proper sorting
        auto *sizeItem = new QStandardItem(result.size);
        sizeItem->setData(parseSizeToBytes(result.size), Qt::UserRole);
        row << sizeItem;

        // Seeders/Leechers: store as int in UserRole for numeric sorting
        auto *seedItem = new QStandardItem(result.seeders);
        seedItem->setData(result.seeders.toInt(), Qt::UserRole);
        row << seedItem;

        auto *leechItem = new QStandardItem(result.leechers);
        leechItem->setData(result.leechers.toInt(), Qt::UserRole);
        row << leechItem;

        row << new QStandardItem(result.uploadDate);
        row << new QStandardItem(result.category);
        
        // Store full result data in the first item
        row[0]->setData(QVariant::fromValue(result), Qt::UserRole);
        
        m_resultsModel->appendRow(row);
    }
}

void TorrentSearchService::clearResults()
{
    m_resultsModel->clear();
    QStringList headers;
    headers << "Name" << "Size" << "Seeders" << "Leechers" << "Upload Date" << "Category";
    m_resultsModel->setHorizontalHeaderLabels(headers);
}

void TorrentSearchService::fetchMagnetLink(const QString &torrentPagePath, std::function<void(const QString &magnetLink)> callback)
{
    if (!m_torService || !m_torService->isTorReady()) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService",
                         "Cannot fetch magnet link: Tor not ready");
        if (callback) callback(QString());
        return;
    }

    // Determine base URL: prefer onion, fall back to clearnet
    QString baseUrl;
    if (!m_onionSearchSites.isEmpty()) {
        baseUrl = m_onionSearchSites[0];
    } else if (m_useClearnetFallback) {
        baseUrl = m_clearnetUrl;
    } else {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService",
                         "Cannot fetch magnet link: no onion sites and clearnet fallback disabled");
        if (callback) callback(QString());
        return;
    }

    // Build the full URL: base + torrent page path (e.g. /torrent/12345/name)
    QString fullUrl = baseUrl + torrentPagePath;
    
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService",
                     QString("Fetching magnet link from: %1").arg(fullUrl));

    QUrl url(fullUrl);
    QNetworkReply *reply = nullptr;
    
    if (url.host().endsWith(".onion")) {
        reply = m_torService->makeRequest(url);
    } else {
        reply = m_torService->makeRequestClearnet(url);
    }
    
    if (!reply) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService",
                         "makeRequest returned null for torrent detail page");
        if (callback) callback(QString());
        return;
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        QString magnetLink;
        if (reply->error() == QNetworkReply::NoError) {
            QString html = QString::fromUtf8(reply->readAll());
            
            // Extract magnet link from the detail page
            QRegularExpression magnetRe("magnet:\\?xt=urn:btih:[^\"'\\s<>]+");
            QRegularExpressionMatch match = magnetRe.match(html);
            if (match.hasMatch()) {
                magnetLink = match.captured(0);
                ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService",
                                 QString("Found magnet link: %1").arg(magnetLink.left(80) + "..."));
            } else {
                ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService",
                                 "No magnet link found on torrent detail page");
            }
        } else {
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "TorrentSearchService",
                             QString("Failed to fetch torrent detail page: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
        if (callback) callback(magnetLink);
    });
}

void TorrentSearchService::enableClearnetFallback(bool enable)
{
    m_useClearnetFallback = enable;
    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "TorrentSearchService", 
                     QString("Clearnet fallback %1").arg(enable ? "enabled" : "disabled"));
}
