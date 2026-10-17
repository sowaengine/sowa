#include "math/matrix.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// gtx/matrix_decompose.hpp is a GTX ("experimental") extension; glm requires
// this define to opt in before including it.
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace sowa::Matrix {

glm::mat4 CalculateTransform2D(Vector2 position, f32 rotation, Vector2 scale,
	Vector2 offset /* = Vector2(0, 0)*/, const glm::mat4 &base /*= glm::mat4(1.f)*/) {
	glm::mat4 transform = glm::translate(base, {position.x, position.y, 0.f});
	transform = glm::rotate(transform, glm::radians(rotation), {0.f, 0.f, 1.f});
	transform = glm::translate(transform, {offset.x, offset.y, 0.f});
	transform = glm::scale(transform, {scale.x, scale.y, 1.f});

	return transform;
}

glm::mat4 CalculateTransform3D(Vector3 position, Vector3 rotation, Vector3 scale,
	const glm::mat4 &base /*= glm::mat4(1.f)*/) {
	glm::mat4 transform = glm::translate(base, {position.x, position.y, position.z});
	transform = glm::rotate(transform, glm::radians(rotation.z), {0.f, 0.f, 1.f});
	transform = glm::rotate(transform, glm::radians(rotation.y), {0.f, 1.f, 0.f});
	transform = glm::rotate(transform, glm::radians(rotation.x), {1.f, 0.f, 0.f});
	transform = glm::scale(transform, {scale.x, scale.y, scale.z});

	return transform;
}

bool DecomposeTransform2D(const glm::mat4 &mat, Vector2 *position, f32 *rotation, Vector2 *scale) {
	glm::vec3 decomposedPosition;
	glm::vec3 decomposedScale;
	glm::quat decomposedOrientation;
	glm::vec3 skew;
	glm::vec4 perspective;

	bool ok = glm::decompose(mat, decomposedScale, decomposedOrientation, decomposedPosition, skew,
		perspective);
	if (!ok) {
		return false;
	}

	if (position) {
		*position = Vector2(decomposedPosition.x, decomposedPosition.y);
	}
	if (rotation) {
		*rotation = glm::degrees(glm::eulerAngles(decomposedOrientation).z);
	}
	if (scale) {
		*scale = Vector2(decomposedScale.x, decomposedScale.y);
	}

	return true;
}

glm::mat4 CalculateOrtho(f32 left, f32 right, f32 bottom, f32 top) {
	return glm::ortho(left, right, bottom, top, -128.f, 128.f);
}

} // namespace sowa::Matrix
