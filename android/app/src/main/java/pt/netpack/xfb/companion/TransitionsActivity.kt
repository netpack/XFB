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
        list.adapter = TransitionAdapter(tracks, cacheDir) { index, overlap ->
            tracks[index] = tracks[index].copy(overlapMs = overlap)
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
        holder.wave.overlapMs = incoming.overlapMs
        holder.wave.setTracks(
            WaveformStore.peek(outgoing.file),
            WaveformStore.peek(incoming.file)
        )
        holder.wave.onOverlapChanged = { overlap -> onOverlap(trackIndex, overlap) }

        // Decoding is slow the first time and instant afterwards; the row draws
        // straight away and fills in when the peaks arrive.
        val target = holder.wave
        val tag = incoming.id
        target.tag = tag
        for (file in listOf(outgoing.file, incoming.file)) {
            WaveformStore.fetch(file, cacheDir) {
                main.post {
                    if (target.tag == tag) {
                        target.setTracks(
                            WaveformStore.peek(outgoing.file),
                            WaveformStore.peek(incoming.file)
                        )
                    }
                }
            }
        }
    }

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val label: TextView = view.findViewById(R.id.label)
        val wave: CrossfadeView = view.findViewById(R.id.wave)
    }
}
