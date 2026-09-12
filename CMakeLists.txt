cmake_minimum_required(VERSION 3.16)

project(SanctifyLive VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler needs these on for any QObject-derived class.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(SanctifyLive
    src/main.cpp
    src/core/Slide.h
    src/core/ScheduleModel.h
    src/core/ScheduleModel.cpp
    src/core/OutputWindow.h
    src/core/OutputWindow.cpp
    src/core/OperatorWindow.h
    src/core/OperatorWindow.cpp
)

target_include_directories(SanctifyLive PRIVATE src)

target_link_libraries(SanctifyLive PRIVATE Qt6::Widgets)

# Keep warnings visible during early development rather than silencing them.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SanctifyLive PRIVATE -Wall -Wextra)
endif()