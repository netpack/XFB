#include "LoudnessScanner.h"

#include "FxEngine.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
// A track is never boosted or cut by more than this. It is a guard against
// a bad measurement (a file whose first seconds are silence, a mangled
// summary) rather than a musical choice.
//
// The boost limit is deliberately generous: a genuinely quiet transfer can
// sit 15 dB under a -16 LUFS target, and a cap that bites there would
// silently leave exactly the tracks this feature exists for still too
// quiet. Clipping is not what bounds it — the true-peak cap in
// normalizationGainDb() and the limiter downstream do that — so the only
// job left for this number is to stop a nonsense measurement from
// detonating the output.
constexpr double kMaxGainDb = 18.0;
constexpr double kMinGainDb = -24.0;

/** Pull one "<label>: <number>" out of a block, or NaN. */
double capture(const QString &text, const QString &pattern)
{
    static QHash<QString, QRegularExpression> cache;
    auto it = cache.find(pattern);
    if (it == cache.end())
        it = cache.insert(pattern, QRegularExpression(pattern));

    const QRegularExpressionMatch m = it->match(text);
    if (!m.hasMatch())
        return std::nan("");

    const QString value = m.captured(1);
    if (value.contains(QLatin1String("inf"), Qt::CaseInsensitive))
        return value.startsWith(QLatin1Char('-')) ? -std::numeric_limits<double>::infinity()
                                                  : std::numeric_limits<double>::infinity();
    bool ok = false;
    const double d = value.toDouble(&ok);
    return ok ? d : std::nan("");
}
} // namespace

LoudnessScanner::LoudnessScanner(QObject *parent)
    : QObject(parent)
{
}

LoudnessScanner::~LoudnessScanner()
{
    // Never leave ffmpeg children behind when the window closes.
    for (auto it = m_running.begin(); it != m_running.end(); ++it) {
        QProcess *proc = it.key();
        proc->disconnect(this);
        proc->kill();
        proc->waitForFinished(500);
        proc->deleteLater();
    }
    m_running.clear();
}

bool LoudnessScanner::available()
{
    return !FxEngine::ffmpegExecutable().isEmpty();
}

void LoudnessScanner::setMaxConcurrent(int jobs)
{
    m_maxConcurrent = std::clamp(jobs, 1, 8);
}

// ---------------------------------------------------------------- Parsing

LoudnessMeasurement LoudnessScanner::parseSummary(const QString &stderrText)
{
    LoudnessMeasurement result;

    // ebur128 prints a running "t: ... I: ... LRA: ..." line ten times a
    // second; only the trailing Summary block is the measurement. Anchor on
    // the LAST summary so a re-used buffer cannot yield a stale answer.
    const int summaryAt = stderrText.lastIndexOf(QLatin1String("Summary:"));
    if (summaryAt < 0) {
        result.error = QObject::tr("ffmpeg produced no loudness summary");
        return result;
    }
    const QString summary = stderrText.mid(summaryAt);

    const double integrated =
        capture(summary, QStringLiteral("I:\\s*(-?[0-9.]+|-?inf)\\s*LUFS"));
    const double range =
        capture(summary, QStringLiteral("LRA:\\s*(-?[0-9.]+|-?inf)\\s*LU"));
    // The true-peak block is the only place a bare "Peak:" appears.
    const double peak =
        capture(summary, QStringLiteral("True peak:\\s*\\n?\\s*Peak:\\s*(-?[0-9.]+|-?inf)"));

    if (std::isnan(integrated)) {
        result.error = QObject::tr("no integrated loudness in the ffmpeg summary");
        return result;
    }
    if (std::isinf(integrated)) {
        // Digital silence, or so close to it that the gate never opened.
        result.error = QObject::tr("the file is silent");
        return result;
    }

    result.integratedLufs = integrated;
    result.rangeLu = std::isnan(range) || std::isinf(range) ? 0.0 : range;
    // A missing or -inf true peak must not read as "0 dBTP, no headroom";
    // fall back to the integrated loudness, which is always an
    // under-estimate of the peak and therefore the safe direction.
    result.truePeakDbtp = (std::isnan(peak) || std::isinf(peak)) ? integrated : peak;
    result.valid = true;
    return result;
}

double LoudnessScanner::normalizationGainDb(double integratedLufs, double truePeakDbtp,
                                            double targetLufs, double ceilingDbTp)
{
    if (!std::isfinite(integratedLufs) || !std::isfinite(targetLufs))
        return 0.0;

    double gain = targetLufs - integratedLufs;

    // True-peak guard: after the gain the track peaks at
    // truePeak + gain dBTP, which must stay under the ceiling.
    if (std::isfinite(truePeakDbtp) && std::isfinite(ceilingDbTp)) {
        const double headroom = ceilingDbTp - truePeakDbtp;
        if (gain > headroom)
            gain = headroom;
    }

    return std::clamp(gain, kMinGainDb, kMaxGainDb);
}

// ---------------------------------------------------------------- Sweeping

void LoudnessScanner::measure(const QStringList &filePaths)
{
    if (m_active)
        return;

    m_queue.clear();
    QSet<QString> seen;
    for (const QString &path : filePaths) {
        if (path.isEmpty() || seen.contains(path))
            continue;
        seen.insert(path);
        m_queue.append(path);
    }

    m_total = m_queue.size();
    m_done = 0;
    m_ok = 0;
    m_failed = 0;
    m_canceling = false;

    if (m_total == 0) {
        emit finished(0, 0, false);
        return;
    }

    m_active = true;
    emit progress(0, m_total);
    pump();
}

void LoudnessScanner::cancel()
{
    if (!m_active || m_canceling)
        return;

    m_canceling = true;
    m_queue.clear();

    // Killing the children makes their finished() handlers run; the
    // m_canceling flag keeps those from re-entering finish().
    const QList<QProcess *> running = m_running.keys();
    for (QProcess *proc : running) {
        proc->disconnect(this);
        proc->kill();
        proc->waitForFinished(500);
        proc->deleteLater();
    }
    m_running.clear();

    finish(true);
}

void LoudnessScanner::pump()
{
    while (m_active && !m_canceling
           && m_running.size() < m_maxConcurrent && !m_queue.isEmpty()) {
        startOne(m_queue.takeFirst());
    }

    if (m_active && !m_canceling && m_running.isEmpty() && m_queue.isEmpty())
        finish(false);
}

void LoudnessScanner::startOne(const QString &path)
{
    const QString ffmpeg = FxEngine::ffmpegExecutable();
    if (ffmpeg.isEmpty() || !QFile::exists(path)) {
        LoudnessMeasurement failed;
        failed.filePath = path;
        failed.error = ffmpeg.isEmpty() ? tr("ffmpeg was not found")
                                        : tr("the file is missing");
        ++m_done;
        ++m_failed;
        emit measured(failed);
        emit progress(m_done, m_total);
        return;
    }

    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::SeparateChannels);
    m_running.insert(proc, path);

    connect(proc, &QProcess::finished, this,
            [this, proc, path](int, QProcess::ExitStatus) { complete(proc, path); });
    connect(proc, &QProcess::errorOccurred, this,
            [this, proc, path](QProcess::ProcessError) {
        if (m_running.contains(proc))
            complete(proc, path);
    });

    // -nostats keeps ffmpeg's own progress line out of stderr; the ebur128
    // running lines are still there and are skipped by the parser.
    proc->start(ffmpeg, {QStringLiteral("-nostdin"),
                         QStringLiteral("-hide_banner"),
                         QStringLiteral("-nostats"),
                         QStringLiteral("-i"), path,
                         QStringLiteral("-vn"),
                         QStringLiteral("-af"), QStringLiteral("ebur128=peak=true"),
                         QStringLiteral("-f"), QStringLiteral("null"),
                         QStringLiteral("-")});
}

void LoudnessScanner::complete(QProcess *proc, const QString &path)
{
    if (!m_running.remove(proc))
        return; // already accounted for (finished + errorOccurred both fired)

    const QString text = QString::fromUtf8(proc->readAllStandardError());
    proc->disconnect(this);
    proc->deleteLater();

    LoudnessMeasurement result = parseSummary(text);
    result.filePath = path;

    ++m_done;
    if (result.valid)
        ++m_ok;
    else
        ++m_failed;

    emit measured(result);
    emit progress(m_done, m_total);

    if (!m_canceling)
        pump();
}

void LoudnessScanner::finish(bool canceled)
{
    m_active = false;
    m_canceling = false;
    emit finished(m_ok, m_failed, canceled);
}
