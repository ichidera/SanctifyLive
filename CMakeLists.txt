cmake_minimum_required(VERSION 3.16)

project(SanctifyLive VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler needs these on for any QObject-derived class.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Sql: BibleLibrary reads resources/scripture/bibles.sqlite.
# Concurrent: SlideServer decodes/encodes background images on a worker
# thread (QtConcurrent::run) instead of blocking the GUI thread.
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Concurrent)

# src/core is organized as one module folder per feature area (schedule,
# output, media, scripture, history, remote, settings), each exposing a
# small, mostly self-contained API -- e.g. scripture/ owns everything
# Bible-related, remote/ owns everything about the Android link. Only
# OperatorWindow.{h,cpp} sits directly in src/core/: it's the composition
# root that wires the modules together, not a module itself. When adding
# a new module, give it its own folder here rather than dropping loose
# files into src/core/ directly.

add_executable(SanctifyLive
    src/main.cpp

    # Shared utilities used by more than one module.
    src/core/common/IconFactory.h

    # schedule: the plain-data content model (Slide) and the operator's
    # planned running order (ScheduleModel) -- no UI of its own.
    src/core/schedule/Slide.h
    src/core/schedule/ScheduleModel.h
    src/core/schedule/ScheduleModel.cpp

    # output: the actual on-screen rendering widget, reused for the real
    # congregation-facing window and both in-app preview panes.
    src/core/output/OutputWindow.h
    src/core/output/OutputWindow.cpp
    src/core/output/SlideRenderer.h
    src/core/output/SlideRenderer.cpp
    src/core/output/LiveAppearancePreview.h
    src/core/output/LiveAppearancePreview.cpp

    # media: the resource library grid (images/video/audio placeholders)
    # plus its background-image crop/framing editor.
    src/core/media/MediaLibraryPanel.h
    src/core/media/MediaLibraryPanel.cpp
    src/core/media/ImageFramingDialog.h
    src/core/media/ImageFramingDialog.cpp

    # scripture: Bible verse lookup/search, backed by a bundled SQLite DB.
    src/core/scripture/BibleLibrary.h
    src/core/scripture/BibleLibrary.cpp
    src/core/scripture/ScriptureSearchEdit.h
    src/core/scripture/ScriptureSearchEdit.cpp
    src/core/scripture/TranslationLibraryDialog.h
    src/core/scripture/TranslationLibraryDialog.cpp
    src/core/scripture/ScripturePanel.h
    src/core/scripture/ScripturePanel.cpp
    src/core/scripture/ScriptureFormatting.h

    src/core/song/SongPanel.h
    src/core/song/SongPanel.cpp
    src/core/song/SongEditorDialog.h
    src/core/song/SongEditorDialog.cpp

    src/core/theme/ThemePanel.h
    src/core/theme/ThemePanel.cpp

    # history: the "recently shown" strip above the live workspace.
    src/core/history/HistoryBar.h
    src/core/history/HistoryBar.cpp

    # remote: the Android stage-display link (see src/Android/PROTOCOL.md).
    src/core/remote/SlideServer.h
    src/core/remote/SlideServer.cpp

    # settings: the persistent Edit > Options window. One OutputProfile
    # struct + one OutputSettingsPage tab strip is shared by every output
    # destination (Main Output, Alternate Output, Foldback, Android);
    # pages/ holds each individually-selectable sidebar page.
    src/core/settings/OutputProfile.h
    src/core/settings/SettingsWindow.h
    src/core/settings/SettingsWindow.cpp
    src/core/settings/pages/OutputContentTabs.h
    src/core/settings/pages/OutputContentTabs.cpp
    src/core/settings/pages/OutputSettingsPage.h
    src/core/settings/pages/OutputSettingsPage.cpp
    src/core/settings/pages/OutputPreviewThumbnail.h
    src/core/settings/pages/OutputPreviewThumbnail.cpp
    src/core/settings/pages/AndroidOutputPage.h
    src/core/settings/pages/AndroidOutputPage.cpp
    src/core/settings/pages/ServiceIntervalsPage.h
    src/core/settings/pages/ServiceIntervalsPage.cpp
    src/core/settings/pages/AdvancedPage.h
    src/core/settings/pages/AdvancedPage.cpp

    # The composition root: wires every module above into one app shell.
    src/core/OperatorWindow.h
    src/core/OperatorWindow.cpp
)

target_include_directories(SanctifyLive PRIVATE src)

target_link_libraries(SanctifyLive PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql Qt6::Concurrent)

# Lets BibleLibrary find resources/scripture/bibles.sqlite when running
# straight out of the build tree during development, without requiring
# an install/packaging step first. See BibleLibrary::findDefaultDatabasePath.
target_compile_definitions(SanctifyLive PRIVATE SANCTIFYLIVE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Also copy the Scripture database next to the built executable, so a
# packaged/installed build (where SANCTIFYLIVE_SOURCE_DIR won't exist on
# the target machine) finds it via the "next to the exe" candidate path.
add_custom_command(TARGET SanctifyLive POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:SanctifyLive>/resources/scripture"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/scripture/bibles.sqlite"
        "$<TARGET_FILE_DIR:SanctifyLive>/resources/scripture/bibles.sqlite"
    COMMENT "Copying Scripture database next to the built executable"
)

# Keep warnings visible during early development rather than silencing them.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SanctifyLive PRIVATE -Wall -Wextra)
endif()
