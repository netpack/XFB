#include "FxEngine.h"

#include <QAudioSink>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QMediaDevices>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <cstring>

namespace
{
QString findTool(const QString &name)
{
    QString path = QStandardPaths::findExecutable(name);
    if (!path.isEmpty())
        return path;

    // GUI apps launched from Finder/desktop launchers often have a minimal
    // PATH that misses Homebrew / local installs — check the usual places.
    const QStringList extraDirs = {
        "/opt/homebrew/bin", "/usr/local/bin", "/opt/local/bin", "/usr/bin"
    };
    path = QStandardPaths::findExecutable(name, extraDirs);
    return path;
}
} // namespace

FxEngine::FxEngine(QObject *parent)
    : QObject(parent)
{
    m_chunk.resize(kChunkFrames * kChannels);
    m_chunk16.resize(kChunkFrames * kChannels);
    m_djFilter.setup(kSampleRate);
    m_echo.setup(kSampleRate);
}

FxEngine::~FxEngine()
{
    stopProcess();
    teardownSink();
}

QString FxEngine::ffmpegExecutable()
{
    static const QString path = findTool("ffmpeg");
    return path;
}

QString FxEngine::ffprobeExecutable()
{
    static const QString path = findTool("ffprobe");
    return path;
}

bool FxEngine::available()
{
    return !ffmpegExecutable().isEmpty();
}

// ------------------------------------------------------------------ transport

void FxEngine::setSource(const QString &pathOrUrl)
{
    stop();
    m_path = pathOrUrl;
    m_durationMs = 0;
    m_sourceIs432 = false;
    m_isLive = m_path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
               || m_path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);

    if (m_path.isEmpty())
        return;

    if (m_isLive) {
        // Live network streams have no meaningful duration and probing
        // them would block on the network.
        emit durationChanged(0);
        return;
    }

    probeLocalSource(m_path);
    emit durationChanged(m_durationMs);
}

void FxEngine::play()
{
    if (m_path.isEmpty() || m_state == State::Playing)
        return;

    const qint64 startAt = (m_state == State::Paused) ? m_pausedPosMs : 0;

    if (!ensureSink()) {
        failTrack(tr("No usable audio output device for the FX engine"));
        return;
    }

    startProcessAt(startAt);
    if (!m_proc)
        return; // failTrack already emitted

    m_state = State::Playing;

    if (!m_pumpTimer) {
        m_pumpTimer = new QTimer(this);
        m_pumpTimer->setInterval(15);
        connect(m_pumpTimer, &QTimer::timeout, this, &FxEngine::pump);
    }
    m_pumpTimer->start();
}

void FxEngine::pause()
{
    if (m_state != State::Playing)
        return;

    if (m_scratchActive) {
        // Pausing mid-scratch: freeze at the scratch position
        m_pausedPosMs = std::max<qint64>(0, scratchPosMs());
        m_scratchActive = false;
        m_scratchMode = 0;
        m_scratchBuf.clear();
        if (m_pumpTimer)
            m_pumpTimer->stop();
        if (m_sink)
            m_sink->reset();
        m_io = nullptr;
        m_state = State::Paused;
        return;
    }

    // Position the resume point at what the listener actually heard:
    // subtract audio still queued in the sink from the decode position.
    qint64 bufferedMs = 0;
    if (m_sink && m_io) {
        const int bytesPerFrame = m_sinkIsFloat ? 8 : 4;
        const qint64 bufferedBytes = m_sink->bufferSize() - m_sink->bytesFree();
        bufferedMs = (bufferedBytes / bytesPerFrame) * 1000 / kSampleRate;
    }
    m_pausedPosMs = std::max<qint64>(0, currentPositionMs() - bufferedMs);

    stopProcess();
    if (m_pumpTimer)
        m_pumpTimer->stop();
    if (m_sink)
        m_sink->reset();
    m_io = nullptr;
    m_state = State::Paused;
}

void FxEngine::stop()
{
    stopProcess();
    m_scratchActive = false;
    m_scratchMode = 0;
    m_scratchBuf.clear();
    m_history.clear();
    if (m_pumpTimer)
        m_pumpTimer->stop();
    if (m_sink)
        m_sink->reset();
    m_io = nullptr;
    m_state = State::Stopped;
    m_baseMs = 0;
    m_framesTaken = 0;
    m_pausedPosMs = 0;
    m_producedAudio = false;
    resetDspState();
}

void FxEngine::seek(qint64 positionMs)
{
    if (m_isLive)
        return; // seeking has no meaning on a live stream

    positionMs = std::max<qint64>(0, positionMs);
    if (m_durationMs > 0)
        positionMs = std::min(positionMs, m_durationMs);

    if (m_state == State::Playing) {
        stopProcess();
        if (m_sink)
            m_sink->reset();
        m_io = nullptr;
        startProcessAt(positionMs);
        if (m_proc && m_sink)
            m_io = m_sink->start();
    } else {
        m_pausedPosMs = positionMs;
        m_baseMs = positionMs;
        if (m_state == State::Stopped)
            m_state = State::Paused; // remember the seek for the next play()
    }
    emit positionChanged(positionMs);
}

void FxEngine::setVolume(float linearVolume)
{
    m_volume = std::clamp(linearVolume, 0.0f, 1.0f);
    if (m_sink)
        m_sink->setVolume(m_volume);
}

void FxEngine::setParams(const FxParams &params)
{
    m_params = params;
    m_eq.configure(kSampleRate, m_params);
    m_comp.configure(kSampleRate, m_params);

    // The retune runs inside the ffmpeg filter chain, so toggling it live
    // requires a decoder restart at the current position. Sources that are
    // already at 432 Hz never carry the filter, so no restart is needed.
    const bool effectiveBefore = m_retuneOn && !m_sourceIs432;
    const bool effectiveAfter = m_params.retune432 && !m_sourceIs432;
    if (effectiveBefore != effectiveAfter || m_retuneOn != m_params.retune432) {
        m_retuneOn = m_params.retune432;
        if (m_state == State::Playing && effectiveBefore != effectiveAfter) {
            const qint64 pos = currentPositionMs();
            stopProcess();
            if (m_sink)
                m_sink->reset();
            m_io = nullptr;
            startProcessAt(pos);
            if (m_proc && m_sink)
                m_io = m_sink->start();
        }
    }
}

void FxEngine::shutdown()
{
    stop();
    teardownSink();
}

// ------------------------------------------------------------------- internals

void FxEngine::startProcessAt(qint64 positionMs)
{
    stopProcess();

    const QString ffmpeg = ffmpegExecutable();
    if (ffmpeg.isEmpty()) {
        failTrack(tr("ffmpeg not found — audio FX engine unavailable"));
        return;
    }
    if (!m_isLive && !QFile::exists(m_path)) {
        failTrack(tr("File not found: %1").arg(m_path));
        return;
    }

    m_baseMs = m_isLive ? 0 : positionMs;
    m_framesTaken = 0;
    m_partialFrame.clear();
    m_fifo.clear();
    // The timeline jumps: played-audio history and any scratch in progress
    // are no longer valid.
    m_history.clear();
    m_scratchActive = false;
    m_scratchBuf.clear();
    resetDspState();

    QStringList args;
    args << "-nostdin" << "-loglevel" << "error";
    if (m_isLive) {
        // Network streams are paced by the server; make ffmpeg ride out
        // short network hiccups instead of exiting.
        args << "-reconnect" << "1"
             << "-reconnect_streamed" << "1"
             << "-reconnect_delay_max" << "5";
    } else {
        args << "-re"; // decode paced at realtime: keeps process buffering bounded
        if (positionMs > 0)
            args << "-ss" << QString::number(positionMs / 1000.0, 'f', 3);
    }
    args << "-i" << m_path
         << "-vn" << "-sn" << "-dn";
    if (m_retuneOn && !m_sourceIs432) {
        // A440 -> A432 pitch shift with tempo preserved: reinterpret the
        // sample rate to lower the pitch, resample back to the engine rate,
        // then atempo restores the original speed (so the track length does
        // not change). atempo uses the exact inverse of the applied ratio.
        // Files that were already converted to 432 Hz (detected via the
        // embedded XFB_TUNING tag or the _432Hz suffix) are played as-is.
        const int retunedRate = qRound(kSampleRate * 432.0 / 440.0);
        const double tempoComp = static_cast<double>(kSampleRate) / retunedRate;
        args << "-af" << QString("asetrate=%1,aresample=%2,atempo=%3")
                             .arg(retunedRate)
                             .arg(kSampleRate)
                             .arg(tempoComp, 0, 'f', 8);
    }
    args << "-f" << "f32le"
         << "-acodec" << "pcm_f32le"
         << "-ac" << QString::number(kChannels)
         << "-ar" << QString::number(kSampleRate)
         << "-";

    m_proc = new QProcess(this);
    m_proc->setReadChannel(QProcess::StandardOutput);
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart)
            failTrack(tr("Failed to start ffmpeg for FX playback"));
    });

    m_proc->start(ffmpeg, args);
    if (!m_proc->waitForStarted(3000)) {
        failTrack(tr("ffmpeg did not start (FX playback)"));
        return;
    }
}

void FxEngine::stopProcess()
{
    if (!m_proc)
        return;
    QProcess *proc = m_proc;
    m_proc = nullptr;
    proc->disconnect(this);
    if (proc->state() != QProcess::NotRunning) {
        proc->kill();
        proc->waitForFinished(1000);
    }
    proc->deleteLater();
}

bool FxEngine::ensureSink()
{
    if (!m_sink) {
        const QAudioDevice device = QMediaDevices::defaultAudioOutput();
        if (device.isNull())
            return false;

        QAudioFormat fmt;
        fmt.setSampleRate(kSampleRate);
        fmt.setChannelCount(kChannels);
        fmt.setSampleFormat(QAudioFormat::Float);
        m_sinkIsFloat = true;

        if (!device.isFormatSupported(fmt)) {
            fmt.setSampleFormat(QAudioFormat::Int16);
            m_sinkIsFloat = false;
            if (!device.isFormatSupported(fmt))
                return false;
        }

        m_sink = new QAudioSink(device, fmt, this);
        const int bytesPerFrame = m_sinkIsFloat ? 8 : 4;
        m_sink->setBufferSize(kSampleRate * bytesPerFrame * 350 / 1000); // ~350 ms
    }

    m_sink->setVolume(m_volume);
    if (!m_io)
        m_io = m_sink->start();
    return m_io != nullptr;
}

void FxEngine::teardownSink()
{
    if (m_sink) {
        m_sink->stop();
        delete m_sink;
        m_sink = nullptr;
    }
    m_io = nullptr;
}

void FxEngine::resetDspState()
{
    m_eq.configure(kSampleRate, m_params);
    m_comp.configure(kSampleRate, m_params);
    m_eq.reset();
    m_comp.reset();
    m_djFilter.reset();
    m_echo.reset();
    m_retuneOn = m_params.retune432;
}

void FxEngine::probeLocalSource(const QString &filePath)
{
    m_durationMs = 0;
    // Copy-mode conversions carry the marker in the file name
    m_sourceIs432 = QFileInfo(filePath).completeBaseName()
                        .endsWith(QStringLiteral("_432Hz"));

    const QString ffprobe = ffprobeExecutable();
    if (!ffprobe.isEmpty()) {
        QProcess probe;
        probe.start(ffprobe, {"-v", "error",
                              "-show_entries", "format=duration:format_tags:stream_tags",
                              "-of", "default=noprint_wrappers=1", filePath});
        if (probe.waitForFinished(5000)) {
            const QString out = QString::fromLocal8Bit(probe.readAllStandardOutput());

            // Files converted by XFB carry an embedded tuning marker; they
            // must not be retuned a second time by the live 432 Hz mode.
            if (out.contains(QStringLiteral("xfb_tuning=432"), Qt::CaseInsensitive)
                || out.contains(QStringLiteral("XFB-432Hz"), Qt::CaseInsensitive)) {
                m_sourceIs432 = true;
            }

            const QRegularExpression durRe(QStringLiteral("^duration=([0-9.]+)"),
                                           QRegularExpression::MultilineOption);
            const QRegularExpressionMatch dm = durRe.match(out);
            if (dm.hasMatch()) {
                const double secs = dm.captured(1).toDouble();
                if (secs > 0) {
                    m_durationMs = static_cast<qint64>(secs * 1000.0);
                    return;
                }
            }
        } else {
            probe.kill();
        }
    }

    // Fallback: parse "Duration: hh:mm:ss.cc" from ffmpeg -i output
    const QString ffmpeg = ffmpegExecutable();
    if (!ffmpeg.isEmpty()) {
        QProcess info;
        info.start(ffmpeg, {"-hide_banner", "-i", filePath});
        if (info.waitForFinished(5000)) {
            const QString err = QString::fromLocal8Bit(info.readAllStandardError());
            const QRegularExpression re(QStringLiteral("Duration:\\s*(\\d+):(\\d+):(\\d+)\\.(\\d+)"));
            const QRegularExpressionMatch m = re.match(err);
            if (m.hasMatch()) {
                m_durationMs = m.captured(1).toLongLong() * 3600000
                             + m.captured(2).toLongLong() * 60000
                             + m.captured(3).toLongLong() * 1000
                             + m.captured(4).toLongLong() * 10;
            }
        }
    }
}

qint64 FxEngine::inputFramesConsumed() const
{
    // With the tempo-preserving retune the output timeline matches the
    // source timeline 1:1, so consumed frames map directly to position.
    return m_framesTaken;
}

qint64 FxEngine::currentPositionMs() const
{
    return m_baseMs + inputFramesConsumed() * 1000 / kSampleRate;
}

void FxEngine::readProcessOutput()
{
    if (!m_proc)
        return;

    QByteArray data = m_proc->readAllStandardOutput();
    if (data.isEmpty())
        return;

    if (!m_partialFrame.isEmpty()) {
        data.prepend(m_partialFrame);
        m_partialFrame.clear();
    }

    constexpr int bytesPerInFrame = sizeof(float) * kChannels;
    const int completeBytes = (data.size() / bytesPerInFrame) * bytesPerInFrame;
    if (completeBytes < data.size()) {
        m_partialFrame = data.mid(completeBytes);
        data.truncate(completeBytes);
    }
    if (data.isEmpty())
        return;

    const float *samples = reinterpret_cast<const float *>(data.constData());
    const int frames = completeBytes / bytesPerInFrame;
    m_fifo.insert(m_fifo.end(), samples, samples + frames * kChannels);
}

int FxEngine::fillChunk(float *out, int maxFrames)
{
    const int availFrames = static_cast<int>(m_fifo.size() / kChannels);
    const int n = std::min(maxFrames, availFrames);
    if (n > 0) {
        std::memcpy(out, m_fifo.data(), n * kChannels * sizeof(float));

        // Feed the scratch history with everything that gets played
        m_history.insert(m_history.end(), m_fifo.begin(), m_fifo.begin() + n * kChannels);
        const size_t maxHistory = static_cast<size_t>(kHistorySeconds) * kSampleRate * kChannels;
        if (m_history.size() > maxHistory)
            m_history.erase(m_history.begin(),
                            m_history.begin() + (m_history.size() - maxHistory));

        m_fifo.erase(m_fifo.begin(), m_fifo.begin() + n * kChannels);
        m_framesTaken += n;
    }
    return n;
}

void FxEngine::applyFxChain(float *chunk, int frames)
{
    m_eq.process(chunk, frames);
    m_comp.process(chunk, frames);
    m_djFilter.process(chunk, frames);
    m_echo.process(chunk, frames);
    fxdsp::clampBuffer(chunk, frames);
}

void FxEngine::writeChunkToSink(const float *chunk, int frames)
{
    if (m_sinkIsFloat) {
        m_io->write(reinterpret_cast<const char *>(chunk),
                    static_cast<qint64>(frames) * kChannels * sizeof(float));
    } else {
        for (int i = 0; i < frames * kChannels; ++i)
            m_chunk16[i] = static_cast<qint16>(chunk[i] * 32767.0f);
        m_io->write(reinterpret_cast<const char *>(m_chunk16.data()),
                    static_cast<qint64>(frames) * kChannels * sizeof(qint16));
    }
    m_producedAudio = true;
}

void FxEngine::pump()
{
    if (m_state != State::Playing || !m_sink || !m_io)
        return;

    if (m_scratchActive) {
        pumpScratch();
        return;
    }

    readProcessOutput();

    const int bytesPerOutFrame = m_sinkIsFloat ? 8 : 4;

    while (true) {
        const int freeFrames = static_cast<int>(m_sink->bytesFree()) / bytesPerOutFrame;
        const int wantFrames = std::min(kChunkFrames, freeFrames);
        if (wantFrames <= 0)
            break;

        const int got = fillChunk(m_chunk.data(), wantFrames);
        if (got <= 0) {
            maybeFinish();
            break;
        }

        applyFxChain(m_chunk.data(), got);
        writeChunkToSink(m_chunk.data(), got);
    }

    // Emit the playback position roughly four times per second
    if (++m_positionEmitDivider >= 16) {
        m_positionEmitDivider = 0;
        emit positionChanged(currentPositionMs());
    }
}

void FxEngine::maybeFinish()
{
    const bool procDone = !m_proc || m_proc->state() == QProcess::NotRunning;
    if (!procDone)
        return; // just waiting for more data (realtime-paced decode)

    // Pull any bytes that arrived after the process exited
    readProcessOutput();
    if (m_fifo.size() >= static_cast<size_t>(kChannels))
        return;

    if (m_proc && m_proc->exitCode() != 0 && !m_producedAudio) {
        failTrack(tr("ffmpeg could not decode: %1")
                      .arg(QString::fromLocal8Bit(m_proc->readAllStandardError()).trimmed()));
        return;
    }

    // Wait for the sink to drain what has already been written
    if (m_sink && m_sink->bytesFree() < m_sink->bufferSize())
        return;

    stopProcess();
    if (m_pumpTimer)
        m_pumpTimer->stop();
    if (m_sink)
        m_sink->reset();
    m_io = nullptr;
    m_state = State::Stopped;

    const qint64 finalPos = (m_durationMs > 0) ? m_durationMs : currentPositionMs();
    m_baseMs = 0;
    m_framesTaken = 0;
    m_producedAudio = false;
    emit positionChanged(finalPos);
    emit playbackFinished();
}

// ---------------------------------------------------------- DJ / scratch mode

void FxEngine::setDjFx(double filterAmount, double echoAmount)
{
    m_djFilter.setAmount(filterAmount);
    m_echo.setAmount(echoAmount);
}

bool FxEngine::enterScratchMode()
{
    if (m_scratchActive)
        return true;
    if (m_state != State::Playing || m_isLive || !m_sink || !m_io)
        return false;

    // Freeze the decoder: while scratching, audio comes from the snapshot
    stopProcess();

    const qint64 nowFrame = m_baseMs * kSampleRate / 1000 + m_framesTaken;
    const qint64 historyFrames = static_cast<qint64>(m_history.size() / kChannels);

    m_scratchBuf.clear();
    m_scratchBuf.reserve(m_history.size() + m_fifo.size());
    m_scratchBuf.insert(m_scratchBuf.end(), m_history.begin(), m_history.end());
    m_scratchBuf.insert(m_scratchBuf.end(), m_fifo.begin(), m_fifo.end());
    m_scratchBufStartFrame = nowFrame - historyFrames;

    m_scratchPos = static_cast<double>(nowFrame);
    m_scratchVel = 1.0;
    m_scratchTargetVel = 0.0;
    m_scratchActive = true;
    return true;
}

void FxEngine::scratchBegin()
{
    m_scratchMode = 0;
    enterScratchMode();
}

void FxEngine::scratchMove(double targetRate)
{
    if (m_scratchActive && m_scratchMode == 0)
        m_scratchTargetVel = std::clamp(targetRate, -12.0, 12.0);
}

void FxEngine::scratchEnd()
{
    if (!m_scratchActive || m_scratchMode != 0)
        return;

    qint64 resumeMs = scratchPosMs();
    resumeMs = std::max<qint64>(0, resumeMs);
    if (m_durationMs > 0)
        resumeMs = std::min(resumeMs, m_durationMs);

    m_scratchActive = false;
    m_scratchBuf.clear();

    // Resume normal decoding from where the record was released
    if (m_sink)
        m_sink->reset();
    m_io = nullptr;
    startProcessAt(resumeMs);
    if (m_proc && m_sink)
        m_io = m_sink->start();
    emit positionChanged(resumeMs);
}

void FxEngine::djBrake()
{
    if (m_scratchActive)
        return;
    if (enterScratchMode()) {
        m_scratchMode = 1;
        m_scratchTargetVel = 0.0; // platter spins down towards zero
    }
}

void FxEngine::djBackspin()
{
    if (m_scratchActive)
        return;
    if (enterScratchMode()) {
        m_scratchMode = 2;
        m_scratchTargetVel = -9.0; // whip the record backwards
    }
}

qint64 FxEngine::scratchPosMs() const
{
    return static_cast<qint64>(m_scratchPos * 1000.0 / kSampleRate);
}

void FxEngine::renderScratch(float *out, int frames)
{
    // Vinyl inertia: the platter velocity eases towards the hand's target.
    // Manual scratching reacts in ~50 ms; the brake takes ~1 s to die out;
    // a backspin snaps to full reverse almost immediately.
    const double ease = (m_scratchMode == 1) ? 0.00008
                        : (m_scratchMode == 2) ? 0.001
                                               : 0.0004;

    const qint64 bufFrames = static_cast<qint64>(m_scratchBuf.size() / kChannels);
    const float *buf = m_scratchBuf.data();

    for (int i = 0; i < frames; ++i) {
        m_scratchVel += (m_scratchTargetVel - m_scratchVel) * ease;
        m_scratchPos += m_scratchVel;

        const double rel = m_scratchPos - static_cast<double>(m_scratchBufStartFrame);
        if (rel >= 0.0 && rel < static_cast<double>(bufFrames - 1)) {
            const qint64 idx = static_cast<qint64>(rel);
            const float t = static_cast<float>(rel - static_cast<double>(idx));
            const float *f0 = buf + idx * kChannels;
            const float *f1 = f0 + kChannels;
            out[2 * i] = f0[0] + (f1[0] - f0[0]) * t;
            out[2 * i + 1] = f0[1] + (f1[1] - f0[1]) * t;
        } else {
            // Ran off the buffered vinyl: silence
            out[2 * i] = 0.0f;
            out[2 * i + 1] = 0.0f;
            m_scratchPos = std::clamp(m_scratchPos,
                                      static_cast<double>(m_scratchBufStartFrame) - 1.0,
                                      static_cast<double>(m_scratchBufStartFrame + bufFrames));
        }
    }
}

void FxEngine::pumpScratch()
{
    const int bytesPerOutFrame = m_sinkIsFloat ? 8 : 4;

    while (true) {
        const int freeFrames = static_cast<int>(m_sink->bytesFree()) / bytesPerOutFrame;
        const int wantFrames = std::min(kChunkFrames, freeFrames);
        if (wantFrames <= 0)
            break;

        renderScratch(m_chunk.data(), wantFrames);
        applyFxChain(m_chunk.data(), wantFrames);
        writeChunkToSink(m_chunk.data(), wantFrames);
    }

    if (++m_positionEmitDivider >= 16) {
        m_positionEmitDivider = 0;
        emit positionChanged(scratchPosMs());
    }

    // While hand-scratching, the target velocity decays between mouse
    // updates: a hand holding the record still means "stopped", and mouse
    // move events only arrive while the hand actually moves.
    if (m_scratchMode == 0)
        m_scratchTargetVel *= 0.85;

    // Effect endings: a brake ends when the platter has stopped, a backspin
    // when the record has been spun back past the start of the buffer.
    if (m_scratchMode == 1
        && std::fabs(m_scratchVel) < 0.02 && std::fabs(m_scratchTargetVel) < 0.02) {
        stopFromScratch();
    } else if (m_scratchMode == 2
               && m_scratchPos <= static_cast<double>(m_scratchBufStartFrame)) {
        stopFromScratch();
    }
}

void FxEngine::stopFromScratch()
{
    const qint64 finalPos = std::max<qint64>(0, scratchPosMs());

    m_scratchActive = false;
    m_scratchMode = 0;
    m_scratchBuf.clear();

    if (m_pumpTimer)
        m_pumpTimer->stop();
    if (m_sink)
        m_sink->reset();
    m_io = nullptr;
    m_state = State::Stopped;
    m_baseMs = 0;
    m_framesTaken = 0;
    m_producedAudio = false;

    emit positionChanged(finalPos);
    emit playbackFinished();
}

void FxEngine::failTrack(const QString &message)
{
    qWarning() << "FxEngine:" << message;
    stopProcess();
    if (m_pumpTimer)
        m_pumpTimer->stop();
    if (m_sink)
        m_sink->reset();
    m_io = nullptr;
    m_state = State::Stopped;
    m_baseMs = 0;
    m_framesTaken = 0;
    m_producedAudio = false;
    emit engineError(message);
}
