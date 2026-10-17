#include <doctest/doctest.h>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "physics/collider2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"

using namespace sowa;

namespace {

Entity MakeBody(World &world, Vector2 position, BodyType2D type = BodyType2D::Static) {
	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{position, 0.f, Vector2(1.f, 1.f)});
	RigidBody2D body;
	body.type = type;
	entity.Add(body);
	return entity;
}

} // namespace

TEST_CASE("DebugDraw reports no segments for an empty world") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	i32 segments = 0;
	physics.DebugDraw([&](Vector2, Vector2, Color) { ++segments; });
	CHECK(segments == 0);
}

TEST_CASE("DebugDraw reports outline segments for a box collider") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity floor = MakeBody(world, {});
	Collider2D box;
	box.size = Vector2(400.f, 20.f);
	floor.Add(box);
	physics.Sync();

	i32 segments = 0;
	physics.DebugDraw([&](Vector2, Vector2, Color) { ++segments; });
	CHECK(segments >= 4);
}

TEST_CASE("DebugDraw reports a circle outline for a circle collider") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity ball = MakeBody(world, Vector2(0.f, 100.f), BodyType2D::Dynamic);
	Collider2D circle;
	circle.type = ShapeType2D::Circle;
	circle.radius = 40.f;
	ball.Add(circle);
	physics.Sync();

	i32 segments = 0;
	physics.DebugDraw([&](Vector2, Vector2, Color) { ++segments; });
	CHECK(segments >= 16);
}
