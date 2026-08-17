package pt.netpack.xfb.companion

import android.content.Context

/**
 * The phone's copy of one XFB channel's FX settings.
 *
 * Field names, defaults and ranges are the desktop's `FxParams` verbatim, so a
 * value read off the desk means the same thing typed in here. The desktop keeps
 * these per channel ("Main", "LP1", "LP2") in xfb.conf; the phone has only one
 * output, so it keeps one set.
 */
data class FxSettings(
    val retune432: Boolean = false,
    val eqEnabled: Boolean = false,
    val preampDb: Double = 0.0,
    val bandGainsDb: DoubleArray = DoubleArray(BANDS),
    val compEnabled: Boolean = false,
    val compThresholdDb: Double = -18.0,
    val compRatio: Double = 3.0,
    val compAttackMs: Double = 10.0,
    val compReleaseMs: Double = 150.0,
    val compMakeupDb: Double = 0.0,
) {

    /** True when nothing here would change the audio. */
    val isFlat: Boolean
        get() = !retune432 &&
            (!eqEnabled || (preampDb == 0.0 && bandGainsDb.all { it == 0.0 })) &&
            !compEnabled

    // DoubleArray gives a data class identity equals/hashCode, which would make
    // "have the settings changed?" answer wrongly and re-configure the chain on
    // every touch of a slider.
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is FxSettings) return false
        return retune432 == other.retune432 &&
            eqEnabled == other.eqEnabled &&
            preampDb == other.preampDb &&
            bandGainsDb.contentEquals(other.bandGainsDb) &&
            compEnabled == other.compEnabled &&
            compThresholdDb == other.compThresholdDb &&
            compRatio == other.compRatio &&
            compAttackMs == other.compAttackMs &&
            compReleaseMs == other.compReleaseMs &&
            compMakeupDb == other.compMakeupDb
    }

    override fun hashCode(): Int {
        var result = retune432.hashCode()
        result = 31 * result + eqEnabled.hashCode()
        result = 31 * result + preampDb.hashCode()
        result = 31 * result + bandGainsDb.contentHashCode()
        result = 31 * result + compEnabled.hashCode()
        result = 31 * result + compThresholdDb.hashCode()
        result = 31 * result + compRatio.hashCode()
        result = 31 * result + compAttackMs.hashCode()
        result = 31 * result + compReleaseMs.hashCode()
        result = 31 * result + compMakeupDb.hashCode()
        return result
    }

    companion object {
        const val BANDS = 10

        /** ISO octave band centres, matching FxParams::kBandFreqHz. */
        val BAND_FREQ_HZ = doubleArrayOf(
            31.25, 62.5, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0
        )

        /** The desktop's slider range for a band, and for the preamp. */
        const val MAX_GAIN_DB = 12.0

        /**
         * A=432 as a ratio against A=440. The desktop reaches it by resampling
         * in ffmpeg and restoring the tempo afterwards; ExoPlayer's Sonic pitch
         * shift is the same operation, so this is the number handed to it.
         */
        const val RETUNE_432_PITCH = 432.0f / 440.0f

        fun bandLabel(index: Int): String {
            val hz = BAND_FREQ_HZ[index]
            return if (hz >= 1000) "${(hz / 1000).toInt()}k" else hz.toInt().toString()
        }
    }
}

/** Persists [FxSettings] on the phone, in the app's private preferences. */
class FxSettingsStore(context: Context) {

    private val prefs =
        context.applicationContext.getSharedPreferences("fx", Context.MODE_PRIVATE)

    fun load(): FxSettings {
        val bands = DoubleArray(FxSettings.BANDS)
        for (i in bands.indices) {
            bands[i] = prefs.getFloat(bandKey(i), 0f).toDouble()
        }
        return FxSettings(
            retune432 = prefs.getBoolean(KEY_RETUNE, false),
            eqEnabled = prefs.getBoolean(KEY_EQ_ENABLED, false),
            preampDb = prefs.getFloat(KEY_PREAMP, 0f).toDouble(),
            bandGainsDb = bands,
            compEnabled = prefs.getBoolean(KEY_COMP_ENABLED, false),
            compThresholdDb = prefs.getFloat(KEY_COMP_THRESHOLD, -18f).toDouble(),
            compRatio = prefs.getFloat(KEY_COMP_RATIO, 3f).toDouble(),
            compAttackMs = prefs.getFloat(KEY_COMP_ATTACK, 10f).toDouble(),
            compReleaseMs = prefs.getFloat(KEY_COMP_RELEASE, 150f).toDouble(),
            compMakeupDb = prefs.getFloat(KEY_COMP_MAKEUP, 0f).toDouble(),
        )
    }

    fun save(settings: FxSettings) {
        val editor = prefs.edit()
            .putBoolean(KEY_RETUNE, settings.retune432)
            .putBoolean(KEY_EQ_ENABLED, settings.eqEnabled)
            .putFloat(KEY_PREAMP, settings.preampDb.toFloat())
            .putBoolean(KEY_COMP_ENABLED, settings.compEnabled)
            .putFloat(KEY_COMP_THRESHOLD, settings.compThresholdDb.toFloat())
            .putFloat(KEY_COMP_RATIO, settings.compRatio.toFloat())
            .putFloat(KEY_COMP_ATTACK, settings.compAttackMs.toFloat())
            .putFloat(KEY_COMP_RELEASE, settings.compReleaseMs.toFloat())
            .putFloat(KEY_COMP_MAKEUP, settings.compMakeupDb.toFloat())
        for (i in 0 until FxSettings.BANDS) {
            editor.putFloat(bandKey(i), settings.bandGainsDb[i].toFloat())
        }
        editor.apply()
    }

    private fun bandKey(index: Int) = "eqBand$index"

    private companion object {
        const val KEY_RETUNE = "retune432"
        const val KEY_EQ_ENABLED = "eqEnabled"
        const val KEY_PREAMP = "preampDb"
        const val KEY_COMP_ENABLED = "compEnabled"
        const val KEY_COMP_THRESHOLD = "compThresholdDb"
        const val KEY_COMP_RATIO = "compRatio"
        const val KEY_COMP_ATTACK = "compAttackMs"
        const val KEY_COMP_RELEASE = "compReleaseMs"
        const val KEY_COMP_MAKEUP = "compMakeupDb"
    }
}
