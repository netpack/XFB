#ifndef VOICERECORDER_H
#define VOICERECORDER_H

#include <QAudioFormat>
#include <QObject>
#include <QString>
#include <QStringList>

class QAudioSource;
class QFile;
class QIODevice;

/**
 * @brief Records one voice-track take to a WAV file, with live levels.
 *
 * XFB already records with QMediaRecorder (the "Record a new Program" flow in
 * player.cpp), and this is deliberately NOT that. A voice track needs three
 * things QMediaRecorder does not give:
 *
 *  - a level while recording. QMediaRecorder has no sample tap, and opening a
 *    second QAudioSource on the same device just to meter it is how you get a
 *    device-busy failure on the take itself.
 *  - an exact duration the instant the presenter stops, because the take's
 *    length is what positions it against the join and what the ducking is
 *    mapped onto. Probing the file afterwards is a round trip through ffmpeg
 *    for a number we already know to the sample.
 *  - a file WaveformStore can decode without surprises. Linear PCM in a RIFF
 *    container is the one format nothing argues about.
 *
 * So this pulls samples from a QAudioSource, writes them straight into a WAV
 * whose header is patched on stop, and emits peak levels on the way past.
 *
 * The input device is chosen by DESCRIPTION, the same way player.cpp's
 * program recorder resolves `recDevice`, so both honour the operator's
 * existing choice and both fall back to the system default when the device
 * they were told about is not plugged in.
 */
class VoiceRecorder : public QObject
{
    Q_OBJECT

public:
    explicit VoiceRecorder(QObject *parent = nullptr);
    ~VoiceRecorder() override;

    /** Human-readable input device names, for a combo box. */
    static QStringList inputDeviceNames();
    /** The name the system would pick on its own, or empty if there is none. */
    static QString defaultInputDeviceName();

    /** Where takes are kept: AppDataLocation/voicetracks, created on demand.
     *  Empty when the folder could not be made. */
    static QString takeFolder();
    /** A fresh, unused take path inside takeFolder(), stamped with the time. */
    static QString newTakePath(const QString &hint = QString());

    /**
     * Start recording to @a wavPath from the input device with this
     * description (empty = system default).
     * @return false with @a error filled in — the caller must show it: a
     *         voice track that silently records nothing is the worst outcome.
     */
    bool start(const QString &deviceDescription, const QString &wavPath, QString *error);

    /** Close the file and patch its header. Emits finished() on success. */
    void stop();

    bool isRecording() const { return m_recording; }
    qint64 recordedMs() const;
    QString filePath() const { return m_path; }

    /**
     * Re-encode a finished take to Opus with the FX engine's ffmpeg, and
     * delete the WAV when it works. Returns the path that should be used —
     * the .opus file on success, the untouched WAV on any failure, because a
     * take that exists as a big file beats a take that does not exist.
     */
    static QString convertToOpus(const QString &wavPath, QString *error = nullptr);

signals:
    /** Linear peak levels 0..1, for LevelMeter::setLevels(). */
    void levels(float left, float right);
    /** Length recorded so far, roughly every 100 ms. */
    void progress(qint64 recordedMs);
    void finished(const QString &wavPath, qint64 durationMs);
    /** The device died or the file could not be written; recording has stopped. */
    void failed(const QString &message);

private:
    void pull();
    void abort(const QString &message);
    static void writePlaceholderHeader(QFile *file);
    static void patchHeader(QFile *file, const QAudioFormat &format, qint64 dataBytes);

    QAudioSource *m_source = nullptr;
    QIODevice *m_input = nullptr; // owned by m_source
    QFile *m_file = nullptr;
    QAudioFormat m_format;
    QString m_path;
    qint64 m_dataBytes = 0;
    qint64 m_lastProgressMs = -1;
    bool m_recording = false;
};

#endif // VOICERECORDER_H
