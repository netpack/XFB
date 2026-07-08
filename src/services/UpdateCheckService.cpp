#include "UpdateCheckService.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>

namespace
{
const char kLatestReleaseUrl[] = "https://api.github.com/repos/netpack/XFB/releases/latest";

QString platformAssetSuffix()
{
#if defined(Q_OS_MAC)
    return QStringLiteral(".dmg");
#elif defined(Q_OS_WIN)
    return QStringLiteral(".exe");
#else
    return QStringLiteral(".deb");
#endif
}
} // namespace

UpdateCheckService::UpdateCheckService(QNetworkAccessManager *network, QObject *parent)
    : QObject(parent)
    , m_network(network)
{
}

QString UpdateCheckService::currentVersion()
{
    return QCoreApplication::applicationVersion();
}

bool UpdateCheckService::isNewerVersion(const QString &a, const QString &b)
{
    auto normalize = [](QString v) {
        v = v.trimmed();
        if (v.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            v.remove(0, 1);
        return v;
    };

    const QVersionNumber va = QVersionNumber::fromString(normalize(a));
    const QVersionNumber vb = QVersionNumber::fromString(normalize(b));
    if (va.isNull() || vb.isNull())
        return false; // unparseable tags never trigger a notification
    return QVersionNumber::compare(va, vb) > 0;
}

void UpdateCheckService::checkNow(bool manual)
{
    if (!m_network) {
        if (manual)
            emit checkFailed(tr("No network manager available."));
        return;
    }
    if (m_requestRunning)
        return;
    m_requestRunning = true;

    QNetworkRequest request{QUrl(QString::fromLatin1(kLatestReleaseUrl))};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("XFB/%1 (update check)").arg(currentVersion()));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manual]() {
        m_requestRunning = false;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Update check failed:" << reply->errorString();
            if (manual)
                emit checkFailed(reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            if (manual)
                emit checkFailed(tr("Unexpected response from GitHub."));
            return;
        }

        const QJsonObject release = doc.object();
        const QString tag = release.value(QStringLiteral("tag_name")).toString();
        const QUrl releasePage(release.value(QStringLiteral("html_url")).toString());
        const QString notes = release.value(QStringLiteral("body")).toString();

        if (tag.isEmpty()) {
            if (manual)
                emit checkFailed(tr("No releases found."));
            return;
        }

        if (!isNewerVersion(tag, currentVersion())) {
            qInfo() << "Update check: running" << currentVersion()
                    << "- latest release is" << tag << "(up to date)";
            if (manual)
                emit upToDate(currentVersion());
            return;
        }

        // Pick the artifact for this platform, if the release ships one
        QUrl downloadUrl;
        const QString suffix = platformAssetSuffix();
        const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue &assetVal : assets) {
            const QJsonObject asset = assetVal.toObject();
            const QString name = asset.value(QStringLiteral("name")).toString();
            if (name.endsWith(suffix, Qt::CaseInsensitive)) {
                downloadUrl = QUrl(asset.value(QStringLiteral("browser_download_url")).toString());
                break;
            }
        }

        qInfo() << "Update check: new version available:" << tag
                << "asset:" << downloadUrl.toDisplayString();
        emit updateAvailable(tag, releasePage, downloadUrl, notes);
    });
}
