#include <string>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/game_time.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"
#include "particles/particles2d.hpp"
#include "physics/collider2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/rigid_body2d.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {

struct Ticker {
	i32 runs = 0;
};

struct Fixture {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes;
	ScriptServer scripts;
	PhysicsWorld physics;

	Fixture() : scenes(world, resources, fs), scripts(world) {
		physics.Init(world);
		REQUIRE(world.RegisterComponent<Ticker>("Ticker").Result());
		world.SetPlayOnlySystemsEnabled(false);
	}

	// The game-time part of Application::Update, in the same order.
	void frame(f32 delta) {
		scenes.Flush();
		const GameTime time = GetGameTime(scenes.IsPlaying(), scenes.IsPaused(),
			scenes.TakeStep(), delta, physics.Config().fixedDelta);
		if (time.running) {
			scripts.UpdateTimers(time.delta);
			scripts.UpdateTweens(time.delta);
		}
		world.SetPlayOnlySystemsEnabled(time.running);
		world.Progress(time.delta);
	}

	// Level with a falling box and a particle emitter, playing.
	Scene play() {
		Scene level = scenes.Create("Level");
		Entity box = level.CreateEntity("Box");
		box.Add(Transform2D{Vector2(0.f, 200.f), 0.f, Vector2(1.f, 1.f)});
		RigidBody2D body;
		body.type = BodyType2D::Dynamic;
		box.Add(body);
		Collider2D collider;
		collider.type = ShapeType2D::Box;
		collider.size = Vector2(10.f, 10.f);
		box.Add(collider);
		Entity sparks = level.CreateEntity("Sparks");
		sparks.Add(Transform2D{});
		Particles2D particles;
		particles.emissionRate = 60.f;
		sparks.Add(particles);

		REQUIRE(scenes.SetCurrent(level));
		scenes.PlayCurrent();
		frame(0.f);
		REQUIRE(scenes.IsPlaying());
		Scene running = scenes.Current();
		running.Root().Find("Box").Add(Ticker{});
		return running;
	}
};

f32 BoxY(Scene scene) {
	return scene.Root().Find("Box").Get<Transform2D>()->position.y;
}

i32 Runs(Scene scene) {
	return scene.Root().Find("Box").Get<Ticker>()->runs;
}

const Particles2D &Sparks(Scene scene) {
	return *scene.Root().Find("Sparks").Get<Particles2D>();
}

} // namespace

TEST_CASE("Pause freezes gameplay, and each Step advances exactly one physics tick") {
	Fixture f;
	f.world.CreateSystem<Ticker>("Count").Each([](Entity, Ticker &ticker) { ++ticker.runs; });
	REQUIRE(f.scripts.DoString(R"(
		t = timers:create(10, true)
		tw = tweens:create(10)
	)"));
	Scene play = f.play();
	const f32 tick = f.physics.Config().fixedDelta;

	for (int i = 0; i < 3; ++i) {
		f.frame(tick);
	}
	REQUIRE(Runs(play) == 3);
	REQUIRE(BoxY(play) < 200.f);

	f.scenes.Pause();
	const f32 y = BoxY(play);
	const f32 accumulator = f.physics.Accumulator();
	const f64 cycleTime = Sparks(play).runtime.cycleTime;
	const size_t particleCount = Sparks(play).runtime.particles.size();
	REQUIRE(particleCount > 0);
	REQUIRE(f.scripts.DoString("assert(t:is_running() and t.remaining < 10 and tw.elapsed > 0)\nremaining = t.remaining; elapsed = tw.elapsed"));

	for (int i = 0; i < 5; ++i) {
		f.frame(tick);
	}
	CHECK(Runs(play) == 3);
	CHECK(BoxY(play) == y);
	CHECK(f.physics.Accumulator() == doctest::Approx(accumulator));
	CHECK(Sparks(play).runtime.cycleTime == doctest::Approx(cycleTime));
	CHECK(Sparks(play).runtime.particles.size() == particleCount);
	CHECK(f.scripts.DoString("assert(t.remaining == remaining and tw.elapsed == elapsed)"));

	// A long frame delta does not matter: one step is one fixed tick.
	f.scenes.Step();
	f.frame(0.5f);
	CHECK(f.scenes.IsPaused());
	CHECK(Runs(play) == 4);
	CHECK(BoxY(play) < y);
	// Exactly one tick ran: the leftover below one tick is unchanged.
	CHECK(f.physics.Accumulator() == doctest::Approx(accumulator));
	REQUIRE(f.scripts.DoString("tick = " + std::to_string(tick)));
	CHECK(f.scripts.DoString(
		"assert(math.abs((remaining - t.remaining) - tick) < 1e-4, 'timer')\n"
		"assert(math.abs((tw.elapsed - elapsed) - tick) < 1e-4, 'tween')"));

	const f32 stepped = BoxY(play);
	f.frame(tick);
	CHECK(Runs(play) == 4);
	CHECK(BoxY(play) == stepped);

	f.scenes.Resume();
	f.frame(tick);
	CHECK(Runs(play) == 5);
	CHECK(BoxY(play) < stepped);
}

// A mid-frame pause affects the next frame's game time; the current frame finishes running.
TEST_CASE("Pause from inside a frame lets that frame finish and freezes the next") {
	Fixture f;
	bool pauseNow = false;
	f.world.CreateSystem<Ticker>("Breakpoint").Each([&](Entity, Ticker &) {
		if (pauseNow) {
			f.scenes.Pause();
			CHECK(f.scenes.IsPaused());
		}
	});
	f.world.CreateSystem<Ticker>("After").Each([](Entity, Ticker &ticker) { ++ticker.runs; });
	Scene play = f.play();
	const f32 tick = f.physics.Config().fixedDelta;

	f.frame(tick);
	REQUIRE(Runs(play) == 1);

	pauseNow = true;
	const f32 y = BoxY(play);
	f.frame(tick);
	pauseNow = false;
	CHECK(f.scenes.IsPaused());
	CHECK(Runs(play) == 2);
	CHECK(BoxY(play) < y);

	const f32 paused = BoxY(play);
	f.frame(tick);
	CHECK(Runs(play) == 2);
	CHECK(BoxY(play) == paused);
}

TEST_CASE("Paused Step runs exactly one fixed callback with a fractional remainder and cap one") {
	Fixture f;
	PhysicsWorldConfig config;
	config.maxStepsPerFrame = 1;
	f.physics.Init(f.world, config);
	Scene play = f.play();
	const f32 tick = f.physics.Config().fixedDelta;
	int fixed = 0;
	int frames = 0;
	f.world.CreateFixedSystem<Ticker>("FixedPausedCounter").Each(
		[&](Entity, f32 dt, Ticker &) { CHECK(dt == tick); ++fixed; });
	f.world.CreateSystem<Ticker>("FramePausedCounter").Each(
		[&](Entity, Ticker &) { ++frames; });
	REQUIRE(f.scripts.DoString(R"(
		local M = world:component('PausedFixedLua', { frames = 0, fixed = 0 })
		function M:_update(dt) self.frames = self.frames + 1 end
		function M:_physics_update(dt) self.fixed = self.fixed + 1 end
	)"));
	Entity box = play.Root().Find("Box");
	box.Add(f.world.FindComponent("PausedFixedLua"));
	f.scripts.SetEntity("pausedBox", box);
	f.frame(tick * 0.4f);
	const f32 remainder = f.physics.Accumulator();
	REQUIRE(remainder > 0.f);
	f.scenes.Pause();
	const f32 y = BoxY(play);
	f.frame(10.f);
	CHECK(fixed == 0);
	CHECK(frames == 1);
	CHECK(BoxY(play) == y);
	for (int n = 1; n <= 3; ++n) {
		f.scenes.Step();
		f.frame(10.f);
		CHECK(f.scenes.IsPaused());
		CHECK(fixed == n);
		CHECK(frames == n + 1);
		CHECK(f.physics.Accumulator() == doctest::Approx(remainder));
		f.frame(10.f);
		CHECK(fixed == n);
	}
	CHECK(f.scripts.DoString("local m = pausedBox:get('PausedFixedLua'); assert(m.frames == 4 and m.fixed == 3)"));
	f.scenes.Resume();
	f.frame(tick * 0.6f);
	CHECK(fixed == 4);
	CHECK(f.physics.Accumulator() == doctest::Approx(0.f).epsilon(0.00001));
}

TEST_CASE("Pause inside fixed gameplay finishes the selected catch-up frame and Stop resets tick state") {
	Fixture f;
	f.scenes.SetPlaySessionCallbacks([&] { f.physics.ResetSimulation(); },
		[&] { f.physics.ResetSimulation(); });
	Scene play = f.play();
	int runs = 0;
	f.world.CreateFixedSystem<Ticker>("PauseFromFixed").Each([&](Entity, f32, Ticker &) {
		++runs;
		if (runs == 1) { f.scenes.Pause(); }
	});
	const f32 tick = f.physics.Config().fixedDelta;
	f.frame(tick * 2.5f);
	CHECK(runs == 2);
	CHECK(f.scenes.IsPaused());
	f.frame(tick * 3.f);
	CHECK(runs == 2);
	Entity box = play.Root().Find("Box");
	f.physics.ApplyForce(box, {10000.f, 0.f});
	f.scenes.Stop();
	f.frame(tick);
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.physics.Accumulator() == 0.f);
	CHECK(f.physics.FrameEvents().empty());
	CHECK_FALSE(b2Body_IsValid(f.physics.BodyId(box)));
	play = f.play();
	f.frame(tick);
	CHECK(runs == 3);
	CHECK(f.physics.GetLinearVelocity(play.Root().Find("Box")).x == 0.f);
}

TEST_CASE("Paused single stepping is exact at different fixed rates and near-full remainders") {
	for (f32 tick : {1.f / 30.f, 1.f / 120.f}) {
		for (f32 fraction : {0.1f, 0.999f}) {
			Fixture f;
			PhysicsWorldConfig config;
			config.fixedDelta = tick;
			config.maxStepsPerFrame = 1;
			f.physics.Init(f.world, config);
			f.play();
			int callbacks = 0;
			f.world.CreateFixedSystem<Ticker>("DifferentRateStep").Each(
				[&](Entity, f32 dt, Ticker &) { CHECK(dt == tick); ++callbacks; });
			f.frame(tick * fraction);
			const f32 remainder = f.physics.Accumulator();
			REQUIRE(callbacks == 0);
			f.scenes.Pause();
			for (int n = 1; n <= 30; ++n) {
				f.scenes.Step();
				f.frame(10.f);
				CHECK(callbacks == n);
				CHECK(f.scenes.IsPaused());
				CHECK(f.physics.Accumulator() == doctest::Approx(remainder));
				f.frame(10.f);
				CHECK(callbacks == n);
			}
		}
	}
}
