#include "editor/editor.hpp"

#include <imgui.h>

#include "editor/icons.hpp"
#include "editor/inspector.hpp"

namespace sowa {
namespace {

using Mode = editor::GizmoMode;
using Handle = editor::GizmoHandle;

constexpr ImU32 kHighlight = IM_COL32(255, 215, 100, 255);
constexpr ImU32 kNeutral = IM_COL32(230, 230, 240, 255);
constexpr ImU32 kShadow = IM_COL32(15, 15, 20, 230);

ImVec2 point(Vector2 p) {
	return {p.x, p.y};
}

void drawHandles(const editor::GizmoGeometry &gizmo, Mode mode, Handle active, Vector2 mouse) {
	auto &draw = *ImGui::GetWindowDrawList();
	const Vector2 center = gizmo.origin;

	if (mode == Mode::Rotation) {
		const ImU32 color = active == Handle::Ring ? kHighlight : kNeutral;
		draw.AddCircle(point(center), editor::kGizmoRadius, kShadow, 64, 6.f);
		draw.AddCircle(point(center), editor::kGizmoRadius, color, 64, 2.5f);
		draw.AddCircleFilled(point(center), 4.f, kNeutral);
		if (active == Handle::Ring) {
			const Vector2 end = center + (mouse - center).Normalized() * editor::kGizmoRadius;
			draw.AddLine(point(center), point(end), kHighlight, 1.5f);
		}
		return;
	}

	auto axis = [&](Vector2 direction, Handle handle, ImU32 color) {
		if (active == handle) {
			color = kHighlight;
		}
		const Vector2 start = center + direction * 12.f;
		const Vector2 end = center + direction * editor::kGizmoLength;
		draw.AddLine(point(start), point(end), kShadow, 6.f);
		draw.AddLine(point(start), point(end), color, 2.5f);
		if (mode == Mode::Position) {
			const Vector2 base = end - direction * 12.f;
			const Vector2 side = Vector2(-direction.y, direction.x) * 5.f;
			draw.AddTriangleFilled(point(end), point(base + side), point(base - side), color);
		} else {
			draw.AddRectFilled(point(end - Vector2(5.f)), point(end + Vector2(5.f)), color);
		}
	};
	axis(gizmo.xDirection, Handle::X, IM_COL32(235, 95, 85, 255));
	axis(gizmo.yDirection, Handle::Y, IM_COL32(95, 215, 120, 255));

	const ImU32 color = active == Handle::Center ? kHighlight : kNeutral;
	draw.AddRectFilled(point(center - Vector2(8.f)), point(center + Vector2(8.f)), kShadow, 2.f);
	draw.AddRect(point(center - Vector2(7.f)), point(center + Vector2(7.f)), color, 2.f, 0, 2.f);
}

} // namespace

void Editor::drawGizmoToolbar(bool disabled) {
	struct Button {
		Mode mode;
		const char *icon;
		const char *tooltip;
	};

	const Button buttons[] = {
		{Mode::Position, ICON_FA_ARROWS, "Move"},
		{Mode::Rotation, ICON_FA_ROTATE_RIGHT, "Rotate"},
		{Mode::Scale, ICON_FA_EXPAND, "Resize"},
		{Mode::None, ICON_FA_EYE_SLASH, "Hide transform gizmos"},
	};

	ImGui::BeginDisabled(disabled);
	ImGui::BeginGroup();
	for (const Button &button : buttons) {
		if (button.mode != Mode::Position) {
			ImGui::SameLine(0.f, 4.f);
		}
		const ImGuiCol color = _gizmoMode == button.mode ? ImGuiCol_ButtonActive : ImGuiCol_Button;
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(color));
		if (ImGui::Button(button.icon, ImVec2(30.f, 0.f))) {
			finishTransformGizmo();
			_gizmoMode = button.mode;
		}
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", button.tooltip);
		}
	}
	ImGui::EndGroup();
	ImGui::EndDisabled();
}

void Editor::finishTransformGizmo(bool cancel) {
	if (!_gizmoDrag) {
		return;
	}
	const auto drag = *_gizmoDrag;
	_gizmoDrag.reset();

	auto *transform = _gizmoEntity.Get<Transform2D>();
	if (!transform) {
		return;
	}
	if (cancel) {
		*transform = drag.before;
		return;
	}

	const char *fieldName = nullptr;
	editor::FieldValue before, after;
	switch (drag.mode) {
	case Mode::Position:
		fieldName = "position";
		before = drag.before.position;
		after = transform->position;
		break;
	case Mode::Rotation:
		fieldName = "rotation";
		before = drag.before.rotation;
		after = transform->rotation;
		break;
	case Mode::Scale:
		fieldName = "scale";
		before = drag.before.scale;
		after = transform->scale;
		break;
	case Mode::None:
		return;
	}

	for (ComponentType type : _gizmoEntity.GetComponents()) {
		if (type.Name() != "Transform2D") {
			continue;
		}
		if (const FieldInfo *field = type.FindField(fieldName)) {
			editor::RecordFieldEdit(_sceneSaveStates, _gizmoScene, _gizmoEntity, type, *field,
				before, after);
		}
		break;
	}
}

bool Editor::drawTransformGizmo(const glm::mat4 &viewProjection, const Rect &image) {
	if (_gizmoMode == Mode::None || !editor::CanEdit(_selected, false)) {
		return false;
	}
	auto gizmo = editor::BuildGizmo(_selected, _gizmoMode, viewProjection, image);
	if (!gizmo) {
		finishTransformGizmo();
		return false;
	}

	const ImVec2 mousePosition = ImGui::GetIO().MousePos;
	const Vector2 mouse(mousePosition.x, mousePosition.y);
	Handle hovered = Handle::None;
	if (ImGui::IsItemHovered() && !_cameraDragging) {
		hovered = editor::HitTestGizmo(*gizmo, _gizmoMode, mouse);
	}
	if (!_gizmoDrag && hovered != Handle::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		_gizmoDrag =
			editor::BeginGizmoDrag(_selected, _gizmoMode, hovered, viewProjection, image, mouse);
		_gizmoEntity = _selected;
		_gizmoScene = _editingScene;
		StopAnimationPreview();
	}
	if (_gizmoDrag) {
		*_gizmoEntity.Get<Transform2D>() = editor::UpdateGizmoDrag(*_gizmoDrag, mouse);
		gizmo = editor::BuildGizmo(_selected, _gizmoMode, viewProjection, image);
	}

	const Handle active = _gizmoDrag ? _gizmoDrag->handle : hovered;
	if (active != Handle::None) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}
	if (gizmo) {
		drawHandles(*gizmo, _gizmoMode, active, mouse);
	}
	return active != Handle::None;
}

} // namespace sowa
