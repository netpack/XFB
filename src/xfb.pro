#-------------------------------------------------
#
# Project created by QtCreator 2015-09-04T12:24:37
# Writen by Netpack - Online Solutions
# www.netpack.pt
#
#-------------------------------------------------

QT       += core gui concurrent multimedia sql widgets network webenginecore webenginequick quickwidgets

CONFIG += c++17

TARGET = XFB
TEMPLATE = app

SOURCES += \
    externaldownloader.cpp \
    permission_utils.mm \
    player.cpp \
    main.cpp \
    add_music_single.cpp \
    add_full_dir.cpp \
    addgenre.cpp \
    addjingle.cpp \
    add_pub.cpp \
    optionsdialog.cpp \
    aboutus.cpp \
    add_program.cpp \
    audioinput.cpp \
    config.cpp

HEADERS  += \
    externaldownloader.h \
    permission_utils.h \
    player.h \
    add_music_single.h \
    add_full_dir.h \
    addgenre.h \
    addjingle.h \
    add_pub.h \
    optionsdialog.h \
    aboutus.h \
    add_program.h \
    audioinput.h \
    config.h

FORMS    += \
    externaldownloader.ui \
    player.ui \
    add_music_single.ui \
    add_full_dir.ui \
    addgenre.ui \
    addjingle.ui \
    add_pub.ui \
    optionsdialog.ui \
    aboutus.ui \
    add_program.ui

RESOURCES += \
    tr.qrc \
    resources.qrc

DISTFILES += \
    setup.sh

macx {
    LIBS += -framework AVFoundation
    # Add microphone usage description to Info.plist
    QMAKE_INFO_PLIST_KEY_NSMicrophoneUsageDescription = "XFB needs access to the microphone to list devices and enable recording features."
    ICON = ../XFB.icns
    QMAKE_INFO_PLIST = XFB.app/Contents/Info.plist

}

contains(CONFIG, cross_compile) {
    include(cross_compile.pri)
}

unix {
    QMAKE_LFLAGS_RPATH=
    QMAKE_LFLAGS += -Wl,-rpath,'$$ORIGIN'
}
