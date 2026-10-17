#include <vector>

#include <doctest/doctest.h>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"
#include "physics/collider2d.hpp"
#include "physics/collision2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"

using namespace sowa;

namespace {

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

bool Involves(const Collision2D &event, Entity a, Entity b) {
	return (event.colliderA == a && event.colliderB == b) ||
		   (event.colliderA == b && event.colliderB == a);
}

bool HasKind(const std::vector<Collision2D> &events, CollisionKind kind, Entity a, Entity b) {
	for (const Collision2D &event : events) {
		if (event.kind == kind && Involves(event, a, b))
			return true;
	}
	return false;
}

i32 CountKind(const std::vector<Collision2D> &events, CollisionKind kind, Entity a, Entity b) {
	i32 count = 0;
	for (const Collision2D &event : events) {
		if (event.kind == kind && Involves(event, a, b))
			++count;
	}
	return count;
}

} // namespace

TEST_CASE("A solid floor emits Begin when a box lands") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity floor = MakeBody(world, {}, BodyType2D::Static);
	floor.Add(BoxCollider(Vector2(400.f, 20.f)));
	Entity box = MakeBody(world, Vector2(0.f, 200.f));
	box.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();

	bool began = false;
	Collision2D hit;
	const f32 dt = physics.Config().fixedDelta;
	for (i32 i = 0; i < 90; ++i) {
		physics.Step(dt);
		if (HasKind(physics.FrameEvents(), CollisionKind::Begin, floor, box)) {
			for (const Collision2D &event : physics.FrameEvents()) {
				if (event.kind == CollisionKind::Begin && Involves(event, floor, box)) {
					hit = event;
					break;
				}
			}
			began = true;
			break;
		}
	}
	REQUIRE(began);
	const bool floorIsBody = hit.bodyA == floor || hit.bodyB == floor;
	const bool boxIsBody = hit.bodyA == box || hit.bodyB == box;
	CHECK(floorIsBody);
	CHECK(boxIsBody);
	const bool hasNormal = hit.normal.x != 0.f || hit.normal.y != 0.f;
	CHECK(hasNormal);

	i32 extra = 0;
	for (i32 i = 0; i < 30; ++i) {
		physics.Step(dt);
		extra += CountKind(physics.FrameEvents(), CollisionKind::Begin, floor, box);
	}
	CHECK(extra == 0);
}

TEST_CASE("isTrigger emits TriggerBegin, not Begin") {
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

	bool triggered = false;
	i32 begins = 0;
	const f32 dt = physics.Config().fixedDelta;
	for (i32 i = 0; i < 90; ++i) {
		physics.Step(dt);
		if (HasKind(physics.FrameEvents(), CollisionKind::TriggerBegin, floor, box))
			triggered = true;
		begins += CountKind(physics.FrameEvents(), CollisionKind::Begin, floor, box);
	}
	CHECK(triggered);
	CHECK(begins == 0);
}

TEST_CASE("Disable after contact emits End") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity floor = MakeBody(world, {}, BodyType2D::Static);
	floor.Add(BoxCollider(Vector2(400.f, 20.f)));
	Entity box = MakeBody(world, Vector2(0.f, 200.f));
	box.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();

	const f32 dt = physics.Config().fixedDelta;
	bool began = false;
	for (i32 i = 0; i < 90; ++i) {
		physics.Step(dt);
		if (HasKind(physics.FrameEvents(), CollisionKind::Begin, floor, box)) {
			began = true;
			break;
		}
	}
	REQUIRE(began);

	box.SetEnabled(false);
	physics.Sync();
	physics.Step(dt);
	CHECK(HasKind(physics.FrameEvents(), CollisionKind::End, floor, box));
}

TEST_CASE("A child collider event names the body entity") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity floor = MakeBody(world, {}, BodyType2D::Static);
	floor.Add(BoxCollider(Vector2(400.f, 20.f)));
	Entity body = MakeBody(world, Vector2(0.f, 200.f));
	Entity child = world.CreateEntity();
	child.Add(Transform2D{});
	REQUIRE(child.SetParent(body));
	child.Add(BoxCollider(Vector2(100.f, 100.f)));
	physics.Sync();

	bool began = false;
	Collision2D hit;
	const f32 dt = physics.Config().fixedDelta;
	for (i32 i = 0; i < 90; ++i) {
		physics.Step(dt);
		for (const Collision2D &event : physics.FrameEvents()) {
			if (event.kind == CollisionKind::Begin && Involves(event, floor, child)) {
				hit = event;
				began = true;
				break;
			}
		}
		if (began)
			break;
	}
	REQUIRE(began);
	const bool named = (hit.colliderA == child && hit.bodyA == body) ||
					   (hit.colliderB == child && hit.bodyB == body);
	CHECK(named);
}

TEST_CASE("A hitch still reports events from each tick") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity trigger = MakeBody(world, {}, BodyType2D::Static);
	Collider2D sensor = BoxCollider(Vector2(200.f, 200.f));
	sensor.isTrigger = true;
	trigger.Add(sensor);
	Entity box = MakeBody(world, {});
	box.Add(BoxCollider(Vector2(40.f, 40.f)));
	physics.Sync();
	physics.SetLinearVelocity(box, Vector2(0.f, -4000.f));

	physics.Step(physics.Config().fixedDelta * static_cast<f32>(physics.Config().maxStepsPerFrame));
	CHECK(HasKind(physics.FrameEvents(), CollisionKind::TriggerBegin, trigger, box));
	CHECK(HasKind(physics.FrameEvents(), CollisionKind::TriggerEnd, trigger, box));
}
