#include "WaveformStore.h"

#include "BpmDetector.h"
#include "FxEngine.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <memory>

namespace
{
constexpr int kSampleRate = 4000; // decode rate; plenty for a visual envelope
constexpr int kSamplesPerPeak = kSampleRate * WaveformStore::MsPerPeak / 1000;
constexpr int kBytesPerPeak = kSamplesPerPeak * 2; // s16le
constexpr int kMaxParallelJobs = 2;
constexpr quint32 kCacheMagic = 0x58465756; // "XFWV"
constexpr quint16 kCacheVersion = 3;

// Resolution of the envelope the tempo is measured from. Finer than the
// visual one — at 20 ms a beat period is only known to about ±5 BPM, which
// is the width of a whole "similar tempo" bucket — and a whole number of
// sub-slices per visual peak, so both envelopes come out of one pass.
constexpr int kMsPerOnset = 5;
constexpr int kSamplesPerOnset = kSampleRate * kMsPerOnset / 1000;
static_assert(kSamplesPerPeak % kSamplesPerOnset == 0,
              "the visual peak must be a whole number of onset slices");

// Per-decode state: PCM is reduced to peaks incrementally so the raw
// samples never accumulate in memory.
//
// Shared (rather than owned by one handler) because several of the process
// signals can still reach their handlers after the decode is over — see the
// one-shot teardown in startNext().
struct DecodeJob
{
    QByteArray carry;
    QVector<quint8> peaks;
    QVector<quint8> onsets; // fine envelope, only lives until the BPM is out
    bool done = false;      // teardown already ran for this job
};

void consumePcm(DecodeJob &job, const QByteArray &chunk)
{
    job.carry.append(chunk);
    const char *data = job.carry.constData();
    int offset = 0;
    while (job.carry.size() - offset >= kBytesPerPeak) {
        int peak = 0;
        for (int slice = 0; slice < kSamplesPerPeak / kSamplesPerOnset; ++slice) {
            int slicePeak = 0;
            for (int i = 0; i < kSamplesPerOnset; ++i) {
                const int idx = offset + (slice * kSamplesPerOnset + i) * 2;
                const qint16 sample = qint16(quint8(data[idx]) | (quint8(data[idx + 1]) << 8));
                slicePeak = qMax(slicePeak, qAbs(int(sample)));
            }
            job.onsets.append(quint8(qMin(slicePeak >> 7, 255)));
            peak = qMax(peak, slicePeak);
        }
        job.peaks.append(quint8(qMin(peak >> 7, 255)));
        offset += kBytesPerPeak;
    }
    job.carry.remove(0, offset);
}
} // namespace

WaveformStore::WaveformStore(QObject *parent)
    : QObject(parent)
{
}

const WaveformData *WaveformStore::peek(const QString &filePath) const
{
    const auto it = m_cache.constFind(filePath);
    return it == m_cache.constEnd() ? nullptr : &it.value();
}

const WaveformData *WaveformStore::fetch(const QString &filePath)
{
    if (const WaveformData *known = peek(filePath))
        return known;
    if (filePath.isEmpty() || m_pending.contains(filePath))
        return nullptr;

    WaveformData cached;
    if (loadFromDisk(filePath, cached)) {
        m_cache.insert(filePath, cached);
        return peek(filePath);
    }

    m_pending.insert(filePath);
    m_queue.append(filePath);
    startNext();
    return nullptr;
}

void WaveformStore::forget(const QString &filePath)
{
    if (!m_pending.contains(filePath))
        m_cache.remove(filePath);
}

void WaveformStore::startNext()
{
    while (m_running < kMaxParallelJobs && !m_queue.isEmpty()) {
        const QString path = m_queue.takeFirst();

        const QString ffmpeg = FxEngine::ffmpegExecutable();
        if (ffmpeg.isEmpty() || !QFile::exists(path)) {
            WaveformData failed;
            failed.failed = true;
            m_cache.insert(path, failed);
            m_pending.remove(path);
            emit waveformReady(path);
            continue;
        }

        ++m_running;
        auto *proc = new QProcess(this);
        const auto job = std::make_shared<DecodeJob>();

        // Teardown is one-shot: a process can report its end twice. An ffmpeg
        // that crashes emits errorOccurred(Crashed) — by which point the state
        // is already NotRunning, so the guard below does not catch it — and
        // then finished(CrashExit). Running the teardown for both would free
        // the job twice. The job is shared with the readyRead handler for the
        // same reason: on Windows the pipe reader delivers readyRead through a
        // queued call, which can still arrive after finished().
        const auto teardown = [this, proc, job, path](bool ok) {
            if (job->done)
                return;
            job->done = true;

            WaveformData result;
            if (ok && !job->peaks.isEmpty()) {
                result.peaks = job->peaks;
                result.durationMs = qint64(job->peaks.size()) * MsPerPeak;
                result.bpm = BpmDetector::estimateFromEnvelope(job->onsets, kMsPerOnset);
                saveToDisk(path, result);
            } else {
                result.failed = true;
            }
            m_cache.insert(path, result);
            m_pending.remove(path);

            proc->deleteLater();
            --m_running;
            emit waveformReady(path);
            startNext();
        };

        connect(proc, &QProcess::readyReadStandardOutput, this, [proc, job]() {
            if (!job->done)
                consumePcm(*job, proc->readAllStandardOutput());
        });
        connect(proc, &QProcess::finished, this,
                [proc, job, path, teardown](int exitCode, QProcess::ExitStatus status) {
            if (job->done)
                return;
            consumePcm(*job, proc->readAllStandardOutput());

            const bool ok = (status == QProcess::NormalExit && exitCode == 0);
            if (!ok)
                qWarning() << "WaveformStore: ffmpeg decode failed for" << path
                           << proc->readAllStandardError();
            teardown(ok);
        });
        connect(proc, &QProcess::errorOccurred, this,
                [proc, path, teardown](QProcess::ProcessError) {
            if (proc->state() != QProcess::NotRunning)
                return; // still alive: finished() will do the teardown
            qWarning() << "WaveformStore: ffmpeg failed for" << path
                       << proc->errorString();
            teardown(false);
        });

        proc->start(ffmpeg,
                    {QStringLiteral("-v"), QStringLiteral("error"),
                     QStringLiteral("-nostdin"),
                     QStringLiteral("-i"), path,
                     QStringLiteral("-vn"),
                     QStringLiteral("-ac"), QStringLiteral("1"),
                     QStringLiteral("-ar"), QString::number(kSampleRate),
                     QStringLiteral("-f"), QStringLiteral("s16le"),
                     QStringLiteral("-")});
    }
}

QString WaveformStore::cacheFileFor(const QString &filePath) const
{
    const QFileInfo info(filePath);
    const QByteArray key = QString(filePath + QLatin1Char('|')
                                   + QString::number(info.size()) + QLatin1Char('|')
                                   + QString::number(info.lastModified().toSecsSinceEpoch()))
                               .toUtf8();
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                        + QStringLiteral("/waveforms");
    return dir + QLatin1Char('/')
           + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex())
           + QStringLiteral(".wf");
}

bool WaveformStore::loadFromDisk(const QString &filePath, WaveformData &out) const
{
    QFile file(cacheFileFor(filePath));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);
    quint32 magic = 0;
    quint16 version = 0;
    in >> magic >> version;
    if (magic != kCacheMagic || version != kCacheVersion)
        return false;

    qint64 durationMs = 0;
    QByteArray peaks;
    double bpm = 0.0;
    in >> durationMs >> peaks >> bpm;
    if (in.status() != QDataStream::Ok || peaks.isEmpty() || durationMs <= 0)
        return false;

    out.durationMs = durationMs;
    out.peaks.resize(peaks.size());
    memcpy(out.peaks.data(), peaks.constData(), size_t(peaks.size()));
    out.bpm = bpm;
    out.failed = false;
    return true;
}

void WaveformStore::saveToDisk(const QString &filePath, const WaveformData &data) const
{
    const QString cachePath = cacheFileFor(filePath);
    QDir().mkpath(QFileInfo(cachePath).absolutePath());

    QFile file(cachePath);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QDataStream outStream(&file);
    outStream << kCacheMagic << kCacheVersion << data.durationMs
              << QByteArray(reinterpret_cast<const char *>(data.peaks.constData()),
                            data.peaks.size())
              << data.bpm;
}
