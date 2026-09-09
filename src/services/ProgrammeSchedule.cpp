#include "ProgrammeSchedule.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QLocale>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtGlobal>

#include <algorithm>

namespace {

const char *const kConnection = "xfb_connection";

QSqlDatabase scheduleDb()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

/**
 * The values `scheduler.week_day` is written with, and the first thing a
 * stored value is matched against. Not display strings: translating one is
 * exactly the bug weekDayFor() below has to make good.
 */
const char *const kEnglishDays[7] = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

/** The name of one item, and the file behind it. */
struct Item
{
    QString name;
    QString path;
};

/** Everything in `pub` or in `programs`, by id, so a walk of the rules can
 *  name each one without a query per row. */
QHash<qint64, Item> itemsOf(const QString &table)
{
    QHash<qint64, Item> out;
    QSqlDatabase db = scheduleDb();
    if (!db.isOpen())
        return out;

    QSqlQuery qry(db);
    if (!qry.exec(QStringLiteral("SELECT id, name, path FROM %1").arg(table))) {
        qWarning() << "ProgrammeSchedule: could not read" << table
                   << ":" << qry.lastError().text();
        return out;
    }
    while (qry.next()) {
        Item item;
        item.name = qry.value(1).toString();
        item.path = qry.value(2).toString();
        out.insert(qry.value(0).toLongLong(), item);
    }
    return out;
}

/** A date built from three columns, invalid when any of them is missing. */
QDate dateFrom(const QSqlQuery &qry, int yearCol, int monthCol, int dayCol)
{
    if (qry.value(yearCol).isNull() || qry.value(monthCol).isNull()
        || qry.value(dayCol).isNull())
        return QDate();
    return QDate(qry.value(yearCol).toInt(), qry.value(monthCol).toInt(),
                 qry.value(dayCol).toInt());
}

} // namespace

namespace ProgrammeSchedule {

QString englishDayName(int day)
{
    if (day < 1 || day > 7)
        return QString();
    return QString::fromLatin1(kEnglishDays[day - 1]);
}

int weekDayFor(const QString &stored)
{
    const QString value = stored.trimmed();
    if (value.isEmpty())
        return 0;

    // What every rule written from now on holds.
    for (int day = 1; day <= 7; ++day) {
        if (value.compare(QLatin1String(kEnglishDays[day - 1]),
                          Qt::CaseInsensitive) == 0)
            return day;
    }

    // Rules written before this was fixed hold whatever the combo box was
    // showing, so on a translated installation the table holds "Segunda" or
    // "Lundi". Asking the translator for the very strings those combo boxes
    // are filled from is what keeps this right in a language nobody has added
    // yet — including one added after this line was written.
    for (int day = 1; day <= 7; ++day) {
        const char *english = kEnglishDays[day - 1];
        if (value.compare(QCoreApplication::translate("add_pub", english),
                          Qt::CaseInsensitive) == 0
            || value.compare(QCoreApplication::translate("add_program", english),
                             Qt::CaseInsensitive) == 0)
            return day;
    }

    // A table filled in by hand, or by an older XFB in another locale.
    const QList<QLocale> locales = {QLocale(), QLocale::system(),
                                    QLocale(QLocale::Portuguese, QLocale::Portugal),
                                    QLocale(QLocale::French, QLocale::France)};
    for (const QLocale &locale : locales) {
        for (int day = 1; day <= 7; ++day) {
            if (value.compare(locale.dayName(day, QLocale::LongFormat),
                              Qt::CaseInsensitive) == 0
                || value.compare(locale.dayName(day, QLocale::ShortFormat),
                                 Qt::CaseInsensitive) == 0)
                return day;
        }
    }

    return 0;
}

QList<Rule> rules()
{
    QList<Rule> out;
    QSqlDatabase db = scheduleDb();
    if (!db.isOpen()) {
        qWarning() << "ProgrammeSchedule: the library database is not open.";
        return out;
    }

    const QHash<qint64, Item> adverts = itemsOf(QStringLiteral("pub"));
    const QHash<qint64, Item> programmes = itemsOf(QStringLiteral("programs"));
    const QDate today = QDate::currentDate();
    const QDateTime now = QDateTime::currentDateTime();

    QSqlQuery qry(db);
    // Column order is the table's own, which run_scheduler() reads by index;
    // rowid rides along last so a rule can be named exactly.
    if (!qry.exec(QStringLiteral("SELECT *, rowid FROM scheduler"))) {
        qWarning() << "ProgrammeSchedule: could not read the scheduler table:"
                   << qry.lastError().text();
        return out;
    }

    while (qry.next()) {
        Rule rule;
        rule.itemId = qry.value(0).toLongLong();
        rule.hour = qry.value(4).toInt();
        rule.minute = qry.value(5).toInt();
        rule.kind = qry.value(14).toString() == QLatin1String("1") ? Kind::Programme
                                                                   : Kind::Advert;
        rule.rowId = qry.value(15).toLongLong();

        const QHash<qint64, Item> &table =
            rule.kind == Kind::Programme ? programmes : adverts;
        const Item item = table.value(rule.itemId);
        rule.name = item.name;
        rule.path = item.path;

        const QString tipo = qry.value(6).toString();
        if (tipo == QLatin1String("1")) {
            rule.recurrence = Recurrence::Once;
            rule.onDate = dateFrom(qry, 1, 2, 3);
        } else if (tipo == QLatin1String("2")) {
            rule.recurrence = Recurrence::Weekly;
            rule.weekDayText = qry.value(7).toString();
        } else if (tipo == QLatin1String("3")) {
            rule.recurrence = Recurrence::DailyInterval;
            rule.from = dateFrom(qry, 8, 9, 10);
            rule.to = dateFrom(qry, 11, 12, 13);
        }

        // What stops it airing, most fundamental first: a booking with no file
        // behind it is not a weekday problem, whatever else is also true.
        if (!table.contains(rule.itemId)) {
            rule.problem = Problem::ItemGone;
        } else if (rule.path.isEmpty()) {
            rule.problem = Problem::NoFile;
        } else if (!QFileInfo::exists(rule.path)) {
            rule.problem = Problem::FileMissing;
        } else {
            switch (rule.recurrence) {
            case Recurrence::Once: {
                const QTime at(rule.hour, rule.minute);
                if (!rule.onDate.isValid() || !at.isValid())
                    rule.problem = Problem::BadDates;
                else if (QDateTime(rule.onDate, at) < now)
                    rule.problem = Problem::PastOneOff;
                break;
            }
            case Recurrence::Weekly:
                rule.weekDay = weekDayFor(rule.weekDayText);
                if (rule.weekDay == 0)
                    rule.problem = Problem::WeekDayUnknown;
                break;
            case Recurrence::DailyInterval:
                if (!rule.from.isValid() || !rule.to.isValid())
                    rule.problem = Problem::BadDates;
                else if (today > rule.to)
                    rule.problem = Problem::IntervalOver;
                break;
            case Recurrence::Unknown:
                rule.problem = Problem::UnknownType;
                break;
            }
        }

        out.append(rule);
    }

    return out;
}

QList<Airing> upcoming(const QDateTime &from, const QDateTime &to, int limit)
{
    QList<Airing> out;
    if (!from.isValid() || !to.isValid() || to < from)
        return out;

    const QList<Rule> all = rules();
    for (const Rule &rule : all) {
        if (!rule.willAir())
            continue;
        if (rule.hour < 0 || rule.hour > 23 || rule.minute < 0 || rule.minute > 59)
            continue;

        const QTime at(rule.hour, rule.minute);

        if (rule.recurrence == Recurrence::Once) {
            const QDateTime when(rule.onDate, at);
            if (when >= from && when <= to)
                out.append({when, rule});
            continue;
        }

        // Weekly and interval rules both come down to "does this day count?",
        // so they walk the period the same way. The period is what the
        // operator chose, and the caller caps it — see the note on limit.
        for (QDate day = from.date(); day <= to.date(); day = day.addDays(1)) {
            if (rule.recurrence == Recurrence::Weekly) {
                if (day.dayOfWeek() != rule.weekDay)
                    continue;
            } else {
                // The scheduler fires on the closing date too, so it is in.
                if (day < rule.from || day > rule.to)
                    continue;
            }
            const QDateTime when(day, at);
            if (when < from || when > to)
                continue;
            out.append({when, rule});
        }
    }

    std::sort(out.begin(), out.end(), [](const Airing &a, const Airing &b) {
        if (a.at != b.at)
            return a.at < b.at;
        // Two things booked for the same minute: a stable order beats an
        // arbitrary one, and the rowid is the order they were booked in.
        return a.rule.rowId < b.rule.rowId;
    });

    if (limit > 0 && out.size() > limit)
        out = out.mid(0, limit);
    return out;
}

QString kindLabel(Kind kind)
{
    return kind == Kind::Programme
               ? QCoreApplication::translate("ProgrammeSchedule", "Programme")
               : QCoreApplication::translate("ProgrammeSchedule", "Advertisement");
}

QString describe(const Rule &rule)
{
    const QString at = QTime(qBound(0, rule.hour, 23), qBound(0, rule.minute, 59))
                           .toString(QStringLiteral("HH:mm"));

    switch (rule.recurrence) {
    case Recurrence::Once:
        if (!rule.onDate.isValid())
            return QCoreApplication::translate("ProgrammeSchedule", "Once, on a date that is not readable");
        return QCoreApplication::translate("ProgrammeSchedule", "Once, on %1 at %2")
            .arg(QLocale().toString(rule.onDate, QLocale::ShortFormat), at);

    case Recurrence::Weekly: {
        const int day = rule.weekDay != 0 ? rule.weekDay
                                          : weekDayFor(rule.weekDayText);
        if (day == 0)
            return QCoreApplication::translate("ProgrammeSchedule", "Every %1 at %2 — and %1 is not a day")
                .arg(rule.weekDayText, at);
        return QCoreApplication::translate("ProgrammeSchedule", "Every %1 at %2")
            .arg(QLocale().dayName(day, QLocale::LongFormat), at);
    }

    case Recurrence::DailyInterval:
        if (!rule.from.isValid() || !rule.to.isValid())
            return QCoreApplication::translate("ProgrammeSchedule", "Every day at %1, between dates that are not readable")
                .arg(at);
        return QCoreApplication::translate("ProgrammeSchedule", "Every day at %1, from %2 to %3")
            .arg(at, QLocale().toString(rule.from, QLocale::ShortFormat),
                 QLocale().toString(rule.to, QLocale::ShortFormat));

    case Recurrence::Unknown:
        break;
    }
    return QCoreApplication::translate("ProgrammeSchedule", "A kind of schedule XFB does not act on");
}

QString problemText(Problem problem)
{
    switch (problem) {
    case Problem::None:
        return QString();
    case Problem::ItemGone:
        return QCoreApplication::translate("ProgrammeSchedule",
            "The advertisement or programme it was booked for is no longer listed. "
            "Nothing will air; delete the schedule, or add the item back.");
    case Problem::NoFile:
        return QCoreApplication::translate("ProgrammeSchedule",
            "No file was ever chosen for it.");
    case Problem::FileMissing:
        return QCoreApplication::translate("ProgrammeSchedule",
            "The file is not where the library says it is. Nothing will air "
            "until it is put back or the item is added again.");
    case Problem::PastOneOff:
        return QCoreApplication::translate("ProgrammeSchedule",
            "This time has gone by without XFB running, so it never aired. A "
            "one-off is only cleared when it fires, so it will sit here until "
            "it is deleted.");
    case Problem::WeekDayUnknown:
        return QCoreApplication::translate("ProgrammeSchedule",
            "The day of the week is not a day XFB recognises, so nothing will air.");
    case Problem::BadDates:
        return QCoreApplication::translate("ProgrammeSchedule",
            "The dates on this schedule are not readable, so XFB ignores it.");
    case Problem::IntervalOver:
        return QCoreApplication::translate("ProgrammeSchedule",
            "The campaign has finished. XFB clears it, and the advertisement "
            "with it if this was its last booking, on the next check.");
    case Problem::UnknownType:
        return QCoreApplication::translate("ProgrammeSchedule",
            "This is a kind of schedule XFB does not act on.");
    }
    return QString();
}

} // namespace ProgrammeSchedule
