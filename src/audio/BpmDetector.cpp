#include "BpmDetector.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
// A tempo needs a few bars of audio before it means anything. Shorter
// material (stings, one-shot jingles) reports "unknown" instead of a number
// read off two or three transients.
constexpr int kMinDurationMs = 15000;

// Analysis window and hop. Each window votes for a tempo on its own; the
// overlap means a beat pattern straddling a boundary still lands whole
// inside a neighbour.
constexpr int kMaxWindowMs = 24000;
constexpr int kMinWindowMs = 8000;

// How far two window votes may sit apart and still count as the same tempo.
// Deliberately tight: at 4 BPM the votes of pink noise start agreeing with
// each other by chance, which is exactly the failure this gate exists to
// catch.
constexpr double kClusterToleranceBpm = 2.0;

// A window only votes when its own correlation peak is at least this much
// above the typical one. Lax on purpose — the agreement between windows,
// not the strength of any single peak, is what decides the outcome.
constexpr double kWindowPeakRatio = 2.0;

// Octave prior. Autocorrelation peaks just as hard at half and double the
// real tempo, so candidates are weighted by how close they are to the tempo
// most music actually sits at; the width is deliberately loose enough to
// leave 70 BPM ballads and 175 BPM drum & bass reachable.
constexpr double kPriorCenterBpm = 120.0;
constexpr double kPriorOctaves = 0.9;

int s_minAgreementPercent = 35;

double octavePrior(double bpm)
{
    const double octaves = std::log2(bpm / kPriorCenterBpm) / kPriorOctaves;
    return std::exp(-0.5 * octaves * octaves);
}

double foldIntoRange(double bpm)
{
    while (bpm < BpmDetector::MinBpm)
        bpm *= 2.0;
    while (bpm > BpmDetector::MaxBpm)
        bpm /= 2.0;
    return bpm;
}

// Onset strength: the half-wave-rectified rise of the log envelope. The log
// is what makes a beat in a quiet intro count as much as one in a loud
// chorus; the rectification keeps only the attacks, so what the correlation
// looks for is the beat grid and not the overall loudness shape.
QVector<double> onsetStrength(const QVector<quint8> &envelope)
{
    const int n = envelope.size();
    QVector<double> onset(n, 0.0);
    if (n < 2)
        return onset;

    double previous = std::log1p(double(envelope.at(0)));
    for (int i = 1; i < n; ++i) {
        const double current = std::log1p(double(envelope.at(i)));
        onset[i] = qMax(0.0, current - previous);
        previous = current;
    }
    return onset;
}

// The tempo one window argues for, or 0 when nothing stands out in it.
double windowTempo(const QVector<double> &onset, int from, int to,
                   int minLag, int maxLag, int msPerStep)
{
    const int n = to - from;
    if (n < maxLag * 4)
        return 0.0;

    double sum = 0.0;
    for (int i = from; i < to; ++i)
        sum += onset.at(i);
    const double mean = sum / n;

    QVector<double> correlation(maxLag + 2, 0.0);
    double magnitudeSum = 0.0;
    double bestScore = -std::numeric_limits<double>::max();
    int bestLag = -1;
    for (int lag = minLag; lag <= maxLag; ++lag) {
        double acc = 0.0;
        for (int i = from + lag; i < to; ++i)
            acc += (onset.at(i) - mean) * (onset.at(i - lag) - mean);
        // Per-sample average, otherwise short lags win on overlap alone.
        const double score = (acc / (n - lag)) * octavePrior(60000.0 / (lag * msPerStep));
        correlation[lag] = score;
        magnitudeSum += std::fabs(score);
        if (score > bestScore) {
            bestScore = score;
            bestLag = lag;
        }
    }

    // Measured against the mean of the absolute scores: the onset function
    // is mean-centred, so most lags correlate slightly negatively and the
    // plain mean can be negative, which makes it useless as a yardstick.
    const double typicalScore = magnitudeSum / (maxLag - minLag + 1);
    if (bestLag < 0 || bestScore <= 0.0 || typicalScore <= 0.0
        || bestScore < typicalScore * kWindowPeakRatio) {
        return 0.0;
    }

    // Sub-step precision: the true period rarely lands on a whole envelope
    // step, so fit a parabola through the winning lag and its neighbours.
    double refinedLag = bestLag;
    if (bestLag > minLag && bestLag < maxLag) {
        const double left = correlation.at(bestLag - 1);
        const double right = correlation.at(bestLag + 1);
        const double denominator = left - 2.0 * bestScore + right;
        if (denominator < 0.0)
            refinedLag += 0.5 * (left - right) / denominator;
    }

    return foldIntoRange(60000.0 / (refinedLag * msPerStep));
}
} // namespace

int BpmDetector::minAgreementPercent()
{
    return s_minAgreementPercent;
}

void BpmDetector::setMinAgreementPercent(int percent)
{
    s_minAgreementPercent = qBound(10, percent, 100);
}

double BpmDetector::distance(double a, double b)
{
    if (a <= 0.0 || b <= 0.0)
        return std::numeric_limits<double>::max();

    return qMin(std::fabs(a - b), qMin(std::fabs(a - b * 2.0), std::fabs(a - b / 2.0)));
}

double BpmDetector::estimateFromEnvelope(const QVector<quint8> &envelope, int msPerStep)
{
    const int n = envelope.size();
    if (msPerStep <= 0 || qint64(n) * msPerStep < kMinDurationMs)
        return 0.0;

    const QVector<double> onset = onsetStrength(envelope);

    // Candidate beat periods, in envelope steps.
    const int minLag = qMax(2, int(std::floor(60000.0 / (MaxBpm * msPerStep))));
    const int maxLag = qMin(int(std::ceil(60000.0 / (MinBpm * msPerStep))), n / 4);
    if (minLag >= maxLag)
        return 0.0;

    // Every window gets an equal say, whatever its loudness. One correlation
    // over the whole track instead lets the busiest section dominate, and
    // smears the peak when the tempo drifts or the recording holds more than
    // one piece — which is how a rubato solo performance used to come out
    // with a confident-looking number that no two parts of it agreed on.
    const int durationMs = n * msPerStep;
    const int windowMs = qBound(kMinWindowMs, durationMs / 3, kMaxWindowMs);
    const int windowSteps = windowMs / msPerStep;
    const int hopSteps = qMax(1, windowSteps / 2);

    QVector<double> votes;
    int windows = 0;
    for (int start = 0; start + windowSteps <= n; start += hopSteps) {
        ++windows;
        const double tempo = windowTempo(onset, start, start + windowSteps,
                                         minLag, maxLag, msPerStep);
        if (tempo > 0.0)
            votes.append(tempo);
    }
    if (votes.size() < 3)
        return 0.0; // nothing to corroborate

    // Largest group of votes that agree with each other, half and double
    // time counted as the same tempo. Its size against the number of
    // windows is the confidence: a real pulse is heard everywhere in the
    // track, while a chance peak is heard once.
    int bestCount = 0;
    QVector<double> bestGroup;
    for (const double centre : votes) {
        QVector<double> group;
        for (const double vote : votes) {
            if (distance(vote, centre) > kClusterToleranceBpm)
                continue;
            // Bring half/double-time votes onto the group's own octave
            // before averaging, so the median stays a real tempo.
            double aligned = vote;
            if (aligned < centre / 1.5)
                aligned *= 2.0;
            else if (aligned > centre * 1.5)
                aligned /= 2.0;
            group.append(aligned);
        }
        if (group.size() > bestCount) {
            bestCount = group.size();
            bestGroup = group;
        }
    }

    // Three windows minimum, whatever the share: two stretches of a slow
    // swell happening to land on the same lag is how a sustained pad talks
    // its way into a tempo, and no real pulse is carried by only two parts
    // of a track.
    if (bestCount < 3 || bestCount * 100 < windows * s_minAgreementPercent)
        return 0.0; // no tempo the whole track agrees on

    std::sort(bestGroup.begin(), bestGroup.end());
    const double bpm = bestGroup.at(bestGroup.size() / 2);
    return std::round(bpm * 10.0) / 10.0;
}
