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
