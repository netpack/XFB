#ifndef NGROKTUNNELSERVICE_H
#define NGROKTUNNELSERVICE_H

#include <QObject>
#include <QString>

class QProcess;
class QTimer;

/**
 * @brief Manages an ngrok tunnel that exposes the local streaming server
 *        (Icecast on port 8888 by default) through a public URL that
 *        listeners can use from anywhere — no router/NAT setup needed.
 *
 * The user's ngrok authtoken is handed straight to the ngrok CLI
 * ("ngrok config add-authtoken"), which persists it in ngrok's own
 * configuration file. XFB deliberately never stores the token itself.
 *
 * The tunnel is started with "ngrok http <port>" using JSON logs on
 * stdout; the public URL is parsed from the "started tunnel" log line.
 */
class NgrokTunnelService : public QObject
{
    Q_OBJECT

public:
    explicit NgrokTunnelService(QObject *parent = nullptr);
    ~NgrokTunnelService() override;

    /** Locate the ngrok executable (PATH plus common install dirs). */
    static QString ngrokExecutable();
    static bool available() { return !ngrokExecutable().isEmpty(); }

    bool isRunning() const;
    QString publicUrl() const { return m_publicUrl; }

public slots:
    /**
     * Stores the user's authtoken via "ngrok config add-authtoken".
     * Emits authTokenConfigured() with the outcome.
     */
    void configureAuthToken(const QString &token);

    /** Start a tunnel to http://localhost:<localPort>. */
    void start(int localPort);

    /** Stop the tunnel (no-op when not running). */
    void stop();

signals:
    void authTokenConfigured(bool ok, const QString &message);
    void tunnelStarted(const QString &publicUrl);
    void tunnelStopped();
    void tunnelError(const QString &message);

private:
    void handleLogLine(const QByteArray &line);
    void failAndStop(const QString &message);
    void recordError(const QString &text);

    QProcess *m_proc = nullptr;
    QTimer *m_startTimeout = nullptr;
    QString m_publicUrl;
    QString m_lastErrorText;  // best error output seen, for exit diagnostics
    int m_lastErrorScore = 0; // relevance of m_lastErrorText (higher wins)
    bool m_announcedStart = false;
};

#endif // NGROKTUNNELSERVICE_H
