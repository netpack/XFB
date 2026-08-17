LOCAL_PATH := $(call my-dir)

# ndk-build rather than CMake on purpose: the NDK carries its own make and
# toolchain, so this builds offline with nothing installed beyond the NDK
# itself. The SDK here has no cmake package, and the system CMake is far newer
# than the Android Gradle plugin will accept.
include $(CLEAR_VARS)

LOCAL_MODULE := xfbfx

# The DSP is compiled straight out of the XFB tree rather than copied, so the
# phone and the desk cannot drift apart. FxDsp.cpp is plain C++; the only Qt in
# its include chain is the settings helpers in FxParams.h, which FXPARAMS_NO_QT
# leaves out.
XFB_AUDIO_DIR := $(LOCAL_PATH)/../../../../../src/audio

LOCAL_SRC_FILES := \
    FxBridge.cpp \
    $(XFB_AUDIO_DIR)/FxDsp.cpp

LOCAL_C_INCLUDES := $(XFB_AUDIO_DIR)
LOCAL_CPPFLAGS := -std=c++17 -DFXPARAMS_NO_QT -Wall -Wextra
# -ffast-math is deliberately absent: the compressor's envelope follower relies
# on ordinary IEEE denormal behaviour, and the biquads on predictable rounding.
LOCAL_CPPFLAGS += -O2

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
