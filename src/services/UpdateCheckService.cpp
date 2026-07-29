#include "UpdateCheckService.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSysInfo>

namespace
{
const char kLatestReleaseUrl[] = "https://api.github.com/repos/netpack/XFB/releases/latest";

// True when a release asset is the right download for this platform AND this
// CPU architecture.
//
// Matching on the file extension alone was actively harmful: GitHub returns
// assets sorted case-insensitively by name, so "XFB-<ver>-arm64-Setup.exe"
// precedes "XFB-<ver>-Setup.exe" and every x64 Windows machine was handed the
// ARM64 installer. Because both installers use the same $PROGRAMFILES64
// directory, running it replaced a working x64 install with ARM64 binaries and
// Windows then refused to start the app ("This app can't run on your PC").
// The Linux .deb assets have the same hazard: amd64 is listed before arm64.
bool assetMatchesPlatform(const QString &name)
{
    const QString lower = name.toLower();

    // QSysInfo reports the architecture of the running build, which is what
    // matters here — an x86 build under emulation must keep getting x86.
    const QString arch = QSysInfo::buildCpuArchitecture();
    const bool wantArm64 = arch.contains(QLatin1String("arm"))
                           || arch.contains(QLatin1String("aarch"));
    const bool assetIsArm64 = lower.contains(QLatin1String("arm64"))
                              || lower.contains(QLatin1String("aarch64"));

#if defined(Q_OS_MAC)
    // A single universal/arm64 dmg is published, so the extension is enough.
    return lower.endsWith(QLatin1String(".dmg"));
#elif defined(Q_OS_WIN)
    if (!lower.endsWith(QLatin1String(".exe")))
        return false;
    return wantArm64 == assetIsArm64;
#else
    if (!lower.endsWith(QLatin1String(".deb")))
        return false;
    return wantArm64 == assetIsArm64;
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

// Compares one dot-separated segment of a version as an arbitrary-precision
// decimal integer: shorter (once leading zeros are gone) is smaller, and equal
// lengths compare digit by digit. Returns <0, 0 or >0 like strcmp.
//
// This exists because XFB's versions are digits of pi and the fractional
// segment outgrows a 32-bit int: "3.14159265358" has 14159265358 there, past
// INT_MAX. QVersionNumber, which parses segments as int, silently gave up at
// that point and returned plain "3" — so it read every 11-decimal release as
// OLDER than the 3.141592653 before it, and the updater told everybody they
// were already up to date. Comparing the digits as text has no such ceiling.
static int compareSegment(QStringView a, QStringView b)
{
    while (a.size() > 1 && a.startsWith(QLatin1Char('0')))
        a = a.mid(1);
    while (b.size() > 1 && b.startsWith(QLatin1Char('0')))
        b = b.mid(1);
    if (a.size() != b.size())
        return a.size() < b.size() ? -1 : 1;
    return a.compare(b);
}

bool UpdateCheckService::isNewerVersion(const QString &a, const QString &b)
{
    auto normalize = [](QString v) {
        v = v.trimmed();
        if (v.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            v.remove(0, 1);
        return v;
    };

    const QString na = normalize(a);
    const QString nb = normalize(b);

    // Digits and dots only, and never an empty or dot-terminated string:
    // anything else is a tag shape we do not understand, and an unparseable
    // tag must never trigger a notification.
    auto wellFormed = [](const QString &v) {
        if (v.isEmpty() || v.startsWith(QLatin1Char('.')) || v.endsWith(QLatin1Char('.')))
            return false;
        bool lastWasDot = false;
        for (const QChar c : v) {
            if (c == QLatin1Char('.')) {
                if (lastWasDot)
                    return false; // ".." is not a segment separator
                lastWasDot = true;
            } else if (!c.isDigit()) {
                return false;
            } else {
                lastWasDot = false;
            }
        }
        return true;
    };
    if (!wellFormed(na) || !wellFormed(nb))
        return false;

    const QList<QStringView> sa = QStringView(na).split(QLatin1Char('.'));
    const QList<QStringView> sb = QStringView(nb).split(QLatin1Char('.'));

    for (qsizetype i = 0; i < qMax(sa.size(), sb.size()); ++i) {
        // A missing trailing segment counts as 0, so 3.14 == 3.14.0.
        const QStringView xa = i < sa.size() ? sa.at(i) : QStringView(u"0");
        const QStringView xb = i < sb.size() ? sb.at(i) : QStringView(u"0");
        const int c = compareSegment(xa, xb);
        if (c != 0)
            return c > 0;
    }
    return false; // identical
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

        // Pick the artifact for this platform and architecture, if the release
        // ships one. Leaving downloadUrl empty is safe: the update dialog then
        // sends the user to the release page instead of installing anything.
        QUrl downloadUrl;
        const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
        for (const QJsonValue &assetVal : assets) {
            const QJsonObject asset = assetVal.toObject();
            const QString name = asset.value(QStringLiteral("name")).toString();
            if (assetMatchesPlatform(name)) {
                downloadUrl = QUrl(asset.value(QStringLiteral("browser_download_url")).toString());
                qInfo() << "Update check: selected asset" << name
                        << "for" << QSysInfo::buildCpuArchitecture();
                break;
            }
        }
        if (downloadUrl.isEmpty()) {
            qWarning() << "Update check: no asset matches this platform/architecture ("
                       << QSysInfo::buildCpuArchitecture() << ") - will open the release page";
        }

        qInfo() << "Update check: new version available:" << tag
                << "asset:" << downloadUrl.toDisplayString();
        emit updateAvailable(tag, releasePage, downloadUrl, notes);
    });
}
