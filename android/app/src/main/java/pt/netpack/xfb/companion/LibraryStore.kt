package pt.netpack.xfb.companion

import android.content.Context
import android.os.Environment
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * Where downloaded music lives on the phone.
 *
 * Everything goes in the app's own external files directory: no storage
 * permission is needed for it, and it is removed when the app is uninstalled,
 * which is the right behaviour for a copy of someone else's library.
 *
 * The playlist manifest is saved next to the audio because it carries the
 * crossfade overlaps and volume lines from the desktop. Phase 03 reads it back
 * to reproduce the transitions; nothing here uses those fields yet.
 */
class LibraryStore(context: Context) {

    private val root: File =
        context.getExternalFilesDir(Environment.DIRECTORY_MUSIC) ?: context.filesDir

    private val trackDir = File(root, "tracks")
    private val playlistDir = File(root, "playlists")

    /**
     * Tracks are named by their station id rather than by artist and title:
     * the id is already opaque and filesystem-safe, so no sanitising is needed
     * and two tracks with the same title cannot collide.
     */
    fun trackFile(track: Track): File = File(trackDir, track.id)

    fun isComplete(track: Track): Boolean {
        val file = trackFile(track)
        return file.exists() && track.bytes > 0 && file.length() == track.bytes
    }

    fun downloadedBytes(track: Track): Long {
        val file = trackFile(track)
        return if (file.exists()) file.length() else 0L
    }

    /**
     * Writes what the station says a playlist is.
     *
     * Crossfades dragged on this phone are laid back over the station's own:
     * the desk has no idea a join was edited here, so taking its number would
     * quietly undo the edit the next time the set was downloaded.
     */
    fun saveManifest(playlistName: String, tracks: List<Track>) {
        val edited = editedOverlaps(playlistName)

        writeManifest(
            playlistName,
            tracks.map { track ->
                val edit = edited[track.id]
                SavedTrack(
                    id = track.id,
                    artist = track.artist,
                    song = track.song,
                    duration = track.duration,
                    bytes = track.bytes,
                    overlapMs = edit?.overlapMs ?: track.overlapMs,
                    // An overlap that arrived from the desk is somebody's
                    // decision; a zero there is simply a join nobody set.
                    overlapPinned = edit?.overlapPinned ?: (track.overlapMs > 0),
                    overlapEditedHere = edit != null,
                    volumeEnvelope = track.volumeEnvelope,
                    file = trackFile(track)
                )
            },
            // Not carried over, unlike an overlap edit: this method is only
            // ever called with what the station just sent, so the download
            // itself is proof the playlist is not one made here. Reading the
            // old flag instead would leave any manifest the earlier bug
            // mislabelled stuck as local for ever — re-syncing it from the
            // desk, the obvious repair, would keep saying "made here".
            madeHere = false
        )
    }

    /**
     * The station's tracks with the crossfades edited here laid over them, for
     * showing a playlist that is being read live from the desk.
     */
    fun withLocalOverlaps(playlistName: String, tracks: List<Track>): List<Track> {
        val edited = editedOverlaps(playlistName)
        if (edited.isEmpty()) return tracks
        return tracks.map { track ->
            edited[track.id]?.let { track.copy(overlapMs = it.overlapMs) } ?: track
        }
    }

    /** The joins of a saved playlist that were dragged on this phone, by id. */
    private fun editedOverlaps(playlistName: String): Map<String, SavedTrack> =
        loadManifest(playlistName)?.tracks.orEmpty()
            .filter { it.overlapEditedHere }
            .associateBy { it.id }

    private fun writeManifest(name: String, tracks: List<SavedTrack>, madeHere: Boolean) {
        playlistDir.mkdirs()

        // A manifest is the only record that a file in tracks/ is music, and a
        // playlist name is not unique across stations: two desks both offer
        // "Marked for this phone", so the second one's download overwrites the
        // first one's manifest and would take the first station's tracks off
        // this phone's library with it, audio and all. Everything a write could
        // drop goes into the ledger first.
        if (name != DOWNLOADS) {
            remember(loadManifest(name)?.tracks.orEmpty() + tracks)
        }

        val array = JSONArray()
        tracks.forEach { track ->
            array.put(
                JSONObject()
                    .put("id", track.id)
                    .put("artist", track.artist)
                    .put("song", track.song)
                    .put("duration", track.duration)
                    .put("bytes", track.bytes)
                    .put("overlapMs", track.overlapMs)
                    .put("overlapPinned", track.overlapPinned)
                    .put("overlapEditedHere", track.overlapEditedHere)
                    .put("volumeEnvelope", track.volumeEnvelope)
                    .put("file", track.file.absolutePath)
            )
        }

        val manifest = JSONObject()
            .put("name", name)
            .put("savedAt", System.currentTimeMillis())
            .put("tracks", array)
        // Written only when true, so a synced manifest keeps the shape it has
        // always had: no flag at all.
        if (madeHere) manifest.put("madeHere", true)

        File(playlistDir, manifestName(name)).writeText(manifest.toString())
    }

    /**
     * Adds tracks to this phone's ledger of everything it has downloaded.
     *
     * Keyed by id and never truncated by a playlist write, so a track outlives
     * the playlist it arrived in — including that playlist being replaced by
     * another station's playlist of the same name. Entries whose audio has gone
     * are dropped on the way past: the ledger says what is on this phone now,
     * not what once was.
     */
    private fun remember(tracks: List<SavedTrack>) {
        val before = loadManifest(DOWNLOADS)?.tracks.orEmpty()

        val known = LinkedHashMap<String, SavedTrack>()
        for (track in before + tracks) {
            if (track.id.isEmpty() || !track.file.exists()) continue
            // The later entry wins: it carries whatever a station said about
            // the track most recently.
            known[track.id] = track
        }

        // Rewriting this after every track of a 300-track download would be a
        // lot of flash for no change, so it only moves when it moves.
        if (known.size == before.size && before.all { known.containsKey(it.id) }) return

        writeManifest(DOWNLOADS, known.values.toList(), madeHere = false)
    }

    fun manifestExists(playlistName: String): Boolean = manifestFile(playlistName).exists()

    fun manifestFile(playlistName: String): File =
        File(playlistDir, manifestName(playlistName))

    /** Every playlist saved on this phone, readable with no station in reach. */
    fun localPlaylistNames(): List<String> {
        val files = playlistDir.listFiles { file -> file.extension == "json" } ?: return emptyList()
        return files.mapNotNull { file ->
            runCatching { JSONObject(file.readText()).optString("name") }
                .getOrNull()
                ?.takeIf { it.isNotEmpty() && it != ALL_TRACKS && it != DOWNLOADS }
        }.sorted()
    }

    /**
     * Writes a playlist the operator built here rather than one pulled from a
     * station. Same shape as a synced manifest, so playback cannot tell them
     * apart — only the file it came from differs.
     *
     * The name is a new one, so this is where the origin gets decided; a
     * manifest already on disk keeps whatever it said, since nothing here
     * knows better than the write that created it.
     */
    fun saveLocalPlaylist(name: String, tracks: List<SavedTrack>) =
        writeManifest(name, tracks, madeHere = !manifestExists(name) || isLocalPlaylist(name))

    /**
     * Rewrites a playlist whose crossfades were edited on the phone.
     *
     * Dragging a join says nothing about where the playlist came from, so the
     * origin flag is carried over rather than stamped: a synced playlist that
     * is edited here stays a synced playlist, and goes on being looked up on
     * the station like one.
     */
    fun saveOverlapEdit(name: String, tracks: List<SavedTrack>) =
        writeManifest(name, tracks, madeHere = isLocalPlaylist(name))

    fun deleteLocalPlaylist(name: String): Boolean = manifestFile(name).delete()

    /** True for playlists built on this phone, which no station knows about. */
    fun isLocalPlaylist(name: String): Boolean {
        val file = manifestFile(name)
        if (!file.exists()) return false
        return runCatching { JSONObject(file.readText()).optBoolean("madeHere", false) }
            .getOrDefault(false)
    }

    /**
     * Every distinct track sitting on this phone, gathered from all the saved
     * manifests. This is what search runs against: the audio is the same files
     * playback uses, so anything listed here can be played right now.
     */
    fun downloadedTracks(): List<SavedTrack> {
        val files = playlistDir.listFiles { file -> file.extension == "json" } ?: return emptyList()

        val byId = LinkedHashMap<String, SavedTrack>()
        for (file in files) {
            val json = runCatching { JSONObject(file.readText()) }.getOrNull() ?: continue
            val array = json.optJSONArray("tracks") ?: continue
            for (index in 0 until array.length()) {
                val item = array.optJSONObject(index) ?: continue
                val path = item.optString("file")
                if (path.isEmpty()) continue
                val audio = File(path)
                if (!audio.exists()) continue
                // The station's byte count is what a complete copy weighs. A
                // manifest is written while a download is still running, so a
                // shorter file here is a track still arriving, and it must not
                // show up as music until the rest of it does.
                val bytes = item.optLong("bytes")
                if (bytes > 0 && audio.length() != bytes) continue

                val id = item.optString("id")
                if (id.isEmpty() || byId.containsKey(id)) continue

                byId[id] = SavedTrack(
                    id = id,
                    artist = item.optString("artist"),
                    song = item.optString("song"),
                    duration = item.optString("duration"),
                    bytes = bytes,
                    // Overlaps belong to a playlist, not to the track itself.
                    overlapMs = 0,
                    overlapPinned = false,
                    overlapEditedHere = false,
                    volumeEnvelope = item.optString("volumeEnvelope"),
                    file = audio
                )
            }
        }
        return byId.values.sortedWith(compareBy({ it.artist.lowercase() }, { it.song.lowercase() }))
    }

    /**
     * Whether there is anything on this phone worth opening the player for.
     *
     * Stops at the first playable file rather than building the whole list:
     * this runs on the way into the app, where the answer is wanted before the
     * first frame and a library of a few hundred tracks would be felt.
     */
    fun hasDownloadedTracks(): Boolean {
        val files = playlistDir.listFiles { file -> file.extension == "json" } ?: return false
        for (file in files) {
            val json = runCatching { JSONObject(file.readText()) }.getOrNull() ?: continue
            val array = json.optJSONArray("tracks") ?: continue
            for (index in 0 until array.length()) {
                val item = array.optJSONObject(index) ?: continue
                val path = item.optString("file")
                if (path.isEmpty()) continue
                val audio = File(path)
                val bytes = item.optLong("bytes")
                // Complete files only, on the same reading as downloadedTracks:
                // a phone holding nothing but a half-finished download has
                // nothing to open the player for.
                if (audio.exists() && (bytes <= 0 || audio.length() == bytes)) return true
            }
        }
        return false
    }

    /**
     * "Everything on this phone", with any crossfades that have been set on it.
     *
     * [downloadedTracks] deliberately drops overlaps, because one belongs to a
     * playlist rather than to a track. The gathered set keeps its own in a
     * manifest under the reserved name, so they are laid back over the top
     * here — by id, so tracks downloaded since simply arrive with none.
     *
     * Both the player screen and the playback service go through this: reading
     * the gathered set two different ways is what made a crossfade set on it
     * save correctly and then play as a hard cut.
     */
    fun gatheredSet(): List<SavedTrack> {
        val everything = downloadedTracks()
        if (everything.isEmpty()) return everything

        val saved = loadManifest(ALL_TRACKS)?.tracks.orEmpty().associateBy { it.id }
        return everything.map { track ->
            val previous = saved[track.id] ?: return@map track
            track.copy(
                overlapMs = previous.overlapMs,
                overlapPinned = previous.overlapPinned,
                overlapEditedHere = previous.overlapEditedHere,
                volumeEnvelope = previous.volumeEnvelope
            )
        }
    }

    companion object {
        /**
         * The reserved name for "everything on this phone". It is stored like
         * a playlist so the crossfades set on it survive a restart, but it is
         * not one the operator made, so it stays out of the playlists list.
         */
        const val ALL_TRACKS = "__all__"

        /**
         * The reserved name for the ledger of every track downloaded onto this
         * phone. Held in the same shape as a playlist so [downloadedTracks]
         * reads it with everything else, and kept out of the playlists list for
         * the same reason as [ALL_TRACKS].
         */
        const val DOWNLOADS = "__downloads__"
    }

    /** A playlist name is arbitrary text; a file name is not. */
    private fun manifestName(playlistName: String): String {
        val safe = playlistName.map { character ->
            if (character.isLetterOrDigit() || character == '-' || character == '_') character
            else '_'
        }.joinToString("")
        return "$safe.json"
    }
}
