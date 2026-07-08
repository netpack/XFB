#ifndef TORRENTDOWNLOADSERVICE_H
#define TORRENTDOWNLOADSERVICE_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include <QJsonObject>
#include "BaseService.h"
#include "TorNetworkService.h"

struct TorrentDownload {
    QString magnetLink;
    QString name;
    QString downloadPath;
    QString status;      // "starting", "downloading", "seeding", "completed", "error", "cancelled"
    double progress;     // 0.0 to 100.0
    QString speed;
    QString eta;
    QString size;
    QString errorMessage; // human-readable error detail
    QStringList audioFiles;
    bool isStreaming;
    QProcess *process = nullptr;  // owning process for this download
};

class TorrentDownloadService : public BaseService
{
    Q_OBJECT

public:
    explicit TorrentDownloadService(QObject *parent = nullptr);
    ~TorrentDownloadService();

    QString getServiceName() const override { return "TorrentDownloadService"; }

    // Download management
    QString startDownload(const QString &magnetLink, const QString &name);
    void cancelDownload(const QString &downloadId);
    void cancelAllDownloads();
    void removeCompleted();
    bool retryDownload(const QString &downloadId);

    // Persistence — survives app close/kill
    void saveDownloadState();
    void loadDownloadState();
    void resumeDownloads();

    // Streaming
    void enableStreaming(const QString &downloadId);
    QStringList getStreamableFiles(const QString &downloadId);

    // Tor service
    void setTorService(TorNetworkService *torService);

    // Model for the downloads table view
    QStandardItemModel* getDownloadsModel() const { return m_downloadsModel; }

    // Query
    int activeDownloadCount() const;
    QString downloadDirectory() const { return m_downloadDirectory; }
    bool hasDownload(const QString &downloadId) const { return m_downloads.contains(downloadId); }
    QString downloadIdForRow(int row) const;

signals:
    void downloadStarted(const QString &downloadId);
    void downloadProgress(const QString &downloadId, double progress);
    void downloadCompleted(const QString &downloadId, const QStringList &audioFiles);
    void downloadError(const QString &downloadId, const QString &error);
    void downloadCancelled(const QString &downloadId);
    void streamingReady(const QString &downloadId, const QString &filePath);
    void downloadsChanged();  // emitted whenever the model is refreshed

private:
    TorNetworkService *m_torService;
    QTimer *m_statusTimer;
    QFileSystemWatcher *m_fileWatcher;
    QStandardItemModel *m_downloadsModel;

    QHash<QString, TorrentDownload> m_downloads;
    QStringList m_downloadOrder;  // keeps insertion order for row mapping
    QString m_downloadDirectory;
    QString m_torrentClient;
    QString m_stateFilePath;

    bool findTorrentClient();
    bool startTorrentClientForDownload(const QString &downloadId);
    void stopAllProcesses();
    void updateDownloadModel();
    void checkForCompletedFiles(const QString &downloadId);

    QStringList findAudioFiles(const QString &directory);
    bool isAudioFile(const QString &filePath);

    void setupStreaming(const QString &downloadId);
    bool canStreamFile(const QString &filePath, double downloadProgress);
    QString getFirstStreamableFile(const QString &downloadId);
};

#endif // TORRENTDOWNLOADSERVICE_H
