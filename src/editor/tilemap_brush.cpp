#include "editor/editor.hpp"

#include <algorithm>
#include <imgui.h>

#include "ecs/components/transform2d.hpp"
#include "editor/inspector.hpp"
#include "editor/tilemap_tools.hpp"
#include "editor/viewport.hpp"

namespace sowa {
void Editor::finishTileStroke() {
	if (_tileStrokeBefore && _tileStrokeChanged && _tileStrokeEntity.IsAlive()) {
		if (const auto *map = _tileStrokeEntity.Get<Tilemap>()) {
			editor::RecordTilemapEdit(_sceneSaveStates, _tileStrokeScene, _tileStrokeEntity,
				*_tileStrokeBefore, *map);
		}
	}
	_tileStrokeBefore.reset();
	_tileStrokeLast.reset();
	_tileStrokeEntity = {};
	_tileStrokeChanged = false;
}

bool Editor::drawTilemapBrush(const glm::mat4 &vp, const Rect &image) {
	if (!_showTilemapEditor || !_tilePainting || !editor::CanEdit(_selected, false)) {
		return false;
	}
	auto *map = _selected.Get<Tilemap>();
	if (!map || !_selected.Has<Transform2D>() || !map->ValidCellSize()) {
		return false;
	}
	const auto world = ComputeWorldMatrix(_selected);
	const bool hovered = ImGui::IsItemHovered();
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	const auto cell = editor::TilemapViewportCell(*map, world, vp, image, {mouse.x, mouse.y});
	auto project = [&](float x, float y) {
		const auto p = world * glm::vec4(x, y, 0, 1);
		const auto screen = editor::WorldToViewportImage(vp, {p.x, p.y}, image);
		return ImVec2(screen.x, screen.y);
	};
	auto *draw = ImGui::GetWindowDrawList();
	// Bound the grid to the visible local rectangle, including rotated maps.
	i32 minX = INT32_MAX, minY = INT32_MAX, maxX = INT32_MIN, maxY = INT32_MIN;
	for (Vector2 point : {Vector2(image.x, image.y), Vector2(image.Right(), image.y),
			 Vector2(image.x, image.Top()), Vector2(image.Right(), image.Top())}) {
		if (auto c = editor::TilemapViewportCell(*map, world, vp, image, point)) {
			minX = std::min(minX, c->x);
			maxX = std::max(maxX, c->x);
			minY = std::min(minY, c->y);
			maxY = std::max(maxY, c->y);
		}
	}
	if (int64_t(maxX) - minX < 256 && int64_t(maxY) - minY < 256 && maxX < INT32_MAX &&
		maxY < INT32_MAX) {
		for (int64_t x = minX; x <= int64_t(maxX) + 1; ++x) {
			draw->AddLine(project(x * map->cellSize.x, float(minY) * map->cellSize.y),
				project(x * map->cellSize.x, (float(maxY) + 1) * map->cellSize.y),
				IM_COL32(100, 190, 240, 90));
		}
		for (int64_t y = minY; y <= int64_t(maxY) + 1; ++y) {
			draw->AddLine(project(float(minX) * map->cellSize.x, y * map->cellSize.y),
				project((float(maxX) + 1) * map->cellSize.x, y * map->cellSize.y),
				IM_COL32(100, 190, 240, 90));
		}
	}
	if (hovered && cell) {
		const float x = float(cell->x) * map->cellSize.x, y = float(cell->y) * map->cellSize.y;
		const ImVec2 points[] = {project(x, y), project(x + map->cellSize.x, y),
			project(x + map->cellSize.x, y + map->cellSize.y), project(x, y + map->cellSize.y)};
		const bool erase = _tileErasing || ImGui::GetIO().KeyShift;
		draw->AddPolyline(points, 4,
			erase ? IM_COL32(255, 100, 100, 255) : IM_COL32(100, 240, 150, 255), ImDrawFlags_Closed,
			2.f);
		const auto *layer = map->Layer(_tileLayer);
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl) {
			_tileBrush = map->GetTile(_tileLayer, cell->x, cell->y);
		} else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyCtrl && layer &&
				   layer->visible && (erase || map->FindTile(_tileBrush))) {
			if (!_tileStrokeBefore) {
				if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					return true;
				}
				_tileStrokeBefore = *map;
				_tileStrokeEntity = _selected;
				_tileStrokeScene = _editingScene;
			}
			_tileStrokeChanged =
				editor::PaintTileLine(*map, _tileLayer, _tileStrokeLast.value_or(*cell), *cell,
					erase ? 0 : _tileBrush) ||
				_tileStrokeChanged;
			_tileStrokeLast = cell;
		}
	} else {
		_tileStrokeLast.reset();
	}
	return true;
}
} // namespace sowa
