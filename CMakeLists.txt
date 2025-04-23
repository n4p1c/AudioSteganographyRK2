cmake_minimum_required(VERSION 3.16)
project(AudioSteganography LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please install Qt6 (e.g., via Qt Online Installer).")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
if(NOT SNDFILE_FOUND)
    message(FATAL_ERROR "libsndfile not found. Please install it (e.g., brew install libsndfile on macOS).")
endif()

include_directories(${SNDFILE_INCLUDE_DIRS})

link_directories(${SNDFILE_LIBRARY_DIRS})

add_executable(AudioSteganography src/main.cpp)

target_link_libraries(AudioSteganography PRIVATE Qt6::Widgets ${SNDFILE_LIBRARIES})