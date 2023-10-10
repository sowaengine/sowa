if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
  set(SOWA_THIRDPARTY_INCLUDES
      ${SOWA_THIRDPARTY_INCLUDES}
      "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")

  set(SOWA_THIRDPARTY_SOURCES ${SOWA_THIRDPARTY_SOURCES}
                              "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c"

                              # angelscript addons
                              "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
                              "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
                              )

  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_BUILD_DOCS OFF)
  set(GLFW_INSTALL OFF)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8" EXCLUDE_FROM_ALL)
endif()

set(SOWA_THIRDPARTY_INCLUDES
    ${SOWA_THIRDPARTY_INCLUDES}
    "${CMAKE_CURRENT_SOURCE_DIR}/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/tomlplusplus-3.3.0"
    "${CMAKE_CURRENT_SOURCE_DIR}/eventpp-0.1.2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/glm-0.9.9.8/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.13.1/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/lunasvg-2.3.8/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.8.0/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/angelscript/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on"
    "${CMAKE_CURRENT_SOURCE_DIR}/fmt-10.1.1/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/openal-soft-1.23.1/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.2.2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/box2d-2.4.1/include"
    
    PARENT_SCOPE
)


set(SOWA_THIRDPARTY_SOURCES
    ${SOWA_THIRDPARTY_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/backends/imgui_impl_opengl3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/backends/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui.cpp"

    PARENT_SCOPE)


set(ALSOFT_INSTALL_EXAMPLES OFF)

set(BOX2D_BUILD_UNIT_TESTS OFF)
set(BOX2D_BUILD_TESTBED OFF)
set(BOX2D_BUILD_DOCS OFF)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/eventpp-0.1.2" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.13.1" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lunasvg-2.3.8" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.8.0" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/angelscript/projects/cmake" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/fmt-10.1.1" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/openal-soft-1.23.1" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.2.2" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/box2d-2.4.1" EXCLUDE_FROM_ALL)