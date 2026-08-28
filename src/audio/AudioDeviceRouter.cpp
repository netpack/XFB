#include "AudioDeviceRouter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMediaDevices>

namespace AudioDeviceRouter {

QList<QAudioDevice> outputs()
{
    return QMediaDevices::audioOutputs();
}

bool isPresent(const QByteArray &storedId)
{
    if (storedId.isEmpty())
        return false;
    const QList<QAudioDevice> devices = outputs();
    for (const QAudioDevice &device : devices) {
        if (device.id() == storedId)
            return true;
    }
    return false;
}

QAudioDevice resolve(const QByteArray &storedId, bool *fellBack)
{
    if (fellBack)
        *fellBack = false;

    if (!storedId.isEmpty()) {
        const QList<QAudioDevice> devices = outputs();
        for (const QAudioDevice &device : devices) {
            if (device.id() == storedId)
                return device;
        }
        // Named, but gone. On air that means the operator unplugged
        // something mid-show: keep the audio going on whatever the system
        // now considers the default and let the caller say so in the log.
        if (fellBack)
            *fellBack = true;
    }

    return QMediaDevices::defaultAudioOutput();
}

QAudioDevice resolveStrict(const QByteArray &storedId)
{
    if (storedId.isEmpty())
        return QAudioDevice();

    const QList<QAudioDevice> devices = outputs();
    for (const QAudioDevice &device : devices) {
        if (device.id() == storedId)
            return device;
    }
    return QAudioDevice(); // never the default: see the header
}

QString describe(const QByteArray &storedId)
{
    if (storedId.isEmpty()) {
        const QAudioDevice def = QMediaDevices::defaultAudioOutput();
        return def.isNull()
                   ? QCoreApplication::translate("AudioDeviceRouter", "System default")
                   : QCoreApplication::translate("AudioDeviceRouter", "System default (%1)")
                         .arg(def.description());
    }

    const QList<QAudioDevice> devices = outputs();
    for (const QAudioDevice &device : devices) {
        if (device.id() == storedId)
            return device.description();
    }
    return QCoreApplication::translate("AudioDeviceRouter", "%1 (not connected)")
        .arg(QString::fromUtf8(storedId));
}

} // namespace AudioDeviceRouter
