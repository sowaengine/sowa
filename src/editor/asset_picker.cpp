#include "editor/asset_picker.hpp"

#include <algorithm>

#include "core/filesystem/file_system.hpp"
#include "editor/icons.hpp"
#include "editor/style.hpp"

namespace sowa::editor {
namespace {
struct Appearance {
	const char *icon;
	ImVec4 color;
	const char *description;
};

Appearance appearance(AssetKind kind) {
	switch (kind) {
	case AssetKind::Scene:
		return {ICON_FA_FILM, theme::Blue, "Scene files"};
	case AssetKind::Script:
		return {ICON_FA_FILE_CODE, theme::Green, "Lua scripts"};
	case AssetKind::Image:
		return {ICON_FA_IMAGE, theme::Blue, "Images"};
	case AssetKind::Audio:
		return {ICON_FA_MUSIC, theme::Amber, "Audio files"};
	case AssetKind::Font:
		return {ICON_FA_FONT, theme::Rose, "Fonts"};
	default:
		return {ICON_FA_FILE, theme::Blue, "Project files"};
	}
}

std::optional<std::string> drawDirectory(FileSystem &files, const std::string &directory,
	AssetKind accepted, const std::string &currentPath, bool defaultOpen) {
	std::optional<std::string> selected;
	bool any = false;
	for (const auto &entry : ListAssets(files, directory)) {
		const auto path = JoinVirtualPath(directory, entry.name);
		const auto kind = KindFromName(entry.name, entry.isDirectory);
		if (!entry.isDirectory && accepted != AssetKind::Other && kind != accepted) {
			continue;
		}
		any = true;
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
		if (entry.isDirectory) {
			const bool containsCurrent = currentPath.starts_with(path + "/");
			ImGui::SetNextItemOpen(defaultOpen || containsCurrent, ImGuiCond_Once);
			if (TreeRow(path.c_str(), entry.name.c_str(), ICON_FA_FOLDER, theme::Blue,
					flags | ImGuiTreeNodeFlags_OpenOnArrow |
						ImGuiTreeNodeFlags_OpenOnDoubleClick)) {
				if (auto child = drawDirectory(files, path, accepted, currentPath, false)) {
					selected = child;
				}
				ImGui::TreePop();
			}
		} else {
			const auto look = appearance(kind);
			if (path == currentPath) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}
			TreeRow(path.c_str(), entry.name.c_str(), look.icon, look.color,
				flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			if (ImGui::IsItemClicked()) {
				selected = path;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", path.c_str());
			}
		}
	}
	if (!any) {
		ImGui::TextDisabled("No matching files in this folder.");
	}
	return selected;
}
} // namespace

bool BeginAssetPickerPopup(const char *id) {
	const auto *viewport = ImGui::GetMainViewport();
	// Fixed each frame: the scrolling child must not drive popup auto-sizing.
	ImGui::SetNextWindowSize(ImVec2(std::min(480.f, viewport->WorkSize.x - 32.f),
								 std::min(400.f, viewport->WorkSize.y - 32.f)),
		ImGuiCond_Always);
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	return ImGui::BeginPopup(id);
}

AssetPickerResult DrawAssetPicker(FileSystem *files, AssetKind kind, const char *title,
	const std::string &error, const std::string &currentPath) {
	AssetPickerResult result;
	const auto look = appearance(kind);
	ImGui::TextColored(look.color, "%s  %s", look.icon, title);
	ImGui::TextDisabled("%s  /  res://", look.description);
	ImGui::Separator();
	const float spacing = ImGui::GetStyle().ItemSpacing.y;
	const float errorHeight = error.empty() ? 0.f : ImGui::GetTextLineHeight() * 2.f + spacing;
	const float footer = 32.f + spacing * 2.f + 1.f + errorHeight;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
	const bool visible = ImGui::BeginChild("##asset_files", ImVec2(0.f, -footer),
		ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 2.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 5.f));
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.f);
	if (visible) {
		if (files) {
			result.path = drawDirectory(*files, "res://", kind, currentPath, true);
		} else {
			ImGui::TextDisabled("Project files are unavailable.");
		}
	}
	ImGui::PopStyleVar(3);
	ImGui::EndChild();
	ImGui::PopStyleVar();
	if (!error.empty()) {
		ImGui::BeginChild("##picker_error", ImVec2(0.f, errorHeight - spacing));
		ImGui::PushStyleColor(ImGuiCol_Text, theme::Rose);
		ImGui::TextWrapped("%s", error.c_str());
		ImGui::PopStyleColor();
		ImGui::EndChild();
	}
	ImGui::Separator();
	ImGui::SetCursorPosX(
		ImGui::GetCursorPosX() + std::max(0.f, ImGui::GetContentRegionAvail().x - 100.f));
	result.cancelled =
		ImGui::Button("Cancel", ImVec2(100.f, 32.f)) || ImGui::IsKeyPressed(ImGuiKey_Escape);
	if (result.cancelled) {
		result.path.reset();
	}
	return result;
}
} // namespace sowa::editor
