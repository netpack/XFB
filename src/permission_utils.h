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

// Function declaration (implementation depends on platform)
MicrophonePermissionStatus checkMicrophonePermission();

// Helper function to show standard warning messages
void showMicrophonePermissionWarning(MicrophonePermissionStatus status, QWidget *parent = nullptr);


#endif // PERMISSION_UTILS_H
