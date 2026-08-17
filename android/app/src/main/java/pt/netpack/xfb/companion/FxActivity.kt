package pt.netpack.xfb.companion

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import com.google.android.material.materialswitch.MaterialSwitch
import com.google.android.material.slider.Slider

/**
 * XFB's own equalizer and compressor, on the phone.
 *
 * The controls, their ranges and their defaults are the desktop's, because the
 * DSP underneath is literally the desktop's — see [FxChain]. Changes are saved
 * as they are made and pushed to whatever is playing, so a set can be shaped
 * while it runs rather than only before it starts.
 */
class FxActivity : AppCompatActivity() {

    private lateinit var store: FxSettingsStore
    private var settings = FxSettings()

    private lateinit var retuneSwitch: MaterialSwitch
    private lateinit var eqSwitch: MaterialSwitch
    private lateinit var compSwitch: MaterialSwitch
    private lateinit var eqBody: View
    private lateinit var compBody: LinearLayout
    private lateinit var bandHolder: LinearLayout
    private lateinit var preampSlider: Slider
    private lateinit var preampValue: TextView

    private val bandSliders = mutableListOf<Slider>()
    private val bandValues = mutableListOf<TextView>()

    /** Set while writing values into the controls, so listeners stay quiet. */
    private var loading = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_fx)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.apply {
            setDisplayHomeAsUpEnabled(true)
            title = getString(R.string.fx_title)
            subtitle = getString(R.string.fx_subtitle)
        }
        toolbar.setNavigationOnClickListener { finish() }

        store = FxSettingsStore(this)
        settings = store.load()

        retuneSwitch = findViewById(R.id.retuneSwitch)
        eqSwitch = findViewById(R.id.eqSwitch)
        compSwitch = findViewById(R.id.compSwitch)
        eqBody = findViewById(R.id.eqBody)
        compBody = findViewById(R.id.compBody)
        bandHolder = findViewById(R.id.bandHolder)
        preampSlider = findViewById(R.id.preampSlider)
        preampValue = findViewById(R.id.preampValue)

        if (!FxChain.isAvailable) {
            // The EQ and compressor are native; the retune is ExoPlayer's own
            // and still works, so only the two cards are shut off.
            findViewById<View>(R.id.unavailableLabel).visibility = View.VISIBLE
            eqSwitch.isEnabled = false
            compSwitch.isEnabled = false
        }

        buildBands()
        buildCompressorControls()

        retuneSwitch.setOnCheckedChangeListener { _, on ->
            if (!loading) commit(settings.copy(retune432 = on))
        }
        // Each of these has to update the section's enabled look as well as the
        // setting, or the controls stay greyed out under a switch that is on.
        eqSwitch.setOnCheckedChangeListener { _, on ->
            if (loading) return@setOnCheckedChangeListener
            commit(settings.copy(eqEnabled = on))
            updateEnabledState()
        }
        compSwitch.setOnCheckedChangeListener { _, on ->
            if (loading) return@setOnCheckedChangeListener
            commit(settings.copy(compEnabled = on))
            updateEnabledState()
        }

        preampSlider.addOnChangeListener { _, value, fromUser ->
            if (fromUser && !loading) commit(settings.copy(preampDb = value.toDouble()))
        }

        findViewById<MaterialButton>(R.id.flattenButton).setOnClickListener {
            commit(settings.copy(preampDb = 0.0, bandGainsDb = DoubleArray(FxSettings.BANDS)))
            render()
        }

        findViewById<MaterialButton>(R.id.resetButton).setOnClickListener {
            // Deliberately keeps the retune: it is a listening preference, not
            // part of the shaping the rest of this screen does.
            commit(FxSettings(retune432 = settings.retune432))
            render()
        }

        render()
    }

    private fun buildBands() {
        val inflater = LayoutInflater.from(this)
        for (index in 0 until FxSettings.BANDS) {
            val row = inflater.inflate(R.layout.item_eq_band, bandHolder, false)
            val slider = row.findViewById<Slider>(R.id.bandSlider)
            val value = row.findViewById<TextView>(R.id.bandValue)

            row.findViewById<TextView>(R.id.bandLabel).text = FxSettings.bandLabel(index)

            // The painted number is not enough on its own: a screen reader has
            // to hear which band it is on as well as where it sits.
            slider.contentDescription = getString(
                R.string.fx_band, FxSettings.bandLabel(index), settings.bandGainsDb[index]
            )
            slider.addOnChangeListener { _, gain, fromUser ->
                if (!fromUser || loading) return@addOnChangeListener
                val gains = settings.bandGainsDb.copyOf()
                gains[index] = gain.toDouble()
                commit(settings.copy(bandGainsDb = gains))
                value.text = getString(R.string.fx_gain_db, gain)
                slider.contentDescription =
                    getString(R.string.fx_band, FxSettings.bandLabel(index), gain)
            }

            bandSliders += slider
            bandValues += value
            bandHolder.addView(row)
        }
    }

    /**
     * The compressor's five controls, built from one description each so the
     * ranges and the units live in a single place.
     */
    private fun buildCompressorControls() {
        addSlider(
            compBody, getString(R.string.fx_comp_threshold), -60f, 0f, 1f,
            { it.compThresholdDb.toFloat() },
            { s, v -> s.copy(compThresholdDb = v.toDouble()) },
            { getString(R.string.fx_gain_db, it) }
        )
        addSlider(
            compBody, getString(R.string.fx_comp_ratio), 1f, 20f, 0.5f,
            { it.compRatio.toFloat() },
            { s, v -> s.copy(compRatio = v.toDouble()) },
            { getString(R.string.fx_comp_ratio_value, it) }
        )
        addSlider(
            compBody, getString(R.string.fx_comp_attack), 1f, 100f, 1f,
            { it.compAttackMs.toFloat() },
            { s, v -> s.copy(compAttackMs = v.toDouble()) },
            { getString(R.string.fx_ms, it) }
        )
        addSlider(
            compBody, getString(R.string.fx_comp_release), 20f, 1000f, 10f,
            { it.compReleaseMs.toFloat() },
            { s, v -> s.copy(compReleaseMs = v.toDouble()) },
            { getString(R.string.fx_ms, it) }
        )
        addSlider(
            compBody, getString(R.string.fx_comp_makeup), 0f, 24f, 0.5f,
            { it.compMakeupDb.toFloat() },
            { s, v -> s.copy(compMakeupDb = v.toDouble()) },
            { getString(R.string.fx_gain_db, it) }
        )
    }

    private val boundSliders = mutableListOf<Pair<Slider, (FxSettings) -> Float>>()
    private val boundReadouts = mutableListOf<Triple<TextView, (FxSettings) -> Float, (Float) -> String>>()

    private fun addSlider(
        parent: ViewGroup,
        label: String,
        from: Float,
        to: Float,
        step: Float,
        read: (FxSettings) -> Float,
        write: (FxSettings, Float) -> FxSettings,
        format: (Float) -> String,
    ) {
        val inflater = LayoutInflater.from(this)
        val row = inflater.inflate(R.layout.item_eq_band, parent, false)

        val caption = row.findViewById<TextView>(R.id.bandLabel)
        val slider = row.findViewById<Slider>(R.id.bandSlider)
        val value = row.findViewById<TextView>(R.id.bandValue)

        // The band row's label column is sized for "16k"; a compressor control
        // needs a word, so it gets its own line above the fader instead.
        caption.visibility = View.GONE

        val holder = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
        }
        val heading = TextView(this).apply {
            text = label
            setTextAppearance(R.style.TextAppearance_Xfb_SectionLabel)
        }
        holder.addView(heading)
        holder.addView(row)
        parent.addView(holder)

        slider.valueFrom = from
        slider.valueTo = to
        slider.stepSize = step
        slider.contentDescription = label
        slider.addOnChangeListener { _, v, fromUser ->
            if (!fromUser || loading) return@addOnChangeListener
            commit(write(settings, v))
            value.text = format(v)
        }

        boundSliders += slider to read
        boundReadouts += Triple(value, read, format)
    }

    /** Saves, then nudges the service so a running set changes as you listen. */
    private fun commit(updated: FxSettings) {
        settings = updated
        store.save(updated)
        startService(
            Intent(this, PlaybackService::class.java)
                .setAction(PlaybackService.ACTION_RELOAD_FX)
        )
    }

    private fun render() {
        loading = true

        retuneSwitch.isChecked = settings.retune432
        eqSwitch.isChecked = settings.eqEnabled
        compSwitch.isChecked = settings.compEnabled

        preampSlider.value = settings.preampDb.toFloat().coerceIn(-12f, 12f)
        preampValue.text = getString(R.string.fx_gain_db, preampSlider.value)

        for (index in bandSliders.indices) {
            val gain = settings.bandGainsDb[index].toFloat().coerceIn(-12f, 12f)
            bandSliders[index].value = gain
            bandValues[index].text = getString(R.string.fx_gain_db, gain)
            bandSliders[index].contentDescription =
                getString(R.string.fx_band, FxSettings.bandLabel(index), gain)
        }

        for ((slider, read) in boundSliders) {
            slider.value = read(settings).coerceIn(slider.valueFrom, slider.valueTo)
        }
        for ((view, read, format) in boundReadouts) {
            view.text = format(read(settings))
        }

        loading = false

        updateEnabledState()
    }

    /**
     * Switched off, a section's controls stay visible but plainly inert —
     * hiding them would make the card jump about as the switch is toggled.
     */
    private fun updateEnabledState() {
        setEnabled(eqBody, settings.eqEnabled && FxChain.isAvailable)
        setEnabled(compBody, settings.compEnabled && FxChain.isAvailable)
    }

    private fun setEnabled(view: View, enabled: Boolean) {
        view.animate().alpha(if (enabled) 1f else 0.4f).setDuration(180).start()
        setEnabledDeep(view, enabled)
    }

    private fun setEnabledDeep(view: View, enabled: Boolean) {
        view.isEnabled = enabled
        if (view is ViewGroup) {
            for (i in 0 until view.childCount) setEnabledDeep(view.getChildAt(i), enabled)
        }
    }
}
