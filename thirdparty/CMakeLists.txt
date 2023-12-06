if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
  set(SOWA_THIRDPARTY_INCLUDES
      ${SOWA_THIRDPARTY_INCLUDES}
      "${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.3.8/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")

  set(SOWA_THIRDPARTY_SOURCES
      ${SOWA_THIRDPARTY_SOURCES}
      "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.2.2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/box2d-2.4.1/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6"
    "${CMAKE_CURRENT_SOURCE_DIR}/sol"
    "${CMAKE_CURRENT_SOURCE_DIR}/backward-cpp-1.6"
    )

if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
    set(SOWA_THIRDPARTY_INCLUDES
        ${SOWA_THIRDPARTY_INCLUDES}
        "${CMAKE_CURRENT_SOURCE_DIR}/openal-soft-1.23.1/include"
    )
endif()

set(SOWA_THIRDPARTY_INCLUDES ${SOWA_THIRDPARTY_INCLUDES} PARENT_SCOPE)

set(SOWA_THIRDPARTY_SOURCES
    ${SOWA_THIRDPARTY_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/backends/imgui_impl_opengl3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/backends/imgui_impl_glfw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.89.9-docking/imgui.cpp"

    # angelscript addons
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/add_on/scriptarray/scriptarray.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs/tinyfiledialogs.c"

    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lapi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lauxlib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lbaselib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lcode.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lcode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lcorolib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lctype.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lctype.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ldblib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ldebug.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ldo.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ldump.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lfunc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lfunc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lgc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lgc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/linit.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/liolib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/llex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lmathlib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/loadlib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lobject.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lobject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lopcodes.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lopcodes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/loslib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lparser.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lstate.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lstring.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lstrlib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ltable.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ltablib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ltests.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/ltm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lua.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/luaconf.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lundump.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lutf8lib.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lvm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/lzio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.4.6/onelua.c"
    PARENT_SCOPE)


set(BOX2D_BUILD_UNIT_TESTS
    OFF
    CACHE BOOL "")
set(BOX2D_BUILD_TESTBED
    OFF
    CACHE BOOL "")
set(BOX2D_BUILD_DOCS
    OFF
    CACHE BOOL "")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/eventpp-0.1.2" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.13.1" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lunasvg-2.3.8" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp-0.8.0" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/angelscript_2.36.1/sdk/angelscript/projects/cmake" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/fmt-10.1.1" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.2.2" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/box2d-2.4.1" EXCLUDE_FROM_ALL)

if(NOT ${TARGET_PLATFORM} STREQUAL "Web")
    set(ALSOFT_INSTALL_EXAMPLES
        OFF
        CACHE BOOL "")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/openal-soft-1.23.1" EXCLUDE_FROM_ALL)
endif()