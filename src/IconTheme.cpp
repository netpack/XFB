#include "IconTheme.h"
#include "ThemeManager.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QHash>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVariant>
#include <QWidget>

#include <cmath>

namespace
{
// The property a widget's untouched artwork is parked in, so every re-theme
// derives from the original rather than from the previous derivation.
const char *kOriginalIcon = "xfbOriginalIcon";

QString configFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/**
 * How much ink a derived icon puts down.
 *
 * `floor` is the coverage the palest part of the artwork keeps — small, so
 * that the white inside of a speech bubble stays a bubble instead of turning
 * into a lozenge — and `gamma` bends everything between that and full ink:
 * below 1 it pushes the mid-tones towards opaque, which is what makes the
 * high-contrast theme carry across a room.
 */
struct Recipe
{
    double floorCoverage;
    double gamma;
};

/**
 * Derived themes only ("classic" never reaches here), and the floor depends
 * on which way round the theme is. On a light theme the pale parts of an
 * icon can simply be left to the background, which is pale too; on a dark
 * one they cannot, or a white-bodied icon comes out as an outline with a
 * hole in it — so there the artwork keeps a wash of ink under it.
 */
Recipe recipeFor(const QString &id, bool lightInk)
{
    if (id == QLatin1String("contrast"))
        return {lightInk ? 0.22 : 0.00, 0.45};   // mid-tones near-solid ink
    return {lightInk ? 0.30 : 0.10, 0.85};       // line art, lightly shaded
}

// State picked up by reload().
QString s_theme = QStringLiteral("classic");
QColor s_inkColor;
QHash<QString, QIcon> s_cache;

// Every icon this file has derived, keyed by the cache key of the result and
// holding the artwork it came from. It is what lets retheme() park a real
// original even when the icon it finds on a widget was already a derived one
// — an icon handed to insertTab() by icon(), say, on a tab XFB adds after
// the window has been re-themed once. Cleared with everything else in
// reload(), because a derived icon from the previous theme is of no use.
QHash<qint64, QIcon> s_artworkOf;

/**
 * The artwork an object was given before any icon theme touched it.
 *
 * Parked on first sight — under "classic" too, so that a later switch away
 * from it still has the artwork to work from — and handed back on every
 * later call. Deriving from this rather than from whatever the object is
 * showing is what stops repeated re-themes from grinding an icon down.
 */
QIcon artworkBehind(const QIcon &icon)
{
    QIcon artwork = icon;
    // Bounded rather than recursive: one hop is all a well-behaved icon
    // needs, and a loop here would be a hang rather than a wrong icon.
    for (int hop = 0; hop < 4; ++hop) {
        const auto found = s_artworkOf.constFind(artwork.cacheKey());
        if (found == s_artworkOf.constEnd())
            break;
        artwork = found.value();
    }
    return artwork;
}

QIcon parkOriginal(QObject *object, const QIcon &current)
{
    const QVariant parked = object->property(kOriginalIcon);
    if (parked.isValid())
        return parked.value<QIcon>();
    const QIcon artwork = artworkBehind(current);
    if (!artwork.isNull())
        object->setProperty(kOriginalIcon, QVariant::fromValue(artwork));
    return artwork;
}

/** The colour a derived icon is drawn in, under the current colour theme. */
QColor inkColor()
{
    if (s_inkColor.isValid())
        return s_inkColor;
    if (s_theme == QLatin1String("accent"))
        s_inkColor = ThemeManager::currentAccent();
    else if (qApp)
        s_inkColor = qApp->palette().color(QPalette::WindowText);
    if (!s_inkColor.isValid())
        s_inkColor = ThemeManager::currentIsDark() ? QColor(Qt::white) : QColor(Qt::black);
    return s_inkColor;
}

/**
 * The luminance range the artwork actually occupies, as [darkest, lightest].
 *
 * Taken at the 2nd and 98th percentile rather than as a plain min/max so a
 * couple of stray pixels cannot decide the whole icon's contrast, and
 * weighted by alpha so a soft drop shadow counts for less than the drawing.
 */
bool luminanceRange(const QImage &image, int *darkest, int *lightest)
{
    quint64 histogram[256] = {0};
    quint64 total = 0;
    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const int alpha = qAlpha(line[x]);
            if (alpha == 0)
                continue;
            histogram[qGray(line[x])] += alpha;
            total += alpha;
        }
    }
    if (total == 0)
        return false;

    const quint64 cut = total / 50;   // 2% off each end
    quint64 seen = 0;
    *darkest = 0;
    for (int i = 0; i < 256; ++i) {
        seen += histogram[i];
        if (seen > cut) { *darkest = i; break; }
    }
    seen = 0;
    *lightest = 255;
    for (int i = 255; i >= 0; --i) {
        seen += histogram[i];
        if (seen > cut) { *lightest = i; break; }
    }
    return true;
}

/**
 * Re-draws one image in the current icon theme: every pixel becomes the
 * theme's ink, and the artwork's own shading survives as how solidly that
 * ink is laid down — its darkest tone opaque, its lightest barely there.
 *
 * The scale is the icon's own, not an absolute one, so a pale flat icon and
 * a dark engraved one both come out with the same weight; and artwork with
 * no shading to speak of (a flat play triangle) is drawn as a silhouette
 * rather than being stretched over a range it does not have.
 */
QImage themedImage(const QImage &source)
{
    const QColor ink = inkColor();
    const Recipe recipe = recipeFor(s_theme, ink.lightness() > 128);
    const int inkR = ink.red(), inkG = ink.green(), inkB = ink.blue();

    QImage out = source.convertToFormat(QImage::Format_ARGB32);
    int darkest = 0, lightest = 255;
    const bool haveRange = luminanceRange(out, &darkest, &lightest);
    // Below this the artwork is one flat tone; there is no shading to keep.
    const bool flat = !haveRange || (lightest - darkest) < 30;
    const double span = flat ? 1.0 : double(lightest - darkest);

    for (int y = 0; y < out.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(out.scanLine(y));
        for (int x = 0; x < out.width(); ++x) {
            const int alpha = qAlpha(line[x]);
            if (alpha == 0) {
                line[x] = qRgba(0, 0, 0, 0);
                continue;
            }
            double coverage = 1.0;
            if (!flat) {
                const double depth = qBound(0.0, (lightest - qGray(line[x])) / span, 1.0);
                coverage = recipe.floorCoverage
                           + (1.0 - recipe.floorCoverage) * std::pow(depth, recipe.gamma);
            }
            line[x] = qRgba(inkR, inkG, inkB,
                            qBound(0, qRound(alpha * coverage), 255));
        }
    }
    return out;
}

QPixmap themedPixmap(const QPixmap &source)
{
    if (source.isNull())
        return source;
    QPixmap out = QPixmap::fromImage(themedImage(source.toImage()));
    out.setDevicePixelRatio(source.devicePixelRatio());
    return out;
}
} // namespace

QStringList IconTheme::themeIds()
{
    return {QStringLiteral("classic"), QStringLiteral("mono"),
            QStringLiteral("accent"), QStringLiteral("contrast")};
}

QString IconTheme::themeName(const QString &id)
{
    if (id == QLatin1String("classic"))  return QObject::tr("Classic (full colour)");
    if (id == QLatin1String("mono"))     return QObject::tr("Monochrome");
    if (id == QLatin1String("accent"))   return QObject::tr("Accent tinted");
    if (id == QLatin1String("contrast")) return QObject::tr("High contrast");
    return id;
}

QString IconTheme::configuredTheme()
{
    QSettings settings(configFilePath(), QSettings::IniFormat);
    const QString id = settings.value(QStringLiteral("IconTheme")).toString().trimmed().toLower();
    return themeIds().contains(id) ? id : QStringLiteral("classic");
}

void IconTheme::reload()
{
    s_theme = configuredTheme();
    s_inkColor = QColor();   // re-resolved from the palette on first use
    s_cache.clear();
    s_artworkOf.clear();
}

QString IconTheme::currentTheme()
{
    return s_theme;
}

QIcon IconTheme::derive(const QIcon &source)
{
    if (s_theme == QLatin1String("classic") || source.isNull())
        return source;

    // availableSizes() reports the sizes an icon really carries, so a
    // one-pixmap resource icon — which is nearly all of XFB's — costs
    // exactly one conversion here. An icon that reports none draws itself
    // at any size it is asked for (a scalable engine): there is no pixmap
    // to re-draw, so it is handed back as it came.
    const QList<QSize> sizes = source.availableSizes();
    if (sizes.isEmpty())
        return source;

    QIcon out;
    for (const QSize &size : sizes)
        out.addPixmap(themedPixmap(source.pixmap(size)));
    s_artworkOf.insert(out.cacheKey(), source);
    return out;
}

QIcon IconTheme::icon(const QString &resourcePath)
{
    if (s_theme == QLatin1String("classic"))
        return QIcon(resourcePath);

    const auto cached = s_cache.constFind(resourcePath);
    if (cached != s_cache.constEnd())
        return cached.value();

    const QIcon themed = derive(QIcon(resourcePath));
    s_cache.insert(resourcePath, themed);
    return themed;
}

QIcon IconTheme::icon(const QIcon &source)
{
    return derive(source);
}

QIcon IconTheme::preview(const QString &themeId, const QString &resourcePath)
{
    const QString wasTheme = s_theme;
    const QColor wasInk = s_inkColor;
    s_theme = themeIds().contains(themeId) ? themeId : QStringLiteral("classic");
    s_inkColor = QColor();          // re-resolved for this theme's ink
    const QIcon preview = derive(QIcon(resourcePath));
    s_theme = wasTheme;
    s_inkColor = wasInk;
    return preview;
}

void IconTheme::retheme(QWidget *root)
{
    if (!root)
        return;

    const auto buttons = root->findChildren<QAbstractButton *>();
    for (QAbstractButton *button : buttons) {
        const QIcon source = parkOriginal(button, button->icon());
        if (!source.isNull())
            button->setIcon(derive(source));
    }

    const auto actions = root->findChildren<QAction *>();
    for (QAction *action : actions) {
        const QIcon source = parkOriginal(action, action->icon());
        if (!source.isNull())
            action->setIcon(derive(source));
    }

    // A tab icon belongs to the tab bar rather than to a QObject of its own,
    // so the original is parked on the page the tab shows. Parking it there
    // rather than in a list on the tab widget is what keeps a re-theme honest
    // across the tabs XFB adds and removes as options change (the FX tab, the
    // Pads tab, Torrents): the page carries its own artwork wherever it ends
    // up, and no tab is ever re-derived from another tab's icon.
    const auto tabWidgets = root->findChildren<QTabWidget *>();
    for (QTabWidget *tabs : tabWidgets) {
        for (int i = 0; i < tabs->count(); ++i) {
            QWidget *page = tabs->widget(i);
            if (!page)
                continue;
            const QIcon source = parkOriginal(page, tabs->tabIcon(i));
            if (!source.isNull())
                tabs->setTabIcon(i, derive(source));
        }
    }
}
