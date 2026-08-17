package pt.netpack.xfb.companion

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import android.widget.Button
import android.widget.LinearLayout
import android.text.Editable
import android.text.TextWatcher
import android.view.animation.AnimationUtils
import android.widget.EditText
import android.widget.ProgressBar
import com.google.android.material.appbar.MaterialToolbar
import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton
import kotlinx.coroutines.launch

/** The playlists the paired station is offering. */
class PlaylistsActivity : AppCompatActivity() {

    private lateinit var store: PairingStore

    private lateinit var list: RecyclerView
    private lateinit var progress: ProgressBar
    private lateinit var emptyLabel: TextView
    private lateinit var errorGroup: LinearLayout
    private lateinit var errorLabel: TextView
    private lateinit var retryButton: Button
    private lateinit var searchField: EditText
    private lateinit var newPlaylistButton: ExtendedFloatingActionButton
    private lateinit var library: LibraryStore
    private val adapter = PlaylistAdapter(::open)
    private var everything: List<PlaylistSummary> = emptyList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_playlists)

        list = findViewById(R.id.list)
        progress = findViewById(R.id.progress)
        emptyLabel = findViewById(R.id.emptyLabel)
        errorGroup = findViewById(R.id.errorGroup)
        errorLabel = findViewById(R.id.errorLabel)
        retryButton = findViewById(R.id.retryButton)
        searchField = findViewById(R.id.searchField)
        newPlaylistButton = findViewById(R.id.newPlaylistButton)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        setTitle(R.string.playlists_title)
        toolbar.setNavigationOnClickListener { finish() }

        store = PairingStore(this)
        library = LibraryStore(this)

        list.layoutManager = LinearLayoutManager(this)
        list.adapter = adapter
        list.layoutAnimation = AnimationUtils.loadLayoutAnimation(this, R.anim.layout_rise)

        // The button collapses to its icon while scrolling, so a long list is
        // never hidden behind the words.
        list.addOnScrollListener(object : RecyclerView.OnScrollListener() {
            override fun onScrolled(view: RecyclerView, dx: Int, dy: Int) {
                if (dy > 6) newPlaylistButton.shrink()
                else if (dy < -6) newPlaylistButton.extend()
            }
        })

        retryButton.setOnClickListener { load() }

        newPlaylistButton.setOnClickListener {
            startActivity(Intent(this, LibraryActivity::class.java))
        }

        searchField.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(t: CharSequence?, a: Int, b: Int, c: Int) = Unit
            override fun onTextChanged(t: CharSequence?, a: Int, b: Int, c: Int) = showFiltered()
            override fun afterTextChanged(t: Editable?) = Unit
        })
    }

    override fun onResume() {
        super.onResume()
        load()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    private fun load() {
        val station = store.station()
        if (station == null) {
            finish()
            return
        }

        // What is already on the phone shows first and always, so the list is
        // useful with no station in reach — which is the whole point of taking
        // a set on the road.
        val local = library.localPlaylistNames().map { name ->
            PlaylistSummary(name = name, title = name, live = false, trackCount = -1)
        }
        everything = local
        showFiltered()

        setBusy(true)
        lifecycleScope.launch {
            runCatching { SyncClient.playlists(station) }
                .onSuccess { fromStation ->
                    setBusy(false)
                    // A station playlist already downloaded appears once, not twice.
                    val localNames = local.map { it.name }.toSet()
                    everything = local + fromStation.filter { it.name !in localNames }
                    showFiltered()
                    errorGroup.visibility = View.GONE
                }
                .onFailure { error ->
                    setBusy(false)
                    if (everything.isEmpty()) {
                        errorLabel.text = error.message ?: getString(R.string.error_generic)
                        errorGroup.visibility = View.VISIBLE
                    }
                }
        }
    }

    private fun showFiltered() {
        val needle = searchField.text.toString().trim()
        val matches = if (needle.isEmpty()) everything
                      else everything.filter { it.title.contains(needle, ignoreCase = true) }
        adapter.submit(matches, library)
        emptyLabel.visibility = if (matches.isEmpty()) View.VISIBLE else View.GONE
    }

    private fun open(playlist: PlaylistSummary) {
        startActivity(
            Intent(this, PlaylistActivity::class.java)
                .putExtra(PlaylistActivity.EXTRA_NAME, playlist.name)
                .putExtra(PlaylistActivity.EXTRA_TITLE, playlist.title)
        )
    }

    private fun setBusy(busy: Boolean) {
        progress.visibility = if (busy) View.VISIBLE else View.GONE
    }
}

private class PlaylistAdapter(
    private val onClick: (PlaylistSummary) -> Unit
) : RecyclerView.Adapter<PlaylistAdapter.Holder>() {

    private var items: List<PlaylistSummary> = emptyList()
    private var library: LibraryStore? = null

    fun submit(playlists: List<PlaylistSummary>, library: LibraryStore) {
        this.items = playlists
        this.library = library
        notifyDataSetChanged()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_playlist, parent, false)
        return Holder(view)
    }

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val item = items[position]
        holder.title.text = item.title

        val context = holder.itemView.context

        // What the set *is* goes in the badge; what is in it goes underneath.
        val badge = when {
            item.live -> context.getString(R.string.playlist_on_air)
            library?.manifestExists(item.name) == true ->
                context.getString(R.string.playlist_saved)
            else -> null
        }
        holder.badge.text = badge.orEmpty()
        holder.badge.visibility = if (badge == null) View.GONE else View.VISIBLE

        val parts = mutableListOf<String>()
        if (item.trackCount >= 0) {
            parts += context.resources.getQuantityString(
                R.plurals.track_count, item.trackCount, item.trackCount
            )
        }
        if (item.live && library?.manifestExists(item.name) == true) {
            parts += context.getString(R.string.playlist_saved)
        }

        holder.subtitle.text = parts.joinToString(" · ")
        holder.subtitle.visibility = if (parts.isEmpty()) View.GONE else View.VISIBLE
        holder.itemView.setOnClickListener { onClick(item) }
    }

    override fun getItemCount(): Int = items.size

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val title: TextView = view.findViewById(R.id.title)
        val subtitle: TextView = view.findViewById(R.id.subtitle)
        val badge: TextView = view.findViewById(R.id.badge)
    }
}
