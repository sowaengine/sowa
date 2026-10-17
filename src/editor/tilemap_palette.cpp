#include "editor/tilemap_palette.hpp"

#include <algorithm>
#include <imgui.h>

#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"

namespace sowa::editor {

std::optional<i32> DrawTilemapPalette(const Tilemap &map, ResourceRegistry &resources,
	i32 selected) {
	std::optional<i32> chosen;
	ImGui::SeparatorText("Palette");
	ImGui::Text("Selected tile: %d", selected);
	ImGui::BeginChild("Tile palette");
	constexpr float tileSize = 48.f;
	constexpr float spacing = 8.f;
	constexpr float rowHeight = tileSize + spacing;
	// Let submitted items advance the layout; the clipper uses the same stride.
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
	const i32 columns =
		std::max(1, static_cast<i32>((ImGui::GetContentRegionAvail().x + spacing) / rowHeight));
	const auto &tiles = map.Tiles();
	ImGuiListClipper clipper;
	clipper.Begin((static_cast<i32>(tiles.size()) + columns - 1) / columns, rowHeight);
	while (clipper.Step()) {
		for (i32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
			for (i32 col = 0; col < columns; ++col) {
				const i32 index = row * columns + col;
				if (index >= static_cast<i32>(tiles.size())) {
					break;
				}
				const auto &tile = tiles[index];
				ImGui::PushID(tile.id);
				if (col) {
					ImGui::SameLine();
				}
				const ImVec2 pos = ImGui::GetCursorScreenPos();
				if (ImGui::Selectable("##tile", tile.id == selected, 0,
						ImVec2(tileSize, tileSize))) {
					chosen = tile.id;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%d: %s%s", tile.id, tile.name.c_str(),
						tile.scene ? " (prefab)" : "");
				}
				auto *draw = ImGui::GetWindowDrawList();
				const auto texture = resources.Get<ImageTexture>(tile.resource);
				const Rect uv =
					texture ? TileUV(tile, texture->Size(), texture->flipVertically) : Rect{};
				if (!tile.scene && texture && texture->IsLoaded() && uv.w != 0 &&
					resources.Context().device) {
					const auto native =
						resources.Context().device->GetNativeTextureId(texture->Handle());
					draw->AddImage(ImTextureRef(static_cast<ImTextureID>(native)),
						ImVec2(pos.x + 3, pos.y + 3), ImVec2(pos.x + 45, pos.y + 45),
						ImVec2(uv.Left(), uv.Top()), ImVec2(uv.Right(), uv.Bottom()));
				} else {
					draw->AddText(ImVec2(pos.x + 3, pos.y + 15), IM_COL32_WHITE,
						tile.scene ? "Scene" : "N/A");
				}
				ImGui::PopID();
			}
		}
	}
	ImGui::PopStyleVar();
	ImGui::EndChild();
	return chosen;
}

} // namespace sowa::editor
