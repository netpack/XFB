# UI tests directory

# UI test for enhanced music dialogs
add_executable(test_enhanced_music_dialogs
    TestEnhancedMusicDialogs.cpp
    TestEnhancedMusicDialogs.h
    ${CMAKE_SOURCE_DIR}/src/dialogs/EnhancedAddMusicSingleDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/dialogs/EnhancedAddDirectoryDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/models/MusicListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/MusicRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/InputValidator.cpp
)

target_link_libraries(test_enhanced_music_dialogs
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Test
    TestUtils
)

target_include_directories(test_enhanced_music_dialogs PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# UI tests require a display, so they're not run automatically in CI
add_test(NAME EnhancedMusicDialogsUITest 
         COMMAND test_enhanced_music_dialogs
         CONFIGURATIONS Debug Release)

# Set test properties for UI tests
set_tests_properties(EnhancedMusicDialogsUITest PROPERTIES
    TIMEOUT 120  # 2 minutes timeout for UI tests
    LABELS "ui"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"  # Run tests without display
)

# Add custom target for UI tests
add_custom_target(ui_tests
    DEPENDS test_enhanced_music_dialogs
    COMMENT "Building UI tests"
)

# Custom command to run UI tests
add_custom_target(run_ui_tests
    COMMAND test_enhanced_music_dialogs
    DEPENDS test_enhanced_music_dialogs
    COMMENT "Running UI tests"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)