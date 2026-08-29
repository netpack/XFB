#include "AirLog.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QVariant>

namespace {

const char *const kConnection = "xfb_connection";

QSqlDatabase logDb()
{
    return QSqlDatabase::database(QLatin1String(kConnection));
}

/** The same xfb.conf every other XFB setting lives in. */
QString settingsFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/** "h:mm:ss" as the library stores it, in milliseconds. -1 when unusable. */
qint64 msFromLibraryTime(const QString &text)
{
    const QStringList parts = text.trimmed().split(QLatin1Char(':'));
    if (parts.size() < 2 || parts.size() > 3)
        return -1;
    bool ok = false;
    qint64 total = 0;
    for (const QString &part : parts) {
        const int value = part.toInt(&ok);
        if (!ok || value < 0)
            return -1;
        total = total * 60 + value;
    }
    return total * 1000;
}

} // namespace

AirLog::AirLog(QObject *parent)
    : QObject(parent)
{
    // A clean quit must not leave rows looking like a crash.
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        closeAll(QStringLiteral("stopped"));
        flush();
    });
}

AirLog *AirLog::instance()
{
    static AirLog *self = new AirLog(qApp);
    return self;
}

// ---------------------------------------------------------------- schema ---

bool AirLog::ensureSchema(QSqlDatabase db)
{
    if (!db.isOpen())
        return false;

    QSqlQuery qry(db);
    // The column set is fixed: other parts of XFB read these rows, so the
    // shape has to be the same on a fresh install and on an old one.
    const char *const kCreate =
        "CREATE TABLE IF NOT EXISTS airlog ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  started_at TEXT NOT NULL,"
        "  ended_at TEXT,"
        "  source TEXT NOT NULL,"
        "  source_id INTEGER,"
        "  artist TEXT, title TEXT, path TEXT,"
        "  planned_ms INTEGER, played_ms INTEGER,"
        "  ended_reason TEXT,"
        "  operator_mode TEXT"
        ")";
    if (!qry.exec(QLatin1String(kCreate))) {
        qWarning() << "AirLog: could not create the airlog table:" << qry.lastError().text();
        return false;
    }
    if (!qry.exec(QLatin1String("CREATE INDEX IF NOT EXISTS airlog_started "
                                "ON airlog(started_at)"))) {
        qWarning() << "AirLog: could not create airlog_started:" << qry.lastError().text();
        return false;
    }

    // started_at is ISO 8601 carrying the local UTC offset, which reads well
    // and compares badly: "…T01:30:00+01:00" sorts after "…T01:15:00+00:00"
    // as text although it happened fifteen minutes EARLIER. One hour of every
    // year — the one the clocks go back — a range filter therefore returns the
    // wrong rows, and it is the advertiser report that bills from them. So the
    // instant is also kept as seconds since the epoch, which is monotonic
    // through any clock change, and every range and ordering uses that.
    if (!db.record(QStringLiteral("airlog")).contains(QStringLiteral("started_epoch"))) {
        if (!qry.exec(QLatin1String("ALTER TABLE airlog ADD COLUMN started_epoch INTEGER"))) {
            qWarning() << "AirLog: could not add started_epoch:" << qry.lastError().text();
            return false;
        }
    }
    // SQLite parses the offset itself, so existing rows convert exactly.
    if (!qry.exec(QLatin1String("UPDATE airlog SET started_epoch = "
                                "CAST(strftime('%s', started_at) AS INTEGER) "
                                "WHERE started_epoch IS NULL AND started_at IS NOT NULL"))) {
        qWarning() << "AirLog: could not backfill started_epoch:" << qry.lastError().text();
    } else if (qry.numRowsAffected() > 0) {
        qInfo() << "AirLog: dated" << qry.numRowsAffected() << "existing entries";
    }
    if (!qry.exec(QLatin1String("CREATE INDEX IF NOT EXISTS airlog_started_epoch "
                                "ON airlog(started_epoch)"))) {
        qWarning() << "AirLog: could not create airlog_started_epoch:"
                   << qry.lastError().text();
        return false;
    }
    return true;
}

void AirLog::start()
{
    if (m_started)
        return;
    QSqlDatabase db = logDb();
    if (!db.isOpen())
        return;             // retried on the next call
    m_started = true;

    // Crash recovery. A row with no ended_at is one that was on air when the
    // process died. Guessing "it ran to the end" would invent airtime that
    // may never have happened, so close it at the last position the
    // heartbeat managed to record, and mark why: nobody knows what came out
    // of the transmitter after that point.
    QSqlQuery dangling(db);
    dangling.prepare(QStringLiteral(
        "SELECT id, started_at, played_ms FROM airlog WHERE ended_at IS NULL"));
    if (!dangling.exec()) {
        qWarning() << "AirLog: could not look for unclosed rows:" << dangling.lastError().text();
    } else {
        struct Repair { qint64 id; QString endedAt; qint64 playedMs; };
        QList<Repair> repairs;
        while (dangling.next()) {
            const qint64 id = dangling.value(0).toLongLong();
            const QDateTime startedAt = parseTimestamp(dangling.value(1).toString());
            const qint64 playedMs = dangling.value(2).isNull()
                                        ? 0 : dangling.value(2).toLongLong();
            if (!startedAt.isValid())
                continue;
            repairs.append({ id,
                             formatTimestamp(startedAt.addMSecs(qMax<qint64>(0, playedMs))),
                             playedMs });
        }
        if (!repairs.isEmpty()) {
            db.transaction();
            QSqlQuery fix(db);
            fix.prepare(QStringLiteral(
                "UPDATE airlog SET ended_at = :ended, played_ms = :played, "
                "ended_reason = 'deadair' WHERE id = :id"));
            for (const Repair &r : repairs) {
                fix.bindValue(QStringLiteral(":ended"), r.endedAt);
                fix.bindValue(QStringLiteral(":played"), r.playedMs);
                fix.bindValue(QStringLiteral(":id"), r.id);
                if (!fix.exec())
                    qWarning() << "AirLog: could not close row" << r.id
                               << ":" << fix.lastError().text();
            }
            db.commit();
            qInfo() << "AirLog: closed" << repairs.size()
                    << "row(s) left open by an unclean shutdown";
        }
    }

    const int pruned = prune();
    if (pruned > 0)
        qInfo() << "AirLog: pruned" << pruned << "row(s) past the retention window";
}

// ------------------------------------------------------------ timestamps ---

QString AirLog::formatTimestamp(const QDateTime &when)
{
    if (!when.isValid())
        return QString();
    // ISO 8601 with the local offset spelled out. Without the offset a log
    // read a year later cannot be told apart across a daylight-saving change,
    // and "did the spot run at 08:15" is exactly the question being asked.
    const QDateTime local = when.toLocalTime();
    return local.toOffsetFromUtc(local.offsetFromUtc()).toString(Qt::ISODate);
}

QDateTime AirLog::parseTimestamp(const QString &text)
{
    if (text.isEmpty())
        return QDateTime();
    QDateTime dt = QDateTime::fromString(text, Qt::ISODate);
    if (!dt.isValid())  // logs written before the offset was carried
        dt = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    if (!dt.isValid())
        return QDateTime();
    return dt.toLocalTime();
}

// ------------------------------------------------------------ classifying --

AirLog::Entry AirLog::entryForPath(const QString &path, bool autoMode)
{
    Entry entry;
    entry.path = path;
    entry.autoMode = autoMode;
    entry.source = QStringLiteral("fallback");

    QSqlDatabase db = logDb();
    if (path.isEmpty() || !db.isOpen()) {
        entry.title = path.section(QLatin1Char('/'), -1);
        return entry;
    }

    // musics first: it is what plays most of the time, and it is the only
    // table that carries an artist.
    QSqlQuery qry(db);
    qry.prepare(QStringLiteral("SELECT rowid, id, artist, song, time FROM musics "
                               "WHERE path = :path LIMIT 1"));
    qry.bindValue(QStringLiteral(":path"), path);
    if (qry.exec() && qry.next()) {
        entry.source = QStringLiteral("musics");
        entry.sourceId = qry.value(1).isNull() ? qry.value(0).toLongLong()
                                               : qry.value(1).toLongLong();
        entry.artist = qry.value(2).toString();
        entry.title = qry.value(3).toString();
        entry.plannedMs = msFromLibraryTime(qry.value(4).toString());
        return entry;
    }

    // The other three are name/path only. jingles has no id column at all, so
    // its rowid is the row id an aired jingle is identified by.
    struct Lookup { const char *table; const char *sql; };
    static const Lookup kLookups[] = {
        { "jingles",  "SELECT rowid, name FROM jingles  WHERE path = :path LIMIT 1" },
        { "pub",      "SELECT id,    name FROM pub      WHERE path = :path LIMIT 1" },
        { "programs", "SELECT id,    name FROM programs WHERE path = :path LIMIT 1" },
    };
    for (const Lookup &lookup : kLookups) {
        QSqlQuery other(db);
        other.prepare(QLatin1String(lookup.sql));
        other.bindValue(QStringLiteral(":path"), path);
        if (other.exec() && other.next()) {
            entry.source = QLatin1String(lookup.table);
            entry.sourceId = other.value(0).toLongLong();
            entry.title = other.value(1).toString();
            return entry;
        }
    }

    // Aired, but nothing in the library claims it — a file dropped straight
    // into the running order, say. It still went out, so it is still logged.
    entry.title = path.section(QLatin1Char('/'), -1);
    return entry;
}

// ---------------------------------------------------------------- writing --

qint64 AirLog::open(const Entry &entry)
{
    if (entry.source.isEmpty())
        return 0;

    const qint64 handle = m_nextHandle++;
    Op op;
    op.kind = Op::Open;
    op.handle = handle;
    op.entry = entry;
    op.stamp = QDateTime::currentDateTime();
    m_pending.append(op);
    m_lastBeat.remove(handle);
    scheduleFlush();
    return handle;
}

qint64 AirLog::openPath(const QString &path, bool autoMode, qint64 plannedMs)
{
    Entry entry = entryForPath(path, autoMode);
    if (plannedMs > 0)
        entry.plannedMs = plannedMs;
    return open(entry);
}

void AirLog::setPlannedMs(qint64 handle, qint64 plannedMs)
{
    if (handle <= 0 || plannedMs <= 0)
        return;
    Op op;
    op.kind = Op::Planned;
    op.handle = handle;
    op.value = plannedMs;
    m_pending.append(op);
    scheduleFlush();
}

void AirLog::heartbeat(qint64 handle, qint64 playedMs)
{
    if (handle <= 0 || playedMs < 0)
        return;
    QElapsedTimer &timer = m_lastBeat[handle];
    if (timer.isValid() && timer.elapsed() < kHeartbeatMs)
        return;                 // the position tick fires ten times a second
    timer.start();

    Op op;
    op.kind = Op::Position;
    op.handle = handle;
    op.value = playedMs;
    m_pending.append(op);
    scheduleFlush();
}

void AirLog::close(qint64 handle, qint64 playedMs, const QString &reason)
{
    if (handle <= 0)
        return;
    Op op;
    op.kind = Op::Close;
    op.handle = handle;
    op.value = playedMs;   // < 0: keep whatever the heartbeat last recorded
    op.reason = reason.isEmpty() ? QStringLiteral("end") : reason;
    op.stamp = QDateTime::currentDateTime();
    m_pending.append(op);
    m_lastBeat.remove(handle);
    scheduleFlush();
}

void AirLog::closeAll(const QString &reason)
{
    // Anything this process opened and never closed. Handles still queued for
    // insertion count too — flush() writes them in order, so the close lands.
    QSet<qint64> open;
    for (auto it = m_rowIds.constBegin(); it != m_rowIds.constEnd(); ++it)
        open.insert(it.key());
    for (const Op &op : std::as_const(m_pending)) {
        if (op.kind == Op::Open)
            open.insert(op.handle);
        else if (op.kind == Op::Close)
            open.remove(op.handle);
    }
    for (const qint64 handle : std::as_const(open))
        close(handle, -1, reason);
}

void AirLog::scheduleFlush()
{
    if (m_flushQueued)
        return;
    m_flushQueued = true;
    // Zero-timer: the caller is inside a media-player signal, and the point
    // of the whole queue is that the write does not happen there.
    QTimer::singleShot(0, this, [this]() { flush(); });
}

void AirLog::flush()
{
    m_flushQueued = false;
    if (m_pending.isEmpty())
        return;

    QSqlDatabase db = logDb();
    if (!db.isOpen())
        return;                 // keep the queue; the next flush retries

    const QList<Op> batch = m_pending;
    m_pending.clear();

    const bool inTransaction = batch.size() > 1 && db.transaction();

    for (const Op &op : batch) {
        switch (op.kind) {
        case Op::Open: {
            QSqlQuery insert(db);
            insert.prepare(QStringLiteral(
                "INSERT INTO airlog (started_at, started_epoch, source, source_id, "
                "artist, title, path, planned_ms, played_ms, operator_mode) "
                "VALUES (:started, :startedEpoch, :source, :sourceId, :artist, :title, "
                ":path, :planned, 0, :mode)"));
            insert.bindValue(QStringLiteral(":started"), formatTimestamp(op.stamp));
            insert.bindValue(QStringLiteral(":startedEpoch"), op.stamp.toSecsSinceEpoch());
            insert.bindValue(QStringLiteral(":source"), op.entry.source);
            insert.bindValue(QStringLiteral(":sourceId"),
                             op.entry.sourceId >= 0 ? QVariant(op.entry.sourceId)
                                                    : QVariant(QMetaType(QMetaType::LongLong)));
            insert.bindValue(QStringLiteral(":artist"), op.entry.artist);
            insert.bindValue(QStringLiteral(":title"), op.entry.title);
            insert.bindValue(QStringLiteral(":path"), op.entry.path);
            insert.bindValue(QStringLiteral(":planned"),
                             op.entry.plannedMs > 0 ? QVariant(op.entry.plannedMs)
                                                    : QVariant(QMetaType(QMetaType::LongLong)));
            insert.bindValue(QStringLiteral(":mode"),
                             op.entry.autoMode ? QStringLiteral("auto")
                                               : QStringLiteral("manual"));
            if (!insert.exec()) {
                qWarning() << "AirLog: insert failed:" << insert.lastError().text();
                break;
            }
            m_rowIds.insert(op.handle, insert.lastInsertId().toLongLong());
            break;
        }
        case Op::Close: {
            const qint64 rowId = m_rowIds.value(op.handle, 0);
            if (rowId <= 0)
                break;
            QSqlQuery upd(db);
            // played_ms: a close that does not know how far it got (a clean
            // quit, say) keeps whatever the heartbeat last recorded.
            upd.prepare(QStringLiteral(
                "UPDATE airlog SET ended_at = :ended, ended_reason = :reason, "
                "played_ms = CASE WHEN :playedTest < 0 THEN played_ms ELSE :playedSet END "
                "WHERE id = :id AND ended_at IS NULL"));
            upd.bindValue(QStringLiteral(":ended"), formatTimestamp(op.stamp));
            upd.bindValue(QStringLiteral(":reason"), op.reason);
            upd.bindValue(QStringLiteral(":playedTest"), op.value);
            upd.bindValue(QStringLiteral(":playedSet"), op.value);
            upd.bindValue(QStringLiteral(":id"), rowId);
            if (!upd.exec())
                qWarning() << "AirLog: close failed:" << upd.lastError().text();
            m_rowIds.remove(op.handle);
            break;
        }
        case Op::Planned: {
            const qint64 rowId = m_rowIds.value(op.handle, 0);
            if (rowId <= 0)
                break;
            QSqlQuery upd(db);
            upd.prepare(QStringLiteral("UPDATE airlog SET planned_ms = :planned "
                                       "WHERE id = :id"));
            upd.bindValue(QStringLiteral(":planned"), op.value);
            upd.bindValue(QStringLiteral(":id"), rowId);
            if (!upd.exec())
                qWarning() << "AirLog: planned_ms failed:" << upd.lastError().text();
            break;
        }
        case Op::Position: {
            const qint64 rowId = m_rowIds.value(op.handle, 0);
            if (rowId <= 0)
                break;
            QSqlQuery upd(db);
            // The crash breadcrumb: if the process dies now, start() closes
            // the row here rather than pretending the track ran to the end.
            upd.prepare(QStringLiteral("UPDATE airlog SET played_ms = :played "
                                       "WHERE id = :id AND ended_at IS NULL"));
            upd.bindValue(QStringLiteral(":played"), op.value);
            upd.bindValue(QStringLiteral(":id"), rowId);
            if (!upd.exec())
                qWarning() << "AirLog: heartbeat failed:" << upd.lastError().text();
            break;
        }
        }
    }

    if (inTransaction)
        db.commit();
}

// ---------------------------------------------------------------- reading --

QStringList AirLog::sourceKeys()
{
    return { QStringLiteral("musics"),   QStringLiteral("jingles"),
             QStringLiteral("pub"),      QStringLiteral("programs"),
             QStringLiteral("pad"),      QStringLiteral("stream"),
             QStringLiteral("fallback") };
}

QString AirLog::sourceLabel(const QString &key)
{
    if (key == QLatin1String("musics"))   return tr("Music");
    if (key == QLatin1String("jingles"))  return tr("Jingle");
    if (key == QLatin1String("pub"))      return tr("Advertisement");
    if (key == QLatin1String("programs")) return tr("Programme");
    if (key == QLatin1String("pad"))      return tr("Pad");
    if (key == QLatin1String("stream"))   return tr("Stream");
    if (key == QLatin1String("fallback")) return tr("Other file");
    return key;
}

QString AirLog::reasonLabel(const QString &key)
{
    if (key == QLatin1String("segue"))   return tr("Segued");
    if (key == QLatin1String("end"))     return tr("Played out");
    if (key == QLatin1String("skipped")) return tr("Skipped");
    if (key == QLatin1String("stopped")) return tr("Stopped");
    if (key == QLatin1String("deadair")) return tr("Interrupted");
    if (key.isEmpty())                   return tr("On air");
    return key;
}

static AirLog::Record recordFromQuery(const QSqlQuery &qry)
{
    AirLog::Record rec;
    rec.id = qry.value(0).toLongLong();
    rec.startedAt = AirLog::parseTimestamp(qry.value(1).toString());
    rec.endedAt = AirLog::parseTimestamp(qry.value(2).toString());
    rec.source = qry.value(3).toString();
    rec.sourceId = qry.value(4).isNull() ? -1 : qry.value(4).toLongLong();
    rec.artist = qry.value(5).toString();
    rec.title = qry.value(6).toString();
    rec.path = qry.value(7).toString();
    rec.plannedMs = qry.value(8).isNull() ? -1 : qry.value(8).toLongLong();
    rec.playedMs = qry.value(9).isNull() ? -1 : qry.value(9).toLongLong();
    rec.endedReason = qry.value(10).toString();
    rec.operatorMode = qry.value(11).toString();
    return rec;
}

static const char *const kSelectColumns =
    "SELECT id, started_at, ended_at, source, source_id, artist, title, path, "
    "planned_ms, played_ms, ended_reason, operator_mode FROM airlog";

QList<AirLog::Record> AirLog::query(const Filter &filter)
{
    QList<Record> out;
    QSqlDatabase db = logDb();
    if (!db.isOpen())
        return out;

    QString sql = QLatin1String(kSelectColumns);
    QStringList where;
    if (filter.from.isValid())
        where << QStringLiteral("started_epoch >= :from");
    if (filter.to.isValid())
        where << QStringLiteral("started_epoch <= :to");
    if (!filter.source.isEmpty())
        where << QStringLiteral("source = :source");
    if (!filter.text.isEmpty())
        where << QStringLiteral("(IFNULL(artist,'') LIKE :textA OR IFNULL(title,'') "
                                "LIKE :textB OR IFNULL(path,'') LIKE :textC)");
    if (!where.isEmpty())
        sql += QStringLiteral(" WHERE ") + where.join(QStringLiteral(" AND "));
    sql += QStringLiteral(" ORDER BY started_epoch DESC, id DESC");
    if (filter.limit > 0)
        sql += QStringLiteral(" LIMIT :limit");

    QSqlQuery qry(db);
    qry.prepare(sql);
    if (filter.from.isValid())
        qry.bindValue(QStringLiteral(":from"), filter.from.toSecsSinceEpoch());
    if (filter.to.isValid())
        qry.bindValue(QStringLiteral(":to"), filter.to.toSecsSinceEpoch());
    if (!filter.source.isEmpty())
        qry.bindValue(QStringLiteral(":source"), filter.source);
    if (!filter.text.isEmpty()) {
        const QString like = QStringLiteral("%") + filter.text + QStringLiteral("%");
        qry.bindValue(QStringLiteral(":textA"), like);
        qry.bindValue(QStringLiteral(":textB"), like);
        qry.bindValue(QStringLiteral(":textC"), like);
    }
    if (filter.limit > 0)
        qry.bindValue(QStringLiteral(":limit"), filter.limit);

    if (!qry.exec()) {
        qWarning() << "AirLog: query failed:" << qry.lastError().text();
        return out;
    }
    while (qry.next())
        out.append(recordFromQuery(qry));
    return out;
}

QList<QPair<qint64, QString>> AirLog::advertisements()
{
    QList<QPair<qint64, QString>> out;
    QSqlDatabase db = logDb();
    if (!db.isOpen())
        return out;
    QSqlQuery qry(db);
    if (!qry.exec(QStringLiteral("SELECT id, name FROM pub ORDER BY name COLLATE NOCASE"))) {
        qWarning() << "AirLog: could not list advertisements:" << qry.lastError().text();
        return out;
    }
    while (qry.next())
        out.append({ qry.value(0).toLongLong(), qry.value(1).toString() });
    return out;
}

QList<AirLog::Record> AirLog::advertiserReport(qint64 pubId, const QDateTime &from,
                                                const QDateTime &to)
{
    QList<Record> out;
    QSqlDatabase db = logDb();
    if (!db.isOpen())
        return out;

    QString sql = QLatin1String(kSelectColumns)
                  + QStringLiteral(" WHERE source = 'pub' AND source_id = :id");
    if (from.isValid())
        sql += QStringLiteral(" AND started_epoch >= :from");
    if (to.isValid())
        sql += QStringLiteral(" AND started_epoch <= :to");
    // Oldest first: an invoice reads down the month, not up it.
    sql += QStringLiteral(" ORDER BY started_epoch ASC, id ASC");

    QSqlQuery qry(db);
    qry.prepare(sql);
    qry.bindValue(QStringLiteral(":id"), pubId);
    if (from.isValid())
        qry.bindValue(QStringLiteral(":from"), from.toSecsSinceEpoch());
    if (to.isValid())
        qry.bindValue(QStringLiteral(":to"), to.toSecsSinceEpoch());
    if (!qry.exec()) {
        qWarning() << "AirLog: advertiser report failed:" << qry.lastError().text();
        return out;
    }
    while (qry.next())
        out.append(recordFromQuery(qry));
    return out;
}

// -------------------------------------------------------------- retention --

int AirLog::retentionDays()
{
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("AirLog"));
    const int days = settings.value(QStringLiteral("RetentionDays"),
                                    kDefaultRetentionDays).toInt();
    settings.endGroup();
    return qMax(0, days);
}

void AirLog::setRetentionDays(int days)
{
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("AirLog"));
    settings.setValue(QStringLiteral("RetentionDays"), qMax(0, days));
    settings.endGroup();
    settings.sync();
}

int AirLog::prune()
{
    const int days = retentionDays();
    if (days <= 0)
        return 0;               // 0 means keep it all, which is a valid answer

    QSqlDatabase db = logDb();
    if (!db.isOpen())
        return 0;

    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-days);
    QSqlQuery qry(db);
    qry.prepare(QStringLiteral("DELETE FROM airlog WHERE started_epoch < :cutoff"));
    qry.bindValue(QStringLiteral(":cutoff"), cutoff.toSecsSinceEpoch());
    if (!qry.exec()) {
        qWarning() << "AirLog: prune failed:" << qry.lastError().text();
        return 0;
    }
    return qry.numRowsAffected();
}
