# Test configuration
find_package(Qt6 REQUIRED COMPONENTS Test)

# Enable Qt Test features
set(CMAKE_AUTOMOC ON)

# Include source directory for testing
include_directories(${CMAKE_SOURCE_DIR}/src)

# Test utility functions
add_library(TestUtils STATIC
    test_utils.cpp
    test_utils.h
)

target_link_libraries(TestUtils
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

# Unit tests
add_subdirectory(unit)

# Integration tests
add_subdirectory(integration)

# Performance tests
add_subdirectory(performance)

# UI tests
add_subdirectory(ui)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS unit_tests integration_tests performance_tests ui_tests
    COMMENT "Running all tests"
)

# Test coverage (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()