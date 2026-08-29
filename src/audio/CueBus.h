#ifndef CUEBUS_H
#define CUEBUS_H

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QUrl>

class FxPlayer;
class QProcess;
class QTemporaryDir;

/**
 * @brief The cue bus: pre-fade listen on a second output device.
 *
 * A radio operator needs to hear the next track, a jingle or a pad in
 * headphones while something else is on air, and a blind operator needs a
 * private ear for spoken cues that must never reach the transmitter. Both
 * are the same thing: audio that plays on a *different* output from the one
 * feeding air.
 *
 * ## Why cueing is refused rather than approximated
 *
 * The bus is only "available" when a cue output device has been chosen in
 * Options AND it is a different device from the one the on-air players use.
 * With one sound card there is no such thing as pre-fade listen: cueing onto
 * the on-air output would put the audition to air, which is worse than not
 * having the feature. So the bus says why it cannot cue instead of quietly
 * doing the dangerous thing.
 *
 * ## How a leak is made impossible rather than unlikely
 *
 * Both internal players are put in cue-lock (FxPlayer::lockToCueDevice):
 * their passthrough QMediaPlayer has no QAudioOutput at all, and their FX
 * engine opens its QAudioSink strictly on the cue device — if that device
 * vanishes the cue goes silent, it never falls back to the default. The bus
 * additionally re-checks the device pairing at every cue() call, so a
 * settings change that makes the two devices the same stops cueing at once.
 *
 * The on-air players are never touched by anything here: the bus owns its
 * own FxPlayer instances and has no reference to the main player's output.
 */
class CueBus : public QObject
{
    Q_OBJECT

public:
    explicit CueBus(QObject *parent = nullptr);
    ~CueBus() override;

    /** The device the on-air players render to (empty = system default). */
    void setMainDeviceId(const QByteArray &deviceId);
    QByteArray mainDeviceId() const { return m_mainDeviceId; }

    /** The private ear. Empty means "no cue device configured". */
    void setCueDeviceId(const QByteArray &deviceId);
    QByteArray cueDeviceId() const { return m_cueDeviceId; }

    /** Cue monitor level, 0..100, independent of every on-air fader. */
    void setVolume(int percent);
    int volume() const { return m_volume; }

    /** Route XFB's own spoken feedback to the cue ear as well as the screen. */
    void setSpeechEnabled(bool on);
    bool speechEnabled() const { return m_speechEnabled; }

    /**
     * Whether a cue can be started right now.
     * @param why filled with a translated explanation when it cannot.
     */
    bool isAvailable(QString *why = nullptr) const;

    bool isCueing() const;
    /** How far into the cued file playback has reached; -1 when idle. Polled
     *  by the voice-track dialog, which has to switch monitor sources at an
     *  exact moment on the timeline. */
    qint64 cuePosition() const;
    QString currentLabel() const { return m_label; }
    QUrl currentSource() const { return m_source; }

    /** True when this machine has a text-to-speech tool for spoken cues. */
    static bool speechAvailable();

public slots:
    /** Start auditioning @a file privately. False (with cueFailed) on refusal. */
    bool cue(const QUrl &file, const QString &label);
    /**
     * Cue @a file starting @a startMs into it, rather than from the top.
     *
     * Voice tracking needs this: what the presenter has to hear while they
     * record is the last twenty seconds of the outgoing song, not its
     * beginning. Same refusal rules as cue() — this is still pre-fade
     * listen, and it still will not run onto the on-air output.
     */
    bool cueFrom(const QUrl &file, const QString &label, qint64 startMs);
    /** Cue @a file, or stop if that same file is already being cued. */
    void toggleCue(const QUrl &file, const QString &label);
    void stopCue();
    /**
     * Speak @a text into the cue ear only. Does nothing when spoken cues are
     * off, no cue device is configured, or no text-to-speech tool exists —
     * it is a private extra, never the only channel a message arrives on.
     */
    void speak(const QString &text);

signals:
    /** Cue started/stopped; @a label is what is being cued (for the UI). */
    void cueStateChanged(bool cueing, const QString &label);
    /** A cue was refused or died; @a reason is user-facing and translated. */
    void cueFailed(const QString &reason);

private:
    void applyDevice();
    void renderNextSpeech();
    void onSpeechRendered(const QString &wavPath);
    /** The command that renders text to an audio file on this platform. */
    static bool ttsCommand(const QString &text, const QString &outFile,
                           QString *program, QStringList *arguments);

    FxPlayer *m_player = nullptr;  // auditioned tracks
    FxPlayer *m_speech = nullptr;  // spoken cues (its own player: a countdown
                                   // must not interrupt the track being cued)
    QByteArray m_mainDeviceId;
    QByteArray m_cueDeviceId;
    int m_volume = 80;
    bool m_speechEnabled = false;

    QUrl m_source;
    QString m_label;

    QTemporaryDir *m_speechDir = nullptr;
    QProcess *m_ttsProc = nullptr;
    QQueue<QString> m_speechQueue;
    int m_speechSerial = 0;
};

#endif // CUEBUS_H
