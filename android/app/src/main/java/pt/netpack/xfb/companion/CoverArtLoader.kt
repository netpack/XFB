package pt.netpack.xfb.companion

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.net.Uri
import androidx.core.content.ContextCompat
import androidx.media3.common.MediaMetadata
import androidx.media3.common.util.BitmapLoader
import com.google.common.util.concurrent.Futures
import com.google.common.util.concurrent.ListenableFuture

/**
 * The cover the notification and the lock screen show, with a sleeve of XFB's
 * own — the radio, on the dark of the desk — for the tracks that have none.
 *
 * Android 12 and up colour the media card from the cover: the system reads the
 * bitmap, takes a background from it and picks the title, artist and transport
 * colours to sit on that background. A track with no cover leaves it nothing to
 * read, and the fallback the phone reaches for is a near-white plate it still
 * writes white on — the whole card comes out blank, with the three transport
 * buttons invisible on it. Handing over a sleeve of our own is what settles
 * those colours, and it has to be a dark one for the card to come back dark.
 *
 * The card does not show the cover as a thumbnail: it stretches the whole
 * bitmap across itself and writes the track over the top. So the radio is set
 * well inside its square, and the ground around it is most of what the phone
 * reads the card's colours from.
 *
 * Most tracks arrive without a cover: the desk embeds one only when the
 * operator had "Embed thumbnail" on, so a set gathered before that was switched
 * on carries none at all. A track that does have one still shows it, because
 * [delegate] is asked first and only its "nothing here" is replaced.
 */
class CoverArtLoader(
    private val context: Context,
    private val delegate: BitmapLoader,
) : BitmapLoader {

    /** Drawn once and kept: every coverless track shows the same sleeve. */
    private var sleeve: Bitmap? = null

    override fun supportsMimeType(mimeType: String): Boolean =
        delegate.supportsMimeType(mimeType)

    override fun decodeBitmap(data: ByteArray): ListenableFuture<Bitmap> =
        delegate.decodeBitmap(data)

    override fun loadBitmap(uri: Uri): ListenableFuture<Bitmap> =
        delegate.loadBitmap(uri)

    override fun loadBitmapFromMetadata(metadata: MediaMetadata): ListenableFuture<Bitmap> =
        delegate.loadBitmapFromMetadata(metadata) ?: Futures.immediateFuture(sleeve())

    private fun sleeve(): Bitmap {
        sleeve?.let { return it }

        val side = 512
        val drawn = Bitmap.createBitmap(side, side, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(drawn)
        // Filled corner to corner, and opaque: the phone samples the whole
        // square, and anything transparent in it is read as white.
        canvas.drawColor(ContextCompat.getColor(context, R.color.xfb_sleeve))

        val radio = BitmapFactory.decodeResource(context.resources, R.drawable.ic_xfb_large)
        val inset = side * 0.18f
        canvas.drawBitmap(
            radio,
            null,
            RectF(inset, inset, side - inset, side - inset),
            Paint(Paint.FILTER_BITMAP_FLAG)
        )
        radio.recycle()

        sleeve = drawn
        return drawn
    }
}
