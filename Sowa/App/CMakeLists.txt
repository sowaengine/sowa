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
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty EXCLUDE_FROM_ALL)
set(SOWA_INCLUDE_DIRS
    ${SOWA_INCLUDE_DIRS}
    PARENT_SCOPE)

set(LD_FLAG_STATIC "")

if(WIN32)
  set(LD_FLAG_STATIC -static)
endif()

add_library(lsowa ${SOWA_SOURCES})
target_link_libraries(
  lsowa
  PRIVATE nmGfx
          pugixml
          fmt
          -static-libgcc
          -static-libstdc++
          yaml-cpp
          ${LD_FLAG_STATIC}
          ${CMAKE_DL_LIBS})
target_include_directories(lsowa PRIVATE ${SOWA_INCLUDE_DIRS})
target_precompile_headers(
  lsowa PRIVATE
  "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/stlpch.hpp>")

add_executable(sowa ${SW_MAIN})
target_include_directories(
  sowa
  PRIVATE ${SOWA_INCLUDE_DIRS}
)
target_link_libraries(sowa PUBLIC lsowa)
set_target_properties(sowa
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../pkg/bin/main/"
    RUNTIME_OUTPUT_NAME "sowa-${CMAKE_PROJECT_VERSION}-${SOWA_VER_NUMBER}"
)

if(CMAKE_BUILD_TYPE MATCHES Release)
  target_compile_options(sowa PRIVATE -O2)
  target_compile_options(lsowa PRIVATE -O2)
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_options(sowa PRIVATE -O0 -g)
  target_compile_options(lsowa PRIVATE -O0 -g)
endif()

install(TARGETS sowa RUNTIME DESTINATION bin/main/)
install(FILES ${CMAKE_SOURCE_DIR}/runnerconfig.default.yml DESTINATION . RENAME runnerconfig.yml)

set(CPACK_PACKAGE_NAME "sowaengine")
set(CPACK_PACKAGE_VENDOR Lexographics)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION "Multiplatform 2D Game/Rendering Engine")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sowa Game Engine")
set(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}/.res/icon.png)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Sowa Engine")
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

set(CPACK_GENERATOR ZIP)

include(CPack)