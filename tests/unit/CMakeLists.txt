# Unit tests directory

# Test for configuration functionality
add_executable(test_config
    test_config.cpp
    ${CMAKE_SOURCE_DIR}/src/config.cpp
    ${CMAKE_SOURCE_DIR}/src/config.h
)

target_link_libraries(test_config
    Qt6::Core
    Qt6::Test
    TestUtils
)

target_include_directories(test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME ConfigTest COMMAND test_config)

# Test for database operations (basic)
add_executable(test_database
    test_database.cpp
)

target_link_libraries(test_database
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_database PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME DatabaseTest COMMAND test_database)

# Service layer tests
add_executable(test_service_container
    services/TestServiceContainer.cpp
    services/TestServiceContainer.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
)

target_link_libraries(test_service_container
    Qt6::Core
    Qt6::Test
    TestUtils
)

target_include_directories(test_service_container PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME ServiceContainerTest COMMAND test_service_container)

add_executable(test_base_service
    services/TestBaseService.cpp
    services/TestBaseService.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
)

target_link_libraries(test_base_service
    Qt6::Core
    Qt6::Test
    TestUtils
)

target_include_directories(test_base_service PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME BaseServiceTest COMMAND test_base_service)

add_executable(test_database_service_unit
    services/TestDatabaseService.cpp
    services/TestDatabaseService.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseService.cpp
)

target_link_libraries(test_database_service_unit
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_database_service_unit PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME DatabaseServiceUnitTest COMMAND test_database_service_unit)

# Repository layer tests
add_executable(test_music_repository
    repositories/TestMusicRepository.cpp
    repositories/TestMusicRepository.h
    ${CMAKE_SOURCE_DIR}/src/repositories/MusicRepository.cpp
)

target_link_libraries(test_music_repository
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_music_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME MusicRepositoryTest COMMAND test_music_repository)

add_executable(test_genre_repository
    repositories/TestGenreRepository.cpp
    repositories/TestGenreRepository.h
    ${CMAKE_SOURCE_DIR}/src/repositories/GenreRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
)

target_link_libraries(test_genre_repository
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_genre_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME GenreRepositoryTest COMMAND test_genre_repository)

add_executable(test_playlist_repository
    repositories/TestPlaylistRepository.cpp
    repositories/TestPlaylistRepository.h
    ${CMAKE_SOURCE_DIR}/src/repositories/PlaylistRepository.cpp
)

target_link_libraries(test_playlist_repository
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_playlist_repository PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME PlaylistRepositoryTest COMMAND test_playlist_repository)

add_executable(test_database_migrator
    repositories/TestDatabaseMigrator.cpp
    repositories/TestDatabaseMigrator.h
    ${CMAKE_SOURCE_DIR}/src/repositories/DatabaseMigrator.cpp
)

target_link_libraries(test_database_migrator
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_database_migrator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME DatabaseMigratorTest COMMAND test_database_migrator)

add_executable(test_audio_service
    services/TestAudioService.cpp
    services/TestAudioService.h
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
)

target_link_libraries(test_audio_service
    Qt6::Core
    Qt6::Multimedia
    Qt6::Test
    TestUtils
)

target_include_directories(test_audio_service PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME AudioServiceTest COMMAND test_audio_service)

add_executable(test_error_handler
    services/TestErrorHandler.cpp
    services/TestErrorHandler.h
    ${CMAKE_SOURCE_DIR}/src/services/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
)

target_link_libraries(test_error_handler
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    TestUtils
)

target_include_directories(test_error_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME ErrorHandlerTest COMMAND test_error_handler)

add_executable(test_logger
    services/TestLogger.cpp
    services/TestLogger.h
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
)

target_link_libraries(test_logger
    Qt6::Core
    Qt6::Test
    TestUtils
)

target_include_directories(test_logger PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME LoggerTest COMMAND test_logger)

add_executable(test_input_validator
    services/TestInputValidator.cpp
    services/TestInputValidator.h
    ${CMAKE_SOURCE_DIR}/src/services/InputValidator.cpp
)

target_link_libraries(test_input_validator
    Qt6::Core
    Qt6::Test
    TestUtils
)

target_include_directories(test_input_validator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME InputValidatorTest COMMAND test_input_validator)

add_executable(test_database_optimizer
    services/TestDatabaseOptimizer.cpp
    services/TestDatabaseOptimizer.h
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseOptimizer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
)

target_link_libraries(test_database_optimizer
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
    Qt6::Test
    TestUtils
)

target_include_directories(test_database_optimizer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME DatabaseOptimizerTest COMMAND test_database_optimizer)

add_executable(test_music_cache
    services/TestMusicCache.cpp
    services/TestMusicCache.h
    ${CMAKE_SOURCE_DIR}/src/services/MusicCache.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/MusicRepository.cpp
)

target_link_libraries(test_music_cache
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
    Qt6::Test
    TestUtils
)

target_include_directories(test_music_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME MusicCacheTest COMMAND test_music_cache)

# Controller layer tests
add_executable(test_main_controller
    controllers/TestMainController.cpp
    controllers/TestMainController.h
    ${CMAKE_SOURCE_DIR}/src/controllers/MainController.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ConfigurationService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WidgetAccessibilityEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioFeedbackService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BackgroundOperationFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/services/LiveRegionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlaybackStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/SystemStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlayerAudioFeedbackIntegration.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/MusicRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GenreRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/PlaylistRepository.cpp
)

target_link_libraries(test_main_controller
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Test
    TestUtils
)

target_include_directories(test_main_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME MainControllerTest COMMAND test_main_controller)

add_executable(test_accessibility_manager
    services/TestAccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceContainer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AccessibilityManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/WidgetAccessibilityEnhancer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioFeedbackService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BackgroundOperationFeedback.cpp
    ${CMAKE_SOURCE_DIR}/src/services/LiveRegionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlaybackStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/SystemStatusAnnouncer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/PlayerAudioFeedbackIntegration.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AudioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/Logger.cpp
)

target_link_libraries(test_accessibility_manager
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Test
    TestUtils
)

target_include_directories(test_accessibility_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

add_test(NAME AccessibilityManagerTest COMMAND test_accessibility_manager)

# Add custom target for unit tests
add_custom_target(unit_tests
    DEPENDS test_config test_database test_service_container test_base_service test_database_service_unit test_music_repository test_genre_repository test_playlist_repository test_database_migrator test_audio_service test_error_handler test_logger test_input_validator test_database_optimizer test_music_cache test_main_controller test_accessibility_manager
    COMMENT "Building unit tests"
)