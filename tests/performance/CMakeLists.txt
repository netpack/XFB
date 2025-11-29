# Performance tests directory

# Performance test for MusicListModel
add_executable(test_music_list_model_performance
    TestMusicListModelPerformance.cpp
    TestMusicListModelPerformance.h
    ${CMAKE_SOURCE_DIR}/src/models/MusicListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/MusicRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/BaseService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/DatabaseService.cpp
)

target_link_libraries(test_music_list_model_performance
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Test
    TestUtils
)

target_include_directories(test_music_list_model_performance PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Performance tests are not run automatically in CI
# They can be run manually for performance analysis
add_test(NAME MusicListModelPerformanceTest 
         COMMAND test_music_list_model_performance
         CONFIGURATIONS Release)

# Set test properties for performance tests
set_tests_properties(MusicListModelPerformanceTest PROPERTIES
    TIMEOUT 300  # 5 minutes timeout for performance tests
    LABELS "performance"
)

# Add custom target for performance tests
add_custom_target(performance_tests
    DEPENDS test_music_list_model_performance
    COMMENT "Building performance tests"
)

# Custom command to run performance tests
add_custom_target(run_performance_tests
    COMMAND test_music_list_model_performance
    DEPENDS test_music_list_model_performance
    COMMENT "Running performance tests"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)