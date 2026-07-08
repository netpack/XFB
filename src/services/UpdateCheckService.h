#ifndef UPDATECHECKSERVICE_H
#define UPDATECHECKSERVICE_H

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;

/**
 * @brief Checks GitHub releases for a newer XFB version.
 *
 * Queries https://api.github.com/repos/netpack/XFB/releases/latest and
 * compares the tag against the running version. When a newer release is
 * found, updateAvailable() carries the version, the release page URL and
 * the direct download URL of the artifact matching this platform
 * (.dmg on macOS, .deb on Linux, .exe on Windows) when one is attached
 * to the release.
 */
class UpdateCheckService : public QObject
{
    Q_OBJECT

public:
    explicit UpdateCheckService(QNetworkAccessManager *network, QObject *parent = nullptr);

    /**
     * Check for a new release.
     * @param manual true when the user asked explicitly: upToDate() and
     *        checkFailed() are then emitted too, so the UI can respond.
     *        Automatic (startup/periodic) checks only ever emit
     *        updateAvailable().
     */
    void checkNow(bool manual);

    /** Version of the running build (QCoreApplication::applicationVersion). */
    static QString currentVersion();

    /** True when a is a strictly newer version than b (handles "v" prefix). */
    static bool isNewerVersion(const QString &a, const QString &b);

signals:
    void updateAvailable(const QString &version, const QUrl &releasePage,
                         const QUrl &downloadUrl, const QString &releaseNotes);
    void upToDate(const QString &currentVersion);
    void checkFailed(const QString &error);

private:
    QNetworkAccessManager *m_network = nullptr;
    bool m_requestRunning = false;
};

#endif // UPDATECHECKSERVICE_H
