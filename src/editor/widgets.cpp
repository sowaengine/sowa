#include "editor/widgets.hpp"

#include <algorithm>
#include <cfloat>

#include "animation/animated_sprite.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/tilemap.hpp"
#include "editor/hierarchy.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/style.hpp"
#include "particles/particles2d.hpp"
#include "scene/scene.hpp"

namespace sowa::editor {

TypeAppearance EntityAppearance(Entity entity) {
	using namespace editor::theme;
	if (entity.Has<Camera2D>()) {
		return {ICON_FA_CAMERA, Rose, "Camera 2D"};
	}
	if (entity.Has<Tilemap>()) {
		return {ICON_FA_LAYER_GROUP, Green, "Tilemap"};
	}
	if (entity.Has<Particles2D>()) {
		return {ICON_FA_WAND_MAGIC_SPARKLES, Amber, "Particles 2D"};
	}
	if (entity.Has<AnimatedSprite>()) {
		return {ICON_FA_FILM, Blue, "Animated sprite"};
	}
	if (entity.Has<Sprite>()) {
		return {ICON_FA_IMAGE, Blue, "Sprite"};
	}
	if (IsInstanceRoot(entity)) {
		return {ICON_FA_LINK, Blue, "Scene instance"};
	}
	if (editor::IsSceneRoot(entity)) {
		return {ICON_FA_SITEMAP, Blue, "Scene"};
	}
	return {ICON_FA_CUBE, Blue, "Node"};
}

void InspectorFieldRow(const char *id, const std::string &label) {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::AlignTextToFramePadding();
	ImGui::TextColored(ImVec4(0.68f, 0.72f, 0.80f, 1.f), "%s", label.c_str());
	ImGui::TableNextColumn();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::PushID(id);
}

bool BeginInspectorTable(const char *id, float labelWidth) {
	const ImGuiTableFlags flags =
		ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
	if (!ImGui::BeginTable(id, 2, flags)) {
		return false;
	}
	labelWidth = std::min(labelWidth, ImGui::GetContentRegionAvail().x * 0.45f);
	ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
	ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
	return true;
}

bool IconActionButton(const char *icon, const char *tooltip, ImVec2 size) {
	const bool pressed = ImGui::Button(icon, size);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("%s", tooltip);
	}
	return pressed;
}

float InspectorLabelWidth(const std::string &nameLabel,
	const std::vector<const FieldInfo *> &fields) {
	float width = ImGui::CalcTextSize(nameLabel.c_str()).x;
	for (const FieldInfo *field : fields) {
		if (field == nullptr) {
			continue;
		}
		const std::string label = editor::DisplayName(field->name);
		width = std::max(width, ImGui::CalcTextSize(label.c_str()).x);
	}
	return width + 12.f;
}

} // namespace sowa::editor
