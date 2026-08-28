#include "IntroLibrary.h"

#include "IntroDetector.h"
#include "WaveformStore.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <utility>

namespace
{
// The one connection the rest of the app opens the music database on.
const char *kConnectionName = "xfb_connection";

// Decodes in flight at once. WaveformStore runs two ffmpeg processes in
// parallel and queues the rest; keeping the backlog short is what stops a
// sweep over a big library from holding thousands of waveforms in memory.
constexpr int kMaxInFlight = 4;

QSqlQuery libraryQuery()
{
    return QSqlQuery(QSqlDatabase::database(QString::fromLatin1(kConnectionName)));
}

/** False when the database has not been migrated yet (an install whose
 *  checkDbOpen() has not run, or a harness with a bare schema). Every
 *  public entry point checks it, so nothing here ever throws SQL errors at
 *  a database that simply does not have the columns. */
bool schemaReady()
{
    const QSqlRecord rec =
        QSqlDatabase::database(QString::fromLatin1(kConnectionName)).record(
            QStringLiteral("musics"));
    return rec.contains(QStringLiteral("intro_ms"))
        && rec.contains(QStringLiteral("outro_ms"))
        && rec.contains(QStringLiteral("intro_locked"));
}
} // namespace

IntroLibrary::IntroLibrary(WaveformStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
    if (m_store) {
        connect(m_store, &WaveformStore::waveformReady,
                this, &IntroLibrary::onWaveformReady);
    }
}

void IntroLibrary::setThresholdPercent(int percent)
{
    m_thresholdPercent = qBound(1, percent, 50);
}

IntroTimes IntroLibrary::timesFor(const QString &filePath) const
{
    IntroTimes out;
    if (filePath.isEmpty() || !schemaReady())
        return out;

    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral(
        "SELECT intro_ms, outro_ms, intro_locked FROM musics WHERE path = :path"));
    query.bindValue(QStringLiteral(":path"), filePath);
    if (!query.exec() || !query.next())
        return out;

    out.known = true;
    if (!query.value(0).isNull())
        out.introMs = query.value(0).toLongLong();
    if (!query.value(1).isNull())
        out.outroMs = query.value(1).toLongLong();
    out.locked = query.value(2).toInt() != 0;
    return out;
}

namespace
{
QStringList pathsOf(QSqlQuery &&query, const char *what)
{
    QStringList paths;
    if (!query.exec()) {
        qWarning() << "IntroLibrary: could not list" << what << query.lastError().text();
        return paths;
    }
    while (query.next()) {
        const QString path = query.value(0).toString();
        if (!path.isEmpty())
            paths.append(path);
    }
    return paths;
}
} // namespace

QStringList IntroLibrary::tracksMissingIntro() const
{
    if (!schemaReady())
        return {};
    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral("SELECT path FROM musics WHERE intro_ms IS NULL"));
    return pathsOf(std::move(query), "tracks without an intro:");
}

QStringList IntroLibrary::allTracks() const
{
    if (!schemaReady())
        return {};
    QSqlQuery query = libraryQuery();
    // Hand-set rows are deliberately still in this list: they are counted
    // as skipped when store() refuses them, which is how the operator gets
    // told how many of their own values a forced re-run left alone.
    query.prepare(QStringLiteral("SELECT path FROM musics"));
    return pathsOf(std::move(query), "library tracks:");
}

void IntroLibrary::analyze(const QStringList &filePaths)
{
    if (m_active || !m_store)
        return;

    m_queue.clear();
    QSet<QString> seen;
    for (const QString &path : filePaths) {
        if (!path.isEmpty() && !seen.contains(path)) {
            seen.insert(path);
            m_queue.append(path);
        }
    }

    m_total = m_queue.size();
    m_done = 0;
    m_measured = 0;
    m_skipped = 0;
    m_active = true;

    emit progress(0, m_total);
    pump();
}

void IntroLibrary::cancel()
{
    if (!m_active)
        return;
    // Decodes already handed to WaveformStore keep running in the
    // background — there is no job cancel there — but they only warm the
    // disk cache now; nothing more is written to the database.
    m_queue.clear();
    m_active = false;
    m_inFlight.clear();
    emit finished(m_measured, m_skipped, true);
}

void IntroLibrary::analyzeQuietly(const QString &filePath)
{
    if (!m_store || filePath.isEmpty() || m_inFlight.contains(filePath)
        || m_quiet.contains(filePath) || !schemaReady()) {
        return;
    }

    // A row that already carries a number has had its decode; repeating it
    // would cost an ffmpeg run every time the track went to air. A locked
    // row is never re-measured at all.
    const IntroTimes known = timesFor(filePath);
    if (!known.known || known.locked || known.introMs >= 0)
        return;

    m_quiet.insert(filePath);
    // fetch() can emit waveformReady synchronously (missing file, no
    // ffmpeg) — and does return the data directly when it is already
    // cached — so the membership is armed before the call, and the
    // synchronous return is handled here rather than in the signal.
    if (const WaveformData *ready = m_store->fetch(filePath)) {
        if (m_quiet.remove(filePath) && !ready->failed)
            store(filePath, *ready);
    }
}

bool IntroLibrary::setIntroByHand(const QString &filePath, qint64 introMs)
{
    if (filePath.isEmpty() || introMs < 0 || !schemaReady())
        return false;

    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral(
        "UPDATE musics SET intro_ms = :intro, intro_locked = 1 WHERE path = :path"));
    query.bindValue(QStringLiteral(":intro"), introMs);
    query.bindValue(QStringLiteral(":path"), filePath);
    if (!query.exec()) {
        qWarning() << "IntroLibrary: could not store the hand-set intro of"
                   << filePath << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() <= 0)
        return false; // not a library track (a file dropped into the playlist)

    emit introMeasured(filePath, timesFor(filePath));
    return true;
}

bool IntroLibrary::clearLock(const QString &filePath)
{
    if (filePath.isEmpty() || !schemaReady())
        return false;

    QSqlQuery query = libraryQuery();
    // The value goes back to NULL along with the flag, so the row looks
    // exactly like one that was never measured and every "missing intro"
    // path picks it up again.
    query.prepare(QStringLiteral(
        "UPDATE musics SET intro_locked = 0, intro_ms = NULL, outro_ms = NULL "
        "WHERE path = :path"));
    query.bindValue(QStringLiteral(":path"), filePath);
    if (!query.exec()) {
        qWarning() << "IntroLibrary: could not clear the intro lock of"
                   << filePath << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

void IntroLibrary::pump()
{
    while (m_active && m_inFlight.size() < kMaxInFlight && !m_queue.isEmpty()) {
        const QString path = m_queue.takeFirst();
        m_inFlight.insert(path);
        // Already decoded, in memory or in the on-disk cache: no
        // waveformReady is coming for it, so account for it right here.
        if (const WaveformData *ready = m_store->fetch(path))
            complete(path, *ready);
    }

    if (m_active && m_inFlight.isEmpty() && m_queue.isEmpty()) {
        m_active = false;
        emit finished(m_measured, m_skipped, false);
    }
}

void IntroLibrary::onWaveformReady(const QString &filePath)
{
    if (m_quiet.remove(filePath)) {
        if (const WaveformData *data = m_store->peek(filePath)) {
            if (!data->failed)
                store(filePath, *data);
        }
    }

    if (!m_inFlight.contains(filePath))
        return; // someone else's waveform (the wave view, a preview)

    if (const WaveformData *data = m_store->peek(filePath))
        complete(filePath, *data);
    else
        complete(filePath, WaveformData{});

    pump();
}

void IntroLibrary::complete(const QString &filePath, WaveformData data)
{
    m_inFlight.remove(filePath);
    ++m_done;

    if (data.failed || !data.ready() || !store(filePath, data))
        ++m_skipped;
    else
        ++m_measured;

    // The sweep only needs the numbers, and the disk cache keeps the wave.
    m_store->forget(filePath);
    emit progress(m_done, m_total);
}

bool IntroLibrary::store(const QString &filePath, const WaveformData &data)
{
    if (!schemaReady())
        return false;

    // The lock is checked here rather than when the queue is built, because
    // an operator can drag the marker of the track on air while a sweep of
    // the whole library is running behind it.
    const IntroTimes existing = timesFor(filePath);
    if (!existing.known || existing.locked)
        return false;

    const IntroPoints points = IntroDetector::detect(data, m_thresholdPercent);
    if (!points.valid)
        return false; // silent or undecodable: leave the row NULL, not 0

    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral(
        "UPDATE musics SET intro_ms = :intro, outro_ms = :outro WHERE path = :path"));
    query.bindValue(QStringLiteral(":intro"), points.introMs);
    query.bindValue(QStringLiteral(":outro"), points.outroMs);
    query.bindValue(QStringLiteral(":path"), filePath);
    if (!query.exec()) {
        qWarning() << "IntroLibrary: could not store the intro of" << filePath
                   << query.lastError().text();
        return false;
    }

    IntroTimes stored;
    stored.known = true;
    stored.introMs = points.introMs;
    stored.outroMs = points.outroMs;
    stored.locked = false;
    emit introMeasured(filePath, stored);
    return true;
}
