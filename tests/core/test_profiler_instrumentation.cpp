#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <string_view>

#include "core/filesystem/file_system.hpp"
#include "core/profiler.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "particles/particles2d.hpp"
#include "physics/physics_world.hpp"
#include "render/renderer2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "script/script_server.hpp"

using namespace sowa;

// Profiler instrumentation for systems, Lua hooks, physics ticks, and renderer statistics.

namespace {

Profiler &FreshProfiler() {
	Profiler &profiler = Profiler::Main();
	profiler.Clear();
	profiler.SetEnabled(true);
	profiler.SetFrozen(false);
	return profiler;
}

const ProfileFrame &LastFrame(const Profiler &profiler) {
	REQUIRE(profiler.FrameCount() > 0);
	return profiler.FrameAt(profiler.FrameCount() - 1);
}

int CountScopes(const Profiler &profiler, const ProfileFrame &frame, std::string_view name) {
	int count = 0;
	for (const ProfileScopeRecord &scope : frame.scopes) {
		if (profiler.NameOf(scope.name) == name) {
			++count;
		}
	}
	return count;
}

const ProfileScopeRecord *FindScope(const Profiler &profiler, const ProfileFrame &frame,
	std::string_view name) {
	for (const ProfileScopeRecord &scope : frame.scopes) {
		if (profiler.NameOf(scope.name) == name) {
			return &scope;
		}
	}
	return nullptr;
}

const ProfileCounterRecord *FindCounter(const Profiler &profiler, const ProfileFrame &frame,
	std::string_view name) {
	for (const ProfileCounterRecord &counter : frame.counters) {
		if (profiler.NameOf(counter.name) == name) {
			return &counter;
		}
	}
	return nullptr;
}

class CountingDevice : public StubGraphicsDevice {
  public:
	PipelineHandle CreatePipeline(const PipelineDesc &) override {
		return {++pipelineCount, 1};
	}

	BufferHandle CreateBuffer(BufferType, BufferUsage, u32, const void *) override {
		return {++bufferCount, 1};
	}

	void DrawIndexed(BufferHandle, BufferHandle, u32) override {
		++drawIndexedCount;
	}

	int drawIndexedCount = 0;
	u32 pipelineCount = 0;
	u32 bufferCount = 0;
};

} // namespace

TEST_CASE("Every flecs system appears under its name with its run time") {
	Profiler &profiler = FreshProfiler();
	World world;
	world.CreateEntity().Add(Transform2D{});
	world.CreateSystem<Transform2D>("KnownWorkload").Always().Each([](Entity, Transform2D &) {
		const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
		while (std::chrono::steady_clock::now() < until) {
		}
	});

	profiler.BeginFrame();
	world.Progress(1.f / 60.f);
	profiler.EndFrame();

	const ProfileFrame &frame = LastFrame(profiler);
	const ProfileScopeRecord *workload = FindScope(profiler, frame, "KnownWorkload");
	REQUIRE(workload != nullptr);
	CHECK(workload->depth == 0);
	CHECK(workload->endNs - workload->beginNs >= 2'000'000);
	// Built-in systems are timed the same way, without registering anything.
	// Systems are siblings: each one's scope ends before the next begins.
	const ProfileScopeRecord *transforms = FindScope(profiler, frame, "CalculateTransforms");
	REQUIRE(transforms != nullptr);
	CHECK(transforms->depth == 0);
	CHECK(workload->endNs <= transforms->beginNs);
	// flecs' own internal traces are not recorded.
	for (const ProfileScopeRecord &scope : frame.scopes) {
		CHECK_FALSE(profiler.NameOf(scope.name).starts_with("flecs."));
	}
	profiler.Clear();
}

TEST_CASE("Systems run outside a recorded frame record nothing") {
	Profiler &profiler = FreshProfiler();
	World world;
	world.CreateEntity().Add(Transform2D{});
	world.CreateSystem<Transform2D>("Gameplay").Always().Each([](Entity, Transform2D &) {});
	world.Progress(1.f / 60.f);
	CHECK(profiler.FrameCount() == 0);

	// The trace stack stayed balanced: a later frame still nests correctly.
	profiler.BeginFrame();
	world.Progress(1.f / 60.f);
	profiler.EndFrame();
	const ProfileFrame &frame = LastFrame(profiler);
	REQUIRE(frame.scopes.size() >= 2);
	for (const ProfileScopeRecord &scope : frame.scopes) {
		CHECK(scope.depth == 0);
	}
	profiler.Clear();
}

TEST_CASE("Lua hooks are timed per script type") {
	Profiler &profiler = FreshProfiler();
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		local Mover = world:component("Mover", { speed = 1 })
		function Mover:_update(dt)
			self.speed = self.speed + dt
		end
		function Mover:_exit()
		end
		a = world:create_entity()
		a:add("Mover")
		b = world:create_entity()
		b:add("Mover")
	)"));

	profiler.BeginFrame();
	world.Progress(1.f / 60.f);
	REQUIRE(scripts.DoString("a:remove('Mover')"));
	profiler.EndFrame();

	const ProfileFrame &frame = LastFrame(profiler);
	// _update is one flecs system over both entities: one scope, not two.
	// flecs escapes the dots in the system's path; the name is unescaped.
	CHECK(CountScopes(profiler, frame, "Lua.Mover._update") == 1);
	// _exit runs from an observer, not a system, and gets its own scope.
	CHECK(CountScopes(profiler, frame, "Lua.Mover._exit") == 1);
	profiler.Clear();
}

TEST_CASE("A catch-up frame reports each physics tick") {
	Profiler &profiler = FreshProfiler();
	PhysicsWorldConfig config;
	PhysicsWorld physics;
	physics.Init(config);

	profiler.BeginFrame();
	physics.Step(config.fixedDelta * 3.5f);
	profiler.EndFrame();

	const ProfileFrame &frame = LastFrame(profiler);
	const ProfileCounterRecord *ticks = FindCounter(profiler, frame, "Physics ticks");
	REQUIRE(ticks != nullptr);
	CHECK(ticks->value == 3.0);
	CHECK(CountScopes(profiler, frame, "Physics tick") == 3);

	profiler.BeginFrame();
	physics.Step(config.fixedDelta * 0.25f);
	profiler.EndFrame();
	const ProfileCounterRecord *none = FindCounter(profiler, LastFrame(profiler), "Physics ticks");
	REQUIRE(none != nullptr);
	CHECK(none->value == 0.0);
	physics.Shutdown();
	profiler.Clear();
}

TEST_CASE("Renderer stats count draw calls, quads and batch textures") {
	CountingDevice device;
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Renderer2D renderer;
	REQUIRE(renderer.Init(device, fs));

	TextureHandle textures[20];
	for (TextureHandle &texture : textures) {
		texture = device.CreateTexture(1, 1, nullptr, TextureFilter::Linear);
	}

	SUBCASE("sprites sharing a few textures batch into one draw") {
		renderer.Begin();
		for (int i = 0; i < 12; ++i) {
			renderer.DrawQuad(0, 0, 1, 1, textures[i % 3]);
		}
		renderer.End();
		CHECK(renderer.GetStats().drawCalls == 1);
		CHECK(device.drawIndexedCount == 1);
		CHECK(renderer.GetStats().quads == 12);
		CHECK(renderer.GetStats().textureBinds == 3);
	}

	SUBCASE("more textures than sampler slots split the batch") {
		renderer.Begin();
		for (const TextureHandle texture : textures) {
			renderer.DrawQuad(0, 0, 1, 1, texture);
		}
		renderer.End();
		CHECK(renderer.GetStats().drawCalls == 2);
		CHECK(device.drawIndexedCount == 2);
		CHECK(renderer.GetStats().quads == 20);
		CHECK(renderer.GetStats().textureBinds == 20);
	}

	SUBCASE("a full vertex buffer splits the batch") {
		renderer.Begin();
		for (int i = 0; i < 2500; ++i) {
			renderer.DrawQuad(0, 0, 1, 1, textures[0]);
		}
		renderer.End();
		CHECK(renderer.GetStats().drawCalls == 3);
		CHECK(device.drawIndexedCount == 3);
		CHECK(renderer.GetStats().quads == 2500);
		CHECK(renderer.GetStats().textureBinds == 3);
	}

	SUBCASE("stats add up across passes until reset") {
		renderer.Begin();
		renderer.DrawQuad(0, 0, 1, 1, textures[0]);
		renderer.End();
		renderer.Begin();
		renderer.DrawQuad(0, 0, 1, 1, textures[1]);
		renderer.End();
		CHECK(renderer.GetStats().drawCalls == 2);
		CHECK(renderer.GetStats().quads == 2);
		renderer.ResetStats();
		CHECK(renderer.GetStats().drawCalls == 0);
		CHECK(renderer.GetStats().quads == 0);
		CHECK(renderer.GetStats().textureBinds == 0);
	}

	renderer.Shutdown();
}

TEST_CASE("CountLiveParticles2D sums live particles over every emitter") {
	World world;
	CHECK(CountLiveParticles2D(world) == 0);

	Entity first = world.CreateEntity();
	first.Add(Transform2D{});
	first.Add<Particles2D>();
	first.Get<Particles2D>()->runtime.particles.resize(3);
	Entity second = world.CreateEntity();
	second.Add(Transform2D{});
	second.Add<Particles2D>();
	second.Get<Particles2D>()->runtime.particles.resize(5);
	CHECK(CountLiveParticles2D(world) == 8);
}
