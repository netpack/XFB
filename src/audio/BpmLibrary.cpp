#include "BpmLibrary.h"

#include "WaveformStore.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
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
} // namespace

BpmLibrary::BpmLibrary(WaveformStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
    if (m_store) {
        connect(m_store, &WaveformStore::waveformReady,
                this, &BpmLibrary::onWaveformReady);
    }
}

double BpmLibrary::bpmFor(const QString &filePath) const
{
    if (filePath.isEmpty())
        return 0.0;

    if (m_store) {
        if (const WaveformData *known = m_store->peek(filePath))
            return known->bpm;
    }

    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral("SELECT bpm FROM musics WHERE path = :path"));
    query.bindValue(QStringLiteral(":path"), filePath);
    if (query.exec() && query.next())
        return query.value(0).toDouble();

    return 0.0;
}

namespace
{
QStringList pathsOf(QSqlQuery &&query, const char *what)
{
    QStringList paths;
    if (!query.exec()) {
        qWarning() << "BpmLibrary: could not list" << what << query.lastError().text();
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

QStringList BpmLibrary::tracksMissingBpm() const
{
    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral("SELECT path FROM musics WHERE bpm IS NULL"));
    return pathsOf(std::move(query), "tracks without a BPM:");
}

QStringList BpmLibrary::allTracks() const
{
    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral("SELECT path FROM musics"));
    return pathsOf(std::move(query), "library tracks:");
}

void BpmLibrary::analyze(const QStringList &filePaths)
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
    m_analyzed = 0;
    m_skipped = 0;
    m_active = true;

    emit progress(0, m_total);
    pump();
}

void BpmLibrary::cancel()
{
    if (!m_active)
        return;
    m_queue.clear();
    finish(true);
}

void BpmLibrary::analyzeQuietly(const QString &filePath)
{
    if (!m_store || filePath.isEmpty() || m_inFlight.contains(filePath)
        || m_quiet.contains(filePath)) {
        return;
    }

    // A row that already carries a number — including the 0 that means "no
    // steady tempo" — has had its decode; repeating it would cost an ffmpeg
    // run every time auto mode looked at the track.
    QSqlQuery known = libraryQuery();
    known.prepare(QStringLiteral("SELECT bpm FROM musics WHERE path = :path"));
    known.bindValue(QStringLiteral(":path"), filePath);
    if (known.exec() && known.next() && !known.value(0).isNull())
        return;

    m_quiet.insert(filePath);
    if (const WaveformData *ready = m_store->fetch(filePath)) {
        m_quiet.remove(filePath);
        if (!ready->failed)
            store(filePath, ready->bpm);
    }
}

void BpmLibrary::pump()
{
    while (m_active && m_inFlight.size() < kMaxInFlight && !m_queue.isEmpty()) {
        const QString path = m_queue.takeFirst();
        m_inFlight.insert(path);
        // Already decoded, in memory or in the on-disk cache: no
        // waveformReady is coming for it, so account for it right here.
        if (const WaveformData *ready = m_store->fetch(path))
            complete(path, *ready);
    }

    if (m_active && m_inFlight.isEmpty() && m_queue.isEmpty())
        finish(false);
}

void BpmLibrary::onWaveformReady(const QString &filePath)
{
    if (m_quiet.remove(filePath)) {
        if (const WaveformData *data = m_store->peek(filePath)) {
            if (!data->failed)
                store(filePath, data->bpm);
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

void BpmLibrary::complete(const QString &filePath, WaveformData data)
{
    m_inFlight.remove(filePath);
    ++m_done;

    if (data.failed || !data.ready()) {
        ++m_skipped;
    } else {
        store(filePath, data.bpm);
        if (data.bpm > 0.0)
            ++m_analyzed;
        else
            ++m_skipped; // decoded fine, but holds no steady tempo
    }

    // The sweep only needs the number, and the disk cache keeps the wave.
    m_store->forget(filePath);
    emit progress(m_done, m_total);
}

void BpmLibrary::store(const QString &filePath, double bpm)
{
    QSqlQuery query = libraryQuery();
    query.prepare(QStringLiteral("UPDATE musics SET bpm = :bpm WHERE path = :path"));
    query.bindValue(QStringLiteral(":bpm"), bpm);
    query.bindValue(QStringLiteral(":path"), filePath);
    if (!query.exec()) {
        qWarning() << "BpmLibrary: could not store the BPM of" << filePath
                   << query.lastError().text();
        return;
    }
    emit bpmMeasured(filePath, bpm);
}

void BpmLibrary::finish(bool canceled)
{
    m_active = false;
    m_inFlight.clear();
    emit finished(m_analyzed, m_skipped, canceled);
}
