#include "player.h"
#include <QApplication>
#include <QTranslator>
#include "QFile"
#include <QDebug>
#include <QSplashScreen>
#include <QTimer>

static void Sleep(int ms)
{
#ifdef Q_OS_WIN
    Sleep(uint(ms));
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}


int main(int argc, char *argv[])
{
    QApplication::setDesktopSettingsAware(false);
    QApplication a(argc, argv);
    QSplashScreen splash(QPixmap("://images/splash.png"));
    splash.show();
    a.processEvents();

    Qt::Alignment topCenter = Qt::AlignVCenter | Qt::AlignHCenter;
    splash.showMessage(QObject::tr("Setting up the main window..."),topCenter, Qt::white);

    a.processEvents();
    Sleep(500);




    QThread::currentThread()->setPriority(QThread::HighPriority);
    QTranslator tr;

    splash.showMessage(QObject::tr("Moving to a high priority thread..."),topCenter, Qt::white);
    a.processEvents();

    Sleep(500);

    QFile f ("../config/settings.conf");
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "0 ../config/settings.conf could not be opened for read only ..???.. does it exist?";
        splash.showMessage(QObject::tr("ERROR: FAILD TO OPEN SETTINGS.CONF !!!"),topCenter, Qt::white);
        a.processEvents();
        Sleep(10000);
    }

    QString fullScreen;
    QString idioma = "en";
    QTextStream in(&f);
    qDebug() << "Loading language settings from settings.conf";
    splash.showMessage(QObject::tr("Loading settings..."),topCenter, Qt::white);
    a.processEvents();
    Sleep(500);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList results = line.split(" = ");

        if(results[0]=="Language"){
            idioma = results[1];
            if(idioma=="pt"){
                tr.load(":/portugues.qm");
                qDebug()<<"A Carregar o idioma Portugûes para a GUI...";
                splash.showMessage(QObject::tr("Loading Portuguese GUI..."),topCenter, Qt::white);
                Sleep(300);
            } else if(idioma=="fr"){
                tr.load(":/frances.qm");
                qDebug()<<"A Carregar o idioma Frances para a GUI...";
                splash.showMessage(QObject::tr("Loading French GUI..."),topCenter, Qt::white);
                Sleep(300);
            } else {
                qDebug()<<"Loading English GUI...";
                splash.showMessage(QObject::tr("Loading English GUI..."),topCenter, Qt::white);
                Sleep(300);
            }

        }

        if(results[0]=="FullScreen"){
            fullScreen = results[1];
        }



        a.processEvents();

    }


    splash.showMessage(QObject::tr("Loading settings... Done!"),topCenter, Qt::white);
    Sleep(500);

    if(idioma!="en"){
        a.installTranslator(&tr);
    }


    a.processEvents();


    player w;

    splash.showMessage(QObject::tr("XFB is Loaded!"),topCenter, Qt::white);
    Sleep(1000);
    a.processEvents();

    if(fullScreen=="true"){
        w.showFullScreen();

    } else {
        w.show();
    }

    splash.finish(&w);

   // QApplication::setStyle("cleanlooks");

    return a.exec();
}
