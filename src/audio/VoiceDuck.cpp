#include "VoiceDuck.h"

#include "IntroDetector.h"
#include "WaveformStore.h"

#include <algorithm>

namespace
{
constexpr int kMs = WaveformStore::MsPerPeak;

/** Linear interpolation over a raw (unclipped) node list, extended flat. */
double gainAt(const QVector<QPointF> &nodes, double ms)
{
    if (nodes.isEmpty())
        return 1.0;
    if (ms <= nodes.first().x())
        return nodes.first().y();
    if (ms >= nodes.last().x())
        return nodes.last().y();
    for (int i = 1; i < nodes.size(); ++i) {
        if (ms <= nodes[i].x()) {
            const double span = nodes[i].x() - nodes[i - 1].x();
            if (span <= 0.0)
                return nodes[i].y();
            const double t = (ms - nodes[i - 1].x()) / span;
            return nodes[i - 1].y() + t * (nodes[i].y() - nodes[i - 1].y());
        }
    }
    return nodes.last().y();
}
} // namespace

QVector<VoiceDuck::Span> VoiceDuck::voiceSpans(const WaveformData &voice,
                                               const Params &params)
{
    QVector<Span> spans;
    if (!voice.ready())
        return spans;

    // The same threshold rule the intro detector and the auto-mix use: a
    // percentage of the file's OWN maximum peak, so a quietly recorded take
    // and a hot one behave the same. -1 means the whole file is below the
    // "there is audio here at all" floor.
    const int threshold = IntroDetector::quietThreshold(voice, params.thresholdPercent);
    if (threshold < 0)
        return spans;

    // 1. Raw runs of loud buckets.
    const int n = voice.peaks.size();
    int runStart = -1;
    for (int i = 0; i <= n; ++i) {
        const bool loud = (i < n) && (int(voice.peaks[i]) >= threshold);
        if (loud && runStart < 0) {
            runStart = i;
        } else if (!loud && runStart >= 0) {
            spans.append({qint64(runStart) * kMs, qint64(i) * kMs});
            runStart = -1;
        }
    }
    if (spans.isEmpty())
        return spans;

    // 2. Bridge the pauses between words and sentences before anything else:
    //    a run that is too short on its own may well be a syllable that
    //    belongs to the sentence next to it.
    QVector<Span> merged;
    merged.append(spans.first());
    for (int i = 1; i < spans.size(); ++i) {
        if (spans[i].startMs - merged.last().endMs < params.minGapMs)
            merged.last().endMs = spans[i].endMs;
        else
            merged.append(spans[i]);
    }

    // 3. Now drop what is still too short to be speech (a click, a chair).
    spans.clear();
    for (const Span &s : std::as_const(merged))
        if (s.endMs - s.startMs >= params.minRunMs)
            spans.append(s);
    if (spans.isEmpty())
        return spans;

    // 4. Pad, then merge again: the padding can make two spans touch.
    for (Span &s : spans) {
        s.startMs = qMax<qint64>(0, s.startMs - params.leadInMs);
        s.endMs += params.holdMs;
    }
    merged.clear();
    merged.append(spans.first());
    for (int i = 1; i < spans.size(); ++i) {
        // Closer than two fades apart and the ramp back up would cross the
        // ramp back down — one duck, not two.
        if (spans[i].startMs - merged.last().endMs < 2 * qint64(params.fadeMs))
            merged.last().endMs = qMax(merged.last().endMs, spans[i].endMs);
        else
            merged.append(spans[i]);
    }
    return merged;
}

QVector<QPointF> VoiceDuck::envelopeFromSpans(const QVector<Span> &spans,
                                              qint64 voiceStartMs,
                                              qint64 trackDurationMs,
                                              const Params &params)
{
    QVector<QPointF> out;
    if (spans.isEmpty() || trackDurationMs <= 0)
        return out;

    const double duck = qBound(0.0, params.duckGain, 1.0);
    const double fade = qMax(1, params.fadeMs);

    // Raw nodes on the music track's own timeline; some may fall outside it.
    QVector<QPointF> raw;
    bool touches = false;
    for (const Span &s : spans) {
        const double a = double(voiceStartMs + s.startMs);
        const double b = double(voiceStartMs + s.endMs);
        // Skip a span that misses this track entirely (fade tails included).
        if (b + fade <= 0.0 || a - fade >= double(trackDurationMs))
            continue;
        touches = true;
        raw.append(QPointF(a - fade, 1.0));
        raw.append(QPointF(a, duck));
        raw.append(QPointF(b, duck));
        raw.append(QPointF(b + fade, 1.0));
    }
    if (!touches)
        return out;

    // Clip to [0, duration]. envelopeGainAt() holds the first node's value
    // before it and the last node's value after it, so a duck that is already
    // under way when the track starts just needs its level at t = 0, and one
    // that finishes before the end needs nothing after its ramp back up.
    const double g0 = gainAt(raw, 0.0);
    if (raw.first().x() > 0.0) {
        // Nothing ducked yet at t = 0: an explicit unity node keeps the line
        // readable in the wave view (and flat, not sloping in from nowhere).
        out.append(QPointF(0.0, 1.0));
    } else {
        out.append(QPointF(0.0, g0));
    }

    for (const QPointF &p : std::as_const(raw)) {
        if (p.x() <= 0.0 || p.x() >= double(trackDurationMs))
            continue;
        // Two nodes landing on the same millisecond after rounding would be a
        // vertical step the operator cannot grab; keep the later one.
        if (!out.isEmpty() && qint64(out.last().x()) == qint64(p.x()))
            out.last().setY(p.y());
        else
            out.append(p);
    }

    const double gEnd = gainAt(raw, double(trackDurationMs));
    if (gEnd < 1.0 - 1e-6) {
        // The duck runs past the end of this track (the link is still going
        // when the outgoing song finishes): hold it rather than letting the
        // line spring back up on the last node.
        if (!out.isEmpty() && qint64(out.last().x()) == trackDurationMs)
            out.last().setY(gEnd);
        else
            out.append(QPointF(double(trackDurationMs), gEnd));
    }

    // A line that never leaves unity is not a duck; do not write one.
    bool anyDuck = false;
    for (const QPointF &p : std::as_const(out))
        if (p.y() < 1.0 - 1e-6)
            anyDuck = true;
    if (!anyDuck)
        out.clear();
    return out;
}

QVector<QPointF> VoiceDuck::envelopeForTrack(const WaveformData &voice,
                                             qint64 voiceStartMs,
                                             qint64 trackDurationMs,
                                             const Params &params)
{
    return envelopeFromSpans(voiceSpans(voice, params), voiceStartMs,
                             trackDurationMs, params);
}
