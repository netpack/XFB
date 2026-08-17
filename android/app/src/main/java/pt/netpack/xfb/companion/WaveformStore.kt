package pt.netpack.xfb.companion

import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.util.Log
import java.io.File
import java.nio.ByteOrder
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.Executors
import kotlin.math.abs
import kotlin.math.max

/**
 * Peak data for drawing a track's waveform.
 *
 * The desktop gets these from ffmpeg, which does not exist on a phone, so the
 * file is decoded with MediaCodec instead and reduced to one peak per 20 ms —
 * the same bucket XFB's own WaveformStore uses, so the two look alike at the
 * same zoom.
 *
 * Results are cached next to the audio, because decoding a track takes long
 * enough to be worth never doing twice.
 */
class Waveform(
    val peaks: FloatArray,      // 0..1, one per bucket
    val bucketMs: Int,
    val durationMs: Long
)

object WaveformStore {

    const val BUCKET_MS = 20

    private const val TAG = "XfbWaveform"
    private const val MAGIC = "XFWV2"

    private val memory = ConcurrentHashMap<String, Waveform>()
    private val workers = Executors.newSingleThreadExecutor()
    private val inFlight = ConcurrentHashMap<String, Boolean>()

    /** Already-known peaks, or null. Never blocks. */
    fun peek(file: File): Waveform? = memory[file.absolutePath]

    /**
     * Requests peaks for [file]. [onReady] runs on a background thread, once,
     * when they are available — or not at all if the file cannot be decoded.
     */
    fun fetch(file: File, cacheDir: File, onReady: (Waveform) -> Unit) {
        val key = file.absolutePath
        memory[key]?.let { onReady(it); return }

        if (inFlight.putIfAbsent(key, true) != null) return

        workers.execute {
            try {
                val cache = File(cacheDir, "${file.name}.wf")
                val loaded = readCache(cache) ?: analyse(file)?.also { writeCache(cache, it) }
                if (loaded != null) {
                    memory[key] = loaded
                    onReady(loaded)
                }
            } catch (e: Exception) {
                Log.w(TAG, "waveform failed for ${file.name}", e)
            } finally {
                inFlight.remove(key)
            }
        }
    }

    // ------------------------------------------------------------- decoding

    private fun analyse(file: File): Waveform? {
        val extractor = MediaExtractor()
        extractor.setDataSource(file.absolutePath)

        var trackIndex = -1
        var format: MediaFormat? = null
        for (i in 0 until extractor.trackCount) {
            val candidate = extractor.getTrackFormat(i)
            if (candidate.getString(MediaFormat.KEY_MIME)?.startsWith("audio/") == true) {
                trackIndex = i
                format = candidate
                break
            }
        }
        if (trackIndex < 0 || format == null) {
            extractor.release()
            return null
        }

        extractor.selectTrack(trackIndex)
        val mime = format.getString(MediaFormat.KEY_MIME)!!
        val sampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE)
        val channels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT)
        val durationMs =
            if (format.containsKey(MediaFormat.KEY_DURATION))
                format.getLong(MediaFormat.KEY_DURATION) / 1000
            else 0L

        val codec = MediaCodec.createDecoderByType(mime)
        codec.configure(format, null, null, 0)
        codec.start()

        val samplesPerBucket = max(1, sampleRate * BUCKET_MS / 1000)
        val peaks = ArrayList<Float>(1024)

        var bucketPeak = 0f
        var samplesInBucket = 0
        var sawInputEnd = false
        var sawOutputEnd = false

        val info = MediaCodec.BufferInfo()

        try {
            while (!sawOutputEnd) {
                if (!sawInputEnd) {
                    val inIndex = codec.dequeueInputBuffer(10_000)
                    if (inIndex >= 0) {
                        val buffer = codec.getInputBuffer(inIndex)!!
                        val size = extractor.readSampleData(buffer, 0)
                        if (size < 0) {
                            codec.queueInputBuffer(
                                inIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM
                            )
                            sawInputEnd = true
                        } else {
                            codec.queueInputBuffer(inIndex, 0, size, extractor.sampleTime, 0)
                            extractor.advance()
                        }
                    }
                }

                val outIndex = codec.dequeueOutputBuffer(info, 10_000)
                if (outIndex >= 0) {
                    if (info.size > 0) {
                        val output = codec.getOutputBuffer(outIndex)!!
                        output.position(info.offset)
                        output.limit(info.offset + info.size)
                        val shorts = output.order(ByteOrder.nativeOrder()).asShortBuffer()

                        // One peak per bucket, taking the loudest channel so a
                        // hard-panned moment still shows up.
                        var i = 0
                        while (i < shorts.remaining()) {
                            var frame = 0f
                            for (c in 0 until channels) {
                                if (i + c >= shorts.remaining()) break
                                frame = max(frame, abs(shorts.get(i + c).toFloat()) / 32768f)
                            }
                            bucketPeak = max(bucketPeak, frame)
                            samplesInBucket++
                            if (samplesInBucket >= samplesPerBucket) {
                                peaks.add(bucketPeak)
                                bucketPeak = 0f
                                samplesInBucket = 0
                            }
                            i += channels
                        }
                    }
                    codec.releaseOutputBuffer(outIndex, false)
                    if (info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0)
                        sawOutputEnd = true
                }
            }
        } finally {
            runCatching { codec.stop() }
            codec.release()
            extractor.release()
        }

        if (samplesInBucket > 0) peaks.add(bucketPeak)
        if (peaks.isEmpty()) return null

        return Waveform(
            peaks = peaks.toFloatArray(),
            bucketMs = BUCKET_MS,
            durationMs = if (durationMs > 0) durationMs
                         else peaks.size.toLong() * BUCKET_MS
        )
    }

    // ---------------------------------------------------------------- cache

    private fun readCache(cache: File): Waveform? {
        if (!cache.exists()) return null
        return runCatching {
            cache.inputStream().buffered().use { stream ->
                val magic = ByteArray(MAGIC.length)
                if (stream.read(magic) != magic.size) return null
                if (String(magic) != MAGIC) return null

                val duration = readLong(stream)
                val count = readInt(stream)
                if (count <= 0 || count > 5_000_000) return null

                val bytes = ByteArray(count)
                var read = 0
                while (read < count) {
                    val n = stream.read(bytes, read, count - read)
                    if (n <= 0) return null
                    read += n
                }
                Waveform(
                    FloatArray(count) { (bytes[it].toInt() and 0xFF) / 255f },
                    BUCKET_MS,
                    duration
                )
            }
        }.getOrNull()
    }

    private fun writeCache(cache: File, waveform: Waveform) {
        runCatching {
            cache.parentFile?.mkdirs()
            cache.outputStream().buffered().use { stream ->
                stream.write(MAGIC.toByteArray())
                writeLong(stream, waveform.durationMs)
                writeInt(stream, waveform.peaks.size)
                // A byte per bucket is plenty for something drawn a few hundred
                // pixels wide, and keeps the cache small.
                stream.write(ByteArray(waveform.peaks.size) {
                    (waveform.peaks[it].coerceIn(0f, 1f) * 255f).toInt().toByte()
                })
            }
        }
    }

    private fun writeInt(stream: java.io.OutputStream, value: Int) {
        for (shift in 24 downTo 0 step 8) stream.write((value shr shift) and 0xFF)
    }

    private fun writeLong(stream: java.io.OutputStream, value: Long) {
        for (shift in 56 downTo 0 step 8) stream.write(((value shr shift) and 0xFF).toInt())
    }

    private fun readInt(stream: java.io.InputStream): Int {
        var value = 0
        repeat(4) {
            val b = stream.read()
            if (b < 0) return -1
            value = (value shl 8) or b
        }
        return value
    }

    private fun readLong(stream: java.io.InputStream): Long {
        var value = 0L
        repeat(8) {
            val b = stream.read()
            if (b < 0) return -1
            value = (value shl 8) or b.toLong()
        }
        return value
    }
}
