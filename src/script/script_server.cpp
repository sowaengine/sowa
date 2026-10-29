#include "script/script_server.hpp"
#include "script/lua_function.hpp"

#include <algorithm>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lauxlib.h>
#include <lua.h>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include "core/project_settings.hpp"
#include "core/reflection/field_ref.hpp"
#include "core/reflection/field_type.hpp"
#include "core/scope_exit.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/script_tags.hpp"
#include "ecs/world.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "physics/collision2d.hpp"
#include "physics/physics_world.hpp"
#include "resource/resource_id.hpp"
#include "script/lua_bindings.hpp"
#include "script/lua_debugger.hpp"
#include "script/lua_editor.hpp"
#include "script/lua_types.hpp"

namespace sowa {

struct FieldTypeTag {
	FieldType type = FieldType::F32;
};

struct FieldDefault {
	std::string name;
	FieldType type = FieldType::F32;
	bool b = false;
	i32 i = 0;
	u32 u = 0;
	f32 f = 0.f;
	f64 d = 0.0;
	std::string s;
	Entity entity;
	ResourceID resource;
	Vector2 v2;
	Vector3 v3;
	Color color;
};

struct ScriptServer::Impl {
	sol::state lua;
	// After lua: removes its hooks and wrappers before the state closes.
	std::unique_ptr<LuaDebugger> debugger;
	CommandRegistry commands;
	sowa::CommandInputs commandInputs;
	std::unordered_map<std::string, sol::table> prototypes;
	std::unordered_map<std::string, std::vector<FieldDefault>> defaults;
	std::unordered_map<std::string, sol::table> extras;

	struct ReloadStage {
		std::vector<std::function<void()>> componentDefinitions;
		std::vector<std::function<void()>> registrations;
		std::list<Timer> timers;
		std::list<Tween> tweens;
		bool clearTimers = false;
		bool clearTweens = false;
		std::unordered_set<std::string> components;
		std::vector<std::string> queriedComponents;
	};

	std::unique_ptr<ReloadStage> reloadStage;
	std::vector<std::pair<std::string, System>> luaSystems;
	std::vector<std::string> modulePaths;
	std::vector<std::string> directPaths;
	std::shared_ptr<FileSystemEvents> fileEvents;
	FileSystemEvents::Paths::Handle fileChanged;
	std::unordered_set<std::string> scriptFiles;
	std::unordered_set<std::string> pendingFiles;
	std::unordered_map<std::string, std::string> sourceContents;
	std::vector<std::string> loading;
	std::vector<std::string> sourcePaths;
	std::map<std::string, std::vector<std::string>> sourceComponents;
	std::unordered_map<std::string, std::string> componentSources;
	std::unordered_set<std::string> projectSources;
	FileSystem *fileSystem = nullptr;
	bool migrating = false;
	bool loadingProjectModules = false;
	// After lua: destroyed first so on_timeout / on_update / on_finish
	// closures can still use the state.
	std::list<Timer> timers;
	std::list<Tween> tweens;
};

namespace {

// Restore tables in place so existing closures and instances keep their references.
class LuaTableSnapshot {
	struct Entry {
		sol::table table;
		sol::object metatable;
		std::vector<std::pair<sol::object, sol::object>> values;
	};

	std::unordered_set<const void *> _seen;
	std::vector<Entry> _entries;

  public:
	void Capture(sol::table table) {
		auto *L = table.lua_state();
		sol::stack::push(L, table);
		const void *identity = lua_topointer(L, -1);
		lua_pop(L, 1);
		if (!_seen.insert(identity).second) {
			return;
		}
		Entry entry{table, table[sol::metatable_key], {}};
		for (const auto &value : table) {
			entry.values.emplace_back(value.first, value.second);
		}
		for (const auto &[key, value] : entry.values) {
			if (key.get_type() == sol::type::table) {
				Capture(key.as<sol::table>());
			}
			if (value.get_type() == sol::type::table) {
				Capture(value.as<sol::table>());
			}
		}
		if (entry.metatable.get_type() == sol::type::table) {
			Capture(entry.metatable.as<sol::table>());
		}
		_entries.push_back(std::move(entry));
	}

	void Restore() {
		for (auto &entry : _entries) {
			std::vector<sol::object> keys;
			for (const auto &value : entry.table) {
				keys.push_back(value.first);
			}
			for (const auto &key : keys) {
				entry.table.raw_set(key, sol::lua_nil);
			}
			for (const auto &[key, value] : entry.values) {
				entry.table.raw_set(key, value);
			}
			entry.table[sol::metatable_key] = entry.metatable;
		}
	}
};

#if SOWA_PROFILER
// Event hooks (_exit, collisions) are not flecs systems, so they get their own
// scope, named like the _start/_update systems flecs reports ("Lua.Type.hook")
// so the profiler aggregates them per script type. Built only while recording.
ProfileName hookProfileName(const std::string &typeName, const char *hook) {
	Profiler &profiler = Profiler::Main();
	if (!profiler.IsRecording()) {
		return 0;
	}
	return profiler.Intern("Lua." + typeName + "." + hook);
}
#endif

std::string ExtraKey(Entity entity, ComponentType type) {
	return std::to_string(entity.Raw().id()) + "/" + type.Name();
}

bool InferField(const sol::object &value, FieldDesc &desc, FieldDefault &def) {
	if (value.is<FieldTypeTag>()) {
		desc.type = value.as<FieldTypeTag>().type;
		def.type = desc.type;
		return true;
	}
	switch (value.get_type()) {
	case sol::type::boolean:
		desc.type = FieldType::Bool;
		def.type = FieldType::Bool;
		def.b = value.as<bool>();
		return true;
	case sol::type::number:
		desc.type = FieldType::F32;
		def.type = FieldType::F32;
		def.f = value.as<f32>();
		return true;
	case sol::type::string:
		desc.type = FieldType::String;
		def.type = FieldType::String;
		def.s = value.as<std::string>();
		return true;
	default:
		break;
	}
	if (value.is<Vector2>()) {
		desc.type = FieldType::Vector2;
		def.type = FieldType::Vector2;
		def.v2 = value.as<Vector2>();
		return true;
	}
	if (value.is<Vector3>()) {
		desc.type = FieldType::Vector3;
		def.type = FieldType::Vector3;
		def.v3 = value.as<Vector3>();
		return true;
	}
	if (value.is<Color>()) {
		desc.type = FieldType::Color;
		def.type = FieldType::Color;
		def.color = value.as<Color>();
		return true;
	}
	if (value.is<Entity>()) {
		desc.type = FieldType::EntityRef;
		def.type = FieldType::EntityRef;
		def.entity = value.as<Entity>();
		return true;
	}
	return false;
}

void WriteDefault(Entity entity, ComponentType type, const FieldDefault &def) {
	FieldRef ref = entity.GetField(type, def.name);
	if (!ref || ref.Type() != def.type) {
		return;
	}
	switch (def.type) {
	case FieldType::Bool:
		ref.Set(def.b);
		break;
	case FieldType::I32:
		ref.Set(def.i);
		break;
	case FieldType::U32:
		ref.Set(def.u);
		break;
	case FieldType::F32:
		ref.Set(def.f);
		break;
	case FieldType::F64:
		ref.Set(def.d);
		break;
	case FieldType::String:
		ref.Set(def.s);
		break;
	case FieldType::EntityRef:
		ref.Set(def.entity);
		break;
	case FieldType::ResourceRef:
		ref.Set(def.resource);
		break;
	case FieldType::Enum:
		if (ref.Data() != nullptr) {
			*static_cast<i32 *>(ref.Data()) = def.i;
		}
		break;
	case FieldType::Vector2:
		ref.Set(def.v2);
		break;
	case FieldType::Vector3:
		ref.Set(def.v3);
		break;
	case FieldType::Color:
		ref.Set(def.color);
		break;
	}
}

void BindFieldTags(sol::state &lua) {
	lua.new_usertype<FieldTypeTag>("FieldTypeTag", sol::no_constructor);
	sol::table field = lua.create_named_table("Field");
	field["Bool"] = FieldTypeTag{FieldType::Bool};
	field["I32"] = FieldTypeTag{FieldType::I32};
	field["U32"] = FieldTypeTag{FieldType::U32};
	field["F32"] = FieldTypeTag{FieldType::F32};
	field["F64"] = FieldTypeTag{FieldType::F64};
	field["String"] = FieldTypeTag{FieldType::String};
	field["Entity"] = FieldTypeTag{FieldType::EntityRef};
	field["Resource"] = FieldTypeTag{FieldType::ResourceRef};
	field["Vector2"] = FieldTypeTag{FieldType::Vector2};
	field["Vector3"] = FieldTypeTag{FieldType::Vector3};
	field["Color"] = FieldTypeTag{FieldType::Color};
}

bool IsLuaTrue(const sol::object &object) {
	if (!object.valid()) {
		return false;
	}
	switch (object.get_type()) {
	case sol::type::none:
	case sol::type::lua_nil:
		return false;
	case sol::type::boolean:
		return object.as<bool>();
	default:
		return true;
	}
}

std::string CycleMessage(const std::vector<std::string> &loading, const std::string &path) {
	std::string msg = "require cycle: ";
	for (const std::string &item : loading) {
		msg += item;
		msg += " -> ";
	}
	msg += path;
	return msg;
}

} // namespace

ScriptServer::ScriptServer(World &world) : _world(world), _impl(MakeScope<Impl>()) {
	_impl->lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table,
		sol::lib::coroutine, sol::lib::utf8);
	_impl->lua["loadfile"] = sol::lua_nil;
	_impl->lua["dofile"] = sol::lua_nil;
	sol::table package = _impl->lua.create_named_table("package");
	package["loaded"] = _impl->lua.create_table();
	_impl->lua.set_function("require",
		[this](const std::string &name) { return requireModule(name); });
	BindEditorCommands(_impl->lua, _impl->commands, _impl->commandInputs, _impl->sourcePaths);
	BindFieldTags(_impl->lua);
	BindScriptApi(_impl->lua, _world, *this);
	attachDebugger();
}

// Initialize after libraries so coroutines are known. Replace sol's protected-call
// message handler for break-on-error and expose component and entity fields to inspection.
void ScriptServer::attachDebugger() {
	lua_State *L = _impl->lua.lua_state();
	_impl->debugger = std::make_unique<LuaDebugger>(L);
	_impl->debugger->PushErrorHandler(L);
	lua_setglobal(L, sol::detail::default_handler_name());

	const sol::object expander =
		MakeLuaFunction(_impl->lua, [this](sol::object value, sol::this_state s) -> sol::object {
			sol::state_view lua(s);
			if (value.is<ComponentView>()) {
				const ComponentView view = value.as<ComponentView>();
				sol::userdata component = value;
				sol::table fields = lua.create_table();
				fields["entity"] = view.entity;
				for (const FieldInfo &field : view.type.Fields()) {
					if (field.IsExposed()) {
						fields[field.name] = component[field.name].get<sol::object>();
					}
				}
				if (sol::table extras = extrasTable(view.entity, view.type, false);
					extras.valid()) {
					for (const auto &[key, extra] : extras) {
						fields[key] = extra;
					}
				}
				return fields;
			}
			if (value.is<Entity>()) {
				const Entity entity = value.as<Entity>();
				sol::table fields = lua.create_table();
				fields["alive"] = entity.IsAlive();
				if (entity.IsAlive()) {
					fields["name"] = entity.GetName();
					fields["path"] = entity.GetPath();
					fields["visible"] = entity.IsVisible();
					sol::table components = lua.create_table();
					for (const ComponentType type : entity.GetComponents()) {
						components[type.Name()] = ComponentView{entity, type};
					}
					fields["components"] = components;
				}
				return fields;
			}
			return sol::make_object(s, sol::lua_nil);
		});
	expander.push(L);
	_impl->debugger->SetUserdataExpander(L);
}

LuaDebugger &ScriptServer::Debugger() {
	return *_impl->debugger;
}

ScriptServer::~ScriptServer() {
	if (_impl->fileEvents) {
		_impl->fileEvents->changed.remove(_impl->fileChanged);
	}
	dropLuaSystems();
}

sol::state &ScriptServer::Lua() {
	return _impl->lua;
}

CommandRegistry &ScriptServer::Commands() {
	return _impl->commands;
}

sowa::CommandInputs &ScriptServer::CommandInputs() {
	return _impl->commandInputs;
}

Timer *ScriptServer::NewTimer(f32 timeout, bool autoStart, bool repeating) {
	auto &timers = _impl->reloadStage ? _impl->reloadStage->timers : _impl->timers;
	return &timers.emplace_back(timeout, autoStart, repeating);
}

void ScriptServer::UpdateTimers(f32 delta) {
	for (Timer &timer : _impl->timers) {
		timer.Update(delta);
	}
}

void ScriptServer::ClearTimers() {
	if (_impl->reloadStage) {
		_impl->reloadStage->clearTimers = true;
		_impl->reloadStage->timers.clear();
		return;
	}
	_impl->timers.clear();
}

std::list<Timer> &ScriptServer::Timers() {
	return _impl->timers;
}

Tween *ScriptServer::NewTween(f32 duration, bool autoStart) {
	auto &tweens = _impl->reloadStage ? _impl->reloadStage->tweens : _impl->tweens;
	return &tweens.emplace_back(duration, autoStart);
}

void ScriptServer::UpdateTweens(f32 delta) {
	for (Tween &tween : _impl->tweens) {
		tween.Update(delta);
	}
}

void ScriptServer::ClearTweens() {
	if (_impl->reloadStage) {
		_impl->reloadStage->clearTweens = true;
		_impl->reloadStage->tweens.clear();
		return;
	}
	_impl->tweens.clear();
}

std::list<Tween> &ScriptServer::Tweens() {
	return _impl->tweens;
}

bool ScriptServer::DoString(std::string_view code, std::string_view chunkName) {
	return runChunk(code, chunkName, chunkName);
}

// Lua treats @path chunks as files, giving errors and breakpoints the same source path.
bool ScriptServer::runChunk(std::string_view code, std::string_view chunkName,
	std::string_view displayName) {
	sol::protected_function_result result = _impl->lua.safe_script(std::string(code),
		sol::script_pass_on_error, std::string(chunkName));
	if (!result.valid()) {
		const sol::error err = result;
		Log::Error("Lua error in {}: {}", displayName, err.what());
		return false;
	}
	SyncLifecycles();
	return true;
}

void ScriptServer::bindFileEvents(FileSystem &fs) {
	const auto events = fs.Events();
	if (_impl->fileEvents == events) {
		return;
	}
	if (_impl->fileEvents) {
		_impl->fileEvents->changed.remove(_impl->fileChanged);
	}
	_impl->fileEvents = events;
	_impl->pendingFiles.clear();
	_impl->fileChanged = events->changed.append([this](const std::string &path) {
		if (_impl->scriptFiles.contains(path)) {
			_impl->pendingFiles.insert(path);
		}
	});
}

std::size_t ScriptServer::MemoryUsed() const {
	return _impl->lua.memory_used();
}

bool ScriptServer::FlushFileChanges() {
	if (_impl->pendingFiles.empty()) {
		return true;
	}
	if (_world.IsDeferred()) {
		return false; // Leave the batch queued.
	}
	SOWA_PROFILE_SCOPE("Script hot reload");
	FileSystem &fs = *_impl->fileSystem;
	const auto changed = std::exchange(_impl->pendingFiles, {});
	bool different = false;
	// Validate changed files before tearing down any running systems. Syntax
	// errors leave the previous code running; a later save retries via events.
	for (const auto &path : changed) {
		const auto file = fs.Load(path);
		if (!file || !file.value) {
			Log::Error("Script hot reload: cannot read '{}'; keeping current code", path);
			return false;
		}
		const std::string code(reinterpret_cast<const char *>(file.value->Data()),
			file.value->Size());
		const auto old = _impl->sourceContents.find(path);
		if (old != _impl->sourceContents.end() && old->second == code) {
			continue;
		}
		different = true;
		sol::load_result chunk = _impl->lua.load(code, "@" + path);
		if (!chunk.valid()) {
			const sol::error error = chunk.get<sol::error>();
			Log::Error("Script hot reload: {}; keeping current code", error.what());
			return false;
		}
	}
	if (!different) {
		return true;
	}
	const bool success = reloadConfiguredModules(fs);
	if (success) {
		Log::Info("Hot reloaded scripts");
	} else {
		Log::Error("Script hot reload failed; fix the reported error and save again");
	}
	return success;
}

bool ScriptServer::DoFile(FileSystem &fs, std::string_view path) {
	bindFileEvents(fs);
	_impl->fileSystem = &fs;
	const FsResult<PathData> resolved = fs.Resolve(path);
	if (!resolved) {
		Log::Error("Lua: invalid path '{}': {}", path, FsErrorToString(resolved.error));
		return false;
	}
	const std::string canonical = fs.Normalize(path);
	_impl->scriptFiles.insert(canonical);
	if (!_impl->loadingProjectModules && _impl->sourcePaths.empty() &&
		std::find(_impl->directPaths.begin(), _impl->directPaths.end(), canonical) ==
			_impl->directPaths.end()) {
		_impl->directPaths.push_back(canonical);
	}
	const FsResult<Ref<FileData>> loaded = fs.Load(canonical);
	if (!loaded) {
		Log::Error("Lua: failed to load '{}': {}", canonical, FsErrorToString(loaded.error));
		return false;
	}
	const FileData &data = *loaded.value;
	const std::string code(reinterpret_cast<const char *>(data.Data()), data.Size());
	_impl->commandInputs.Cancel();
	_impl->commands.RemoveSource(canonical);
	_impl->sourcePaths.push_back(canonical);
	if (_impl->loadingProjectModules) {
		_impl->projectSources.insert(canonical);
	}
	const auto popSource = MakeScopeExit([this] { _impl->sourcePaths.pop_back(); });
	const bool success = runChunk(code, "@" + canonical, canonical);
	if (!success) {
		_impl->commands.RemoveSource(canonical);
	} else {
		_impl->sourceContents[canonical] = code;
	}
	return success;
}

bool ScriptServer::LoadModules(FileSystem &fs, const std::vector<std::string> &paths) {
	bindFileEvents(fs);
	_impl->scriptFiles.clear();
	_impl->pendingFiles.clear();
	_impl->fileSystem = &fs;
	_impl->modulePaths.assign(paths.begin(), paths.end());
	_impl->projectSources.clear();
	const bool wasLoadingProjectModules = _impl->loadingProjectModules;
	_impl->loadingProjectModules = true;
	const auto restoreLoadingState = MakeScopeExit([this, wasLoadingProjectModules] {
		_impl->loadingProjectModules = wasLoadingProjectModules;
	});
	for (const std::string &path : paths) {
		if (!DoFile(fs, path)) {
			return false;
		}
	}
	return true;
}

bool ScriptServer::ReloadModules(FileSystem &fs) {
	if (_impl->modulePaths.empty() && _impl->directPaths.empty()) {
		return true;
	}
	return reloadConfiguredModules(fs);
}

bool ScriptServer::ReloadModules(FileSystem &fs, const std::vector<std::string> &paths) {
	const auto previous = _impl->modulePaths;
	_impl->modulePaths = paths;
	if (reloadConfiguredModules(fs)) {
		return true;
	}
	_impl->modulePaths = previous;
	return false;
}

bool ScriptServer::reloadConfiguredModules(FileSystem &fs) {
	if (_world.IsDeferred()) {
		Log::Error("ReloadModules: refused while Progress is deferred");
		return false;
	}
	SOWA_PROFILE_SCOPE("Script load");
	bindFileEvents(fs);
	_impl->fileSystem = &fs;
	if (_impl->reloadStage) {
		Log::Error("Recursive script reload is not supported");
		return false;
	}
	LuaTableSnapshot tables;
	tables.Capture(_impl->lua.globals());
	for (const auto &[name, table] : _impl->prototypes) {
		tables.Capture(table);
	}
	for (const auto &[name, table] : _impl->extras) {
		tables.Capture(table);
	}
	const auto prototypes = _impl->prototypes;
	const auto sourceComponents = _impl->sourceComponents;
	const auto componentSources = _impl->componentSources;
	const auto projectSources = _impl->projectSources;
	const auto sourceContents = _impl->sourceContents;
	const auto commands = _impl->commands;
	const auto inputs = _impl->commandInputs;
	const auto previousFiles = std::exchange(_impl->scriptFiles, {});
	const auto errorsBefore = Log::ErrorCount();
	_impl->reloadStage = std::make_unique<Impl::ReloadStage>();
	bool completed = false;
	const auto rollback = MakeScopeExit([&] {
		_impl->reloadStage.reset();
		if (completed) {
			return;
		}
		tables.Restore();
		_impl->prototypes = prototypes;
		_impl->sourceComponents = sourceComponents;
		_impl->componentSources = componentSources;
		_impl->projectSources = projectSources;
		_impl->sourceContents = sourceContents;
		_impl->commands = commands;
		_impl->commandInputs = inputs;
		// New failed dependencies must remain watched so a corrected save retries.
		_impl->scriptFiles.insert(previousFiles.begin(), previousFiles.end());
	});
	clearLoaded();
	_impl->commandInputs.Cancel();
	_impl->commands.Clear();
	// Patch the existing prototype table identity, removing obsolete hooks
	// and methods as well as replacing functions that were edited.
	for (const auto &[type, source] : _impl->componentSources) {
		const auto found = _impl->prototypes.find(type);
		if (found == _impl->prototypes.end()) {
			continue;
		}
		std::vector<sol::object> keys;
		for (const auto &entry : found->second) {
			keys.push_back(entry.first);
		}
		for (const auto &key : keys) {
			found->second[key] = sol::lua_nil;
		}
	}
	_impl->sourceComponents.clear();
	_impl->componentSources.clear();
	_impl->projectSources.clear();
	const bool wasLoadingProjectModules = _impl->loadingProjectModules;
	_impl->loadingProjectModules = true;
	const auto restoreLoadingState = MakeScopeExit([this, wasLoadingProjectModules] {
		_impl->loadingProjectModules = wasLoadingProjectModules;
	});
	for (const std::string &path : _impl->modulePaths) {
		if (!DoFile(fs, path)) {
			return false;
		}
	}
	_impl->loadingProjectModules = false;
	const auto direct = _impl->directPaths;
	for (const auto &path : direct) {
		if (std::find(_impl->modulePaths.begin(), _impl->modulePaths.end(), path) !=
			_impl->modulePaths.end()) {
			continue;
		}
		if (!DoFile(fs, path)) {
			return false;
		}
	}
	for (const auto &name : _impl->reloadStage->queriedComponents) {
		if (!_impl->reloadStage->components.contains(name) &&
			!_world.FindComponent(name).IsValid()) {
			Log::Error("Script reload: unknown component '{}'", name);
			return false;
		}
	}
	if (Log::ErrorCount() != errorsBefore) {
		return false;
	}
	auto stage = std::move(_impl->reloadStage);
	dropLuaSystems();
	for (auto &definition : stage->componentDefinitions) {
		definition();
	}
	for (auto &registration : stage->registrations) {
		registration();
	}
	if (stage->clearTimers) {
		_impl->timers.clear();
	}
	if (stage->clearTweens) {
		_impl->tweens.clear();
	}
	_impl->timers.splice(_impl->timers.end(), stage->timers);
	_impl->tweens.splice(_impl->tweens.end(), stage->tweens);
	completed = true;
	return true;
}

void ScriptServer::clearLoaded() {
	sol::table package = _impl->lua["package"];
	if (!package.valid()) {
		return;
	}
	package["loaded"] = _impl->lua.create_table();
}

sol::object ScriptServer::requireModule(const std::string &name) {
	if (_impl->fileSystem == nullptr) {
		throw std::runtime_error("require: no filesystem");
	}

	const FsResult<PathData> resolved = _impl->fileSystem->Resolve(name);
	if (!resolved) {
		throw std::runtime_error(
			fmt::format("require: invalid path '{}': {}", name, FsErrorToString(resolved.error)));
	}
	const std::string path = _impl->fileSystem->Normalize(name);
	_impl->scriptFiles.insert(path);

	sol::table loaded = _impl->lua["package"]["loaded"];
	if (!loaded.valid()) {
		throw std::runtime_error("require: package.loaded is missing");
	}

	sol::object cached = loaded[path];
	if (IsLuaTrue(cached)) {
		if (_impl->loadingProjectModules) {
			_impl->projectSources.insert(path);
		}
		return cached;
	}

	for (const std::string &inFlight : _impl->loading) {
		if (inFlight == path) {
			throw std::runtime_error(CycleMessage(_impl->loading, path));
		}
	}

	_impl->loading.push_back(path);
	const auto popLoading = MakeScopeExit([this] { _impl->loading.pop_back(); });

	const FsResult<Ref<FileData>> file = _impl->fileSystem->Load(path);
	if (!file) {
		throw std::runtime_error(
			fmt::format("Lua: failed to load '{}': {}", path, FsErrorToString(file.error)));
	}
	const FileData &data = *file.value;
	const std::string code(reinterpret_cast<const char *>(data.Data()), data.Size());
	_impl->commands.RemoveSource(path);
	_impl->sourcePaths.push_back(path);
	if (_impl->loadingProjectModules) {
		_impl->projectSources.insert(path);
	}
	const auto popSource = MakeScopeExit([this] { _impl->sourcePaths.pop_back(); });

	sol::load_result chunk = _impl->lua.load(code, "@" + path);
	if (!chunk.valid()) {
		const sol::error err = chunk.get<sol::error>();
		throw std::runtime_error(err.what());
	}

	sol::protected_function_result result = chunk(path);
	if (!result.valid()) {
		_impl->commands.RemoveSource(path);
		const sol::error err = result;
		throw std::runtime_error(err.what());
	}

	if (result.return_count() > 0 && result.get_type(0) != sol::type::lua_nil) {
		loaded[path] = result.get<sol::object>(0);
	}

	sol::object stored = loaded[path];
	if (!stored.valid() || stored.get_type() == sol::type::lua_nil) {
		loaded[path] = true;
		stored = loaded[path];
	}
	_impl->sourceContents[path] = code;
	SyncLifecycles();
	return stored;
}

void ScriptServer::SetEntity(const std::string &name, Entity entity) {
	_impl->lua[name] = entity;
}

void ScriptServer::BindEngine(Input &input, SceneManager &scenes, ResourceRegistry &resources,
	const ProjectSettings &settings, AudioMixer *mixer, PhysicsWorld *physics,
	StringStore *globalStore) {
	BindEngineApi(_impl->lua, _world, input, scenes, resources, settings, mixer, physics,
		globalStore);
	if (physics != nullptr) {
		physics->Events().append([this](const Collision2D &event) { dispatchCollision(event); });
	}
}

void ScriptServer::UpdateSettings(const ProjectSettings &settings) {
	sol::object object = _impl->lua["settings"];
	sol::table table;
	if (object.valid() && object.get_type() == sol::type::table) {
		table = object.as<sol::table>();
	} else {
		table = _impl->lua.create_named_table("settings");
	}
	table["main_scene"] = settings.application.mainScene;
	table["viewport_width"] = settings.rendering.viewport.width;
	table["viewport_height"] = settings.rendering.viewport.height;
}

bool ScriptServer::CallComponentMethod(Entity entity, const std::string &component,
	const std::string &method) {
	auto type = _world.FindComponent(component);
	if (!entity.Has(type) || method.empty()) {
		return false;
	}
	auto it = _impl->prototypes.find(component);
	if (it == _impl->prototypes.end()) {
		return false;
	}
	sol::object function = it->second[method];
	if (function.get_type() != sol::type::function) {
		return false;
	}
	return callHook(component, method.c_str(), entity, type, nullptr);
}

bool ScriptServer::Call(std::string_view functionName) {
	return call(functionName, nullptr);
}

bool ScriptServer::Call(std::string_view functionName, f32 arg) {
	return call(functionName, &arg);
}

bool ScriptServer::call(std::string_view functionName, const f32 *arg) {
	sol::object object = _impl->lua[std::string(functionName)];
	if (object.get_type() != sol::type::function) {
		return true;
	}

	sol::protected_function fn = object;
	sol::protected_function_result result = arg != nullptr ? fn(*arg) : fn();
	if (result.valid()) {
		return true;
	}

	const sol::error err = result;
	Log::Error("Lua error in {}: {}", functionName, err.what());
	return false;
}

sol::object ScriptServer::DefineComponent(const std::string &name, sol::optional<sol::table> fields,
	sol::this_state s) {
	if (name.empty()) {
		Log::Error("world:component: empty name");
		return sol::make_object(s, sol::lua_nil);
	}

	std::vector<FieldDesc> descs;
	std::vector<FieldDefault> defaults;
	if (fields && fields->valid()) {
		std::unordered_set<std::string> seen;

		struct Parsed {
			FieldDesc desc;
			FieldDefault def;
		};

		std::vector<Parsed> parsed;
		for (const auto &pair : *fields) {
			if (!pair.first.is<std::string>()) {
				Log::Error("world:component '{}': field names must be strings", name);
				return sol::make_object(s, sol::lua_nil);
			}
			Parsed item;
			item.desc.name = pair.first.as<std::string>();
			item.def.name = item.desc.name;
			if (!seen.insert(item.desc.name).second) {
				Log::Error("world:component '{}': duplicate field '{}'", name, item.desc.name);
				return sol::make_object(s, sol::lua_nil);
			}
			if (!InferField(pair.second, item.desc, item.def)) {
				Log::Error("world:component '{}': cannot infer type of field '{}'", name,
					item.desc.name);
				return sol::make_object(s, sol::lua_nil);
			}
			parsed.push_back(std::move(item));
		}
		std::sort(parsed.begin(), parsed.end(),
			[](const Parsed &a, const Parsed &b) { return a.desc.name < b.desc.name; });
		for (Parsed &item : parsed) {
			descs.push_back(std::move(item.desc));
			defaults.push_back(std::move(item.def));
		}
	}

	if (_impl->reloadStage) {
		const auto existing = _world.FindComponent(name);
		if (existing.IsValid() && !_world.IsDataDescribed(existing)) {
			throw std::runtime_error(
				"world:component '" + name + "': a native component already uses this name");
		}
		_impl->reloadStage->components.insert(name);
		_impl->reloadStage->componentDefinitions.push_back(
			[this, name, descs, defaults] { applyComponentDefinition(name, descs, defaults); });
	} else if (!applyComponentDefinition(name, descs, defaults).IsValid()) {
		return sol::make_object(s, sol::lua_nil);
	}
	recordScriptComponent(name);

	auto it = _impl->prototypes.find(name);
	if (it == _impl->prototypes.end()) {
		it = _impl->prototypes.emplace(name, _impl->lua.create_table()).first;
	}

	return it->second;
}

ComponentType ScriptServer::applyComponentDefinition(const std::string &name,
	const std::vector<FieldDesc> &descs, const std::vector<FieldDefault> &defaults) {
	ComponentType type = _world.FindComponent(name);
	std::unordered_set<std::string> previousNames;
	if (type.IsValid()) {
		for (const FieldInfo &info : type.Fields()) {
			previousNames.insert(info.name);
		}
	}

	bool migrated = false;
	if (type.IsValid()) {
		if (!_world.IsDataDescribed(type)) {
			Log::Error("world:component '{}': a native component already uses this name", name);
			return {};
		}
		if (!_world.ComponentFieldsMatch(type, descs)) {
			// Flecs cannot replace a component while lifecycle systems still query it.
			destroyComponentSystems(name);
			_impl->migrating = true;
			EcsResult<ComponentType> replaced = _world.ReplaceDataDescribedFields(type, descs);
			_impl->migrating = false;
			if (!replaced) {
				Log::Error("world:component '{}': failed to migrate fields", name);
				return {};
			}
			type = replaced.value;
			migrated = true;
		}
	} else {
		EcsResult<ComponentType> created = _world.RegisterComponent(name, descs);
		if (!created) {
			Log::Error("world:component '{}': registration failed", name);
			return {};
		}
		type = created.value;
	}

	_impl->defaults[name] = defaults;
	if (migrated) {
		const std::vector<FieldDefault> &defs = _impl->defaults[name];
		_world.EachWithType(type, true, [&](Entity entity) {
			for (const FieldDefault &def : defs) {
				if (previousNames.count(def.name) != 0) {
					continue;
				}
				WriteDefault(entity, type, def);
			}
		});
	}

	ensureStartSystem(name, type);
	return type;
}

void ScriptServer::recordScriptComponent(const std::string &typeName) {
	if (_impl->sourcePaths.empty()) {
		return;
	}
	const std::string &source = _impl->sourcePaths.back();
	auto previous = _impl->componentSources.find(typeName);
	if (previous != _impl->componentSources.end() && previous->second != source) {
		auto sourceIt = _impl->sourceComponents.find(previous->second);
		if (sourceIt != _impl->sourceComponents.end()) {
			auto &names = sourceIt->second;
			names.erase(std::remove(names.begin(), names.end(), typeName), names.end());
			if (names.empty()) {
				_impl->sourceComponents.erase(sourceIt);
			}
		}
	}
	_impl->componentSources[typeName] = source;
	auto &names = _impl->sourceComponents[source];
	if (std::find(names.begin(), names.end(), typeName) == names.end()) {
		names.push_back(typeName);
	}
}

std::vector<ComponentType> ScriptServer::ScriptComponentTypes(std::string_view path) const {
	std::vector<ComponentType> out;
	auto it = _impl->sourceComponents.find(std::string(path));
	if (it == _impl->sourceComponents.end()) {
		return out;
	}
	out.reserve(it->second.size());
	for (const std::string &name : it->second) {
		ComponentType type = _world.FindComponent(name);
		if (type.IsValid()) {
			out.push_back(type);
		}
	}
	return out;
}

std::vector<std::string> ScriptServer::AttachedScriptPaths(Entity entity) const {
	std::vector<std::string> out;
	if (!entity.IsAlive()) {
		return out;
	}
	for (const auto &[path, names] : _impl->sourceComponents) {
		for (const std::string &name : names) {
			const ComponentType type = _world.FindComponent(name);
			if (type.IsValid() && entity.Has(type)) {
				out.push_back(path);
				break;
			}
		}
	}
	return out;
}

bool ScriptServer::AttachScript(Entity entity, std::string_view path) {
	if (!entity.IsAlive()) {
		return false;
	}
	const std::vector<ComponentType> types = ScriptComponentTypes(path);
	if (types.empty()) {
		return false;
	}
	for (ComponentType type : types) {
		if (entity.Has(type)) {
			continue;
		}
		entity.Add(type);
		ApplyDefaults(entity, type);
	}
	return true;
}

bool ScriptServer::RemoveScript(Entity entity, std::string_view path) {
	if (!entity.IsAlive()) {
		return false;
	}
	bool removed = false;
	for (ComponentType type : ScriptComponentTypes(path)) {
		if (!entity.Has(type)) {
			continue;
		}
		entity.Remove(type);
		removed = true;
	}
	return removed;
}

bool ScriptServer::IsProjectScriptSource(std::string_view path) const {
	return _impl->projectSources.contains(std::string(path));
}

void ScriptServer::FinishSystem(const std::string &name, const std::vector<std::string> &withNames,
	const std::vector<std::string> &withoutNames, bool always, sol::protected_function fn) {
	if (_impl->reloadStage) {
		if (name.empty()) {
			throw std::runtime_error("world:system: empty name");
		}
		for (const auto &component : withNames) {
			_impl->reloadStage->queriedComponents.push_back(component);
		}
		for (const auto &component : withoutNames) {
			_impl->reloadStage->queriedComponents.push_back(component);
		}
		_impl->reloadStage->registrations.push_back(
			[this, name, withNames, withoutNames, always, fn] {
				FinishSystem(name, withNames, withoutNames, always, fn);
			});
		return;
	}

	if (name.empty()) {
		Log::Error("world:system: empty name");
		return;
	}

	std::vector<ComponentType> with;
	for (const std::string &typeName : withNames) {
		ComponentType type = _world.FindComponent(typeName);
		if (!type.IsValid()) {
			Log::Error("world:system '{}': unknown component '{}'", name, typeName);
			return;
		}
		with.push_back(type);
	}
	std::vector<ComponentType> without;
	for (const std::string &typeName : withoutNames) {
		ComponentType type = _world.FindComponent(typeName);
		if (!type.IsValid()) {
			Log::Error("world:system '{}': unknown component '{}'", name, typeName);
			return;
		}
		without.push_back(type);
	}

	destroyLuaSystem(name);
	System system = _world.RegisterUntypedSystem(name, with, without, always,
		[this, fn, with](Entity entity) mutable {
			sol::protected_function_result result;
			switch (with.size()) {
			case 0:
				result = fn();
				break;
			case 1:
				result = fn(entity, ComponentView{entity, with[0]});
				break;
			case 2:
				result = fn(entity, ComponentView{entity, with[0]}, ComponentView{entity, with[1]});
				break;
			case 3:
				result = fn(entity, ComponentView{entity, with[0]}, ComponentView{entity, with[1]},
					ComponentView{entity, with[2]});
				break;
			default:
				result = fn(entity, ComponentView{entity, with[0]}, ComponentView{entity, with[1]},
					ComponentView{entity, with[2]}, ComponentView{entity, with[3]});
				break;
			}
			if (!result.valid()) {
				const sol::error err = result;
				Log::Error("Lua error in system: {}", err.what());
			}
		});
	rememberLuaSystem(name, system);
}

void ScriptServer::dropLuaSystems() {
	for (auto &[name, system] : _impl->luaSystems) {
		_world.DestroyTrackedSystem(system);
	}
	_impl->luaSystems.clear();
}

void ScriptServer::destroyLuaSystem(const std::string &name) {
	for (auto it = _impl->luaSystems.begin(); it != _impl->luaSystems.end(); ++it) {
		if (it->first != name) {
			continue;
		}
		_world.DestroyTrackedSystem(it->second);
		_impl->luaSystems.erase(it);
		return;
	}
	flecs::entity existing = _world.Flecs().lookup(name.c_str());
	if (existing) {
		existing.destruct();
	}
}

void ScriptServer::rememberLuaSystem(const std::string &name, System system) {
	_impl->luaSystems.emplace_back(name, system);
}

void ScriptServer::SyncLifecycles() {
	if (_impl->reloadStage) {
		_impl->reloadStage->registrations.push_back([this] { SyncLifecycles(); });
		return;
	}

	for (auto &[typeName, proto] : _impl->prototypes) {
		ComponentType type = _world.FindComponent(typeName);
		if (!type.IsValid()) {
			continue;
		}
		const bool hasStart = proto["_start"].get_type() == sol::type::function;
		const bool hasUpdate = proto["_update"].get_type() == sol::type::function;
		const bool hasExit = proto["_exit"].get_type() == sol::type::function;
		const bool hasFixed = proto["_physics_update"].get_type() == sol::type::function;
		if (hasStart || hasExit || hasFixed) {
			// Preserve _start's pipeline position before script-defined systems.
			ensureStartSystem(typeName, type);
		}
		if (hasUpdate) {
			bindUpdateSystem(typeName, type);
		}
		destroyLuaSystem("Lua." + typeName + "._physics_update");
		if (hasFixed) {
			bindFixedUpdateSystem(typeName, type);
		}
		bindExitObserver(typeName, type);
	}
}

void ScriptServer::bindStartSystem(const std::string &typeName, ComponentType type) {
	const std::string name = "Lua." + typeName + "._start";
	destroyLuaSystem(name);
	auto builder = _world.Flecs().system(name.c_str())
		.with(type.Raw()).without<ScriptLive>(type.Raw());
	flecs::system sys = builder.each([this, typeName, type](flecs::entity e) {
		// Hooks are assigned after component definition; check them at runtime.
		auto it = _impl->prototypes.find(typeName);
		if (it == _impl->prototypes.end()) {
			return;
		}
		const sol::table &proto = it->second;
		const bool hasHook = proto["_start"].get_type() == sol::type::function ||
			proto["_exit"].get_type() == sol::type::function ||
			proto["_physics_update"].get_type() == sol::type::function;
		if (!hasHook) {
			return;
		}
		startInstance(typeName, type, Entity(e));
	});
	System handle(sys);
	_world.TrackPlayOnly(handle);
	rememberLuaSystem(name, handle);
}

void ScriptServer::destroyComponentSystems(const std::string &typeName) {
	destroyLuaSystem("Lua." + typeName + "._start");
	destroyLuaSystem("Lua." + typeName + "._update");
	destroyLuaSystem("Lua." + typeName + "._physics_update");
}

void ScriptServer::ensureStartSystem(const std::string &typeName, ComponentType type) {
	const std::string name = "Lua." + typeName + "._start";
	const bool bound = std::any_of(_impl->luaSystems.begin(), _impl->luaSystems.end(),
		[&name](const auto &entry) { return entry.first == name; });
	if (bound) {
		return;
	}
	bindStartSystem(typeName, type);
}

void ScriptServer::bindUpdateSystem(const std::string &typeName, ComponentType type) {
	const std::string name = "Lua." + typeName + "._update";
	destroyLuaSystem(name);
	System system = _world.RegisterUntypedSystem(name, {type}, {}, false,
		[this, typeName, type](Entity entity) {
			const f32 dt = _world.DeltaTime();
			callHook(typeName, "_update", entity, type, &dt);
		});
	rememberLuaSystem(name, system);
}

void ScriptServer::bindFixedUpdateSystem(const std::string &typeName, ComponentType type) {
	const std::string name = "Lua." + typeName + "._physics_update";
	System system = _world.RegisterUntypedFixedSystem(name, {type}, {},
		[this, typeName, type](Entity entity, f32 dt) {
			if (!startInstance(typeName, type, entity)) {
				return;
			}
			callHook(typeName, "_physics_update", entity, type, &dt);
		});
	rememberLuaSystem(name, system);
}

bool ScriptServer::startInstance(const std::string &typeName, ComponentType type, Entity entity) {
	if (_world.FixedTicksCancelled() || !entity.IsAlive() || !entity.Has(type) ||
		!entity.Raw().enabled()) {
		return false;
	}
	if (entity.Raw().has<ScriptLive>(type.Raw())) {
		return true;
	}
	// Commit before _start so removal observers can clear the live marker.
	const bool deferred = _world.IsDeferred();
	if (deferred) { _world.SuspendDefer(); }
	entity.Raw().add<ScriptLive>(type.Raw());
	if (deferred) { _world.ResumeDefer(); }
	callHook(typeName, "_start", entity, type, nullptr);
	if (_world.FixedTicksCancelled()) {
		if (entity.IsAlive()) {
			entity.Raw().remove<ScriptLive>(type.Raw());
		}
		return false;
	}
	return entity.IsAlive() && entity.Has(type);
}

void ScriptServer::bindExitObserver(const std::string &typeName, ComponentType type) {
	const std::string name = "Lua." + typeName + "._exit";
	destroyLuaSystem(name);
	flecs::observer observer = _world.Flecs()
								   .observer(name.c_str())
								   .with(type.Raw())
								   .event(flecs::OnRemove)
								   .each([this, typeName, type](flecs::entity e) {
									   Entity entity(e);
									   if (e.has<ScriptLive>(type.Raw())) {
#if SOWA_PROFILER
										   const ProfileScopeGuard profileScope(Profiler::Main(),
											   hookProfileName(typeName, "_exit"));
#endif
										   callHook(typeName, "_exit", entity, type, nullptr);
										   if (e.is_alive()) {
											   e.remove<ScriptLive>(type.Raw());
										   }
									   }
									   if (!_impl->migrating) {
										   clearExtras(entity, type);
									   }
								   });
	rememberLuaSystem(name, System(observer));
}

bool ScriptServer::callHook(const std::string &typeName, const char *hook, Entity entity,
	ComponentType type, const f32 *dt) {
	auto it = _impl->prototypes.find(typeName);
	if (it == _impl->prototypes.end()) {
		return true;
	}
	sol::object object = it->second[hook];
	if (object.get_type() != sol::type::function) {
		return true;
	}
	sol::protected_function fn = object;
	const u64 abortGeneration = Debugger().AbortGeneration();
	sol::protected_function_result result =
		dt != nullptr ? fn(ComponentView{entity, type}, *dt) : fn(ComponentView{entity, type});
	if (Debugger().AbortGeneration() != abortGeneration) {
		_world.CancelFixedTicks();
		return false;
	}
	if (result.valid()) {
		return true;
	}
	const sol::error err = result;
	Log::Error("Lua error in {} {}: {}", typeName, hook, err.what());
	return false;
}

void ScriptServer::dispatchCollision(const Collision2D &event) {
	const char *hook = nullptr;
	switch (event.kind) {
	case CollisionKind::Begin:
		hook = "_collision_begin";
		break;
	case CollisionKind::End:
		hook = "_collision_end";
		break;
	case CollisionKind::TriggerBegin:
		hook = "_trigger_begin";
		break;
	case CollisionKind::TriggerEnd:
		hook = "_trigger_end";
		break;
	}
	if (hook == nullptr) {
		return;
	}

	Entity targets[] = {event.colliderA, event.colliderB, event.bodyA, event.bodyB};
	for (i32 i = 0; i < 4; ++i) {
		if (_world.FixedTicksCancelled()) {
			return;
		}
		const Entity self = targets[i];
		if (!self.IsAlive()) {
			continue;
		}
		bool seen = false;
		for (i32 j = 0; j < i; ++j) {
			if (targets[j] == self) {
				seen = true;
				break;
			}
		}
		if (seen) {
			continue;
		}

		const Entity other =
			(self == event.colliderA || self == event.bodyA) ? event.colliderB : event.colliderA;
		for (auto &[typeName, proto] : _impl->prototypes) {
			(void)proto;
			ComponentType type = _world.FindComponent(typeName);
			if (!type.IsValid() || !self.Has(type)) {
				continue;
			}
			if (_world.FixedTicksCancelled()) {
				return;
			}
			callCollisionHook(typeName, hook, self, type, other);
		}
	}
}

void ScriptServer::callCollisionHook(const std::string &typeName, const char *hook, Entity entity,
	ComponentType type, Entity other) {
	auto it = _impl->prototypes.find(typeName);
	if (it == _impl->prototypes.end()) {
		return;
	}
	sol::object object = it->second[hook];
	if (object.get_type() != sol::type::function) {
		return;
	}
#if SOWA_PROFILER
	const ProfileScopeGuard profileScope(Profiler::Main(), hookProfileName(typeName, hook));
#endif
	sol::protected_function fn = object;
	const u64 abortGeneration = Debugger().AbortGeneration();
	sol::protected_function_result result = fn(ComponentView{entity, type}, other);
	if (Debugger().AbortGeneration() != abortGeneration) {
		_world.CancelFixedTicks();
		return;
	}
	if (result.valid()) {
		return;
	}
	const sol::error err = result;
	Log::Error("Lua error in {} {}: {}", typeName, hook, err.what());
}

sol::table ScriptServer::extrasTable(Entity entity, ComponentType type, bool create) {
	const std::string key = ExtraKey(entity, type);
	auto it = _impl->extras.find(key);
	if (it != _impl->extras.end()) {
		return it->second;
	}
	if (!create) {
		return sol::table{};
	}
	sol::table table = _impl->lua.create_table();
	_impl->extras.emplace(key, table);
	return table;
}

void ScriptServer::clearExtras(Entity entity, ComponentType type) {
	_impl->extras.erase(ExtraKey(entity, type));
}

sol::object ScriptServer::IndexComponent(Entity entity, ComponentType type, std::string_view key,
	sol::this_state s) {
	if (key == "entity") {
		return sol::make_object(s, entity);
	}

	sol::table extras = extrasTable(entity, type, false);
	if (extras.valid()) {
		sol::object extra = extras[std::string(key)];
		if (extra.valid() && extra.get_type() != sol::type::lua_nil) {
			return extra;
		}
	}

	auto it = _impl->prototypes.find(type.Name());
	if (it != _impl->prototypes.end()) {
		sol::object method = it->second[std::string(key)];
		if (method.valid() && method.get_type() != sol::type::lua_nil) {
			return method;
		}
	}
	return sol::make_object(s, sol::lua_nil);
}

void ScriptServer::NewIndexComponent(Entity entity, ComponentType type, std::string_view key,
	sol::object value) {
	if (key == "entity") {
		return;
	}
	sol::table extras = extrasTable(entity, type, true);
	extras[std::string(key)] = value;
}

void ScriptServer::ApplyDefaults(Entity entity, ComponentType type) {
	auto it = _impl->defaults.find(type.Name());
	if (it == _impl->defaults.end()) {
		return;
	}
	for (const FieldDefault &def : it->second) {
		WriteDefault(entity, type, def);
	}
}

} // namespace sowa
