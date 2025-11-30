# Integration tests directory

# Integration test for application startup
add_executable(test_app_integration
    test_app_integration.cpp
)

target_link_libraries(test_app_integration
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    TestUtils
)

target_include_directories(test_app_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME AppIntegrationTest COMMAND test_app_integration)

# Integration test for DatabaseService
add_executable(test_database_service_integration
    TestDatabaseService.cpp
    TestDatabaseService.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseService.cpp
)

target_link_libraries(test_database_service_integration
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_database_service_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME DatabaseServiceIntegrationTest COMMAND test_database_service_integration)

# Integration test for PlayerUIController
add_executable(test_player_ui_controller_integration
    TestPlayerUIController.cpp
    TestPlayerUIController.h
    ${CMAKE_SOURCE_DIR}/src/controllers/MainController.cpp
    ${CMAKE_SOURCE_DIR}/src/controllers/PlayerUIController.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ConfigurationService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WidgetAccessibilityEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioFeedbackService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/LiveRegionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlaybackStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/SystemStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlayerAudioFeedbackIntegration.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BackgroundOperationFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/MusicRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/PlaylistRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GenreRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ProgressIndicatorWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/player.cpp
    ${CMAKE_SOURCE_DIR}/src/commonFunctions.cpp
)

target_link_libraries(test_player_ui_controller_integration
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Network
    Qt6::Test
    TestUtils
)

target_include_directories(test_player_ui_controller_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME PlayerUIControllerIntegrationTest COMMAND test_player_ui_controller_integration)

# Accessibility integration tests
add_executable(test_accessibility_integration
    TestAccessibilityIntegration.cpp
    TestAccessibilityIntegration.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WidgetAccessibilityEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioFeedbackService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/LiveRegionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlaybackStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/SystemStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlayerAudioFeedbackIntegration.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BackgroundOperationFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ErrorHandler.cpp
    # Disabled services (not in main build):
    # ${CMAKE_SOURCE_DIR}/src/services/KeyboardNavigationController.cpp
    # ${CMAKE_SOURCE_DIR}/src/services/AccessibilityPerformanceMonitor.cpp
    # ${CMAKE_SOURCE_DIR}/src/services/AccessibilityErrorHandler.cpp
    # ${CMAKE_SOURCE_DIR}/src/services/ATSPIConnectionManager.cpp
)

target_link_libraries(test_accessibility_integration
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Test
    TestUtils
)

target_include_directories(test_accessibility_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME AccessibilityIntegrationTest COMMAND test_accessibility_integration)

# ORCA compatibility tests
add_executable(test_orca_compatibility
    TestORCACompatibility.cpp
    TestORCACompatibility.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WidgetAccessibilityEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioFeedbackService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/LiveRegionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlaybackStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/SystemStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlayerAudioFeedbackIntegration.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BackgroundOperationFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
)

target_link_libraries(test_orca_compatibility
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Test
    TestUtils
)

target_include_directories(test_orca_compatibility PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME ORCACompatibilityTest COMMAND test_orca_compatibility)

# Accessibility user acceptance tests
add_executable(test_accessibility_user_acceptance
    TestAccessibilityUserAcceptance.cpp
    TestAccessibilityUserAcceptance.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WidgetAccessibilityEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioFeedbackService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/LiveRegionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlaybackStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/SystemStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlayerAudioFeedbackIntegration.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BackgroundOperationFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
    # Disabled service (not in main build):
    # ${CMAKE_SOURCE_DIR}/src/services/KeyboardNavigationController.cpp
)

target_link_libraries(test_accessibility_user_acceptance
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Test
    TestUtils
)

target_include_directories(test_accessibility_user_acceptance PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME AccessibilityUserAcceptanceTest COMMAND test_accessibility_user_acceptance)

# Add custom target for integration tests
add_custom_target(integration_tests
    DEPENDS test_app_integration test_database_service_integration test_player_ui_controller_integration 
            test_accessibility_integration test_orca_compatibility test_accessibility_user_acceptance
    COMMENT "Building integration tests"
)