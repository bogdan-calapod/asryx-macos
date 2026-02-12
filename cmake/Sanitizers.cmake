set(ASRYX_SANITIZER "" CACHE STRING "Sanitizer profile: address, undefined, or empty")
set_property(CACHE ASRYX_SANITIZER PROPERTY STRINGS "" "address" "undefined")

function(asryx_enable_sanitizers target)
  if(NOT ASRYX_SANITIZER)
    return()
  endif()

  if(MSVC)
    message(FATAL_ERROR "Sanitizer presets are configured for GCC/Clang on Linux.")
  endif()

  if(ASRYX_SANITIZER STREQUAL "address")
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address)
    return()
  endif()

  if(ASRYX_SANITIZER STREQUAL "undefined")
    target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=undefined)
    return()
  endif()

  message(FATAL_ERROR "Unknown sanitizer profile: ${ASRYX_SANITIZER}")
endfunction()
