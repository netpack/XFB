package pt.netpack.xfb.companion

import android.os.Build
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File
import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

/** One playlist as listed by the station. */
data class PlaylistSummary(
    val name: String,
    val title: String,
    val live: Boolean,
    val trackCount: Int
)

/** One track, carrying the mix settings built on the desktop. */
data class Track(
    val id: String,
    val artist: String,
    val song: String,
    val duration: String,
    val bytes: Long,
    val overlapMs: Long,
    val volumeEnvelope: String
) {
    val label: String
        get() = if (artist.isEmpty()) song else "$artist — $song"
}

/** Raised for anything the user should see a sentence about. */
class SyncException(message: String) : IOException(message)

object SyncClient {

    private const val CONNECT_TIMEOUT_MS = 8_000
    private const val READ_TIMEOUT_MS = 20_000

    /**
     * Exchanges a pairing code for a token. The code is single use: the station
     * closes its pairing window as soon as this succeeds.
     */
    suspend fun pair(request: PairingRequest): Station = withContext(Dispatchers.IO) {
        val body = JSONObject()
            .put("code", request.code)
            .put("device", deviceName())
            .toString()

        val url = URL("http://${request.host}:${request.port}/api/pair")
        val connection = (url.openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            doOutput = true
            connectTimeout = CONNECT_TIMEOUT_MS
            readTimeout = READ_TIMEOUT_MS
            setRequestProperty("Content-Type", "application/json")
        }

        try {
            connection.outputStream.use { it.write(body.toByteArray()) }

            val status = connection.responseCode
            val text = connection.readBodyText()
            if (status != 200) {
                throw SyncException(errorMessage(text, status))
            }

            val token = JSONObject(text).optString("token")
            if (token.isEmpty()) throw SyncException("XFB did not return a token.")
            Station(request.host, request.port, token)
        } finally {
            connection.disconnect()
        }
    }

    suspend fun playlists(station: Station): List<PlaylistSummary> =
        withContext(Dispatchers.IO) {
            val json = JSONObject(get(station, "/api/playlists"))
            val array = json.optJSONArray("playlists") ?: return@withContext emptyList()
            (0 until array.length()).map { index ->
                val item = array.getJSONObject(index)
                PlaylistSummary(
                    name = item.optString("name"),
                    title = item.optString("title", item.optString("name")),
                    live = item.optBoolean("live", false),
                    trackCount = item.optInt("tracks", -1)
                )
            }
        }

    suspend fun playlist(station: Station, name: String): List<Track> =
        withContext(Dispatchers.IO) {
            val encoded = URLEncoder.encode(name, "UTF-8")
            val json = JSONObject(get(station, "/api/playlist?name=$encoded"))
            val array = json.optJSONArray("tracks") ?: return@withContext emptyList()
            (0 until array.length()).map { index ->
                val item = array.getJSONObject(index)
                Track(
                    id = item.optString("id"),
                    artist = item.optString("artist"),
                    song = item.optString("song"),
                    duration = item.optString("duration"),
                    bytes = item.optLong("bytes"),
                    overlapMs = item.optLong("overlapMs"),
                    volumeEnvelope = item.optString("volumeEnvelope")
                )
            }
        }

    /**
     * Downloads one track, resuming from whatever is already on disk. Returns
     * the number of bytes actually transferred, which is zero when the file was
     * already complete.
     */
    suspend fun downloadTrack(
        station: Station,
        track: Track,
        destination: File,
        onProgress: (downloaded: Long, total: Long) -> Unit
    ): Long = withContext(Dispatchers.IO) {

        var have = if (destination.exists()) destination.length() else 0L
        if (track.bytes > 0 && have == track.bytes) {
            onProgress(have, track.bytes)
            return@withContext 0L
        }
        // A local file larger than the station's copy means the two have gone
        // out of step; starting over is the only safe reading of that.
        if (track.bytes in 1 until have) {
            destination.delete()
            have = 0L
        }

        val url = URL(station.url("/api/track?id=${URLEncoder.encode(track.id, "UTF-8")}"))
        val connection = (url.openConnection() as HttpURLConnection).apply {
            connectTimeout = CONNECT_TIMEOUT_MS
            readTimeout = READ_TIMEOUT_MS
            setRequestProperty("Authorization", "Bearer ${station.token}")
            if (have > 0) setRequestProperty("Range", "bytes=$have-")
        }

        try {
            when (val status = connection.responseCode) {
                HttpURLConnection.HTTP_PARTIAL -> Unit
                HttpURLConnection.HTTP_OK ->
                    // The station ignored our Range and is sending the whole
                    // file, so anything already written has to go.
                    have = 0L
                416 -> {
                    // Already past the end: the file is complete as it stands.
                    onProgress(destination.length(), track.bytes)
                    return@withContext 0L
                }
                else -> throw SyncException(
                    errorMessage(connection.readBodyText(), status)
                )
            }

            val total = if (track.bytes > 0) track.bytes else have + connection.contentLength
            var written = 0L

            destination.parentFile?.mkdirs()
            java.io.RandomAccessFile(destination, "rw").use { file ->
                file.setLength(have)
                file.seek(have)
                connection.inputStream.use { input ->
                    val buffer = ByteArray(64 * 1024)
                    while (true) {
                        val read = input.read(buffer)
                        if (read <= 0) break
                        file.write(buffer, 0, read)
                        written += read
                        onProgress(have + written, total)
                    }
                }
            }
            written
        } finally {
            connection.disconnect()
        }
    }

    // ------------------------------------------------------------------

    private fun get(station: Station, path: String): String {
        val connection = (URL(station.url(path)).openConnection() as HttpURLConnection).apply {
            connectTimeout = CONNECT_TIMEOUT_MS
            readTimeout = READ_TIMEOUT_MS
            setRequestProperty("Authorization", "Bearer ${station.token}")
        }
        try {
            val status = connection.responseCode
            val text = connection.readBodyText()
            if (status != 200) throw SyncException(errorMessage(text, status))
            return text
        } finally {
            connection.disconnect()
        }
    }

    private fun HttpURLConnection.readBodyText(): String {
        val stream: InputStream = try {
            if (responseCode in 200..299) inputStream else errorStream ?: inputStream
        } catch (e: IOException) {
            return ""
        }
        return stream.use { it.readBytes().toString(Charsets.UTF_8) }
    }

    /** Prefers the station's own wording; falls back to something plain. */
    private fun errorMessage(body: String, status: Int): String {
        val fromServer = runCatching { JSONObject(body).optString("error") }.getOrNull()
        if (!fromServer.isNullOrEmpty()) return fromServer
        return when (status) {
            401 -> "This phone is no longer paired with XFB."
            403 -> "XFB turned the request down."
            404 -> "XFB does not have that any more."
            else -> "XFB answered with an error ($status)."
        }
    }

    private fun deviceName(): String {
        val manufacturer = Build.MANUFACTURER.orEmpty().replaceFirstChar { it.uppercase() }
        val model = Build.MODEL.orEmpty()
        return when {
            model.startsWith(manufacturer, ignoreCase = true) -> model
            manufacturer.isEmpty() -> model
            else -> "$manufacturer $model"
        }.ifEmpty { "Android phone" }
    }
}
