#ifndef WAVEFORMSTORE_H
#define WAVEFORMSTORE_H

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVector>

/**
 * Amplitude envelope ("sound wave") of one audio file: one 0..255 peak
 * value per MsPerPeak milliseconds of audio, plus the tempo measured from
 * the same decode.
 */
struct WaveformData
{
    qint64 durationMs = 0;
    QVector<quint8> peaks;
    double bpm = 0.0; // 0 = no tempo could be measured
    bool failed = false;

    bool ready() const { return !peaks.isEmpty(); }
};

/**
 * Asynchronous provider of waveforms for local audio files.
 *
 * Files are decoded with the same ffmpeg executable the FX engine uses
 * (FxEngine::ffmpegExecutable) to low-rate mono PCM and reduced to one
 * peak per 20 ms. The same pass measures the track's tempo, off a finer
 * envelope that is discarded once the BPM is known. Results are cached in
 * memory and on disk (CacheLocation/waveforms, keyed by path + size +
 * mtime), so a playlist only pays the decode cost once per file.
 */
class WaveformStore : public QObject
{
    Q_OBJECT

public:
    static constexpr int MsPerPeak = 20;

    explicit WaveformStore(QObject *parent = nullptr);

    /** Returns the waveform if already known (possibly a failed marker). */
    const WaveformData *peek(const QString &filePath) const;

    /** Like peek(), but schedules a background extraction when unknown. */
    const WaveformData *fetch(const QString &filePath);

    /** Drops the in-memory copy of a waveform. The disk cache keeps it, so
     *  a later fetch() reloads it without decoding again. Lets a sweep over
     *  the whole library (BpmLibrary) run without holding every waveform it
     *  ever touched in memory. Ignored while an extraction is in flight. */
    void forget(const QString &filePath);

signals:
    /** Emitted when an extraction finishes, successfully or not. */
    void waveformReady(const QString &filePath);

private:
    void startNext();
    QString cacheFileFor(const QString &filePath) const;
    bool loadFromDisk(const QString &filePath, WaveformData &out) const;
    void saveToDisk(const QString &filePath, const WaveformData &data) const;

    QHash<QString, WaveformData> m_cache;
    QStringList m_queue;
    QSet<QString> m_pending; // queued or currently decoding
    int m_running = 0;
};

#endif // WAVEFORMSTORE_H
