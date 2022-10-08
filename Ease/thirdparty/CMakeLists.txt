set(ANGELSCRIPT_ADDON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.0/sdk/add_on/)

set(EASE_SOURCES
   ${EASE_SOURCES}

   # angelscript
   ${ANGELSCRIPT_ADDON_DIR}/scriptarray/scriptarray.cpp
   ${ANGELSCRIPT_ADDON_DIR}/scriptbuilder/scriptbuilder.cpp
   ${ANGELSCRIPT_ADDON_DIR}/scriptstdstring/scriptstdstring.cpp
   ${ANGELSCRIPT_ADDON_DIR}/scriptstdstring/scriptstdstring_utils.cpp

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

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.0/sdk/angelscript/projects/cmake/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt-9.1.0/)