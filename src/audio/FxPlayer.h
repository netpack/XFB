#ifndef FXPLAYER_H
#define FXPLAYER_H

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

    QUrl m_source;
    FxParams m_params;
    QMediaPlayer::PlaybackState m_fxState = QMediaPlayer::StoppedState;
    qint64 m_fxPos = 0;
    qint64 m_fxDuration = 0;
};

#endif // FXPLAYER_H
