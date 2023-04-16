cmake_minimum_required(VERSION 3.0.0)
project(
  Sowa
  VERSION 0.1.1
  LANGUAGES CXX C)

set(tag_a      0)
set(tag_b      1)
set(tag_rc     2)
set(tag_stable 3)

set(SOWA_VER_MAJOR      0)  # max: 255
set(SOWA_VER_MINOR      1)  # max: 255
set(SOWA_VER_PATCH      1)  # max: 255
set(SOWA_VER_TAG        a)  # a: alpha, b: beta, rc, stable
set(SOWA_VER_TAG_REV    1)  # max: 15

# {major}.{minor}.{patch}-{tag}{rev}
set(CMAKE_PROJECT_VERSION "${SOWA_VER_MAJOR}.${SOWA_VER_MINOR}.${SOWA_VER_PATCH}-${SOWA_VER_TAG}${SOWA_VER_TAG_REV}")
if("${SOWA_VER_MAJOR}" STREQUAL "") 
  set(SOWA_VER_MAJOR 0)
endif()
if("${SOWA_VER_MINOR}" STREQUAL "") 
  set(SOWA_VER_MINOR 0)
endif()
if("${SOWA_VER_PATCH}" STREQUAL "") 
  set(SOWA_VER_PATCH 0)
endif()
if("${SOWA_VER_TAG}" STREQUAL "") 
  set(SOWA_VER_TAG 0)
endif()
if("${SOWA_VER_TAG_REV}" STREQUAL "") 
  set(SOWA_VER_TAG_REV 0)
endif()
#   MAJOR       MINOR       PATCH      TAG REV
# 0000'0000   0000'0000   0000'0000   0000'0000
math(EXPR SOWA_VER_NUMBER "
      (
        ((${SOWA_VER_MAJOR} % 256) << 24) |
        ((${SOWA_VER_MINOR} % 256) << 16) |
        ((${SOWA_VER_PATCH} % 256) << 8) |
        (0x1 << (4 + ${tag_${SOWA_VER_TAG}} % 4)) |
        (${SOWA_VER_TAG_REV} % 16 ) | 0x00
      )"
)
add_definitions(-DSOWA_VERSION_MAJOR=${SOWA_VER_MAJOR})
add_definitions(-DSOWA_VERSION_MINOR=${SOWA_VER_MINOR})
add_definitions(-DSOWA_VERSION_PATCH=${SOWA_VER_PATCH})
add_definitions(-DSOWA_VERSION_TAG="${SOWA_VER_TAG}")
add_definitions(-DSOWA_VERSION_TAG_REV=${SOWA_VER_TAG_REV})
add_definitions(-DSOWA_VERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DSOWA_VERSION_NUMBER=${SOWA_VER_NUMBER})

if(CMAKE_BUILD_TYPE STREQUAL "") 
  set(CMAKE_BUILD_TYPE Debug)
  message("No CMAKE_BUILD_TYPE specified. Selected 'Debug'")
  message("Possible values: Release | Debug")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Platform detection
if(WIN32)
  add_definitions(-DSW_WINDOWS)
elseif(UNIX AND NOT APPLE)
  add_definitions(-DSW_LINUX)
else()
  message(FATAL_ERROR "Sowa Engine is not supported on target platform "
                      ${CMAKE_SYSTEM})
endif()

set(BUILD_SHARED_LIBS OFF)
set(BUILD_DOC OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_REGRESS OFF)
set(BUILD_TESTING OFF)
set(BUILD_TOOLS OFF)
set(YAML_BUILD_SHARED_LIBS OFF)

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0063 NEW)
cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty EXCLUDE_FROM_ALL)

include_directories(${CMAKE_SOURCE_DIR}/libsw/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/Sowa)
add_subdirectory(${CMAKE_SOURCE_DIR}/runner)

# libsw tests
file(GLOB_RECURSE TEST_SOURCES ${CMAKE_SOURCE_DIR}/libsw/tests/*.cpp)

add_subdirectory(googletest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(sw_test ${TEST_SOURCES})
target_link_libraries(sw_test gtest)
add_test(sw_test sw_test)
add_custom_target(run_unit_test ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS sw_test)