#ifndef BPMDETECTOR_H
#define BPMDETECTOR_H

#include <QVector>

/**
 * Tempo (BPM) estimation from an amplitude envelope.
 *
 * The envelope is the same kind of data WaveformStore already builds while
 * decoding a file — one 0..255 amplitude value per fixed slice of audio —
 * so a track's tempo costs no extra decoding pass.
 */
namespace BpmDetector
{
/** Tempo range considered. Everything outside is folded into it. */
constexpr double MinBpm = 60.0;
constexpr double MaxBpm = 200.0;

/**
 * Estimates the tempo of the audio whose amplitude envelope is @a envelope,
 * sampled one value per @a msPerStep milliseconds.
 *
 * The envelope is cut into overlapping windows that each estimate a tempo
 * independently; the answer is the tempo the largest group of them agrees
 * on. Agreement, rather than the height of any one correlation peak, is
 * what decides: a real pulse is heard in every part of a track, while a
 * chance peak is heard once. Rubato and multi-piece recordings therefore
 * report nothing instead of a confident-looking number no two parts of the
 * track support.
 *
 * Returns 0 when no tempo stands out: silence, speech, a jingle too short to
 * hold a beat, or freely-played music. Callers should treat 0 as "unknown"
 * rather than as a number — in a library of produced music expect roughly a
 * third of it, concentrated in solo and live acoustic material.
 *
 * The result can land an octave off — a 174 BPM track reported as 87 — when
 * the material gives no reason to prefer one over the other, which is why
 * distance() treats half and double time as a match. Tempos are otherwise
 * accurate to a few tenths of a BPM.
 */
double estimateFromEnvelope(const QVector<quint8> &envelope, int msPerStep);

/**
 * Share of windows that must agree before a tempo is reported, in percent
 * (BpmMinAgreementPercent in xfb.conf). Lowering it fills in more of the
 * library at the cost of tempos the track itself does not hold to; below
 * about 30 the votes of unpitched noise start to qualify. Default 35.
 */
int minAgreementPercent();
void setMinAgreementPercent(int percent);

/**
 * Difference between two tempos in BPM, counting half and double time as a
 * match: 140 and 70 are 0 apart, because a track at either tempo mixes over
 * the other. Returns a large value when either tempo is unknown (<= 0).
 */
double distance(double a, double b);
} // namespace BpmDetector

#endif // BPMDETECTOR_H
