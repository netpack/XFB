#include "VoiceRecorder.h"

#include "FxEngine.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

namespace
{
constexpr int kHeaderBytes = 44;

void putTag(QFile *f, const char *tag)
{
    f->write(tag, 4);
}

void putU32(QFile *f, quint32 v)
{
    char b[4] = {char(v & 0xFF), char((v >> 8) & 0xFF), char((v >> 16) & 0xFF),
                 char((v >> 24) & 0xFF)};
    f->write(b, 4);
}

void putU16(QFile *f, quint16 v)
{
    char b[2] = {char(v & 0xFF), char((v >> 8) & 0xFF)};
    f->write(b, 2);
}
} // namespace

VoiceRecorder::VoiceRecorder(QObject *parent)
    : QObject(parent)
{
}

VoiceRecorder::~VoiceRecorder()
{
    if (m_recording)
        stop();
    delete m_source;
    delete m_file;
}

QStringList VoiceRecorder::inputDeviceNames()
{
    QStringList names;
    const QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    for (const QAudioDevice &d : devices)
        if (!names.contains(d.description()))
            names << d.description();
    return names;
}

QString VoiceRecorder::defaultInputDeviceName()
{
    const QAudioDevice d = QMediaDevices::defaultAudioInput();
    return d.isNull() ? QString() : d.description();
}

QString VoiceRecorder::takeFolder()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/.xfb");
    const QString folder = base + QStringLiteral("/voicetracks");
    QDir dir;
    if (!dir.mkpath(folder)) {
        qWarning() << "VoiceRecorder: cannot create the voice-track folder" << folder;
        return QString();
    }
    return folder;
}

QString VoiceRecorder::newTakePath(const QString &hint)
{
    const QString folder = takeFolder();
    if (folder.isEmpty())
        return QString();

    QString stem = hint;
    // The take's name ends up in the playlist and in playlist XML, so keep it
    // to characters every filesystem and every XML reader is happy with.
    stem.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9 _-]")), QString());
    stem = stem.trimmed().left(40);
    if (stem.isEmpty())
        stem = QStringLiteral("link");

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
    QString path = QStringLiteral("%1/VT %2 %3.wav").arg(folder, stamp, stem);
    int n = 2;
    while (QFileInfo::exists(path))
        path = QStringLiteral("%1/VT %2 %3 (%4).wav").arg(folder, stamp, stem).arg(n++);
    return path;
}

bool VoiceRecorder::start(const QString &deviceDescription, const QString &wavPath,
                          QString *error)
{
    const auto fail = [error](const QString &msg) {
        if (error)
            *error = msg;
        return false;
    };

    if (m_recording)
        return fail(tr("A take is already being recorded."));
    if (wavPath.isEmpty())
        return fail(tr("There is nowhere to write the take."));

    // Same resolution rule as the program recorder: match on description,
    // fall back to the system default rather than refusing to record.
    QAudioDevice device;
    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice &d : inputs) {
        if (d.description() == deviceDescription) {
            device = d;
            break;
        }
    }
    if (device.isNull())
        device = QMediaDevices::defaultAudioInput();
    if (device.isNull())
        return fail(tr("No microphone or line input was found on this computer."));

    QAudioFormat format = device.preferredFormat();
    format.setSampleFormat(QAudioFormat::Int16); // what a WAV can hold
    if (format.channelCount() < 1 || format.channelCount() > 2)
        format.setChannelCount(qMin(2, qMax(1, format.channelCount())));
    if (format.sampleRate() <= 0)
        format.setSampleRate(48000);
    if (!device.isFormatSupported(format)) {
        QAudioFormat fallback;
        fallback.setSampleRate(48000);
        fallback.setChannelCount(1);
        fallback.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(fallback))
            return fail(tr("%1 cannot record 16-bit audio, which is what a voice "
                           "track is written as.").arg(device.description()));
        format = fallback;
    }

    auto *file = new QFile(wavPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString msg = tr("The take could not be written to %1: %2")
                                .arg(wavPath, file->errorString());
        delete file;
        return fail(msg);
    }
    writePlaceholderHeader(file);

    m_file = file;
    m_format = format;
    m_path = wavPath;
    m_dataBytes = 0;
    m_lastProgressMs = -1;

    m_source = new QAudioSource(device, format, this);
    m_input = m_source->start();
    if (!m_input) {
        const QString msg = tr("%1 could not be opened for recording.")
                                .arg(device.description());
        delete m_source;
        m_source = nullptr;
        m_file->close();
        delete m_file;
        m_file = nullptr;
        return fail(msg);
    }
    connect(m_input, &QIODevice::readyRead, this, &VoiceRecorder::pull);
    connect(m_source, &QAudioSource::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState && m_recording
                && m_source->error() != QAudio::NoError) {
            abort(tr("The recording stopped: the input device reported an error."));
        }
    });

    m_recording = true;
    qInfo() << "VoiceRecorder: recording" << device.description() << format.sampleRate()
            << "Hz" << format.channelCount() << "ch ->" << wavPath;
    return true;
}

void VoiceRecorder::pull()
{
    if (!m_recording || !m_input || !m_file)
        return;
    const QByteArray chunk = m_input->readAll();
    if (chunk.isEmpty())
        return;

    if (m_file->write(chunk) != chunk.size()) {
        abort(tr("Writing the take failed: %1").arg(m_file->errorString()));
        return;
    }
    m_dataBytes += chunk.size();

    // Peak per channel over this chunk, for the meter.
    const int channels = qMax(1, m_format.channelCount());
    const auto *samples = reinterpret_cast<const qint16 *>(chunk.constData());
    const int count = int(chunk.size() / qint64(sizeof(qint16)));
    int peak[2] = {0, 0};
    for (int i = 0; i < count; ++i) {
        const int ch = (i % channels) & 1;
        peak[ch] = qMax(peak[ch], qAbs(int(samples[i])));
    }
    const float l = float(peak[0]) / 32768.0f;
    const float r = channels > 1 ? float(peak[1]) / 32768.0f : l;
    emit levels(qMin(1.0f, l), qMin(1.0f, r));

    const qint64 ms = recordedMs();
    if (ms / 100 != m_lastProgressMs / 100) {
        m_lastProgressMs = ms;
        emit progress(ms);
    }
}

qint64 VoiceRecorder::recordedMs() const
{
    const int bytesPerFrame = qMax(1, m_format.bytesPerFrame());
    const int rate = qMax(1, m_format.sampleRate());
    return (m_dataBytes / bytesPerFrame) * 1000 / rate;
}

void VoiceRecorder::stop()
{
    if (!m_recording)
        return;
    m_recording = false;

    if (m_source) {
        // One last read: QAudioSource keeps whatever arrived since the final
        // readyRead, and dropping it clips the presenter's last word.
        if (m_input && m_file) {
            const QByteArray tail = m_input->readAll();
            if (!tail.isEmpty()) {
                m_file->write(tail);
                m_dataBytes += tail.size();
            }
        }
        m_source->stop();
        m_source->deleteLater();
        m_source = nullptr;
        m_input = nullptr;
    }

    const qint64 duration = recordedMs();
    const QString path = m_path;
    if (m_file) {
        patchHeader(m_file, m_format, m_dataBytes);
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    qInfo() << "VoiceRecorder: take finished," << duration << "ms into" << path;
    emit finished(path, duration);
}

void VoiceRecorder::abort(const QString &message)
{
    if (!m_recording)
        return;
    m_recording = false;
    if (m_source) {
        m_source->stop();
        m_source->deleteLater();
        m_source = nullptr;
        m_input = nullptr;
    }
    if (m_file) {
        patchHeader(m_file, m_format, m_dataBytes);
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    qWarning() << "VoiceRecorder:" << message;
    emit failed(message);
}

void VoiceRecorder::writePlaceholderHeader(QFile *file)
{
    const QByteArray zeros(kHeaderBytes, '\0');
    file->write(zeros);
}

void VoiceRecorder::patchHeader(QFile *file, const QAudioFormat &format, qint64 dataBytes)
{
    const quint16 channels = quint16(qMax(1, format.channelCount()));
    const quint32 rate = quint32(qMax(1, format.sampleRate()));
    const quint16 bits = 16;
    const quint32 byteRate = rate * channels * bits / 8;
    const quint16 blockAlign = quint16(channels * bits / 8);

    file->seek(0);
    putTag(file, "RIFF");
    putU32(file, quint32(36 + dataBytes));
    putTag(file, "WAVE");
    putTag(file, "fmt ");
    putU32(file, 16);
    putU16(file, 1); // PCM
    putU16(file, channels);
    putU32(file, rate);
    putU32(file, byteRate);
    putU16(file, blockAlign);
    putU16(file, bits);
    putTag(file, "data");
    putU32(file, quint32(dataBytes));
    file->flush();
}

QString VoiceRecorder::convertToOpus(const QString &wavPath, QString *error)
{
    const auto keepWav = [&wavPath, error](const QString &why) {
        if (error)
            *error = why;
        return wavPath;
    };

    const QString ffmpeg = FxEngine::ffmpegExecutable();
    if (ffmpeg.isEmpty())
        return keepWav(tr("ffmpeg was not found, so the take stays as a WAV file."));
    if (!QFileInfo::exists(wavPath))
        return keepWav(tr("The take is missing."));

    QString opusPath = wavPath;
    opusPath.chop(4);
    opusPath += QStringLiteral(".opus");

    QProcess proc;
    proc.start(ffmpeg, {QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"),
                        QStringLiteral("error"), QStringLiteral("-y"),
                        QStringLiteral("-i"), wavPath,
                        QStringLiteral("-c:a"), QStringLiteral("libopus"),
                        QStringLiteral("-b:a"), QStringLiteral("96k"),
                        opusPath});
    if (!proc.waitForStarted(5000))
        return keepWav(tr("ffmpeg could not be started, so the take stays as a WAV file."));
    if (!proc.waitForFinished(120000)) {
        proc.kill();
        proc.waitForFinished(2000);
        QFile::remove(opusPath);
        return keepWav(tr("Converting the take to Opus took too long; it stays as a WAV file."));
    }
    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0
            || !QFileInfo::exists(opusPath) || QFileInfo(opusPath).size() == 0) {
        QFile::remove(opusPath);
        // Most often: this ffmpeg build has no libopus. Not worth an error
        // dialog — the WAV is a perfectly good voice track.
        return keepWav(tr("This ffmpeg cannot encode Opus, so the take stays as a "
                          "WAV file."));
    }

    QFile::remove(wavPath);
    if (error)
        error->clear();
    return opusPath;
}
