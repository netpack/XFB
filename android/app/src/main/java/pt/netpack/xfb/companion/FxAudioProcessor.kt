package pt.netpack.xfb.companion

import android.util.Log
import androidx.media3.common.C
import androidx.media3.common.audio.AudioProcessor
import androidx.media3.common.audio.BaseAudioProcessor
import java.nio.ByteBuffer

/**
 * Puts XFB's equalizer and compressor into ExoPlayer's audio pipeline.
 *
 * One of these belongs to each deck, since each deck has its own audio sink and
 * the two are audible together across a crossfade — a single shared chain would
 * have both decks writing through the same filter state.
 *
 * The 432 Hz retune is not done here. ExoPlayer already carries Sonic for
 * playback parameters, and asking it for a pitch of 432/440 is the same
 * operation the desktop asks ffmpeg for; see [PlaybackService.applyFx]. Sonic
 * sits *after* this processor in the chain, so the phone equalizes and then
 * retunes where the desktop retunes and then equalizes. The difference is that
 * the EQ's band centres land 1.8% off — a third of a semitone on a filter an
 * octave wide, which is not audible and is not worth fighting the pipeline for.
 *
 * The processor stays in the chain whenever the format is stereo 16-bit, even
 * with every effect switched off, and copies straight through in that case:
 * whether it is active decides the shape of the sink's pipeline, and changing
 * that mid-track would force a reconfigure the listener would hear.
 */
class FxAudioProcessor(initial: FxSettings) : BaseAudioProcessor() {

    private val chain: FxChain? = if (FxChain.isAvailable) FxChain() else null

    /** Written by the UI thread, read by the audio thread — hence volatile. */
    @Volatile
    private var pendingSettings: FxSettings? = initial

    private var sampleRate = 0
    private var channelCount = 0

    /** Latest settings the native chain has actually been configured with. */
    private var appliedSettings: FxSettings? = null

    /** True when those settings would really change the audio. */
    private var filtering = false

    val isSupported: Boolean get() = chain != null

    /**
     * Hands new settings over to be picked up on the audio thread. Safe to call
     * from anywhere; nothing is reconfigured under a block being processed.
     */
    fun updateSettings(settings: FxSettings) {
        pendingSettings = settings
    }

    override fun onConfigure(inputAudioFormat: AudioProcessor.AudioFormat): AudioProcessor.AudioFormat {
        // FxDsp works on interleaved stereo only, and 16-bit is the encoding
        // every device's sink is guaranteed to give us.
        // 16-bit is the encoding every device's sink is guaranteed to give us.
        // Mono and stereo are both handled; anything with more channels is left
        // alone rather than folded down behind the listener's back.
        if (chain == null ||
            inputAudioFormat.encoding != C.ENCODING_PCM_16BIT ||
            inputAudioFormat.channelCount !in 1..2
        ) {
            Log.d(TAG, "standing aside: native=${chain != null} " +
                       "encoding=${inputAudioFormat.encoding} " +
                       "channels=${inputAudioFormat.channelCount}")
            return AudioProcessor.AudioFormat.NOT_SET
        }

        sampleRate = inputAudioFormat.sampleRate
        channelCount = inputAudioFormat.channelCount
        // The filters are built for a sample rate, so a new format invalidates
        // them however unchanged the settings themselves are.
        appliedSettings = null
        Log.d(TAG, "in the chain at ${sampleRate}Hz, ${inputAudioFormat.channelCount}ch")
        return inputAudioFormat
    }

    override fun queueInput(inputBuffer: ByteBuffer) {
        val remaining = inputBuffer.remaining()
        if (remaining == 0) return

        applyPendingSettings()

        val output = replaceOutputBuffer(remaining)
        val frameSize = BYTES_PER_SAMPLE * channelCount
        val frames = if (frameSize > 0) remaining / frameSize else 0

        if (chain != null && filtering && frames > 0 && remaining % frameSize == 0) {
            chain.process(inputBuffer, inputBuffer.position(), output, 0, frames, channelCount)
            output.position(remaining)
            inputBuffer.position(inputBuffer.limit())
        } else {
            // Nothing switched on, or a block that is not whole frames.
            output.put(inputBuffer)
        }

        output.flip()
    }

    private fun applyPendingSettings() {
        val settings = pendingSettings ?: return
        if (settings == appliedSettings || sampleRate == 0) return

        chain?.let {
            it.configure(sampleRate, settings)
            filtering = it.isActive
        }
        appliedSettings = settings
        Log.d(TAG, "settings applied: filtering=$filtering eq=${settings.eqEnabled} " +
                   "comp=${settings.compEnabled} preamp=${settings.preampDb}")
    }

    override fun onFlush() {
        // Seeks and track changes leave the biquads holding the previous
        // audio's tail, which would ring over the first samples of the new one.
        chain?.reset()
    }

    override fun onReset() {
        chain?.reset()
        appliedSettings = null
    }

    /** Frees the native chain. Call once the owning player has been released. */
    fun release() {
        chain?.close()
    }

    private companion object {
        /** 16-bit PCM. */
        const val BYTES_PER_SAMPLE = 2
        const val TAG = "XfbFx"
    }
}
