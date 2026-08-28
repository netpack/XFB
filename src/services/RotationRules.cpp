#include "RotationRules.h"

#include <QCoreApplication>
#include <QDebug>
#include <QRandomGenerator>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QVariant>

#include <algorithm>

#include "AirLog.h"

namespace {

const char *const kConnection = "xfb_connection";

QSqlDatabase rotationDb()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

/** The same xfb.conf every other XFB setting lives in. */
QString settingsFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/** The three stamps XFB has written into musics.last_played over the years. */
QDateTime parseLastPlayed(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed == QLatin1String("-"))
        return QDateTime();
    // The current writer: "yyyy-MM-dd || hh:mm:ss" (player.cpp).
    QDateTime dt = QDateTime::fromString(trimmed,
                                         QStringLiteral("yyyy-MM-dd || hh:mm:ss"));
    if (!dt.isValid())
        dt = QDateTime::fromString(trimmed, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    if (!dt.isValid())
        dt = QDateTime::fromString(trimmed, Qt::ISODate);
    return dt;
}

/** yyyy-MM-dd, or an empty string for "no limit". */
QString dateToText(const QDate &date)
{
    return date.isValid() ? date.toString(QStringLiteral("yyyy-MM-dd")) : QString();
}

QDate dateFromText(const QString &text)
{
    if (text.trimmed().isEmpty())
        return QDate();
    return QDate::fromString(text.trimmed(), QStringLiteral("yyyy-MM-dd"));
}

/** How many "why not" lines one pick keeps. A person reads these. */
constexpr int kMaxRejections = 12;

} // namespace

RotationRules::RotationRules(QObject *parent)
    : QObject(parent)
{
}

RotationRules *RotationRules::instance()
{
    static RotationRules *self = new RotationRules(qApp);
    return self;
}

QString RotationRules::fold(const QString &text)
{
    return text.simplified().toCaseFolded();
}

// ---------------------------------------------------------------- schema ---

bool RotationRules::ensureSchema(QSqlDatabase db)
{
    if (!db.isOpen())
        return false;

    QSqlQuery qry(db);
    // Keyed to musics.id, one row per track that has been given rules at all.
    // A track with no row is not "unprogrammed" — it is on the station's
    // defaults, which is the only sane starting point for a library of
    // thousands that nobody is going to categorise by hand on day one.
    const char *const kCreate =
        "CREATE TABLE IF NOT EXISTS rotation ("
        "  music_id INTEGER PRIMARY KEY,"
        "  category TEXT NOT NULL DEFAULT 'secondary',"
        "  hour_mask INTEGER NOT NULL DEFAULT 16777215,"  // 24 bits set
        "  day_mask INTEGER NOT NULL DEFAULT 127,"        // 7 bits set
        "  start_date TEXT,"
        "  end_date TEXT,"
        "  weight REAL,"
        "  updated_at TEXT"
        ")";
    if (!qry.exec(QLatin1String(kCreate))) {
        qWarning() << "RotationRules: could not create the rotation table:"
                   << qry.lastError().text();
        return false;
    }
    if (!qry.exec(QLatin1String("CREATE INDEX IF NOT EXISTS rotation_category "
                                "ON rotation(category)"))) {
        qWarning() << "RotationRules: could not create rotation_category:"
                   << qry.lastError().text();
        return false;
    }
    return true;
}

// -------------------------------------------------------------- settings ---

RotationRules::Settings RotationRules::settings()
{
    Settings s;
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("Rotation"));
    s.enabled = config.value(QStringLiteral("Enabled"), s.enabled).toBool();
    s.artistSeparationMinutes =
        qBound(0, config.value(QStringLiteral("ArtistSeparationMinutes"),
                               s.artistSeparationMinutes).toInt(), 24 * 60);
    s.titleSeparationHours =
        qBound(0, config.value(QStringLiteral("TitleSeparationHours"),
                               s.titleSeparationHours).toInt(), 24 * 30);
    s.powerWeight     = qBound(0.0, config.value(QStringLiteral("PowerWeight"),
                                                 s.powerWeight).toDouble(), 1000.0);
    s.secondaryWeight = qBound(0.0, config.value(QStringLiteral("SecondaryWeight"),
                                                 s.secondaryWeight).toDouble(), 1000.0);
    s.goldWeight      = qBound(0.0, config.value(QStringLiteral("GoldWeight"),
                                                 s.goldWeight).toDouble(), 1000.0);
    s.defaultCategory = config.value(QStringLiteral("DefaultCategory"),
                                     s.defaultCategory).toString().toLower();
    if (!categoryKeys().contains(s.defaultCategory))
        s.defaultCategory = QStringLiteral("secondary");
    s.candidateLimit = qBound(20, config.value(QStringLiteral("CandidateLimit"),
                                               s.candidateLimit).toInt(), 5000);
    config.endGroup();
    return s;
}

void RotationRules::saveSettings(const Settings &s)
{
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("Rotation"));
    config.setValue(QStringLiteral("Enabled"), s.enabled);
    config.setValue(QStringLiteral("ArtistSeparationMinutes"), s.artistSeparationMinutes);
    config.setValue(QStringLiteral("TitleSeparationHours"), s.titleSeparationHours);
    config.setValue(QStringLiteral("PowerWeight"), s.powerWeight);
    config.setValue(QStringLiteral("SecondaryWeight"), s.secondaryWeight);
    config.setValue(QStringLiteral("GoldWeight"), s.goldWeight);
    config.setValue(QStringLiteral("DefaultCategory"), s.defaultCategory);
    config.setValue(QStringLiteral("CandidateLimit"), s.candidateLimit);
    config.endGroup();
    config.sync();
}

QStringList RotationRules::categoryKeys()
{
    return { QStringLiteral("power"), QStringLiteral("secondary"),
             QStringLiteral("gold") };
}

QString RotationRules::categoryLabel(const QString &key)
{
    const QString k = key.toLower();
    if (k == QLatin1String("power"))
        return tr("Power");
    if (k == QLatin1String("gold"))
        return tr("Gold");
    if (k == QLatin1String("secondary"))
        return tr("Secondary");
    return key;
}

double RotationRules::categoryWeight(const QString &key, const Settings &s)
{
    const QString k = key.toLower();
    if (k == QLatin1String("power"))
        return s.powerWeight;
    if (k == QLatin1String("gold"))
        return s.goldWeight;
    return s.secondaryWeight;
}

QString RotationRules::ruleLabel(int relaxationStep)
{
    switch (relaxationStep) {
    case ArtistHalved:      return tr("artist separation halved");
    case ArtistDropped:     return tr("artist separation dropped");
    case TitleHalved:       return tr("title separation halved");
    case TitleDropped:      return tr("title separation dropped");
    case DaypartDropped:    return tr("daypart restrictions dropped");
    case DateWindowDropped: return tr("date windows dropped");
    default:                return QString();
    }
}

QStringList RotationRules::relaxedRuleNames(int relaxation)
{
    QStringList names;
    for (int step = ArtistHalved; step <= relaxation && step <= kMaxRelaxation; ++step) {
        // A halving is superseded by the drop that follows it: saying both
        // "halved" and "dropped" about the same rule reads as two failures.
        if (step == ArtistHalved && relaxation >= ArtistDropped)
            continue;
        if (step == TitleHalved && relaxation >= TitleDropped)
            continue;
        names << ruleLabel(step);
    }
    return names;
}

// ----------------------------------------------------------------- rules ---

RotationRules::Rule RotationRules::defaultRule(qint64 musicId, const Settings &s)
{
    Rule r;
    r.musicId  = musicId;
    r.category = s.defaultCategory;
    return r;
}

RotationRules::Rule RotationRules::ruleFor(qint64 musicId)
{
    const QHash<qint64, Rule> found = rulesFor({ musicId });
    if (found.contains(musicId))
        return found.value(musicId);
    return defaultRule(musicId, settings());
}

QHash<qint64, RotationRules::Rule> RotationRules::rulesFor(const QList<qint64> &musicIds)
{
    QHash<qint64, Rule> out;
    if (musicIds.isEmpty())
        return out;
    QSqlDatabase db = rotationDb();
    if (!db.isOpen())
        return out;

    // One statement for the whole pool. Chunked because SQLite's variable
    // limit is 999 by default and a candidate list can be larger.
    const int kChunk = 400;
    for (int start = 0; start < musicIds.size(); start += kChunk) {
        const int count = qMin(kChunk, musicIds.size() - start);
        QStringList placeholders;
        for (int i = 0; i < count; ++i)
            placeholders << QStringLiteral(":i%1").arg(i);
        QSqlQuery qry(db);
        qry.prepare(QStringLiteral(
            "SELECT music_id, category, hour_mask, day_mask, start_date, end_date, weight"
            " FROM rotation WHERE music_id IN (%1)")
                        .arg(placeholders.join(QStringLiteral(", "))));
        for (int i = 0; i < count; ++i)
            qry.bindValue(QStringLiteral(":i%1").arg(i), musicIds.at(start + i));
        if (!qry.exec()) {
            qWarning() << "RotationRules: could not read rules:" << qry.lastError().text();
            return out;
        }
        while (qry.next()) {
            Rule r;
            r.musicId  = qry.value(0).toLongLong();
            r.category = qry.value(1).toString().toLower();
            r.hourMask = static_cast<quint32>(qry.value(2).toUInt()) & 0xFFFFFFu;
            r.dayMask  = static_cast<quint8>(qry.value(3).toUInt()) & 0x7F;
            r.startDate = dateFromText(qry.value(4).toString());
            r.endDate   = dateFromText(qry.value(5).toString());
            r.weight    = qry.value(6).isNull() ? -1.0 : qry.value(6).toDouble();
            r.stored    = true;
            // A row that manages to store "no hour at all" would silence the
            // track for ever, which nobody means. Read it as "any hour".
            if ((r.hourMask & 0xFFFFFFu) == 0)
                r.hourMask = 0xFFFFFFu;
            if ((r.dayMask & 0x7F) == 0)
                r.dayMask = 0x7F;
            out.insert(r.musicId, r);
        }
    }
    return out;
}

bool RotationRules::saveRule(const Rule &r)
{
    if (r.musicId < 0)
        return false;
    QSqlDatabase db = rotationDb();
    if (!db.isOpen())
        return false;

    QSqlQuery qry(db);
    qry.prepare(QStringLiteral(
        "INSERT INTO rotation (music_id, category, hour_mask, day_mask,"
        " start_date, end_date, weight, updated_at)"
        " VALUES (:id, :category, :hours, :days, :start, :end, :weight, :updated)"
        " ON CONFLICT(music_id) DO UPDATE SET"
        " category = excluded.category, hour_mask = excluded.hour_mask,"
        " day_mask = excluded.day_mask, start_date = excluded.start_date,"
        " end_date = excluded.end_date, weight = excluded.weight,"
        " updated_at = excluded.updated_at"));
    qry.bindValue(QStringLiteral(":id"), r.musicId);
    qry.bindValue(QStringLiteral(":category"),
                  categoryKeys().contains(r.category.toLower())
                      ? r.category.toLower() : QStringLiteral("secondary"));
    qry.bindValue(QStringLiteral(":hours"),
                  static_cast<uint>((r.hourMask & 0xFFFFFFu) ? (r.hourMask & 0xFFFFFFu)
                                                             : 0xFFFFFFu));
    qry.bindValue(QStringLiteral(":days"),
                  static_cast<uint>((r.dayMask & 0x7F) ? (r.dayMask & 0x7F) : 0x7F));
    qry.bindValue(QStringLiteral(":start"),
                  r.startDate.isValid() ? QVariant(dateToText(r.startDate))
                                        : QVariant(QMetaType(QMetaType::QString)));
    qry.bindValue(QStringLiteral(":end"),
                  r.endDate.isValid() ? QVariant(dateToText(r.endDate))
                                      : QVariant(QMetaType(QMetaType::QString)));
    qry.bindValue(QStringLiteral(":weight"),
                  r.weight >= 0.0 ? QVariant(r.weight)
                                  : QVariant(QMetaType(QMetaType::Double)));
    qry.bindValue(QStringLiteral(":updated"),
                  QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!qry.exec()) {
        qWarning() << "RotationRules: could not save the rule for" << r.musicId
                   << ":" << qry.lastError().text();
        return false;
    }
    return true;
}

bool RotationRules::clearRule(qint64 musicId)
{
    QSqlDatabase db = rotationDb();
    if (!db.isOpen())
        return false;
    QSqlQuery qry(db);
    qry.prepare(QStringLiteral("DELETE FROM rotation WHERE music_id = :id"));
    qry.bindValue(QStringLiteral(":id"), musicId);
    if (!qry.exec()) {
        qWarning() << "RotationRules: could not clear the rule for" << musicId
                   << ":" << qry.lastError().text();
        return false;
    }
    return true;
}

int RotationRules::ruleCount()
{
    QSqlDatabase db = rotationDb();
    if (!db.isOpen())
        return 0;
    QSqlQuery qry(db);
    if (qry.exec(QStringLiteral("SELECT count(*) FROM rotation")) && qry.next())
        return qry.value(0).toInt();
    return 0;
}

quint32 RotationRules::hoursFromList(const QList<int> &hours)
{
    quint32 mask = 0;
    for (int h : hours) {
        if (h >= 0 && h < 24)
            mask |= (1u << h);
    }
    return mask ? mask : 0xFFFFFFu;
}

QList<int> RotationRules::hoursFromMask(quint32 mask)
{
    QList<int> hours;
    for (int h = 0; h < 24; ++h) {
        if (mask & (1u << h))
            hours << h;
    }
    return hours;
}

QString RotationRules::describeHours(quint32 mask)
{
    mask &= 0xFFFFFFu;
    if (mask == 0xFFFFFFu || mask == 0)
        return tr("any hour");
    const QList<int> hours = hoursFromMask(mask);
    // A contiguous run reads as a daypart; anything else is just a count.
    bool contiguous = true;
    for (int i = 1; i < hours.size(); ++i) {
        if (hours.at(i) != hours.at(i - 1) + 1) {
            contiguous = false;
            break;
        }
    }
    if (contiguous && hours.size() > 1) {
        return tr("%1:00-%2:59")
            .arg(hours.first(), 2, 10, QLatin1Char('0'))
            .arg(hours.last(), 2, 10, QLatin1Char('0'));
    }
    if (hours.size() == 1)
        return tr("%1:00 only").arg(hours.first(), 2, 10, QLatin1Char('0'));
    return tr("%n hour(s)", nullptr, static_cast<int>(hours.size()));
}

QString RotationRules::describeDays(quint8 mask)
{
    mask &= 0x7F;
    if (mask == 0x7F || mask == 0)
        return tr("every day");
    static const char *const kShort[7] = { QT_TR_NOOP("Mon"), QT_TR_NOOP("Tue"),
                                           QT_TR_NOOP("Wed"), QT_TR_NOOP("Thu"),
                                           QT_TR_NOOP("Fri"), QT_TR_NOOP("Sat"),
                                           QT_TR_NOOP("Sun") };
    QStringList parts;
    for (int d = 0; d < 7; ++d) {
        if (mask & (1u << d))
            parts << tr(kShort[d]);
    }
    return parts.join(QStringLiteral(" "));
}

QString RotationRules::describeDates(const QDate &from, const QDate &to)
{
    const QString fmt = QStringLiteral("yyyy-MM-dd");
    if (!from.isValid() && !to.isValid())
        return tr("always");
    if (from.isValid() && to.isValid())
        return tr("%1 to %2").arg(from.toString(fmt), to.toString(fmt));
    if (from.isValid())
        return tr("from %1").arg(from.toString(fmt));
    return tr("until %1").arg(to.toString(fmt));
}

// --------------------------------------------------------------- history ---

RotationRules::History RotationRules::recentHistory(const Settings &s,
                                                    const QDateTime &now)
{
    History history;
    QSqlDatabase db = rotationDb();
    if (!db.isOpen())
        return history;

    // One window wide enough for both rules; each rule applies its own cut.
    const qint64 windowSecs =
        qMax<qint64>(qint64(s.artistSeparationMinutes) * 60,
                     qint64(s.titleSeparationHours) * 3600);
    if (windowSecs <= 0)
        return history;
    const QDateTime cutoff = now.addSecs(-windowSecs);

    // The as-run log is what actually went to air, which is the only honest
    // answer to "have we played this lately". Anything else is a guess.
    bool haveLog = false;
    if (db.tables().contains(QStringLiteral("airlog"))) {
        QSqlQuery any(db);
        if (any.exec(QStringLiteral("SELECT 1 FROM airlog LIMIT 1")) && any.next())
            haveLog = true;
    }

    if (haveLog) {
        QSqlQuery qry(db);
        // Music only: a jingle bed by the same artist is not a rotation clash,
        // and the advertisements have nothing to do with this at all.
        qry.prepare(QStringLiteral(
            "SELECT artist, title, started_at FROM airlog"
            " WHERE source = 'musics' AND started_at >= :cutoff"
            " ORDER BY started_at ASC"));
        qry.bindValue(QStringLiteral(":cutoff"), AirLog::formatTimestamp(cutoff));
        if (qry.exec()) {
            history.fromAirLog = true;
            history.usable = true;
            while (qry.next()) {
                const QDateTime when = AirLog::parseTimestamp(qry.value(2).toString());
                if (!when.isValid())
                    continue;
                const QString artist = fold(qry.value(0).toString());
                const QString title  = fold(qry.value(1).toString());
                // Rows arrive oldest first, so the last write wins and is
                // the most recent airing of that artist or title.
                if (!artist.isEmpty())
                    history.lastArtist.insert(artist, when);
                if (!title.isEmpty())
                    history.lastTitle.insert(title, when);
            }
            return history;
        }
        qWarning() << "RotationRules: could not read the as-run log:"
                   << qry.lastError().text();
    }

    // Fresh install: the log has never recorded anything, so the only record
    // of what played is the counter the library keeps. It is per-track and it
    // is only ever the *last* play, which is exactly enough for these two
    // rules and no more.
    QSqlQuery fallback(db);
    fallback.prepare(QStringLiteral(
        "SELECT artist, song, last_played FROM musics"
        " WHERE last_played IS NOT NULL AND last_played <> '' AND last_played <> '-'"));
    if (!fallback.exec()) {
        qWarning() << "RotationRules: could not read last_played:"
                   << fallback.lastError().text();
        return history;
    }
    history.fromAirLog = false;
    history.usable = true;
    while (fallback.next()) {
        const QDateTime when = parseLastPlayed(fallback.value(2).toString());
        if (!when.isValid() || when < cutoff)
            continue;
        const QString artist = fold(fallback.value(0).toString());
        const QString title  = fold(fallback.value(1).toString());
        const auto keepLater = [&when](QHash<QString, QDateTime> &map, const QString &key) {
            if (key.isEmpty())
                return;
            const auto it = map.constFind(key);
            if (it == map.constEnd() || it.value() < when)
                map.insert(key, when);
        };
        keepLater(history.lastArtist, artist);
        keepLater(history.lastTitle, title);
    }
    return history;
}

// --------------------------------------------------------------- picking ---

QList<QPair<RotationRules::Candidate, double>>
RotationRules::survivors(const QList<Candidate> &candidates,
                         const QHash<qint64, Rule> &rules,
                         const History &history,
                         const Settings &s,
                         int relaxation,
                         const QDateTime &now,
                         QStringList *rejections)
{
    QList<QPair<Candidate, double>> pool;
    if (candidates.isEmpty())
        return pool;

    const QDate today = now.date();
    const int hour = now.time().hour();
    const int dayBit = qBound(1, now.date().dayOfWeek(), 7) - 1;

    // What each rule's window is at this rung of the ladder. A halved window
    // is still a window: it is the difference between "not this half hour"
    // and "we gave up on separation", and the log says which happened.
    const bool checkArtist = relaxation < ArtistDropped;
    const bool checkTitle  = relaxation < TitleDropped;
    const bool checkDaypart = relaxation < DaypartDropped;
    const bool checkDates   = relaxation < DateWindowDropped;
    const qint64 artistSecs =
        qint64(s.artistSeparationMinutes) * 60 / (relaxation >= ArtistHalved ? 2 : 1);
    const qint64 titleSecs =
        qint64(s.titleSeparationHours) * 3600 / (relaxation >= TitleHalved ? 2 : 1);

    for (const Candidate &c : candidates) {
        const Rule rule = rules.contains(c.musicId)
                              ? rules.value(c.musicId)
                              : defaultRule(c.musicId, s);

        if (checkDates) {
            if (rule.startDate.isValid() && today < rule.startDate) {
                if (rejections && rejections->size() < kMaxRejections) {
                    *rejections << tr("%1 — not in rotation until %2")
                                       .arg(c.title.isEmpty() ? c.path : c.title,
                                            rule.startDate.toString(QStringLiteral("yyyy-MM-dd")));
                }
                continue;
            }
            if (rule.endDate.isValid() && today > rule.endDate) {
                if (rejections && rejections->size() < kMaxRejections) {
                    *rejections << tr("%1 — expired on %2")
                                       .arg(c.title.isEmpty() ? c.path : c.title,
                                            rule.endDate.toString(QStringLiteral("yyyy-MM-dd")));
                }
                continue;
            }
        }

        if (checkDaypart) {
            if (!(rule.hourMask & (1u << hour))) {
                if (rejections && rejections->size() < kMaxRejections) {
                    *rejections << tr("%1 — not scheduled for this hour")
                                       .arg(c.title.isEmpty() ? c.path : c.title);
                }
                continue;
            }
            if (!(rule.dayMask & (1u << dayBit))) {
                if (rejections && rejections->size() < kMaxRejections) {
                    *rejections << tr("%1 — not scheduled for this weekday")
                                       .arg(c.title.isEmpty() ? c.path : c.title);
                }
                continue;
            }
        }

        if (checkTitle && titleSecs > 0 && !c.title.isEmpty()) {
            const QDateTime last = history.lastTitle.value(fold(c.title));
            if (last.isValid() && last.secsTo(now) < titleSecs) {
                if (rejections && rejections->size() < kMaxRejections) {
                    *rejections << tr("%1 — same title %n minute(s) ago", nullptr,
                                      static_cast<int>(last.secsTo(now) / 60))
                                       .arg(c.title);
                }
                continue;
            }
        }

        if (checkArtist && artistSecs > 0 && !c.artist.isEmpty()) {
            const QDateTime last = history.lastArtist.value(fold(c.artist));
            if (last.isValid() && last.secsTo(now) < artistSecs) {
                if (rejections && rejections->size() < kMaxRejections) {
                    *rejections << tr("%1 — same artist %n minute(s) ago", nullptr,
                                      static_cast<int>(last.secsTo(now) / 60))
                                       .arg(c.artist);
                }
                continue;
            }
        }

        const double weight = rule.weight >= 0.0 ? rule.weight
                                                 : categoryWeight(rule.category, s);
        pool.append(qMakePair(c, qMax(0.0, weight)));
    }
    return pool;
}

RotationRules::Candidate
RotationRules::weightedPick(const QList<QPair<Candidate, double>> &pool,
                            double *chosenWeight)
{
    if (pool.isEmpty())
        return Candidate();

    double total = 0.0;
    for (const auto &entry : pool)
        total += entry.second;

    // Every survivor rested to nothing: a category weight of zero is a
    // preference, not a ban, and "nothing to play" is never the better answer.
    if (total <= 0.0) {
        const int index = QRandomGenerator::global()->bounded(pool.size());
        if (chosenWeight)
            *chosenWeight = 0.0;
        return pool.at(index).first;
    }

    const double roll = QRandomGenerator::global()->generateDouble() * total;
    double running = 0.0;
    for (const auto &entry : pool) {
        running += entry.second;
        if (roll < running) {
            if (chosenWeight)
                *chosenWeight = entry.second;
            return entry.first;
        }
    }
    if (chosenWeight)
        *chosenWeight = pool.last().second;
    return pool.last().first;   // rounding only
}

RotationRules::Selection
RotationRules::select(const std::function<QList<Candidate>(int pass)> &candidatesForPass,
                      int passCount,
                      const QStringList &passNames,
                      const QDateTime &now)
{
    Selection selection;
    if (passCount <= 0 || !candidatesForPass)
        return selection;

    const Settings s = settings();

    // Every pass is asked at most once however far down the ladder we go.
    QList<QList<Candidate>> cache;
    QList<bool> fetched;
    for (int i = 0; i < passCount; ++i) {
        cache.append(QList<Candidate>());
        fetched.append(false);
    }
    auto poolFor = [&](int pass) -> const QList<Candidate> & {
        if (!fetched[pass]) {
            cache[pass] = candidatesForPass(pass);
            fetched[pass] = true;
        }
        return cache[pass];
    };

    // Rotation switched off: the picker keeps its old behaviour exactly —
    // first pass that has anything, chosen uniformly.
    if (!s.enabled) {
        for (int pass = 0; pass < passCount; ++pass) {
            const QList<Candidate> &pool = poolFor(pass);
            if (pool.isEmpty())
                continue;
            Decision d;
            d.when = now;
            d.pass = pass;
            d.passName = passNames.value(pass);
            d.poolSize = pool.size();
            d.survivors = pool.size();
            d.rulesActive = false;
            d.relaxation = None;
            const Candidate &c = pool.at(QRandomGenerator::global()->bounded(pool.size()));
            d.path = c.path;
            d.artist = c.artist;
            d.title = c.title;
            selection.ok = true;
            selection.track = c;
            selection.decision = d;
            recordDecision(d);
            return selection;
        }
        return selection;
    }

    const History history = recentHistory(s, now);

    // Rules first, passes second. A track that breaks no rotation rule but is
    // outside this hour's genre beats the right genre with the wrong artist:
    // the genre is a colour, the separation is what a listener complains about.
    for (int relaxation = None; relaxation <= kMaxRelaxation; ++relaxation) {
        for (int pass = 0; pass < passCount; ++pass) {
            const QList<Candidate> &pool = poolFor(pass);
            if (pool.isEmpty())
                continue;

            QList<qint64> ids;
            ids.reserve(pool.size());
            for (const Candidate &c : pool)
                ids << c.musicId;
            const QHash<qint64, Rule> rules = rulesFor(ids);

            QStringList rejections;
            const QList<QPair<Candidate, double>> kept =
                survivors(pool, rules, history, s, relaxation, now, &rejections);
            if (kept.isEmpty())
                continue;

            double weight = 0.0;
            const Candidate chosen = weightedPick(kept, &weight);
            const Rule rule = rules.contains(chosen.musicId)
                                  ? rules.value(chosen.musicId)
                                  : defaultRule(chosen.musicId, s);

            Decision d;
            d.when       = now;
            d.path       = chosen.path;
            d.artist     = chosen.artist;
            d.title      = chosen.title;
            d.category   = rule.category.isEmpty() ? s.defaultCategory : rule.category;
            d.weight     = weight;
            d.relaxation = relaxation;
            d.pass       = pass;
            d.passName   = passNames.value(pass);
            d.poolSize   = pool.size();
            d.survivors  = kept.size();
            d.rulesActive = true;
            d.historyFromAirLog = history.fromAirLog;
            d.relaxedRules = relaxedRuleNames(relaxation);
            d.rejections   = rejections;

            selection.ok = true;
            selection.track = chosen;
            selection.decision = d;
            recordDecision(d);
            return selection;
        }
    }

    // Every pass was empty. Not a rotation problem — there was nothing to
    // choose from at all, and the caller's own last-resort path handles it.
    return selection;
}

// ------------------------------------------------------------ the "why" ----

void RotationRules::recordDecision(const Decision &d)
{
    m_decisions.append(d);
    while (m_decisions.size() > kDecisionsKept)
        m_decisions.removeFirst();
    emit decisionRecorded();
}

QList<RotationRules::Decision> RotationRules::recentDecisions() const
{
    QList<Decision> out = m_decisions;
    std::reverse(out.begin(), out.end());
    return out;
}

void RotationRules::clearDecisions()
{
    m_decisions.clear();
    emit decisionRecorded();
}
