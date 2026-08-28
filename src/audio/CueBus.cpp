#include "CueBus.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "AudioDeviceRouter.h"
#include "FxPlayer.h"

namespace
{
/** Locate a helper binary, including the dirs a GUI app's PATH misses. */
QString findTool(const QString &name)
{
    QString path = QStandardPaths::findExecutable(name);
    if (!path.isEmpty())
        return path;
    static const QStringList extraDirs = {
        QStringLiteral("/usr/bin"), QStringLiteral("/bin"),
        QStringLiteral("/opt/homebrew/bin"), QStringLiteral("/usr/local/bin"),
        QStringLiteral("/opt/local/bin")
    };
    return QStandardPaths::findExecutable(name, extraDirs);
}
} // namespace

CueBus::CueBus(QObject *parent)
    : QObject(parent)
{
    // Both players are cue-locked before they are ever handed a source, so
    // there is no window in which either could reach the on-air output.
    m_player = new FxPlayer(this);
    m_player->setPreferEngineAlways(true); // only the engine can pick a device
    m_player->lockToCueDevice(QByteArray());

    m_speech = new FxPlayer(this);
    m_speech->setPreferEngineAlways(true);
    m_speech->lockToCueDevice(QByteArray());

    connect(m_player, &FxPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia && !m_label.isEmpty()) {
            m_label.clear();
            m_source.clear();
            emit cueStateChanged(false, QString());
        }
    });
    connect(m_player, &FxPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &msg) {
        m_label.clear();
        m_source.clear();
        emit cueStateChanged(false, QString());
        emit cueFailed(tr("The cue could not be played: %1").arg(msg));
    });
}

CueBus::~CueBus()
{
    if (m_ttsProc) {
        m_ttsProc->kill();
        m_ttsProc->waitForFinished(1000);
    }
    delete m_speechDir;
}

void CueBus::setMainDeviceId(const QByteArray &deviceId)
{
    if (m_mainDeviceId == deviceId)
        return;
    m_mainDeviceId = deviceId;
    // The on-air device may have become the cue device: stop at once.
    if (isCueing() && !isAvailable(nullptr))
        stopCue();
}

void CueBus::setCueDeviceId(const QByteArray &deviceId)
{
    if (m_cueDeviceId == deviceId)
        return;
    m_cueDeviceId = deviceId;
    if (isCueing())
        stopCue(); // never move a running cue onto a device the user just changed
    applyDevice();
}

void CueBus::applyDevice()
{
    // Re-point both locks. An empty id leaves the engines strictly bound to
    // "no device", which is silence — exactly what we want when the operator
    // has not configured a cue output.
    m_player->lockToCueDevice(m_cueDeviceId);
    m_speech->lockToCueDevice(m_cueDeviceId);
    m_player->setCueVolume(m_volume / 100.0f);
    m_speech->setCueVolume(m_volume / 100.0f);
    qInfo() << "CueBus: cue output is" << AudioDeviceRouter::describe(m_cueDeviceId);
}

void CueBus::setVolume(int percent)
{
    m_volume = qBound(0, percent, 100);
    m_player->setCueVolume(m_volume / 100.0f);
    m_speech->setCueVolume(m_volume / 100.0f);
}

void CueBus::setSpeechEnabled(bool on)
{
    m_speechEnabled = on;
}

bool CueBus::isAvailable(QString *why) const
{
    if (m_cueDeviceId.isEmpty()) {
        if (why)
            *why = tr("No cue output device is set. Choose one in Options, Cue and "
                      "outputs — cueing needs a second output, otherwise the "
                      "audition would go to air.");
        return false;
    }
    if (!AudioDeviceRouter::isPresent(m_cueDeviceId)) {
        if (why)
            *why = tr("The cue output device (%1) is not connected.")
                       .arg(AudioDeviceRouter::describe(m_cueDeviceId));
        return false;
    }
    // The on-air device is whatever the main players resolve to right now,
    // which is the system default when nothing was configured.
    const QAudioDevice onAir = AudioDeviceRouter::resolve(m_mainDeviceId, nullptr);
    if (!onAir.isNull() && onAir.id() == m_cueDeviceId) {
        if (why)
            *why = tr("The cue output and the on-air output are the same device "
                      "(%1). Cueing is disabled so an audition cannot go to air.")
                       .arg(onAir.description());
        return false;
    }
    return true;
}

bool CueBus::isCueing() const
{
    return m_player && m_player->playbackState() != QMediaPlayer::StoppedState;
}

bool CueBus::cue(const QUrl &file, const QString &label)
{
    QString why;
    if (!isAvailable(&why)) {
        emit cueFailed(why);
        return false;
    }
    if (file.isEmpty()) {
        emit cueFailed(tr("There is nothing to cue."));
        return false;
    }
    if (file.isLocalFile() && !QFileInfo::exists(file.toLocalFile())) {
        emit cueFailed(tr("The file to cue is missing: %1").arg(file.toLocalFile()));
        return false;
    }
    if (!FxPlayer::fxAvailable()) {
        // Without ffmpeg the engine cannot run, and the cue player has no
        // passthrough output to fall back on — by design. Say so plainly.
        emit cueFailed(tr("Cueing needs ffmpeg, which was not found on this system."));
        return false;
    }

    m_source = file;
    m_label = label.isEmpty() ? QFileInfo(file.toLocalFile()).fileName() : label;
    m_player->stop();
    m_player->setSource(file);
    m_player->setCueVolume(m_volume / 100.0f);
    m_player->play();
    qInfo() << "CueBus: cueing" << m_label << "on"
            << AudioDeviceRouter::describe(m_cueDeviceId);
    emit cueStateChanged(true, m_label);
    return true;
}

void CueBus::toggleCue(const QUrl &file, const QString &label)
{
    if (isCueing() && file == m_source) {
        stopCue();
        return;
    }
    cue(file, label);
}

void CueBus::stopCue()
{
    if (!m_player)
        return;
    const bool was = isCueing();
    m_player->stop();
    m_source.clear();
    m_label.clear();
    if (was)
        emit cueStateChanged(false, QString());
}

bool CueBus::ttsCommand(const QString &text, const QString &outFile,
                        QString *program, QStringList *arguments)
{
#if defined(Q_OS_MACOS)
    static const QString say = findTool(QStringLiteral("say"));
    if (!say.isEmpty()) {
        *program = say;
        *arguments = QStringList{QStringLiteral("-o"), outFile, text};
        return true;
    }
#elif defined(Q_OS_WIN)
    static const QString ps = findTool(QStringLiteral("powershell"));
    if (!ps.isEmpty()) {
        // SAPI is part of Windows; no extra dependency is introduced.
        QString escaped = text;
        escaped.replace(QLatin1Char('\''), QLatin1String("''"));
        QString file = outFile;
        file.replace(QLatin1Char('\''), QLatin1String("''"));
        *program = ps;
        *arguments = QStringList{
            QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
            QStringLiteral("Add-Type -AssemblyName System.Speech; "
                           "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
                           "$s.SetOutputToWaveFile('%1'); $s.Speak('%2'); $s.Dispose()")
                .arg(file, escaped)
        };
        return true;
    }
#else
    for (const QString &name : {QStringLiteral("espeak-ng"), QStringLiteral("espeak")}) {
        const QString exe = findTool(name);
        if (!exe.isEmpty()) {
            *program = exe;
            *arguments = QStringList{QStringLiteral("-w"), outFile, text};
            return true;
        }
    }
    const QString pico = findTool(QStringLiteral("pico2wave"));
    if (!pico.isEmpty()) {
        *program = pico;
        *arguments = QStringList{QStringLiteral("-w"), outFile, text};
        return true;
    }
#endif
    Q_UNUSED(text)
    Q_UNUSED(outFile)
    return false;
}

bool CueBus::speechAvailable()
{
    QString program;
    QStringList arguments;
    return ttsCommand(QStringLiteral("x"), QStringLiteral("/tmp/xfb-cue-probe.wav"),
                      &program, &arguments);
}

void CueBus::speak(const QString &text)
{
    if (!m_speechEnabled || text.trimmed().isEmpty())
        return;
    if (!isAvailable(nullptr))
        return; // no private ear: the message still reached the screen reader
    if (!speechAvailable() || !FxPlayer::fxAvailable())
        return;

    // A short queue, not an unbounded one: spoken cues are time-critical and
    // a backlog of stale ones is worse than dropping them.
    if (m_speechQueue.size() >= 3)
        m_speechQueue.dequeue();
    m_speechQueue.enqueue(text.trimmed());
    renderNextSpeech();
}

void CueBus::renderNextSpeech()
{
    if (m_ttsProc || m_speechQueue.isEmpty())
        return;

    if (!m_speechDir) {
        m_speechDir = new QTemporaryDir();
        if (!m_speechDir->isValid()) {
            qWarning() << "CueBus: no writable place to render spoken cues";
            m_speechQueue.clear();
            return;
        }
    }

    const QString text = m_speechQueue.dequeue();
#if defined(Q_OS_MACOS)
    const QString suffix = QStringLiteral(".aiff"); // what `say -o` writes
#else
    const QString suffix = QStringLiteral(".wav");
#endif
    const QString outFile = m_speechDir->filePath(
        QStringLiteral("cue-%1%2").arg(++m_speechSerial).arg(suffix));

    QString program;
    QStringList arguments;
    if (!ttsCommand(text, outFile, &program, &arguments))
        return;

    m_ttsProc = new QProcess(this);
    connect(m_ttsProc, &QProcess::finished, this,
            [this, outFile](int exitCode, QProcess::ExitStatus status) {
        m_ttsProc->deleteLater();
        m_ttsProc = nullptr;
        if (exitCode == 0 && status == QProcess::NormalExit && QFileInfo::exists(outFile))
            onSpeechRendered(outFile);
        else
            qWarning() << "CueBus: text-to-speech render failed for a spoken cue";
        renderNextSpeech();
    });
    m_ttsProc->start(program, arguments);
}

void CueBus::onSpeechRendered(const QString &wavPath)
{
    // The speech player is cue-locked exactly like the audition player, so
    // this cannot come out of the on-air output either.
    m_speech->stop();
    m_speech->setSource(QUrl::fromLocalFile(wavPath));
    m_speech->setCueVolume(m_volume / 100.0f);
    m_speech->play();
}
