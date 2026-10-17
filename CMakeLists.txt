cmake_minimum_required(VERSION 3.25)

project(
  SowaEngine
  VERSION 0.1.0
  LANGUAGES CXX C)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# So clangd (and other tooling) can resolve include paths/defines. See .clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EMSCRIPTEN)
  set(SOWA_PLATFORM "Web")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SOWA_PLATFORM "Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(SOWA_PLATFORM "Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(SOWA_PLATFORM "macOS")
else()
  message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()
message(STATUS "SowaEngine target platform: ${SOWA_PLATFORM}")

if(APPLE AND NOT EMSCRIPTEN)
  enable_language(OBJC OBJCXX)
endif()

# Editor build, or an export template that only runs a packaged game (like
# Godot's templates). Web only ships as an export template.
option(SOWA_ENABLE_EDITOR "Build the editor instead of an export template" ON)
if(EMSCRIPTEN AND SOWA_ENABLE_EDITOR)
  message(FATAL_ERROR "The editor cannot be built for Web; use -DSOWA_ENABLE_EDITOR=OFF")
endif()
# SOWA_PROFILE_* macros compile to nothing when profiling is disabled.
# Enabled by default for editor builds; opt in for export templates.
option(SOWA_ENABLE_PROFILER "Include the frame profiler" ${SOWA_ENABLE_EDITOR})
# Loopback DAP server for VS Code. Enabled by default for editor builds and disabled
# for Web. The Lua debugger itself is built independently of the adapter.
option(SOWA_ENABLE_DEBUGGER "Include the Lua debug adapter (VS Code)" ${SOWA_ENABLE_EDITOR})
if(EMSCRIPTEN)
  set(SOWA_ENABLE_DEBUGGER OFF CACHE BOOL "" FORCE)
endif()
# The suite covers editor code, so it only exists in editor builds.
option(SOWA_BUILD_TESTS "Build the sowa_tests unit test suite (see tests/)" ON)

include(cmake/Dependencies.cmake)
include(cmake/Sources.cmake)

if(NOT EMSCRIPTEN)
  add_subdirectory(thirdparty/glad)
endif()

add_subdirectory(src)

if(SOWA_BUILD_TESTS AND SOWA_ENABLE_EDITOR)
  enable_testing()
  add_subdirectory(tests)
endif()
