file(GLOB LUA_SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.4/src/*.c)
# Remove lua entry point
list(REMOVE_ITEM LUA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.4/src/lua.c)
list(REMOVE_ITEM LUA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.4/src/luac.c)

set(SOWA_SOURCES
   ${SOWA_SOURCES}

   # imgui-docking
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/imgui_demo.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/imgui_draw.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/imgui_tables.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/imgui_widgets.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/imgui.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/misc/cpp/imgui_stdlib.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/backends/imgui_impl_glfw.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking/backends/imgui_impl_opengl3.cpp

   # lua-5.4.4
   ${LUA_SOURCES}


   #tinyfiledialogs
   ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs/tinyfiledialogs.c
   PARENT_SCOPE
)

set(SOWA_INCLUDE_DIRS
   ${SOWA_INCLUDE_DIRS}

   ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/nmGfx/src/
   ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/nmGfx/include/
   # ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/glfw-3.3.8/include/ is included for all targets
   ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/imgui-docking/
   ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/lua-5.4.4/src/
   ${CMAKE_SOURCE_DIR}/Sowa/thirdparty/sol2-3.3.0/
   ${CMAKE_SOURCE_DIR}/Sowa/include/tinyfiledialogs/

   PARENT_SCOPE
)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)

set(NMGFX_FIND_GLFW OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/box2d-2.4.1/ EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt-9.1.0/ EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8 EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nmGfx/ EXCLUDE_FROM_ALL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0/ EXCLUDE_FROM_ALL)