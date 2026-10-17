#pragma once

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"
#include "math/vector2.hpp"

namespace sowa {

// Collider shape kind. Compound bodies use child entities with additional Collider2D
// components.
enum class ShapeType2D {
	Box,
	Circle,
	Capsule,
};

SOWA_REFLECT_ENUM(ShapeType2D, {"Box", ShapeType2D::Box}, {"Circle", ShapeType2D::Circle},
	{"Capsule", ShapeType2D::Capsule});

// Collider data attached to the nearest RigidBody2D; missing bodies produce no shape.
// PhysicsWorld registers and owns shapes. Units are pixels/degrees; density determines
// mass. Triggers report overlaps without collision response.
struct Collider2D {
	ShapeType2D type = ShapeType2D::Box;
	// Box: full width and height. Capsule: size.y is full height, caps
	// included. Circle ignores size.
	Vector2 size{100.f, 100.f};
	// Circle and capsule. Unused on a box.
	f32 radius = 50.f;
	// Local shift from the collider origin, same idea as Sprite.offset.
	Vector2 offset;
	// Extra local rotation in degrees. Applied on top of the collider
	// entity's transform. Circle ignores this around its own centre.
	f32 rotation = 0.f;
	// Reports overlaps without solid collision response.
	bool isTrigger = false;
	f32 density = 1.f;
	// Box2D's default.
	f32 friction = 0.6f;
	f32 restitution = 0.f;
	// Bilateral filtering: each shape's layer must be accepted by the other's mask.
	u32 collisionLayer = 1u;
	u32 collisionMask = 0xffffffffu;
};

} // namespace sowa
