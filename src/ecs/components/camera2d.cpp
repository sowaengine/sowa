#include "ecs/components/camera2d.hpp"

#include <vector>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/matrix.hpp"

namespace sowa {

glm::mat4 CalculateViewProjection(Entity camera, Vector2 viewportSize) {
	if (!camera.IsAlive()) {
		return CalculateBlankViewProjection(viewportSize);
	}

	const Camera2D *settings = camera.Get<Camera2D>();
	const Vector2 offset = settings != nullptr ? settings->offset : Vector2(0.f, 0.f);
	const bool rotatable = settings != nullptr && settings->rotatable;
	const Vector2 anchor = settings != nullptr ? settings->anchor : Vector2(0.5f, 0.5f);

	Vector2 position(0.f, 0.f);
	Vector2 scale(1.f, 1.f);
	f32 rotation = 0.f;
	Matrix::DecomposeTransform2D(ComputeWorldMatrix(camera), &position, &rotation, &scale);

	// Rebuilt from the decomposed parts rather than inverting the world matrix
	// directly, because dropping rotation is only expressible on the parts.
	if (!rotatable) {
		rotation = 0.f;
	}
	const glm::mat4 view =
		glm::inverse(Matrix::CalculateTransform2D(position, rotation, scale, offset));

	// Shifts the ortho box so the camera's position lands on anchor within
	// the viewport. Falls out of solving 2*(0 - left)/w - 1 == 2*anchor - 1
	// for the camera's own position in view space, which is the origin.
	const f32 left = -viewportSize.x * anchor.x;
	const f32 bottom = -viewportSize.y * anchor.y;
	return Matrix::CalculateOrtho(left, left + viewportSize.x, bottom, bottom + viewportSize.y) *
		   view;
}

Vector2 CalculateCameraOrigin(Entity camera, const glm::mat4 &viewProjection) {
	if (!camera.IsAlive()) {
		return {};
	}
	const auto *settings = camera.Get<Camera2D>();
	const Vector2 anchor = settings ? settings->anchor : Vector2(0.5f, 0.5f);
	const glm::vec4 anchorPoint{anchor.x * 2.f - 1.f, anchor.y * 2.f - 1.f, 0.f, 1.f};
	const auto point = glm::inverse(viewProjection) * anchorPoint;
	return {point.x / point.w, point.y / point.w};
}

glm::mat4 CalculateBlankViewProjection(Vector2 viewportSize) {
	return Matrix::CalculateOrtho(0.f, viewportSize.x, 0.f, viewportSize.y);
}

} // namespace sowa
