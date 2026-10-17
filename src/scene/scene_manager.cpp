#include "scene/scene_manager.hpp"

#include <algorithm>
#include <utility>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/scope_exit.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"

namespace sowa {

SceneManager::SceneManager(World &world, ResourceRegistry &resources, FileSystem &fs)
	: _world(world), _resources(resources), _fs(fs) {
	_fileEvents = fs.Events();
	_fileChanged = _fileEvents->changed.append(
		[this](const std::string &path) { _changedFiles.insert(path); });
	_resourceReloaded = resources.Reloaded().append([this](ResourceID id) {
		if (_resources.Get<PackedScene>(id)) {
			_changedPacked.insert(id);
		}
	});
}

SceneManager::~SceneManager() {
	_fileEvents->changed.remove(_fileChanged);
	_resources.Reloaded().remove(_resourceReloaded);
}

void SceneManager::applyHotReload() {
	// Keep the snapshot and runtime trees untouched. Apply once Stop has
	// restored the editor session, even if the changes arrived during play.
	if (IsPlaying() || (_changedFiles.empty() && _changedPacked.empty())) {
		return;
	}

	struct Reference {
		Entity holder;
		ComponentType type;
		std::string field;
		std::string targetPath;
	};

	std::vector<Reference> references;
	std::function<void(Entity)> capture = [&](Entity entity) {
		for (const auto &type : entity.GetComponents()) {
			for (const auto &field : type.Fields()) {
				if (field.type != FieldType::EntityRef) {
					continue;
				}
				const auto *target = entity.GetField(type, field.name).As<Entity>();
				if (target && target->IsAlive()) {
					references.push_back({entity, type, field.name, target->GetPath()});
				}
			}
		}
		for (Entity child : entity.GetChildren()) {
			capture(child);
		}
	};
	for (Scene scene : EditorScenes()) {
		capture(scene.Root());
	}
	auto restoreReferences = MakeScopeExit([&] {
		for (auto &ref : references) {
			if (!ref.holder.IsAlive()) {
				continue;
			}
			FieldRef field = ref.holder.GetField(ref.type, ref.field);
			if (const auto *target = field.As<Entity>(); target && !target->IsAlive()) {
				field.Set(_world.Find(ref.targetPath));
			}
		}
	});
	auto files = std::exchange(_changedFiles, {});
	for (Scene scene : EditorScenes()) {
		if (!files.contains(_fs.Normalize(scene.SourcePath()))) {
			continue;
		}
		// Compare bytes before consulting the dirty guard: our own saves
		// generate native events too, and must not reset history or selection.
		const auto file = _fs.Load(scene.SourcePath());
		if (!file || !file.value) {
			continue;
		}
		const std::string text(reinterpret_cast<const char *>(file.value->Data()),
			file.value->Size());
		if (text == scene.Root().Get<SceneRoot>()->diskContent) {
			continue;
		}
		if (_reloadGuard && !_reloadGuard(scene)) {
			Log::Warn("'{}' changed on disk; keeping unsaved editor edits", scene.SourcePath());
			continue;
		}
		if (ReloadSceneFromFile(_fs, scene, _world, _resources)) {
			Log::Info("Reloaded scene '{}'", scene.SourcePath());
		}
	}
	auto packed = std::exchange(_changedPacked, {});
	if (packed.empty()) {
		return;
	}
	// Collect before mutation; refresh can replace nested instances. Walking
	// editor trees includes inactive tabs and excludes undo/snapshot copies.
	std::vector<Entity> instances;
	std::function<void(Entity)> collect = [&](Entity entity) {
		if (const auto *instance = entity.Get<SceneInstance>();
			instance && packed.contains(instance->packed)) {
			instances.push_back(entity);
		}
		for (Entity child : entity.GetChildren()) {
			collect(child);
		}
	};
	for (Scene scene : EditorScenes()) {
		collect(scene.Root());
	}
	for (Entity entity : instances) {
		if (!entity.IsAlive()) {
			continue;
		}
		const auto *instance = entity.Get<SceneInstance>();
		if (!instance) {
			continue;
		}
		if (auto resource = _resources.Get<PackedScene>(instance->packed)) {
			resource->RefreshInstance(_world, _resources, entity);
		}
	}
}

Scene SceneManager::Current() const {
	return _current.IsValid() ? _current : Scene();
}

bool SceneManager::SetCurrent(Scene scene) {
	if (!scene.IsValid()) {
		// Default-constructed only: a destroyed handle still has a non-zero
		// root id, and treating that as "clear" would hide a caller bug.
		if (scene.Root() == Entity()) {
			_current = Scene();
			return true;
		}
		Log::Error("SceneManager::SetCurrent: scene is not valid");
		return false;
	}

	if (!isLoaded(scene)) {
		Log::Error("SceneManager::SetCurrent: '{}' is not a loaded scene", scene.Name());
		return false;
	}

	if (_playKind != PlayKind::Stopped && isFrozen(scene)) {
		Log::Error("SceneManager::SetCurrent: '{}' is a frozen editor scene", scene.Name());
		return false;
	}

	_current = scene;
	return true;
}

std::vector<Scene> SceneManager::Loaded() const {
	return GetScenes(_world);
}

std::vector<Scene> SceneManager::EditorScenes() const {
	if (_playKind == PlayKind::Stopped) {
		return Loaded();
	}

	std::vector<Scene> scenes;
	scenes.reserve(_frozen.size());
	for (const FrozenScene &entry : _frozen) {
		if (entry.scene.IsValid()) {
			scenes.push_back(entry.scene);
		}
	}
	return scenes;
}

std::vector<Scene> SceneManager::RuntimeScenes() const {
	std::vector<Scene> scenes;
	if (_playKind == PlayKind::Stopped) {
		return scenes;
	}
	for (Scene scene : Loaded()) {
		if (!isFrozen(scene)) {
			scenes.push_back(scene);
		}
	}
	return scenes;
}

Scene SceneManager::EditorCurrent() const {
	if (_playKind == PlayKind::Stopped) {
		return Current();
	}
	return _frozenCurrent.IsValid() ? _frozenCurrent : Scene();
}

Scene SceneManager::Find(std::string_view name) const {
	Scene scene = FindScene(_world, std::string(name));
	return isLoaded(scene) ? scene : Scene();
}

Scene SceneManager::FindByPath(std::string_view path) const {
	if (path.empty()) {
		return Scene();
	}

	for (Scene scene : Loaded()) {
		if (scene.SourcePath() == path) {
			return scene;
		}
	}
	return Scene();
}

Scene SceneManager::Create(const std::string &name) {
	return CreateScene(_world, name);
}

void SceneManager::Open(std::string_view path) {
	_queue.push_back({PendingOp::Kind::Open, std::string(path), Scene()});
}

void SceneManager::Close(Scene scene) {
	_queue.push_back({PendingOp::Kind::Close, {}, scene});
}

void SceneManager::Change(std::string_view path) {
	_queue.erase(std::remove_if(_queue.begin(), _queue.end(),
					 [](const PendingOp &op) { return op.kind == PendingOp::Kind::Change; }),
		_queue.end());
	_queue.push_back({PendingOp::Kind::Change, std::string(path), Scene()});
}

void SceneManager::PlayProject(std::string_view path) {
	_queue.push_back({PendingOp::Kind::PlayProject, std::string(path), Scene()});
}

void SceneManager::PlayCurrent() {
	_queue.push_back({PendingOp::Kind::PlayCurrent, {}, Scene()});
}

void SceneManager::Stop() {
	_queue.push_back({PendingOp::Kind::Stop, {}, Scene()});
}

void SceneManager::Pause() {
	if (IsPlaying()) {
		_paused = true;
	}
}

void SceneManager::Resume() {
	_paused = false;
	_stepRequested = false;
}

void SceneManager::Step() {
	if (_paused) {
		_stepRequested = true;
	}
}

bool SceneManager::IsStopPending() const {
	return std::any_of(_queue.begin(), _queue.end(),
		[](const PendingOp &op) { return op.kind == PendingOp::Kind::Stop; });
}

bool SceneManager::IsPlayPending() const {
	return std::any_of(_queue.begin(), _queue.end(), [](const PendingOp &op) {
		return op.kind == PendingOp::Kind::PlayProject || op.kind == PendingOp::Kind::PlayCurrent;
	});
}

bool SceneManager::TakeStep() {
	const bool step = _stepRequested;
	_stepRequested = false;
	return step;
}

void SceneManager::SetAutoloadPaths(std::vector<std::string> paths) {
	_autoloads = std::move(paths);
}

bool SceneManager::loadAutoloads() {
	for (const std::string &path : _autoloads) {
		if (path.empty()) {
			continue;
		}
		Scene loaded = LoadSceneFromFile(_fs, path, _world, _resources);
		if (!loaded.IsValid()) {
			Log::Error("SceneManager: failed to load autoload '{}'", path);
			return false;
		}
		loaded.SetEnabled(true);
	}
	return true;
}

void SceneManager::closeUnfrozenLoaded() {
	std::vector<Scene> loaded = Loaded();
	for (Scene scene : loaded) {
		if (!isFrozen(scene)) {
			applyClose(scene);
		}
	}
}

void SceneManager::Flush() {
	std::vector<PendingOp> ops;
	ops.swap(_queue);
	for (PendingOp &op : ops) {
		switch (op.kind) {
		case PendingOp::Kind::Open:
			LoadSceneFromFile(_fs, op.path, _world, _resources);
			break;
		case PendingOp::Kind::Close:
			applyClose(op.scene);
			break;
		case PendingOp::Kind::Change:
			applyChange(op.path);
			break;
		case PendingOp::Kind::PlayProject:
			applyPlayProject(op.path);
			break;
		case PendingOp::Kind::PlayCurrent:
			applyPlayCurrent();
			break;
		case PendingOp::Kind::Stop:
			applyStop();
			break;
		}
	}
	applyHotReload();
}

void SceneManager::applyClose(Scene scene) {
	if (_playKind != PlayKind::Stopped && isFrozen(scene)) {
		Log::Error("SceneManager::Close: '{}' is a frozen editor scene",
			scene.IsValid() ? scene.Name() : std::string());
		return;
	}

	const bool wasCurrent =
		_current.IsValid() && scene.IsValid() && _current.Root() == scene.Root();
	UnloadScene(scene, _resources);
	if (wasCurrent) {
		_current = Scene();
	}
}

void SceneManager::Clear() {
	discardEditorSession();
	for (Scene scene : Loaded()) {
		UnloadScene(scene, _resources);
	}
	_current = {};
	_queue.clear();
	_changedFiles.clear();
	_changedPacked.clear();
	_paused = false;
	_stepRequested = false;
	_scriptSuspended = false;
}

void SceneManager::applyChange(std::string_view path) {
	// Load first so shared resources gain the incoming owner before the outgoing scene releases
	// them.
	Scene incoming = LoadSceneFromFile(_fs, path, _world, _resources);
	if (!incoming.IsValid()) {
		return;
	}

	Scene outgoing = _current;
	if (!SetCurrent(incoming)) {
		UnloadScene(incoming, _resources);
		return;
	}
	if (outgoing.IsValid() && outgoing.Root() != incoming.Root()) {
		UnloadScene(outgoing, _resources);
	}
}

void SceneManager::applyPlayProject(std::string_view path) {
	if (_playKind != PlayKind::Stopped) {
		Log::Error("SceneManager::PlayProject: already playing");
		return;
	}
	if (path.empty()) {
		Log::Error("SceneManager::PlayProject: start scene path is empty");
		return;
	}

	recordEditorSession();

	if (!loadAutoloads()) {
		closeUnfrozenLoaded();
		_frozen.clear();
		_frozenCurrent = Scene();
		return;
	}

	Scene play = LoadSceneFromFile(_fs, path, _world, _resources);
	if (!play.IsValid()) {
		closeUnfrozenLoaded();
		_frozen.clear();
		_frozenCurrent = Scene();
		return;
	}
	play.SetEnabled(true);

	if (!snapshotEditorSession()) {
		UnloadScene(play, _resources);
		closeUnfrozenLoaded();
		discardEditorSession();
		return;
	}
	disableEditorSession();

	if (!SetCurrent(play)) {
		UnloadScene(play, _resources);
		closeUnfrozenLoaded();
		discardEditorSession();
		return;
	}
	_playKind = PlayKind::Project;
	if (_onPlayStarted) {
		_onPlayStarted();
	}
}

void SceneManager::applyPlayCurrent() {
	if (_playKind != PlayKind::Stopped) {
		Log::Error("SceneManager::PlayCurrent: already playing");
		return;
	}

	Scene current = Current();
	if (!current.IsValid()) {
		Log::Error("SceneManager::PlayCurrent: no current scene");
		return;
	}

	recordEditorSession();

	if (!loadAutoloads()) {
		closeUnfrozenLoaded();
		_frozen.clear();
		_frozenCurrent = Scene();
		return;
	}

	// Duplicate before TakeSnapshot so the play instance does not inherit
	// the SceneSnapshot component. Play-in-place would let Change unload
	// the restore point.
	Entity copy = current.Root().Duplicate();
	if (!copy.IsAlive()) {
		Log::Error("SceneManager::PlayCurrent: failed to duplicate '{}'", current.Name());
		closeUnfrozenLoaded();
		_frozen.clear();
		_frozenCurrent = Scene();
		return;
	}

	Scene play(copy);
	// Duplicate copies SceneResources without Retain. Without this the play
	// instance's Unload would Release ids it never claimed and Stop would
	// drop the editor's resource.
	retainCopiedOwnership(play);
	play.SetEnabled(true);

	if (!snapshotEditorSession()) {
		UnloadScene(play, _resources);
		closeUnfrozenLoaded();
		discardEditorSession();
		return;
	}
	disableEditorSession();

	if (!SetCurrent(play)) {
		UnloadScene(play, _resources);
		closeUnfrozenLoaded();
		discardEditorSession();
		return;
	}
	_playKind = PlayKind::Current;
	if (_onPlayStarted) {
		_onPlayStarted();
	}
}

void SceneManager::applyStop() {
	if (_playKind == PlayKind::Stopped) {
		return;
	}

	std::vector<Scene> loaded = Loaded();
	for (Scene scene : loaded) {
		if (!isFrozen(scene)) {
			applyClose(scene);
		}
	}

	Scene restoredCurrent;
	for (FrozenScene &entry : _frozen) {
		const bool wasCurrent = _frozenCurrent.IsValid() && entry.scene.IsValid() &&
								entry.scene.Root() == _frozenCurrent.Root();
		if (!RestoreSnapshot(entry.scene, _resources)) {
			if (entry.scene.IsValid()) {
				entry.scene.SetEnabled(entry.enabled);
			}
			continue;
		}
		entry.scene.SetEnabled(entry.enabled);
		if (wasCurrent) {
			restoredCurrent = entry.scene;
		}
	}

	_frozen.clear();
	_frozenCurrent = Scene();
	_playKind = PlayKind::Stopped;
	_paused = false;
	_stepRequested = false;

	if (restoredCurrent.IsValid()) {
		SetCurrent(restoredCurrent);
	} else {
		SetCurrent(Scene());
	}

	if (_onPlayStopped) {
		_onPlayStopped();
	}
}

void SceneManager::SetPlaySessionCallbacks(std::function<void()> onStarted,
	std::function<void()> onStopped) {
	_onPlayStarted = std::move(onStarted);
	_onPlayStopped = std::move(onStopped);
}

void SceneManager::recordEditorSession() {
	_frozen.clear();
	_frozenCurrent = _current;
	for (Scene scene : Loaded()) {
		_frozen.push_back({scene, scene.IsEnabled()});
	}
}

bool SceneManager::snapshotEditorSession() {
	for (size_t i = 0; i < _frozen.size(); ++i) {
		if (!TakeSnapshot(_frozen[i].scene, _resources)) {
			for (size_t j = 0; j < i; ++j) {
				DiscardSnapshot(_frozen[j].scene, _resources);
			}
			return false;
		}
	}
	return true;
}

void SceneManager::disableEditorSession() {
	for (FrozenScene &entry : _frozen) {
		entry.scene.SetEnabled(false);
	}
}

void SceneManager::discardEditorSession() {
	for (FrozenScene &entry : _frozen) {
		DiscardSnapshot(entry.scene, _resources);
		if (entry.scene.IsValid()) {
			entry.scene.SetEnabled(entry.enabled);
		}
	}
	_frozen.clear();
	_frozenCurrent = Scene();
	_playKind = PlayKind::Stopped;
}

void SceneManager::retainCopiedOwnership(Scene scene) {
	for (const ResourceID id : scene.OwnedResources()) {
		if (!_resources.Retain(id)) {
			Log::Error("SceneManager: failed to retain copied resource {}", id.value);
		}
	}
}

bool SceneManager::isLoaded(Scene scene) const {
	if (!scene.IsValid()) {
		return false;
	}
	for (Scene loaded : Loaded()) {
		if (loaded.Root() == scene.Root()) {
			return true;
		}
	}
	return false;
}

bool SceneManager::isFrozen(Scene scene) const {
	if (!scene.IsValid()) {
		return false;
	}
	for (const FrozenScene &entry : _frozen) {
		if (entry.scene.IsValid() && entry.scene.Root() == scene.Root()) {
			return true;
		}
	}
	return false;
}

} // namespace sowa
