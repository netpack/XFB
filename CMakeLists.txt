cmake_minimum_required(VERSION 3.12)

project(XFB)

set(CMAKE_CXX_STANDARD 11) # This is equal to QMAKE_CXX_FLAGS += -std=c++0x

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to run uic automatically when needed.
set(CMAKE_AUTOUIC ON)
SET( QT_USE_PHONON TRUE )
find_package(Qt5 REQUIRED COMPONENTS Core Quick Sql Widgets Multimedia DataVisualization)

set(SOURCES aboutus.cpp add_full_dir.cpp add_music_single.cpp add_program.cpp add_pub.cpp addgenre.cpp addjingle.cpp audioinput.cpp audiooutput.cpp buymecoffee.cpp commonFunctions.cpp dbconn.cpp dialog.cpp main.cpp manage_db.cpp moc_aboutus.cpp moc_add_full_dir.cpp moc_add_music_single.cpp moc_add_program.cpp moc_add_pub.cpp moc_addgenre.cpp moc_addjingle.cpp moc_audioinput.cpp moc_config.cpp moc_optionsdialog.cpp moc_player.cpp moc_qaudiolevel.cpp moc_youtubedownloader.cpp optionsdialog.cpp player.cpp qaudiolevel.cpp qrc_jquery.cpp qrc_resources.cpp qrc_tr.cpp qvumeter.cpp youtubedownloader.cpp) 
set(HEADERS aboutus.h add_full_dir.h add_music_single.h add_program.h add_pub.h addgenre.h addjingle.h audioinput.h audiooutput.h buymecoffee.h commonFunctions.h config.h dbconn.h dialog.h manage_db.h moc_predefs.h optionsdialog.h player.h qaudiolevel.h qvumeter.h ui_aboutus.h ui_add_full_dir.h ui_add_music_single.h ui_add_program.h ui_add_pub.h ui_addgenre.h ui_addjingle.h ui_optionsdialog.h ui_player.h ui_youtubedownloader.h youtubedownloader.h)
set(UI aboutus.ui add_full_dir.ui add_music_single.ui add_program.ui add_pub.ui addgenre.ui addjingle.ui buymecoffee.ui dialog.ui manage_db.ui optionsdialog.ui player.ui youtubedownloader.ui
)

# This will create you executable
add_executable(XFB ${SOURCES} ${HEADERS} ${UI})
# This will link necessary Qt5 libraries to your project
target_link_libraries(XFB Qt5::Core Qt5::Quick Qt5::Sql Qt5::Widgets Qt5::Multimedia Qt5::DataVisualization)
