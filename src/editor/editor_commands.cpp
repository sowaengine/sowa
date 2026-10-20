#include "editor/editor.hpp"

#include <stdexcept>
#include <utility>

#include "core/log.hpp"
#include "editor/icons.hpp"
#include "editor/play_controls.hpp"
#include "imgui.h"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"

namespace sowa {

void Editor::drawCommandPalette(SceneManager &scenes, ProjectSettings &project,
	ScriptServer &scripts, ResourceRegistry &resources) {
	if (!_commandPalette.IsOpen() && !scripts.CommandInputs().Current()) {
		return;
	}
	auto commands = scripts.Commands();
	auto add = [&](const char *id, const char *title, std::function<void()> run) -> auto & {
		PaletteCommand command;
		command.id = id;
		command.title = title;
		command.run = std::move(run);
		command.allowWhilePlaying = true;
		return commands.Register(std::move(command));
	};
	const auto scene = scenes.EditorCurrent();
	const auto *state = editor::FindSceneDocumentState(_sceneSaveStates, scene);
	const bool editing = !scenes.IsPlaying();
	auto &save = add("editor.scene.save", "Scene: Save", [this] { _saveSceneRequested = true; });
	save.shortcut = "Ctrl+S";
	save.enabled = editing && scene.IsValid() && !scene.SourcePath().empty();

	auto &undo = add("editor.edit.undo", "Edit: Undo", [this] { _undoSceneRequested = true; });
	undo.shortcut = "Ctrl+Z";
	undo.enabled = editing && state && state->history.CanUndo();

	auto &redo = add("editor.edit.redo", "Edit: Redo", [this] { _redoSceneRequested = true; });
	redo.shortcut = "Ctrl+Y";
	redo.enabled = editing && state && state->history.CanRedo();

	const auto available = editor::AvailablePlayControls(scenes.PlayMode(), scenes.IsPaused(),
		scene.IsValid(), !project.application.mainScene.empty(), scenes.IsScriptSuspended());
	const auto play = [&](editor::PlayControl control) {
		commitPendingSceneEdits(resources);
		StopAnimationPreview();
		editor::RequestPlayControl(scenes, control, project.application.mainScene);
	};
	auto &playProject = add("editor.play.project", "Play: Project",
		[&] { play(editor::PlayControl::PlayProject); });
	playProject.shortcut = "F5";
	playProject.enabled = available.playProject;

	auto &playScene = add("editor.play.scene", "Play: Current Scene",
		[&] { play(editor::PlayControl::PlayCurrent); });
	playScene.shortcut = "F6";
	playScene.enabled = available.playCurrent;

	auto &stop = add("editor.play.stop", "Play: Stop", [&] {
		editor::RequestPlayControl(scenes, editor::PlayControl::Stop,
			project.application.mainScene);
	});
	stop.shortcut = "F8";
	stop.enabled = available.stop;

	auto &pause = add("editor.play.pause", "Play: Pause / Resume", [&] {
		editor::RequestPlayControl(scenes,
			scenes.IsPaused() ? editor::PlayControl::Resume : editor::PlayControl::Pause,
			project.application.mainScene);
	});
	pause.shortcut = "F7";
	pause.enabled = available.pause || available.resume;

	auto &step = add("editor.play.step", "Play: Step Frame", [&] {
		editor::RequestPlayControl(scenes, editor::PlayControl::Step,
			project.application.mainScene);
	});
	step.shortcut = "F9";
	step.enabled = available.step;

	add("editor.viewport.reset", "Viewport: Reset Camera", [this] {
		_camera = {};
		_previewSceneCamera = false;
	}).enabled = editing;
	add("editor.viewport.camera", "Viewport: Toggle Camera Preview", [this] {
		_previewSceneCamera = !_previewSceneCamera;
	}).enabled = editing && scenes.Current().ActiveCamera().IsAlive();
	add("editor.layout.reset", "View: Reset Panel Layout", [this] { _applyDefaultDock = true; });
	add("editor.console.clear", "Console: Clear", [] { Log::Clear(); });

	const auto focus = [&](const char *id, const char *title, const char *panel) {
		add(id, title, [panel] { ImGui::SetWindowFocus(panel); });
	};
	focus("editor.view.console", "View: Focus Console", ICON_FA_TERMINAL "  Console");
	add("editor.view.settings", "Project: Project Settings", [this] { _projectSettings.Open(); });
	add("editor.entity.duplicate", "Entity: Duplicate", [&] {
		duplicateEntity(resources);
	}).enabled = editing && canMutateSelection(true);
	add("editor.entity.copy", "Entity: Copy", [&] { copyEntity(resources); }).enabled =
		editing && !multiSelected() && editor::CanDelete(Selected());
	add("editor.entity.paste", "Entity: Paste as Child", [&] {
		pasteEntity(Selected().IsAlive() ? Selected() : scene.Root(), resources);
	}).enabled = editing && !multiSelected() && scene.IsValid();
	focus("editor.view.hierarchy", "View: Focus Hierarchy", ICON_FA_SITEMAP "  Hierarchy");
	focus("editor.view.filesystem", "View: Focus Filesystem", ICON_FA_FOLDER "  Filesystem");

	add("editor.view.style", "View: Toggle Style Editor",
		[this] { _showStyleEditor = !_showStyleEditor; });
	add("editor.debug.physics", "Debug: Toggle Physics Debug Draw",
		[this] { _showPhysicsDebug = !_showPhysicsDebug; });
	add("editor.debug.profiler", "Debug: Toggle Profiler",
		[this] { _showProfiler = !_showProfiler; });
	add("editor.project.export", "Project: Export...", [this] { _projectExport.Open(); }).enabled =
		editing;
	add("editor.scripts.reload", "Scripts: Reload Project Scripts", [&] {
		if (!scripts.ReloadModules(*_fileSystem, project.application.scripts)) {
			throw std::runtime_error("Project script reload failed; see the Console for details");
		}
		Log::Info("Project scripts reloaded");
	}).enabled = editing && _fileSystem;

	if (const auto chosen =
			_commandPalette.Draw(commands, scripts.CommandInputs(), scenes.IsPlaying())) {
		commitPendingSceneEdits(resources);
		std::string error;
		if (!commands.Execute(*chosen, scenes.IsPlaying(), error)) {
			Log::Error("Command '{}': {}", *chosen, error);
		}
	}
}

} // namespace sowa
