cmake_minimum_required(VERSION 3.0.0)
project(Ease VERSION 0.1.0 LANGUAGES CXX C)

option(Editor "Build Editor executable" ON)

include(CTest)
enable_testing()

file(GLOB_RECURSE SOWA_SOURCES
   ${CMAKE_SOURCE_DIR}/Ease/src/*.cpp
)
list(REMOVE_ITEM SOWA_SOURCES ${CMAKE_SOURCE_DIR}/Ease/src/main.cpp)

if(CMAKE_BUILD_TYPE MATCHES "")
   set(CMAKE_BUILD_TYPE Debug)
   message("No CMAKE_BUILD_TYPE specified. Selected 'Debug'")
   message("Possible values: Release | Debug")
endif()

set(SOWA_INCLUDE_DIRS
   ${CMAKE_SOURCE_DIR}/Ease/include/
   ${CMAKE_SOURCE_DIR}/Ease/src/
   ${CMAKE_SOURCE_DIR}/Ease/thirdparty/
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


if(Editor)
   add_compile_definitions(EASE_EDITOR)
endif()


include_directories(${CMAKE_SOURCE_DIR}/Ease/thirdparty/glfw-3.3.8/include/)

# thirdparty
# Modifies ${SOWA_SOURCES} and ${SOWA_INCLUDE_DIRS}
add_subdirectory(${CMAKE_SOURCE_DIR}/Ease/thirdparty)


add_executable(Sowa
   ${CMAKE_SOURCE_DIR}/Ease/src/main.cpp
   ${SOWA_SOURCES})
target_link_libraries(Sowa PRIVATE ${CMAKE_DL_LIBS} yaml-cpp box2d nmGfx -static-libgcc -static-libstdc++ fmt)
# target_compile_options(Sowa PRIVATE -Wall -Wextra -pedantic -Wshadow -Wconversion -Wuninitialized)
target_include_directories(Sowa PRIVATE ${SOWA_INCLUDE_DIRS})

if(CMAKE_BUILD_TYPE MATCHES Release)
   target_compile_options(Sowa PRIVATE -O2)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
   target_compile_options(Sowa PRIVATE -O0 -g)
endif()

if(Editor)
   target_compile_definitions(Sowa PRIVATE EASE_EDITOR)
   set_target_properties(Sowa PROPERTIES OUTPUT_NAME "Sowa Editor")
else()
   set_target_properties(Sowa PROPERTIES OUTPUT_NAME "Sowa Engine")
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
