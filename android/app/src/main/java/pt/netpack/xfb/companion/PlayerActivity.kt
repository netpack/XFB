package pt.netpack.xfb.companion

import android.Manifest
import android.content.ComponentName
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.view.View
import android.graphics.BitmapFactory
import android.os.Looper
import android.view.Menu
import android.view.MenuItem
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import androidx.media3.common.Player
import androidx.media3.session.MediaController
import androidx.media3.session.SessionToken
import com.google.common.util.concurrent.MoreExecutors

/**
 * Transport for the playlist on this phone. The work happens in
 * [PlaybackService]; this is a window onto it, so playback carries on when the
 * screen goes off or this screen is closed.
 */
class PlayerActivity : AppCompatActivity() {

    private var controller: MediaController? = null
    private val ticker = Handler(Looper.getMainLooper())

    private lateinit var titleLabel: TextView
    private lateinit var artistLabel: TextView
    private lateinit var positionLabel: TextView
    private lateinit var strip: WaveStripView
    private lateinit var crossfade: CrossfadeView
    private lateinit var crossfadeGroup: View
    private lateinit var crossfadeLabel: TextView
    private lateinit var playButton: MaterialButton
    private lateinit var previousButton: Button
    private lateinit var nextButton: Button
    private lateinit var turntable: TurntableView

    private var shownArtwork: ByteArray? = null
    private lateinit var library: LibraryStore
    private var loadedPlaylist: String? = null
    private var loadedIndex = -1
    private var loadedTracks: List<SavedTrack> = emptyList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_player)

        titleLabel = findViewById(R.id.titleLabel)
        artistLabel = findViewById(R.id.artistLabel)
        positionLabel = findViewById(R.id.positionLabel)
        strip = findViewById(R.id.strip)
        crossfade = findViewById(R.id.crossfade)
        crossfadeGroup = findViewById(R.id.crossfadeGroup)
        crossfadeLabel = findViewById(R.id.crossfadeLabel)
        playButton = findViewById(R.id.playButton)
        previousButton = findViewById(R.id.previousButton)
        nextButton = findViewById(R.id.nextButton)
        turntable = findViewById(R.id.turntable)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        setTitle(R.string.player_title)
        toolbar.setNavigationOnClickListener { finish() }

        library = LibraryStore(this)
        requestNotificationPermissionIfNeeded()

        playButton.setOnClickListener {
            val player = controller ?: return@setOnClickListener
            if (player.isPlaying) player.pause() else player.play()
            render()
        }
        previousButton.setOnClickListener { controller?.seekToPreviousMediaItem() }
        nextButton.setOnClickListener { controller?.seekToNextMediaItem() }

        strip.onSeek = { position -> controller?.seekTo(position) }

        setPlayIcon(playing = false)

        crossfade.onOverlapChanged = { overlap -> rememberOverlap(overlap) }

        // Starting the service is what loads the queue; the controller below
        // only drives it.
        val playlist = intent.getStringExtra(EXTRA_PLAYLIST)
        if (playlist != null) {
            ContextCompat.startForegroundService(
                this,
                Intent(this, PlaybackService::class.java)
                    .putExtra(PlaybackService.EXTRA_PLAYLIST, playlist)
                    .putExtra(PlaybackService.EXTRA_INDEX, intent.getIntExtra(EXTRA_INDEX, 0))
            )
        }
    }

    override fun onStart() {
        super.onStart()
        val token = SessionToken(this, ComponentName(this, PlaybackService::class.java))
        val future = MediaController.Builder(this, token).buildAsync()
        future.addListener({
            controller = runCatching { future.get() }.getOrNull()
            controller?.addListener(object : Player.Listener {
                override fun onEvents(player: Player, events: Player.Events) = render()
            })
            render()
        }, MoreExecutors.directExecutor())

        ticker.post(tick)
    }

    override fun onStop() {
        ticker.removeCallbacksAndMessages(null)
        controller?.release()
        controller = null
        super.onStop()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    private val tick = object : Runnable {
        override fun run() {
            render()
            ticker.postDelayed(this, 500)
        }
    }

    private fun render() {
        val player = controller ?: return

        val metadata = player.mediaMetadata
        titleLabel.text = metadata.title ?: getString(R.string.player_nothing)
        artistLabel.text = metadata.artist ?: ""

        // ExoPlayer pulls embedded cover art out of the file itself, so this
        // works offline with no help from the desktop. Tracks without any fall
        // back to XFB's own icon rather than an empty hole.
        val art = metadata.artworkData
        if (!art.contentEquals(shownArtwork)) {
            shownArtwork = art
            turntable.setLabel(art?.let { BitmapFactory.decodeByteArray(it, 0, it.size) })
        }
        // The record turns while the music plays and stops when it does.
        turntable.setSpinning(player.isPlaying)

        setPlayIcon(player.isPlaying)

        val duration = player.duration.takeIf { it > 0 } ?: 0L
        val position = player.currentPosition.coerceAtLeast(0L)

        strip.durationMs = duration
        strip.positionMs = position
        positionLabel.text = getString(
            R.string.player_position, clock(position), clock(duration)
        )

        refreshMixViews(player)
    }

    /**
     * Keeps the on-air strip and the transition editor pointed at whatever is
     * playing. The playlist name rides along on the metadata, so this works
     * even when the screen was opened from the notification with no extras.
     */
    private fun refreshMixViews(player: MediaController) {
        val playlistName = player.mediaMetadata.albumTitle?.toString().orEmpty()
        val index = player.currentMediaItemIndex

        if (playlistName != loadedPlaylist || index != loadedIndex) {
            loadedPlaylist = playlistName
            loadedIndex = index
            loadedTracks = library.loadManifest(playlistName)?.tracks.orEmpty()

            val current = loadedTracks.getOrNull(index)
            val next = loadedTracks.getOrNull(index + 1)

            strip.setWaveform(null, current?.volumeEnvelope.orEmpty())
            current?.let { track ->
                WaveformStore.fetch(track.file, cacheDir) { wave ->
                    runOnUiThread {
                        if (loadedIndex == index)
                            strip.setWaveform(wave, track.volumeEnvelope)
                    }
                }
            }

            if (next == null) {
                crossfadeGroup.visibility = View.GONE
            } else {
                crossfadeGroup.visibility = View.VISIBLE
                crossfadeLabel.text = getString(R.string.player_into_next, next.song)
                crossfade.onOverlapChanged = null
                crossfade.maxOverlapMs = MAX_OVERLAP_MS
                crossfade.overlapMs = next.overlapMs
                crossfade.setTracks(
                    WaveformStore.peek(current!!.file), WaveformStore.peek(next.file)
                )
                crossfade.onOverlapChanged = { overlap -> rememberOverlap(overlap) }
                for (file in listOfNotNull(current.file, next.file)) {
                    WaveformStore.fetch(file, cacheDir) {
                        runOnUiThread {
                            if (loadedIndex == index) {
                                crossfade.setTracks(
                                    WaveformStore.peek(current.file),
                                    WaveformStore.peek(next.file)
                                )
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Writes the new overlap to the playlist and tells the service, so a change
     * made mid-set applies to the segue that is about to happen.
     */
    private fun rememberOverlap(overlapMs: Long) {
        val name = loadedPlaylist ?: return
        val nextIndex = loadedIndex + 1
        if (nextIndex !in loadedTracks.indices) return

        loadedTracks = loadedTracks.toMutableList().also {
            it[nextIndex] = it[nextIndex].copy(overlapMs = overlapMs)
        }
        library.saveLocalPlaylist(name, loadedTracks)

        startService(
            Intent(this, PlaybackService::class.java)
                .setAction(PlaybackService.ACTION_RELOAD_MIX)
        )
    }

    private fun clock(ms: Long): String {
        val total = ms / 1000
        val minutes = total / 60
        val seconds = total % 60
        return "%d:%02d".format(minutes, seconds)
    }

    // Through the activity rather than Toolbar.inflateMenu: once the toolbar is
    // the support action bar, the activity owns its menu and anything inflated
    // straight onto the view is discarded.
    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.player, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == R.id.action_fx) {
            startActivity(Intent(this, FxActivity::class.java))
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    /**
     * Swaps play for pause. The button dips through a slightly smaller scale as
     * it changes, so the tap registers even where the two glyphs sit in the
     * same place — the icon alone changing is easy to miss.
     */
    private fun setPlayIcon(playing: Boolean) {
        val icon = if (playing) R.drawable.ic_pause else R.drawable.ic_play
        if (playButton.tag == icon) return
        playButton.tag = icon

        playButton.animate()
            .scaleX(0.86f).scaleY(0.86f)
            .setDuration(90)
            .withEndAction {
                playButton.setIconResource(icon)
                playButton.contentDescription =
                    getString(if (playing) R.string.player_pause else R.string.player_play)
                playButton.animate().scaleX(1f).scaleY(1f).setDuration(130).start()
            }
            .start()
    }

    private fun requestNotificationPermissionIfNeeded() {
        if (Build.VERSION.SDK_INT < 33) return
        val granted = ContextCompat.checkSelfPermission(
            this, Manifest.permission.POST_NOTIFICATIONS
        ) == PackageManager.PERMISSION_GRANTED
        if (!granted) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1)
        }
    }

    companion object {
        const val EXTRA_PLAYLIST = "playlist"
        const val EXTRA_INDEX = "index"
    }
}
