#pragma once

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "ecs/entity.hpp"
#include "math/vector2.hpp"

namespace sowa {

// Local position, rotation, and scale with a derived world matrix. Registered by World.
// Rotation is in degrees: positive counter-clockwise, zero along +X.
struct Transform2D {
	Vector2 position{0.f, 0.f};
	f32 rotation = 0.f;
	Vector2 scale{1.f, 1.f};

	// Derived each frame by CalculateTransforms. Set position/rotation/scale to change it.
	// Excluded from reflection and serialization.
	glm::mat4 worldMatrix{1.f};

	// This entity's own transform, ignoring any parent.
	glm::mat4 LocalMatrix() const;

	// World-space position. Cheap: it is a direct read of the matrix's
	// translation column, no decomposition.
	Vector2 WorldPosition() const;

	// World rotation and scale derived from worldMatrix. Approximate when a rotated
	// parent with non-uniform scale introduces skew; the matrix remains exact.
	f32 WorldRotation() const;
	Vector2 WorldScale() const;
};

// Computes current transforms through ancestors in O(depth), skipping missing Transform2D.
// Use cached worldMatrix after Progress. Returns identity for dead or transformless chains.
glm::mat4 ComputeWorldMatrix(Entity entity);

// Reparents while preserving the current world transform. Null parent detaches.
// Rejects dead handles, self-parenting, and cycles without changes.
bool Reparent(Entity child, Entity newParent);

} // namespace sowa
