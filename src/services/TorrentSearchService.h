#ifndef TORRENTSEARCHSERVICE_H
#define TORRENTSEARCHSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <functional>
#include "BaseService.h"
#include "TorNetworkService.h"
#include "TorrentTypes.h"

class TorrentSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TorrentSortProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}
protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override {
        int col = left.column();
        // Columns 1 (Size), 2 (Seeders), 3 (Leechers) — sort by UserRole numeric data
        if (col >= 1 && col <= 3) {
            QVariant leftData = sourceModel()->data(left, Qt::UserRole);
            QVariant rightData = sourceModel()->data(right, Qt::UserRole);
            return leftData.toLongLong() < rightData.toLongLong();
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }
};

class TorrentSearchService : public BaseService
{
    Q_OBJECT

public:
    explicit TorrentSearchService(QObject *parent = nullptr);
    ~TorrentSearchService();

    // BaseService implementation
    QString getServiceName() const override { return "TorrentSearchService"; }

    // Search functionality
    void searchTorrents(const QString &query);
    void cancelSearch();
    
    // Set Tor service
    void setTorService(TorNetworkService *torService);
    
    // Get results model (returns the sort proxy for proper numeric sorting)
    QAbstractItemModel* getResultsModel() const { return m_sortProxy; }
    // Get the underlying source model (for direct row manipulation)
    QStandardItemModel* getSourceModel() const { return m_resultsModel; }

signals:
    void searchStarted();
    void searchFinished();
    void searchProgress(int percentage);
    void searchError(const QString &error);
    void resultsReady(const QList<TorrentSearchResult> &results);
    void onionSitesUnavailable();

private slots:
    void handleSearchReply();
    void onTorReady();
    void onSearchTimeout();

private:
    TorNetworkService *m_torService;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_searchTimeout;
    QStandardItemModel *m_resultsModel;
    TorrentSortProxyModel *m_sortProxy;
    QList<QNetworkReply*> m_activeReplies;
    QString m_currentQuery;
    
    // Search sites
    QStringList m_searchSites;
    QStringList m_onionSearchSites;
    int m_currentSiteIndex;
    bool m_waitingForMirror;
    int m_retryCount = 0;
    bool m_useClearnetFallback = false;
    QString m_clearnetUrl = "https://1337x.to";
    
    // Parsing methods
    QList<TorrentSearchResult> parse1337xResults(const QString &html);
    QList<TorrentSearchResult> parseFallback(const QString &html);
    QList<TorrentSearchResult> parseGenericResults(const QString &html);
    
    // Filtering and validation
    bool isAudioTorrent(const QString &name, const QString &category);
    bool isValidTorrent(const TorrentSearchResult &result);
    bool isSecureTorrent(const TorrentSearchResult &result);
    QString sanitizeHtml(const QString &html);
    QString extractMagnetLink(const QString &html, const QString &torrentName);
    
    // Security
    bool containsMaliciousContent(const QString &content);
    QString removeAds(const QString &html);
    
    // Model management
    void updateResultsModel(const QList<TorrentSearchResult> &results);
    
    // Site management
    void searchNextSite();
    QString getCurrentSiteUrl();
    void findOnionMirrors();
    
public:
    // Model management
    void clearResults();

    // Fetch the magnet link from a torrent detail page
    void fetchMagnetLink(const QString &torrentPagePath, std::function<void(const QString &magnetLink)> callback);

    // Enable clearnet fallback (search 1337x.to through Tor SOCKS proxy)
    void enableClearnetFallback(bool enable);
    bool isClearnetFallbackEnabled() const { return m_useClearnetFallback; }
};

#endif // TORRENTSEARCHSERVICE_H