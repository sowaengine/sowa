#include <cmath>
#include <doctest/doctest.h>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/scene_commands.hpp"
#include "particles/particles2d.hpp"
#include "render/sprite_renderer2d.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {
constexpr double Step = 1.0 / 60;

void step(Particles2D &p, double dt = Step, const glm::mat4 &matrix = glm::mat4(1)) {
	UpdateParticles2D(p, matrix, dt);
}

Particles2D burst() {
	Particles2D p;
	p.burstCount = 10;
	p.emissionRate = 0;
	p.oneShot = true;
	p.duration = 0.05f;
	p.lifetime = 1;
	p.initialVelocity = {0, 0};
	return p;
}

Entity spawn(World &world, Particles2D particles = Particles2D{}) {
	auto entity = world.CreateEntity();
	entity.Add<Transform2D>();
	entity.Add(particles);
	return entity;
}
} // namespace

TEST_CASE("Particles: fixed steps preserve seeded trajectories across frame rates") {
	auto a = MakeParticlePreset(ParticlePreset::Water), b = a;
	for (int i = 0; i < 60; ++i) {
		step(a);
	}
	for (int i = 0; i < 144; ++i) {
		step(b, 1.0 / 144);
	}
	REQUIRE(a.runtime.particles.size() == 120);
	REQUIRE(a.runtime.particles.size() == b.runtime.particles.size());
	for (size_t i = 0; i < a.runtime.particles.size(); ++i) {
		CHECK(a.runtime.particles[i].position == b.runtime.particles[i].position);
		CHECK(a.runtime.particles[i].lifetime == b.runtime.particles[i].lifetime);
	}
	a.Restart();
	for (int i = 0; i < 60; ++i) {
		step(a);
	}
	CHECK(a.runtime.particles.front().position == b.runtime.particles.front().position);
}

TEST_CASE("Particles: stopping drains particles and starting resumes emission") {
	Particles2D p;
	p.emissionRate = 30;
	p.lifetime = 0.2f;
	step(p, 0.1);
	CHECK(p.runtime.particles.size() == 3);
	p.isEmitting = false;
	step(p, 0.3);
	CHECK(p.runtime.particles.empty());
	p.isEmitting = true;
	step(p, 0.1);
	CHECK(p.runtime.particles.size() == 3);
}

TEST_CASE("Particles: delayed one-shot bursts complete without changing authored emitting flag") {
	auto p = burst();
	p.startDelay = 0.1f;
	step(p, 0.1);
	CHECK(p.runtime.particles.empty());
	step(p);
	CHECK(p.runtime.particles.size() == 10);
	step(p, 0.1);
	CHECK_FALSE(p.IsEmitting());
	CHECK_FALSE(p.IsFinished());
	CHECK(p.isEmitting);
	step(p, 1);
	CHECK(p.IsFinished());
	p.Restart();
	step(p, 0.2);
	CHECK(p.runtime.particles.size() == 10);
}

TEST_CASE("Particles: repeating bursts, lifetime randomness and live duration edits") {
	auto p = burst();
	p.oneShot = false;
	p.duration = 0.1f;
	p.lifetimeRandomness = 0.5f;
	step(p, 0.25);
	CHECK(p.runtime.particles.size() == 30);
	for (auto &particle : p.runtime.particles) {
		CHECK(particle.lifetime >= 0.5f);
		CHECK(particle.lifetime <= 1);
	}
	p.duration = 0.02f;
	step(p, 0.1); // shortening an in-flight cycle must not hang or run backwards
	CHECK(p.runtime.particles.size() > 30);
}

TEST_CASE("Particles: preprocess, pause, invalid input and capacity have bounded behavior") {
	Particles2D p;
	p.preprocess = 1;
	p.amount = 10;
	step(p);
	CHECK(p.runtime.particles.size() == 10);
	const auto position = p.runtime.particles.front().position;
	p.speedScale = 0;
	step(p, 1);
	CHECK(p.runtime.particles.front().position == position);
	p.amount = 2;
	step(p);
	CHECK(p.runtime.particles.size() == 2);
	p.amount = -100;
	step(p);
	CHECK(p.runtime.particles.empty());
	p.speedScale = std::numeric_limits<float>::quiet_NaN();
	step(p, std::numeric_limits<double>::infinity());
	step(p, -1);
	CHECK(p.runtime.particles.empty());
	auto bad = burst();
	bad.lifetime = 0;
	bad.direction = {NAN, INFINITY};
	bad.gravity = {NAN, INFINITY};
	bad.duration = -1;
	bad.emissionRate = INFINITY;
	step(bad, 1);
	CHECK(bad.runtime.particles.empty());
	auto huge = burst();
	huge.amount = INT_MAX;
	huge.burstCount = INT_MAX;
	step(huge);
	CHECK(huge.runtime.particles.size() == MaxParticles2D);
}

TEST_CASE("Particles: emission regions use uniform area sampling and respect their bounds") {
	for (auto shape :
		 {ParticleEmissionShape::Point, ParticleEmissionShape::Circle, ParticleEmissionShape::Ring,
		  ParticleEmissionShape::Rectangle, ParticleEmissionShape::Line}) {
		auto p = burst();
		p.burstCount = p.amount = 1000;
		p.emissionShape = shape;
		p.emissionRadius = 10;
		p.emissionInnerRadius = 5;
		p.emissionSize = {20, 6};
		step(p);
		double radiusSquared = 0;
		for (const auto &particle : p.runtime.particles) {
			const auto pos = particle.position;
			radiusSquared += pos.LengthSquared();
			if (shape == ParticleEmissionShape::Point) {
				CHECK(pos == Vector2{});
			}
			if (shape == ParticleEmissionShape::Circle || shape == ParticleEmissionShape::Ring) {
				CHECK(pos.Length() <= 10);
			}
			if (shape == ParticleEmissionShape::Ring) {
				CHECK(pos.Length() >= 5);
			}
			if (shape == ParticleEmissionShape::Rectangle || shape == ParticleEmissionShape::Line) {
				CHECK(std::abs(pos.x) <= 10);
				CHECK(std::abs(pos.y) <= 3);
			}
			if (shape == ParticleEmissionShape::Line) {
				CHECK(pos.y == doctest::Approx(pos.x * 0.3f));
			}
		}
		if (shape == ParticleEmissionShape::Circle) {
			CHECK(radiusSquared / 1000 == doctest::Approx(50).epsilon(0.1));
		}
	}
}

TEST_CASE("Particles: world trails stay put, local particles follow hierarchy transforms") {
	World world;
	auto parent = world.CreateEntity();
	parent.Add(Transform2D{{100, 200}});
	auto worldParticle = spawn(world, burst());
	worldParticle.SetParent(parent);
	auto localSettings = burst();
	localSettings.localCoords = true;
	auto localParticle = spawn(world, localSettings);
	localParticle.SetParent(parent);
	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(1.f / 60);
	REQUIRE_FALSE(worldParticle.Get<Particles2D>()->runtime.particles.empty());
	REQUIRE_FALSE(localParticle.Get<Particles2D>()->runtime.particles.empty());
	CHECK(worldParticle.Get<Particles2D>()->runtime.particles.front().position ==
		  Vector2(100, 200));
	parent.Get<Transform2D>()->position.x += 100;
	world.Progress(1.f / 60);
	const auto *global = worldParticle.Get<Particles2D>();
	const auto *local = localParticle.Get<Particles2D>();
	CHECK(global->runtime.particles.front().position == Vector2(100, 200));
	const auto model = ParticleTransform(*local, local->runtime.particles.front(),
										 localParticle.Get<Transform2D>()->worldMatrix);
	CHECK(model[3].x == doctest::Approx(200));
	CHECK(model[3].y == doctest::Approx(200));
	// Disabled documents freeze; hiding only controls drawing.
	const auto age = local->runtime.particles.front().age;
	localParticle.SetEnabled(false);
	world.Progress(0.1f);
	CHECK(localParticle.Get<Particles2D>()->runtime.particles.front().age == age);
}

TEST_CASE("Particles: forces, plane bounce and velocity inheritance") {
	auto p = burst();
	p.burstCount = 1;
	p.initialVelocity = {60, 60};
	p.direction = {0, -1};
	p.spread = 0;
	p.gravity = {0, -60};
	p.collisionEnabled = true;
	p.collisionY = -1;
	p.collisionBounce = 0.5f;
	step(p);
	REQUIRE(p.runtime.particles.size() == 1);
	CHECK(p.runtime.particles[0].position.y == doctest::Approx(-1));
	CHECK(p.runtime.particles[0].velocity.y == doctest::Approx(30.5));
	Particles2D trail;
	trail.initialVelocity = {0, 0};
	trail.inheritVelocity = 1;
	step(trail);
	step(trail, Step, glm::translate(glm::mat4(1), glm::vec3(1, 0, 0)));
	CHECK(trail.runtime.particles.back().velocity.x == doctest::Approx(60));
}

TEST_CASE("Particles: ramps and sprite sheet UVs") {
	Particles2D c;
	Particle2D p;
	p.lifetime = 2;
	p.age = 1;
	p.size = 10;
	c.sizeStart = 0;
	c.sizeMiddle = 2;
	c.sizeEnd = 0;
	c.sizeMidpoint = 0.5f;
	c.colorMiddle = {0.2f, 0.3f, 0.4f, 0.5f};
	CHECK(ParticleSize(c, p) == doctest::Approx(20));
	CHECK(ParticleColor(c, p) == c.colorMiddle);
	c.hFrames = 4;
	c.vFrames = 2;
	const auto uv = ParticleUV(c, p);
	CHECK(uv.x == 0);
	CHECK(uv.y == 0.5f);
	CHECK(uv.w == 0.25f);
	CHECK(uv.h == 0.5f);
	const auto flipped = ParticleUV(c, p, true);
	CHECK(flipped.y == 0.5f);
	CHECK(flipped.h == -0.5f);
	c.animationSpeed = 10;
	c.animationLoop = true;
	CHECK(ParticleUV(c, p).x == 0.5f);
}

TEST_CASE("Particles: scene save, duplicate and play snapshots exclude live particle state") {
	World world;
	ResourceRegistry resources;
	Scene scene = CreateScene(world, "Effects");
	auto entity = scene.CreateEntity("Fire");
	entity.Add<Transform2D>();
	entity.Add(MakeParticlePreset(ParticlePreset::Flame));
	world.Progress(0.1f);
	REQUIRE_FALSE(entity.Get<Particles2D>()->runtime.particles.empty());
	const auto copy = entity.Duplicate(scene.Root());
	CHECK(copy.Get<Particles2D>()->runtime.particles.empty());
	CHECK(copy.Get<Particles2D>()->blendMode == ParticleBlendMode::Additive);
	auto saved = Archive::Save();
	REQUIRE(SaveScene(saved, scene, resources));
	CHECK(saved.ToString().find("runtime") == std::string::npos);
	CHECK(saved.ToString().find("is_emitting: true") != std::string::npos);
	World restoredWorld;
	auto loaded = Archive::Load(saved.ToString());
	Scene restored = LoadScene(loaded, restoredWorld, resources);
	REQUIRE(restored.IsValid());
	auto restoredEntity = restoredWorld.Find("Effects/Fire");
	REQUIRE(restoredEntity.Has<Particles2D>());
	CHECK(restoredEntity.Get<Particles2D>()->runtime.particles.empty());
	CHECK(restoredEntity.Get<Particles2D>()->blendMode == ParticleBlendMode::Additive);
	REQUIRE(TakeSnapshot(scene, resources));
	entity.Get<Particles2D>()->isEmitting = false;
	REQUIRE(RestoreSnapshot(scene, resources));
	auto fresh = world.Find("Effects/Fire");
	CHECK(fresh.Get<Particles2D>()->isEmitting);
	CHECK(fresh.Get<Particles2D>()->runtime.particles.empty());
}

TEST_CASE("Particles: editor creation, presets and undo integrate with document history") {
	World world;
	ScriptServer scripts(world);
	Scene scene = CreateScene(world, "Effects");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	auto entity = editor::ExecuteCreateEntity(states, scene, scene.Root(),
											  editor::EntityArchetype::Particles2D);
	REQUIRE(entity.Has<Transform2D>());
	REQUIRE(entity.Has<Particles2D>());
	const auto before = *entity.Get<Particles2D>();
	*entity.Get<Particles2D>() = MakeParticlePreset(ParticlePreset::Smoke);
	REQUIRE(editor::RecordParticlesEdit(states, scene, entity, before, *entity.Get<Particles2D>()));
	world.Progress(0.1f);
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(entity.Get<Particles2D>()->emissionRate == before.emissionRate);
	CHECK(entity.Get<Particles2D>()->runtime.particles.empty());
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(entity.Get<Particles2D>()->emissionRate == 24);
	auto empty = scene.CreateEntity("Empty");
	REQUIRE(editor::ExecuteAddComponent(states, scene, empty, world.FindComponent("Particles2D"),
										scripts));
	CHECK(empty.Has<Transform2D>());
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK_FALSE(empty.Has<Transform2D>());
	CHECK_FALSE(empty.Has<Particles2D>());
}

TEST_CASE("Particles: render collection shares ordering, visibility and picking with sprites") {
	World world;
	ResourceRegistry resources;
	SpriteRenderer2D renderer;
	resources.RegisterType<ImageTexture>("ImageTexture");
	renderer.Init(world, resources);
	auto p = burst();
	p.blendMode = ParticleBlendMode::Additive;
	auto entity = spawn(world, p);
	entity.Add(RenderOrder{4});
	auto texture = resources.Create<ImageTexture>();
	texture->Adopt(resources.Context(), {12, 1}, 8, 8);
	auto sprite = world.CreateEntity();
	sprite.Add<Transform2D>();
	sprite.Add(Sprite{texture->Id()});
	sprite.Add(RenderOrder{5});
	world.Progress(1.f / 60);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 11);
	CHECK(renderer.DrawList().front().entity == entity);
	CHECK(renderer.DrawList().front().proceduralParticle);
	CHECK(renderer.DrawList().front().blendMode == ParticleBlendMode::Additive);
	CHECK(renderer.DrawList().back().entity == sprite);
	entity.SetVisible(false);
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 1);
	entity.SetVisible(true);
	entity.Get<Particles2D>()->texture = texture->Id();
	renderer.Collect();
	CHECK_FALSE(renderer.DrawList().front().proceduralParticle);
	CHECK(renderer.DrawList().front().texture.id == 12);
	entity.Destroy();
	renderer.Collect();
	CHECK(renderer.DrawList().size() == 1);
}

TEST_CASE("Particles: Lua controls and is_emitting field work in stopped editor worlds") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	Input input;
	ProjectSettings settings;
	SceneManager scenes(world, resources, fs);
	ScriptServer scripts(world);
	scripts.BindEngine(input, scenes, resources, settings);
	world.SetPlayOnlySystemsEnabled(false);
	REQUIRE(scripts.DoString(R"(
		e = world:create_entity("Effect")
		e:add("Transform2D")
		e:add("Particles2D")
		e:get("Particles2D").is_emitting = false
		assert(not particles.is_emitting(e))
		assert(particles.restart(e))
	)"));
	world.Progress(0.1f);
	REQUIRE(scripts.DoString("assert(particles.count(e) == 6); particles.set_emitting(e, false)"));
	world.Progress(1);
	world.Progress(1);
	REQUIRE(scripts.DoString("assert(particles.count(e) == 0)"));
}

TEST_CASE("Particles: showcase scene loads and previews every continuous preset without Play") {
	World world;
	FileSystem fs;
	ResourceRegistry resources;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	auto scene = LoadSceneFromFile(fs, "res://scenes/particles.scene", world, resources);
	REQUIRE(scene.IsValid());
	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(0.2f);
	for (const char *name : {"Smoke", "Flame", "Water", "Sparks", "Snow"}) {
		CAPTURE(name);
		auto entity = world.Find(std::string("ParticleShowcase/") + name);
		REQUIRE(entity.Has<Particles2D>());
		CHECK_FALSE(entity.Get<Particles2D>()->runtime.particles.empty());
	}
}

TEST_CASE("Particles: world velocity alignment retains the emitter birth scale") {
	auto c = burst();
	c.alignToVelocity = true;
	c.direction = {1, 0};
	c.spread = 0;
	c.initialVelocity = {10, 10};
	step(c, Step, glm::scale(glm::mat4(1), glm::vec3(2, 3, 1)));
	REQUIRE_FALSE(c.runtime.particles.empty());
	const auto model = ParticleTransform(c, c.runtime.particles.front(), glm::mat4(1));
	CHECK(glm::length(glm::vec2(model[0])) == doctest::Approx(2));
	CHECK(glm::length(glm::vec2(model[1])) == doctest::Approx(3));
	CHECK(model[1].x == doctest::Approx(3));
}

TEST_CASE("Particles: Emitting resumes without repeating delay or the current burst") {
	auto emitter = burst();
	emitter.oneShot = false;
	emitter.startDelay = 0.1f;
	emitter.duration = 1;
	emitter.emissionRate = 60;
	step(emitter, 0.2);
	const auto count = emitter.runtime.particles.size();
	const auto cycleTime = emitter.runtime.cycleTime;
	emitter.SetEmitting(false);
	step(emitter, 0.2);
	CHECK(emitter.runtime.cycleTime == cycleTime);
	emitter.SetEmitting(true);
	step(emitter);
	CHECK(emitter.runtime.particles.size() == count + 1);
}

TEST_CASE("Particles: Emitting can start another one-shot without clearing live particles") {
	auto emitter = burst();
	step(emitter, 0.1);
	REQUIRE_FALSE(emitter.IsEmitting());
	const auto count = emitter.runtime.particles.size();
	emitter.SetEmitting(true);
	step(emitter);
	CHECK(emitter.runtime.particles.size() == count + emitter.burstCount);
}
