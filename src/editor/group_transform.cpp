#include "editor/group_transform.hpp"

#include <algorithm>
#include <cmath>

#include "editor/inspector.hpp"
#include "math/matrix.hpp"
#include "ui/components.hpp"

namespace sowa::editor {

std::optional<Transform2D> ExactTransform2D(const glm::mat4 &matrix) {
	for (int c = 0; c < 4; ++c) {
		for (int r = 0; r < 4; ++r) {
			if (!std::isfinite(matrix[c][r])) {
				return {};
			}
		}
	}
	const f32 sx = std::hypot(matrix[0][0], matrix[0][1]);
	const f32 determinant = matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1];
	if (sx < 1e-6f || std::abs(determinant) < 1e-8f) {
		return {};
	}
	Transform2D result;
	result.position = {matrix[3][0], matrix[3][1]};
	result.rotation = glm::degrees(std::atan2(matrix[0][1], matrix[0][0]));
	result.scale = {sx, determinant / sx};
	const auto rebuilt = result.LocalMatrix();
	for (int c = 0; c < 4; ++c) {
		for (int r = 0; r < 4; ++r) {
			if (std::abs(matrix[c][r] - rebuilt[c][r]) >
				1e-4f * std::max(1.f, std::abs(matrix[c][r]))) {
				return {};
			}
		}
	}
	result.worldMatrix = matrix;
	return result;
}

std::optional<GroupTransform> CaptureGroupTransform(const EntitySelection &selection,
	bool activePivot, bool localAxes) {
	GroupTransform group;
	std::vector<Entity> candidates;
	const auto collect = [&](const auto &self, Entity entity) -> void {
		if (!entity.Has<Transform2D>()) {
			for (Entity child : entity.GetChildren()) {
				self(self, child);
			}
		} else if (CanEdit(entity, false) && !entity.Has<UIRect>()) {
			candidates.push_back(entity);
		}
	};
	for (Entity entity : selection.Members()) {
		collect(collect, entity);
	}
	for (Entity entity : SelectionRoots(candidates)) {
		const auto parent = ComputeWorldMatrix(entity.GetParent());
		const f32 determinant = glm::determinant(parent);
		if (!std::isfinite(determinant) || std::abs(determinant) < 1e-8f) {
			continue;
		}
		const auto world = ComputeWorldMatrix(entity);
		const auto before = *entity.Get<Transform2D>();
		const f32 angle = glm::radians(before.rotation);
		const auto x = parent * glm::vec4(std::cos(angle), std::sin(angle), 0, 0);
		const auto y = parent * glm::vec4(-std::sin(angle), std::cos(angle), 0, 0);
		const Vector2 position(world[3][0], world[3][1]);
		group.targets.push_back({entity, entity.GetParent(), before, position, {x.x, x.y},
			{y.x, y.y}, glm::inverse(parent)});
		group.pivot += position;
	}
	if (group.targets.empty()) {
		return {};
	}
	group.pivot /= static_cast<f32>(group.targets.size());
	Entity activeRoot = selection.ActiveRoot();
	if (std::none_of(group.targets.begin(), group.targets.end(), [&](const auto &target) {
			return target.entity == activeRoot || target.entity.IsDescendantOf(activeRoot);
		})) {
		activeRoot = group.targets.front().entity;
	}
	const auto active = ComputeWorldMatrix(activeRoot);
	if (activePivot) {
		group.pivot = {active[3][0], active[3][1]};
	}
	if (localAxes) {
		group.angle = std::atan2(active[0][1], active[0][0]);
	}
	return group;
}

std::optional<std::vector<Transform2D>> TransformGroup(const GroupTransform &group,
	const GroupTransformDelta &delta) {
	const auto rotate = [](Vector2 point, f32 angle) {
		const f32 c = std::cos(angle), s = std::sin(angle);
		return Vector2(c * point.x - s * point.y, s * point.x + c * point.y);
	};
	const auto scaleInBasis = [&](Vector2 point) {
		return rotate(rotate(point, -group.angle) * delta.scale, group.angle);
	};
	std::vector<Transform2D> result;
	for (const auto &target : group.targets) {
		Transform2D value = target.before;
		const Vector2 beforeOffset = target.worldPosition - group.pivot;
		Vector2 offset = beforeOffset;
		if (delta.scale != Vector2(1.f)) {
			offset = scaleInBasis(offset);
		}
		offset = rotate(offset, delta.rotation);
		const Vector2 movement = offset - beforeOffset + delta.movement;
		const auto local = target.worldToParent * glm::vec4(movement.x, movement.y, 0.f, 0.f);
		value.position += Vector2(local.x, local.y);
		const bool axisScale = delta.scale.x != delta.scale.y;
		if (!axisScale) {
			value.scale *= delta.scale.x;
		}
		if (axisScale || delta.rotation != 0.f) {
			const auto transformAxis = [&](Vector2 axis) {
				if (axisScale) {
					axis = scaleInBasis(axis);
				}
				axis = rotate(axis, delta.rotation);
				return target.worldToParent * glm::vec4(axis.x, axis.y, 0, 0);
			};
			const auto x = transformAxis(target.worldX), y = transformAxis(target.worldY);
			const f32 sx = std::hypot(x.x, x.y);
			f32 angle;
			if (sx > 1e-6f) {
				angle = std::atan2(x.y, x.x);
				if (axisScale) {
					// TRS cannot store shear: keep the transformed X direction and signed area.
					value.scale *= Vector2(sx, (x.x * y.y - y.x * x.y) / sx);
				}
			} else {
				angle = std::atan2(-y.x, y.y);
				if (axisScale) {
					value.scale *= Vector2(0, std::hypot(y.x, y.y));
				}
			}
			value.rotation += glm::degrees(
				std::remainder(angle - glm::radians(value.rotation), glm::radians(360.f)));
		}
		if (!std::isfinite(value.position.x) || !std::isfinite(value.position.y) ||
			!std::isfinite(value.rotation) || !std::isfinite(value.scale.x) ||
			!std::isfinite(value.scale.y)) {
			return {};
		}
		result.push_back(value);
	}
	return result;
}

bool ApplyGroupTransform(const GroupTransform &group, const std::vector<Transform2D> &values) {
	if (values.size() != group.targets.size()) {
		return false;
	}
	for (const auto &target : group.targets) {
		if (!CanEdit(target.entity, false) || !target.entity.Has<Transform2D>() ||
			target.entity.GetParent() != target.parent) {
			return false;
		}
	}
	for (std::size_t i = 0; i < values.size(); ++i) {
		*Entity(group.targets[i].entity).Get<Transform2D>() = values[i];
	}
	return true;
}
} // namespace sowa::editor
