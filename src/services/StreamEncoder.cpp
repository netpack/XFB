#include "StreamEncoder.h"

#include "../audio/FxEngine.h"

#include <QDebug>
#include <QProcess>
#include <QTimer>

StreamEncoder::StreamEncoder(QObject *parent)
    : QObject(parent)
{
    m_restartTimer = new QTimer(this);
    m_restartTimer->setSingleShot(true);
    connect(m_restartTimer, &QTimer::timeout, this, [this]() {
        if (m_wantRunning)
            spawn();
    });
}

StreamEncoder::~StreamEncoder()
{
    m_wantRunning = false;
    killProcess();
}

QString StreamEncoder::contentTypeFor(Codec codec)
{
    switch (codec) {
    case Codec::Opus: return QStringLiteral("audio/ogg");
    case Codec::Mp3:  break;
    }
    return QStringLiteral("audio/mpeg");
}

QString StreamEncoder::codecName(Codec codec)
{
    switch (codec) {
    case Codec::Opus: return QStringLiteral("opus");
    case Codec::Mp3:  break;
    }
    return QStringLiteral("mp3");
}

StreamEncoder::Codec StreamEncoder::codecFromName(const QString &name)
{
    if (name.compare(QStringLiteral("opus"), Qt::CaseInsensitive) == 0)
        return Codec::Opus;
    return Codec::Mp3;
}

QList<int> StreamEncoder::bitratesFor(Codec codec)
{
    if (codec == Codec::Opus)
        return {32, 48, 64, 96, 128, 160};
    return {64, 96, 128, 160, 192, 256, 320};
}

bool StreamEncoder::isRunning() const
{
    return m_proc && m_proc->state() == QProcess::Running;
}

void StreamEncoder::setConfig(const Config &config)
{
    const bool changed = config.codec != m_config.codec
            || config.bitrateKbps != m_config.bitrateKbps
            || config.sampleRate != m_config.sampleRate
            || config.channels != m_config.channels;
    m_config = config;
    if (changed && m_wantRunning) {
        // A codec or bitrate change cannot be applied to a running encoder;
        // the mount has to be re-established anyway.
        killProcess();
        spawn();
    }
}

QStringList StreamEncoder::ffmpegArguments() const
{
    QStringList args;
    args << QStringLiteral("-hide_banner")
         << QStringLiteral("-loglevel") << QStringLiteral("error")
         << QStringLiteral("-nostdin")
         // Input: exactly what the FxEngine tap delivers.
         << QStringLiteral("-f") << QStringLiteral("s16le")
         << QStringLiteral("-ar") << QString::number(m_config.sampleRate)
         << QStringLiteral("-ac") << QString::number(m_config.channels)
         << QStringLiteral("-i") << QStringLiteral("pipe:0");

    switch (m_config.codec) {
    case Codec::Opus:
        args << QStringLiteral("-c:a") << QStringLiteral("libopus")
             << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(m_config.bitrateKbps)
             << QStringLiteral("-vbr") << QStringLiteral("constrained")
             << QStringLiteral("-application") << QStringLiteral("audio")
             // Short Ogg pages: a page is only sent once it is full, and a
             // long one shows up as seconds of latency on the listener side.
             << QStringLiteral("-page_duration") << QStringLiteral("20000")
             << QStringLiteral("-f") << QStringLiteral("ogg");
        break;
    case Codec::Mp3:
        args << QStringLiteral("-c:a") << QStringLiteral("libmp3lame")
             << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(m_config.bitrateKbps)
             // A Xing/LAME header describes a file with a known length; in an
             // endless stream it is nonsense and confuses some players.
             << QStringLiteral("-write_xing") << QStringLiteral("0")
             // No ID3v2 tag either: ffmpeg writes one by default, and a
             // listener joining mid-stream gets it as a burst of garbage
             // ahead of the first frame it can decode.
             << QStringLiteral("-id3v2_version") << QStringLiteral("0")
             << QStringLiteral("-f") << QStringLiteral("mp3");
        break;
    }

    args << QStringLiteral("-flush_packets") << QStringLiteral("1")
         << QStringLiteral("pipe:1");
    return args;
}

void StreamEncoder::start()
{
    if (m_wantRunning)
        return;
    m_wantRunning = true;
    m_restartDelayMs = kFirstRestartDelayMs;
    spawn();
}

void StreamEncoder::stop()
{
    if (!m_wantRunning && !m_proc)
        return;
    m_wantRunning = false;
    m_restartTimer->stop();
    killProcess();
    emit runningChanged(false);
}

void StreamEncoder::spawn()
{
    killProcess();

    const QString ffmpeg = FxEngine::ffmpegExecutable();
    if (ffmpeg.isEmpty()) {
        // Nothing to retry against: without ffmpeg the encoder can never
        // work, so say so once and stay down.
        m_wantRunning = false;
        emit logMessage(tr("ffmpeg was not found - the built-in encoder needs it."));
        emit runningChanged(false);
        return;
    }

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_proc, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray data = m_proc->readAllStandardOutput();
        if (!data.isEmpty())
            emit encoded(data);
    });
    connect(m_proc, &QProcess::readyReadStandardError, this, [this]() {
        const QString err = QString::fromUtf8(m_proc->readAllStandardError()).trimmed();
        if (!err.isEmpty())
            emit logMessage(tr("encoder: %1").arg(err));
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            scheduleRestart(tr("ffmpeg failed to start"));
    });
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
        if (!m_wantRunning)
            return;
        scheduleRestart(status == QProcess::CrashExit
                            ? tr("ffmpeg crashed")
                            : tr("ffmpeg exited with code %1").arg(code));
    });

    m_proc->start(ffmpeg, ffmpegArguments());
    if (!m_proc->waitForStarted(5000)) {
        scheduleRestart(tr("ffmpeg did not start"));
        return;
    }

    // A start that survives its first seconds is a healthy one; reset the
    // backoff so a single hiccup hours from now still recovers quickly.
    QTimer::singleShot(10000, this, [this]() {
        if (isRunning())
            m_restartDelayMs = kFirstRestartDelayMs;
    });

    m_dropping = false;
    m_droppedBytes = 0;
    emit logMessage(tr("Encoder started: %1 at %2 kbit/s.")
                        .arg(codecName(m_config.codec).toUpper())
                        .arg(m_config.bitrateKbps));
    emit runningChanged(true);
}

void StreamEncoder::killProcess()
{
    if (!m_proc)
        return;

    QProcess *proc = m_proc;
    m_proc = nullptr;                 // the finished handler must not respawn
    proc->disconnect(this);
    if (proc->state() != QProcess::NotRunning) {
        proc->closeWriteChannel();    // let ffmpeg flush its last frames
        if (!proc->waitForFinished(1000)) {
            proc->kill();
            proc->waitForFinished(1000);
        }
    }
    proc->deleteLater();
}

void StreamEncoder::scheduleRestart(const QString &reason)
{
    killProcess();
    emit runningChanged(false);
    if (!m_wantRunning)
        return;

    emit logMessage(tr("%1 - restarting the encoder in %2 s.")
                        .arg(reason)
                        .arg(m_restartDelayMs / 1000.0, 0, 'g', 2));
    m_restartTimer->start(m_restartDelayMs);
    m_restartDelayMs = qMin(kMaxRestartDelayMs, m_restartDelayMs * 2);
}

void StreamEncoder::writePcm(const QByteArray &pcm)
{
    if (pcm.isEmpty() || !isRunning())
        return;

    if (m_proc->bytesToWrite() > kMaxPendingBytes) {
        // Queuing this would only add latency to audio that is already late.
        m_droppedBytes += pcm.size();
        if (!m_dropping) {
            m_dropping = true;
            emit logMessage(tr("The encoder is not keeping up; dropping audio."));
        }
        return;
    }

    if (m_dropping) {
        m_dropping = false;
        emit logMessage(tr("The encoder caught up (%1 kB of audio was dropped).")
                            .arg(m_droppedBytes / 1024));
        m_droppedBytes = 0;
    }

    m_proc->write(pcm);
}
