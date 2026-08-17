package pt.netpack.xfb.companion

import java.nio.ByteBuffer

/**
 * Kotlin face of XFB's own equalizer and compressor, compiled for the NDK from
 * the desktop's `src/audio/FxDsp.cpp` rather than reimplemented here — a set
 * mixed on the phone should sound like the same set on the desk.
 *
 * One instance belongs to one audio pipeline (so, one deck). It is touched only
 * from the audio thread once playback is running, apart from [configure], which
 * the UI calls; the native side rebuilds its filters under that call, so a
 * configure mid-block is the one thing to keep off the audio thread.
 *
 * The 432 Hz retune is deliberately not part of this: ExoPlayer's own Sonic
 * pitch shift already does what the desktop asks ffmpeg for. See [FxSettings].
 */
class FxChain : AutoCloseable {

    private var handle: Long = nativeCreate()

    /** True when the current settings would actually change the audio. */
    var isActive: Boolean = false
        private set

    fun configure(sampleRate: Int, settings: FxSettings) {
        if (handle == 0L) return
        isActive = nativeConfigure(
            handle,
            sampleRate,
            settings.eqEnabled,
            settings.preampDb,
            settings.bandGainsDb,
            settings.compEnabled,
            settings.compThresholdDb,
            settings.compRatio,
            settings.compAttackMs,
            settings.compReleaseMs,
            settings.compMakeupDb,
        )
    }

    /** Drops filter state; use when seeking or changing track. */
    fun reset() {
        if (handle != 0L) nativeReset(handle)
    }

    /**
     * Filters [frames] of interleaved 16-bit PCM from [input] into [output].
     * Both must be direct buffers. [channels] is 1 or 2; mono is widened and
     * narrowed again inside, since the DSP itself is stereo-only.
     */
    fun process(
        input: ByteBuffer,
        inputOffset: Int,
        output: ByteBuffer,
        outputOffset: Int,
        frames: Int,
        channels: Int,
    ) {
        if (handle == 0L) return
        nativeProcess(handle, input, inputOffset, output, outputOffset, frames, channels)
    }

    override fun close() {
        if (handle != 0L) {
            nativeDestroy(handle)
            handle = 0L
        }
    }

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeConfigure(
        handle: Long,
        sampleRate: Int,
        eqEnabled: Boolean,
        preampDb: Double,
        bandGainsDb: DoubleArray,
        compEnabled: Boolean,
        thresholdDb: Double,
        ratio: Double,
        attackMs: Double,
        releaseMs: Double,
        makeupDb: Double,
    ): Boolean

    private external fun nativeReset(handle: Long)
    private external fun nativeProcess(
        handle: Long,
        input: ByteBuffer,
        inputOffset: Int,
        output: ByteBuffer,
        outputOffset: Int,
        frames: Int,
        channels: Int,
    )

    companion object {
        /**
         * False on a device whose ABI the APK was not built for, which is the
         * one failure worth surviving: the app is a player first, and it can
         * still play with the FX section switched off.
         */
        val isAvailable: Boolean = try {
            System.loadLibrary("xfbfx")
            true
        } catch (_: UnsatisfiedLinkError) {
            false
        }
    }
}
