#include "commonFunctions.h"
#include <QDebug>

void AudioXversion(){
    qDebug()<<"This version of AudioX is: v0 Beta"<<Qt::endl<<"developed by Netpack - Online Solutions!";

}


#include "commonFunctions.h"

#include <QApplication>
#include <QMessageBox>
#include <QObject>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif

bool ensureMicrophoneAccess(QWidget *parent, const std::function<void()> &onGranted)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QMicrophonePermission permission;
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Granted:
        return true;

    case Qt::PermissionStatus::Undetermined:
        // The system puts the question to the operator. Nothing can be
        // recorded until they answer, so the caller stops here and the work
        // resumes in the reply.
        qApp->requestPermission(permission, parent ? static_cast<QObject *>(parent)
                                                   : static_cast<QObject *>(qApp),
                                [onGranted](const QPermission &answer) {
            if (answer.status() == Qt::PermissionStatus::Granted && onGranted)
                onGranted();
        });
        return false;

    case Qt::PermissionStatus::Denied:
        QMessageBox::warning(parent, QObject::tr("Microphone"),
                             QObject::tr("XFB is not allowed to use the microphone, so it "
                                         "cannot record.\n\nAllow it under Privacy & "
                                         "Security \u2192 Microphone in the system settings, "
                                         "then try again."));
        return false;
    }
    return false;
#else
    // Older Qt has no permission API; the platforms XFB builds against there
    // do not gate the microphone either.
    Q_UNUSED(parent);
    Q_UNUSED(onGranted);
    return true;
#endif
}
