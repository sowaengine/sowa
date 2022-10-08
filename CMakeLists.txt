cmake_minimum_required(VERSION 3.0.0)
project(Ease VERSION 0.1.0 LANGUAGES CXX C)

option(Editor "Build Editor executable" ON)

include(CTest)
enable_testing()

file(GLOB_RECURSE EASE_SOURCES
   ${CMAKE_SOURCE_DIR}/Ease/src/*.cpp
)
message("Excluding " ${CMAKE_SOURCE_DIR}/Ease/src/main.cpp)
list(REMOVE_ITEM EASE_SOURCES ${CMAKE_SOURCE_DIR}/Ease/src/main.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)


if(Editor)
   add_compile_definitions(EASE_EDITOR)
endif()

# thirdparty
add_subdirectory(${CMAKE_SOURCE_DIR}/Ease/thirdparty)

# nmGfx
add_subdirectory(${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx)


add_executable(Sowa
   ${CMAKE_SOURCE_DIR}/Ease/src/main.cpp
   ${EASE_SOURCES})
target_link_libraries(Sowa PRIVATE ${CMAKE_DL_LIBS} yaml-cpp box2d nmGfx -static-libgcc -static-libstdc++ fmt angelscript)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/thirdparty/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/include/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx/include/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/angelscript/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/imgui-docking/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/include/thirdparty/tinyfiledialogs/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx/src/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/angelscript_2.36.0/sdk/angelscript/include/)
target_include_directories(Sowa PRIVATE ${CMAKE_SOURCE_DIR}/Ease/thirdparty/angelscript_2.36.0/sdk/)


if(Editor)
   add_compile_definitions(EASE_EDITOR)
   set_target_properties(Sowa PROPERTIES OUTPUT_NAME "Sowa Editor")
else()
   set_target_properties(Sowa PROPERTIES OUTPUT_NAME "Sowa Engine")
endif()


set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
