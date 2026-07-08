#include "NgrokTunnelService.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

namespace
{
QString findNgrok()
{
    QString path = QStandardPaths::findExecutable(QStringLiteral("ngrok"));
    if (!path.isEmpty())
        return path;

    // GUI apps launched from Finder/desktop launchers often have a minimal
    // PATH that misses Homebrew / local installs — check the usual places.
    const QStringList extraDirs = {
        QStringLiteral("/opt/homebrew/bin"), QStringLiteral("/usr/local/bin"),
        QStringLiteral("/opt/local/bin"), QStringLiteral("/usr/bin"),
        QStringLiteral("/snap/bin")
    };
    return QStandardPaths::findExecutable(QStringLiteral("ngrok"), extraDirs);
}
} // namespace

NgrokTunnelService::NgrokTunnelService(QObject *parent)
    : QObject(parent)
{
}

NgrokTunnelService::~NgrokTunnelService()
{
    stop();
}

QString NgrokTunnelService::ngrokExecutable()
{
    static const QString path = findNgrok();
    return path;
}

bool NgrokTunnelService::isRunning() const
{
    return m_proc && m_proc->state() != QProcess::NotRunning;
}

void NgrokTunnelService::configureAuthToken(const QString &token)
{
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        emit authTokenConfigured(false, tr("The authtoken is empty."));
        return;
    }

    const QString ngrok = ngrokExecutable();
    if (ngrok.isEmpty()) {
        emit authTokenConfigured(false, tr("ngrok executable not found."));
        return;
    }

    // The token is handed to ngrok's own config (never written by XFB)
    QProcess config;
    config.start(ngrok, {QStringLiteral("config"), QStringLiteral("add-authtoken"), trimmed});
    if (!config.waitForFinished(15000)) {
        config.kill();
        emit authTokenConfigured(false, tr("ngrok did not respond while saving the authtoken."));
        return;
    }

    if (config.exitStatus() == QProcess::NormalExit && config.exitCode() == 0) {
        // Note: ngrok only stores the token here; it is validated against
        // your account the first time a tunnel is started.
        qInfo() << "ngrok authtoken saved";
        emit authTokenConfigured(true, tr("Authtoken saved. Click Share to create your public link."));
    } else {
        const QString err = QString::fromLocal8Bit(config.readAllStandardError()).trimmed();
        emit authTokenConfigured(false, tr("ngrok rejected the authtoken: %1").arg(err));
    }
}

void NgrokTunnelService::start(int localPort)
{
    if (isRunning())
        stop();

    const QString ngrok = ngrokExecutable();
    if (ngrok.isEmpty()) {
        emit tunnelError(tr("ngrok executable not found."));
        return;
    }

    m_publicUrl.clear();
    m_lastErrorText.clear();
    m_lastErrorScore = 0;
    m_announcedStart = false;

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_proc, &QProcess::readyReadStandardOutput, this, [this]() {
        while (m_proc && m_proc->canReadLine())
            handleLogLine(m_proc->readLine());
    });

    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus status) {
        if (m_startTimeout)
            m_startTimeout->stop();
        const bool hadTunnel = m_announcedStart;
        m_publicUrl.clear();
        m_announcedStart = false;
        if (status != QProcess::NormalExit || exitCode != 0) {
            QString detail = m_lastErrorText.isEmpty()
                                 ? tr("ngrok exited unexpectedly (code %1)").arg(exitCode)
                                 : m_lastErrorText;
            emit tunnelError(detail);
        } else if (hadTunnel) {
            emit tunnelStopped();
        }
    });

    // JSON logs let us read the public URL straight from stdout without
    // relying on ngrok's local web API.
    m_proc->start(ngrok, {QStringLiteral("http"),
                          QString::number(localPort),
                          QStringLiteral("--log"), QStringLiteral("stdout"),
                          QStringLiteral("--log-format"), QStringLiteral("json")});

    if (!m_proc->waitForStarted(5000)) {
        failAndStop(tr("Failed to start ngrok."));
        return;
    }

    // Give the agent a generous window to establish the tunnel
    if (!m_startTimeout) {
        m_startTimeout = new QTimer(this);
        m_startTimeout->setSingleShot(true);
        connect(m_startTimeout, &QTimer::timeout, this, [this]() {
            if (!m_announcedStart)
                failAndStop(tr("Timed out waiting for the ngrok tunnel "
                               "(is your authtoken configured?)"));
        });
    }
    m_startTimeout->start(20000);
}

void NgrokTunnelService::stop()
{
    if (m_startTimeout)
        m_startTimeout->stop();

    if (!m_proc)
        return;

    QProcess *proc = m_proc;
    m_proc = nullptr;
    proc->disconnect(this);
    if (proc->state() != QProcess::NotRunning) {
        proc->terminate();
        if (!proc->waitForFinished(3000)) {
            proc->kill();
            proc->waitForFinished(1000);
        }
    }
    proc->deleteLater();

    const bool hadTunnel = m_announcedStart;
    m_publicUrl.clear();
    m_announcedStart = false;
    if (hadTunnel)
        emit tunnelStopped();
}

void NgrokTunnelService::recordError(const QString &text)
{
    // ngrok emits several error-ish lines (update-check noise, repeated
    // retries, a plain-text "ERROR:" banner). Keep the most useful one:
    // messages carrying an ERR_NGROK code beat generic failures, which
    // beat any other stray output.
    QString cleaned = text;
    cleaned.replace(QStringLiteral("\r"), QString());
    cleaned = cleaned.trimmed();
    if (cleaned.isEmpty() || cleaned == QLatin1String("ERROR:"))
        return;

    int score = 1;
    if (cleaned.contains(QLatin1String("ERR_NGROK")))
        score = 3;
    else if (cleaned.contains(QLatin1String("authentication"), Qt::CaseInsensitive)
             || cleaned.contains(QLatin1String("failed"), Qt::CaseInsensitive))
        score = 2;

    if (score > m_lastErrorScore) {
        // First line carries the human-readable cause; keep it short for the UI
        m_lastErrorText = cleaned.section('\n', 0, 0).trimmed();
        m_lastErrorScore = score;
    }
}

void NgrokTunnelService::handleLogLine(const QByteArray &line)
{
    const QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) {
        // Non-JSON output (e.g. plain error banner) — keep it for diagnostics
        recordError(QString::fromLocal8Bit(line));
        return;
    }

    const QJsonObject obj = doc.object();
    const QString lvl = obj.value(QStringLiteral("lvl")).toString();
    const QString msg = obj.value(QStringLiteral("msg")).toString();
    const QString err = obj.value(QStringLiteral("err")).toString();

    if (msg == QLatin1String("started tunnel")) {
        m_publicUrl = obj.value(QStringLiteral("url")).toString();
        if (!m_publicUrl.isEmpty()) {
            m_announcedStart = true;
            if (m_startTimeout)
                m_startTimeout->stop();
            qInfo() << "ngrok tunnel started:" << m_publicUrl;
            emit tunnelStarted(m_publicUrl);
        }
        return;
    }

    // ngrok logs use logrus-style abbreviated levels ("eror", "crit")
    const bool isError = lvl.startsWith(QLatin1String("err"))
                         || lvl.startsWith(QLatin1String("eror"))
                         || lvl.startsWith(QLatin1String("crit"))
                         || lvl.startsWith(QLatin1String("fatal"));
    if (isError || (!err.isEmpty() && err != QLatin1String("<nil>"))) {
        const QString text = (err.isEmpty() || err == QLatin1String("<nil>")) ? msg : err;
        qWarning() << "ngrok:" << text;
        recordError(text);
    }
}

void NgrokTunnelService::failAndStop(const QString &message)
{
    qWarning() << "NgrokTunnelService:" << message;
    stop();
    emit tunnelError(message);
}
