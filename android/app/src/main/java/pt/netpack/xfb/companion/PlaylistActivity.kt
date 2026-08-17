package pt.netpack.xfb.companion

import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.view.animation.AnimationUtils
import android.widget.Button
import com.google.android.material.appbar.MaterialToolbar
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

/**
 * One playlist: what is in it, and getting it onto the phone.
 *
 * Downloads run one track at a time and resume where they left off, so a
 * dropped connection costs the current track's tail rather than the whole set.
 */
class PlaylistActivity : AppCompatActivity() {

    private lateinit var store: PairingStore

    private lateinit var list: RecyclerView
    private lateinit var progress: ProgressBar
    private lateinit var summaryLabel: TextView
    private lateinit var downloadButton: Button
    private lateinit var playButton: Button
    private lateinit var crossfadesButton: Button
    private lateinit var library: LibraryStore
    private val adapter = TrackAdapter()

    private var tracks: List<Track> = emptyList()
    private var downloadJob: Job? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_playlist)

        list = findViewById(R.id.list)
        progress = findViewById(R.id.progress)
        summaryLabel = findViewById(R.id.summaryLabel)
        downloadButton = findViewById(R.id.downloadButton)
        playButton = findViewById(R.id.playButton)
        crossfadesButton = findViewById(R.id.crossfadesButton)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        title = intent.getStringExtra(EXTRA_TITLE) ?: getString(R.string.playlists_title)
        toolbar.setNavigationOnClickListener { finish() }

        store = PairingStore(this)
        library = LibraryStore(this)

        list.layoutManager = LinearLayoutManager(this)
        list.adapter = adapter
        list.layoutAnimation = AnimationUtils.loadLayoutAnimation(this, R.anim.layout_rise)

        downloadButton.setOnClickListener {
            if (downloadJob?.isActive == true) cancelDownload() else startDownload()
        }

        crossfadesButton.setOnClickListener {
            startActivity(
                Intent(this, TransitionsActivity::class.java)
                    .putExtra(TransitionsActivity.EXTRA_NAME, playlistName())
            )
        }

        playButton.setOnClickListener {
            startActivity(
                Intent(this, PlayerActivity::class.java)
                    .putExtra(PlayerActivity.EXTRA_PLAYLIST, playlistName())
                    .putExtra(PlayerActivity.EXTRA_INDEX, 0)
            )
        }

        load()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    override fun onDestroy() {
        // Tied to the screen on purpose: a background download service is
        // phase 03 work, and pretending otherwise would leave a job running
        // with nothing to report to.
        downloadJob?.cancel()
        super.onDestroy()
    }

    private fun playlistName(): String = intent.getStringExtra(EXTRA_NAME).orEmpty()

    private fun load() {
        val name = playlistName()

        // A playlist built here has no station copy to ask for, and one already
        // downloaded should still open with no station in reach — taking a set
        // on the road is the whole point.
        if (library.isLocalPlaylist(name)) {
            showLocal(name)
            return
        }

        val station = store.station() ?: run { showLocal(name); return }

        progress.visibility = View.VISIBLE
        lifecycleScope.launch {
            runCatching { SyncClient.playlist(station, name) }
                .onSuccess { loaded ->
                    progress.visibility = View.GONE
                    tracks = loaded
                    adapter.submit(loaded, library)
                    updateSummary()
                    downloadButton.isEnabled = loaded.isNotEmpty()
                }
                .onFailure { error ->
                    progress.visibility = View.GONE
                    if (!showLocal(name)) {
                        summaryLabel.text =
                            error.message ?: getString(R.string.error_generic)
                    }
                }
        }
    }

    /** Falls back to the manifest on disk. Returns false when there is none. */
    private fun showLocal(name: String): Boolean {
        val saved = library.loadManifest(name) ?: return false

        tracks = saved.tracks.map { track ->
            Track(
                id = track.id,
                artist = track.artist,
                song = track.song,
                duration = "",
                bytes = track.bytes,
                overlapMs = track.overlapMs,
                volumeEnvelope = track.volumeEnvelope
            )
        }
        progress.visibility = View.GONE
        adapter.submit(tracks, library)
        updateSummary()
        // Nothing to fetch: these files are already here.
        downloadButton.isEnabled = false
        return true
    }

    private fun startDownload() {
        val station = store.station() ?: return
        if (tracks.isEmpty()) return

        downloadButton.setText(R.string.download_stop)

        downloadJob = lifecycleScope.launch {
            var failed = 0
            var firstError: String? = null

            for ((index, track) in tracks.withIndex()) {
                summaryLabel.text = getString(
                    R.string.download_progress, index + 1, tracks.size, track.label
                )

                var lastPercent = -1
                val result = runCatching {
                    SyncClient.downloadTrack(
                        station, track, library.trackFile(track)
                    ) { downloaded, total ->
                        // This runs on the download thread. Touching the
                        // adapter directly from here throws, and because the
                        // whole call sits inside runCatching that surfaced as
                        // a mysteriously truncated file rather than as a crash.
                        val percent =
                            if (total > 0) ((downloaded * 100) / total).toInt() else 0
                        if (percent != lastPercent) {
                            lastPercent = percent
                            runOnUiThread { adapter.updateProgress(track.id, downloaded, total) }
                        }
                    }
                }

                // Cancellation is the operator stopping the run, not a failure.
                result.exceptionOrNull()?.let { if (it is CancellationException) throw it }

                if (result.isFailure) {
                    failed++
                    adapter.markFailed(track.id)
                    val error = result.exceptionOrNull()
                    if (firstError == null) firstError = error?.message
                    Log.w(TAG, "Download failed for ${track.label}", error)
                } else {
                    adapter.markComplete(track.id)
                }
            }

            // The manifest is what phase 03 will read, so it is written once
            // everything that could arrive has arrived.
            library.saveManifest(playlistName(), tracks)

            downloadButton.setText(R.string.download_start)
            playButton.isEnabled = tracks.any { library.isComplete(it) }
            crossfadesButton.isEnabled = playButton.isEnabled
            summaryLabel.text = when {
                failed == 0 -> getString(R.string.download_complete, tracks.size)
                firstError != null -> getString(
                    R.string.download_partial_reason,
                    tracks.size - failed, tracks.size, firstError
                )
                else -> getString(R.string.download_partial, tracks.size - failed, tracks.size)
            }
        }
    }

    private fun cancelDownload() {
        downloadJob?.cancel()
        downloadJob = null
        downloadButton.setText(R.string.download_start)
        summaryLabel.setText(R.string.download_stopped)
    }

    private fun updateSummary() {
        val onPhone = tracks.count { library.isComplete(it) }
        summaryLabel.text = getString(R.string.download_summary, onPhone, tracks.size)
        // Playing needs both the audio and the manifest that carries the mix.
        val ready = onPhone > 0 && library.manifestExists(playlistName())
        playButton.isEnabled = ready
        crossfadesButton.isEnabled = ready
    }

    companion object {
        const val EXTRA_NAME = "name"
        const val EXTRA_TITLE = "title"
        private const val TAG = "XfbDownload"
    }
}

private class TrackAdapter : RecyclerView.Adapter<TrackAdapter.Holder>() {

    private var items: List<Track> = emptyList()
    private var library: LibraryStore? = null
    private val progress = mutableMapOf<String, Int>()
    private val failedIds = mutableSetOf<String>()

    fun submit(tracks: List<Track>, library: LibraryStore) {
        this.items = tracks
        this.library = library
        progress.clear()
        failedIds.clear()
        tracks.forEach { track ->
            if (library.isComplete(track)) progress[track.id] = 100
        }
        notifyDataSetChanged()
    }

    fun updateProgress(id: String, downloaded: Long, total: Long) {
        val percent = if (total > 0) ((downloaded * 100) / total).toInt() else 0
        progress[id] = percent.coerceIn(0, 100)
        notifyItemChangedById(id)
    }

    fun markComplete(id: String) {
        progress[id] = 100
        failedIds.remove(id)
        notifyItemChangedById(id)
    }

    fun markFailed(id: String) {
        failedIds.add(id)
        notifyItemChangedById(id)
    }

    private fun notifyItemChangedById(id: String) {
        val index = items.indexOfFirst { it.id == id }
        if (index >= 0) notifyItemChanged(index)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_track, parent, false)
        return Holder(view)
    }

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val track = items[position]
        val context = holder.itemView.context

        holder.title.text = track.label
        holder.index.text = (position + 1).toString()

        val percent = progress[track.id] ?: 0
        holder.bar.progress = percent
        holder.bar.visibility = if (percent in 1..99) View.VISIBLE else View.GONE

        // A tick for the ordinary case, words only when there is something to
        // say — a column of "On phone" repeated down the screen is just noise.
        val onPhone = percent >= 100 && !failedIds.contains(track.id)
        holder.onPhone.visibility = if (onPhone) View.VISIBLE else View.GONE

        val state = when {
            failedIds.contains(track.id) -> context.getString(R.string.track_failed)
            onPhone -> ""
            percent > 0 -> "$percent%"
            track.duration.isNotEmpty() -> track.duration
            else -> ""
        }
        holder.state.text = state
        holder.state.visibility = if (state.isEmpty()) View.GONE else View.VISIBLE

        // The overlap is the desktop's crossfade into this track; showing it
        // makes clear the mix travelled with the playlist.
        holder.detail.visibility = if (track.overlapMs > 0) View.VISIBLE else View.GONE
        if (track.overlapMs > 0) {
            holder.detail.text = context.getString(
                R.string.track_overlap, track.overlapMs / 1000.0
            )
        }
    }

    override fun getItemCount(): Int = items.size

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val title: TextView = view.findViewById(R.id.title)
        val index: TextView = view.findViewById(R.id.index)
        val state: TextView = view.findViewById(R.id.state)
        val detail: TextView = view.findViewById(R.id.detail)
        val bar: ProgressBar = view.findViewById(R.id.bar)
        val onPhone: View = view.findViewById(R.id.onPhone)
    }
}
