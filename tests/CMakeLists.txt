# Engine tests share one executable, run through CTest.
add_executable(sowa_tests
  ${SOWA_RUNTIME_SOURCES}
  ${SOWA_TOOL_SOURCES}

  particles/test_particles2d.cpp
  particles/test_particle_rendering.cpp
  animation/test_animation.cpp
  animation/test_animated_sprite.cpp
  core/test_random.cpp
  core/test_id_generator.cpp
  core/test_string.cpp
  core/test_store.cpp
  core/test_scope_exit.cpp
  core/test_log.cpp
  core/test_game_time.cpp
  core/test_frame_clock.cpp
  core/test_profiler.cpp
  core/test_profiler_disabled.cpp
  core/test_profiler_instrumentation.cpp
  core/test_profiler_report.cpp
  core/test_timer.cpp
  core/test_tween.cpp
  core/test_input.cpp
  core/test_input_binding.cpp
  core/test_file_watcher.cpp
  core/test_filesystem.cpp
  core/test_zip.cpp
  core/test_arg_parser.cpp
  core/test_project_settings.cpp
  core/test_archive.cpp

  build/test_glob.cpp
  build/test_build_params.cpp
  build/test_build_runner.cpp
  build/test_icns.cpp

  ecs/test_tilemap.cpp
  scene/test_tilemap_scenes.cpp
  ecs/test_entity.cpp
  ecs/test_entity_pick_map.cpp
  ecs/test_entity_duplicate.cpp
  ecs/test_queue_destroy.cpp
  ecs/test_component_type.cpp
  ecs/test_component_ops.cpp
  ecs/test_field_reflection.cpp
  ecs/test_replace_fields.cpp
  ecs/test_query.cpp
  ecs/test_system.cpp
  ecs/test_hierarchy.cpp
  ecs/test_naming.cpp
  ecs/test_groups.cpp
  ecs/test_transform2d.cpp
  ecs/test_render_order.cpp
  ecs/test_enum_fields.cpp
  ecs/test_visibility.cpp
  ecs/test_camera2d.cpp
  ecs/test_component_serializer.cpp

  math/test_vector2.cpp
  math/test_vector3.cpp
  math/test_rect.cpp
  math/test_matrix.cpp
  math/test_math.cpp
  math/test_easing.cpp
  math/test_color.cpp

  scene/test_scene.cpp
  scene/test_scene_serializer.cpp
  scene/test_scene_snapshot.cpp
  scene/test_scene_manager.cpp
  scene/test_scene_pause.cpp
  scene/test_scene_play.cpp
  scene/test_packed_scene.cpp
  scene/test_hot_reload.cpp
  scene/test_example_project.cpp

  script/test_script_server.cpp
  script/test_lua_debugger.cpp
  script/test_script_suspension.cpp
  script/test_script_watchdog.cpp
  script/test_lua_ecs.cpp

  resource/test_resource_registry.cpp
  resource/test_resource_reflection.cpp
  resource/test_image_texture.cpp
  resource/test_font.cpp
  resource/test_audio_clip.cpp
  resource/test_sprite_sheet_animation.cpp

  audio/test_audio_device.cpp
  audio/test_audio_mixer.cpp

  physics/test_physics_world.cpp
  physics/test_rigid_body2d.cpp
  physics/test_collider2d.cpp
  physics/test_transform_sync.cpp
  physics/test_collision_events.cpp
  physics/test_debug_draw.cpp
  physics/test_physics_queries.cpp

  editor/test_command_palette.cpp
  editor/test_style.cpp
  editor/test_fonts.cpp
  editor/test_tilemap_palette.cpp
  editor/test_sprite_sheet_panel.cpp
  editor/test_viewport.cpp
  editor/test_camera.cpp
  editor/test_gizmos.cpp
  editor/test_nine_patch.cpp
  editor/test_entity_actions.cpp
  editor/test_project_manager.cpp
  editor/test_hierarchy.cpp
  editor/test_inspector.cpp
  editor/test_resource_inspector.cpp
  editor/test_filesystem_browser.cpp
  editor/test_console.cpp
  editor/test_project_data.cpp
  editor/test_project_settings.cpp
  editor/test_binding_capture.cpp
  editor/test_play_controls.cpp
  editor/test_profiler_panel.cpp
  editor/test_scene_documents.cpp

  render/test_text_draw.cpp
  render/test_text2d.cpp
  ui/test_ui.cpp
  render/test_shader.cpp
  render/test_builtin_assets.cpp
  render/test_viewport_scale.cpp
  render/test_image_patches.cpp
  render/test_sprite_renderer2d.cpp
  render/test_parallax2d.cpp
)

if(SOWA_ENABLE_DEBUGGER)
  target_sources(sowa_tests PRIVATE
    ${SOWA_DEBUGGER_SOURCES}
    script/test_dap_framing.cpp
    script/test_dap_session.cpp
    script/test_dap_server.cpp
  )
  target_link_libraries(sowa_tests PRIVATE nlohmann_json::nlohmann_json)
endif()
target_compile_features(sowa_tests PRIVATE cxx_std_20)
add_dependencies(sowa_tests sowa_build_info)
target_compile_definitions(sowa_tests PRIVATE
  SOWA_EDITOR=1
  SOWA_PROFILER=1
  SOWA_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
  SOWA_EXAMPLE_DIR="${CMAKE_SOURCE_DIR}/example"
  SOWA_ASSETS_DIR="${CMAKE_SOURCE_DIR}/src/assets"
)
target_include_directories(sowa_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/thirdparty/stb
  ${CMAKE_SOURCE_DIR}/thirdparty/dr_libs
  ${CMAKE_SOURCE_DIR}/thirdparty/miniaudio
)
target_link_libraries(sowa_tests PRIVATE
  sowa_assets
  miniz
  glm::glm
  fmt::fmt
  eventpp::eventpp
  yaml-cpp::yaml-cpp
  flecs::flecs_static
  sol2::sol2
  box2d::box2d
  imgui
  doctest::doctest_with_main
)
sowa_link_audio(sowa_tests)

add_test(NAME sowa_tests COMMAND sowa_tests)

if(NOT EMSCRIPTEN)
  target_link_libraries(sowa_tests PRIVATE uv_a)
endif()
