#include "PlaylistWaveView.h"

#include "audio/FxPlayer.h"
#include "audio/WaveformStore.h"

#include <QAudioOutput>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTimer>
#include <QToolTip>

#include <algorithm>

namespace
{
constexpr int kRowHeight = 88;
constexpr int kMargin = 5;
constexpr int kTextHeight = 17;
constexpr int kFullWaveHeight = 22;
constexpr int kPreviewBtnSize = 22;
constexpr int kPreviewLeadMs = 6000;   // audition starts this long before the segue
constexpr int kPreviewTailMs = 4000;   // and keeps playing this long into the next track
constexpr double kPreviewVolume = 0.85;

QString formatDuration(qint64 ms)
{
    const qint64 secs = ms / 1000;
    if (secs >= 3600)
        return QStringLiteral("%1:%2:%3").arg(secs / 3600)
            .arg((secs / 60) % 60, 2, 10, QLatin1Char('0'))
            .arg(secs % 60, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(secs / 60)
        .arg(secs % 60, 2, 10, QLatin1Char('0'));
}

// Volume-line coordinate mapping over the full-track waveform rect
double envNodeX(const QRect &waveRect, qint64 durationMs, double ms)
{
    return waveRect.left()
           + (durationMs > 0 ? ms / double(durationMs) : 0.0) * waveRect.width();
}

double envNodeY(const QRect &waveRect, double gain)
{
    return waveRect.top() + (1.0 - qBound(0.0, gain, 1.0)) * (waveRect.height() - 1);
}

int envelopeNodeHit(const QVector<QPointF> &points, const QRect &waveRect,
                    qint64 durationMs, const QPoint &pos)
{
    for (int i = 0; i < points.size(); ++i) {
        const double x = envNodeX(waveRect, durationMs, points[i].x());
        const double y = envNodeY(waveRect, points[i].y());
        if (qAbs(pos.x() - x) <= 5 && qAbs(pos.y() - y) <= 5)
            return i;
    }
    return -1;
}

// Grabbing the line away from any node: the indices of the segment's end
// nodes (equal on the flat run before the first / after the last node).
struct SegmentHit
{
    int first = -1;
    int last = -1;
};

SegmentHit envelopeSegmentAt(const QVector<QPointF> &points, const QRect &waveRect,
                             qint64 durationMs, const QPoint &pos)
{
    if (points.isEmpty() || durationMs <= 0)
        return {};
    const double ms = double(pos.x() - waveRect.left())
                      / qMax(1, waveRect.width()) * durationMs;
    const double lineY =
        envNodeY(waveRect, PlaylistWaveView::envelopeGainAt(points, qint64(ms)));
    if (qAbs(pos.y() - lineY) > 4)
        return {};
    if (ms <= points.first().x())
        return {0, 0};
    if (ms >= points.last().x())
        return {int(points.size()) - 1, int(points.size()) - 1};
    for (int i = 1; i < points.size(); ++i)
        if (ms <= points[i].x())
            return {i - 1, i};
    return {int(points.size()) - 1, int(points.size()) - 1};
}

// Moves the grabbed nodes together; the delta is clamped as a group so the
// segment keeps its shape when it reaches 0% or 100%.
void applySegmentDelta(QVector<QPointF> &points, int first, int last,
                       const QVector<double> &startGains, double delta)
{
    double lowest = 1.0, highest = 0.0;
    for (double g : startGains) {
        lowest = qMin(lowest, g);
        highest = qMax(highest, g);
    }
    delta = qBound(-lowest, delta, 1.0 - highest);
    for (int i = first; i <= last && i < points.size(); ++i)
        points[i].setY(startGains[i - first] + delta);
}

void paintWaveform(QPainter *painter, const QRect &rect, const WaveformData &data,
                   qint64 fromMs, qint64 toMs, const QColor &color)
{
    if (!data.ready() || rect.width() <= 0 || toMs <= fromMs)
        return;

    const double msPerPx = double(toMs - fromMs) / rect.width();
    const double midY = rect.center().y() + 0.5;
    const double halfH = rect.height() / 2.0 - 1.0;

    QVector<QLineF> lines;
    lines.reserve(rect.width());
    for (int x = 0; x < rect.width(); ++x) {
        const qint64 msStart = fromMs + qint64(x * msPerPx);
        const qint64 msEnd = fromMs + qint64((x + 1) * msPerPx);
        int i0 = int(msStart / WaveformStore::MsPerPeak);
        int i1 = int(msEnd / WaveformStore::MsPerPeak);
        i1 = qMax(i1, i0 + 1);
        if (i0 < 0 || i0 >= data.peaks.size())
            continue;
        i1 = qMin(i1, int(data.peaks.size()));
        int peak = 0;
        for (int i = i0; i < i1; ++i)
            peak = qMax(peak, int(data.peaks[i]));
        const double amp = qMax(0.6, halfH * peak / 255.0);
        const double px = rect.left() + x + 0.5;
        lines.append(QLineF(px, midY - amp, px, midY + amp));
    }
    painter->setPen(QPen(color, 1.0));
    painter->drawLines(lines);
}

// The Sonar-style volume line: a polyline over the waveform with square
// nodes; hotIndex (dragged/hovered node) is drawn emphasised.
void paintEnvelope(QPainter *painter, const QRect &waveRect, qint64 durationMs,
                   const QVector<QPointF> &env, int hotIndex, const QColor &hotPen)
{
    if (env.isEmpty())
        return;
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QColor envColor(80, 200, 120);

    QPolygonF line;
    line << QPointF(waveRect.left(), envNodeY(waveRect, env.first().y()));
    for (const QPointF &p : env)
        line << QPointF(envNodeX(waveRect, durationMs, p.x()),
                        envNodeY(waveRect, p.y()));
    line << QPointF(waveRect.right(), envNodeY(waveRect, env.last().y()));
    painter->setPen(QPen(envColor, 1.6));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(line);

    for (int i = 0; i < env.size(); ++i) {
        const QPointF c(envNodeX(waveRect, durationMs, env[i].x()),
                        envNodeY(waveRect, env[i].y()));
        const bool hot = (i == hotIndex);
        const double r = hot ? 4.0 : 2.8;
        painter->setPen(QPen(hot ? hotPen : envColor, 1));
        painter->setBrush(envColor);
        painter->drawRect(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r));
    }
    painter->setRenderHint(QPainter::Antialiasing, false);
}
} // namespace

qint64 PlaylistWaveView::s_maxOverlapMs = 25000;

qint64 PlaylistWaveView::maxOverlapMs()
{
    return s_maxOverlapMs;
}

void PlaylistWaveView::setMaxOverlapMs(qint64 ms)
{
    s_maxOverlapMs = qBound(qint64(5000), ms, qint64(300000));
}

QVector<QPointF> PlaylistWaveView::parseEnvelope(const QString &encoded)
{
    QVector<QPointF> points;
    const QStringList parts = encoded.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const int colon = part.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;
        bool okMs = false, okGain = false;
        const double ms = part.left(colon).toDouble(&okMs);
        const double gain = part.mid(colon + 1).toDouble(&okGain);
        if (okMs && okGain && ms >= 0)
            points.append(QPointF(ms, qBound(0.0, gain, 1.0)));
    }
    std::sort(points.begin(), points.end(),
              [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
    return points;
}

QString PlaylistWaveView::encodeEnvelope(const QVector<QPointF> &points)
{
    QStringList parts;
    parts.reserve(points.size());
    for (const QPointF &p : points)
        parts << QStringLiteral("%1:%2").arg(qint64(p.x()))
                     .arg(QString::number(p.y(), 'f', 3));
    return parts.join(QLatin1Char(';'));
}

double PlaylistWaveView::envelopeGainAt(const QVector<QPointF> &points, qint64 positionMs)
{
    if (points.isEmpty())
        return 1.0;
    if (positionMs <= points.first().x())
        return points.first().y();
    if (positionMs >= points.last().x())
        return points.last().y();
    for (int i = 1; i < points.size(); ++i) {
        if (positionMs <= points[i].x()) {
            const QPointF &a = points[i - 1];
            const QPointF &b = points[i];
            const double span = b.x() - a.x();
            if (span <= 0)
                return b.y();
            const double t = (positionMs - a.x()) / span;
            return a.y() + t * (b.y() - a.y());
        }
    }
    return points.last().y();
}

PlaylistWaveView::PlaylistWaveView(QListWidget *list, WaveformStore *store, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_list(list)
    , m_store(store)
{
    m_list->setItemDelegate(this);
    m_list->viewport()->installEventFilter(this);

    connect(m_store, &WaveformStore::waveformReady, this, [this](const QString &) {
        if (m_active)
            m_list->viewport()->update();
    });

    // New rows appear all the time (auto mode, drag & drop, context menus):
    // give them the tall size hint and fetch their waveforms.
    connect(m_list->model(), &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex &, int first, int last) {
        if (!m_active)
            return;
        applyRowSizeHints(first, last);
        requestVisibleWaveforms();
    });
    const auto cancelPreviewOnChange = [this]() { stopPreview(); };
    connect(m_list->model(), &QAbstractItemModel::rowsRemoved, this, cancelPreviewOnChange);
    connect(m_list->model(), &QAbstractItemModel::rowsMoved, this, cancelPreviewOnChange);
    connect(m_list->model(), &QAbstractItemModel::modelReset, this, cancelPreviewOnChange);

    // Transition audition players. Kept FX-free (passthrough) — this is a
    // local cue/preview, not the on-air chain.
    m_prevOutput = new QAudioOutput(this);
    m_nextOutput = new QAudioOutput(this);
    m_prevPlayer = new FxPlayer(this);
    m_nextPlayer = new FxPlayer(this);
    m_prevPlayer->setAudioOutput(m_prevOutput);
    m_nextPlayer->setAudioOutput(m_nextOutput);

    m_previewTimer = new QTimer(this);
    m_previewTimer->setInterval(50);
    connect(m_previewTimer, &QTimer::timeout, this, &PlaylistWaveView::previewTick);
}

void PlaylistWaveView::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;

    if (active) {
        m_hadMouseTracking = m_list->viewport()->hasMouseTracking();
        m_list->viewport()->setMouseTracking(true);
        requestVisibleWaveforms();
    } else {
        stopPreview();
        m_dragRow = -1;
        m_pressedPreviewRow = -1;
        m_envDragRow = -1;
        m_envDragIndex = -1;
        m_envSegRow = -1;
        m_envSegFirst = -1;
        m_envSegLast = -1;
        m_envSegStartGains.clear();
        m_envHoverRow = -1;
        m_envHoverIndex = -1;
        m_list->viewport()->setMouseTracking(m_hadMouseTracking);
        m_list->viewport()->unsetCursor();
    }

    // Setting the per-item size hint (or clearing it back to the default)
    // emits dataChanged, which is what makes the view relayout the rows.
    applyRowSizeHints(0, m_list->count() - 1);
    m_list->viewport()->update();
}

void PlaylistWaveView::setNowPlayingProvider(std::function<QString()> provider)
{
    m_nowPlaying = std::move(provider);
}

void PlaylistWaveView::refresh()
{
    if (!m_active)
        return;
    requestVisibleWaveforms();
    m_list->viewport()->update();
}

void PlaylistWaveView::applyRowSizeHints(int first, int last) const
{
    for (int row = first; row <= last; ++row) {
        if (QListWidgetItem *item = m_list->item(row))
            item->setSizeHint(m_active ? QSize(160, kRowHeight) : QSize());
    }
}

void PlaylistWaveView::requestVisibleWaveforms()
{
    for (int row = 0; row < m_list->count(); ++row)
        m_store->fetch(m_list->item(row)->text());
    if (m_nowPlaying) {
        const QString playing = m_nowPlaying();
        if (!playing.isEmpty())
            m_store->fetch(playing);
    }
}

QString PlaylistWaveView::previousTrackPath(int row) const
{
    if (row > 0)
        return m_list->item(row - 1)->text();
    return m_nowPlaying ? m_nowPlaying() : QString();
}

QRect PlaylistWaveView::fullWaveRect(const QRect &itemRect) const
{
    return QRect(itemRect.left() + kMargin,
                 itemRect.top() + kMargin + kTextHeight + 1,
                 itemRect.width() - 2 * kMargin,
                 kFullWaveHeight);
}

PlaylistWaveView::StripGeometry PlaylistWaveView::stripGeometry(const QRect &itemRect) const
{
    StripGeometry g;
    const int stripTop = itemRect.top() + kMargin + kTextHeight + kFullWaveHeight + 4;
    g.strip = QRect(itemRect.left() + kMargin, stripTop,
                    itemRect.width() - 2 * kMargin,
                    itemRect.bottom() - stripTop - kMargin + 1);
    g.previewBtn = QRect(g.strip.left() + 2,
                         g.strip.center().y() - kPreviewBtnSize / 2,
                         kPreviewBtnSize, kPreviewBtnSize);
    g.wave = g.strip.adjusted(kPreviewBtnSize + 8, 1, -1, -1);
    g.prevEndX = g.wave.left() + int(g.wave.width() * 0.45);
    // Lower bound keeps the scale finite when the playlist is very narrow
    g.pxPerMs = qMax(1e-4, double(g.prevEndX - g.wave.left()) / double(maxOverlapMs()));
    return g;
}

qint64 PlaylistWaveView::clampedOverlap(int row, qint64 overlapMs) const
{
    qint64 maxOverlap = maxOverlapMs();
    if (const WaveformData *prev = m_store->peek(previousTrackPath(row)))
        if (prev->ready())
            maxOverlap = qMin(maxOverlap, prev->durationMs);
    if (QListWidgetItem *item = m_list->item(row))
        if (const WaveformData *cur = m_store->peek(item->text()))
            if (cur->ready())
                maxOverlap = qMin(maxOverlap, cur->durationMs);
    return qBound<qint64>(0, overlapMs, maxOverlap);
}

void PlaylistWaveView::setRowOverlap(int row, qint64 overlapMs)
{
    if (QListWidgetItem *item = m_list->item(row))
        item->setData(OverlapRole, clampedOverlap(row, overlapMs));
}

void PlaylistWaveView::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (!m_active) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    const QRect rect = option.rect;
    const int row = index.row();
    const bool selected = option.state & QStyle::State_Selected;
    const QPalette &pal = option.palette;

    // Background
    if (selected)
        painter->fillRect(rect, pal.highlight());
    else
        painter->fillRect(rect, (row % 2) ? pal.alternateBase() : pal.base());
    painter->setPen(QPen(pal.mid().color(), 1));
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());

    const QColor textColor = selected ? pal.highlightedText().color() : pal.text().color();
    QColor subtleText = textColor;
    subtleText.setAlpha(170);

    const QString path = index.data(Qt::DisplayRole).toString();
    const qint64 overlapMs = index.data(OverlapRole).toLongLong();
    const WaveformData *cur = m_store->peek(path);

    const QString prevPath = previousTrackPath(row);
    const WaveformData *prev = prevPath.isEmpty() ? nullptr : m_store->peek(prevPath);

    // --- Title line ---
    QFont titleFont = option.font;
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(textColor);
    const QRect textRect(rect.left() + kMargin, rect.top() + kMargin,
                         rect.width() - 2 * kMargin, kTextHeight);
    QString durText;
    if (cur && cur->ready())
        durText = formatDuration(cur->durationMs);
    const int durWidth = durText.isEmpty() ? 0
        : painter->fontMetrics().horizontalAdvance(durText) + 8;
    painter->drawText(textRect.adjusted(0, 0, -durWidth, 0), Qt::AlignVCenter | Qt::AlignLeft,
                      painter->fontMetrics().elidedText(QFileInfo(path).fileName(),
                                                        Qt::ElideMiddle,
                                                        textRect.width() - durWidth));
    if (!durText.isEmpty()) {
        painter->setPen(subtleText);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, durText);
    }

    // --- Full-track waveform ---
    const QRect waveArea = fullWaveRect(rect);
    const QColor waveColor = selected ? pal.highlightedText().color()
                                      : QColor(96, 158, 214);
    if (cur && cur->ready()) {
        paintWaveform(painter, waveArea, *cur, 0, cur->durationMs, waveColor);

        // Mark where the NEXT track will come in over this one
        const QModelIndex nextIdx = index.sibling(row + 1, 0);
        const qint64 nextOverlap = nextIdx.isValid()
            ? nextIdx.data(OverlapRole).toLongLong() : 0;
        if (nextOverlap > 0 && cur->durationMs > 0) {
            const int x = waveArea.left()
                + int(double(waveArea.width())
                      * double(cur->durationMs - qMin(nextOverlap, cur->durationMs))
                      / double(cur->durationMs));
            QPen markPen(QColor(235, 165, 60), 1, Qt::DashLine);
            painter->setPen(markPen);
            painter->drawLine(x, waveArea.top(), x, waveArea.bottom());
        }

        // Volume line (Sonar-style envelope): a polyline over the waveform
        // with draggable nodes; the level holds flat before the first and
        // after the last node.
        const QVector<QPointF> env =
            parseEnvelope(index.data(VolumeEnvelopeRole).toString());
        if (!env.isEmpty()) {
            int hotIndex = -1;
            if (m_envDragRow == row)
                hotIndex = m_envDragIndex;
            else if (m_envDragRow < 0 && m_envHoverRow == row)
                hotIndex = m_envHoverIndex;
            paintEnvelope(painter, waveArea, cur->durationMs, env, hotIndex, textColor);
        }
    } else {
        painter->setFont(option.font);
        painter->setPen(subtleText);
        painter->drawText(waveArea, Qt::AlignCenter,
                          (cur && cur->failed)
                              ? tr("Waveform unavailable (ffmpeg not found or unreadable file)")
                              : tr("Analyzing waveform..."));
    }

    // --- Transition strip ---
    const StripGeometry g = stripGeometry(rect);

    painter->setPen(QPen(pal.mid().color(), 1));
    painter->setBrush((row % 2) ? pal.base() : pal.alternateBase());
    painter->drawRoundedRect(g.strip.adjusted(0, 0, -1, -1), 3, 3);

    // Audition (preview) button
    const bool previewing = (m_previewRow == row);
    const bool canPreview = prev && prev->ready();
    painter->setPen(QPen(pal.mid().color(), 1));
    painter->setBrush(previewing ? QColor(235, 165, 60)
                                 : pal.button());
    painter->drawEllipse(g.previewBtn.adjusted(1, 1, -1, -1));
    QColor glyphColor = canPreview ? textColor : subtleText;
    painter->setBrush(glyphColor);
    painter->setPen(Qt::NoPen);
    const QPoint c = g.previewBtn.center();
    if (previewing) {
        painter->drawRect(QRect(c.x() - 3, c.y() - 3, 7, 7)); // stop glyph
    } else {
        QPainterPath tri;
        tri.moveTo(c.x() - 3, c.y() - 4);
        tri.lineTo(c.x() + 5, c.y() + 0.5);
        tri.lineTo(c.x() - 3, c.y() + 5);
        tri.closeSubpath();
        painter->drawPath(tri);
    }

    // Ghost of the previous track's tail (everything left of prevEndX)
    if (prev && prev->ready()) {
        const qint64 tailMs = qint64((g.prevEndX - g.wave.left()) / g.pxPerMs);
        const qint64 fromMs = qMax<qint64>(0, prev->durationMs - tailMs);
        const int left = g.prevEndX - int((prev->durationMs - fromMs) * g.pxPerMs);
        const QRect ghostRect(left, g.wave.top() + 2,
                              g.prevEndX - left, g.wave.height() - 4);
        paintWaveform(painter, ghostRect, *prev, fromMs, prev->durationMs,
                 QColor(150, 150, 150, 140));
    }

    // "Previous track ends here" marker
    painter->setPen(QPen(QColor(220, 80, 80), 1));
    painter->drawLine(g.prevEndX, g.strip.top() + 2, g.prevEndX, g.strip.bottom() - 2);

    // This track's wave, shifted left by the overlap (an overlap beyond the
    // current Max overlap window pins to the strip's left edge)
    const int startX = qMax(g.wave.left(),
                            g.prevEndX - int(overlapMs * g.pxPerMs));
    if (overlapMs > 0)
        painter->fillRect(QRect(QPoint(startX, g.strip.top() + 2),
                                QPoint(g.prevEndX, g.strip.bottom() - 2)),
                          QColor(235, 165, 60, 40));
    if (cur && cur->ready()) {
        const qint64 visibleMs = qMin<qint64>(cur->durationMs,
                                              qint64((g.wave.right() - startX) / g.pxPerMs));
        if (visibleMs > 0) {
            const QRect inRect(startX, g.wave.top() + 2,
                               int(visibleMs * g.pxPerMs), g.wave.height() - 4);
            paintWaveform(painter, inRect, *cur, 0, visibleMs, QColor(235, 165, 60));
        }
    }

    // Audition playheads
    if (previewing) {
        painter->setPen(QPen(QColor(230, 60, 60), 1));
        if (m_prevPlayer->playbackState() == QMediaPlayer::PlayingState) {
            const int x = g.prevEndX
                - int((m_previewPrevDurMs - m_prevPlayer->position()) * g.pxPerMs);
            if (x >= g.wave.left() && x <= g.wave.right())
                painter->drawLine(x, g.strip.top() + 2, x, g.strip.bottom() - 2);
        }
        if (m_previewNextStarted
            && m_nextPlayer->playbackState() == QMediaPlayer::PlayingState) {
            const int x = startX + int(m_nextPlayer->position() * g.pxPerMs);
            if (x >= g.wave.left() && x <= g.wave.right())
                painter->drawLine(x, g.strip.top() + 2, x, g.strip.bottom() - 2);
        }
    }

    // Status label
    QFont smallFont = option.font;
    smallFont.setPointSizeF(qMax(7.0, option.font.pointSizeF() * 0.85));
    painter->setFont(smallFont);
    painter->setPen(subtleText);
    QString status;
    if (prevPath.isEmpty())
        status = tr("No previous track — plays as scheduled");
    else if (overlapMs > 0)
        status = tr("Starts %1 s before the previous track ends — drag the wave to adjust")
                     .arg(overlapMs / 1000.0, 0, 'f', 1);
    else
        status = tr("Starts when the previous track ends — drag the wave left to crossfade");
    painter->drawText(QRect(g.wave.left() + 2, g.strip.top() + 1,
                            g.wave.width() - 4, 12),
                      Qt::AlignLeft | Qt::AlignTop,
                      painter->fontMetrics().elidedText(status, Qt::ElideRight,
                                                        g.wave.width() - 4));

    painter->restore();
}

bool PlaylistWaveView::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_active || watched != m_list->viewport())
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton)
            return false;
        const QPoint pos = me->position().toPoint();
        QListWidgetItem *item = m_list->itemAt(pos);
        if (!item)
            return false;
        const int row = m_list->row(item);
        const QRect itemRect = m_list->visualItemRect(item);

        // Volume line: grab a node on the full waveform
        const QRect waveArea = fullWaveRect(itemRect);
        if (waveArea.contains(pos)) {
            const WaveformData *cur = m_store->peek(item->text());
            const QVector<QPointF> env =
                parseEnvelope(item->data(VolumeEnvelopeRole).toString());
            if (cur && cur->ready() && !env.isEmpty()) {
                const int idx = envelopeNodeHit(env, waveArea, cur->durationMs, pos);
                if (idx >= 0) {
                    m_list->setCurrentRow(row);
                    m_envDragRow = row;
                    m_envDragIndex = idx;
                    return true;
                }
                // Grabbing the line between nodes drags them together
                const SegmentHit seg =
                    envelopeSegmentAt(env, waveArea, cur->durationMs, pos);
                if (seg.first >= 0) {
                    m_list->setCurrentRow(row);
                    m_envSegRow = row;
                    m_envSegFirst = seg.first;
                    m_envSegLast = seg.last;
                    m_envSegStartY = pos.y();
                    m_envSegStartGains.clear();
                    for (int i = seg.first; i <= seg.last; ++i)
                        m_envSegStartGains.append(env[i].y());
                    return true;
                }
            }
            return false; // plain press on the wave keeps working as reorder/select
        }

        const StripGeometry g = stripGeometry(itemRect);
        if (g.previewBtn.contains(pos)) {
            m_pressedPreviewRow = row;
            return true;
        }
        if (g.wave.contains(pos)) {
            m_list->setCurrentRow(row);
            m_dragRow = row;
            m_dragStartX = pos.x();
            m_dragStartOverlap = item->data(OverlapRole).toLongLong();
            return true;
        }
        return false;
    }
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent *>(event);
        const QPoint pos = me->position().toPoint();
        if (m_dragRow >= 0 && (me->buttons() & Qt::LeftButton)) {
            if (QListWidgetItem *item = m_list->item(m_dragRow)) {
                const StripGeometry g =
                    stripGeometry(m_list->visualItemRect(item));
                const qint64 delta = qint64((m_dragStartX - pos.x()) / g.pxPerMs);
                setRowOverlap(m_dragRow, m_dragStartOverlap + delta);
                const qint64 now = item->data(OverlapRole).toLongLong();
                QToolTip::showText(me->globalPosition().toPoint(),
                                   now > 0
                                       ? tr("Overlap: %1 s").arg(now / 1000.0, 0, 'f', 1)
                                       : tr("No overlap"),
                                   m_list->viewport());
            }
            return true;
        }
        // Volume line: drag a node (time between its neighbours, gain 0..1)
        if (m_envDragRow >= 0 && (me->buttons() & Qt::LeftButton)) {
            QListWidgetItem *item = m_list->item(m_envDragRow);
            const WaveformData *cur = item ? m_store->peek(item->text()) : nullptr;
            if (item && cur && cur->ready()) {
                QVector<QPointF> env =
                    parseEnvelope(item->data(VolumeEnvelopeRole).toString());
                if (m_envDragIndex >= 0 && m_envDragIndex < env.size()) {
                    const QRect waveArea =
                        fullWaveRect(m_list->visualItemRect(item));
                    double ms = double(pos.x() - waveArea.left())
                                / qMax(1, waveArea.width()) * cur->durationMs;
                    const double lo = (m_envDragIndex > 0)
                        ? env[m_envDragIndex - 1].x() + 1 : 0.0;
                    const double hi = (m_envDragIndex < env.size() - 1)
                        ? env[m_envDragIndex + 1].x() - 1 : double(cur->durationMs);
                    ms = qBound(lo, ms, qMax(lo, hi));
                    const double gain = qBound(0.0,
                        1.0 - double(pos.y() - waveArea.top())
                              / qMax(1, waveArea.height() - 1), 1.0);
                    env[m_envDragIndex] = QPointF(ms, gain);
                    item->setData(VolumeEnvelopeRole, encodeEnvelope(env));
                    QToolTip::showText(me->globalPosition().toPoint(),
                                       tr("Volume %1% at %2")
                                           .arg(qRound(gain * 100))
                                           .arg(formatDuration(qint64(ms))),
                                       m_list->viewport());
                }
            }
            return true;
        }
        // Volume line: drag a whole segment vertically (both end nodes move)
        if (m_envSegRow >= 0 && (me->buttons() & Qt::LeftButton)) {
            QListWidgetItem *item = m_list->item(m_envSegRow);
            const WaveformData *cur = item ? m_store->peek(item->text()) : nullptr;
            if (item && cur && cur->ready()) {
                QVector<QPointF> env =
                    parseEnvelope(item->data(VolumeEnvelopeRole).toString());
                if (m_envSegLast < env.size()
                        && m_envSegLast - m_envSegFirst + 1 == m_envSegStartGains.size()) {
                    const QRect waveArea =
                        fullWaveRect(m_list->visualItemRect(item));
                    const double delta = -double(pos.y() - m_envSegStartY)
                                         / qMax(1, waveArea.height() - 1);
                    applySegmentDelta(env, m_envSegFirst, m_envSegLast,
                                      m_envSegStartGains, delta);
                    item->setData(VolumeEnvelopeRole, encodeEnvelope(env));
                    const int loPct = qRound(env[m_envSegFirst].y() * 100);
                    const int hiPct = qRound(env[m_envSegLast].y() * 100);
                    QToolTip::showText(me->globalPosition().toPoint(),
                                       loPct == hiPct
                                           ? tr("Volume %1%").arg(loPct)
                                           : tr("Volume %1% – %2%").arg(loPct).arg(hiPct),
                                       m_list->viewport());
                }
            }
            return true;
        }
        // Hover feedback
        const int oldHoverRow = m_envHoverRow;
        const int oldHoverIndex = m_envHoverIndex;
        m_envHoverRow = -1;
        m_envHoverIndex = -1;
        if (QListWidgetItem *item = m_list->itemAt(pos)) {
            const QRect itemRect = m_list->visualItemRect(item);
            const QRect waveArea = fullWaveRect(itemRect);
            const StripGeometry g = stripGeometry(itemRect);
            if (waveArea.contains(pos)) {
                const WaveformData *cur = m_store->peek(item->text());
                const QVector<QPointF> env =
                    parseEnvelope(item->data(VolumeEnvelopeRole).toString());
                if (cur && cur->ready() && !env.isEmpty()) {
                    const int idx = envelopeNodeHit(env, waveArea, cur->durationMs, pos);
                    if (idx >= 0) {
                        m_envHoverRow = m_list->row(item);
                        m_envHoverIndex = idx;
                        m_list->viewport()->setCursor(Qt::PointingHandCursor);
                    } else if (envelopeSegmentAt(env, waveArea, cur->durationMs,
                                                 pos).first >= 0) {
                        // On the line between nodes: vertical segment drag
                        m_list->viewport()->setCursor(Qt::SizeVerCursor);
                    } else {
                        m_list->viewport()->setCursor(Qt::CrossCursor);
                    }
                } else {
                    m_list->viewport()->unsetCursor();
                }
            } else if (g.previewBtn.contains(pos)) {
                m_list->viewport()->setCursor(Qt::PointingHandCursor);
            } else if (g.wave.contains(pos)) {
                m_list->viewport()->setCursor(Qt::SizeHorCursor);
            } else {
                m_list->viewport()->unsetCursor();
            }
            if (oldHoverRow != m_envHoverRow || oldHoverIndex != m_envHoverIndex)
                m_list->viewport()->update(itemRect);
        } else {
            m_list->viewport()->unsetCursor();
        }
        if (oldHoverRow >= 0 && oldHoverRow != m_envHoverRow) {
            if (QListWidgetItem *old = m_list->item(oldHoverRow))
                m_list->viewport()->update(m_list->visualItemRect(old));
        }
        return false;
    }
    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton)
            return false;
        const QPoint pos = me->position().toPoint();
        bool handled = false;
        if (m_pressedPreviewRow >= 0) {
            if (QListWidgetItem *item = m_list->item(m_pressedPreviewRow)) {
                const StripGeometry g =
                    stripGeometry(m_list->visualItemRect(item));
                if (g.previewBtn.contains(pos))
                    togglePreview(m_pressedPreviewRow);
            }
            m_pressedPreviewRow = -1;
            handled = true;
        }
        if (m_dragRow >= 0) {
            m_dragRow = -1;
            handled = true;
        }
        if (m_envDragRow >= 0) {
            m_envDragRow = -1;
            m_envDragIndex = -1;
            handled = true;
        }
        if (m_envSegRow >= 0) {
            m_envSegRow = -1;
            m_envSegFirst = -1;
            m_envSegLast = -1;
            m_envSegStartGains.clear();
            handled = true;
        }
        return handled;
    }
    case QEvent::MouseButtonDblClick: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton)
            return false;
        const QPoint pos = me->position().toPoint();
        if (QListWidgetItem *item = m_list->itemAt(pos)) {
            const QRect itemRect = m_list->visualItemRect(item);

            // Volume line: double-click adds a node; double-click on a
            // node resets it to 0 dB (right-click on a node removes it)
            const QRect waveArea = fullWaveRect(itemRect);
            if (waveArea.contains(pos)) {
                const WaveformData *cur = m_store->peek(item->text());
                QVector<QPointF> env =
                    parseEnvelope(item->data(VolumeEnvelopeRole).toString());
                if (cur && cur->ready() && !env.isEmpty()) {
                    const int idx = envelopeNodeHit(env, waveArea, cur->durationMs, pos);
                    if (idx >= 0) {
                        env[idx].setY(1.0);
                        item->setData(VolumeEnvelopeRole, encodeEnvelope(env));
                    } else {
                        const double ms = double(pos.x() - waveArea.left())
                                          / qMax(1, waveArea.width()) * cur->durationMs;
                        const double gain = qBound(0.0,
                            1.0 - double(pos.y() - waveArea.top())
                                  / qMax(1, waveArea.height() - 1), 1.0);
                        int insertAt = 0;
                        while (insertAt < env.size() && env[insertAt].x() < ms)
                            ++insertAt;
                        env.insert(insertAt, QPointF(ms, gain));
                        item->setData(VolumeEnvelopeRole, encodeEnvelope(env));
                    }
                    return true;
                }
                return false;
            }

            const StripGeometry g = stripGeometry(itemRect);
            if (g.wave.contains(pos)) {
                setRowOverlap(m_list->row(item), 0); // double-click resets
                return true;
            }
        }
        return false;
    }
    case QEvent::ContextMenu: {
        // Right-click on a volume-line node deletes it (removing the last
        // node removes the line); elsewhere the playlist menu opens normally
        auto *ce = static_cast<QContextMenuEvent *>(event);
        const QPoint pos = ce->pos();
        QListWidgetItem *item = m_list->itemAt(pos);
        if (!item)
            return false;
        const QRect waveArea = fullWaveRect(m_list->visualItemRect(item));
        if (!waveArea.contains(pos))
            return false;
        const WaveformData *cur = m_store->peek(item->text());
        QVector<QPointF> env =
            parseEnvelope(item->data(VolumeEnvelopeRole).toString());
        if (cur && cur->ready() && !env.isEmpty()) {
            const int idx = envelopeNodeHit(env, waveArea, cur->durationMs, pos);
            if (idx >= 0) {
                env.removeAt(idx);
                item->setData(VolumeEnvelopeRole,
                              env.isEmpty() ? QVariant()
                                            : QVariant(encodeEnvelope(env)));
                return true;
            }
        }
        return false;
    }
    case QEvent::Leave:
        m_list->viewport()->unsetCursor();
        return false;
    default:
        return false;
    }
}

void PlaylistWaveView::togglePreview(int row)
{
    if (m_previewRow == row)
        stopPreview();
    else
        startPreview(row);
}

void PlaylistWaveView::startPreview(int row)
{
    stopPreview();

    QListWidgetItem *item = m_list->item(row);
    if (!item)
        return;
    const QString prevPath = previousTrackPath(row);
    const WaveformData *prev = prevPath.isEmpty() ? nullptr : m_store->peek(prevPath);
    if (!prev || !prev->ready())
        return; // nothing to audition against yet

    m_previewOverlapMs = clampedOverlap(row, item->data(OverlapRole).toLongLong());
    m_previewPrevDurMs = prev->durationMs;
    m_previewRow = row;
    m_previewNextStarted = false;

    // Honour the tracks' volume lines during the audition (the previous
    // track of row 0 is the on-air one, whose item is gone — skip it)
    m_previewPrevEnv = (row > 0 && m_list->item(row - 1))
        ? parseEnvelope(m_list->item(row - 1)->data(VolumeEnvelopeRole).toString())
        : QVector<QPointF>();
    m_previewNextEnv = parseEnvelope(item->data(VolumeEnvelopeRole).toString());

    const qint64 startA =
        qMax<qint64>(0, m_previewPrevDurMs - m_previewOverlapMs - kPreviewLeadMs);
    m_prevOutput->setVolume(kPreviewVolume);
    m_nextOutput->setVolume(kPreviewVolume);
    m_prevPlayer->setSource(QUrl::fromLocalFile(prevPath));
    m_prevPlayer->play();
    m_prevPlayer->setPosition(startA);
    m_nextPlayer->setSource(QUrl::fromLocalFile(item->text())); // preloaded, starts later

    m_previewTimer->start();
    m_list->viewport()->update();
}

void PlaylistWaveView::previewTick()
{
    if (m_previewRow < 0) {
        m_previewTimer->stop();
        return;
    }

    const bool prevPlaying =
        m_prevPlayer->playbackState() == QMediaPlayer::PlayingState;
    const qint64 remaining = m_previewPrevDurMs - m_prevPlayer->position();

    if (!m_previewNextStarted && (!prevPlaying || remaining <= m_previewOverlapMs)) {
        m_nextPlayer->play();
        m_previewNextStarted = true;
    }

    // Fade the outgoing track across the overlap, like the real segue does,
    // shaped by its volume line if it has one
    if (prevPlaying) {
        double f = 1.0;
        if (m_previewOverlapMs > 0 && remaining <= m_previewOverlapMs)
            f = qBound(0.0, double(remaining) / double(m_previewOverlapMs), 1.0);
        const double gainPrev = m_previewPrevEnv.isEmpty()
            ? 1.0 : envelopeGainAt(m_previewPrevEnv, m_prevPlayer->position());
        m_prevOutput->setVolume(kPreviewVolume * f * gainPrev);
    }

    const bool nextPlaying =
        m_nextPlayer->playbackState() == QMediaPlayer::PlayingState;
    if (m_previewNextStarted && nextPlaying && !m_previewNextEnv.isEmpty())
        m_nextOutput->setVolume(kPreviewVolume
                                * envelopeGainAt(m_previewNextEnv,
                                                 m_nextPlayer->position()));
    if (m_previewNextStarted
        && (!nextPlaying || m_nextPlayer->position() >= m_previewOverlapMs + kPreviewTailMs)) {
        stopPreview();
        return;
    }

    if (QListWidgetItem *item = m_list->item(m_previewRow))
        m_list->viewport()->update(m_list->visualItemRect(item));
}

void PlaylistWaveView::stopPreview()
{
    m_previewTimer->stop();
    if (m_previewRow < 0)
        return;
    m_previewRow = -1;
    m_previewNextStarted = false;
    m_prevPlayer->stop();
    m_nextPlayer->stop();
    m_list->viewport()->update();
}

// ---------------------------------------------------------------------------
// NowPlayingWaveStrip
// ---------------------------------------------------------------------------

NowPlayingWaveStrip::NowPlayingWaveStrip(WaveformStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setFixedHeight(kMargin + kTextHeight + kFullWaveHeight + kMargin + 1);
    setMouseTracking(true);
    setVisible(false);
    connect(m_store, &WaveformStore::waveformReady, this, [this](const QString &path) {
        if (path == m_path)
            update();
    });
}

void NowPlayingWaveStrip::setTrack(const QString &filePath)
{
    if (m_path == filePath)
        return;
    m_path = filePath;
    m_env.clear(); // the player pushes the track's envelope right after
    m_positionMs = 0;
    m_dragNode = -1;
    m_hoverNode = -1;
    m_segFirst = -1;
    m_segLast = -1;
    m_segStartGains.clear();
    if (!m_path.isEmpty())
        m_store->fetch(m_path);
    update();
}

void NowPlayingWaveStrip::setEnvelope(const QVector<QPointF> &points)
{
    m_env = points;
    update();
}

void NowPlayingWaveStrip::setPlayhead(qint64 positionMs)
{
    if (m_positionMs == positionMs)
        return;
    m_positionMs = positionMs;
    if (isVisible())
        update();
}

QRect NowPlayingWaveStrip::waveRect() const
{
    return QRect(kMargin, kMargin + kTextHeight + 1,
                 width() - 2 * kMargin, kFullWaveHeight);
}

qint64 NowPlayingWaveStrip::durationMs() const
{
    const WaveformData *data = m_store->peek(m_path);
    return (data && data->ready()) ? data->durationMs : 0;
}

void NowPlayingWaveStrip::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const QPalette &pal = palette();
    painter.fillRect(rect(), pal.base());
    painter.setPen(QPen(pal.mid().color(), 1));
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());

    if (m_path.isEmpty())
        return;

    QColor subtleText = pal.text().color();
    subtleText.setAlpha(170);

    const WaveformData *data = m_store->peek(m_path);
    const bool ready = data && data->ready();

    QFont titleFont = font();
    titleFont.setBold(true);
    painter.setFont(titleFont);
    const QRect textR(kMargin, kMargin, width() - 2 * kMargin, kTextHeight);
    const QString timeText = ready
        ? formatDuration(m_positionMs) + QStringLiteral(" / ")
              + formatDuration(data->durationMs)
        : QString();
    const int timeW = timeText.isEmpty() ? 0
        : painter.fontMetrics().horizontalAdvance(timeText) + 8;
    painter.setPen(pal.text().color());
    painter.drawText(textR.adjusted(0, 0, -timeW, 0),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     painter.fontMetrics().elidedText(
                         tr("Now playing: %1").arg(QFileInfo(m_path).fileName()),
                         Qt::ElideMiddle, textR.width() - timeW));
    if (!timeText.isEmpty()) {
        painter.setPen(subtleText);
        painter.drawText(textR, Qt::AlignVCenter | Qt::AlignRight, timeText);
    }

    const QRect r = waveRect();
    if (ready) {
        paintWaveform(&painter, r, *data, 0, data->durationMs, QColor(96, 158, 214));
        paintEnvelope(&painter, r, data->durationMs, m_env,
                      m_dragNode >= 0 ? m_dragNode : m_hoverNode,
                      pal.text().color());

        // Playhead
        const int x = r.left()
            + int(double(r.width()) * m_positionMs
                  / double(qMax<qint64>(1, data->durationMs)));
        painter.setPen(QPen(QColor(230, 60, 60), 1));
        painter.drawLine(x, r.top(), x, r.bottom());
    } else {
        painter.setFont(font());
        painter.setPen(subtleText);
        painter.drawText(r, Qt::AlignCenter,
                         (data && data->failed)
                             ? tr("Waveform unavailable (ffmpeg not found or unreadable file)")
                             : tr("Analyzing waveform..."));
    }
}

void NowPlayingWaveStrip::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return QWidget::mousePressEvent(event);
    const qint64 dur = durationMs();
    const QRect r = waveRect();
    const QPoint pos = event->position().toPoint();
    if (dur <= 0 || m_env.isEmpty() || !r.contains(pos))
        return;

    const int idx = envelopeNodeHit(m_env, r, dur, pos);
    if (idx >= 0) {
        m_dragNode = idx;
        update();
        return;
    }
    const SegmentHit seg = envelopeSegmentAt(m_env, r, dur, pos);
    if (seg.first >= 0) {
        m_segFirst = seg.first;
        m_segLast = seg.last;
        m_segStartY = pos.y();
        m_segStartGains.clear();
        for (int i = seg.first; i <= seg.last; ++i)
            m_segStartGains.append(m_env[i].y());
    }
}

void NowPlayingWaveStrip::mouseMoveEvent(QMouseEvent *event)
{
    const qint64 dur = durationMs();
    const QRect r = waveRect();
    const QPoint pos = event->position().toPoint();

    if (m_dragNode >= 0 && (event->buttons() & Qt::LeftButton)) {
        if (dur > 0 && m_dragNode < m_env.size()) {
            double ms = double(pos.x() - r.left()) / qMax(1, r.width()) * dur;
            const double lo = (m_dragNode > 0) ? m_env[m_dragNode - 1].x() + 1 : 0.0;
            const double hi = (m_dragNode < m_env.size() - 1)
                ? m_env[m_dragNode + 1].x() - 1 : double(dur);
            ms = qBound(lo, ms, qMax(lo, hi));
            const double gain = qBound(0.0,
                1.0 - double(pos.y() - r.top()) / qMax(1, r.height() - 1), 1.0);
            m_env[m_dragNode] = QPointF(ms, gain);
            emit envelopeEdited(m_env);
            QToolTip::showText(event->globalPosition().toPoint(),
                               tr("Volume %1% at %2").arg(qRound(gain * 100))
                                   .arg(formatDuration(qint64(ms))), this);
            update();
        }
        return;
    }
    if (m_segFirst >= 0 && (event->buttons() & Qt::LeftButton)) {
        if (m_segLast < m_env.size()
                && m_segLast - m_segFirst + 1 == m_segStartGains.size()) {
            const double delta = -double(pos.y() - m_segStartY)
                                 / qMax(1, r.height() - 1);
            applySegmentDelta(m_env, m_segFirst, m_segLast, m_segStartGains, delta);
            emit envelopeEdited(m_env);
            const int loPct = qRound(m_env[m_segFirst].y() * 100);
            const int hiPct = qRound(m_env[m_segLast].y() * 100);
            QToolTip::showText(event->globalPosition().toPoint(),
                               loPct == hiPct
                                   ? tr("Volume %1%").arg(loPct)
                                   : tr("Volume %1% – %2%").arg(loPct).arg(hiPct),
                               this);
            update();
        }
        return;
    }

    // Hover feedback
    const int oldHover = m_hoverNode;
    m_hoverNode = -1;
    if (dur > 0 && r.contains(pos)) {
        if (!m_env.isEmpty()) {
            const int idx = envelopeNodeHit(m_env, r, dur, pos);
            if (idx >= 0) {
                m_hoverNode = idx;
                setCursor(Qt::PointingHandCursor);
            } else if (envelopeSegmentAt(m_env, r, dur, pos).first >= 0) {
                setCursor(Qt::SizeVerCursor);
            } else {
                setCursor(Qt::CrossCursor);
            }
        } else {
            setCursor(Qt::CrossCursor); // double-click adds a volume line
        }
    } else {
        unsetCursor();
    }
    if (oldHover != m_hoverNode)
        update();
}

void NowPlayingWaveStrip::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragNode = -1;
    m_segFirst = -1;
    m_segLast = -1;
    m_segStartGains.clear();
    update();
}

void NowPlayingWaveStrip::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return QWidget::mouseDoubleClickEvent(event);
    const qint64 dur = durationMs();
    const QRect r = waveRect();
    const QPoint pos = event->position().toPoint();
    if (dur <= 0 || !r.contains(pos))
        return;

    if (!m_env.isEmpty()) {
        const int idx = envelopeNodeHit(m_env, r, dur, pos);
        if (idx >= 0) {
            m_env[idx].setY(1.0); // back to 0 dB (right-click removes it)
            emit envelopeEdited(m_env);
            update();
            return;
        }
    }
    const double ms = double(pos.x() - r.left()) / qMax(1, r.width()) * dur;
    const double gain = qBound(0.0,
        1.0 - double(pos.y() - r.top()) / qMax(1, r.height() - 1), 1.0);
    int insertAt = 0;
    while (insertAt < m_env.size() && m_env[insertAt].x() < ms)
        ++insertAt;
    m_env.insert(insertAt, QPointF(ms, gain));
    emit envelopeEdited(m_env);
    update();
}

void NowPlayingWaveStrip::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_path.isEmpty())
        return;
    const qint64 dur = durationMs();
    const QRect r = waveRect();
    const QPoint pos = event->pos();

    // Right-click on a node deletes it (removing the last one removes the line)
    if (dur > 0 && !m_env.isEmpty() && r.contains(pos)) {
        const int idx = envelopeNodeHit(m_env, r, dur, pos);
        if (idx >= 0) {
            m_env.removeAt(idx);
            emit envelopeEdited(m_env);
            update();
            return;
        }
    }

    QMenu menu(this);
    QAction *addAction = nullptr;
    QAction *resetAction = nullptr;
    QAction *removeAction = nullptr;
    if (m_env.isEmpty()) {
        addAction = menu.addAction(tr("Add a volume line"));
    } else {
        resetAction = menu.addAction(tr("Reset the volume line"));
        removeAction = menu.addAction(tr("Remove the volume line"));
    }
    QAction *chosen = menu.exec(event->globalPos());
    if (!chosen)
        return;
    if (chosen == addAction || chosen == resetAction)
        m_env = {QPointF(0.0, 1.0)}; // flat 0 dB line
    else if (chosen == removeAction)
        m_env.clear();
    emit envelopeEdited(m_env);
    update();
}

void NowPlayingWaveStrip::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hoverNode = -1;
    unsetCursor();
    update();
}
