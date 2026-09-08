#ifndef HOURCLOCK_H
#define HOURCLOCK_H

#include <QCoreApplication>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

class QSqlDatabase;

/**
 * @brief The hour clock: the format, written the way radio actually thinks
 *        about it.
 *
 * XFB already has an hour grid — one genre per hour per weekday (`hourgenre`)
 * and programmes pinned to an hour and a minute (`hourprograms`). That is a
 * table. Radio does not plan an hour as a table; it plans it as a *clock*: a
 * pie, read clockwise from the top, with music sweeps, ad breaks, jingle
 * positions, news at :00 and the traffic report at :20.
 *
 * The difference is not decoration. A clock says two things a grid cannot:
 *
 *  - **order**, so "sweep, jingle, sweep, ad break, news" is a fact about the
 *    hour rather than something the operator remembers, and
 *  - **which items own their time**. News at the top of the hour must start at
 *    :00 exactly — it is *hard-timed*. The sweep before it does not have a
 *    length of its own; it has "whatever is left until the news", and it is
 *    *floating*.
 *
 * ### The arithmetic (Timeline)
 *
 * resolve() turns a clock into a timeline of second-accurate slots:
 *
 *  - A hard-timed slot **always** keeps its nominal start. Nothing that
 *    happens to the slots around it can move it. That is the whole point of
 *    the flag, and the harness asserts it.
 *  - Floating slots between two hard-timed anchors are laid end to end from
 *    the end of the earlier anchor. If there is room to spare, the **music
 *    sweeps** in that run absorb it, in proportion to their nominal length
 *    (if the run has no sweep, the last floating slot takes it). That is what
 *    "the sweep ends when the news is due" means numerically.
 *  - If the floating slots do **not** fit, nothing is silently shortened. The
 *    slots keep their nominal lengths, they are marked as overrunning, and
 *    Timeline::overfullSeconds says by how much. An operator who has written
 *    an impossible hour is told so; they are not handed a quietly truncated
 *    one and left to discover it on air.
 *  - Room that no floating slot can absorb — a gap before an anchor with
 *    nothing between it and the previous one — is reported as
 *    underfilledSeconds rather than pretended away.
 *
 * ### What this does NOT do
 *
 * It does not replace `hourgenre` or `hourprograms`, and it is off by default
 * (HourClock/Enabled in xfb.conf). A station that never opens the feature has
 * three unused tables and byte-identical behaviour. When it *is* on, the
 * clock's current music sweep supplies Auto Mode's genre in place of the
 * hourgenre row, and the picker's three-pass relaxation ladder is untouched:
 * the genre only ever narrows passes 0 and 1, and pass 2 is still the whole
 * library. A clock can no more silence a station than an hourgenre row can.
 */
class HourClock
{
    Q_DECLARE_TR_FUNCTIONS(HourClock)

public:
    /** Seconds in the hour a clock describes. */
    static constexpr int kHourSeconds = 3600;

    /** What a slot puts on air. */
    enum class SlotType {
        MusicSweep = 0,  ///< a run of music; `reference` is the genre (may be empty)
        AdBreak,         ///< commercial break; `reference` names a `pub` row
        Jingle,          ///< `reference` names a `jingles` row
        Program,         ///< `reference` names a `programs` row
        News,            ///< normally hard-timed at :00; a `programs` row
        StationId        ///< the ident; a `jingles` row
    };

    /** One item in the hour, as the operator wrote it. */
    struct Slot
    {
        qint64   id       = -1;      ///< row id, -1 until saved
        qint64   clockId  = -1;
        int      position = 0;       ///< order within the hour, 0-based
        SlotType type     = SlotType::MusicSweep;
        QString  label;              ///< what the operator calls it
        QString  reference;          ///< genre / jingle / ad / programme name
        int      startSeconds  = 0;  ///< nominal offset from the top of the hour
        int      lengthSeconds = 600;
        bool     hardTimed     = false;

        bool isMusic() const { return type == SlotType::MusicSweep; }
    };

    /** A named hour, and its slots in order. */
    struct Clock
    {
        qint64      id = -1;
        QString     name;
        QString     notes;
        QList<Slot> items;   ///< NOT "slots": Qt #defines that keyword

        bool isValid() const { return id > 0 || !name.isEmpty(); }
    };

    /** One slot after the arithmetic has been done to it. */
    struct ResolvedSlot
    {
        Slot slot;
        int  start  = 0;         ///< actual start, seconds from the top of the hour
        int  length = 0;         ///< actual length after slack / overrun
        int  slackSeconds = 0;   ///< how much room this slot absorbed (may be < 0)
        bool overruns = false;   ///< this slot runs past the next hard-timed item

        int end() const { return start + length; }
    };

    /** A whole hour, resolved. */
    struct Timeline
    {
        QList<ResolvedSlot> items;  ///< NOT "slots": Qt #defines that keyword
        int  overfullSeconds    = 0;  ///< by how much the hour cannot fit
        int  underfilledSeconds = 0;  ///< room nothing floating could absorb
        QStringList warnings;         ///< translated, for the operator to read

        bool isOverfull() const    { return overfullSeconds > 0; }
        bool isUnderfilled() const { return underfilledSeconds > 0; }
        bool fits() const { return overfullSeconds == 0 && underfilledSeconds == 0; }

        /** The slot covering @a second of the hour, or -1. */
        int indexAt(int second) const;
        /** The first music sweep at or after @a second, else the first one
         *  at all, else -1. Auto Mode asks this: it wants a genre to fill
         *  with, and it would rather have the next sweep's than none. */
        int musicIndexFor(int second) const;
    };

    /** Whether Auto Mode and the scheduler follow the clock at all. */
    struct Settings
    {
        bool enabled       = false;  ///< off by default: the old hour grid rules
        /** Put the clock's non-music items on air at the time the timeline
         *  gives them — pinned for a hard-timed item, computed for a
         *  floating one. Stored as FireHardTimed, which is what it was
         *  called when only hard-timed items were fired. */
        bool fireHardTimed = true;
        /** How close to an item's start counts as "now", in seconds. The
         *  check runs once a minute, so this is a minute. */
        int  fireWindowSeconds = 60;
    };

    // ------------------------------------------------------------ schema --

    /**
     * Creates the three clock tables. Idempotent, and called from the same
     * place in player::checkDbOpen() as every other XFB migration, so an
     * install that predates the feature gets them on its next launch.
     */
    static bool ensureSchema(QSqlDatabase db);

    // ---------------------------------------------------------- settings --

    static Settings settings();
    static void     saveSettings(const Settings &s);

    // -------------------------------------------------------- vocabulary --

    static QList<SlotType> slotTypes();
    static QString typeKey(SlotType type);          ///< stored in the table
    static SlotType typeFromKey(const QString &key);
    static QString typeLabel(SlotType type);        ///< translated, for people
    /** The table a slot of this type draws its audio from, or empty for
     *  music sweeps (which draw from the library through Auto Mode). */
    static QString mediaTable(SlotType type);

    /** "07:30" — a second offset as a clock position in the hour. */
    static QString formatOffset(int seconds);
    /** "4 min 30 s", "45 s" — a duration, translated. */
    static QString formatLength(int seconds);
    /** One slot as a screen reader should hear it. */
    static QString describeSlot(const ResolvedSlot &r);
    /** The hour's total, over- or under-filled, as one translated line. */
    static QString describeTotal(const Timeline &t);

    // ------------------------------------------------------ the arithmetic --

    /**
     * The whole of the timing rule set. Pure: it touches no database and no
     * settings, which is what makes it testable and what makes both the wheel
     * and the accessible list able to render exactly the same numbers.
     */
    static Timeline resolve(const Clock &clock);

    /** Slots sorted by position, positions renumbered 0..n-1. */
    static void normalise(Clock &clock);

    // -------------------------------------------------------- persistence --

    struct ClockInfo { qint64 id = -1; QString name; int slotCount = 0; };

    static QList<ClockInfo> clockList();
    static Clock loadClock(qint64 id);
    static Clock loadClockByName(const QString &name);
    /** Inserts or updates, slots and all. Fills in @a clock's ids. */
    static bool  saveClock(Clock &clock);
    static bool  deleteClock(qint64 id);
    static bool  renameClock(qint64 id, const QString &newName);
    /** Copies a clock and its slots under @a newName; returns the new id. */
    static qint64 duplicateClock(qint64 id, const QString &newName);
    /** A name not already taken, derived from @a wanted ("Breakfast 2"). */
    static QString uniqueName(const QString &wanted);

    /** The starter clock a station gets when it first opens the library:
     *  news at :00, sweeps, a jingle, an ad break at :20. Not written to the
     *  database by anything except an explicit "create". */
    static Clock exampleClock(const QString &name);

    // -------------------------------------------------------- assignments --

    /** @param day Qt weekday, Mon=1..Sun=7. @param hour 0-23. */
    static bool   assignClock(int day, int hour, qint64 clockId);
    /** Clears the assignment for that hour (back to the plain hour grid). */
    static bool   clearAssignment(int day, int hour);
    /** -1 when that hour has no clock. */
    static qint64 clockIdFor(int day, int hour);
    /** The whole week in one query: key = day * 24 + hour. */
    static QHash<int, qint64> assignments();
    /** Assigns @a clockId to every hour of @a day. */
    static bool assignWholeDay(int day, qint64 clockId);

    // ------------------------------------------------------------- media --

    /**
     * A file to play for a non-music slot: the row of the slot's table whose
     * name matches `reference`, else a random row of that table, else empty.
     * Empty is a normal answer — a station with no ads has no ad break audio,
     * and the caller simply plays nothing.
     */
    static QString mediaPathFor(const Slot &slot);

    // ------------------------------------------------- what Auto Mode asks --

    /**
     * The genre the clock wants filled at @a second of the hour on @a day, or
     * an empty string when the clock has nothing to say (feature off, no clock
     * assigned, the sweep has no genre). An empty string means "fall back to
     * the hourgenre row", which is exactly the old behaviour.
     */
    static QString genreForMoment(int day, int hour, int second);

private:
    static QSqlDatabase db();
};

#endif // HOURCLOCK_H
