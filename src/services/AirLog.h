#ifndef AIRLOG_H
#define AIRLOG_H

#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class QSqlDatabase;

/**
 * @brief The as-run log: what actually went to air, and when.
 *
 * The musics table counts plays, but a count cannot answer the two questions
 * a station is actually asked — "did my spot run at 08:15?" and "what did you
 * play last Tuesday afternoon?". This writes one row per item that reaches
 * the air, with the wall-clock time it started, the time it ended and why it
 * ended, so both questions have an answer that survives a restart.
 *
 * Writes are queued, never done from the call site. Every write point here
 * sits inside a QMediaPlayer signal handler — the source switch, the
 * crossfade, the position tick — and those are the moments where a stalled
 * disk would be heard on air. Callers get an integer handle back immediately;
 * the INSERT and the UPDATEs happen when the event loop next spins, on the
 * main thread's "xfb_connection" (the only connection the library database
 * has, and the one every other query in XFB uses).
 *
 * A handle is not a row id: the row does not exist yet when open() returns.
 * flush() resolves handles to row ids in order, so a close() queued before
 * its own insert has been written still lands on the right row.
 */
class AirLog : public QObject
{
    Q_OBJECT

public:
    /** One item on its way to air. */
    struct Entry
    {
        QString source;             ///< 'musics'|'jingles'|'pub'|'programs'|'pad'|'stream'|'fallback'
        qint64  sourceId  = -1;     ///< row id in that table; < 0 is written as NULL
        QString artist;
        QString title;
        QString path;
        qint64  plannedMs = -1;     ///< intended length; < 0 is written as NULL
        bool    autoMode  = false;  ///< true: Auto Mode chose it, false: the operator did
    };

    /** One aired row, as read back by the viewer and the reports. */
    struct Record
    {
        qint64    id = 0;
        QDateTime startedAt;
        QDateTime endedAt;          ///< invalid when the row was never closed
        QString   source;
        qint64    sourceId = -1;
        QString   artist;
        QString   title;
        QString   path;
        qint64    plannedMs = -1;
        qint64    playedMs  = -1;
        QString   endedReason;
        QString   operatorMode;
    };

    /** What the viewer is asking for. Empty/invalid members mean "no filter". */
    struct Filter
    {
        QDateTime from;
        QDateTime to;
        QString   source;           ///< one source key, or empty for all
        QString   text;             ///< matched against artist, title and path
        int       limit = 5000;     ///< a busy station logs ~20 rows an hour
    };

    static AirLog *instance();

    /**
     * Creates the airlog table and its index on an existing database.
     * Idempotent — existing installs get the table on the next launch.
     */
    static bool ensureSchema(QSqlDatabase db);

    /**
     * Startup housekeeping, run once the library database is open: closes the
     * rows a crash left hanging and prunes anything older than the retention
     * setting. Safe to call more than once.
     */
    void start();

    /**
     * Works out what a path is by looking it up in the library, so a jingle
     * fired from the running order is logged as a jingle and an advertisement
     * carries the pub row id an invoice can be built from. A path that is in
     * no table at all is logged as 'fallback' — it aired, and the log says so,
     * even though nothing in the library claims it.
     */
    static Entry entryForPath(const QString &path, bool autoMode);

    /** Opens an entry. Returns a handle for close()/heartbeat(), or 0. */
    qint64 open(const Entry &entry);

    /** entryForPath() + open(), which is what every playback site wants. */
    qint64 openPath(const QString &path, bool autoMode, qint64 plannedMs = -1);

    /** The real duration, once the decoder reports it. */
    void setPlannedMs(qint64 handle, qint64 plannedMs);

    /**
     * How far the open entry has played. Throttled to one write every
     * kHeartbeatMs — this is called from the position tick, and the point of
     * it is only to leave a usable "last known position" behind after a crash.
     */
    void heartbeat(qint64 handle, qint64 playedMs);

    /**
     * @param reason 'segue'|'end'|'skipped'|'stopped'|'deadair'
     */
    void close(qint64 handle, qint64 playedMs, const QString &reason);

    /** Closes every entry still open — used when XFB shuts down cleanly. */
    void closeAll(const QString &reason);

    /** Writes anything still queued. Called on quit; otherwise automatic. */
    void flush();

    // --- reading ---------------------------------------------------------

    static QList<Record> query(const Filter &filter);

    /** Every advertisement in the pub table, as (id, name) pairs. */
    static QList<QPair<qint64, QString>> advertisements();

    /** Every time one pub row aired in a period, oldest first. */
    static QList<Record> advertiserReport(qint64 pubId, const QDateTime &from,
                                          const QDateTime &to);

    /** The source keys the schema allows, in the order the viewer lists them. */
    static QStringList sourceKeys();
    /** A translated, human name for a source key or an ended_reason. */
    static QString sourceLabel(const QString &key);
    static QString reasonLabel(const QString &key);

    // --- retention -------------------------------------------------------

    /** Days of log to keep. 0 means forever. */
    static int  retentionDays();
    static void setRetentionDays(int days);
    static constexpr int kDefaultRetentionDays = 730;  // two years of proof

    /** Removes rows older than the retention window. Returns rows deleted. */
    static int prune();

    /** The timestamp format the airlog columns store (local time, offset). */
    static QString formatTimestamp(const QDateTime &when);
    static QDateTime parseTimestamp(const QString &text);

private:
    explicit AirLog(QObject *parent = nullptr);

    /** One queued write. */
    struct Op
    {
        enum Kind { Open, Close, Planned, Position } kind = Open;
        qint64  handle = 0;
        Entry   entry;
        qint64  value  = -1;        ///< played_ms or planned_ms
        QString reason;
        QDateTime stamp;
    };

    void scheduleFlush();

    static constexpr qint64 kHeartbeatMs = 15000;

    QList<Op>          m_pending;
    QHash<qint64, qint64> m_rowIds;      ///< handle -> airlog.id, once inserted
    QHash<qint64, QElapsedTimer> m_lastBeat;
    qint64             m_nextHandle = 1;
    bool               m_flushQueued = false;
    bool               m_started = false;
};

#endif // AIRLOG_H
