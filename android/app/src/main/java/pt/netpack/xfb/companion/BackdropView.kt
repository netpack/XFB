package pt.netpack.xfb.companion

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View
import kotlin.math.max

/**
 * The track's cover art, blurred out behind the deck.
 *
 * The same thing Android does behind a media notification: the artwork is what
 * gives the panel the colour of whatever is playing, without ever being
 * something you look at. It is blurred past recognition and then most of the
 * way back to the card's own colour, so the type on top keeps its contrast in
 * either theme and a bright cover cannot take the screen over.
 *
 * The blur is a downscale and a box pass rather than RenderEffect: that arrived
 * in API 31 and this app runs from 24. At this size it is a fraction of a
 * millisecond, and nothing about a blur this heavy needs to be exact.
 */
class BackdropView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyle: Int = 0
) : View(context, attrs, defStyle) {

    private var artwork: Bitmap? = null
    /** The cover on its way out, so a change of record is not a hard swap. */
    private var previous: Bitmap? = null
    private var fadeStartedAt = 0L

    private val bitmapPaint = Paint(Paint.FILTER_BITMAP_FLAG or Paint.ANTI_ALIAS_FLAG)
    private val scrimPaint = Paint(Paint.ANTI_ALIAS_FLAG)

    private val clip = Path()
    private val bounds = RectF()
    private val source = Rect()
    private val destination = Rect()

    private val corner = resources.getDimension(R.dimen.xfb_corner)

    init {
        scrimPaint.color = scrimColor()
    }

    /**
     * Puts a cover behind the deck. Null clears it, which is what a track with
     * no embedded artwork gets — the card's own surface, as before.
     */
    fun setArtwork(bitmap: Bitmap?) {
        val blurred = bitmap?.let { blur(it) }
        if (blurred == null && artwork == null) return

        previous = artwork
        artwork = blurred
        fadeStartedAt = System.currentTimeMillis()
        invalidate()
    }

    /**
     * Asks for nothing. This sits behind the deck in a FrameLayout the card
     * sizes to its content, and a plain View answers an at-most spec with the
     * whole of it — which would make the card as tall as the screen. Wanting
     * zero lets FrameLayout do what match_parent means here: measure the rest,
     * then stretch this to whatever they came to.
     */
    override fun onMeasure(widthSpec: Int, heightSpec: Int) {
        fun size(spec: Int) =
            if (MeasureSpec.getMode(spec) == MeasureSpec.EXACTLY) MeasureSpec.getSize(spec) else 0
        setMeasuredDimension(size(widthSpec), size(heightSpec))
    }

    override fun onDraw(canvas: Canvas) {
        if (width <= 0 || height <= 0) return

        bounds.set(0f, 0f, width.toFloat(), height.toFloat())
        clip.reset()
        clip.addRoundRect(bounds, corner, corner, Path.Direction.CW)
        canvas.save()
        canvas.clipPath(clip)

        val elapsed = System.currentTimeMillis() - fadeStartedAt
        val t = if (fadeStartedAt == 0L) 1f
                else (elapsed.toFloat() / FADE_MS).coerceIn(0f, 1f)

        previous?.let { draw(canvas, it, ((1f - t) * 255).toInt()) }
        artwork?.let { draw(canvas, it, (t * 255).toInt()) }

        // Over the top rather than mixed into the artwork: the covers cross-fade
        // under a scrim that never moves, so the wash changes colour without
        // the panel appearing to flash.
        if (artwork != null || previous != null) canvas.drawRect(bounds, scrimPaint)

        canvas.restore()

        if (t < 1f) {
            postInvalidateOnAnimation()
        } else if (previous != null) {
            previous = null
        }
    }

    /** Centre-crops [bitmap] across the view, the way a cover fills a panel. */
    private fun draw(canvas: Canvas, bitmap: Bitmap, alpha: Int) {
        if (alpha <= 0) return

        val scale = max(
            width.toFloat() / bitmap.width, height.toFloat() / bitmap.height
        )
        val takeWidth = (width / scale).coerceAtMost(bitmap.width.toFloat())
        val takeHeight = (height / scale).coerceAtMost(bitmap.height.toFloat())
        source.set(
            ((bitmap.width - takeWidth) / 2f).toInt(),
            ((bitmap.height - takeHeight) / 2f).toInt(),
            ((bitmap.width + takeWidth) / 2f).toInt(),
            ((bitmap.height + takeHeight) / 2f).toInt()
        )
        destination.set(0, 0, width, height)

        bitmapPaint.alpha = alpha
        canvas.drawBitmap(bitmap, source, destination, bitmapPaint)
    }

    /**
     * Reduces a cover to a handful of pixels and softens what is left.
     *
     * Downscaling alone leaves visible blocks once it is stretched back over a
     * card; one box pass over an image this small costs nothing and is the
     * difference between a blur and a mosaic.
     */
    private fun blur(bitmap: Bitmap): Bitmap? {
        if (bitmap.width <= 0 || bitmap.height <= 0) return null

        val small = runCatching {
            Bitmap.createScaledBitmap(bitmap, BLUR_PX, BLUR_PX, true)
        }.getOrNull() ?: return null

        val pixels = IntArray(BLUR_PX * BLUR_PX)
        small.getPixels(pixels, 0, BLUR_PX, 0, 0, BLUR_PX, BLUR_PX)

        val out = IntArray(pixels.size)
        for (y in 0 until BLUR_PX) {
            for (x in 0 until BLUR_PX) {
                var r = 0; var g = 0; var b = 0; var n = 0
                for (dy in -1..1) {
                    val sy = y + dy
                    if (sy !in 0 until BLUR_PX) continue
                    for (dx in -1..1) {
                        val sx = x + dx
                        if (sx !in 0 until BLUR_PX) continue
                        val p = pixels[sy * BLUR_PX + sx]
                        r += Color.red(p); g += Color.green(p); b += Color.blue(p)
                        n++
                    }
                }
                out[y * BLUR_PX + x] = Color.rgb(r / n, g / n, b / n)
            }
        }

        small.setPixels(out, 0, BLUR_PX, 0, 0, BLUR_PX, BLUR_PX)
        return small
    }

    /**
     * The card's own colour, at the opacity that leaves the cover as a tint
     * rather than a picture. Taken from the theme, so it dims a bright cover in
     * the light theme and lifts a dark one in the dark theme without either
     * needing a rule of its own.
     */
    private fun scrimColor(): Int {
        val typed = android.util.TypedValue()
        val resolved = context.theme.resolveAttribute(
            com.google.android.material.R.attr.colorSurfaceContainer, typed, true
        )
        val base = if (resolved) typed.data
                   else androidx.core.content.ContextCompat.getColor(context, R.color.xfb_base)
        return Color.argb(SCRIM_ALPHA, Color.red(base), Color.green(base), Color.blue(base))
    }

    private companion object {
        /**
         * The blur, in pixels. Small enough that no cover survives it as an
         * image — which is the point: this is a colour, not a picture.
         */
        const val BLUR_PX = 20

        /**
         * How much of the card's own colour sits over it. Anything less and
         * body text on a busy cover stops being comfortably readable; anything
         * more and the cover stops being visible at all.
         */
        const val SCRIM_ALPHA = 168

        /** As long as a record takes to change on the deck next to it. */
        const val FADE_MS = 620f
    }
}
