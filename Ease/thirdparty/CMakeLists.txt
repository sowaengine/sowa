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


   #tinyfiledialogs
   ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs/tinyfiledialogs.c
   PARENT_SCOPE
)

set(SOWA_INCLUDE_DIRS
   ${SOWA_INCLUDE_DIRS}

   ${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx/src/
   ${CMAKE_SOURCE_DIR}/Ease/thirdparty/nmGfx/include/
   ${CMAKE_SOURCE_DIR}/Ease/thirdparty/imgui-docking/
   ${CMAKE_SOURCE_DIR}/Ease/include/tinyfiledialogs/

   PARENT_SCOPE
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/box2d-2.4.1/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt-9.1.0/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nmGfx/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.7.0/)