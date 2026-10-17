#pragma once

#include <cstddef>
#include <list>
#include <string>
#include <string_view>
#include <vector>

#include <sol/sol.hpp>

#include "core/command_inputs.hpp"
#include "core/commands.hpp"
#include "core/reflection/field_type.hpp"
#include "core/store.hpp"
#include "core/timer.hpp"
#include "core/tween.hpp"
#include "core/types.hpp"
#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"
#include "ecs/system.hpp"

namespace sowa {

class AudioMixer;
class FileSystem;
class Input;
class LuaDebugger;
class PhysicsWorld;
class ProjectSettings;
class ResourceRegistry;
class SceneManager;
class World;
struct Collision2D;

// Owns Lua bindings and component lifecycle hooks. Must be destroyed before World.
class ScriptServer {
  public:
	explicit ScriptServer(World &world);
	~ScriptServer();

	ScriptServer(const ScriptServer &) = delete;
	ScriptServer &operator=(const ScriptServer &) = delete;
	ScriptServer(ScriptServer &&) = delete;
	ScriptServer &operator=(ScriptServer &&) = delete;

	bool DoString(std::string_view code, std::string_view chunkName = "string");
	bool DoFile(FileSystem &fs, std::string_view path);

	// Runs each path once (Init), then SyncLifecycles. Used for
	// Application.Scripts. Missing files are errors.
	bool LoadModules(FileSystem &fs, const std::vector<std::string> &paths);

	// Re-runs the last LoadModules list: package.loaded is replaced, method
	// tables are patched, field layout changes migrate live entities
	// (including disabled). Lua systems are destroyed and recreated. No-op
	// if LoadModules was never called.
	bool ReloadModules(FileSystem &fs);
	// Drain subscribed source changes at a frame boundary, outside Progress.
	// Reloads project entry points and their require graph once per batch.
	// No filesystem work is performed when no event is pending.
	bool FlushFileChanges();

	// Bytes currently allocated by the Lua state.
	std::size_t MemoryUsed() const;

	// Lua debugger; hooks are idle until breakpoints, stepping, or the pause watch are active.
	LuaDebugger &Debugger();
	// Replaces the configured project module list, then performs the same live
	// reload. Unlike the no-argument-list overload, an empty list explicitly
	// removes Lua systems and project-source ownership from the old list.
	bool ReloadModules(FileSystem &fs, const std::vector<std::string> &paths);

	// Tracks the Lua file declaring each component for inspector attachment. Files required
	// by project scripts also count as project sources, avoiding redundant startup entries.
	std::vector<ComponentType> ScriptComponentTypes(std::string_view path) const;
	std::vector<std::string> AttachedScriptPaths(Entity entity) const;
	bool AttachScript(Entity entity, std::string_view path);
	bool RemoveScript(Entity entity, std::string_view path);
	bool IsProjectScriptSource(std::string_view path) const;

	void SetEntity(const std::string &name, Entity entity);

	void BindEngine(Input &input, SceneManager &scenes, ResourceRegistry &resources,
		const ProjectSettings &settings, AudioMixer *mixer = nullptr,
		PhysicsWorld *physics = nullptr, StringStore *globalStore = nullptr);
	// Refreshes the value-copy exposed as Lua's global settings table.
	void UpdateSettings(const ProjectSettings &settings);

	bool CallComponentMethod(Entity entity, const std::string &component,
		const std::string &method);
	bool Call(std::string_view functionName);
	bool Call(std::string_view functionName, f32 arg);

	World &GetWorld() {
		return _world;
	}

	const World &GetWorld() const {
		return _world;
	}

	sol::state &Lua();
	CommandRegistry &Commands();
	sowa::CommandInputs &CommandInputs();

	// Timers and tweens live in lists so Lua userdata can hold a pointer
	// across create. UpdateTimers / UpdateTweens are what Application calls
	// each frame.
	Timer *NewTimer(f32 timeout, bool autoStart = false, bool repeating = false);
	void UpdateTimers(f32 delta);
	void ClearTimers();
	std::list<Timer> &Timers();

	Tween *NewTween(f32 duration, bool autoStart = true);
	void UpdateTweens(f32 delta);
	void ClearTweens();
	std::list<Tween> &Tweens();

	sol::object DefineComponent(const std::string &name, sol::optional<sol::table> fields,
		sol::this_state s);
	void FinishSystem(const std::string &name, const std::vector<std::string> &with,
		const std::vector<std::string> &without, bool always, sol::protected_function fn);
	void SyncLifecycles();

	sol::object IndexComponent(Entity entity, ComponentType type, std::string_view key,
		sol::this_state s);
	void NewIndexComponent(Entity entity, ComponentType type, std::string_view key,
		sol::object value);
	void ApplyDefaults(Entity entity, ComponentType type);

  private:
	struct Impl;

	void attachDebugger();
	bool runChunk(std::string_view code, std::string_view chunkName, std::string_view displayName);
	bool call(std::string_view functionName, const f32 *arg);
	bool reloadConfiguredModules(FileSystem &fs);
	void bindFileEvents(FileSystem &fs);
	sol::object requireModule(const std::string &name);
	void clearLoaded();
	void dropLuaSystems();
	void destroyLuaSystem(const std::string &name);
	void rememberLuaSystem(const std::string &name, System system);
	void bindStartSystem(const std::string &typeName, ComponentType type);
	bool startInstance(const std::string &typeName, ComponentType type, Entity entity);
	void bindUpdateSystem(const std::string &typeName, ComponentType type);
	void bindFixedUpdateSystem(const std::string &typeName, ComponentType type);
	void bindExitObserver(const std::string &typeName, ComponentType type);
	sol::table extrasTable(Entity entity, ComponentType type, bool create);
	void clearExtras(Entity entity, ComponentType type);
	bool callHook(const std::string &typeName, const char *hook, Entity entity, ComponentType type,
		const f32 *dt);
	void dispatchCollision(const Collision2D &event);
	void callCollisionHook(const std::string &typeName, const char *hook, Entity entity,
		ComponentType type, Entity other);
	void recordScriptComponent(const std::string &typeName);

	World &_world;
	Scope<Impl> _impl;
};

} // namespace sowa
