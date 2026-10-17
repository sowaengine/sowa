#include <doctest/doctest.h>

#include <box2d/box2d.h>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"
#include "physics/collider2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"

using namespace sowa;

namespace {

void StepTicks(PhysicsWorld &physics, i32 ticks) {
	const f32 dt = physics.Config().fixedDelta;
	for (i32 i = 0; i < ticks; ++i)
		physics.Step(dt);
}

Entity MakeBody(World &world, Vector2 position, BodyType2D type = BodyType2D::Dynamic) {
	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{position, 0.f, Vector2(1.f, 1.f)});
	RigidBody2D body;
	body.type = type;
	entity.Add(body);
	return entity;
}

Collider2D BoxCollider(Vector2 size) {
	Collider2D collider;
	collider.type = ShapeType2D::Box;
	collider.size = size;
	return collider;
}

} // namespace

TEST_CASE("Step writes a dynamic body pose into Transform2D") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 200.f));
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	CHECK(entity.Get<Transform2D>()->position.y == doctest::Approx(200.f));

	StepTicks(physics, 30);
	CHECK(entity.Get<Transform2D>()->position.y < 200.f);
	CHECK(entity.Get<Transform2D>()->position.y ==
		  doctest::Approx(b2Body_GetPosition(physics.BodyId(entity)).y * 100.f));
}

TEST_CASE("A half-tick does not write Transform2D") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 200.f));
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	physics.Step(physics.Config().fixedDelta * 0.5f);
	CHECK(entity.Get<Transform2D>()->position.y == doctest::Approx(200.f));
}

TEST_CASE("Static Transform2D is not overwritten by Step") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(50.f, 80.f), BodyType2D::Static);
	entity.Add(BoxCollider(Vector2(100.f, 20.f)));
	physics.Sync();
	StepTicks(physics, 10);
	CHECK(entity.Get<Transform2D>()->position.x == doctest::Approx(50.f));
	CHECK(entity.Get<Transform2D>()->position.y == doctest::Approx(80.f));
}

TEST_CASE("Moving a static Transform2D teleports the Box2D body") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 0.f), BodyType2D::Static);
	entity.Add(BoxCollider(Vector2(100.f, 20.f)));
	physics.Sync();
	CHECK(b2Body_GetPosition(physics.BodyId(entity)).x == doctest::Approx(0.f));

	entity.Get<Transform2D>()->position = Vector2(150.f, 40.f);
	physics.Step(0.f);
	CHECK(b2Body_GetPosition(physics.BodyId(entity)).x == doctest::Approx(1.5f));
	CHECK(b2Body_GetPosition(physics.BodyId(entity)).y == doctest::Approx(0.4f));
	CHECK(entity.Get<Transform2D>()->position.x == doctest::Approx(150.f));
}

TEST_CASE("Kinematic velocity write-back moves Transform2D") {
	World world;
	PhysicsWorldConfig config;
	config.gravity = Vector2(0.f, 0.f);
	PhysicsWorld physics;
	physics.Init(world, config);

	Entity entity = MakeBody(world, {}, BodyType2D::Kinematic);
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.SetLinearVelocity(entity, Vector2(100.f, 0.f));
	physics.Sync();
	physics.Step(physics.Config().fixedDelta);

	CHECK(entity.Get<Transform2D>()->position.x ==
		  doctest::Approx(physics.Config().fixedDelta * 100.f).epsilon(0.05));
}

TEST_CASE("Write-back of a parented body keeps local x and drops local y") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 0.f), 0.f, Vector2(1.f, 1.f)});
	Entity child = MakeBody(world, Vector2(0.f, 200.f));
	REQUIRE(child.SetParent(parent));
	child.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	CHECK(b2Body_GetPosition(physics.BodyId(child)).x == doctest::Approx(1.f));

	StepTicks(physics, 30);
	const Transform2D *transform = child.Get<Transform2D>();
	REQUIRE(transform != nullptr);
	CHECK(transform->position.x == doctest::Approx(0.f).epsilon(0.05));
	CHECK(transform->position.y < 200.f);
}

TEST_CASE("Progress writes the body pose before CalculateTransforms") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 200.f));
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	world.Progress(physics.Config().fixedDelta);

	const Transform2D *transform = entity.Get<Transform2D>();
	REQUIRE(transform != nullptr);
	CHECK(transform->position.y < 200.f);
	CHECK(transform->WorldPosition().y == doctest::Approx(transform->position.y));
}

TEST_CASE("Write-back does not change Transform2D.scale") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 200.f));
	entity.Get<Transform2D>()->scale = Vector2(2.f, 2.f);
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	StepTicks(physics, 10);
	CHECK(entity.Get<Transform2D>()->scale.x == doctest::Approx(2.f));
	CHECK(entity.Get<Transform2D>()->scale.y == doctest::Approx(2.f));
}
