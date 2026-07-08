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

/** Hard safety clamp to [-1, 1] applied after the FX chain. */
void clampBuffer(float *interleaved, int frames);

} // namespace fxdsp

#endif // FXDSP_H
