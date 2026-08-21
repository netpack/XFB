package pt.netpack.xfb.companion

import android.content.Context
import java.io.File
import java.util.concurrent.ConcurrentHashMap
import kotlin.math.max
import kotlin.math.min

/**
 * XFB's Auto-mix, on the phone.
 *
 * The desktop works out a crossfade by measuring the dead air at the join: how
 * long the outgoing track stays quiet at its end, plus how long the incoming
 * one takes to get going. Overlapping by exactly that much lands the first
 * sound of the new track where the last sound of the old one stopped, which is
 * why it works on material nobody has beat-matched.
 *
 * The scan is ported from PlaylistWaveView verbatim — same 20 ms buckets, same
 * threshold relative to each track's own loudest peak, same three-bucket
 * sustain rule so a click in a faded tail does not count as the song still
 * going. The peaks here are 0..1 floats rather than 0..255 bytes; the
 * thresholds are converted, not re-chosen.
 *
 * The one deliberate difference is [MIN_BLEND_MS]. On the desktop Auto-mix is a
 * button somebody presses on a playlist they shaped, and leaving a tightly
 * mastered join as a clean cut is a fair answer. Here it runs unattended over
 * whatever happens to be on the phone, where a hard cut reads as the feature
 * not working, so a join always gets at least a short blend.
 */
object AutoMix {

    /**
     * Loud is this share of the track's own loudest peak. AutoMixThresholdPercent
     * on the desktop, where it defaults to 5 and has no UI control either.
     */
    const val THRESHOLD_PERCENT = 5

    /**
     * A moment counts as loud only as part of a run of this many consecutive
     * peaks at or above the threshold — 3 x 20 ms, as on the desktop.
     */
    private const val SUSTAIN_BUCKETS = 3

    /** A track whose loudest peak is below this (about -30 dBFS) is all quiet. */
    private const val MIN_LOUD_PEAK = 8f / 255f

    /** The shortest auto crossfade. See the note above on why there is a floor. */
    const val MIN_BLEND_MS = 1_200L

    /** What a track's silent edges measure, once its waveform has been read. */
    data class Edges(val headQuietMs: Long, val tailQuietMs: Long, val durationMs: Long)

    /**
     * Edges are a property of a track, not of a join: the overlap for a pair is
     * the outgoing track's tail plus the incoming track's head. Keeping them
     * per file means shuffle — which changes every pair — costs nothing.
     */
    private val edges = ConcurrentHashMap<String, Edges>()

    /**
     * Files whose waveform could not be read. Without this a track that will
     * not decode is asked for again every time a join is considered.
     */
    private val undecodable = ConcurrentHashMap.newKeySet<String>()

    // ------------------------------------------------------------- the scan

    /**
     * Loud, in peak units, for this track — relative to its own maximum so
     * differently mastered tracks trim comparably. Negative means all quiet.
     */
    private fun threshold(waveform: Waveform): Float {
        var maxPeak = 0f
        for (peak in waveform.peaks) maxPeak = max(maxPeak, peak)
        if (maxPeak < MIN_LOUD_PEAK) return -1f
        return max(1f / 255f, maxPeak * THRESHOLD_PERCENT / 100f)
    }

    /** How long the track takes to get going. Its whole length if it never does. */
    fun quietHeadMs(waveform: Waveform): Long {
        val level = threshold(waveform)
        if (level < 0f) return waveform.durationMs
        var run = 0
        for (i in waveform.peaks.indices) {
            run = if (waveform.peaks[i] >= level) run + 1 else 0
            if (run >= SUSTAIN_BUCKETS)
                return (i - SUSTAIN_BUCKETS + 1).toLong() * waveform.bucketMs
        }
        return waveform.durationMs
    }

    /** How long the track stays quiet at the end. Its whole length if it is silent. */
    fun quietTailMs(waveform: Waveform): Long {
        val level = threshold(waveform)
        if (level < 0f) return waveform.durationMs
        var run = 0
        for (i in waveform.peaks.indices.reversed()) {
            run = if (waveform.peaks[i] >= level) run + 1 else 0
            if (run >= SUSTAIN_BUCKETS) {
                // Scanning backwards, so this is the last sustained-loud run and
                // it ends just before bucket i + SUSTAIN_BUCKETS. Anything past
                // the end of the peak buffer is quiet too.
                val loudEndMs = (i + SUSTAIN_BUCKETS).toLong() * waveform.bucketMs
                return max(0L, waveform.durationMs - loudEndMs)
            }
        }
        return waveform.durationMs
    }

    fun edgesOf(waveform: Waveform): Edges =
        Edges(quietHeadMs(waveform), quietTailMs(waveform), waveform.durationMs)

    // ------------------------------------------------------------ the cache

    /** What is already known about [file], or null. Never blocks. */
    fun peek(file: File): Edges? = edges[file.absolutePath]

    /**
     * Reads [file]'s edges in the background if they are not known yet.
     * [onReady] runs when they land, so a screen showing the crossfade can
     * redraw; it does not run for a file that cannot be decoded, which is what
     * [undecodable] is for.
     *
     * Safe to call repeatedly, and from more than one place at once. There is
     * deliberately no in-flight guard here: the store already queues every
     * caller for a file being decoded and answers all of them. A guard would
     * mean whichever caller asked second — routinely the player screen, behind
     * the service — was never told the answer had arrived, and would sit
     * showing a clean cut on a join that has a crossfade.
     */
    fun ensure(file: File, cacheDir: File, onReady: (() -> Unit)? = null) {
        val key = file.absolutePath
        if (undecodable.contains(key)) return

        WaveformStore.fetch(file, cacheDir) { waveform ->
            if (waveform == null) {
                undecodable.add(key)
            } else {
                edges[key] = edgesOf(waveform)
                onReady?.invoke()
            }
        }
    }

    /**
     * The crossfade Auto-mix would set for this join, or null while the two
     * tracks are still being read.
     */
    fun overlapFor(outgoing: File, incoming: File): Long? {
        val tail = peek(outgoing) ?: return null
        val head = peek(incoming) ?: return null

        val overlap = max(tail.tailQuietMs + head.headQuietMs, MIN_BLEND_MS)

        // The same clamp the desktop applies: an overlap can never be longer
        // than either of the tracks it joins, nor than XFB's own ceiling.
        var ceiling = MAX_OVERLAP_MS
        if (tail.durationMs > 0) ceiling = min(ceiling, tail.durationMs)
        if (head.durationMs > 0) ceiling = min(ceiling, head.durationMs)
        return overlap.coerceIn(0L, ceiling)
    }

    /**
     * The overlap this join will actually play with.
     *
     * A crossfade somebody set by hand always wins — including one they set to
     * zero, which is a clean cut they asked for. [SavedTrack.overlapSet] is what
     * tells those apart from a join nobody has touched, which is where Auto-mix
     * gets to have an opinion.
     */
    fun effectiveOverlapMs(
        enabled: Boolean,
        outgoing: SavedTrack?,
        incoming: SavedTrack?
    ): Long {
        if (incoming == null) return 0L
        if (incoming.overlapPinned) return incoming.overlapMs
        if (!enabled || outgoing == null) return incoming.overlapMs
        return overlapFor(outgoing.file, incoming.file) ?: incoming.overlapMs
    }

    /** True when the value shown for this join came from Auto-mix. */
    fun isAutomatic(enabled: Boolean, outgoing: SavedTrack?, incoming: SavedTrack?): Boolean =
        enabled && incoming != null && !incoming.overlapPinned && outgoing != null &&
            overlapFor(outgoing.file, incoming.file) != null
}

/**
 * Whether Auto-mix is on, remembered across launches.
 *
 * On by default: the app's own "everything on this phone" set is an arbitrary
 * pile with no crossfades in it, and clean cuts between unrelated tracks is the
 * thing this was added to stop. Anyone who wants the cuts back has one switch
 * to find.
 */
object AutoMixSettings {

    private const val PREFS = "automix"
    private const val KEY_ENABLED = "enabled"

    fun isEnabled(context: Context): Boolean =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getBoolean(KEY_ENABLED, true)

    fun setEnabled(context: Context, enabled: Boolean) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .edit().putBoolean(KEY_ENABLED, enabled).apply()
    }
}
