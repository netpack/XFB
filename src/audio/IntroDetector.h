#ifndef INTRODETECTOR_H
#define INTRODETECTOR_H

#include <QtGlobal>

struct WaveformData;

/**
 * @brief Where a track's ramp ends and where its run-out begins.
 *
 * `introMs` is the position, from the very start of the file, at which a
 * presenter has to stop talking — the "intro" or "ramp" number every radio
 * automation system prints next to a title. `outroMs` is the LENGTH of the
 * run-out at the other end (so the run-out starts at
 * durationMs - outroMs), because that is the number an operator thinks in:
 * "this one has twelve seconds of outro to talk over".
 *
 * The flags are not decoration: they say how much the numbers can be
 * trusted, and the caller may want to show them differently.
 */
struct IntroPoints
{
    /** First moment the track is audible at all (leading silence trimmed). */
    qint64 firstSoundMs = 0;
    /** Vocal entry, measured from t = 0. Never negative. */
    qint64 introMs = 0;
    /** Length of the run-out at the end of the track. */
    qint64 outroMs = 0;
    /** False when nothing usable could be measured (silent/empty waveform). */
    bool valid = false;
    /** The track is already at full level when it starts: introMs is the
     *  first sound and there is nothing to talk over. */
    bool startsLoud = false;
    /** No step up was found within the search cap; introMs fell back to the
     *  first sound. A long, level instrumental does this. */
    bool noRamp = false;
};

/**
 * @brief Intro / outro detection from a track's 20 ms peak envelope.
 *
 * This works on exactly the buckets WaveformStore already produces (one
 * 0..255 peak per 20 ms), so measuring a track costs nothing beyond the
 * decode the wave view pays for anyway, and a library sweep can run off
 * the on-disk waveform cache.
 *
 * The idea: an intro is the opening plateau of a song, and the vocal
 * arrives as a step UP in level that then stays up. So the detector
 *
 *   1. trims the leading silence (the same quiet-edge rule the auto-mix
 *      uses — see quietHeadMs below),
 *   2. smooths the envelope over kSmoothMs so a single drum hit cannot
 *      move it,
 *   3. takes a high quantile of the smoothed envelope as the track's
 *      "body" level — how loud this song is when it is properly going,
 *   4. takes the median of the first kPlateauWindowMs after the first
 *      sound as the opening plateau's own level, and
 *   5. calls the intro the first place where the smoothed level crosses
 *      half way from the plateau to the body — and stays across it for
 *      kIntroSustainMs, so a fill or a crash does not count.
 *
 * ---------------------------------------------------------------------
 * WHAT THIS IS NOT
 *
 * This is a LEVEL heuristic. It does not know what a voice is; there is
 * no vocal detection, no spectral analysis and no machine learning here.
 * It finds the place where the record gets louder and stays louder, which
 * on most produced pop, rock and dance records is where the singer comes
 * in — and on plenty of records is not. It is wrong, by construction, on:
 *
 *   - a cappella and spoken-word openings. The voice IS the opening
 *     plateau, so there is no step up to find. The detector reports the
 *     first sound (noRamp), i.e. "no ramp", which is the honest answer
 *     but is not what the sleeve says.
 *   - tracks that start at full level (the drop is bar one). startsLoud
 *     is set and introMs is the first sound: there is nothing to talk
 *     over, and the operator should be told that rather than given a
 *     fabricated number.
 *   - arrangements whose instrumental intro is as loud as the verse — a
 *     wall-of-sound intro, a loud guitar riff under a quiet vocal. The
 *     step never happens, and the result is noRamp.
 *   - tracks that build in stages (a long fade-in, an intro that adds a
 *     drum machine before the vocal). The step up that the detector
 *     finds may be the drums arriving, not the singer, so the number
 *     comes out short.
 *   - anything with a false start, a spoken count-in, or applause.
 *
 * That is why the number is editable and why a hand-corrected value is
 * locked against re-measurement (musics.intro_locked). Treat the machine
 * value as a first guess that is right often enough to be worth having,
 * never as ground truth.
 * ---------------------------------------------------------------------
 */
namespace IntroDetector
{
/** A moment counts as "loud" only inside a run of this many consecutive
 *  buckets at or above the threshold, so one click in a fade is not
 *  mistaken for the song still playing. 3 × 20 ms. */
constexpr int kSustainPeaks = 3;

/** A track whose loudest peak is below this (≈ -30 dBFS) is all quiet. */
constexpr int kMinLoudPeak = 8;

/** Smoothing window applied before the intro search: wide enough that a
 *  single drum hit cannot lift the curve, narrow enough that the vocal
 *  entry is still a sharp edge. */
constexpr int kSmoothMs = 200;

/** How much of the opening is taken to be "the plateau" whose level the
 *  step up is measured against. A song whose intro changes character
 *  inside this window measures against the mixture of the two. */
constexpr int kPlateauWindowMs = 3000;

/** How long the level has to stay up before the crossing counts as the
 *  vocal rather than a fill. */
constexpr int kIntroSustainMs = 600;

/** Same rule at the other end: how long the level has to stay up for the
 *  run-out not to have started yet. */
constexpr int kOutroSustainMs = 600;

/** Never report an intro longer than this. Past it the number stops being
 *  useful to a presenter and starts being a wrong answer with authority. */
constexpr qint64 kMaxIntroMs = 45000;

/** Nor longer than this share of the track, for short items. */
constexpr double kMaxIntroFraction = 0.45;

/** Threshold in peak units (0..255) for one track, relative to its own
 *  maximum peak so differently mastered tracks trim comparably.
 *  Returns -1 when the whole track is below kMinLoudPeak. */
int quietThreshold(const WaveformData &data, int thresholdPercent);

/** How long the track stays quiet at its head / tail. An all-quiet track
 *  returns its full duration. These are the primitives the playlist
 *  auto-mix has always used; they live here so the intro detector and the
 *  crossfade prep cannot drift apart. */
qint64 quietHeadMs(const WaveformData &data, int thresholdPercent);
qint64 quietTailMs(const WaveformData &data, int thresholdPercent);

/** Run the full analysis. `thresholdPercent` is the auto-mix quietness
 *  threshold (PlaylistWaveView::autoMixThresholdPercent()), used only for
 *  the quiet-edge trimming. */
IntroPoints detect(const WaveformData &data, int thresholdPercent);
} // namespace IntroDetector

#endif // INTRODETECTOR_H
