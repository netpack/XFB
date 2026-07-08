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
    main.cpp \
    player.cpp \
    audio/FxDsp.cpp \
    audio/FxEngine.cpp \
    audio/FxPlayer.cpp \
    dialogs/AudioFxDialog.cpp \
    add_music_single.cpp \
    add_full_dir.cpp \
    addgenre.cpp \
    addjingle.cpp \
    add_pub.cpp \
    optionsdialog.cpp \
    aboutus.cpp \
    add_program.cpp \
    audioinput.cpp \
    config.cpp \
    externaldownloader.cpp \
    commonFunctions.cpp \
    services/IService.cpp \
    services/ServiceContainer.cpp \
    services/BaseService.cpp \
    services/DatabaseService.cpp \
    services/AudioService.cpp \
    services/ConfigurationService.cpp \
    services/ErrorHandler.cpp \
    services/Logger.cpp \
    services/InputValidator.cpp \
    services/DatabaseOptimizer.cpp \
    services/MusicCache.cpp \
    services/AccessibilityManager.cpp \
    services/AccessibilitySettingsService.cpp \
    services/BrailleDisplayService.cpp \
    services/WidgetAccessibilityEnhancer.cpp \
    services/LiveRegionManager.cpp \
    services/PlaybackStatusAnnouncer.cpp \
    services/SystemStatusAnnouncer.cpp \
    services/AudioFeedbackService.cpp \
    services/PlayerAudioFeedbackIntegration.cpp \
    services/BackgroundOperationFeedback.cpp \
    controllers/MainController.cpp \
    controllers/PlayerUIController.cpp \
    ui/ProgressIndicatorWidget.cpp \
    models/MusicListModel.cpp \
    dialogs/EnhancedAddMusicSingleDialog.cpp \
    dialogs/EnhancedAddDirectoryDialog.cpp \
    dialogs/AccessibilityPreferencesDialog.cpp \
    repositories/MusicRepository.cpp \
    repositories/GenreRepository.cpp \
    repositories/PlaylistRepository.cpp \
    repositories/DatabaseMigrator.cpp \
    services/TorNetworkService.cpp \
    services/TorrentSearchService.cpp \
    services/TorrentDownloadService.cpp \
    services/NgrokTunnelService.cpp \
    services/UpdateCheckService.cpp

HEADERS += \
    player.h \
    audio/FxParams.h \
    audio/FxDsp.h \
    audio/FxEngine.h \
    audio/FxPlayer.h \
    dialogs/AudioFxDialog.h \
    add_music_single.h \
    add_full_dir.h \
    addgenre.h \
    addjingle.h \
    add_pub.h \
    optionsdialog.h \
    aboutus.h \
    add_program.h \
    audioinput.h \
    config.h \
    externaldownloader.h \
    commonFunctions.h \
    services/IService.h \
    services/ServiceContainer.h \
    services/BaseService.h \
    services/DatabaseService.h \
    services/AudioService.h \
    services/ConfigurationService.h \
    services/ErrorHandler.h \
    services/Logger.h \
    services/InputValidator.h \
    services/DatabaseOptimizer.h \
    services/MusicCache.h \
    services/AccessibilityManager.h \
    services/AccessibilitySettingsService.h \
    services/BrailleDisplayService.h \
    services/WidgetAccessibilityEnhancer.h \
    services/LiveRegionManager.h \
    services/PlaybackStatusAnnouncer.h \
    services/SystemStatusAnnouncer.h \
    services/AudioFeedbackService.h \
    services/PlayerAudioFeedbackIntegration.h \
    services/BackgroundOperationFeedback.h \
    controllers/MainController.h \
    controllers/PlayerUIController.h \
    controllers/ModernSignalConnections.h \
    ui/ProgressIndicatorWidget.h \
    models/MusicListModel.h \
    dialogs/EnhancedAddMusicSingleDialog.h \
    dialogs/EnhancedAddDirectoryDialog.h \
    dialogs/AccessibilityPreferencesDialog.h \
    repositories/MusicRepository.h \
    repositories/GenreRepository.h \
    repositories/PlaylistRepository.h \
    repositories/DatabaseMigrator.h \
    services/TorNetworkService.h \
    services/TorrentSearchService.h \
    services/TorrentDownloadService.h \
    services/NgrokTunnelService.h \
    services/UpdateCheckService.h

FORMS += \
    player.ui \
    add_music_single.ui \
    add_full_dir.ui \
    addgenre.ui \
    addjingle.ui \
    add_pub.ui \
    optionsdialog.ui \
    aboutus.ui \
    add_program.ui \
    externaldownloader.ui \
    dialogs/AccessibilityPreferencesDialog.ui

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
    
    # Add macOS-specific source files
    SOURCES += permission_utils.mm
    HEADERS += permission_utils.h
    
    # Ensure proper deployment target
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
}

contains(CONFIG, cross_compile) {
    include(cross_compile.pri)
}

unix {
    QMAKE_LFLAGS_RPATH=
}
