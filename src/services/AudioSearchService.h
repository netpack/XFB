#ifndef AUDIOSEARCHSERVICE_H
#define AUDIOSEARCHSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "BaseService.h"

struct AudioSearchResult {
    QString title;
    QString artist;
    QString album;
    QString duration;
    QString previewUrl;
    QString sourceType; // "podcast", "radio", "archive", etc.
    QString description;
    bool isLegal;
};

class AudioSearchService : public BaseService
{
    Q_OBJECT

public:
    explicit AudioSearchService(QObject *parent = nullptr);
    ~AudioSearchService();

    // Search for legal audio content
    void searchPodcasts(const QString &query);
    void searchInternetArchive(const QString &query);
    void searchRadioStations(const QString &query);
    void searchFreeMusicArchive(const QString &query);
    
    // Cancel ongoing searches
    void cancelSearch();

signals:
    void searchResultsReady(const QList<AudioSearchResult> &results);
    void searchError(const QString &error);
    void searchProgress(int percentage);

private slots:
    void handlePodcastSearchReply();
    void handleArchiveSearchReply();
    void handleRadioSearchReply();
    void handleFreeMusicReply();
    void handleNetworkError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_networkManager;
    QList<QNetworkReply*> m_activeReplies;
    
    // Parse different API responses
    QList<AudioSearchResult> parsePodcastResults(const QJsonDocument &doc);
    QList<AudioSearchResult> parseArchiveResults(const QJsonDocument &doc);
    QList<AudioSearchResult> parseRadioResults(const QJsonDocument &doc);
    QList<AudioSearchResult> parseFreeMusicResults(const QJsonDocument &doc);
    
    // Security and validation
    bool validateUrl(const QString &url);
    QString sanitizeQuery(const QString &query);
};

#endif // AUDIOSEARCHSERVICE_H