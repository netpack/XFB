package pt.netpack.xfb.companion

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.BitmapShader
import android.graphics.BlurMaskFilter
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.Shader
import android.util.AttributeSet
import android.view.View
import android.view.animation.PathInterpolator
import kotlin.math.min

/**
 * XFB's deck platter, spinning, with the track's artwork as the record label.
 *
 * The platter and the Netpack record are the desktop's own images, so the phone
 * shows the same object the DJ decks do rather than a generic album square.
 * With no artwork the Netpack LP is shown whole, which is both the fallback and
 * the place the logo lives.
 */
class TurntableView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyle: Int = 0
) : View(context, attrs, defStyle) {

    private val platter: Bitmap? =
        BitmapFactory.decodeResource(resources, R.drawable.lp_platter)
    private val netpack: Bitmap? =
        BitmapFactory.decodeResource(resources, R.drawable.lp_netpack)

    private var label: Bitmap? = null
    private var angle = 0f
    private var spinning = false
    private var lastFrameAt = 0L

    private val bitmapPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { isFilterBitmap = true }

    /**
     * A soft contact shadow under the platter. The artwork is a flat cut-out,
     * so without it the deck reads as a sticker on the card rather than an
     * object resting on it. Drawn with a blur mask rather than View elevation
     * because elevation would follow the square view bounds, not the record.
     */
    private val shadowPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.argb(70, 0, 0, 0)
        maskFilter = BlurMaskFilter(
            10f * resources.displayMetrics.density, BlurMaskFilter.Blur.NORMAL
        )
    }
    private val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { isFilterBitmap = true }
    private val matrix = Matrix()

    init {
        // Blur masks are not supported by the hardware pipeline, so this view
        // paints itself in software. It is 132dp square and redraws at 60fps at
        // most, and doing this once here rather than per frame keeps onDraw
        // free of the layer churn that setting it per frame would cause.
        setLayerType(LAYER_TYPE_SOFTWARE, null)
    }

    /**
     * Puts a record on the deck.
     *
     * @param slide true to change records the way a DJ does — the old one
     *        travelling off to the left as the new one comes in from the
     *        right. A track changing under a crossfade is a gradual thing in
     *        the audio, and having the picture of it snap was the one abrupt
     *        moment left on the screen.
     */
    fun setLabel(bitmap: Bitmap?, slide: Boolean = false, slideMs: Long = SLIDE_MS) {
        if (label == bitmap && !slide) return

        if (slide) {
            outgoing = label
            outgoingAngle = angle
            slideStartedAt = System.currentTimeMillis()
            slideDuration = slideMs.coerceIn(SLIDE_MS, MAX_SLIDE_MS)
        }
        label = bitmap
        contentDescription = context.getString(
            if (bitmap == null) R.string.turntable_netpack else R.string.player_artwork
        )
        invalidate()
    }

    /** The record on its way out, and where its label had turned to. */
    private var outgoing: Bitmap? = null
    private var outgoingAngle = 0f
    private var slideStartedAt = 0L
    private var slideDuration = SLIDE_MS

    private val sliding: Boolean
        get() = slideStartedAt != 0L &&
            System.currentTimeMillis() - slideStartedAt < slideDuration

    fun setSpinning(value: Boolean) {
        if (spinning == value) return
        spinning = value
        lastFrameAt = System.nanoTime()
        invalidate()
    }

    override fun onMeasure(widthSpec: Int, heightSpec: Int) {
        // Always square: a record is.
        val width = MeasureSpec.getSize(widthSpec)
        val height = MeasureSpec.getSize(heightSpec)
        val side = when (MeasureSpec.getMode(heightSpec)) {
            MeasureSpec.EXACTLY -> min(width, height)
            else -> width
        }
        setMeasuredDimension(side, side)
    }

    override fun onDraw(canvas: Canvas) {
        val side = min(width, height).toFloat()
        if (side <= 0f) return

        val cx = width / 2f
        val cy = height / 2f

        // Advance by wall clock rather than per frame, so a dropped frame does
        // not slow the record down.
        if (spinning) {
            val previousAngle = angle
            val now = System.nanoTime()
            if (lastFrameAt != 0L) {
                val seconds = (now - lastFrameAt) / 1_000_000_000f
                // 33 1/3 rpm, because that is what a twelve inch does.
                angle = (angle + seconds * 360f / 1.8f) % 360f
            }
            lastFrameAt = now
            if (sliding) outgoingAngle = (outgoingAngle + (angle - previousAngle)) % 360f
        }

        if (sliding) {
            // Linear, and lasting exactly as long as the crossfade it is
            // showing: both records stay partly in view the whole way across,
            // which is the point — the audio has two tracks running, so the
            // deck should look like it does too. Easing would rush that to a
            // near-stop and spend most of the crossfade looking settled.
            val t = ((System.currentTimeMillis() - slideStartedAt) /
                slideDuration.toFloat()).coerceIn(0f, 1f)
            val travel = width * 1.15f

            // The outgoing record is still playing under the fade, so it is
            // still turning; freezing it made it look like it had stopped.
            drawRecord(canvas, outgoing, cx - t * travel, cy, side, outgoingAngle)
            drawRecord(canvas, label, cx + (1f - t) * travel, cy, side, angle)
            postInvalidateOnAnimation()
            return
        }

        // The slide is over; let go of the record that left.
        if (slideStartedAt != 0L) {
            slideStartedAt = 0L
            outgoing = null
        }

        drawRecord(canvas, label, cx, cy, side, angle)
        if (spinning) postInvalidateOnAnimation()
    }

    /** One complete record — shadow, platter, label and spindle — centred on [cx]. */
    private fun drawRecord(
        canvas: Canvas, artwork: Bitmap?, cx: Float, cy: Float, side: Float, degrees: Float
    ) {
        canvas.drawOval(
            cx - side * VINYL_RADIUS_RATIO * 0.94f,
            cy - side * VINYL_RADIUS_RATIO * 0.80f + side * 0.045f,
            cx + side * VINYL_RADIUS_RATIO * 0.94f,
            cy + side * VINYL_RADIUS_RATIO * 0.98f + side * 0.045f,
            shadowPaint
        )

        if (artwork == null) {
            // No cover: the Netpack record, turning.
            netpack?.let { drawDisc(canvas, it, cx, cy, side, degrees) }
            return
        }

        // The platter stays put; the label turning is what reads as motion,
        // and the vinyl itself is symmetric so rotating it would show nothing.
        platter?.let { drawDisc(canvas, it, cx, cy, side, 0f) }

        val vinylRadius = side * VINYL_RADIUS_RATIO
        val labelRadius = vinylRadius * LABEL_RADIUS_RATIO

        canvas.save()
        canvas.rotate(degrees, cx, cy)

        val shader = BitmapShader(artwork, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP)
        val scale = (labelRadius * 2f) / min(artwork.width, artwork.height).toFloat()
        matrix.reset()
        matrix.setScale(scale, scale)
        matrix.postTranslate(
            cx - artwork.width * scale / 2f,
            cy - artwork.height * scale / 2f
        )
        shader.setLocalMatrix(matrix)
        labelPaint.shader = shader
        canvas.drawCircle(cx, cy, labelRadius, labelPaint)
        labelPaint.shader = null

        // The spindle sits on top of the label, as on a real record.
        canvas.restore()
        platter?.let { drawSpindleFrom(canvas, it, cx, cy, side) }
    }

    private fun drawDisc(
        canvas: Canvas, bitmap: Bitmap, cx: Float, cy: Float, side: Float, degrees: Float
    ) {
        canvas.save()
        canvas.rotate(degrees, cx, cy)
        val scale = side / min(bitmap.width, bitmap.height).toFloat()
        matrix.reset()
        matrix.setScale(scale, scale)
        matrix.postTranslate(cx - bitmap.width * scale / 2f, cy - bitmap.height * scale / 2f)
        canvas.drawBitmap(bitmap, matrix, bitmapPaint)
        canvas.restore()
    }

    /** Re-draws just the hub out of the platter image, over the label. */
    private fun drawSpindleFrom(canvas: Canvas, source: Bitmap, cx: Float, cy: Float, side: Float) {
        val hubRadius = side * HUB_RADIUS_RATIO
        canvas.save()
        canvas.clipPath(android.graphics.Path().apply {
            addCircle(cx, cy, hubRadius, android.graphics.Path.Direction.CW)
        })
        val scale = side / min(source.width, source.height).toFloat()
        matrix.reset()
        matrix.setScale(scale, scale)
        matrix.postTranslate(cx - source.width * scale / 2f, cy - source.height * scale / 2f)
        canvas.drawBitmap(source, matrix, bitmapPaint)
        canvas.restore()
    }

    companion object {
        // Measured from the platter image: 241 px square, vinyl radius 95,
        // hub radius 15, both centred.
        const val VINYL_RADIUS_RATIO = 95f / 241f
        const val HUB_RADIUS_RATIO = 15f / 241f
        // A twelve inch label is about a third of the record's radius; a little
        // larger here so the cover is actually visible on a phone.
        const val LABEL_RADIUS_RATIO = 0.42f

        /**
         * How long a record takes to travel across. Long enough to read as a
         * change of record, short enough that it is over well inside even the
         * shortest crossfade anybody sets.
         */
        /** The shortest slide, and the one a clean cut gets. */
        const val SLIDE_MS = 620L

        /**
         * The longest. XFB allows a 25 s overlap and a record drifting for
         * that long stops reading as a change and starts reading as a fault.
         */
        const val MAX_SLIDE_MS = 6_000L

        /** Material's standard easing: leaves briskly, settles gently. */
        val slideInterpolator = PathInterpolator(0.2f, 0f, 0f, 1f)
    }
}
