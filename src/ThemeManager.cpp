#include "ThemeManager.h"

#include <QApplication>
#include <QDebug>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>

namespace
{
// Resolved darkness of the theme applied last (queried by player.cpp for
// its own light/dark-dependent accents, e.g. the Play button green).
bool s_currentIsDark = false;

QString configFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}
} // namespace

/** The full color recipe of one theme. */
struct ThemeManager::Spec
{
    bool dark = false;
    QColor window;      // window / panel background
    QColor base;        // input fields, item views
    QColor alt;         // alternate rows, subtle fills
    QColor text;        // main text
    QColor subText;     // secondary text
    QColor border;      // lines and control borders
    QColor button;      // button background
    QColor hover;       // button hover
    QColor pressed;     // button pressed
    QColor accent;      // highlight / selection / progress
    QColor accentText;  // text over the accent color
};

QStringList ThemeManager::themeIds()
{
    return {QStringLiteral("system"), QStringLiteral("light"),
            QStringLiteral("dark"), QStringLiteral("midnight"),
            QStringLiteral("studio")};
}

QString ThemeManager::themeName(const QString &id)
{
    if (id == QLatin1String("system"))   return QObject::tr("System (follow the OS)");
    if (id == QLatin1String("light"))    return QObject::tr("Light");
    if (id == QLatin1String("dark"))     return QObject::tr("Dark");
    if (id == QLatin1String("midnight")) return QObject::tr("Midnight (deep blue)");
    if (id == QLatin1String("studio"))   return QObject::tr("Studio (black / amber)");
    return id;
}

QColor ThemeManager::themeAccent(const QString &id)
{
    if (id == QLatin1String("midnight")) return QColor(0x4f, 0xc3, 0xf7);
    if (id == QLatin1String("studio"))   return QColor(0xff, 0xb3, 0x00);
    // system/light/dark share the XFB periwinkle
    return QColor(0x7c, 0x7c, 0xba);
}

bool ThemeManager::systemPrefersDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (qApp)
        return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#endif
    return false; // older Qt (Debian bookworm): default to light
}

QString ThemeManager::configuredTheme()
{
    QSettings settings(configFilePath(), QSettings::IniFormat);
    QString id = settings.value("Theme").toString().trimmed().toLower();
    if (id.isEmpty()) {
        // Migrate from the legacy DarkMode checkbox
        id = settings.value("DarkMode", false).toBool()
                 ? QStringLiteral("dark") : QStringLiteral("light");
    }
    if (!themeIds().contains(id))
        id = QStringLiteral("light");
    return id;
}

QColor ThemeManager::configuredAccent()
{
    QSettings settings(configFilePath(), QSettings::IniFormat);
    const QString accent = settings.value("AccentColor").toString().trimmed();
    return accent.isEmpty() ? QColor() : QColor(accent);
}

ThemeManager::Spec ThemeManager::resolveSpec(const QString &id, const QColor &accentOverride)
{
    QString effective = id;
    if (effective == QLatin1String("system"))
        effective = systemPrefersDark() ? QStringLiteral("dark") : QStringLiteral("light");

    Spec s;
    if (effective == QLatin1String("light")) {
        s.dark = false;
        s.window  = QColor(0xf2, 0xf3, 0xf7);
        s.base    = QColor(0xff, 0xff, 0xff);
        s.alt     = QColor(0xe8, 0xea, 0xf0);
        s.text    = QColor(0x24, 0x28, 0x30);
        s.subText = QColor(0x6a, 0x70, 0x80);
        s.border  = QColor(0xc9, 0xcd, 0xd6);
        s.button  = QColor(0xe9, 0xeb, 0xf0);
        s.hover   = QColor(0xdd, 0xe0, 0xe8);
        s.pressed = QColor(0xcf, 0xd3, 0xdd);
    } else if (effective == QLatin1String("midnight")) {
        s.dark = true;
        s.window  = QColor(0x14, 0x18, 0x2b);
        s.base    = QColor(0x0e, 0x11, 0x20);
        s.alt     = QColor(0x1c, 0x21, 0x3a);
        s.text    = QColor(0xd8, 0xdd, 0xf2);
        s.subText = QColor(0x8d, 0x95, 0xb5);
        s.border  = QColor(0x2c, 0x33, 0x52);
        s.button  = QColor(0x20, 0x26, 0x42);
        s.hover   = QColor(0x2a, 0x31, 0x55);
        s.pressed = QColor(0x18, 0x1d, 0x33);
    } else if (effective == QLatin1String("studio")) {
        s.dark = true;
        s.window  = QColor(0x1b, 0x1b, 0x1b);
        s.base    = QColor(0x12, 0x12, 0x12);
        s.alt     = QColor(0x24, 0x24, 0x24);
        s.text    = QColor(0xe6, 0xe2, 0xd8);
        s.subText = QColor(0x9a, 0x96, 0x8c);
        s.border  = QColor(0x38, 0x38, 0x38);
        s.button  = QColor(0x2a, 0x2a, 0x2a);
        s.hover   = QColor(0x38, 0x38, 0x38);
        s.pressed = QColor(0x20, 0x20, 0x20);
    } else { // dark (default)
        s.dark = true;
        s.window  = QColor(0x35, 0x35, 0x35);
        s.base    = QColor(0x2a, 0x2a, 0x2a);
        s.alt     = QColor(0x42, 0x42, 0x42);
        s.text    = QColor(0xe4, 0xe4, 0xe4);
        s.subText = QColor(0xa8, 0xa8, 0xa8);
        s.border  = QColor(0x55, 0x55, 0x55);
        s.button  = QColor(0x44, 0x44, 0x44);
        s.hover   = QColor(0x55, 0x55, 0x55);
        s.pressed = QColor(0x3a, 0x3a, 0x3a);
    }

    s.accent = accentOverride.isValid() ? accentOverride : themeAccent(effective);
    // Black or white, whichever reads better over the accent
    s.accentText = (s.accent.lightness() > 140) ? QColor(Qt::black) : QColor(Qt::white);
    return s;
}

QString ThemeManager::buildStyleSheet(const Spec &s)
{
    QString qss = QStringLiteral(R"XFBQSS(
QMainWindow, QDialog, QWidget {
    background-color: %WINDOW%;
    color: %TEXT%;
}
QLabel { background: transparent; color: %TEXT%; }
QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit {
    background-color: %BASE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 3px;
    padding: 2px 4px;
    selection-background-color: %ACCENT%;
    selection-color: %ACCENT_TEXT%;
}
QPushButton, QToolButton {
    background-color: %BTN%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 3px;
    padding: 4px 8px;
}
QPushButton:hover, QToolButton:hover { background-color: %HOVER%; }
QPushButton:pressed, QToolButton:pressed { background-color: %PRESSED%; }
QPushButton:disabled, QToolButton:disabled { color: %SUBTEXT%; }
QToolButton:checked { background-color: %ACCENT%; color: %ACCENT_TEXT%; }
QComboBox {
    background-color: %BASE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 3px;
    padding: 2px 6px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background-color: %BASE%;
    color: %TEXT%;
    selection-background-color: %ACCENT%;
    selection-color: %ACCENT_TEXT%;
    border: 1px solid %BORDER%;
    outline: 0px;
}
QTableView, QListView, QTreeView, QListWidget {
    background-color: %BASE%;
    color: %TEXT%;
    alternate-background-color: %ALT%;
    gridline-color: %BORDER%;
    selection-background-color: %ACCENT%;
    selection-color: %ACCENT_TEXT%;
    border: 1px solid %BORDER%;
}
QHeaderView::section {
    background-color: %BTN%;
    color: %TEXT%;
    padding: 4px;
    border: 1px solid %BORDER%;
}
QProgressBar {
    border: 1px solid %BORDER%;
    border-radius: 3px;
    text-align: center;
    color: %TEXT%;
    background-color: %BASE%;
}
QProgressBar::chunk { background-color: %ACCENT%; }
QTabWidget::pane { border: 1px solid %BORDER%; background-color: %WINDOW%; }
QTabBar::tab {
    background: %BTN%;
    border: 1px solid %BORDER%;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
    min-width: 8ex;
    padding: 4px 8px;
    color: %TEXT%;
}
QTabBar::tab:selected { background: %WINDOW%; border-bottom-color: %WINDOW%; }
QTabBar::tab:hover { background: %HOVER%; }
QTabBar::tab:!selected { margin-top: 2px; }
QTabBar::tab:bottom {
    border-top-left-radius: 0px;
    border-top-right-radius: 0px;
    border-bottom-left-radius: 4px;
    border-bottom-right-radius: 4px;
}
QTabBar::tab:bottom:!selected { margin-top: 0px; margin-bottom: 2px; }
QCheckBox, QRadioButton { color: %TEXT%; background: transparent; }
QCheckBox::indicator, QRadioButton::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid %BORDER%;
    background-color: %BASE%;
}
QRadioButton::indicator { border-radius: 7px; }
QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: %ACCENT%;
    border: 1px solid %ACCENT%;
}
QSlider::groove:horizontal {
    border: 1px solid %BORDER%;
    height: 8px;
    background: %BASE%;
    margin: 2px 0;
    border-radius: 4px;
}
QSlider::handle:horizontal {
    background: %ACCENT%;
    border: 1px solid %BORDER%;
    width: 16px;
    margin: -4px 0;
    border-radius: 4px;
}
QSlider::groove:vertical {
    border: 1px solid %BORDER%;
    width: 8px;
    background: %BASE%;
    margin: 0 2px;
    border-radius: 4px;
}
QSlider::handle:vertical {
    background: %ACCENT%;
    border: 1px solid %BORDER%;
    height: 16px;
    margin: 0 -4px;
    border-radius: 4px;
}
QMenuBar { background-color: %WINDOW%; color: %TEXT%; }
QMenuBar::item { background: transparent; padding: 4px 8px; }
QMenuBar::item:selected { background: %HOVER%; border-radius: 3px; }
QMenu { background-color: %BASE%; color: %TEXT%; border: 1px solid %BORDER%; }
QMenu::item { padding: 4px 24px 4px 12px; }
QMenu::item:selected { background-color: %ACCENT%; color: %ACCENT_TEXT%; }
QMenu::separator { height: 1px; background: %BORDER%; margin: 4px 8px; }
QStatusBar { background-color: %WINDOW%; color: %TEXT%; border: none; }
QStatusBar::item { border: none; }
QStatusBar QLabel { background: transparent; }
QToolBar { background: %WINDOW%; border: 1px solid %BORDER%; }
QToolBox::tab {
    background-color: %BTN%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 3px;
    padding: 2px;
}
QToolBox::tab:selected { background-color: %ACCENT%; color: %ACCENT_TEXT%; }
QScrollBar:vertical {
    background: %WINDOW%;
    width: 12px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: %BORDER%;
    border-radius: 4px;
    min-height: 24px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover { background: %SUBTEXT%; }
QScrollBar:horizontal {
    background: %WINDOW%;
    height: 12px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: %BORDER%;
    border-radius: 4px;
    min-width: 24px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover { background: %SUBTEXT%; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: none; }
QDockWidget {
    color: %TEXT%;
    titlebar-close-icon: none;
    titlebar-normal-icon: none;
}
QDockWidget::title {
    background: %ALT%;
    color: %SUBTEXT%;
    padding: 3px 8px;
    border: 1px solid %BORDER%;
    border-radius: 3px;
    text-align: left;
}
QDockWidget::close-button, QDockWidget::float-button {
    background: %BTN%;
    border: 1px solid %BORDER%;
    border-radius: 2px;
}
QDockWidget::close-button:hover, QDockWidget::float-button:hover { background: %HOVER%; }
QMainWindow::separator { background: %WINDOW%; width: 4px; height: 4px; }
QMainWindow::separator:hover { background: %ACCENT%; }
QSplitter::handle { background: %WINDOW%; }
QSplitter::handle:hover { background: %ACCENT%; }
QToolTip {
    background-color: %BASE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    padding: 2px;
}
QLCDNumber { color: %ACCENT%; background: transparent; border: none; }
/* Widget-specific accents (ids from player.ui) */
#historyList { background-color: %ALT%; }
#txt_horas { color: %ACCENT%; }
#txt_bottom_info { color: %SUBTEXT%; }
)XFBQSS");

    const auto hex = [](const QColor &c) { return c.name(QColor::HexRgb); };
    qss.replace(QLatin1String("%WINDOW%"), hex(s.window));
    qss.replace(QLatin1String("%BASE%"), hex(s.base));
    qss.replace(QLatin1String("%ALT%"), hex(s.alt));
    qss.replace(QLatin1String("%TEXT%"), hex(s.text));
    qss.replace(QLatin1String("%SUBTEXT%"), hex(s.subText));
    qss.replace(QLatin1String("%BORDER%"), hex(s.border));
    qss.replace(QLatin1String("%BTN%"), hex(s.button));
    qss.replace(QLatin1String("%HOVER%"), hex(s.hover));
    qss.replace(QLatin1String("%PRESSED%"), hex(s.pressed));
    qss.replace(QLatin1String("%ACCENT%"), hex(s.accent));
    qss.replace(QLatin1String("%ACCENT_TEXT%"), hex(s.accentText));
    return qss;
}

void ThemeManager::apply(QApplication *app)
{
    if (!app)
        return;

    const QString id = configuredTheme();
    const Spec spec = resolveSpec(id, configuredAccent());
    s_currentIsDark = spec.dark;

    QPalette palette;
    palette.setColor(QPalette::Window, spec.window);
    palette.setColor(QPalette::WindowText, spec.text);
    palette.setColor(QPalette::Base, spec.base);
    palette.setColor(QPalette::AlternateBase, spec.alt);
    palette.setColor(QPalette::ToolTipBase, spec.base);
    palette.setColor(QPalette::ToolTipText, spec.text);
    palette.setColor(QPalette::Text, spec.text);
    palette.setColor(QPalette::PlaceholderText, spec.subText);
    palette.setColor(QPalette::Button, spec.button);
    palette.setColor(QPalette::ButtonText, spec.text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, spec.accent);
    palette.setColor(QPalette::HighlightedText, spec.accentText);
    palette.setColor(QPalette::Link, spec.accent);
    palette.setColor(QPalette::Mid, spec.border);
    palette.setColor(QPalette::Disabled, QPalette::Text, spec.subText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, spec.subText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, spec.subText);
    app->setPalette(palette);
    app->setStyleSheet(buildStyleSheet(spec));

    // Keep the legacy key in sync for code that still branches on it
    QSettings settings(configFilePath(), QSettings::IniFormat);
    settings.setValue("DarkMode", spec.dark);
    if (!settings.contains("Theme"))
        settings.setValue("Theme", id);

    qDebug() << "ThemeManager: applied theme" << id << "(dark:" << spec.dark << ")";
}

bool ThemeManager::currentIsDark()
{
    return s_currentIsDark;
}
