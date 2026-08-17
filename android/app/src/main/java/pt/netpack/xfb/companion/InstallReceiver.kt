package pt.netpack.xfb.companion

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInstaller
import android.util.Log
import android.widget.Toast

/**
 * Where the install session reports back.
 *
 * The interesting case is PENDING_USER_ACTION: Android will not replace an app
 * without the person saying so, and it hands back the confirmation screen for
 * us to show. Everything else is just reporting.
 */
class InstallReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        when (intent.getIntExtra(PackageInstaller.EXTRA_STATUS, -1)) {
            PackageInstaller.STATUS_PENDING_USER_ACTION -> {
                @Suppress("DEPRECATION")
                val confirm = intent.getParcelableExtra<Intent>(Intent.EXTRA_INTENT)
                confirm?.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                confirm?.let { context.startActivity(it) }
            }
            PackageInstaller.STATUS_SUCCESS -> Unit   // the new build takes over
            else -> {
                val message = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE)
                Log.w("XfbUpdate", "install failed: $message")
                Toast.makeText(
                    context, context.getString(R.string.update_install_failed),
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }
}
