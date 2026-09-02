package pt.netpack.xfb.companion

import android.app.Activity
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import java.net.HttpURLConnection

/**
 * The station telling this phone that it has a newer app than the one running.
 *
 * XFB cannot call the phone — the phone is always the one that connects — so
 * "the desktop pushes an update" is really the desktop volunteering its version
 * on every answer it gives, in the X-XFB-Companion-Code header, and the phone
 * acting on it wherever it happens to be. That covers pairing and syncing both,
 * which is when the two are in touch at all, and it costs a header rather than
 * a second request.
 *
 * State lives for the run rather than in preferences: an operator who says "not
 * now" means not now, not never, and the next launch is a fair time to ask
 * again.
 */
object CompanionUpdate {

    /** The newest versionCode the station has advertised this run. */
    @Volatile
    private var advertised = 0

    /** The version the operator has already waved away this run. */
    @Volatile
    private var declined = 0

    /** One prompt at a time, however many screens finish syncing at once. */
    @Volatile
    private var showing = false

    /** Reads the version header off any answer the station has just given. */
    fun noteStationVersion(connection: HttpURLConnection) {
        val code = connection.getHeaderField(HEADER_CODE)?.trim()?.toIntOrNull() ?: return
        if (code > advertised) advertised = code
    }

    private fun behind(): Boolean =
        advertised > BuildConfig.VERSION_CODE && advertised > declined && !showing

    /**
     * Offers the update if the station has advertised one, and does nothing at
     * all otherwise — this is called after ordinary syncing, so silence is the
     * common case and the only acceptable one.
     */
    fun offer(activity: Activity, station: Station) {
        if (!behind()) return
        if (activity.isFinishing || activity.isDestroyed) return
        val owner = activity as? LifecycleOwner ?: return

        showing = true
        owner.lifecycleScope.launch {
            val release = runCatching {
                AppUpdater.checkForUpdate(station, BuildConfig.VERSION_CODE)
            }.getOrNull()

            // The header said there was one and the station now says otherwise:
            // nothing to offer, and nothing worth telling anyone about.
            if (release == null) {
                showing = false
                return@launch
            }
            if (activity.isFinishing || activity.isDestroyed) {
                showing = false
                return@launch
            }

            MaterialAlertDialogBuilder(activity)
                .setTitle(activity.getString(R.string.update_offer_title, release.versionName))
                .setMessage(R.string.update_offer_message)
                .setCancelable(false)
                .setPositiveButton(R.string.update_offer_install) { _, _ ->
                    download(activity, station, release)
                }
                .setNegativeButton(R.string.update_offer_later) { _, _ ->
                    declined = release.versionCode
                    showing = false
                }
                .show()
        }
    }

    private fun download(activity: Activity, station: Station, release: CompanionRelease) {
        val owner = activity as? LifecycleOwner ?: return

        val progress = MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.update_offer_downloading)
            .setMessage(activity.getString(R.string.update_downloading, 0))
            .setCancelable(false)
            .show()

        owner.lifecycleScope.launch {
            var lastPercent = -1
            val apk = runCatching {
                AppUpdater.download(activity, station, release) { done, total ->
                    val percent = if (total > 0) ((done * 100) / total).toInt() else 0
                    if (percent != lastPercent) {
                        lastPercent = percent
                        activity.runOnUiThread {
                            progress.setMessage(
                                activity.getString(R.string.update_downloading, percent)
                            )
                        }
                    }
                }
            }.getOrNull()

            progress.dismiss()
            showing = false

            if (apk == null) {
                // Not an error the operator has to act on: the set they were
                // syncing is unaffected, and the offer comes round again.
                declined = release.versionCode
                MaterialAlertDialogBuilder(activity)
                    .setMessage(R.string.update_failed)
                    .setPositiveButton(android.R.string.ok, null)
                    .show()
                return@launch
            }
            AppUpdater.install(activity, apk)
        }
    }

    private const val HEADER_CODE = "X-XFB-Companion-Code"
}
