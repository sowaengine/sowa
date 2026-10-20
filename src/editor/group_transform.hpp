#pragma once

#include <optional>
#include <vector>

#include "ecs/components/transform2d.hpp"
#include "editor/selection.hpp"

namespace sowa::editor {
// Rejects singular, non-finite and sheared matrices instead of approximating them.
std::optional<Transform2D> ExactTransform2D(const glm::mat4 &matrix);

struct GroupTransformTarget {
	Entity entity;
	Entity parent;
	Transform2D before;
	Vector2 worldPosition, worldX, worldY;
	glm::mat4 worldToParent{1.f};
};

struct GroupTransformDelta {
	Vector2 movement;
	f32 rotation = 0.f; // Radians around the group pivot.
	Vector2 scale{1.f, 1.f};
};

struct GroupTransform {
	std::vector<GroupTransformTarget> targets;
	Vector2 pivot;
	f32 angle = 0;
};

std::optional<GroupTransform> CaptureGroupTransform(const EntitySelection &selection,
	bool activePivot, bool localAxes);
std::optional<std::vector<Transform2D>> TransformGroup(const GroupTransform &group,
	const GroupTransformDelta &delta);
bool ApplyGroupTransform(const GroupTransform &group, const std::vector<Transform2D> &values);
} // namespace sowa::editor
