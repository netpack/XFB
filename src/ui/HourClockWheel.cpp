#include "HourClockWheel.h"

#include <QApplication>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include "../ThemeManager.h"

namespace {

/** Qt's angle unit: sixteenths of a degree, anticlockwise from three o'clock.
 *  A clock face is clockwise from twelve, so every angle goes through here. */
int qtAngleForSecond(int second)
{
    const double degrees = 90.0 - (static_cast<double>(second)
                                   / HourClock::kHourSeconds) * 360.0;
    return static_cast<int>(qRound(degrees * 16.0));
}

constexpr double kInnerRatio = 0.42;   ///< the hole in the middle of the donut

} // namespace

HourClockWheel::HourClockWheel(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(240, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(tr("Hour clock wheel"));
    setAccessibleDescription(
        tr("The hour drawn as a clock face. Left and right arrows move between "
           "items, Return edits the selected item. Everything here can also be "
           "done in the list of slots beside it."));
}

void HourClockWheel::setClock(const HourClock::Clock &clock)
{
    m_clock    = clock;
    HourClock::normalise(m_clock);
    m_timeline = HourClock::resolve(m_clock);
    if (m_selected >= m_clock.items.size())
        m_selected = m_clock.items.isEmpty() ? -1 : m_clock.items.size() - 1;
    update();
}

void HourClockWheel::setSelectedIndex(int index)
{
    const int clamped = (index >= 0 && index < m_clock.items.size()) ? index : -1;
    if (clamped == m_selected)
        return;
    m_selected = clamped;
    update();
    emit selectionChanged(m_selected);
}

void HourClockWheel::setNowSecond(int second)
{
    const int clamped = (second >= 0 && second < HourClock::kHourSeconds) ? second : -1;
    if (clamped == m_nowSecond)
        return;
    m_nowSecond = clamped;
    update();
}

void HourClockWheel::retargetDrag(int newIndex)
{
    if (m_drag == Drag::None || newIndex < 0 || newIndex >= m_timeline.items.size())
        return;
    const int offset = m_dragGrabSecond - m_dragOriginalStart;
    m_dragIndex          = newIndex;
    m_dragOriginalStart  = m_timeline.items.at(newIndex).start;
    m_dragOriginalLength = m_timeline.items.at(newIndex).length;
    m_dragGrabSecond     = m_dragOriginalStart + offset;
    m_selected           = newIndex;
    update();
}

QSize HourClockWheel::sizeHint() const       { return QSize(360, 360); }
QSize HourClockWheel::minimumSizeHint() const { return QSize(240, 240); }

QRectF HourClockWheel::faceRect() const
{
    const qreal side = qMin(width(), height()) - 24.0;
    const qreal s = qMax<qreal>(80.0, side);
    return QRectF((width() - s) / 2.0, (height() - s) / 2.0, s, s);
}

int HourClockWheel::secondAt(const QPointF &pos) const
{
    const QRectF face = faceRect();
    const QPointF centre = face.center();
    const qreal dx = pos.x() - centre.x();
    const qreal dy = pos.y() - centre.y();
    const qreal r  = std::hypot(dx, dy);
    const qreal outer = face.width() / 2.0;
    if (r > outer || r < outer * kInnerRatio * 0.6)
        return -1;
    // atan2 with y flipped gives the usual maths angle; convert to clockwise
    // from twelve.
    qreal degrees = qRadiansToDegrees(std::atan2(-dy, dx));
    qreal fromTop = 90.0 - degrees;
    while (fromTop < 0.0)   fromTop += 360.0;
    while (fromTop >= 360.0) fromTop -= 360.0;
    return static_cast<int>(fromTop / 360.0 * HourClock::kHourSeconds);
}

int HourClockWheel::indexAt(const QPointF &pos) const
{
    const int second = secondAt(pos);
    if (second < 0)
        return -1;
    const int resolved = m_timeline.indexAt(second);
    if (resolved < 0)
        return -1;
    // The timeline is in the clock's own order, so the two indices agree.
    return (resolved < m_clock.items.size()) ? resolved : -1;
}

bool HourClockWheel::nearTrailingEdge(const QPointF &pos, int index) const
{
    if (index < 0 || index >= m_timeline.items.size())
        return false;
    const int second = secondAt(pos);
    if (second < 0)
        return false;
    const int end = m_timeline.items.at(index).end();
    // A generous grab area, in seconds: 40 s of the hour is about 4 degrees.
    return qAbs(second - end) <= 45 || qAbs(second - (end - HourClock::kHourSeconds)) <= 45;
}

QColor HourClockWheel::colourFor(HourClock::SlotType type, bool selected) const
{
    const QColor accent = ThemeManager::currentAccent();
    const bool dark = ThemeManager::currentIsDark();
    // Every wedge colour is the theme's accent rotated round the wheel, so a
    // station on the studio theme gets ambers and one on midnight gets blues.
    // Nothing here is a literal.
    int shift = 0;
    int sat   = 0;
    switch (type) {
    case HourClock::SlotType::MusicSweep: shift = 0;   sat = 0;   break;
    case HourClock::SlotType::AdBreak:    shift = 150; sat = -30; break;
    case HourClock::SlotType::Jingle:     shift = 60;  sat = -10; break;
    case HourClock::SlotType::Program:    shift = 210; sat = -20; break;
    case HourClock::SlotType::News:       shift = 300; sat = 0;   break;
    case HourClock::SlotType::StationId:  shift = 30;  sat = -40; break;
    }
    int h, s, v, a;
    accent.getHsv(&h, &s, &v, &a);
    if (h < 0) h = 0;   // a grey accent still has to produce distinct wedges
    h = (h + shift) % 360;
    s = qBound(60, s + sat, 255);
    v = qBound(60, dark ? v : qMin(250, v + 20), 255);
    QColor c = QColor::fromHsv(h, s, v, a);
    if (!selected)
        c.setAlpha(dark ? 190 : 205);
    return c;
}

void HourClockWheel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPalette pal = palette();
    const QRectF face = faceRect();
    const QPointF centre = face.center();
    const qreal outer = face.width() / 2.0;
    const qreal inner = outer * kInnerRatio;

    p.fillRect(rect(), pal.color(QPalette::Base));

    // The face itself, so an empty clock still reads as a clock.
    p.setPen(QPen(pal.color(QPalette::Mid), 1.0));
    p.setBrush(pal.color(QPalette::Window));
    p.drawEllipse(face);

    // Wedges.
    for (int i = 0; i < m_timeline.items.size(); ++i) {
        const HourClock::ResolvedSlot &r = m_timeline.items.at(i);
        if (r.length <= 0)
            continue;
        const int start = qBound(0, r.start, HourClock::kHourSeconds);
        const int end   = qMin(HourClock::kHourSeconds, r.end());
        if (end <= start)
            continue;

        const int startAngle = qtAngleForSecond(start);
        const int spanAngle  = -static_cast<int>(qRound(
            (static_cast<double>(end - start) / HourClock::kHourSeconds) * 360.0 * 16.0));

        QPainterPath wedge;
        wedge.moveTo(centre);
        wedge.arcTo(face, startAngle / 16.0, spanAngle / 16.0);
        wedge.closeSubpath();

        const bool selected = (i == m_selected);
        QColor fill = colourFor(r.slot.type, selected);
        p.setBrush(fill);
        p.setPen(QPen(pal.color(QPalette::Base), selected ? 2.5 : 1.0));
        p.drawPath(wedge);

        if (r.overruns) {
            // An item that runs past a fixed one is hatched, so an impossible
            // hour looks impossible rather than merely being described as one.
            QBrush hatch(ThemeManager::contrastingInk(pal.color(QPalette::Text)),
                         Qt::BDiagPattern);
            p.setBrush(hatch);
            p.setPen(Qt::NoPen);
            p.drawPath(wedge);
        }
    }

    // The hole, and the totals written in it.
    p.setPen(QPen(pal.color(QPalette::Mid), 1.0));
    p.setBrush(pal.color(QPalette::Base));
    p.drawEllipse(centre, inner, inner);

    // Minute ticks every five minutes, and the labels at the quarters.
    p.save();
    p.setPen(QPen(pal.color(QPalette::Mid), 1.0));
    for (int minute = 0; minute < 60; minute += 5) {
        const double rad = qDegreesToRadians(
            90.0 - (minute / 60.0) * 360.0);
        const QPointF a(centre.x() + std::cos(rad) * outer,
                        centre.y() - std::sin(rad) * outer);
        const QPointF b(centre.x() + std::cos(rad) * (outer - (minute % 15 ? 6.0 : 11.0)),
                        centre.y() - std::sin(rad) * (outer - (minute % 15 ? 6.0 : 11.0)));
        p.drawLine(a, b);
    }
    p.restore();

    // The "now" hand.
    if (m_nowSecond >= 0) {
        const double rad = qDegreesToRadians(
            90.0 - (static_cast<double>(m_nowSecond) / HourClock::kHourSeconds) * 360.0);
        p.setPen(QPen(ThemeManager::contrastingInk(ThemeManager::currentAccent()), 2.0));
        p.drawLine(QPointF(centre.x() + std::cos(rad) * inner,
                           centre.y() - std::sin(rad) * inner),
                   QPointF(centre.x() + std::cos(rad) * outer,
                           centre.y() - std::sin(rad) * outer));
    }

    // The middle: name and total.
    p.setPen(pal.color(QPalette::Text));
    QFont title = font();
    title.setBold(true);
    p.setFont(title);
    const QRectF hole(centre.x() - inner * 0.86, centre.y() - inner * 0.6,
                      inner * 1.72, inner * 1.2);
    QString middle = m_clock.name.isEmpty() ? tr("(no clock)") : m_clock.name;
    if (m_timeline.isOverfull())
        middle += QLatin1Char('\n') + tr("over by %1")
                      .arg(HourClock::formatLength(m_timeline.overfullSeconds));
    else if (m_timeline.isUnderfilled())
        middle += QLatin1Char('\n') + tr("short by %1")
                      .arg(HourClock::formatLength(m_timeline.underfilledSeconds));
    else if (!m_clock.items.isEmpty())
        middle += QLatin1Char('\n') + tr("exactly full");
    p.drawText(hole, Qt::AlignCenter | Qt::TextWordWrap, middle);

    // The keyboard focus ring: this widget takes focus, so it must show it.
    if (hasFocus()) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(ThemeManager::currentAccent(), 2.0, Qt::DashLine));
        p.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 6, 6);
    }
}

void HourClockWheel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    setFocus(Qt::MouseFocusReason);
    const int index = indexAt(event->position());
    if (index < 0) {
        setSelectedIndex(-1);
        return;
    }
    setSelectedIndex(index);
    announceSelection();

    m_dragIndex          = index;
    m_dragGrabSecond     = secondAt(event->position());
    m_dragOriginalStart  = m_timeline.items.at(index).start;
    m_dragOriginalLength = m_timeline.items.at(index).length;
    m_dragMoved          = false;
    m_drag = nearTrailingEdge(event->position(), index) ? Drag::ResizeEnd : Drag::Move;
}

void HourClockWheel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drag == Drag::None) {
        const int hovered = indexAt(event->position());
        setCursor(hovered >= 0 && nearTrailingEdge(event->position(), hovered)
                      ? Qt::SizeHorCursor
                      : (hovered >= 0 ? Qt::OpenHandCursor : Qt::ArrowCursor));
        return;
    }
    const int second = secondAt(event->position());
    if (second < 0)
        return;
    m_dragMoved = true;

    if (m_drag == Drag::ResizeEnd) {
        const int wanted = qMax(5, second - m_dragOriginalStart);
        emit slotResized(m_dragIndex, wanted);
    } else {
        const int delta = second - m_dragGrabSecond;
        const int wanted = qBound(0, m_dragOriginalStart + delta,
                                  HourClock::kHourSeconds - 1);
        emit slotMoved(m_dragIndex, wanted);
    }
}

void HourClockWheel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (m_drag != Drag::None && m_dragMoved && m_dragIndex >= 0
        && m_dragIndex < m_timeline.items.size()) {
        emit announcementRequested(
            HourClock::describeSlot(m_timeline.items.at(m_dragIndex)));
    }
    m_drag = Drag::None;
    m_dragIndex = -1;
    m_dragMoved = false;
}

void HourClockWheel::mouseDoubleClickEvent(QMouseEvent *event)
{
    const int index = indexAt(event->position());
    if (index >= 0) {
        setSelectedIndex(index);
        emit editRequested(index);
    }
}

void HourClockWheel::keyPressEvent(QKeyEvent *event)
{
    if (m_clock.items.isEmpty()) {
        QWidget::keyPressEvent(event);
        return;
    }
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Down:
        setSelectedIndex(m_selected < 0 ? 0
                                        : (m_selected + 1) % m_clock.items.size());
        announceSelection();
        return;
    case Qt::Key_Left:
    case Qt::Key_Up:
        setSelectedIndex(m_selected <= 0 ? m_clock.items.size() - 1
                                         : m_selected - 1);
        announceSelection();
        return;
    case Qt::Key_Home:
        setSelectedIndex(0);
        announceSelection();
        return;
    case Qt::Key_End:
        setSelectedIndex(m_clock.items.size() - 1);
        announceSelection();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (m_selected >= 0)
            emit editRequested(m_selected);
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void HourClockWheel::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    update();
    if (m_selected < 0 && !m_clock.items.isEmpty())
        setSelectedIndex(0);
}

void HourClockWheel::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    update();
}

void HourClockWheel::announceSelection()
{
    if (m_selected < 0 || m_selected >= m_timeline.items.size())
        return;
    emit announcementRequested(
        HourClock::describeSlot(m_timeline.items.at(m_selected)));
}
