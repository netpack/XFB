package pt.netpack.xfb.companion

import android.content.Context
import android.net.Uri

/** A desktop XFB this phone has paired with. */
data class Station(
    val host: String,
    val port: Int,
    val token: String
) {
    fun url(path: String): String = "http://$host:$port$path"
}

/**
 * Remembers the one station this phone is paired with.
 *
 * The token is the whole of the phone's access, so it lives in private
 * SharedPreferences and never leaves the device except as a bearer header back
 * to the station that issued it.
 */
class PairingStore(context: Context) {

    private val prefs =
        context.applicationContext.getSharedPreferences("pairing", Context.MODE_PRIVATE)

    fun station(): Station? {
        val host = prefs.getString(KEY_HOST, null) ?: return null
        val token = prefs.getString(KEY_TOKEN, null) ?: return null
        val port = prefs.getInt(KEY_PORT, 0)
        if (host.isEmpty() || token.isEmpty() || port <= 0) return null
        return Station(host, port, token)
    }

    fun save(station: Station) {
        prefs.edit()
            .putString(KEY_HOST, station.host)
            .putInt(KEY_PORT, station.port)
            .putString(KEY_TOKEN, station.token)
            .apply()
    }

    fun forget() {
        prefs.edit().clear().apply()
    }

    private companion object {
        const val KEY_HOST = "host"
        const val KEY_PORT = "port"
        const val KEY_TOKEN = "token"
    }
}

/** The host, port and code carried by an "xfb://pair?..." link. */
data class PairingRequest(
    val host: String,
    val port: Int,
    val code: String
) {
    companion object {
        const val DEFAULT_PORT = 8642

        /** Returns null when the link is not a pairing link we understand. */
        fun fromUri(uri: Uri?): PairingRequest? {
            if (uri == null) return null
            if (!uri.scheme.equals("xfb", ignoreCase = true)) return null
            if (!uri.host.equals("pair", ignoreCase = true)) return null

            val host = uri.getQueryParameter("host")?.trim().orEmpty()
            val code = uri.getQueryParameter("code")?.trim().orEmpty()
            val port = uri.getQueryParameter("port")?.toIntOrNull() ?: DEFAULT_PORT

            if (host.isEmpty() || !isSixDigits(code) || port !in 1..65535) return null
            return PairingRequest(host, port, code)
        }

        /**
         * Accepts what someone would actually type: "192.168.1.20",
         * "192.168.1.20:8642", or a full "http://192.168.1.20:8642/".
         */
        fun fromTyped(address: String, code: String): PairingRequest? {
            var text = address.trim()
            if (text.isEmpty() || !isSixDigits(code)) return null

            text = text.removePrefix("http://").removePrefix("https://")
            text = text.substringBefore('/')

            val host: String
            val port: Int
            val colon = text.lastIndexOf(':')
            if (colon > 0) {
                host = text.substring(0, colon)
                port = text.substring(colon + 1).toIntOrNull() ?: return null
            } else {
                host = text
                port = DEFAULT_PORT
            }

            if (host.isEmpty() || port !in 1..65535) return null
            return PairingRequest(host, port, code)
        }

        private fun isSixDigits(code: String) =
            code.length == 6 && code.all { it.isDigit() }
    }
}
