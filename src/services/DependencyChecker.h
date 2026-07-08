#ifndef DEPENDENCYCHECKER_H
#define DEPENDENCYCHECKER_H

#include <QObject>
#include <QStringList>

class QWidget;

struct DependencyInfo {
    QString name;           // human-readable name
    QString executable;     // binary name to check (e.g. "aria2c")
    QString brewPackage;    // Homebrew package name (macOS)
    QString aptPackage;     // apt package name (Debian/Ubuntu)
    QString pacmanPackage;  // pacman package name (Arch)
    QString wingetPackage;  // winget package name (Windows)
    bool required;          // true = block without it, false = optional
};

class DependencyChecker : public QObject
{
    Q_OBJECT

public:
    explicit DependencyChecker(QObject *parent = nullptr);

    // Check all dependencies, returns list of missing ones
    QList<DependencyInfo> checkDependencies();

    // Install missing dependencies (returns true if all installed successfully)
    bool installMissing(const QList<DependencyInfo> &missing);

    // Convenience: check and auto-install, returns list of still-missing deps
    QList<DependencyInfo> checkAndInstall();

    // Check if a single executable is available
    static bool isAvailable(const QString &executable);

    // Look up the catalog entry for an executable. If the executable is not in
    // the known catalog, a generic entry is returned that uses the executable
    // name as the package name for every package manager.
    DependencyInfo lookup(const QString &executable) const;

    // Interactive, consent-based install. If 'executable' is already available
    // this returns true immediately. Otherwise it explains to the user WHY the
    // feature needs the tool (via 'reason'), asks for permission, and — only on
    // agreement — installs it while showing live progress. Returns true if the
    // tool is available afterwards.
    bool ensureDependency(const QString &executable, const QString &reason, QWidget *parent);

    // Like ensureDependency but for a feature that can use any one of several
    // interchangeable tools (e.g. aria2c OR transmission-cli). If none are
    // present, the user is offered the first (preferred) one in the list.
    bool ensureAnyOf(const QStringList &executables, const QString &reason, QWidget *parent);

    // Provision yt-dlp as a self-updating standalone binary in the user's
    // ~/.local/bin (no root required), so it can keep itself current via
    // "yt-dlp -U". YouTube changes frequently and distro-packaged yt-dlp is
    // often too old. Prompts for consent and shows download progress. Returns
    // the path to a usable yt-dlp (the local binary if provisioned, otherwise a
    // system one if present), or an empty string if none is available.
    QString ensureYtDlp(QWidget *parent);

    // Absolute path where the self-updating yt-dlp binary is/should be stored.
    static QString localYtDlpPath();

    // macOS only: make sure Homebrew (the package manager XFB uses to install
    // tools like FFmpeg) is present. If it isn't, prompt for consent and launch
    // the official Homebrew installer in Terminal.app, then re-check. Returns
    // true if Homebrew is available afterwards. On non-macOS platforms this is
    // a no-op that returns false.
    bool ensureHomebrew(QWidget *parent);

signals:
    void dependencyInstalling(const QString &name);
    void dependencyInstalled(const QString &name);
    void dependencyFailed(const QString &name, const QString &error);

private:
    QList<DependencyInfo> m_dependencies;
    QString detectPackageManager();
    // Absolute path to the Homebrew binary if installed, otherwise empty.
    static QString brewPath();
    QString friendlyManagerName(const QString &pm) const;
    // Returns {program, arg0, arg1, ...} to install 'package' with 'pm', or empty.
    QStringList installCommand(const QString &pm, const QString &package) const;
    // Runs an install command with a live-progress dialog. Returns true on success.
    bool runInstallWithProgress(const DependencyInfo &dep, QWidget *parent);
    bool installWithBrew(const QString &package);
    bool installWithApt(const QString &package);
    bool installWithPacman(const QString &package);
    bool installWithWinget(const QString &package);
};

#endif // DEPENDENCYCHECKER_H
