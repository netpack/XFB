#ifndef VOICEDUCK_H
#define VOICEDUCK_H

#include <QPointF>
#include <QVector>
#include <QtGlobal>

struct WaveformData;

/**
 * @brief Writing the ducking for a voice track from the take's own level.
 *
 * A voice track is the presenter's link recorded across the join between two
 * songs. What makes it sound like radio rather than like two things playing
 * at once is the ducking: the music drops while the presenter is talking and
 * comes back up when they stop. Doing that by hand means dragging four
 * envelope nodes per sentence, which is tedious sighted and close to
 * impossible by ear.
 *
 * So the duck is derived from the recording itself. The take is analysed as
 * exactly the same 20 ms peak buckets WaveformStore produces for every other
 * file in XFB, the loud stretches are taken to be "the presenter is talking",
 * and those stretches become a volume line on the music.
 *
 * ---------------------------------------------------------------------
 * WHAT THIS IS NOT
 *
 * This is a LEVEL heuristic, the same family as IntroDetector — and it
 * borrows that file's threshold rule on purpose, so the two cannot drift
 * apart. It does not know what a voice is. It finds the parts of the take
 * that are louder than the take's own noise floor. That means:
 *
 *   - A noisy studio, headphone spill or a fan on the desk raises the floor
 *     and can read as continuous speech, which ducks the whole link.
 *   - A take recorded with the monitor speakers open has the music itself in
 *     it, and the music will duck the music.
 *   - Breaths, lip noise and the presenter shuffling paper are "voice".
 *   - A whispered word under a loud passage falls below the threshold and
 *     will not duck at all.
 *
 * The output is deliberately an ORDINARY volume envelope
 * (PlaylistWaveView::VolumeEnvelopeRole, the "ms:gain;..." string saved as
 * the volenv attribute in playlist XML), not a special kind of object. The
 * operator can drag it, add nodes to it, flatten it, and it survives save
 * and load like any other. The generator is a first draft that is right
 * often enough to be worth having, never something the operator is stuck
 * with.
 * ---------------------------------------------------------------------
 *
 * CRITICAL: gains here are LINEAR multipliers clamped to 0..1, not decibels.
 * That is what VolumeEnvelopeRole means everywhere else in XFB.
 */
namespace VoiceDuck
{

/** Shape of the duck. Times are ms, gains are linear multipliers 0..1. */
struct Params
{
    /** Music level while the presenter is talking. 0.25 ~= -12 dB. */
    double duckGain = 0.25;

    /** Ramp down into the duck and back up out of it. */
    int fadeMs = 400;

    /** Start ducking this long before the first loud bucket, so the music is
     *  already down on the presenter's first syllable rather than diving
     *  under it. */
    int leadInMs = 250;

    /** Stay ducked this long after the presenter stops, so the music does not
     *  jump back up between two sentences. */
    int holdMs = 700;

    /** Silences shorter than this inside the take do not lift the duck.
     *  Below the natural pause between sentences on purpose: a link that
     *  bobs up and down mid-sentence sounds broken. */
    int minGapMs = 1500;

    /** Loud runs shorter than this are noise, not speech. */
    int minRunMs = 120;

    /** Threshold for "the presenter is talking", as a percentage of the
     *  take's own maximum peak, fed to IntroDetector::quietThreshold(). The
     *  auto-mix default (5%) is a silence-trimming threshold and is far too
     *  low for a voice recording, whose noise floor sits well above the
     *  digital black of a mastered music file. */
    int thresholdPercent = 14;
};

/** One stretch of the take in which the presenter is talking, in ms from the
 *  start of the take. */
struct Span
{
    qint64 startMs = 0;
    qint64 endMs = 0;
};

/**
 * The stretches of @a voice in which the presenter is audible, already
 * merged across short pauses and padded by leadInMs / holdMs.
 *
 * Empty when the take is silent (or has no usable waveform), which is the
 * honest answer: nothing to duck under.
 */
QVector<Span> voiceSpans(const WaveformData &voice, const Params &params);

/**
 * The duck for ONE music track, as envelope nodes ready for
 * PlaylistWaveView::encodeEnvelope().
 *
 * @param voice           the take's waveform.
 * @param voiceStartMs    where the take begins on THIS track's own timeline.
 *                        Negative when the take started before the track did
 *                        (the usual case for the incoming song, which comes
 *                        up under a link already in progress).
 * @param trackDurationMs the music track's length.
 *
 * Returns an empty vector when nothing of the take overlaps the track, so the
 * caller can leave that track's volume line alone rather than writing a flat
 * one over an envelope the operator drew by hand.
 */
QVector<QPointF> envelopeForTrack(const WaveformData &voice,
                                  qint64 voiceStartMs,
                                  qint64 trackDurationMs,
                                  const Params &params);

/**
 * envelopeForTrack() from spans that have already been computed. The dialog
 * uses this so nudging the link against the join re-maps the same analysis
 * instead of re-running it on every 100 ms step.
 */
QVector<QPointF> envelopeFromSpans(const QVector<Span> &spans,
                                   qint64 voiceStartMs,
                                   qint64 trackDurationMs,
                                   const Params &params);

} // namespace VoiceDuck

#endif // VOICEDUCK_H
