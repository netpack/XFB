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
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.button.MaterialButton
import androidx.media3.common.C
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

    private var shuffleItem: MenuItem? = null
    private var shownArtwork: ByteArray? = null
    private var shownLabel: android.graphics.Bitmap? = null
    /** Which track the deck is showing, so a change can be animated. */
    private var shownTrackKey: String? = null
    private lateinit var library: LibraryStore
    private var loadedPlaylist: String? = null
    private var loadedIndex = -1
    private var loadedNextIndex = -1
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
            when {
                player.isPlaying -> player.pause()
                // Reached from "Now playing" with nothing queued: play what is
                // on the phone rather than sitting there doing nothing.
                player.mediaItemCount == 0 -> playEverything()
                else -> player.play()
            }
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
        val artChanged = !art.contentEquals(shownArtwork)
        if (artChanged) {
            shownArtwork = art
            shownLabel = art?.let { BitmapFactory.decodeByteArray(it, 0, it.size) }
        }

        // A change of track is what the deck should animate, not a change of
        // picture: two tracks running back to back with the same cover — or
        // with none at all — are still a change of record.
        val trackKey = player.currentMediaItem?.mediaId
        val changedTrack = trackKey != null && shownTrackKey != null && trackKey != shownTrackKey
        if (artChanged || changedTrack) {
            turntable.setLabel(shownLabel, slide = changedTrack)
        }
        shownTrackKey = trackKey

        // The record turns while the music plays and stops when it does.
        turntable.setSpinning(player.isPlaying)

        setPlayIcon(player.isPlaying)
        updateShuffleIcon()

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
        // Asking the player rather than adding one: under shuffle the next
        // track is not the one after this in the stored order, and this card
        // has to name the track the crossfade will actually run into.
        val nextIndex = player.nextMediaItemIndex.takeIf { it != C.INDEX_UNSET } ?: -1

        if (playlistName != loadedPlaylist || index != loadedIndex || nextIndex != loadedNextIndex) {
            if (playlistName != loadedPlaylist) {
                loadedTracks = if (playlistName == PlaybackService.ALL_TRACKS) {
                    library.downloadedTracks()
                } else {
                    library.loadManifest(playlistName)?.tracks.orEmpty()
                }
            }
            loadedPlaylist = playlistName
            loadedIndex = index
            loadedNextIndex = nextIndex

            val current = loadedTracks.getOrNull(index)
            val next = loadedTracks.getOrNull(nextIndex)

            strip.setWaveform(null, current?.volumeEnvelope.orEmpty())
            strip.isAnalysing = current != null
            current?.let { track ->
                WaveformStore.fetch(track.file, cacheDir) { wave ->
                    runOnUiThread {
                        if (loadedIndex != index) return@runOnUiThread
                        // Cleared even when wave is null: a file that cannot be
                        // decoded must stop the indicator, not spin for ever.
                        strip.isAnalysing = false
                        if (wave != null) strip.setWaveform(wave, track.volumeEnvelope)
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

                // Which files are still being read. Tracked by hand rather than
                // by asking the store, because a file that cannot be decoded
                // never gains peaks and would leave its lane pulsing for ever.
                val pending = listOf(current.file, next.file)
                    .filter { WaveformStore.peek(it) == null }
                    .toMutableSet()
                crossfade.setAnalysing(current.file in pending, next.file in pending)

                for (file in pending.toList()) {
                    WaveformStore.fetch(file, cacheDir) {
                        runOnUiThread {
                            if (loadedIndex != index) return@runOnUiThread
                            pending -= file
                            crossfade.setTracks(
                                WaveformStore.peek(current.file),
                                WaveformStore.peek(next.file)
                            )
                            crossfade.setAnalysing(
                                current.file in pending, next.file in pending
                            )
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
        // Never for the gathered set: it has no manifest to write back to, and
        // saving one under its internal name would invent a playlist.
        if (name == PlaybackService.ALL_TRACKS) return

        val nextIndex = loadedNextIndex
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
        shuffleItem = menu.findItem(R.id.action_shuffle)
        updateShuffleIcon()
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.action_fx -> {
                startActivity(Intent(this, FxActivity::class.java))
                return true
            }
            R.id.action_shuffle -> {
                toggleShuffle()
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    /**
     * Shuffle runs through the service rather than straight onto the
     * controller: both decks have to be given the same order, and a controller
     * only reaches the audible one.
     */
    private fun toggleShuffle() {
        val wanted = !(controller?.shuffleModeEnabled ?: false)
        startService(
            Intent(this, PlaybackService::class.java)
                .setAction(PlaybackService.ACTION_SHUFFLE)
                .putExtra(PlaybackService.EXTRA_SHUFFLE, wanted)
        )
        Toast.makeText(
            this,
            if (wanted) R.string.player_shuffle_on else R.string.player_shuffle_off,
            Toast.LENGTH_SHORT
        ).show()
    }

    /** Lit when shuffle is on; the icon is the only thing carrying that state. */
    private fun updateShuffleIcon() {
        val item = shuffleItem ?: return
        val on = controller?.shuffleModeEnabled ?: false
        item.isChecked = on
        item.icon = ContextCompat.getDrawable(this, R.drawable.ic_shuffle)?.apply {
            setTint(
                ContextCompat.getColor(
                    this@PlayerActivity,
                    if (on) R.color.xfb_accent else R.color.xfb_text
                )
            )
        }
        item.title =
            getString(if (on) R.string.player_shuffle_is_on else R.string.player_shuffle_is_off)
    }

    /** Queues everything downloaded onto this phone and starts it. */
    private fun playEverything() {
        if (library.downloadedTracks().isEmpty()) {
            Toast.makeText(this, R.string.library_nothing_downloaded, Toast.LENGTH_LONG).show()
            return
        }
        ContextCompat.startForegroundService(
            this,
            Intent(this, PlaybackService::class.java)
                .putExtra(PlaybackService.EXTRA_PLAYLIST, PlaybackService.ALL_TRACKS)
                .putExtra(PlaybackService.EXTRA_INDEX, 0)
        )
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
