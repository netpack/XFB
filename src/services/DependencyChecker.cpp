#include "DependencyChecker.h"
#include "ErrorHandler.h"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QOperatingSystemVersion>
#include <QMessageBox>
#include <QProgressDialog>
#include <QEventLoop>
#include <QWidget>
#include <QApplication>
#include <QFile>

DependencyChecker::DependencyChecker(QObject *parent)
    : QObject(parent)
{
    // Define all external dependencies XFB can use. These are installed
    // on-demand (with the user's consent) the first time a feature needs them,
    // never automatically at startup.
    //          name                 executable          brew                apt                 pacman              winget                    required
    m_dependencies = {
        {"aria2",            "aria2c",           "aria2",            "aria2",            "aria2",            "aria2.aria2",            false},
        {"Tor",              "tor",              "tor",              "tor",              "tor",              "TorProject.TorBrowser",  false},
        {"transmission-cli", "transmission-cli", "transmission-cli", "transmission-cli", "transmission-cli", "",                       false},
        {"SoundConverter",   "soundconverter",   "",                 "soundconverter",   "soundconverter",   "",                       false},
        {"Audacity",         "audacity",         "audacity",         "audacity",         "audacity",         "Audacity.Audacity",      false},
        {"yt-dlp",           "yt-dlp",           "yt-dlp",           "yt-dlp",           "yt-dlp",           "yt-dlp.yt-dlp",          false},
        {"FFmpeg",           "ffmpeg",           "ffmpeg",           "ffmpeg",           "ffmpeg",           "Gyan.FFmpeg",            false},
        // ExifTool: used to read media duration/metadata. The apt and pacman
        // package names differ from the executable name.
        {"ExifTool",         "exiftool",         "exiftool",         "libimage-exiftool-perl", "perl-image-exiftool", "OliverBetz.ExifTool", false},
        // Node.js: JavaScript runtime used by yt-dlp for YouTube extraction.
        // deno is yt-dlp's default but is not available in apt; node is and is
        // fully supported via "--js-runtimes node".
        {"Node.js",          "node",             "node",             "nodejs",           "nodejs",           "OpenJS.NodeJS",          false},
    };
}

bool DependencyChecker::isAvailable(const QString &executable)
{
#ifdef Q_OS_WIN
    // On Windows, check common install locations and PATH
    QString exeWithExt = executable.endsWith(".exe") ? executable : executable + ".exe";
    QStringList paths = {
        QCoreApplication::applicationDirPath() + "/" + exeWithExt,
        QDir::homePath() + "/AppData/Local/Programs/" + executable + "/" + exeWithExt,
        "C:/Program Files/" + executable + "/" + exeWithExt,
        "C:/Program Files (x86)/" + executable + "/" + exeWithExt,
    };
    for (const QString &p : paths) {
        if (QFileInfo::exists(p)) return true;
    }
#else
    // Unix: check well-known paths first
    QStringList paths = {
        "/opt/homebrew/bin/" + executable,
        "/usr/local/bin/" + executable,
        "/usr/bin/" + executable,
    };
    for (const QString &p : paths) {
        if (QFileInfo::exists(p)) return true;
    }
#ifdef Q_OS_MACOS
    // GUI tools installed as Homebrew casks (e.g. Audacity) ship as an app
    // bundle, not a binary on PATH. Use the same lookup that
    // player::launchExternalApplication uses to start them via "open -a".
    const QString appBundle = executable.left(1).toUpper() + executable.mid(1) + ".app";
    if (QFileInfo::exists("/Applications/" + appBundle) ||
        QFileInfo::exists(QDir::homePath() + "/Applications/" + appBundle)) {
        return true;
    }
#endif
#endif
    // Fallback to PATH
    return !QStandardPaths::findExecutable(executable).isEmpty();
}

QList<DependencyInfo> DependencyChecker::checkDependencies()
{
    QList<DependencyInfo> missing;
    for (const DependencyInfo &dep : m_dependencies) {
        if (!isAvailable(dep.executable)) {
            missing.append(dep);
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "DependencyChecker",
                             QString("Missing dependency: %1 (%2)").arg(dep.name, dep.executable));
        }
    }
    return missing;
}

QString DependencyChecker::brewPath()
{
    const QStringList candidates = {"/opt/homebrew/bin/brew", "/usr/local/bin/brew"};
    for (const QString &c : candidates) {
        if (QFileInfo::exists(c)) {
            return c;
        }
    }
    return {};
}

QString DependencyChecker::detectPackageManager()
{
#ifdef Q_OS_WIN
    if (!QStandardPaths::findExecutable("winget").isEmpty()) return "winget";
    if (!QStandardPaths::findExecutable("choco").isEmpty()) return "choco";
#endif
#ifdef Q_OS_MACOS
    if (!brewPath().isEmpty()) {
        return "brew";
    }
#endif
#ifdef Q_OS_LINUX
    if (!QStandardPaths::findExecutable("pacman").isEmpty()) return "pacman";
    if (!QStandardPaths::findExecutable("apt-get").isEmpty()) return "apt";
#endif
    return {};
}

bool DependencyChecker::installWithBrew(const QString &package)
{
    QProcess proc;
    const QString brew = brewPath();
    if (brew.isEmpty()) {
        return false;
    }
    proc.start(brew, {"install", package});
    proc.waitForFinished(300000); // 5 min timeout
    return proc.exitCode() == 0;
}

bool DependencyChecker::installWithApt(const QString &package)
{
    QProcess proc;
    // Use pkexec for graphical privilege escalation (no terminal needed)
    proc.start("pkexec", {"apt-get", "install", "-y", package});
    proc.waitForFinished(300000);
    return proc.exitCode() == 0;
}

bool DependencyChecker::installWithPacman(const QString &package)
{
    QProcess proc;
    proc.start("pkexec", {"pacman", "-S", "--noconfirm", package});
    proc.waitForFinished(300000);
    return proc.exitCode() == 0;
}

bool DependencyChecker::installWithWinget(const QString &package)
{
    QProcess proc;
    proc.start("winget", {"install", "--accept-package-agreements", "--accept-source-agreements", "-e", "--id", package});
    proc.waitForFinished(300000);
    return proc.exitCode() == 0;
}

bool DependencyChecker::installMissing(const QList<DependencyInfo> &missing)
{
    if (missing.isEmpty()) return true;

    QString pm = detectPackageManager();
    if (pm.isEmpty()) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "DependencyChecker",
                         "No supported package manager found — cannot auto-install dependencies");
        return false;
    }

    bool allOk = true;
    for (const DependencyInfo &dep : missing) {
        emit dependencyInstalling(dep.name);

        QString pkg;
        bool ok = false;
        if (pm == "brew") {
            pkg = dep.brewPackage;
            ok = installWithBrew(pkg);
        } else if (pm == "apt") {
            pkg = dep.aptPackage;
            ok = installWithApt(pkg);
        } else if (pm == "pacman") {
            pkg = dep.pacmanPackage;
            ok = installWithPacman(pkg);
        } else if (pm == "winget" || pm == "choco") {
            pkg = dep.wingetPackage;
            if (!pkg.isEmpty()) {
                ok = installWithWinget(pkg);
            }
        }

        if (ok) {
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "DependencyChecker",
                             QString("Successfully installed %1").arg(dep.name));
            emit dependencyInstalled(dep.name);
        } else {
            ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "DependencyChecker",
                             QString("Failed to install %1 (%2)").arg(dep.name, pkg));
            emit dependencyFailed(dep.name, QString("Failed to install %1 via %2").arg(pkg, pm));
            allOk = false;
        }
    }
    return allOk;
}

QList<DependencyInfo> DependencyChecker::checkAndInstall()
{
    QList<DependencyInfo> missing = checkDependencies();
    if (!missing.isEmpty()) {
        installMissing(missing);
        // Re-check after install attempt
        missing = checkDependencies();
    }
    return missing;
}

void DependencyChecker::installAllInteractive(QWidget *parent)
{
    QList<DependencyInfo> missing = checkDependencies();
    if (missing.isEmpty()) {
        QMessageBox::information(parent, QObject::tr("Dependencies"),
            QObject::tr("All external tools XFB can use are already installed."));
        return;
    }

    QString pm = detectPackageManager();

#ifdef Q_OS_MACOS
    // Installing anything on macOS goes through Homebrew; offer to bootstrap it
    // first if it isn't there yet.
    if (pm.isEmpty()) {
        if (ensureHomebrew(parent)) {
            pm = detectPackageManager();
        }
    }
#endif

    if (pm.isEmpty()) {
        QStringList names;
        for (const DependencyInfo &dep : missing) names << dep.name;
        QMessageBox::warning(parent, QObject::tr("Cannot Install Dependencies"),
            QObject::tr("No supported package manager was found, so XFB cannot install "
                        "these tools for you:\n\n%1\n\nPlease install them manually.")
                .arg(names.join(", ")));
        return;
    }

    // On the chosen package manager some catalog entries may have no package
    // (e.g. transmission-cli on winget); list them separately so the summary
    // is honest about what will actually be installed.
    QList<DependencyInfo> installable;
    QStringList unavailable;
    for (const DependencyInfo &dep : missing) {
        // The self-updating yt-dlp in ~/.local/bin (see ensureYtDlp) is not on
        // the paths isAvailable() checks; if it's provisioned, yt-dlp isn't
        // actually missing.
        if (dep.executable == "yt-dlp" && QFileInfo(localYtDlpPath()).isExecutable()) {
            continue;
        }
        QString pkg;
        if (pm == "brew")        pkg = dep.brewPackage;
        else if (pm == "apt")    pkg = dep.aptPackage;
        else if (pm == "pacman") pkg = dep.pacmanPackage;
        else if (pm == "winget" || pm == "choco") pkg = dep.wingetPackage;
        if (pkg.isEmpty()) {
            unavailable << dep.name;
        } else {
            installable.append(dep);
        }
    }

    if (installable.isEmpty() && unavailable.isEmpty()) {
        QMessageBox::information(parent, QObject::tr("Dependencies"),
            QObject::tr("All external tools XFB can use are already installed."));
        return;
    }

    if (installable.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("Cannot Install Dependencies"),
            QObject::tr("None of the missing tools (%1) can be installed automatically "
                        "with %2. Please install them manually.")
                .arg(unavailable.join(", "), friendlyManagerName(pm)));
        return;
    }

    QStringList installNames;
    for (const DependencyInfo &dep : installable) installNames << dep.name;
    QString question = QObject::tr("The following optional tools are not installed:\n\n%1\n\n"
                                   "XFB can install them now using %2. This may ask for your "
                                   "password and take several minutes.\n\nInstall them all now?")
                           .arg(installNames.join(", "), friendlyManagerName(pm));
    if (!unavailable.isEmpty()) {
        question += QObject::tr("\n\n(Not available via %1 and skipped: %2)")
                        .arg(friendlyManagerName(pm), unavailable.join(", "));
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(parent,
        QObject::tr("Install All Dependencies?"), question,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return;
    }

    QStringList installed, failed;
    for (const DependencyInfo &dep : installable) {
        if (runInstallWithProgress(dep, parent) && isAvailable(dep.executable)) {
            installed << dep.name;
        } else {
            failed << dep.name;
        }
    }

    QString summary;
    if (!installed.isEmpty()) {
        summary += QObject::tr("Installed: %1").arg(installed.join(", "));
    }
    if (!failed.isEmpty()) {
        if (!summary.isEmpty()) summary += "\n\n";
        summary += QObject::tr("Failed or canceled: %1").arg(failed.join(", "));
    }
    if (!unavailable.isEmpty()) {
        if (!summary.isEmpty()) summary += "\n\n";
        summary += QObject::tr("Not available via %1: %2")
                       .arg(friendlyManagerName(pm), unavailable.join(", "));
    }

    if (failed.isEmpty() && unavailable.isEmpty()) {
        QMessageBox::information(parent, QObject::tr("Dependencies Installed"), summary);
    } else {
        QMessageBox::warning(parent, QObject::tr("Dependencies"), summary);
    }
}

DependencyInfo DependencyChecker::lookup(const QString &executable) const
{
    for (const DependencyInfo &dep : m_dependencies) {
        if (dep.executable == executable) {
            return dep;
        }
    }
    // Unknown tool: use the executable name as the package name everywhere.
    DependencyInfo generic;
    generic.name = executable;
    generic.executable = executable;
    generic.brewPackage = executable;
    generic.aptPackage = executable;
    generic.pacmanPackage = executable;
    generic.wingetPackage = executable;
    generic.required = false;
    return generic;
}

QString DependencyChecker::friendlyManagerName(const QString &pm) const
{
    if (pm == "brew")   return "Homebrew";
    if (pm == "apt")    return "APT (apt-get)";
    if (pm == "pacman") return "pacman";
    if (pm == "winget") return "winget";
    if (pm == "choco")  return "Chocolatey";
    return pm;
}

QStringList DependencyChecker::installCommand(const QString &pm, const QString &package) const
{
    if (package.isEmpty()) {
        return {};
    }
    if (pm == "brew") {
        const QString brew = brewPath();
        return {brew, "install", package};
    }
    if (pm == "apt") {
        // Refresh package lists first, then install — all under a single pkexec
        // prompt. Without an "apt-get update", installs can fail on systems with
        // stale or missing package indexes (a common cause of the package not
        // being found). Package names come from our trusted catalog.
        return {"pkexec", "sh", "-c",
                QString("apt-get update && apt-get install -y %1").arg(package)};
    }
    if (pm == "pacman") {
        // -Sy refreshes the package databases before installing.
        return {"pkexec", "pacman", "-Sy", "--noconfirm", package};
    }
    if (pm == "winget" || pm == "choco") {
        return {"winget", "install", "--accept-package-agreements",
                "--accept-source-agreements", "-e", "--id", package};
    }
    return {};
}

bool DependencyChecker::runInstallWithProgress(const DependencyInfo &dep, QWidget *parent)
{
    const QString pm = detectPackageManager();

    QString pkg;
    if (pm == "brew")        pkg = dep.brewPackage;
    else if (pm == "apt")    pkg = dep.aptPackage;
    else if (pm == "pacman") pkg = dep.pacmanPackage;
    else if (pm == "winget" || pm == "choco") pkg = dep.wingetPackage;

    const QStringList cmd = installCommand(pm, pkg);
    if (cmd.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("Cannot Install %1").arg(dep.name),
            QObject::tr("XFB could not determine how to install \"%1\" automatically on this "
                        "system. Please install it manually using your package manager.")
                .arg(dep.name));
        return false;
    }

    emit dependencyInstalling(dep.name);

    QProgressDialog progress(parent);
    progress.setWindowTitle(QObject::tr("Installing %1").arg(dep.name));
    progress.setLabelText(QObject::tr("Installing %1 via %2…\n\nYou may be asked for your "
                                      "password to authorize the installation.")
                              .arg(dep.name, friendlyManagerName(pm)));
    progress.setMinimum(0);
    progress.setMaximum(0); // busy indicator
    progress.setMinimumWidth(460);
    progress.setWindowModality(Qt::WindowModal);
    progress.setAutoClose(false);
    progress.setAutoReset(false);

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    QEventLoop loop;
    bool canceled = false;

    QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&proc, &QProcess::errorOccurred, &loop, &QEventLoop::quit);

    QObject::connect(&proc, &QProcess::readyReadStandardOutput, &proc, [&]() {
        const QString chunk = QString::fromUtf8(proc.readAllStandardOutput());
        const QStringList lines = chunk.split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            // Show the most recent line of output so the user sees progress.
            progress.setLabelText(QObject::tr("Installing %1 via %2…\n\n%3")
                                      .arg(dep.name, friendlyManagerName(pm),
                                           lines.last().trimmed()));
        }
    });

    QObject::connect(&progress, &QProgressDialog::canceled, &proc, [&]() {
        canceled = true;
        proc.kill();
    });

    proc.start(cmd.first(), cmd.mid(1));
    if (!proc.waitForStarted(15000)) {
        QMessageBox::warning(parent, QObject::tr("Installation Failed"),
            QObject::tr("Could not start the installer for \"%1\".\n\nMake sure \"%2\" is "
                        "available on your system.").arg(dep.name, cmd.first()));
        emit dependencyFailed(dep.name, QObject::tr("Failed to start installer"));
        return false;
    }

    loop.exec(); // pumps events; keeps the progress dialog responsive

    const bool ok = (!canceled &&
                     proc.exitStatus() == QProcess::NormalExit &&
                     proc.exitCode() == 0);

    progress.reset();

    if (ok) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "DependencyChecker",
                                 QString("Successfully installed %1").arg(dep.name));
        emit dependencyInstalled(dep.name);
    } else if (canceled) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "DependencyChecker",
                                 QString("Installation of %1 canceled by user").arg(dep.name));
        emit dependencyFailed(dep.name, QObject::tr("Installation canceled"));
    } else {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Warning, "DependencyChecker",
                                 QString("Failed to install %1 (%2)").arg(dep.name, pkg));
        emit dependencyFailed(dep.name, QObject::tr("Installation failed"));
    }
    return ok;
}

bool DependencyChecker::ensureDependency(const QString &executable, const QString &reason,
                                         QWidget *parent)
{
    if (isAvailable(executable)) {
        return true;
    }

    const DependencyInfo dep = lookup(executable);
    QString pm = detectPackageManager();

#ifdef Q_OS_MACOS
    // On macOS the package manager is Homebrew. If it isn't installed yet, offer
    // to bootstrap it first so the dependency install below can proceed.
    if (pm.isEmpty()) {
        if (ensureHomebrew(parent)) {
            pm = detectPackageManager();
        }
    }
#endif

    if (pm.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("%1 Required").arg(dep.name),
            QObject::tr("%1\n\nThis feature needs \"%2\", which is not installed. No supported "
                        "package manager was found, so XFB cannot install it for you. Please "
                        "install \"%2\" manually and try again.")
                .arg(reason, dep.name));
        return false;
    }

    const QMessageBox::StandardButton reply = QMessageBox::question(parent,
        QObject::tr("Install %1?").arg(dep.name),
        QObject::tr("%1\n\nTo do this, XFB needs to install \"%2\" using %3. "
                    "This requires your permission and may ask for your password.\n\n"
                    "Install it now?")
            .arg(reason, dep.name, friendlyManagerName(pm)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (reply != QMessageBox::Yes) {
        return false;
    }

    runInstallWithProgress(dep, parent);

    // Re-check: the package metadata name may differ from the executable.
    const bool nowAvailable = isAvailable(executable);
    if (!nowAvailable) {
        QMessageBox::warning(parent, QObject::tr("%1 Not Available").arg(dep.name),
            QObject::tr("\"%1\" still doesn't appear to be installed. The feature that needs it "
                        "is unavailable for now.").arg(dep.name));
    }
    return nowAvailable;
}

bool DependencyChecker::ensureAnyOf(const QStringList &executables, const QString &reason,
                                    QWidget *parent)
{
    if (executables.isEmpty()) {
        return false;
    }

    // Already satisfied if any one of the alternatives is present.
    for (const QString &exe : executables) {
        if (isAvailable(exe)) {
            return true;
        }
    }

    // Offer the first (preferred) alternative.
    return ensureDependency(executables.first(), reason, parent);
}


bool DependencyChecker::ensureHomebrew(QWidget *parent)
{
#ifdef Q_OS_MACOS
    // Already installed?
    if (!brewPath().isEmpty()) {
        return true;
    }

    const auto reply = QMessageBox::question(parent,
        QObject::tr("Install Homebrew?"),
        QObject::tr("XFB uses Homebrew (the macOS package manager) to install the tools it "
                    "needs, such as FFmpeg, but Homebrew isn't installed yet.\n\n"
                    "XFB can launch the official Homebrew installer in the Terminal. You may be "
                    "asked for your password, and Apple's Command Line Tools may be installed "
                    "first. This can take several minutes.\n\nInstall Homebrew now?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return false;
    }

    emit dependencyInstalling("Homebrew");

    // The official installer must run as the normal user (it refuses to run
    // under sudo) and needs an interactive terminal: it prompts for the admin
    // password via sudo and may trigger Apple's Command Line Tools installer.
    // A GUI app has no controlling terminal, so we launch it in Terminal.app
    // via AppleScript and let the user complete those prompts there.
    const QString installCmd =
        "/bin/bash -c \"$(curl -fsSL "
        "https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"";

    // Escape the command for embedding inside an AppleScript double-quoted
    // string (backslashes first, then double quotes).
    QString escaped = installCmd;
    escaped.replace("\\", "\\\\").replace("\"", "\\\"");
    const QString appleScript = QString(
        "tell application \"Terminal\"\n"
        "  activate\n"
        "  do script \"%1\"\n"
        "end tell").arg(escaped);

    QProcess osa;
    osa.start("osascript", {"-e", appleScript});
    const bool launched = osa.waitForStarted(10000) && osa.waitForFinished(15000);
    if (!launched) {
        QMessageBox::warning(parent, QObject::tr("Could Not Launch Installer"),
            QObject::tr("XFB couldn't open the Terminal to install Homebrew. Please install it "
                        "manually from https://brew.sh and try again."));
        emit dependencyFailed("Homebrew", QObject::tr("Failed to launch installer"));
        return false;
    }

    // The Terminal install runs independently of XFB, so we can't reliably block
    // on it. Ask the user to confirm once it has finished, then re-check.
    QMessageBox::information(parent, QObject::tr("Completing Homebrew Installation"),
        QObject::tr("Homebrew is installing in the Terminal window that just opened.\n\n"
                    "Follow any prompts there (you may need to enter your password and press "
                    "Return). When the Terminal reports the installation is complete, click OK "
                    "here to continue."));

    if (!brewPath().isEmpty()) {
        ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "DependencyChecker",
                                 "Homebrew installed successfully");
        emit dependencyInstalled("Homebrew");
        return true;
    }

    QMessageBox::warning(parent, QObject::tr("Homebrew Not Detected"),
        QObject::tr("Homebrew still doesn't appear to be installed. If the Terminal is still "
                    "working, wait for it to finish and try again."));
    emit dependencyFailed("Homebrew", QObject::tr("Homebrew not detected after installation"));
    return false;
#else
    Q_UNUSED(parent);
    return false;
#endif
}

QString DependencyChecker::localYtDlpPath()
{
#ifdef Q_OS_WIN
    // Windows uses the standalone yt-dlp.exe (bundles Python).
    return QDir::homePath() + "/.local/bin/yt-dlp.exe";
#else
    return QDir::homePath() + "/.local/bin/yt-dlp";
#endif
}

QString DependencyChecker::ensureYtDlp(QWidget *parent)
{
    const QString target = localYtDlpPath();

    // Already provisioned as a self-updating local binary?
    if (QFileInfo(target).isExecutable()) {
        return target;
    }

    // Is there a system yt-dlp (e.g. from a package manager) we could fall back
    // to? It works, but cannot self-update, so we still offer the local binary.
    QString systemYtDlp = QStandardPaths::findExecutable("yt-dlp");

    const QString msg = systemYtDlp.isEmpty()
        ? QObject::tr("Downloading audio from an external source uses yt-dlp, which isn't "
                      "installed yet. XFB can install the official yt-dlp into your home "
                      "folder:\n\n%1\n\nInstalled there, it can keep itself up to date "
                      "(YouTube changes often, and an outdated yt-dlp fails). No administrator "
                      "password is needed.\n\nInstall it now?").arg(target)
        : QObject::tr("A system yt-dlp was found, but it can't update itself and may be too old "
                      "for current YouTube changes. XFB can install the official, self-updating "
                      "yt-dlp into your home folder:\n\n%1\n\nNo administrator password is "
                      "needed.\n\nInstall it now?").arg(target);

    const auto reply = QMessageBox::question(parent, QObject::tr("Install yt-dlp?"), msg,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) {
        return systemYtDlp; // may be empty
    }

    // Make sure the destination directory exists.
    QDir().mkpath(QFileInfo(target).absolutePath());

    // Pick a downloader (no root required).
    // Use the platform-appropriate release asset:
    //  - macOS: "yt-dlp_macos" is a standalone binary that bundles Python, so
    //    it needs no separate Python install.
    //  - Windows: "yt-dlp.exe" is a standalone binary that also bundles Python
    //    (the x64 build runs under emulation on Windows ARM64).
    //  - Linux/other: the "yt-dlp" zipapp (small) which uses the system python3
    //    (a package dependency of XFB on Debian-based systems).
#if defined(Q_OS_MAC)
    const QString assetName = "yt-dlp_macos";
#elif defined(Q_OS_WIN)
    const QString assetName = "yt-dlp.exe";
#else
    const QString assetName = "yt-dlp";
#endif
    const QString url =
        "https://github.com/yt-dlp/yt-dlp/releases/latest/download/" + assetName;
    QString tool = QStandardPaths::findExecutable("curl");
    QStringList args;
    if (!tool.isEmpty()) {
        args = {"-L", "--fail", "--retry", "2", "-o", target, url};
    } else {
        tool = QStandardPaths::findExecutable("wget");
        if (!tool.isEmpty()) {
            args = {"-O", target, url};
        }
    }
    if (tool.isEmpty()) {
        QMessageBox::warning(parent, QObject::tr("Cannot Download yt-dlp"),
            QObject::tr("Neither curl nor wget is available to download yt-dlp. Please install "
                        "one of them and try again, or install yt-dlp manually."));
        return systemYtDlp;
    }

    emit dependencyInstalling("yt-dlp");

    QProgressDialog progress(parent);
    progress.setWindowTitle(QObject::tr("Installing yt-dlp"));
    progress.setLabelText(QObject::tr("Downloading the latest yt-dlp to:\n%1").arg(target));
    progress.setMinimum(0);
    progress.setMaximum(0); // busy indicator
    progress.setMinimumWidth(460);
    progress.setWindowModality(Qt::WindowModal);
    progress.setAutoClose(false);
    progress.setAutoReset(false);

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    QEventLoop loop;
    bool canceled = false;
    QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&proc, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    QObject::connect(&progress, &QProgressDialog::canceled, &proc, [&]() {
        canceled = true;
        proc.kill();
    });

    proc.start(tool, args);
    if (!proc.waitForStarted(15000)) {
        QMessageBox::warning(parent, QObject::tr("Installation Failed"),
            QObject::tr("Could not start the download of yt-dlp."));
        emit dependencyFailed("yt-dlp", QObject::tr("Failed to start downloader"));
        return systemYtDlp;
    }
    loop.exec();

    const bool downloaded = (!canceled &&
                             proc.exitStatus() == QProcess::NormalExit &&
                             proc.exitCode() == 0 &&
                             QFileInfo(target).exists() &&
                             QFileInfo(target).size() > 0);
    progress.reset();

    if (!downloaded) {
        QFile::remove(target); // clean up any partial file
        if (!canceled) {
            QMessageBox::warning(parent, QObject::tr("Installation Failed"),
                QObject::tr("Downloading yt-dlp did not complete successfully."));
            emit dependencyFailed("yt-dlp", QObject::tr("Download failed"));
        }
        return systemYtDlp;
    }

    // Make the downloaded file executable (chmod 0755).
    QFile::setPermissions(target,
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
        QFile::ReadGroup | QFile::ExeGroup |
        QFile::ReadOther | QFile::ExeOther);

    // Verify it runs. The standalone "yt-dlp" zipapp requires python3 at runtime.
    QProcess verify;
    verify.start(target, {"--version"});
    const bool runs = verify.waitForStarted(10000) && verify.waitForFinished(20000) &&
                      verify.exitStatus() == QProcess::NormalExit && verify.exitCode() == 0;
    if (!runs) {
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        QMessageBox::warning(parent, QObject::tr("yt-dlp Could Not Run"),
            QObject::tr("yt-dlp was downloaded to %1 but could not be run. The download may be "
                        "incomplete or blocked by the system. Please try again.").arg(target));
#else
        QMessageBox::warning(parent, QObject::tr("yt-dlp Needs Python"),
            QObject::tr("yt-dlp was downloaded to %1 but could not be run. It requires Python 3 "
                        "to be installed. Please install Python 3 (e.g. the \"python3\" package) "
                        "and try again.").arg(target));
#endif
        // Keep the binary; it'll work once the runtime requirement is met.
        return systemYtDlp.isEmpty() ? target : systemYtDlp;
    }

    ErrorHandler::logMessage(ErrorHandler::ErrorSeverity::Info, "DependencyChecker",
                             QString("Installed self-updating yt-dlp at %1").arg(target));
    emit dependencyInstalled("yt-dlp");
    return target;
}
