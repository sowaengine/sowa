set(EASE_SOURCES
   ${EASE_SOURCES}
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