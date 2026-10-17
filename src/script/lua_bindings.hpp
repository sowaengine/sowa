#pragma once

#include <string>

#include <sol/sol.hpp>

#include "core/store.hpp"

namespace sowa {

class AudioMixer;
class Input;
class PhysicsWorld;
class ProjectSettings;
class ResourceRegistry;
class SceneManager;
class ScriptServer;
class World;

// Registers ECS, math, timers, and tweens, and sets the world global.
// Replaces Lua's math library with engine bindings.
void BindScriptApi(sol::state &lua, World &world, ScriptServer &scripts);

// Input, SceneManager, ResourceRegistry, log, settings, ResourceView.
// Pointers must outlive the lua_State. Called from ScriptServer::BindEngine.
void BindEngineApi(sol::state &lua, World &world, Input &input, SceneManager &scenes,
	ResourceRegistry &resources, const ProjectSettings &settings, AudioMixer *mixer = nullptr,
	PhysicsWorld *physics = nullptr, StringStore *globalStore = nullptr);

// Formats the Lua value at formatIndex with the values after it as
// arguments, using {} placeholders (the formatting behind log.info).
// Throws std::exception on a malformed format string.
std::string FormatLuaArgs(lua_State *L, int formatIndex);

// The global log table: log.info / log.warn / log.error.
void BindLog(sol::state &lua);

} // namespace sowa
