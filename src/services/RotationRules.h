#ifndef ROTATIONRULES_H
#define ROTATIONRULES_H

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class QSqlDatabase;

/**
 * @brief Rotation rules: what turns a good picker into a programmed station.
 *
 * Auto Mode already picks well — this hour's genre, a tempo that segues, and
 * nothing it played in the last few tracks. What a listener actually notices
 * is none of that. It is the same artist twice inside half an hour, the same
 * song either side of the news, and a Christmas record in June. Those are
 * rotation rules, and they are evaluated here.
 *
 * Five rules, all of them preferences rather than filters:
 *
 *  - **artist separation** — no artist twice inside N minutes (default 40)
 *  - **title separation**  — no title twice inside N hours (default 3)
 *  - **category weight**   — power / secondary / gold, so the A-list turns
 *                            over faster than the oldies
 *  - **dayparting**        — a track allowed only in given hours / weekdays
 *  - **date windows**      — not before a date, dead after a date (seasonal)
 *
 * ### Why nothing here is a hard filter
 *
 * A station with 300 tracks and a 40-minute artist separation will, at some
 * point in a quiet night, have nothing left that satisfies every rule. A
 * filter would then hand Auto Mode an empty list, and an empty list is dead
 * air — the one fault a station cannot survive. So the rules are relaxed, one
 * at a time, in a fixed and published order, until something survives:
 *
 *   0. everything honoured
 *   1. artist separation halved
 *   2. artist separation dropped
 *   3. title separation halved
 *   4. title separation dropped
 *   5. daypart restrictions dropped
 *   6. date windows dropped
 *
 * The order runs from the least audible compromise to the most. Hearing the
 * same artist twice in 20 minutes is a shame; hearing a Christmas record in
 * June is a phone call, so the seasonal window is the last thing to go. Which
 * step a pick needed is recorded (see Decision) and logged, because a rule
 * that keeps being relaxed is a rule that is wrong, and the operator has to be
 * able to see that.
 *
 * ### Where "recently" comes from
 *
 * The as-run log (AirLog), which is what actually went to air — not the
 * playlist, not played_times. Only on a fresh install, where the log has no
 * rows at all, does this fall back to musics.last_played.
 */
class RotationRules : public QObject
{
    Q_OBJECT

public:
    // ------------------------------------------------------------- model --

    /** One track's rotation rules. Tracks with no row get the defaults. */
    struct Rule
    {
        qint64  musicId  = -1;
        QString category;              ///< "power" | "secondary" | "gold"
        quint32 hourMask = 0xFFFFFFu;  ///< bit h set: may play in hour h (0-23)
        quint8  dayMask  = 0x7F;       ///< bit d-1 set: may play on Qt day d (Mon=1)
        QDate   startDate;             ///< invalid: no start restriction
        QDate   endDate;               ///< invalid: never expires
        double  weight   = -1.0;       ///< < 0: use the category's weight
        bool    stored   = false;      ///< false: defaults, no row in the table

        bool restrictsHours() const { return (hourMask & 0xFFFFFFu) != 0xFFFFFFu; }
        bool restrictsDays()  const { return (dayMask & 0x7F) != 0x7F; }
        bool restrictsDates() const { return startDate.isValid() || endDate.isValid(); }
    };

    /** Station-wide rotation settings, from the Rotation group of xfb.conf. */
    struct Settings
    {
        bool   enabled                 = true;
        int    artistSeparationMinutes = 40;
        int    titleSeparationHours    = 3;
        double powerWeight             = 10.0;
        double secondaryWeight         = 5.0;
        double goldWeight              = 2.0;
        QString defaultCategory        = QStringLiteral("secondary");
        /** How many rows a pass offers the rules. Bounds the work on a huge
         *  library while still being a fair random sample of it. */
        int    candidateLimit          = 400;
    };

    /** One row Auto Mode is considering. */
    struct Candidate
    {
        qint64  musicId = -1;
        QString path;
        QString artist;
        QString title;
    };

    /**
     * The relaxation ladder. Cumulative: at TitleHalved the artist rule has
     * already been given up. Keep the order — it is the documented behaviour
     * and the harness asserts it.
     */
    enum Relaxation {
        None = 0,
        ArtistHalved,
        ArtistDropped,
        TitleHalved,
        TitleDropped,
        DaypartDropped,
        DateWindowDropped
    };
    static constexpr int kMaxRelaxation = DateWindowDropped;

    /** Why one pick came out the way it did — the "why this track?" view. */
    struct Decision
    {
        QDateTime   when;
        QString     path;
        QString     artist;
        QString     title;
        QString     category;
        double      weight      = 0.0;
        int         relaxation  = None;
        QString     passName;       ///< "tempo-matched, genre Pop"
        int         pass         = 0;
        int         poolSize     = 0;   ///< rows the pass offered
        int         survivors    = 0;   ///< rows that survived the rules
        bool        rulesActive  = true;///< false when rotation is switched off
        bool        historyFromAirLog = true;
        QStringList relaxedRules;   ///< translated names of what had to go
        QStringList rejections;     ///< a few "why not" lines, for the view
    };

    /** What select() came back with. */
    struct Selection
    {
        bool      ok = false;
        Candidate track;
        Decision  decision;
    };

    /** Artists and titles that were on air recently, and where that came from. */
    struct History
    {
        QHash<QString, QDateTime> lastArtist;  ///< key: folded artist
        QHash<QString, QDateTime> lastTitle;   ///< key: folded title
        bool fromAirLog = true;
        bool usable     = false;   ///< false: no history at all (first ever run)
    };

    static RotationRules *instance();

    // ------------------------------------------------------------ schema --

    /**
     * Creates the rotation table. Idempotent, called from the same place in
     * checkDbOpen() as every other XFB migration.
     */
    static bool ensureSchema(QSqlDatabase db);

    // ---------------------------------------------------------- settings --

    static Settings settings();
    static void     saveSettings(const Settings &s);

    static QStringList categoryKeys();
    static QString     categoryLabel(const QString &key);
    static double      categoryWeight(const QString &key, const Settings &s);

    /** Translated name of one rule, for the log and the "why" view. */
    static QString ruleLabel(int relaxationStep);
    /** Every rule given up at this level, in the order they were given up. */
    static QStringList relaxedRuleNames(int relaxation);

    // ------------------------------------------------------------- rules --

    static Rule defaultRule(qint64 musicId, const Settings &s);
    static Rule ruleFor(qint64 musicId);
    /** One query for a whole candidate list — the picker's hot path. */
    static QHash<qint64, Rule> rulesFor(const QList<qint64> &musicIds);
    static bool saveRule(const Rule &r);
    static bool clearRule(qint64 musicId);
    /** Rows in the rotation table, for the editor's summary line. */
    static int  ruleCount();

    static quint32 hoursFromList(const QList<int> &hours);
    static QList<int> hoursFromMask(quint32 mask);
    /** "all hours", "06:00-10:00", "3 hours" — for the editor's table. */
    static QString describeHours(quint32 mask);
    static QString describeDays(quint8 mask);
    static QString describeDates(const QDate &from, const QDate &to);

    // ----------------------------------------------------------- history --

    /**
     * What was on air inside the longest separation window. Reads the as-run
     * log; only when that log has never recorded anything does it fall back
     * to musics.last_played.
     */
    static History recentHistory(const Settings &s, const QDateTime &now);

    // ------------------------------------------------------------ picking --

    /**
     * The whole ladder. @a candidatesForPass is asked for the rows of each
     * pass (Auto Mode's tempo/genre passes); it is called at most once per
     * pass, and the result cached, so the SQL cost does not multiply by the
     * number of relaxation levels.
     *
     * All rules are tried against every pass before any rule is relaxed: a
     * track that breaks no rotation rule but sits outside this hour's genre
     * is a better choice than the right genre with the wrong artist.
     */
    Selection select(const std::function<QList<Candidate>(int pass)> &candidatesForPass,
                     int passCount,
                     const QStringList &passNames,
                     const QDateTime &now);

    /**
     * One level of the ladder against one pool. Exposed for the harness and
     * for the editor's explanation of a hypothetical pick.
     * @param rejections when non-null, receives a few human-readable "why not"
     *        lines (bounded — this is for a person to read, not a report).
     */
    static QList<QPair<Candidate, double>>
    survivors(const QList<Candidate> &candidates,
              const QHash<qint64, Rule> &rules,
              const History &history,
              const Settings &s,
              int relaxation,
              const QDateTime &now,
              QStringList *rejections = nullptr);

    /** Weighted random choice. Falls back to uniform if every weight is 0. */
    static Candidate weightedPick(const QList<QPair<Candidate, double>> &pool,
                                  double *chosenWeight = nullptr);

    // -------------------------------------------------------- the "why" view --

    void recordDecision(const Decision &d);
    QList<Decision> recentDecisions() const;   ///< newest first
    void clearDecisions();
    static constexpr int kDecisionsKept = 50;

signals:
    /** A pick was made — the editor's "why this track?" tab refreshes on it. */
    void decisionRecorded();

private:
    explicit RotationRules(QObject *parent = nullptr);

    /** Case- and whitespace-insensitive key for an artist or a title. */
    static QString fold(const QString &text);

    QList<Decision> m_decisions;   ///< newest last, bounded to kDecisionsKept
};

#endif // ROTATIONRULES_H
