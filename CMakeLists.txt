cmake_minimum_required(VERSION 3.0.0)
project(Ease VERSION 0.1.0)

include(CTest)
enable_testing()


link_directories(thirdparty/)
link_libraries(${CMAKE_DL_LIBS} raylib yaml-cpp box2d)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/raylib/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking)

add_definitions(-DEASE_EDITOR)

file(GLOB_RECURSE SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(modules)
add_subdirectory(thirdparty)





add_executable(Ease ${SOURCES})



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
