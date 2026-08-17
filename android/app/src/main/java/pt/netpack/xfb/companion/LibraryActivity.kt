package pt.netpack.xfb.companion

import android.app.AlertDialog
import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.appbar.MaterialToolbar

/**
 * Everything on this phone, searchable, and the place a playlist gets built.
 *
 * The corpus is the audio that has actually been downloaded, gathered from
 * every saved manifest, so anything listed here can be played immediately —
 * there is no such thing as a result you cannot use.
 */
class LibraryActivity : AppCompatActivity() {

    private lateinit var library: LibraryStore
    private lateinit var list: RecyclerView
    private lateinit var searchField: EditText
    private lateinit var emptyLabel: TextView
    private lateinit var saveButton: Button

    private var all: List<SavedTrack> = emptyList()
    private val adapter = TrackPickerAdapter { updateSaveButton() }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_library)

        list = findViewById(R.id.list)
        searchField = findViewById(R.id.searchField)
        emptyLabel = findViewById(R.id.emptyLabel)
        saveButton = findViewById(R.id.saveButton)

        val toolbar = findViewById<MaterialToolbar>(R.id.toolbar)
        setSupportActionBar(toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        setTitle(R.string.library_title)
        toolbar.setNavigationOnClickListener { finish() }

        library = LibraryStore(this)
        list.layoutManager = LinearLayoutManager(this)
        list.adapter = adapter

        searchField.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, a: Int, b: Int, c: Int) = Unit
            override fun onTextChanged(s: CharSequence?, a: Int, b: Int, c: Int) = applyFilter()
            override fun afterTextChanged(s: Editable?) = Unit
        })

        saveButton.setOnClickListener { askForNameAndSave() }

        all = library.downloadedTracks()
        applyFilter()
    }

    override fun onSupportNavigateUp(): Boolean {
        finish()
        return true
    }

    private fun applyFilter() {
        val needle = searchField.text.toString().trim()
        val matches = if (needle.isEmpty()) all else all.filter { track ->
            // Artist and title both, because people search by whichever they
            // remember. Case-insensitive, substring anywhere.
            track.artist.contains(needle, ignoreCase = true) ||
                track.song.contains(needle, ignoreCase = true)
        }

        adapter.submit(matches)
        emptyLabel.visibility = if (matches.isEmpty()) View.VISIBLE else View.GONE
        emptyLabel.setText(
            if (all.isEmpty()) R.string.library_nothing_downloaded else R.string.library_no_matches
        )
        updateSaveButton()
    }

    private fun updateSaveButton() {
        val chosen = adapter.selected().size
        saveButton.isEnabled = chosen > 0
        saveButton.text = if (chosen == 0) getString(R.string.library_save)
                          else getString(R.string.library_save_n, chosen)
    }

    private fun askForNameAndSave() {
        val chosen = adapter.selected()
        if (chosen.isEmpty()) return

        val input = EditText(this).apply {
            setHint(R.string.library_name_hint)
            setSingleLine()
        }

        AlertDialog.Builder(this)
            .setTitle(R.string.library_name_title)
            .setView(input)
            .setPositiveButton(R.string.library_save) { _, _ ->
                val name = input.text.toString().trim()
                if (name.isEmpty()) {
                    Toast.makeText(this, R.string.library_name_needed, Toast.LENGTH_SHORT).show()
                    return@setPositiveButton
                }
                if (library.manifestExists(name)) {
                    Toast.makeText(this, R.string.library_name_taken, Toast.LENGTH_LONG).show()
                    return@setPositiveButton
                }
                library.saveLocalPlaylist(name, chosen)
                Toast.makeText(
                    this, getString(R.string.library_saved, name, chosen.size), Toast.LENGTH_LONG
                ).show()
                finish()
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }
}

private class TrackPickerAdapter(
    private val onSelectionChanged: () -> Unit
) : RecyclerView.Adapter<TrackPickerAdapter.Holder>() {

    private var items: List<SavedTrack> = emptyList()
    // Kept by id, and ordered, because the order tracks are picked in is the
    // order they will play.
    private val chosen = LinkedHashMap<String, SavedTrack>()

    fun submit(tracks: List<SavedTrack>) {
        items = tracks
        notifyDataSetChanged()
    }

    fun selected(): List<SavedTrack> = chosen.values.toList()

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_pick_track, parent, false)
        return Holder(view)
    }

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val track = items[position]
        holder.title.text = track.song
        holder.subtitle.text = track.artist
        holder.subtitle.visibility = if (track.artist.isEmpty()) View.GONE else View.VISIBLE

        holder.check.setOnCheckedChangeListener(null)
        holder.check.isChecked = chosen.containsKey(track.id)

        val toggle = {
            if (chosen.remove(track.id) == null) chosen[track.id] = track
            holder.check.isChecked = chosen.containsKey(track.id)
            onSelectionChanged()
        }
        holder.itemView.setOnClickListener { toggle() }
        holder.check.setOnClickListener { toggle() }
    }

    override fun getItemCount(): Int = items.size

    class Holder(view: View) : RecyclerView.ViewHolder(view) {
        val title: TextView = view.findViewById(R.id.title)
        val subtitle: TextView = view.findViewById(R.id.subtitle)
        val check: CheckBox = view.findViewById(R.id.check)
    }
}
