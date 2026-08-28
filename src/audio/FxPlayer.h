#ifndef FXPLAYER_H
#define FXPLAYER_H

#include <QByteArray>
#include <QMediaPlayer>
#include <QObject>
#include <QThread>
#include <QUrl>

#include "FxParams.h"

class QAudioOutput;
class FxEngine;

/**
 * @brief Drop-in player used by the XFB main window.
 *
 * Mirrors the subset of the QMediaPlayer API that player.cpp uses, and
 * routes playback through one of two internal paths:
 *
 *  - Passthrough: a plain QMediaPlayer + QAudioOutput. Used whenever no
 *    FX are enabled (the battle-tested default) or when the FX engine is
 *    unavailable (no ffmpeg, non-local source, engine failure).
 *  - FX engine: ffmpeg decode -> 432 Hz retuner -> 10-band EQ ->
 *    compressor -> QAudioSink, running on a dedicated worker thread.
 *
 * State-change signals triggered by direct calls (play/pause/stop) are
 * emitted synchronously, matching QMediaPlayer's behaviour that the
 * m_manualAdvancing guards in player.cpp rely on. Natural end-of-media is
 * reported asynchronously, also matching QMediaPlayer.
 */
class FxPlayer : public QObject
{
    Q_OBJECT

public:
    explicit FxPlayer(QObject *parent = nullptr);
    ~FxPlayer() override;

    // QMediaPlayer-compatible API subset
    void setAudioOutput(QAudioOutput *output);
    QAudioOutput *audioOutput() const { return m_output; }

    void setSource(const QUrl &source);
    QUrl source() const { return m_source; }

    /**
     * Gapless: preload the upcoming local track so the next setSource()
     * starts instantly. The FX engine spawns the decoder ahead of time and
     * hands off without breaking the audio stream; plain playback preloads
     * the media on a standby QMediaPlayer and swaps it in. A preload that
     * doesn't match the eventual setSource() is discarded automatically.
     */
    void prepareNext(const QUrl &url);
    /** True when prepareNext() is armed for exactly this source. */
    bool hasPreparedNext(const QUrl &url) const;
    /**
     * FX engine only: the next preloaded handoff crossfades — the outgoing
     * track keeps playing inside the engine mix, fading over fadeMs,
     * instead of being cut when the new source is adopted.
     */
    void setNextCrossfade(qint64 fadeMs);

    /**
     * Rebuild the FX engine's audio sink on the current default device.
     * The stall watchdog calls this: a wedged output device otherwise
     * survives recovery and stalls every following track too.
     */
    void resetAudioSink();

    // --- Output routing ---
    /**
     * Render this player to a specific audio output.
     *
     * @param deviceId a stored QAudioDevice::id(), or empty for the system
     *                 default (what every player did before this existed).
     *
     * Applies to both internal paths — the passthrough QAudioOutput and the
     * FX engine's QAudioSink — so a track already playing moves output
     * without stopping. If the device is gone, playback continues on the
     * system default and the fallback is logged: on air, silence is worse
     * than the wrong speaker.
     */
    void setOutputDeviceId(const QByteArray &deviceId);
    QByteArray outputDeviceId() const { return m_deviceId; }

    /**
     * Turn this player into a cue (pre-fade listen) player, permanently
     * bound to one output device.
     *
     * This is a one-way switch and it is the mechanism that makes a cue leak
     * impossible rather than merely unlikely:
     *
     *  - the passthrough QMediaPlayer's QAudioOutput is detached and never
     *    re-attached, so that path is physically mute — it has no output
     *    object to render through, whatever the routing settings say;
     *  - the FX engine's sink is opened *strictly* on @a deviceId, so if
     *    that device disappears the cue goes silent instead of falling back
     *    to the default (which is the on-air output);
     *  - the engine-failure fallback that would normally hand a track to the
     *    passthrough player is disabled.
     *
     * A locked player therefore has exactly one route to a speaker, and it
     * is the cue device. Cue mode itself is one-way; calling this again
     * only re-points the lock at a different device (Options changed).
     */
    void lockToCueDevice(const QByteArray &deviceId);
    bool isCueLocked() const { return m_deviceLocked; }

    /**
     * Monitor level for a cue-locked player, 0..1. A cue player has no
     * QAudioOutput to carry the volume, so it goes straight to the engine's
     * sink — where it is applied after the tap, exactly as it is for the
     * on-air player.
     */
    void setCueVolume(float linearVolume);

    void play();
    void pause();
    void stop();

    void setPosition(qint64 positionMs);
    qint64 position() const;
    qint64 duration() const;

    QMediaPlayer::PlaybackState playbackState() const;

    // FX control
    void setFxParams(const FxParams &params);
    FxParams fxParams() const { return m_params; }
    bool fxEngineActive() const { return m_mode == Mode::Fx; }
    static bool fxAvailable();

    /**
     * Route local files through the FX engine even when no FX are enabled.
     * Used by the LP decks so scratching and the DJ effects are always
     * available (falls back to plain playback when ffmpeg is missing).
     */
    void setPreferEngineAlways(bool on) { m_preferEngine = on; }

    /**
     * Arm the broadcast tap: post-DSP master PCM is re-emitted through
     * pcmTap() for the stream encoder.
     *
     * Only the FX engine can produce the tap, so while it is armed local
     * files are routed through the engine even with no FX enabled — the
     * same trick the LP decks use. Disarming restores the ordinary
     * passthrough/FX choice, mid-track and without a gap in the audio.
     */
    void setPcmTapEnabled(bool enabled);
    bool pcmTapEnabled() const { return m_pcmTap; }
    // --- EBU R128 loudness normalisation ---
    /**
     * Turn loudness normalisation on for this player. While on, local
     * files are routed through the FX engine: a *positive* normalisation
     * gain is impossible any other way, because QAudioOutput's volume is
     * clamped to 0..1 and a quiet transfer needs a boost, not a cut.
     */
    void setLoudnessActive(bool on);
    bool loudnessActive() const { return m_loudnessActive; }
    /**
     * Per-track gain in dB (target LUFS minus the measured integrated
     * loudness, already capped against the true-peak ceiling). It lands in
     * the engine's gain stage and multiplies with the sink volume the
     * fader and the playlist volume envelope drive — the envelope is a
     * LINEAR 0..1 multiplier and is never mixed up with these decibels.
     */
    void setLoudnessGainDb(double gainDb, bool immediate = true);
    double loudnessGainDb() const { return m_loudnessGainDb; }
    /** Master true-peak limiter (safety net against a bad measurement). */
    void setLimiter(bool enabled, double ceilingDbTp);

    // DJ performance controls — active only while the engine drives playback
    void setDjFx(double filterAmount, double echoAmount);
    void scratchBegin();
    void scratchMove(double targetRate);
    void scratchEnd();
    void djBrake();
    void djBackspin();

signals:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    /** Output peaks 0..1 for the level meter (FX-engine playback only). */
    void levels(float left, float right);
    /**
     * Post-DSP master PCM (interleaved s16le), emitted only while the tap
     * is armed and the engine is the active path.
     */
    void pcmTap(const QByteArray &pcm, int sampleRate, int channels);
    void sourceChanged(const QUrl &media);
    void playbackStateChanged(QMediaPlayer::PlaybackState newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void errorOccurred(QMediaPlayer::Error error, const QString &errorString);

private:
    enum class Mode { Passthrough, Fx };

    template <typename F> void engineCall(F &&f);
    bool wantFxFor(const QUrl &url) const;
    void connectPassthrough(QMediaPlayer *p);
    void discardPrepared();
    /** Re-evaluate passthrough vs engine for the current source, and switch. */
    void applyModeForCurrentSource();
    void switchToFx(QMediaPlayer::PlaybackState resumeState, qint64 resumePos);
    void switchToPassthrough(QMediaPlayer::PlaybackState resumeState, qint64 resumePos);

    QMediaPlayer *m_qt = nullptr;
    QMediaPlayer *m_qtStandby = nullptr; // preloads the next track (gapless swap)
    QUrl m_preparedUrl;                  // armed prepareNext() target
    bool m_preparedInEngine = false;     // preload lives in the FX engine
    QAudioOutput *m_output = nullptr;

    QThread m_engineThread;
    FxEngine *m_engine = nullptr;

    Mode m_mode = Mode::Passthrough;
    bool m_switching = false;        // suppress signal forwarding during internal mode switches
    bool m_fxFailedForTrack = false; // engine gave up on the current track
    bool m_preferEngine = false;     // LP decks: engine even without FX params
    bool m_pcmTap = false;           // streaming: engine even without FX params
    bool m_loudnessActive = false;   // loudness normalisation needs the engine
    QByteArray m_deviceId;           // output device (empty = system default)
    bool m_deviceLocked = false;     // cue player: this device or silence
    double m_loudnessGainDb = 0.0;
    bool m_limiterOn = false;
    double m_limiterCeilingDbTp = -1.0;

    QUrl m_source;
    FxParams m_params;
    QMediaPlayer::PlaybackState m_fxState = QMediaPlayer::StoppedState;
    qint64 m_fxPos = 0;
    qint64 m_fxDuration = 0;
};

#endif // FXPLAYER_H
