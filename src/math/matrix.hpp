#pragma once

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

// Transform matrices use Y-up coordinates and degrees: positive counter-clockwise, zero along
// +X.
namespace sowa::Matrix {

glm::mat4 CalculateTransform2D(Vector2 position, f32 rotation, Vector2 scale,
	Vector2 offset = Vector2(0.f, 0.f), const glm::mat4 &base = glm::mat4(1.f));
glm::mat4 CalculateTransform3D(Vector3 position, Vector3 rotation, Vector3 scale,
	const glm::mat4 &base = glm::mat4(1.f));

bool DecomposeTransform2D(const glm::mat4 &mat, Vector2 *position, f32 *rotation, Vector2 *scale);

glm::mat4 CalculateOrtho(f32 left, f32 right, f32 bottom, f32 top);

} // namespace sowa::Matrix
