#ifndef FXENGINE_H
#define FXENGINE_H

#include <QObject>
#include <QString>
#include <QAudioFormat>
#include <vector>

#include "FxDsp.h"
#include "FxParams.h"

class QProcess;
class QAudioSink;
class QIODevice;
class QTimer;

/**
 * @brief Worker-thread playback engine with a real DSP chain.
 *
 * Decodes any audio file through the ffmpeg CLI (already a runtime
 * dependency of XFB's conversion features) into 48 kHz stereo float PCM
 * (optionally retuned to A=432 Hz with tempo preserved, via ffmpeg's
 * asetrate/aresample/atempo filters), runs it through the in-process FX
 * chain (10-band EQ -> compressor -> safety clamp) and renders it with
 * QAudioSink.
 *
 * The engine lives in its own thread (owned by FxPlayer), so playback
 * keeps running even when the GUI thread is busy. All public slots must
 * be invoked via queued connections / QMetaObject::invokeMethod.
 */
class FxEngine : public QObject
{
    Q_OBJECT

public:
    explicit FxEngine(QObject *parent = nullptr);
    ~FxEngine() override;

    /** Locate the ffmpeg executable (PATH plus common install dirs). */
    static QString ffmpegExecutable();
    /** Locate the ffprobe executable (PATH plus common install dirs). */
    static QString ffprobeExecutable();
    /** True when the FX engine can be used on this system. */
    static bool available();

public slots:
    /** Accepts a local file path or an http(s) stream URL (live mode). */
    void setSource(const QString &pathOrUrl);
    void play();
    void pause();
    void stop();
    void seek(qint64 positionMs);
    void setVolume(float linearVolume);
    void setParams(const FxParams &params);
    void shutdown();

    // --- DJ performance controls (LP decks) ---
    /** Live one-knob filter (-1..1) and echo (0..1); not persisted. */
    void setDjFx(double filterAmount, double echoAmount);
    /** Grab the platter: freezes the deck and enters scratch mode. */
    void scratchBegin();
    /** Target platter rate while scratching (1.0 = normal, negative = reverse). */
    void scratchMove(double targetRate);
    /** Release the platter: resumes normal playback from the scratch position. */
    void scratchEnd();
    /** Vinyl brake: the platter spins down to a stop (~1 s). */
    void djBrake();
    /** Backspin: the platter whips backwards, then stops. */
    void djBackspin();

signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    /** Natural end of the current track (media fully played). */
    void playbackFinished();
    /** Fatal engine error for the current track. */
    void engineError(const QString &message);

private slots:
    void pump();

private:
    void startProcessAt(qint64 positionMs);
    void stopProcess();
    bool ensureSink();
    void teardownSink();
    void resetDspState();
    /** One ffprobe pass: fills m_durationMs and m_sourceIs432. */
    void probeLocalSource(const QString &filePath);
    qint64 inputFramesConsumed() const;
    qint64 currentPositionMs() const;
    int fillChunk(float *out, int maxFrames);
    void readProcessOutput();
    void maybeFinish();
    void failTrack(const QString &message);
    void applyFxChain(float *chunk, int frames);
    void writeChunkToSink(const float *chunk, int frames);
    bool enterScratchMode();
    void pumpScratch();
    void renderScratch(float *out, int frames);
    void stopFromScratch();
    qint64 scratchPosMs() const;

    static constexpr int kSampleRate = 48000;
    static constexpr int kChannels = 2;
    static constexpr int kChunkFrames = 2048;

    // Transport
    enum class State { Stopped, Playing, Paused };
    State m_state = State::Stopped;
    QString m_path;
    bool m_isLive = false;        // http(s) stream: no duration, no seek, no -re
    bool m_sourceIs432 = false;   // file already retuned: never retune it again
    qint64 m_durationMs = 0;
    qint64 m_baseMs = 0;          // input-timeline offset of the running decode
    qint64 m_framesTaken = 0;     // input frames consumed since m_baseMs (non-retune path)
    qint64 m_pausedPosMs = 0;
    float m_volume = 1.0f;
    bool m_producedAudio = false;

    // Decode process
    QProcess *m_proc = nullptr;
    QByteArray m_partialFrame;    // leftover bytes (< one PCM frame) between reads

    // Output
    QAudioSink *m_sink = nullptr;
    QIODevice *m_io = nullptr;
    bool m_sinkIsFloat = true;

    // DSP (the 432 Hz retune itself runs inside the ffmpeg filter chain so
    // the tempo can be preserved; EQ and compressor run in-process)
    FxParams m_params;
    fxdsp::Equalizer m_eq;
    fxdsp::Compressor m_comp;
    fxdsp::DjFilter m_djFilter;
    fxdsp::Echo m_echo;
    bool m_retuneOn = false;
    std::vector<float> m_fifo;    // interleaved float input (non-retune path)
    std::vector<float> m_chunk;
    std::vector<qint16> m_chunk16;

    // Scratch mode: a rolling history of played audio lets the read head
    // move backwards; while scratching the decoder is frozen and playback
    // is rendered from the snapshot at a mouse-driven variable rate.
    static constexpr int kHistorySeconds = 8;
    std::vector<float> m_history;      // recently consumed input (interleaved)
    bool m_scratchActive = false;
    int m_scratchMode = 0;             // 0=manual, 1=brake, 2=backspin
    std::vector<float> m_scratchBuf;   // history+future snapshot
    qint64 m_scratchBufStartFrame = 0; // absolute input frame of m_scratchBuf[0]
    double m_scratchPos = 0.0;         // absolute input frame (fractional)
    double m_scratchVel = 1.0;         // rate: input frames per output frame
    double m_scratchTargetVel = 0.0;

    QTimer *m_pumpTimer = nullptr;
    int m_positionEmitDivider = 0;
};

#endif // FXENGINE_H
