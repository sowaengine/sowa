#include "ui/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "ecs/components/camera2d.hpp"
#include "ecs/components/transform2d.hpp"

namespace sowa {
Vector2 UITransformPoint(const glm::mat4 &matrix, Vector2 point) {
	const auto result = matrix * glm::vec4(point.x, point.y, 0, 1);
	return {result.x / result.w, result.y / result.w};
}

std::array<Vector2, 4> UIRectCorners(Rect rect, const glm::mat4 &matrix) {
	return {UITransformPoint(matrix, {rect.x, rect.y}),
		UITransformPoint(matrix, {rect.Right(), rect.y}),
		UITransformPoint(matrix, {rect.Right(), rect.Top()}),
		UITransformPoint(matrix, {rect.x, rect.Top()})};
}

Entity FindUICanvas(Entity entity) {
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		if (current.Has<UICanvas>()) {
			return current;
		}
	}
	return {};
}

bool UpdateUICanvasMapping(Entity entity, Vector2 viewport, const glm::mat4 &worldViewProjection) {
	auto &canvas = *entity.Get<UICanvas>();
	const auto transform = ComputeWorldMatrix(entity);
	const auto flipY = glm::scale(glm::mat4(1), glm::vec3(1, -1, 1));
	const auto drawToLocal = glm::translate(glm::mat4(1), glm::vec3(0, -viewport.y, 0));
	const auto layoutToDraw = glm::translate(glm::mat4(1), glm::vec3(0, viewport.y, 0)) * flipY;
	canvas.unitScale = 1;
	if (canvas.space == UISpace::World) {
		canvas.projection = worldViewProjection * transform * drawToLocal;
	} else {
		canvas.projection = CalculateBlankViewProjection(viewport) * transform;
		if (canvas.scaleMode == UIScaleMode::FitReference && canvas.referenceSize.x > 0 &&
			canvas.referenceSize.y > 0) {
			canvas.unitScale =
				std::min(viewport.x / canvas.referenceSize.x, viewport.y / canvas.referenceSize.y);
		}
	}
	const auto clipToViewport =
		glm::translate(glm::mat4(1), glm::vec3(viewport.x / 2, viewport.y / 2, 0)) *
		glm::scale(glm::mat4(1), glm::vec3(viewport.x / 2, -viewport.y / 2, 1));
	canvas.layoutToViewport = clipToViewport * canvas.projection * layoutToDraw;
	const f32 determinant = glm::determinant(canvas.layoutToViewport);
	if (!std::isfinite(determinant) || std::abs(determinant) < 1e-8f) {
		return false;
	}
	canvas.viewportToLayout = glm::inverse(canvas.layoutToViewport);
	return true;
}

UIRect ResizeUIRect(const UIRect &before, Vector2 delta, Vector2 corner, f32 unitScale) {
	UIRect result = before;
	const Vector2 direction{corner.x == 0 ? -1.f : 1.f, corner.y == 0 ? -1.f : 1.f};
	const Vector2 original{before.resolved.w / unitScale, before.resolved.h / unitScale};
	Vector2 size = original + Vector2(delta.x * direction.x, delta.y * direction.y) / unitScale;
	auto constrain = [](f32 value, f32 minimum, f32 maximum) {
		value = std::max(value, std::max(0.f, minimum));
		if (maximum > 0) {
			value = std::min(value, std::max(minimum, maximum));
		}
		return value;
	};
	size.x = constrain(size.x, before.minSize.x, before.maxSize.x);
	size.y = constrain(size.y, before.minSize.y, before.maxSize.y);
	const Vector2 change = size - original;
	result.size = size - before.anchorSpan;
	result.position += Vector2(change.x * (before.pivot.x + corner.x - 1),
		change.y * (before.pivot.y + corner.y - 1));
	result.fitContentX = result.fitContentY = false;
	return result;
}
} // namespace sowa
