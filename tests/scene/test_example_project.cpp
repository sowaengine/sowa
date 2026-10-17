#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "audio/audio_mixer.hpp"
#include "audio/audio_source.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "core/scope_exit.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/parallax2d.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/text2d.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/script_tags.hpp"
#include "ecs/world.hpp"
#include "editor/scene_documents.hpp"
#include "math/color.hpp"
#include "math/math.hpp"
#include "math/vector2.hpp"
#include "physics/collider2d.hpp"
#include "physics/physics_world.hpp"
#include "physics/raycast2d.hpp"
#include "physics/rigid_body2d.hpp"
#include "render/sprite_renderer2d.hpp"
#include "resource/audio_clip.hpp"
#include "resource/font.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {

struct ExampleFixture {
	World world;
	Input input;
	FileSystem fs;
	ResourceRegistry resources;
	SceneManager scenes;
	ProjectSettings settings;
	ScriptServer scripts;
	SpriteRenderer2D sprites;
	AudioMixer mixer;
	PhysicsWorld physics;

	ExampleFixture() : scenes(world, resources, fs), scripts(world) {
		const std::filesystem::path exampleDir(SOWA_EXAMPLE_DIR);
		fs.MountFolder("res", exampleDir);
		resources.SetContext(ResourceContext{nullptr, &fs, &resources});
		resources.RegisterType<ImageTexture>("ImageTexture")
			.Field("path", &ImageTexture::path)
			.Field("flipVertically", &ImageTexture::flipVertically)
			.Field("retainPixels", &ImageTexture::retainPixels)
			.Field("filter", &ImageTexture::filter);
		resources.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);
		resources.RegisterType<AudioClip>("AudioClip").Field("path", &AudioClip::path);
		CHECK_FALSE(world.FindComponent("Velocity2D").IsValid());
		CHECK_FALSE(world.FindComponent("MouseFollower").IsValid());
		CHECK_FALSE(world.FindComponent("Spin2D").IsValid());
		sprites.Init(world, resources);
		mixer.Init(world, resources);
		physics.Init(world);
		REQUIRE(settings.Load(fs));
		scripts.BindEngine(input, scenes, resources, settings, &mixer, &physics);
		REQUIRE(scripts.LoadModules(fs, settings.application.scripts));
		world.SetPlayOnlySystemsEnabled(false);
	}

	Scene open(std::string_view path) {
		scenes.Open(path);
		scenes.Flush();
		Scene scene = scenes.FindByPath(path);
		REQUIRE(scene.IsValid());
		REQUIRE(scenes.SetCurrent(scene));
		return scene;
	}

	// Update timers and tweens, Flush, run systems, then age input edges -
	// the same order as Application::Update after events have already been
	// posted.
	void tick(f32 dt) {
		scripts.UpdateTimers(dt);
		scripts.UpdateTweens(dt);
		scenes.Flush();
		world.SetPlayOnlySystemsEnabled(scenes.IsPlaying());
		world.Progress(dt);
		mixer.Sync();
		input.Update();
	}

	void play() {
		scenes.PlayCurrent();
		scenes.Flush();
		world.SetPlayOnlySystemsEnabled(true);
		Entity follower = scenes.Current().Root().Find("Object1");
		if (follower.IsAlive()) {
			parkMouseOn(follower);
		}
		world.Progress(0.f);
		mixer.Sync();
		input.Update();
	}

	void parkMouseOn(Entity entity) {
		const Transform2D *transform = entity.Get<Transform2D>();
		REQUIRE(transform != nullptr);
		input.HandleCursorPos(transform->position.x, transform->position.y);
	}

	Entity child(Scene scene, std::string_view name) {
		Entity entity = scene.Root().Find(std::string(name));
		REQUIRE(entity.IsAlive());
		return entity;
	}

	ComponentType type(const char *name) {
		ComponentType found = world.FindComponent(name);
		REQUIRE(found.IsValid());
		return found;
	}

	f32 fieldF32(Entity entity, const char *typeName, const char *field) {
		f32 value = 0.f;
		REQUIRE(entity.GetField(type(typeName), field).Get(value));
		return value;
	}

	Vector2 fieldVector2(Entity entity, const char *typeName, const char *field) {
		Vector2 value;
		REQUIRE(entity.GetField(type(typeName), field).Get(value));
		return value;
	}

	std::vector<Entity> movers(Scene scene) {
		std::vector<Entity> out;
		for (Entity child : scene.Root().GetChildren()) {
			if (child.Has(type("Velocity2D"))) {
				out.push_back(child);
			}
		}
		return out;
	}
};

} // namespace

TEST_CASE("Raycast demo: direct queries and component respond to the scene controls") {
	ExampleFixture f;
	f.resources.RegisterType<Font>("Font")
		.Field("path", &Font::path)
		.Field("pixelSize", &Font::pixelSize);
	f.open("res://scenes/physics_queries.scene");
	f.play();
	const Scene scene = f.scenes.Current();
	Entity probe = f.child(scene, "Probe");
	Entity status = f.child(scene, "Status");
	auto advance = [&] {
		// One tick publishes the new component settings; the next Lua update
		// displays that cache alongside the direct query of the same snapshot.
		f.tick(1.f / 60.f);
		f.tick(1.f / 60.f);
	};
	auto press = [&](Key key) {
		f.input.HandleKey(key, KeyAction::Press);
		advance();
		f.input.HandleKey(key, KeyAction::Release);
		f.input.Update();
	};
	auto checkHit = [&](const char *name) {
		auto hit = f.physics.RaycastHit(probe);
		REQUIRE(hit.has_value());
		CHECK(hit->collider == f.child(scene, name));
		const auto &text = status.Get<Text2D>()->text;
		CHECK(text.find(std::string("Direct (orange): ") + name + " /") != std::string::npos);
		CHECK(
			text.find(std::string("Component cache (green): ") + name + " /") != std::string::npos);
	};
	advance();
	checkHit("Box");
	CHECK(status.Get<Text2D>()->text.find("Box -> Circle -> RotatedBox") != std::string::npos);
	press(Key::S);
	checkHit("Sensor");
	press(Key::Two);
	checkHit("Circle");
	press(Key::Three);
	checkHit("RotatedBox");
	press(Key::R);
	press(Key::X);
	checkHit("Circle");
	press(Key::R);
	checkHit("Box");

	press(Key::Space);
	CHECK_FALSE(probe.Get<Raycast2D>()->enabled);
	CHECK_FALSE(f.physics.RaycastHit(probe).has_value());
	press(Key::F);
	checkHit("Box");
	f.input.HandleKey(Key::Up, KeyAction::Press);
	f.tick(1.f);
	f.input.HandleKey(Key::Up, KeyAction::Release);
	advance();
	CHECK(status.Get<Text2D>()->text.find("Direct (orange): miss") != std::string::npos);
	CHECK(f.physics.RaycastHit(probe)->collider == f.child(scene, "Box"));
	press(Key::F);
	CHECK_FALSE(f.physics.RaycastHit(probe).has_value());
	CHECK(status.Get<Text2D>()->text.find("Component cache (green): miss") != std::string::npos);
	press(Key::R);
	checkHit("Box");
}

TEST_CASE("Example demo.scene instances the object prefab") {
	ExampleFixture f;
	REQUIRE(f.settings.application.mainScene == "res://scenes/demo.scene");
	REQUIRE(f.settings.application.scripts.size() == 2);

	CHECK(f.world.FindComponent("Pulse").IsValid());
	CHECK(f.world.FindComponent("Lifetime").IsValid());
	CHECK(f.world.FindComponent("Game").IsValid());
	CHECK_FALSE(f.world.Find("Game").IsAlive());
	CHECK_FALSE(f.world.Find("Pulse").IsAlive());

	Scene demo = f.open("res://scenes/demo.scene");
	CHECK(demo.Name() == "Demo");
	REQUIRE(demo.ActiveCamera().IsAlive());
	CHECK(demo.ActiveCamera().Has<Camera2D>());
	CHECK(demo.Root().Has(f.type("Game")));

	i32 movers = 0;
	Entity first;
	for (Entity child : demo.Root().GetChildren()) {
		if (!child.Has(f.type("Velocity2D"))) {
			continue;
		}
		if (movers == 0) {
			first = child;
		}
		++movers;
		CHECK(child.Find("Satellite").IsAlive());
		CHECK(IsInstanceRoot(child));
		CHECK(child.Has(f.type("Pulse")));
	}
	CHECK(movers == 5);
	REQUIRE(first.IsAlive());
	CHECK(first.Has(f.type("MouseFollower")));
	REQUIRE(first.Get<Transform2D>() != nullptr);
	CHECK(first.Get<Transform2D>()->position.x == doctest::Approx(620.f));
	CHECK(first.Get<Sprite>()->size.x == doctest::Approx(150.f));
	CHECK(f.fieldF32(first, "Pulse", "amplitude") == doctest::Approx(0.15f));
	CHECK(f.fieldF32(first, "Pulse", "speed") == doctest::Approx(4.f));

	Entity second = f.child(demo, "Object2");
	CHECK(f.fieldF32(second, "Pulse", "amplitude") == doctest::Approx(0.08f));
}

TEST_CASE("Example Pulse YAML override is independent of the prefab default") {
	ExampleFixture f;
	Scene demo = f.open("res://scenes/demo.scene");
	CHECK(f.fieldF32(f.child(demo, "Object1"), "Pulse", "amplitude") == doctest::Approx(0.15f));
	CHECK(f.fieldF32(f.child(demo, "Object2"), "Pulse", "amplitude") == doctest::Approx(0.08f));
}

TEST_CASE("Example scripts do not run while the session is stopped") {
	ExampleFixture f;
	Scene demo = f.open("res://scenes/demo.scene");
	Entity object2 = f.child(demo, "Object2");
	const Vector2 before = object2.Get<Transform2D>()->position;
	const f32 scale = object2.Get<Transform2D>()->scale.x;
	const f32 rotation = object2.Get<Transform2D>()->rotation;

	f.tick(0.1f);
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(object2.Get<Transform2D>()->position.x == doctest::Approx(before.x));
	CHECK(object2.Get<Transform2D>()->scale.x == doctest::Approx(scale));
	CHECK(object2.Get<Transform2D>()->rotation == doctest::Approx(rotation));
	CHECK_FALSE(object2.Raw().has<ScriptLive>(f.type("Pulse").Raw()));
}

TEST_CASE("Example Game _start and Bounce/Spin/Pulse run on Play") {
	ExampleFixture f;
	Scene demo = f.open("res://scenes/demo.scene");
	f.play();

	Scene playing = f.scenes.Current();
	REQUIRE(playing.IsValid());
	REQUIRE(playing.Root() != demo.Root());
	CHECK(playing.Root().Has(f.type("Game")));
	REQUIRE(f.scripts.DoString(R"(
		local root = scenes:current():root()
		local game = root:get("Game")
		assert(game ~= nil)
		assert(game._prefab ~= nil)
		assert(resources:get(game._prefab).path == "res://scenes/object.scene")
		local tex = resources:get(root:find("Object1"):get("Sprite").texture)
		assert(tex.path == "res://icon.png")
		assert(game._camera ~= nil)
		assert(game._camera:has("Camera2D"))
		assert(game._ticks == 0)
		assert(game._clock:is_running())
	)"));

	Entity first = f.child(playing, "Object1");
	Entity second = f.child(playing, "Object2");
	CHECK(first.Has(f.type("MouseFollower")));
	CHECK_FALSE(second.Has(f.type("MouseFollower")));
	CHECK(first.Raw().has<ScriptLive>(f.type("Pulse").Raw()));
	CHECK(second.Raw().has<ScriptLive>(f.type("Pulse").Raw()));

	Entity satellite = first.Find("Satellite");
	REQUIRE(satellite.IsAlive());
	REQUIRE(satellite.Get<RenderOrder>() != nullptr);
	CHECK(satellite.Get<RenderOrder>()->layer == 1);
	CHECK(satellite.Get<RenderOrder>()->layerMode == RenderLayerMode::Explicit);

	f.parkMouseOn(first);
	const Vector2 before = second.Get<Transform2D>()->position;
	const f32 rotationBefore = second.Get<Transform2D>()->rotation;
	f.tick(0.1f);
	const Vector2 after = second.Get<Transform2D>()->position;
	CHECK(after.x == doctest::Approx(before.x - 15.f));
	CHECK(after.y == doctest::Approx(before.y + 17.f));
	CHECK(first.Get<Transform2D>()->position.x == doctest::Approx(620.f));
	CHECK(second.Get<Transform2D>()->rotation == doctest::Approx(rotationBefore - 3.f));

	const f32 expected = 1.f + std::sin(0.1f * 4.f) * f.fieldF32(first, "Pulse", "amplitude");
	CHECK(first.Get<Transform2D>()->scale.x == doctest::Approx(expected));
	const f32 expected2 = 1.f + std::sin(0.1f * 4.f) * 0.08f;
	CHECK(second.Get<Transform2D>()->scale.x == doctest::Approx(expected2));
	REQUIRE(f.scripts.DoString("assert(scenes:current():root():get('Game')._ticks == 0)"));
}

TEST_CASE("Example Game clock timer ticks while playing") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	REQUIRE(f.scripts.DoString("assert(scenes:current():root():get('Game')._ticks == 0)"));
	f.tick(0.3f);
	REQUIRE(f.scripts.DoString("assert(scenes:current():root():get('Game')._ticks >= 1)"));
}

TEST_CASE("Example Pulse extras and amplitude are per-entity") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	REQUIRE(f.scripts.DoString(R"(
		local root = scenes:current():root()
		local a = root:find("Object1"):get("Pulse")
		local b = root:find("Object2"):get("Pulse")
		assert(a._t ~= nil)
		assert(b._t ~= nil)
		local other = b._t
		a._t = 1.5707963267948966
		assert(b._t == other)
		root:find("Object1"):get("Pulse"):boost()
	)"));
	CHECK(f.fieldF32(f.child(f.scenes.Current(), "Object1"), "Pulse", "amplitude") ==
		  doctest::Approx(0.20f));
	CHECK(f.fieldF32(f.child(f.scenes.Current(), "Object2"), "Pulse", "amplitude") ==
		  doctest::Approx(0.08f));

	f.tick(0.f);
	CHECK(f.child(f.scenes.Current(), "Object1").Get<Transform2D>()->scale.x ==
		  doctest::Approx(1.2f));
	CHECK(
		f.child(f.scenes.Current(), "Object2").Get<Transform2D>()->scale.x == doctest::Approx(1.f));
}

TEST_CASE("Example B boosts Pulse amplitude on every mover") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	f.input.HandleKey(Key::B, KeyAction::Press);
	f.tick(0.f);
	CHECK(f.fieldF32(f.child(f.scenes.Current(), "Object1"), "Pulse", "amplitude") ==
		  doctest::Approx(0.20f));
	CHECK(f.fieldF32(f.child(f.scenes.Current(), "Object2"), "Pulse", "amplitude") ==
		  doctest::Approx(0.13f));
}

TEST_CASE("Example T tweens camera scale to 2") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	Entity camera = f.scenes.Current().ActiveCamera();
	REQUIRE(camera.IsAlive());
	REQUIRE(camera.Get<Transform2D>() != nullptr);
	CHECK(camera.Get<Transform2D>()->scale.x == doctest::Approx(1.f));

	f.input.HandleKey(Key::T, KeyAction::Press);
	f.tick(0.f);
	CHECK(camera.Get<Transform2D>()->scale.x == doctest::Approx(1.f));
	REQUIRE(f.scripts.DoString(R"(
		local tw = scenes:current():root():get("Game")._zoom_tween
		assert(tw ~= nil)
		assert(tw:is_running())
	)"));

	f.tick(0.2f);
	CHECK(camera.Get<Transform2D>()->scale.x == doctest::Approx(1.75f));
	CHECK(camera.Get<Transform2D>()->scale.y == doctest::Approx(1.75f));

	f.tick(0.2f);
	CHECK(camera.Get<Transform2D>()->scale.x == doctest::Approx(2.f));
	CHECK(camera.Get<Transform2D>()->scale.y == doctest::Approx(2.f));
	REQUIRE(f.scripts.DoString("assert(scenes:current():root():get('Game')._zoom_tween == nil)"));
}

TEST_CASE("Example Follow chases the mouse and Bounce skips the follower") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	Entity first = f.child(f.scenes.Current(), "Object1");
	REQUIRE(first.Has(f.type("MouseFollower")));
	const Vector2 before = first.Get<Transform2D>()->position;
	const Vector2 target(100.f, 100.f);
	f.input.HandleCursorPos(target.x, target.y);
	f.tick(0.1f);
	const Vector2 after = first.Get<Transform2D>()->position;
	const Vector2 delta = target - before;
	const f32 step = f.fieldF32(first, "MouseFollower", "speed") * 0.1f;
	REQUIRE(delta.Length() > step);
	const Vector2 expected = before + delta * (step / delta.Length());
	CHECK(after.x == doctest::Approx(expected.x));
	CHECK(after.y == doctest::Approx(expected.y));
	CHECK(after.x != doctest::Approx(before.x + 18.f));
}

TEST_CASE("Example Bounce sets inward velocity at the viewport edge") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	Entity second = f.child(f.scenes.Current(), "Object2");
	REQUIRE(second.Has(f.type("Velocity2D")));
	const f32 half = 150.f * 0.5f;
	const f32 right = static_cast<f32>(f.settings.rendering.viewport.width) - half;
	const f32 midY = static_cast<f32>(f.settings.rendering.viewport.height) * 0.5f;

	second.Get<Transform2D>()->position = Vector2(right, midY);
	REQUIRE(second.GetField(f.type("Velocity2D"), "velocity").Set(Vector2(200.f, 0.f)));
	f.tick(0.1f);
	CHECK(second.Get<Transform2D>()->position.x == doctest::Approx(right));
	CHECK(f.fieldVector2(second, "Velocity2D", "velocity").x == doctest::Approx(-200.f));

	f.tick(1e-8f);
	CHECK(f.fieldVector2(second, "Velocity2D", "velocity").x == doctest::Approx(-200.f));
	f.tick(0.1f);
	CHECK(second.Get<Transform2D>()->position.x == doctest::Approx(right - 20.f));
	CHECK(f.fieldVector2(second, "Velocity2D", "velocity").x == doctest::Approx(-200.f));
}

TEST_CASE("Example play-only systems pause when play-only is off") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	f.tick(0.1f);
	Entity second = f.child(f.scenes.Current(), "Object2");
	const Vector2 pos = second.Get<Transform2D>()->position;
	const f32 scale = second.Get<Transform2D>()->scale.x;
	const f32 rotation = second.Get<Transform2D>()->rotation;

	f.world.SetPlayOnlySystemsEnabled(false);
	f.world.Progress(1.f);
	CHECK(second.Get<Transform2D>()->position.x == doctest::Approx(pos.x));
	CHECK(second.Get<Transform2D>()->scale.x == doctest::Approx(scale));
	CHECK(second.Get<Transform2D>()->rotation == doctest::Approx(rotation));
}

TEST_CASE("Example F spawn adds Lifetime with Lua defaults and _exit counts it") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	Entity follower = f.child(f.scenes.Current(), "Object1");
	const size_t before = follower.GetChildren().size();

	f.input.HandleKey(Key::F, KeyAction::Press);
	f.tick(0.f);

	const std::vector<Entity> children = follower.GetChildren();
	REQUIRE(children.size() == before + 1);
	Entity spawned;
	for (Entity child : children) {
		if (child.Has(f.type("Lifetime"))) {
			spawned = child;
		}
	}
	REQUIRE(spawned.IsAlive());
	CHECK(f.fieldF32(spawned, "Lifetime", "seconds") == doctest::Approx(6.f));
	CHECK(spawned.Has(f.type("Pulse")));
	CHECK_FALSE(spawned.Has(f.type("Velocity2D")));
	CHECK_FALSE(spawned.Find("Satellite").IsAlive());
	REQUIRE(spawned.Has<Sprite>());
	REQUIRE(spawned.Get<Transform2D>() != nullptr);
	const Vector2 pos = spawned.Get<Transform2D>()->position;
	const Color expected = Color::HSV(std::atan2(pos.y, pos.x) / Math::Pi * 180.f, 0.6f, 1.f);
	const Color modulate = spawned.Get<Sprite>()->modulate;
	CHECK(modulate.r == doctest::Approx(expected.r));
	CHECK(modulate.g == doctest::Approx(expected.g));
	CHECK(modulate.b == doctest::Approx(expected.b));
	CHECK(modulate.a == doctest::Approx(1.f));
	CHECK(modulate != Color());

	REQUIRE(f.scripts.DoString(R"(
		local root = scenes:current():root()
		local spawned = nil
		for _, child in ipairs(root:find("Object1"):children()) do
			if child:has("Lifetime") then
				spawned = child
			end
		end
		assert(spawned ~= nil)
		spawned:get("Lifetime").seconds = 0.05
		assert(root:get("Game")._expired == nil)
	)"));

	f.tick(0.1f);
	CHECK_FALSE(spawned.IsAlive());
	REQUIRE(f.scripts.DoString(R"(
		assert(scenes:current():root():get("Game")._expired == 1)
	)"));
}

TEST_CASE("Example Change loads alt.scene and Game _start runs on the new tree") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	f.input.HandleKey(Key::N, KeyAction::Press);
	f.tick(0.f);
	f.tick(0.f);

	Scene current = f.scenes.Current();
	REQUIRE(current.IsValid());
	CHECK(current.Name() == "Alt");
	CHECK(f.movers(current).size() == 2);
	CHECK(current.Root().Has(f.type("Game")));
	REQUIRE(f.scripts.DoString(R"(
		local game = scenes:current():root():get("Game")
		assert(game._prefab ~= nil)
		assert(game._expired == nil)
	)"));
	Entity first = f.movers(current)[0];
	Entity satellite = first.Find("Satellite");
	REQUIRE(satellite.IsAlive());
	CHECK(satellite.Get<RenderOrder>()->layer == 1);
}

TEST_CASE("Example Pulse _exit restores the scale captured in _start") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	Entity first = f.child(f.scenes.Current(), "Object1");
	Entity second = f.child(f.scenes.Current(), "Object2");
	f.parkMouseOn(first);
	f.tick(0.1f);
	CHECK(first.Get<Transform2D>()->scale.x != doctest::Approx(1.f));
	const f32 secondScale = second.Get<Transform2D>()->scale.x;
	REQUIRE(f.scripts.DoString(R"(
		scenes:current():root():find("Object1"):remove("Pulse")
	)"));
	CHECK(first.Get<Transform2D>()->scale.x == doctest::Approx(1.f));
	CHECK(second.Get<Transform2D>()->scale.x == doctest::Approx(secondScale));
	CHECK_FALSE(first.Has(f.type("Pulse")));
}

TEST_CASE("Example Stop restores the editor tree and drops ScriptLive") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	f.parkMouseOn(f.child(f.scenes.Current(), "Object1"));
	f.tick(0.1f);
	f.scenes.Stop();
	f.scenes.Flush();
	f.world.SetPlayOnlySystemsEnabled(false);
	CHECK_FALSE(f.scenes.IsPlaying());
	Scene restored = f.scenes.Current();
	REQUIRE(restored.IsValid());
	CHECK(restored.Name() == "Demo");
	Entity first = f.child(restored, "Object1");
	CHECK(first.Get<Transform2D>()->position.x == doctest::Approx(620.f));
	CHECK(first.Get<Transform2D>()->scale.x == doctest::Approx(1.f));
	CHECK(f.fieldF32(first, "Pulse", "amplitude") == doctest::Approx(0.15f));
	CHECK_FALSE(first.Raw().has<ScriptLive>(f.type("Pulse").Raw()));
}

TEST_CASE("Example ReloadModules keeps Bounce and Pulse running") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	f.play();
	REQUIRE(f.scripts.ReloadModules(f.fs));
	Entity second = f.child(f.scenes.Current(), "Object2");
	const Vector2 before = second.Get<Transform2D>()->position;
	f.tick(0.1f);
	CHECK(second.Get<Transform2D>()->position.x == doctest::Approx(before.x - 15.f));
	const f32 expected = 1.f + std::sin(0.1f * 4.f) * 0.08f;
	CHECK(second.Get<Transform2D>()->scale.x == doctest::Approx(expected));
}

TEST_CASE("Example M plays AudioSource on the mouse chaser") {
	ExampleFixture f;
	f.open("res://scenes/demo.scene");
	CHECK_FALSE(f.scenes.IsPlaying());
	Entity chaser = f.child(f.scenes.Current(), "Object1");
	REQUIRE(chaser.Has(f.type("MouseFollower")));
	REQUIRE(chaser.Has<AudioSource>());
	CHECK(chaser.Get<AudioSource>()->spatial);
	CHECK(chaser.Get<AudioSource>()->loop);
	CHECK_FALSE(f.scenes.Current().Root().Has<AudioSource>());

	f.input.HandleKey(Key::M, KeyAction::Press);
	f.tick(0.f);

	CHECK_FALSE(f.mixer.IsPlaying(chaser));
	f.input.HandleKey(Key::M, KeyAction::Release);
	f.tick(0.f);
	f.play();
	chaser = f.child(f.scenes.Current(), "Object1");
	f.input.HandleKey(Key::M, KeyAction::Press);
	f.tick(0.f);
	CHECK(f.mixer.IsPlaying(chaser));
	std::vector<f32> buffer(512 * 2, 0.f);
	f.mixer.Mix(buffer.data(), 512, 44100, 2);
	f32 sum = 0.f;
	for (f32 sample : buffer) {
		sum += sample * sample;
	}
	CHECK(sum > 0.f);
}

TEST_CASE("Example physics crates fall and the floor is static") {
	ExampleFixture f;
	Scene demo = f.open("res://scenes/demo.scene");
	Entity floor = f.child(demo, "Floor");
	REQUIRE(floor.Has<RigidBody2D>());
	CHECK(floor.Get<RigidBody2D>()->type == BodyType2D::Static);
	REQUIRE(floor.Has<Collider2D>());
	Entity crate = f.child(demo, "Crate1");
	REQUIRE(crate.Has<RigidBody2D>());
	REQUIRE(crate.Has<Collider2D>());
	CHECK(f.world.FindComponent("Crate").IsValid());
	CHECK(crate.Has(f.world.FindComponent("Crate")));
	CHECK_FALSE(crate.Has(f.type("Velocity2D")));

	f.play();
	crate = f.child(f.scenes.Current(), "Crate1");
	const f32 yBefore = crate.Get<Transform2D>()->position.y;
	f.tick(0.2f);
	CHECK(crate.Get<Transform2D>()->position.y < yBefore);
}

TEST_CASE("Example editor can save Object's first child while Demo instances have MouseFollower") {
	const auto directory =
		std::filesystem::temp_directory_path() / "sowa_example_reload_regression";
	std::filesystem::remove_all(directory);
	std::filesystem::copy(SOWA_EXAMPLE_DIR, directory, std::filesystem::copy_options::recursive);
	const auto cleanup = MakeScopeExit([&] { std::filesystem::remove_all(directory); });
	ExampleFixture f;
	f.fs.MountFolder("res", directory);
	Scene demo = f.open("res://scenes/demo.scene");
	Scene object = f.open("res://scenes/object.scene");
	REQUIRE(editor::ActivateSceneDocument(f.scenes, object));
	Entity instance = f.child(demo, "Object1");
	REQUIRE(instance.Has(f.type("MouseFollower")));
	Entity satellite = f.child(object, "Satellite");
	satellite.Get<Transform2D>()->position.x = 234;
	REQUIRE(SaveSceneToFile(f.fs, object.SourcePath(), object, f.resources) == FsError::Ok);
	f.fs.Events()->changed(object.SourcePath());
	CHECK_NOTHROW(f.scenes.Flush());
	CHECK(satellite.IsAlive());
	REQUIRE(instance.Has(f.type("MouseFollower")));
	CHECK(f.fieldF32(instance, "MouseFollower", "speed") == doctest::Approx(900));
	CHECK(instance.Find("Satellite").Get<Transform2D>()->position.x == doctest::Approx(234));
}

TEST_CASE("Parallax demo: top-down movement carries the camera and normalizes diagonals") {
	ExampleFixture f;
	f.resources.RegisterType<Font>("Font")
		.Field("path", &Font::path)
		.Field("pixelSize", &Font::pixelSize);
	f.resources.RegisterType<SpriteSheetAnimation>("SpriteSheetAnimation");
	f.open("res://scenes/parallax.scene");
	f.play();
	const Scene scene = f.scenes.Current();
	Entity player = f.child(scene, "Player");
	const Entity camera = player.Find("Camera");
	REQUIRE(camera.IsAlive());
	CHECK(scene.ActiveCamera() == camera);
	CHECK(f.child(scene, "BoundedLayer").Has<Parallax2D>());

	f.input.HandleKey(Key::D, KeyAction::Press);
	f.tick(0.5f);
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(200));
	CHECK(camera.Get<Transform2D>()->WorldPosition().x == doctest::Approx(200));

	player.Get<Transform2D>()->position = {0, 0};
	f.input.HandleKey(Key::W, KeyAction::Press);
	f.tick(0.5f);
	CHECK(player.Get<Transform2D>()->position.Length() == doctest::Approx(200));
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(141.42136f));
	CHECK(player.Get<Transform2D>()->position.y == doctest::Approx(141.42136f));

	f.input.HandleKey(Key::D, KeyAction::Release);
	f.input.HandleKey(Key::W, KeyAction::Release);
	f.input.HandleKey(Key::R, KeyAction::Press);
	f.tick(0.1f);
	CHECK(player.Get<Transform2D>()->position == Vector2(0, 0));
	CHECK(camera.Get<Transform2D>()->WorldPosition() == Vector2(0, 0));
}

TEST_CASE("Example Player moves with the project's WASD move actions") {
	ExampleFixture f;
	REQUIRE(f.settings.input.actions.size() >= 4);
	f.input.SetActions(f.settings.input.actions);
	f.open("res://scenes/demo.scene");
	f.play();

	Entity player = f.child(f.scenes.Current(), "Player");
	CHECK(player.Has(f.type("CharacterController")));
	const Vector2 start = player.Get<Transform2D>()->position;

	f.input.HandleKey(Key::D, KeyAction::Press);
	f.tick(0.5f);
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(start.x + 300.f));
	CHECK(player.Get<Transform2D>()->position.y == doctest::Approx(start.y));

	// Diagonals are normalized rather than faster.
	f.input.HandleKey(Key::W, KeyAction::Press);
	f.tick(0.5f);
	const f32 step = 300.f / std::sqrt(2.f);
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(start.x + 300.f + step));
	CHECK(player.Get<Transform2D>()->position.y == doctest::Approx(start.y + step));

	f.input.HandleKey(Key::D, KeyAction::Release);
	f.input.HandleKey(Key::W, KeyAction::Release);
	const Vector2 stopped = player.Get<Transform2D>()->position;
	f.tick(0.5f);
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(stopped.x));
	CHECK(player.Get<Transform2D>()->position.y == doctest::Approx(stopped.y));
}

TEST_CASE("Fixed gameplay demo buffers a frame input edge through zero ticks and consumes it once") {
	ExampleFixture f;
	f.open("res://scenes/fixed_gameplay.scene");
	f.play();
	Scene scene = f.scenes.Current();
	Entity root = scene.Root();
	Entity ship = f.child(scene, "Ship");
	const f32 tick = f.physics.Config().fixedDelta;
	f.input.HandleKey(Key::Space, KeyAction::Press);
	f.tick(tick * 0.25f);
	CHECK(f.fieldF32(root, "FixedGameplayDemo", "ticks") == 0.f);
	CHECK(f.fieldF32(root, "FixedGameplayDemo", "kicks") == 0.f);
	CHECK(f.physics.GetLinearVelocity(ship).y == 0.f);
	// The edge expires before a tick is due; the script retains the command.
	f.input.HandleKey(Key::Space, KeyAction::Release);
	f.tick(tick * 2.75f);
	CHECK(f.fieldF32(root, "FixedGameplayDemo", "ticks") == 3.f);
	CHECK(f.fieldF32(root, "FixedGameplayDemo", "kicks") == 1.f);
	CHECK(f.physics.GetLinearVelocity(ship).y > 100.f);
	const f32 previous = f.physics.GetLinearVelocity(ship).y;
	f.tick(tick);
	CHECK(f.fieldF32(root, "FixedGameplayDemo", "kicks") == 1.f);
	CHECK(f.physics.GetLinearVelocity(ship).y < previous); // Damping, no repeated impulse.
	f.input.HandleKey(Key::D, KeyAction::Press);
	f.tick(tick * 2.f);
	CHECK(f.physics.GetLinearVelocity(ship).x > 0.f);
	CHECK(f.child(scene, "Status").Get<Text2D>()->text.find("Physics ticks: 6") != std::string::npos);
}
