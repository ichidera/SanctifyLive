cmake_minimum_required(VERSION 3.16)

project(SanctifyLive VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler needs these on for any QObject-derived class.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# src/core is split into niches by responsibility, not just alphabetically:
#   model/  - plain data + state (Slide, ScheduleModel). No Qt widgets,
#             no rendering. Safe to reuse from a future headless/CLI mode.
#   render/ - turns model data into pixels (SlideRenderer, the design-
#             resolution contract). No knowledge of physical windows.
#   ui/     - Qt widgets that display things and take input
#             (OutputWindow, OperatorWindow, Theme). No slide-drawing
#             logic of its own -- it only asks render/ for pixmaps.
#   ui/panels/ - self-contained widgets OperatorWindow assembles
#             (QueuePanel, LibraryPanel, LiveCaptionsPanel). Each one
#             only talks to ScheduleModel, never to OperatorWindow, so
#             they can be rearranged or reused independently.
# Dependency direction is one-way: ui/ -> render/ -> model/. Nothing in
# model/ or render/ ever includes anything from ui/.
add_executable(SanctifyLive
    src/main.cpp
    src/core/model/Slide.h
    src/core/model/ScheduleModel.h
    src/core/model/ScheduleModel.cpp
    src/core/render/RenderResolution.h
    src/core/render/SlideRenderer.h
    src/core/render/SlideRenderer.cpp
    src/core/ui/Theme.h
    src/core/ui/Theme.cpp
    src/core/ui/OutputWindow.h
    src/core/ui/OutputWindow.cpp
    src/core/ui/OperatorWindow.h
    src/core/ui/OperatorWindow.cpp
    src/core/ui/panels/QueuePanel.h
    src/core/ui/panels/QueuePanel.cpp
    src/core/ui/panels/LibraryPanel.h
    src/core/ui/panels/LibraryPanel.cpp
    src/core/ui/panels/LiveCaptionsPanel.h
    src/core/ui/panels/LiveCaptionsPanel.cpp
)

target_include_directories(SanctifyLive PRIVATE src)

target_link_libraries(SanctifyLive PRIVATE Qt6::Widgets)

# Keep warnings visible during early development rather than silencing them.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SanctifyLive PRIVATE -Wall -Wextra)
endif()