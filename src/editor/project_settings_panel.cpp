#include "editor/project_settings_panel.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/input_binding.hpp"
#include "core/log.hpp"
#include "editor/asset_picker.hpp"
#include "editor/icons.hpp"
#include "editor/project_data.hpp"
#include "editor/project_settings.hpp"
#include "editor/style.hpp"
#include "editor/widgets.hpp"
#include "imgui_internal.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cfloat>
#include <utility>

namespace sowa::editor {

ProjectSettingsPanel::~ProjectSettingsPanel() {
	Shutdown();
}

void ProjectSettingsPanel::Init(Input *input) {
	Shutdown();
	_input = input;
	if (_input) {
		_bindingCaptureHandle = _input->UnfilteredEvents().append(
			[this](const InputEvent &event) { _bindingCapture.Feed(event); });
	}
}

void ProjectSettingsPanel::Shutdown() {
	if (_input) {
		_input->UnfilteredEvents().remove(_bindingCaptureHandle);
		_input = nullptr;
	}
	if (ImGui::GetCurrentContext() && _bindingCapture.GetState() != BindingCapture::State::Idle) {
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}
	_bindingCapture = {};
	_bindingCaptureAction = 0;
	_openRequested = false;
	_draft = {};
	_source = {};
	_draftInitialized = false;
	_saveRequest.reset();
	_saveStatus.clear();
	_saveFailed = false;
	_assetPickerTarget = AssetPickerTarget::None;
	_assetPickerIndex.reset();
	_openAssetPicker = false;
	_assetPickerError.clear();
	_newStoreKey.clear();
	_newStoreValue.clear();
}

void ProjectSettingsPanel::Open() {
	_openRequested = true;
}

void ProjectSettingsPanel::BeginFrame() {
	if (_bindingCapture.GetState() != BindingCapture::State::Idle) {
		const ImGuiID owner = ImHashStr("BindingCapture");
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
			ImGui::SetKeyOwner(static_cast<ImGuiKey>(key), owner, ImGuiInputFlags_LockUntilRelease);
		}
	}
}

bool ProjectSettingsPanel::HasUnsavedChanges(const ProjectSettings &project) const {
	return _saveRequest.has_value() ||
		   (_draftInitialized && !EditableProjectSettingsEqual(_draft, project));
}

std::optional<ProjectSettings> ProjectSettingsPanel::TakeSaveRequest() {
	return std::exchange(_saveRequest, std::nullopt);
}

void ProjectSettingsPanel::ReportSaveResult(bool success, std::string message) {
	_saveStatus = std::move(message);
	_saveFailed = !success;
}

void ProjectSettingsPanel::drawPathList(const char *id, std::vector<std::string> &paths,
	bool scripts) {
	ImGui::PushID(id);
	std::optional<std::size_t> remove;
	std::optional<std::size_t> moveUp;
	std::optional<std::size_t> moveDown;
	const ImGuiTableFlags flags =
		ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
	if (ImGui::BeginTable("##paths", 2, flags)) {
		ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 156.f);
		for (std::size_t i = 0; i < paths.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(paths[i].c_str());
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", paths[i].c_str());
			}

			ImGui::TableNextColumn();
			const ImVec2 itemSpacing = ImGui::GetStyle().ItemSpacing;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.f, itemSpacing.y));
			if (IconActionButton(ICON_FA_PEN_TO_SQUARE, "Change")) {
				_assetPickerTarget =
					scripts ? AssetPickerTarget::Script : AssetPickerTarget::Autoload;
				_assetPickerIndex = i;
				_assetPickerError.clear();
				_openAssetPicker = true;
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(i == 0);
			if (IconActionButton(ICON_FA_ARROW_UP, "Move up")) {
				moveUp = i;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(i + 1 == paths.size());
			if (IconActionButton(ICON_FA_ARROW_DOWN, "Move down")) {
				moveDown = i;
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (IconActionButton(ICON_FA_TRASH, "Remove")) {
				remove = i;
			}
			ImGui::PopStyleVar();
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (moveUp) {
		std::swap(paths[*moveUp], paths[*moveUp - 1]);
	} else if (moveDown) {
		std::swap(paths[*moveDown], paths[*moveDown + 1]);
	} else if (remove) {
		paths.erase(paths.begin() + static_cast<std::ptrdiff_t>(*remove));
	}

	if (IconActionButton(ICON_FA_PLUS, scripts ? "Add script" : "Add autoload")) {
		_assetPickerTarget = scripts ? AssetPickerTarget::Script : AssetPickerTarget::Autoload;
		_assetPickerIndex.reset();
		_assetPickerError.clear();
		_openAssetPicker = true;
	}
	ImGui::PopID();
}

void ProjectSettingsPanel::drawAssetPicker(FileSystem &files, bool playing) {
	if (_openAssetPicker) {
		ImGui::OpenPopup("##project_asset_picker");
		_openAssetPicker = false;
	}

	const DialogStyle dialogStyle;
	if (!BeginAssetPickerPopup("##project_asset_picker")) {
		return;
	}
	if (playing) {
		_assetPickerTarget = AssetPickerTarget::None;
		_assetPickerIndex.reset();
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	const bool script = _assetPickerTarget == AssetPickerTarget::Script;
	const bool icon = _assetPickerTarget == AssetPickerTarget::Icon;
	const auto picker = DrawAssetPicker(&files,
		script ? AssetKind::Script : (icon ? AssetKind::Image : AssetKind::Scene),
		script ? "Choose a script" : (icon ? "Choose an icon" : "Choose a scene"),
		_assetPickerError);
	if (const auto &path = picker.path; path) {
		bool accepted = true;
		if (_assetPickerTarget == AssetPickerTarget::MainScene) {
			_draft.application.mainScene = *path;
		} else if (icon) {
			_draft.application.icon = *path;
		} else {
			std::vector<std::string> &paths =
				script ? _draft.application.scripts : _draft.application.autoloads;
			const auto duplicate = std::find(paths.begin(), paths.end(), *path);
			if (duplicate != paths.end() &&
				(!_assetPickerIndex ||
					static_cast<std::size_t>(duplicate - paths.begin()) != *_assetPickerIndex)) {
				_assetPickerError = "That file is already in the list.";
				accepted = false;
			} else if (_assetPickerIndex && *_assetPickerIndex < paths.size()) {
				paths[*_assetPickerIndex] = *path;
			} else {
				paths.push_back(*path);
			}
		}
		if (accepted) {
			_assetPickerTarget = AssetPickerTarget::None;
			_assetPickerIndex.reset();
			_assetPickerError.clear();
			ImGui::CloseCurrentPopup();
		}
	}
	if (picker.cancelled) {
		_assetPickerTarget = AssetPickerTarget::None;
		_assetPickerIndex.reset();
		_assetPickerError.clear();
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void ProjectSettingsPanel::Draw(ProjectSettings &project, FileSystem &files, bool playing) {
	if (!_draftInitialized) {
		_draft = project;
		_source = project;
		_draftInitialized = true;
	} else {
		// Inspector actions can add a startup script directly to the live
		// settings. Pull external changes into a clean draft, but never overwrite
		// edits the user already has pending in this panel.
		const bool draftWasClean = EditableProjectSettingsEqual(_draft, _source);
		const bool sourceChanged = !EditableProjectSettingsEqual(_source, project);
		if (draftWasClean && sourceChanged) {
			_draft = project;
		}
		_source = project;
	}

	if (_openRequested) {
		ImGui::OpenPopup("Project Settings");
		_openRequested = false;
	}
	const DialogStyle dialogStyle;
	const auto *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, {0.5f, 0.5f});
	ImGui::SetNextWindowSize({std::min(760.f, viewport->WorkSize.x - 40.f),
								 std::min(720.f, viewport->WorkSize.y - 40.f)},
		ImGuiCond_Appearing);
	bool open = true;
	if (!ImGui::BeginPopupModal("Project Settings", &open)) {
		_bindingCapture.Cancel();
		finishBindingCapture();
		return;
	}
	if (playing) {
		_bindingCapture.Cancel();
	}
	finishBindingCapture();

	const bool pending = _saveRequest.has_value();
	const bool dirty = !EditableProjectSettingsEqual(_draft, project);
	const std::string problem = ValidateEditableProjectSettings(_draft);

	ImGui::BeginDisabled(playing || pending);
	ImGui::BeginDisabled(!dirty || !problem.empty());
	if (ImGui::Button("Save")) {
		_saveRequest = _draft;
		_saveStatus = "Saving and applying...";
		_saveFailed = false;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!dirty);
	if (ImGui::Button("Revert")) {
		_draft = project;
		_bindingCapture.Cancel();
		_newStoreKey.clear();
		_newStoreValue.clear();
		_assetPickerTarget = AssetPickerTarget::None;
		_assetPickerIndex.reset();
		_saveStatus.clear();
	}
	ImGui::EndDisabled();
	ImGui::EndDisabled();

	if (playing) {
		ImGui::SameLine();
		ImGui::TextDisabled("Read-only while playing");
	} else if (dirty) {
		ImGui::SameLine();
		ImGui::TextDisabled("Unsaved changes");
	}

	if (!problem.empty()) {
		ImGui::TextColored(ImVec4(1.f, 0.48f, 0.42f, 1.f), "%s", problem.c_str());
	} else if (!_saveStatus.empty()) {
		ImGui::TextColored(_saveFailed ? ImVec4(1.f, 0.48f, 0.42f, 1.f)
									   : ImVec4(0.48f, 0.84f, 0.55f, 1.f),
			"%s", _saveStatus.c_str());
	}

	bool inputTabActive = false;
	if (ImGui::BeginTabBar("##project_settings_tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
		if (ImGui::BeginTabItem(ICON_FA_CUBE " Application")) {
			ImGui::BeginChild("##project_application_content");
			ImGui::BeginDisabled(playing || pending);
			if (BeginInspectorTable("##project_application", 104.f)) {
				InspectorFieldRow("project_name", "Name");
				ImGui::InputText("##", &_draft.application.name);
				ImGui::PopID();

				InspectorFieldRow("project_author", "Author");
				ImGui::InputText("##", &_draft.application.author);
				ImGui::PopID();

				InspectorFieldRow("project_version", "Version");
				ImGui::InputText("##", &_draft.application.version);
				ImGui::PopID();

				InspectorFieldRow("project_icon", "Icon");
				const std::string &icon = _draft.application.icon;
				ImGui::TextUnformatted(icon.empty() ? "None" : icon.c_str());
				if (IconActionButton(ICON_FA_FOLDER_OPEN, "Choose icon")) {
					_assetPickerTarget = AssetPickerTarget::Icon;
					_assetPickerIndex.reset();
					_assetPickerError.clear();
					_openAssetPicker = true;
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(icon.empty());
				if (IconActionButton(ICON_FA_XMARK, "Clear icon")) {
					_draft.application.icon.clear();
				}
				ImGui::EndDisabled();
				ImGui::PopID();

				InspectorFieldRow("project_main_scene", "Main Scene");
				const std::string &mainScene = _draft.application.mainScene;
				ImGui::TextUnformatted(mainScene.empty() ? "None" : mainScene.c_str());
				if (IconActionButton(ICON_FA_FOLDER_OPEN, "Choose main scene")) {
					_assetPickerTarget = AssetPickerTarget::MainScene;
					_assetPickerIndex.reset();
					_assetPickerError.clear();
					_openAssetPicker = true;
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(mainScene.empty());
				if (IconActionButton(ICON_FA_XMARK, "Clear main scene")) {
					_draft.application.mainScene.clear();
				}
				ImGui::EndDisabled();
				ImGui::PopID();
				ImGui::EndTable();
			}

			ImGui::SeparatorText("Startup Scripts");
			drawPathList("scripts", _draft.application.scripts, true);
			ImGui::SeparatorText("Autoload Scenes");
			drawPathList("autoloads", _draft.application.autoloads, false);
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_FA_IMAGE " Rendering")) {
			ImGui::BeginChild("##project_rendering_content");
			ImGui::BeginDisabled(playing || pending);
			if (BeginInspectorTable("##project_rendering", 104.f)) {
				int windowSize[2] = {_draft.rendering.window.width, _draft.rendering.window.height};
				InspectorFieldRow("project_window_size", "Window Size");
				if (ImGui::InputInt2("##", windowSize)) {
					_draft.rendering.window.width = windowSize[0];
					_draft.rendering.window.height = windowSize[1];
				}
				ImGui::PopID();

				int viewportSize[2] = {_draft.rendering.viewport.width,
					_draft.rendering.viewport.height};
				InspectorFieldRow("project_viewport_size", "Viewport Size");
				if (ImGui::InputInt2("##", viewportSize)) {
					_draft.rendering.viewport.width = viewportSize[0];
					_draft.rendering.viewport.height = viewportSize[1];
				}
				ImGui::PopID();

				InspectorFieldRow("project_scale_mode", "Scale Mode");
				const char *preview =
					ViewportScaleModeToString(_draft.rendering.viewport.scaleMode);
				if (ImGui::BeginCombo("##", preview)) {
					for (const EnumConstant &constant : ViewportScaleModeConstants()) {
						const auto mode = static_cast<ViewportScaleMode>(constant.value);
						const bool selected = mode == _draft.rendering.viewport.scaleMode;
						if (ImGui::Selectable(constant.name.c_str(), selected)) {
							_draft.rendering.viewport.scaleMode = mode;
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopID();
				ImGui::EndTable();
			}
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_FA_KEYBOARD " Input")) {
			inputTabActive = true;
			ImGui::BeginChild("##project_input_content");
			ImGui::BeginDisabled(playing || pending);
			drawInputActions();
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_FA_DATABASE " Global Store")) {
			ImGui::BeginChild("##project_global_store_content");
			ImGui::BeginDisabled(playing || pending);
			StringStore &store = _draft.globalStore;
			const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
										  ImGuiTableFlags_SizingStretchProp |
										  ImGuiTableFlags_PadOuterX;
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.f, 4.f));
			if (ImGui::BeginTable("##global_store", 3, flags)) {
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 0.8f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.2f);
				ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 48.f);
				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.f, 8.f));
				ImGui::TableHeadersRow();
				ImGui::PopStyleVar();

				std::string removeKey;
				for (const GlobalStoreEntry &entry : GlobalStoreEntries(store)) {
					ImGui::PushID(entry.key.c_str());
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(entry.key.c_str());
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-FLT_MIN);
					std::string value = entry.value;
					if (ImGui::InputText("##value", &value)) {
						store.Set(entry.key, value);
					}
					ImGui::TableNextColumn();
					if (IconActionButton(ICON_FA_TRASH, "Remove global-store entry")) {
						removeKey = entry.key;
					}
					ImGui::PopID();
				}
				if (!removeKey.empty()) {
					TryRemoveGlobalStoreEntry(store, removeKey);
				}
				ImGui::EndTable();
			}

			ImGui::PopStyleVar();

			ImGui::SeparatorText("Add entry");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.35f);
			ImGui::InputTextWithHint("##new_key", "Key", &_newStoreKey);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-56.f);
			ImGui::InputTextWithHint("##new_value", "Value", &_newStoreValue);
			ImGui::SameLine();
			const bool canAdd = CanAddGlobalStoreEntry(store, _newStoreKey);
			ImGui::BeginDisabled(!canAdd);
			if (IconActionButton(ICON_FA_PLUS, "Add global-store entry")) {
				TryAddGlobalStoreEntry(store, _newStoreKey, _newStoreValue);
				_newStoreKey.clear();
				_newStoreValue.clear();
			}
			ImGui::EndDisabled();
			if (!_newStoreKey.empty() && store.Has(_newStoreKey)) {
				ImGui::TextDisabled("That key already exists.");
			}
			ImGui::EndDisabled();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	if (!inputTabActive) {
		_bindingCapture.Cancel();
		finishBindingCapture();
	}

	drawAssetPicker(files, playing);

	ImGui::EndPopup();
}

void ProjectSettingsPanel::beginBindingCapture(std::size_t action) {
	_bindingCaptureAction = action;
	_bindingCapture.Begin();
	// Navigation runs inside NewFrame, before BeginFrame can lock keys, so
	// Space/Enter would otherwise activate the focused Listen button again.
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void ProjectSettingsPanel::finishBindingCapture() {
	using State = BindingCapture::State;
	if (_bindingCapture.GetState() != State::Captured &&
		_bindingCapture.GetState() != State::Cancelled) {
		return;
	}
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	const std::optional<InputBinding> binding = _bindingCapture.Take();
	std::vector<InputAction> &actions = _draft.input.actions;
	if (!binding || _bindingCaptureAction >= actions.size()) {
		return;
	}
	// The one place the editor adds bindings, so the one duplicate check.
	std::vector<InputBinding> &bindings = actions[_bindingCaptureAction].bindings;
	if (std::find(bindings.begin(), bindings.end(), *binding) == bindings.end()) {
		bindings.push_back(*binding);
	}
}

void ProjectSettingsPanel::drawInputActions() {
	std::vector<InputAction> &actions = _draft.input.actions;
	const bool listening = _bindingCapture.IsListening();

	std::optional<std::size_t> removeAction;
	const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
								  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX;
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.f, 4.f));
	if (!actions.empty() && ImGui::BeginTable("##input_actions", 3, flags)) {
		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.7f);
		ImGui::TableSetupColumn("Bindings", ImGuiTableColumnFlags_WidthStretch, 1.6f);
		ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 78.f);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.f, 8.f));
		ImGui::TableHeadersRow();
		ImGui::PopStyleVar();

		for (std::size_t i = 0; i < actions.size(); ++i) {
			InputAction &action = actions[i];
			ImGui::PushID(static_cast<int>(i));
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputText("##name", &action.name);

			ImGui::TableNextColumn();
			std::optional<std::size_t> removeBinding;
			const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
			for (std::size_t j = 0; j < action.bindings.size(); ++j) {
				const InputBinding &binding = action.bindings[j];
				const std::vector<std::string> shared = ActionsSharingBinding(actions, i, binding);
				std::string label = FormatInputBinding(binding);
				if (!shared.empty()) {
					label = std::string(ICON_FA_TRIANGLE_EXCLAMATION) + " " + label;
				}
				label += "  " ICON_FA_XMARK "##" + std::to_string(j);

				const float width = ImGui::CalcTextSize(label.c_str(), nullptr, true).x +
									ImGui::GetStyle().FramePadding.x * 2.f;
				if (j > 0) {
					ImGui::SameLine();
					if (ImGui::GetCursorScreenPos().x + width > right) {
						ImGui::NewLine();
					}
				}
				if (ImGui::SmallButton(label.c_str())) {
					removeBinding = j;
				}
				if (ImGui::IsItemHovered()) {
					std::string tip = "Remove binding";
					if (!shared.empty()) {
						tip += "\nAlso bound to:";
						for (const std::string &name : shared) {
							tip += " " + name;
						}
					}
					ImGui::SetTooltip("%s", tip.c_str());
				}
			}
			if (removeBinding) {
				action.bindings.erase(
					action.bindings.begin() + static_cast<std::ptrdiff_t>(*removeBinding));
			}
			if (listening && _bindingCaptureAction == i) {
				ImGui::TextColored(theme::Accent, "Press a key or mouse button...");
			} else if (action.bindings.empty()) {
				ImGui::TextDisabled("No bindings");
			}

			ImGui::TableNextColumn();
			ImGui::BeginDisabled(listening);
			if (IconActionButton(ICON_FA_KEYBOARD, "Listen for a key or mouse button")) {
				beginBindingCapture(i);
			}
			ImGui::SameLine();
			if (IconActionButton(ICON_FA_TRASH, "Remove action")) {
				removeAction = i;
			}
			ImGui::EndDisabled();
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();

	if (removeAction) {
		actions.erase(actions.begin() + static_cast<std::ptrdiff_t>(*removeAction));
	}

	if (actions.empty()) {
		ImGui::TextDisabled("No input actions. Scripts query actions by name, e.g. "
							"input:is_action_pressed(\"jump\").");
	}
	ImGui::BeginDisabled(listening);
	if (ImGui::Button(ICON_FA_PLUS " Add action")) {
		actions.push_back({UniqueActionName(actions, "new_action"), {}});
	}
	ImGui::EndDisabled();
}

} // namespace sowa::editor
