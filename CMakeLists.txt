cmake_minimum_required(VERSION 3.0.0)
project(Ease VERSION 0.1.0)

include(CTest)
enable_testing()


find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
link_libraries(${CMAKE_DL_LIBS} glfw raylib yaml-cpp spdlog box2d)

include_directories(
   ${OPENGL_INCLUDE_DIR}
   ${GLFW_INCLUDE_DIRS}
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking)

file(GLOB_RECURSE SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
   ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_subdirectory(modules)
add_subdirectory(thirdparty)



set(CMAKE_CXX_STANDARD 17)


add_executable(Ease ${SOURCES})



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
