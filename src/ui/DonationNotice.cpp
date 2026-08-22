#include "DonationNotice.h"

#include <QDesktopServices>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

namespace {
// Long enough to read twice, short enough to stay out of the way.
constexpr int kAutoHideMs = 20000;
constexpr int kFadeInMs   = 260;
constexpr int kFadeOutMs  = 400;
// Distance from the main window's bottom-right corner. The bottom margin
// clears the status bar so the notice never sits on top of it.
constexpr int kMarginRight  = 18;
constexpr int kMarginBottom = 42;
constexpr int kCornerRadius = 8;
constexpr int kIconSize     = 18;

const char *kDonateUrl =
    "https://www.paypal.com/donate/?hosted_button_id=TFDSZU78WLMC6";

/** The card colour: the window colour, lifted a step off what it covers. */
QColor cardColor(const QPalette &palette)
{
    const QColor base = palette.color(QPalette::Window);
    return base.lightness() < 128 ? base.lighter(135) : base.darker(106);
}

/**
 * The accent, pushed until it is legible on the card.
 *
 * Themes pick their accent to sit on the window colour, and some of them
 * (a blue accent on the midnight theme) land too close to the card to read
 * as a link. Move it away from the card's lightness until it separates.
 */
QColor linkColor(const QPalette &palette)
{
    const int cardLightness = cardColor(palette).lightness();
    QColor link = palette.color(QPalette::Highlight);
    for (int step = 0; step < 8 && qAbs(link.lightness() - cardLightness) < 70; ++step)
        link = cardLightness < 128 ? link.lighter(115) : link.darker(115);
    return link;
}
}

DonationNotice::DonationNotice(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("donationNotice");
    // Never take focus away from the decks: the notice is read, not operated.
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(tr("Support XFB"));

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(12, 10, 8, 10);
    row->setSpacing(9);

    auto *icon = new QLabel(this);
    // A heart, not the 16px coin or the donate artwork (that one is the
    // PayPal QR code, which is illegible at this size). Scaled from the large
    // original so it stays sharp on a high-DPI screen.
    const qreal dpr = devicePixelRatioF();
    QPixmap art = QPixmap(":/icons/heart200.png")
                      .scaled(int(kIconSize * dpr), int(kIconSize * dpr),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
    art.setDevicePixelRatio(dpr);
    icon->setPixmap(art);
    icon->setFixedSize(kIconSize, kIconSize);
    row->addWidget(icon, 0, Qt::AlignVCenter);

    m_message = new QLabel(this);
    m_message->setTextFormat(Qt::RichText);
    m_message->setWordWrap(true);
    m_message->setOpenExternalLinks(false);
    m_message->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_message, &QLabel::linkActivated, this, [this](const QString &link) {
        QDesktopServices::openUrl(QUrl(link));
        dismiss();
    });
    row->addWidget(m_message, 1);

    m_close = new QToolButton(this);
    m_close->setText(QString::fromUtf8("×"));
    m_close->setCursor(Qt::ArrowCursor);
    m_close->setFixedSize(18, 18);
    m_close->setToolTip(tr("Close this reminder"));
    m_close->setAccessibleName(tr("Close this reminder"));
    connect(m_close, &QToolButton::clicked, this, &DonationNotice::dismiss);
    row->addWidget(m_close, 0, Qt::AlignTop);

    applyPalette();

    setFixedWidth(330);
    adjustSize();

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(0.0);
    setGraphicsEffect(m_opacity);

    m_fade = new QPropertyAnimation(m_opacity, "opacity", this);

    m_autoHide = new QTimer(this);
    m_autoHide->setSingleShot(true);
    m_autoHide->setInterval(kAutoHideMs);
    connect(m_autoHide, &QTimer::timeout, this, &DonationNotice::dismiss);

    if (parent)
        parent->installEventFilter(this);

    hide();
    reposition();
}

QString DonationNotice::spokenText() const
{
    return tr("Enjoying XFB? It is kept alive by donations. "
              "A donation link is shown in the corner of the window.");
}

void DonationNotice::popUp()
{
    reposition();
    show();
    raise();
    fadeTo(1.0, kFadeInMs, false);
    m_autoHide->start();
}

void DonationNotice::dismiss()
{
    m_autoHide->stop();
    if (!isVisible())
        return;
    fadeTo(0.0, kFadeOutMs, true);
}

void DonationNotice::fadeTo(qreal opacity, int durationMs, bool hideWhenDone)
{
    m_fade->stop();
    m_fade->setDuration(durationMs);
    m_fade->setStartValue(m_opacity->opacity());
    m_fade->setEndValue(opacity);
    m_fade->disconnect(this);
    if (hideWhenDone) {
        connect(m_fade, &QPropertyAnimation::finished, this, [this]() {
            if (qFuzzyIsNull(m_opacity->opacity()))
                hide();
        });
    }
    m_fade->start();
}

void DonationNotice::reposition()
{
    QWidget *host = parentWidget();
    if (!host)
        return;
    adjustSize();
    move(qMax(0, host->width()  - width()  - kMarginRight),
         qMax(0, host->height() - height() - kMarginBottom));
}

// The message and the × are recoloured from the palette rather than from a
// stylesheet with fixed colours, so switching theme (or accent) in Options
// carries the notice with it.
void DonationNotice::applyPalette()
{
    const QColor link = linkColor(palette());
    m_message->setText(tr("Enjoying XFB? It is kept alive by donations — "
                          "<a style=\"color:%1;\" href=\"%2\">chip in via PayPal</a> "
                          "if you can.")
                           .arg(link.name(), QString::fromLatin1(kDonateUrl)));

    QColor closeColor = palette().color(QPalette::WindowText);
    closeColor.setAlpha(160);
    m_close->setStyleSheet(
        QStringLiteral("QToolButton { border: none; background: transparent;"
                       "              color: %1; font-size: 14px; }"
                       "QToolButton:hover { color: %2; }")
            .arg(closeColor.name(QColor::HexArgb),
                 palette().color(QPalette::WindowText).name()));
}

bool DonationNotice::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
        reposition();
    }
    return QWidget::eventFilter(watched, event);
}

void DonationNotice::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        applyPalette();
        update();
    }
}

// Reading time is the reader's, not the timer's: hovering holds the notice
// open, leaving it starts the countdown again from the top.
void DonationNotice::enterEvent(QEnterEvent *event)
{
    m_autoHide->stop();
    QWidget::enterEvent(event);
}

void DonationNotice::leaveEvent(QEvent *event)
{
    if (isVisible())
        m_autoHide->start();
    QWidget::leaveEvent(event);
}

// Painted from the palette rather than a stylesheet so every theme (and any
// accent override) carries the notice without extra work.
void DonationNotice::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                        kCornerRadius, kCornerRadius);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, cardColor(palette()));
    QColor border = palette().color(QPalette::Highlight);
    border.setAlpha(170);
    painter.setPen(QPen(border, 1.0));
    painter.drawPath(path);
}
