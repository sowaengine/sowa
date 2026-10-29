#include <doctest/doctest.h>
#include <limits>

#include "core/filesystem/file_system.hpp"
#include "core/game_time.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "core/serialize/archive.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/inspector.hpp"
#include "physics/physics_world.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {
struct QueryFixture {
	World world;
	PhysicsWorld physics;

	QueryFixture(f32 ppm = 100.f) {
		PhysicsWorldConfig config;
		config.gravity = {};
		config.pixelsPerMeter = ppm;
		physics.Init(world, config);
	}

	Entity Box(Vector2 position, Vector2 size = Vector2(20.f),
		BodyType2D type = BodyType2D::Static) {
		Entity entity = world.CreateEntity();
		entity.Add(Transform2D{position});
		RigidBody2D body;
		body.type = type;
		entity.Add(body);
		Collider2D collider;
		collider.size = size;
		entity.Add(collider);
		return entity;
	}

	Entity Ray(Vector2 target = Vector2(200.f, 0.f), Entity parent = {}) {
		Entity entity = world.CreateEntity();
		entity.Add(Transform2D{});
		Raycast2D ray;
		ray.targetPosition = target;
		entity.Add(ray);
		if (parent.IsAlive()) {
			REQUIRE(entity.SetParent(parent));
		}
		return entity;
	}
};
} // namespace

TEST_CASE("ray hits are sorted and measured in pixels at nondefault scale") {
	QueryFixture f(25.f);
	Entity far = f.Box({160.f, 0.f});
	Entity near = f.Box({100.f, 0.f});
	f.physics.Sync();
	auto all = f.physics.RaycastAll({}, {200.f, 0.f});
	REQUIRE(all);
	REQUIRE(all.value.size() == 2);
	CHECK(all.value[0].collider == near);
	CHECK(all.value[1].collider == far);
	const auto hit = f.physics.RaycastClosest({}, {200.f, 0.f});
	REQUIRE(hit.value);
	CHECK(hit.value->body == near);
	CHECK(hit.value->point.x == doctest::Approx(90.f));
	CHECK(hit.value->point.y == doctest::Approx(0.f));
	CHECK(hit.value->distance == doctest::Approx(90.f));
	CHECK(hit.value->fraction == doctest::Approx(.45f));
	CHECK(hit.value->normal == Vector2(-1.f, 0.f));
	CHECK_FALSE(hit.value->isSensor);
	CHECK_FALSE(f.physics.RaycastClosest({}, {-200.f, 0.f}).value);
	CHECK_FALSE(f.physics.RaycastClosest({}, {}).value);
}

TEST_CASE("query filtering is bilateral with zero multiple and high bits") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	target.Get<Collider2D>()->collisionLayer = 0x80000002u;
	target.Get<Collider2D>()->collisionMask = 4u;
	f.physics.Sync();
	PhysicsQueryOptions2D options;
	options.collisionMask = 0x80000000u;
	options.collisionLayer = 4;
	REQUIRE(f.physics.RaycastClosest({}, {200.f, 0.f}, options).value);
	options.collisionLayer = 2;
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}, options).value);
	options.collisionLayer = 4;
	options.collisionMask = 1;
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}, options).value);
	options.collisionMask = 0;
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}, options).value);
	target.Get<Collider2D>()->collisionMask = 0;
	f.physics.Sync();
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}).value);
	target.Get<Collider2D>()->collisionMask = AllPhysicsLayers;
	target.Get<Collider2D>()->collisionLayer = 0;
	f.physics.Sync();
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}).value);
}

TEST_CASE("filters apply at creation and refilter without recreating shapes") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f}, Vector2(20.f), BodyType2D::Dynamic);
	target.Get<Collider2D>()->collisionLayer = 2;
	target.Get<Collider2D>()->collisionMask = 8;
	f.physics.Sync();
	const b2ShapeId id = f.physics.ShapeId(target);
	CHECK(b2Shape_GetFilter(id).categoryBits == 2);
	CHECK(b2Shape_GetFilter(id).maskBits == 8);
	const f32 mass = b2Body_GetMass(f.physics.BodyId(target));
	target.Get<Collider2D>()->collisionMask = 0x80000000u;
	target.Get<Collider2D>()->collisionLayer = 16;
	f.physics.Sync();
	CHECK(b2StoreShapeId(id) == b2StoreShapeId(f.physics.ShapeId(target)));
	CHECK(b2Shape_GetFilter(id).categoryBits == 16);
	CHECK(b2Shape_GetFilter(id).maskBits == 0x80000000u);
	CHECK(b2Body_GetMass(f.physics.BodyId(target)) == mass);
}

TEST_CASE("sensors and compound-body exclusions use the shared collector") {
	QueryFixture f;
	Entity sensor = f.Box({50.f, 0.f});
	sensor.Get<Collider2D>()->isTrigger = true;
	Entity parent = f.Box({100.f, 0.f});
	Entity child = f.world.CreateEntity();
	child.Add(Transform2D{{40.f, 0.f}});
	child.Add(Collider2D{});
	child.Get<Collider2D>()->size = Vector2(20.f);
	REQUIRE(child.SetParent(parent));
	Entity far = f.Box({180.f, 0.f});
	f.physics.Sync();
	CHECK(f.physics.RaycastClosest({}, {250.f, 0.f}).value->collider == parent);
	PhysicsQueryOptions2D options;
	options.includeSensors = true;
	CHECK(f.physics.RaycastClosest({}, {250.f, 0.f}, options).value->collider == sensor);
	options.includeSensors = false;
	options.excludeColliders = {parent};
	auto hit = f.physics.RaycastClosest({}, {250.f, 0.f}, options);
	REQUIRE(hit.value);
	CHECK(hit.value->collider == child);
	CHECK(hit.value->body == parent);
	options.excludeBodies = {parent};
	CHECK(f.physics.RaycastClosest({}, {250.f, 0.f}, options).value->collider == far);
	PhysicsQueryShape2D shape;
	shape.size = {250.f, 100.f};
	shape.position = {125.f, 0.f};
	auto overlaps = f.physics.OverlapShape(shape, options);
	REQUIRE(overlaps.value.size() == 1);
	CHECK(overlaps.value[0].collider == far);
}

TEST_CASE("closest and all resolve equal distance by entity ID") {
	QueryFixture f;
	Entity first = f.Box({100.f, 0.f});
	Entity second = f.Box({100.f, 0.f});
	f.physics.Sync();
	auto all = f.physics.RaycastAll({}, {200.f, 0.f});
	REQUIRE(all.value.size() == 2);
	CHECK(all.value[0].collider.Raw().id() < all.value[1].collider.Raw().id());
	CHECK(f.physics.RaycastClosest({}, {200.f, 0.f}).value->collider == all.value[0].collider);
}

TEST_CASE("box circle and capsule casts and exact overlap") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	f.physics.Sync();
	for (ShapeType2D type : {ShapeType2D::Box, ShapeType2D::Circle, ShapeType2D::Capsule}) {
		PhysicsQueryShape2D shape;
		shape.type = type;
		shape.radius = 5;
		shape.size = {10.f, 20.f};
		auto hit = f.physics.ShapeCastClosest(shape, {200.f, 0.f});
		REQUIRE(hit);
		REQUIRE(hit.value);
		CHECK(hit.value->collider == target);
		CHECK(hit.value->distance == doctest::Approx(85.f).epsilon(.02));
		CHECK(hit.value->point.x == doctest::Approx(90.f).epsilon(.02));
		CHECK(hit.value->normal.x == doctest::Approx(-1.f));
		CHECK(f.physics.ShapeCastAll(shape, {200.f, 0.f}).value.size() == 1);
		shape.position = {100.f, 0.f};
		CHECK(f.physics.OverlapShape(shape).value.size() == 1);
		CHECK_FALSE(f.physics.ShapeCastClosest(shape, {200.f, 0.f}).value);
		CHECK_FALSE(f.physics.RaycastClosest({100.f, 0.f}, {200.f, 0.f}).value);
	}
	PhysicsQueryShape2D rotated;
	rotated.size = {100.f, 2.f};
	rotated.position = {100.f, 40.f};
	rotated.rotation = 45;
	// AABBs overlap, but the rotated thin box does not intersect target.
	CHECK(f.physics.OverlapShape(rotated).value.empty());
}

TEST_CASE("queries match transformed child collider geometry") {
	QueryFixture f(40.f);
	Entity parent = f.Box({100.f, 50.f});
	parent.Remove<Collider2D>();
	parent.Get<Transform2D>()->rotation = 90;
	parent.Get<Transform2D>()->scale = {-2.f, 3.f};
	Entity child = f.world.CreateEntity();
	child.Add(Transform2D{{10.f, 0.f}});
	Collider2D circle;
	circle.type = ShapeType2D::Circle;
	circle.radius = 4;
	circle.offset = {5.f, 0.f};
	child.Add(circle);
	REQUIRE(child.SetParent(parent));
	f.physics.Sync();
	const auto geometry = b2Shape_GetCircle(f.physics.ShapeId(child));
	const auto center = b2Body_GetWorldPoint(f.physics.BodyId(parent), geometry.center);
	const Vector2 centerPx{center.x * 40.f, center.y * 40.f};
	const f32 radiusPx = geometry.radius * 40.f;
	auto hit = f.physics.RaycastClosest({centerPx.x - 100.f, centerPx.y}, {200.f, 0.f});
	REQUIRE(hit.value);
	CHECK(hit.value->collider == child);
	CHECK(hit.value->body == parent);
	CHECK(hit.value->point.x == doctest::Approx(centerPx.x - radiusPx));
}

TEST_CASE("queries validate input and never return stale or foreign shapes") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	f.physics.Sync();
	SUBCASE("removed before sync") {
		target.Remove<Collider2D>();
	}
	SUBCASE("disabled before sync") {
		target.SetEnabled(false);
	}
	SUBCASE("destroyed and ID recycled") {
		target.Destroy();
		f.Box({100.f, 0.f});
	}
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}).value);
	PhysicsQueryOptions2D options;
	World another;
	options.excludeBodies.push_back(another.CreateEntity());
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}, options));
	const auto nan = std::numeric_limits<f32>::quiet_NaN();
	CHECK_FALSE(f.physics.RaycastClosest({nan, 0.f}, {200.f, 0.f}));
	CHECK_FALSE(f.physics.RaycastClosest({}, {nan, 0.f}));
	PhysicsQueryShape2D shape;
	shape.size = {-1.f, 2.f};
	CHECK_FALSE(f.physics.OverlapShape(shape));
	shape.type = ShapeType2D::Capsule;
	shape.radius = 20;
	shape.size.y = 10;
	CHECK_FALSE(f.physics.ShapeCastClosest(shape, {10.f, 0.f}));
	f.physics.Shutdown();
	CHECK(f.physics.RaycastAll({}, {200.f, 0.f}).value.empty());
}

TEST_CASE("ray component shares hit format and respects owner runtime exceptions") {
	QueryFixture f;
	Entity owner = f.Box({50.f, 0.f});
	Entity other = f.Box({100.f, 0.f});
	Entity far = f.Box({150.f, 0.f});
	Entity ray = f.Ray({200.f, 0.f}, owner);
	f.physics.Sync();
	f.physics.Step(f.physics.Config().fixedDelta);
	auto hit = f.physics.RaycastHit(ray);
	REQUIRE(hit);
	CHECK(hit->collider == other);
	CHECK(hit->distance == doctest::Approx(40.f));
	PhysicsQueryOptions2D options;
	options.excludeBodies = {owner};
	auto direct = f.physics.RaycastClosest({50.f, 0.f}, {200.f, 0.f}, options);
	CHECK(hit->point == direct.value->point);
	REQUIRE(f.physics.AddRaycastException(ray, other, true));
	REQUIRE(f.physics.AddRaycastException(ray, other, true));
	CHECK(f.physics.ForceRaycastUpdate(ray).value->collider == far);
	f.physics.RemoveRaycastException(ray, other, true);
	CHECK(f.physics.ForceRaycastUpdate(ray).value->collider == other);
	REQUIRE(f.physics.AddRaycastException(ray, other, false));
	CHECK(f.physics.ForceRaycastUpdate(ray).value->collider == far);
	f.physics.ClearRaycastExceptions(ray);
	CHECK(f.physics.ForceRaycastUpdate(ray).value->collider == other);
	other.Destroy();
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK(f.physics.ForceRaycastUpdate(ray).value->collider == far);
}

TEST_CASE("ray samples current solver ancestry during every catch-up tick") {
	QueryFixture f;
	Entity owner = f.Box({0.f, 0.f}, Vector2(10.f), BodyType2D::Kinematic);
	Entity target = f.Box({100.f, 0.f});
	Entity ray = f.Ray({200.f, 0.f}, owner);
	f.physics.Sync();
	f.physics.SetLinearVelocity(owner, {600.f, 0.f});
	int callbacks = 0;
	// Add a sensor so event dispatch can inspect the cache before write-back.
	Entity sensor = f.Box({20.f, 0.f}, {40.f, 40.f});
	sensor.Get<Collider2D>()->isTrigger = true;
	f.physics.Sync();
	f.physics.Events().append([&](const Collision2D &) {
		++callbacks;
		auto hit = f.physics.RaycastHit(ray);
		REQUIRE(hit);
		const f32 solverX = b2Body_GetPosition(f.physics.BodyId(owner)).x * 100.f;
		CHECK(hit->distance == doctest::Approx(90.f - solverX));
	});
	f.physics.Step(f.physics.Config().fixedDelta * 3.f);
	CHECK(callbacks > 0);
	REQUIRE(f.physics.RaycastHit(ray));
	CHECK(f.physics.RaycastHit(ray)->distance == doctest::Approx(60.f));
	const auto cached = f.physics.RaycastHit(ray);
	ray.Get<Raycast2D>()->targetPosition = {-200.f, 0.f};
	f.physics.Step(0);
	CHECK(f.physics.RaycastHit(ray)->point == cached->point);
	CHECK_FALSE(f.physics.ForceRaycastUpdate(ray).value);
}

TEST_CASE("local ray endpoints inherit rotation nonuniform and negative scale") {
	QueryFixture f;
	Entity parent = f.world.CreateEntity();
	parent.Add(Transform2D{{100.f, 0.f}, 90.f, {-2.f, 3.f}});
	Entity ray = f.Ray({50.f, 0.f}, parent);
	Entity target = f.Box({100.f, -60.f});
	f.physics.Sync();
	auto hit = f.physics.ForceRaycastUpdate(ray);
	REQUIRE(hit.value);
	CHECK(hit.value->point.y == doctest::Approx(-50.f));
	CHECK(hit.value->distance == doctest::Approx(50.f));
	CHECK(hit.value->fraction == doctest::Approx(.5f));
}

TEST_CASE("ray disable manual refresh target edits and session reset") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	Entity ray = f.Ray();
	f.physics.Sync();
	f.physics.Step(f.physics.Config().fixedDelta);
	REQUIRE(f.physics.RaycastHit(ray));
	ray.Get<Raycast2D>()->enabled = false;
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK(f.physics.ForceRaycastUpdate(ray).value.has_value());
	CHECK(f.physics.RaycastHit(ray).has_value());
	// No implicit Sync/Step: a moved target stays at the solver snapshot.
	target.Get<Transform2D>()->position.x = 160.f;
	CHECK(f.physics.ForceRaycastUpdate(ray).value->distance == doctest::Approx(90.f));
	f.physics.Sync();
	f.physics.Step(0);
	CHECK(f.physics.ForceRaycastUpdate(ray).value->distance == doctest::Approx(150.f));
	ray.SetEnabled(false);
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK_FALSE(f.physics.ForceRaycastUpdate(ray).value);
	ray.SetEnabled(true);
	f.physics.ResetSimulation();
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK(f.physics.RaycastAll({}, {200.f, 0.f}).value.empty());
	f.physics.Sync();
	CHECK(f.physics.ForceRaycastUpdate(ray).value.has_value());
	ray.Remove<Raycast2D>();
	CHECK_FALSE(f.physics.RaycastHit(ray));
}

TEST_CASE("read-only queries do not consume force accumulator events or transforms") {
	QueryFixture f;
	Entity body = f.Box({}, Vector2(10.f), BodyType2D::Dynamic);
	f.Box({100.f, 0.f});
	f.physics.Sync();
	f.physics.ApplyForce(body, {100.f, 0.f});
	const auto accumulator = f.physics.Accumulator();
	const auto position = body.Get<Transform2D>()->position;
	const auto eventCount = f.physics.FrameEvents().size();
	for (int i = 0; i < 4; ++i) {
		CHECK(f.physics.RaycastClosest({}, {200.f, 0.f}).value.has_value());
	}
	CHECK(f.physics.Accumulator() == accumulator);
	CHECK(f.physics.FrameEvents().size() == eventCount);
	CHECK(body.Get<Transform2D>()->position == position);
	f.physics.Step(f.physics.Config().fixedDelta);
	CHECK(f.physics.GetLinearVelocity(body).x > 0.f);
}

TEST_CASE("solid and sensor contacts respect bilateral filters and refiltering") {
	QueryFixture f;
	Entity a = f.Box({}, {40.f, 40.f});
	Entity b = f.Box({}, {20.f, 20.f}, BodyType2D::Dynamic);
	SUBCASE("solid") {
	}
	SUBCASE("sensor") {
		a.Get<Collider2D>()->isTrigger = true;
	}
	a.Get<Collider2D>()->collisionLayer = 1;
	a.Get<Collider2D>()->collisionMask = 2;
	b.Get<Collider2D>()->collisionLayer = 2;
	b.Get<Collider2D>()->collisionMask = 0;
	f.physics.Sync();
	f.physics.Step(f.physics.Config().fixedDelta);
	CHECK(f.physics.FrameEvents().empty());
	b.Get<Collider2D>()->collisionMask = 1;
	f.physics.Sync();
	f.physics.Step(f.physics.Config().fixedDelta);
	REQUIRE(f.physics.FrameEvents().size() == 1);
	CHECK(f.physics.FrameEvents()[0].kind ==
		  (a.Get<Collider2D>()->isTrigger ? CollisionKind::TriggerBegin : CollisionKind::Begin));
	b.Get<Collider2D>()->collisionMask = 0;
	f.physics.Sync();
	f.physics.Step(f.physics.Config().fixedDelta);
	REQUIRE(f.physics.FrameEvents().size() == 1);
	CHECK(f.physics.FrameEvents()[0].kind ==
		  (a.Get<Collider2D>()->isTrigger ? CollisionKind::TriggerEnd : CollisionKind::End));
}

TEST_CASE("authored collider and ray settings round trip without runtime state") {
	QueryFixture source;
	ResourceRegistry resources;
	Scene scene = CreateScene(source.world, "Level");
	Entity entity = scene.CreateEntity("Probe");
	entity.Add(Transform2D{});
	Collider2D collider;
	collider.collisionLayer = 0x80000000u;
	collider.collisionMask = 0;
	entity.Add(collider);
	Raycast2D ray;
	ray.targetPosition = {80.f, -20.f};
	ray.includeSensors = true;
	ray.collisionMask = 0x80000000u;
	entity.Add(ray);
	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, resources));
	QueryFixture target;
	Archive loading = Archive::Load(saving.ToString());
	Scene loaded = LoadScene(loading, target.world, resources);
	REQUIRE(loaded.IsValid());
	Entity restored = target.world.Find("Level/Probe");
	REQUIRE(restored.Has<Raycast2D>());
	CHECK(restored.Get<Collider2D>()->collisionLayer == 0x80000000u);
	CHECK(restored.Get<Collider2D>()->collisionMask == 0);
	CHECK(restored.Get<Raycast2D>()->targetPosition == ray.targetPosition);
	CHECK(restored.Get<Raycast2D>()->collisionMask == 0x80000000u);
	CHECK(restored.Get<Raycast2D>()->includeSensors);
	CHECK_FALSE(target.physics.RaycastHit(restored));
	CHECK(editor::ComponentCategory(target.world.FindComponent("Raycast2D")) == "Physics");
}

TEST_CASE("Lua direct and component queries share options hit copies and validation") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	Entity ray = f.Ray();
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	ProjectSettings settings;
	SceneManager scenes(f.world, resources, fs);
	ScriptServer scripts(f.world);
	scripts.BindEngine(input, scenes, resources, settings, nullptr, &f.physics);
	scripts.SetEntity("target", target);
	scripts.SetEntity("probe", ray);
	f.physics.Sync();
	f.physics.Step(f.physics.Config().fixedDelta);
	REQUIRE(scripts.DoString(R"(
		local hit = physics.raycast(Vector2(0, 0), Vector2(200, 0))
		assert(hit.collider == target and hit.body == target)
		assert(math.abs(hit.distance - 90) < .01 and hit.normal.x == -1)
		local hits = physics.raycast_all(Vector2(0, 0), Vector2(200, 0))
		assert(#hits == 1 and hits[1].collider == target)
		assert(physics.raycast(Vector2(0, 0), Vector2(200, 0), {excludeBodies={target}}) == nil)
		local shape = {type='Circle', position=Vector2(0, 0), radius=5}
		assert(physics.shape_cast(shape, Vector2(200, 0)).collider == target)
		assert(#physics.shape_cast_all(shape, Vector2(200, 0)) == 1)
		shape.position = Vector2(100, 0)
		local overlaps = physics.overlap(shape)
		assert(#overlaps == 1 and overlaps[1].collider == target)
		assert(overlaps[1].point == nil and overlaps[1].distance == nil)
		local ray = probe:get('Raycast2D')
		assert(type(ray.force_update) == 'function')
		assert(ray:is_colliding() and ray:hit().collider == target)
		local copy = ray:hit()
		copy.point.x = 999
		assert(math.abs(ray:hit().point.x - 90) < .01)
		ray:add_body_exception(target)
		assert(ray:force_update() == nil)
		ray:clear_exceptions()
		assert(ray:force_update().collider == target)
		ray.enabled = false
		assert(not ray:is_colliding())
		assert(ray:force_update().collider == target)
		ray.collisionMask = physics.layers(32)
		target:get('Collider2D').collisionLayer = physics.layers(32)
		assert(ray.collisionMask == 2147483648)
		assert(physics.ALL_LAYERS == 4294967295)
		assert(physics.layers(1, 32) == 2147483649)
		for _, n in ipairs({-1, 0, 33, 1.5}) do
			assert(not pcall(physics.layers, n))
		end
		assert(not pcall(function() ray.collisionMask = -1 end))
		assert(not pcall(function() target:get('Collider2D').collisionMask = 1.5 end))
		assert(not pcall(function() physics.raycast(Vector2(), Vector2(200, 0), {collisionMask=-1}) end))
		assert(not pcall(function() physics.raycast(Vector2(), Vector2(200, 0), {includeSensors=1}) end))
		assert(not pcall(function() physics.raycast(Vector2(), Vector2(200, 0), 17) end))
		assert(not pcall(function() physics.raycast_all(Vector2(), Vector2(200, 0), false) end))
		assert(not pcall(function() physics.raycast(Vector2(), Vector2(200, 0), {excludeBodies={[2]=target}}) end))
		assert(not pcall(function() physics.overlap({type='Circle', position=Vector2(), radius=1, typo=true}) end))
		assert(not pcall(function() physics.overlap({type='Circle', position=Vector2(), radius=-1}) end))
	)"));
	f.physics.Sync();
	REQUIRE(scripts.DoString("assert(probe:get('Raycast2D'):force_update().collider == target)"));
	f.physics.Shutdown();
	REQUIRE(scripts.DoString("assert(physics.raycast(Vector2(), Vector2(200,0)) == nil)"));
}

TEST_CASE("removed components cannot revive cached hits or runtime exceptions") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	Entity ray = f.Ray();
	f.physics.Sync();
	REQUIRE(f.physics.ForceRaycastUpdate(ray).value);
	target.Remove<Collider2D>();
	target.Add(Collider2D{});
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}).value);
	f.physics.Sync();
	REQUIRE(f.physics.AddRaycastException(ray, target, true));
	CHECK_FALSE(f.physics.ForceRaycastUpdate(ray).value);
	ray.Remove<Raycast2D>();
	Raycast2D data;
	data.targetPosition = {200.f, 0.f};
	ray.Add(data);
	CHECK(f.physics.ForceRaycastUpdate(ray).value.has_value());
	target.Remove<RigidBody2D>();
	RigidBody2D body;
	body.type = BodyType2D::Static;
	target.Add(body);
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK_FALSE(f.physics.RaycastClosest({}, {200.f, 0.f}).value);
	f.physics.Sync();
	CHECK(f.physics.ForceRaycastUpdate(ray).value.has_value());
}

TEST_CASE("older collider scenes retain default filters") {
	QueryFixture f;
	Entity entity = f.world.CreateEntity();
	auto type = f.world.FindComponent("Collider2D");
	Archive loading = Archive::Load("type: Box\nsize: [20, 30]\nisTrigger: false\n");
	REQUIRE(SerializeComponent(loading, entity, type));
	CHECK_FALSE(loading.HasErrors());
	CHECK(entity.Get<Collider2D>()->collisionLayer == 1);
	CHECK(entity.Get<Collider2D>()->collisionMask == AllPhysicsLayers);
}

TEST_CASE("Lua update observes previous snapshot then synchronized changes") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	ProjectSettings settings;
	SceneManager scenes(f.world, resources, fs);
	ScriptServer scripts(f.world);
	scripts.BindEngine(input, scenes, resources, settings, nullptr, &f.physics);
	scripts.SetEntity("target", target);
	f.physics.Sync();
	REQUIRE(scripts.DoString(R"(
		checks = 0
		world:system('QueryTiming'):with('Collider2D'):each(function(e, dt)
			local hit = physics.raycast(Vector2(), Vector2(200, 0))
			assert(math.abs(hit.distance - (checks == 0 and 90 or 150)) < .01)
			checks = checks + 1
		end)
	)"));
	target.Get<Transform2D>()->position.x = 160.f;
	f.world.Progress(f.physics.Config().fixedDelta);
	f.world.Progress(f.physics.Config().fixedDelta);
	REQUIRE(scripts.DoString("assert(checks == 2)"));
}

TEST_CASE("Play Pause Step Stop isolate ray caches from editor snapshots") {
	QueryFixture f;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(f.world, resources, fs);
	scenes.SetPlaySessionCallbacks([&] { f.physics.ResetSimulation(); },
		[&] { f.physics.ResetSimulation(); });
	Scene scene = scenes.Create("Level");
	Entity target = f.Box({100.f, 0.f});
	target.SetName("Target");
	REQUIRE(target.SetParent(scene.Root()));
	Entity ray = f.Ray();
	ray.SetName("Probe");
	REQUIRE(ray.SetParent(scene.Root()));
	REQUIRE(scenes.SetCurrent(scene));
	scenes.PlayCurrent();
	auto frame = [&](f32 dt) {
		scenes.Flush();
		const GameTime time = GetGameTime(scenes.IsPlaying(), scenes.IsPaused(), scenes.TakeStep(),
			dt, f.physics.Config().fixedDelta);
		f.world.SetPlayOnlySystemsEnabled(time.running);
		f.world.Progress(time.delta);
	};
	frame(f.physics.Config().fixedDelta);
	Entity runtimeRay = scenes.Current().Root().Find("Probe");
	REQUIRE(runtimeRay.IsAlive());
	REQUIRE(f.physics.RaycastHit(runtimeRay));
	scenes.Pause();
	runtimeRay.Get<Raycast2D>()->targetPosition = {-200.f, 0.f};
	frame(1.f);
	CHECK(f.physics.RaycastHit(runtimeRay).has_value());
	CHECK_FALSE(f.physics.ForceRaycastUpdate(runtimeRay).value);
	runtimeRay.Get<Raycast2D>()->targetPosition = {200.f, 0.f};
	scenes.Step();
	frame(1.f);
	CHECK(f.physics.RaycastHit(runtimeRay).has_value());
	scenes.Stop();
	frame(0.f);
	CHECK_FALSE(f.physics.RaycastHit(runtimeRay));
	CHECK(f.physics.RaycastAll({}, {200.f, 0.f}).value.empty());
	Entity restored = scenes.EditorCurrent().Root().Find("Probe");
	REQUIRE(restored.Has<Raycast2D>());
	CHECK(restored.Get<Raycast2D>()->targetPosition == Vector2(200.f, 0.f));
	scenes.PlayCurrent();
	frame(f.physics.Config().fixedDelta);
	CHECK(f.physics.RaycastHit(scenes.Current().Root().Find("Probe")).has_value());
}

TEST_CASE("unbound Lua queries still validate malformed geometry") {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	ProjectSettings settings;
	SceneManager scenes(world, resources, fs);
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);
	REQUIRE(scripts.DoString(R"(
		assert(physics.raycast(Vector2(), Vector2(100, 0)) == nil)
		assert(#physics.raycast_all(Vector2(), Vector2(100, 0)) == 0)
		assert(not pcall(function() physics.raycast(Vector2(0/0, 0), Vector2(100,0)) end))
		assert(not pcall(function() physics.overlap({type='Circle', position=Vector2(), radius=-1}) end))
	)"));
}

TEST_CASE(
	"ray geometry recreation clears cached points and touching casts skip initial shapes") {
	QueryFixture f;
	Entity target = f.Box({100.f, 0.f});
	Entity far = f.Box({180.f, 0.f});
	Entity ray = f.Ray();
	f.physics.Sync();
	CHECK(f.physics.RaycastClosest({90.f, 0.f}, {150.f, 0.f}).value->collider == far);
	REQUIRE(f.physics.ForceRaycastUpdate(ray).value);
	target.Get<Collider2D>()->size.x = 40.f;
	f.physics.Sync();
	CHECK_FALSE(f.physics.RaycastHit(ray));
	CHECK(f.physics.ForceRaycastUpdate(ray).value->distance == doctest::Approx(80.f));
}

TEST_CASE("physics overlay draws ray endpoints and cached surface normals") {
	QueryFixture f;
	f.Box({100.f, 0.f});
	Entity ray = f.Ray();
	f.physics.Sync();
	REQUIRE(f.physics.ForceRaycastUpdate(ray).value);
	std::vector<std::pair<Vector2, Vector2>> lines;
	f.physics.DebugDraw([&](Vector2 a, Vector2 b, Color) { lines.emplace_back(a, b); });
	CHECK(std::any_of(lines.begin(), lines.end(), [](const auto &line) {
		return line.first == Vector2(0.f, 0.f) && line.second == Vector2(200.f, 0.f);
	}));
	CHECK(std::any_of(lines.begin(), lines.end(), [](const auto &line) {
		return std::abs(line.first.x - 90.f) < .01f && std::abs(line.second.x - 70.f) < .01f &&
			   line.first.y == 0.f && line.second.y == 0.f;
	}));
}

TEST_CASE("hit identities preserve entity generations without pointer encoding") {
	QueryFixture f;
	Entity previous = f.Box({100.f, 0.f});
	previous.Destroy();
	Entity target = f.Box({100.f, 0.f});
	f.physics.Sync();
	CHECK((target.Raw().id() >> 32) != 0);
	// Backend user data is opaque; even a truncated/foreign value cannot
	// replace the authoritative full-generation shape-to-entity mapping.
	b2Shape_SetUserData(f.physics.ShapeId(target), reinterpret_cast<void *>(uintptr_t{1}));
	const auto hit = f.physics.RaycastClosest({}, {200.f, 0.f});
	REQUIRE(hit.value);
	CHECK(hit.value->collider == target);
	CHECK(hit.value->collider.IsAlive());
}
