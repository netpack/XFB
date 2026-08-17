package pt.netpack.xfb.companion

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import kotlin.math.abs
import kotlin.math.min
import kotlin.math.roundToLong

/**
 * The transition between two tracks, drawn the way XFB's playlist wave view
 * draws it: the outgoing track's tail on top, the incoming track's head below,
 * and the region where they sound together shaded across both.
 *
 * Dragging left and right changes how long that region is. The desktop lets you
 * drag the same overlap on the transition strip; a phone has less room, so the
 * whole width is the handle rather than a small grip.
 */
class CrossfadeView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyle: Int = 0
) : View(context, attrs, defStyle) {

    /** Seconds of each track shown either side of the join. */
    var windowMs: Long = 30_000
        set(value) { field = value.coerceAtLeast(2_000); invalidate() }

    var maxOverlapMs: Long = 25_000
        set(value) { field = value.coerceAtLeast(0); invalidate() }

    var overlapMs: Long = 0
        set(value) {
            val clamped = value.coerceIn(0, effectiveMaxOverlap())
            if (field != clamped) {
                field = clamped
                describe()
                invalidate()
                onOverlapChanged?.invoke(clamped)
            }
        }

    var onOverlapChanged: ((Long) -> Unit)? = null

    private var outgoing: Waveform? = null
    private var incoming: Waveform? = null

    private val wavePaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val dimPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val overlapPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val linePaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG)

    private var dragging = false
    private var dragStartX = 0f
    private var dragStartOverlap = 0L

    init {
        wavePaint.color = accent()
        dimPaint.color = withAlpha(accent(), 90)
        overlapPaint.color = withAlpha(accent(), 60)
        linePaint.color = withAlpha(onSurface(), 140)
        linePaint.strokeWidth = dp(1f)
        textPaint.color = onSurface()
        textPaint.textSize = dp(12f)

        isFocusable = true
        describe()
    }

    fun setTracks(outgoing: Waveform?, incoming: Waveform?) {
        this.outgoing = outgoing
        this.incoming = incoming
        describe()
        invalidate()
    }

    /**
     * The label is painted on the canvas, which a screen reader cannot see, so
     * the same words go on the view itself. XFB is used by operators working by
     * ear; a control whose only readout is drawn would be inert for them.
     */
    private fun describe() {
        contentDescription = if (overlapMs <= 0)
            context.getString(R.string.wave_no_overlap)
        else
            context.getString(R.string.wave_overlap, overlapMs / 1000.0)
    }

    /** Neither track can lend more than it has. */
    private fun effectiveMaxOverlap(): Long {
        val outDuration = outgoing?.durationMs ?: Long.MAX_VALUE
        val inDuration = incoming?.durationMs ?: Long.MAX_VALUE
        return min(maxOverlapMs, min(outDuration, inDuration))
    }

    override fun onMeasure(widthSpec: Int, heightSpec: Int) {
        val width = MeasureSpec.getSize(widthSpec)
        val height = dp(160f).toInt()
        setMeasuredDimension(width, height)
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0f) return

        val laneHeight = (h - dp(28f)) / 2f
        val pixelsPerMs = w / windowMs.toFloat()

        // The join sits in the middle; the overlap grows leftwards from it,
        // because the incoming track starts earlier as it widens.
        val joinX = w / 2f
        val overlapWidth = overlapMs * pixelsPerMs
        val overlapLeft = joinX - overlapWidth

        // Shade the region where both are heard.
        if (overlapWidth > 0f) {
            canvas.drawRect(RectF(overlapLeft, 0f, joinX, h - dp(20f)), overlapPaint)
        }

        // Outgoing: its last `windowMs` up to the join, then its tail continues
        // under the overlap until it runs out.
        outgoing?.let { wave ->
            drawWave(
                canvas, wave,
                laneTop = 0f, laneHeight = laneHeight,
                // right edge of the lane is the end of the track
                rightX = joinX + overlapWidth,
                pixelsPerMs = pixelsPerMs,
                fadeFromX = overlapLeft, fadeToX = joinX + overlapWidth
            )
        }

        // Incoming: starts at overlapLeft and runs to the right.
        incoming?.let { wave ->
            drawWaveForward(
                canvas, wave,
                laneTop = laneHeight + dp(8f), laneHeight = laneHeight,
                startX = overlapLeft,
                pixelsPerMs = pixelsPerMs,
                fadeFromX = overlapLeft, fadeToX = joinX
            )
        }

        // The join and the overlap edge.
        canvas.drawLine(joinX, 0f, joinX, h - dp(20f), linePaint)
        if (overlapWidth > 0f)
            canvas.drawLine(overlapLeft, 0f, overlapLeft, h - dp(20f), linePaint)

        val label = if (overlapMs <= 0) context.getString(R.string.wave_no_overlap)
                    else context.getString(R.string.wave_overlap, overlapMs / 1000.0)
        canvas.drawText(label, dp(4f), h - dp(4f), textPaint)
    }

    /** Draws a waveform ending at [rightX], reading backwards from the track end. */
    private fun drawWave(
        canvas: Canvas, wave: Waveform,
        laneTop: Float, laneHeight: Float,
        rightX: Float, pixelsPerMs: Float,
        fadeFromX: Float, fadeToX: Float
    ) {
        val centre = laneTop + laneHeight / 2f
        val bucketPx = wave.bucketMs * pixelsPerMs
        if (bucketPx <= 0f) return

        var index = wave.peaks.size - 1
        var x = rightX
        while (index >= 0 && x > -bucketPx) {
            if (x <= width) {
                val amplitude = wave.peaks[index] * (laneHeight / 2f)
                wavePaint.color = shadeFor(x, fadeFromX, fadeToX, fadingOut = true)
                drawBar(canvas, x - bucketPx, x, centre, amplitude)
            }
            x -= bucketPx
            index--
        }
    }

    /** Draws a waveform starting at [startX], reading forwards from the track start. */
    private fun drawWaveForward(
        canvas: Canvas, wave: Waveform,
        laneTop: Float, laneHeight: Float,
        startX: Float, pixelsPerMs: Float,
        fadeFromX: Float, fadeToX: Float
    ) {
        val centre = laneTop + laneHeight / 2f
        val bucketPx = wave.bucketMs * pixelsPerMs
        if (bucketPx <= 0f) return

        var index = 0
        var x = startX
        while (index < wave.peaks.size && x < width) {
            if (x + bucketPx >= 0f) {
                val amplitude = wave.peaks[index] * (laneHeight / 2f)
                wavePaint.color = shadeFor(x, fadeFromX, fadeToX, fadingOut = false)
                drawBar(canvas, x, x + bucketPx, centre, amplitude)
            }
            x += bucketPx
            index++
        }
    }

    /**
     * One bucket of a waveform, as a rounded bar with a hairline of air either
     * side. Matching [WaveStripView], so the two views read as the same object
     * seen at two zoom levels — butted rectangles read as a block of colour.
     *
     * The gap is a fraction of the bucket rather than a fixed size, because the
     * bucket narrows as the crossfade is zoomed out and a fixed gap would eat
     * the whole bar.
     */
    private fun drawBar(canvas: Canvas, left: Float, right: Float, centre: Float, amplitude: Float) {
        val gap = ((right - left) * 0.18f).coerceAtMost(dp(0.75f))
        val l = left + gap
        val r = (right - gap).coerceAtLeast(l + 0.5f)
        val radius = ((r - l) / 2f).coerceAtMost(dp(1.5f))
        // Silence keeps a stub so a quiet passage still shows the track running.
        val a = amplitude.coerceAtLeast(radius)
        canvas.drawRoundRect(l, centre - a, r, centre + a, radius, radius, wavePaint)
    }

    /**
     * Inside the overlap the two tracks are drawn at the level each is actually
     * contributing, so the picture shows the crossfade rather than just marking
     * where it happens.
     */
    private fun shadeFor(x: Float, fromX: Float, toX: Float, fadingOut: Boolean): Int {
        if (toX <= fromX) return accent()
        val t = ((x - fromX) / (toX - fromX)).coerceIn(0f, 1f)
        val level = if (fadingOut) 1f - t else t
        val outside = x < fromX || x > toX
        val alpha = if (outside) 255 else (70 + level * 185f).toInt().coerceIn(0, 255)
        return withAlpha(accent(), alpha)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                dragging = true
                dragStartX = event.x
                dragStartOverlap = overlapMs
                parent?.requestDisallowInterceptTouchEvent(true)
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                if (!dragging) return false
                val pixelsPerMs = width / windowMs.toFloat()
                if (pixelsPerMs <= 0f) return true
                // Dragging left widens the overlap: the incoming track starts
                // earlier, which is the direction its edge moves.
                val deltaMs = ((dragStartX - event.x) / pixelsPerMs).roundToLong()
                overlapMs = dragStartOverlap + deltaMs
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                dragging = false
                parent?.requestDisallowInterceptTouchEvent(false)
                if (abs(event.x - dragStartX) < dp(4f)) performClick()
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }

    // ------------------------------------------------------------- helpers

    private fun dp(value: Float) = value * resources.displayMetrics.density

    private fun accent(): Int = themeColor(
        com.google.android.material.R.attr.colorPrimary, Color.parseColor("#7C7CBA")
    )

    private fun onSurface(): Int = themeColor(
        com.google.android.material.R.attr.colorOnSurface, Color.LTGRAY
    )

    private fun themeColor(attr: Int, fallback: Int): Int {
        val typed = android.util.TypedValue()
        return if (context.theme.resolveAttribute(attr, typed, true)) typed.data else fallback
    }

    private fun withAlpha(color: Int, alpha: Int) =
        Color.argb(alpha, Color.red(color), Color.green(color), Color.blue(color))
}

/** Largest overlap the editor offers, matching the desktop's default ceiling. */
const val MAX_OVERLAP_MS = 25_000L
