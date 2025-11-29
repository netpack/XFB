# Custom Qt6 finder for XFB project
# This helps with Qt6 detection across different platforms

if(Qt6_FOUND)
    return()
endif()

# Set Qt6 installation hints
if(DEFINED ENV{QT_DIR})
    set(Qt6_DIR $ENV{QT_DIR})
endif()

if(DEFINED ENV{QTDIR})
    set(Qt6_DIR $ENV{QTDIR})
endif()

# Platform-specific Qt6 paths
if(WIN32)
    set(Qt6_POSSIBLE_PATHS
        "C:/Qt/6.5.0/msvc2019_64"
        "C:/Qt/6.4.0/msvc2019_64"
        "C:/Qt/Tools/QtCreator/bin"
    )
elseif(APPLE)
    set(Qt6_POSSIBLE_PATHS
        "/usr/local/Qt-6.5.0"
        "/opt/homebrew/opt/qt6"
        "/usr/local/opt/qt6"
        "$ENV{HOME}/Qt/6.5.0/clang_64"
    )
else()
    set(Qt6_POSSIBLE_PATHS
        "/usr/lib/qt6"
        "/usr/local/qt6"
        "/opt/qt6"
    )
endif()

# Try to find Qt6 in possible paths
foreach(path ${Qt6_POSSIBLE_PATHS})
    if(EXISTS "${path}/lib/cmake/Qt6")
        set(Qt6_DIR "${path}/lib/cmake/Qt6")
        break()
    endif()
endforeach()

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

if(Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_DIR}")
    
    # Set Qt6 variables for compatibility
    set(QT_VERSION_MAJOR 6)
    set(QT_VERSION_MINOR ${Qt6Core_VERSION_MINOR})
    set(QT_VERSION_PATCH ${Qt6Core_VERSION_PATCH})
    set(QT_VERSION "${Qt6Core_VERSION}")
    
    # Enable Qt features
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    # Qt6 specific settings
    if(Qt6_VERSION VERSION_GREATER_EQUAL "6.0.0")
        set_property(TARGET Qt6::Core PROPERTY INTERFACE_QT_MAJOR_VERSION 6)
    endif()
    
else()
    message(FATAL_ERROR "Qt6 not found. Please install Qt6 or set Qt6_DIR")
endif()