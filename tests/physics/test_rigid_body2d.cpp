#include <string>

#include <doctest/doctest.h>

#include <box2d/box2d.h>

#include "core/log.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"

using namespace sowa;

namespace {

void AddUnitBox(b2BodyId body) {
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.f;
	const b2Polygon box = b2MakeBox(0.5f, 0.5f);
	b2CreatePolygonShape(body, &shapeDef, &box);
}

Entity MakeBodyEntity(World &world, Vector2 position = {}, BodyType2D type = BodyType2D::Dynamic) {
	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{position, 0.f, Vector2(1.f, 1.f)});
	RigidBody2D body;
	body.type = type;
	entity.Add(body);
	return entity;
}

} // namespace

TEST_CASE("Sync creates a Box2D body at the Transform2D world pose") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = MakeBodyEntity(world, Vector2(100.f, 200.f));
	physics.Sync();

	const b2BodyId body = physics.BodyId(entity);
	REQUIRE(b2Body_IsValid(body));
	CHECK(b2Body_GetPosition(body).x == doctest::Approx(1.f));
	CHECK(b2Body_GetPosition(body).y == doctest::Approx(2.f));
}

TEST_CASE("Sync uses ComputeWorldMatrix so a parented body is not at local pose") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 0.f), 0.f, Vector2(1.f, 1.f)});
	Entity child = MakeBodyEntity(world, Vector2(50.f, 0.f));
	REQUIRE(child.SetParent(parent));
	physics.Sync();

	CHECK(b2Body_GetPosition(physics.BodyId(child)).x == doctest::Approx(1.5f));
}

TEST_CASE("Sync skips a RigidBody2D with no Transform2D and warns once") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);

	Entity entity = world.CreateEntity();
	entity.SetName("NoPose");
	entity.Add<RigidBody2D>();

	Log::Clear();
	physics.Sync();
	CHECK_FALSE(b2Body_IsValid(physics.BodyId(entity)));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Warn);

	const std::size_t afterFirst = Log::GetLines().size();
	physics.Sync();
	CHECK(Log::GetLines().size() == afterFirst);
}

TEST_CASE("Sync destroys the body when RigidBody2D is removed") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Entity entity = MakeBodyEntity(world);
	physics.Sync();
	REQUIRE(b2Body_IsValid(physics.BodyId(entity)));

	entity.Remove<RigidBody2D>();
	physics.Sync();
	CHECK_FALSE(b2Body_IsValid(physics.BodyId(entity)));
}

TEST_CASE("Sync skips a disabled entity") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Entity entity = MakeBodyEntity(world);
	physics.Sync();
	REQUIRE(b2Body_IsValid(physics.BodyId(entity)));

	entity.SetEnabled(false);
	physics.Sync();
	CHECK_FALSE(b2Body_IsValid(physics.BodyId(entity)));
}

TEST_CASE("Kinematic SetLinearVelocity moves the body on Step") {
	World world;
	PhysicsWorldConfig config;
	config.gravity = Vector2(0.f, 0.f);
	PhysicsWorld physics;
	physics.Init(world, config);

	Entity entity = MakeBodyEntity(world, {}, BodyType2D::Kinematic);
	physics.SetLinearVelocity(entity, Vector2(100.f, 0.f));
	physics.Sync();
	physics.Step(physics.Config().fixedDelta);

	const f32 x = b2Body_GetPosition(physics.BodyId(entity)).x;
	CHECK(x == doctest::Approx(physics.Config().fixedDelta).epsilon(0.05));
	CHECK(physics.GetLinearVelocity(entity).x == doctest::Approx(100.f));
}

TEST_CASE("Progress Syncs RigidBody2D entities") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Entity entity = MakeBodyEntity(world);
	CHECK_FALSE(b2Body_IsValid(physics.BodyId(entity)));
	world.Progress(physics.Config().fixedDelta);
	CHECK(b2Body_IsValid(physics.BodyId(entity)));
}

TEST_CASE("ApplyForce applies only to the next physics tick even in a hitch") {
	PhysicsWorldConfig config;
	config.gravity = Vector2(0.f, 0.f);
	config.maxStepsPerFrame = 8;

	auto setup = [&](f32 frameDelta) {
		World w;
		PhysicsWorld p;
		p.Init(w, config);
		Entity e = MakeBodyEntity(w);
		p.Sync();
		AddUnitBox(p.BodyId(e));
		p.ApplyForce(e, Vector2(0.f, 1000.f));
		p.Step(frameDelta);
		return p.GetLinearVelocity(e).y;
	};

	const f32 velOne = setup(config.fixedDelta);
	const f32 velTwo = setup(config.fixedDelta * 2.f);
	REQUIRE(velOne > 0.f);
	CHECK(velTwo == doctest::Approx(velOne).epsilon(0.001));
}

TEST_CASE("ApplyImpulse runs once even when a hitch takes two ticks") {
	PhysicsWorldConfig config;
	config.gravity = Vector2(0.f, 0.f);

	auto setup = [&](f32 frameDelta) {
		World w;
		PhysicsWorld p;
		p.Init(w, config);
		Entity e = MakeBodyEntity(w);
		p.Sync();
		AddUnitBox(p.BodyId(e));
		p.ApplyImpulse(e, Vector2(0.f, 100.f));
		p.Step(frameDelta);
		return p.GetLinearVelocity(e).y;
	};

	const f32 velOne = setup(config.fixedDelta);
	const f32 velTwo = setup(config.fixedDelta * 2.f);
	REQUIRE(velOne > 0.f);
	CHECK(velTwo == doctest::Approx(velOne).epsilon(0.15));
}

TEST_CASE("Changing type to Static stops a falling body") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	Entity entity = MakeBodyEntity(world, Vector2(0.f, 1000.f));
	physics.Sync();
	AddUnitBox(physics.BodyId(entity));

	physics.Step(physics.Config().fixedDelta);
	const f32 yMoving = b2Body_GetPosition(physics.BodyId(entity)).y;
	CHECK(yMoving < 10.f);

	entity.Get<RigidBody2D>()->type = BodyType2D::Static;
	physics.Sync();
	physics.Step(physics.Config().fixedDelta);
	physics.Step(physics.Config().fixedDelta);
	CHECK(b2Body_GetPosition(physics.BodyId(entity)).y == doctest::Approx(yMoving));
}
