#ifndef LOUDNESSSCANNER_H
#define LOUDNESSSCANNER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

/**
 * @brief One file's EBU R128 measurement.
 *
 * `integratedLufs` is programme loudness (ITU-R BS.1770 gated), `rangeLu`
 * is the loudness range and `truePeakDbtp` is the inter-sample peak that
 * ffmpeg's `ebur128=peak=true` reports. A file that is entirely silent
 * measures at -inf LUFS; that is reported as `valid == false` with a
 * reason, so a sweep never stores a nonsense number for it.
 */
struct LoudnessMeasurement
{
    QString filePath;
    double integratedLufs = 0.0;
    double rangeLu = 0.0;
    double truePeakDbtp = 0.0;
    bool valid = false;
    QString error;
};

/**
 * @brief Measures the loudness of audio files to EBU R128, off the GUI.
 *
 * Each file is measured by one `ffmpeg -nostats -i <file> -af
 * ebur128=peak=true -f null -` run whose stderr summary is parsed. The
 * measurement itself therefore happens in a child process — the GUI
 * thread only ever runs the (tiny) parse in a signal handler, never a
 * decode. This is the same shape as WaveformStore's decoder pool, and for
 * the same reason: it keeps the window responsive during a library sweep
 * without the thread-affinity problems a QThread-per-file pool brings.
 *
 * Runs are **queued**: at most maxConcurrent() of them exist at any
 * moment (2 by default), so scanning a ten-thousand-track library does
 * not try to fork ten thousand processes. A sweep is cancellable — cancel()
 * kills whatever is running and drops the rest of the queue.
 *
 * Nothing here touches the database; the caller decides where a result is
 * stored. That keeps the scanner usable for a single ad-hoc file as well
 * as for a library sweep.
 */
class LoudnessScanner : public QObject
{
    Q_OBJECT

public:
    explicit LoudnessScanner(QObject *parent = nullptr);
    ~LoudnessScanner() override;

    /** True when ffmpeg can be found, i.e. measuring is possible at all. */
    static bool available();

    /** Concurrent ffmpeg runs. Clamped to 1..8; default 2. */
    void setMaxConcurrent(int jobs);
    int maxConcurrent() const { return m_maxConcurrent; }

    /**
     * Start measuring these files. Duplicates and empty paths are dropped.
     * A sweep already in progress is left alone (call cancel() first).
     */
    void measure(const QStringList &filePaths);
    /** Kill the running measurements and drop the queue. */
    void cancel();
    bool busy() const { return m_active; }
    int remaining() const { return m_queue.size() + m_running.size(); }

    /**
     * Parse the `Summary:` block ffmpeg's ebur128 filter writes to stderr.
     * Public and static so it can be exercised without spawning anything.
     */
    static LoudnessMeasurement parseSummary(const QString &stderrText);

    /**
     * The playback gain, in dB, that brings a track measured at
     * `integratedLufs` to `targetLufs` — reduced when that would push the
     * track's true peak above `ceilingDbTp`.
     *
     * NOTE: the result is decibels. XFB's playlist volume envelope is a
     * separate LINEAR 0..1 multiplier; the two are combined by multiplying
     * the linear equivalents, never by adding.
     */
    static double normalizationGainDb(double integratedLufs, double truePeakDbtp,
                                      double targetLufs, double ceilingDbTp);

    /** Default target/ceiling and the range the Options dialog offers. */
    static constexpr double kDefaultTargetLufs = -16.0;
    static constexpr double kDefaultCeilingDbTp = -1.0;
    static constexpr double kMinTargetLufs = -23.0;
    static constexpr double kMaxTargetLufs = -9.0;

signals:
    /** One file is done. Emitted for failures too (valid == false). */
    void measured(const LoudnessMeasurement &result);
    void progress(int done, int total);
    void finished(int measured, int failed, bool canceled);

private:
    void pump();
    void startOne(const QString &path);
    void complete(QProcess *proc, const QString &path);
    void finish(bool canceled);

    QStringList m_queue;
    QHash<QProcess *, QString> m_running;

    int m_maxConcurrent = 2;
    bool m_active = false;
    bool m_canceling = false;
    int m_total = 0;
    int m_done = 0;
    int m_ok = 0;
    int m_failed = 0;
};

#endif // LOUDNESSSCANNER_H
