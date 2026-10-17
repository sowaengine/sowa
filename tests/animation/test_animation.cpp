#include <doctest/doctest.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <limits>

#include "animation/animation.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/animation_preview.hpp"
#include "editor/animation_timeline.hpp"
#include "editor/inspector.hpp"
#include "editor/scene_commands.hpp"
#include "editor/viewport.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"
#include <fstream>
#include <sstream>

using namespace sowa;

namespace {
AnimationTrack scalar(std::string field = "rotation", f32 from = 0, f32 to = 10) {
	AnimationTrack track;
	track.binding = {".", "Transform2D", std::move(field), FieldType::F32};
	track.Insert(0, from);
	track.Insert(1, to);
	return track;
}

struct Fixture {
	World world;
	ResourceRegistry resources;
	Ref<AnimationLibrary> library;
	Entity entity;

	Fixture() {
		resources.RegisterType<AnimationLibrary>("AnimationLibrary");
		library = resources.Create<AnimationLibrary>();
		entity = world.CreateEntity();
		entity.Add<Transform2D>();
		AnimationPlayer player;
		player.library = library->Id();
		player.currentAnimation = "move";
		entity.Add(player);
		library->clips.push_back({"move", 1, false, {scalar()}, {}});
	}

	f32 rotation() { return entity.Get<Transform2D>()->rotation; }

	void play(std::string name = "move") { REQUIRE(PlayAnimation(world, resources, entity, name)); }

	void advance(f64 delta, AnimationCallHandler handler = {}) {
		AdvanceAnimation(world, resources, entity, delta, handler);
	}
};

std::string saved(AnimationLibrary &lib) {
	auto a = Archive::Save();
	lib.Serialize(a);
	return a.ToString();
}
} // namespace

TEST_CASE("Animation keys keep identity through sorting, reject collisions and wrong types") {
	AnimationTrack t = scalar();
	auto id = t.Insert(.5, 3.f);
	REQUIRE(id != 0);
	REQUIRE(t.Update(id, .75, 4.f, KeyInterpolation::Smooth));
	CHECK(t.Keys()[1].id == id);
	CHECK_FALSE(t.Update(id, 1, 4.f, KeyInterpolation::Linear));
	CHECK(t.Insert(.75, 5.f) == id);
	CHECK(t.Insert(2, true) == 0);
	CHECK(t.Insert(-1, 1.f) == 0);
	CHECK(t.Insert(std::numeric_limits<double>::infinity(), 1.f) == 0);
	CHECK(t.Insert(2, std::numeric_limits<float>::quiet_NaN()) == 0);
	CHECK(t.Remove(id));
	CHECK_FALSE(t.Remove(id));
}

TEST_CASE("Animation interpolation supports smooth, step, shortest-angle and discrete values") {
	auto t = scalar();
	CHECK(std::get<f32>(*t.Sample(.25)) == doctest::Approx(2.5));
	auto id = t.Keys().front().id;
	REQUIRE(t.Update(id, 0, 0.f, KeyInterpolation::Smooth));
	CHECK(std::get<f32>(*t.Sample(.25)) == doctest::Approx(1.5625));
	REQUIRE(t.Update(id, 0, 0.f, KeyInterpolation::Step));
	CHECK(std::get<f32>(*t.Sample(.9)) == 0);
	CHECK(std::get<f32>(*t.Sample(1)) == 10);
	t = scalar("rotation", 350, 10);
	REQUIRE(t.Update(t.Keys()[0].id, 0, 350.f, KeyInterpolation::AngleDegrees));
	CHECK(std::get<f32>(*t.Sample(.5)) == doctest::Approx(360));
	AnimationTrack visible;
	visible.binding.type = FieldType::Bool;
	visible.Insert(0, true);
	visible.Insert(.5, false);
	CHECK(std::get<bool>(*visible.Sample(.49)));
	CHECK_FALSE(std::get<bool>(*visible.Sample(.5)));
	CHECK_FALSE(AnimationTrack().Sample(0));
}

TEST_CASE("Animation plays endpoints, pauses, seeks silently, holds the last pose") {
	Fixture f;
	f.entity.Get<Transform2D>()->rotation = 25;
	f.play();
	CHECK(f.rotation() == 0);
	f.advance(.5);
	CHECK(f.rotation() == doctest::Approx(5));
	PauseAnimation(f.entity);
	f.advance(1);
	CHECK(f.rotation() == doctest::Approx(5));
	REQUIRE(SeekAnimation(f.world, f.resources, f.entity, .25));
	CHECK(f.rotation() == doctest::Approx(2.5));
	PauseAnimation(f.entity, false);
	f.advance(4);
	CHECK(f.rotation() == 10);
	CHECK_FALSE(f.entity.Get<AnimationPlayer>()->isPlaying);
	StopAnimation(f.entity);
	CHECK(f.rotation() == 10);
	f.advance(1);
	CHECK(f.rotation() == 10);
}

TEST_CASE("Animation targets custom fields, colors, references and structural visibility safely") {
	Fixture f;
	auto result = f.world.RegisterComponent("Custom", {{"amount", FieldType::F64},
													   {"modulate", FieldType::Color},
													   {"label", FieldType::String},
													   {"target", FieldType::EntityRef},
													   {"texture", FieldType::ResourceRef}});
	auto custom = f.world.FindComponent("Custom");
	REQUIRE(custom.IsValid());
	Entity child = f.entity.CreateChild("Child");
	child.Add(custom);
	AnimationTrack amount;
	amount.binding = {"Child", "Custom", "amount", FieldType::F64};
	amount.Insert(0, 0.);
	amount.Insert(1, 20.);
	AnimationTrack color;
	color.binding = {"Child", "Custom", "modulate", FieldType::Color};
	color.Insert(0, Color(0, 0, 0, 0));
	color.Insert(1, Color(1, 1, 1, 1));
	AnimationTrack visible;
	visible.binding = {".", "", "isVisible", FieldType::Bool};
	visible.Insert(0, true);
	visible.Insert(.5, false);
	f.library->clips[0].tracks = {visible, amount, color};
	f.play();
	f.advance(.5);
	CHECK_FALSE(f.entity.IsVisible());
	CHECK(*child.GetField(custom, "amount").As<f64>() == 10);
	CHECK(*child.GetField(custom, "modulate").As<Color>() == Color(.5, .5, .5, .5));
	CHECK(WriteAnimationProperty(
		f.world, f.entity, {"Child", "Custom", "target", FieldType::EntityRef}, std::string(".")));
	CHECK(*child.GetField(custom, "target").As<Entity>() == f.entity);
	CHECK(WriteAnimationProperty(f.world, f.entity,
								 {"Child", "Custom", "texture", FieldType::ResourceRef},
								 ResourceID{123}));
	CHECK(WriteAnimationProperty(f.world, f.entity, {"Child", "Custom", "label", FieldType::String},
								 std::string("hello")));
	CHECK_FALSE(WriteAnimationProperty(f.world, f.entity,
									   {"Child", "Custom", "amount", FieldType::F32}, 1.f));
	child.Remove(custom);
	f.advance(.2); // missing targets skipped
	CHECK_FALSE(ReadAnimationProperty(f.world, f.entity, amount.binding));
	CHECK(ResolveAnimationTarget(child, "..") == f.entity);
}

TEST_CASE("Animation callbacks fire once per crossing in time order across multiple loops") {
	Fixture f;
	auto &clip = f.library->clips[0];
	clip.loop = true;
	clip.calls = {{.75, ".", "Test", "late"},
				  {0, ".", "Test", "zero"},
				  {.25, ".", "Test", "early"},
				  {1, ".", "Test", "end"}};
	std::vector<std::string> calls;
	auto handler = [&](Entity, const std::string &, const std::string &method) {
		calls.push_back(method);
	};
	f.play();
	f.advance(0, handler);
	CHECK(calls == std::vector<std::string>{"zero"});
	f.advance(0, handler);
	CHECK(calls.size() == 1);
	f.advance(2.25, handler);
	CHECK(calls == std::vector<std::string>{"zero", "early", "late", "end", "zero", "early", "late",
											"end", "zero", "early"});
	CHECK(f.entity.Get<AnimationPlayer>()->playback.time == doctest::Approx(.25));
	calls.clear();
	REQUIRE(SeekAnimation(f.world, f.resources, f.entity, .75));
	f.advance(0, handler);
	CHECK(calls.empty());
}

TEST_CASE("Animation callbacks may remove the player or destroy the target") {
	Fixture f;
	f.library->clips[0].calls = {
		{.1, ".", "Test", "remove"}, {.2, ".", "Test", "destroy"}, {.3, ".", "Test", "never"}};
	int calls = 0;
	f.play();
	f.advance(.5, [&](Entity target, const auto &, const auto &method) {
		++calls;
		if (method == "remove") {
			target.Remove<AnimationPlayer>();
		} else {
			target.Destroy();
		}
	});
	CHECK(calls == 2);
	CHECK_FALSE(f.entity.IsAlive());
}

TEST_CASE("Animation system honors play-only, disabled entities and applies before transforms") {
	Fixture f;
	f.entity.Get<AnimationPlayer>()->autoplay = true;
	RegisterAnimationSystem(f.world, f.resources);
	f.world.SetPlayOnlySystemsEnabled(false);
	f.world.Progress(.5);
	CHECK(f.rotation() == 0);
	f.world.SetPlayOnlySystemsEnabled(true);
	f.world.Progress(.5);
	CHECK(f.rotation() == doctest::Approx(5));
	CHECK(f.entity.Get<Transform2D>()->WorldRotation() == doctest::Approx(5));
	f.entity.SetEnabled(false);
	f.world.Progress(.2);
	CHECK(f.rotation() == doctest::Approx(5));
	f.entity.SetEnabled(true);
	f.world.Progress(.5);
	CHECK(f.rotation() == 10);
}

TEST_CASE("Animation player clocks are independent and invalid input cannot poison playback") {
	Fixture f;
	Entity other = f.entity.Duplicate();
	f.play();
	REQUIRE(PlayAnimation(f.world, f.resources, other, "move"));
	f.advance(.5);
	CHECK(other.Get<AnimationPlayer>()->playback.time == 0);
	f.advance(-1);
	f.advance(std::numeric_limits<double>::quiet_NaN());
	CHECK(f.rotation() == 5);
	CHECK_FALSE(PlayAnimation(f.world, f.resources, f.entity, "missing"));
	CHECK_FALSE(SeekAnimation(f.world, f.resources, f.entity, -1));
}

TEST_CASE("Animation scene serialization restores authored libraries but never runtime clocks") {
	Fixture f;
	Scene scene = CreateScene(f.world, "Animations");
	f.entity.SetName("Player");
	f.entity.SetParent(scene.Root());
	REQUIRE(scene.Own(f.resources, f.library->Id()));
	f.entity.Get<AnimationPlayer>()->autoplay = true;
	f.library->clips[0].calls = {{.5, ".", "Custom", "ding"}};
	f.play();
	f.advance(.3);
	auto archive = Archive::Save();
	REQUIRE(SaveScene(archive, scene, f.resources));
	World world;
	ResourceRegistry resources;
	resources.RegisterType<AnimationLibrary>("AnimationLibrary");
	auto load = Archive::Load(archive.ToString());
	Scene loaded = LoadScene(load, world, resources);
	REQUIRE(loaded.IsValid());
	CHECK_FALSE(load.HasErrors());
	Entity player = loaded.Root().Find("Player");
	REQUIRE(player.Has<AnimationPlayer>());
	CHECK(player.Get<AnimationPlayer>()->playback.time == 0);
	CHECK_FALSE(player.Get<AnimationPlayer>()->playback.initialized);
	auto library = resources.Get<AnimationLibrary>(player.Get<AnimationPlayer>()->library);
	REQUIRE(library);
	CHECK(saved(*library) == saved(*f.library));
	CHECK(library->clips[0].tracks[0].Keys().size() == 2);
}

TEST_CASE("Animation library validates malformed duration and key times") {
	AnimationLibrary library;
	auto a = Archive::Load(R"(clips:
  - name: bad
    duration: 0
    tracks:
      - type: F32
        keys:
          - time: -1
            value: 3
)");
	library.Serialize(a);
	CHECK(a.HasErrors());
	CHECK(library.clips[0].duration == 1);
	CHECK(library.clips[0].tracks[0].Keys().empty());
}

TEST_CASE("Animation authoring undo restores keys and player attachment") {
	Fixture f;
	Scene scene = CreateScene(f.world, "Editing");
	f.entity.SetParent(scene.Root());
	REQUIRE(scene.Own(f.resources, f.library->Id()));
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	auto before = saved(*f.library);
	auto player = *f.entity.Get<AnimationPlayer>();
	f.library->clips[0].tracks[0].Insert(.5, 42.f);
	auto after = saved(*f.library);
	REQUIRE(editor::RecordAnimationEdit(states, scene, f.entity, f.resources, f.library->Id(),
										std::nullopt, player, before, after));
	CHECK(editor::IsSceneDocumentUnsaved(states, scene));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK(saved(*f.library) == before);
	CHECK_FALSE(f.entity.Has<AnimationPlayer>());
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(saved(*f.library) == after);
	CHECK(f.entity.Has<AnimationPlayer>());
}

TEST_CASE("Animation Lua API controls playback, custom fields and no-argument methods") {
	Fixture f;
	ScriptServer scripts(f.world);
	Input input;
	FileSystem fs;
	SceneManager scenes(f.world, f.resources, fs);
	ProjectSettings settings;
	scripts.BindEngine(input, scenes, f.resources, settings);
	REQUIRE(scripts.DoString(R"(
        local Effect = world:component("Effect", { amount = 0, calls = 0 })
        function Effect:hit() self.calls = self.calls + 1 end
    )"));
	auto custom = f.world.FindComponent("Effect");
	f.entity.Add(custom);
	scripts.SetEntity("player", f.entity);
	AnimationTrack track;
	track.binding = {".", "Effect", "amount", FieldType::F32};
	track.Insert(0, 0.f);
	track.Insert(1, 100.f);
	f.library->clips[0].tracks.push_back(track);
	f.library->clips[0].calls = {{.5, ".", "Effect", "hit"}};
	REQUIRE(scripts.DoString(
		"assert(animation.play(player, 'move')); assert(animation.is_playing(player))"));
	f.advance(.5, [&](Entity e, const auto &c, const auto &m) {
		REQUIRE(scripts.CallComponentMethod(e, c, m));
	});
	REQUIRE(scripts.DoString(
		"assert(player:get('Effect').amount == 50); assert(player:get('Effect').calls == 1); "
		"animation.pause(player); assert(not animation.is_playing(player)); "
		"assert(animation.seek(player, 0.7)); animation.stop(player)"));
	CHECK_FALSE(scripts.CallComponentMethod(f.entity, "Effect", "absent"));
}

TEST_CASE("Animation system can hide and remove components during deferred dispatch") {
	Fixture f;
	AnimationTrack visibility;
	visibility.binding = {".", "", "isVisible", FieldType::Bool};
	visibility.Insert(0, false);
	f.library->clips[0].tracks.push_back(visibility);
	f.library->clips[0].calls = {{.1, ".", "Test", "remove"}};
	f.entity.Get<AnimationPlayer>()->autoplay = true;
	RegisterAnimationSystem(f.world, f.resources, [](Entity e, const auto &, const auto &) {
		e.Remove<AnimationPlayer>();
	});
	f.world.Progress(.5);
	CHECK_FALSE(f.entity.IsVisible());
	CHECK_FALSE(f.entity.Has<AnimationPlayer>());
}

TEST_CASE("Animation callback collection is bounded for huge deltas") {
	Fixture f;
	f.library->clips[0].loop = true;
	f.library->clips[0].calls = {{0, ".", "Test", "tick"}};
	f.play();
	size_t count = 0;
	f.advance(1e100, [&](Entity, const auto &, const auto &) { ++count; });
	CHECK(count == 4096);
	CHECK(std::isfinite(f.entity.Get<AnimationPlayer>()->playback.time));
}

TEST_CASE("Animation example scene loads cleanly and loops the current animation") {
	Fixture f;
	f.resources.RegisterType<ImageTexture>("ImageTexture")
		.Field("path", &ImageTexture::path)
		.Field("flipVertically", &ImageTexture::flipVertically)
		.Field("filter", &ImageTexture::filter);
	f.world.RegisterComponent<Sprite>("Sprite")
		.Field("texture", &Sprite::texture)
		.Field("modulate", &Sprite::modulate)
		.Field("size", &Sprite::size)
		.Field("centered", &Sprite::centered);
	std::ifstream file(std::string(SOWA_EXAMPLE_DIR) + "/scenes/animation.scene");
	std::stringstream content;
	content << file.rdbuf();
	auto archive = Archive::Load(content.str());
	auto scene = LoadScene(archive, f.world, f.resources);
	REQUIRE(scene.IsValid());
	CHECK_FALSE(archive.HasErrors());
	Entity player = scene.Root().Find("AnimatedIcon");
	REQUIRE(player.Has<AnimationPlayer>());
	AdvanceAnimation(f.world, f.resources, player, 1);
	CHECK(player.Get<Transform2D>()->position == Vector2(960, 740));
	AdvanceAnimation(f.world, f.resources, player, 1);
	AdvanceAnimation(f.world, f.resources, player, 2);
}

TEST_CASE("Animation undo of library creation restores scene ownership and retains redo data") {
	Fixture f;
	Scene scene = CreateScene(f.world, "Ownership");
	f.entity.SetParent(scene.Root());
	REQUIRE(scene.Own(f.resources, f.library->Id()));
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	auto player = *f.entity.Get<AnimationPlayer>();
	auto data = saved(*f.library);
	REQUIRE(editor::RecordAnimationEdit(states, scene, f.entity, f.resources, f.library->Id(),
										std::nullopt, player, data, data, true));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK_FALSE(scene.Owns(f.library->Id()));
	CHECK(f.resources.Get<AnimationLibrary>(f.library->Id()) != nullptr);
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(scene.Owns(f.library->Id()));
	REQUIRE(editor::UndoSceneDocument(states, scene));
	states.clear();
	CHECK(f.resources.Get<AnimationLibrary>(f.library->Id()) == nullptr);
}

TEST_CASE("Animation values round-trip every reflected storage type") {
	std::vector<std::pair<FieldType, AnimationValue>> values = {
		{FieldType::Bool, true},
		{FieldType::I32, i32(-10)},
		{FieldType::U32, u32(42)},
		{FieldType::F32, 1.5f},
		{FieldType::F64, 2.5},
		{FieldType::String, std::string("text")},
		{FieldType::EntityRef, std::string("../Target")},
		{FieldType::Vector2, Vector2(1, 2)},
		{FieldType::Vector3, Vector3(1, 2, 3)},
		{FieldType::Color, Color(.25, .5, .75, 1)},
		{FieldType::Enum, i32(7)},
		{FieldType::ResourceRef, ResourceID{1234}}};
	for (auto &[type, value] : values) {
		CAPTURE(static_cast<int>(type));
		AnimationTrack track;
		track.binding.type = type;
		REQUIRE(track.Insert(0, value) != 0);
		auto save = Archive::Save();
		track.Serialize(save);
		auto load = Archive::Load(save.ToString());
		AnimationTrack restored;
		restored.Serialize(load);
		CHECK_FALSE(load.HasErrors());
		REQUIRE(restored.Sample(0));
		CHECK(*restored.Sample(0) == value);
	}
	CHECK(std::get<i32>(InterpolateAnimationValues(i32(0), i32(10), .25)) == 3);
	CHECK(std::get<u32>(InterpolateAnimationValues(u32(0), u32(10), .25)) == 3);
	CHECK(std::get<i32>(InterpolateAnimationValues(i32(1), i32(10),
												   std::numeric_limits<double>::quiet_NaN())) == 1);
}

TEST_CASE(
	"Animation player has a named current animation, autoplay toggle and exposed playing flag") {
	Fixture f;
	const auto type = f.world.FindComponent("AnimationPlayer");
	CHECK(type.FindField("currentAnimation")->type == FieldType::String);
	CHECK(type.FindField("autoplay")->type == FieldType::Bool);
	CHECK(type.FindField("isPlaying")->type == FieldType::Bool);
	CHECK(type.FindField("isPlaying")->IsExposed());
	CHECK_FALSE(type.FindField("isPlaying")->IsSerialized());
	CHECK_FALSE(type.FindField("library")->IsExposed());
	f.advance(.5);
	CHECK(f.rotation() == 0); // merely selecting an animation does not start it
	f.entity.Get<AnimationPlayer>()->isPlaying = true;
	f.advance(.5);
	CHECK(f.rotation() == doctest::Approx(5));
}

TEST_CASE("Animation autoplay starts the selected clip once") {
	Fixture f;
	f.entity.Get<AnimationPlayer>()->autoplay = true;
	f.advance(.5);
	CHECK(f.rotation() == doctest::Approx(5));
	StopAnimation(f.entity);
	f.advance(.5);
	CHECK_FALSE(f.entity.Get<AnimationPlayer>()->isPlaying);
	CHECK(f.rotation() == doctest::Approx(5));
}

TEST_CASE("Animation preview renders a pose then restores authored values before saving") {
	Fixture f;
	f.entity.Get<Transform2D>()->rotation = 75;
	editor::AnimationPreview preview;
	preview.Select(f.entity);
	f.entity.Get<AnimationPlayer>()->isPlaying = true;
	preview.Apply(f.world, f.resources, .5, {800, 600});
	CHECK(f.rotation() == doctest::Approx(5));
	CHECK(f.entity.Get<AnimationPlayer>()->playback.time == 0);
	preview.Restore(f.world);
	CHECK(f.rotation() == 75);
	preview.Seek(0);
	preview.Apply(f.world, f.resources, 0, {800, 600});
	CHECK(f.rotation() == 0);
	preview.Restore(f.world);
	CHECK(f.rotation() == 75);
	preview.Stop();
	CHECK_FALSE(f.entity.Get<AnimationPlayer>()->isPlaying);
	CHECK_FALSE(preview.IsActive());
}

TEST_CASE("Animation preview restores visibility and never invokes method keys") {
	Fixture f;
	AnimationTrack visibility;
	visibility.binding = {".", "", "isVisible", FieldType::Bool};
	visibility.Insert(0, false);
	f.library->clips[0].tracks.push_back(visibility);
	f.library->clips[0].calls = {{0, ".", "Test", "destroy"}};
	editor::AnimationPreview preview;
	preview.Select(f.entity);
	preview.Seek(.5);
	preview.Apply(f.world, f.resources, 0, {800, 600});
	CHECK_FALSE(f.entity.IsVisible());
	preview.Restore(f.world);
	CHECK(f.entity.IsVisible());
	CHECK(f.entity.IsAlive());
	Entity another = f.world.CreateEntity();
	f.entity.Get<AnimationPlayer>()->isPlaying = true;
	preview.Select(another);
	CHECK_FALSE(f.entity.Get<AnimationPlayer>()->isPlaying);
}

TEST_CASE("Animation targets a sibling camera in game playback and editor preview") {
	Fixture f;
	Scene scene = CreateScene(f.world, "CameraAnimation");
	f.entity.SetParent(scene.Root());
	Entity camera = scene.CreateEntity("Camera");
	camera.Add<Transform2D>();
	camera.Get<Transform2D>()->position = {960, 540};
	camera.Add<Camera2D>();
	camera.Get<Camera2D>()->rotatable = true;
	scene.SetActiveCamera(camera);

	auto rotation = scalar("rotation", 0, 90);
	rotation.binding.path = "../Camera";
	AnimationTrack position;
	position.binding = {"../Camera", "Transform2D", "position", FieldType::Vector2};
	position.Insert(0, Vector2(960, 540));
	position.Insert(1, Vector2(0, 0));
	f.library->clips[0].tracks = {rotation, position};
	REQUIRE(ResolveAnimationTarget(f.entity, "../Camera") == camera);

	const Vector2 viewport(1920, 1080);
	const auto authoredView = CalculateViewProjection(camera, viewport);
	editor::AnimationPreview preview;
	bool previewing = false;
	SUBCASE("Autoplay updates the active camera before rendering") {
		f.entity.Get<AnimationPlayer>()->autoplay = true;
		RegisterAnimationSystem(f.world, f.resources);
		f.world.Progress(.5);
	}
	SUBCASE("Editor playback retains the animated camera outline after restoration") {
		previewing = true;
		preview.Select(f.entity);
		f.entity.Get<AnimationPlayer>()->isPlaying = true;
		preview.Apply(f.world, f.resources, .5, viewport);
		f.world.Progress(0);
	}
	CHECK(camera.Get<Transform2D>()->position == Vector2(480, 270));
	CHECK(camera.Get<Transform2D>()->rotation == doctest::Approx(45));
	CHECK(camera.Get<Transform2D>()->WorldPosition() == Vector2(480, 270));
	CHECK(camera.Get<Transform2D>()->WorldRotation() == doctest::Approx(45));
	const auto animatedView = CalculateViewProjection(scene.ActiveCamera(), viewport);
	CHECK(animatedView != authoredView);
	const auto center = animatedView * glm::vec4(480, 270, 0, 1);
	CHECK(center.x == doctest::Approx(0).epsilon(.00001));
	CHECK(center.y == doctest::Approx(0).epsilon(.00001));
	if (previewing) {
		preview.Restore(f.world);
		CHECK(camera.Get<Transform2D>()->position == Vector2(960, 540));
		CHECK(camera.Get<Transform2D>()->rotation == 0);
		CHECK(CalculateViewProjection(camera, viewport) == authoredView);
		CHECK(editor::ViewportWorldCorners(preview.CameraViewProjection(camera, viewport)) ==
			  editor::ViewportWorldCorners(animatedView));
		preview.Stop();
		CHECK(preview.CameraViewProjection(camera, viewport) == authoredView);
	}
}

TEST_CASE("Animation preview camera outlines include animated parents and camera settings") {
	Fixture f;
	Entity camera = f.world.CreateEntity();
	camera.SetName("Camera");
	camera.SetParent(f.entity);
	camera.Add<Transform2D>();
	camera.Get<Transform2D>()->position = {100, 0};
	camera.Add<Camera2D>();
	camera.Get<Camera2D>()->rotatable = true;
	AnimationTrack offset;
	offset.binding = {"Camera", "Camera2D", "offset", FieldType::Vector2};
	offset.Insert(0, Vector2(20, 30));
	f.library->clips[0].tracks.push_back(offset);
	const Vector2 viewport(800, 600);
	const auto authoredView = CalculateViewProjection(camera, viewport);
	editor::AnimationPreview preview;
	preview.Select(f.entity);
	preview.Seek(.5);
	preview.Apply(f.world, f.resources, 0, viewport);
	const auto animatedView = CalculateViewProjection(camera, viewport);
	CHECK(animatedView != authoredView);
	preview.Restore(f.world);
	CHECK(preview.CameraViewProjection(camera, viewport) == animatedView);
	CHECK(CalculateViewProjection(camera, viewport) == authoredView);
	preview.Select(camera);
	CHECK(preview.CameraViewProjection(camera, viewport) == authoredView);
}

TEST_CASE("Add Component lists native and custom components without internal types") {
	Fixture f;
	ScriptServer scripts(f.world);
	REQUIRE(scripts.DoString("world:component('Inventory', { count = 7 })"));
	auto available = editor::AddableComponents(f.world, f.entity);
	std::vector<std::string> names;
	for (auto type : available) {
		names.push_back(type.Name());
	}
	CHECK(std::find(names.begin(), names.end(), "Inventory") != names.end());
	CHECK(std::find(names.begin(), names.end(), "Tilemap") != names.end());
	CHECK(std::find(names.begin(), names.end(), "Transform2D") == names.end());
	CHECK(std::find(names.begin(), names.end(), "SceneRoot") == names.end());
	CHECK(std::find(names.begin(), names.end(), "Hidden") == names.end());
}

TEST_CASE("Add Component preserves script defaults and undoes required transforms together") {
	Fixture f;
	ScriptServer scripts(f.world);
	REQUIRE(scripts.DoString("world:component('Inventory', { count = 7 })"));
	Scene scene = CreateScene(f.world, "Components");
	Entity target = scene.CreateEntity("Target");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {scene});
	auto inventory = f.world.FindComponent("Inventory");
	REQUIRE(editor::ExecuteAddComponent(states, scene, target, inventory, scripts));
	CHECK(*target.GetField(inventory, "count").As<f32>() == 7);
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK_FALSE(target.Has(inventory));
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(*target.GetField(inventory, "count").As<f32>() == 7);
	REQUIRE(editor::ExecuteAddComponent(states, scene, target, f.world.FindComponent("Tilemap"),
										scripts));
	CHECK(target.Has<Tilemap>());
	CHECK(target.Has<Transform2D>());
	REQUIRE(editor::UndoSceneDocument(states, scene));
	CHECK_FALSE(target.Has<Tilemap>());
	CHECK_FALSE(target.Has<Transform2D>());
	REQUIRE(editor::RedoSceneDocument(states, scene));
	CHECK(target.Has<Tilemap>());
	CHECK(target.Has<Transform2D>());
}

TEST_CASE("Animation timeline selects, drags and scrubs keys with ImGui input") {
	Fixture f;
	editor::AnimationTimeline timeline;
	auto *context = ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() { ImGui::DestroyContext(); }
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {1100, 600};
	io.DeltaTime = 1.f / 60;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	f64 cursor = 0;
	auto frame = [&] {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({1000, 500});
		ImGui::Begin("TimelineTest", nullptr, ImGuiWindowFlags_NoTitleBar);
		const bool changed = timeline.Draw(f.world, f.entity, f.library->clips[0], cursor);
		ImGui::End();
		ImGui::Render();
		return changed;
	};
	frame();
	frame();
	ImGuiWindow *child = nullptr;
	for (auto *window : context->Windows) {
		if (window->ParentWindow && std::string(window->ParentWindow->Name) == "TimelineTest") {
			child = window;
		}
	}
	REQUIRE(child != nullptr);
	const ImVec2 origin = child->DC.CursorStartPos;
	const float timelineWidth = child->ContentSize.x - 220 - 16;
	const ImVec2 keyEnd(origin.x + 220 + timelineWidth, origin.y + 45);
	io.AddMousePosEvent(keyEnd.x, keyEnd.y);
	frame();
	io.AddMouseButtonEvent(0, true);
	frame();
	CHECK(cursor == doctest::Approx(1));
	io.AddMousePosEvent(origin.x + 220 + timelineWidth / 2, keyEnd.y);
	frame();
	io.AddMouseButtonEvent(0, false);
	CHECK(frame());
	CHECK(f.library->clips[0].tracks[0].Keys().back().time == doctest::Approx(.5));
	io.AddMousePosEvent(origin.x + 220, origin.y + 10);
	frame();
	io.AddMouseButtonEvent(0, true);
	frame();
	CHECK(cursor == 0);
	CHECK(timeline.DidScrub());
	io.AddMouseButtonEvent(0, false);
	frame();
}

TEST_CASE("Animation resource keys retain newly created resources across undo and redo") {
	Fixture fixture;
	fixture.resources.RegisterType<ImageTexture>("ImageTexture");
	Scene scene = CreateScene(fixture.world, "Resource keys");
	fixture.entity.SetParent(scene.Root());
	REQUIRE(scene.Own(fixture.resources, fixture.library->Id()));
	std::vector<editor::SceneDocumentSaveState> documents;
	editor::UpdateSceneDocumentStates(documents, {scene});
	const auto before = saved(*fixture.library);
	const auto texture = fixture.resources.Create<ImageTexture>();
	const auto textureId = texture->Id();
	REQUIRE(scene.Own(fixture.resources, textureId));
	AnimationTrack track;
	track.binding = {".", "Sprite", "texture", FieldType::ResourceRef};
	track.Insert(0, textureId);
	fixture.library->clips.front().tracks.push_back(track);
	const auto player = *fixture.entity.Get<AnimationPlayer>();
	REQUIRE(editor::RecordAnimationEdit(documents,
		scene,
		fixture.entity,
		fixture.resources,
		fixture.library->Id(),
		player,
		player,
		before,
		saved(*fixture.library),
		false,
		textureId));
	REQUIRE(editor::UndoSceneDocument(documents, scene));
	CHECK(saved(*fixture.library) == before);
	CHECK_FALSE(scene.Owns(textureId));
	CHECK(fixture.resources.Has(textureId));
	REQUIRE(editor::RedoSceneDocument(documents, scene));
	CHECK(scene.Owns(textureId));
	CHECK(std::get<ResourceID>(fixture.library->clips.front().tracks.back().Keys().front().value) ==
		  textureId);
	REQUIRE(editor::UndoSceneDocument(documents, scene));
	documents.clear();
	CHECK_FALSE(fixture.resources.Has(textureId));
}
