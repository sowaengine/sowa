#include <doctest/doctest.h>

#include <box2d/box2d.h>

#include "ecs/world.hpp"
#include "physics/physics_world.hpp"

using namespace sowa;

namespace {

b2BodyId MakeFallingBox(b2WorldId worldId, f32 y) {
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = {0.f, y};
	const b2BodyId body = b2CreateBody(worldId, &bodyDef);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.f;
	const b2Polygon box = b2MakeBox(0.5f, 0.5f);
	b2CreatePolygonShape(body, &shapeDef, &box);
	return body;
}

} // namespace

TEST_CASE("PhysicsWorld Init creates a Box2D world and Shutdown destroys it") {
	PhysicsWorld physics;
	physics.Init();
	CHECK(physics.IsInitialized());
	const b2WorldId worldId = physics.WorldId();
	REQUIRE(b2World_IsValid(worldId));
	physics.Shutdown();
	CHECK_FALSE(physics.IsInitialized());
	CHECK_FALSE(b2World_IsValid(physics.WorldId()));
	CHECK_FALSE(b2World_IsValid(worldId));
	physics.Shutdown();
}

TEST_CASE("A dynamic body falls only after a full fixed step has accumulated") {
	PhysicsWorld physics;
	physics.Init();
	const f32 startY = 10.f;
	const b2BodyId body = MakeFallingBox(physics.WorldId(), startY);

	physics.Step(physics.Config().fixedDelta * 0.5f);
	CHECK(b2Body_GetPosition(body).y == doctest::Approx(startY));
	CHECK(physics.Accumulator() == doctest::Approx(physics.Config().fixedDelta * 0.5f));

	physics.Step(physics.Config().fixedDelta * 0.5f);
	CHECK(b2Body_GetPosition(body).y < startY);
	CHECK(physics.Accumulator() == doctest::Approx(0.f));
}

TEST_CASE("A long hitch runs at most maxStepsPerFrame physics ticks") {
	PhysicsWorldConfig config;
	config.maxStepsPerFrame = 2;
	PhysicsWorld physics;
	physics.Init(config);
	const b2BodyId body = MakeFallingBox(physics.WorldId(), 10.f);

	physics.Step(10.f);
	const f32 yAfterHitch = b2Body_GetPosition(body).y;
	REQUIRE(yAfterHitch < 10.f);

	PhysicsWorld expected;
	expected.Init(config);
	const b2BodyId expectedBody = MakeFallingBox(expected.WorldId(), 10.f);
	expected.Step(config.fixedDelta);
	expected.Step(config.fixedDelta);
	CHECK(yAfterHitch == doctest::Approx(b2Body_GetPosition(expectedBody).y));
}

TEST_CASE("PhysicsStep runs in OnValidate: after OnUpdate, before PostUpdate") {
	World world;
	PhysicsWorld physics;

	b2BodyId body = b2_nullBodyId;
	f32 yOnUpdate = -1.f;
	f32 yPostUpdate = -1.f;

	world.Flecs().system("RecordPost").kind(flecs::PostUpdate).run([&](flecs::iter &) {
		if (b2Body_IsValid(body)) {
			yPostUpdate = b2Body_GetPosition(body).y;
		}
	});

	physics.Init(world);
	REQUIRE(physics.IsInitialized());
	body = MakeFallingBox(physics.WorldId(), 10.f);

	world.Flecs().system("RecordUpdate").kind(flecs::OnUpdate).run([&](flecs::iter &) {
		if (b2Body_IsValid(body)) {
			yOnUpdate = b2Body_GetPosition(body).y;
		}
	});

	world.Progress(physics.Config().fixedDelta);

	CHECK(yOnUpdate == doctest::Approx(10.f));
	CHECK(yPostUpdate < 10.f);
	CHECK(b2Body_GetPosition(body).y == doctest::Approx(yPostUpdate));
}

TEST_CASE("PhysicsStep is play-only and does not run while stopped") {
	World world;
	PhysicsWorld physics;
	physics.Init(world);
	const b2BodyId body = MakeFallingBox(physics.WorldId(), 10.f);

	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(physics.Config().fixedDelta);
	CHECK(b2Body_GetPosition(body).y == doctest::Approx(10.f));

	world.SetPlayOnlySystemsEnabled(true);
	world.Progress(physics.Config().fixedDelta);
	CHECK(b2Body_GetPosition(body).y < 10.f);
}
