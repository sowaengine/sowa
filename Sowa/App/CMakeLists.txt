file(GLOB_RECURSE SOWA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(WIN32)
  set(SW_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/backends/win_main.cpp)
elseif(UNIX AND NOT APPLE)
  set(SW_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/backends/linux_main.cpp)
endif()

set(SOWA_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw-3.3.8/include/)

# thirdparty Modifies ${SOWA_SOURCES} and ${SOWA_INCLUDE_DIRS}
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
set(SOWA_INCLUDE_DIRS
    ${SOWA_INCLUDE_DIRS}
    PARENT_SCOPE)

set(LD_FLAG_STATIC "")

if(WIN32)
  set(LD_FLAG_STATIC -static)
endif()

add_library(sowa ${SOWA_SOURCES})
target_link_libraries(
  sowa
  PRIVATE nmGfx
          pugixml
          fmt
          -static-libgcc
          -static-libstdc++
          ${LD_FLAG_STATIC}
          ${CMAKE_DL_LIBS})
target_include_directories(sowa PRIVATE ${SOWA_INCLUDE_DIRS})
target_precompile_headers(
  sowa PRIVATE
  "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stlpch.hpp>")

add_executable(app ${SW_MAIN})
target_include_directories(
  app
  PRIVATE ${SOWA_INCLUDE_DIRS}
)
target_link_libraries(app PUBLIC sowa)

if(CMAKE_BUILD_TYPE MATCHES Release)
  target_compile_options(sowa PRIVATE -O2)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_options(sowa PRIVATE -O0 -g)
endif()

# Platform detection
if(WIN32)
  target_compile_definitions(sowa PRIVATE SW_WINDOWS)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(sowa PRIVATE SW_LINUX)
else()
  message(FATAL_ERROR "Sowa Engine is not supported on target platform "
                      ${CMAKE_SYSTEM})
endif()
