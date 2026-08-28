#include "RequestLine.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>
#include <QDebug>

namespace {

const char *const kConnection = "xfb_connection";
const char *const kGroup      = "PublicPage";

/**
 * XFB opens its library on a *named* connection, so asking for the default
 * one finds nothing and every query here comes back empty against the real
 * application while a harness with no database at all passes.
 */
QSqlDatabase libraryDb()
{
    QSqlDatabase named = QSqlDatabase::database(QLatin1String(kConnection), false);
    if (named.isValid() && named.isOpen())
        return named;
    return QSqlDatabase::database();
}

/** The same xfb.conf every other XFB setting lives in. */
QString settingsFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

QVariant readSetting(const char *key, const QVariant &fallback)
{
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    const QVariant value = settings.value(QLatin1String(key), fallback);
    settings.endGroup();
    return value;
}

void writeSetting(const char *key, const QVariant &value)
{
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.beginGroup(QLatin1String(kGroup));
    settings.setValue(QLatin1String(key), value);
    settings.endGroup();
    settings.sync();
}

/** The timestamp format used here, matching the as-run log's. */
QString stamp(const QDateTime &when)
{
    if (!when.isValid())
        return QString();
    const QDateTime local = when.toLocalTime();
    return local.toOffsetFromUtc(local.offsetFromUtc()).toString(Qt::ISODate);
}

QDateTime unstamp(const QString &text)
{
    if (text.isEmpty())
        return QDateTime();
    QDateTime dt = QDateTime::fromString(text, Qt::ISODate);
    if (!dt.isValid())
        dt = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    return dt.isValid() ? dt.toLocalTime() : QDateTime();
}

bool isKnownStatus(const QString &status)
{
    return status == RequestLine::statusNew()
        || status == RequestLine::statusQueued()
        || status == RequestLine::statusPlayed()
        || status == RequestLine::statusDismissed();
}

} // namespace

// ------------------------------------------------------------------ schema

bool RequestLine::ensureSchema(QSqlDatabase db)
{
    if (!db.isValid() || !db.isOpen())
        return false;

    QSqlQuery create(db);
    const QString sql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS requests ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  received_at  TEXT NOT NULL,"
        "  music_id     INTEGER,"          // musics.rowid
        "  path         TEXT,"             // never served, never sent anywhere
        "  artist       TEXT,"
        "  title        TEXT,"
        "  requester    TEXT,"
        "  dedication   TEXT,"
        "  from_address TEXT,"
        "  status       TEXT NOT NULL DEFAULT 'new',"
        "  handled_at   TEXT"
        ")");
    if (!create.exec(sql)) {
        qWarning() << "RequestLine: could not create the requests table:"
                   << create.lastError().text();
        return false;
    }

    QSqlQuery index(db);
    if (!index.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_requests_status "
            "ON requests(status, received_at)"))) {
        qWarning() << "RequestLine: could not create the requests index:"
                   << index.lastError().text();
    }
    return true;
}

// ---------------------------------------------------------------- settings

bool RequestLine::pageEnabled()
{
    // Off until the operator says otherwise. Every route under /public/
    // answers 404 while this is false, which is the whole "off by default"
    // promise in one line.
    return readSetting("Enabled", false).toBool();
}

void RequestLine::setPageEnabled(bool enabled)
{
    writeSetting("Enabled", enabled);
}

bool RequestLine::requestsEnabled()
{
    return readSetting("Requests", false).toBool();
}

void RequestLine::setRequestsEnabled(bool enabled)
{
    writeSetting("Requests", enabled);
}

QString RequestLine::stationName()
{
    return clean(readSetting("StationName", QString()).toString(), 60);
}

void RequestLine::setStationName(const QString &name)
{
    writeSetting("StationName", clean(name, 60));
}

QString RequestLine::tagline()
{
    return clean(readSetting("Tagline", QString()).toString(), 120);
}

void RequestLine::setTagline(const QString &text)
{
    writeSetting("Tagline", clean(text, 120));
}

int RequestLine::maxPending()
{
    const int value = readSetting("MaxPending", 200).toInt();
    return qBound(10, value, 5000);
}

void RequestLine::setMaxPending(int count)
{
    writeSetting("MaxPending", qBound(10, count, 5000));
}

// ------------------------------------------------------------ the library

QList<RequestLine::Match> RequestLine::search(const QString &text, int limit)
{
    QList<Match> out;

    const QString needle = clean(text, kMaxQueryChars);
    if (needle.size() < 2)
        return out;   // one letter matches half a library; not worth the scan

    QSqlDatabase db = libraryDb();
    if (!db.isValid() || !db.isOpen())
        return out;

    // rowid rather than the "id" column: musics declares id as a plain
    // INTEGER, not INTEGER PRIMARY KEY, so rows added by XFB itself leave it
    // NULL. rowid is always there and always unique.
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT rowid, artist, song FROM musics "
        "WHERE path IS NOT NULL AND path <> '' "
        "  AND (artist LIKE :a ESCAPE '\\' OR song LIKE :b ESCAPE '\\') "
        "ORDER BY artist COLLATE NOCASE, song COLLATE NOCASE "
        "LIMIT :limit"));

    // The listener's text is bound, never interpolated. LIKE's own wildcards
    // are escaped as well, so a query of "%" asks for tracks containing a
    // percent sign rather than for the whole catalogue.
    QString escaped = needle;
    escaped.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    escaped.replace(QLatin1Char('%'),  QLatin1String("\\%"));
    escaped.replace(QLatin1Char('_'),  QLatin1String("\\_"));
    const QString like = QStringLiteral("%") + escaped + QStringLiteral("%");

    query.bindValue(QStringLiteral(":a"), like);
    query.bindValue(QStringLiteral(":b"), like);
    query.bindValue(QStringLiteral(":limit"), qBound(1, limit, 100));

    if (!query.exec()) {
        qWarning() << "RequestLine: search failed:" << query.lastError().text();
        return out;
    }

    while (query.next()) {
        Match match;
        match.musicId = query.value(0).toLongLong();
        match.artist  = clean(query.value(1).toString(), 120);
        match.title   = clean(query.value(2).toString(), 120);
        if (match.title.isEmpty() && match.artist.isEmpty())
            continue;
        out.append(match);
    }
    return out;
}

QString RequestLine::pathForMusicId(qint64 musicId)
{
    QSqlDatabase db = libraryDb();
    if (musicId <= 0 || !db.isValid() || !db.isOpen())
        return QString();

    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT path FROM musics WHERE rowid = :id"));
    query.bindValue(QStringLiteral(":id"), musicId);
    if (!query.exec() || !query.next())
        return QString();
    return query.value(0).toString();
}

// ----------------------------------------------------------- the requests

bool RequestLine::submit(Entry entry, QString *error)
{
    auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    QSqlDatabase db = libraryDb();
    if (!db.isValid() || !db.isOpen())
        return fail(QStringLiteral("The library is not open."));

    if (entry.musicId <= 0)
        return fail(QStringLiteral("That track is not in the library."));

    // Artist and title are read from the library row, never taken from the
    // caller: what the operator reads in the tray is what XFB itself holds,
    // so a submitted string cannot pretend to be a track that exists.
    QSqlQuery lookup(db);
    lookup.prepare(QStringLiteral(
        "SELECT artist, song, path FROM musics WHERE rowid = :id"));
    lookup.bindValue(QStringLiteral(":id"), entry.musicId);
    if (!lookup.exec() || !lookup.next())
        return fail(QStringLiteral("That track is not in the library."));

    entry.artist = clean(lookup.value(0).toString(), 120);
    entry.title  = clean(lookup.value(1).toString(), 120);
    entry.path   = lookup.value(2).toString();
    if (entry.path.isEmpty())
        return fail(QStringLiteral("That track is not in the library."));

    entry.requester   = clean(entry.requester, kMaxNameChars);
    entry.dedication  = clean(entry.dedication, kMaxNoteChars);
    entry.fromAddress = clean(entry.fromAddress, 45);   // longest IPv6 literal
    if (!entry.receivedAt.isValid())
        entry.receivedAt = QDateTime::currentDateTime();

    if (pendingCount() >= maxPending())
        return fail(QStringLiteral("The request list is full just now."));

    QSqlQuery insert(db);
    insert.prepare(QStringLiteral(
        "INSERT INTO requests (received_at, music_id, path, artist, title, "
        "                      requester, dedication, from_address, status) "
        "VALUES (:received, :music, :path, :artist, :title, :requester, "
        "        :dedication, :address, :status)"));
    insert.bindValue(QStringLiteral(":received"),   stamp(entry.receivedAt));
    insert.bindValue(QStringLiteral(":music"),      entry.musicId);
    insert.bindValue(QStringLiteral(":path"),       entry.path);
    insert.bindValue(QStringLiteral(":artist"),     entry.artist);
    insert.bindValue(QStringLiteral(":title"),      entry.title);
    insert.bindValue(QStringLiteral(":requester"),  entry.requester);
    insert.bindValue(QStringLiteral(":dedication"), entry.dedication);
    insert.bindValue(QStringLiteral(":address"),    entry.fromAddress);
    insert.bindValue(QStringLiteral(":status"),     statusNew());

    if (!insert.exec()) {
        qWarning() << "RequestLine: could not store a request:"
                   << insert.lastError().text();
        return fail(QStringLiteral("XFB could not store that request."));
    }
    return true;
}

QList<RequestLine::Entry> RequestLine::list(bool pendingOnly, int limit)
{
    QList<Entry> out;
    QSqlDatabase db = libraryDb();
    if (!db.isValid() || !db.isOpen())
        return out;

    QString sql = QStringLiteral(
        "SELECT id, received_at, music_id, path, artist, title, requester, "
        "       dedication, from_address, status, handled_at FROM requests");
    if (pendingOnly)
        sql += QStringLiteral(" WHERE status = :status");
    sql += QStringLiteral(" ORDER BY received_at DESC, id DESC LIMIT :limit");

    QSqlQuery query(db);
    query.prepare(sql);
    if (pendingOnly)
        query.bindValue(QStringLiteral(":status"), statusNew());
    query.bindValue(QStringLiteral(":limit"), qBound(1, limit, 5000));

    if (!query.exec()) {
        qWarning() << "RequestLine: could not read the requests:"
                   << query.lastError().text();
        return out;
    }

    while (query.next()) {
        Entry entry;
        entry.id          = query.value(0).toLongLong();
        entry.receivedAt  = unstamp(query.value(1).toString());
        entry.musicId     = query.value(2).isNull() ? -1 : query.value(2).toLongLong();
        entry.path        = query.value(3).toString();
        entry.artist      = query.value(4).toString();
        entry.title       = query.value(5).toString();
        entry.requester   = query.value(6).toString();
        entry.dedication  = query.value(7).toString();
        entry.fromAddress = query.value(8).toString();
        entry.status      = query.value(9).toString();
        entry.handledAt   = unstamp(query.value(10).toString());
        out.append(entry);
    }
    return out;
}

int RequestLine::pendingCount()
{
    QSqlDatabase db = libraryDb();
    if (!db.isValid() || !db.isOpen())
        return 0;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM requests WHERE status = :status"));
    query.bindValue(QStringLiteral(":status"), statusNew());
    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}

bool RequestLine::setStatus(qint64 id, const QString &status)
{
    QSqlDatabase db = libraryDb();
    if (id <= 0 || !db.isValid() || !db.isOpen())
        return false;
    if (!isKnownStatus(status))
        return false;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE requests SET status = :status, handled_at = :handled "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":status"), status);
    query.bindValue(QStringLiteral(":handled"),
                    status == statusNew() ? QString()
                                          : stamp(QDateTime::currentDateTime()));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        qWarning() << "RequestLine: could not update a request:"
                   << query.lastError().text();
        return false;
    }
    return true;
}

int RequestLine::purgeHandled(int days)
{
    QSqlDatabase db = libraryDb();
    if (days <= 0 || !db.isValid() || !db.isOpen())
        return 0;

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "DELETE FROM requests WHERE status <> :new AND received_at < :cutoff"));
    query.bindValue(QStringLiteral(":new"), statusNew());
    query.bindValue(QStringLiteral(":cutoff"),
                    stamp(QDateTime::currentDateTime().addDays(-days)));
    if (!query.exec())
        return 0;
    return query.numRowsAffected();
}

// ------------------------------------------------------------ untrusted text

QString RequestLine::clean(const QString &raw, int maxChars)
{
    QString out;
    out.reserve(qMin(raw.size(), maxChars));

    bool lastWasSpace = true;   // also trims the leading run
    for (const QChar character : raw) {
        if (out.size() >= maxChars)
            break;

        QChar ch = character;

        // Anything that is not printable text becomes a space: control
        // characters, newlines, and the bidirectional and zero-width
        // formatting marks that let a string on screen read as something
        // other than what is stored. Surrogates are left alone — a name in
        // a non-BMP script is two of them and is perfectly legitimate.
        const QChar::Category category = ch.category();
        if (ch.isSpace() || category == QChar::Other_Control
            || category == QChar::Other_Format) {
            ch = QLatin1Char(' ');
        }

        if (ch == QLatin1Char(' ')) {
            if (lastWasSpace)
                continue;       // collapse runs
            lastWasSpace = true;
        } else {
            lastWasSpace = false;
        }
        out.append(ch);
    }

    // Nothing here escapes anything: an apostrophe, a quote and an angle
    // bracket are all perfectly good things to have in a name, and they are
    // made safe where they are used — bound as SQL parameters on the way in,
    // HTML-escaped on the way out.
    return out.trimmed();
}
