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

/** Parse `ffprobe -show_entries format=duration:format_tags:stream_tags`
    output: fills the duration and flags files already retuned to 432 Hz. */
void parseProbeOutput(const QString &out, qint64 *durationMs, bool *is432)
{
    // Files converted by XFB carry an embedded tuning marker; they must
    // not be retuned a second time by the live 432 Hz mode.
    if (out.contains(QStringLiteral("xfb_tuning=432"), Qt::CaseInsensitive)
        || out.contains(QStringLiteral("XFB-432Hz"), Qt::CaseInsensitive)) {
        *is432 = true;
    }

    const QRegularExpression durRe(QStringLiteral("^duration=([0-9.]+)"),
                                   QRegularExpression::MultilineOption);
    const QRegularExpressionMatch dm = durRe.match(out);
    if (dm.hasMatch()) {
        const double secs = dm.captured(1).toDouble();
        if (secs > 0)
            *durationMs = static_cast<qint64>(secs * 1000.0);
    }
}
} // namespace

FxEngine::FxEngine(QObject *parent)
    : QObject(parent)
{
    m_chunk.resize(kChunkFrames * kChannels);
    m_chunk16.resize(kChunkFrames * kChannels);
    m_djFilter.setup(kSampleRate);
    m_echo.setup(kSampleRate);

    // Follow audio-device changes (headphones unplugged, Bluetooth/AirPlay
    // dropped): the sink is bound to the device it was opened on, so it
    // must be rebuilt on the new default or playback wedges silently.
    QMediaDevices *devices = new QMediaDevices(this);
    connect(devices, &QMediaDevices::audioOutputsChanged, this, [this]() {
        const QAudioDevice def = QMediaDevices::defaultAudioOutput();
        if (!m_sink || def.isNull() || def.id() == m_sinkDeviceId)
            return;
        qDebug() << "FxEngine: default audio output changed, rebuilding sink";
        const bool wasActive = (m_state == State::Playing && m_io);
        teardownSink();
        if (wasActive && !ensureSink())
            failTrack(tr("Audio output device disappeared"));
    });
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
    // Gapless handoff: the upcoming track was preloaded and its decoder is
    // already running — adopt it instead of cold-starting a new one. A
    // short track's decoder may have exited already with all of its PCM
    // still buffered, which is just as usable.
    if (!pathOrUrl.isEmpty() && pathOrUrl == m_nextPath && m_nextProc
            && (m_nextProc->state() == QProcess::Running
                || m_nextProc->bytesAvailable() > 0)) {
        adoptPreloaded();
        return;
    }

    cancelPreload();
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

    if (m_procPreloaded && startAt == 0 && m_proc
            && m_proc->state() == QProcess::Running) {
        // Adopted preloaded decoder: already decoding from position 0
        m_procPreloaded = false;
    } else {
        startProcessAt(startAt);
        if (!m_proc)
            return; // failTrack already emitted
    }

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

    stopTailMix(); // pausing mid-crossfade drops the fading tail
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
    cancelPreload();
    stopTailMix();
    stopProcess();
    m_procPreloaded = false;
    m_finishEmitted = false;
    m_nextCrossfadeMs = 0;
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
    m_meterPeakL = 0.0f;
    m_meterPeakR = 0.0f;
    emit levels(0.0f, 0.0f); // let the meter fall silent
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
    // A preloaded decoder was spawned with the previous filter chain; a
    // retune toggle would make it play at the wrong pitch.
    if (m_params.retune432 != params.retune432)
        cancelPreload();

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

QProcess *FxEngine::spawnDecoder(const QString &path, qint64 positionMs,
                                 bool retune, bool isLive, bool waitForStart,
                                 QString *error)
{
    const QString ffmpeg = ffmpegExecutable();
    if (ffmpeg.isEmpty()) {
        if (error)
            *error = tr("ffmpeg not found — audio FX engine unavailable");
        return nullptr;
    }
    if (!isLive && !QFile::exists(path)) {
        if (error)
            *error = tr("File not found: %1").arg(path);
        return nullptr;
    }

    QStringList args;
    args << "-nostdin" << "-loglevel" << "error";
    if (isLive) {
        // Network streams are paced by the server; make ffmpeg ride out
        // short network hiccups instead of exiting.
        args << "-reconnect" << "1"
             << "-reconnect_streamed" << "1"
             << "-reconnect_delay_max" << "5";
    } else {
        args << "-re"; // decode paced at realtime: keeps process buffering bounded
        // Plain -re leaves the pipeline (and so the sink) only as full as
        // ffmpeg's small startup burst, so scheduling hiccups and track
        // seams could run the sink dry and click. Front-load a few seconds
        // at full speed where ffmpeg supports it (>= 6.1), then pace.
        static const bool haveInitialBurst = [] {
            QProcess probe;
            probe.start(ffmpegExecutable(), {"-h", "long"});
            probe.waitForFinished(3000);
            return probe.readAllStandardOutput().contains("readrate_initial_burst");
        }();
        // 12 s: also gives the auto-cue tail trim enough decoded lookahead
        // to chop long encoded outro silences (YouTube rips) off the fifo.
        if (haveInitialBurst)
            args << "-readrate_initial_burst" << "12";
        if (positionMs > 0)
            args << "-ss" << QString::number(positionMs / 1000.0, 'f', 3);
    }
    args << "-i" << path
         << "-vn" << "-sn" << "-dn";
    if (retune) {
        // A440 -> A432 pitch shift with tempo preserved: reinterpret the
        // sample rate to lower the pitch, resample back to the engine rate,
        // then atempo restores the original speed (so the track length does
        // not change). The input is normalized to the engine rate FIRST:
        // asetrate is an absolute rate, so without the leading aresample a
        // 44.1 kHz file (typical MP3) came out +6.9% sharp and fast instead
        // of 1.8% flat. Files that were already converted to 432 Hz
        // (embedded XFB_TUNING tag or _432Hz suffix) are played as-is.
        const int retunedRate = qRound(kSampleRate * 432.0 / 440.0);
        const double tempoComp = 440.0 / 432.0;
        args << "-af" << QString("aresample=%1,asetrate=%2,aresample=%1,atempo=%3")
                             .arg(kSampleRate)
                             .arg(retunedRate)
                             .arg(tempoComp, 0, 'f', 8);
    }
    args << "-f" << "f32le"
         << "-acodec" << "pcm_f32le"
         << "-ac" << QString::number(kChannels)
         << "-ar" << QString::number(kSampleRate)
         << "-";

    QProcess *proc = new QProcess(this);
    proc->setReadChannel(QProcess::StandardOutput);
    proc->start(ffmpeg, args);
    if (waitForStart && !proc->waitForStarted(3000)) {
        if (error)
            *error = tr("ffmpeg did not start (FX playback)");
        proc->kill();
        proc->deleteLater();
        return nullptr;
    }
    return proc;
}

void FxEngine::startProcessAt(qint64 positionMs)
{
    stopProcess();
    stopTailMix(); // a timeline jump ends any crossfade still fading
    m_procPreloaded = false;

    m_baseMs = m_isLive ? 0 : positionMs;
    m_framesTaken = 0;
    m_partialFrame.clear();
    m_fifo.clear();
    // Auto-cue: skip encoded leading silence only when the track starts
    // from the top (a user seek must land exactly where asked), and trim
    // the encoded trailing silence for any local decode.
    m_leadSkipped = m_isLive || positionMs > 0;
    m_tailTrimmed = m_isLive;
    // The timeline jumps: played-audio history and any scratch in progress
    // are no longer valid.
    m_history.clear();
    m_scratchActive = false;
    m_scratchBuf.clear();
    resetDspState();

    QString error;
    m_proc = spawnDecoder(m_path, positionMs, m_retuneOn && !m_sourceIs432,
                          m_isLive, /*waitForStart*/ true, &error);
    if (!m_proc)
        failTrack(error);
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

// ------------------------------------------------------------ gapless preload

void FxEngine::preloadNext(const QString &path)
{
    if (!path.isEmpty() && path == m_nextPath && (m_nextProc || m_nextProbe))
        return; // this track is already being preloaded

    cancelPreload();

    if (path.isEmpty() || !QFile::exists(path) || ffmpegExecutable().isEmpty())
        return; // nothing to preload — setSource() will take the normal path

    m_nextPath = path;
    m_nextDurationMs = 0;
    m_nextIs432 = QFileInfo(path).completeBaseName()
                      .endsWith(QStringLiteral("_432Hz"));

    const QString ffprobe = ffprobeExecutable();
    if (ffprobe.isEmpty()) {
        spawnPreloadDecoder(); // no duration metadata, but still gapless
        return;
    }

    // The probe runs asynchronously: the current track is still playing,
    // and a blocking wait here would starve the pump.
    m_nextProbe = new QProcess(this);
    QProcess *probe = m_nextProbe;
    connect(probe, &QProcess::finished, this,
            [this, probe, path](int, QProcess::ExitStatus) {
        probe->deleteLater();
        if (probe != m_nextProbe || path != m_nextPath)
            return; // canceled or superseded meanwhile
        m_nextProbe = nullptr;
        parseProbeOutput(QString::fromLocal8Bit(probe->readAllStandardOutput()),
                         &m_nextDurationMs, &m_nextIs432);
        spawnPreloadDecoder();
    });
    QTimer::singleShot(5000, probe, [probe] {
        // A wedged probe must not block the preload forever; killing it
        // still fires finished() and the decoder spawns without a duration.
        if (probe->state() != QProcess::NotRunning)
            probe->kill();
    });
    probe->start(ffprobe, {"-v", "error",
                           "-show_entries", "format=duration:format_tags:stream_tags",
                           "-of", "default=noprint_wrappers=1", path});
}

void FxEngine::cancelPreload()
{
    m_nextPath.clear();

    // Reap asynchronously: cancelPreload can run while a track is playing
    // (superseded preload, mid-track source switch), and waiting for the
    // kill here would starve the pump.
    const auto reap = [this](QProcess *p) {
        if (!p)
            return;
        p->disconnect(this);
        if (p->state() != QProcess::NotRunning) {
            connect(p, &QProcess::finished, p, &QObject::deleteLater);
            p->kill(); // SIGKILL: finished always follows and deletes it
        } else {
            p->deleteLater();
        }
    };

    QProcess *probe = m_nextProbe;
    m_nextProbe = nullptr;
    reap(probe);

    QProcess *proc = m_nextProc;
    m_nextProc = nullptr;
    reap(proc);
}

void FxEngine::spawnPreloadDecoder()
{
    if (m_nextPath.isEmpty() || m_nextProc)
        return;
    QString error;
    const bool retune = m_params.retune432 && !m_nextIs432;
    // No waitForStarted here: the current track is still playing and a
    // blocked engine thread would starve the pump (audible dropout).
    m_nextProc = spawnDecoder(m_nextPath, 0, retune, false,
                              /*waitForStart*/ false, &error);
    if (!m_nextProc) {
        qWarning() << "FxEngine: gapless preload failed:" << error;
        m_nextPath.clear(); // setSource() will fall back to a cold start
        return;
    }
    QProcess *proc = m_nextProc;
    connect(proc, &QProcess::errorOccurred, this,
            [this, proc](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart && proc == m_nextProc) {
            qWarning() << "FxEngine: preload ffmpeg failed to start";
            m_nextProc = nullptr;
            m_nextPath.clear();
            proc->deleteLater();
        }
    });
    qDebug() << "FxEngine: preload decoder ready for" << m_nextPath;
}

void FxEngine::adoptPreloaded()
{
    // After a natural end the finish was announced early (m_finishEmitted)
    // and the pump kept running: the sink still holds the old track's tail,
    // so the new audio queues right behind it — a sample-continuous handoff.
    const bool seamless = m_finishEmitted && m_state == State::Playing;
    // Overlap segue with a crossfade armed: the outgoing decoder moves to
    // the tail mix and fades out inside the same stream — no cut either.
    const bool crossfade = !seamless && m_state == State::Playing
                           && m_nextCrossfadeMs > 0 && !m_scratchActive
                           && (m_proc || !m_fifo.empty());
    qDebug() << "FxEngine: adopting preloaded decoder for" << m_nextPath
             << "seamless=" << seamless << "crossfade=" << crossfade
             << "state=" << int(m_state);

    if (crossfade) {
        stopTailMix();
        m_tailProc = m_proc; // keeps decoding; drained by mixTail()
        m_proc = nullptr;
        if (m_tailProc)
            m_tailProc->disconnect(this);
        m_tailPartial = m_partialFrame;
        m_tailFifo = std::move(m_fifo);
        m_fifo = std::vector<float>();
        m_tailGain = 1.0;
        m_tailGainStep = 1000.0 / (double(qMax(qint64(200), m_nextCrossfadeMs))
                                   * kSampleRate);
    } else {
        stopProcess(); // the old decoder (already exited after a natural end)
        if (!seamless) {
            // Mid-play cut (manual skip) or engine already stopped: the
            // sink content belongs to the old track — start a clean stream.
            if (m_sink)
                m_sink->reset();
            m_io = (m_state == State::Playing && m_sink) ? m_sink->start() : nullptr;
        }
    }
    m_nextCrossfadeMs = 0;

    m_proc = m_nextProc;
    m_nextProc = nullptr;
    m_path = m_nextPath;
    m_nextPath.clear();
    m_durationMs = m_nextDurationMs;
    m_sourceIs432 = m_nextIs432;
    m_isLive = false;

    m_partialFrame.clear();
    m_fifo.clear();
    m_history.clear();
    m_scratchActive = false;
    m_scratchMode = 0;
    m_scratchBuf.clear();
    if (crossfade) {
        // The stream is continuous: zeroing the EQ/compressor state here
        // would click. Only refresh the retune flag for future respawns.
        m_retuneOn = m_params.retune432;
    } else {
        resetDspState();
    }

    m_baseMs = 0;
    m_framesTaken = 0;
    m_pausedPosMs = 0;
    m_producedAudio = false;
    m_finishEmitted = false;
    m_procPreloaded = true; // play() must not respawn the decoder

    // Auto-cue the adopted track. During a crossfade the chunk-drop skip
    // would discard mixed tail audio, and the overlap was computed from
    // the real waveform anyway — so only non-crossfade starts skip.
    m_leadSkipped = crossfade;
    m_tailTrimmed = false;

    emit durationChanged(m_durationMs);
}

bool FxEngine::ensureSink()
{
    // A sink opened on a device that is no longer the default is stale:
    // rebuild on the current default instead of writing into the void.
    if (m_sink) {
        const QAudioDevice def = QMediaDevices::defaultAudioOutput();
        if (def.isNull() || def.id() != m_sinkDeviceId)
            teardownSink();
    }

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
        m_sinkDeviceId = device.id();
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
    m_sinkDeviceId.clear();
}

void FxEngine::rebuildSink()
{
    qDebug() << "FxEngine: rebuilding audio sink on request";
    const bool wasActive = (m_state == State::Playing && m_io);
    teardownSink();
    if (wasActive && !ensureSink())
        failTrack(tr("No usable audio output device for the FX engine"));
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
            parseProbeOutput(QString::fromLocal8Bit(probe.readAllStandardOutput()),
                             &m_durationMs, &m_sourceIs432);
            if (m_durationMs > 0)
                return;
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

namespace
{
/** Move decoded PCM from a decoder process into a float FIFO, keeping
    sub-frame leftovers in `partial` between reads. */
void drainDecoder(QProcess *proc, QByteArray &partial, std::vector<float> &fifo)
{
    if (!proc)
        return;

    QByteArray data = proc->readAllStandardOutput();
    if (data.isEmpty())
        return;

    if (!partial.isEmpty()) {
        data.prepend(partial);
        partial.clear();
    }

    constexpr int bytesPerInFrame = sizeof(float) * 2; // kChannels
    const int completeBytes = (data.size() / bytesPerInFrame) * bytesPerInFrame;
    if (completeBytes < data.size()) {
        partial = data.mid(completeBytes);
        data.truncate(completeBytes);
    }
    if (data.isEmpty())
        return;

    const float *samples = reinterpret_cast<const float *>(data.constData());
    fifo.insert(fifo.end(), samples, samples + completeBytes / sizeof(float));
}
} // namespace

void FxEngine::readProcessOutput()
{
    drainDecoder(m_proc, m_partialFrame, m_fifo);
}

void FxEngine::setNextCrossfade(qint64 fadeMs)
{
    m_nextCrossfadeMs = qMax(qint64(0), fadeMs);
}

void FxEngine::mixTail(float *out, int frames)
{
    if (!m_tailProc && m_tailFifo.empty())
        return;

    drainDecoder(m_tailProc, m_tailPartial, m_tailFifo);

    const int avail = static_cast<int>(m_tailFifo.size() / kChannels);
    const int n = std::min(frames, avail);
    for (int i = 0; i < n * kChannels; i += kChannels) {
        const float g = static_cast<float>(m_tailGain);
        out[i] += m_tailFifo[i] * g;
        out[i + 1] += m_tailFifo[i + 1] * g;
        m_tailGain = std::max(0.0, m_tailGain - m_tailGainStep);
    }
    if (n > 0)
        m_tailFifo.erase(m_tailFifo.begin(), m_tailFifo.begin() + n * kChannels);

    const bool exhausted = m_tailFifo.empty()
        && (!m_tailProc || m_tailProc->state() == QProcess::NotRunning)
        && (!m_tailProc || m_tailProc->bytesAvailable() == 0);
    if (m_tailGain <= 0.0 || exhausted)
        stopTailMix();
}

void FxEngine::stopTailMix()
{
    if (m_tailProc) {
        QProcess *proc = m_tailProc;
        m_tailProc = nullptr;
        proc->disconnect(this);
        if (proc->state() != QProcess::NotRunning) {
            connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
            proc->kill(); // async reap: never block the pump
        } else {
            proc->deleteLater();
        }
    }
    m_tailPartial.clear();
    m_tailFifo.clear();
    m_tailGain = 0.0;
    m_tailGainStep = 0.0;
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
    // Feed the level meter from the final samples actually sent out
    for (int i = 0; i < frames * kChannels; i += kChannels) {
        m_meterPeakL = std::max(m_meterPeakL, std::abs(chunk[i]));
        m_meterPeakR = std::max(m_meterPeakR, std::abs(chunk[i + 1]));
    }

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

    // Auto-cue, tail side: once the decoder has delivered everything, chop
    // the encoded trailing silence off the fifo so the track finishes where
    // its audio does (YouTube rips carry seconds of outro silence, which a
    // gapless handoff would otherwise play in full).
    if (!m_tailTrimmed && m_proc && m_proc->state() == QProcess::NotRunning
            && m_proc->bytesAvailable() == 0) {
        m_tailTrimmed = true;
        size_t end = m_fifo.size();
        while (end >= static_cast<size_t>(kChannels)
               && std::abs(m_fifo[end - 1]) <= kSilenceFloor
               && std::abs(m_fifo[end - 2]) <= kSilenceFloor) {
            end -= kChannels;
        }
        if (end < m_fifo.size()) {
            qDebug() << "FxEngine: auto-cue trimmed"
                     << (m_fifo.size() - end) / kChannels * 1000 / kSampleRate
                     << "ms of trailing silence";
            m_fifo.resize(end);
        }
    }

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

        // Auto-cue, head side: drop whole-silent chunks until the first
        // audible frame so the track starts where its audio does.
        if (!m_leadSkipped) {
            int firstLoud = -1;
            for (int i = 0; i < got * kChannels; ++i) {
                if (std::abs(m_chunk[i]) > kSilenceFloor) {
                    firstLoud = i / kChannels;
                    break;
                }
            }
            if (firstLoud < 0) {
                if (currentPositionMs() - m_baseMs > kLeadSkipCapMs)
                    m_leadSkipped = true; // quiet piece, stop scanning
                continue; // silent lead: consume without writing
            }
            m_leadSkipped = true;
            if (firstLoud > 0) {
                std::memmove(m_chunk.data(),
                             m_chunk.data() + firstLoud * kChannels,
                             (got - firstLoud) * kChannels * sizeof(float));
            }
            const int audible = got - firstLoud;
            mixTail(m_chunk.data(), audible);
            applyFxChain(m_chunk.data(), audible);
            writeChunkToSink(m_chunk.data(), audible);
            continue;
        }

        mixTail(m_chunk.data(), got); // fading crossfade tail, when active
        applyFxChain(m_chunk.data(), got);
        writeChunkToSink(m_chunk.data(), got);
    }

    // Level meter: emit the accumulated output peaks ~22 times per second,
    // scaled by the sink volume so the LEDs track what is actually audible.
    if (++m_meterEmitDivider >= 3) {
        m_meterEmitDivider = 0;
        emit levels(std::min(1.0f, m_meterPeakL * m_volume),
                    std::min(1.0f, m_meterPeakR * m_volume));
        m_meterPeakL = 0.0f;
        m_meterPeakR = 0.0f;
    }

    // Emit the playback position roughly four times per second. Report the
    // audible position: the decode position runs up to a full sink buffer
    // (~350 ms) ahead of the speakers, which made the volume line, the
    // crossfade trigger and the tail handoff all act early.
    if (++m_positionEmitDivider >= 16) {
        m_positionEmitDivider = 0;
        qint64 bufferedMs = 0;
        if (m_sink && m_io) {
            const qint64 bufferedBytes = m_sink->bufferSize() - m_sink->bytesFree();
            bufferedMs = (bufferedBytes / bytesPerOutFrame) * 1000 / kSampleRate;
        }
        emit positionChanged(std::max<qint64>(0, currentPositionMs() - bufferedMs));
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

    // Early finish: when the next track is already preloaded, announce the
    // end while the sink is still draining the tail. The handoff then
    // adopts the new decoder under the live sink and playback is gapless.
    if (!m_finishEmitted && m_nextProc
            && (m_nextProc->state() == QProcess::Running
                || m_nextProc->bytesAvailable() > 0)) {
        m_finishEmitted = true;
        stopProcess();
        qDebug() << "FxEngine: early finish, handoff armed for" << m_nextPath;
        const qint64 finalPos = (m_durationMs > 0) ? m_durationMs : currentPositionMs();
        emit positionChanged(finalPos);
        emit playbackFinished();
        return; // the pump keeps ticking until the handoff (or full drain)
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
    if (!m_finishEmitted) {
        emit positionChanged(finalPos);
        emit playbackFinished();
    }
    m_finishEmitted = false;
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
