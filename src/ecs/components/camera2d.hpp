#pragma once

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "ecs/entity.hpp"
#include "math/vector2.hpp"

namespace sowa {

// Scene viewpoint using the entity's Transform2D, including parent transforms.
// Larger scale shows more of the world because the view matrix inverts that transform.
// Registered by World; requires only math types.
struct Camera2D {
	// Shifts the view without moving the entity - useful for a look-ahead or
	// screen-shake offset layered on top of whatever the transform is doing.
	Vector2 offset{0.f, 0.f};

	// Allow view rotation from the camera transform. Otherwise use only position and scale.
	bool rotatable = false;

	// Normalized viewport position of the camera: (0,0) bottom-left, (1,1) top-right,
	// and (0.5,0.5) center. Independent of zoom: the anchored world point stays fixed
	// in screen space as the transform scale changes.
	Vector2 anchor{0.5f, 0.5f};
};

glm::mat4 CalculateViewProjection(Entity camera, Vector2 viewportSize);

// World-space view origin at the camera's anchor, including offset and inherited
// transforms. Pass the same projection used for rendering, or no camera for a
// free editor view (which must leave parallax at its authored position).
Vector2 CalculateCameraOrigin(Entity camera, const glm::mat4 &viewProjection);

// Without a camera, world origin is the viewport bottom-left. A centered camera at
// the origin instead places it at the viewport center.
glm::mat4 CalculateBlankViewProjection(Vector2 viewportSize);

} // namespace sowa
