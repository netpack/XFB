#ifndef FXDSP_H
#define FXDSP_H

#include <cstddef>
#include <vector>
#include "FxParams.h"

/**
 * Lightweight DSP building blocks used by the FxEngine.
 *
 * All processors work on interleaved stereo float frames at a fixed
 * sample rate. They are real-time safe (no allocation in process paths)
 * and are only ever touched from the engine's worker thread.
 */
namespace fxdsp
{

/** RBJ cookbook peaking equalizer biquad (stereo). */
class BiquadPeak
{
public:
    void setup(double sampleRate, double freqHz, double q, double gainDb);
    void reset();
    bool isIdentity() const { return m_identity; }

    inline void processFrame(float &l, float &r)
    {
        // Direct form I, per channel
        const double outL = m_b0 * l + m_b1 * m_x1l + m_b2 * m_x2l - m_a1 * m_y1l - m_a2 * m_y2l;
        m_x2l = m_x1l; m_x1l = l;
        m_y2l = m_y1l; m_y1l = outL;
        l = static_cast<float>(outL);

        const double outR = m_b0 * r + m_b1 * m_x1r + m_b2 * m_x2r - m_a1 * m_y1r - m_a2 * m_y2r;
        m_x2r = m_x1r; m_x1r = r;
        m_y2r = m_y1r; m_y1r = outR;
        r = static_cast<float>(outR);
    }

private:
    double m_b0 = 1.0, m_b1 = 0.0, m_b2 = 0.0, m_a1 = 0.0, m_a2 = 0.0;
    double m_x1l = 0, m_x2l = 0, m_y1l = 0, m_y2l = 0;
    double m_x1r = 0, m_x2r = 0, m_y1r = 0, m_y2r = 0;
    bool m_identity = true;
};

/** 10-band graphic equalizer with preamp. */
class Equalizer
{
public:
    void configure(double sampleRate, const FxParams &p);
    void reset();
    void process(float *interleaved, int frames);

private:
    BiquadPeak m_bands[FxParams::kBands];
    double m_preampLin = 1.0;
    bool m_active = false;
};

/** Feed-forward soft-knee compressor with program-adaptive envelope. */
class Compressor
{
public:
    void configure(double sampleRate, const FxParams &p);
    void reset();
    void process(float *interleaved, int frames);

private:
    bool m_active = false;
    double m_thresholdDb = -18.0;
    double m_ratio = 3.0;
    double m_kneeDb = 6.0;
    double m_makeupLin = 1.0;
    double m_attackCoef = 0.0;
    double m_releaseCoef = 0.0;
    double m_envDb = -120.0;      // level detector state (dBFS)
    double m_gainReductionDb = 0.0;
};

/**
 * One-knob DJ filter: the classic performance filter found on DJ mixers.
 * Negative amounts sweep a low-pass filter down (muffles the track),
 * positive amounts sweep a high-pass filter up (thins the track),
 * zero is bypass.
 */
class DjFilter
{
public:
    void setup(double sampleRate);
    void setAmount(double amount); // -1..1, 0 = off
    void reset();
    void process(float *interleaved, int frames);

private:
    void computeCoefficients();

    double m_sampleRate = 48000.0;
    double m_amount = 0.0;
    bool m_active = false;
    double m_b0 = 1, m_b1 = 0, m_b2 = 0, m_a1 = 0, m_a2 = 0;
    double m_x1l = 0, m_x2l = 0, m_y1l = 0, m_y2l = 0;
    double m_x1r = 0, m_x2r = 0, m_y1r = 0, m_y2r = 0;
};

/**
 * One-knob feedback echo (fixed 380 ms delay). The knob drives both the
 * wet mix and the feedback, so a single gesture goes from dry to a long
 * echo-out tail — the way DJs actually use it.
 */
class Echo
{
public:
    void setup(double sampleRate);
    void setAmount(double amount); // 0..1
    void reset();
    void process(float *interleaved, int frames);

private:
    std::vector<float> m_buf; // interleaved stereo delay line
    size_t m_pos = 0;
    double m_amount = 0.0;
};

/**
 * Smoothly ramped broadband gain, in decibels.
 *
 * This is the stage EBU R128 loudness normalisation uses: the per-track
 * offset (target LUFS minus the track's measured integrated loudness,
 * capped so the true peak stays under the ceiling) is set here once per
 * track and the stage ramps to it over ~50 ms so a live change never
 * clicks.
 *
 * IMPORTANT — this is *decibels*. XFB's playlist volume envelope is a
 * separate, LINEAR 0..1 multiplier that lands on the audio sink's volume;
 * the two multiply, they are never added, and neither one moves the
 * operator's fader.
 */
class GainStage
{
public:
    void setup(double sampleRate);
    /** Target gain in dB (0 = unity). Ramped, not applied instantly. */
    void setGainDb(double db);
    double gainDb() const { return m_targetDb; }
    /** Jump straight to the target — used at a track change. */
    void snap();
    void reset();
    bool isUnity() const { return m_targetDb == 0.0 && m_currentLin == m_targetLin; }
    void process(float *interleaved, int frames);

private:
    double m_targetDb = 0.0;
    double m_targetLin = 1.0;
    double m_currentLin = 1.0;
    double m_coef = 0.0; // one-pole smoothing coefficient
};

/**
 * Look-ahead, true-peak aware limiter for the master output.
 *
 * The point of this stage is that a mis-measured (or unmeasured) track
 * must never be able to clip the transmitter. Inter-sample peaks are
 * estimated with a 4x polyphase oversampler, the gain needed to hold the
 * ceiling is computed per input frame, and the signal is delayed by the
 * look-ahead window so the gain is always already down by the time the
 * offending sample arrives. The running minimum over the look-ahead
 * window is what is applied, which makes the ceiling a guarantee rather
 * than an average; release is slow enough not to pump.
 */
class TruePeakLimiter
{
public:
    void setup(double sampleRate);
    void setEnabled(bool on);
    bool enabled() const { return m_enabled; }
    /** Ceiling in dBTP (e.g. -1.0). */
    void setCeilingDb(double dbtp);
    double ceilingDb() const { return m_ceilingDb; }
    void reset();
    void process(float *interleaved, int frames);
    /** Current gain reduction in dB (>= 0), for metering / diagnostics. */
    double gainReductionDb() const;

    /** Oversampling factor and taps per phase of the true-peak estimator. */
    static constexpr int kOversample = 4;
    static constexpr int kTapsPerPhase = 12;
    /** Look-ahead, in frames at the configured sample rate. */
    static constexpr int kLookaheadFrames = 64;
    /**
     * Safety margin, in dB, between the configured ceiling and what the
     * limiter actually aims for.
     *
     * Any 4x true-peak estimator (this one, and ffmpeg's, and the one in
     * every hardware limiter) reads a little under the real continuous
     * peak, and the limiter's own gain modulation adds a little back. On a
     * full-scale 7.9 kHz tone the two together left the output measuring
     * 0.2 dB above a ceiling this stage believed it was holding — measured
     * against ffmpeg's own meter, not assumed. Aiming this far under makes
     * the stated ceiling true as an independent meter sees it.
     */
    static constexpr double kEstimatorMarginDb = 0.35;

private:
    double estimateTruePeak(float l, float r);

    bool m_enabled = false;
    double m_ceilingDb = -1.0;
    double m_ceilingLin = 0.891250938;
    double m_releaseCoef = 0.0;

    // 4x polyphase interpolation kernel (windowed sinc), built in setup()
    double m_phase[kOversample][kTapsPerPhase] = {};

    // Per-channel input history feeding the oversampler
    double m_histL[kTapsPerPhase] = {};
    double m_histR[kTapsPerPhase] = {};
    int m_histPos = 0;

    // Look-ahead rings: the delayed signal and the gain each frame demands
    float m_delay[kLookaheadFrames * 2] = {};
    double m_required[kLookaheadFrames] = {};
    int m_ringPos = 0;

    double m_gain = 1.0;
};

/** Hard safety clamp to [-1, 1] applied after the FX chain. */
void clampBuffer(float *interleaved, int frames);

/** dB <-> linear helpers shared by the engine and the loudness code. */
double dbToLinear(double db);
double linearToDb(double lin);

} // namespace fxdsp

#endif // FXDSP_H
