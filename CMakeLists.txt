cmake_minimum_required(VERSION 3.16)

project(XFB VERSION 2.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
    Multimedia
    Sql
    Network
    WebEngineCore
    WebEngineQuick
    QuickWidgets
    Test
)

# Enable Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add subdirectories
add_subdirectory(src)

# Add tests subdirectory if BUILD_TESTING is enabled (default ON)
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "xfb")
set(CPACK_PACKAGE_VENDOR "Netpack")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XFB Radio Automation Software")
set(CPACK_PACKAGE_DESCRIPTION "XFB Radio Automation Software is a professional radio automation solution designed for radio stations and broadcasting professionals. Features comprehensive accessibility support including ORCA screen reader integration, keyboard navigation, and audio feedback.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "XFB")
set(CPACK_PACKAGE_CONTACT "Netpack <info@netpack.pt>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://netpack.pt")

# Set maintainer for all package types
set(CPACK_PACKAGE_MAINTAINER "Netpack <info@netpack.pt>")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;RPM")
    
    # Debian-specific packaging
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Netpack <info@netpack.pt>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    # Auto-detect architecture or use provided value
    if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
        execute_process(COMMAND dpkg --print-architecture
                       OUTPUT_VARIABLE DETECTED_ARCH
                       OUTPUT_STRIP_TRAILING_WHITESPACE
                       ERROR_QUIET)
        if(DETECTED_ARCH)
            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${DETECTED_ARCH}")
        else()
            # Fallback architecture detection
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
                set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
                set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
            else()
                set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")  # Default fallback
            endif()
        endif()
    endif()
    message(STATUS "Package architecture: ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    
    # Enhanced dependencies for accessibility
    set(CPACK_DEBIAN_PACKAGE_DEPENDS 
        "libqt6core6, libqt6gui6, libqt6widgets6, libqt6multimedia6, libqt6sql6, libqt6network6, libqt6webenginecore6, libqt6webengine6-data, libqt6quick6, libqt6qml6, libatspi2.0-0, at-spi2-core, libspeechd2, libasound2, libpulse0, libsqlite3-0, libcurl4")
    
    # Recommended packages for full accessibility
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS 
        "orca, speech-dispatcher, espeak-ng, brltty, liblouis20, pulseaudio")
    
    # Suggested packages for enhanced experience
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS 
        "festival, pipewire, jackd2, qjackctl")
    
    # Package conflicts
    set(CPACK_DEBIAN_PACKAGE_CONFLICTS "xfb")
    
    # Control scripts
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
        "${CMAKE_SOURCE_DIR}/debian/postinst;${CMAKE_SOURCE_DIR}/debian/prerm;${CMAKE_SOURCE_DIR}/debian/postrm")
    
    # File permissions
    set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
    
    # Package description
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION 
        "XFB Radio Automation Software
 XFB is a professional radio automation software designed for radio stations
 and broadcasting professionals. The software provides comprehensive
 accessibility support for visually impaired users including:
 .
  * Full ORCA screen reader integration
  * Complete keyboard navigation support
  * Audio feedback for all user actions
  * Braille display support via BrlTTY
  * Customizable accessibility preferences
  * Live region updates for dynamic content
  * Professional audio processing capabilities
 .
 The software supports multiple audio formats, playlist management, live
 broadcasting, and automated scheduling with full accessibility compliance
 following WCAG 2.1 AA guidelines.")

endif()

# Include CPack at the end
include(CPack)