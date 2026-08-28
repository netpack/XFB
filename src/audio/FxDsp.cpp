#include "FxDsp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace fxdsp
{

namespace
{
// Local constant: M_PI is not portable (MSVC needs _USE_MATH_DEFINES)
constexpr double kPi = 3.14159265358979323846;

inline double dbToLin(double db) { return std::pow(10.0, db / 20.0); }
inline double linToDb(double lin) { return 20.0 * std::log10(std::max(lin, 1e-9)); }
}

// ---------------------------------------------------------------- BiquadPeak

void BiquadPeak::setup(double sampleRate, double freqHz, double q, double gainDb)
{
    if (gainDb == 0.0 || freqHz >= sampleRate / 2.0) {
        m_b0 = 1.0; m_b1 = 0.0; m_b2 = 0.0; m_a1 = 0.0; m_a2 = 0.0;
        m_identity = true;
        return;
    }

    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * freqHz / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw0 = std::cos(w0);

    const double b0 = 1.0 + alpha * A;
    const double b1 = -2.0 * cosw0;
    const double b2 = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A;
    const double a1 = -2.0 * cosw0;
    const double a2 = 1.0 - alpha / A;

    m_b0 = b0 / a0;
    m_b1 = b1 / a0;
    m_b2 = b2 / a0;
    m_a1 = a1 / a0;
    m_a2 = a2 / a0;
    m_identity = false;
}

void BiquadPeak::reset()
{
    m_x1l = m_x2l = m_y1l = m_y2l = 0.0;
    m_x1r = m_x2r = m_y1r = m_y2r = 0.0;
}

// ----------------------------------------------------------------- Equalizer

void Equalizer::configure(double sampleRate, const FxParams &p)
{
    m_active = p.eqEnabled && !p.eqIsFlat();
    m_preampLin = dbToLin(p.preampDb);
    for (int i = 0; i < FxParams::kBands; ++i) {
        // Q ~1.1 gives smooth octave-band overlap for a graphic EQ
        m_bands[i].setup(sampleRate, FxParams::kBandFreqHz[i], 1.1,
                         p.eqEnabled ? p.eqGainDb[i] : 0.0);
    }
}

void Equalizer::reset()
{
    for (auto &b : m_bands)
        b.reset();
}

void Equalizer::process(float *interleaved, int frames)
{
    if (!m_active)
        return;

    for (int i = 0; i < frames; ++i) {
        float l = interleaved[2 * i] * static_cast<float>(m_preampLin);
        float r = interleaved[2 * i + 1] * static_cast<float>(m_preampLin);
        for (auto &b : m_bands) {
            if (!b.isIdentity())
                b.processFrame(l, r);
        }
        interleaved[2 * i] = l;
        interleaved[2 * i + 1] = r;
    }
}

// ---------------------------------------------------------------- Compressor

void Compressor::configure(double sampleRate, const FxParams &p)
{
    m_active = p.compEnabled;
    m_thresholdDb = p.compThresholdDb;
    m_ratio = std::max(1.0, p.compRatio);
    m_makeupLin = dbToLin(p.compMakeupDb);

    const double attackSec = std::max(0.0001, p.compAttackMs / 1000.0);
    const double releaseSec = std::max(0.001, p.compReleaseMs / 1000.0);
    m_attackCoef = std::exp(-1.0 / (attackSec * sampleRate));
    m_releaseCoef = std::exp(-1.0 / (releaseSec * sampleRate));
}

void Compressor::reset()
{
    m_envDb = -120.0;
    m_gainReductionDb = 0.0;
}

void Compressor::process(float *interleaved, int frames)
{
    if (!m_active)
        return;

    for (int i = 0; i < frames; ++i) {
        float &l = interleaved[2 * i];
        float &r = interleaved[2 * i + 1];

        // Peak detector on the louder channel
        const double level = std::max(std::fabs(static_cast<double>(l)),
                                      std::fabs(static_cast<double>(r)));
        const double levelDb = linToDb(level);

        // Smooth the detected level: fast attack, slow release
        if (levelDb > m_envDb)
            m_envDb = m_attackCoef * m_envDb + (1.0 - m_attackCoef) * levelDb;
        else
            m_envDb = m_releaseCoef * m_envDb + (1.0 - m_releaseCoef) * levelDb;

        // Soft-knee gain computer
        const double overDb = m_envDb - m_thresholdDb;
        double reductionDb = 0.0;
        if (overDb >= m_kneeDb / 2.0) {
            reductionDb = overDb * (1.0 - 1.0 / m_ratio);
        } else if (overDb > -m_kneeDb / 2.0) {
            const double t = overDb + m_kneeDb / 2.0;
            reductionDb = (1.0 - 1.0 / m_ratio) * t * t / (2.0 * m_kneeDb);
        }

        const double gain = dbToLin(-reductionDb) * m_makeupLin;
        l = static_cast<float>(l * gain);
        r = static_cast<float>(r * gain);
    }
}

// ------------------------------------------------------------------ DjFilter

void DjFilter::setup(double sampleRate)
{
    m_sampleRate = sampleRate;
    computeCoefficients();
}

void DjFilter::setAmount(double amount)
{
    m_amount = std::clamp(amount, -1.0, 1.0);
    computeCoefficients();
}

void DjFilter::computeCoefficients()
{
    if (std::fabs(m_amount) < 0.02) {
        m_active = false;
        return;
    }

    // Logarithmic sweeps: LP from 20 kHz down to ~160 Hz, HP from 20 Hz up
    // to ~7 kHz — matches the feel of a mixer's filter knob.
    const bool lowPass = m_amount < 0.0;
    const double t = std::fabs(m_amount);
    const double cutoff = lowPass ? 20000.0 * std::pow(10.0, -2.1 * t)
                                  : 20.0 * std::pow(10.0, 2.55 * t);

    const double q = 0.85;
    const double w0 = 2.0 * kPi * cutoff / m_sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosw0 = std::cos(w0);
    const double a0 = 1.0 + alpha;

    if (lowPass) {
        const double b1 = 1.0 - cosw0;
        m_b0 = (b1 / 2.0) / a0;
        m_b1 = b1 / a0;
        m_b2 = (b1 / 2.0) / a0;
    } else {
        const double b1 = 1.0 + cosw0;
        m_b0 = (b1 / 2.0) / a0;
        m_b1 = -b1 / a0;
        m_b2 = (b1 / 2.0) / a0;
    }
    m_a1 = (-2.0 * cosw0) / a0;
    m_a2 = (1.0 - alpha) / a0;
    m_active = true;
}

void DjFilter::reset()
{
    m_x1l = m_x2l = m_y1l = m_y2l = 0.0;
    m_x1r = m_x2r = m_y1r = m_y2r = 0.0;
}

void DjFilter::process(float *interleaved, int frames)
{
    if (!m_active)
        return;

    for (int i = 0; i < frames; ++i) {
        const double inl = interleaved[2 * i];
        const double outl = m_b0 * inl + m_b1 * m_x1l + m_b2 * m_x2l - m_a1 * m_y1l - m_a2 * m_y2l;
        m_x2l = m_x1l; m_x1l = inl;
        m_y2l = m_y1l; m_y1l = outl;
        interleaved[2 * i] = static_cast<float>(outl);

        const double inr = interleaved[2 * i + 1];
        const double outr = m_b0 * inr + m_b1 * m_x1r + m_b2 * m_x2r - m_a1 * m_y1r - m_a2 * m_y2r;
        m_x2r = m_x1r; m_x1r = inr;
        m_y2r = m_y1r; m_y1r = outr;
        interleaved[2 * i + 1] = static_cast<float>(outr);
    }
}

// ---------------------------------------------------------------------- Echo

void Echo::setup(double sampleRate)
{
    const int delayFrames = static_cast<int>(sampleRate * 0.38); // 380 ms
    m_buf.assign(static_cast<size_t>(delayFrames) * 2, 0.0f);
    m_pos = 0;
}

void Echo::setAmount(double amount)
{
    m_amount = std::clamp(amount, 0.0, 1.0);
}

void Echo::reset()
{
    std::fill(m_buf.begin(), m_buf.end(), 0.0f);
    m_pos = 0;
}

void Echo::process(float *interleaved, int frames)
{
    if (m_buf.empty())
        return;
    // Keep feeding the delay line even at zero mix so the echo is "primed"
    // the moment the knob comes up.
    const float mix = static_cast<float>(m_amount);
    const float feedback = static_cast<float>(std::min(0.75, 0.30 + 0.45 * m_amount));

    for (int i = 0; i < frames; ++i) {
        for (int c = 0; c < 2; ++c) {
            const float dry = interleaved[2 * i + c];
            const float wet = m_buf[m_pos];
            m_buf[m_pos] = dry + wet * feedback;
            interleaved[2 * i + c] = dry + wet * mix;
            ++m_pos;
        }
        if (m_pos >= m_buf.size())
            m_pos = 0;
    }
}

// ----------------------------------------------------------------- GainStage

void GainStage::setup(double sampleRate)
{
    // ~50 ms to settle: inaudible as a ramp, fast enough that a track
    // change is over long before the first bar.
    const double tau = std::max(1.0, 0.050 * sampleRate);
    m_coef = 1.0 - std::exp(-1.0 / tau);
    m_currentLin = m_targetLin;
}

void GainStage::setGainDb(double db)
{
    // A sane window: nothing in a radio library legitimately needs more
    // than 24 dB of correction, and a wild value here would be a bug
    // reaching the transmitter.
    m_targetDb = std::clamp(db, -24.0, 24.0);
    m_targetLin = dbToLin(m_targetDb);
}

void GainStage::snap()
{
    m_currentLin = m_targetLin;
}

void GainStage::reset()
{
    m_currentLin = m_targetLin;
}

void GainStage::process(float *interleaved, int frames)
{
    if (m_targetLin == 1.0 && m_currentLin == 1.0)
        return;

    for (int i = 0; i < frames; ++i) {
        m_currentLin += (m_targetLin - m_currentLin) * m_coef;
        const float g = static_cast<float>(m_currentLin);
        interleaved[2 * i] *= g;
        interleaved[2 * i + 1] *= g;
    }

    // Park exactly on the target once the ramp has effectively arrived, so
    // the unity fast path can be taken again.
    if (std::fabs(m_targetLin - m_currentLin) < 1e-6)
        m_currentLin = m_targetLin;
}

// ----------------------------------------------------------- TruePeakLimiter

void TruePeakLimiter::setup(double sampleRate)
{
    // 4x polyphase interpolation kernel: a windowed sinc, split into
    // kOversample phases of kTapsPerPhase taps. Phase 0 is (near) the
    // identity, the other three estimate the inter-sample values that a
    // sample-peak meter cannot see.
    const int taps = kOversample * kTapsPerPhase;
    const double centre = (taps - 1) / 2.0;
    std::vector<double> proto(static_cast<size_t>(taps), 0.0);
    for (int n = 0; n < taps; ++n) {
        const double x = (n - centre) / static_cast<double>(kOversample);
        const double sinc = (std::fabs(x) < 1e-9)
            ? 1.0
            : std::sin(kPi * x) / (kPi * x);
        // Blackman window
        const double w = 0.42
            - 0.5 * std::cos(2.0 * kPi * n / (taps - 1))
            + 0.08 * std::cos(4.0 * kPi * n / (taps - 1));
        proto[static_cast<size_t>(n)] = sinc * w;
    }

    for (int p = 0; p < kOversample; ++p) {
        double sum = 0.0;
        for (int k = 0; k < kTapsPerPhase; ++k) {
            const int idx = k * kOversample + p;
            m_phase[p][k] = proto[static_cast<size_t>(idx)];
            sum += m_phase[p][k];
        }
        // Unity DC gain per phase, so the estimate is an amplitude, not a
        // filter response.
        if (std::fabs(sum) > 1e-12) {
            for (int k = 0; k < kTapsPerPhase; ++k)
                m_phase[p][k] /= sum;
        }
    }

    // 200 ms release: long enough not to pump on dense material.
    const double relTau = std::max(1.0, 0.200 * sampleRate);
    m_releaseCoef = 1.0 - std::exp(-1.0 / relTau);

    reset();
}

void TruePeakLimiter::setEnabled(bool on)
{
    if (m_enabled == on)
        return;
    m_enabled = on;
    reset();
}

void TruePeakLimiter::setCeilingDb(double dbtp)
{
    m_ceilingDb = std::clamp(dbtp, -12.0, 0.0);
    m_ceilingLin = dbToLin(m_ceilingDb - kEstimatorMarginDb);
}

void TruePeakLimiter::reset()
{
    std::fill(std::begin(m_histL), std::end(m_histL), 0.0);
    std::fill(std::begin(m_histR), std::end(m_histR), 0.0);
    m_histPos = 0;
    std::fill(std::begin(m_delay), std::end(m_delay), 0.0f);
    std::fill(std::begin(m_required), std::end(m_required), 1.0);
    m_ringPos = 0;
    m_gain = 1.0;
}

double TruePeakLimiter::gainReductionDb() const
{
    return -linToDb(m_gain);
}

double TruePeakLimiter::estimateTruePeak(float l, float r)
{
    m_histL[m_histPos] = l;
    m_histR[m_histPos] = r;
    m_histPos = (m_histPos + 1) % kTapsPerPhase;

    double peak = 0.0;
    for (int p = 0; p < kOversample; ++p) {
        double accL = 0.0;
        double accR = 0.0;
        for (int k = 0; k < kTapsPerPhase; ++k) {
            // m_histPos now points at the oldest sample in the ring
            const int idx = (m_histPos + k) % kTapsPerPhase;
            const double c = m_phase[p][kTapsPerPhase - 1 - k];
            accL += c * m_histL[idx];
            accR += c * m_histR[idx];
        }
        peak = std::max(peak, std::max(std::fabs(accL), std::fabs(accR)));
    }
    // The kernel can only ever under-estimate between its own phases, so
    // never report less than the sample peak itself.
    return std::max(peak, std::max(std::fabs(static_cast<double>(l)),
                                   std::fabs(static_cast<double>(r))));
}

void TruePeakLimiter::process(float *interleaved, int frames)
{
    if (!m_enabled)
        return;

    for (int i = 0; i < frames; ++i) {
        const float inL = interleaved[2 * i];
        const float inR = interleaved[2 * i + 1];

        const double peak = estimateTruePeak(inL, inR);
        const double required = (peak > m_ceilingLin && peak > 0.0)
            ? m_ceilingLin / peak
            : 1.0;

        // Oldest slot in both rings is the frame about to be emitted.
        const float outL = m_delay[m_ringPos * 2];
        const float outR = m_delay[m_ringPos * 2 + 1];
        m_delay[m_ringPos * 2] = inL;
        m_delay[m_ringPos * 2 + 1] = inR;
        m_required[m_ringPos] = required;
        m_ringPos = (m_ringPos + 1) % kLookaheadFrames;

        // Running minimum over the whole look-ahead window. Because the
        // frame being emitted is the oldest member of that window, its own
        // requirement is included — that is what makes the ceiling a hard
        // guarantee instead of a target.
        double target = 1.0;
        for (int k = 0; k < kLookaheadFrames; ++k)
            target = std::min(target, m_required[k]);

        if (target < m_gain)
            m_gain = target;             // duck immediately (still ahead of the peak)
        else
            m_gain += (target - m_gain) * m_releaseCoef;

        interleaved[2 * i] = static_cast<float>(outL * m_gain);
        interleaved[2 * i + 1] = static_cast<float>(outR * m_gain);
    }
}

// -------------------------------------------------------------------- Common

void clampBuffer(float *interleaved, int frames)
{
    for (int i = 0; i < frames * 2; ++i)
        interleaved[i] = std::clamp(interleaved[i], -1.0f, 1.0f);
}

double dbToLinear(double db) { return dbToLin(db); }
double linearToDb(double lin) { return linToDb(lin); }

} // namespace fxdsp
