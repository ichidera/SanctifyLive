cmake_minimum_required(VERSION 3.16)
project(SanctifyLive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Locate Qt --------------------------------------------------------
# CMake does not scan your whole disk for Qt - it needs CMAKE_PREFIX_PATH
# (or the Qt6_DIR/Qt5_DIR env var) pointing at the Qt kit you want to build
# against. See README.md for the exact command for your setup. Common fix:
#   cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
if(DEFINED ENV{QTDIR} AND NOT DEFINED CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
if(NOT QT_FOUND)
    message(FATAL_ERROR
        "\nQt was not found by CMake.\n"
        "Point CMake at your Qt install, e.g.:\n"
        "  cmake .. -DCMAKE_PREFIX_PATH=\"C:/Qt/6.8.0/msvc2022_64\"\n"
        "or set the CMAKE_PREFIX_PATH / QTDIR environment variable.\n"
        "See README.md for platform-specific instructions.\n")
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(SanctifyLive
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/TopToolBar.h
    src/TopToolBar.cpp
    src/LiveTranscriptionPanel.h
    src/LiveTranscriptionPanel.cpp
    src/PreviewLivePanel.h
    src/PreviewLivePanel.cpp
    src/CitationFeedPanel.h
    src/CitationFeedPanel.cpp
    src/LibraryPanel.h
    src/LibraryPanel.cpp
    src/QueuePanel.h
    src/QueuePanel.cpp
    src/StatusBarWidget.h
    src/StatusBarWidget.cpp
    src/Theme.h
)

target_link_libraries(SanctifyLive PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
