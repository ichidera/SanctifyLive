cmake_minimum_required(VERSION 3.16)

project(SanctifyLive VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler needs these on for any QObject-derived class.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL)

# src/ is split by responsibility, not just alphabetically:
#   core/model/   - plain data + state (Slide, ScheduleModel) and file I/O
#                   for it (ScheduleIO). No Qt widgets, no rendering. Safe
#                   to reuse from a future headless/CLI mode.
#   core/storage/ - persistence for app-level state that isn't a Slide
#                   schedule (MediaLibraryStore remembers imported media
#                   across relaunches). Same no-Qt-widgets rule as
#                   core/model/.
#   core/scripture/ - ScriptureLibrary, the bundled-translation loader
#                   for the Scriptures tab (reads the embedded KJV.json
#                   resource -- see resources/Scriptures.qrc). Same
#                   no-Qt-widgets rule as core/model/.
#   core/render/  - turns model data into pixels (SlideRenderer, the
#                   design-resolution contract). No knowledge of physical
#                   windows.
#   hardware/    - HardwareProbe/TierScorer/FeaturePreset/HardwareSettings:
#                  measures the machine SanctifyLive is running on (CPU/
#                  RAM/GPU), classifies it into a HardwareTier, and
#                  persists the result. A top-level sibling of core/
#                  (not nested under it) because it isn't slide/schedule
#                  data -- it's a different kind of "state the app needs
#                  regardless of UI", same no-Qt-widgets rule. Currently
#                  invisible: main.cpp calls it once at startup purely to
#                  detect/log/persist the tier; nothing reads the result
#                  back yet. It exists so a future compositor, transition
#                  system, or thumbnail pipeline has one place to ask
#                  "what can this machine handle" instead of assuming
#                  every machine can do motion backgrounds and live video.
#   ui/          - Qt widgets that display things and take input
#                  (OperatorWindow, OutputWindow, SlideCanvas,
#                  MediaLibraryPanel, ScripturePanel, Theme). No
#                  slide-drawing logic of its own -- it only asks
#                  core/render for pixmaps.
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

    src/core/scripture/ScriptureLibrary.h
    src/core/scripture/ScriptureLibrary.cpp
    src/core/scripture/ScriptureReference.h
    src/core/scripture/ScriptureReference.cpp
    src/core/scripture/ScriptureDetector.h
    src/core/scripture/ScriptureDetector.cpp

    src/core/song/Song.h
    src/core/song/Song.cpp
    src/core/song/SongTextFormat.h
    src/core/song/SongTextFormat.cpp
    src/core/song/SongLibrary.h
    src/core/song/SongLibrary.cpp

    src/core/render/RenderResolution.h
    src/core/render/SlideRenderer.h
    src/core/render/SlideRenderer.cpp

    src/hardware/HardwareProfile.h
    src/hardware/HardwareProbe.h
    src/hardware/HardwareProbe.cpp
    src/hardware/FeaturePreset.h
    src/hardware/FeaturePreset.cpp
    src/hardware/TierScorer.h
    src/hardware/TierScorer.cpp
    src/hardware/HardwareSettings.h
    src/hardware/HardwareSettings.cpp

    src/ui/Theme.h
    src/ui/Theme.cpp
    src/ui/SlideCanvas.h
    src/ui/SlideCanvas.cpp
    src/ui/MediaLibraryPanel.h
    src/ui/MediaLibraryPanel.cpp
    src/ui/HardwareSetupDialog.h
    src/ui/HardwareSetupDialog.cpp
    src/ui/ScripturePanel.h
    src/ui/ScripturePanel.cpp
    src/ui/SongsPanel.h
    src/ui/SongsPanel.cpp
    src/ui/SongEditorDialog.h
    src/ui/SongEditorDialog.cpp
    src/ui/VerseDetectionPanel.h
    src/ui/VerseDetectionPanel.cpp
    src/ui/LiveCaptionsPanel.h
    src/ui/LiveCaptionsPanel.cpp
    src/ui/OutputWindow.h
    src/ui/OutputWindow.cpp
    src/ui/OperatorWindow.h
    src/ui/OperatorWindow.cpp

    resources/Scriptures.qrc
)

target_include_directories(SanctifyLive PRIVATE src)

target_link_libraries(SanctifyLive PRIVATE Qt6::Widgets Qt6::OpenGL)

# Keep warnings visible during early development rather than silencing them.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SanctifyLive PRIVATE -Wall -Wextra)
endif()
