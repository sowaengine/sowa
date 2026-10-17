#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "physics/collider2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/lua_debugger.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {
struct FixedCounter {
	int ticks = 0;
};
struct SkipFixed {};
struct Fixture {
	World world;
	PhysicsWorld physics;
	Fixture(PhysicsWorldConfig config = {}) {
		config.gravity = {};
		physics.Init(world, config);
		REQUIRE(world.RegisterComponent<FixedCounter>("FixedCounter").Result());
		REQUIRE(world.RegisterComponent<SkipFixed>("SkipFixed").Result());
	}
	Entity body(BodyType2D type = BodyType2D::Dynamic, Vector2 position = {}) {
		Entity entity = world.CreateEntity();
		entity.Add(Transform2D{position});
		RigidBody2D body;
		body.type = type;
		entity.Add(body);
		Collider2D shape;
		shape.size = {100.f, 100.f}; // Unit mass at the default pixels/metre.
		shape.density = 1.f;
		entity.Add(shape);
		entity.Add(FixedCounter{});
		return entity;
	}
};
} // namespace

TEST_CASE("Fixed gameplay runs only for actual ticks with explicit tick delta and frame ordering") {
	PhysicsWorldConfig config;
	config.fixedDelta = 0.02f;
	config.maxStepsPerFrame = 3;
	Fixture f(config);
	Entity entity = f.body();
	std::vector<int> order;
	int frames = 0;
	f32 frameDelta = 0.f;
	f.world.CreateSystem<FixedCounter>("FrameCounter").Each([&](Entity, FixedCounter &) {
		++frames;
		order.push_back(0);
	});
	f.world.CreateFixedSystem<FixedCounter>("FirstFixed")
		.Each([&](Entity, f32 dt, FixedCounter &counter) {
			CHECK(dt == config.fixedDelta);
			CHECK(f.world.DeltaTime() == frameDelta);
			++counter.ticks;
			order.push_back(1);
		});
	f.world.CreateFixedSystem<FixedCounter>("SecondFixed").Each([&](Entity, f32, FixedCounter &) {
		order.push_back(2);
	});
	f.world.Progress(0.f);
	f.world.Progress(0.01f);
	CHECK(entity.Get<FixedCounter>()->ticks == 0);
	order.clear();
	frameDelta = 0.05f;
	f.world.Progress(frameDelta);
	CHECK(entity.Get<FixedCounter>()->ticks == 3);
	CHECK(order == std::vector<int>{0, 1, 2, 1, 2, 1, 2});
	CHECK(frames == 3);
	order.clear();
	frameDelta = 10.f;
	f.world.Progress(frameDelta);
	CHECK(entity.Get<FixedCounter>()->ticks == 6);
	CHECK(order == std::vector<int>{0, 1, 2, 1, 2, 1, 2});
	CHECK(f.physics.Accumulator() < config.fixedDelta);
}

TEST_CASE(
	"Fixed system filters, handles and pause gating do not enroll systems in the frame pipeline") {
	Fixture f;
	Entity entity = f.body();
	Entity excluded = f.body();
	excluded.Add<SkipFixed>();
	Entity disabled = f.body();
	disabled.SetEnabled(false);
	int untyped = 0;
	System fixed = f.world.CreateFixedSystem<FixedCounter>("FilteredFixed")
					   .Without<SkipFixed>()
					   .Each([](Entity, f32, FixedCounter &c) { ++c.ticks; });
	System global =
		f.world.RegisterUntypedFixedSystem("GlobalFixed", {}, {}, [&](Entity e, f32 dt) {
			CHECK_FALSE(e.IsAlive());
			CHECK(dt > 0.f);
			++untyped;
		});
	const f32 tick = f.physics.Config().fixedDelta;
	fixed.SetEnabled(false);
	f.world.Progress(tick);
	CHECK(entity.Get<FixedCounter>()->ticks == 0);
	fixed.SetEnabled(true);
	f.world.Progress(tick);
	CHECK(entity.Get<FixedCounter>()->ticks == 1);
	CHECK(excluded.Get<FixedCounter>()->ticks == 0);
	CHECK(disabled.Get<FixedCounter>()->ticks == 0);
	CHECK(untyped == 2);
	f.world.SetPlayOnlySystemsEnabled(false);
	f.world.Progress(tick * 4.f);
	f.physics.Step(tick);
	CHECK(untyped == 2);
	f.world.SetPlayOnlySystemsEnabled(true);
	f.world.DestroyTrackedSystem(fixed);
	global.Destroy();
	f.world.Progress(tick);
	CHECK(entity.Get<FixedCounter>()->ticks == 1);
	CHECK(untyped == 2);
}

TEST_CASE("Fixed forces and poses agree across render frame partitions") {
	struct Result {
		Vector2 position;
		Vector2 velocity;
		int ticks;
	};
	auto run = [](int hz) {
		Fixture f;
		Entity entity = f.body();
		Entity parent = f.world.CreateEntity();
		parent.Add(Transform2D{Vector2(200.f, 50.f)});
		REQUIRE(entity.SetParent(parent));
		f.world.CreateFixedSystem<FixedCounter>("Accelerate")
			.Each([&](Entity e, f32, FixedCounter &c) {
				CHECK(e.Get<Transform2D>()->WorldPosition().x ==
					  doctest::Approx(ComputeWorldMatrix(e)[3][0]));
				CHECK(e.Get<Transform2D>()->WorldPosition().x ==
					  doctest::Approx(b2Body_GetPosition(f.physics.BodyId(e)).x *
									  f.physics.Config().pixelsPerMeter));
				f.physics.ApplyForce(e, {600.f, 0.f});
				++c.ticks;
			});
		if (hz > 0) {
			for (int frame = 0; frame < hz; ++frame) {
				f.world.Progress(1.f / hz);
			}
		} else {
			for (int cycle = 0; cycle < 15; ++cycle) {
				for (f32 portion : {0.25f, 0.75f, 2.f, 0.5f, 0.5f}) {
					f.world.Progress(f.physics.Config().fixedDelta * portion);
				}
			}
		}
		return Result{entity.Get<Transform2D>()->WorldPosition(),
					  f.physics.GetLinearVelocity(entity), entity.Get<FixedCounter>()->ticks};
	};
	const Result at60 = run(60);
	REQUIRE(at60.ticks == 60);
	CHECK(at60.velocity.x == doctest::Approx(600.f).epsilon(0.001));
	for (int hz : {30, 144, 0}) {
		const Result other = run(hz);
		CHECK(other.ticks == at60.ticks);
		CHECK(other.position.x == doctest::Approx(at60.position.x).epsilon(0.0001));
		CHECK(other.velocity.x == doctest::Approx(at60.velocity.x).epsilon(0.0001));
	}
}

TEST_CASE(
	"Fixed structural edits commit between ticks and QueueDestroy retains its frame lifetime") {
	Fixture f;
	Entity first = f.body();
	Entity spawned;
	int firstRuns = 0;
	int spawnedRuns = 0;
	f.world.CreateFixedSystem<FixedCounter>("Spawner").Each([&](Entity e, f32, FixedCounter &) {
		if (e == first) {
			++firstRuns;
			if (!spawned.IsAlive()) {
				spawned = f.body();
			}
			e.QueueDestroy();
		} else {
			++spawnedRuns;
			CHECK(b2Body_IsValid(f.physics.BodyId(e)));
		}
	});
	int seenByLaterSystem = 0;
	f.world.CreateFixedSystem<FixedCounter>("LaterSystem").Each([&](Entity e, f32, FixedCounter &) {
		if (e == spawned) {
			++seenByLaterSystem;
		}
	});
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(firstRuns == 2);
	CHECK_FALSE(first.IsAlive());
	CHECK(spawnedRuns == 1);
	CHECK(seenByLaterSystem == 1); // Not eligible during the spawning tick.
}

TEST_CASE("Fixed field and kinematic pose edits affect the upcoming solver tick") {
	Fixture f;
	Entity entity = f.body(BodyType2D::Kinematic);
	f.world.CreateFixedSystem<FixedCounter>("MoveKinematic")
		.Each([&](Entity e, f32, FixedCounter &counter) {
			e.Get<Transform2D>()->position.x += 20.f;
			f.physics.SetLinearVelocity(e, {60.f, 0.f});
			++counter.ticks;
		});
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(entity.Get<Transform2D>()->position.x == doctest::Approx(42.f));
}

TEST_CASE("Commands survive zero ticks and each force torque and impulse is consumed once") {
	auto run = [](bool split) {
		Fixture f;
		Entity entity = f.body();
		f.physics.ApplyForce(entity, {300.f, 0.f});
		f.physics.ApplyForce(entity, {300.f, 0.f});
		f.physics.ApplyTorque(entity, 10000.f);
		f.physics.ApplyImpulse(entity, {20.f, 0.f});
		f.physics.ApplyAngularImpulse(entity, 1000.f);
		const f32 tick = f.physics.Config().fixedDelta;
		f.world.Progress(tick * 0.25f);
		CHECK(f.physics.GetLinearVelocity(entity).x == 0.f);
		if (split) {
			f.world.Progress(tick * 0.75f);
			const f32 angular = f.physics.GetAngularVelocity(entity);
			f.world.Progress(tick);
			CHECK(f.physics.GetAngularVelocity(entity) == doctest::Approx(angular));
		} else {
			f.world.Progress(tick * 1.75f);
		}
		CHECK(f.physics.GetLinearVelocity(entity).x == doctest::Approx(30.f).epsilon(0.001));
		return f.physics.GetAngularVelocity(entity);
	};
	CHECK(run(false) == doctest::Approx(run(true)).epsilon(0.0001));
}

TEST_CASE("Collision commands target the next tick and collision hooks see completed transforms "
		  "and raycasts") {
	for (bool sameFrame : {false, true}) {
		Fixture f;
		Entity moving = f.body();
		Entity trigger = f.body(BodyType2D::Static);
		trigger.Get<Collider2D>()->isTrigger = true;
		Entity ray = f.world.CreateEntity();
		ray.Add(Transform2D{Vector2(-200.f, 0.f)});
		Raycast2D raycast;
		raycast.targetPosition = {400.f, 0.f};
		ray.Add(raycast);
		f.physics.Sync();
		f.physics.SetLinearVelocity(moving, {60.f, 0.f});
		int events = 0;
		f.physics.Events().append([&](const Collision2D &event) {
			if (event.kind != CollisionKind::TriggerBegin) {
				return;
			}
			++events;
			CHECK(moving.Get<Transform2D>()->WorldPosition().x == doctest::Approx(1.f));
			REQUIRE(f.physics.RaycastHit(ray).has_value());
			f.physics.ApplyForce(moving, {600.f, 0.f});
		});
		const f32 tick = f.physics.Config().fixedDelta;
		if (sameFrame) {
			f.world.Progress(tick * 2.f);
		} else {
			f.world.Progress(tick);
			f.world.Progress(tick);
		}
		CHECK(events == 1);
		CHECK(f.physics.GetLinearVelocity(moving).x == doctest::Approx(70.f));
		f.world.Progress(tick);
		CHECK(f.physics.GetLinearVelocity(moving).x == doctest::Approx(70.f));
	}
}

TEST_CASE("Lua fixed hooks start once, coexist with frame hooks and initialize catch-up spawns") {
	Fixture f;
	ScriptServer scripts(f.world);
	REQUIRE(scripts.DoString(R"(
		starts, frames, fixed, exits = 0, 0, 0, 0
		local M = world:component('FixedLua', { age = 0 })
		function M:_start() starts = starts + 1; self.age = 10 end
		function M:_update(dt) frames = frames + 1 end
		function M:_physics_update(dt)
			assert(self.age >= 10)
			assert(math.abs(dt - 1/60) < 1e-6)
			self.age = self.age + 1
			fixed = fixed + 1
			if fixed == 1 then spawned = world:create_entity('Spawn'); spawned:add('FixedLua') end
		end
		function M:_exit() exits = exits + 1 end
		e = world:create_entity('Original'); e:add('FixedLua')
	)"));
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(scripts.DoString("assert(starts == 2 and frames == 1 and fixed == 3)"));
	f.world.Progress(0.f);
	CHECK(scripts.DoString("assert(starts == 2 and frames == 3 and fixed == 3)"));
	REQUIRE(
		scripts.DoString("e:remove('FixedLua'); spawned:remove('FixedLua'); assert(exits == 2)"));
}

TEST_CASE("A fixed-only Lua component errors without cancelling other callbacks or physics") {
	Fixture f;
	ScriptServer scripts(f.world);
	Entity body = f.body();
	REQUIRE(scripts.DoString(R"(
		local M = world:component('FaultyFixed', { ticks = 0 })
		function M:_physics_update(dt) self.ticks = self.ticks + 1; error('ordinary fixed error') end
		e = world:create_entity(); e:add('FaultyFixed')
	)"));
	int native = 0;
	f.world.CreateFixedSystem<FixedCounter>("AfterLua").Each([&](Entity e, f32, FixedCounter &) {
		++native;
		f.physics.SetLinearVelocity(e, {60.f, 0.f});
	});
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(native == 2);
	CHECK(body.Get<Transform2D>()->position.x == doctest::Approx(2.f));
	CHECK(scripts.DoString("assert(e:get('FaultyFixed').ticks == 2)"));
}

TEST_CASE(
	"Fixed Lua debugger Continue never duplicates a hook and Abort cancels unexecuted ticks") {
	for (bool abort : {false, true}) {
		Fixture f;
		ScriptServer scripts(f.world);
		Entity body = f.body();
		f.physics.SetLinearVelocity(body, {60.f, 0.f});
		REQUIRE(scripts.DoString("local M = world:component('DebugFixed', {})\n"
								 "count = 0\n"
								 "function M:_physics_update(dt)\n"
								 "  count = count + 1\n"
								 "end\n"
								 "e = world:create_entity(); e:add('DebugFixed')\n",
								 "@res://fixed_debug.lua"));
		scripts.Debugger().SetBreakpoints("res://fixed_debug.lua", {LuaBreakpoint{4}});
		int stops = 0;
		scripts.Debugger().SetStopHandler([&](const LuaStopEvent &) {
			++stops;
			CHECK(f.physics.GetLinearVelocity(body).x == doctest::Approx(60.f));
			return abort ? LuaResumeAction::Abort : LuaResumeAction::Continue;
		});
		int later = 0;
		f.world.CreateFixedSystem<FixedCounter>("AfterDebug")
			.Each([&](Entity, f32, FixedCounter &) { ++later; });
		f.world.Progress(f.physics.Config().fixedDelta * 2.f);
		CHECK(stops == (abort ? 1 : 2));
		CHECK(later == (abort ? 0 : 2));
		CHECK(body.Get<Transform2D>()->position.x == doctest::Approx(abort ? 0.f : 2.f));
		CHECK(scripts.DoString(abort ? "assert(count == 0)" : "assert(count == 2)"));
	}
}

TEST_CASE("A Lua error resembling debugger Abort does not cancel physics") {
	Fixture f;
	ScriptServer scripts(f.world);
	Entity body = f.body();
	f.physics.SetLinearVelocity(body, {60.f, 0.f});
	REQUIRE(scripts.DoString(R"(
		local M = world:component('SimilarError', {})
		function M:_physics_update(dt) error('execution stopped by the debugger') end
		e = world:create_entity(); e:add('SimilarError')
	)"));
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK_FALSE(f.world.FixedTicksCancelled());
	CHECK(body.Get<Transform2D>()->position.x == doctest::Approx(2.f));
}

TEST_CASE("A fixed hook cannot hide debugger Abort by catching it in Lua") {
	Fixture f;
	ScriptServer scripts(f.world);
	Entity body = f.body();
	f.physics.SetLinearVelocity(body, {60.f, 0.f});
	REQUIRE(scripts.DoString("local M = world:component('CaughtAbort', {})\n"
							 "function M:_physics_update(dt)\n"
							 "  pcall(function()\n"
							 "    local n = 1\n"
							 "  end)\n"
							 "end\n"
							 "e = world:create_entity(); e:add('CaughtAbort')\n",
							 "@res://caught_abort.lua"));
	scripts.Debugger().SetBreakpoints("res://caught_abort.lua", {LuaBreakpoint{4}});
	scripts.Debugger().SetStopHandler([](const LuaStopEvent &) { return LuaResumeAction::Abort; });
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(f.world.FixedTicksCancelled());
	CHECK(body.Get<Transform2D>()->position.x == 0.f);
}

TEST_CASE("Fixed synchronization keeps collider identity while bodies rotate") {
	Fixture f;
	Entity body = f.body();
	Entity child = f.world.CreateEntity();
	child.Add(Transform2D{Vector2(70.f, 0.f)});
	REQUIRE(child.SetParent(body));
	child.Add(Collider2D{});
	f.physics.Sync();
	const b2ShapeId ownShape = f.physics.ShapeId(body);
	const b2ShapeId childShape = f.physics.ShapeId(child);
	f.physics.SetAngularVelocity(body, 90.f);
	for (int n = 0; n < 90; ++n) {
		f.world.Progress(f.physics.Config().fixedDelta);
		CHECK(b2StoreShapeId(f.physics.ShapeId(body)) == b2StoreShapeId(ownShape));
		CHECK(b2StoreShapeId(f.physics.ShapeId(child)) == b2StoreShapeId(childShape));
	}
}

TEST_CASE("Lua immediate additions become physics bodies only at the next tick boundary") {
	Fixture f;
	ScriptServer scripts(f.world);
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes(f.world, resources, fs);
	ProjectSettings settings;
	scripts.BindEngine(input, scenes, resources, settings, nullptr, &f.physics);
	REQUIRE(scripts.DoString(R"(
		local M = world:component('MakePhysics', {})
		function M:_physics_update(dt)
			if spawned == nil then
				spawned = world:create_entity('PhysicsSpawn')
				spawned:add('Transform2D')
				spawned:add('RigidBody2D')
				spawned:add('Collider2D')
				spawned:get('RigidBody2D'):apply_impulse(Vector2(100, 0))
			end
		end
		e = world:create_entity(); e:add('MakePhysics')
	)"));
	const f32 tick = f.physics.Config().fixedDelta;
	f.world.Progress(tick);
	Entity spawned = f.world.Find("PhysicsSpawn");
	REQUIRE(spawned.IsAlive());
	CHECK_FALSE(b2Body_IsValid(f.physics.BodyId(spawned)));
	f.world.Progress(tick);
	REQUIRE(b2Body_IsValid(f.physics.BodyId(spawned)));
	CHECK(spawned.Get<Transform2D>()->position.x > 0.f);
	CHECK(f.physics.GetLinearVelocity(spawned).x == doctest::Approx(100.f));
}

TEST_CASE("Fixed deferred shape removal preserves End identities across the last tick and "
		  "zero-tick frames") {
	Fixture f;
	Entity moving = f.body();
	Entity trigger = f.body(BodyType2D::Static);
	trigger.Get<Collider2D>()->isTrigger = true;
	const f32 tick = f.physics.Config().fixedDelta;
	f.world.Progress(tick);
	REQUIRE_FALSE(f.physics.FrameEvents().empty());
	bool removed = false;
	f.world.CreateFixedSystem<FixedCounter>("RemoveCollider")
		.Each([&](Entity e, f32, FixedCounter &) {
			if (e == moving && !removed) {
				e.Remove<Collider2D>();
				removed = true;
			}
		});
	f.world.Progress(tick);
	CHECK_FALSE(moving.Has<Collider2D>());
	f.world.Progress(0.f);
	f.world.Progress(tick);
	int ends = 0;
	for (const Collision2D &event : f.physics.FrameEvents()) {
		if (event.kind == CollisionKind::TriggerEnd) {
			++ends;
			CHECK(((event.colliderA == trigger && event.colliderB == moving) ||
				   (event.colliderA == moving && event.colliderB == trigger)));
		}
	}
	CHECK(ends == 1);
}

TEST_CASE("Lua fixed hooks never run in a zero-tick frame after a completed tick") {
	Fixture f;
	ScriptServer scripts(f.world);
	REQUIRE(scripts.DoString(R"(
		count = 0
		local M = world:component('OnlyFixed', {})
		function M:_physics_update(dt) count = count + 1 end
		e = world:create_entity(); e:add('OnlyFixed')
	)"));
	const f32 tick = f.physics.Config().fixedDelta;
	f.world.Progress(tick);
	REQUIRE(scripts.Lua()["count"].get<int>() == 1);
	// Inspect directly: DoString would re-register hooks and hide accidental
	// frame-pipeline execution by replacing their old membership snapshots.
	f.world.Progress(tick * 0.25f);
	CHECK(scripts.Lua()["count"].get<int>() == 1);
	f.world.Progress(tick * 0.75f);
	CHECK(scripts.Lua()["count"].get<int>() == 2);
}

TEST_CASE("A Lua collider addition to an existing body waits until the next tick") {
	Fixture f;
	ScriptServer scripts(f.world);
	Entity body = f.body();
	body.Remove<Collider2D>();
	f.physics.Sync();
	scripts.SetEntity("target", body);
	REQUIRE(scripts.DoString(R"(
		local M = world:component('MakeShape', {})
		function M:_physics_update(dt)
			if not target:has('Collider2D') then target:add('Collider2D') end
		end
		e = world:create_entity(); e:add('MakeShape')
	)"));
	const f32 tick = f.physics.Config().fixedDelta;
	f.world.Progress(tick);
	CHECK(body.Has<Collider2D>());
	CHECK_FALSE(b2Shape_IsValid(f.physics.ShapeId(body)));
	f.world.Progress(tick);
	CHECK(b2Shape_IsValid(f.physics.ShapeId(body)));
}

TEST_CASE("Parented physics bodies write back independent solver poses in parent-first order") {
	for (bool childFirst : {false, true}) {
		Fixture f;
		Entity parent;
		Entity child;
		if (childFirst) {
			child = f.body(BodyType2D::Kinematic, {100.f, 0.f});
			parent = f.body(BodyType2D::Kinematic);
		} else {
			parent = f.body(BodyType2D::Kinematic);
			child = f.body(BodyType2D::Kinematic, {100.f, 0.f});
		}
		REQUIRE(child.SetParent(parent));
		f.physics.SetLinearVelocity(parent, {60.f, 0.f});
		const f32 tick = f.physics.Config().fixedDelta;
		for (int frame = 0; frame < 5; ++frame) {
			f.world.Progress(tick * 2.f);
			const auto solverChild = b2Body_GetPosition(f.physics.BodyId(child));
			CHECK(child.Get<Transform2D>()->WorldPosition().x ==
				  doctest::Approx(solverChild.x * f.physics.Config().pixelsPerMeter));
			CHECK(child.Get<Transform2D>()->WorldPosition().x == doctest::Approx(100.f));
		}
	}
}

TEST_CASE("Manual paused physics stepping preserves the clock and pending commands") {
	Fixture f;
	Entity body = f.body();
	const f32 tick = f.physics.Config().fixedDelta;
	f.world.Progress(tick * 0.25f);
	const f32 remainder = f.physics.Accumulator();
	f.physics.ApplyImpulse(body, {100.f, 0.f});
	f.world.SetPlayOnlySystemsEnabled(false);
	for (int n = 0; n < 10; ++n) {
		f.physics.Step(tick * 3.f);
	}
	CHECK(f.physics.Accumulator() == doctest::Approx(remainder));
	CHECK(f.physics.GetLinearVelocity(body).x == 0.f);
	f.world.SetPlayOnlySystemsEnabled(true);
	int fixed = 0;
	f.world.CreateFixedSystem<FixedCounter>("ManualResume").Each([&](Entity, f32, FixedCounter &) { ++fixed; });
	f.world.Progress(tick);
	CHECK(fixed == 1);
	CHECK(f.physics.Accumulator() == doctest::Approx(remainder));
	CHECK(f.physics.GetLinearVelocity(body).x == doctest::Approx(100.f));
}

TEST_CASE("The fixed runner rejects paused work before snapshot preparation") {
	Fixture f;
	f.body();
	int fixed = 0;
	int preparation = 0;
	int simulation = 0;
	f.world.CreateFixedSystem<FixedCounter>("DirectPausedRunner").Each([&](Entity, f32, FixedCounter &) { ++fixed; });
	f.world.SetPlayOnlySystemsEnabled(false);
	CHECK_FALSE(f.world.RunFixedTick(f.physics.Config().fixedDelta, [&] { ++preparation; }, [&] { ++simulation; }));
	CHECK(fixed == 0);
	CHECK(preparation == 0);
	CHECK(simulation == 0);
}

TEST_CASE("Fixed gameplay stays synchronized through long jittered frames and capped hitches") {
	for (f32 tick : {1.f / 30.f, 1.f / 60.f, 1.f / 120.f}) {
		for (int cap : {1, 2, 8}) {
			PhysicsWorldConfig config;
			config.fixedDelta = tick;
			config.maxStepsPerFrame = cap;
			Fixture f(config);
			Entity body = f.body();
			int callbacks = 0;
			f.world.CreateFixedSystem<FixedCounter>("JitteredThrust").Each([&](Entity e, f32 dt, FixedCounter &) {
				CHECK(dt == tick);
				++callbacks;
				f.physics.ApplyForce(e, {60.f, 0.f});
			});
			u32 random = 0x94b4u;
			for (int frame = 0; frame < 1000; ++frame) {
				random = random * 1664525u + 1013904223u;
				const f32 delta = frame % 7 == 0 ? 0.f : (frame % 31 == 0 ? tick * 20.f : tick * (random % 4000) / 1000.f);
				const int before = callbacks;
				f.world.Progress(delta);
				CHECK(callbacks - before <= cap);
				CHECK(f.physics.Accumulator() >= 0.f);
				CHECK(f.physics.Accumulator() < tick);
				if (delta == 0.f) {
					CHECK(callbacks == before);
				}
			}
			REQUIRE(callbacks > 500);
			CHECK(f.physics.GetLinearVelocity(body).x ==
				  doctest::Approx(60.f * tick * callbacks).epsilon(0.001));
			CHECK(body.Get<Transform2D>()->WorldPosition().x == doctest::Approx(
																	b2Body_GetPosition(f.physics.BodyId(body)).x * config.pixelsPerMeter));
		}
	}
}

TEST_CASE("Native fixed callbacks with no component terms run once per tick") {
	Fixture f;
	int runs = 0;
	f.world.CreateFixedSystem<>("NativeGlobalFixed").Each([&](Entity entity, f32 dt) {
		CHECK_FALSE(entity.IsAlive());
		CHECK(dt == f.physics.Config().fixedDelta);
		++runs;
	});
	f.body();
	f.body();
	f.world.Progress(f.physics.Config().fixedDelta * 3.f);
	CHECK(runs == 3);
	f.world.Progress(0.f);
	CHECK(runs == 3);
}

TEST_CASE("Fixed callbacks may register systems that join the following catch-up tick") {
	Fixture f;
	f.body();
	int early = 0;
	int late = 0;
	f.world.CreateFixedSystem<FixedCounter>("RegisterDuringFixed").Each([&](Entity, f32, FixedCounter &) {
		if (++early == 1) {
			f.world.CreateFixedSystem<FixedCounter>("RegisteredDuringFixed").Each([&](Entity, f32, FixedCounter &) { ++late; });
		}
	});
	f.world.Progress(f.physics.Config().fixedDelta * 3.f);
	CHECK(early == 3);
	CHECK(late == 2);
}

TEST_CASE("A fixed system can destroy itself without running later catch-up ticks") {
	Fixture f;
	f.body();
	int runs = 0;
	System self;
	self = f.world.CreateFixedSystem<FixedCounter>("DestroySelfFixed").Each([&](Entity, f32, FixedCounter &) {
		++runs;
		f.world.DestroyTrackedSystem(self);
	});
	f.world.Progress(f.physics.Config().fixedDelta * 3.f);
	CHECK(runs == 1);
	CHECK_FALSE(self.IsValid());
	f.world.Progress(f.physics.Config().fixedDelta);
	CHECK(runs == 1);
}

TEST_CASE("Lua fixed callbacks can add components to their own instances exactly once per tick") {
	Fixture f;
	ScriptServer scripts(f.world);
	REQUIRE(scripts.DoString(R"(
		local Added = world:component('FixedAdded', { value = 17 })
		local M = world:component('SelfAddingFixed', { ticks = 0 })
		function M:_physics_update(dt)
			self.ticks = self.ticks + 1
			local added = self.entity:add('FixedAdded')
			assert(added.value == 17)
		end
		for n = 1, 32 do
			local e = world:create_entity('SelfAdding' .. n)
			e:add('SelfAddingFixed')
		end
	)"));
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	const auto type = f.world.FindComponent("SelfAddingFixed");
	int visited = 0;
	f.world.EachWithType(type, false, [&](Entity entity) {
		++visited;
		f32 ticks = 0.f;
		REQUIRE(entity.GetField(type, "ticks").Get(ticks));
		CHECK(ticks == 2.f);
		CHECK(entity.Has(f.world.FindComponent("FixedAdded")));
	});
	CHECK(visited == 32);
}

TEST_CASE("Nested independently simulated bodies retain world poses under moving rotating ancestors") {
	for (BodyType2D type : {BodyType2D::Dynamic, BodyType2D::Kinematic}) {
		for (bool childFirst : {false, true}) {
			Fixture f;
			Entity parent;
			Entity child;
			if (childFirst) {
				child = f.body(type, {150.f, 70.f});
				parent = f.body(BodyType2D::Kinematic, {2000.f, 1000.f});
			} else {
				parent = f.body(BodyType2D::Kinematic, {2000.f, 1000.f});
				child = f.body(type, {150.f, 70.f});
			}
			Entity middle = f.world.CreateEntity();
			middle.Add(Transform2D{{20.f, 30.f}, 25.f, {1.25f, 1.25f}});
			REQUIRE(middle.SetParent(parent));
			REQUIRE(child.SetParent(middle));
			f.physics.Sync();
			const b2Vec2 initial = b2Body_GetPosition(f.physics.BodyId(child));
			f.physics.SetLinearVelocity(parent, {60.f, 20.f});
			f.physics.SetAngularVelocity(parent, 90.f);
			for (int frame = 0; frame < 10; ++frame) {
				f.world.Progress(f.physics.Config().fixedDelta * 2.f);
				const auto solver = b2Body_GetPosition(f.physics.BodyId(child));
				const auto *pose = child.Get<Transform2D>();
				CHECK(pose->WorldPosition().x == doctest::Approx(solver.x * 100.f).epsilon(1e-5));
				CHECK(pose->WorldPosition().y == doctest::Approx(solver.y * 100.f).epsilon(1e-5));
				CHECK(solver.x == doctest::Approx(initial.x).epsilon(1e-5));
				CHECK(solver.y == doctest::Approx(initial.y).epsilon(1e-5));
			}
		}
	}
}

TEST_CASE("Fixed system eligibility is captured before callbacks enable other systems") {
	Fixture f;
	f.body();
	System typed;
	System nativeGlobal;
	System untypedGlobal;
	int typedRuns = 0;
	int nativeRuns = 0;
	int untypedRuns = 0;
	f.world.RegisterUntypedFixedSystem("EnableOtherFixed", {}, {}, [&](Entity, f32) {
		f.world.SuspendDefer();
		typed.SetEnabled(true);
		nativeGlobal.SetEnabled(true);
		untypedGlobal.SetEnabled(true);
		f.world.ResumeDefer();
	});
	typed = f.world.CreateFixedSystem<FixedCounter>("EnabledTyped").Each([&](Entity, f32, FixedCounter &) { ++typedRuns; });
	nativeGlobal = f.world.CreateFixedSystem<>("EnabledNativeGlobal").Each([&](Entity, f32) { ++nativeRuns; });
	untypedGlobal = f.world.RegisterUntypedFixedSystem("EnabledUntypedGlobal", {}, {},
													   [&](Entity, f32) { ++untypedRuns; });
	for (System system : {typed, nativeGlobal, untypedGlobal}) {
		system.SetEnabled(false);
	}
	f.world.Progress(f.physics.Config().fixedDelta);
	CHECK(typedRuns == 0);
	CHECK(nativeRuns == 0);
	CHECK(untypedRuns == 0);
	f.world.Progress(f.physics.Config().fixedDelta);
	CHECK(typedRuns == 1);
	CHECK(nativeRuns == 1);
	CHECK(untypedRuns == 1);
}

TEST_CASE("Untyped fixed snapshots revalidate removal disabling and filters before dispatch") {
	Fixture f;
	std::vector<Entity> entities;
	for (int n = 0; n < 8; ++n) {
		entities.push_back(f.body());
	}
	entities[3].Add<SkipFixed>();
	entities[4].SetEnabled(false);
	entities[5].Remove<FixedCounter>();
	bool changed = false;
	f.world.RegisterUntypedFixedSystem("ChangeEligibility", {}, {}, [&](Entity, f32) {
		if (changed) {
			return;
		}
		changed = true;
		f.world.SuspendDefer();
		entities[0].Remove<FixedCounter>();
		entities[1].SetEnabled(false);
		entities[2].Add<SkipFixed>();
		entities[3].Remove<SkipFixed>();
		entities[4].SetEnabled(true);
		entities[5].Add<FixedCounter>();
		f.world.ResumeDefer();
	});
	std::vector<Entity> seen;
	f.world.RegisterUntypedFixedSystem("RevalidatedFixed", {f.world.FindComponent("FixedCounter")},
									   {f.world.FindComponent("SkipFixed")}, [&](Entity e, f32) { seen.push_back(e); });
	f.world.Progress(f.physics.Config().fixedDelta);
	REQUIRE(seen.size() == 2);
	CHECK((seen[0] == entities[6] || seen[0] == entities[7]));
	CHECK((seen[1] == entities[6] || seen[1] == entities[7]));
	CHECK(seen[0] != seen[1]);
	seen.clear();
	f.world.Progress(f.physics.Config().fixedDelta);
	CHECK(seen.size() == 5);
}

TEST_CASE("A fixed-only Lua start that adds a component still runs once per instance") {
	Fixture f;
	ScriptServer scripts(f.world);
	REQUIRE(scripts.DoString(R"(
		starts, fixed = 0, 0
		world:component('StartAdded', { value = 42 })
		local M = world:component('StartingFixed', {})
		function M:_start()
			starts = starts + 1
			self.entity:add('StartAdded')
		end
		function M:_physics_update(dt)
			assert(self.entity:get('StartAdded').value == 42)
			fixed = fixed + 1
		end
		for n = 1, 32 do
			world:create_entity('Starting' .. n):add('StartingFixed')
		end
	)"));
	f.world.Progress(0.f);
	CHECK(scripts.Lua()["starts"].get<int>() == 32);
	CHECK(scripts.Lua()["fixed"].get<int>() == 0);
	f.world.Progress(f.physics.Config().fixedDelta);
	CHECK(scripts.Lua()["starts"].get<int>() == 32);
	CHECK(scripts.Lua()["fixed"].get<int>() == 32);
	f.world.Progress(f.physics.Config().fixedDelta);
	CHECK(scripts.Lua()["starts"].get<int>() == 32);
	CHECK(scripts.Lua()["fixed"].get<int>() == 64);
}

TEST_CASE("Rotating child colliders preserve shape identity across full turns and world offsets") {
	for (Vector2 origin : {Vector2{}, Vector2{2000.f, 1000.f}}) {
		Fixture f;
		Entity body = f.body(BodyType2D::Kinematic, origin);
		Entity child = f.world.CreateEntity();
		child.Add(Transform2D{{70.f, 30.f}, 45.f, {1.f, 1.f}});
		REQUIRE(child.SetParent(body));
		child.Add(Collider2D{});
		f.physics.Sync();
		const auto shape = b2StoreShapeId(f.physics.ShapeId(child));
		f.physics.SetAngularVelocity(body, 180.f);
		for (int n = 0; n < 240; ++n) {
			f.world.Progress(f.physics.Config().fixedDelta);
			CAPTURE(origin.x);
			CAPTURE(origin.y);
			CAPTURE(n);
			CHECK(b2StoreShapeId(f.physics.ShapeId(child)) == shape);
		}
	}
}

TEST_CASE("A Lua instance removed during start can be added and started again") {
	for (bool spawnDuringFixed : {false, true}) {
		Fixture f;
		ScriptServer scripts(f.world);
		REQUIRE(scripts.DoString(R"(
			starts, fixed, exits = 0, 0, 0
			local M = world:component('StartRemovingFixed', {})
			function M:_start()
				starts = starts + 1
				if starts == 1 then self.entity:remove('StartRemovingFixed') end
			end
			function M:_physics_update(dt) fixed = fixed + 1 end
			function M:_exit() exits = exits + 1 end
			local Spawner = world:component('StartRemovalSpawner', {})
			function Spawner:_physics_update(dt)
				if e == nil then
					e = world:create_entity('StartRemovalTarget')
					e:add('StartRemovingFixed')
				end
			end
		)"));
		REQUIRE(scripts.DoString(spawnDuringFixed ? "world:create_entity():add('StartRemovalSpawner')" : "e = world:create_entity('StartRemovalTarget'); e:add('StartRemovingFixed')"));
		f.world.Progress(f.physics.Config().fixedDelta * 2.f);
		CHECK(scripts.Lua()["starts"].get<int>() == 1);
		CHECK(scripts.Lua()["exits"].get<int>() == 1);
		CHECK(scripts.Lua()["fixed"].get<int>() == (spawnDuringFixed ? 1 : 0));
		REQUIRE(scripts.DoString("e:add('StartRemovingFixed')"));
		f.world.Progress(f.physics.Config().fixedDelta);
		CHECK(scripts.Lua()["starts"].get<int>() == 2);
		CHECK(scripts.Lua()["exits"].get<int>() == 1);
		CHECK(scripts.Lua()["fixed"].get<int>() == (spawnDuringFixed ? 2 : 1));
	}
}

TEST_CASE("Global fixed callbacks with filters release their query iterators") {
	Fixture f;
	f.body();
	int native = 0;
	int untyped = 0;
	f.world.CreateFixedSystem<>("NativeFilteredGlobal").Without<SkipFixed>().Each(
		[&](Entity entity, f32) {
			CHECK_FALSE(entity.IsAlive());
			++native;
		});
	f.world.RegisterUntypedFixedSystem("UntypedFilteredGlobal", {},
		{f.world.FindComponent("SkipFixed")}, [&](Entity entity, f32) {
			CHECK_FALSE(entity.IsAlive());
			++untyped;
		});
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(native == 2);
	CHECK(untyped == 2);
}

TEST_CASE("Debugger Abort during start skips later instances and fixed simulation") {
	Fixture f;
	ScriptServer scripts(f.world);
	Entity body = f.body();
	f.physics.SetLinearVelocity(body, {60.f, 0.f});
	REQUIRE(scripts.DoString(
		"local M = world:component('AbortingStartFixed', {})\n"
		"starts, fixed = 0, 0\n"
		"function M:_start()\n"
		"  starts = starts + 1\n"
		"end\n"
		"function M:_physics_update(dt) fixed = fixed + 1 end\n"
		"for n = 1, 32 do world:create_entity():add('AbortingStartFixed') end\n",
		"@res://aborting_start_fixed.lua"));
	scripts.Debugger().SetBreakpoints("res://aborting_start_fixed.lua", {LuaBreakpoint{4}});
	int stops = 0;
	scripts.Debugger().SetStopHandler([&](const LuaStopEvent &) {
		++stops;
		return LuaResumeAction::Abort;
	});
	f.world.Progress(f.physics.Config().fixedDelta * 2.f);
	CHECK(stops == 1);
	CHECK(f.world.FixedTicksCancelled());
	CHECK(scripts.Lua()["starts"].get<int>() == 0);
	CHECK(scripts.Lua()["fixed"].get<int>() == 0);
	CHECK(body.Get<Transform2D>()->WorldPosition().x == 0.f);
}

TEST_CASE("Instances created by an earlier fixed system wait a tick in Lua and native systems") {
	Fixture f;
	ScriptServer scripts(f.world);
	bool spawned = false;
	f.world.RegisterUntypedFixedSystem("SpawnBeforeOtherFixed", {}, {}, [&](Entity, f32) {
		if (spawned) { return; }
		spawned = true;
		sol::protected_function spawn = scripts.Lua()["spawnFixedTarget"];
		REQUIRE(spawn().valid());
	});
	REQUIRE(scripts.DoString(R"(
		local M = world:component('CrossSystemFixed', { ticks = 0 })
		function M:_physics_update(dt) self.ticks = self.ticks + 1 end
		function spawnFixedTarget()
			target = world:create_entity('CrossSystemTarget')
			target:add('CrossSystemFixed')
			target:add('FixedCounter')
		end
	)"));
	int native = 0;
	f.world.CreateFixedSystem<FixedCounter>("NativeAfterSpawn").Each(
		[&](Entity, f32, FixedCounter &) { ++native; });
	f.world.Progress(f.physics.Config().fixedDelta);
	Entity target = f.world.Find("CrossSystemTarget");
	REQUIRE(target.IsAlive());
	f32 ticks = -1.f;
	REQUIRE(target.GetField(f.world.FindComponent("CrossSystemFixed"), "ticks").Get(ticks));
	CHECK(ticks == 0.f);
	CHECK(native == 0);
	f.world.Progress(f.physics.Config().fixedDelta);
	REQUIRE(target.GetField(f.world.FindComponent("CrossSystemFixed"), "ticks").Get(ticks));
	CHECK(ticks == 1.f);
	CHECK(native == 1);
}

TEST_CASE("Collision debugger Continue and Abort preserve completed ticks and skip remaining work") {
	for (bool abort : {false, true}) {
		Fixture f;
		ScriptServer scripts(f.world);
		Input input;
		FileSystem fs;
		ResourceRegistry resources;
		SceneManager scenes(f.world, resources, fs);
		ProjectSettings settings;
		scripts.BindEngine(input, scenes, resources, settings, nullptr, &f.physics);
		Entity moving = f.body();
		Entity trigger = f.body(BodyType2D::Static);
		trigger.Get<Collider2D>()->isTrigger = true;
		f.physics.SetLinearVelocity(moving, {60.f, 0.f});
		REQUIRE(scripts.DoString(
			"local M = world:component('CollisionDebugFixed', {})\n"
			"collisionCalls = 0\n"
			"function M:_trigger_begin(other)\n"
			"  collisionCalls = collisionCalls + 1\n"
			"end\n"
			"local N = world:component('CollisionDebugSecond', {})\n"
			"N._trigger_begin = M._trigger_begin\n",
			"@res://fixed_collision_debug.lua"));
		for (Entity e : {moving, trigger}) {
			e.Add(f.world.FindComponent("CollisionDebugFixed"));
			e.Add(f.world.FindComponent("CollisionDebugSecond"));
		}
		scripts.Debugger().SetBreakpoints("res://fixed_collision_debug.lua", {LuaBreakpoint{4}});
		int stops = 0;
		scripts.Debugger().SetStopHandler([&](const LuaStopEvent &) {
			++stops;
			CHECK(moving.Get<Transform2D>()->WorldPosition().x == doctest::Approx(1.f));
			return abort ? LuaResumeAction::Abort : LuaResumeAction::Continue;
		});
		int ticks = 0;
		f.world.RegisterUntypedFixedSystem("CountCollisionTicks", {}, {},
			[&](Entity, f32) { ++ticks; });
		f.world.Progress(f.physics.Config().fixedDelta * 2.f);
		CHECK(stops == (abort ? 1 : 4));
		CHECK(ticks == (abort ? 1 : 2));
		CHECK(moving.Get<Transform2D>()->WorldPosition().x == doctest::Approx(abort ? 1.f : 2.f));
		CHECK(scripts.Lua()["collisionCalls"].get<int>() == (abort ? 0 : 4));
		CHECK(f.world.FixedTicksCancelled() == abort);
	}
}
