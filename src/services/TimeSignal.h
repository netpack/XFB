#ifndef TIMESIGNAL_H
#define TIMESIGNAL_H

#include <QCoreApplication>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

class QSqlDatabase;

/**
 * @brief The time signal: the pips, and the ident that names the hour.
 *
 * A station says the time. Sometimes that is six pips at :00; more often it is
 * a recorded line — *"it's ten o'clock, and you're listening to..."* — and the
 * whole point of it is that it names the hour it is played in. Both are the
 * same job: **this audio, at this second past the hour, on these hours, on
 * these days**.
 *
 * XFB could already do a version of it two ways, and neither is this job:
 *
 *  - the **hour clock** fires a hard-timed Station ID slot, but a clock is
 *    assigned per hour, so a different ident for each hour of the day means
 *    twenty-four clocks; it only runs while Auto Mode is driving; and a slot
 *    whose name matches nothing plays a *random* jingle, which for an ident
 *    that names the hour is the one wrong answer;
 *  - the **scheduler** takes a weekday and an hour, so an hourly signal is a
 *    hundred and sixty-eight rows typed by hand.
 *
 * So: one row per signal, with the hours and days it applies to carried as
 * masks on that row. "Every hour, every day, at :00" is one row. "The hour
 * ident, six to midnight, weekdays" is one row. And the audio can be named by
 * a **pattern** — `Hour %H` resolved against the hour that is firing — so the
 * twenty-four idents a station records once are twenty-four rows in the
 * jingles table and *one* row here.
 *
 * ### Three ways onto the air, and the difference is the whole feature
 *
 * A time signal that plays two and a half minutes late is not a time signal.
 * The existing machinery only ever queues (`insertItem(0, ...)`), which means
 * "after whatever is playing" — fine for an ident, useless for a pip. So a
 * signal says how it goes to air:
 *
 *  - **Next** — the top of the running order. Never interrupts, always late by
 *    up to the length of the current track. Right for an hour ident on a
 *    music station that would rather not chop a record.
 *  - **Interrupt** — the deck stops where it is and the signal plays, then the
 *    running order carries on with the next item. On time, and audibly so.
 *  - **Over the top** — the signal plays on its own output while the music
 *    keeps going underneath, with the music ducked by however much the
 *    operator asked for. On time, and nothing is lost.
 *
 * ### What it will not do
 *
 * If a pattern resolves to a name that no jingle has, **nothing plays** and
 * the operator is told. That is deliberate, and it is the opposite of what the
 * hour clock does: a random sweeper in place of a missing "it's three o'clock"
 * is worse than a hole, because a hole is silence and the sweeper is a lie.
 *
 * The feature is **off by default**. While it is off it owns one empty table
 * and changes nothing.
 */
class TimeSignal
{
    Q_DECLARE_TR_FUNCTIONS(TimeSignal)

public:
    /** Where the audio comes from. */
    enum class Source {
        Jingle = 0,   ///< a row of the `jingles` table, named exactly
        File,         ///< a path on disk, used as it stands
        HourPattern   ///< a jingles name built from the firing hour
    };

    /** How the signal gets onto the air. */
    enum class Placement {
        Next = 0,     ///< top of the running order: late, never interrupts
        Interrupt,    ///< stop the deck and play it now
        OverTheTop    ///< its own output, music ducked underneath
    };

    /** Bit 0 = Monday .. bit 6 = Sunday, to match Qt's Mon=1..Sun=7. */
    static constexpr quint8  kAllDays  = 0x7F;
    /** Bit h = hour h, 0..23. */
    static constexpr quint32 kAllHours = 0x00FFFFFF;

    /** One signal, as the operator wrote it. */
    struct Signal
    {
        qint64    id       = -1;
        QString   label;                       ///< what the operator calls it
        bool      enabled  = true;
        quint8    dayMask  = kAllDays;
        quint32   hourMask = kAllHours;
        int       offsetSeconds = 0;           ///< 0..3599 past the top of the hour
        Source    source   = Source::Jingle;
        QString   reference;                   ///< jingle name, path, or pattern
        Placement placement = Placement::Next;
        int       volumePercent = 100;         ///< level of the signal itself
        /** Where the music sits while an over-the-top signal plays, as a
         *  percentage of its normal level. 100 = no ducking at all. */
        int       duckPercent = 35;
        /** How late still counts as on time. A signal missed by more than
         *  this is skipped for that hour rather than played wrong. */
        int       graceSeconds = 20;

        bool firesOnDay(int qtDay) const;      ///< Mon=1..Sun=7
        bool firesInHour(int hour) const;      ///< 0..23
    };

    /** One firing, resolved: what plays, when, and what it is called. */
    struct Upcoming
    {
        QDateTime when;
        qint64    signalId = -1;
        QString   label;
        QString   path;        ///< empty when nothing matches — a hole
        QString   reference;   ///< the name the pattern resolved to
    };

    // ------------------------------------------------------------ schema --

    /** Creates the `time_signals` table. Idempotent; called from the same
     *  place in checkDbOpen() as every other XFB migration. */
    static bool ensureSchema(QSqlDatabase db);

    // ---------------------------------------------------------- settings --

    /** The switches that are not per-signal. */
    struct Settings
    {
        bool enabled = false;        ///< off by default
        /** Fire while the operator is driving, not only under Auto Mode.
         *  A station's pips are the station's, whoever is at the desk. */
        bool whenLive = true;
        /** How often the clock is looked at, in milliseconds. This is the
         *  accuracy of a signal, so it is a second by default. */
        int  tickMs = 1000;
    };

    static Settings settings();
    static void     saveSettings(const Settings &s);

    // -------------------------------------------------------- vocabulary --

    static QString sourceKey(Source s);
    static Source  sourceFromKey(const QString &key);
    static QString sourceLabel(Source s);            ///< translated

    static QString placementKey(Placement p);
    static Placement placementFromKey(const QString &key);
    static QString placementLabel(Placement p);      ///< translated

    /** "10:00", "59:55" — an offset in the hour, as a person reads it. */
    static QString formatOffset(int seconds);
    /** One signal as a sentence: days, hours, time and what plays. */
    static QString describe(const Signal &signal);

    // ------------------------------------------------------ the arithmetic --

    /**
     * The pattern tokens, resolved against @a hour (0..23):
     *
     *  - `%H` two-digit 24-hour  ("09", "22")
     *  - `%h` 24-hour, unpadded  ("9", "22")
     *  - `%I` 12-hour clock      ("9", "10")
     *  - `%p` "am" / "pm"
     *  - `%%` a literal percent
     *
     * Pure: no database, no locale surprises, so the dialog can show the
     * operator exactly what tonight's name will be while they type it.
     */
    static QString resolvePattern(const QString &pattern, int hour);

    /**
     * True when @a signal is due at this moment of the week — that is, the day
     * and hour are in its masks and @a second is inside
     * [offset, offset + grace). Pure, and the whole of the timing rule.
     */
    static bool isDue(const Signal &signal, int qtDay, int hour, int second);

    /** The next @a count firings from @a from, soonest first. Reads the
     *  jingles table to say what would actually play (and what would not). */
    static QList<Upcoming> upcoming(const QList<Signal> &signalList,
                                    const QDateTime &from, int count);

    // -------------------------------------------------------- persistence --

    static QList<Signal> allSignals();
    static Signal signalById(qint64 id);
    /** Inserts or updates; fills in @a signal's id. */
    static bool   save(Signal &signal);
    static bool   remove(qint64 id);

    /** The row a station most likely wants first: the pips at :00, every
     *  hour, every day, over the top. Not written by anything but a
     *  deliberate "add". */
    static Signal defaultSignal();

    // ------------------------------------------------------------- media --

    /**
     * The file this signal would play in @a hour, or an empty string.
     *
     * A `File` source is its own path. A `Jingle` or `HourPattern` source is
     * looked up in the `jingles` table by name, case-insensitively and
     * exactly — never a random row. An empty answer means "there is nothing
     * for this hour", and the caller says so out loud rather than playing
     * something else.
     */
    static QString mediaPathFor(const Signal &signal, int hour);

    /** The name a signal looks for in @a hour (for messages and previews). */
    static QString referenceFor(const Signal &signal, int hour);

    /** Every name in the jingles table, for the pickers. */
    static QStringList jingleNames();

private:
    static QSqlDatabase db();
};

#endif // TIMESIGNAL_H
