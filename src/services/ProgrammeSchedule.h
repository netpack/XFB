#ifndef PROGRAMMESCHEDULE_H
#define PROGRAMMESCHEDULE_H

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>

/**
 * @brief What is booked to go on air, and when.
 *
 * The `scheduler` table has been in XFB since the beginning and is written by
 * "Add a publicity" and "Add a program", but until now nothing could read it
 * back: the only way to see a booking was to reopen the window that made it,
 * one advert at a time, and even that only listed the rules added in that same
 * sitting. An operator asked what is going out tomorrow had no answer.
 *
 * This turns those rules into two things the station actually asks for:
 * the rules themselves, each in a sentence (rules()), and the concrete
 * airings they come to over a period (upcoming()).
 *
 * It reads; it never writes. Firing the rules stays where it was, in
 * player::run_scheduler(), and this deliberately mirrors that function's
 * reading of the table rather than inventing a second interpretation — so a
 * rule the scheduler will not fire is reported here as one that will not air,
 * instead of being quietly drawn on the timetable.
 */
namespace ProgrammeSchedule {

/** Which library the booking points into. */
enum class Kind {
    Advert,     ///< a row of `pub`   (is_program = 0)
    Programme   ///< a row of `programs` (is_program = 1)
};

/** The shape of a booking — the `tipo` column, named. */
enum class Recurrence {
    Once,           ///< tipo 1: one date and time, then the rule is gone
    Weekly,         ///< tipo 2: every <weekday> at hh:mm, for ever
    DailyInterval,  ///< tipo 3: every day at hh:mm, between two dates
    Unknown         ///< a tipo the scheduler does not act on
};

/** Why a rule will not put anything to air. Empty means it is sound. */
enum class Problem {
    None,
    ItemGone,       ///< the advert or programme it points at is no longer listed
    FileMissing,    ///< the row is there, the file on disk is not
    NoFile,         ///< the row was never given a file
    PastOneOff,     ///< a one-off whose minute went by without XFB running
    WeekDayUnknown, ///< a weekday that is not a day name at all
    BadDates,       ///< an interval with no usable dates
    IntervalOver,   ///< the campaign has finished; the next check clears it
    UnknownType     ///< a `tipo` nothing acts on
};

/** One row of the `scheduler` table, read into something with names. */
struct Rule
{
    qint64     rowId = 0;       ///< scheduler.rowid — how a rule is named exactly
    qint64     itemId = -1;     ///< pub.id or programs.id
    Kind       kind = Kind::Advert;
    Recurrence recurrence = Recurrence::Unknown;

    int   hour = -1;
    int   minute = -1;
    QDate onDate;               ///< Once
    int   weekDay = 0;          ///< Weekly: 1 = Monday .. 7 = Sunday, 0 = unresolved
    QString weekDayText;        ///< Weekly: exactly what the table holds
    QDate from;                 ///< DailyInterval
    QDate to;                   ///< DailyInterval

    QString name;               ///< the advert's or programme's name
    QString path;               ///< the file it will play
    Problem problem = Problem::None;

    bool willAir() const { return problem == Problem::None; }
};

/** One concrete airing, worked out from a rule. */
struct Airing
{
    QDateTime at;
    Rule      rule;
};

/** Every booking in the table, adverts and programmes together. */
QList<Rule> rules();

/**
 * The airings the rules come to between @a from and @a to, both included,
 * earliest first.
 *
 * Rules that will not fire contribute nothing — an operator reading this is
 * asking what the listener will hear, and a booking XFB cannot honour is not
 * part of that. They are in rules() instead, each with the reason.
 *
 * @param limit a ceiling on the number of airings returned, so a wide period
 *        over a daily campaign cannot fill memory. 0 means no ceiling.
 */
QList<Airing> upcoming(const QDateTime &from, const QDateTime &to, int limit = 5000);

/** "Every Tuesday at 21:30", and the like — the rule as a sentence. */
QString describe(const Rule &rule);

/** What is wrong with a rule, in words an operator can act on. */
QString problemText(Problem problem);

/** "Advertisement" / "Programme". */
QString kindLabel(Kind kind);

/**
 * The day of the week a stored `week_day` value means.
 *
 * Weekly rules written before 2026-09-09 hold whatever the combo box was
 * *showing* when they were saved, which on a translated installation is
 * "Segunda" or "Lundi". This resolves those as well as the English names now
 * written, and player::run_scheduler() fires on the answer — so a booking made
 * in Portuguese finally goes on air.
 *
 * @return 1 = Monday .. 7 = Sunday, or 0 when the value is not a day at all.
 */
int weekDayFor(const QString &stored);

/**
 * "Monday" .. "Sunday" for 1..7, empty otherwise. Deliberately untranslated:
 * this is the value stored in `scheduler.week_day`, not something to show, and
 * writing a translated one there is the bug weekDayFor() has to clean up
 * after. Storing English also keeps a row readable by an older XFB it is
 * published to.
 */
QString englishDayName(int day);

} // namespace ProgrammeSchedule

#endif // PROGRAMMESCHEDULE_H
