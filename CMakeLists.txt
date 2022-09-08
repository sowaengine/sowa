cmake_minimum_required(VERSION 3.0.0)
project(Ease VERSION 0.1.0 LANGUAGES CXX C)

option(Editor "Build Editor executable" ON)

include(CTest)
enable_testing()

file(GLOB_RECURSE EASE_SOURCES
   ${CMAKE_SOURCE_DIR}/Ease/src/*.cpp
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

link_directories(Ease/thirdparty/)
link_libraries(box2d)

if(Editor)
   add_compile_definitions(EASE_EDITOR)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/Modules)
add_subdirectory(${CMAKE_SOURCE_DIR}/Ease/thirdparty)

# Project Manager
add_subdirectory(${CMAKE_SOURCE_DIR}/ProjectManager)

# nmGfx
add_subdirectory(${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx)

add_executable(Ease ${EASE_SOURCES})

target_link_directories(Ease PRIVATE Ease/thirdparty/)
target_link_libraries(Ease PRIVATE ${CMAKE_DL_LIBS} yaml-cpp box2d openal sndfile nmGfx)

target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/src/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/thirdparty/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/thirdparty/glm/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/thirdparty/tinyfiledialogs/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/imgui-docking/)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx/src)
target_include_directories(Ease PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx/include/)

if(Editor)
   add_compile_definitions(EASE_EDITOR)
   set_target_properties(Ease PROPERTIES OUTPUT_NAME "Ease_Editor")
else()
   set_target_properties(Ease PROPERTIES OUTPUT_NAME "Easengine")
endif()


set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
