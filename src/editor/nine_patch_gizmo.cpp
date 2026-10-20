#include "editor/editor.hpp"
#include "editor/inspector.hpp"
#include "editor/nine_patch_tools.hpp"
#include "editor/viewport.hpp"
#include "ui/geometry.hpp"

#include <cmath>
#include <imgui.h>

namespace sowa {

void Editor::finishNinePatchResize(bool cancel) {
	if (!_ninePatchBefore) {
		return;
	}
	if (auto *rect = _ninePatchEntity.Get<NinePatchRect>()) {
		if (cancel) {
			*rect = *_ninePatchBefore;
		} else {
			editor::RecordNinePatchResize(_sceneSaveStates, _ninePatchScene, _ninePatchEntity,
				*_ninePatchBefore, *rect);
		}
	}
	_ninePatchBefore.reset();
}

bool Editor::drawNinePatchGizmo(const glm::mat4 &viewProjection, const Rect &image) {
	if (multiSelected()) {
		return false;
	}
	auto *rect = Selected().Get<NinePatchRect>();
	if (!rect || !Selected().Has<Transform2D>() || !editor::CanEdit(Selected(), false)) {
		return false;
	}
	const glm::mat4 projection = viewProjection * ComputeWorldMatrix(Selected());
	if (std::abs(glm::determinant(projection)) < 1e-20f) {
		return false;
	}
	const Rect local = rect->LocalRect();
	const Vector2 directions[] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
	Vector2 corners[4];
	for (int i = 0; i < 4; ++i) {
		const Vector2 point{local.x + directions[i].x * local.w,
			local.y + directions[i].y * local.h};
		corners[i] = editor::WorldToViewportImage(projection, point, image);
	}
	const auto mouse = ImGui::GetMousePos();
	auto &draw = *ImGui::GetWindowDrawList();
	int hovered = -1;
	for (int i = 0; i < 4; ++i) {
		const auto a = corners[i], b = corners[(i + 1) % 4];
		draw.AddLine({a.x, a.y}, {b.x, b.y}, IM_COL32(80, 190, 255, 255), 2);
		draw.AddRectFilled({a.x - 4, a.y - 4}, {a.x + 4, a.y + 4}, IM_COL32(80, 190, 255, 255));
		if (std::abs(mouse.x - a.x) < 8 && std::abs(mouse.y - a.y) < 8) {
			hovered = i;
		}
	}
	const Vector2 ndc{2 * (mouse.x - image.x) / image.w - 1, 1 - 2 * (mouse.y - image.y) / image.h};
	if (hovered >= 0 && !_gizmoDrag) {
		ImGui::SetMouseCursor(
			hovered % 2 ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE);
		if (!_ninePatchBefore && ImGui::IsWindowHovered() &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			_ninePatchBefore = *rect;
			_ninePatchEntity = Selected();
			_ninePatchScene = _editingScene;
			_ninePatchMapping = glm::inverse(projection);
			_ninePatchStart = UITransformPoint(_ninePatchMapping, ndc);
			_ninePatchCorner = directions[hovered];
		}
	}
	if (!_ninePatchBefore) {
		return false;
	}
	const Vector2 delta = UITransformPoint(_ninePatchMapping, ndc) - _ninePatchStart;
	*rect = editor::ResizeNinePatchRect(*_ninePatchBefore, delta, _ninePatchCorner);
	return true;
}

} // namespace sowa
