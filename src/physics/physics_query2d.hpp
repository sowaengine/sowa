#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ecs/entity.hpp"
#include "physics/collider2d.hpp"

namespace sowa {

inline constexpr u32 AllPhysicsLayers = 0xffffffffu;

struct PhysicsQueryOptions2D {
	u32 collisionLayer = AllPhysicsLayers;
	u32 collisionMask = AllPhysicsLayers;
	bool includeSensors = false;
	std::vector<Entity> excludeColliders;
	std::vector<Entity> excludeBodies;
};

// Standalone world-space geometry. Lengths are pixels, rotation is degrees.
struct PhysicsQueryShape2D {
	ShapeType2D type = ShapeType2D::Box;
	Vector2 position;
	Vector2 size{100.f, 100.f};
	f32 radius = 50.f;
	f32 rotation = 0.f;
};

struct PhysicsOverlap2D {
	Entity collider;
	Entity body;
	bool isSensor = false;
};

struct PhysicsHit2D : PhysicsOverlap2D {
	Vector2 point;
	Vector2 normal;
	f32 fraction = 0.f;
	f32 distance = 0.f;
};

// Empty results are successful misses; errors never reach backend assertions.
template <typename T> struct PhysicsQueryResult2D {
	T value{};
	std::string error;

	explicit operator bool() const {
		return error.empty();
	}
};

using PhysicsClosestResult2D = PhysicsQueryResult2D<std::optional<PhysicsHit2D>>;
using PhysicsHitsResult2D = PhysicsQueryResult2D<std::vector<PhysicsHit2D>>;
using PhysicsOverlapsResult2D = PhysicsQueryResult2D<std::vector<PhysicsOverlap2D>>;

} // namespace sowa
