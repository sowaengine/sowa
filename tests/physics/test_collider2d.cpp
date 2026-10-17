#include <string>

#include <doctest/doctest.h>

#include <box2d/box2d.h>

#include "core/log.hpp"
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

TEST_CASE("A RigidBody2D with no Collider2D does not fall") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 200.f));
	physics.Sync();
	REQUIRE(b2Body_IsValid(physics.BodyId(entity)));
	CHECK(b2Body_GetMass(physics.BodyId(entity)) == doctest::Approx(0.f));

	StepTicks(physics, 30);
	CHECK(b2Body_GetPosition(physics.BodyId(entity)).y == doctest::Approx(2.f));
}

TEST_CASE("A dynamic Collider2D box falls") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, Vector2(0.f, 200.f));
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();

	REQUIRE(b2Shape_IsValid(physics.ShapeId(entity)));
	CHECK(b2Body_GetMass(physics.BodyId(entity)) == doctest::Approx(1.f));

	StepTicks(physics, 30);
	CHECK(b2Body_GetPosition(physics.BodyId(entity)).y < 2.f);
}

TEST_CASE("A static Collider2D floor stops a falling box") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity floor = MakeBody(world, {}, BodyType2D::Static);
	floor.Add(BoxCollider(Vector2(400.f, 20.f)));
	Entity box = MakeBody(world, Vector2(0.f, 200.f));
	box.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();

	StepTicks(physics, 90);
	const f32 y = b2Body_GetPosition(physics.BodyId(box)).y;
	CHECK(y == doctest::Approx(0.6f).epsilon(0.15));
}

TEST_CASE("isTrigger floor does not stop a falling box") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity floor = MakeBody(world, {}, BodyType2D::Static);
	Collider2D trigger = BoxCollider(Vector2(400.f, 20.f));
	trigger.isTrigger = true;
	floor.Add(trigger);
	Entity box = MakeBody(world, Vector2(0.f, 200.f));
	box.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	CHECK(b2Shape_IsSensor(physics.ShapeId(floor)));

	StepTicks(physics, 60);
	CHECK(b2Body_GetPosition(physics.BodyId(box)).y < 0.f);
}

TEST_CASE("A child Collider2D sits at the child's pose in body space") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity body = MakeBody(world, {});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(50.f, 0.f), 0.f, Vector2(1.f, 1.f)});
	REQUIRE(child.SetParent(body));
	Collider2D circle;
	circle.type = ShapeType2D::Circle;
	circle.radius = 50.f;
	child.Add(circle);
	physics.Sync();

	REQUIRE(b2Shape_IsValid(physics.ShapeId(child)));
	CHECK_FALSE(b2Shape_IsValid(physics.ShapeId(body)));
	const b2Circle shape = b2Shape_GetCircle(physics.ShapeId(child));
	CHECK(shape.center.x == doctest::Approx(0.5f));
	CHECK(shape.center.y == doctest::Approx(0.f));
	CHECK(shape.radius == doctest::Approx(0.5f));
}

TEST_CASE("Collider2D offset is in body-local pixels") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, {});
	Collider2D circle;
	circle.type = ShapeType2D::Circle;
	circle.radius = 50.f;
	circle.offset = Vector2(50.f, 0.f);
	entity.Add(circle);
	physics.Sync();

	const b2Circle shape = b2Shape_GetCircle(physics.ShapeId(entity));
	CHECK(shape.center.x == doctest::Approx(0.5f));
}

TEST_CASE("Removing Collider2D destroys the shape") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Entity entity = MakeBody(world, {});
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	REQUIRE(b2Shape_IsValid(physics.ShapeId(entity)));

	entity.Remove<Collider2D>();
	physics.Sync();
	CHECK_FALSE(b2Shape_IsValid(physics.ShapeId(entity)));
	CHECK(b2Body_IsValid(physics.BodyId(entity)));
	CHECK(b2Body_GetMass(physics.BodyId(entity)) == doctest::Approx(0.f));
}

TEST_CASE("Disabling a collider entity destroys its shape") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity body = MakeBody(world, {});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(0.f, 0.f), 0.f, Vector2(1.f, 1.f)});
	REQUIRE(child.SetParent(body));
	child.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	REQUIRE(b2Shape_IsValid(physics.ShapeId(child)));

	child.SetEnabled(false);
	physics.Sync();
	CHECK_FALSE(b2Shape_IsValid(physics.ShapeId(child)));
	CHECK(b2Body_IsValid(physics.BodyId(body)));
}

TEST_CASE("Collider2D with no RigidBody2D warns once and creates no shape") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = world.CreateEntity();
	entity.SetName("NoBody");
	entity.Add(Transform2D{});
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));

	Log::Clear();
	physics.Sync();
	CHECK_FALSE(b2Shape_IsValid(physics.ShapeId(entity)));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Warn);

	const std::size_t afterFirst = Log::GetLines().size();
	physics.Sync();
	CHECK(Log::GetLines().size() == afterFirst);
}

TEST_CASE("Density change updates body mass") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, {});
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();
	CHECK(b2Body_GetMass(physics.BodyId(entity)) == doctest::Approx(1.f));

	entity.Get<Collider2D>()->density = 2.f;
	physics.Sync();
	CHECK(b2Body_GetMass(physics.BodyId(entity)) == doctest::Approx(2.f));
}

TEST_CASE("World scale of the collider entity scales the box") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, {});
	entity.Get<Transform2D>()->scale = Vector2(2.f, 2.f);
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();

	const b2Polygon poly = b2Shape_GetPolygon(physics.ShapeId(entity));
	f32 maxX = 0.f;
	for (i32 i = 0; i < poly.count; ++i)
		maxX = poly.vertices[i].x > maxX ? poly.vertices[i].x : maxX;
	CHECK(maxX == doctest::Approx(1.f));
}

TEST_CASE("Capsule Collider2D creates a capsule shape") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBody(world, {});
	Collider2D capsule;
	capsule.type = ShapeType2D::Capsule;
	capsule.size = Vector2(100.f, 200.f);
	capsule.radius = 50.f;
	entity.Add(capsule);
	physics.Sync();

	REQUIRE(b2Shape_IsValid(physics.ShapeId(entity)));
	CHECK(b2Shape_GetType(physics.ShapeId(entity)) == b2_capsuleShape);
	const b2Capsule shape = b2Shape_GetCapsule(physics.ShapeId(entity));
	CHECK(shape.radius == doctest::Approx(0.5f));
	CHECK(shape.center1.y == doctest::Approx(0.5f));
	CHECK(shape.center2.y == doctest::Approx(-0.5f));
}

TEST_CASE("Progress Syncs Collider2D shapes") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Entity entity = MakeBody(world, {});
	entity.Add(BoxCollider(Vector2(100.f, 100.f)));
	CHECK_FALSE(b2Shape_IsValid(physics.ShapeId(entity)));
	world.Progress(physics.Config().fixedDelta);
	CHECK(b2Shape_IsValid(physics.ShapeId(entity)));
}
