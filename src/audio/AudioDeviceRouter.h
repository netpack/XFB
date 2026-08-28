#ifndef AUDIODEVICEROUTER_H
#define AUDIODEVICEROUTER_H

#include <QAudioDevice>
#include <QByteArray>
#include <QList>
#include <QString>

/**
 * @brief One place that turns a *stored* audio-output device id into a real
 *        QAudioDevice.
 *
 * XFB persists output devices by QAudioDevice::id(), which is an opaque
 * platform handle: it is stable while the device exists and simply stops
 * being listed when the headphones are unplugged, the USB card is pulled or
 * the Bluetooth link drops. Everything that opens an output — the FX
 * engine's QAudioSink, the passthrough QAudioOutput, the pads — has to make
 * the same decision about what to do then, so the decision lives here.
 *
 * There are deliberately two resolutions, and the difference matters:
 *
 *  - resolve() is for the *on-air* outputs. An empty id means "system
 *    default", and an id that no longer names a present device also falls
 *    back to the default, because silence on air is worse than the wrong
 *    speaker. The caller is told (through @a fellBack) so it can log it.
 *
 *  - resolveStrict() is for the *cue* (pre-fade listen) output. It returns
 *    the named device or a null device, never the default. A cue that fell
 *    back to the default would be played out of the on-air output — the one
 *    failure mode that makes a cue bus worse than having none — so a cue
 *    whose device has vanished must go silent instead.
 */
namespace AudioDeviceRouter {

/** Every audio output the system currently offers. */
QList<QAudioDevice> outputs();

/** True when @a storedId names a device that is present right now. */
bool isPresent(const QByteArray &storedId);

/**
 * Resolve a stored id for an on-air output.
 *
 * @param storedId  empty for "system default", otherwise a QAudioDevice::id()
 * @param fellBack  set to true when a named device was asked for and the
 *                  system default was returned instead (the caller logs it)
 * @return the device to open; null only when the machine has no output at all
 */
QAudioDevice resolve(const QByteArray &storedId, bool *fellBack = nullptr);

/**
 * Resolve a stored id for the cue output: that device or nothing.
 * Returns a null QAudioDevice for an empty id and for one that is not
 * currently present. Never returns the default device.
 */
QAudioDevice resolveStrict(const QByteArray &storedId);

/** Human-readable name for a stored id, for logs, tooltips and speech. */
QString describe(const QByteArray &storedId);

} // namespace AudioDeviceRouter

#endif // AUDIODEVICEROUTER_H
