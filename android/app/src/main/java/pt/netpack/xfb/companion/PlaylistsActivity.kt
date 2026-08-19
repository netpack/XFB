package pt.netpack.xfb.companion

import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Menu
import android.view.MenuItem
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
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

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
    private val poller = Handler(Looper.getMainLooper())

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
        poller.postDelayed(poll, POLL_MS)
    }

    override fun onPause() {
        // Only while the screen is in front: a poll running behind another
        // activity would keep the station awake for nothing.
        poller.removeCallbacksAndMessages(null)
        super.onPause()
    }

    private val poll = object : Runnable {
        override fun run() {
            load(silent = true)
            poller.postDelayed(this, POLL_MS)
        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.playlists, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (item.itemId == R.id.action_refresh) {
            load()
            return true
        }
        return super.onOptionsItemSelected(item)
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    /**
     * @param silent for the background poll: it must not flash the spinner or
     *        raise an error panel over a list that is already on screen, or the
     *        screen would twitch every few seconds on a flaky network.
     */
    private fun load(silent: Boolean = false) {
        val station = store.station()
        if (station == null) {
            finish()
            return
        }

        setBusy(!silent)
        lifecycleScope.launch {
            // What is already on the phone shows first and always, so the list
            // is useful with no station in reach — which is the whole point of
            // taking a set on the road. Reading every manifest for its name is
            // disk work and the poll repeats it every few seconds, so it does
            // not belong on the main thread.
            val local = withContext(Dispatchers.IO) {
                library.localPlaylistNames().map { name ->
                    PlaylistSummary(name = name, title = name, live = false, trackCount = -1)
                }
            }
            if (!silent) {
                everything = local
                showFiltered()
            }

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
                    if (!silent && everything.isEmpty()) {
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

        // submit() rebinds only when the playlists actually differ. The poll
        // runs every few seconds and almost always finds the same ones, and
        // rebinding regardless would restart the entrance animation and throw
        // away the scroll position on a timer.
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

    private companion object {
        /**
         * How often to ask the station again while this screen is open. Slow
         * enough to be free on a LAN, quick enough that a playlist saved on the
         * desk shows up before anyone reaches for the refresh button.
         */
        const val POLL_MS = 5_000L
    }
}

private class PlaylistAdapter(
    private val onClick: (PlaylistSummary) -> Unit
) : RecyclerView.Adapter<PlaylistAdapter.Holder>() {

    private var items: List<PlaylistSummary> = emptyList()
    private var library: LibraryStore? = null

    /** What the rows currently show; null until the first submit. */
    private var shownSignature: String? = null

    /**
     * Rebinds only when something a row actually shows has changed. The
     * playlists screen re-asks the station on a timer, so this is called far
     * more often than anything changes, and notifying every time would restart
     * the row animation and lose the scroll position every few seconds.
     *
     * The signature includes whether each playlist is on the phone, because
     * that is drawn as a badge and changes on its own when a download finishes
     * — comparing only the station's own fields would leave it stale.
     */
    fun submit(playlists: List<PlaylistSummary>, library: LibraryStore) {
        val signature = playlists.joinToString("\u0000") { item ->
            "${item.name}|${item.title}|${item.live}|${item.trackCount}|" +
                library.manifestExists(item.name)
        }
        if (signature == shownSignature && this.library === library) return

        shownSignature = signature
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
