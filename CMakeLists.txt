cmake_minimum_required(VERSION 3.0.0)
project(
  Sowa
  VERSION 1.0
  LANGUAGES CXX C)

file(GLOB_RECURSE SOWA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx")
set(SOWA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src/")

set(SOWA_THIRDPARTY_SOURCES "")
set(SOWA_THIRDPARTY_INCLUDES "")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty" EXCLUDE_FROM_ALL)

add_executable(sowa ${SOWA_SOURCES} ${SOWA_THIRDPARTY_SOURCES})
target_include_directories(
  sowa
  PUBLIC ${SOWA_INCLUDES}
  PRIVATE ${SOWA_THIRDPARTY_INCLUDES})

target_link_libraries(sowa PRIVATE glfw)