# Minimal source files for beta build
set(SOURCES
    main.cpp
    player.cpp
    add_music_single.cpp
    add_full_dir.cpp
    addgenre.cpp
    addjingle.cpp
    add_pub.cpp
    optionsdialog.cpp
    aboutus.cpp
    add_program.cpp
    audioinput.cpp
    config.cpp
    externaldownloader.cpp
    commonFunctions.cpp
    QrCode.cpp
    # Shared track-duration probe (exiftool with ffmpeg fallback)
    mediaduration.cpp
    # One definition of "an audio file XFB will import" (extensions, folder walk)
    audioformats.cpp
    # Spotify / Apple Music track listings for the downloader
    streamingcatalog.cpp
    # Audio FX engine (EQ / compressor / 432 Hz)
    audio/FxDsp.cpp
    audio/FxEngine.cpp
    audio/FxPlayer.cpp
    # Output routing: stored device id -> QAudioDevice (on-air vs cue rules)
    audio/AudioDeviceRouter.cpp
    # Cue bus: pre-fade listen on a second output device
    audio/CueBus.cpp
    audio/WaveformStore.cpp
    # Track tempo: detection from the waveform + the library's BPM column
    audio/BpmDetector.cpp
    audio/BpmLibrary.cpp
    # EBU R128 loudness measurement (ffmpeg ebur128) for playback normalisation
    audio/LoudnessScanner.cpp
    # Intro (ramp) / outro detection from the waveform + the library columns
    audio/IntroDetector.cpp
    audio/IntroLibrary.cpp
    # Voice tracking: the take's recorder and the ducking it writes
    audio/VoiceRecorder.cpp
    audio/VoiceDuck.cpp
    dialogs/AudioFxDialog.cpp
    dialogs/MobileSyncDialog.cpp
    # As-run log viewer + advertiser report
    dialogs/AirLogDialog.cpp
    # Rotation editor + the "why did it pick this?" view
    dialogs/RotationDialog.cpp
    dialogs/DeadAirDialog.cpp
    # Listener requests + the switch that puts the public page on the network
    dialogs/RequestTrayDialog.cpp
    dialogs/StationSyncDialog.cpp
    dialogs/ProductionSyncDialog.cpp
    dialogs/StreamDialog.cpp
    # Voice track: record the link over a join, XFB writes the ducking
    dialogs/VoiceTrackDialog.cpp
    # Playlist sound-wave view (crossfade preparation)
    PlaylistWaveView.cpp
    # Pads tab: touch-friendly grid of instant-play pads (cart wall)
    PadBoard.cpp
    # Stereo LED output level meter (Options toggle)
    LevelMeter.cpp
    # Track artwork (cover icons + the Artwork dock panel)
    ArtworkStore.cpp
    # Theme support (palette + generated stylesheet)
    ThemeManager.cpp
    # Essential service layer (working components only)
    services/IService.cpp
    services/ServiceContainer.cpp
    services/BaseService.cpp
    services/DatabaseService.cpp
    services/AudioService.cpp
    services/ConfigurationService.cpp
    services/ErrorHandler.cpp
    services/Logger.cpp
    services/InputValidator.cpp
    services/DatabaseOptimizer.cpp
    services/MusicCache.cpp
    # As-run log: one row per item that actually went to air
    services/AirLog.cpp
    # Rotation rules: separation, categories, dayparts, seasonal windows
    services/RotationRules.cpp
    services/DeadAirWatchdog.cpp
    # The public now-playing page's request line (table, settings, search)
    services/RequestLine.cpp
    # Basic accessibility (working components)
    services/AccessibilityManager.cpp
    services/AccessibilitySettingsService.cpp
    services/BrailleDisplayService.cpp
    services/WidgetAccessibilityEnhancer.cpp
    # Skip problematic accessibility components:
    # services/AccessibleTableInterface.cpp
    # services/AccessiblePlaylistInterface.cpp  # Has compilation errors - disabled for beta
    # services/AccessibleTableEditingEnhancer.cpp  # Has compilation errors - disabled for beta
    # services/AccessibilityValidator.cpp
    # services/KeyboardNavigationController.cpp
    # services/PlayerKeyboardNavigationEnhancer.cpp
    # services/DatabaseGridKeyboardNavigationEnhancer.cpp
    # services/AccessibleHelpSystem.cpp  # Has compilation errors - disabled for beta
    services/LiveRegionManager.cpp
    services/SystemStatusAnnouncer.cpp
    services/AudioFeedbackService.cpp
    services/PlayerAudioFeedbackIntegration.cpp
    services/BackgroundOperationFeedback.cpp
    # Temporarily exclude complex accessibility components for beta build:
    # services/AccessibilityPerformanceMonitor.cpp
    # services/AccessibilityMemoryOptimizer.cpp
    # services/AccessibilityErrorHandler.cpp
    # services/ATSPIConnectionManager.cpp
    # services/ContextSensitiveHelpService.cpp  # Disabled for beta
    # Controller layer
    controllers/MainController.cpp
    controllers/PlayerUIController.cpp
    # UI Components
    ui/ProgressIndicatorWidget.cpp
    # Corner "support XFB" notice (replaces the old startup donation dialog)
    ui/DonationNotice.cpp
    # Models
    models/MusicListModel.cpp
    # Enhanced Dialogs
    dialogs/EnhancedAddMusicSingleDialog.cpp
    dialogs/EnhancedAddDirectoryDialog.cpp
    dialogs/AccessibilityPreferencesDialog.cpp
    dialogs/AccessibilityTutorialDialog.cpp
    # Repository layer
    repositories/MusicRepository.cpp
    repositories/GenreRepository.cpp
    repositories/PlaylistRepository.cpp
    repositories/DatabaseMigrator.cpp
    # Torrent services (required by player.cpp)
    services/TorNetworkService.cpp
    services/TorrentSearchService.cpp
    services/TorrentDownloadService.cpp
    services/DependencyChecker.cpp
    services/NgrokTunnelService.cpp
    services/MobileSyncServer.cpp
    services/StationSyncClient.cpp
    services/ProductionSyncClient.cpp
    services/StreamEncoder.cpp
    services/IcecastSource.cpp
    services/StreamService.cpp
    services/UpdateCheckService.cpp
)

# Header files (corresponding to sources)
set(HEADERS
    player.h
    add_music_single.h
    add_full_dir.h
    addgenre.h
    addjingle.h
    add_pub.h
    optionsdialog.h
    aboutus.h
    add_program.h
    audioinput.h
    config.h
    externaldownloader.h
    commonFunctions.h
    QrCode.h
    mediaduration.h
    audioformats.h
    streamingcatalog.h
    # Audio FX engine headers
    audio/FxParams.h
    audio/FxDsp.h
    audio/FxEngine.h
    audio/FxPlayer.h
    audio/AudioDeviceRouter.h
    audio/CueBus.h
    audio/LoudnessScanner.h
    audio/IntroDetector.h
    audio/IntroLibrary.h
    audio/VoiceRecorder.h
    audio/VoiceDuck.h
    dialogs/AudioFxDialog.h
    dialogs/MobileSyncDialog.h
    dialogs/StationSyncDialog.h
    dialogs/ProductionSyncDialog.h
    dialogs/StreamDialog.h
    dialogs/AirLogDialog.h
    dialogs/RotationDialog.h
    dialogs/DeadAirDialog.h
    dialogs/RequestTrayDialog.h
    dialogs/VoiceTrackDialog.h
    # Pads tab
    PadBoard.h
    # Service layer headers
    services/IService.h
    services/ServiceContainer.h
    services/BaseService.h
    services/DatabaseService.h
    services/AudioService.h
    services/ConfigurationService.h
    services/ErrorHandler.h
    services/Logger.h
    services/InputValidator.h
    services/DatabaseOptimizer.h
    services/MusicCache.h
    services/AirLog.h
    services/RotationRules.h
    services/DeadAirWatchdog.h
    services/RequestLine.h
    services/AccessibilityManager.h
    services/AccessibilitySettingsService.h
    services/BrailleDisplayService.h
    services/WidgetAccessibilityEnhancer.h
    services/LiveRegionManager.h
    services/SystemStatusAnnouncer.h
    services/AudioFeedbackService.h
    services/PlayerAudioFeedbackIntegration.h
    services/BackgroundOperationFeedback.h
    # services/AccessibleHelpSystem.h  # Disabled for beta
    # services/AccessiblePlaylistInterface.h  # Disabled for beta
    # services/AccessibleTableEditingEnhancer.h  # Disabled for beta
    # services/ContextSensitiveHelpService.h  # Disabled for beta
    # Temporarily exclude complex accessibility component headers for beta build:
    # services/AccessibilityPerformanceMonitor.h
    # services/AccessibilityMemoryOptimizer.h
    # services/AccessibilityErrorHandler.h
    # services/ATSPIConnectionManager.h

    # Controller layer
    controllers/MainController.h
    controllers/PlayerUIController.h
    controllers/ModernSignalConnections.h
    # UI Components
    ui/ProgressIndicatorWidget.h
    ui/DonationNotice.h
    # Models
    models/MusicListModel.h
    # Enhanced Dialogs
    dialogs/EnhancedAddMusicSingleDialog.h
    dialogs/EnhancedAddDirectoryDialog.h
    dialogs/AccessibilityPreferencesDialog.h
    dialogs/AccessibilityTutorialDialog.h
    # Repository layer
    repositories/MusicRepository.h
    repositories/GenreRepository.h
    repositories/PlaylistRepository.h
    repositories/DatabaseMigrator.h
    # Torrent service headers
    services/TorNetworkService.h
    services/TorrentSearchService.h
    services/TorrentDownloadService.h
    services/DependencyChecker.h
    services/TorrentTypes.h
    services/NgrokTunnelService.h
    services/MobileSyncServer.h
    services/StationSyncClient.h
    services/ProductionSyncClient.h
    services/StreamEncoder.h
    services/IcecastSource.h
    services/StreamService.h
    services/UpdateCheckService.h
)

# UI files
set(UI_FILES
    player.ui
    add_music_single.ui
    add_full_dir.ui
    addgenre.ui
    addjingle.ui
    add_pub.ui
    optionsdialog.ui
    aboutus.ui
    add_program.ui
    externaldownloader.ui
    dialogs/AccessibilityPreferencesDialog.ui
)

# Resource files
set(RESOURCE_FILES
    tr.qrc
    resources.qrc
)

# Platform-specific sources
if(APPLE)
    list(APPEND SOURCES permission_utils.mm)
    list(APPEND HEADERS permission_utils.h)
elseif(WIN32)
    # Windows-specific sources can be added here
    # list(APPEND SOURCES permission_utils_win.cpp)
endif()

# Create the executable
if(WIN32)
    add_executable(XFB WIN32 ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCE_FILES})
else()
    add_executable(XFB ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCE_FILES})
endif()

# Link Qt6 libraries
target_link_libraries(XFB
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Network
)

# WebEngine / QuickWidgets are optional (not available on e.g. Windows ARM64).
# Link them only when the Qt build provides them. The app does not reference
# their APIs, so their absence does not affect functionality.
foreach(_optional_qt_module WebEngineCore WebEngineQuick QuickWidgets)
    if(TARGET Qt6::${_optional_qt_module})
        target_link_libraries(XFB Qt6::${_optional_qt_module})
    else()
        message(STATUS "Optional Qt module Qt6::${_optional_qt_module} not found - skipping")
    endif()
endforeach()

# Define XFB_HAS_WEBENGINE when Qt WebEngine is available so the (currently
# unused) WebEngine include/code in player.cpp is guarded out on platforms that
# lack it, e.g. Windows on ARM64.
if(TARGET Qt6::WebEngineQuick)
    target_compile_definitions(XFB PRIVATE XFB_HAS_WEBENGINE)
endif()

# Screen reader announcements on older Qt.
#
# QAccessibleAnnouncementEvent — the only way to hand announcement text to a
# screen reader through Qt — arrived in 6.8. Qt 6.4 (what Debian bookworm
# ships) lists QAccessible::Alert among the events its AT-SPI adaptor ignores,
# so on those builds a blind operator is told nothing at all. speech-dispatcher
# is the daemon Orca itself speaks through, so it is the one route left; see
# AccessibilityManager::speakThroughSpeechDispatcher. Not needed once the build
# is on 6.8 or newer, and irrelevant on macOS and Windows.
if(UNIX AND NOT APPLE AND Qt6_VERSION VERSION_LESS "6.8.0")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SPEECHD QUIET speech-dispatcher)
    endif()
    if(SPEECHD_FOUND)
        target_include_directories(XFB PRIVATE ${SPEECHD_INCLUDE_DIRS})
        target_link_directories(XFB PRIVATE ${SPEECHD_LIBRARY_DIRS})
        target_link_libraries(XFB ${SPEECHD_LIBRARIES})
        target_compile_definitions(XFB PRIVATE XFB_HAVE_SPEECHD)
        message(STATUS "Qt ${Qt6_VERSION} predates QAccessibleAnnouncementEvent - "
                       "announcements will be spoken through speech-dispatcher")
    else()
        message(WARNING "Qt ${Qt6_VERSION} predates QAccessibleAnnouncementEvent and "
                        "speech-dispatcher was not found: XFB will be silent for screen "
                        "reader users. Install libspeechd-dev and reconfigure.")
    endif()
endif()

# Platform-specific linking
if(APPLE)
    target_link_libraries(XFB ${AVFOUNDATION_FRAMEWORK})
    
    # Set app bundle properties
    set_target_properties(XFB PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/XFB.app/Contents/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "XFB.icns"
        MACOSX_BUNDLE_GUI_IDENTIFIER "pt.netpack.xfb"
        MACOSX_BUNDLE_BUNDLE_NAME "XFB"
        MACOSX_BUNDLE_DISPLAY_NAME "XFB Radio Automation"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
    
    # Copy icon to bundle
    set(ICON_FILE "${CMAKE_SOURCE_DIR}/XFB.icns")
    if(EXISTS ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(XFB PRIVATE ${ICON_FILE})
    endif()

    # Remove stale qt.conf from previous macdeployqt runs — it overrides Qt's
    # default plugin search paths and breaks dev builds where the binary links
    # to Homebrew Qt rather than bundled frameworks.
    add_custom_command(TARGET XFB POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_BUNDLE_DIR:XFB>/Contents/Resources/qt.conf"
        COMMENT "Removing stale qt.conf from app bundle (if any)"
    )

    # Bundle the server-sync helper script templates (TakeOver / program
    # upload) so serverScriptPath() finds them in Resources/scripts.
    file(GLOB XFB_SERVER_SCRIPTS "${CMAKE_SOURCE_DIR}/scripts/serverFtpCmds*")
    add_custom_command(TARGET XFB POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:XFB>/Contents/Resources/scripts"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XFB_SERVER_SCRIPTS} "$<TARGET_BUNDLE_DIR:XFB>/Contents/Resources/scripts"
        COMMENT "Copying server script templates into app bundle"
    )

    # The phone app, if one has been staged. XFB serves this to phones itself —
    # there is no store in the loop — and Resources is the second place
    # MobileSyncServer::companionApkPath() looks on macOS. Run at build time
    # rather than as an install rule because build-macos.sh takes the bundle
    # straight out of the build tree and never calls cmake --install.
    add_custom_command(TARGET XFB POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DAPK_DIR=${CMAKE_SOURCE_DIR}/packaging/companion
                -DDEST=$<TARGET_BUNDLE_DIR:XFB>/Contents/Resources
                -P ${CMAKE_SOURCE_DIR}/cmake/StageCompanionApk.cmake
        COMMENT "Staging the companion APK into the app bundle (if present)"
    )
elseif(WIN32)
    # Windows-specific properties
    set_target_properties(XFB PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Version resource: gives the executable a publisher, product name,
    # description and version in Windows' file properties and in the
    # SmartScreen prompt. Without it every one of those fields is blank, which
    # is one of the signals that made Windows treat XFB as an unknown app.
    # Deliberately a separate .rc from xfb.rc: that one carries the icon and
    # resolves "xfb.ico" relative to its own directory, so it must stay put.
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/xfb_version.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/xfb_version.rc" @ONLY)
    target_sources(XFB PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/xfb_version.rc")

    # Add Windows application icon via .rc file if it exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/xfb.rc")
        target_sources(XFB PRIVATE "${CMAKE_SOURCE_DIR}/xfb.rc")
        # The .rc only references xfb.ico by name, so CMake/MSVC can't see that
        # the compiled resource depends on the icon. Without this, editing
        # xfb.ico alone does NOT re-run the resource compiler on an incremental
        # build and the executable keeps embedding the OLD icon (e.g. a stale
        # white-background version). Declare the dependency explicitly so the
        # icon is recompiled into the exe whenever it changes.
        if(EXISTS "${CMAKE_SOURCE_DIR}/xfb.ico")
            set_source_files_properties("${CMAKE_SOURCE_DIR}/xfb.rc"
                PROPERTIES OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/xfb.ico")
        endif()
    endif()
else()
    # Linux/Unix
    set_target_properties(XFB PROPERTIES
        INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# Install targets
install(TARGETS XFB
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Install additional files
if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/XFB.desktop"
        DESTINATION share/applications
    )

    # Server-sync helper script templates (TakeOver / program upload). The app
    # resolves these via QStandardPaths::GenericDataLocation ("xfb/scripts/...")
    # and runs them through bash, so no exec bit is required.
    file(GLOB XFB_SERVER_SCRIPTS "${CMAKE_SOURCE_DIR}/scripts/serverFtpCmds*")
    install(FILES ${XFB_SERVER_SCRIPTS}
        DESTINATION share/xfb/scripts
    )

    # The phone app, if one has been staged. OPTIONAL because it is a build
    # product of the Android toolchain: a desktop build with none staged must
    # still install, and XFB then simply says it has no app to hand out.
    # /usr/share/xfb is the last path companionApkPath() tries.
    install(FILES
        "${CMAKE_SOURCE_DIR}/packaging/companion/xfb-companion.apk"
        "${CMAKE_SOURCE_DIR}/packaging/companion/xfb-companion.json"
        DESTINATION share/xfb
        OPTIONAL
    )

    # Pixmaps fallback (used by older menus). The file name must match the
    # desktop entry's "Icon=xfb_icon" key, otherwise the desktop environment
    # falls back to a generic placeholder icon.
    install(FILES "${CMAKE_SOURCE_DIR}/xfb_icon.png"
        DESTINATION share/pixmaps
    )

    # hicolor theme icons (preferred by GNOME/Ubuntu). Each size is installed
    # under the same "xfb_icon" name referenced by XFB.desktop.
    install(FILES "${CMAKE_SOURCE_DIR}/XFB.iconset/icon_16x16.png"
        DESTINATION share/icons/hicolor/16x16/apps RENAME xfb_icon.png)
    install(FILES "${CMAKE_SOURCE_DIR}/XFB.iconset/icon_32x32.png"
        DESTINATION share/icons/hicolor/32x32/apps RENAME xfb_icon.png)
    install(FILES "${CMAKE_SOURCE_DIR}/xfb_icon.png"
        DESTINATION share/icons/hicolor/48x48/apps RENAME xfb_icon.png)
    install(FILES "${CMAKE_SOURCE_DIR}/XFB.iconset/icon_128x128.png"
        DESTINATION share/icons/hicolor/128x128/apps RENAME xfb_icon.png)
    install(FILES "${CMAKE_SOURCE_DIR}/XFB.iconset/icon_256x256.png"
        DESTINATION share/icons/hicolor/256x256/apps RENAME xfb_icon.png)
    install(FILES "${CMAKE_SOURCE_DIR}/XFB.iconset/icon_512x512.png"
        DESTINATION share/icons/hicolor/512x512/apps RENAME xfb_icon.png)
endif()
