set(SOWA_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SOWA_BUILD_INFO_SRC "${SOWA_GENERATED_DIR}/core/build_info.cpp")

# Regenerated on every build (not just on reconfigure) so the embedded git
# hash/branch/dirty-flag never go stale. See cmake/GenerateBuildInfo.cmake.
add_custom_target(sowa_build_info
  COMMAND ${CMAKE_COMMAND}
    -D IN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/core/build_info.cpp.in
    -D OUT_FILE=${SOWA_BUILD_INFO_SRC}
    -D SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -D SOWA_VERSION=${PROJECT_VERSION}
    -P ${CMAKE_SOURCE_DIR}/cmake/GenerateBuildInfo.cmake
  BYPRODUCTS ${SOWA_BUILD_INFO_SRC}
  COMMENT "Generating build info (git hash/branch)"
)

add_library(sowa_assets STATIC)
target_compile_features(sowa_assets PRIVATE cxx_std_20)
target_include_directories(sowa_assets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

function(sowa_embed_asset symbol path)
  set(output "${SOWA_GENERATED_DIR}/assets/${symbol}.cpp")
  add_custom_command(OUTPUT "${output}"
    COMMAND ${CMAKE_COMMAND}
      "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/${path}"
      "-DOUTPUT=${output}" "-DSYMBOL=${symbol}"
      -P "${CMAKE_SOURCE_DIR}/cmake/EmbedFile.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${path}"
      "${CMAKE_SOURCE_DIR}/cmake/EmbedFile.cmake"
    VERBATIM)
  target_sources(sowa_assets PRIVATE "${output}")
endfunction()

sowa_embed_asset(DefaultFont assets/fonts/Roboto-Regular.ttf)
sowa_embed_asset(DefaultFontLicense assets/fonts/Roboto-Regular.LICENSE.txt)
sowa_embed_asset(SpriteShader assets/shaders/sprite2d.glsl)
sowa_embed_asset(FullscreenShader assets/shaders/fullscreen.glsl)
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/licenses")
configure_file("assets/fonts/Roboto-Regular.LICENSE.txt"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/licenses/Roboto-Regular.LICENSE.txt" COPYONLY)
if(SOWA_ENABLE_EDITOR)
  sowa_embed_asset(EditorIcons editor/fonts/fa-solid-900.ttf)
  sowa_embed_asset(EditorIconsLicense editor/fonts/fa-solid-900.LICENSE.txt)
  configure_file("editor/fonts/fa-solid-900.LICENSE.txt"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/licenses/fa-solid-900.LICENSE.txt" COPYONLY)
endif()

add_executable(sowa ${SOWA_RUNTIME_SOURCES} ${SOWA_APPLICATION_SOURCES})
if(SOWA_ENABLE_EDITOR)
  target_sources(sowa PRIVATE ${SOWA_TOOL_SOURCES} ${SOWA_EDITOR_SOURCES})
endif()
add_dependencies(sowa sowa_build_info)

if(SOWA_ENABLE_EDITOR AND APPLE)
  target_sources(sowa PRIVATE editor/native_titlebar.mm)
  set_source_files_properties(editor/native_titlebar.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_link_libraries(sowa PRIVATE "-framework Cocoa")
endif()

target_compile_features(sowa PRIVATE cxx_std_20)
target_include_directories(sowa PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/thirdparty/stb
  ${CMAKE_SOURCE_DIR}/thirdparty/dr_libs
  ${CMAKE_SOURCE_DIR}/thirdparty/miniaudio
)
target_link_libraries(sowa PRIVATE
  sowa_assets
  miniz
  glm::glm
  fmt::fmt
  glfw
  eventpp::eventpp
  yaml-cpp::yaml-cpp
  flecs::flecs_static
  sol2::sol2
  box2d::box2d
)
sowa_link_audio(sowa)

# Debug Adapter Protocol over TCP for VS Code.
if(SOWA_ENABLE_DEBUGGER)
  target_sources(sowa PRIVATE ${SOWA_DEBUGGER_SOURCES})
  target_compile_definitions(sowa PRIVATE SOWA_DEBUGGER=1)
  target_link_libraries(sowa PRIVATE nlohmann_json::nlohmann_json)
else()
  target_compile_definitions(sowa PRIVATE SOWA_DEBUGGER=0)
endif()

if(SOWA_ENABLE_PROFILER)
  target_compile_definitions(sowa PRIVATE SOWA_PROFILER=1)
else()
  target_compile_definitions(sowa PRIVATE SOWA_PROFILER=0)
endif()

# SOWA_EDITOR selects the build flavour: 1 is the editor (ImGui chrome,
# project manager, hot reload), 0 is the export template that only runs a
# packaged game.
if(SOWA_ENABLE_EDITOR)
  target_compile_definitions(sowa PRIVATE SOWA_EDITOR=1)
  target_link_libraries(sowa PRIVATE imgui_backends)
else()
  target_compile_definitions(sowa PRIVATE SOWA_EDITOR=0)
endif()

if(NOT EMSCRIPTEN)
  target_link_libraries(sowa PRIVATE glad uv_a)
endif()

if(EMSCRIPTEN)
  # main.cpp downloads game.zip at startup; heap growth covers larger games.
  set_target_properties(sowa PROPERTIES SUFFIX ".html")
  target_link_options(sowa PRIVATE
    -sALLOW_MEMORY_GROWTH=1
    -sSTACK_SIZE=1MB
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/platform/web/shell.html"
  )
  set_property(TARGET sowa APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/web/shell.html")
endif()
