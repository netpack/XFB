package pt.netpack.xfb.companion

import org.json.JSONObject
import java.io.File

/** A track as it sits on the phone, with the mix settings from the desktop. */
data class SavedTrack(
    val id: String,
    val artist: String,
    val song: String,
    val bytes: Long,
    /** Crossfade into this track, in milliseconds. Zero means a clean cut. */
    val overlapMs: Long,
    val volumeEnvelope: String,
    val file: File
) {
    val label: String
        get() = if (artist.isEmpty()) song else "$artist — $song"

    val playable: Boolean
        get() = file.exists() && (bytes <= 0 || file.length() == bytes)
}

data class SavedPlaylist(
    val name: String,
    val tracks: List<SavedTrack>
)

/**
 * XFB's volume line, ported from PlaylistWaveView.
 *
 * The stored gain is a **linear multiplier clamped to 0..1**, not decibels, and
 * the curve is linear between points, holding the first and last value beyond
 * the ends. Reading it as dB would silence quiet passages and blow out loud
 * ones, so the semantics are copied rather than reinvented.
 */
object VolumeEnvelope {

    fun parse(encoded: String): List<Pair<Long, Float>> {
        if (encoded.isEmpty()) return emptyList()
        return encoded.split(';')
            .mapNotNull { part ->
                val colon = part.indexOf(':')
                if (colon <= 0) return@mapNotNull null
                val ms = part.substring(0, colon).toDoubleOrNull() ?: return@mapNotNull null
                val gain = part.substring(colon + 1).toDoubleOrNull() ?: return@mapNotNull null
                if (ms < 0) return@mapNotNull null
                ms.toLong() to gain.coerceIn(0.0, 1.0).toFloat()
            }
            .sortedBy { it.first }
    }

    fun gainAt(points: List<Pair<Long, Float>>, positionMs: Long): Float {
        if (points.isEmpty()) return 1f
        if (positionMs <= points.first().first) return points.first().second
        if (positionMs >= points.last().first) return points.last().second

        for (i in 1 until points.size) {
            if (positionMs <= points[i].first) {
                val (aMs, aGain) = points[i - 1]
                val (bMs, bGain) = points[i]
                val span = (bMs - aMs).toDouble()
                if (span <= 0) return bGain
                val t = (positionMs - aMs) / span
                return (aGain + t * (bGain - aGain)).toFloat()
            }
        }
        return points.last().second
    }
}

/** Reads back what [LibraryStore.saveManifest] wrote. */
fun LibraryStore.loadManifest(playlistName: String): SavedPlaylist? {
    val file = manifestFile(playlistName)
    if (!file.exists()) return null

    val json = runCatching { JSONObject(file.readText()) }.getOrNull() ?: return null
    val array = json.optJSONArray("tracks") ?: return null

    val tracks = (0 until array.length()).mapNotNull { index ->
        val item = array.optJSONObject(index) ?: return@mapNotNull null
        val path = item.optString("file")
        if (path.isEmpty()) return@mapNotNull null
        SavedTrack(
            id = item.optString("id"),
            artist = item.optString("artist"),
            song = item.optString("song"),
            bytes = item.optLong("bytes"),
            overlapMs = item.optLong("overlapMs"),
            volumeEnvelope = item.optString("volumeEnvelope"),
            file = File(path)
        )
    }

    return SavedPlaylist(json.optString("name", playlistName), tracks)
}
