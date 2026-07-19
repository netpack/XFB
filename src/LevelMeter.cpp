#include "LevelMeter.h"

#include <QPainter>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace
{
// Map a linear sample peak (0..1) to meter fraction using a dB scale from
// -48 dBFS to 0 dBFS — linear mapping would leave most music pinned in the
// first few segments.
double meterFraction(float linear)
{
    if (linear <= 0.0f)
        return 0.0;
    const double db = 20.0 * std::log10(double(linear));
    return std::clamp((db + 48.0) / 48.0, 0.0, 1.0);
}
} // namespace

LevelMeter::LevelMeter(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    // ~33 fps decay: fast enough to look alive, cheap enough to not matter
    m_decayTimer = new QTimer(this);
    m_decayTimer->setInterval(30);
    connect(m_decayTimer, &QTimer::timeout, this, &LevelMeter::decayTick);
}

void LevelMeter::setMeterOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation)
        return;
    m_orientation = orientation;
    if (orientation == Qt::Vertical)
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    else
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    updateGeometry();
    update();
}

QSize LevelMeter::sizeHint() const
{
    if (m_orientation == Qt::Vertical)
        return QSize(30, kSegments * 6 + 16);
    return QSize(kSegments * 6 + 14, 22);
}

QSize LevelMeter::minimumSizeHint() const
{
    if (m_orientation == Qt::Vertical)
        return QSize(30, 80);
    return sizeHint();
}

void LevelMeter::setLevels(float left, float right)
{
    m_left = std::max(m_left, std::clamp(left, 0.0f, 1.0f));
    m_right = std::max(m_right, std::clamp(right, 0.0f, 1.0f));
    m_peakLeft = std::max(m_peakLeft, m_left);
    m_peakRight = std::max(m_peakRight, m_right);
    if (!m_decayTimer->isActive())
        m_decayTimer->start();
    update();
}

void LevelMeter::clear()
{
    m_left = m_right = 0.0f;
    m_peakLeft = m_peakRight = 0.0f;
    m_decayTimer->stop();
    update();
}

void LevelMeter::decayTick()
{
    // Level falls quickly, the peak-hold marker drifts down slowly
    m_left = std::max(0.0f, m_left - 0.06f);
    m_right = std::max(0.0f, m_right - 0.06f);
    m_peakLeft = std::max(m_peakLeft - 0.012f, m_left);
    m_peakRight = std::max(m_peakRight - 0.012f, m_right);
    if (m_left <= 0.0f && m_right <= 0.0f
        && m_peakLeft <= 0.0f && m_peakRight <= 0.0f) {
        m_decayTimer->stop();
    }
    update();
}

QColor LevelMeter::segmentBaseColor(int segment, int total)
{
    // Green up to 70%, yellow to 90%, red above — classic console look
    if (segment < total * 7 / 10)
        return QColor(40, 210, 60);
    if (segment < total * 9 / 10)
        return QColor(230, 200, 40);
    return QColor(230, 50, 40);
}

void LevelMeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(20, 20, 20));
    p.setPen(QColor(160, 160, 160));
    QFont f = p.font();

    const struct Chan {
        const char *label;
        float level;
        float peak;
    } chans[2] = {
        {"L", m_left, m_peakLeft},
        {"R", m_right, m_peakRight},
    };

    if (m_orientation == Qt::Horizontal) {
        const int labelW = 10;
        const int rowH = (height() - 3) / 2;
        const int segW = (width() - labelW - 2) / kSegments;
        f.setPixelSize(rowH - 2);
        p.setFont(f);

        for (int c = 0; c < 2; ++c) {
            const int y = (c == 0) ? 1 : rowH + 2;
            p.setPen(QColor(160, 160, 160));
            p.drawText(QRect(0, y, labelW, rowH), Qt::AlignCenter, chans[c].label);

            const int lit = int(std::lround(meterFraction(chans[c].level) * kSegments));
            const int peakSeg = int(std::lround(meterFraction(chans[c].peak) * kSegments));
            for (int s = 0; s < kSegments; ++s) {
                const QColor on = segmentBaseColor(s, kSegments);
                const QColor color = (s < lit) ? on
                                     : (s == peakSeg - 1) ? on.lighter(115)
                                                          : on.darker(750);
                p.fillRect(labelW + 1 + s * segW, y, segW - 1, rowH, color);
            }
        }
        return;
    }

    // Vertical: two columns rising bottom-up, channel labels underneath
    const int labelH = 12;
    const int colW = (width() - 3) / 2;
    const int segH = std::max(2, (height() - labelH - 2) / kSegments);
    f.setPixelSize(labelH - 3);
    p.setFont(f);

    for (int c = 0; c < 2; ++c) {
        const int x = (c == 0) ? 1 : colW + 2;
        p.setPen(QColor(160, 160, 160));
        p.drawText(QRect(x, height() - labelH, colW, labelH),
                   Qt::AlignCenter, chans[c].label);

        const int lit = int(std::lround(meterFraction(chans[c].level) * kSegments));
        const int peakSeg = int(std::lround(meterFraction(chans[c].peak) * kSegments));
        for (int s = 0; s < kSegments; ++s) {
            const QColor on = segmentBaseColor(s, kSegments);
            const QColor color = (s < lit) ? on
                                 : (s == peakSeg - 1) ? on.lighter(115)
                                                      : on.darker(750);
            const int y = height() - labelH - 2 - (s + 1) * segH;
            p.fillRect(x, y, colW - 1, segH - 1, color);
        }
    }
}
