package pt.netpack.xfb.companion

import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInstaller
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File
import java.net.HttpURLConnection
import java.net.URL

/** What the paired station is offering, if anything. */
data class CompanionRelease(
    val versionCode: Int,
    val versionName: String,
    val bytes: Long
)

/**
 * Updates come from the desktop this phone is paired with.
 *
 * That is deliberate rather than a compromise: the station is already trusted
 * and reachable, it is where the app came from in the first place, and it means
 * no store account and no server on the internet. The cost is that an update
 * only arrives when the operator is on the same network as their XFB, which is
 * exactly when they would be syncing anyway.
 */
object AppUpdater {

    /** Null when the station has no APK, or none newer than what is installed. */
    suspend fun checkForUpdate(station: Station, installedVersionCode: Int): CompanionRelease? =
        withContext(Dispatchers.IO) {
            val json = runCatching {
                val url = URL(station.url("/api/app"))
                val connection = (url.openConnection() as HttpURLConnection).apply {
                    connectTimeout = 5_000
                    readTimeout = 10_000
                }
                try {
                    if (connection.responseCode != 200) return@runCatching null
                    JSONObject(connection.inputStream.use {
                        it.readBytes().toString(Charsets.UTF_8)
                    })
                } finally {
                    connection.disconnect()
                }
            }.getOrNull() ?: return@withContext null

            if (!json.optBoolean("available", false)) return@withContext null

            // No version on the sidecar means the station cannot say whether its
            // copy is newer, so it is not offered as an update.
            val versionCode = json.optInt("versionCode", 0)
            if (versionCode <= installedVersionCode) return@withContext null

            CompanionRelease(
                versionCode = versionCode,
                versionName = json.optString("versionName", versionCode.toString()),
                bytes = json.optLong("bytes")
            )
        }

    /** Downloads the APK and returns the file, or null if it did not arrive whole. */
    suspend fun download(
        context: Context,
        station: Station,
        release: CompanionRelease,
        onProgress: (Long, Long) -> Unit
    ): File? = withContext(Dispatchers.IO) {
        val target = File(context.cacheDir, "xfb-companion-${release.versionCode}.apk")
        if (target.exists() && release.bytes > 0 && target.length() == release.bytes) {
            return@withContext target
        }
        target.delete()

        val connection = (URL(station.url("/app.apk")).openConnection() as HttpURLConnection).apply {
            connectTimeout = 8_000
            readTimeout = 30_000
        }
        try {
            if (connection.responseCode != 200) return@withContext null
            val total = if (release.bytes > 0) release.bytes
                        else connection.contentLength.toLong()
            var written = 0L
            target.outputStream().use { out ->
                connection.inputStream.use { input ->
                    val buffer = ByteArray(64 * 1024)
                    while (true) {
                        val read = input.read(buffer)
                        if (read <= 0) break
                        out.write(buffer, 0, read)
                        written += read
                        onProgress(written, total)
                    }
                }
            }
            // A truncated APK would fail to install with a far less obvious
            // message than this one.
            if (release.bytes > 0 && target.length() != release.bytes) {
                target.delete()
                null
            } else {
                target
            }
        } catch (e: Exception) {
            target.delete()
            null
        } finally {
            connection.disconnect()
        }
    }

    /**
     * Hands the APK to the system installer. Android shows its own
     * confirmation — this cannot and should not install anything silently.
     *
     * The PackageInstaller session is used rather than an ACTION_VIEW intent on
     * the file, because that opens an "Open with" chooser listing every app
     * that claims the APK mime type — a terminal emulator, say — which is a
     * baffling thing to show someone who asked to update.
     */
    fun install(context: Context, apk: File) {
        val installer = context.packageManager.packageInstaller
        val params = PackageInstaller.SessionParams(
            PackageInstaller.SessionParams.MODE_FULL_INSTALL
        )
        val sessionId = installer.createSession(params)

        installer.openSession(sessionId).use { session ->
            session.openWrite("xfb-companion", 0, apk.length()).use { out ->
                apk.inputStream().use { input -> input.copyTo(out) }
                session.fsync(out)
            }

            val callback = PendingIntent.getBroadcast(
                context,
                sessionId,
                Intent(context, InstallReceiver::class.java)
                    .setPackage(context.packageName),
                // Mutable: the system fills in the session status.
                PendingIntent.FLAG_MUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
            )
            session.commit(callback.intentSender)
        }
    }
}
