#ifndef STREAMENCODER_H
#define STREAMENCODER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;
class QTimer;

/**
 * @brief Encodes the player's master PCM for one Icecast mount by piping it
 *        through the ffmpeg CLI.
 *
 * XFB already depends on the ffmpeg binary for decoding and conversion, so
 * the encoder side costs nothing extra: raw interleaved 16-bit PCM goes in
 * on stdin (`-f s16le -ar 48000 -ac 2 -i -`) and a muxed MP3 or Ogg/Opus
 * stream comes back out on stdout, which is exactly the byte stream an
 * Icecast source connection wants for its body.
 *
 * Two failure modes matter on a machine that is on air and are handled here
 * rather than left to the caller:
 *
 *  - **Backpressure.** ffmpeg encodes faster than real time, so its stdin
 *    should never build up; when it does (the process is wedged, or the
 *    machine is thrashing) the buffered audio is stale by definition. Past
 *    a high-water mark incoming PCM is dropped rather than queued, so the
 *    stream stays current and memory does not grow without bound.
 *  - **ffmpeg dying.** The process is respawned with an exponential backoff
 *    so a permanently broken configuration (a codec this build of ffmpeg
 *    does not have) does not turn into a fork bomb.
 *
 * The encoder does not know about Icecast; it just produces bytes.
 */
class StreamEncoder : public QObject
{
    Q_OBJECT

public:
    enum class Codec {
        Mp3,   ///< libmp3lame in an MP3 stream — understood by everything
        Opus,  ///< libopus in an Ogg stream — much better at low bitrates
    };

    struct Config {
        Codec codec = Codec::Mp3;
        int bitrateKbps = 128;
        int sampleRate = 48000;
        int channels = 2;
    };

    explicit StreamEncoder(QObject *parent = nullptr);
    ~StreamEncoder() override;

    void setConfig(const Config &config);
    Config config() const { return m_config; }

    /** True once ffmpeg is up; false while it is being restarted. */
    bool isRunning() const;

    /** The Content-Type an Icecast mount has to be told about. */
    static QString contentTypeFor(Codec codec);
    /** Stable identifier used in settings and in the UI ("mp3", "opus"). */
    static QString codecName(Codec codec);
    static Codec codecFromName(const QString &name);
    /** Bitrates offered in the UI, in kbit/s. */
    static QList<int> bitratesFor(Codec codec);

public slots:
    void start();
    void stop();
    /** Interleaved signed 16-bit little-endian PCM at the configured rate. */
    void writePcm(const QByteArray &pcm);

signals:
    /** A chunk of encoded, muxed stream data. */
    void encoded(const QByteArray &data);
    /** Human-readable progress and trouble, for the dialog's log. */
    void logMessage(const QString &message);
    void runningChanged(bool running);

private:
    void spawn();
    void killProcess();
    void scheduleRestart(const QString &reason);
    QStringList ffmpegArguments() const;

    /**
     * How much unwritten PCM may sit in ffmpeg's stdin buffer before new
     * audio is dropped. 48 kHz stereo s16 is 192 kB/s, so this is a bit
     * over two seconds — far longer than a healthy encoder ever needs.
     */
    static constexpr qint64 kMaxPendingBytes = 384 * 1024;
    static constexpr int kFirstRestartDelayMs = 1000;
    static constexpr int kMaxRestartDelayMs = 30000;

    Config m_config;
    QProcess *m_proc = nullptr;
    QTimer *m_restartTimer = nullptr;
    bool m_wantRunning = false;
    bool m_dropping = false;        // in a drop episode: log the edges only
    qint64 m_droppedBytes = 0;
    int m_restartDelayMs = kFirstRestartDelayMs;
};

#endif // STREAMENCODER_H
