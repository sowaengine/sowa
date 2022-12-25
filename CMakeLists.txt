cmake_minimum_required(VERSION 3.0.0)
project(Sowa VERSION 0.1.1 LANGUAGES CXX C)

option(Editor "Build Editor executable" ON)

include(CTest)
enable_testing()

file(GLOB_RECURSE SOWA_SOURCES
   ${CMAKE_SOURCE_DIR}/Sowa/src/*.cpp
)
list(REMOVE_ITEM SOWA_SOURCES ${CMAKE_SOURCE_DIR}/Sowa/src/main.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "")
   set(CMAKE_BUILD_TYPE Debug)
   message("No CMAKE_BUILD_TYPE specified. Selected 'Debug'")
   message("Possible values: Release | Debug")
endif()

set(SOWA_INCLUDE_DIRS
   ${CMAKE_SOURCE_DIR}/Sowa/include/
   ${CMAKE_SOURCE_DIR}/Sowa/src/
   ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


include_directories(${CMAKE_SOURCE_DIR}/Sowa/thirdparty/glfw-3.3.8/include/)

# thirdparty
# Modifies ${SOWA_SOURCES} and ${SOWA_INCLUDE_DIRS}
add_subdirectory(${CMAKE_SOURCE_DIR}/Sowa/thirdparty)

set(LD_FLAG_STATIC "")
if(WIN32)
   set(LD_FLAG_STATIC -static)
endif()


add_executable(engine
   ${CMAKE_SOURCE_DIR}/Sowa/src/main.cpp
   ${SOWA_SOURCES})
target_link_libraries(engine PRIVATE ${CMAKE_DL_LIBS} yaml-cpp box2d nmGfx -static-libgcc -static-libstdc++ ${LD_FLAG_STATIC} fmt)
# target_compile_options(engine PRIVATE -Wall -Wextra -pedantic -Wshadow -Wconversion -Wuninitialized)
target_include_directories(engine PRIVATE ${SOWA_INCLUDE_DIRS})
target_precompile_headers(engine PRIVATE
   "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/Sowa/src/stlpch.hpp>"
   "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/Sowa/thirdparty/sol2-3.3.0/sol/sol.hpp>")

if(CMAKE_BUILD_TYPE MATCHES Release)
   target_compile_options(engine PRIVATE -O2)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
   target_compile_options(engine PRIVATE -O0 -g)
endif()

if(Editor)
   target_compile_definitions(engine PRIVATE SW_EDITOR)
   set_target_properties(engine PROPERTIES OUTPUT_NAME "sowa.engine")
else()
   set_target_properties(engine PROPERTIES OUTPUT_NAME "sowa.template")
endif()


# Platform detection
if(WIN32)
   target_compile_definitions(engine PRIVATE SW_WINDOWS)
elseif(UNIX AND NOT APPLE)
   target_compile_definitions(engine PRIVATE SW_LINUX)
else()
   message(FATAL_ERROR "Sowa Engine is not supported on target platform " ${CMAKE_SYSTEM})
endif()




install(TARGETS engine RUNTIME DESTINATION Editor/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Editor/ DESTINATION Editor/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/ProjectManager/ DESTINATION .)



set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/package)
set(CPACK_PACKAGE_NAME sowaengine)

set(CPACK_PACKAGE_VENDOR Lexographics)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION "Multiplatform 2D Game/Rendering Engine")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sowa Game Engine")
set(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}/res/icon.png)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sowa Engine")

if(UNIX)
   set(CPACK_GENERATOR TGZ)
elseif(WIN32 AND Editor)
   set(CPACK_GENERATOR ZIP)
elseif(WIN32 AND NOT Editor)   
   set(CPACK_GENERATOR ZIP)
endif()

include(CPack)
