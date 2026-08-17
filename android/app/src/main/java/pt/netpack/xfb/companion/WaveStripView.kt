package pt.netpack.xfb.companion

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.DashPathEffect
import android.graphics.Paint
import android.graphics.Path
import android.util.AttributeSet
import android.util.TypedValue
import android.view.MotionEvent
import android.view.View

/**
 * The on-air track's waveform with a playhead running through it — XFB's own
 * now-playing strip, which is the thing that makes the desktop look like
 * itself: you can see where you are in the track, not just read a number.
 *
 * The volume line is drawn over it, as on the desktop, so a set that was shaped
 * on the desk looks shaped here too.
 */
class WaveStripView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyle: Int = 0
) : View(context, attrs, defStyle) {

    private var waveform: Waveform? = null
    private var envelope: List<Pair<Long, Float>> = emptyList()

    var durationMs: Long = 0
        set(value) { field = value.coerceAtLeast(0); invalidate() }

    var positionMs: Long = 0
        set(value) {
            val clamped = value.coerceAtLeast(0)
            // Only repaint when the playhead would actually move a pixel.
            if (kotlin.math.abs(clamped - field) >= msPerPixel()) {
                field = clamped
                invalidate()
            } else {
                field = clamped
            }
        }

    /** Called when the operator drags the playhead. */
    var onSeek: ((Long) -> Unit)? = null

    private val playedPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val aheadPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val headPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val knobPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val envelopePaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val envelopePath = Path()

    init {
        val accent = themeColor(com.google.android.material.R.attr.colorPrimary,
                                Color.parseColor("#7C7CBA"))
        val onSurface = themeColor(com.google.android.material.R.attr.colorOnSurface,
                                   Color.LTGRAY)
        playedPaint.color = accent
        // The part not yet played is a ghost of the same shape rather than a
        // second colour: two hues across one waveform reads as two waveforms.
        aheadPaint.color = withAlpha(onSurface, 56)
        headPaint.color = onSurface
        knobPaint.color = onSurface
        envelopePaint.color = withAlpha(accent, 190)
        envelopePaint.style = Paint.Style.STROKE
        envelopePaint.strokeWidth = dp(1.5f)
        envelopePaint.strokeJoin = Paint.Join.ROUND
        envelopePaint.strokeCap = Paint.Cap.ROUND
        // A dashed volume line so it never reads as part of the waveform.
        envelopePaint.pathEffect = DashPathEffect(floatArrayOf(dp(5f), dp(4f)), 0f)
    }

    fun setWaveform(waveform: Waveform?, volumeEnvelope: String) {
        this.waveform = waveform
        this.envelope = VolumeEnvelope.parse(volumeEnvelope)
        if (waveform != null && durationMs <= 0) durationMs = waveform.durationMs
        invalidate()
    }

    private fun msPerPixel(): Long {
        if (width <= 0 || durationMs <= 0) return Long.MAX_VALUE
        return (durationMs / width).coerceAtLeast(1)
    }

    override fun onMeasure(widthSpec: Int, heightSpec: Int) {
        setMeasuredDimension(MeasureSpec.getSize(widthSpec), dp(72f).toInt())
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0f || h <= 0f) return

        val centre = h / 2f
        val wave = waveform
        val headX = if (durationMs > 0) (positionMs.toFloat() / durationMs) * w else 0f

        if (wave != null && wave.peaks.isNotEmpty()) {
            // Discrete rounded bars rather than a column per pixel. A solid
            // mass of one-pixel columns is what makes a waveform look like a
            // readout; separated bars with round caps read as a waveform.
            val step = BAR_WIDTH_DP + BAR_GAP_DP
            val barWidth = dp(BAR_WIDTH_DP)
            val radius = barWidth / 2f
            val maxAmplitude = h / 2f - dp(3f)
            val bars = (w / dp(step)).toInt().coerceAtLeast(1)
            val peaksPerBar = wave.peaks.size.toFloat() / bars

            for (bar in 0 until bars) {
                val from = (bar * peaksPerBar).toInt()
                val to = ((bar + 1) * peaksPerBar).toInt()
                    .coerceAtMost(wave.peaks.size)
                    .coerceAtLeast(from + 1)

                var peak = 0f
                for (i in from until to.coerceAtMost(wave.peaks.size)) {
                    peak = maxOf(peak, wave.peaks[i])
                }

                // Silence still gets a stub, so the track's full length reads
                // as track rather than as empty space.
                val amplitude = maxOf(peak * maxAmplitude, radius)
                val left = bar * dp(step)
                canvas.drawRoundRect(
                    left, centre - amplitude, left + barWidth, centre + amplitude,
                    radius, radius,
                    if (left + barWidth / 2f <= headX) playedPaint else aheadPaint
                )
            }
        }

        // The volume line, as drawn on the desktop's wave view.
        if (envelope.isNotEmpty() && durationMs > 0) {
            envelopePath.reset()
            var x = 0f
            envelopePath.moveTo(0f, h - VolumeEnvelope.gainAt(envelope, 0) * h)
            while (x <= w) {
                val atMs = (x / w * durationMs).toLong()
                envelopePath.lineTo(x, h - VolumeEnvelope.gainAt(envelope, atMs) * h)
                x += dp(3f)
            }
            canvas.drawPath(envelopePath, envelopePaint)
        }

        // The playhead carries a grip, because this strip can be dragged and a
        // bare hairline gives no sign of it. Held a little short of the top and
        // bottom edges so it reads as sitting on the waveform rather than
        // ruling the whole panel.
        val headWidth = dp(2.5f)
        val inset = dp(2f)
        val clampedHead = headX.coerceIn(headWidth / 2f, w - headWidth / 2f)
        canvas.drawRoundRect(
            clampedHead - headWidth / 2f, inset, clampedHead + headWidth / 2f, h - inset,
            headWidth / 2f, headWidth / 2f, headPaint
        )
        canvas.drawCircle(clampedHead, centre, dp(5f), knobPaint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (durationMs <= 0) return false
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                parent?.requestDisallowInterceptTouchEvent(true)
                positionMs = ((event.x / width).coerceIn(0f, 1f) * durationMs).toLong()
                return true
            }
            MotionEvent.ACTION_UP -> {
                parent?.requestDisallowInterceptTouchEvent(false)
                onSeek?.invoke(positionMs)
                performClick()
                return true
            }
            MotionEvent.ACTION_CANCEL -> {
                parent?.requestDisallowInterceptTouchEvent(false)
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    override fun performClick(): Boolean {
        super.performClick()
        return true
    }

    private fun dp(value: Float) = value * resources.displayMetrics.density

    private fun themeColor(attr: Int, fallback: Int): Int {
        val typed = TypedValue()
        return if (context.theme.resolveAttribute(attr, typed, true)) typed.data else fallback
    }

    private fun withAlpha(color: Int, alpha: Int) =
        Color.argb(alpha, Color.red(color), Color.green(color), Color.blue(color))

    private companion object {
        const val BAR_WIDTH_DP = 2.5f
        const val BAR_GAP_DP = 1.5f
    }
}
