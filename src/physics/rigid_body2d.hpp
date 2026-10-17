#pragma once

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"

namespace sowa {

// Body kind corresponding to Box2D's three body types.
enum class BodyType2D {
	// Infinite mass. Transform2D supplies the pose.
	Static,
	// Infinite mass. Gameplay sets velocity or pose. Forces are ignored.
	Kinematic,
	// Simulated. Default.
	Dynamic,
};

SOWA_REFLECT_ENUM(BodyType2D, {"Static", BodyType2D::Static}, {"Kinematic", BodyType2D::Kinematic},
	{"Dynamic", BodyType2D::Dynamic});

// Authored rigid-body data. PhysicsWorld owns the body handle and force queue.
// Velocity and simulated pose use PhysicsWorld accessors rather than serialized fields.
// Mass comes from collider density; a body without a collider has zero mass and is
// unaffected by gravity. Registered by PhysicsWorld::Init.
struct RigidBody2D {
	BodyType2D type = BodyType2D::Dynamic;
	f32 gravityScale = 1.f;
	f32 linearDamping = 0.f;
	f32 angularDamping = 0.f;
	bool fixedRotation = false;
};

} // namespace sowa
