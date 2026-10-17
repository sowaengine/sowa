#include <doctest/doctest.h>
#include <limits>

#include "animation/animated_sprite.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/visibility.hpp"
#include "ecs/world.hpp"
#include "editor/resource_inspector.hpp"
#include "editor/scene_commands.hpp"
#include "render/sprite_renderer2d.hpp"
#include "resource/image_texture.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

using namespace sowa;

namespace {
struct SpriteFixture {
	World world;
	ResourceRegistry resources;
	Ref<SpriteSheetAnimation> library;
	Ref<ImageTexture> texture;

	SpriteFixture() {
		resources.RegisterType<SpriteSheetAnimation>("SpriteSheetAnimation");
		resources.RegisterType<ImageTexture>("ImageTexture");
		texture = resources.Create<ImageTexture>();
		texture->Adopt(resources.Context(), TextureHandle{17, 1}, 74, 38);
		library = resources.Create<SpriteSheetAnimation>();
		SpriteAnimation run;
		run.texture = texture->Id();
		run.columns = 4;
		run.rows = 2;
		run.paddingX = run.paddingY = 2;
		run.spacingX = run.spacingY = 2;
		run.frames = {4, 1, 7, 1};
		run.framesPerSecond = 4;
		library->Set("Running", run);
		run.loop = false;
		library->Set("Once", run);
	}

	AnimatedSprite Make() {
		AnimatedSprite sprite;
		sprite.animations = library->Id();
		sprite.currentAnimation = "Running";
		return sprite;
	}
};
} // namespace

TEST_CASE("AnimatedSprite playback loops independently and stops on the last nonlooping frame") {
	SpriteFixture f;
	auto first = f.Make(), second = f.Make();
	AdvanceAnimatedSprite(first, f.resources, .625);
	CHECK(first.frame == 2);
	CHECK(first.isPlaying);
	AdvanceAnimatedSprite(second, f.resources, .25);
	CHECK(second.frame == 1);
	AdvanceAnimatedSprite(first, f.resources, 100.5);
	CHECK(first.frame == 0);
	CHECK(first.playback.time == doctest::Approx(.125));
	first.currentAnimation = "Once";
	AdvanceAnimatedSprite(first, f.resources, 5);
	CHECK(first.frame == 3);
	CHECK_FALSE(first.isPlaying);
	CHECK(second.frame == 1);
}

TEST_CASE("AnimatedSprite playback respects autoplay, speed, pause, stop and invalid clips") {
	SpriteFixture f;
	auto entity = f.world.CreateEntity();
	auto sprite = f.Make();
	sprite.autoplay = false;
	sprite.speed = 2;
	entity.Add(sprite);
	AdvanceAnimatedSprite(*entity.Get<AnimatedSprite>(), f.resources, .5);
	CHECK(entity.Get<AnimatedSprite>()->frame == 0);
	REQUIRE(PlayAnimatedSprite(entity, f.resources, "Running"));
	auto &player = *entity.Get<AnimatedSprite>();
	AdvanceAnimatedSprite(player, f.resources, .25);
	CHECK(player.frame == 2);
	player.isPlaying = false;
	AdvanceAnimatedSprite(player, f.resources, 5);
	CHECK(player.frame == 2);
	CHECK_FALSE(PlayAnimatedSprite(entity, f.resources, "Missing"));
	CHECK(player.currentAnimation == "Running");
	StopAnimatedSprite(entity);
	CHECK(player.frame == 0);
	AdvanceAnimatedSprite(player, f.resources, 1);
	CHECK_FALSE(player.isPlaying);
	player.currentAnimation = "Missing";
	player.isPlaying = true;
	AdvanceAnimatedSprite(player, f.resources, 1);
	CHECK_FALSE(player.isPlaying);
	player = f.Make();
	AdvanceAnimatedSprite(player, f.resources, std::numeric_limits<double>::infinity());
	CHECK(player.frame == 0);
	player.speed = std::numeric_limits<float>::quiet_NaN();
	AdvanceAnimatedSprite(player, f.resources, 1);
	CHECK(player.frame == 0);
}

TEST_CASE("AnimatedSprite renders the selected padded cell at cell size and switches textures") {
	SpriteFixture f;
	SpriteRenderer2D renderer;
	renderer.Init(f.world, f.resources);
	f.world.SetPlayOnlySystemsEnabled(false);
	auto entity = f.world.CreateEntity();
	entity.Add<Transform2D>();
	entity.Add(f.Make());
	f.world.Progress(.5);
	CHECK(entity.Get<AnimatedSprite>()->frame == 0);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 1);
	const auto item = renderer.DrawList().front();
	CHECK(item.texture.id == 17);
	CHECK(item.localRect.w == doctest::Approx(16));
	CHECK(item.localRect.h == doctest::Approx(16));
	CHECK(item.localRect.x == doctest::Approx(-8));
	CHECK(item.uvRect.x == doctest::Approx(2. / 74));
	CHECK(item.uvRect.y == doctest::Approx(2. / 38));
	f.world.SetPlayOnlySystemsEnabled(true);
	f.world.Progress(.25);
	CHECK(entity.Get<AnimatedSprite>()->frame == 1);
	auto other = f.resources.Create<ImageTexture>();
	other->Adopt(f.resources.Context(), TextureHandle{18, 1}, 16, 16);
	SpriteAnimation idle;
	idle.texture = other->Id();
	idle.frames = {0};
	f.library->Set("Idle", idle);
	REQUIRE(PlayAnimatedSprite(entity, f.resources, "Idle"));
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 1);
	CHECK(renderer.DrawList()[0].texture.id == 18);
	entity.Add<Hidden>();
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
}

TEST_CASE("Sprite grid padding, texture orientation and malformed frames are handled") {
	SpriteFixture f;
	auto clip = *f.library->Find("Running");
	const auto top = clip.CellRect(0, f.texture->Size());
	CHECK(top.x == 2);
	CHECK(top.y == 2);
	CHECK(top.w == 16);
	CHECK(top.h == 16);
	const auto uv = clip.FrameUV(0, f.texture->Size(), false);
	CHECK(uv.h == doctest::Approx(-16. / 38));
	CHECK(uv.Top() == doctest::Approx(20. / 38));
	clip.paddingX = clip.paddingY = 100;
	CHECK(clip.FrameUV(0, f.texture->Size()).w == 0);
	clip.paddingX = clip.paddingY = 2;
	clip.frames = {8};
	CHECK_FALSE(clip.IsValid());
	CHECK(clip.FrameUV(0, f.texture->Size()).w == 0);
	clip.columns = std::numeric_limits<int>::max();
	clip.rows = 2;
	clip.frames = {0};
	CHECK(clip.CellRect(0, f.texture->Size()).w == 0);
}

TEST_CASE(
	"AnimatedSprite scene saves authored settings and multiple sheets without runtime state") {
	SpriteFixture f;
	auto scene = CreateScene(f.world, "Sprites");
	scene.Own(f.resources, f.library->Id());
	scene.Own(f.resources, f.texture->Id());
	auto entity = scene.CreateEntity("Runner");
	entity.Add(f.Make());
	entity.Add<Transform2D>();
	AdvanceAnimatedSprite(*entity.Get<AnimatedSprite>(), f.resources, .75);
	auto archive = Archive::Save();
	REQUIRE(SerializeComponent(archive, entity, f.world.FindComponent("AnimatedSprite")));
	CHECK(archive.ToString().find("isPlaying") == std::string::npos);
	CHECK(archive.ToString().find("frame:") == std::string::npos);
	auto copy = f.world.CreateEntity();
	auto load = Archive::Load(archive.ToString());
	REQUIRE(SerializeComponent(load, copy, f.world.FindComponent("AnimatedSprite")));
	CHECK(copy.Get<AnimatedSprite>()->currentAnimation == "Running");
	CHECK(copy.Get<AnimatedSprite>()->frame == 0);
	CHECK_FALSE(copy.Get<AnimatedSprite>()->playback.initialized);
	auto data = editor::SaveResource(*f.library);
	auto resourceLoad = Archive::Load(data);
	auto restored = f.resources.Create<SpriteSheetAnimation>();
	restored->Serialize(resourceLoad);
	REQUIRE_FALSE(resourceLoad.HasErrors());
	REQUIRE(restored->Find("Running"));
	CHECK(restored->Find("Running")->paddingX == 2);
	CHECK(restored->Find("Running")->spacingX == 2);
	CHECK(restored->Find("Running")->frames == std::vector<i32>{4, 1, 7, 1});
	CHECK(restored->Count() == 2);
	auto savedScene = Archive::Save();
	REQUIRE(SaveScene(savedScene, scene, f.resources));
	World target;
	ResourceRegistry targetResources;
	targetResources.RegisterType<SpriteSheetAnimation>("SpriteSheetAnimation");
	targetResources.RegisterType<ImageTexture>("ImageTexture");
	auto sceneLoad = Archive::Load(savedScene.ToString());
	const auto restoredScene = LoadScene(sceneLoad, target, targetResources);
	REQUIRE(restoredScene.IsValid());
	REQUIRE_FALSE(sceneLoad.HasErrors());
	const auto runner = target.Find("Sprites/Runner");
	REQUIRE(runner.IsAlive());
	const auto *restoredSprite = runner.Get<AnimatedSprite>();
	REQUIRE(restoredSprite);
	CHECK(restoredSprite->frame == 0);
	const auto restoredLibrary =
		targetResources.Get<SpriteSheetAnimation>(restoredSprite->animations);
	REQUIRE(restoredLibrary);
	REQUIRE(restoredLibrary->Find("Running"));
	CHECK(restoredLibrary->Find("Running")->frames == std::vector<i32>{4, 1, 7, 1});
	CHECK(targetResources.Has(restoredLibrary->Find("Running")->texture));
}

TEST_CASE("Sprite sheet edits undo and redo animation data, selection and resource ownership") {
	SpriteFixture f;
	auto scene = CreateScene(f.world, "Sprites");
	std::vector<editor::SceneDocumentSaveState> documents;
	editor::UpdateSceneDocumentStates(documents, {scene});
	auto entity = scene.Root();
	entity.Add<AnimatedSprite>();
	const auto before = *entity.Get<AnimatedSprite>();
	const auto beforeData = editor::SaveResource(*f.library);
	scene.Own(f.resources, f.library->Id());
	scene.Own(f.resources, f.texture->Id());
	entity.Add(f.Make());
	auto clip = *f.library->Find("Running");
	clip.frames = {7, 3, 0};
	f.library->Set("Running", clip);
	REQUIRE(editor::RecordAnimatedSpriteEdit(documents,
		scene,
		entity,
		f.resources,
		f.library->Id(),
		before,
		*entity.Get<AnimatedSprite>(),
		beforeData,
		editor::SaveResource(*f.library),
		true,
		f.texture->Id()));
	REQUIRE(editor::UndoSceneDocument(documents, scene));
	CHECK_FALSE(scene.Owns(f.library->Id()));
	CHECK_FALSE(scene.Owns(f.texture->Id()));
	CHECK_FALSE(entity.Get<AnimatedSprite>()->animations.IsValid());
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{4, 1, 7, 1});
	REQUIRE(editor::RedoSceneDocument(documents, scene));
	CHECK(scene.Owns(f.library->Id()));
	CHECK(scene.Owns(f.texture->Id()));
	CHECK(entity.Get<AnimatedSprite>()->currentAnimation == "Running");
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{7, 3, 0});
}

TEST_CASE("AnimatedSprite Lua API starts, pauses, resumes and stops named animations") {
	SpriteFixture f;
	ScriptServer scripts(f.world);
	FileSystem files;
	SceneManager scenes(f.world, f.resources, files);
	Input input;
	ProjectSettings settings;
	scripts.BindEngine(input, scenes, f.resources, settings);
	auto entity = f.world.CreateEntity();
	entity.Add(f.Make());
	scripts.SetEntity("runner", entity);
	REQUIRE(scripts.DoString("animated_sprite.pause(runner)"));
	AdvanceAnimatedSprite(*entity.Get<AnimatedSprite>(), f.resources, 1);
	CHECK_FALSE(entity.Get<AnimatedSprite>()->isPlaying);
	REQUIRE(scripts.DoString("assert(animated_sprite.play(runner, 'Running'))"));
	AdvanceAnimatedSprite(*entity.Get<AnimatedSprite>(), f.resources, .5);
	REQUIRE(scripts.DoString(
		"assert(animated_sprite.frame(runner) == 2); animated_sprite.pause(runner)"));
	AdvanceAnimatedSprite(*entity.Get<AnimatedSprite>(), f.resources, 1);
	REQUIRE(scripts.DoString(
		"assert(animated_sprite.frame(runner) == 2); assert(not "
		"animated_sprite.is_playing(runner)); animated_sprite.pause(runner, false)"));
	AdvanceAnimatedSprite(*entity.Get<AnimatedSprite>(), f.resources, .25);
	REQUIRE(scripts.DoString(
		"assert(animated_sprite.frame(runner) == 3); animated_sprite.stop(runner); "
		"assert(animated_sprite.frame(runner) == 0)"));
}

TEST_CASE("Sprite sheets support independent horizontal and vertical padding and spacing") {
	SpriteFixture f;
	auto clip = *f.library->Find("Running");
	clip.columns = 3;
	clip.rows = 2;
	clip.paddingX = 2;
	clip.paddingY = 4;
	clip.spacingX = 3;
	clip.spacingY = 6;
	clip.frames = {4};
	// 3 x 10px cells + 2 x 3px gaps + 2 x 2px border = 40px.
	// 2 x 12px cells + 1 x 6px gap + 2 x 4px border = 38px.
	const auto cell = clip.CellRect(4, {40, 38});
	CHECK(cell.x == 15);
	CHECK(cell.y == 22);
	CHECK(cell.w == 10);
	CHECK(cell.h == 12);
	const auto uv = clip.FrameUV(0, {40, 38});
	CHECK(uv.x == doctest::Approx(15. / 40));
	CHECK(uv.y == doctest::Approx(4. / 38));
	CHECK(uv.w == doctest::Approx(10. / 40));
	CHECK(uv.h == doctest::Approx(12. / 38));
	f.library->Set("Asymmetric", clip);
	auto restored = f.resources.Create<SpriteSheetAnimation>();
	auto load = Archive::Load(editor::SaveResource(*f.library));
	restored->Serialize(load);
	REQUIRE_FALSE(load.HasErrors());
	const auto *saved = restored->Find("Asymmetric");
	REQUIRE(saved);
	CHECK(saved->paddingX == 2);
	CHECK(saved->paddingY == 4);
	CHECK(saved->spacingX == 3);
	CHECK(saved->spacingY == 6);
}

TEST_CASE("Old scalar sprite padding and spacing still load on both axes") {
	SpriteFixture f;
	auto load = Archive::Load(R"(
animations:
  - name: Legacy
    texture: 1
    columns: 4
    rows: 2
    padding: 3
    spacing: 5
    frames: [0, 1]
)");
	f.library->Serialize(load);
	REQUIRE_FALSE(load.HasErrors());
	const auto *clip = f.library->Find("Legacy");
	REQUIRE(clip);
	CHECK(clip->paddingX == 3);
	CHECK(clip->paddingY == 3);
	CHECK(clip->spacingX == 5);
	CHECK(clip->spacingY == 5);
}
