cmake_minimum_required(VERSION 3.0.0)
project(
  Sowa
  VERSION 1.0
  LANGUAGES CXX C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

message("Building in ${CMAKE_BUILD_TYPE}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fcolor-diagnostics)
endif()


set(LD_FLAG_STATIC "")
if(WIN32)
  link_libraries(
    -static-libgcc
    -static-libstdc++
  )
  set(LD_FLAG_STATIC -static)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(${CMAKE_SYSTEM_NAME})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(TARGET_PLATFORM "Linux")
  add_definitions(-DSW_LINUX)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(TARGET_PLATFORM "Windows")
  add_definitions(-DSW_WINDOWS)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
  set(TARGET_PLATFORM "Web")
  add_definitions(-DSW_WEB)

else()
  message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}")
endif()

file(GLOB_RECURSE SOWA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx")
file(GLOB_RECURSE SOWA_PROTO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.cc")
set(SOWA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src/")

set(SOWA_THIRDPARTY_SOURCES "")
set(SOWA_THIRDPARTY_INCLUDES "")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty" EXCLUDE_FROM_ALL)

add_library(thirdparty ${SOWA_THIRDPARTY_SOURCES})
target_include_directories(thirdparty PRIVATE ${SOWA_THIRDPARTY_INCLUDES})

add_executable(sowa ${SOWA_SOURCES} ${SOWA_PROTO_SOURCES})
target_include_directories(
  sowa
  PUBLIC ${SOWA_INCLUDES}
  PRIVATE ${SOWA_THIRDPARTY_INCLUDES})

target_compile_options(sowa PRIVATE "-Werror")
target_link_libraries(
  sowa
  PRIVATE thirdparty
          glfw
          eventpp::eventpp
          freetype
          lunasvg
          yaml-cpp
          angelscript
          fmt
          sndfile
          box2d
          protobuf)

if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
target_link_libraries(
  sowa
  PRIVATE
    OpenAL
)
endif()
target_precompile_headers(
  sowa
  PRIVATE
  <algorithm>
  <any>
  <cmath>
  <codecvt>
  <cstdint>
  <cstring>
  <ctime>
  <deque>
  <filesystem>
  <fstream>
  <iomanip>
  <iostream>
  <locale>
  <memory.h>
  <memory>
  <sstream>
  <stdexcept>
  <stdio.h>
  <string>
  <vector>)

if(WIN32)
  target_link_options(sowa PRIVATE -mwindows)
endif()

if(${TARGET_PLATFORM} STREQUAL "Web")
  set_target_properties(
    sowa
    PROPERTIES
      OUTPUT_NAME "index"
      SUFFIX ".html"
      LINK_FLAGS
      " --bind -sUSE_GLFW=3 -sWASM=1 -sFULL_ES3=1 -sEXPORTED_FUNCTIONS=_main,_Unload -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString --shell-file ${CMAKE_SOURCE_DIR}/layout.html -lidbfs.js -sASYNCIFY=1 -sASSERTIONS -sASYNCIFY_STACK_SIZE=16384"
  )
endif()

if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
  install(TARGETS sowa RUNTIME DESTINATION .)
else()
  install(FILES "$<TARGET_FILE_DIR:sowa>/index.html"
                "$<TARGET_FILE_DIR:sowa>/index.wasm"
                "$<TARGET_FILE_DIR:sowa>/index.js" DESTINATION .)
endif()

set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/package)
set(CPACK_PACKAGE_NAME sowaengine)

set(CPACK_PACKAGE_VENDOR Lexographics)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION "Multiplatform 2D Game engine")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sowa Game Engine")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/.res/icon.png")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

if(UNIX)
  set(CPACK_GENERATOR TGZ)
elseif(WIN32)
  set(CPACK_GENERATOR ZIP)
endif()

include(CPack)
