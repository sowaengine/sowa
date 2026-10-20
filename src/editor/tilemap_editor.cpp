#include "editor/editor.hpp"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "core/filesystem/file_system.hpp"
#include "ecs/components/transform2d.hpp"
#include "editor/asset_picker.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/style.hpp"
#include "editor/tilemap_palette.hpp"
#include "editor/tilemap_tools.hpp"
#include "editor/viewport.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"

namespace sowa {

void Editor::drawTilemapInspector(ResourceRegistry &, bool playing) {
	if (!Selected().Has<Tilemap>()) {
		return;
	}
	if (drawComponentHeader("Tilemap", "Tilemap", ICON_FA_LAYER_GROUP, playing)) {
		if (ImGui::Button("Edit Tilemap")) {
			_showTilemapEditor = true;
			ImGui::SetWindowFocus("Tilemap");
		}
	}
}

void Editor::drawTilemapEditor(ResourceRegistry &resources, bool playing) {
	if (multiSelected()) {
		return;
	}
	if (!_showTilemapEditor) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(1050, 460), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Tilemap", &_showTilemapEditor)) {
		ImGui::End();
		return;
	}
	auto *map = Selected().Get<Tilemap>();
	if (!map) {
		ImGui::TextDisabled("Select an entity with a Tilemap component.");
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(!editor::CanEdit(Selected(), playing));
	std::optional<Tilemap> before;
	const auto capture = [&] {
		finishTileStroke();
		if (!before) {
			before = *Selected().Get<Tilemap>();
		}
	};
	bool changed = false;
	for (const auto &sheet : map->Sheets()) {
		if (auto image = resources.Get<ImageTexture>(sheet.resource)) {
			if (image->Size() != sheet.imageSize && image->Width() > 0) {
				capture();
				changed = map->RefreshSheet(sheet.id, image->Size()) || changed;
			}
		}
	}
	ImGui::Checkbox("Paint in viewport", &_tilePainting);
	ImGui::SameLine();
	ImGui::Checkbox("Eraser", &_tileErasing);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(130);
	Vector2 size = map->cellSize;
	if (ImGui::InputFloat2("Cell size", &size.x, "%.0f", ImGuiInputTextFlags_EnterReturnsTrue) &&
		size.x > 0 && size.y > 0 && std::isfinite(size.x) && std::isfinite(size.y)) {
		capture();
		map->cellSize = size;
		changed = true;
	}
	ImGui::TextDisabled("Left drag: paint   Shift: erase   Ctrl-click: sample   Right drag: pan");

	if (ImGui::BeginTable("Tilemap layout", 3,
			ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
		ImGui::TableSetupColumn("Layers", ImGuiTableColumnFlags_WidthFixed, 210);
		ImGui::TableSetupColumn("Palette", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 320);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::BeginChild("Layers");
		changed = drawTilemapLayers(*map, capture) || changed;
		ImGui::EndChild();
		ImGui::TableNextColumn();
		drawTilemapPalette(*map, resources);
		ImGui::TableNextColumn();
		ImGui::BeginChild("Sources");
		changed = drawTilemapSources(resources, capture) || changed;
		ImGui::EndChild();
		ImGui::EndTable();
	}
	if (changed) {
		editor::RecordTilemapEdit(_sceneSaveStates, _editingScene, Selected(), std::move(*before),
			*Selected().Get<Tilemap>());
	}
	ImGui::EndDisabled();
	ImGui::End();
}

bool Editor::drawTilemapLayers(Tilemap &map, const std::function<void()> &capture) {
	bool changed = false;
	ImGui::SeparatorText("Layers");
	_tileLayer = std::clamp(_tileLayer, 0, std::max(0, static_cast<i32>(map.Layers().size()) - 1));
	for (i32 i = 0; i < static_cast<i32>(map.Layers().size()); ++i) {
		ImGui::PushID(i);
		auto *layer = map.Layer(i);
		bool visible = layer->visible;
		if (ImGui::Checkbox("##visible", &visible)) {
			capture();
			layer->visible = visible;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Selectable(layer->name.empty() ? "(unnamed layer)" : layer->name.c_str(),
				_tileLayer == i)) {
			finishTileStroke();
			_tileLayer = i;
		}
		ImGui::PopID();
	}
	if (ImGui::Button("Add layer")) {
		capture();
		_tileLayer = map.AddLayer("Layer " + std::to_string(map.Layers().size()));
		changed = true;
	}
	if (auto *layer = map.Layer(_tileLayer)) {
		std::string name = layer->name;
		ImGui::SetNextItemWidth(-1);
		if (ImGui::InputText("##Layer name", &name, ImGuiInputTextFlags_EnterReturnsTrue)) {
			capture();
			layer->name = name;
			changed = true;
		}
		if (ImGui::Button("Backward") && _tileLayer > 0) {
			capture();
			map.MoveLayer(_tileLayer, _tileLayer - 1);
			--_tileLayer;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Forward") && _tileLayer + 1 < static_cast<i32>(map.Layers().size())) {
			capture();
			map.MoveLayer(_tileLayer, _tileLayer + 1);
			++_tileLayer;
			changed = true;
		}
		if (ImGui::Button("Clear layer")) {
			capture();
			changed = map.ClearLayer(_tileLayer) || changed;
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete layer")) {
			capture();
			map.RemoveLayer(_tileLayer);
			_tileLayer = 0;
			changed = true;
		}
	}
	return changed;
}

void Editor::drawTilemapPalette(const Tilemap &map, ResourceRegistry &resources) {
	if (const auto tile = editor::DrawTilemapPalette(map, resources, _tileBrush)) {
		finishTileStroke();
		_tileBrush = *tile;
		_tileErasing = false;
	}
}

void Editor::drawTileSourceForm(TileSourceForm &source, bool adding) {
	const char *kinds[] = {"Tilesheet", "Image file", "Scene prefab"};
	if (adding) {
		ImGui::Combo("Type", &source.kind, kinds, 3);
	} else if (source.kind != 0) {
		i32 kind = source.kind - 1;
		if (ImGui::Combo("Type", &kind, kinds + 1, 2)) {
			source.kind = kind + 1;
			source.region = {};
		}
	}
	ImGui::SetNextItemWidth(-75);
	if (ImGui::InputTextWithHint("##path", "res://...", &source.path) && source.kind == 1) {
		source.region = {};
	}
	ImGui::SameLine();
	if (ImGui::Button("Browse")) {
		ImGui::OpenPopup("Tile asset");
	}
	if (const editor::DialogStyle dialogStyle; editor::BeginAssetPickerPopup("Tile asset")) {
		const auto kind = source.kind == 2 ? editor::AssetKind::Scene : editor::AssetKind::Image;
		const auto picker =
			editor::DrawAssetPicker(_fileSystem, kind, "Choose a tile asset", {}, source.path);
		if (const auto &path = picker.path; path) {
			source.path = *path;
			// Picking an individual image means the whole new image.
			if (source.kind == 1) {
				source.region = {};
			}
			ImGui::CloseCurrentPopup();
		}
		if (picker.cancelled) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (source.kind == 0) {
		ImGui::InputFloat2("Tile size", &source.tileSize.x, "%.0f");
		ImGui::InputInt("Margin", &source.margin);
		ImGui::InputInt("Spacing", &source.spacing);
	} else if (source.kind == 1 && ImGui::TreeNode("Image region (optional)")) {
		ImGui::InputFloat2("Position", &source.region.x, "%.0f");
		ImGui::InputFloat2("Size", &source.region.w, "%.0f");
		ImGui::TextDisabled("Size 0, 0 uses the whole image.");
		ImGui::TreePop();
	}
}

bool Editor::applyTileSource(ResourceRegistry &resources, TileSourceForm &source, bool adding,
	const std::function<void()> &capture) {
	if (!_fileSystem) {
		return false;
	}
	const auto asset = editor::LoadTileAsset(resources, *_fileSystem, source.path, source.kind == 2,
		_editingScene.SourcePath());
	_tileImportError = asset.error;
	if (!asset.id.IsValid()) {
		return false;
	}
	capture();
	auto *map = Selected().Get<Tilemap>();
	const auto texture = resources.Get<ImageTexture>(asset.id);
	const std::string name = editor::AssetName(source.path);
	bool changed = false;
	if (adding) {
		i32 tile = 0;
		if (source.kind == 0 && texture) {
			const auto ids = map->AddSheet(asset.id, texture->Size(), source.tileSize,
				source.margin, source.spacing);
			if (!ids.empty()) {
				tile = ids.front();
			}
		} else if (source.kind == 1) {
			tile = map->AddTexture(asset.id, name, source.region);
		} else if (source.kind == 2) {
			tile = map->AddScene(asset.id, name);
		}
		changed = tile != 0;
		if (changed) {
			_tileBrush = tile;
		}
	} else if (source.kind == 0 && texture) {
		if (const auto *sheet = map->SheetForTile(_tileBrush)) {
			changed = map->UpdateSheet(sheet->id, asset.id, texture->Size(), source.tileSize,
				source.margin, source.spacing);
		}
	} else {
		changed = map->SetTileSource(_tileBrush, asset.id, source.kind == 2, source.region);
	}
	if (changed) {
		// Own can migrate a scene-root entity. Do not reuse map after this.
		_editingScene.Own(resources, asset.id);
		_tileSourceNeedsRefresh = true;
	} else {
		if (asset.created) {
			resources.Remove(asset.id);
		}
		_tileImportError =
			"Check the tile size, region, margin and spacing (maximum 65536 tiles per sheet).";
	}
	return changed;
}

bool Editor::drawTilemapSources(ResourceRegistry &resources, const std::function<void()> &capture) {
	bool changed = false;
	if (ImGui::BeginTabBar("Tile sources")) {
		if (ImGui::BeginTabItem("Selected tile")) {
			auto *map = Selected().Get<Tilemap>();
			const auto *tile = map->FindTile(_tileBrush);
			if (tile) {
				const auto *sheet = map->SheetForTile(tile->id);
				if (_tileSourceEntity != Selected() || _tileSourceBrush != _tileBrush) {
					_tileSourceNeedsRefresh = true;
					_tileEditWholeSheet = sheet != nullptr;
					_tileImportError.clear();
				}
				if (sheet) {
					if (ImGui::Checkbox("Edit whole sheet", &_tileEditWholeSheet)) {
						_tileSourceNeedsRefresh = true;
					}
				} else {
					_tileEditWholeSheet = false;
				}
				if (_tileSourceNeedsRefresh) {
					_tileEditSource = {};
					_tileEditSource.kind = tile->scene ? 2 : 1;
					_tileEditSource.region = tile->region;
					ResourceID resource = tile->resource;
					if (sheet && _tileEditWholeSheet) {
						_tileEditSource.kind = 0;
						_tileEditSource.tileSize = sheet->tileSize;
						_tileEditSource.margin = sheet->margin;
						_tileEditSource.spacing = sheet->spacing;
						resource = sheet->resource;
					}
					_tileEditSource.path = editor::TileAssetPath(resources, resource);
					_tileSourceEntity = Selected();
					_tileSourceBrush = _tileBrush;
					_tileSourceNeedsRefresh = false;
				}
				std::string name = tile->name;
				if (ImGui::InputText("Tile name", &name, ImGuiInputTextFlags_EnterReturnsTrue)) {
					capture();
					changed = map->SetTileName(tile->id, name);
				}
				drawTileSourceForm(_tileEditSource, false);
				if (ImGui::Button("Apply source")) {
					changed =
						applyTileSource(resources, _tileEditSource, false, capture) || changed;
				}
				ImGui::SameLine();
				if (_tileEditSource.kind != 2 && ImGui::Button("Reload from disk")) {
					capture();
					map = Selected().Get<Tilemap>();
					tile = map->FindTile(_tileBrush);
					sheet = map->SheetForTile(_tileBrush);
					const ResourceID resource =
						sheet && _tileEditWholeSheet ? sheet->resource : tile->resource;
					resources.Reload(resource);
					if (const auto image = resources.Get<ImageTexture>(resource);
						image && image->Width() > 0) {
						if (sheet) {
							changed = map->RefreshSheet(sheet->id, image->Size()) || changed;
						}
						_tileImportError.clear();
					} else {
						_tileImportError = "Could not reload the image.";
					}
				}
				ImGui::TextWrapped(_tileEditWholeSheet
									   ? "Sheet changes update every tile in this sheet. IDs and "
										 "painted cells are preserved."
									   : "Source changes update every painted use of this tile.");
				if (ImGui::Button("Remove tile and painted uses")) {
					capture();
					Selected().Get<Tilemap>()->RemoveTile(_tileBrush);
					_tileBrush = 0;
					changed = true;
				}
			} else {
				ImGui::TextWrapped("Select a palette tile to edit its image, prefab or sheet.");
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Add tiles")) {
			drawTileSourceForm(_tileAddSource, true);
			if (ImGui::Button("Add to palette")) {
				changed = applyTileSource(resources, _tileAddSource, true, capture) || changed;
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	if (!_tileImportError.empty()) {
		ImGui::TextWrapped("%s", _tileImportError.c_str());
	}
	return changed;
}

} // namespace sowa
