#include "permission_utils.h"
#include <QDebug>
#include <QMessageBox> // Include for the helper function

#ifdef Q_OS_MAC
#import <AVFoundation/AVFoundation.h> // Keep this, it includes AVCaptureDevice

// Helper function implemented in Objective-C++ for macOS
MicrophonePermissionStatus getMicrophonePermissionStatus_macOS() {
    // Check authorization status for the microphone media type
    // This method is available on macOS 10.14+
    if (@available(macOS 10.14, *)) {
        AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];

        switch (status) {
            case AVAuthorizationStatusAuthorized:
                qDebug() << "[Permission Check macOS] Microphone permission: Authorized";
                return MicrophonePermissionStatus::Granted;

            case AVAuthorizationStatusDenied:
                qWarning() << "[Permission Check macOS] Microphone permission: Denied";
                return MicrophonePermissionStatus::Denied;

            case AVAuthorizationStatusRestricted:
                // Restricted, e.g., by parental controls. Treat as Denied for practical purposes.
                qWarning() << "[Permission Check macOS] Microphone permission: Restricted";
                return MicrophonePermissionStatus::Denied; // Treat restricted as denied

            case AVAuthorizationStatusNotDetermined:
                qInfo() << "[Permission Check macOS] Microphone permission: Not Determined (will prompt user)";
                // We should *request* access here to trigger the prompt *before* Qt Multimedia tries.
                // Note: Requesting access is asynchronous. The status might not be Granted
                // immediately after this call returns. The first Qt Multimedia call will likely
                // block until the user responds if we don't handle the callback here.
                // For simplicity in this example, we'll just return Undetermined and let
                // the OS/Qt handle the prompt on first actual use.
                 [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                     // This block executes AFTER the user responds to the prompt.
                     // We can't easily return the result from here directly to the initial check.
                     if (granted) {
                         qInfo() << "[Permission Check macOS] User explicitly granted microphone access after prompt.";
                     } else {
                         qWarning() << "[Permission Check macOS] User explicitly denied microphone access after prompt.";
                     }
                 }];
                 qInfo() << "[Permission Check macOS] Requested microphone access (prompt should appear). Returning Undetermined for now.";
                return MicrophonePermissionStatus::Undetermined;

            default:
                qWarning() << "[Permission Check macOS] Unknown microphone authorization status value:" << status;
                return MicrophonePermissionStatus::Error;
        }
    } else {
        // On macOS versions older than 10.14, permission wasn't explicitly required this way.
        qInfo() << "[Permission Check macOS] Microphone permission check not needed/applicable on this macOS version (< 10.14)";
        return MicrophonePermissionStatus::Granted; // Assume granted on older systems
    }
}

#endif // Q_OS_MAC

// --- C++ Interface Implementation ---

MicrophonePermissionStatus checkMicrophonePermission() {
#ifdef Q_OS_MAC
    // Call the Objective-C++ helper function for macOS
    return getMicrophonePermissionStatus_macOS();
#else
    // On non-Mac platforms, this check isn't needed/applicable
    return MicrophonePermissionStatus::NotApplicable;
#endif
}

// --- C++ Helper for Warnings (Keep this as is) ---

void showMicrophonePermissionWarning(MicrophonePermissionStatus status, QWidget *parent) {
    switch(status) {
        case MicrophonePermissionStatus::Denied:
            QMessageBox::critical(parent,
                                  QObject::tr("Microphone Access Denied"),
                                  QObject::tr("XFB has been denied access to the microphone.\n\n"
                                              "Audio recording and potentially other audio features will not work.\n\n"
                                              "Please go to System Settings > Privacy & Security > Microphone "
                                              "and enable access for XFB."),
                                  QMessageBox::Ok);
            break;
        case MicrophonePermissionStatus::Undetermined:
             // Slightly different message now that we trigger the request
             QMessageBox::information(parent,
                                   QObject::tr("Microphone Access Required"),
                                   QObject::tr("XFB needs access to the microphone for audio features.\n\n"
                                               "macOS should now prompt you for permission.\n\n"
                                               "Please click 'Allow' or 'OK' when prompted for audio features to work correctly."),
                                   QMessageBox::Ok);
             break;
        case MicrophonePermissionStatus::Error:
            QMessageBox::warning(parent,
                                 QObject::tr("Permission Check Error"),
                                 QObject::tr("Could not determine microphone permission status."),
                                 QMessageBox::Ok);
            break;
        case MicrophonePermissionStatus::Granted:
        case MicrophonePermissionStatus::NotApplicable:
            // No warning needed
            break;
    }
}
