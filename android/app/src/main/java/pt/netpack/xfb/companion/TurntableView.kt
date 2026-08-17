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

    /** Artwork to use as the label, or null for the Netpack record. */
    fun setLabel(bitmap: Bitmap?) {
        if (label == bitmap) return
        label = bitmap
        contentDescription = context.getString(
            if (bitmap == null) R.string.turntable_netpack else R.string.player_artwork
        )
        invalidate()
    }

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
            val now = System.nanoTime()
            if (lastFrameAt != 0L) {
                val seconds = (now - lastFrameAt) / 1_000_000_000f
                // 33 1/3 rpm, because that is what a twelve inch does.
                angle = (angle + seconds * 360f / 1.8f) % 360f
            }
            lastFrameAt = now
        }

        canvas.drawOval(
            cx - side * VINYL_RADIUS_RATIO * 0.94f,
            cy - side * VINYL_RADIUS_RATIO * 0.80f + side * 0.045f,
            cx + side * VINYL_RADIUS_RATIO * 0.94f,
            cy + side * VINYL_RADIUS_RATIO * 0.98f + side * 0.045f,
            shadowPaint
        )

        val artwork = label
        if (artwork == null) {
            // No cover: the Netpack record, turning.
            netpack?.let { drawDisc(canvas, it, cx, cy, side, angle) }
            if (spinning) postInvalidateOnAnimation()
            return
        }

        // The platter stays put; the label turning is what reads as motion,
        // and the vinyl itself is symmetric so rotating it would show nothing.
        platter?.let { drawDisc(canvas, it, cx, cy, side, 0f) }

        val vinylRadius = side * VINYL_RADIUS_RATIO
        val labelRadius = vinylRadius * LABEL_RADIUS_RATIO

        canvas.save()
        canvas.rotate(angle, cx, cy)

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

        if (spinning) postInvalidateOnAnimation()
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

    private companion object {
        // Measured from the platter image: 241 px square, vinyl radius 95,
        // hub radius 15, both centred.
        const val VINYL_RADIUS_RATIO = 95f / 241f
        const val HUB_RADIUS_RATIO = 15f / 241f
        // A twelve inch label is about a third of the record's radius; a little
        // larger here so the cover is actually visible on a phone.
        const val LABEL_RADIUS_RATIO = 0.42f
    }
}
