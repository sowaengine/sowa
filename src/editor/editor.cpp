#include "editor/editor.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/filesystem/folder_file_server.hpp"
#include "core/log.hpp"
#include "core/window.hpp"
#include "ecs/world.hpp"
#include "editor/fonts.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/native_titlebar.hpp"
#include "editor/play_controls.hpp"
#include "editor/style.hpp"
#include "editor/viewport.hpp"
#include "editor/widgets.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "render/graphics_device.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"
#include "script/script_suspension.hpp"

#include <filesystem>
#include <utility>

namespace sowa {
namespace {

const char *glslVersion() {
#if defined(__EMSCRIPTEN__)
	return "#version 300 es";
#else
	return "#version 330";
#endif
}

} // namespace

Editor::~Editor() {
	Shutdown();
}

bool Editor::Init(Window &window, FileSystem &fs) {
	if (_initialized) {
		return true;
	}
	if (!window.IsCreated()) {
		Log::Error("Editor::Init called with an uncreated Window");
		return false;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();
	editor::ApplyStyle(ImGui::GetStyle());

	if (!editor::LoadEmbeddedEditorFonts(*io.Fonts)) {
		Log::Warn("Editor: using ImGui default font");
	}

	if (!ImGui_ImplGlfw_InitForOpenGL(window.Handle(), true)) {
		Log::Error("Editor: ImGui GLFW backend init failed");
		ImGui::DestroyContext();
		return false;
	}
	if (!ImGui_ImplOpenGL3_Init(glslVersion())) {
		Log::Error("Editor: ImGui OpenGL3 backend init failed");
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		return false;
	}

	_window = &window;
	editor::SetNativeTitlebarStyle(window.Handle(), true);
	_fileSystem = &fs;
	_projectSettings.Init(window.GetInput());
	_initialized = true;
	_applyDefaultDock = true;
	Log::Info("Editor initialized");
	return true;
}

void Editor::Shutdown() {
	_selectionContext.reset();
	_sceneSaveStates.clear();
	if (!_initialized) {
		return;
	}
	editor::SetNativeTitlebarStyle(_window->Handle(), false);
	_projectSettings.Shutdown();
	_projectExport.Reset();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	_window = nullptr;
	_fileSystem = nullptr;
	_initialized = false;
}

void Editor::BeginFrame() {
	if (!_initialized) {
		return;
	}
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGuiIO &io = ImGui::GetIO();
	io.MouseWheel = editor::ClampScrollDelta(io.MouseWheel);
	io.MouseWheelH = editor::ClampScrollDelta(io.MouseWheelH);
	ImGui::NewFrame();
	_projectSettings.BeginFrame();
	// Open before gameplay runs so the shortcut's P never reaches the game.
	if ((io.KeyCtrl || io.KeySuper) && io.KeyShift && !io.KeyAlt &&
		ImGui::IsKeyPressed(ImGuiKey_P, false)) {
		_commandPalette.Open();
	}
}

void Editor::SetCameraView(Vector2 position, f32 zoom) {
	_camera.position = position;
	_camera.zoom = editor::ClampZoom(zoom);
}

std::optional<ProjectSettings> Editor::TakeProjectSettingsSaveRequest() {
	return _projectSettings.TakeSaveRequest();
}

void Editor::ReportProjectSettingsSaveResult(bool success, std::string message) {
	_projectSettings.ReportSaveResult(success, std::move(message));
}

void Editor::Draw(u64 gameTextureId, Vector2 gameSize, bool playing, World &world,
	SceneManager &scenes, ProjectSettings &project, ScriptServer &scripts,
	ResourceRegistry &resources, Entity hoveredEntity) {
	if (!_initialized) {
		return;
	}
	_editingWorld = &world;
	if (_bulkPending &&
		(playing || _bulkPending->selection != selection() ||
			_bulkPending->scene.Root() != scenes.EditorCurrent().Root() ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))) {
		finishBulkInspector(playing || ImGui::IsKeyPressed(ImGuiKey_Escape));
	}
	if (_gizmoDrag &&
		(playing || _gizmoDrag->selection != selection() ||
			_gizmoScene.Root() != scenes.EditorCurrent().Root() ||
			!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !ImGui::IsMousePosValid() ||
			(_previewSceneCamera && scenes.Current().ActiveCamera().IsAlive()) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))) {
		finishTransformGizmo(playing || ImGui::IsKeyPressed(ImGuiKey_Escape));
	}

	if (_raycastDrag &&
		(playing || Selected() != _raycastDrag->entity ||
			_raycastDrag->scene.Root() != scenes.EditorCurrent().Root() ||
			!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !ImGui::IsMousePosValid() ||
			!editor::CanEdit(_raycastDrag->entity, playing) ||
			(_previewSceneCamera && scenes.Current().ActiveCamera().IsAlive()) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))) {
		finishRaycastGizmo(ImGui::IsKeyPressed(ImGuiKey_Escape));
	}
	if (_tileStrokeBefore &&
		(playing || !ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
			Selected() != _tileStrokeEntity)) {
		finishTileStroke();
	}
	if (_ninePatchBefore &&
		(playing || Selected() != _ninePatchEntity ||
			_ninePatchScene.Root() != scenes.EditorCurrent().Root() ||
			!ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
			!editor::CanEdit(_ninePatchEntity, playing) ||
			(_previewSceneCamera && scenes.Current().ActiveCamera().IsAlive()) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))) {
		finishNinePatchResize(ImGui::IsKeyPressed(ImGuiKey_Escape));
	}
	if (_paddingDrag &&
		(playing || !_showNinePatchEditor || Selected() != _paddingDrag->entity ||
			!ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
			!editor::CanEdit(_paddingDrag->entity, playing) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))) {
		finishNinePatchPadding(ImGui::IsKeyPressed(ImGuiKey_Escape));
	}
	bindSelection(scenes);
	// No active editor document while playing; live edits remain transient.
	_editingScene = playing ? Scene() : scenes.EditorCurrent();
	_saveSceneRequested = false;
	_undoSceneRequested = false;
	_redoSceneRequested = false;
	drawDockHost(scenes, project, resources);
	drawCommandPalette(scenes, project, scripts, resources);
	_projectExport.Draw(*_fileSystem, playing, _projectSettings.HasUnsavedChanges(project), [&] {
		commitPendingSceneEdits(resources);
		StopAnimationPreview();
		for (const Scene scene : scenes.EditorScenes()) {
			if (editor::IsSceneDocumentUnsaved(_sceneSaveStates, scene) &&
				!editor::SaveSceneDocument(*_fileSystem, resources, scene, _sceneSaveStates)) {
				return false;
			}
		}
		return true;
	});
	if (!playing && (_undoSceneRequested || _redoSceneRequested || _saveSceneRequested)) {
		commitPendingSceneEdits(resources);
	}
	if (!playing && _undoSceneRequested) {
		editor::UndoSceneDocument(_sceneSaveStates, scenes.EditorCurrent());
		_tileSourceNeedsRefresh = true;
	}
	if (!playing && _redoSceneRequested) {
		editor::RedoSceneDocument(_sceneSaveStates, scenes.EditorCurrent());
		_tileSourceNeedsRefresh = true;
	}
	if (_saveSceneRequested && _fileSystem != nullptr) {
		editor::SaveSceneDocument(*_fileSystem, resources, scenes.EditorCurrent(),
			_sceneSaveStates);
	}
	drawViewport(gameTextureId, gameSize, playing, world, scenes, resources, hoveredEntity);
	drawScriptSuspension(scenes, project);
	drawDebuggerWait();
	_editingScene = playing ? Scene() : scenes.EditorCurrent();
	drawHierarchy(world, scenes, resources, playing);
	if (!playing) {
		_animationPreview.Select(Selected());
	}
	drawInspector(world, scenes, scripts, project, resources, playing);
	if (!ImGui::IsAnyItemActive()) {
		_resourceInspector.Commit(resources, _sceneSaveStates);
	}
	drawAnimationEditor(world, resources, playing);
	drawTilemapEditor(resources, playing);
	drawSpriteSheetEditor(resources, playing);
	drawNinePatchEditor(world, resources, playing);
	drawFilesystem(scenes, playing);
	drawConsole();
	_projectSettings.Draw(project, *_fileSystem, playing);
	// Finish drawing the current document before switching scenes. This also
	// commits any inspector edit that lost focus when the icon was clicked.
	if (!_openInstanceSceneRequested.empty()) {
		const std::string path = std::exchange(_openInstanceSceneRequested, {});
		commitPendingSceneEdits(resources);
		if (editor::TryOpenAsset(scenes, path, editor::AssetKind::Scene)) {
			bindSelection(scenes);
		} else {
			Log::Warn("Could not open source scene '{}'", path);
		}
	}
	{
		const auto *folder = dynamic_cast<FolderFileServer *>(_fileSystem->Get("res"));
		_profilerPanel.Draw(_showProfiler, folder ? folder->Root() : std::filesystem::path());
	}
	if (_showStyleEditor) {
		ImGui::Begin("Style editor", &_showStyleEditor);
		ImGui::ShowStyleEditor();
		ImGui::End();
	}
	for (auto &state : _sceneSaveStates) {
		state.context->selection.Prune(state.context->root);
		state.context->selectionBookmark =
			editor::SelectionBookmark::Capture(state.context->root, state.context->selection);
	}
}

void Editor::commitPendingSceneEdits(ResourceRegistry &resources) {
	finishBulkInspector();
	finishUIGizmo();
	_resourceInspector.Commit(resources, _sceneSaveStates);
	finishRaycastGizmo();
	finishTileStroke();
	finishTransformGizmo();
	finishNinePatchResize();
	finishNinePatchPadding();
	// ImGui edits component storage live and normally commits the command when
	// the widget deactivates. History actions and saving need history to first
	// represent the exact live state.
	if (_pendingFieldEdit) {
		const FieldInfo *field = _pendingFieldEdit->type.FindField(_pendingFieldEdit->field);
		editor::FieldValue after;
		if (field != nullptr && editor::CaptureFieldValue(_pendingFieldEdit->entity,
									_pendingFieldEdit->type, *field, after)) {
			editor::RecordFieldEdit(_sceneSaveStates, _pendingFieldEdit->scene,
				_pendingFieldEdit->entity, _pendingFieldEdit->type, *field,
				_pendingFieldEdit->before, std::move(after));
		}
		_pendingFieldEdit.reset();
	}
	if (_pendingRenameEntity.IsAlive()) {
		editor::RecordRename(_sceneSaveStates, _pendingRenameScene, _pendingRenameEntity,
			_pendingRenameBefore, _pendingRenameEntity.GetName());
		_pendingRenameEntity = {};
		_pendingRenameScene = {};
		_pendingRenameBefore.clear();
	}
}

void Editor::EndFrame(GraphicsDevice &device, Window &window) {
	if (!_initialized) {
		return;
	}
	ImGui::Render();
	const Vector2 fb = window.GetSize();
	device.SetViewport(0, 0, static_cast<i32>(fb.x), static_cast<i32>(fb.y));
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::SuspendFrame() {
	if (_initialized && GImGui != nullptr && GImGui->WithinFrameScope) {
		ImGui::EndFrame();
	}
}

void Editor::drawScriptSuspension(SceneManager &scenes, const ProjectSettings &project) {
	if (_scriptSuspension == nullptr || !_scriptSuspension->IsActive()) {
		return;
	}
	const LuaStopEvent &stop = _scriptSuspension->Event();
	const char *what = "Paused at";
	switch (stop.reason) {
	case LuaStopReason::Breakpoint:
		what = "Breakpoint at";
		break;
	case LuaStopReason::Step:
		what = "Stepped to";
		break;
	case LuaStopReason::Exception:
		what = "Error at";
		break;
	case LuaStopReason::Pause:
		break;
	}
	const std::string where =
		stop.source.empty() ? std::string("Lua") : fmt::format("{}:{}", stop.source, stop.line);

	const ImVec2 origin = ImGui::GetMainViewport()->Pos;
	ImGui::SetNextWindowPos(
		ImVec2(origin.x + _gameImageRect.x + 8.f, origin.y + _gameImageRect.y + 8.f));
	ImGui::SetNextWindowBgAlpha(0.85f);
	constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoFocusOnAppearing;
	if (ImGui::Begin("##script_suspension", nullptr, flags)) {
		ImGui::TextColored(stop.reason == LuaStopReason::Exception ? editor::theme::Rose
																   : editor::theme::Amber,
			ICON_FA_PAUSE "  %s %s", what, where.c_str());
		if (!stop.message.empty()) {
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 36.f);
			ImGui::TextUnformatted(stop.message.c_str());
			ImGui::PopTextWrapPos();
		}
		if (ImGui::Button(ICON_FA_PLAY "  Continue")) {
			_scriptSuspension->Request(LuaResumeAction::Continue);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Run until the next breakpoint (F7 resumes the session)");
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP "  Stop")) {
			editor::RequestPlayControl(scenes, editor::PlayControl::Stop,
				project.application.mainScene);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Abort the running script and stop playing (F8)");
		}
	}
	ImGui::End();
}

void Editor::drawDebuggerWait() {
	if (!_debuggerWaitPort) {
		return;
	}
	const ImVec2 origin = ImGui::GetMainViewport()->Pos;
	ImGui::SetNextWindowPos(
		ImVec2(origin.x + _gameImageRect.x + 8.f, origin.y + _gameImageRect.y + 8.f));
	ImGui::SetNextWindowBgAlpha(0.85f);
	constexpr ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoFocusOnAppearing;
	if (ImGui::Begin("##debugger_wait", nullptr, flags)) {
		ImGui::TextColored(editor::theme::Amber,
			ICON_FA_PAUSE "  Waiting for VS Code on 127.0.0.1:%u",
			static_cast<unsigned>(*_debuggerWaitPort));
		ImGui::TextDisabled("Play starts once the debugger has attached (--wait-for-debugger).");
		if (ImGui::Button("Don't wait")) {
			_debuggerWaitCancelled = true;
		}
	}
	ImGui::End();
}

bool Editor::WantsKeyboard() const {
	return _initialized && ImGui::GetIO().WantCaptureKeyboard;
}

bool Editor::WantsMouse() const {
	return _initialized && ImGui::GetIO().WantCaptureMouse;
}

bool Editor::WantsTextInput() const {
	return _initialized && ImGui::GetIO().WantTextInput;
}

bool Editor::CapturesGameKeyboard() const {
	return _initialized &&
		   (ImGui::GetIO().WantTextInput || _commandPalette.IsOpen() || ImGui::IsAnyItemActive() ||
			   ImGui::IsPopupOpen(nullptr,
				   ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel));
}

void Editor::drawConsole() {
	ImGui::Begin(ICON_FA_TERMINAL "  Console");
	if (editor::IconActionButton(ICON_FA_TRASH, "Clear console")) {
		Log::Clear();
	}

	struct FilterControl {
		const char *label;
		bool *value;
	};

	const FilterControl controls[] = {
		{"Auto-scroll", &_consoleAutoScroll},
		{"Trace", &_consoleFilter.trace},
		{"Info", &_consoleFilter.info},
		{"Warn", &_consoleFilter.warn},
		{"Error", &_consoleFilter.error},
	};
	const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
	for (const auto &control : controls) {
		const float width = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x +
							ImGui::CalcTextSize(control.label).x;
		if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <= right) {
			ImGui::SameLine();
		}
		ImGui::Checkbox(control.label, control.value);
	}

	ImGui::BeginChild("##log", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders,
		ImGuiWindowFlags_HorizontalScrollbar);
	const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.f;

	editor::DrawConsoleRows(editor::BuildConsoleRows(Log::GetLines(), _consoleFilter));

	if (editor::ShouldStickToBottom(_consoleAutoScroll, wasAtBottom)) {
		ImGui::SetScrollHereY(1.f);
	}
	ImGui::EndChild();
	ImGui::End();
}

void Editor::bindSelection(SceneManager &scenes) {
	const bool playing = scenes.IsPlaying();
	if (playing != _selectionPlaying) {
		_runtimeSelection.Clear();
		_selectionPlaying = playing;
	}
	if (playing) {
		_selectionContext.reset();
		_runtimeSelection.Prune();
		return;
	}
	editor::UpdateSceneDocumentStates(_sceneSaveStates, scenes.EditorScenes());
	auto *state = editor::FindSceneDocumentState(_sceneSaveStates, scenes.EditorCurrent());
	_selectionContext = state ? state->context : nullptr;
	if (!state) {
		_runtimeSelection.Clear();
	}
}

bool Editor::canMutateSelection(bool structural) const {
	return !_selectionPlaying && editor::CanEditSelection(selection(), _editingScene, structural);
}

void Editor::deleteSelection(ResourceRegistry &resources) {
	commitPendingSceneEdits(resources);
	editor::ExecuteDeleteSelection(_sceneSaveStates, _editingScene);
}

void Editor::handleSelectionShortcuts(ResourceRegistry &resources, Entity parent) {
	if (_selectionPlaying || !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
		ImGui::GetIO().WantTextInput) {
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
		deleteSelection(resources);
	}
	if (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper) {
		if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
			duplicateEntity(resources);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
			copyEntity(resources);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
			pasteEntity(parent, resources);
		}
	}
}
} // namespace sowa
