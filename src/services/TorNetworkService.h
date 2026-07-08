#ifndef TORNETWORKSERVICE_H
#define TORNETWORKSERVICE_H

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include "BaseService.h"

class TorNetworkService : public BaseService
{
    Q_OBJECT

public:
    explicit TorNetworkService(QObject *parent = nullptr);
    ~TorNetworkService();

    // BaseService implementation
    QString getServiceName() const override { return "TorNetworkService"; }

    // Tor connection management
    bool startTorProcess();
    void stopTorProcess();
    bool isTorRunning() const;
    bool isTorReady() const;
    
    // Manual connection control
    bool connectToTor();
    void disconnectFromTor();

    // (Re)locate the Tor executable; returns true if a usable "tor" was found
    // (bundled with XFB, in a Tor Browser install, or on PATH).
    bool isTorAvailable();

    // Network requests through Tor
    QNetworkReply* makeRequest(const QUrl &url);
    QNetworkReply* makeRequestClearnet(const QUrl &url);
    void setTorProxy();
    void clearProxy();

    // Onion service discovery
    void findOnionMirror(const QString &clearnetDomain);
    QString getCurrentOnionMirror(const QString &clearnetDomain);
    
    // Security
    bool isSecureRequest(const QUrl &url, const QString &content = QString());

signals:
    void torStarted();
    void torStopped();
    void torReady();
    void torError(const QString &error);
    void onionMirrorFound(const QString &clearnetDomain, const QString &onionUrl);
    void onionMirrorSearchFailed(const QString &clearnetDomain);
    void searchingForOnionMirror(const QString &clearnetDomain);
    void requestFinished(QNetworkReply *reply);

private slots:
    void onTorProcessStarted();
    void onTorProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTorProcessError(QProcess::ProcessError error);
    void checkTorConnection();
    void handleOnionMirrorReply();

private:
    QProcess *m_torProcess;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_connectionTimer;
    bool m_torReady;
    bool m_autoConnect;
    int m_socksPort;
    int m_controlPort;
    int m_connectionRetries;
    QString m_torDataDir;
    QString m_torExecutable;
    QString m_torOutput;
    
    // Security filters
    QStringList m_blockedDomains;
    QStringList m_suspiciousPatterns;

    // Helper methods
    bool findTorExecutable();
    QString createTorConfig();
    bool testTorConnection();
    void setupTorProxy();
    void setupSecurityFilters();
    void findOnionMirrorThroughDirectory(const QString &clearnetDomain);
    QStringList getKnownOnionMirrors(const QString &domain);
};

#endif // TORNETWORKSERVICE_H