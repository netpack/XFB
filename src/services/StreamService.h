#ifndef STREAMSERVICE_H
#define STREAMSERVICE_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QVector>

#include "IcecastSource.h"
#include "StreamEncoder.h"

class QTimer;

/**
 * @brief XFB's own way onto the air: one PCM tap, many Icecast mounts.
 *
 * The station's existing route to its listeners is an external Icecast plus
 * an external butt reading a sound card. That works, but butt hears whatever
 * the machine plays — a notification, a preview in another program — and it
 * has no idea what track is on, so the stream carries no titles. This service
 * replaces both ends of that for operators who want it, without removing it:
 * the audio comes from inside the player, already mixed and processed, and
 * the now-playing title is pushed to the server as the track changes.
 *
 * One tap feeds every mount, so a 128 kbit/s MP3 for the website and a
 * 64 kbit/s Opus for phones cost one decode and one DSP pass between them —
 * each mount only adds its own ffmpeg encoder.
 *
 * Silence is deliberate output, not the absence of it: when playback stops,
 * the service keeps feeding digital silence to the encoders so the mount
 * stays up. A source that simply stops sending is dropped by the server, and
 * every listener with it — which is not what "the operator paused for a
 * moment" should mean.
 */
class StreamService : public QObject
{
    Q_OBJECT

public:
    /** One configured mount: where to send, and what to send. */
    struct Mount {
        QString name;              ///< operator-facing label ("Website 128k")
        bool enabled = true;
        IcecastSource::Config server;
        StreamEncoder::Config encoder;
    };

    explicit StreamService(QObject *parent = nullptr);
    ~StreamService() override;

    QVector<Mount> mounts() const { return m_mounts; }
    /** Replaces the configuration; a live stream is rebuilt around it. */
    void setMounts(const QVector<Mount> &mounts);

    /** True from start() until stop(), whatever the mounts are doing. */
    bool isActive() const { return m_active; }
    /** How many enabled mounts currently have a live source connection. */
    int connectedCount() const;
    /** Per-mount connection state, index-aligned with mounts(). */
    QVector<IcecastSource::State> mountStates() const;
    /** Per-mount last failure reason, index-aligned with mounts(). */
    QVector<QString> mountErrors() const;

    /** The last title pushed to the servers. */
    QString nowPlaying() const { return m_nowPlaying; }

    /** Where the Stream/ settings live (the shared xfb.conf). */
    static QString configPath();
    static QVector<Mount> loadMounts();
    static void saveMounts(const QVector<Mount> &mounts);
    static bool autoStartEnabled();
    static void setAutoStartEnabled(bool on);
    /** A sensible first mount for an operator opening the dialog cold. */
    static Mount defaultMount();

public slots:
    void start();
    void stop();
    /** Wired to FxPlayer::pcmTap. Interleaved s16le master audio. */
    void feedPcm(const QByteArray &pcm, int sampleRate, int channels);
    /** Called when the on-air track changes; pushed to every mount. */
    void setNowPlaying(const QString &artist, const QString &title);

signals:
    void activeChanged(bool active);
    /** Any mount connected, dropped or changed state. */
    void statusChanged();
    void logMessage(const QString &message);
    /** The title actually sent to the servers, for the UI and the log. */
    void nowPlayingChanged(const QString &song);

private:
    /** A configured mount, running. */
    struct Feed {
        StreamEncoder *encoder = nullptr;
        IcecastSource *source = nullptr;
    };

    void buildFeeds();
    void tearDownFeeds();
    void feedSilence();

    /** Silence is only injected once the gap is longer than this. */
    static constexpr qint64 kSilenceThresholdMs = 300;
    /** Never inject more than this in one go, whatever the clock says. */
    static constexpr qint64 kMaxSilenceBurstMs = 1000;

    QVector<Mount> m_mounts;
    QVector<Feed> m_feeds;
    bool m_active = false;
    QString m_nowPlaying;

    // Keep-alive bookkeeping: how much audio real time has passed against
    // how much the tap actually delivered.
    QTimer *m_silenceTimer = nullptr;
    QElapsedTimer m_clock;
    qint64 m_bytesFed = 0;
    int m_sampleRate = 48000;
    int m_channels = 2;
};

#endif // STREAMSERVICE_H
