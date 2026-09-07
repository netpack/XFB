#include "TimeSignal.h"

#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

#include <algorithm>

namespace {

const char *const kConnection = "xfb_connection";

/** The same xfb.conf every other XFB setting lives in. */
QString settingsFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/** Mon=1..Sun=7 -> bit 0..6. Anything else is not a day. */
inline int dayBit(int qtDay)
{
    return (qtDay >= 1 && qtDay <= 7) ? (qtDay - 1) : -1;
}

} // namespace

QSqlDatabase TimeSignal::db()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

bool TimeSignal::Signal::firesOnDay(int qtDay) const
{
    const int bit = dayBit(qtDay);
    return bit >= 0 && (dayMask & (quint8(1) << bit));
}

bool TimeSignal::Signal::firesInHour(int hour) const
{
    return hour >= 0 && hour <= 23 && (hourMask & (quint32(1) << hour));
}

// ------------------------------------------------------------------ schema --

bool TimeSignal::ensureSchema(QSqlDatabase database)
{
    if (!database.isOpen())
        return false;

    QSqlQuery qry(database);
    const char *const kTable =
        "CREATE TABLE IF NOT EXISTS time_signals ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  label TEXT,"
        "  enabled INTEGER NOT NULL DEFAULT 1,"
        "  day_mask INTEGER NOT NULL DEFAULT 127,"
        "  hour_mask INTEGER NOT NULL DEFAULT 16777215,"
        "  offset_seconds INTEGER NOT NULL DEFAULT 0,"
        "  source TEXT NOT NULL DEFAULT 'jingle',"
        "  reference TEXT,"
        "  placement TEXT NOT NULL DEFAULT 'next',"
        "  volume_percent INTEGER NOT NULL DEFAULT 100,"
        "  duck_percent INTEGER NOT NULL DEFAULT 35,"
        "  grace_seconds INTEGER NOT NULL DEFAULT 20"
        ")";
    if (!qry.exec(QLatin1String(kTable))) {
        qWarning() << "TimeSignal: could not create the time_signals table:"
                   << qry.lastError().text();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- settings --

TimeSignal::Settings TimeSignal::settings()
{
    Settings s;
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("TimeSignal"));
    s.enabled  = config.value(QStringLiteral("Enabled"), s.enabled).toBool();
    s.whenLive = config.value(QStringLiteral("WhenLive"), s.whenLive).toBool();
    // Half a second is as fine as this is worth being; ten is coarse enough
    // that a signal can miss its own grace window.
    s.tickMs = qBound(500, config.value(QStringLiteral("TickMs"), s.tickMs).toInt(), 10000);
    config.endGroup();
    return s;
}

void TimeSignal::saveSettings(const Settings &s)
{
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("TimeSignal"));
    config.setValue(QStringLiteral("Enabled"), s.enabled);
    config.setValue(QStringLiteral("WhenLive"), s.whenLive);
    config.setValue(QStringLiteral("TickMs"), s.tickMs);
    config.endGroup();
    config.sync();
}

// -------------------------------------------------------------- vocabulary --

QString TimeSignal::sourceKey(Source s)
{
    switch (s) {
    case Source::Jingle:      return QStringLiteral("jingle");
    case Source::File:        return QStringLiteral("file");
    case Source::HourPattern: return QStringLiteral("pattern");
    }
    return QStringLiteral("jingle");
}

TimeSignal::Source TimeSignal::sourceFromKey(const QString &key)
{
    if (key == QLatin1String("file"))    return Source::File;
    if (key == QLatin1String("pattern")) return Source::HourPattern;
    return Source::Jingle;
}

QString TimeSignal::sourceLabel(Source s)
{
    switch (s) {
    case Source::Jingle:      return tr("A jingle, by name");
    case Source::File:        return tr("A file on disk");
    case Source::HourPattern: return tr("One jingle per hour, by pattern");
    }
    return QString();
}

QString TimeSignal::placementKey(Placement p)
{
    switch (p) {
    case Placement::Next:       return QStringLiteral("next");
    case Placement::Interrupt:  return QStringLiteral("interrupt");
    case Placement::OverTheTop: return QStringLiteral("over");
    }
    return QStringLiteral("next");
}

TimeSignal::Placement TimeSignal::placementFromKey(const QString &key)
{
    if (key == QLatin1String("interrupt")) return Placement::Interrupt;
    if (key == QLatin1String("over"))      return Placement::OverTheTop;
    return Placement::Next;
}

QString TimeSignal::placementLabel(Placement p)
{
    switch (p) {
    case Placement::Next:       return tr("Next in the running order");
    case Placement::Interrupt:  return tr("Interrupt what is playing");
    case Placement::OverTheTop: return tr("Over the top, music ducked");
    }
    return QString();
}

QString TimeSignal::formatOffset(int seconds)
{
    const int s = qBound(0, seconds, 3599);
    return QStringLiteral("%1:%2")
        .arg(s / 60, 2, 10, QLatin1Char('0'))
        .arg(s % 60, 2, 10, QLatin1Char('0'));
}

QString TimeSignal::describe(const Signal &signal)
{
    // Days first, because that is how an operator says it out loud: "weekdays,
    // six to midnight, at the top of the hour".
    QString days;
    if (signal.dayMask == kAllDays)
        days = tr("every day");
    else if (signal.dayMask == 0x1F)
        days = tr("Monday to Friday");
    else if (signal.dayMask == 0x60)
        days = tr("at the weekend");
    else {
        QStringList names;
        const QLocale locale;
        for (int day = 1; day <= 7; ++day)
            if (signal.firesOnDay(day))
                names << locale.dayName(day, QLocale::ShortFormat);
        days = names.join(QStringLiteral(", "));
    }

    QString hours;
    if (signal.hourMask == kAllHours) {
        hours = tr("every hour");
    } else {
        // A single run of hours reads as a range; anything else is listed.
        int first = -1, last = -1, runs = 0;
        bool inRun = false;
        for (int h = 0; h <= 23; ++h) {
            const bool on = signal.firesInHour(h);
            if (on && !inRun) { ++runs; if (first < 0) first = h; }
            if (on) last = h;
            inRun = on;
        }
        if (runs == 1 && first >= 0)
            hours = (first == last) ? tr("at %1:00").arg(first, 2, 10, QLatin1Char('0'))
                                    : tr("%1:00 to %2:00").arg(first, 2, 10, QLatin1Char('0'))
                                                          .arg(last,  2, 10, QLatin1Char('0'));
        else {
            QStringList list;
            for (int h = 0; h <= 23; ++h)
                if (signal.firesInHour(h))
                    list << QStringLiteral("%1").arg(h, 2, 10, QLatin1Char('0'));
            hours = list.isEmpty() ? tr("no hours at all") : list.join(QStringLiteral(", "));
        }
    }

    return tr("%1, %2, at %3 past the hour — %4")
        .arg(days, hours, formatOffset(signal.offsetSeconds),
             placementLabel(signal.placement));
}

// ------------------------------------------------------------ the arithmetic --

QString TimeSignal::resolvePattern(const QString &pattern, int hour)
{
    const int h = qBound(0, hour, 23);
    QString out;
    out.reserve(pattern.size() + 8);

    for (int i = 0; i < pattern.size(); ++i) {
        if (pattern.at(i) != QLatin1Char('%') || i + 1 >= pattern.size()) {
            out.append(pattern.at(i));
            continue;
        }
        const QChar token = pattern.at(++i);
        if (token == QLatin1Char('H'))
            out.append(QStringLiteral("%1").arg(h, 2, 10, QLatin1Char('0')));
        else if (token == QLatin1Char('h'))
            out.append(QString::number(h));
        else if (token == QLatin1Char('I'))
            out.append(QString::number(h % 12 == 0 ? 12 : h % 12));
        else if (token == QLatin1Char('p'))
            out.append(h < 12 ? QStringLiteral("am") : QStringLiteral("pm"));
        else if (token == QLatin1Char('%'))
            out.append(QLatin1Char('%'));
        else {
            // An unknown token is not an error worth refusing a name over:
            // it stays as it was typed, so the operator sees it in the
            // preview and fixes it.
            out.append(QLatin1Char('%'));
            out.append(token);
        }
    }
    return out;
}

bool TimeSignal::isDue(const Signal &signal, int qtDay, int hour, int second)
{
    if (!signal.enabled)
        return false;
    if (!signal.firesOnDay(qtDay) || !signal.firesInHour(hour))
        return false;

    const int start = qBound(0, signal.offsetSeconds, 3599);
    const int grace = qBound(1, signal.graceSeconds, 600);
    return second >= start && second < start + grace;
}

QList<TimeSignal::Upcoming> TimeSignal::upcoming(const QList<Signal> &signalList,
                                                 const QDateTime &from, int count)
{
    QList<Upcoming> out;
    if (signalList.isEmpty() || count <= 0)
        return out;

    // A week of hours is the longest wait that can exist: a signal with any
    // day and any hour set fires inside one. Walking them is 168 iterations,
    // which is cheaper than being clever about it.
    const QDateTime base(from.date(), QTime(from.time().hour(), 0));
    for (int step = 0; step <= 24 * 7 && out.size() < count; ++step) {
        const QDateTime hourStart = base.addSecs(qint64(step) * 3600);
        const int day  = hourStart.date().dayOfWeek();
        const int hour = hourStart.time().hour();

        for (const Signal &signal : signalList) {
            if (!signal.enabled || !signal.firesOnDay(day) || !signal.firesInHour(hour))
                continue;
            const QDateTime when = hourStart.addSecs(qBound(0, signal.offsetSeconds, 3599));
            if (when < from)
                continue;   // already gone by this hour

            Upcoming u;
            u.when      = when;
            u.signalId  = signal.id;
            u.label     = signal.label;
            u.reference = referenceFor(signal, hour);
            u.path      = mediaPathFor(signal, hour);
            out.append(u);
        }
    }

    std::sort(out.begin(), out.end(), [](const Upcoming &a, const Upcoming &b) {
        return a.when < b.when;
    });
    if (out.size() > count)
        out.erase(out.begin() + count, out.end());
    return out;
}

// -------------------------------------------------------------- persistence --

namespace {

TimeSignal::Signal signalFromRow(const QSqlQuery &qry)
{
    TimeSignal::Signal s;
    s.id            = qry.value(0).toLongLong();
    s.label         = qry.value(1).toString();
    s.enabled       = qry.value(2).toInt() != 0;
    s.dayMask       = quint8(qry.value(3).toUInt() & TimeSignal::kAllDays);
    s.hourMask      = quint32(qry.value(4).toUInt() & TimeSignal::kAllHours);
    s.offsetSeconds = qBound(0, qry.value(5).toInt(), 3599);
    s.source        = TimeSignal::sourceFromKey(qry.value(6).toString());
    s.reference     = qry.value(7).toString();
    s.placement     = TimeSignal::placementFromKey(qry.value(8).toString());
    s.volumePercent = qBound(0, qry.value(9).toInt(), 100);
    s.duckPercent   = qBound(0, qry.value(10).toInt(), 100);
    s.graceSeconds  = qBound(1, qry.value(11).toInt(), 600);
    return s;
}

const char *const kColumns =
    "id, label, enabled, day_mask, hour_mask, offset_seconds, source,"
    " reference, placement, volume_percent, duck_percent, grace_seconds";

} // namespace

QList<TimeSignal::Signal> TimeSignal::allSignals()
{
    QList<Signal> out;
    QSqlDatabase database = db();
    if (!database.isOpen())
        return out;

    QSqlQuery qry(database);
    if (!qry.exec(QStringLiteral("select %1 from time_signals order by offset_seconds, id")
                      .arg(QLatin1String(kColumns)))) {
        qWarning() << "TimeSignal: could not read the signals:" << qry.lastError().text();
        return out;
    }
    while (qry.next())
        out.append(signalFromRow(qry));
    return out;
}

TimeSignal::Signal TimeSignal::signalById(qint64 id)
{
    Signal s;
    QSqlDatabase database = db();
    if (!database.isOpen() || id <= 0)
        return s;

    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("select %1 from time_signals where id = :id")
                    .arg(QLatin1String(kColumns)));
    qry.bindValue(QStringLiteral(":id"), id);
    if (qry.exec() && qry.next())
        s = signalFromRow(qry);
    return s;
}

bool TimeSignal::save(Signal &signal)
{
    QSqlDatabase database = db();
    if (!database.isOpen())
        return false;

    QSqlQuery qry(database);
    if (signal.id > 0) {
        qry.prepare(QStringLiteral(
            "update time_signals set label = :label, enabled = :enabled,"
            " day_mask = :days, hour_mask = :hours, offset_seconds = :offset,"
            " source = :source, reference = :reference, placement = :placement,"
            " volume_percent = :volume, duck_percent = :duck,"
            " grace_seconds = :grace where id = :id"));
        qry.bindValue(QStringLiteral(":id"), signal.id);
    } else {
        qry.prepare(QStringLiteral(
            "insert into time_signals (label, enabled, day_mask, hour_mask,"
            " offset_seconds, source, reference, placement, volume_percent,"
            " duck_percent, grace_seconds)"
            " values (:label, :enabled, :days, :hours, :offset, :source,"
            " :reference, :placement, :volume, :duck, :grace)"));
    }
    qry.bindValue(QStringLiteral(":label"), signal.label);
    qry.bindValue(QStringLiteral(":enabled"), signal.enabled ? 1 : 0);
    qry.bindValue(QStringLiteral(":days"), uint(signal.dayMask));
    qry.bindValue(QStringLiteral(":hours"), uint(signal.hourMask));
    qry.bindValue(QStringLiteral(":offset"), qBound(0, signal.offsetSeconds, 3599));
    qry.bindValue(QStringLiteral(":source"), sourceKey(signal.source));
    qry.bindValue(QStringLiteral(":reference"), signal.reference);
    qry.bindValue(QStringLiteral(":placement"), placementKey(signal.placement));
    qry.bindValue(QStringLiteral(":volume"), qBound(0, signal.volumePercent, 100));
    qry.bindValue(QStringLiteral(":duck"), qBound(0, signal.duckPercent, 100));
    qry.bindValue(QStringLiteral(":grace"), qBound(1, signal.graceSeconds, 600));

    if (!qry.exec()) {
        qWarning() << "TimeSignal: could not save the signal:" << qry.lastError().text();
        return false;
    }
    if (signal.id <= 0)
        signal.id = qry.lastInsertId().toLongLong();
    return true;
}

bool TimeSignal::remove(qint64 id)
{
    QSqlDatabase database = db();
    if (!database.isOpen() || id <= 0)
        return false;

    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("delete from time_signals where id = :id"));
    qry.bindValue(QStringLiteral(":id"), id);
    if (!qry.exec()) {
        qWarning() << "TimeSignal: could not delete the signal:" << qry.lastError().text();
        return false;
    }
    return true;
}

TimeSignal::Signal TimeSignal::defaultSignal()
{
    Signal s;
    s.label     = tr("Time signal");
    s.enabled   = true;
    s.dayMask   = kAllDays;
    s.hourMask  = kAllHours;
    s.offsetSeconds = 0;
    s.source    = Source::Jingle;
    s.reference.clear();
    s.placement = Placement::OverTheTop;
    return s;
}

// ------------------------------------------------------------------- media --

QString TimeSignal::referenceFor(const Signal &signal, int hour)
{
    if (signal.source == Source::HourPattern)
        return resolvePattern(signal.reference, hour);
    return signal.reference;
}

QString TimeSignal::mediaPathFor(const Signal &signal, int hour)
{
    if (signal.source == Source::File) {
        const QString path = signal.reference.trimmed();
        if (path.isEmpty() || !QFileInfo::exists(path))
            return QString();
        return path;
    }

    const QString wanted = referenceFor(signal, hour).trimmed();
    if (wanted.isEmpty())
        return QString();

    QSqlDatabase database = db();
    if (!database.isOpen())
        return QString();

    // Exactly this name, and no fallback to a random row. An ident that names
    // the hour is either the right one or nothing at all: the hour clock's
    // "any jingle will do" rule is what makes a station announce four o'clock
    // at nine, and it is not repeated here.
    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("select path from jingles"
                               " where lower(trim(name)) = lower(trim(:name)) limit 1"));
    qry.bindValue(QStringLiteral(":name"), wanted);
    if (qry.exec() && qry.next()) {
        const QString path = qry.value(0).toString();
        if (!path.isEmpty() && QFileInfo::exists(path))
            return path;
        if (!path.isEmpty())
            qWarning() << "TimeSignal: the jingle" << wanted << "points at"
                       << path << "which is not there";
    }
    return QString();
}

QStringList TimeSignal::jingleNames()
{
    QStringList out;
    QSqlDatabase database = db();
    if (!database.isOpen())
        return out;

    QSqlQuery qry(database);
    if (!qry.exec(QStringLiteral("select name from jingles where name is not null"
                                 " and trim(name) <> '' order by name")))
        return out;
    while (qry.next())
        out << qry.value(0).toString();
    return out;
}
