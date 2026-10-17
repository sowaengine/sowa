#include "editor/project_export_panel.hpp"
#include "build/build_runner.hpp"
#include "build/build_script.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/filesystem/folder_file_server.hpp"
#include "core/log.hpp"
#include "editor/icons.hpp"
#include "editor/style.hpp"
#include "editor/widgets.hpp"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>

namespace sowa::editor {

void ProjectExportPanel::Open() {
	_openRequested = true;
}

void ProjectExportPanel::Reset() {
	_openRequested = false;
	_projectRoot.clear();
	_targets = {};
	_script.reset();
	_loadError.clear();
	_selected = -1;
	_dirty = false;
	_status.clear();
	_succeeded = false;
}

void ProjectExportPanel::reload(FileSystem &files) {
	_targets = {};
	_script.reset();
	_loadError.clear();
	_dirty = false;
	_status.clear();
	_succeeded = false;

	const auto *folder = dynamic_cast<FolderFileServer *>(files.Get("res"));
	std::error_code ec;
	_projectRoot = folder ? std::filesystem::canonical(folder->Root(), ec) : "";
	if (!folder || ec) {
		_loadError = "Export requires an open project directory.";
		return;
	}

	auto script = std::make_shared<build::BuildScript>();
	std::string error;
	if (script->Load(_projectRoot, error)) {
		_script = std::move(script);
	} else {
		_loadError = "build.lua: " + error;
	}
	if (!_targets.Load(_projectRoot, error)) {
		_loadError = error;
	}
	if (_selected >= static_cast<int>(_targets.targets.size())) {
		_selected = static_cast<int>(_targets.targets.size()) - 1;
	}
	if (_selected < 0 && !_targets.targets.empty()) {
		_selected = 0;
	}
}

bool ProjectExportPanel::saveTargets() {
	if (!_dirty) {
		return true;
	}
	std::string error;
	if (!_targets.Save(_projectRoot, error)) {
		_status = error;
		_succeeded = false;
		return false;
	}
	_dirty = false;
	return true;
}

void ProjectExportPanel::drawTarget(build::ExportTarget &target) {
	const auto changed = [this] {
		_dirty = true;
		_status.clear();
	};
	const std::vector<std::string> &platforms =
		_script ? _script->Platforms() : build::KnownPlatforms();

	if (BeginInspectorTable("##export_target_fields", 104.f)) {
		InspectorFieldRow("name", "Name");
		if (ImGui::InputText("##", &target.name)) {
			changed();
		}
		ImGui::PopID();

		InspectorFieldRow("platform", "Platform");
		if (ImGui::BeginCombo("##", target.platform.c_str())) {
			for (const std::string &platform : platforms) {
				if (ImGui::Selectable(platform.c_str(), platform == target.platform)) {
					target.platform = platform;
					changed();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();

		InspectorFieldRow("output", "Output");
		if (ImGui::InputText("##", &target.output)) {
			changed();
		}
		ImGui::TextDisabled("%s", build::ExportTargetOutput(_projectRoot, target).string().c_str());
		ImGui::PopID();
		ImGui::EndTable();
	}

	const auto find = [&target](const std::string &name) -> std::string * {
		for (auto &[key, value] : target.params) {
			if (key == name) {
				return &value;
			}
		}
		return nullptr;
	};
	const auto set = [&](const std::string &name, std::string value) {
		if (std::string *existing = find(name)) {
			*existing = std::move(value);
		} else {
			target.params.emplace_back(name, std::move(value));
		}
		changed();
	};

	ImGui::SeparatorText("Params");
	if (!_script) {
		ImGui::TextDisabled("Fix build.lua to edit params.");
	} else if (_script->Params().empty()) {
		ImGui::TextDisabled("build.lua declares no params.");
	} else if (BeginInspectorTable("##export_target_params", 104.f)) {
		for (const build::ParamDecl &decl : _script->Params()) {
			const std::string label = decl.required ? decl.name + " *" : decl.name;
			InspectorFieldRow(decl.name.c_str(), label);
			if (!decl.description.empty() && ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", decl.description.c_str());
			}
			const std::string *text = find(decl.name);
			const std::string fallback = build::ParamValueToString(decl.defaultValue);
			const std::string current = text ? *text : fallback;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (text ? 40.f : 0.f));
			switch (decl.type) {
			case build::ParamType::Bool: {
				bool value = current == "true";
				if (ImGui::Checkbox("##", &value)) {
					set(decl.name, value ? "true" : "false");
				}
				break;
			}
			case build::ParamType::Int: {
				long long value = std::strtoll(current.c_str(), nullptr, 10);
				if (ImGui::InputScalar("##", ImGuiDataType_S64, &value)) {
					set(decl.name, std::to_string(value));
				}
				break;
			}
			case build::ParamType::Float: {
				double value = std::strtod(current.c_str(), nullptr);
				if (ImGui::InputDouble("##", &value)) {
					set(decl.name, fmt::format("{}", value));
				}
				break;
			}
			case build::ParamType::Enum: {
				const char *preview = current == "nil" ? "(choose)" : current.c_str();
				if (ImGui::BeginCombo("##", preview)) {
					for (const std::string &choice : decl.choices) {
						if (ImGui::Selectable(choice.c_str(), choice == current)) {
							set(decl.name, choice);
						}
					}
					ImGui::EndCombo();
				}
				break;
			}
			case build::ParamType::String:
			case build::ParamType::Path: {
				std::string value = text ? *text : (fallback == "nil" ? "" : fallback);
				if (ImGui::InputText("##", &value)) {
					set(decl.name, value);
				}
				break;
			}
			}
			if (text) {
				ImGui::SameLine();
				if (IconActionButton(ICON_FA_XMARK,
						decl.required ? "Clear value" : "Use build.lua's default",
						ImVec2(32.f, 0.f))) {
					target.params.erase(std::find_if(target.params.begin(), target.params.end(),
						[&](const auto &entry) { return entry.first == decl.name; }));
					changed();
				}
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	// Values build.lua no longer declares can only be removed.
	for (std::size_t i = 0; i < target.params.size();) {
		const auto &params = _script ? _script->Params() : std::vector<build::ParamDecl>{};
		const std::string &name = target.params[i].first;
		const bool declared = std::any_of(params.begin(), params.end(),
			[&](const build::ParamDecl &decl) { return decl.name == name; });
		if (declared || !_script) {
			++i;
			continue;
		}
		ImGui::PushID(name.c_str());
		ImGui::TextColored(theme::Rose, "Unknown param '%s' = %s", name.c_str(),
			target.params[i].second.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove")) {
			target.params.erase(target.params.begin() + static_cast<std::ptrdiff_t>(i));
			changed();
			ImGui::PopID();
			continue;
		}
		ImGui::PopID();
		++i;
	}

	if (_script) {
		const std::string problem =
			build::ValidateExportTarget(*_script, _targets, target, _projectRoot);
		if (!problem.empty()) {
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, theme::Rose);
			ImGui::TextWrapped("%s", problem.c_str());
			ImGui::PopStyleColor();
		}
	}
}

void ProjectExportPanel::runExport(const std::function<bool()> &prepare,
	std::vector<std::string> targets, bool all) {
	_status.clear();
	_succeeded = false;
	if (!prepare()) {
		_status = "Could not save an open scene. See the Console for details.";
		return;
	}
	if (!saveTargets()) {
		return;
	}

	build::BuildRequest request;
	request.projectRoot = _projectRoot;
	request.targets = std::move(targets);
	request.all = all;
	const build::BuildReport report = build::RunBuild(request);
	_succeeded = report.ok;
	if (report.ok) {
		_status = report.built.size() == 1
					  ? "Exported to " + report.built.front().second.string()
					  : fmt::format("Exported {} targets.", report.built.size());
		Log::Info("{}", _status);
	} else {
		_status = report.error;
		Log::Error("Export failed: {}", report.error);
	}
}

void ProjectExportPanel::Draw(FileSystem &files, bool playing, bool settingsUnsaved,
	const std::function<bool()> &prepare) {
	if (_openRequested) {
		_openRequested = false;
		reload(files);
		ImGui::OpenPopup("Export Project");
	}

	const DialogStyle dialogStyle;
	const auto *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(std::min(820.f, viewport->WorkSize.x - 32.f),
								 std::min(560.f, viewport->WorkSize.y - 32.f)),
		ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("Export Project", nullptr)) {
		return;
	}

	if (!_loadError.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, theme::Rose);
		ImGui::TextWrapped("%s", _loadError.c_str());
		ImGui::PopStyleColor();
	}

	auto &targets = _targets.targets;
	const float footer = ImGui::GetFrameHeightWithSpacing() * 2.f +
						 ImGui::GetTextLineHeightWithSpacing() * 2.f +
						 ImGui::GetStyle().ItemSpacing.y * 2.f;
	const float listWidth = 220.f;

	ImGui::BeginGroup();
	ImGui::BeginChild("##export_target_list",
		ImVec2(listWidth, -footer - ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
	for (int i = 0; i < static_cast<int>(targets.size()); ++i) {
		ImGui::PushID(i);
		const std::string label = targets[i].name + "  (" + targets[i].platform + ")";
		if (ImGui::Selectable(label.c_str(), _selected == i)) {
			_selected = i;
		}
		ImGui::PopID();
	}
	if (targets.empty()) {
		ImGui::TextDisabled("No targets yet.");
	}
	ImGui::EndChild();

	const auto uniqueName = [&](const std::string &base) {
		std::string name = base;
		for (int n = 2; _targets.Find(name); ++n) {
			name = base + "-" + std::to_string(n);
		}
		return name;
	};
	if (IconActionButton(ICON_FA_PLUS, "Add target")) {
		build::ExportTarget target;
		const std::vector<std::string> &platforms =
			_script ? _script->Platforms() : build::KnownPlatforms();
		const std::string host = build::HostPlatform();
		target.platform = std::find(platforms.begin(), platforms.end(), host) != platforms.end()
							  ? host
							  : platforms.front();
		target.name = uniqueName(target.platform);
		targets.push_back(std::move(target));
		_selected = static_cast<int>(targets.size()) - 1;
		_dirty = true;
	}
	const bool hasSelection = _selected >= 0 && _selected < static_cast<int>(targets.size());
	ImGui::SameLine();
	ImGui::BeginDisabled(!hasSelection);
	if (IconActionButton(ICON_FA_LAYER_GROUP, "Duplicate target")) {
		build::ExportTarget copy = targets[_selected];
		copy.name = uniqueName(copy.name + "-copy");
		copy.output.clear();
		targets.insert(targets.begin() + _selected + 1, std::move(copy));
		++_selected;
		_dirty = true;
	}
	ImGui::SameLine();
	if (IconActionButton(ICON_FA_TRASH, "Delete target")) {
		targets.erase(targets.begin() + _selected);
		_selected = std::min(_selected, static_cast<int>(targets.size()) - 1);
		_dirty = true;
	}
	ImGui::EndDisabled();
	ImGui::EndGroup();

	ImGui::SameLine();
	ImGui::BeginChild("##export_target_details", ImVec2(0.f, -footer));
	if (_selected >= 0 && _selected < static_cast<int>(targets.size())) {
		drawTarget(targets[_selected]);
	} else {
		ImGui::TextDisabled("Add a target to export the project.");
	}
	ImGui::EndChild();

	if (settingsUnsaved) {
		ImGui::TextWrapped("Save or revert your Project Settings changes before exporting.");
	} else {
		ImGui::TextDisabled("Open scene changes are saved before exporting.");
	}
	if (!_status.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, _succeeded ? theme::Green : theme::Rose);
		ImGui::TextWrapped("%s", _status.c_str());
		ImGui::PopStyleColor();
	}

	ImGui::Separator();
	if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		if (saveTargets()) {
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::SameLine();
	const bool canBuild = _script && !settingsUnsaved && !playing;
	ImGui::BeginDisabled(!canBuild || !hasSelection);
	if (ImGui::Button("Build Selected")) {
		runExport(prepare, {targets[_selected].name}, false);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!canBuild || targets.empty());
	if (ImGui::Button("Build All")) {
		runExport(prepare, {}, true);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!_dirty);
	if (ImGui::Button("Save Targets")) {
		saveTargets();
	}
	ImGui::EndDisabled();
	ImGui::EndPopup();
}

} // namespace sowa::editor
