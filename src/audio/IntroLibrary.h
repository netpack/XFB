#ifndef INTROLIBRARY_H
#define INTROLIBRARY_H

#include <QObject>
#include <QSet>
#include <QStringList>

class WaveformStore;
struct WaveformData;

/** One track's stored intro/outro, as the database holds it. */
struct IntroTimes
{
    /** Vocal entry, ms from the start of the file. -1 = never measured. */
    qint64 introMs = -1;
    /** Length of the run-out at the end. -1 = never measured. */
    qint64 outroMs = -1;
    /** The value was set by hand and must not be overwritten by a sweep. */
    bool locked = false;
    /** False when the file is not in the musics table at all. */
    bool known = false;
};

/**
 * The intro (ramp) and outro times of the tracks in the music database.
 *
 * These live in the `intro_ms` / `outro_ms` / `intro_locked` columns of
 * the `musics` table, measured once per file from the waveform
 * WaveformStore already extracts, and kept there afterwards. NULL means
 * "never measured"; a measured track always carries both numbers.
 *
 * Sibling of BpmLibrary in every respect — same WaveformStore-driven
 * queue, same in-flight cap, same "already cached, so no waveformReady is
 * coming" handling, same quiet single-file path for the track being
 * loaded. It is a separate class only because the two write different
 * columns and a sweep of one should not block a sweep of the other.
 *
 * The one thing it does that BpmLibrary does not: it refuses to overwrite
 * a row whose `intro_locked` is set. An operator who has dragged the
 * marker on the now-playing strip has told XFB something the detector
 * cannot work out for itself, and a later library sweep must not throw
 * that away. clearLock() is the deliberate way back.
 */
class IntroLibrary : public QObject
{
    Q_OBJECT

public:
    explicit IntroLibrary(WaveformStore *store, QObject *parent = nullptr);

    /** Quietness threshold handed to IntroDetector, in % of a track's own
     *  max peak. Same number the playlist auto-mix uses; player.cpp keeps
     *  the two in step from AutoMixThresholdPercent in xfb.conf. */
    void setThresholdPercent(int percent);
    int thresholdPercent() const { return m_thresholdPercent; }

    /** What the database holds for one file. */
    IntroTimes timesFor(const QString &filePath) const;

    /** Library tracks whose intro has never been measured. */
    QStringList tracksMissingIntro() const;

    /** Every library track — for a forced re-run after the detector has
     *  changed. Hand-set rows are still skipped when the result is stored. */
    QStringList allTracks() const;

    /** Measure these files in the background and write the results into the
     *  database. A sweep already running is left alone. */
    void analyze(const QStringList &filePaths);
    void cancel();
    bool busy() const { return m_active; }

    /** Fire-and-forget measurement of a single file: no progress reporting
     *  and no interference with a running sweep. This is what fills in the
     *  intro of a track the moment it is loaded. */
    void analyzeQuietly(const QString &filePath);

    /** Store a value the operator set by hand, and lock it. The outro is
     *  left as it was — dragging the intro marker says nothing about it. */
    bool setIntroByHand(const QString &filePath, qint64 introMs);

    /** Drop the hand-set flag so the detector may measure the row again. */
    bool clearLock(const QString &filePath);

signals:
    void progress(int done, int total);
    void finished(int measured, int skipped, bool canceled);
    /** A measurement (or a hand-set value) reached the database. */
    void introMeasured(const QString &filePath, const IntroTimes &times);

private:
    void pump();
    void onWaveformReady(const QString &filePath);
    // By value: completing a file drops it from the store's cache, which
    // would leave a reference into that cache dangling.
    void complete(const QString &filePath, WaveformData data);
    /** Writes the measurement unless the row is locked. Returns false when
     *  nothing was written (locked row, missing row, SQL error). */
    bool store(const QString &filePath, const WaveformData &data);

    WaveformStore *m_store = nullptr;

    QStringList m_queue;
    QSet<QString> m_inFlight;
    QSet<QString> m_quiet; // analyzeQuietly() requests, outside the sweep

    int m_thresholdPercent = 5;
    bool m_active = false;
    int m_total = 0;
    int m_done = 0;
    int m_measured = 0;
    int m_skipped = 0;
};

#endif // INTROLIBRARY_H
