#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "resource/resource_registry.hpp"
#include "scene/scene.hpp"
#include <unordered_set>

namespace sowa {

class FileSystem;
class ResourceRegistry;
class World;

enum class PlayKind {
	Stopped,
	Project,
	Current
};

// Manages loaded scenes and play sessions. Open/Close/Change/Play/Stop apply at Flush;
// Create and SetCurrent are immediate. Play freezes editor documents and runs separate
// instances.
class SceneManager {
  public:
	SceneManager(World &world, ResourceRegistry &resources, FileSystem &fs);
	~SceneManager();

	// Editor may refuse replacement of a document with unsaved edits.
	void SetReloadGuard(std::function<bool(Scene)> guard) {
		_reloadGuard = std::move(guard);
	}

	SceneManager(const SceneManager &) = delete;
	SceneManager &operator=(const SceneManager &) = delete;
	SceneManager(SceneManager &&) = delete;
	SceneManager &operator=(SceneManager &&) = delete;

	// The designated scene, or invalid if there isn't one (nothing loaded,
	// or Current was Closed). A current whose root has since been destroyed
	// without Close also reads as invalid.
	Scene Current() const;

	// Selects an already-loaded top-level scene without changing enabled state. Null
	// clears current. Rejects nested/dead roots and frozen editor documents during Play.
	bool SetCurrent(Scene scene);

	// Loaded top-level scenes, including disabled ones; excludes snapshot and prefab roots.
	std::vector<Scene> Loaded() const;

	// Scene documents owned by the editor session. While stopped this is the
	// same as Loaded(). During play it is the frozen pre-play list, excluding
	// the runtime play instance, autoloads, and scenes opened by gameplay.
	// Editor panels use this to keep their document UI stable during play.
	std::vector<Scene> EditorScenes() const;

	// Running game scenes, including autoloads, excluding frozen editor documents.
	// Empty while stopped.
	std::vector<Scene> RuntimeScenes() const;

	// The editor document selected before play began. While stopped this is
	// Current(); during play it remains the frozen editor current rather than
	// exposing the separate runtime play instance to editor document panels.
	Scene EditorCurrent() const;

	// By root name, among loaded scenes only. Invalid if missing or nested.
	Scene Find(std::string_view name) const;

	// First loaded scene whose SourcePath is path. Several Opens of the
	// same file are several scenes; this returns the oldest. Invalid if none.
	Scene FindByPath(std::string_view path) const;

	// Empty scene. Does not become current. Immediate - not a session
	// transition, so it is not queued.
	Scene Create(const std::string &name);

	// Queue an additive load from file. Always a new root; does not become
	// current. The file is not read until Flush. A missing file is logged
	// then, and Loaded() is unchanged.
	void Open(std::string_view path);

	// Queues unloading until Flush. Closing current clears it; no successor is selected.
	// Rejects frozen editor documents during Play.
	void Close(Scene scene);

	// Queues replacement of current. Flush loads the new scene before releasing the old
	// one to preserve shared resources. A failed load leaves current unchanged; without
	// a current scene this opens one. Only the last queued Change applies. Other loaded
	// scenes and frozen editor documents remain intact.
	void Change(std::string_view path);

	// Play snapshots and disables loaded editor documents, then runs a separate instance.
	// PlayProject loads saved data; PlayCurrent duplicates the current scene including
	// unsaved edits. Neither saves. Play while playing is refused; restart with Stop
	// then Play in one queue. Stop while stopped is a no-op.

	PlayKind PlayMode() const {
		return _playKind;
	}

	bool IsPlaying() const {
		return _playKind != PlayKind::Stopped;
	}

	void PlayProject(std::string_view path);
	void PlayCurrent();
	void Stop();

	// Pause and Resume apply immediately to a running play session. Application reads
	// GetGameTime once per frame, so a mid-frame change affects the next frame.
	// Pause while stopped is a no-op; Stop clears pause state.
	void Pause();
	void Resume();

	bool IsPaused() const {
		return _paused;
	}

	// Requests one frame of game time while paused. Ignored unless paused.
	// Application calls TakeStep once per frame; it returns true at most once
	// per request, and false if the session was resumed or stopped since.
	void Step();
	bool TakeStep();

	// Whether Stop is queued. A suspended Lua call must unwind before it can be applied.
	bool IsStopPending() const;
	// True while a PlayProject/PlayCurrent is queued and not yet applied.
	bool IsPlayPending() const;

	// Set while ScriptSuspension holds a breakpoint. Frame Step is unavailable because
	// the interrupted frame has not finished.
	void SetScriptSuspended(bool suspended) {
		_scriptSuspended = suspended;
	}

	bool IsScriptSuspended() const {
		return _scriptSuspended;
	}

	// Application-owned session data (global store, timers/tweens) lives
	// outside SceneManager's ECS/resource snapshot. These callbacks fire only
	// after a play transition succeeds and after Stop finishes restoring the
	// editor scenes. Failed/refused play requests fire neither callback.
	void SetPlaySessionCallbacks(std::function<void()> onStarted, std::function<void()> onStopped);

	// Scene files loaded at Play (after editor documents are recorded so they
	// are not frozen), surviving Change, closed on Stop. Set from
	// ProjectSettings::application.autoloads.
	void SetAutoloadPaths(std::vector<std::string> paths);

	const std::vector<std::string> &AutoloadPaths() const {
		return _autoloads;
	}

	// Apply every queued Open / Close / Change / Play / Stop, in order.
	// Application calls this before World::Progress. Safe to call with an
	// empty queue.
	void Flush();

	// Discards queued work and unloads all scenes without restoring a play session.
	void Clear();

  private:
	void applyHotReload();
	bool isLoaded(Scene scene) const;
	bool isFrozen(Scene scene) const;
	void applyClose(Scene scene);
	void applyChange(std::string_view path);
	void applyPlayProject(std::string_view path);
	void applyPlayCurrent();
	void applyStop();

	void recordEditorSession();
	bool snapshotEditorSession();
	void disableEditorSession();
	void discardEditorSession();
	void retainCopiedOwnership(Scene scene);
	bool loadAutoloads();
	void closeUnfrozenLoaded();

	struct PendingOp {
		enum class Kind {
			Open,
			Close,
			Change,
			PlayProject,
			PlayCurrent,
			Stop
		};
		Kind kind;
		std::string path;
		Scene scene;
	};

	struct FrozenScene {
		Scene scene;
		bool enabled = true;
	};

	std::shared_ptr<FileSystemEvents> _fileEvents;
	FileSystemEvents::Paths::Handle _fileChanged;
	ResourceRegistry::ReloadEvents::Handle _resourceReloaded;
	std::unordered_set<std::string> _changedFiles;
	std::unordered_set<ResourceID> _changedPacked;
	std::function<bool(Scene)> _reloadGuard;
	World &_world;
	ResourceRegistry &_resources;
	FileSystem &_fs;
	Scene _current;
	std::vector<PendingOp> _queue;
	PlayKind _playKind = PlayKind::Stopped;
	bool _paused = false;
	bool _stepRequested = false;
	bool _scriptSuspended = false;
	std::vector<FrozenScene> _frozen;
	Scene _frozenCurrent;
	std::vector<std::string> _autoloads;
	std::function<void()> _onPlayStarted;
	std::function<void()> _onPlayStopped;
};

} // namespace sowa
