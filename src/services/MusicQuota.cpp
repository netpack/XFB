#include "MusicQuota.h"

#include <QDebug>
#include <QLocale>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QVariant>

namespace {

const char *const kConnection = "xfb_connection";

QSqlDatabase quotaDb()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

/** The same xfb.conf every other XFB setting lives in. */
QString settingsFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/** yyyy-MM-dd, or an empty string when there is no date. */
QString dateToText(const QDate &date)
{
    return date.isValid() ? date.toString(QStringLiteral("yyyy-MM-dd")) : QString();
}

QDate dateFromText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed == QLatin1String("-"))
        return QDate();
    QDate date = QDate::fromString(trimmed, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid())        // what add_music_single writes into musics
        date = QDate::fromString(trimmed, QStringLiteral("yyyy/MM/dd"));
    if (!date.isValid())
        date = QDate::fromString(trimmed, Qt::ISODate);
    // A year on its own is what a tagger usually gives, and the law asks
    // whether the record is inside a twelve-month window: read it as the
    // first of January, which is the earliest that year can mean and so the
    // reading that never claims a record is newer than it is.
    if (!date.isValid() && trimmed.size() == 4) {
        bool ok = false;
        const int year = trimmed.toInt(&ok);
        if (ok && year > 1000 && year < 3000)
            date = QDate(year, 1, 1);
    }
    return date;
}

/**
 * Does this musics.country cell actually name a country?
 *
 * The downloader writes the literal "Other country / language" into every
 * track it fetches, and older importers left a dash or nothing at all. None
 * of those is somebody saying where the record comes from, and reading them
 * as "not Portugal" would mark a whole downloaded library international on
 * one button press — a guess that afterwards looks exactly like an answer.
 */
bool namesACountry(const QString &country)
{
    const QString folded = country.trimmed().toCaseFolded();
    if (folded.isEmpty() || folded == QLatin1String("-"))
        return false;
    static const QStringList placeholders{
        QStringLiteral("other country / language"),   // what the downloader writes
        QStringLiteral("other"),
        QStringLiteral("unknown"),
        QStringLiteral("n/a"),
    };
    return !placeholders.contains(folded);
}

/** An explicit SQL NULL, rather than a null QString bound as an empty one. */
QVariant nullText()
{
    return QVariant(QMetaType(QMetaType::QString));
}

} // namespace

// ---------------------------------------------------------------- model ---

bool MusicQuota::Mark::isIncomplete(const Settings &s) const
{
    if (origin != National)
        return false;
    if (language == LanguageUnset || !releaseDate.isValid())
        return true;
    // Sung in the national language, but whether the performers are EU
    // citizens was never answered: the language share cannot count it, and
    // that is a gap in the marking rather than a fact about the record.
    return s.requireEuPerformer && language == NationalLanguage
           && performerEu < 0;
}

void MusicQuota::Tally::add(qint64 lengthMs)
{
    ++tracks;
    if (lengthMs > 0)
        ms += lengthMs;
}

double MusicQuota::Tally::value(bool byDuration) const
{
    return byDuration ? double(ms) : double(tracks);
}

double MusicQuota::Slice::percentOfMusic(const Tally &part, bool byDuration) const
{
    const double total = music.value(byDuration);
    if (total <= 0.0)
        return 0.0;
    return part.value(byDuration) * 100.0 / total;
}

double MusicQuota::Slice::percentOfNational(const Tally &part, bool byDuration) const
{
    const double total = national.value(byDuration);
    if (total <= 0.0)
        return 0.0;
    return part.value(byDuration) * 100.0 / total;
}

// --------------------------------------------------------------- schema ---

bool MusicQuota::ensureSchema(QSqlDatabase db)
{
    if (!db.isOpen())
        return false;

    QSqlQuery qry(db);
    // Keyed to musics.id, one row per track somebody has said something
    // about. Absence means unmarked, and the report counts unmarked as its
    // own thing rather than as international — see the class comment.
    const char *const kCreate =
        "CREATE TABLE IF NOT EXISTS music_quota ("
        "  music_id INTEGER PRIMARY KEY,"
        "  origin TEXT NOT NULL DEFAULT 'unset',"
        "  language TEXT NOT NULL DEFAULT 'unset',"
        "  performer_eu INTEGER,"          // 1 yes, 0 no, NULL not said
        "  release_date TEXT,"             // first published / first broadcast
        "  is_music INTEGER NOT NULL DEFAULT 1,"
        "  note TEXT,"
        "  updated_at TEXT"
        ")";
    if (!qry.exec(QLatin1String(kCreate))) {
        qWarning() << "MusicQuota: could not create the music_quota table:"
                   << qry.lastError().text();
        return false;
    }
    // The report reads by origin over a month of as-run rows; the editor's
    // "unmarked only" filter reads the same way.
    if (!qry.exec(QLatin1String("CREATE INDEX IF NOT EXISTS music_quota_origin "
                                "ON music_quota(origin)"))) {
        qWarning() << "MusicQuota: could not create music_quota_origin:"
                   << qry.lastError().text();
        return false;
    }
    return true;
}

// ------------------------------------------------------------- settings ---

MusicQuota::Settings MusicQuota::portugalPreset()
{
    Settings s;
    s.countryName          = QStringLiteral("Portugal");
    s.languageName         = tr("Portuguese");
    s.nationalPercent      = 30.0;
    s.languageSharePercent = 60.0;
    s.requireEuPerformer   = true;
    s.recentSharePercent   = 35.0;
    s.recentMonths         = 12;
    s.primeStartHour       = 7;
    s.primeEndHour         = 20;
    s.primeDayMask         = 0x1F;      // Monday to Friday
    s.byDuration           = true;
    s.includeUnlibraried   = true;
    return s;
}

MusicQuota::Settings MusicQuota::settings()
{
    Settings s = portugalPreset();      // the figures XFB starts with
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("Quota"));
    s.enabled = config.value(QStringLiteral("Enabled"), s.enabled).toBool();
    s.countryName = config.value(QStringLiteral("CountryName"),
                                 s.countryName).toString();
    s.languageName = config.value(QStringLiteral("LanguageName"),
                                  s.languageName).toString();
    s.nationalPercent = qBound(0.0, config.value(QStringLiteral("NationalPercent"),
                                                 s.nationalPercent).toDouble(), 100.0);
    s.languageSharePercent =
        qBound(0.0, config.value(QStringLiteral("LanguageSharePercent"),
                                 s.languageSharePercent).toDouble(), 100.0);
    s.requireEuPerformer = config.value(QStringLiteral("RequireEuPerformer"),
                                        s.requireEuPerformer).toBool();
    s.recentSharePercent =
        qBound(0.0, config.value(QStringLiteral("RecentSharePercent"),
                                 s.recentSharePercent).toDouble(), 100.0);
    s.recentMonths = qBound(1, config.value(QStringLiteral("RecentMonths"),
                                            s.recentMonths).toInt(), 120);
    s.primeStartHour = qBound(0, config.value(QStringLiteral("PrimeStartHour"),
                                              s.primeStartHour).toInt(), 23);
    s.primeEndHour = qBound(1, config.value(QStringLiteral("PrimeEndHour"),
                                            s.primeEndHour).toInt(), 24);
    if (s.primeEndHour <= s.primeStartHour)     // an empty window measures nothing
        s.primeEndHour = qMin(24, s.primeStartHour + 1);
    s.primeDayMask = static_cast<quint8>(
        config.value(QStringLiteral("PrimeDayMask"), s.primeDayMask).toUInt()) & 0x7F;
    if (s.primeDayMask == 0)
        s.primeDayMask = 0x1F;
    s.byDuration = config.value(QStringLiteral("ByDuration"), s.byDuration).toBool();
    s.includeUnlibraried = config.value(QStringLiteral("IncludeUnlibraried"),
                                        s.includeUnlibraried).toBool();
    s.exemptionNote = config.value(QStringLiteral("ExemptionNote")).toString();
    config.endGroup();
    return s;
}

void MusicQuota::saveSettings(const Settings &s)
{
    QSettings config(settingsFile(), QSettings::IniFormat);
    config.beginGroup(QStringLiteral("Quota"));
    config.setValue(QStringLiteral("Enabled"), s.enabled);
    config.setValue(QStringLiteral("CountryName"), s.countryName);
    config.setValue(QStringLiteral("LanguageName"), s.languageName);
    config.setValue(QStringLiteral("NationalPercent"), s.nationalPercent);
    config.setValue(QStringLiteral("LanguageSharePercent"), s.languageSharePercent);
    config.setValue(QStringLiteral("RequireEuPerformer"), s.requireEuPerformer);
    config.setValue(QStringLiteral("RecentSharePercent"), s.recentSharePercent);
    config.setValue(QStringLiteral("RecentMonths"), s.recentMonths);
    config.setValue(QStringLiteral("PrimeStartHour"), s.primeStartHour);
    config.setValue(QStringLiteral("PrimeEndHour"), s.primeEndHour);
    config.setValue(QStringLiteral("PrimeDayMask"), uint(s.primeDayMask));
    config.setValue(QStringLiteral("ByDuration"), s.byDuration);
    config.setValue(QStringLiteral("IncludeUnlibraried"), s.includeUnlibraried);
    config.setValue(QStringLiteral("ExemptionNote"), s.exemptionNote);
    config.endGroup();
    config.sync();
}

// ---------------------------------------------------------------- marks ---

MusicQuota::Mark MusicQuota::markFor(qint64 musicId)
{
    const QHash<qint64, Mark> one = marksFor({ musicId });
    if (one.contains(musicId))
        return one.value(musicId);
    Mark unmarked;                      // stored == false: nothing said yet
    unmarked.musicId = musicId;
    return unmarked;
}

QHash<qint64, MusicQuota::Mark> MusicQuota::marksFor(const QList<qint64> &musicIds)
{
    QHash<qint64, Mark> out;
    if (musicIds.isEmpty())
        return out;
    QSqlDatabase db = quotaDb();
    if (!db.isOpen())
        return out;

    // Chunked for the same reason RotationRules::rulesFor() is: SQLite's
    // bound-variable limit is 999 and a month of as-run rows is longer.
    const int kChunk = 400;
    for (int start = 0; start < musicIds.size(); start += kChunk) {
        const int count = qMin(kChunk, musicIds.size() - start);
        QStringList placeholders;
        for (int i = 0; i < count; ++i)
            placeholders << QStringLiteral(":i%1").arg(i);
        QSqlQuery qry(db);
        qry.prepare(QStringLiteral(
            "SELECT music_id, origin, language, performer_eu, release_date,"
            " is_music, note FROM music_quota WHERE music_id IN (%1)")
                        .arg(placeholders.join(QStringLiteral(", "))));
        for (int i = 0; i < count; ++i)
            qry.bindValue(QStringLiteral(":i%1").arg(i), musicIds.at(start + i));
        if (!qry.exec()) {
            qWarning() << "MusicQuota: could not read the marks:"
                       << qry.lastError().text();
            return out;
        }
        while (qry.next()) {
            Mark mark;
            mark.musicId     = qry.value(0).toLongLong();
            mark.origin      = originFromKey(qry.value(1).toString());
            mark.language    = languageFromKey(qry.value(2).toString());
            mark.performerEu = qry.value(3).isNull() ? -1
                                                     : (qry.value(3).toInt() ? 1 : 0);
            mark.releaseDate = dateFromText(qry.value(4).toString());
            mark.isMusic     = qry.value(5).toInt() != 0;
            mark.note        = qry.value(6).toString();
            mark.stored      = true;
            out.insert(mark.musicId, mark);
        }
    }
    return out;
}

bool MusicQuota::saveMark(const Mark &mark)
{
    if (mark.musicId < 0)
        return false;
    QSqlDatabase db = quotaDb();
    if (!db.isOpen())
        return false;

    QSqlQuery qry(db);
    qry.prepare(QStringLiteral(
        "INSERT INTO music_quota (music_id, origin, language, performer_eu,"
        " release_date, is_music, note, updated_at)"
        " VALUES (:id, :origin, :language, :eu, :release, :music, :note, :updated)"
        " ON CONFLICT(music_id) DO UPDATE SET"
        " origin = excluded.origin, language = excluded.language,"
        " performer_eu = excluded.performer_eu,"
        " release_date = excluded.release_date, is_music = excluded.is_music,"
        " note = excluded.note, updated_at = excluded.updated_at"));
    qry.bindValue(QStringLiteral(":id"), mark.musicId);
    qry.bindValue(QStringLiteral(":origin"), originKey(mark.origin));
    qry.bindValue(QStringLiteral(":language"), languageKey(mark.language));
    qry.bindValue(QStringLiteral(":eu"),
                  mark.performerEu < 0 ? QVariant(QMetaType(QMetaType::Int))
                                       : QVariant(mark.performerEu ? 1 : 0));
    qry.bindValue(QStringLiteral(":release"),
                  mark.releaseDate.isValid() ? QVariant(dateToText(mark.releaseDate))
                                             : nullText());
    qry.bindValue(QStringLiteral(":music"), mark.isMusic ? 1 : 0);
    qry.bindValue(QStringLiteral(":note"),
                  mark.note.isEmpty() ? nullText() : QVariant(mark.note));
    qry.bindValue(QStringLiteral(":updated"),
                  QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!qry.exec()) {
        qWarning() << "MusicQuota: could not save the mark for" << mark.musicId
                   << ":" << qry.lastError().text();
        return false;
    }
    return true;
}

bool MusicQuota::clearMark(qint64 musicId)
{
    QSqlDatabase db = quotaDb();
    if (!db.isOpen())
        return false;
    QSqlQuery qry(db);
    qry.prepare(QStringLiteral("DELETE FROM music_quota WHERE music_id = :id"));
    qry.bindValue(QStringLiteral(":id"), musicId);
    if (!qry.exec()) {
        qWarning() << "MusicQuota: could not clear the mark for" << musicId
                   << ":" << qry.lastError().text();
        return false;
    }
    return true;
}

int MusicQuota::libraryTrackCount()
{
    QSqlDatabase db = quotaDb();
    if (!db.isOpen())
        return 0;
    QSqlQuery qry(db);
    if (!qry.exec(QStringLiteral("SELECT COUNT(*) FROM musics")) || !qry.next())
        return 0;
    return qry.value(0).toInt();
}

int MusicQuota::markedTrackCount()
{
    QSqlDatabase db = quotaDb();
    if (!db.isOpen())
        return 0;
    QSqlQuery qry(db);
    if (!qry.exec(QStringLiteral("SELECT COUNT(*) FROM music_quota"
                                 " WHERE origin <> 'unset'")) || !qry.next())
        return 0;
    return qry.value(0).toInt();
}

int MusicQuota::seedFromLibrary(const Settings &s)
{
    QSqlDatabase db = quotaDb();
    if (!db.isOpen())
        return 0;

    const QString country = s.countryName.trimmed();
    if (country.isEmpty())
        return 0;

    // Only rows that have no mark at all: a guess made from a free-text
    // country column must never overwrite an answer somebody gave on purpose.
    QSqlQuery qry(db);
    qry.prepare(QStringLiteral(
        "SELECT m.id, m.country, m.published_date FROM musics m"
        " LEFT JOIN music_quota q ON q.music_id = m.id"
        " WHERE q.music_id IS NULL AND m.id IS NOT NULL"));
    if (!qry.exec()) {
        qWarning() << "MusicQuota: could not read the library for seeding:"
                   << qry.lastError().text();
        return 0;
    }

    // The library's country column is free text, written by several importers
    // over the years, so the match is on the name as typed, ignoring case and
    // surrounding space. A row whose country says nothing, or says something
    // this does not recognise, is left unmarked rather than guessed at as
    // international: the report counts unmarked as unmarked, and a wrong guess
    // would be indistinguishable from an answer.
    const QString nationalName = country.toCaseFolded();

    struct Seed { qint64 id; Origin origin; QDate release; };
    QList<Seed> seeds;
    while (qry.next()) {
        const qint64 id = qry.value(0).toLongLong();
        if (id <= 0)
            continue;
        const QString rowCountry = qry.value(1).toString().trimmed().toCaseFolded();
        if (!namesACountry(rowCountry))
            continue;               // says nothing: leave it unmarked
        const bool isNational = rowCountry == nationalName;
        seeds.append(Seed{ id, isNational ? National : International,
                           dateFromText(qry.value(2).toString()) });
    }
    if (seeds.isEmpty())
        return 0;

    db.transaction();
    int written = 0;
    for (const Seed &seed : seeds) {
        Mark mark;
        mark.musicId = seed.id;
        mark.origin  = seed.origin;
        mark.releaseDate = seed.release;
        // Language and citizenship are not in the library at all, so they
        // stay unsaid: the sub-quotas must not be built out of a guess.
        if (saveMark(mark))
            ++written;
    }
    db.commit();
    if (written > 0)
        qInfo() << "MusicQuota: seeded" << written << "track(s) from the country column";
    return written;
}

// -------------------------------------------------------------- reading ---

bool MusicQuota::isPrimeTime(const QDateTime &when, const Settings &s)
{
    if (!when.isValid())
        return false;
    const int day = when.date().dayOfWeek();            // Monday = 1
    if (!(s.primeDayMask & (1u << (day - 1))))
        return false;
    const int hour = when.time().hour();
    return hour >= s.primeStartHour && hour < s.primeEndHour;
}

bool MusicQuota::inLanguageQuota(const Mark &mark, const Settings &s)
{
    if (mark.origin != National || mark.language != NationalLanguage)
        return false;
    // "interpretada por cidadãos de Estados-Membros da União Europeia": when
    // the station's rules ask for it, an unanswered citizenship is not a yes.
    return !s.requireEuPerformer || mark.performerEu == 1;
}

bool MusicQuota::isRecent(const Mark &mark, const QDateTime &airedAt,
                          const Settings &s)
{
    if (mark.origin != National || !mark.releaseDate.isValid() || !airedAt.isValid())
        return false;
    const QDate earliest = airedAt.date().addMonths(-s.recentMonths);
    return mark.releaseDate >= earliest && mark.releaseDate <= airedAt.date();
}

MusicQuota::Report MusicQuota::buildForMonth(const QDate &anyDayInIt,
                                             const Settings &s)
{
    const QDate first(anyDayInIt.year(), anyDayInIt.month(), 1);
    return build(QDateTime(first, QTime(0, 0)),
                 QDateTime(first.addMonths(1), QTime(0, 0)), s);
}

MusicQuota::Report MusicQuota::build(const QDateTime &from, const QDateTime &to,
                                     const Settings &s)
{
    Report report;
    report.from = from;
    report.to   = to;
    report.settings = s;

    QSqlDatabase db = quotaDb();
    if (!db.isOpen()) {
        report.databaseOpen = false;
        return report;
    }
    report.libraryTracks = libraryTrackCount();
    report.markedTracks  = markedTrackCount();

    // Music that went to air, from the as-run log and nothing else. 'musics'
    // is a library track; 'fallback' is a file that was played without being
    // in any table, which is still music somebody put on air — it is counted
    // when the station asks for it, and always reported separately, because
    // there is nothing to look up and so nothing to say about it.
    QStringList sources{ QStringLiteral("musics") };
    if (s.includeUnlibraried)
        sources << QStringLiteral("fallback");
    QStringList sourcePlaceholders;
    for (int i = 0; i < sources.size(); ++i)
        sourcePlaceholders << QStringLiteral(":s%1").arg(i);

    QSqlQuery qry(db);
    // started_epoch, not started_at: the text stamp carries a UTC offset and
    // sorts wrong across a clock change (see AirLog::ensureSchema).
    qry.prepare(QStringLiteral(
        "SELECT a.started_epoch, a.source, a.source_id, a.artist, a.title,"
        " a.planned_ms, a.played_ms,"
        " q.origin, q.language, q.performer_eu, q.release_date, q.is_music"
        " FROM airlog a"
        " LEFT JOIN music_quota q"
        "   ON q.music_id = a.source_id AND a.source = 'musics'"
        " WHERE a.started_epoch >= :from AND a.started_epoch < :to"
        "   AND a.source IN (%1)"
        " ORDER BY a.started_epoch").arg(sourcePlaceholders.join(QStringLiteral(", "))));
    qry.bindValue(QStringLiteral(":from"), from.toSecsSinceEpoch());
    qry.bindValue(QStringLiteral(":to"), to.toSecsSinceEpoch());
    for (int i = 0; i < sources.size(); ++i)
        qry.bindValue(sourcePlaceholders.at(i), sources.at(i));
    if (!qry.exec()) {
        qWarning() << "MusicQuota: could not read the as-run log:"
                   << qry.lastError().text();
        return report;
    }

    while (qry.next()) {
        Entry entry;
        entry.when    = QDateTime::fromSecsSinceEpoch(qry.value(0).toLongLong());
        entry.musicId = qry.value(2).isNull() ? -1 : qry.value(2).toLongLong();
        entry.artist  = qry.value(3).toString();
        entry.title   = qry.value(4).toString();
        entry.unidentified = qry.value(1).toString() != QLatin1String("musics")
                             || entry.musicId < 0;

        // What actually aired, not what was meant to: a record faded at two
        // minutes contributed two minutes of the station's music.
        const qint64 played  = qry.value(6).isNull() ? -1 : qry.value(6).toLongLong();
        const qint64 planned = qry.value(5).isNull() ? -1 : qry.value(5).toLongLong();
        entry.ms = played > 0 ? played : (planned > 0 ? planned : 0);

        if (!qry.value(7).isNull()) {       // a stored mark
            Mark &mark = entry.mark;
            mark.musicId     = entry.musicId;
            mark.origin      = originFromKey(qry.value(7).toString());
            mark.language    = languageFromKey(qry.value(8).toString());
            mark.performerEu = qry.value(9).isNull() ? -1
                                                     : (qry.value(9).toInt() ? 1 : 0);
            mark.releaseDate = dateFromText(qry.value(10).toString());
            mark.isMusic     = qry.value(11).isNull() || qry.value(11).toInt() != 0;
            mark.stored      = true;
        }

        // Filed under music but not musical programming: out of both sides
        // of the fraction, and reported so the operator can see it happened.
        if (entry.mark.stored && !entry.mark.isMusic) {
            report.excluded.add(entry.ms);
            continue;
        }

        entry.prime           = isPrimeTime(entry.when, s);
        entry.national        = entry.mark.origin == National;
        entry.inLanguageQuota = inLanguageQuota(entry.mark, s);
        entry.recent          = isRecent(entry.mark, entry.when, s);

        // The same arithmetic over the whole period and over prime time.
        Slice *slices[2] = { &report.overall, entry.prime ? &report.prime : nullptr };
        for (Slice *slice : slices) {
            if (!slice)
                continue;
            slice->music.add(entry.ms);
            if (entry.ms <= 0)
                ++slice->withoutLength;
            if (entry.national) {
                slice->national.add(entry.ms);
                if (entry.inLanguageQuota)
                    slice->language.add(entry.ms);
                if (entry.recent)
                    slice->recent.add(entry.ms);
                if (entry.mark.isIncomplete(s))
                    slice->incomplete.add(entry.ms);
            } else if (entry.mark.isUnmarked()) {
                slice->unmarked.add(entry.ms);
                if (entry.unidentified)
                    slice->unidentified.add(entry.ms);
            }
        }
        report.entries.append(entry);
    }
    return report;
}

QList<MusicQuota::Requirement> MusicQuota::requirementsFor(const Slice &slice,
                                                           const Settings &s)
{
    QList<Requirement> out;

    Requirement national;
    national.key      = QStringLiteral("national");
    national.label    = tr("Music from %1").arg(s.countryName);
    national.basis    = tr("of the musical programming");
    national.required = s.nationalPercent;
    national.achieved = slice.percentOfMusic(slice.national, s.byDuration);
    national.measurable = !slice.music.isEmpty();
    national.met      = national.measurable && national.achieved >= s.nationalPercent;
    out.append(national);

    Requirement language;
    language.key   = QStringLiteral("language");
    language.label = s.requireEuPerformer
                         ? tr("Sung in %1 by performers who are EU citizens")
                               .arg(s.languageName)
                         : tr("Sung in %1").arg(s.languageName);
    language.basis    = tr("of the music from %1").arg(s.countryName);
    language.required = s.languageSharePercent;
    language.achieved = slice.percentOfNational(slice.language, s.byDuration);
    language.measurable = !slice.national.isEmpty();
    language.met = language.measurable && language.achieved >= s.languageSharePercent;
    out.append(language);

    Requirement recent;
    recent.key      = QStringLiteral("recent");
    recent.label    = s.recentMonths == 1
                          ? tr("First published in the last month")
                          : tr("First published in the last %1 months")
                                .arg(s.recentMonths);
    recent.basis    = tr("of the music from %1").arg(s.countryName);
    recent.required = s.recentSharePercent;
    recent.achieved = slice.percentOfNational(slice.recent, s.byDuration);
    recent.measurable = !slice.national.isEmpty();
    recent.met = recent.measurable && recent.achieved >= s.recentSharePercent;
    out.append(recent);

    return out;
}

// --------------------------------------------------------------- labels ---

QStringList MusicQuota::originKeys()
{
    return { QStringLiteral("unset"), QStringLiteral("national"),
             QStringLiteral("international") };
}

QString MusicQuota::originKey(Origin origin)
{
    switch (origin) {
    case National:      return QStringLiteral("national");
    case International: return QStringLiteral("international");
    case OriginUnset:   break;
    }
    return QStringLiteral("unset");
}

MusicQuota::Origin MusicQuota::originFromKey(const QString &key)
{
    const QString folded = key.trimmed().toLower();
    if (folded == QLatin1String("national"))
        return National;
    if (folded == QLatin1String("international"))
        return International;
    return OriginUnset;
}

QString MusicQuota::originLabel(Origin origin)
{
    switch (origin) {
    case National:      return tr("National");
    case International: return tr("International");
    case OriginUnset:   break;
    }
    return tr("Not marked");
}

QStringList MusicQuota::languageKeys()
{
    return { QStringLiteral("unset"), QStringLiteral("national"),
             QStringLiteral("other"), QStringLiteral("instrumental") };
}

QString MusicQuota::languageKey(Language language)
{
    switch (language) {
    case NationalLanguage: return QStringLiteral("national");
    case OtherLanguage:    return QStringLiteral("other");
    case Instrumental:     return QStringLiteral("instrumental");
    case LanguageUnset:    break;
    }
    return QStringLiteral("unset");
}

MusicQuota::Language MusicQuota::languageFromKey(const QString &key)
{
    const QString folded = key.trimmed().toLower();
    if (folded == QLatin1String("national"))
        return NationalLanguage;
    if (folded == QLatin1String("other"))
        return OtherLanguage;
    if (folded == QLatin1String("instrumental"))
        return Instrumental;
    return LanguageUnset;
}

QString MusicQuota::languageLabel(Language language, const Settings &s)
{
    switch (language) {
    case NationalLanguage:
        return s.languageName.isEmpty() ? tr("The national language")
                                        : s.languageName;
    case OtherLanguage: return tr("Another language");
    case Instrumental:  return tr("Instrumental");
    case LanguageUnset: break;
    }
    return tr("Not said");
}

QString MusicQuota::describeMark(const Mark &mark, const Settings &s)
{
    if (mark.isUnmarked())
        return tr("Not marked");
    if (!mark.isMusic)
        return tr("Not musical programming");
    if (mark.origin == International)
        return tr("International");

    QStringList parts{ languageLabel(mark.language, s) };
    if (mark.language == NationalLanguage) {
        if (mark.performerEu == 1)
            parts << tr("EU performers");
        else if (mark.performerEu == 0)
            parts << tr("non-EU performers");
        else if (s.requireEuPerformer)
            parts << tr("citizenship not said");
    }
    parts << (mark.releaseDate.isValid()
                  ? mark.releaseDate.toString(QStringLiteral("yyyy-MM-dd"))
                  : tr("no release date"));
    return parts.join(QStringLiteral(", "));
}

QString MusicQuota::percentText(double percent)
{
    return QStringLiteral("%1%").arg(percent, 0, 'f', 1);
}

QString MusicQuota::airtimeText(qint64 ms)
{
    const qint64 minutes = ms / 60000;
    if (minutes < 60)
        return tr("%n minute(s)", nullptr, int(minutes));
    return tr("%1 h %2 min").arg(minutes / 60).arg(minutes % 60, 2, 10,
                                                   QLatin1Char('0'));
}

QString MusicQuota::describePrimeTime(const Settings &s)
{
    const QLocale locale;
    QStringList days;
    for (int d = 1; d <= 7; ++d) {
        if (s.primeDayMask & (1u << (d - 1)))
            days << locale.dayName(d, QLocale::ShortFormat);
    }
    return tr("%1, %2:00 to %3:00")
        .arg(days.isEmpty() ? tr("no days") : days.join(QStringLiteral(", ")))
        .arg(s.primeStartHour, 2, 10, QLatin1Char('0'))
        .arg(s.primeEndHour, 2, 10, QLatin1Char('0'));
}
