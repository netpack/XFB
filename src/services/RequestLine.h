#ifndef REQUESTLINE_H
#define REQUESTLINE_H

#include <QDateTime>
#include <QList>
#include <QString>

class QSqlDatabase;

/**
 * @brief The listener request line: what the public page is allowed to know,
 *        and what a listener is allowed to leave behind.
 *
 * The page itself is served by MobileSyncServer under its own unauthenticated
 * `/public/` prefix; this holds everything that is not HTTP — the settings
 * that decide whether any of it exists, the `requests` table, the narrow
 * library search the page is allowed to run, and the cleaning every string a
 * stranger typed goes through before it is stored.
 *
 * Two rules shape all of it:
 *
 *  - **Nothing a listener submits may ever queue itself.** A request is a row
 *    in a table and a line in a window the operator looks at. Putting one on
 *    air is an action the operator takes, in XFB, by hand. There is no code
 *    path from an HTTP request to the playlist.
 *  - **A listener never sees a path, and never sees an id that resolves to a
 *    file.** search() returns artist and title and an opaque reference that
 *    only this process can turn back into a library row; the reference is
 *    good for naming a request and for nothing else, because no public route
 *    serves file bytes of any kind.
 *
 * Settings live in the [PublicPage] group of xfb.conf, and the feature is off
 * until the operator turns it on.
 */
class RequestLine
{
public:
    /** One row of `requests`, as the operator's tray shows it. */
    struct Entry
    {
        qint64    id = 0;
        QDateTime receivedAt;
        qint64    musicId = -1;     ///< musics.rowid, resolved on this side
        QString   path;             ///< the file, filled in here and never served
        QString   artist;           ///< copied from the library, not from the caller
        QString   title;            ///< ditto
        QString   requester;        ///< listener-supplied, cleaned and capped
        QString   dedication;       ///< listener-supplied, cleaned and capped
        QString   fromAddress;      ///< peer address, for spotting one pest
        QString   status;           ///< "new" | "queued" | "played" | "dismissed"
        QDateTime handledAt;
    };

    /** One library hit, as the public page is allowed to see it. */
    struct Match
    {
        QString artist;
        QString title;
        qint64  musicId = 0;    ///< musics.rowid; never leaves this process
    };

    /** Statuses a row may hold. */
    static QString statusNew()       { return QStringLiteral("new"); }
    static QString statusQueued()    { return QStringLiteral("queued"); }
    static QString statusPlayed()    { return QStringLiteral("played"); }
    static QString statusDismissed() { return QStringLiteral("dismissed"); }

    /**
     * Creates the requests table and its index on an existing database.
     * Idempotent, and called from player::checkDbOpen() alongside the other
     * tables, so an install that predates this gets it on the next launch.
     */
    static bool ensureSchema(QSqlDatabase db);

    // --- settings ([PublicPage] in xfb.conf) ------------------------------

    /**
     * Whether the public page exists at all. Off by default, and while it is
     * off every route under /public/ answers 404 exactly as an unknown path
     * does — there is nothing there to find.
     */
    static bool pageEnabled();
    static void setPageEnabled(bool enabled);

    /** Whether the page offers a request form. Off by default. */
    static bool requestsEnabled();
    static void setRequestsEnabled(bool enabled);

    /** The name the page calls the station. Empty falls back to the host name. */
    static QString stationName();
    static void setStationName(const QString &name);

    /** A line of the operator's own under the page title. May be empty. */
    static QString tagline();
    static void setTagline(const QString &text);

    /**
     * How many unhandled requests may sit in the table before the form stops
     * accepting more. A backstop against a slow flood that every per-address
     * limit would let through.
     */
    static int  maxPending();
    static void setMaxPending(int count);

    // --- the library, as the page may see it ------------------------------

    /**
     * Titles and artists matching @p text, and nothing else. Never returns a
     * path, and the caller cannot ask for one.
     */
    static QList<Match> search(const QString &text, int limit = 25);

    /** The file behind a library row. XFB-side only; never served or sent. */
    static QString pathForMusicId(qint64 musicId);

    // --- the requests themselves ------------------------------------------

    /**
     * Stores one request. @p entry.musicId must name a real library row;
     * artist and title are read from that row rather than trusted from the
     * caller. Returns false and fills @p error when it was not stored.
     */
    static bool submit(Entry entry, QString *error = nullptr);

    /** Rows for the tray, newest first. @p pendingOnly keeps "new" ones only. */
    static QList<Entry> list(bool pendingOnly, int limit = 500);

    /** How many rows are still waiting for the operator. */
    static int pendingCount();

    /** Moves one row to queued/played/dismissed. */
    static bool setStatus(qint64 id, const QString &status);

    /** Removes handled rows older than @p days. Returns rows deleted. */
    static int purgeHandled(int days = 30);

    // --- untrusted text ---------------------------------------------------

    /**
     * What a string typed by a stranger becomes before XFB keeps it: control
     * characters and line breaks folded to spaces, runs of whitespace
     * collapsed, and the whole thing cut to @p maxChars.
     *
     * This is not escaping — escaping happens where the value is put into a
     * page — and it is not validation either. It exists so that a stored
     * value cannot be a screenful of newlines, a terminal escape sequence in
     * a log line, or a megabyte of anything.
     */
    static QString clean(const QString &raw, int maxChars);

    /** Longest a listener's name and dedication may be, after cleaning. */
    static constexpr int kMaxNameChars = 40;
    static constexpr int kMaxNoteChars = 180;
    /** Longest search text accepted; anything longer is simply cut. */
    static constexpr int kMaxQueryChars = 60;
};

#endif // REQUESTLINE_H
