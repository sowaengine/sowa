#include <filesystem>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/store.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/scene_root.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/world.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"

using namespace sowa;

namespace {

struct Note : public Resource {
	std::string text;
	i32 count = 0;
	i32 reloads = 0;

	void Reload(const ResourceContext &) override { ++reloads; }
};

struct Fixture {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes;
	std::filesystem::path dir;

	Fixture() : scenes(world, resources, fs) {
		dir = std::filesystem::temp_directory_path() / "sowa_scene_play_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);
		resources.RegisterType<Note>("Note")
			.Field("text", &Note::text)
			.Field("count", &Note::count);
	}

	~Fixture() { std::filesystem::remove_all(dir); }

	void save(Scene scene, std::string_view path) {
		REQUIRE(SaveSceneToFile(fs, path, scene, resources) == FsError::Ok);
	}

	i32 snapshotCopies() {
		i32 copies = 0;
		world.EachIncludingDisabled<SceneRoot>([&](Entity entity, SceneRoot &) {
			if (entity.Has<SceneSnapshotCopy>())
				++copies;
		});
		return copies;
	}
};

} // namespace

TEST_CASE("PlayCurrent does not apply until Flush") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);

	f.scenes.PlayCurrent();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.PlayMode() == PlayKind::Stopped);
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(f.scenes.Loaded().size() == 1);

	f.scenes.Flush();
	CHECK(f.scenes.IsPlaying());
	CHECK(f.scenes.PlayMode() == PlayKind::Current);
	CHECK(f.scenes.Current().Root() != level.Root());
	CHECK(level.IsValid());
	CHECK_FALSE(level.IsEnabled());
}

TEST_CASE("EditorScenes hides the runtime root during play") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Scene tools = f.scenes.Create("Tools");
	f.scenes.SetCurrent(level);

	const std::vector<Scene> stoppedScenes = f.scenes.EditorScenes();
	REQUIRE(stoppedScenes.size() == 2);
	CHECK(stoppedScenes[0].Root() == level.Root());
	CHECK(stoppedScenes[1].Root() == tools.Root());

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	const Scene runtime = f.scenes.Current();
	REQUIRE(runtime.IsValid());

	const std::vector<Scene> editorScenes = f.scenes.EditorScenes();
	REQUIRE(editorScenes.size() == 2);
	CHECK(editorScenes[0].Root() == level.Root());
	CHECK(editorScenes[1].Root() == tools.Root());
	CHECK(editorScenes[0].Root() != runtime.Root());
	CHECK(editorScenes[1].Root() != runtime.Root());
	CHECK(f.scenes.EditorCurrent().Root() == level.Root());
}

TEST_CASE("PlayCurrent restores entity and resource mutations on Stop") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	player.Add(transform);

	Ref<Note> note = f.resources.Create<Note>();
	note->text = "a string too long for any small-string optimization";
	note->count = 7;
	REQUIRE(level.Own(f.resources, note->Id()));
	const ResourceID id = note->Id();
	REQUIRE(f.resources.OwnerCount(id) == 1);

	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	// Editor own + play retain + snapshot extra.
	CHECK(f.resources.OwnerCount(id) == 3);

	Scene play = f.scenes.Current();
	REQUIRE(play.Root() != level.Root());
	Entity playPlayer = play.Root().Find("Player");
	REQUIRE(playPlayer.IsAlive());
	playPlayer.Get<Transform2D>()->position = Vector2(99.f, 99.f);
	note->text = "changed";
	note->count = 99;
	play.CreateEntity("Spawned");

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.PlayMode() == PlayKind::Stopped);

	Scene restored = f.scenes.Current();
	REQUIRE(restored.IsValid());
	CHECK(restored.Name() == "Level");
	CHECK(restored.IsEnabled());
	CHECK_FALSE(play.IsValid());
	CHECK_FALSE(level.IsValid());

	Entity restoredPlayer = f.world.Find("Level/Player");
	REQUIRE(restoredPlayer.IsAlive());
	CHECK(restoredPlayer.Get<Transform2D>()->position.x == doctest::Approx(10.f));
	CHECK(restoredPlayer.Get<Transform2D>()->position.y == doctest::Approx(20.f));
	CHECK_FALSE(f.world.Find("Level/Spawned").IsAlive());

	REQUIRE(f.resources.Get<Note>(id) == note);
	CHECK(note->text == "a string too long for any small-string optimization");
	CHECK(note->count == 7);
	CHECK(note->reloads == 1);
	CHECK(f.resources.OwnerCount(id) == 1);
	CHECK(f.snapshotCopies() == 0);
}

TEST_CASE("successful play callbacks snapshot and restore external session data") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	StringStore live;
	StringStore snapshot;
	live.Set("score", "10");
	int starts = 0;
	int stops = 0;
	f.scenes.SetPlaySessionCallbacks(
		[&]() {
			++starts;
			snapshot = live;
		},
		[&]() {
			++stops;
			live = snapshot;
		});

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	CHECK(starts == 1);
	live.Set("score", "99");
	live.Set("runtime_only", "yes");

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(stops == 1);
	CHECK(live.Get("score") == "10");
	CHECK_FALSE(live.Has("runtime_only"));
}

TEST_CASE("failed play does not start an external session snapshot") {
	Fixture f;
	int starts = 0;
	int stops = 0;
	f.scenes.SetPlaySessionCallbacks([&]() { ++starts; },
									 [&]() { ++stops; });
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(starts == 0);
	CHECK(stops == 0);
}

TEST_CASE("Editor tabs do not match systems during play and get their enabled flags back") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	player.Add(Transform2D{});
	Scene hud = f.scenes.Create("Hud");
	Entity score = hud.CreateEntity("Score");
	score.Add(Transform2D{});
	hud.SetEnabled(false);

	f.scenes.SetCurrent(level);
	Query<Transform2D> transforms = f.world.CreateQuery<Transform2D>();
	REQUIRE(transforms.Count() == 1);

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	CHECK_FALSE(level.IsEnabled());
	CHECK_FALSE(hud.IsEnabled());
	// Play instance only. If disable were skipped, editor Level would still
	// match and this would be 2.
	CHECK(transforms.Count() == 1);
	CHECK(transforms.First() != player);

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(f.scenes.Current().IsEnabled());
	Scene restoredHud = f.scenes.Find("Hud");
	REQUIRE(restoredHud.IsValid());
	CHECK_FALSE(restoredHud.IsEnabled());
	CHECK(transforms.Count() == 1);
}

TEST_CASE("PlayProject loads the file, not the unsaved current tree") {
	Fixture f;
	Scene menu = f.scenes.Create("Menu");
	Entity start = menu.CreateEntity("Start");
	start.Add(Transform2D{});
	f.save(menu, "tmp://menu.scene");
	UnloadScene(menu, f.resources);

	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	Transform2D transform;
	transform.position = Vector2(50.f, 60.f);
	player.Add(transform);
	f.scenes.SetCurrent(level);

	f.scenes.PlayProject("tmp://menu.scene");
	f.scenes.Flush();
	REQUIRE(f.scenes.PlayMode() == PlayKind::Project);
	CHECK(f.scenes.Current().Name() == "Menu");
	CHECK(f.world.Find("Menu/Start").IsAlive());
	CHECK(level.IsValid());
	CHECK_FALSE(level.IsEnabled());
	CHECK(player.IsAlive());
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(50.f));

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(f.scenes.Current().Name() == "Level");
	Entity restored = f.world.Find("Level/Player");
	REQUIRE(restored.IsAlive());
	CHECK(restored.Get<Transform2D>()->position.x == doctest::Approx(50.f));
	CHECK_FALSE(f.scenes.Find("Menu").IsValid());
}

TEST_CASE("PlayProject of the current file plays the saved tree") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	player.Add(transform);
	f.save(level, "tmp://level.scene");

	player.Get<Transform2D>()->position = Vector2(99.f, 99.f);
	f.scenes.SetCurrent(level);

	f.scenes.PlayProject("tmp://level.scene");
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	Scene play = f.scenes.Current();
	REQUIRE(play.Root() != level.Root());
	Entity playPlayer = play.Root().Find("Player");
	REQUIRE(playPlayer.IsAlive());
	CHECK(playPlayer.Get<Transform2D>()->position.x == doctest::Approx(10.f));
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(99.f));

	f.scenes.Stop();
	f.scenes.Flush();
	Entity restored = f.world.Find("Level/Player");
	REQUIRE(restored.IsAlive());
	CHECK(restored.Get<Transform2D>()->position.x == doctest::Approx(99.f));
}

TEST_CASE("PlayProject's active camera is the play instance, not the editor copy") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity camera = level.CreateEntity("Camera");
	camera.Add(Camera2D{});
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	camera.Add(transform);
	REQUIRE(level.SetActiveCamera(camera));
	f.save(level, "tmp://level.scene");
	f.scenes.SetCurrent(level);

	f.scenes.PlayProject("tmp://level.scene");
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());

	Scene play = f.scenes.Current();
	REQUIRE(play.Root() != level.Root());
	Entity playCamera = play.ActiveCamera();
	REQUIRE(playCamera.IsAlive());
	CHECK(playCamera != camera);
	CHECK(playCamera == play.Root().Find("Camera"));
}

TEST_CASE("Change during play does not destroy the editor restore point") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	player.Add(Transform2D{});
	f.scenes.SetCurrent(level);

	Scene next = f.scenes.Create("Next");
	next.CreateEntity("New");
	f.save(next, "tmp://next.scene");
	UnloadScene(next, f.resources);

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	Entity playRoot = f.scenes.Current().Root();

	f.scenes.Change("tmp://next.scene");
	f.scenes.Flush();
	CHECK(f.scenes.IsPlaying());
	CHECK(f.scenes.Current().Name() == "Next");
	CHECK_FALSE(playRoot.IsAlive());
	CHECK(level.IsValid());

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.Current().Name() == "Level");
	CHECK(f.world.Find("Level/Player").IsAlive());
	CHECK_FALSE(f.scenes.Find("Next").IsValid());
}

TEST_CASE("A second Play while playing is refused") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	Entity playRoot = f.scenes.Current().Root();
	REQUIRE(f.scenes.IsPlaying());

	Log::Clear();
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	CHECK_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
	Log::Clear();
	CHECK(f.scenes.IsPlaying());
	CHECK(f.scenes.Current().Root() == playRoot);

	f.scenes.PlayProject("tmp://missing.scene");
	f.scenes.Flush();
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	CHECK(f.scenes.PlayMode() == PlayKind::Current);
	CHECK(f.scenes.Current().Root() == playRoot);

	f.scenes.Stop();
	f.scenes.Flush();
}

TEST_CASE("Stop then PlayCurrent in one queue restarts") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	level.CreateEntity("Player").Add(Transform2D{});
	f.scenes.SetCurrent(level);

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	Entity firstPlay = f.scenes.Current().Root();
	f.scenes.Current().Root().Find("Player").Get<Transform2D>()->position =
		Vector2(5.f, 5.f);

	f.scenes.Stop();
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	CHECK(f.scenes.IsPlaying());
	CHECK(f.scenes.PlayMode() == PlayKind::Current);
	CHECK(f.scenes.Current().Root() != firstPlay);
	Entity playPlayer = f.scenes.Current().Root().Find("Player");
	REQUIRE(playPlayer.IsAlive());
	CHECK(playPlayer.Get<Transform2D>()->position.x == doctest::Approx(0.f));

	f.scenes.Stop();
	f.scenes.Flush();
}

TEST_CASE("PlayCurrent with no current is refused") {
	Fixture f;
	f.scenes.Create("Level");
	Log::Clear();
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.Loaded().size() == 1);
}

TEST_CASE("PlayProject with an empty or missing path is refused") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);

	Log::Clear();
	f.scenes.PlayProject("");
	f.scenes.Flush();
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(level.IsEnabled());

	f.scenes.PlayProject("tmp://missing.scene");
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(level.IsEnabled());
}

TEST_CASE("Stop while stopped is a no-op") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.Stop();
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK(f.scenes.Current().Root() == level.Root());
}

TEST_CASE("Close of a frozen editor scene is refused") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Scene hud = f.scenes.Create("Hud");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());

	Log::Clear();
	f.scenes.Close(level);
	f.scenes.Flush();
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	CHECK(level.IsValid());
	CHECK(hud.IsValid());
	CHECK(f.scenes.IsPlaying());

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(f.scenes.Find("Level").IsValid());
	CHECK(f.scenes.Find("Hud").IsValid());
}

TEST_CASE("SetCurrent of a frozen editor scene is refused") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	Entity playRoot = f.scenes.Current().Root();

	Log::Clear();
	CHECK_FALSE(f.scenes.SetCurrent(level));
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	CHECK(f.scenes.Current().Root() == playRoot);

	f.scenes.Stop();
	f.scenes.Flush();
}

TEST_CASE("Autoload scenes load on Play and unload on Stop") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);

	Scene game = f.scenes.Create("Game");
	f.save(game, "tmp://game.scene");
	f.scenes.Close(game);
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.Find("Game").IsValid());

	f.scenes.SetAutoloadPaths({"tmp://game.scene"});
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	REQUIRE(f.scenes.IsPlaying());
	CHECK(f.scenes.Find("Game").IsValid());
	CHECK(f.scenes.Loaded().size() == 3);
	CHECK(f.scenes.Find("Game").IsEnabled());

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.Find("Game").IsValid());
	CHECK(f.scenes.Find("Level").IsValid());
	CHECK(f.scenes.Loaded().size() == 1);
}

TEST_CASE("Pause while stopped is a no-op") {
	Fixture f;
	f.scenes.Pause();
	CHECK_FALSE(f.scenes.IsPaused());
}

TEST_CASE("Pause and Resume apply immediately, without Flush") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPaused());

	f.scenes.Pause();
	CHECK(f.scenes.IsPaused());
	CHECK(f.scenes.IsPlaying());
	f.scenes.Resume();
	CHECK_FALSE(f.scenes.IsPaused());
}

TEST_CASE("Stop clears pause, so the next Play starts running") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	f.scenes.Pause();
	f.scenes.Step();

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPaused());
	CHECK_FALSE(f.scenes.TakeStep());

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	CHECK(f.scenes.IsPlaying());
	CHECK_FALSE(f.scenes.IsPaused());
}

TEST_CASE("Step is taken once, and only while paused") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();

	f.scenes.Step();
	CHECK_FALSE(f.scenes.TakeStep());

	f.scenes.Pause();
	f.scenes.Step();
	f.scenes.Step();
	CHECK(f.scenes.TakeStep());
	CHECK_FALSE(f.scenes.TakeStep());
	CHECK(f.scenes.IsPaused());
}

TEST_CASE("Resume cancels a pending Step") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	f.scenes.PlayCurrent();
	f.scenes.Flush();
	f.scenes.Pause();
	f.scenes.Step();
	f.scenes.Resume();
	CHECK_FALSE(f.scenes.TakeStep());
}

TEST_CASE("RuntimeScenes lists the running game and not the frozen editor documents") {
	Fixture f;
	Scene hud = f.scenes.Create("Hud");
	f.save(hud, "tmp://hud.scene");
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);
	CHECK(f.scenes.RuntimeScenes().empty());

	f.scenes.PlayCurrent();
	f.scenes.Flush();
	Scene play = f.scenes.Current();
	std::vector<Scene> runtime = f.scenes.RuntimeScenes();
	REQUIRE(runtime.size() == 1);
	CHECK(runtime[0].Root() == play.Root());

	f.scenes.Open("tmp://hud.scene");
	f.scenes.Flush();
	runtime = f.scenes.RuntimeScenes();
	CHECK(runtime.size() == 2);
	for (const Scene scene : runtime) {
		CHECK(scene.Root() != level.Root());
		CHECK(scene.Root() != hud.Root());
	}

	f.scenes.Stop();
	f.scenes.Flush();
	CHECK(f.scenes.RuntimeScenes().empty());
}
