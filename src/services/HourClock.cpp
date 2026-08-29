#include "HourClock.h"

#include <QDateTime>
#include <QDebug>
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

} // namespace

QSqlDatabase HourClock::db()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

// ------------------------------------------------------------------ schema --

bool HourClock::ensureSchema(QSqlDatabase database)
{
    if (!database.isOpen())
        return false;

    QSqlQuery qry(database);

    const char *const kClocks =
        "CREATE TABLE IF NOT EXISTS hourclocks ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE,"
        "  notes TEXT,"
        "  updated_at TEXT"
        ")";
    if (!qry.exec(QLatin1String(kClocks))) {
        qWarning() << "HourClock: could not create the hourclocks table:"
                   << qry.lastError().text();
        return false;
    }

    const char *const kSlots =
        "CREATE TABLE IF NOT EXISTS hourclock_slots ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  clock_id INTEGER NOT NULL,"
        "  position INTEGER NOT NULL DEFAULT 0,"
        "  type TEXT NOT NULL DEFAULT 'music',"
        "  label TEXT,"
        "  reference TEXT,"
        "  start_seconds INTEGER NOT NULL DEFAULT 0,"
        "  length_seconds INTEGER NOT NULL DEFAULT 600,"
        "  hard_timed INTEGER NOT NULL DEFAULT 0"
        ")";
    if (!qry.exec(QLatin1String(kSlots))) {
        qWarning() << "HourClock: could not create the hourclock_slots table:"
                   << qry.lastError().text();
        return false;
    }
    if (!qry.exec(QLatin1String("CREATE INDEX IF NOT EXISTS hourclock_slots_clock "
                                "ON hourclock_slots(clock_id, position)"))) {
        qWarning() << "HourClock: could not create hourclock_slots_clock:"
                   << qry.lastError().text();
    }

    // day is the Qt weekday (Mon = 1 .. Sun = 7), hour is 0-23. One clock per
    // hour at most; an hour with no row here simply falls back to the plain
    // hour grid, which is why the feature can be switched on halfway through
    // a week without anything going quiet.
    const char *const kAssign =
        "CREATE TABLE IF NOT EXISTS hourclock_assign ("
        "  day INTEGER NOT NULL,"
        "  hour INTEGER NOT NULL,"
        "  clock_id INTEGER NOT NULL,"
        "  PRIMARY KEY (day, hour)"
        ")";
    if (!qry.exec(QLatin1String(kAssign))) {
        qWarning() << "HourClock: could not create the hourclock_assign table:"
                   << qry.lastError().text();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- settings --

HourClock::Settings HourClock::settings()
{
    Settings s;
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("HourClock"));
    s.enabled = config.value(QStringLiteral("Enabled"), s.enabled).toBool();
    s.fireHardTimed = config.value(QStringLiteral("FireHardTimed"),
                                   s.fireHardTimed).toBool();
    s.fireWindowSeconds = qBound(10, config.value(QStringLiteral("FireWindowSeconds"),
                                                  s.fireWindowSeconds).toInt(), 300);
    config.endGroup();
    return s;
}

void HourClock::saveSettings(const Settings &s)
{
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("HourClock"));
    config.setValue(QStringLiteral("Enabled"), s.enabled);
    config.setValue(QStringLiteral("FireHardTimed"), s.fireHardTimed);
    config.setValue(QStringLiteral("FireWindowSeconds"), s.fireWindowSeconds);
    config.endGroup();
    config.sync();
}

// -------------------------------------------------------------- vocabulary --

QList<HourClock::SlotType> HourClock::slotTypes()
{
    return { SlotType::MusicSweep, SlotType::AdBreak, SlotType::Jingle,
             SlotType::Program,    SlotType::News,    SlotType::StationId };
}

QString HourClock::typeKey(SlotType type)
{
    switch (type) {
    case SlotType::MusicSweep: return QStringLiteral("music");
    case SlotType::AdBreak:    return QStringLiteral("ads");
    case SlotType::Jingle:     return QStringLiteral("jingle");
    case SlotType::Program:    return QStringLiteral("program");
    case SlotType::News:       return QStringLiteral("news");
    case SlotType::StationId:  return QStringLiteral("ident");
    }
    return QStringLiteral("music");
}

HourClock::SlotType HourClock::typeFromKey(const QString &key)
{
    const QString k = key.trimmed().toLower();
    if (k == QLatin1String("ads"))     return SlotType::AdBreak;
    if (k == QLatin1String("jingle"))  return SlotType::Jingle;
    if (k == QLatin1String("program")) return SlotType::Program;
    if (k == QLatin1String("news"))    return SlotType::News;
    if (k == QLatin1String("ident"))   return SlotType::StationId;
    return SlotType::MusicSweep;
}

QString HourClock::typeLabel(SlotType type)
{
    switch (type) {
    case SlotType::MusicSweep: return tr("Music sweep");
    case SlotType::AdBreak:    return tr("Advertisement break");
    case SlotType::Jingle:     return tr("Jingle");
    case SlotType::Program:    return tr("Programme");
    case SlotType::News:       return tr("News");
    case SlotType::StationId:  return tr("Station ID");
    }
    return tr("Music sweep");
}

QString HourClock::mediaTable(SlotType type)
{
    switch (type) {
    case SlotType::MusicSweep: return QString();
    case SlotType::AdBreak:    return QStringLiteral("pub");
    case SlotType::Jingle:     return QStringLiteral("jingles");
    case SlotType::StationId:  return QStringLiteral("jingles");
    case SlotType::Program:    return QStringLiteral("programs");
    case SlotType::News:       return QStringLiteral("programs");
    }
    return QString();
}

QString HourClock::formatOffset(int seconds)
{
    const int s = qBound(0, seconds, kHourSeconds);
    return QStringLiteral("%1:%2")
        .arg(s / 60, 2, 10, QLatin1Char('0'))
        .arg(s % 60, 2, 10, QLatin1Char('0'));
}

QString HourClock::formatLength(int seconds)
{
    const int s = qMax(0, seconds);
    if (s < 60)
        return tr("%1 s").arg(s);
    if (s % 60 == 0)
        return tr("%1 min").arg(s / 60);
    return tr("%1 min %2 s").arg(s / 60).arg(s % 60);
}

QString HourClock::describeSlot(const ResolvedSlot &r)
{
    const QString name = r.slot.label.isEmpty() ? typeLabel(r.slot.type)
                                                : r.slot.label;
    QString line = tr("%1 at %2, %3, %4")
                       .arg(name,
                            formatOffset(r.start),
                            formatLength(r.length),
                            r.slot.hardTimed ? tr("hard-timed") : tr("floating"));
    if (!r.slot.reference.isEmpty()) {
        line += QLatin1Char(' ');
        line += r.slot.isMusic() ? tr("genre %1").arg(r.slot.reference)
                                 : tr("item %1").arg(r.slot.reference);
    }
    if (r.overruns)
        line += QLatin1Char(' ') + tr("(runs past the next fixed item)");
    return line;
}

QString HourClock::describeTotal(const Timeline &t)
{
    if (t.isOverfull())
        return tr("The hour is over-full by %1. Nothing has been shortened — "
                  "shorten something yourself, or the fixed items will be "
                  "run into.").arg(formatLength(t.overfullSeconds));
    if (t.isUnderfilled())
        return tr("The hour is under-filled by %1 that no floating item can "
                  "absorb.").arg(formatLength(t.underfilledSeconds));
    return tr("The hour is exactly full.");
}

// ---------------------------------------------------------------- Timeline --

int HourClock::Timeline::indexAt(int second) const
{
    for (int i = 0; i < items.size(); ++i) {
        if (second >= items.at(i).start && second < items.at(i).end())
            return i;
    }
    return -1;
}

int HourClock::Timeline::musicIndexFor(int second) const
{
    const int here = indexAt(second);
    if (here >= 0 && items.at(here).slot.isMusic())
        return here;
    // Not inside a sweep (an ad break, or a gap): the next one is what will be
    // needed in a moment, and that is the sensible thing to fill towards.
    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i).slot.isMusic() && items.at(i).start >= second)
            return i;
    }
    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i).slot.isMusic())
            return i;
    }
    return -1;
}

// ------------------------------------------------------------ the arithmetic --

void HourClock::normalise(Clock &clock)
{
    std::stable_sort(clock.items.begin(), clock.items.end(),
                     [](const Slot &a, const Slot &b) {
                         return a.position < b.position;
                     });
    for (int i = 0; i < clock.items.size(); ++i)
        clock.items[i].position = i;
}

HourClock::Timeline HourClock::resolve(const Clock &clock)
{
    Timeline out;

    Clock ordered = clock;
    normalise(ordered);

    // One run of floating slots, laid out between `from` and `to`.
    //
    //  - room to spare goes to the music sweeps, in proportion to what they
    //    already are, because a sweep is the only kind of item whose length is
    //    genuinely elastic. A jingle is as long as the jingle is.
    //  - not enough room means the hour cannot be played as written. Nothing
    //    is shortened: the slots keep their nominal lengths, they are flagged,
    //    and the shortfall is added up for the operator.
    auto layoutFloats = [&out](QList<Slot> &pending, int from, int to) {
        if (pending.isEmpty()) {
            if (to > from)
                out.underfilledSeconds += to - from;
            pending.clear();
            return;
        }

        int nominal = 0;
        for (const Slot &s : pending)
            nominal += qMax(0, s.lengthSeconds);
        const int available = to - from;

        QList<int> lengths;
        lengths.reserve(pending.size());
        for (const Slot &s : pending)
            lengths.append(qMax(0, s.lengthSeconds));

        bool overrun = false;
        if (available >= nominal) {
            int slack = available - nominal;
            if (slack > 0) {
                // Prefer the music sweeps. Proportional so that a run with a
                // long sweep and a short one keeps their relative shape.
                QList<int> takers;
                int takerTotal = 0;
                for (int i = 0; i < pending.size(); ++i) {
                    if (pending.at(i).isMusic()) {
                        takers.append(i);
                        takerTotal += lengths.at(i);
                    }
                }
                if (takers.isEmpty()) {
                    // No sweep in this run: the last floating item takes it,
                    // rather than leaving a hole nobody accounted for.
                    takers.append(pending.size() - 1);
                    takerTotal = lengths.at(pending.size() - 1);
                }
                int handed = 0;
                for (int n = 0; n < takers.size(); ++n) {
                    const int i = takers.at(n);
                    int share = 0;
                    if (n == takers.size() - 1) {
                        share = slack - handed;          // remainder, no rounding loss
                    } else if (takerTotal > 0) {
                        share = static_cast<int>(
                            static_cast<qint64>(slack) * lengths.at(i) / takerTotal);
                    } else {
                        share = slack / takers.size();
                    }
                    lengths[i] += share;
                    handed += share;
                }
            }
        } else {
            out.overfullSeconds += nominal - available;
            overrun = true;
        }

        int cursor = from;
        for (int i = 0; i < pending.size(); ++i) {
            ResolvedSlot r;
            r.slot   = pending.at(i);
            r.start  = cursor;
            r.length = lengths.at(i);
            r.slackSeconds = lengths.at(i) - qMax(0, pending.at(i).lengthSeconds);
            r.overruns = overrun && (cursor + lengths.at(i) > to);
            out.items.append(r);
            cursor += lengths.at(i);
        }
        pending.clear();
    };

    QList<Slot> pending;
    int cursor = 0;

    for (const Slot &s : std::as_const(ordered.items)) {
        if (!s.hardTimed) {
            pending.append(s);
            continue;
        }

        const int anchor = qBound(0, s.startSeconds, kHourSeconds);
        // Everything floating since the last anchor has until this one.
        layoutFloats(pending, cursor, qMax(cursor, anchor));
        if (anchor < cursor) {
            // The floating run already ate into this anchor's airtime. The
            // anchor does not move — that is what hard-timed means — so the
            // conflict is counted and said out loud.
            out.overfullSeconds += cursor - anchor;
            if (!out.items.isEmpty())
                out.items.last().overruns = true;
            out.warnings.append(
                tr("%1 is fixed at %2 but the items before it need %3 longer.")
                    .arg(s.label.isEmpty() ? typeLabel(s.type) : s.label,
                         formatOffset(anchor),
                         formatLength(cursor - anchor)));
        }

        ResolvedSlot r;
        r.slot   = s;
        r.start  = anchor;                       // never moved. ever.
        r.length = qMax(0, s.lengthSeconds);
        out.items.append(r);
        cursor = anchor + r.length;
    }

    layoutFloats(pending, cursor, qMax(cursor, kHourSeconds));
    if (cursor > kHourSeconds) {
        out.overfullSeconds += cursor - kHourSeconds;
        if (!out.items.isEmpty())
            out.items.last().overruns = true;
    }

    if (out.isOverfull()) {
        out.warnings.append(tr("The hour is over-full by %1.")
                                .arg(formatLength(out.overfullSeconds)));
    } else if (out.isUnderfilled()) {
        out.warnings.append(tr("%1 of the hour is not covered by any item.")
                                .arg(formatLength(out.underfilledSeconds)));
    }
    return out;
}

// ------------------------------------------------------------- persistence --

QList<HourClock::ClockInfo> HourClock::clockList()
{
    QList<ClockInfo> out;
    QSqlDatabase database = db();
    if (!database.isOpen())
        return out;
    QSqlQuery qry(database);
    if (!qry.exec(QStringLiteral(
            "select c.id, c.name, (select count(*) from hourclock_slots s"
            "  where s.clock_id = c.id)"
            " from hourclocks c order by c.name collate nocase"))) {
        qWarning() << "HourClock: could not list the clocks:" << qry.lastError().text();
        return out;
    }
    while (qry.next()) {
        ClockInfo info;
        info.id        = qry.value(0).toLongLong();
        info.name      = qry.value(1).toString();
        info.slotCount = qry.value(2).toInt();
        out.append(info);
    }
    return out;
}

HourClock::Clock HourClock::loadClock(qint64 id)
{
    Clock clock;
    QSqlDatabase database = db();
    if (!database.isOpen() || id <= 0)
        return clock;

    QSqlQuery head(database);
    head.prepare(QStringLiteral("select id, name, notes from hourclocks where id = :id"));
    head.bindValue(QStringLiteral(":id"), id);
    if (!head.exec() || !head.next())
        return clock;
    clock.id    = head.value(0).toLongLong();
    clock.name  = head.value(1).toString();
    clock.notes = head.value(2).toString();

    QSqlQuery slotQuery(database);
    slotQuery.prepare(QStringLiteral(
        "select id, position, type, label, reference, start_seconds,"
        " length_seconds, hard_timed from hourclock_slots"
        " where clock_id = :id order by position"));
    slotQuery.bindValue(QStringLiteral(":id"), id);
    if (slotQuery.exec()) {
        while (slotQuery.next()) {
            Slot s;
            s.id            = slotQuery.value(0).toLongLong();
            s.clockId       = clock.id;
            s.position      = slotQuery.value(1).toInt();
            s.type          = typeFromKey(slotQuery.value(2).toString());
            s.label         = slotQuery.value(3).toString();
            s.reference     = slotQuery.value(4).toString();
            s.startSeconds  = slotQuery.value(5).toInt();
            s.lengthSeconds = slotQuery.value(6).toInt();
            s.hardTimed     = slotQuery.value(7).toInt() != 0;
            clock.items.append(s);
        }
    } else {
        qWarning() << "HourClock: could not read the slots:" << slotQuery.lastError().text();
    }
    normalise(clock);
    return clock;
}

HourClock::Clock HourClock::loadClockByName(const QString &name)
{
    QSqlDatabase database = db();
    if (!database.isOpen())
        return Clock();
    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("select id from hourclocks where name = :n"));
    qry.bindValue(QStringLiteral(":n"), name);
    if (qry.exec() && qry.next())
        return loadClock(qry.value(0).toLongLong());
    return Clock();
}

bool HourClock::saveClock(Clock &clock)
{
    QSqlDatabase database = db();
    if (!database.isOpen() || clock.name.trimmed().isEmpty())
        return false;

    const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    if (clock.id <= 0) {
        QSqlQuery ins(database);
        ins.prepare(QStringLiteral("insert into hourclocks (name, notes, updated_at)"
                                   " values (:n, :o, :u)"));
        ins.bindValue(QStringLiteral(":n"), clock.name.trimmed());
        ins.bindValue(QStringLiteral(":o"), clock.notes);
        ins.bindValue(QStringLiteral(":u"), stamp);
        if (!ins.exec()) {
            qWarning() << "HourClock: could not create the clock:"
                       << ins.lastError().text();
            return false;
        }
        clock.id = ins.lastInsertId().toLongLong();
    } else {
        QSqlQuery upd(database);
        upd.prepare(QStringLiteral("update hourclocks set name = :n, notes = :o,"
                                   " updated_at = :u where id = :id"));
        upd.bindValue(QStringLiteral(":n"), clock.name.trimmed());
        upd.bindValue(QStringLiteral(":o"), clock.notes);
        upd.bindValue(QStringLiteral(":u"), stamp);
        upd.bindValue(QStringLiteral(":id"), clock.id);
        if (!upd.exec()) {
            qWarning() << "HourClock: could not update the clock:"
                       << upd.lastError().text();
            return false;
        }
    }

    // The slots are rewritten wholesale. A clock has a handful of them, and
    // "delete then insert" cannot leave a half-edited hour behind the way a
    // diff can.
    QSqlQuery del(database);
    del.prepare(QStringLiteral("delete from hourclock_slots where clock_id = :id"));
    del.bindValue(QStringLiteral(":id"), clock.id);
    if (!del.exec()) {
        qWarning() << "HourClock: could not clear the old slots:"
                   << del.lastError().text();
        return false;
    }

    normalise(clock);
    for (int i = 0; i < clock.items.size(); ++i) {
        Slot &s = clock.items[i];
        s.clockId  = clock.id;
        s.position = i;
        QSqlQuery ins(database);
        ins.prepare(QStringLiteral(
            "insert into hourclock_slots (clock_id, position, type, label,"
            " reference, start_seconds, length_seconds, hard_timed)"
            " values (:c, :p, :t, :l, :r, :s, :len, :h)"));
        ins.bindValue(QStringLiteral(":c"), clock.id);
        ins.bindValue(QStringLiteral(":p"), i);
        ins.bindValue(QStringLiteral(":t"), typeKey(s.type));
        ins.bindValue(QStringLiteral(":l"), s.label);
        ins.bindValue(QStringLiteral(":r"), s.reference);
        ins.bindValue(QStringLiteral(":s"), qBound(0, s.startSeconds, kHourSeconds));
        ins.bindValue(QStringLiteral(":len"), qMax(0, s.lengthSeconds));
        ins.bindValue(QStringLiteral(":h"), s.hardTimed ? 1 : 0);
        if (!ins.exec()) {
            qWarning() << "HourClock: could not write a slot:"
                       << ins.lastError().text();
            return false;
        }
        s.id = ins.lastInsertId().toLongLong();
    }
    return true;
}

bool HourClock::deleteClock(qint64 id)
{
    QSqlDatabase database = db();
    if (!database.isOpen() || id <= 0)
        return false;
    for (const char *const sql : {"delete from hourclock_slots where clock_id = :id",
                                  "delete from hourclock_assign where clock_id = :id",
                                  "delete from hourclocks where id = :id"}) {
        QSqlQuery qry(database);
        qry.prepare(QString::fromLatin1(sql));
        qry.bindValue(QStringLiteral(":id"), id);
        if (!qry.exec()) {
            qWarning() << "HourClock: delete failed:" << qry.lastError().text();
            return false;
        }
    }
    return true;
}

bool HourClock::renameClock(qint64 id, const QString &newName)
{
    QSqlDatabase database = db();
    if (!database.isOpen() || id <= 0 || newName.trimmed().isEmpty())
        return false;
    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("update hourclocks set name = :n where id = :id"));
    qry.bindValue(QStringLiteral(":n"), newName.trimmed());
    qry.bindValue(QStringLiteral(":id"), id);
    if (!qry.exec()) {
        qWarning() << "HourClock: rename failed:" << qry.lastError().text();
        return false;
    }
    return true;
}

qint64 HourClock::duplicateClock(qint64 id, const QString &newName)
{
    Clock source = loadClock(id);
    if (source.id <= 0)
        return -1;
    Clock copy = source;
    copy.id   = -1;
    copy.name = uniqueName(newName.trimmed().isEmpty() ? source.name : newName);
    for (Slot &s : copy.items) {
        s.id      = -1;
        s.clockId = -1;
    }
    if (!saveClock(copy))
        return -1;
    return copy.id;
}

QString HourClock::uniqueName(const QString &wanted)
{
    const QString base = wanted.trimmed().isEmpty() ? tr("Clock") : wanted.trimmed();
    QStringList taken;
    for (const ClockInfo &info : clockList())
        taken << info.name.toCaseFolded();
    if (!taken.contains(base.toCaseFolded()))
        return base;
    for (int n = 2; n < 1000; ++n) {
        const QString candidate = QStringLiteral("%1 %2").arg(base).arg(n);
        if (!taken.contains(candidate.toCaseFolded()))
            return candidate;
    }
    return base + QStringLiteral(" ") + QString::number(QDateTime::currentSecsSinceEpoch());
}

HourClock::Clock HourClock::exampleClock(const QString &name)
{
    Clock clock;
    clock.name  = name;
    clock.notes = tr("A starting point: news on the hour, sweeps either side of "
                     "a jingle, and the ad break at twenty past.");

    auto add = [&clock](SlotType type, const QString &label, const QString &reference,
                        int start, int length, bool hard) {
        Slot s;
        s.position      = clock.items.size();
        s.type          = type;
        s.label         = label;
        s.reference     = reference;
        s.startSeconds  = start;
        s.lengthSeconds = length;
        s.hardTimed     = hard;
        clock.items.append(s);
    };

    add(SlotType::News,       tr("News"),          QString(),   0,    180,  true);
    add(SlotType::StationId,  tr("Station ID"),    QString(),   180,  10,   false);
    add(SlotType::MusicSweep, tr("Morning sweep"), QString(),   190,  1010, false);
    add(SlotType::AdBreak,    tr("Ad break"),      QString(),   1200, 120,  true);
    add(SlotType::Jingle,     tr("Jingle"),        QString(),   1320, 15,   false);
    add(SlotType::MusicSweep, tr("Second sweep"),  QString(),   1335, 1465, false);
    add(SlotType::AdBreak,    tr("Ad break"),      QString(),   2800, 120,  true);
    add(SlotType::MusicSweep, tr("Run to the hour"), QString(), 2920, 680,  false);
    return clock;
}

// -------------------------------------------------------------- assignments --

bool HourClock::assignClock(int day, int hour, qint64 clockId)
{
    QSqlDatabase database = db();
    if (!database.isOpen() || day < 1 || day > 7 || hour < 0 || hour > 23)
        return false;
    if (clockId <= 0)
        return clearAssignment(day, hour);
    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("insert or replace into hourclock_assign"
                               " (day, hour, clock_id) values (:d, :h, :c)"));
    qry.bindValue(QStringLiteral(":d"), day);
    qry.bindValue(QStringLiteral(":h"), hour);
    qry.bindValue(QStringLiteral(":c"), clockId);
    if (!qry.exec()) {
        qWarning() << "HourClock: could not assign the clock:" << qry.lastError().text();
        return false;
    }
    return true;
}

bool HourClock::clearAssignment(int day, int hour)
{
    QSqlDatabase database = db();
    if (!database.isOpen())
        return false;
    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("delete from hourclock_assign"
                               " where day = :d and hour = :h"));
    qry.bindValue(QStringLiteral(":d"), day);
    qry.bindValue(QStringLiteral(":h"), hour);
    if (!qry.exec()) {
        qWarning() << "HourClock: could not clear the assignment:"
                   << qry.lastError().text();
        return false;
    }
    return true;
}

qint64 HourClock::clockIdFor(int day, int hour)
{
    QSqlDatabase database = db();
    if (!database.isOpen())
        return -1;
    QSqlQuery qry(database);
    qry.prepare(QStringLiteral("select clock_id from hourclock_assign"
                               " where day = :d and hour = :h"));
    qry.bindValue(QStringLiteral(":d"), day);
    qry.bindValue(QStringLiteral(":h"), hour);
    if (qry.exec() && qry.next())
        return qry.value(0).toLongLong();
    return -1;
}

QHash<int, qint64> HourClock::assignments()
{
    QHash<int, qint64> out;
    QSqlDatabase database = db();
    if (!database.isOpen())
        return out;
    QSqlQuery qry(database);
    if (!qry.exec(QStringLiteral("select day, hour, clock_id from hourclock_assign")))
        return out;
    while (qry.next()) {
        const int day  = qry.value(0).toInt();
        const int hour = qry.value(1).toInt();
        if (day < 1 || day > 7 || hour < 0 || hour > 23)
            continue;
        out.insert(day * 24 + hour, qry.value(2).toLongLong());
    }
    return out;
}

bool HourClock::assignWholeDay(int day, qint64 clockId)
{
    bool ok = true;
    for (int hour = 0; hour < 24; ++hour)
        ok = assignClock(day, hour, clockId) && ok;
    return ok;
}

// ------------------------------------------------------------------- media --

QString HourClock::mediaPathFor(const Slot &slot)
{
    const QString table = mediaTable(slot.type);
    if (table.isEmpty())
        return QString();
    QSqlDatabase database = db();
    if (!database.isOpen())
        return QString();

    if (!slot.reference.trimmed().isEmpty()) {
        QSqlQuery named(database);
        named.prepare(QStringLiteral("select path from %1 where name like :n"
                                     " order by random() limit 1").arg(table));
        named.bindValue(QStringLiteral(":n"), slot.reference.trimmed());
        if (named.exec() && named.next()) {
            const QString path = named.value(0).toString();
            if (!path.isEmpty())
                return path;
        }
    }

    // No name, or a name that matches nothing: any row of the right table is a
    // better answer than silence where a jingle was programmed.
    QSqlQuery any(database);
    if (any.exec(QStringLiteral("select path from %1 order by random() limit 1")
                     .arg(table))
        && any.next()) {
        return any.value(0).toString();
    }
    return QString();
}

// --------------------------------------------------- what Auto Mode asks --

QString HourClock::genreForMoment(int day, int hour, int second)
{
    const Settings s = settings();
    if (!s.enabled)
        return QString();
    const qint64 clockId = clockIdFor(day, hour);
    if (clockId <= 0)
        return QString();
    const Clock clock = loadClock(clockId);
    if (clock.items.isEmpty())
        return QString();
    const Timeline timeline = resolve(clock);
    const int index = timeline.musicIndexFor(qBound(0, second, kHourSeconds - 1));
    if (index < 0)
        return QString();
    return timeline.items.at(index).slot.reference.trimmed();
}
