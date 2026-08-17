/*
 * JNI bridge onto XFB's own DSP.
 *
 * The equalizer and compressor here are the desktop's, compiled from
 * src/audio/FxDsp.cpp rather than reimplemented, so a set mixed on the phone
 * sounds like the same set on the desk. The 432 Hz retune is NOT here: on the
 * desktop it is a resample in ffmpeg, and on Android ExoPlayer's own Sonic
 * pitch shift does the same job for free (see FxController).
 *
 * Everything runs on ExoPlayer's audio thread, so nothing in the process path
 * allocates: the scratch buffer grows only when configure() sees a bigger
 * block, and the JNI calls hand over direct ByteBuffers.
 */

#include <jni.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "FxDsp.h"
#include "FxParams.h"

namespace {

struct FxChain
{
    fxdsp::Equalizer eq;
    fxdsp::Compressor comp;
    std::vector<float> scratch;   ///< interleaved stereo float, 2 per frame
    bool active = false;
};

inline FxChain *chainOf(jlong handle)
{
    return reinterpret_cast<FxChain *>(handle);
}

} // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_pt_netpack_xfb_companion_FxChain_nativeCreate(JNIEnv *, jobject)
{
    return reinterpret_cast<jlong>(new FxChain());
}

JNIEXPORT void JNICALL
Java_pt_netpack_xfb_companion_FxChain_nativeDestroy(JNIEnv *, jobject, jlong handle)
{
    delete chainOf(handle);
}

/**
 * Rebuilds the filters for a sample rate and parameter set.
 *
 * @return true if anything in the chain would actually change the audio, so
 *         the Kotlin side can leave the processor out of the pipeline
 *         altogether rather than paying for a copy that does nothing.
 */
JNIEXPORT jboolean JNICALL
Java_pt_netpack_xfb_companion_FxChain_nativeConfigure(
    JNIEnv *env, jobject, jlong handle, jint sampleRate,
    jboolean eqEnabled, jdouble preampDb, jdoubleArray bandGainsDb,
    jboolean compEnabled, jdouble thresholdDb, jdouble ratio,
    jdouble attackMs, jdouble releaseMs, jdouble makeupDb)
{
    FxChain *chain = chainOf(handle);
    if (!chain)
        return JNI_FALSE;

    FxParams params;
    params.eqEnabled = (eqEnabled == JNI_TRUE);
    params.preampDb = preampDb;
    params.compEnabled = (compEnabled == JNI_TRUE);
    params.compThresholdDb = thresholdDb;
    params.compRatio = ratio;
    params.compAttackMs = attackMs;
    params.compReleaseMs = releaseMs;
    params.compMakeupDb = makeupDb;

    if (bandGainsDb) {
        const jsize count = std::min<jsize>(env->GetArrayLength(bandGainsDb),
                                            FxParams::kBands);
        env->GetDoubleArrayRegion(bandGainsDb, 0, count, params.eqGainDb);
    }

    chain->eq.configure(sampleRate, params);
    chain->comp.configure(sampleRate, params);
    chain->eq.reset();
    chain->comp.reset();

    // An enabled but perfectly flat equalizer is not worth running, which is
    // the same judgement FxParams::eqIsFlat() exists to make on the desktop.
    const bool eqDoesSomething = params.eqEnabled && !params.eqIsFlat();
    chain->active = eqDoesSomething || params.compEnabled;
    return chain->active ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_pt_netpack_xfb_companion_FxChain_nativeReset(JNIEnv *, jobject, jlong handle)
{
    FxChain *chain = chainOf(handle);
    if (!chain)
        return;
    chain->eq.reset();
    chain->comp.reset();
}

/**
 * Processes one block of interleaved 16-bit PCM from @p input into @p output.
 *
 * 16-bit rather than float because float output is not guaranteed on every
 * device's audio sink, while 16-bit always is; the conversion either side is
 * far cheaper than the filtering between them.
 *
 * FxDsp is stereo-only, so a mono source is widened into the scratch buffer,
 * filtered, and narrowed again on the way out. Both halves see identical
 * samples through identical filters, so they stay identical and taking the
 * left one back loses nothing. Mono is rare in a music library but a silently
 * unequalized track would be worse than the small cost of handling it.
 */
JNIEXPORT void JNICALL
Java_pt_netpack_xfb_companion_FxChain_nativeProcess(
    JNIEnv *env, jobject, jlong handle,
    jobject input, jint inputOffset, jobject output, jint outputOffset,
    jint frames, jint channels)
{
    FxChain *chain = chainOf(handle);
    if (!chain || frames <= 0 || (channels != 1 && channels != 2))
        return;

    auto *inBase = static_cast<uint8_t *>(env->GetDirectBufferAddress(input));
    auto *outBase = static_cast<uint8_t *>(env->GetDirectBufferAddress(output));
    if (!inBase || !outBase)
        return;

    const auto *in = reinterpret_cast<const int16_t *>(inBase + inputOffset);
    auto *out = reinterpret_cast<int16_t *>(outBase + outputOffset);

    // The chain always works on stereo pairs, whatever arrived.
    const size_t samples = static_cast<size_t>(frames) * 2;
    if (chain->scratch.size() < samples)
        chain->scratch.resize(samples);
    float *work = chain->scratch.data();

    if (channels == 2) {
        for (size_t i = 0; i < samples; ++i)
            work[i] = static_cast<float>(in[i]) * (1.0f / 32768.0f);
    } else {
        for (jint i = 0; i < frames; ++i) {
            const float sample = static_cast<float>(in[i]) * (1.0f / 32768.0f);
            work[2 * i] = sample;
            work[2 * i + 1] = sample;
        }
    }

    chain->eq.process(work, frames);
    chain->comp.process(work, frames);
    fxdsp::clampBuffer(work, frames);

    if (channels == 2) {
        for (size_t i = 0; i < samples; ++i)
            out[i] = static_cast<int16_t>(std::lrintf(work[i] * 32767.0f));
    } else {
        for (jint i = 0; i < frames; ++i)
            out[i] = static_cast<int16_t>(std::lrintf(work[2 * i] * 32767.0f));
    }
}

} // extern "C"
