cmake_minimum_required(VERSION 3.16)

project(SanctifyLive VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler needs these on for any QObject-derived class.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# src/ is split by responsibility, not just alphabetically:
#   core/model/   - plain data + state (Slide, ScheduleModel) and file I/O
#                   for it (ScheduleIO). No Qt widgets, no rendering. Safe
#                   to reuse from a future headless/CLI mode.
#   core/storage/ - persistence for app-level state that isn't a Slide
#                   schedule (MediaLibraryStore remembers imported media
#                   across relaunches). Same no-Qt-widgets rule as
#                   core/model/.
#   core/render/  - turns model data into pixels (SlideRenderer, the
#                   design-resolution contract). No knowledge of physical
#                   windows.
#   ui/          - Qt widgets that display things and take input
#                  (OperatorWindow, OutputWindow, SlideCanvas,
#                  MediaLibraryPanel, Theme). No slide-drawing logic of
#                  its own -- it only asks core/render for pixmaps.
#
# Dependency direction is one-way: ui/ -> core/render/ -> core/model/.
# Nothing in core/ ever includes anything from ui/, which is what lets a
# future headless mode, CLI, or alternate front-end reuse the same model
# and renderer without dragging in Qt widget code. ui/ living as a
# top-level sibling of core/ (rather than nested inside it) makes that
# boundary visible at a glance in the directory tree, not just in code
# review.
add_executable(SanctifyLive
    src/main.cpp

    src/core/model/Slide.h
    src/core/model/ScheduleModel.h
    src/core/model/ScheduleModel.cpp
    src/core/model/ScheduleIO.h
    src/core/model/ScheduleIO.cpp

    src/core/storage/MediaLibraryStore.h
    src/core/storage/MediaLibraryStore.cpp

    src/core/render/RenderResolution.h
    src/core/render/SlideRenderer.h
    src/core/render/SlideRenderer.cpp

    src/ui/Theme.h
    src/ui/Theme.cpp
    src/ui/SlideCanvas.h
    src/ui/SlideCanvas.cpp
    src/ui/MediaLibraryPanel.h
    src/ui/MediaLibraryPanel.cpp
    src/ui/OutputWindow.h
    src/ui/OutputWindow.cpp
    src/ui/OperatorWindow.h
    src/ui/OperatorWindow.cpp
)

target_include_directories(SanctifyLive PRIVATE src)

target_link_libraries(SanctifyLive PRIVATE Qt6::Widgets)

# Keep warnings visible during early development rather than silencing them.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SanctifyLive PRIVATE -Wall -Wextra)
endif()
