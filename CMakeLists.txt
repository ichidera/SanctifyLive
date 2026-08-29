cmake_minimum_required(VERSION 3.16)

project(SanctifyLive VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler needs these on for any QObject-derived class.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

add_executable(SanctifyLive
    src/main.cpp
    src/core/Slide.h
    src/core/ScheduleModel.h
    src/core/ScheduleModel.cpp
    src/core/OutputWindow.h
    src/core/OutputWindow.cpp
    src/core/IconFactory.h
    src/core/MediaLibraryPanel.h
    src/core/MediaLibraryPanel.cpp
    src/core/scripture/BibleLibrary.h
    src/core/scripture/BibleLibrary.cpp
    src/core/scripture/ScripturePanel.h
    src/core/scripture/ScripturePanel.cpp
    src/core/HistoryBar.h
    src/core/HistoryBar.cpp
    src/core/SlideServer.h
    src/core/SlideServer.cpp
    src/core/OperatorWindow.h
    src/core/OperatorWindow.cpp
)

target_include_directories(SanctifyLive PRIVATE src)

target_link_libraries(SanctifyLive PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql)

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