#pragma once

#include <chrono>
#include <list>
#include <string>
#include <string_view>

#include "audio/audio_device.hpp"
#include "audio/audio_mixer.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/frame_clock.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "core/timer.hpp"
#include "core/tween.hpp"
#include "core/types.hpp"
#include "core/window.hpp"
#include "ecs/entity_pick_map.hpp"
#include "ecs/world.hpp"
#include "physics/physics_world.hpp"
#include "render/fullscreen_present.hpp"
#include "render/graphics_device.hpp"
#include "render/renderer2d.hpp"
#include "render/sprite_renderer2d.hpp"
#include "resource/audio_clip.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"
#include "script/script_suspension.hpp"
#include "script/script_watchdog.hpp"
#if SOWA_DEBUGGER
#include "script/dap_server.hpp"
#include "script/dap_session.hpp"
#endif
#include "ui/ui_system.hpp"

#if SOWA_EDITOR
#include "editor/editor.hpp"
#endif

namespace sowa {

// Owns the runtime session and frame loop.
class Application {
  public:
	Application() = default;
	~Application();

	Application(const Application &) = delete;
	Application &operator=(const Application &) = delete;
	Application(Application &&) = delete;
	Application &operator=(Application &&) = delete;

	// Call before Init. When true, Init starts ImGui docking chrome and
	// does not auto-play the main scene (edit session).
	void SetEditorEnabled(bool enabled) {
		_editorEnabled = enabled;
	}

	bool IsEditorEnabled() const {
		return _editorEnabled;
	}

	// A failed Init releases everything acquired during startup.
	bool Init(i32 width, i32 height, std::string_view title);

	bool IsInitialized() const {
		return _initialized;
	}

	// Polls input, advances the game, and renders one frame.
	void Update();

	void Shutdown();

	bool ShouldClose() const;

	f32 Delta() const {
		return _delta;
	}

	// Set debug adapter options before Init. Builds with SOWA_ENABLE_DEBUGGER disabled
	// warn that these options are ignored.
	static constexpr u16 kDefaultDebugPort = 6510;

	struct DebugAdapterOptions {
		u16 port = kDefaultDebugPort;
		// Hold game scripts until VS Code has attached and configured:
		// before the project loads without an editor, before the first Play
		// in the editor.
		bool waitForDebugger = false;
	};

	void SetDebugAdapterOptions(DebugAdapterOptions options) {
		_debugOptions = options;
	}

	// Restarts delta timing, so the next frame does not count time spent
	// suspended (for example a hidden browser tab).
	void ResetFrameClock() {
		_frameClock.Rebase();
	}

	Window &GetWindow() {
		return _window;
	}

	Input &GetInput() {
		return _input;
	}

	GraphicsDevice &GetGraphicsDevice() {
		return *_graphicsDevice;
	}

	// Null if CreateAudioDevice failed. A non-null device can still be
	// uninitialized: audio Init is fail-soft so a missing output device
	// does not abort the game.
	AudioDevice *GetAudioDevice() {
		return _audioDevice.get();
	}

	const AudioDevice *GetAudioDevice() const {
		return _audioDevice.get();
	}

	AudioMixer &GetAudioMixer() {
		return _audioMixer;
	}

	const AudioMixer &GetAudioMixer() const {
		return _audioMixer;
	}

	PhysicsWorld &GetPhysicsWorld() {
		return _physics;
	}

	const PhysicsWorld &GetPhysicsWorld() const {
		return _physics;
	}

	Renderer2D &GetRenderer2D() {
		return _renderer2d;
	}

	FileSystem &GetFileSystem() {
		return _fileSystem;
	}

	ProjectSettings &GetProjectSettings() {
		return _projectSettings;
	}

	const ProjectSettings &GetProjectSettings() const {
		return _projectSettings;
	}

	StringStore &GetGlobalStore() {
		return _globalStore;
	}

	const StringStore &GetGlobalStore() const {
		return _globalStore;
	}

	World &GetWorld() {
		return _world;
	}

	// The topmost entity written beneath the cursor by the previous completed
	// game pass. Updated every frame in both run and edit modes.
	Entity GetHoveredEntity() const {
		return _hoveredEntity.IsAlive() ? _hoveredEntity : Entity{};
	}

	// Pick token for custom geometry submitted in the current frame.
	u32 GetEntityPickToken(Entity entity) {
		return _entityPicks.TokenFor(entity);
	}

	ResourceRegistry &GetResourceRegistry() {
		return _resources;
	}

	SceneManager &GetSceneManager() {
		return _scenes;
	}

	const SceneManager &GetSceneManager() const {
		return _scenes;
	}

	ScriptServer &GetScriptServer() {
		return _scripts;
	}

	const ScriptServer &GetScriptServer() const {
		return _scripts;
	}

	std::list<Timer> &GetTimers() {
		return _scripts.Timers();
	}

	Timer *NewTimer(f32 timeout, bool autoStart = false, bool repeating = false) {
		return _scripts.NewTimer(timeout, autoStart, repeating);
	}

	std::list<Tween> &GetTweens() {
		return _scripts.Tweens();
	}

	Tween *NewTween(f32 duration, bool autoStart = true) {
		return _scripts.NewTween(duration, autoStart);
	}

  private:
#if SOWA_EDITOR
	void applyPendingProjectSettings();
	bool saveAndApplyProjectSettings(ProjectSettings settings, std::string &status);
#endif
	// True while the editor's free camera, not a scene camera, frames the view.
	bool usesEditorView() const;
	bool loadProjectScene();
	glm::mat4 sceneViewProjection(Vector2 viewportSize) const;
	void drawScene();
	// Polls window input, starts the editor frame and maps input to the game
	// viewport. Returns the game image's rect in the framebuffer.
	Rect beginFrameInput(bool &editorKeyboard);
	// Shows the game target: in the editor's viewport panel, or blitted to
	// the window in run mode. Ends the editor frame.
	void presentFrame(Rect presentRect, Vector2 gameSize);
	// Handles Lua debugger stops.
	LuaResumeAction suspendForDebugger(const LuaStopEvent &event);
	// One frame while suspended: input, editor and the frozen game image,
	// nothing that advances or reloads the game. False when closing.
	bool pumpSuspendedFrame();
	// One frame of input, adapter messages and the game image, with nothing
	// that advances or reloads the game. sessionKeys handles F7/F8 (while
	// suspended). False when the window is closing.
	bool pumpFrame(bool sessionKeys);
	void startDebugAdapter();
	void pollDebugAdapter();
	bool debuggerAttached() const;
	void waitForDebugger();
	// Keeps the main thread responsive while scripts run; see ScriptWatchdog.
	void beatWatchdog();
#if SOWA_PROFILER
	void reportProfileCounters();
#endif

	Window _window;
	Input _input;
	Input::EventCallbackList::Handle _uiInputHandle;
	Scope<GraphicsDevice> _graphicsDevice;
	Scope<AudioDevice> _audioDevice;
	Renderer2D _renderer2d;
	FullscreenPresent _present;
	RenderTargetHandle _gameTarget;
	FileSystem _fileSystem;
	ProjectSettings _projectSettings;
	StringStore _globalStore;
	StringStore _globalStoreSnapshot;
	World _world;
	EntityPickMap _entityPicks;
	Entity _hoveredEntity;
	// Components store ResourceIDs, so resource and world teardown order is independent.
	ResourceRegistry _resources;
	SceneManager _scenes{_world, _resources, _fileSystem};
	// Destroyed before SceneManager and World: Lua globals hold pointers to
	// both, and userdata holds Entity handles, not owners.
	ScriptServer _scripts{_world};
	// Owns sprite collection, sorting, and submission; depends on the renderer.
	SpriteRenderer2D _spriteRenderer;
	UISystem _ui;
	// After World: Query must not outlive it. After the device in Shutdown
	// the callback is already stopped, so Mix cannot run during teardown.
	AudioMixer _audioMixer;
	// After World: PhysicsStep is a flecs system. Shutdown destroys it
	// while the world is still alive.
	PhysicsWorld _physics;
#if SOWA_EDITOR
	// After Window + GraphicsDevice: ImGui backends need both. Declared last
	// so ~Editor runs first and tears down GL objects while the context lives.
	Editor _editor;
#endif
	bool _editorEnabled = false;
	bool _initialized = false;

	// Declared after _scenes and _audioMixer, which it pauses.
	ScriptSuspension _suspension{_scenes, _audioMixer};
	// Editor watchdog: pauses a script that holds a frame for three seconds. Declared
	// after _scripts so its thread joins before the debugger it calls is destroyed.
	ScriptWatchdog _watchdog{std::chrono::seconds(3),
		[this] { _scripts.Debugger().RequestPause(); }};

	DebugAdapterOptions _debugOptions;
	bool _waitingForDebugger = false;
#if SOWA_DEBUGGER
	DapServer _dapServer;
	// After _scripts: holds the debugger by reference.
	Scope<DapSession> _dapSession;
#endif

	FrameClock _frameClock;
	f32 _delta = 0.f;
};

} // namespace sowa
