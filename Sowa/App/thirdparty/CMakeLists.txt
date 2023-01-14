
set(SOWA_SOURCES
   ${SOWA_SOURCES}

   # lua-5.4.4
   ${LUA_SOURCES}


   #tinyfiledialogs
   ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs/tinyfiledialogs.c
   PARENT_SCOPE
)

set(SOWA_INCLUDE_DIRS
   ${SOWA_INCLUDE_DIRS}

   ${CMAKE_SOURCE_DIR}/Sowa/App/thirdparty/nmGfx/src/
   ${CMAKE_SOURCE_DIR}/Sowa/App/thirdparty/nmGfx/include/
   # ${CMAKE_SOURCE_DIR}/Sowa/App/thirdparty/glfw-3.3.8/include/ is included for all targets
   ${CMAKE_SOURCE_DIR}/Sowa/App/thirdparty/fmt-9.1.0/include/
   ${CMAKE_SOURCE_DIR}/Sowa/App/include/tinyfiledialogs/

   PARENT_SCOPE
)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)

set(NMGFX_FIND_GLFW OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt-9.1.0/ EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8 EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nmGfx/ EXCLUDE_FROM_ALL)