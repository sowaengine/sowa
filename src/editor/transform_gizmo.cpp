#include "editor/editor.hpp"

#include <cmath>
#include <imgui.h>

#include "editor/icons.hpp"
#include "editor/viewport.hpp"

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
	if (multiSelected()) {
		ImGui::SameLine();
		if (ImGui::Button(_groupActivePivot ? "Pivot: Active" : "Pivot: Center")) {
			finishTransformGizmo();
			_groupActivePivot = !_groupActivePivot;
		}
		ImGui::SameLine();
		if (ImGui::Button(_groupLocalAxes ? "Axes: Local" : "Axes: World")) {
			finishTransformGizmo();
			_groupLocalAxes = !_groupLocalAxes;
		}
	}
	ImGui::EndGroup();
	ImGui::EndDisabled();
}

namespace {
Vector2 groupPoint(const glm::mat4 &mapping, Vector2 point) {
	const auto p = mapping * glm::vec4(point.x, point.y, 0, 1);
	return {p.x / p.w, p.y / p.w};
}
} // namespace

void Editor::finishTransformGizmo(bool cancel) {
	if (!_gizmoDrag) {
		return;
	}
	auto drag = std::move(*_gizmoDrag);
	_gizmoDrag.reset();
	std::vector<editor::BulkFieldEdit> edits;
	for (const auto &target : drag.group.targets) {
		auto *value = Entity(target.entity).Get<Transform2D>();
		if (!value) {
			continue;
		}
		if (cancel) {
			*value = target.before;
			continue;
		}
		edits.push_back(
			{target.entity, "Transform2D", "position", target.before.position, value->position});
		edits.push_back(
			{target.entity, "Transform2D", "rotation", target.before.rotation, value->rotation});
		edits.push_back({target.entity, "Transform2D", "scale", target.before.scale, value->scale});
	}
	if (!cancel && !editor::RecordBulkFieldEdit(_sceneSaveStates, _gizmoScene, edits)) {
		for (const auto &target : drag.group.targets) {
			if (auto *value = Entity(target.entity).Get<Transform2D>()) {
				*value = target.before;
			}
		}
	}
}

bool Editor::drawTransformGizmo(const glm::mat4 &viewProjection, const Rect &image) {
	if (_selectionPlaying || !_editingScene.IsValid() || _gizmoMode == Mode::None || image.w <= 0 ||
		image.h <= 0) {
		return false;
	}
	std::optional<editor::GroupTransform> captured;
	if (!_gizmoDrag) {
		captured = editor::CaptureGroupTransform(selection(), multiSelected() && _groupActivePivot,
			multiSelected() && _groupLocalAxes);
		if (!captured) {
			return false;
		}
	}
	const auto &group = _gizmoDrag ? _gizmoDrag->group : *captured;
	const Vector2 origin = editor::WorldToViewportImage(viewProjection, group.pivot, image);
	const Vector2 x(std::cos(group.angle), std::sin(group.angle));
	const Vector2 y(-x.y, x.x);
	editor::GizmoGeometry geometry{origin,
		(editor::WorldToViewportImage(viewProjection, group.pivot + x, image) - origin)
			.Normalized(),
		(editor::WorldToViewportImage(viewProjection, group.pivot + y, image) - origin)
			.Normalized()};
	if (!multiSelected() && Selected().Has<Transform2D>()) {
		geometry = *editor::BuildGizmo(Selected(), _gizmoMode, viewProjection, image);
	}
	const ImVec2 m = ImGui::GetIO().MousePos;
	const Vector2 mouse(m.x, m.y);
	auto hovered = ImGui::IsItemHovered() && !_cameraDragging
		? editor::HitTestGizmo(geometry, _gizmoMode, mouse)
		: Handle::None;
	if (!_gizmoDrag && hovered != Handle::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		glm::mat4 screenToNdc(1.f);
		screenToNdc[0][0] = 2.f / image.w;
		screenToNdc[1][1] = -2.f / image.h;
		screenToNdc[3][0] = -1.f - 2.f * image.x / image.w;
		screenToNdc[3][1] = 1.f + 2.f * image.y / image.h;
		TransformDrag drag;
		drag.group = group;
		drag.geometry = geometry;
		drag.handle = hovered;
		drag.mode = _gizmoMode;
		drag.screenToWorld = glm::inverse(viewProjection) * screenToNdc;
		drag.start = mouse;
		drag.selection = selection();
		if (!multiSelected() && Selected().Has<Transform2D>()) {
			drag.single = editor::BeginGizmoDrag(Selected(), _gizmoMode, hovered, viewProjection,
				image, mouse);
		}
		const Vector2 ray = groupPoint(drag.screenToWorld, mouse) - group.pivot;
		drag.lastAngle = std::atan2(ray.y, ray.x);
		_gizmoDrag = std::move(drag);
		_gizmoScene = _editingScene;
		StopAnimationPreview();
	}
	if (_gizmoDrag && _gizmoDrag->single) {
		auto value = editor::UpdateGizmoDrag(*_gizmoDrag->single, mouse);
		editor::ApplyGroupTransform(_gizmoDrag->group, {value});
		geometry = *editor::BuildGizmo(Selected(), _gizmoMode, viewProjection, image);
	} else if (_gizmoDrag) {
		auto &drag = *_gizmoDrag;
		editor::GroupTransformDelta delta;
		const Vector2 worldStart = groupPoint(drag.screenToWorld, drag.start);
		const Vector2 worldMouse = groupPoint(drag.screenToWorld, mouse);
		if (drag.mode == Mode::Position) {
			Vector2 movement = worldMouse - worldStart;
			if (drag.handle == Handle::X || drag.handle == Handle::Y) {
				const Vector2 axis = drag.handle == Handle::X ? x : y;
				movement = axis * (movement.x * axis.x + movement.y * axis.y);
			}
			delta.movement = movement;
		} else if (drag.mode == Mode::Rotation) {
			const Vector2 ray = worldMouse - group.pivot;
			if (mouse.DistanceTo(geometry.origin) > 3.f) {
				const f32 angle = std::atan2(ray.y, ray.x);
				drag.rotation += std::remainder(angle - drag.lastAngle, glm::radians(360.f));
				drag.lastAngle = angle;
			}
			delta.rotation = drag.rotation;
		} else {
			const Vector2 movement = mouse - drag.start;
			Vector2 scale(1.f);
			if (drag.handle == Handle::Center) {
				scale = Vector2(editor::GizmoScaleFactor(movement.x - movement.y));
			} else {
				const auto axis =
					drag.handle == Handle::X ? drag.geometry.xDirection : drag.geometry.yDirection;
				const f32 factor =
					editor::GizmoScaleFactor(movement.x * axis.x + movement.y * axis.y);
				(drag.handle == Handle::X ? scale.x : scale.y) = factor;
			}
			delta.scale = scale;
		}
		geometry.origin =
			editor::WorldToViewportImage(viewProjection, group.pivot + delta.movement, image);
		const auto values = editor::TransformGroup(group, delta);
		if (values) {
			editor::ApplyGroupTransform(group, *values);
		}
	}
	const Handle active = _gizmoDrag ? _gizmoDrag->handle : hovered;
	if (active != Handle::None) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}
	drawHandles(geometry, _gizmoMode, active, mouse);
	return _gizmoDrag.has_value() || hovered != Handle::None;
}
} // namespace sowa
