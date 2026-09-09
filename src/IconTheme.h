#ifndef ICONTHEME_H
#define ICONTHEME_H

#include <QIcon>
#include <QString>
#include <QStringList>

class QWidget;

/**
 * Application-wide icon style.
 *
 * XFB ships one set of artwork (the colourful `:/icons` set, plus the flat
 * `:/icons/flat` one), and an icon theme is a recipe for re-drawing it:
 * "classic" hands the artwork back untouched, the others derive a
 * monochrome, an accent-tinted or a high-contrast version of the very same
 * pixmaps. No second set of files to ship, and a station that runs the dark
 * or studio theme all day gets icons that belong to it.
 *
 * The derived themes read their ink from the palette ThemeManager applies,
 * so reload() has to run *after* ThemeManager::apply() — main.cpp does that
 * at startup and player.cpp's updateConfig() on every options change.
 *
 * Two ways in, and most code wants the first:
 *   icon(":/icons/x.png") — instead of QIcon(":/icons/x.png")
 *   retheme(widget)       — re-derives every icon already set on a widget
 *                           tree, for the icons that come from a .ui file
 *                           and for a live theme change
 *
 * retheme() keeps each widget's original icon in a property, so it always
 * derives from the artwork rather than from its own last output: switching
 * from high contrast back to classic gets the colours back, and no amount
 * of re-applying grinds an icon down.
 *
 * Settings (xfb.conf):
 *   IconTheme — one of themeIds(); absent or unknown reads as "classic"
 */
class IconTheme
{
public:
    /** All selectable icon-theme ids, in presentation order. */
    static QStringList themeIds();

    /** Human-readable name for an icon-theme id (translated). */
    static QString themeName(const QString &id);

    /** The icon theme configured in xfb.conf (never empty). */
    static QString configuredTheme();

    /** Re-reads the setting and drops the derived-icon cache. */
    static void reload();

    /** The theme reload() last picked up. */
    static QString currentTheme();

    /** The themed icon for a resource path. Cached. */
    static QIcon icon(const QString &resourcePath);

    /** The themed version of an icon that is already loaded. */
    static QIcon icon(const QIcon &source);

    /**
     * One resource drawn as some *other* icon theme would draw it, for the
     * options dialog: a theme nobody can picture from its name is a theme
     * nobody picks. Uses the colours of the theme currently in force, so it
     * previews the icons, not an unsaved colour change.
     */
    static QIcon preview(const QString &themeId, const QString &resourcePath);

    /**
     * Re-derives the icons of every button, tool button, action and tab in
     * `root`'s widget tree. Safe to call repeatedly.
     */
    static void retheme(QWidget *root);

private:
    static QIcon derive(const QIcon &source);
};

#endif // ICONTHEME_H
