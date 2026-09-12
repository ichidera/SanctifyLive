cmake_minimum_required(VERSION 3.16)
project(SanctifyLive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
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
