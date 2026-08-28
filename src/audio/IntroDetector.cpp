#include "IntroDetector.h"

#include "WaveformStore.h"

#include <QVector>

#include <algorithm>

namespace
{
constexpr int kMs = WaveformStore::MsPerPeak;

/** Buckets covering `ms`, at least one. */
inline int bucketsFor(int ms)
{
    return qMax(1, ms / kMs);
}

/** Centred moving average of the peak envelope, window `window` buckets.
 *  Computed off a prefix sum so a long track costs one pass, not one pass
 *  per bucket — a library sweep runs this over every file. */
QVector<double> smooth(const QVector<quint8> &peaks, int window)
{
    const int n = peaks.size();
    QVector<double> out(n, 0.0);
    if (n == 0)
        return out;

    QVector<double> prefix(n + 1, 0.0);
    for (int i = 0; i < n; ++i)
        prefix[i + 1] = prefix[i] + double(peaks[i]);

    const int half = qMax(1, window / 2);
    for (int i = 0; i < n; ++i) {
        const int lo = qMax(0, i - half);
        const int hi = qMin(n, i + half + 1);
        out[i] = (prefix[hi] - prefix[lo]) / double(hi - lo);
    }
    return out;
}

/** `q`-quantile (0..1) of the values in [from, to). Copies, because
 *  nth_element reorders — the caller still needs the curve in order. */
double quantile(const QVector<double> &values, int from, int to, double q)
{
    if (to <= from)
        return 0.0;
    QVector<double> slice(values.begin() + from, values.begin() + to);
    const int k = qBound(0, int(q * (slice.size() - 1)), slice.size() - 1);
    std::nth_element(slice.begin(), slice.begin() + k, slice.end());
    return slice[k];
}
} // namespace

int IntroDetector::quietThreshold(const WaveformData &data, int thresholdPercent)
{
    int maxPeak = 0;
    for (quint8 p : data.peaks)
        maxPeak = qMax(maxPeak, int(p));
    if (maxPeak < kMinLoudPeak)
        return -1;
    return qMax(1, maxPeak * thresholdPercent / 100);
}

qint64 IntroDetector::quietHeadMs(const WaveformData &data, int thresholdPercent)
{
    const int threshold = quietThreshold(data, thresholdPercent);
    if (threshold < 0)
        return data.durationMs;
    int run = 0;
    for (int i = 0; i < data.peaks.size(); ++i) {
        run = data.peaks[i] >= threshold ? run + 1 : 0;
        if (run >= kSustainPeaks)
            return qint64(i - kSustainPeaks + 1) * kMs;
    }
    return data.durationMs;
}

qint64 IntroDetector::quietTailMs(const WaveformData &data, int thresholdPercent)
{
    const int threshold = quietThreshold(data, thresholdPercent);
    if (threshold < 0)
        return data.durationMs;
    int run = 0;
    for (int i = data.peaks.size() - 1; i >= 0; --i) {
        run = data.peaks[i] >= threshold ? run + 1 : 0;
        if (run >= kSustainPeaks) {
            // Scanning backwards, so this is the last sustained-loud run in
            // the track and it ends just before peak index i + kSustainPeaks.
            // Any rounding remainder past the peak buffer is quiet too.
            const qint64 loudEndMs = qint64(i + kSustainPeaks) * kMs;
            return qMax(qint64(0), data.durationMs - loudEndMs);
        }
    }
    return data.durationMs;
}

IntroPoints IntroDetector::detect(const WaveformData &data, int thresholdPercent)
{
    IntroPoints out;
    if (!data.ready() || data.durationMs <= 0)
        return out; // nothing decoded: leave it unmeasured, do not guess

    const int n = data.peaks.size();
    if (quietThreshold(data, thresholdPercent) < 0)
        return out; // silent all the way through

    // 1. Trim the edges with the same rule the crossfade auto-mix uses.
    const qint64 headQuiet = quietHeadMs(data, thresholdPercent);
    const qint64 tailQuiet = quietTailMs(data, thresholdPercent);
    const int headIdx = qBound(0, int(headQuiet / kMs), n - 1);
    const int tailIdx = qBound(headIdx + 1, n - int(tailQuiet / kMs), n);
    if (tailIdx - headIdx < kSustainPeaks)
        return out; // a click and nothing else

    out.firstSoundMs = qint64(headIdx) * kMs;
    out.valid = true;

    // 2/3. Smooth, then ask how loud this record is when it is going. The
    // 80th percentile rather than the maximum: one mastering overshoot or
    // one cymbal crash must not define "full level" for the whole track.
    const QVector<double> level = smooth(data.peaks, bucketsFor(kSmoothMs));
    const double body = quantile(level, headIdx, tailIdx, 0.80);
    if (body <= 0.0) {
        out.introMs = out.firstSoundMs;
        out.noRamp = true;
        return out;
    }

    // 4. The opening plateau's own level: the median over the first few
    // seconds of actual sound. Median, not mean, so a stab at the top of
    // the intro does not raise the bar the vocal has to clear.
    const int plateauEnd = qMin(tailIdx, headIdx + bucketsFor(kPlateauWindowMs));
    const double plateau = quantile(level, headIdx, plateauEnd, 0.50);

    // A record that opens at (near) full level has no ramp at all. Saying
    // so is worth more to the operator than a number invented from noise.
    if (plateau >= 0.80 * body) {
        out.startsLoud = true;
        out.introMs = out.firstSoundMs;
    } else {
        // 5. The vocal has to clear half the distance from the plateau to
        // the body AND be a decent fraction of the body in absolute terms.
        // The second condition is what keeps a long fade-in from reporting
        // its own second bar as the vocal: early in a fade the level is up
        // on the plateau, but nowhere near the body.
        const double target = qMax(plateau + 0.50 * (body - plateau), 0.55 * body);
        const int sustain = bucketsFor(kIntroSustainMs);

        qint64 capMs = qMin<qint64>(kMaxIntroMs,
                                    qint64(data.durationMs * kMaxIntroFraction));
        capMs = qMax(capMs, out.firstSoundMs); // never cap below the first sound
        const int capIdx = qBound(headIdx, int(capMs / kMs), tailIdx);

        int run = 0;
        int found = -1;
        for (int i = headIdx; i < tailIdx; ++i) {
            run = level[i] >= target ? run + 1 : 0;
            if (run >= sustain) {
                found = i - sustain + 1;
                break;
            }
        }
        if (found < 0 || found > capIdx) {
            out.introMs = out.firstSoundMs;
            out.noRamp = true;
        } else {
            out.introMs = qint64(found) * kMs;
        }
    }

    // The run-out: scan back for the last place the track was still at
    // performance level for kOutroSustainMs. Everything after that — the
    // fade, the ring-out, the silence quietTailMs already measures — is
    // outro. A cold ending gives an outro of just the trailing silence,
    // which is the right answer: there is nothing to talk over.
    {
        const double target = 0.55 * body;
        const int sustain = bucketsFor(kOutroSustainMs);
        int run = 0;
        qint64 outroStartMs = out.firstSoundMs;
        for (int i = tailIdx - 1; i >= headIdx; --i) {
            run = level[i] >= target ? run + 1 : 0;
            if (run >= sustain) {
                outroStartMs = qint64(i + sustain) * kMs;
                break;
            }
        }
        outroStartMs = qBound(out.introMs, outroStartMs, data.durationMs);
        out.outroMs = data.durationMs - outroStartMs;
    }

    return out;
}
