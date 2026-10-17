#include "core/application.hpp"
#include "animation/animation.hpp"
#include "script/lua_ui.hpp"

#include <string>

#include "core/filesystem/folder_file_server.hpp"
#include "core/game_time.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include "core/scope_exit.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/transform2d.hpp"
#include "math/color.hpp"
#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "particles/particles2d.hpp"
#include "render/viewport_scale.hpp"
#include "resource/audio_clip.hpp"
#include "resource/font.hpp"
#include "resource/image_texture.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include "scene/packed_scene.hpp"

#if SOWA_EDITOR
#include "editor/camera.hpp"
#include "editor/play_controls.hpp"
#include "editor/project_settings.hpp"
#include "editor/viewport.hpp"
#endif

namespace sowa {
namespace {

[[maybe_unused]] std::string projectWindowTitle(const ProjectSettings &settings,
	bool editorEnabled) {
	std::string title =
		settings.application.name.empty() ? std::string("Sowa Engine") : settings.application.name;
	if (editorEnabled) {
		title += " [Editor]";
	}
	return title;
}

} // namespace

Application::~Application() {
	Shutdown();
}

bool Application::Init(i32 width, i32 height, std::string_view title) {
	if (_initialized) {
		Log::Error("Application is already initialized");
		return false;
	}
	auto rollback = MakeScopeExit([this] { Shutdown(); });
	_fileSystem.EnableHotReload(_editorEnabled);
	_globalStore = _projectSettings.globalStore;
	_input.SetActions(_projectSettings.input.actions);
	_graphicsDevice = CreateGraphicsDevice(GraphicsAPI::OpenGL);
	if (!_graphicsDevice) {
		Log::Error("Failed to create graphics device");
		return false;
	}

	if (!Window::EnsureGlfwInitialized()) {
		return false;
	}
	_graphicsDevice->ApplyWindowHints();

	if (!_window.Create(width, height, title)) {
		return false;
	}
	_window.SetInput(&_input);

	if (!_graphicsDevice->Init(_window)) {
		return false;
	}

	const auto &viewport = _projectSettings.rendering.viewport;
	RenderTargetDesc rtDesc;
	rtDesc.width = viewport.width;
	rtDesc.height = viewport.height;
	rtDesc.unsignedIntegerAttachment = true;
	_gameTarget = _graphicsDevice->CreateRenderTarget(rtDesc);
	if (!_gameTarget.IsValid()) {
		Log::Error("Failed to create game render target ({}x{})", rtDesc.width, rtDesc.height);
		return false;
	}

	if (!_renderer2d.Init(*_graphicsDevice, _fileSystem)) {
		return false;
	}
	_renderer2d.SetProjection(static_cast<f32>(viewport.width), static_cast<f32>(viewport.height));

	if (!_present.Init(*_graphicsDevice, _fileSystem)) {
		return false;
	}

	// Register concrete resource types here to keep ResourceRegistry independent of them.
	_resources.SetContext(ResourceContext{_graphicsDevice.get(), &_fileSystem, &_resources});
	_resources.RegisterType<ImageTexture>("ImageTexture")
		.Field("path", &ImageTexture::path)
		.Field("flipVertically", &ImageTexture::flipVertically)
		.Field("retainPixels", &ImageTexture::retainPixels)
		.Field("filter", &ImageTexture::filter);
	_resources.RegisterType<Font>("Font")
		.Field("path", &Font::path)
		.Field("pixelSize", &Font::pixelSize);
	_resources.RegisterType<AudioClip>("AudioClip").Field("path", &AudioClip::path);
	// Named arrays require custom serialization rather than reflected scalar fields.
	_resources.RegisterType<SpriteSheetAnimation>("SpriteSheetAnimation");
	_resources.RegisterType<AnimationLibrary>("AnimationLibrary");
	RegisterAnimationSystem(_world, _resources,
		[this](Entity target, const std::string &component, const std::string &method) {
			if (!_scripts.CallComponentMethod(target, component, method)) {
				Log::Warn("Animation method call failed: {} / {}.{}", target.GetPath(), component,
					method);
			}
		});
	_resources.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);

	const auto defaultFont = _resources.Create<Font>();
	if (!defaultFont || !_resources.Reload(defaultFont->Id()) || !defaultFont->IsLoaded()) {
		Log::Error("Failed to load the embedded default font");
		return false;
	}
	_spriteRenderer.Init(_world, _resources);
	_spriteRenderer.SetDefaultFont(defaultFont->Id());
	RegisterUITheme(_resources);
	_ui.Init(_world, _resources);
	_ui.SetDefaultFont(defaultFont->Id());
	_uiInputHandle =
		_input.Events().append([this](const InputEvent &event) { _ui.QueueInput(event); });
	_ui.SetClipboard([this] { return _window.GetClipboard(); },
		[this](const std::string &text) { _window.SetClipboard(text); });
	BindUI(_scripts, _ui);

	_audioMixer.Init(_world, _resources);
	_physics.Init(_world);
	// Audio output is optional; a missing device leaves the game silent.
	_audioDevice = CreateAudioDevice(AudioAPI::MiniAudio);
	if (_audioDevice == nullptr) {
		Log::Error("Failed to create audio device");
	} else {
		_audioDevice->SetMixer(&_audioMixer);
		if (!_audioDevice->Init(AudioDeviceConfig{})) {
			Log::Error("AudioDevice: playback is unavailable");
		} else {
			Log::Info("Audio {} Hz, {} ch", _audioDevice->SampleRate(),
				_audioDevice->ChannelCount());
		}
	}

	_scripts.BindEngine(_input, _scenes, _resources, _projectSettings, &_audioMixer, &_physics,
		&_globalStore);
	_scenes.SetPlaySessionCallbacks(
		[this]() {
			_physics.ResetSimulation();
			_globalStoreSnapshot = _globalStore;
			_ui.Reset();
		},
		[this]() {
			_ui.Reset();
			_physics.ResetSimulation();
			_globalStore = _globalStoreSnapshot;
			_globalStoreSnapshot.Clear();
			_scripts.ClearTimers();
			_scripts.ClearTweens();
		});
	_scenes.SetAutoloadPaths(_projectSettings.application.autoloads);

#if SOWA_EDITOR
	if (_editorEnabled) {
		if (!_editor.Init(_window, _fileSystem)) {
			Log::Error("Failed to initialize editor");
			return false;
		}
	}

	_scenes.SetReloadGuard([this](Scene scene) { return _editor.CanReloadScene(scene); });
	_editor.SetScriptSuspension(&_suspension);
#endif

	// Set up breakpoint suspension before loading project scripts so run mode can stop
	// during initialization, including when waiting for VS Code.
	_scripts.Debugger().SetStopHandler(
		[this](const LuaStopEvent &event) { return suspendForDebugger(event); });
	_suspension.SetSuspendOutsidePlay(!_editorEnabled);
	if (_editorEnabled) {
		_watchdog.Start();
	}
	startDebugAdapter();
	if (!_editorEnabled && _waitingForDebugger) {
		waitForDebugger();
	}

	if (!loadProjectScene()) {
		return false;
	}

	Log::Info("Viewport {}x{} ({})", viewport.width, viewport.height,
		ViewportScaleModeToString(viewport.scaleMode));

	_frameClock.Rebase();
	_delta = 0.f;
	_initialized = true;
	rollback.Release();
	return true;
}

bool Application::loadProjectScene() {
	if (!_projectSettings.application.scripts.empty()) {
		if (!_scripts.LoadModules(_fileSystem, _projectSettings.application.scripts)) {
			Log::Error("Failed to load project scripts");
			if (!_editorEnabled) {
				return false;
			}
		}
	}

	const std::string &mainScene = _projectSettings.application.mainScene;
	if (!mainScene.empty()) {
		_scenes.Open(mainScene);
		_scenes.Flush();
		if (Scene loaded = _scenes.FindByPath(mainScene); loaded.IsValid()) {
			_scenes.SetCurrent(loaded);
		} else {
			Log::Error("Failed to open main scene '{}'", mainScene);
		}
	}

	if (!_scenes.Current().IsValid()) {
		if (!_editorEnabled && mainScene.empty()) {
			Log::Error("No main scene is configured");
		}
		return _editorEnabled;
	}

#if SOWA_EDITOR
	if (_editorEnabled) {
		if (const Entity camera = _scenes.Current().ActiveCamera(); camera.IsAlive()) {
			Vector2 position(0.f, 0.f);
			Vector2 scale(1.f, 1.f);
			f32 rotation = 0.f;
			Matrix::DecomposeTransform2D(ComputeWorldMatrix(camera), &position, &rotation, &scale);
			_editor.SetCameraView(position, scale.x);
		}
		return true;
	}
#endif

	// Play creates the runtime tree; Lua _start runs on the first Progress.
	_scenes.PlayCurrent();
	_scenes.Flush();
	return _scenes.IsPlaying();
}

bool Application::usesEditorView() const {
#if SOWA_EDITOR
	return _editor.IsInitialized() && !_scenes.IsPlaying() &&
		   !(_editor.IsCameraPreviewEnabled() && _scenes.Current().ActiveCamera().IsAlive());
#else
	return false;
#endif
}

glm::mat4 Application::sceneViewProjection(Vector2 viewportSize) const {
#if SOWA_EDITOR
	if (usesEditorView()) {
		return editor::ViewProjection(_editor.Camera(), viewportSize);
	}
#endif
	const Entity camera = _scenes.Current().ActiveCamera();
	return camera.IsAlive() ? CalculateViewProjection(camera, viewportSize)
							: CalculateBlankViewProjection(viewportSize);
}

void Application::drawScene() {
	const auto &viewport = _projectSettings.rendering.viewport;
	const Vector2 viewportSize(viewport.width, viewport.height);
	_renderer2d.SetViewProjection(sceneViewProjection(viewportSize));

#if SOWA_EDITOR
	if (usesEditorView()) {
		_editor.DrawViewportGrid(_renderer2d, viewportSize);
	}
#endif
	// Resolve the view's camera origin at its anchor, including camera offset
	// and inherited transforms. Free editor panning leaves authored positions
	// in place; camera preview and play show the parallax motion.
	Vector2 parallaxCamera;
	const Entity camera = _scenes.Current().ActiveCamera();
	if (camera.IsAlive() && !usesEditorView()) {
		parallaxCamera = CalculateCameraOrigin(camera, _renderer2d.ViewProjection());
	}
	_spriteRenderer.Draw(_renderer2d, _entityPicks, parallaxCamera);

	if (_physics.DebugDrawEnabled()) {
		_renderer2d.Begin();
		_physics.DebugDraw(
			[&](Vector2 a, Vector2 b, Color color) { _renderer2d.DrawLine(a, b, color); });
		_renderer2d.End();
	}

	_ui.Draw(_renderer2d, _entityPicks);
}

#if SOWA_PROFILER
namespace {

std::size_t countSubtree(Entity entity) {
	std::size_t count = 1;
	entity.EachChild([&count](Entity child) { count += countSubtree(child); });
	return count;
}

} // namespace

// Read counters after drawing so renderer statistics include the game pass.
void Application::reportProfileCounters() {
	Profiler &profiler = Profiler::Main();
	if (!profiler.IsRecording()) {
		return;
	}
	const Renderer2D::Stats &render = _renderer2d.GetStats();
	SOWA_PROFILE_COUNTER("Draw calls", render.drawCalls);
	SOWA_PROFILE_COUNTER("Quads", render.quads);
	SOWA_PROFILE_COUNTER("Batch textures", render.textureBinds);

	// The scenes gameplay sees: frozen editor documents are not counted
	// while playing.
	std::size_t entities = 0;
	const std::vector<Scene> scenes =
		_scenes.IsPlaying() ? _scenes.RuntimeScenes() : _scenes.Loaded();
	for (const Scene scene : scenes) {
		if (scene.IsValid()) {
			entities += countSubtree(scene.Root());
		}
	}
	SOWA_PROFILE_COUNTER("Entities", entities);
	SOWA_PROFILE_COUNTER("Particles", CountLiveParticles2D(_world));
	SOWA_PROFILE_COUNTER("Lua memory (KB)", _scripts.MemoryUsed() / 1024.0);

	// Estimates from resource metadata: decoded RGBA for textures (plus a
	// retained CPU copy), f32 samples for audio clips.
	u64 textureBytes = 0;
	u64 audioBytes = 0;
	_resources.Each([&](ResourceID, Resource &resource, std::string_view) {
		if (const auto *texture = dynamic_cast<const ImageTexture *>(&resource)) {
			if (texture->IsLoaded()) {
				textureBytes += static_cast<u64>(texture->Width()) * texture->Height() * 4;
			}
			textureBytes += texture->Pixels().size();
		} else if (const auto *clip = dynamic_cast<const AudioClip *>(&resource)) {
			audioBytes += clip->Samples().size() * sizeof(f32);
		}
	});
	SOWA_PROFILE_COUNTER("Resources", _resources.Count());
	SOWA_PROFILE_COUNTER("Texture memory (KB)", textureBytes / 1024.0);
	SOWA_PROFILE_COUNTER("Audio memory (KB)", audioBytes / 1024.0);
}
#endif

#if SOWA_EDITOR
void Application::applyPendingProjectSettings() {
	std::optional<ProjectSettings> request = _editor.TakeProjectSettingsSaveRequest();
	if (!request) {
		return;
	}

	std::string status;
	const bool success = saveAndApplyProjectSettings(std::move(*request), status);
	_editor.ReportProjectSettingsSaveResult(success, std::move(status));
}

bool Application::saveAndApplyProjectSettings(ProjectSettings settings, std::string &status) {
	if (_scenes.IsPlaying()) {
		status = "Stop the running project before saving project settings.";
		return false;
	}
	if (std::string problem = editor::ValidateEditableProjectSettings(settings); !problem.empty()) {
		status = std::move(problem);
		return false;
	}

	const ProjectSettings previous = _projectSettings;
	const bool viewportChanged =
		settings.rendering.viewport.width != previous.rendering.viewport.width ||
		settings.rendering.viewport.height != previous.rendering.viewport.height;
	const bool scriptsChanged = settings.application.scripts != previous.application.scripts;

	RenderTargetHandle replacementTarget;
	if (viewportChanged) {
		RenderTargetDesc description;
		description.width = settings.rendering.viewport.width;
		description.height = settings.rendering.viewport.height;
		description.unsignedIntegerAttachment = true;
		replacementTarget = _graphicsDevice->CreateRenderTarget(description);
		if (!replacementTarget.IsValid()) {
			status = "Could not create the requested viewport render target.";
			return false;
		}
	}

	const auto discardReplacement = [this, &replacementTarget]() {
		if (replacementTarget.IsValid()) {
			_graphicsDevice->DestroyRenderTarget(replacementTarget);
		}
	};
	const auto restoreScripts = [this, &previous]() {
		_scripts.UpdateSettings(previous);
		if (!_scripts.ReloadModules(_fileSystem, previous.application.scripts)) {
			Log::Error("Failed to restore project scripts after settings save failure");
		}
	};

	// Project modules can register systems/components and read settings at
	// file scope. Reload them against the proposed values before committing
	// the settings file. If they fail, restore the previous module list.
	if (scriptsChanged) {
		_scripts.UpdateSettings(settings);
		if (!_scripts.ReloadModules(_fileSystem, settings.application.scripts)) {
			restoreScripts();
			discardReplacement();
			status = "Project scripts failed to reload; settings were not saved.";
			return false;
		}
	}

	if (!settings.Save(_fileSystem)) {
		if (scriptsChanged) {
			restoreScripts();
		}
		discardReplacement();
		status = "Failed to write res://project.sowa.";
		return false;
	}

	_projectSettings = std::move(settings);
	_fileSystem.EnableHotReload(_editorEnabled);
	_globalStore = _projectSettings.globalStore;
	_input.SetActions(_projectSettings.input.actions);
	_scripts.UpdateSettings(_projectSettings);
	_scenes.SetAutoloadPaths(_projectSettings.application.autoloads);

	if (replacementTarget.IsValid()) {
		const RenderTargetHandle oldTarget = _gameTarget;
		_gameTarget = replacementTarget;
		replacementTarget = {};
		_graphicsDevice->DestroyRenderTarget(oldTarget);
		_renderer2d.SetProjection(static_cast<f32>(_projectSettings.rendering.viewport.width),
			static_cast<f32>(_projectSettings.rendering.viewport.height));
	}

	if (_projectSettings.rendering.window.width != previous.rendering.window.width ||
		_projectSettings.rendering.window.height != previous.rendering.window.height) {
		_window.SetSize(_projectSettings.rendering.window.width,
			_projectSettings.rendering.window.height);
	}
	if (_projectSettings.application.name != previous.application.name) {
		_window.SetTitle(projectWindowTitle(_projectSettings, _editorEnabled));
	}

	Log::Info("Project settings saved and applied");
	status = "Project settings saved and applied.";
	return true;
}
#endif

void Application::Update() {
	if (!IsInitialized()) {
		Log::Warn("Application::Update called without a successful Init");
		return;
	}
#if SOWA_EDITOR
	if (_editor.IsInitialized()) {
		applyPendingProjectSettings();
	}
#endif

#if SOWA_PROFILER
	Profiler::Main().BeginFrame();
	_renderer2d.ResetStats();
#endif
	SOWA_PROFILE_PHASES();
	SOWA_PROFILE_PHASE("Input");
	beatWatchdog();
	pollDebugAdapter();
	_delta = _frameClock.Tick();
	const auto &viewport = _projectSettings.rendering.viewport;
	const Vector2 gameSize(static_cast<f32>(viewport.width), static_cast<f32>(viewport.height));
	bool editorKeyboard = false;
	const Rect presentRect = beginFrameInput(editorKeyboard);

	// Hover is the previous completed render sampled at this frame's cursor.
	// Resolve before BeginFrame rebuilds the token table, making the cached
	// entity available to gameplay and editor input for the rest of the frame.
	SOWA_PROFILE_PHASE("Entity pick");
	_hoveredEntity = {};
	if (presentRect.w >= 1.f && presentRect.h >= 1.f && _input.IsMouseOverViewport()) {
		const auto pixel =
			ViewportPointToPixel(_input.GetMousePosition(), viewport.width, viewport.height);
		if (pixel) {
			const u32 token =
				_graphicsDevice->ReadRenderTargetUInt(_gameTarget, 1, pixel->x, pixel->y);
			_hoveredEntity = _entityPicks.Resolve(token);
		}
	}
	_entityPicks.BeginFrame();

#if SOWA_EDITOR
	SOWA_PROFILE_PHASE("Editor input and reload");
	// F5–F9 belong to the editor, including playback inside the editor.
	// Ignore text input rather than WantCaptureKeyboard so these shortcuts
	// still work when a docked editor window has focus.
	if (_editorEnabled && !editorKeyboard) {
		if (_input.IsKeyJustPressed(Key::F5) || _input.IsKeyJustPressed(Key::F6)) {
			_editor.StopAnimationPreview();
		}
		if (_input.IsKeyJustPressed(Key::F5)) {
			editor::RequestPlayControl(_scenes, editor::PlayControl::PlayProject,
				_projectSettings.application.mainScene);
		}
		if (_input.IsKeyJustPressed(Key::F6)) {
			editor::RequestPlayControl(_scenes, editor::PlayControl::PlayCurrent,
				_projectSettings.application.mainScene);
		}
		if (_input.IsKeyJustPressed(Key::F7)) {
			editor::RequestPlayControl(_scenes,
				_scenes.IsPaused() ? editor::PlayControl::Resume : editor::PlayControl::Pause,
				_projectSettings.application.mainScene);
		}
		if (_input.IsKeyJustPressed(Key::F8)) {
			editor::RequestPlayControl(_scenes, editor::PlayControl::Stop,
				_projectSettings.application.mainScene);
		}
		if (_input.IsKeyJustPressed(Key::F9)) {
			editor::RequestPlayControl(_scenes, editor::PlayControl::Step,
				_projectSettings.application.mainScene);
		}
	}
	if (_editor.IsInitialized()) {
		_physics.SetDebugDrawEnabled(_editor.ShowPhysicsDebug());
	}

	// Apply queued scene transitions before Progress.
	// Transitions requested during Progress wait until the next frame.
	if (_editorEnabled) {
		_editor.RestoreAnimationPreview(_world);
		_fileSystem.DispatchChanges();
		_scripts.FlushFileChanges();
	}
#endif
	// --wait-for-debugger holds the first Play in the editor.
	if (_waitingForDebugger && _scenes.IsPlayPending()) {
		waitForDebugger();
	}
	SOWA_PROFILE_PHASE("Scene flush");
	_scenes.Flush();
	// Arm the watchdog after scene loading. During editor Play it can pause Lua timers,
	// tweens, and systems through the instruction-count hook.
	beatWatchdog();
	// The pause watch lets a pause reach a running loop: during editor Play
	// (the watchdog) and while VS Code is attached (its Pause button).
	if (_editorEnabled) {
		_watchdog.SetArmed(_scenes.IsPlaying());
	}
	_scripts.Debugger().SetPauseWatch(
		(_editorEnabled && _scenes.IsPlaying()) || debuggerAttached());

	// Choose game time after scene transitions. Later pause/resume requests affect
	// the next frame; stopping clears timers and tweens before they can advance.
	const GameTime time = GetGameTime(_scenes.IsPlaying(), _scenes.IsPaused(), _scenes.TakeStep(),
		_delta, _physics.Config().fixedDelta);
	_audioMixer.SetPaused(_scenes.IsPaused());
	if (time.running) {
		SOWA_PROFILE_PHASE("Timers");
		_scripts.UpdateTimers(time.delta);
		SOWA_PROFILE_PHASE("Tweens");
		_scripts.UpdateTweens(time.delta);
	}

	// Play-only systems (physics, Lua _update, animation) run only while the
	// game runs. Visual systems opt into Always() and get time.delta, which
	// is 0 while paused.
	_world.SetPlayOnlySystemsEnabled(time.running);

	// Runs every registered system in registration order, then the engine's
	// own PostUpdate pass (CalculateTransforms). Has to happen before the draw
	// below, which reads the world matrices that pass produces.
	SOWA_PROFILE_PHASE("UI input");
#if SOWA_EDITOR
	_editor.ApplyAnimationPreview(_world, _resources, _delta, _scenes.IsPlaying(), gameSize);
#endif
	_ui.Layout(gameSize, sceneViewProjection(gameSize));
	_ui.HandleInput(_input, time.delta, time.running && !editorKeyboard);
	SOWA_PROFILE_PHASE("World progress");
	_world.Progress(time.delta);
	SOWA_PROFILE_PHASE("UI layout");
	_ui.Layout(gameSize, sceneViewProjection(gameSize));

	if (const Entity camera = _scenes.Current().ActiveCamera(); camera.IsAlive()) {
		if (const Transform2D *transform = camera.Get<Transform2D>()) {
			f32 rotation = 0.f;
			const Camera2D *camera2d = camera.Get<Camera2D>();
			if (camera2d != nullptr && camera2d->rotatable) {
				rotation = transform->WorldRotation();
			}
			_audioMixer.SetListener(transform->WorldPosition(), rotation);
		} else {
			_audioMixer.ClearListener();
		}
	} else {
		_audioMixer.ClearListener();
	}

	// Game pass, fixed viewport resolution.
	SOWA_PROFILE_PHASE("Scene draw");
	_graphicsDevice->BindRenderTarget(_gameTarget);
	_graphicsDevice->Clear(Color(0.1f, 0.1f, 0.1f, 1.f));
	_graphicsDevice->ClearRenderTargetUInt(_gameTarget, 1, 0);
	drawScene();
#if SOWA_EDITOR
	_editor.RestoreAnimationPreview(_world);
#endif
	_audioMixer.Sync();
#if SOWA_PROFILER
	reportProfileCounters();
#endif

	SOWA_PROFILE_PHASE("Present");
	presentFrame(presentRect, gameSize);

	SOWA_PROFILE_PHASE("Swap");
	_window.SwapBuffers();

	// Retire input edges after game code reads them; browser events arrive between frames.
	_input.Update();
#if SOWA_PROFILER
	// Ends the last phase with the frame (its guard is then stale).
	Profiler::Main().EndFrame();
#endif
}

Rect Application::beginFrameInput(bool &editorKeyboard) {
	_input.BeginEventBatch();
	_window.PollEvents();
#if SOWA_EDITOR
	if (_editor.IsInitialized()) {
		_editor.BeginFrame();
	}
#endif

	const auto &viewport = _projectSettings.rendering.viewport;
	const Vector2 framebufferSize = _window.GetSize();
	const Vector2 contentSize = _window.GetContentSize();
	const Vector2 gameSize(static_cast<f32>(viewport.width), static_cast<f32>(viewport.height));
	Rect presentRect =
		CalculatePresentRect(viewport.width, viewport.height, static_cast<i32>(framebufferSize.x),
			static_cast<i32>(framebufferSize.y), viewport.scaleMode);
#if SOWA_EDITOR
	if (_editor.IsInitialized()) {
		presentRect =
			editor::GameImageToPresentRect(_editor.GameImageRect(), contentSize, framebufferSize);
	}
#endif
	_input.SetViewportMapping(presentRect, contentSize, framebufferSize, gameSize);

	editorKeyboard = false;
	bool editorPointer = false;
#if SOWA_EDITOR
	editorKeyboard = _editor.CapturesGameKeyboard();
	editorPointer = _editor.IsInitialized() && (editorKeyboard || !_input.IsMouseOverViewport());
#endif
	_input.SetEditorCapture(editorPointer, editorKeyboard);
	_input.DispatchEvents();
	return presentRect;
}

void Application::presentFrame(Rect presentRect, Vector2 gameSize) {
	// Run mode blits to the window; edit mode displays the game target in the viewport panel.
	_graphicsDevice->BindRenderTarget({});
	const TextureHandle gameColor = _graphicsDevice->GetRenderTargetColorTexture(_gameTarget);
#if SOWA_EDITOR
	if (_editor.IsInitialized()) {
		SOWA_PROFILE_SCOPE("Editor draw");
		_graphicsDevice->Clear(Color(0.13f, 0.14f, 0.15f, 1.f));
		_editor.Draw(_graphicsDevice->GetNativeTextureId(gameColor), gameSize, _scenes.IsPlaying(),
			_world, _scenes, _projectSettings, _scripts, _resources, _hoveredEntity);
		_editor.EndFrame(*_graphicsDevice, _window);
		return;
	}
#endif
	_graphicsDevice->Clear(Color(0.f, 0.f, 0.f, 1.f));
	_graphicsDevice->SetViewport(static_cast<i32>(presentRect.x), static_cast<i32>(presentRect.y),
		static_cast<i32>(presentRect.w), static_cast<i32>(presentRect.h));
	_present.Draw(gameColor);
}

LuaResumeAction Application::suspendForDebugger(const LuaStopEvent &event) {
	if (!_suspension.WouldSuspend()) {
		// Outside editor Play, or with a Stop queued: ScriptSuspension
		// decides without holding the frame.
		return _suspension.Run(event, {});
	}
#if SOWA_PROFILER
	// A frame held at a breakpoint measures the wait, not the game.
	Profiler::Main().DiscardFrame();
#endif
#if SOWA_EDITOR
	// The breakpoint hit mid-frame, after this frame's ImGui frame began.
	// Close it so each suspended frame can run a whole editor frame, and
	// open a fresh one for the rest of this frame afterwards.
	_editor.SuspendFrame();
#endif
	LuaStopEvent shown = event;
	if (event.reason == LuaStopReason::Pause && _watchdog.HasFired()) {
		shown.message = fmt::format("Script has been running for over {} seconds",
			std::chrono::duration_cast<std::chrono::seconds>(_watchdog.Timeout()).count());
	}
#if SOWA_DEBUGGER
	if (_dapSession) {
		_dapSession->OnStopped(shown);
	}
#endif
	const LuaResumeAction action = _suspension.Run(shown, [this] { return pumpSuspendedFrame(); });
#if SOWA_DEBUGGER
	if (_dapSession) {
		_dapSession->OnContinued();
	}
#endif
	beatWatchdog();
#if SOWA_EDITOR
	_editor.BeginFrame();
#endif
	// The next frame's delta starts now, not before the stop.
	_frameClock.Rebase();
	return action;
}

bool Application::pumpSuspendedFrame() {
	return pumpFrame(true);
}

bool Application::pumpFrame(bool sessionKeys) {
	if (_window.ShouldClose()) {
		return false;
	}
	beatWatchdog();
	pollDebugAdapter();
	const auto &viewport = _projectSettings.rendering.viewport;
	const Vector2 gameSize(static_cast<f32>(viewport.width), static_cast<f32>(viewport.height));
	bool editorKeyboard = false;
	const Rect presentRect = beginFrameInput(editorKeyboard);

#if SOWA_EDITOR
	// Only the session controls apply: F7 resumes (continues), F8 stops.
	if (sessionKeys && _editorEnabled && !editorKeyboard) {
		if (_input.IsKeyJustPressed(Key::F7)) {
			editor::RequestPlayControl(_scenes, editor::PlayControl::Resume,
				_projectSettings.application.mainScene);
		}
		if (_input.IsKeyJustPressed(Key::F8)) {
			editor::RequestPlayControl(_scenes, editor::PlayControl::Stop,
				_projectSettings.application.mainScene);
		}
	}
#endif

	// The game target still holds the last completed frame: the frame that
	// hit the breakpoint has not drawn yet.
	presentFrame(presentRect, gameSize);
	_window.SwapBuffers();
	_input.Update();
	return !_window.ShouldClose();
}

void Application::beatWatchdog() {
	if (_watchdog.Beat()) {
		// It fired, but no script reached the pause: the time went elsewhere.
		_scripts.Debugger().CancelPause();
	}
}

void Application::startDebugAdapter() {
#if SOWA_DEBUGGER
	_dapSession = MakeScope<DapSession>(
		_scripts.Debugger(), [this](std::string json) { _dapServer.Send(std::move(json)); },
		DapSession::Hooks{[this](LuaResumeAction action) { _suspension.Request(action); }, {}});
	if (const auto *folder = dynamic_cast<FolderFileServer *>(_fileSystem.Get("res"))) {
		std::error_code ec;
		const auto root = std::filesystem::weakly_canonical(folder->Root(), ec);
		_dapSession->SetProjectRoot(ec ? std::filesystem::absolute(folder->Root()) : root);
	}
	_scripts.Debugger().SetOutputHandler([this](std::string_view text) {
		Log::Info("{}", text);
		_dapSession->OnOutput(text);
	});
	std::string error;
	if (_dapServer.Start(_debugOptions.port, error)) {
		Log::Info("Lua debugger: VS Code can attach on 127.0.0.1:{}", _dapServer.Port());
		_waitingForDebugger = _debugOptions.waitForDebugger;
	} else {
		Log::Warn("Lua debugger: cannot listen on 127.0.0.1:{} ({}); debugging is off",
			_debugOptions.port, error);
		if (_debugOptions.waitForDebugger) {
			Log::Warn("--wait-for-debugger ignored: nothing to wait on");
		}
	}
#else
	if (_debugOptions.waitForDebugger || _debugOptions.port != kDefaultDebugPort) {
		Log::Warn("This build has no Lua debug adapter; debugging options are ignored");
	}
#endif
}

void Application::pollDebugAdapter() {
#if SOWA_DEBUGGER
	if (!_dapSession) {
		return;
	}
	for (DapServerEvent &event : _dapServer.Take()) {
		switch (event.kind) {
		case DapServerEvent::Kind::Connected:
			Log::Info("Lua debugger: VS Code connected");
			break;
		case DapServerEvent::Kind::Message:
			_dapSession->Handle(event.message);
			break;
		case DapServerEvent::Kind::Disconnected:
			_dapSession->OnConnectionClosed();
			Log::Info("Lua debugger: VS Code disconnected");
			break;
		}
	}
#endif
}

bool Application::debuggerAttached() const {
#if SOWA_DEBUGGER
	return _dapSession && _dapSession->IsAttached();
#else
	return false;
#endif
}

void Application::waitForDebugger() {
#if SOWA_DEBUGGER
	Log::Info("Waiting for VS Code to attach on 127.0.0.1:{} (--wait-for-debugger)",
		_dapServer.Port());
#if SOWA_PROFILER
	Profiler::Main().DiscardFrame();
#endif
#if SOWA_EDITOR
	// In the editor this runs mid-frame, like a suspension.
	const bool editorFrame = _editor.IsInitialized();
	if (editorFrame) {
		_editor.SuspendFrame();
		_editor.SetDebuggerWait(_dapServer.Port());
	}
#endif
	while (!_dapSession->IsConfigured()) {
		if (!pumpFrame(false)) {
			break;
		}
#if SOWA_EDITOR
		if (_editor.TakeDebuggerWaitCancel()) {
			Log::Info("Stopped waiting for the debugger");
			break;
		}
#endif
	}
	_waitingForDebugger = false;
#if SOWA_EDITOR
	if (editorFrame) {
		_editor.SetDebuggerWait({});
		_editor.BeginFrame();
	}
#endif
	_frameClock.Rebase();
#endif
}

void Application::Shutdown() {
	_initialized = false;
	_watchdog.Stop();
	_scripts.Debugger().SetStopHandler({});
#if SOWA_DEBUGGER
	if (_dapSession) {
		_dapSession->OnTerminated();
		_dapSession->OnConnectionClosed();
	}
	_dapServer.Stop();
	_scripts.Debugger().SetOutputHandler({});
	_dapSession.reset();
#endif
	_waitingForDebugger = false;
	_fileSystem.EnableHotReload(false);
	_input.Events().remove(_uiInputHandle);
	_uiInputHandle = {};
#if SOWA_EDITOR
	_editor.Shutdown();
#endif
	// Stop playback before changing the mixer or releasing its sample snapshots.
	if (_audioDevice) {
		_audioDevice->Shutdown();
		_audioDevice.reset();
	}
	_scenes.Clear();
	_ui.Reset();
	_scripts.ClearTimers();
	_scripts.ClearTweens();
	_audioMixer.Shutdown();
	_physics.Shutdown();
	// Resource unloads still need the graphics device and its current context.
	_resources.Clear();
	_resources.SetContext({});
	_present.Shutdown();
	_renderer2d.Shutdown();
	if (_graphicsDevice) {
		if (_gameTarget.IsValid()) {
			_graphicsDevice->DestroyRenderTarget(_gameTarget);
			_gameTarget = {};
		}
		_graphicsDevice->Shutdown();
		_graphicsDevice.reset();
	}
	_window.Destroy();
}

bool Application::ShouldClose() const {
	return _window.ShouldClose();
}

} // namespace sowa
