#ifndef BPMLIBRARY_H
#define BPMLIBRARY_H

#include <QObject>
#include <QSet>
#include <QStringList>

class WaveformStore;
struct WaveformData;

/**
 * The tempo of the tracks in the music database.
 *
 * BPM lives in the `bpm` column of the `musics` table, measured once per
 * file by WaveformStore's decode and kept there afterwards. Three states
 * are distinguished:
 *   - NULL  — never analysed
 *   - 0     — analysed, no tempo could be measured (speech, free tempo)
 *   - > 0   — the measured tempo
 *
 * so a library sweep never re-decodes a file it has already given up on.
 */
class BpmLibrary : public QObject
{
    Q_OBJECT

public:
    explicit BpmLibrary(WaveformStore *store, QObject *parent = nullptr);

    /** Tempo known for a file, or 0 when unknown. Consults the waveform
     *  already in memory first, so files that are not in the library (a
     *  track dropped straight into the playlist) also report a tempo. */
    double bpmFor(const QString &filePath) const;

    /** Library tracks whose BPM has never been measured. */
    QStringList tracksMissingBpm() const;

    /** Every library track, measured or not — for a forced re-run after the
     *  detector itself has changed. */
    QStringList allTracks() const;

    /** Measures the tempo of these files in the background and writes it
     *  into the database. A sweep already running is left alone. */
    void analyze(const QStringList &filePaths);
    void cancel();
    bool busy() const { return m_active; }

    /** Fire-and-forget analysis of a single file: no progress reporting and
     *  no interference with a running sweep. Used to fill in the tempo of a
     *  track auto mode wants to match against. */
    void analyzeQuietly(const QString &filePath);

signals:
    void progress(int done, int total);
    void finished(int analyzed, int skipped, bool canceled);
    /** A tempo was measured and stored (bpm is 0 when none was found). */
    void bpmMeasured(const QString &filePath, double bpm);

private:
    void pump();
    void onWaveformReady(const QString &filePath);
    // By value: completing a file drops it from the store's cache, which
    // would leave a reference into that cache dangling.
    void complete(const QString &filePath, WaveformData data);
    void store(const QString &filePath, double bpm);
    void finish(bool canceled);

    WaveformStore *m_store = nullptr;

    QStringList m_queue;
    QSet<QString> m_inFlight;
    QSet<QString> m_quiet; // analyzeQuietly() requests, outside the sweep

    bool m_active = false;
    int m_total = 0;
    int m_done = 0;
    int m_analyzed = 0;
    int m_skipped = 0;
};

#endif // BPMLIBRARY_H
