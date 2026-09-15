#include "ProgramRecorder.h"

#include "FxEngine.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QDebug>
#include <QMediaDevices>
#include <QProcess>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace {

// The encoders this ffmpeg has, asked once. A build without libmp3lame (or
// libvorbis, which Homebrew's ffmpeg lacks) is common enough that a format
// has to be checked against it rather than assumed.
QStringList ffmpegEncoders(const QString &ffmpeg)
{
    static bool probed = false;
    static QStringList found;
    if (probed)
        return found;
    probed = true;

    QProcess probe;
    probe.start(ffmpeg, {QStringLiteral("-hide_banner"), QStringLiteral("-encoders")});
    if (!probe.waitForFinished(5000)) {
        probe.kill();
        probe.waitForFinished(1000);
        return found;
    }
    // Lines look like " A....D libmp3lame           libmp3lame MP3 (...)".
    const QStringList lines = QString::fromUtf8(probe.readAllStandardOutput()).split(u'\n');
    for (const QString &line : lines) {
        const QStringList fields = line.simplified().split(u' ');
        if (fields.size() >= 2 && fields.at(0).startsWith(u'A'))
            found << fields.at(1);
    }
    return found;
}

} // namespace

ProgramRecorder::ProgramRecorder(QObject *parent)
    : QObject(parent)
{
    m_tick = new QTimer(this);
    m_tick->setTimerType(Qt::PreciseTimer);
    m_tick->setInterval(20);
    connect(m_tick, &QTimer::timeout, this, &ProgramRecorder::mixTick);
}

ProgramRecorder::~ProgramRecorder()
{
    stop();
}

bool ProgramRecorder::available()
{
    return !FxEngine::ffmpegExecutable().isEmpty();
}

QString ProgramRecorder::formatCode(Format format)
{
    switch (format) {
    case Format::Mp3:       return QStringLiteral("mp3");
    case Format::Opus:      return QStringLiteral("opus");
    case Format::OggVorbis: break;
    }
    return QStringLiteral("ogg");
}

ProgramRecorder::Format ProgramRecorder::formatFromCode(const QString &code)
{
    if (code == QLatin1String("mp3"))
        return Format::Mp3;
    if (code == QLatin1String("opus"))
        return Format::Opus;
    return Format::OggVorbis;
}

QString ProgramRecorder::extensionFor(Format format)
{
    return formatCode(format);
}

QList<int> ProgramRecorder::bitratesFor(Format format)
{
    switch (format) {
    case Format::Mp3:       return {128, 160, 192, 256, 320};
    case Format::Opus:      return {64, 96, 128, 160, 192};
    case Format::OggVorbis: break;
    }
    return {128, 160, 192, 256, 320};
}

int ProgramRecorder::nearestBitrate(Format format, int kbps)
{
    const QList<int> offered = bitratesFor(format);
    int best = offered.first();
    for (int candidate : offered) {
        if (std::abs(candidate - kbps) < std::abs(best - kbps))
            best = candidate;
    }
    return best;
}

bool ProgramRecorder::start(const QString &path, const Config &config, QString *error)
{
    const auto refuse = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    if (m_recording)
        return refuse(tr("A recording is already running."));

    const QString ffmpeg = FxEngine::ffmpegExecutable();
    if (ffmpeg.isEmpty())
        return refuse(tr("Recording a programme needs ffmpeg, and it was not found."));

    QString unsupported;
    const QStringList arguments = encoderArguments(path, config, &unsupported);
    if (arguments.isEmpty())
        return refuse(unsupported);

    m_lanes.clear();
    m_inputLane = Lane();
    m_framesMixed = 0;
    m_paused = false;
    m_dropping = false;
    m_encoderErrors.clear();

    m_encoder = new QProcess(this);
    m_encoder->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_encoder, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromUtf8(m_encoder->readAllStandardError()).trimmed();
        if (text.isEmpty())
            return;
        qWarning() << "ProgramRecorder: ffmpeg:" << text;
        m_encoderErrors = text.right(400);
    });
    connect(m_encoder, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        // A deliberate stop() closes stdin and waits; anything else ending the
        // encoder while recording is a failure the operator has to hear about.
        if (!m_recording)
            return;
        fail(m_encoderErrors.isEmpty()
                 ? tr("The recording encoder stopped unexpectedly (exit code %1).").arg(code)
                 : tr("The recording encoder stopped: %1").arg(m_encoderErrors));
    });

    m_encoder->start(ffmpeg, arguments);
    if (!m_encoder->waitForStarted(5000)) {
        const QString reason = m_encoder->errorString();
        delete m_encoder;
        m_encoder = nullptr;
        return refuse(tr("ffmpeg could not be started for the recording: %1").arg(reason));
    }

    if (config.includeInput && !openInput(config.inputDeviceDescription, error)) {
        m_encoder->closeWriteChannel();
        if (!m_encoder->waitForFinished(3000))
            m_encoder->kill();
        delete m_encoder;
        m_encoder = nullptr;
        return false;
    }

    m_recording = true;
    m_clock.start();
    m_tick->start();
    qInfo() << "ProgramRecorder: recording"
            << (m_source ? m_inputDescription : QStringLiteral("no input device"))
            << "plus any on-air feeds," << formatCode(config.format)
            << config.bitrateKbps << "kbit/s ->" << path;
    return true;
}

void ProgramRecorder::stop()
{
    if (!m_recording)
        return;

    // Everything heard up to this moment goes in. What is still ahead of the
    // clock — an engine's sink buffer — has not been heard yet and stays out.
    if (m_source)
        pullInput();
    mixUntil(nowFrames());

    m_recording = false;
    m_tick->stop();
    closeInput();

    if (m_encoder) {
        m_encoder->closeWriteChannel();
        if (!m_encoder->waitForFinished(10000)) {
            qWarning() << "ProgramRecorder: ffmpeg did not finish the file in time";
            m_encoder->kill();
            m_encoder->waitForFinished(2000);
        }
        delete m_encoder;
        m_encoder = nullptr;
    }

    m_lanes.clear();
    m_inputLane = Lane();
    qInfo() << "ProgramRecorder: stopped after" << m_framesMixed / kRate << "s";
}

void ProgramRecorder::setPaused(bool paused)
{
    m_paused = paused;
}

void ProgramRecorder::fail(const QString &message)
{
    if (!m_recording)
        return;
    m_recording = false;
    m_tick->stop();
    closeInput();
    if (m_encoder) {
        m_encoder->disconnect(this);
        if (m_encoder->state() != QProcess::NotRunning)
            m_encoder->kill();
        m_encoder->deleteLater();
        m_encoder = nullptr;
    }
    m_lanes.clear();
    m_inputLane = Lane();
    qWarning() << "ProgramRecorder:" << message;
    emit failed(message);
}

QStringList ProgramRecorder::encoderArguments(const QString &path, const Config &config,
                                              QString *error) const
{
    const QStringList encoders = ffmpegEncoders(FxEngine::ffmpegExecutable());
    const QString bitrate = QStringLiteral("%1k").arg(
        nearestBitrate(config.format, config.bitrateKbps));

    QStringList codec;
    QString muxer;
    switch (config.format) {
    case Format::Mp3:
        if (!encoders.contains(QLatin1String("libmp3lame"))) {
            if (error)
                *error = tr("This ffmpeg cannot write MP3 (it has no libmp3lame). "
                            "Choose Ogg Vorbis or Opus in Options.");
            return {};
        }
        codec << QStringLiteral("-c:a") << QStringLiteral("libmp3lame")
              << QStringLiteral("-b:a") << bitrate;
        muxer = QStringLiteral("mp3");
        break;
    case Format::OggVorbis:
        // ffmpeg's own Vorbis encoder is still marked experimental and ignores
        // the bitrate, but a build without libvorbis (Homebrew's) has nothing
        // else that writes Vorbis.
        if (encoders.contains(QLatin1String("libvorbis")))
            codec << QStringLiteral("-c:a") << QStringLiteral("libvorbis")
                  << QStringLiteral("-b:a") << bitrate;
        else
            codec << QStringLiteral("-c:a") << QStringLiteral("vorbis")
                  << QStringLiteral("-strict") << QStringLiteral("-2");
        muxer = QStringLiteral("ogg");
        break;
    case Format::Opus:
        if (encoders.contains(QLatin1String("libopus")))
            codec << QStringLiteral("-c:a") << QStringLiteral("libopus")
                  << QStringLiteral("-b:a") << bitrate
                  << QStringLiteral("-application") << QStringLiteral("audio");
        else
            codec << QStringLiteral("-c:a") << QStringLiteral("opus")
                  << QStringLiteral("-b:a") << bitrate
                  << QStringLiteral("-strict") << QStringLiteral("-2");
        muxer = QStringLiteral("ogg");
        break;
    }

    QStringList args;
    args << QStringLiteral("-hide_banner")
         << QStringLiteral("-loglevel") << QStringLiteral("error")
         << QStringLiteral("-nostdin")
         << QStringLiteral("-f") << QStringLiteral("s16le")
         << QStringLiteral("-ar") << QString::number(kRate)
         << QStringLiteral("-ac") << QString::number(kChannels)
         << QStringLiteral("-i") << QStringLiteral("pipe:0")
         << codec
         // Written out packet by packet: player.cpp's RecCHK judges a
         // recording by its file size a few seconds in, and ffmpeg's own 32 KB
         // buffer would keep a quiet start at zero bytes long enough to raise
         // a false alarm.
         << QStringLiteral("-flush_packets") << QStringLiteral("1")
         << QStringLiteral("-f") << muxer
         << QStringLiteral("-y") << path;
    return args;
}

qint64 ProgramRecorder::nowFrames() const
{
    return m_clock.nsecsElapsed() * kRate / 1000000000LL;
}

void ProgramRecorder::appendToLane(Lane &lane, const float *interleaved, qint64 frames)
{
    const qint64 nowMs = m_clock.elapsed();

    // After a quiet spell a lane is placed by when its audio arrived, not
    // glued to the end of what it last sent: a deck started ten seconds after
    // the playlist must come out ten seconds later, not straight after it.
    // While audio flows the lane stays contiguous, so a burst never tears.
    const bool quiet = lane.lastArrivalMs < 0 || lane.frames() == 0
                       || nowMs - lane.lastArrivalMs > 250;
    if (quiet) {
        const qint64 laneEnd = m_framesMixed + lane.frames();
        const qint64 gap = nowFrames() - laneEnd;
        if (gap > 0)
            lane.samples.insert(lane.samples.end(), size_t(gap * kChannels), 0.0f);
    }
    lane.lastArrivalMs = nowMs;

    lane.samples.insert(lane.samples.end(), interleaved,
                        interleaved + size_t(frames * kChannels));
}

void ProgramRecorder::feed(const QObject *key, const QByteArray &pcm, int sampleRate,
                         int channels, float volume)
{
    if (!m_recording || pcm.isEmpty())
        return;
    if (sampleRate != kRate || channels != kChannels) {
        // The engine's tap format is fixed; if that ever changes this has to
        // learn to convert rather than write noise.
        static bool warned = false;
        if (!warned) {
            warned = true;
            qWarning() << "ProgramRecorder: unexpected tap format" << sampleRate << channels;
        }
        return;
    }

    const qint64 frames = pcm.size() / (sizeof(qint16) * kChannels);
    if (frames <= 0)
        return;

    const auto *in = reinterpret_cast<const qint16 *>(pcm.constData());
    const float gain = std::clamp(volume, 0.0f, 1.0f) / 32768.0f;
    std::vector<float> converted(size_t(frames * kChannels));
    for (size_t i = 0; i < converted.size(); ++i)
        converted[i] = float(in[i]) * gain;

    appendToLane(m_lanes[key], converted.data(), frames);
}

void ProgramRecorder::mixTick()
{
    if (!m_recording)
        return;

    if (m_source)
        pullInput();
    mixUntil(nowFrames() - kLatencyFrames);
}

void ProgramRecorder::mixUntil(qint64 due)
{
    const qint64 nowMs = m_clock.elapsed();

    while (m_framesMixed < due) {
        // A main thread that stalled for a while is caught up in one-second
        // pieces, so a long stall never needs one huge allocation.
        const qint64 n = std::min<qint64>(due - m_framesMixed, kRate);
        m_mix.assign(size_t(n * kChannels), 0.0f);

        const auto mixLane = [&](Lane &lane) {
            const bool live = lane.lastArrivalMs >= 0 && nowMs - lane.lastArrivalMs < 1000;
            for (qint64 i = 0; i < n && lane.frames() > 0; ++i) {
                m_mix[size_t(i * 2)] += lane.samples[lane.head];
                m_mix[size_t(i * 2 + 1)] += lane.samples[lane.head + 1];

                // Clock drift: the wall clock and the sound card never agree
                // exactly. A lane creeping too far ahead loses a frame now and
                // then, one running dry while it is still live gains one.
                bool advance = true;
                if (++lane.correctionCounter >= kCorrectionPeriod) {
                    lane.correctionCounter = 0;
                    const qint64 fill = lane.frames();
                    if (fill > kTooFullFrames && fill > 2)
                        lane.head += kChannels;
                    else if (live && fill < kTooEmptyFrames)
                        advance = false;
                }
                if (advance)
                    lane.head += kChannels;
            }
            if (lane.head > size_t(kRate * kChannels) && lane.head * 2 > lane.samples.size()) {
                lane.samples.erase(lane.samples.begin(),
                                   lane.samples.begin() + std::ptrdiff_t(lane.head));
                lane.head = 0;
            }
        };

        for (Lane &lane : m_lanes)
            mixLane(lane);
        mixLane(m_inputLane);

        m_framesMixed += n;

        if (m_paused || !m_encoder)
            continue;

        m_pcmOut.resize(qsizetype(n * kChannels * sizeof(qint16)));
        auto *out = reinterpret_cast<qint16 *>(m_pcmOut.data());
        for (size_t i = 0; i < m_mix.size(); ++i) {
            const float v = std::clamp(m_mix[i], -1.0f, 1.0f);
            out[i] = qint16(std::lround(v * 32767.0f));
        }

        // ffmpeg writing a file is far faster than real time, so a backlog
        // means it is wedged. Past a generous limit new audio is dropped
        // instead of growing memory until the machine that is on air swaps.
        if (m_encoder->bytesToWrite() > kMaxPendingBytes) {
            if (!m_dropping) {
                m_dropping = true;
                qWarning() << "ProgramRecorder: the encoder is not keeping up, audio is being dropped";
            }
            continue;
        }
        if (m_dropping) {
            m_dropping = false;
            qInfo() << "ProgramRecorder: the encoder caught up";
        }
        m_encoder->write(m_pcmOut);
    }
}

bool ProgramRecorder::openInput(const QString &description, QString *error)
{
    QAudioDevice device;
    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice &d : inputs) {
        if (d.description() == description) {
            device = d;
            break;
        }
    }
    if (device.isNull())
        device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        if (error)
            *error = tr("No microphone or line input was found on this computer.");
        return false;
    }

    // Ask for the mix format first so nothing needs converting; otherwise take
    // what the device prefers and resample it here.
    QAudioFormat format;
    format.setSampleRate(kRate);
    format.setChannelCount(kChannels);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format)) {
        format.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(format)) {
            format.setChannelCount(1);
            if (!device.isFormatSupported(format))
                format = device.preferredFormat();
        }
    }
    if (format.sampleRate() <= 0 || format.channelCount() <= 0
            || format.bytesPerSample() <= 0) {
        if (error)
            *error = tr("%1 does not report a usable recording format.")
                         .arg(device.description());
        return false;
    }

    m_source = new QAudioSource(device, format, this);
    m_input = m_source->start();
    if (!m_input) {
        delete m_source;
        m_source = nullptr;
        if (error)
            *error = tr("%1 could not be opened for recording.").arg(device.description());
        return false;
    }

    m_inputFormat = format;
    m_inputDescription = device.description();
    m_inputRemainder.clear();
    m_inputCarry.clear();
    m_resamplePos = 0.0;
    connect(m_input, &QIODevice::readyRead, this, &ProgramRecorder::pullInput);
    qInfo() << "ProgramRecorder: input" << m_inputDescription << format.sampleRate()
            << "Hz" << format.channelCount() << "ch" << format.sampleFormat();
    return true;
}

void ProgramRecorder::closeInput()
{
    if (m_source) {
        m_source->stop();
        delete m_source;
        m_source = nullptr;
    }
    m_input = nullptr;
    m_inputDescription.clear();
}

void ProgramRecorder::pullInput()
{
    if (!m_recording || !m_input)
        return;

    QByteArray bytes = m_inputRemainder + m_input->readAll();
    const int bytesPerFrame = m_inputFormat.bytesPerFrame();
    const qint64 frames = bytes.size() / bytesPerFrame;
    m_inputRemainder = bytes.right(bytes.size() - frames * bytesPerFrame);
    if (frames <= 0)
        return;

    // Any channel layout becomes stereo: mono is doubled, anything wider keeps
    // its first two channels.
    const int inChannels = m_inputFormat.channelCount();
    const int bytesPerSample = m_inputFormat.bytesPerSample();
    std::vector<float> stereo;
    stereo.reserve(m_inputCarry.size() + size_t(frames * kChannels));
    stereo.insert(stereo.end(), m_inputCarry.begin(), m_inputCarry.end());
    const char *data = bytes.constData();
    for (qint64 f = 0; f < frames; ++f) {
        const char *frame = data + f * bytesPerFrame;
        const float left = m_inputFormat.normalizedSampleValue(frame);
        const float right = inChannels > 1
                                ? m_inputFormat.normalizedSampleValue(frame + bytesPerSample)
                                : left;
        stereo.push_back(left);
        stereo.push_back(right);
    }

    const int inRate = m_inputFormat.sampleRate();
    if (inRate == kRate) {
        appendToLane(m_inputLane, stereo.data() + m_inputCarry.size(), frames);
        return;
    }

    // Linear interpolation is plenty for a microphone going into a programme
    // recording. The last input frame is carried over so the join between two
    // reads interpolates across it instead of clicking.
    const qint64 available = qint64(stereo.size() / kChannels);
    const double step = double(inRate) / double(kRate);
    std::vector<float> resampled;
    resampled.reserve(size_t(double(available) / step + 2) * kChannels);
    double pos = m_resamplePos;
    while (pos + 1.0 < double(available)) {
        const qint64 i = qint64(pos);
        const float frac = float(pos - double(i));
        for (int c = 0; c < kChannels; ++c) {
            const float a = stereo[size_t(i * kChannels + c)];
            const float b = stereo[size_t((i + 1) * kChannels + c)];
            resampled.push_back(a + (b - a) * frac);
        }
        pos += step;
    }
    m_resamplePos = pos - double(available - 1);
    m_inputCarry.assign(stereo.end() - kChannels, stereo.end());

    if (!resampled.empty())
        appendToLane(m_inputLane, resampled.data(), qint64(resampled.size() / kChannels));
}
