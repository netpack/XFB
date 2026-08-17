package pt.netpack.xfb.companion

import android.content.Intent
import android.os.Bundle
import android.view.View
import java.io.File
import androidx.appcompat.app.AppCompatActivity
import android.view.animation.OvershootInterpolator
import android.widget.Button
import android.widget.EditText
import android.widget.ProgressBar
import android.widget.TextView
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch

/**
 * The way in. Either a pairing link arrives from XFB's own pairing page, or the
 * operator reads the address and code off the desktop and they get typed here.
 */
class PairActivity : AppCompatActivity() {

    private lateinit var store: PairingStore

    private lateinit var addressField: EditText
    private lateinit var codeField: EditText
    private lateinit var pairButton: Button
    private lateinit var forgetButton: Button
    private lateinit var openLibraryButton: Button
    private lateinit var nowPlayingButton: Button
    private lateinit var updateButton: Button
    private lateinit var fxButton: Button
    private lateinit var pairingForm: View
    private lateinit var pairedGroup: View
    private lateinit var brand: View
    private lateinit var pairedLabel: TextView
    private lateinit var statusLabel: TextView
    private lateinit var progress: ProgressBar

    /** Suppresses the entrance animation on the first render only. */
    private var rendered = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_pair)

        addressField = findViewById(R.id.addressField)
        codeField = findViewById(R.id.codeField)
        pairButton = findViewById(R.id.pairButton)
        forgetButton = findViewById(R.id.forgetButton)
        openLibraryButton = findViewById(R.id.openLibraryButton)
        nowPlayingButton = findViewById(R.id.nowPlayingButton)
        updateButton = findViewById(R.id.updateButton)
        fxButton = findViewById(R.id.fxButton)
        pairingForm = findViewById(R.id.pairingForm)
        pairedGroup = findViewById(R.id.pairedGroup)
        brand = findViewById(R.id.brand)
        pairedLabel = findViewById(R.id.pairedLabel)
        statusLabel = findViewById(R.id.statusLabel)
        progress = findViewById(R.id.progress)

        store = PairingStore(this)

        animateBrand()

        pairButton.setOnClickListener {
            val request = PairingRequest.fromTyped(
                addressField.text.toString(),
                codeField.text.toString()
            )
            if (request == null) {
                showStatus(getString(R.string.pair_bad_details))
                return@setOnClickListener
            }
            pair(request)
        }

        forgetButton.setOnClickListener {
            store.forget()
            render()
        }

        openLibraryButton.setOnClickListener { openLibrary() }

        // Goes straight to whatever the service is holding; no extras means
        // "attach to what is already playing" rather than starting a playlist.
        nowPlayingButton.setOnClickListener {
            startActivity(Intent(this, PlayerActivity::class.java))
        }

        fxButton.setOnClickListener {
            startActivity(Intent(this, FxActivity::class.java))
        }

        handleIntent(intent)
        render()
    }

    /**
     * The XFB radio settles in as the screen opens. Only on a cold start —
     * coming back from the playlists to see the icon bounce again would be
     * fidgety rather than polished.
     */
    private fun animateBrand() {
        brand.alpha = 0f
        brand.scaleX = 0.86f
        brand.scaleY = 0.86f
        brand.animate()
            .alpha(1f)
            .scaleX(1f)
            .scaleY(1f)
            .setStartDelay(80)
            .setDuration(520)
            .setInterpolator(OvershootInterpolator(1.1f))
            .start()
    }

    override fun onResume() {
        super.onResume()
        // Not onCreate: this activity is singleTask, so coming back to it
        // delivers onNewIntent and onCreate never runs again — which meant the
        // check never happened in the session where pairing had just finished.
        checkForUpdate()
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        setIntent(intent)
        handleIntent(intent)
    }

    /** A tap on "Open in XFB" from the pairing page lands here. */
    private fun handleIntent(intent: Intent?) {
        if (intent?.action != Intent.ACTION_VIEW) return
        val request = PairingRequest.fromUri(intent.data)
        if (request == null) {
            showStatus(getString(R.string.pair_bad_link))
            return
        }
        // The link is single use, so it must not be replayed if the activity
        // is recreated — for a rotation, say.
        intent.data = null
        pair(request)
    }

    private fun pair(request: PairingRequest) {
        setBusy(true)
        showStatus(getString(R.string.pair_working, request.host))

        lifecycleScope.launch {
            runCatching { SyncClient.pair(request) }
                .onSuccess { station ->
                    store.save(station)
                    setBusy(false)
                    render()
                    showStatus(getString(R.string.pair_done, station.host))
                    openLibrary()
                }
                .onFailure { error ->
                    setBusy(false)
                    showStatus(
                        error.message ?: getString(R.string.pair_failed_generic)
                    )
                }
        }
    }

    private fun openLibrary() {
        if (store.station() == null) return
        startActivity(Intent(this, PlaylistsActivity::class.java))
    }

    /**
     * Asks the paired station whether it has a newer build. Silent when it has
     * not, or cannot be reached — an update prompt is not worth an error.
     */
    private fun checkForUpdate() {
        val station = store.station() ?: return
        lifecycleScope.launch {
            val release = runCatching {
                AppUpdater.checkForUpdate(station, BuildConfig.VERSION_CODE)
            }.getOrNull() ?: return@launch

            updateButton.visibility = View.VISIBLE
            updateButton.text = getString(R.string.update_available, release.versionName)
            updateButton.setOnClickListener { downloadAndInstall(station, release) }
        }
    }

    private fun downloadAndInstall(station: Station, release: CompanionRelease) {
        updateButton.isEnabled = false
        lifecycleScope.launch {
            var lastPercent = -1
            val apk: File? = runCatching {
                AppUpdater.download(this@PairActivity, station, release) { done, total ->
                    val percent = if (total > 0) ((done * 100) / total).toInt() else 0
                    if (percent != lastPercent) {
                        lastPercent = percent
                        runOnUiThread {
                            updateButton.text =
                                getString(R.string.update_downloading, percent)
                        }
                    }
                }
            }.getOrNull()

            updateButton.isEnabled = true
            if (apk == null) {
                updateButton.text = getString(R.string.update_available, release.versionName)
                showStatus(getString(R.string.update_failed))
                return@launch
            }
            AppUpdater.install(this@PairActivity, apk)
        }
    }

    private fun render() {
        val station = store.station()
        val paired = station != null

        // Pairing and forgetting swap one card for the other, so the change is
        // worth a fade; the first render is just the screen arriving.
        show(pairedGroup, paired, animate = rendered)
        show(pairingForm, !paired, animate = rendered)
        rendered = true

        if (station != null) {
            pairedLabel.text =
                getString(R.string.pair_paired_with, station.host, station.port)
        }
    }

    private fun show(view: View, visible: Boolean, animate: Boolean) {
        val target = if (visible) View.VISIBLE else View.GONE
        if (!animate) {
            view.alpha = 1f
            view.visibility = target
            return
        }
        if (view.visibility == target) return

        if (visible) {
            view.alpha = 0f
            view.visibility = View.VISIBLE
            view.animate().alpha(1f).setDuration(220).start()
        } else {
            view.animate().alpha(0f).setDuration(160)
                .withEndAction { view.visibility = View.GONE }
                .start()
        }
    }

    private fun setBusy(busy: Boolean) {
        progress.visibility = if (busy) View.VISIBLE else View.GONE
        pairButton.isEnabled = !busy
    }

    private fun showStatus(message: String) {
        statusLabel.text = message
        statusLabel.visibility = View.VISIBLE
    }
}
