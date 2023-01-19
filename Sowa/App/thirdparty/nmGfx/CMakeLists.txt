cmake_minimum_required(VERSION 3.0.0)
project(nmGfx VERSION 0.1.0)

include(CTest)
enable_testing()

if(${NMGFX_FIND_GLFW})
find_package(glfw3 REQUIRED)
endif()

# nmGfx
file(GLOB_RECURSE NMGFX_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
set(NMGFX_SOURCES
    ${NMGFX_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad.c
)

add_library(nmGfx ${NMGFX_SOURCES})
target_include_directories(nmGfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nmGfx PUBLIC glfw)
target_link_libraries(nmGfx PRIVATE -static-libstdc++ -static-libgcc)


# Playground
if(NMGFX_BUILD_PLAYGROUND)
    add_executable(Playground src/main.cpp)
    target_link_libraries(Playground nmGfx)
    target_compile_definitions(Playground PUBLIC NMGFX_BUILD_PLAYGROUND)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
