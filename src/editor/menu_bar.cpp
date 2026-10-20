#include "core/window.hpp"
#include "editor/editor.hpp"
#include "editor/icons.hpp"
#include "editor/play_controls.hpp"
#include "editor/project_manager.hpp"
#include "editor/style.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"

#include <algorithm>

namespace sowa {
namespace {

ImU32 colorWithAlpha(const ImVec4 &color, float alpha) {
	ImVec4 result = color;
	result.w *= alpha;
	return ImGui::ColorConvertFloat4ToU32(result);
}

void drawMenuBarInteraction(ImDrawList &draw, const ImVec2 &min, const ImVec2 &max, bool hovered,
	bool held, bool selected = false) {
	if (hovered || held || selected) {
		// A neutral translucent overlay keeps the underlying gradient visible.
		const float alpha = held ? 0.18f : selected ? 0.14f : 0.09f;
		draw.AddRectFilled(min, max, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, alpha)));
	}
}

bool menuBarNavButton(const char *id, const char *label, const ImVec2 &size, bool menuOpen) {
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const bool pressed = ImGui::InvisibleButton(id, size);
	const bool hovered = ImGui::IsItemHovered();
	const bool held = ImGui::IsItemActive();
	const ImVec2 max(min.x + size.x, min.y + size.y);
	ImDrawList *draw = ImGui::GetWindowDrawList();

	drawMenuBarInteraction(*draw, min, max, hovered, held, menuOpen);
	if (menuOpen) {
		draw->AddRectFilled(ImVec2(min.x + 7.f, max.y - 2.f), ImVec2(max.x - 7.f, max.y),
			colorWithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Text), 0.65f), 1.f);
	}

	const ImVec2 textSize = ImGui::CalcTextSize(label);
	const ImVec2 textPos(min.x + 10.f, min.y + (size.y - textSize.y) * 0.5f);
	draw->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), label);
	const float arrowX = max.x - 11.f;
	const float arrowY = min.y + size.y * 0.5f;
	draw->AddTriangleFilled(ImVec2(arrowX - 3.f, arrowY - 2.f), ImVec2(arrowX + 3.f, arrowY - 2.f),
		ImVec2(arrowX, arrowY + 2.f),
		colorWithAlpha(ImGui::GetStyleColorVec4(ImGuiCol_Text), 0.55f));
	return pressed;
}

bool menuBarIconButton(const char *id, const char *icon, const ImVec2 &size, bool enabled,
	bool selected = false) {
	ImGui::BeginDisabled(!enabled);
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const bool pressed = ImGui::InvisibleButton(id, size);
	const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
	const bool held = ImGui::IsItemActive();
	ImGui::EndDisabled();

	const ImVec2 max(min.x + size.x, min.y + size.y);
	ImDrawList *draw = ImGui::GetWindowDrawList();
	drawMenuBarInteraction(*draw, min, max, hovered && enabled, held && enabled,
		selected && enabled);

	const ImVec2 iconSize = ImGui::CalcTextSize(icon);
	const ImVec2 iconPos(min.x + (size.x - iconSize.x) * 0.5f,
		min.y + (size.y - iconSize.y) * 0.5f);
	draw->AddText(iconPos, ImGui::GetColorU32(ImGuiCol_Text, enabled ? 1.f : 0.4f), icon);
	return pressed && enabled;
}

} // namespace

void Editor::drawDockHost(SceneManager &scenes, const ProjectSettings &project,
	ResourceRegistry &resources) {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
							 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar |
							 ImGuiWindowFlags_NoBackground;

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 8.f));
	ImGui::Begin("##EditorDock", nullptr, flags);
	ImGui::PopStyleVar(3);

	const Scene editorCurrent = scenes.EditorCurrent();
	const bool canSaveScene =
		!scenes.IsPlaying() && editorCurrent.IsValid() && !editorCurrent.SourcePath().empty();
	const editor::SceneDocumentSaveState *documentState =
		editor::FindSceneDocumentState(_sceneSaveStates, editorCurrent);
	const bool canUndoScene =
		!scenes.IsPlaying() && documentState != nullptr && documentState->history.CanUndo();
	const bool canRedoScene =
		!scenes.IsPlaying() && documentState != nullptr && documentState->history.CanRedo();
	if (canSaveScene && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S,
							ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive)) {
		_saveSceneRequested = true;
	}
	if (!ImGui::GetIO().WantTextInput && canUndoScene &&
		ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal)) {
		_undoSceneRequested = true;
	}
	if (!ImGui::GetIO().WantTextInput && canRedoScene &&
		(ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal) ||
			ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z,
				ImGuiInputFlags_RouteGlobal))) {
		_redoSceneRequested = true;
	}

	const ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");
	if (_applyDefaultDock) {
		_applyDefaultDock = false;
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

		ImGuiID main = dockspaceId;
		ImGuiID left = 0;
		ImGuiID right = 0;
		ImGuiID leftBottom = 0;
		ImGuiID bottom = 0;
		ImGui::DockBuilderSplitNode(main, ImGuiDir_Left, 0.20f, &left, &main);
		ImGui::DockBuilderSplitNode(main, ImGuiDir_Right, 0.31f, &right, &main);
		ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.45f, &leftBottom, &left);
		ImGui::DockBuilderSplitNode(main, ImGuiDir_Down, 0.29f, &bottom, &main);

		ImGui::DockBuilderDockWindow(ICON_FA_SITEMAP "  Hierarchy", left);
		ImGui::DockBuilderDockWindow(ICON_FA_FOLDER "  Filesystem", leftBottom);
		ImGui::DockBuilderDockWindow(ICON_FA_SLIDERS "  Inspector", right);
		ImGui::DockBuilderDockWindow(ICON_FA_TERMINAL "  Console", bottom);
		ImGui::DockBuilderDockWindow("Animation", bottom);
		ImGui::DockBuilderDockWindow("Tilemap", bottom);
		ImGui::DockBuilderDockWindow("Sprite Sheet", bottom);
		ImGui::DockBuilderDockWindow(ICON_FA_CUBE "  Viewport", main);
		ImGui::DockBuilderFinish(dockspaceId);
	}
	if (ImGui::BeginMenuBar()) {
		const ImRect bar = ImGui::GetCurrentWindow()->MenuBarRect();
		const ImVec2 middle(bar.Min.x + bar.GetWidth() * 0.58f, bar.Min.y);
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const ImU32 start = ImGui::GetColorU32(editor::theme::TitleStart);
		const ImU32 mid = ImGui::GetColorU32(editor::theme::TitleMiddle);
		const ImU32 end = ImGui::GetColorU32(editor::theme::TitleEnd);
		draw->AddRectFilledMultiColor(bar.Min, ImVec2(middle.x, bar.Max.y), start, mid, mid, start);
		draw->AddRectFilledMultiColor(middle, bar.Max, mid, end, end, mid);
		// Targets fill the bar; text and icons share its vertical center.
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, bar.Min.y));
		const auto menuButtonSize = [&](const char *label) {
			return ImVec2(ImGui::CalcTextSize(label).x + 34.f, bar.GetHeight());
		};
		const bool openFile = menuBarNavButton("##top_menu_file", "File", menuButtonSize("File"),
			ImGui::IsPopupOpen("##file_menu"));
		const ImVec2 fileMenuPos(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f);
		ImGui::SameLine();
		const bool openEdit = menuBarNavButton("##top_menu_edit", "Edit", menuButtonSize("Edit"),
			ImGui::IsPopupOpen("##edit_menu"));
		const ImVec2 editMenuPos(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f);
		ImGui::SameLine();
		const bool openProject = menuBarNavButton("##top_menu_project", "Project",
			menuButtonSize("Project"), ImGui::IsPopupOpen("##project_menu"));
		const ImVec2 projectMenuPos(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f);
		ImGui::SameLine();
		const bool openDebug = menuBarNavButton("##top_menu_debug", "Debug",
			menuButtonSize("Debug"), ImGui::IsPopupOpen("##debug_menu"));
		const ImVec2 debugMenuPos(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2.f);

		if (openFile) {
			ImGui::OpenPopup("##file_menu");
		}
		ImGui::SetNextWindowPos(fileMenuPos, ImGuiCond_Appearing);
		if (const editor::MenuStyle menuStyle; ImGui::BeginPopup("##file_menu")) {
			if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, canSaveScene)) {
				_saveSceneRequested = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Quit") && _window) {
				_window->SetShouldClose(true);
			}
			ImGui::EndPopup();
		}
		if (openEdit) {
			ImGui::OpenPopup("##edit_menu");
		}
		ImGui::SetNextWindowPos(editMenuPos, ImGuiCond_Appearing);
		if (const editor::MenuStyle menuStyle; ImGui::BeginPopup("##edit_menu")) {
			if (ImGui::MenuItem("Command Palette...", "Ctrl+Shift+P")) {
				_commandPalette.Open();
			}
			ImGui::Separator();
			const std::string undoLabel =
				documentState != nullptr && canUndoScene
					? "Undo " + std::string(documentState->history.UndoName())
					: "Undo";
			const std::string redoLabel =
				documentState != nullptr && canRedoScene
					? "Redo " + std::string(documentState->history.RedoName())
					: "Redo";
			if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndoScene)) {
				_undoSceneRequested = true;
			}
			if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canRedoScene)) {
				_redoSceneRequested = true;
			}
			ImGui::Separator();
			drawEntityActions(Selected(),
				Selected().IsAlive() ? Selected() : scenes.EditorCurrent().Root(),
				scenes.IsPlaying(), resources);
			ImGui::EndPopup();
		}
		if (openProject) {
			ImGui::OpenPopup("##project_menu");
		}
		ImGui::SetNextWindowPos(projectMenuPos, ImGuiCond_Appearing);
		if (const editor::MenuStyle menuStyle; ImGui::BeginPopup("##project_menu")) {
			if (ImGui::MenuItem("Project Settings")) {
				_projectSettings.Open();
			}
			if (ImGui::MenuItem("Project Manager...")) {
				std::string error;
				if (!editor::LaunchProjectManager(error)) {
					Log::Error("Cannot open Project Manager: {}", error);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export...", nullptr, false, !scenes.IsPlaying())) {
				_projectExport.Open();
			}
			ImGui::EndPopup();
		}
		if (openDebug) {
			ImGui::OpenPopup("##debug_menu");
		}
		ImGui::SetNextWindowPos(debugMenuPos, ImGuiCond_Appearing);
		if (const editor::MenuStyle menuStyle; ImGui::BeginPopup("##debug_menu")) {
			ImGui::MenuItem("Style editor", nullptr, &_showStyleEditor);
			ImGui::MenuItem("Physics debug draw", nullptr, &_showPhysicsDebug);
			ImGui::MenuItem("Profiler", nullptr, &_showProfiler);
			ImGui::EndPopup();
		}
		drawPlayControls(scenes, project, resources);
		draw->AddLine(ImVec2(bar.Min.x, bar.Max.y - 1.f), ImVec2(bar.Max.x, bar.Max.y - 1.f),
			ImGui::GetColorU32(editor::theme::TitleDivider));
		ImGui::EndMenuBar();
	}
	ImGui::PopStyleVar();

	ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f));
	ImGui::End();
}

void Editor::drawPlayControls(SceneManager &scenes, const ProjectSettings &project,
	ResourceRegistry &resources) {
	const editor::PlayControlAvailability available = editor::AvailablePlayControls(
		scenes.PlayMode(), scenes.IsPaused(), scenes.Current().IsValid(),
		!project.application.mainScene.empty(), scenes.IsScriptSuspended());
	const ImVec2 buttonSize(36.f, ImGui::GetCurrentWindow()->MenuBarRect().GetHeight());
	const float controlsWidth = buttonSize.x * 5.f + ImGui::GetStyle().ItemSpacing.x * 4.f;
	const float rightAligned = ImGui::GetWindowWidth() - controlsWidth - 28.f;
	ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), rightAligned));

	if (menuBarIconButton("##play_project", ICON_FA_PLAY, buttonSize, available.playProject)) {
		commitPendingSceneEdits(resources);
		StopAnimationPreview();
		editor::RequestPlayControl(scenes, editor::PlayControl::PlayProject,
			project.application.mainScene);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Play project (F5)");
	}

	ImGui::SameLine();
	const bool paused = scenes.IsPaused();
	if (menuBarIconButton("##pause", ICON_FA_PAUSE, buttonSize, available.pause || available.resume,
			paused)) {
		editor::RequestPlayControl(scenes,
			paused ? editor::PlayControl::Resume : editor::PlayControl::Pause,
			project.application.mainScene);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip(paused ? "Resume (F7)" : "Pause (F7)");
	}

	ImGui::SameLine();
	if (menuBarIconButton("##step", ICON_FA_FORWARD_STEP, buttonSize, available.step)) {
		editor::RequestPlayControl(scenes, editor::PlayControl::Step,
			project.application.mainScene);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Step one frame (F9)");
	}

	ImGui::SameLine();
	if (menuBarIconButton("##stop", ICON_FA_STOP, buttonSize, available.stop)) {
		editor::RequestPlayControl(scenes, editor::PlayControl::Stop,
			project.application.mainScene);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Stop (F8)");
	}

	ImGui::SameLine();
	if (menuBarIconButton("##play_current", ICON_FA_SITEMAP, buttonSize, available.playCurrent)) {
		commitPendingSceneEdits(resources);
		StopAnimationPreview();
		editor::RequestPlayControl(scenes, editor::PlayControl::PlayCurrent,
			project.application.mainScene);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Play current scene (F6)");
	}
}

} // namespace sowa
