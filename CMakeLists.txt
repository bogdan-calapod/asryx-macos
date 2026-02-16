cmake_minimum_required(VERSION 3.25)

project(asryx LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Warnings)
include(Hardening)
include(Sanitizers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE ASRYX_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ASRYX_HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.hpp")

add_executable(asryx ${ASRYX_SOURCES} ${ASRYX_HEADERS})
target_include_directories(asryx PRIVATE "${PROJECT_SOURCE_DIR}/src")

asryx_enable_warnings(asryx)
asryx_enable_hardening(asryx)
asryx_enable_sanitizers(asryx)

enable_testing()
add_subdirectory(tests)