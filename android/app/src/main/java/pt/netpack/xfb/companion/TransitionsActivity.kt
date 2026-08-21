package pt.netpack.xfb.companion

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.appbar.MaterialToolbar
import java.io.File

/**
 * The crossfades in a playlist, one row per join, each drawn as a wave view you
 * can drag.
 *
 * XFB stores an overlap on the track being mixed *into*, so a playlist of N
 * tracks has N-1 editable transitions; the first track has nothing before it.
 */
class TransitionsActivity : AppCompatActivity() {

    private lateinit var library: LibraryStore
    private lateinit var list: RecyclerView
    private lateinit var emptyLabel: TextView

    private var playlist: SavedPlaylist? = null
    private var tracks: MutableList<SavedTrack> = mutableListOf()
    private var dirty = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_transitions)

        list = findViewById(R.id.list)
        emptyLabel = findViewById(R.id.emptyLabel)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        setTitle(R.string.transitions_title)
        toolbar.setNavigationOnClickListener { finish() }

        library = LibraryStore(this)
        val name = intent.getStringExtra(EXTRA_NAME).orEmpty()
        val loaded = library.loadManifest(name)
        if (loaded == null) {
            finish()
            return
        }

        playlist = loaded
        tracks = loaded.tracks.toMutableList()

        list.layoutManager = LinearLayoutManager(this)
        // Dragging a join pins it: from then on it is the operator's number and
        // Auto-mix stops having an opinion about it.
        list.adapter = TransitionAdapter(tracks, cacheDir, AutoMixSettings.isEnabled(this)) {
            index, overlap ->
            tracks[index] = tracks[index].copy(overlapMs = overlap, overlapPinned = true)
            dirty = true
        }

        emptyLabel.visibility = if (tracks.size < 2) View.VISIBLE else View.GONE
    }

    override fun onStop() {
        // Saved on the way out rather than on every drag: a drag produces
        // hundreds of values and none of the intermediate ones matter.
        if (dirty) {
            playlist?.let { library.saveLocalPlaylist(it.name, tracks) }
            dirty = false
            Toast.makeText(this, R.string.transitions_saved, Toast.LENGTH_SHORT).show()
        }
        super.onStop()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    companion object {
        const val EXTRA_NAME = "name"
    }
}

private class TransitionAdapter(
    private val tracks: List<SavedTrack>,
    private val cacheDir: File,
    private val autoMix: Boolean,
    private val onOverlap: (index: Int, overlapMs: Long) -> Unit
) : RecyclerView.Adapter<TransitionAdapter.Holder>() {

    private val main = Handler(Looper.getMainLooper())

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_transition, parent, false)
        return Holder(view)
    }

    // One row per join, so row 0 is the transition into track 1.
    override fun getItemCount(): Int = (tracks.size - 1).coerceAtLeast(0)

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val outgoing = tracks[position]
        val incoming = tracks[position + 1]
        val trackIndex = position + 1

        holder.label.text = holder.itemView.context.getString(
            R.string.transitions_join, outgoing.song, incoming.song
        )

        holder.wave.onOverlapChanged = null
        holder.wave.maxOverlapMs = MAX_OVERLAP_MS
        // What this join will actually play with, which for one nobody has
        // touched is whatever Auto-mix makes of it. Showing the stored zero
        // instead would tell the operator there is no crossfade on a join that
        // has one.
        holder.wave.overlapMs = AutoMix.effectiveOverlapMs(autoMix, outgoing, incoming)
        holder.wave.setTracks(
            WaveformStore.peek(outgoing.file),
            WaveformStore.peek(incoming.file)
        )
        holder.wave.onOverlapChanged = { overlap -> onOverlap(trackIndex, overlap) }

        // Decoding is slow the first time and instant afterwards; the row draws
        // straight away, pulses while it waits, and fills in when peaks arrive.
        val target = holder.wave
        val tag = incoming.id
        target.tag = tag

        val pending = listOf(outgoing.file, incoming.file)
            .filter { WaveformStore.peek(it) == null }
            .toMutableSet()
        target.setAnalysing(outgoing.file in pending, incoming.file in pending)

        for (file in pending.toList()) {
            WaveformStore.fetch(file, cacheDir) {
                main.post {
                    // The row may have been recycled onto another transition
                    // while this was decoding; the tag is what says so.
                    if (target.tag != tag) return@post
                    pending -= file
                    target.setTracks(
                        WaveformStore.peek(outgoing.file),
                        WaveformStore.peek(incoming.file)
                    )
                    target.setAnalysing(
                        outgoing.file in pending, incoming.file in pending
                    )
                }
            }
        }

        // The auto value arrives with the peaks it is measured from — but a
        // track already read has no fetch above to ride in on, so this asks in
        // its own right. Nulling the listener first: filling the row in is not
        // the operator setting a crossfade.
        if (autoMix && !incoming.overlapPinned) {
            val refresh: () -> Unit = { main.post {
                if (target.tag == tag) {
                    target.onOverlapChanged = null
                    target.overlapMs =
                        AutoMix.effectiveOverlapMs(true, outgoing, incoming)
                    target.onOverlapChanged = { overlap -> onOverlap(trackIndex, overlap) }
                }
            } }
            AutoMix.ensure(outgoing.file, cacheDir, refresh)
            AutoMix.ensure(incoming.file, cacheDir, refresh)
            refresh()
        }
    }

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val label: TextView = view.findViewById(R.id.label)
        val wave: CrossfadeView = view.findViewById(R.id.wave)
    }
}
