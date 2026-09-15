#ifndef PROGRAMRECORDER_H
#define PROGRAMRECORDER_H

#include <QAudioFormat>
#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <vector>

class QAudioSource;
class QIODevice;
class QProcess;
class QTimer;

/**
 * @brief Records a programme: an input device, what XFB puts on air, or both.
 *
 * This replaced QMediaRecorder for the "Record a new Program" flow in
 * player.cpp, for two reasons:
 *
 *  - QMediaRecorder can only record an input device, and on Linux that list
 *    never offers a PulseAudio/PipeWire "Monitor of …" source: Qt's PulseAudio
 *    backend drops monitors while enumerating. So the music never reached a
 *    recording unless the station routed audio around XFB. This takes it from
 *    inside instead — the same FxEngine PCM tap the built-in Icecast stream
 *    uses — which works the same everywhere.
 *  - QMediaRecorder encodes with whatever the platform has: AAC and ALAC on
 *    macOS, no MP3 or Ogg at all. ffmpeg writes MP3, Ogg Vorbis and Opus on
 *    every platform, and XFB already depends on it.
 *
 * Each on-air player feeds its own lane through feed(). An input device, when
 * one is asked for, is one more lane. A wall-clock mixer sums the lanes a
 * fixed latency behind real time, so:
 *
 *  - lanes that arrive in bursts (an engine filling its sink buffer) still
 *    line up with each other, because every burst after a quiet spell is
 *    placed by the time it arrived, not glued onto whatever came before;
 *  - silence is recorded as silence: nothing playing still advances the
 *    file, and a programme that pauses for a minute is a minute long;
 *  - the small drift between the wall clock and each device's own clock is
 *    absorbed a sample at a time instead of ending in a gap hours in.
 *
 * The mix is 48 kHz stereo, piped into the ffmpeg CLI and encoded straight
 * into the target file.
 */
class ProgramRecorder : public QObject
{
    Q_OBJECT

public:
    enum class Format {
        Mp3,       ///< libmp3lame — plays everywhere
        OggVorbis, ///< what the program recorder always wrote into XFB.ogg
        Opus,      ///< Opus in Ogg, .opus — the smallest file for the quality
    };

    struct Config {
        Format format = Format::OggVorbis;
        int bitrateKbps = 192;
        bool includeInput = true;      ///< mix in the input device below
        QString inputDeviceDescription; ///< matched by description; empty = default
    };

    explicit ProgramRecorder(QObject *parent = nullptr);
    ~ProgramRecorder() override;

    /** True when ffmpeg is present, which is all this needs. */
    static bool available();

    /** Stable settings code: "mp3", "ogg", "opus". */
    static QString formatCode(Format format);
    /** Unknown codes read as Ogg Vorbis, the format recordings always had. */
    static Format formatFromCode(const QString &code);
    /** File extension, without the dot. */
    static QString extensionFor(Format format);
    /** Bitrates offered in Options, in kbit/s. */
    static QList<int> bitratesFor(Format format);
    /** @a kbps if offered for @a format, otherwise the nearest offered one. */
    static int nearestBitrate(Format format, int kbps);

    /**
     * Start recording into @a path (overwritten). Feed on-air audio through
     * feed() for it to be in the file; with no feeds and no input the file is
     * silence.
     * @return false with @a error filled in; nothing is left running.
     */
    bool start(const QString &path, const Config &config, QString *error);
    /** Finish the file: the encoder is flushed and closed before this returns. */
    void stop();

    /** While paused the clock keeps running but nothing is written. */
    void setPaused(bool paused);
    bool isPaused() const { return m_paused; }
    bool isRecording() const { return m_recording; }

    /** The description of the input device actually opened, if any. */
    QString inputDeviceDescription() const { return m_inputDescription; }

    /**
     * One on-air player's post-DSP PCM (interleaved s16le), wired to
     * FxPlayer::pcmTap. @a key identifies the lane — any stable pointer.
     * @a volume is that player's output volume, so a fade or a fader move is
     * in the recording as it was on air.
     */
    void feed(const QObject *key, const QByteArray &pcm, int sampleRate,
              int channels, float volume);

signals:
    /** The recording stopped on its own; the file may be incomplete. */
    void failed(const QString &message);

private:
    static constexpr int kRate = 48000;
    static constexpr int kChannels = 2;
    /** How far behind real time the mixer runs; covers burst jitter. */
    static constexpr int kLatencyFrames = kRate / 2;
    /** A lane further ahead than this is losing a frame now and then. */
    static constexpr int kTooFullFrames = kLatencyFrames + kRate;
    /** Where a live lane below this starts gaining a frame now and then. */
    static constexpr int kTooEmptyFrames = kLatencyFrames / 2;
    /** One frame dropped or repeated per this many: 0.2 %, inaudible. */
    static constexpr int kCorrectionPeriod = 500;
    /** Unwritten encoder input before new audio is dropped (~10 min). */
    static constexpr qint64 kMaxPendingBytes = 64 * 1024 * 1024;

    struct Lane {
        std::vector<float> samples; ///< interleaved stereo, read from head
        size_t head = 0;
        qint64 lastArrivalMs = -1;
        int correctionCounter = 0;
        qint64 frames() const
        { return qint64(samples.size() - head) / kChannels; }
    };

    qint64 nowFrames() const;
    void appendToLane(Lane &lane, const float *interleaved, qint64 frames);
    void mixTick();
    /** Mix and write every frame of the timeline before @a due. */
    void mixUntil(qint64 due);
    void pullInput();
    bool openInput(const QString &description, QString *error);
    void closeInput();
    /** Empty with @a error filled in when this ffmpeg cannot write @a config. */
    QStringList encoderArguments(const QString &path, const Config &config,
                                 QString *error) const;
    void fail(const QString &message);

    QProcess *m_encoder = nullptr;
    QTimer *m_tick = nullptr;
    QElapsedTimer m_clock;
    qint64 m_framesMixed = 0;

    QHash<const QObject *, Lane> m_lanes;
    Lane m_inputLane;

    QAudioSource *m_source = nullptr;
    QIODevice *m_input = nullptr;
    QAudioFormat m_inputFormat;
    QString m_inputDescription;
    QByteArray m_inputRemainder;      ///< a partial input frame, kept for next time
    double m_resamplePos = 0.0;       ///< fractional read position, input frames
    std::vector<float> m_inputCarry;  ///< last input frame, for interpolation
    QString m_encoderErrors;          ///< ffmpeg's stderr, for the failure message

    std::vector<float> m_mix;
    QByteArray m_pcmOut;
    bool m_recording = false;
    bool m_paused = false;
    bool m_dropping = false;
};

#endif // PROGRAMRECORDER_H
