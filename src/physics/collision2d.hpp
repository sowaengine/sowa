#pragma once

#include "ecs/entity.hpp"
#include "math/vector2.hpp"

namespace sowa {

// Begin/end events for solid contacts and trigger overlaps.
enum class CollisionKind {
	Begin,
	End,
	TriggerBegin,
	TriggerEnd,
};

// One contact or trigger event from the last physics tick. Units are pixels.
// colliderA / colliderB are the Collider2D entities. bodyA / bodyB are the
// RigidBody2D entities those shapes attach to (self or an ancestor).
// For triggers, A is the sensor and B is the visitor (Box2D's split).
struct Collision2D {
	CollisionKind kind = CollisionKind::Begin;
	Entity colliderA;
	Entity colliderB;
	Entity bodyA;
	Entity bodyB;
	// First manifold point, or zero. Contact begin only.
	Vector2 point;
	// World normal from A to B, or zero. Contact begin only.
	Vector2 normal;
};

} // namespace sowa
