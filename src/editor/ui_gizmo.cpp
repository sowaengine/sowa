#include "editor/editor.hpp"
#include "editor/inspector.hpp"
#include "ui/geometry.hpp"

#include <cmath>
#include <imgui.h>

namespace sowa {
void Editor::finishUIGizmo(bool cancel) {
	if (!_uiDragBefore) {
		return;
	}
	if (_uiDragEntity.IsAlive()) {
		if (auto *rect = _uiDragEntity.Get<UIRect>()) {
			if (cancel) {
				*rect = *_uiDragBefore;
			} else {
				editor::RecordUIRectEdit(_sceneSaveStates, _uiDragScene, _uiDragEntity,
					*_uiDragBefore, *rect);
			}
		}
	}
	_uiDragBefore.reset();
}

bool Editor::drawUIGizmo(const Rect &image, Vector2 gameSize, Entity hoveredEntity) {
	if (multiSelected()) {
		return false;
	}
	if (_uiDragBefore && _uiDragEntity != Selected()) {
		finishUIGizmo();
	}
	if (!Selected().IsAlive() || !Selected().Has<UIRect>()) {
		return false;
	}
	const Entity canvasEntity = FindUICanvas(Selected());
	if (!canvasEntity.IsAlive()) {
		return false;
	}

	const auto &canvas = *canvasEntity.Get<UICanvas>();
	auto &rect = *Selected().Get<UIRect>();
	const auto mouse = ImGui::GetIO().MousePos;
	const Vector2 viewportMouse{(mouse.x - image.x) * gameSize.x / image.w,
		(mouse.y - image.y) * gameSize.y / image.h};
	const Vector2 localMouse = UITransformPoint(canvas.viewportToLayout, viewportMouse);
	auto *draw = ImGui::GetWindowDrawList();
	std::array<ImVec2, 4> corners;
	for (size_t i = 0; i < corners.size(); ++i) {
		const Vector2 point = rect.viewportCorners[i];
		corners[i] = {image.x + point.x * image.w / gameSize.x,
			image.y + point.y * image.h / gameSize.y};
	}
	for (size_t i = 0; i < corners.size(); ++i) {
		draw->AddLine(corners[i], corners[(i + 1) % 4], IM_COL32(80, 190, 255, 255), 2);
	}
	if (!editor::CanEdit(Selected(), false)) {
		return false;
	}

	int hoveredCorner = -1;
	for (size_t i = 0; i < corners.size(); ++i) {
		const auto point = corners[i];
		draw->AddRectFilled({point.x - 4, point.y - 4}, {point.x + 4, point.y + 4},
			IM_COL32(80, 190, 255, 255));
		if (std::abs(mouse.x - point.x) < 8 && std::abs(mouse.y - point.y) < 8) {
			hoveredCorner = static_cast<int>(i);
		}
	}
	if (hoveredCorner >= 0) {
		ImGui::SetMouseCursor(
			hoveredCorner % 2 ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE);
	}
	const bool inside = localMouse.x >= rect.resolved.x && localMouse.x <= rect.resolved.Right() &&
						localMouse.y >= rect.resolved.y && localMouse.y <= rect.resolved.Top();
	if (!_uiDragBefore && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
		ImGui::IsWindowHovered() &&
		(hoveredCorner >= 0 ||
			(inside && (hoveredEntity == Selected() || !hoveredEntity.IsAlive())))) {
		_uiDragBefore = rect;
		_uiDragEntity = Selected();
		_uiDragScene = _editingScene;
		_uiDragStart = localMouse;
		_uiDragMapping = canvas.viewportToLayout;
		_uiDragScale = canvas.unitScale;
		_uiResizeCorner = {-1, -1};
		if (hoveredCorner >= 0) {
			const Vector2 directions[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
			_uiResizeCorner = directions[hoveredCorner];
		}
	}
	if (!_uiDragBefore) {
		return false;
	}

	const Vector2 delta = UITransformPoint(_uiDragMapping, viewportMouse) - _uiDragStart;
	if (_uiResizeCorner.x >= 0) {
		rect = ResizeUIRect(*_uiDragBefore, delta, _uiResizeCorner, _uiDragScale);
	} else {
		rect.position = _uiDragBefore->position + delta / _uiDragScale;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		finishUIGizmo(true);
	} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		finishUIGizmo();
	}
	return true;
}
} // namespace sowa
