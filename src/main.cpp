#include "player.h"
#include "IconTheme.h"
#include "ThemeManager.h"
#include "dialogs/SignInDialog.h"
#include "services/AccessControl.h"

#ifdef XFB_HAS_DARWIN_MIC_PERMISSION
#include <QtPlugin>
// Statically linked (see src/CMakeLists.txt), so it has to be imported by hand:
// nothing scans a plugin directory for this one.
Q_IMPORT_PLUGIN(QDarwinMicrophonePermissionPlugin)
#endif

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QSplashScreen>
#include <QPainter>
#include <QStyleFactory>
#include <QPalette>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QEvent>
#include <QFont>
#include <QTextEdit>
#include <QPlainTextEdit>
#include "services/TorrentTypes.h"

#include <csignal>

#ifdef Q_OS_MAC
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static const char* XFB_VERSION = "3.1423";

/**
 * Splash screen with readable text: paints a soft dark band behind the
 * message and renders it in white (the artwork is light, so the default
 * colored text was hard to read). The dedication (everything before the
 * final blank line) is drawn in italic, the status line below it.
 */
class XfbSplashScreen : public QSplashScreen
{
public:
    explicit XfbSplashScreen(const QPixmap &pixmap)
        : QSplashScreen(pixmap) {}

protected:
    void drawContents(QPainter *painter) override
    {
        const QString msg = message();
        if (msg.isEmpty())
            return;

        painter->setRenderHint(QPainter::Antialiasing);

        const int sep = msg.lastIndexOf(QStringLiteral("\n\n"));
        const QString dedication = sep >= 0 ? msg.left(sep) : QString();
        const QString status = sep >= 0 ? msg.mid(sep + 2) : msg;

        const QRect area = rect().adjusted(16, 0, -16, -12);
        const int flags = Qt::AlignHCenter | Qt::TextWordWrap;

        QFont dedicationFont = painter->font();
        dedicationFont.setItalic(true);
        const QFont statusFont = painter->font();

        const QRect dedicationRect = QFontMetrics(dedicationFont).boundingRect(area, flags, dedication);
        const QRect statusRect = QFontMetrics(statusFont).boundingRect(area, flags, status);

        const int spacing = dedication.isEmpty() ? 0 : 6;
        const int totalHeight = dedicationRect.height() + spacing + statusRect.height();
        int y = area.bottom() - totalHeight;

        // Contrast band behind the text
        const QRect band(area.left(), y - 8, area.width(), totalHeight + 16);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(20, 20, 30, 175));
        painter->drawRoundedRect(band, 10, 10);

        if (!dedication.isEmpty()) {
            painter->setFont(dedicationFont);
            painter->setPen(Qt::white);
            painter->drawText(QRect(area.left(), y, area.width(), dedicationRect.height()),
                              flags, dedication);
            y += dedicationRect.height() + spacing;
        }

        painter->setFont(statusFont);
        painter->setPen(QColor(190, 200, 230)); // status slightly dimmer than the dedication
        painter->drawText(QRect(area.left(), y, area.width(), statusRect.height()),
                          flags, status);
    }
};
static const char* XFB_CODENAME = "Pour Mr. De La Plume (Alain Bogaerts) pour l'Éternité.";

// Global pointer for signal handler cleanup
static player* g_mainWindow = nullptr;

// --------------------------------------------------------------- file log
//
// XFB is a GUI application, so on Windows nothing is attached to stdout and
// every qDebug()/qWarning() the app produces is lost — which is why crash
// reports from users arrive with nothing to go on. Mirror the messages into
// a file next to xfb.conf, flushed on every line so that whatever was said
// just before a crash survives it.
static QFile g_logFile;
static QMutex g_logMutex;
static QtMessageHandler g_previousMessageHandler = nullptr;

static void xfbMessageHandler(QtMsgType type, const QMessageLogContext &context,
                              const QString &msg)
{
    // Keep the console/IDE output the developer builds rely on
    if (g_previousMessageHandler) {
        g_previousMessageHandler(type, context, msg);
    } else {
        fputs(qPrintable(msg), stderr);
        fputc('\n', stderr);
    }

    const char *level = "INFO";
    switch (type) {
    case QtDebugMsg:    level = "DEBUG"; break;
    case QtInfoMsg:     level = "INFO";  break;
    case QtWarningMsg:  level = "WARN";  break;
    case QtCriticalMsg: level = "CRIT";  break;
    case QtFatalMsg:    level = "FATAL"; break;
    }

    QMutexLocker locker(&g_logMutex);
    if (!g_logFile.isOpen()) {
        return;
    }
    g_logFile.write(QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8());
    g_logFile.write(" [");
    g_logFile.write(level);
    g_logFile.write("] ");
    g_logFile.write(msg.toUtf8());
    g_logFile.write("\n");
    g_logFile.flush();
}

// Opens the log and installs the handler. Called once the application and
// organization names are set, so the path resolves to the folder that already
// holds xfb.conf — one place to ask a user for when something goes wrong.
static void setupFileLogging()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dir.isEmpty() || !QDir().mkpath(dir)) {
        return;
    }

    const QString path = dir + QStringLiteral("/xfb.log");

    // Retention. A log is only useful for the report someone is writing now;
    // months-old chatter is just disk. Two rules, applied at every start:
    //
    //  - Roll when the live log passes 5 MB, keeping one previous generation.
    //  - Delete any generation older than kLogRetentionDays, so a machine that
    //    never logs enough to trigger a roll doesn't keep last spring's log
    //    forever either.
    constexpr int kLogRetentionDays = 14;
    constexpr qint64 kLogRollBytes = 5 * 1024 * 1024;

    if (QFileInfo(path).size() > kLogRollBytes) {
        QFile::remove(path + QStringLiteral(".1"));
        QFile::rename(path, path + QStringLiteral(".1"));
    }

    // Sweep old logs, including the ones written by earlier XFB versions
    // (Logger's dated "xfb_YYYYMMDD_HHMMSS.log" files) which nothing else
    // cleans up any more.
    {
        const QDateTime cutoff =
            QDateTime::currentDateTime().addDays(-kLogRetentionDays);
        const QStringList patterns = {QStringLiteral("xfb.log.*"),
                                      QStringLiteral("xfb_*.log")};
        const QFileInfoList stale =
            QDir(dir).entryInfoList(patterns, QDir::Files);
        for (const QFileInfo &info : stale) {
            if (info.lastModified() < cutoff)
                QFile::remove(info.absoluteFilePath());
        }
        // The live log is rolled by size, but if it is simply ancient (the app
        // has not run in a long time) it starts fresh rather than appending to
        // something from another era.
        const QFileInfo current(path);
        if (current.exists() && current.lastModified() < cutoff)
            QFile::remove(path);
    }

    g_logFile.setFileName(path);
    if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }

    g_previousMessageHandler = qInstallMessageHandler(xfbMessageHandler);
    qInfo().noquote() << "--- XFB" << XFB_VERSION << "starting, logging to" << path;
}

// Unix signal handler — kill Tor process on SIGTERM/SIGINT so it doesn't linger
static void signalHandler(int sig)
{
    Q_UNUSED(sig);
    if (g_mainWindow) {
        // Can't do much in a signal handler, but QProcess::kill() is safe enough
        g_mainWindow = nullptr; // prevent double-cleanup
    }
    // Let Qt's default handler take over
    QCoreApplication::quit();
}

#ifdef Q_OS_WIN
// Windows console control handler for proper cleanup on force-quit
static BOOL WINAPI windowsConsoleHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (g_mainWindow) {
            g_mainWindow = nullptr;
        }
        QCoreApplication::quit();
        return TRUE;
    default:
        return FALSE;
    }
}
#endif

// Platform-specific plugin path setup (must happen before QApplication)
static void setupPlatformPluginPath(int /*argc*/, char *argv[])
{
#if defined(Q_OS_MAC)
    // macOS: Don't override QT_PLUGIN_PATH here. Instead, we add the bundle's
    // PlugIns directory as an additional library path after QApplication is created.
    // This way, both deployed bundles (with bundled frameworks) and dev builds
    // (using Homebrew Qt) can find their plugins.
    Q_UNUSED(argv);
#elif defined(Q_OS_WIN)
    // Windows: plugins are typically next to the executable
    QString executablePath = QString::fromLocal8Bit(argv[0]);
    QFileInfo execInfo(executablePath);
    QString pluginPath = execInfo.absolutePath() + "/plugins";
    if (QDir(pluginPath).exists()) {
        qputenv("QT_PLUGIN_PATH", pluginPath.toLocal8Bit());
    }
#else
    Q_UNUSED(argv);
    // Linux: Qt usually finds plugins via standard paths
#endif
}

// Platform-specific multimedia environment setup
static void setupMultimediaEnvironment()
{
#if defined(Q_OS_LINUX)
    // Qt 6 selects the multimedia backend via the QT_MEDIA_BACKEND environment
    // variable ("ffmpeg" or "gstreamer"). We do NOT force a backend here:
    // most Linux distributions (e.g. Ubuntu) ship ONLY the FFmpeg backend
    // plugin, and forcing "gstreamer" there leaves Qt with no backend at all
    // ("No QtMultimedia backends found"). If a user knows their system has the
    // GStreamer backend and wants it, they can set QT_MEDIA_BACKEND themselves.
    //
    // (The old Qt 5 variable QT_MULTIMEDIA_PREFERRED_PLUGINS is ignored by Qt 6
    // and has been removed.)
#elif defined(Q_OS_WIN)
    // Windows uses WMF by default, which is fine
#elif defined(Q_OS_MAC)
    // macOS uses AVFoundation by default, which is fine
#endif

    // Enable accessibility on all platforms
    qputenv("QT_ACCESSIBILITY", "1");
}

// Set the application icon based on platform
static void setupApplicationIcon(QApplication& app)
{
#if defined(Q_OS_MAC)
    // macOS: use the .icns from the app bundle Resources folder
    QString iconPath = QCoreApplication::applicationDirPath() + "/../Resources/XFB.icns";
    if (QFile::exists(iconPath)) {
        app.setWindowIcon(QIcon(iconPath));
    } else {
        app.setWindowIcon(QIcon(":/Resources/48x48.png"));
    }
#elif defined(Q_OS_WIN)
    // Windows: the .ico is typically embedded in the executable via .rc file
    // Fall back to the PNG resource
    app.setWindowIcon(QIcon(":/Resources/48x48.png"));
#else
    // Linux and others: use the PNG resource
    app.setWindowIcon(QIcon(":/Resources/48x48.png"));
#endif
}

// Theming lives in ThemeManager now: it reads Theme/AccentColor from
// xfb.conf (migrating the legacy DarkMode bool) and generates the palette
// and stylesheet for the selected theme.

// Ensure the config directory exists and copy default config if needed
static QString setupConfiguration(QApplication& app)
{
    Q_UNUSED(app);

    const QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if (writableConfigPath.isEmpty()) {
        qCritical() << "Could not determine writable config location";
        return {};
    }

    QDir configDir(writableConfigPath);
    if (!configDir.exists() && !configDir.mkpath(".")) {
        qCritical() << "Failed to create config directory:" << writableConfigPath;
        return {};
    }

    QString configFilePath = configDir.filePath(configFileName);
    QString resourceConfigPath = ":/" + configFileName;

    if (!QFile::exists(configFilePath)) {
        if (QFile::copy(resourceConfigPath, configFilePath)) {
            // Make the copied file writable
            QFile::setPermissions(configFilePath,
                QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
            qInfo() << "Copied default configuration to:" << configFilePath;
        } else {
            qWarning() << "Failed to copy default config from" << resourceConfigPath
                       << "to" << configFilePath;
            // Not fatal - the app can still run with built-in defaults
        }
    }

    return configFilePath;
}

// Apply the language/theme chosen in the Windows installer.
//
// The installer writes an "xfb-defaults.conf" next to XFB.exe capturing the
// user's choices. We can't rely on seeding these only when the user config is
// first created: a returning user (who already has an xfb.conf from a previous
// version) would never see the choices made in a fresh installer run — which is
// exactly the "I picked light + Portuguese but got dark + English" case.
//
// Instead we reconcile on every launch using a token built from the installer's
// choices. When the token differs from the one we last applied, we copy the
// Language/DarkMode values into the user config and record the token. This
// applies each new set of installer choices exactly once, so a user who later
// changes the theme in-app keeps their change (the token stays the same).
static void applyInstallerDefaults(QSettings& settings)
{
    const QString defaultsPath =
        QCoreApplication::applicationDirPath() + "/xfb-defaults.conf";
    if (!QFile::exists(defaultsPath)) {
        return; // No installer defaults (e.g. non-Windows, or portable run).
    }

    QSettings defaults(defaultsPath, QSettings::IniFormat);
    const QString lang = defaults.value("Language").toString().trimmed();
    const QString darkStr = defaults.value("DarkMode").toString().trimmed();
    if (lang.isEmpty() && darkStr.isEmpty()) {
        return;
    }

    const QString token = lang + "|" + darkStr;
    if (settings.value("InstallerDefaultsToken").toString() == token) {
        return; // Already applied this exact set of installer choices.
    }

    if (!lang.isEmpty()) {
        settings.setValue("Language", lang);
    }
    if (!darkStr.isEmpty()) {
        // Store a real bool so downstream .toBool() reads are unambiguous.
        const bool dark = darkStr.compare("true", Qt::CaseInsensitive) == 0;
        settings.setValue("DarkMode", dark);
        // The theme system supersedes DarkMode; map the installer choice.
        settings.setValue("Theme", dark ? QStringLiteral("dark")
                                        : QStringLiteral("light"));
    }
    settings.setValue("InstallerDefaultsToken", token);
    settings.sync();

    qInfo() << "Applied installer defaults — language:" << lang
            << "darkMode:" << darkStr;
}

// Load and install a translator for the given language code
static bool setupTranslator(QApplication& app, QTranslator& translator, const QString& language)
{
    if (language == "en" || language.isEmpty()) {
        return true; // English is the default, no translator needed
    }

    QString translationFile;
    if (language == "pt") {
        translationFile = ":/portugues.qm";
    } else if (language == "fr") {
        translationFile = ":/frances.qm";
    }

    if (!translationFile.isEmpty() && translator.load(translationFile)) {
        app.installTranslator(&translator);

        // Installing the catalogue translates the strings XFB wrote; it does
        // nothing to the ones Qt formats for us. QLocale defaults to the
        // *system* locale, which on a Mac set to English in Portugal is
        // en_PT — Portuguese number separators and English month names. So
        // "Registo de emissão" was printing "1 September 2026", on the very
        // reports a station hands to a client or a regulator. Every display
        // use of QLocale in XFB (the as-run and advertiser reports, the music
        // quota report, the hour-clock weekday names) wants the language the
        // operator chose, so say so once, here.
        //
        // Only when a catalogue was actually installed: leaving English on
        // the system locale is what every existing installation already does,
        // and a station running XFB in English in Portugal has been reading
        // its own region's number formats all along.
        QLocale::setDefault(language == "pt"
                                ? QLocale(QLocale::Portuguese, QLocale::Portugal)
                                : QLocale(QLocale::French, QLocale::France));

        qDebug() << "Installed translator for:" << language;
        return true;
    }

    qWarning() << "Failed to load translator for:" << language;
    return false;
}

// Frees the Tab key from multi-line text fields. By default a QTextEdit or
// QPlainTextEdit swallows Tab to insert a tab character, so a keyboard-only
// user who lands in the notes field of a dialog (Add music, Add jingle, Add
// advert, Add program, Options, ...) can never Tab back out to the OK button.
// Applied application-wide so it covers every dialog, including ones added
// later, and it is deliberately NOT conditional on accessibility being
// enabled: escaping a text field is correct behaviour for everyone. The cost
// is that a literal tab can no longer be typed into these fields, which no
// notes or description field in XFB has any use for.
class TabFocusEscapeFilter : public QObject
{
public:
    explicit TabFocusEscapeFilter(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Polish) {
            if (auto *te = qobject_cast<QTextEdit *>(obj)) {
                te->setTabChangesFocus(true);
            } else if (auto *pte = qobject_cast<QPlainTextEdit *>(obj)) {
                pte->setTabChangesFocus(true);
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

// Scales the font of every widget that has an explicitly-set point size (e.g.
// the sizes hardcoded in the .ui files) by a constant factor, so the user's
// font-size preference affects ALL text while preserving the relative size
// hierarchy (a 20 pt clock stays proportionally larger than 8 pt log text).
// Widgets without an explicit font simply inherit the scaled application font.
// Applied via an application-wide filter so it also covers dialogs and other
// windows created after startup. Each widget is scaled at most once.
class FontScaleFilter : public QObject
{
public:
    explicit FontScaleFilter(double scale, QObject *parent = nullptr)
        : QObject(parent), m_scale(scale) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Polish) {
            QWidget *w = qobject_cast<QWidget *>(obj);
            if (w && !w->property("xfbFontScaled").toBool()) {
                QFont f = w->font();
                // Only rescale a widget that set its own point size; inheriting
                // widgets already get the scaled application font.
                if (f.resolveMask() & QFont::SizeResolved) {
                    const double ps = f.pointSizeF();
                    if (ps > 0.0) {
                        f.setPointSizeF(ps * m_scale);
                        w->setFont(f);
                    }
                }
                w->setProperty("xfbFontScaled", true);
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    double m_scale;
};

int main(int argc, char *argv[])
{
    qDebug() << "XFB main() starting...";

    // 1. Platform setup (before QApplication)
    setupPlatformPluginPath(argc, argv);
    setupMultimediaEnvironment();

    // 2. Create QApplication
    QApplication::setDesktopSettingsAware(false);
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("XFB");
    // A second, separate XFB on the same machine. Everything XFB keeps —
    // settings, library, caches — hangs off the application name, so naming
    // the profile is all it takes to run one alongside another without the
    // two treading on each other. This is what makes it possible to try
    // Broadcast Redundancy (XFB → Broadcast Redundancy) with two instances on one desk
    // before there is a second machine to put the backup on:
    //     XFB_PROFILE=backup open -n /Applications/XFB.app
    if (qEnvironmentVariableIsSet("XFB_PROFILE")) {
        const QString profile = qEnvironmentVariable("XFB_PROFILE").trimmed();
        if (!profile.isEmpty())
            QCoreApplication::setApplicationName("XFB-" + profile);
    }
    QCoreApplication::setApplicationVersion(XFB_VERSION);
    QCoreApplication::setOrganizationName("Netpack - Online Solutions");

    // From here on every qDebug/qWarning also lands in xfb.log
    setupFileLogging();

#ifdef Q_OS_WIN
    // Make command-line tools bundled next to XFB.exe (e.g. ffmpeg.exe /
    // ffprobe.exe shipped in the install folder) discoverable everywhere:
    // QStandardPaths::findExecutable, QProcess with a bare program name, and
    // child processes such as yt-dlp all consult PATH. Prepend our own
    // directory so a bundled ffmpeg is used without any separate install.
    {
        const QString appDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        qputenv("PATH", appDir.toLocal8Bit() + ";" + qgetenv("PATH"));
    }
#endif

    // 3. Handle non-GUI command line options early
    QStringList args = app.arguments();
    if (args.contains("--version") || args.contains("-v")) {
        qDebug() << "XFB Version" << XFB_VERSION << "-" << XFB_CODENAME;
        return 0;
    }
    if (args.contains("--help") || args.contains("-h")) {
        qDebug() << "XFB - Radio Automation Software";
        qDebug() << "Usage: XFB [options]";
        qDebug() << "Options:";
        qDebug() << "  --version, -v    Show version information";
        qDebug() << "  --help, -h       Show this help";
        qDebug() << "  --minimal        Start in minimal mode";
        qDebug() << "  --no-dialogs     Disable popup dialogs";
        return 0;
    }

    // 4. Register metatypes for cross-thread signal/slot usage
    qRegisterMetaType<TorrentSearchResult>("TorrentSearchResult");
    qRegisterMetaType<QList<TorrentSearchResult>>("QList<TorrentSearchResult>");

    // 5. Platform-specific icon and style
    setupApplicationIcon(app);
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 6. Splash screen
    XfbSplashScreen splash(QPixmap("://images/splash.png"));

    // Dédicace — shown above the status line for the whole splash duration
    const QString splashDedication = QString::fromUtf8(
        "En souvenir du plus tendre et du plus beau des papas.\n"
        "Tu demeureras toujours en nos cœurs, Alain Bogaerts (1952-2026).");
    auto showSplashMessage = [&](const QString &status) {
        splash.showMessage(splashDedication + "\n\n" + status);
        app.processEvents();
    };

    splash.show();
    showSplashMessage(QObject::tr("Initializing..."));

    // 7. Configuration
    showSplashMessage(QObject::tr("Loading configuration..."));

    QString configFilePath = setupConfiguration(app);
    if (configFilePath.isEmpty()) {
        splash.hide();
        QMessageBox::critical(nullptr,
            QObject::tr("Configuration Error"),
            QObject::tr("Cannot find or create configuration directory."));
        return 1;
    }

    QSettings settings(configFilePath, QSettings::IniFormat);

    // Apply language/theme chosen during Windows setup (once per install) before
    // reading the effective values below.
    applyInstallerDefaults(settings);

    QString language = settings.value("Language", "en").toString();
    bool fullScreen = settings.value("FullScreen", false).toBool();

    // Apply the configured application font size before building any widgets so
    // the whole UI inherits it. 0/unset keeps the platform default. Widgets
    // with hardcoded point sizes (from the .ui files) are scaled by the same
    // factor via FontScaleFilter so the setting affects every text in the app.
    // The reference base is 10 pt: at size 10 the UI looks as designed, larger
    // values scale everything up proportionally.
    {
        const int fontSize = settings.value("FontSize", 0).toInt();
        if (fontSize > 0) {
            QFont appFont = app.font();
            appFont.setPointSize(fontSize);
            app.setFont(appFont);

            const double scale = static_cast<double>(fontSize) / 10.0;
            if (!qFuzzyCompare(scale, 1.0)) {
                app.installEventFilter(new FontScaleFilter(scale, &app));
            }
        }
    }

    // Let Tab leave multi-line text fields everywhere in the application.
    app.installEventFilter(new TabFocusEscapeFilter(&app));

    // 8. Translation
    QTranslator translator;
    setupTranslator(app, translator, language);

    // 9. Theme
    showSplashMessage(QObject::tr("Applying theme..."));
    ThemeManager::apply(&app);
    // The derived icon themes take their ink from the palette the line above
    // installs, so this reads the setting only once the theme is in force.
    IconTheme::reload();

    // 9b. Who is at the desk
    //
    // On an installation nobody has protected there are no accounts and this
    // does nothing at all — which is every station that has just updated into
    // this version. Where there are accounts, XFB asks before it builds the
    // window: refusing the question has to leave nothing behind, and by the
    // time the player exists it has already opened the database, armed the
    // watched folders and possibly gone on air.
    //
    // The splash is taken down first. A modal question behind a frameless
    // always-on-top splash screen is a machine that looks hung.
    {
        AccessControl &access = AccessControl::instance();
        if (access.isProtected()) {
            if (access.signInRequired()) {
                splash.hide();
                if (!SignInDialog::ask(SignInDialog::Mode::Startup))
                    return 0;
                splash.show();
                showSplashMessage(QObject::tr("Loading main window..."));
            } else {
                // Asked not to be asked: come up as the account the
                // administrator named, or as the one that can do least.
                access.signInAutomatically();
            }
        }
    }

    // 10. Create main window
    showSplashMessage(QObject::tr("Loading main window..."));

    player mainWindow;
    g_mainWindow = &mainWindow;

    // Install signal handlers so Tor gets cleaned up on kill/Ctrl+C
#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(windowsConsoleHandler, TRUE);
#else
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#endif

    // Ensure Tor is stopped when the application quits (covers force-quit scenarios
    // where the destructor might not run)
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&mainWindow]() {
        qDebug() << "Application quitting, cleaning up...";
        g_mainWindow = nullptr; // prevent signal handler from using stale pointer
#ifdef Q_OS_MAC
        // Force immediate exit to prevent crash during Qt's widget teardown.
        // The player::closeEvent should have already called _exit(0), but if
        // we reach here (e.g. Cmd+Q or programmatic quit), force exit now.
        _exit(0);
#endif
    });

    // 11. Show and run
    showSplashMessage(QObject::tr("XFB is Ready!"));
    // Leave the dedication readable for a moment before the window appears
    QThread::msleep(1200);

    if (fullScreen) {
        mainWindow.showFullScreen();
    } else {
        mainWindow.show();
    }

    splash.finish(&mainWindow);

    return app.exec();
}
