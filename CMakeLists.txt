cmake_minimum_required(VERSION 3.25)

if(APPLE)
  # macOS 13.4 is required because the onnxruntime prebuilt that sherpa-onnx
  # downloads is built against the 13.4 SDK. ScreenCaptureKit needs 13.0+.
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.4" CACHE STRING "Minimum macOS deployment target" FORCE)
  endif()
  project(asryx LANGUAGES CXX OBJCXX)
else()
  project(asryx LANGUAGES CXX)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Warnings)
include(Hardening)
include(Sanitizers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ASRYX_WHISPER_SOURCE_DIR_DEFAULT "$ENV{HOME}/.local/opt/whisper.cpp"
    CACHE PATH "Default path the installed asryx binary looks for the whisper.cpp source tree")

set(ASRYX_WHISPER_SOURCE_DIR "${ASRYX_WHISPER_SOURCE_DIR_DEFAULT}"
    CACHE PATH "Path to the whisper.cpp source tree used at build time")

set(ASRYX_SHERPA_SOURCE_DIR_DEFAULT "$ENV{HOME}/.local/opt/sherpa-onnx"
    CACHE PATH "Default path the installed asryx binary looks for the sherpa-onnx source tree")

set(ASRYX_SHERPA_SOURCE_DIR "${ASRYX_SHERPA_SOURCE_DIR_DEFAULT}"
    CACHE PATH "Path to the sherpa-onnx source tree used at build time")

if(NOT EXISTS "${ASRYX_WHISPER_SOURCE_DIR}/CMakeLists.txt")
  message(
    FATAL_ERROR
      "missing whisper.cpp checkout: ${ASRYX_WHISPER_SOURCE_DIR}. run ./scripts/install first."
  )
endif()

if(NOT EXISTS "${ASRYX_SHERPA_SOURCE_DIR}/CMakeLists.txt")
  message(
    FATAL_ERROR
      "missing sherpa-onnx checkout: ${ASRYX_SHERPA_SOURCE_DIR}. run ./scripts/install first."
  )
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WHISPER_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(WHISPER_ALL_WARNINGS_3RD_PARTY OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(WHISPER_CURL OFF CACHE BOOL "" FORCE)
set(WHISPER_SDL2 OFF CACHE BOOL "" FORCE)
set(WHISPER_FFMPEG OFF CACHE BOOL "" FORCE)

# Sherpa-onnx options: keep only the C API + speaker diarization, drop binaries,
# tests, TTS, websocket, portaudio, JNI, WASM. Build static for a self-contained
# asryx binary.
set(SHERPA_ONNX_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_CHECK OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_PORTAUDIO OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_JNI OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_C_API ON CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_CXX_API OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_WEBSOCKET OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_GPU OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_DIRECTML OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_WASM OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_BINARY OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_TTS OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_SPEAKER_DIARIZATION ON CACHE BOOL "" FORCE)
set(SHERPA_ONNX_BUILD_C_API_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SHERPA_ONNX_ENABLE_RKNN OFF CACHE BOOL "" FORCE)

add_subdirectory("${ASRYX_WHISPER_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/whisper.cpp" EXCLUDE_FROM_ALL)

# sherpa-onnx vendors deps (simple-sentencepiece, kaldi-decoder, etc.) that
# still reference std::result_of which was removed in C++20. Compile the whole
# sherpa subtree in C++17 and restore C++20 for asryx's own code afterwards.
set(_asryx_saved_cxx_std "${CMAKE_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD 17)
add_subdirectory("${ASRYX_SHERPA_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/sherpa-onnx" EXCLUDE_FROM_ALL)
set(CMAKE_CXX_STANDARD "${_asryx_saved_cxx_std}")

file(GLOB_RECURSE ASRYX_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ASRYX_HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.hpp")

if(APPLE)
  file(GLOB_RECURSE ASRYX_OBJCXX_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.mm")
  list(APPEND ASRYX_SOURCES ${ASRYX_OBJCXX_SOURCES})
endif()

add_executable(asryx ${ASRYX_SOURCES} ${ASRYX_HEADERS})
target_include_directories(asryx PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_definitions(
  asryx
  PRIVATE
    ASRYX_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    ASRYX_WHISPER_SOURCE_DIR_DEFAULT="${ASRYX_WHISPER_SOURCE_DIR_DEFAULT}"
    ASRYX_SHERPA_SOURCE_DIR_DEFAULT="${ASRYX_SHERPA_SOURCE_DIR_DEFAULT}"
)
target_include_directories(asryx PRIVATE "${ASRYX_SHERPA_SOURCE_DIR}")
target_link_libraries(asryx PRIVATE whisper sherpa-onnx-c-api)

if(APPLE)
  target_link_libraries(
    asryx
    PRIVATE
      "-framework Foundation"
      "-framework AVFoundation"
      "-framework CoreMedia"
      "-framework CoreAudio"
      "-framework CoreGraphics"
      "-framework AudioToolbox"
      "-framework ScreenCaptureKit"
  )
endif()

asryx_enable_warnings(asryx)
asryx_enable_hardening(asryx)
asryx_enable_sanitizers(asryx)

enable_testing()
add_subdirectory(tests)
