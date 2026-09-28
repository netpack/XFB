# RtMidi 5.0.0 (MIT, see LICENSE): the MIDI input behind XFB's MIDI learn.
#
# Built here, from the two upstream files, rather than looked up on the
# system: every package XFB ships (deb, rpm, Arch, Flatpak, AppImage, the
# Windows installer and the macOS bundle) then gets the same MIDI support
# with nothing new to install. The backend is picked per platform:
#
#   Linux    ALSA sequencer, when the ALSA headers are there (they are in
#            every recipe XFB builds with); otherwise RtMidi's dummy API,
#            which lists no ports, and XFB says there is no MIDI support.
#   Windows  Windows Multimedia (winmm), part of the OS.
#   macOS    CoreMIDI, part of the OS.
add_library(xfb_rtmidi STATIC RtMidi.cpp RtMidi.h)
target_include_directories(xfb_rtmidi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(xfb_rtmidi PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    target_compile_definitions(xfb_rtmidi PUBLIC __WINDOWS_MM__)
    target_link_libraries(xfb_rtmidi PUBLIC winmm)
elseif(APPLE)
    target_compile_definitions(xfb_rtmidi PUBLIC __MACOSX_CORE__)
    target_link_libraries(xfb_rtmidi PUBLIC
        "-framework CoreMIDI" "-framework CoreAudio" "-framework CoreFoundation")
else()
    find_package(ALSA QUIET)
    if(ALSA_FOUND)
        target_compile_definitions(xfb_rtmidi PUBLIC __LINUX_ALSA__)
        target_link_libraries(xfb_rtmidi PUBLIC ALSA::ALSA)
        find_package(Threads REQUIRED)
        target_link_libraries(xfb_rtmidi PUBLIC Threads::Threads)
        message(STATUS "MIDI input: ALSA sequencer")
    else()
        target_compile_definitions(xfb_rtmidi PUBLIC __RTMIDI_DUMMY__)
        message(WARNING "ALSA headers not found: XFB builds without MIDI input. "
                        "Install libasound2-dev (alsa-lib-devel) and reconfigure.")
    endif()
endif()

# Upstream code: its warnings are not ours to fix.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xfb_rtmidi PRIVATE -w)
elseif(MSVC)
    target_compile_options(xfb_rtmidi PRIVATE /w)
endif()
