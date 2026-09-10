#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>

class QApplication;

/**
 * Application-wide theme support.
 *
 * A theme is a small set of colors (window, base, text, accent, ...) from
 * which both the QPalette and the application stylesheet are generated, so
 * every theme styles every widget consistently — including the dockable
 * panels of the main window.
 *
 * Available themes: "system" (follows the OS light/dark preference),
 * "light", "dark", "midnight" (deep blue), "studio" (near-black with amber
 * accents), "hacker" (phosphor green on black) and "nature" (paper, moss
 * and bark, the second light theme). The accent color of any theme can be
 * overridden.
 *
 * Settings (xfb.conf):
 *   Theme       — theme id; when absent, migrated from the legacy DarkMode
 *   AccentColor — "#rrggbb" accent override; empty uses the theme's own
 *   NowPlayingClockFont — QFont::toString() of the elapsed-time clock on the
 *                 player panel; empty means the one XFB ships with
 *
 * The legacy DarkMode bool is still written back on apply() so older code
 * paths (and the Windows installer defaults) keep working.
 */
class ThemeManager
{
public:
    /** All selectable theme ids, in presentation order. */
    static QStringList themeIds();

    /** Human-readable name for a theme id (translated). */
    static QString themeName(const QString &id);

    /** Reads Theme/AccentColor from xfb.conf and applies palette + QSS. */
    static void apply(QApplication *app);

    /** Whether the theme applied last (or the one configured) is dark. */
    static bool currentIsDark();

    /** The currently configured theme id (after legacy migration). */
    static QString configuredTheme();

    /** The configured accent override; invalid QColor when unset. */
    static QColor configuredAccent();

    /** Default accent color of a theme (for the options dialog preview). */
    static QColor themeAccent(const QString &id);

    /**
     * The accent color actually in force: the override when one is set,
     * otherwise the current theme's own. Cached by apply(), so a widget
     * may call it from paintEvent() without touching xfb.conf; before
     * apply() has run it resolves from the settings once.
     *
     * Custom painting should take its highlight colors from here (or from
     * QPalette::Highlight, which apply() fills from the same value) rather
     * than hardcoding one, or it goes illegible in midnight and studio.
     */
    static QColor currentAccent();

    /** A color that reads over the current theme's item-view background,
     *  derived from `base` — for markers and overlays on custom widgets. */
    static QColor contrastingInk(const QColor &wanted);

    // --- the now-playing elapsed-time clock -------------------------------
    //
    // The "0:01:23 of 0:03:45" line under the programme name. Its typeface is
    // the operator's to choose (Options -> Appearance), because the desk it is
    // read from may be a metre away. Only the size is not entirely theirs: the
    // player panel gives the clock one row between the programme name and the
    // sliders, so a size past what that row can grow to would push the clock
    // over the transport.

    /** The clock as XFB draws it out of the box: the application typeface,
     *  bold, at 14 pt — the font the player panel was designed with. */
    static QFont defaultNowPlayingClockFont();

    /** The clock the operator chose, or the default when they chose none.
     *  Always within the size range the player panel can show. */
    static QFont nowPlayingClockFont();

    /** `font` held to the size range the player panel can show. */
    static QFont clampNowPlayingClockFont(QFont font);

    /** The xfb.conf key the two above read — written by the options dialog. */
    static const char *nowPlayingClockFontKey();

    /** Smallest and largest clock size the player panel can lay out. */
    static constexpr int kClockFontMinPt = 8;
    static constexpr int kClockFontMaxPt = 28;

private:
    struct Spec;
    static Spec resolveSpec(const QString &id, const QColor &accentOverride);
    static QString buildStyleSheet(const Spec &spec);
    static bool systemPrefersDark();
};

#endif // THEMEMANAGER_H
