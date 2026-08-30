#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include <functional>

class QWidget;

void AudioXversion();

/**
 * Make sure XFB may use the microphone before something tries to record.
 *
 * macOS refuses capture until the operator has agreed, and an application that
 * never asks simply records silence — which is what voice tracking and the
 * programme recorder both did. Returns true when recording can start straight
 * away. When the operator has not been asked yet the system asks them, this
 * returns false, and onGranted() runs if they say yes: the caller should
 * return and let the recording begin from there. A refusal is explained,
 * pointing at where it can be changed.
 *
 * On platforms with no such permission model this is always true.
 */
bool ensureMicrophoneAccess(QWidget *parent, const std::function<void()> &onGranted);

#endif // COMMONFUNCTIONS_H
