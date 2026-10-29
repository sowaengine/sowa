include(FetchContent)

set(SOWA_DEPS_DIR "${CMAKE_SOURCE_DIR}/build/_deps" CACHE PATH
  "Third-party source checkouts shared by all build directories")

function(sowa_declare name)
  cmake_parse_arguments(arg "" "GIT_TAG;GIT_SHALLOW" "" ${ARGN})
  if(NOT DEFINED arg_GIT_SHALLOW)
    set(arg_GIT_SHALLOW TRUE)
  endif()
  set(source "${SOWA_DEPS_DIR}/${name}-${arg_GIT_TAG}")
  if(EXISTS "${source}")
    string(TOUPPER "${name}" upper)
    set(FETCHCONTENT_SOURCE_DIR_${upper} "${source}" PARENT_SCOPE)
  endif()
  FetchContent_Declare(${name} ${arg_UNPARSED_ARGUMENTS}
    GIT_TAG "${arg_GIT_TAG}"
    SOURCE_DIR "${source}"
    GIT_SHALLOW "${arg_GIT_SHALLOW}"
    GIT_PROGRESS TRUE
  )
endfunction()

if(NOT EMSCRIPTEN)
  set(LIBUV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_BENCH OFF CACHE BOOL "" FORCE)
  set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  sowa_declare(libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG v1.52.1
  )
  FetchContent_MakeAvailable(libuv)
endif()

if(SOWA_ENABLE_DEBUGGER)
  set(JSON_BuildTests OFF CACHE INTERNAL "")
  set(JSON_Install OFF CACHE INTERNAL "")
  sowa_declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

function(sowa_add_miniz)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_EXAMPLES OFF)
  set(BUILD_TESTS OFF)
  set(INSTALL_PROJECT OFF)
  sowa_declare(miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.1.0
  )
  FetchContent_MakeAvailable(miniz)
  target_compile_definitions(miniz PRIVATE MINIZ_NO_ZLIB_COMPATIBLE_NAMES)
endfunction()
sowa_add_miniz()

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

sowa_declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_MakeAvailable(glm)
unset(CMAKE_POLICY_VERSION_MINIMUM)

set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)

sowa_declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        12.2.0
)
FetchContent_MakeAvailable(fmt)

if(EMSCRIPTEN)
  add_library(glfw INTERFACE)
  target_link_options(glfw INTERFACE -sUSE_GLFW=3 -sFULL_ES3=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
else()
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

  sowa_declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
  )
  FetchContent_MakeAvailable(glfw)
endif()

if(SOWA_ENABLE_EDITOR)
  sowa_declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9-docking
  )
  FetchContent_GetProperties(imgui)
  if(NOT imgui_POPULATED)
    if(POLICY CMP0169)
      cmake_policy(PUSH)
      cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(imgui)
    if(POLICY CMP0169)
      cmake_policy(POP)
    endif()
  endif()

  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  )
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
  set_target_properties(imgui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  add_library(imgui_backends STATIC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui_backends PUBLIC ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui_backends PUBLIC imgui glfw)
  target_compile_definitions(imgui_backends PRIVATE GLFW_INCLUDE_NONE)
  if(EMSCRIPTEN)
    target_compile_definitions(imgui_backends PUBLIC IMGUI_IMPL_OPENGL_ES3)
  endif()
  set_target_properties(imgui_backends PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )
endif()

sowa_declare(eventpp
  GIT_REPOSITORY https://github.com/wqking/eventpp.git
  GIT_TAG        v0.1.3
)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_MakeAvailable(eventpp)
unset(CMAKE_POLICY_VERSION_MINIMUM)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)

sowa_declare(yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_MakeAvailable(yaml-cpp)
unset(CMAKE_POLICY_VERSION_MINIMUM)

set(FLECS_STATIC ON CACHE BOOL "" FORCE)
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
set(FLECS_TESTS OFF CACHE BOOL "" FORCE)

sowa_declare(flecs
  GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
  GIT_TAG        v4.1.6
)
FetchContent_MakeAvailable(flecs)

target_compile_definitions(flecs_static PUBLIC
  FLECS_CUSTOM_BUILD
  FLECS_CPP
  FLECS_MODULE
  FLECS_META
  FLECS_SYSTEM
  FLECS_PIPELINE
)
if(SOWA_ENABLE_PROFILER)
  target_compile_definitions(flecs_static PUBLIC FLECS_PERF_TRACE)
endif()

sowa_declare(lua
  GIT_REPOSITORY https://github.com/lua/lua.git
  GIT_TAG        v5.4.7
)
FetchContent_MakeAvailable(lua)

set(LUA_C_SOURCES
  ${lua_SOURCE_DIR}/lapi.c
  ${lua_SOURCE_DIR}/lauxlib.c
  ${lua_SOURCE_DIR}/lbaselib.c
  ${lua_SOURCE_DIR}/lcode.c
  ${lua_SOURCE_DIR}/lcorolib.c
  ${lua_SOURCE_DIR}/lctype.c
  ${lua_SOURCE_DIR}/ldblib.c
  ${lua_SOURCE_DIR}/ldebug.c
  ${lua_SOURCE_DIR}/ldo.c
  ${lua_SOURCE_DIR}/ldump.c
  ${lua_SOURCE_DIR}/lfunc.c
  ${lua_SOURCE_DIR}/lgc.c
  ${lua_SOURCE_DIR}/linit.c
  ${lua_SOURCE_DIR}/liolib.c
  ${lua_SOURCE_DIR}/llex.c
  ${lua_SOURCE_DIR}/lmathlib.c
  ${lua_SOURCE_DIR}/lmem.c
  ${lua_SOURCE_DIR}/loadlib.c
  ${lua_SOURCE_DIR}/lobject.c
  ${lua_SOURCE_DIR}/lopcodes.c
  ${lua_SOURCE_DIR}/loslib.c
  ${lua_SOURCE_DIR}/lparser.c
  ${lua_SOURCE_DIR}/lstate.c
  ${lua_SOURCE_DIR}/lstring.c
  ${lua_SOURCE_DIR}/lstrlib.c
  ${lua_SOURCE_DIR}/ltable.c
  ${lua_SOURCE_DIR}/ltablib.c
  ${lua_SOURCE_DIR}/ltm.c
  ${lua_SOURCE_DIR}/lundump.c
  ${lua_SOURCE_DIR}/lutf8lib.c
  ${lua_SOURCE_DIR}/lvm.c
  ${lua_SOURCE_DIR}/lzio.c
)
add_library(lua_static STATIC ${LUA_C_SOURCES})
target_include_directories(lua_static PUBLIC ${lua_SOURCE_DIR})
set_target_properties(lua_static PROPERTIES C_STANDARD 99)
if(UNIX)
  target_compile_definitions(lua_static PRIVATE LUA_USE_POSIX)
endif()
if(NOT WIN32)
  target_link_libraries(lua_static PUBLIC m)
endif()

sowa_declare(sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG        d805d027e0a0a7222e936926139f06e23828ce9f
  GIT_SHALLOW    FALSE
)
FetchContent_GetProperties(sol2)
if(NOT sol2_POPULATED)
  if(POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
  endif()
  FetchContent_Populate(sol2)
  if(POLICY CMP0169)
    cmake_policy(POP)
  endif()
endif()
add_library(sol2 INTERFACE)
add_library(sol2::sol2 ALIAS sol2)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # GCC's pretty names can collide for lambdas, causing sol2 to use the wrong
  # destructor. Use unique RTTI names for metatable identity.
  file(READ "${sol2_SOURCE_DIR}/include/sol/usertype_traits.hpp" sol2_traits)
  string(REPLACE "#include <sol/demangle.hpp>"
    "#include <sol/demangle.hpp>\n#include <typeinfo>" sol2_traits "${sol2_traits}")
  string(REPLACE ".append(detail::demangle<T>())" ".append(typeid(T).name())"
    sol2_unique_traits "${sol2_traits}")
  if(sol2_traits STREQUAL sol2_unique_traits)
    message(FATAL_ERROR "The sol2 GCC metatable compatibility patch needs updating")
  endif()
  set(sol2_compat_include "${CMAKE_BINARY_DIR}/generated/sol2-compat")
  file(MAKE_DIRECTORY "${sol2_compat_include}/sol")
  file(CONFIGURE OUTPUT "${sol2_compat_include}/sol/usertype_traits.hpp"
    CONTENT "${sol2_unique_traits}" @ONLY)
  target_include_directories(sol2 INTERFACE "${sol2_compat_include}")
endif()
target_include_directories(sol2 INTERFACE ${sol2_SOURCE_DIR}/include)
target_link_libraries(sol2 INTERFACE lua_static)
target_compile_definitions(sol2 INTERFACE SOL_ALL_SAFETIES_ON=1)

sowa_declare(box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG        v3.1.1
)
FetchContent_MakeAvailable(box2d)

function(sowa_link_audio target)
  if(APPLE)
    target_link_libraries(${target} PRIVATE
      "-framework CoreFoundation"
      "-framework CoreAudio"
      "-framework AudioToolbox"
      "-framework AudioUnit"
    )
  elseif(UNIX AND NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    target_link_libraries(${target} PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
  endif()
endfunction()

if(SOWA_BUILD_TESTS)
  set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

  sowa_declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
  )

  set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
  FetchContent_MakeAvailable(doctest)
  unset(CMAKE_POLICY_VERSION_MINIMUM)
endif()
