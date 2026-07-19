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
    # Audio FX engine (EQ / compressor / 432 Hz)
    audio/FxDsp.cpp
    audio/FxEngine.cpp
    audio/FxPlayer.cpp
    audio/WaveformStore.cpp
    dialogs/AudioFxDialog.cpp
    # Playlist sound-wave view (crossfade preparation)
    PlaylistWaveView.cpp
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
    services/PlaybackStatusAnnouncer.cpp
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
    # Models
    models/MusicListModel.cpp
    # Enhanced Dialogs
    dialogs/EnhancedAddMusicSingleDialog.cpp
    dialogs/EnhancedAddDirectoryDialog.cpp
    dialogs/AccessibilityPreferencesDialog.cpp
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
    # Audio FX engine headers
    audio/FxParams.h
    audio/FxDsp.h
    audio/FxEngine.h
    audio/FxPlayer.h
    dialogs/AudioFxDialog.h
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
    services/AccessibilityManager.h
    services/AccessibilitySettingsService.h
    services/BrailleDisplayService.h
    services/WidgetAccessibilityEnhancer.h
    services/LiveRegionManager.h
    services/PlaybackStatusAnnouncer.h
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
    # Models
    models/MusicListModel.h
    # Enhanced Dialogs
    dialogs/EnhancedAddMusicSingleDialog.h
    dialogs/EnhancedAddDirectoryDialog.h
    dialogs/AccessibilityPreferencesDialog.h
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
elseif(WIN32)
    # Windows-specific properties
    set_target_properties(XFB PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
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
