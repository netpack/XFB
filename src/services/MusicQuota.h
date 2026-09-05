#ifndef MUSICQUOTA_H
#define MUSICQUOTA_H

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

class QSqlDatabase;

/**
 * @brief The national music quota: marking the library for it, and proving it
 *        from the as-run log.
 *
 * Most countries oblige a broadcaster to keep a share of its musical
 * programming national. Portugal's is the one this was written against — the
 * Lei da Rádio as amended by Lei n.º 16/2024 — and it has the shape the rest
 * of them tend to have:
 *
 *  - at least **30%** of the musical programming is Portuguese music;
 *  - at least **60% of that 30%** is sung in Portuguese by performers who are
 *    citizens of a Member State of the European Union;
 *  - at least **35% of that 30%** was first published or first broadcast in
 *    the twelve months before it aired;
 *  - the figures are taken over the **whole month**, and again over the
 *    weekday daytime — 07:00 to 20:00, Monday to Friday — on its own;
 *  - a themed station whose genre is thinly represented at home can be let off
 *    part or all of it by the regulator.
 *
 * Every one of those numbers is a setting, because the law they come from is
 * not the same law in the next country and is not the same law next year. The
 * Portuguese figures are what XFB starts with, and the Rules tab says so.
 * They are a convenience, not legal advice: what the regulator asks for is the
 * station's business, and this only makes the answer countable.
 *
 * ### Two halves
 *
 * **Marking.** One row per track that has been said anything about, keyed to
 * musics.id, exactly like the rotation rules. A track with no row is not
 * "international" — it is *unmarked*, and the report says how much of the
 * month was unmarked rather than quietly counting it against the station. A
 * library of six thousand tracks is not going to be classified in an evening,
 * so the report has to stay honest while it is half done.
 *
 * **Counting.** The as-run log, and nothing else. Not the playlist, not
 * played_times: the question is what went to air, and the only record of that
 * is AirLog. Airtime is the length that actually played (played_ms), falling
 * back to the intended length, so a record faded at two minutes counts two
 * minutes.
 *
 * ### What the report cannot know
 *
 * Two blind spots, both reported rather than hidden:
 *
 *  - music aired from outside the library (a file opened straight into the
 *    player) has nothing to look up, so it lands in *unidentified*;
 *  - a track marked national but with no language or release date recorded
 *    counts towards the 30% and towards neither sub-quota. Those are counted
 *    as *incomplete*, because a sub-quota that reads low only because nobody
 *    finished the marking is a number that would send an operator hunting for
 *    a problem that is not there.
 */
class MusicQuota
{
    Q_DECLARE_TR_FUNCTIONS(MusicQuota)

public:
    // --------------------------------------------------------------- model --

    /** Where the record was made. */
    enum Origin {
        OriginUnset = 0,   ///< nobody has said; NOT the same as international
        National,          ///< national music, and so inside the quota
        International
    };

    /** What is sung, as the language sub-quota means it. */
    enum Language {
        LanguageUnset = 0,
        NationalLanguage,  ///< sung in the national language
        OtherLanguage,
        Instrumental       ///< nothing sung: national, but not in the sub-quota
    };

    /** The station's reading of its own obligation. */
    struct Settings
    {
        bool    enabled = false;      ///< the station is under a quota at all
        QString countryName;          ///< "Portugal" — what "national" means here
        QString languageName;         ///< "Portuguese" — the sub-quota's language
        double  nationalPercent = 30.0;
        double  languageSharePercent = 60.0;  ///< of the national quota
        bool    requireEuPerformer = true;    ///< the "EU citizens" half of it
        double  recentSharePercent = 35.0;    ///< of the national quota
        int     recentMonths = 12;
        int     primeStartHour = 7;   ///< inclusive
        int     primeEndHour   = 20;  ///< exclusive
        quint8  primeDayMask   = 0x1F;///< bit d-1 for Qt day d: Mon-Fri
        /** true: share of airtime. false: share of the number of records. */
        bool    byDuration = true;
        /** Count music played from outside the library in the denominator. */
        bool    includeUnlibraried = true;
        /** The exemption the regulator granted, if any. Never changes a
         *  figure — it is printed on the report so whoever reads it knows. */
        QString exemptionNote;
    };

    /** One track, as the quota sees it. */
    struct Mark
    {
        qint64   musicId = -1;
        Origin   origin  = OriginUnset;
        Language language = LanguageUnset;
        /** Performers are EU citizens: 1 yes, 0 no, -1 nobody has said. */
        int      performerEu = -1;
        /** First published or first broadcast. Invalid: not recorded. */
        QDate    releaseDate;
        /** false: this row is not musical programming at all (a bed, a
         *  sweeper, a talk item somebody filed under music) and leaves both
         *  sides of the fraction. */
        bool     isMusic = true;
        QString  note;
        bool     stored  = false;  ///< false: no row, i.e. unmarked

        /** Nothing has been said about where this record comes from. */
        bool isUnmarked() const { return !stored || origin == OriginUnset; }
        /** National, but not enough is known to place it in the sub-quotas —
         *  no language, no publication date, or, where the rules ask for it,
         *  nothing said about the performers' citizenship. */
        bool isIncomplete(const Settings &s) const;
    };

    /** The Portuguese figures, which are also XFB's defaults. */
    static Settings portugalPreset();

    // ------------------------------------------------------------ counting --

    /** A set of records, measured both ways so the basis can change later. */
    struct Tally
    {
        int    tracks = 0;
        qint64 ms     = 0;
        void   add(qint64 lengthMs);
        double value(bool byDuration) const;
        bool   isEmpty() const { return tracks == 0; }
    };

    /** Everything counted over one window (the month, or its weekday daytime). */
    struct Slice
    {
        Tally music;         ///< the denominator: the musical programming
        Tally national;      ///< of which national
        Tally language;      ///< of the national, in the language sub-quota
        Tally recent;        ///< of the national, inside the recent window
        Tally incomplete;    ///< of the national, too thinly marked to place
        Tally unmarked;      ///< of the music, nothing said about it
        Tally unidentified;  ///< of the unmarked, aired from outside the library
        int   withoutLength = 0;  ///< records that aired with no length recorded

        double percentOfMusic(const Tally &part, bool byDuration) const;
        double percentOfNational(const Tally &part, bool byDuration) const;
    };

    /** One obligation, measured. */
    struct Requirement
    {
        QString key;        ///< "national" | "language" | "recent"
        QString label;
        QString basis;      ///< what the percentage is a percentage of
        double  achieved = 0.0;
        double  required = 0.0;
        bool    met = false;
        /** false: the denominator was empty, so there is nothing to judge. */
        bool    measurable = true;
    };

    /** One aired record, for the detail table and the CSV. */
    struct Entry
    {
        QDateTime when;
        qint64    musicId = -1;
        QString   artist;
        QString   title;
        qint64    ms = 0;
        bool      prime = false;
        Mark      mark;
        bool      national = false;
        bool      inLanguageQuota = false;
        bool      recent = false;
        bool      unidentified = false;  ///< aired from outside the library
    };

    /** What build() came back with. */
    struct Report
    {
        QDateTime from;
        QDateTime to;          ///< exclusive
        Settings  settings;
        Slice     overall;
        Slice     prime;
        Tally     excluded;    ///< marked "not musical programming"
        QList<Entry> entries;  ///< oldest first
        int  libraryTracks = 0;
        int  markedTracks  = 0;
        bool databaseOpen  = true;
    };

    // ------------------------------------------------------------- schema --

    /**
     * Creates the quota table. Idempotent, called from the same place in
     * checkDbOpen() as every other XFB migration, so an install that predates
     * the feature gets it on its next launch and an untouched library needs no
     * conversion at all.
     */
    static bool ensureSchema(QSqlDatabase db);

    // ----------------------------------------------------------- settings --

    static Settings settings();
    static void     saveSettings(const Settings &s);

    // -------------------------------------------------------------- marks --

    static Mark markFor(qint64 musicId);
    /** One query for a whole list — used by the editor and by the report. */
    static QHash<qint64, Mark> marksFor(const QList<qint64> &musicIds);
    static bool saveMark(const Mark &mark);
    static bool clearMark(qint64 musicId);

    /** Tracks in the library, and how many of them carry a mark. */
    static int libraryTrackCount();
    static int markedTrackCount();

    /**
     * First guess at the marking, from what the library already holds:
     * musics.country against the configured country name, and
     * musics.published_date as the release date. Only ever fills in tracks
     * that have no mark yet — a guess must not overwrite somebody's answer.
     * @return how many tracks were given a mark.
     */
    static int seedFromLibrary(const Settings &s);

    // ------------------------------------------------------------ reading --

    /** @param to exclusive. */
    static Report build(const QDateTime &from, const QDateTime &to,
                        const Settings &s);
    /** The month @a anyDayInIt falls in, which is the period the law uses. */
    static Report buildForMonth(const QDate &anyDayInIt, const Settings &s);

    static QList<Requirement> requirementsFor(const Slice &slice,
                                              const Settings &s);

    /** True when @a when falls inside the station's prime-time window. */
    static bool isPrimeTime(const QDateTime &when, const Settings &s);

    // ------------------------------------------------------------- labels --

    static QStringList originKeys();
    static QString originKey(Origin origin);
    static Origin   originFromKey(const QString &key);
    static QString  originLabel(Origin origin);

    static QStringList languageKeys();
    static QString  languageKey(Language language);
    static Language languageFromKey(const QString &key);
    static QString  languageLabel(Language language, const Settings &s);

    /** "Portuguese, EU performers, 2026-03-11" — one cell in the editor. */
    static QString describeMark(const Mark &mark, const Settings &s);

    /** A percentage as it is written on the report. */
    static QString percentText(double percent);
    /** An airtime in hours and minutes, for the totals. */
    static QString airtimeText(qint64 ms);
    /** "Monday to Friday, 07:00-20:00" for the settings the station holds. */
    static QString describePrimeTime(const Settings &s);

private:
    /** Does this mark satisfy the language sub-quota? */
    static bool inLanguageQuota(const Mark &mark, const Settings &s);
    /** Was it first published inside the window before @a airedAt? */
    static bool isRecent(const Mark &mark, const QDateTime &airedAt,
                         const Settings &s);
};

#endif // MUSICQUOTA_H
