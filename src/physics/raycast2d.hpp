#pragma once

#include "physics/physics_query2d.hpp"

namespace sowa {

// Authored data only. PhysicsWorld owns the cache and runtime exceptions.
struct Raycast2D {
	bool enabled = true;
	Vector2 targetPosition{0.f, -50.f};
	u32 collisionLayer = AllPhysicsLayers;
	u32 collisionMask = AllPhysicsLayers;
	bool includeSensors = false;
	bool excludeOwner = true;
};

} // namespace sowa
