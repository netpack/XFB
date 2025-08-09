#-------------------------------------------------
#
# Project created by QtCreator 2015-09-04T12:24:37
# Writen by Netpack - Online Solutions
# www.netpack.pt
#
#-------------------------------------------------

QT       += core gui multimedia sql webkit webkitwidgets widgets designer network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += C++11

TARGET = XFB
TEMPLATE = app

SOURCES +=\
    externaldownloader.cpp \
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
    audioinput.cpp

HEADERS  += \
    externaldownloader.h \
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
