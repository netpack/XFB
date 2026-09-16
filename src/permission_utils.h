#ifndef PERMISSION_UTILS_H
#define PERMISSION_UTILS_H

#include <QtGlobal> // For Q_OS_MAC
#include<QWidget>

// Enum to represent permission status in C++
enum class MicrophonePermissionStatus {
    Granted,
    Denied,
    Undetermined, // User hasn't been asked yet
    Error,        // Could not determine status
    NotApplicable // Not on macOS
};

// Function declaration (implementation depends on platform).
//
// permission_utils.mm is Objective-C++ and src/CMakeLists.txt compiles it only
// `if(APPLE)`, so off macOS there is no definition at all — and a caller that
// is itself portable then fails at the LINK step, long after everything has
// compiled cleanly:
//
//   PlayerRemoteControl.cpp:606: undefined reference to
//   `checkMicrophonePermission()'
//
// which is how the remote control broke the Linux build. NotApplicable already
// means "not on macOS" in the enum above and every caller treats it as "carry
// on", so the honest non-Apple answer is to say so here rather than to make
// each new call site remember a platform guard.
#ifdef Q_OS_MAC
MicrophonePermissionStatus checkMicrophonePermission();
#else
inline MicrophonePermissionStatus checkMicrophonePermission()
{
    return MicrophonePermissionStatus::NotApplicable;
}
#endif

// Helper function to show standard warning messages.
// Deliberately NOT given a non-Apple stub: there is nothing to warn about when
// the answer is always NotApplicable, and a silent no-op dialog would hide a
// real mistake. Its only callers are the commented-out block in
// externaldownloader.cpp, so nothing off macOS references it today; anything
// that starts to will get a link error here and a decision to make.
void showMicrophonePermissionWarning(MicrophonePermissionStatus status, QWidget *parent = nullptr);


#endif // PERMISSION_UTILS_H
