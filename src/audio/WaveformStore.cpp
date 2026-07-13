#include "WaveformStore.h"

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

namespace
{
constexpr int kSampleRate = 4000; // decode rate; plenty for a visual envelope
constexpr int kSamplesPerPeak = kSampleRate * WaveformStore::MsPerPeak / 1000;
constexpr int kBytesPerPeak = kSamplesPerPeak * 2; // s16le
constexpr int kMaxParallelJobs = 2;
constexpr quint32 kCacheMagic = 0x58465756; // "XFWV"
constexpr quint16 kCacheVersion = 1;

// Per-decode state: PCM is reduced to peaks incrementally so the raw
// samples never accumulate in memory.
struct DecodeJob
{
    QByteArray carry;
    QVector<quint8> peaks;
};

void consumePcm(DecodeJob &job, const QByteArray &chunk)
{
    job.carry.append(chunk);
    const char *data = job.carry.constData();
    int offset = 0;
    while (job.carry.size() - offset >= kBytesPerPeak) {
        int peak = 0;
        for (int i = 0; i < kSamplesPerPeak; ++i) {
            const int idx = offset + i * 2;
            const qint16 sample = qint16(quint8(data[idx]) | (quint8(data[idx + 1]) << 8));
            peak = qMax(peak, qAbs(int(sample)));
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
        auto *job = new DecodeJob;

        connect(proc, &QProcess::readyReadStandardOutput, this, [proc, job]() {
            consumePcm(*job, proc->readAllStandardOutput());
        });
        connect(proc, &QProcess::finished, this,
                [this, proc, job, path](int exitCode, QProcess::ExitStatus status) {
            consumePcm(*job, proc->readAllStandardOutput());

            WaveformData result;
            if (status == QProcess::NormalExit && exitCode == 0 && !job->peaks.isEmpty()) {
                result.peaks = job->peaks;
                result.durationMs = qint64(job->peaks.size()) * MsPerPeak;
                saveToDisk(path, result);
            } else {
                qWarning() << "WaveformStore: ffmpeg decode failed for" << path
                           << proc->readAllStandardError();
                result.failed = true;
            }
            m_cache.insert(path, result);
            m_pending.remove(path);

            delete job;
            proc->deleteLater();
            --m_running;
            emit waveformReady(path);
            startNext();
        });
        connect(proc, &QProcess::errorOccurred, this,
                [this, proc, job, path](QProcess::ProcessError) {
            if (proc->state() != QProcess::NotRunning)
                return; // finished() will handle it
            WaveformData failed;
            failed.failed = true;
            m_cache.insert(path, failed);
            m_pending.remove(path);
            delete job;
            proc->deleteLater();
            --m_running;
            emit waveformReady(path);
            startNext();
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
    in >> durationMs >> peaks;
    if (in.status() != QDataStream::Ok || peaks.isEmpty() || durationMs <= 0)
        return false;

    out.durationMs = durationMs;
    out.peaks.resize(peaks.size());
    memcpy(out.peaks.data(), peaks.constData(), size_t(peaks.size()));
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
                            data.peaks.size());
}
