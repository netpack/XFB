#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QColor>
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
 * "light", "dark", "midnight" (deep blue) and "studio" (near-black with
 * amber accents). The accent color of any theme can be overridden.
 *
 * Settings (xfb.conf):
 *   Theme       — theme id; when absent, migrated from the legacy DarkMode
 *   AccentColor — "#rrggbb" accent override; empty uses the theme's own
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

private:
    struct Spec;
    static Spec resolveSpec(const QString &id, const QColor &accentOverride);
    static QString buildStyleSheet(const Spec &spec);
    static bool systemPrefersDark();
};

#endif // THEMEMANAGER_H
