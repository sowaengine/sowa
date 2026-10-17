#include <filesystem>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
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
#include "scene/scene_snapshot.hpp"

using namespace sowa;

namespace {

struct Fixture {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes;
	std::filesystem::path dir;

	Fixture() : scenes(world, resources, fs) {
		dir = std::filesystem::temp_directory_path() / "sowa_scene_manager_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);
	}

	~Fixture() {
		std::filesystem::remove_all(dir);
	}

	void save(Scene scene, std::string_view path) {
		REQUIRE(SaveSceneToFile(fs, path, scene, resources) == FsError::Ok);
	}
};

struct Note : public Resource {
	std::string text;
	i32 unloads = 0;

	void Unload(const ResourceContext &) override {
		++unloads;
	}
};

} // namespace

TEST_CASE("Clear unloads scenes and snapshots without replaying pending work") {
	Fixture f;
	f.resources.RegisterType<Note>("Note").Field("text", &Note::text);
	const auto note = f.resources.Create<Note>();
	note->text = "shared";
	const auto unowned = f.resources.Create<Note>();
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");
	REQUIRE(first.Own(f.resources, note->Id()));
	REQUIRE(second.Own(f.resources, note->Id()));
	REQUIRE(f.scenes.SetCurrent(first));
	f.save(first, "tmp://first.scene");
	Entity unrelated = f.world.CreateEntity();
	i32 stops = 0;
	f.scenes.SetPlaySessionCallbacks({}, [&] { ++stops; });

	SUBCASE("stopped documents") {
		REQUIRE(TakeSnapshot(first, f.resources));
	}
	SUBCASE("paused play session") {
		f.scenes.PlayCurrent();
		f.scenes.Flush();
		REQUIRE(f.scenes.IsPlaying());
		f.scenes.Pause();
		f.scenes.Step();
		f.scenes.SetScriptSuspended(true);
	}

	f.scenes.Open("tmp://first.scene");
	f.scenes.Change("tmp://first.scene");
	f.scenes.Clear();
	f.scenes.Clear();
	f.scenes.Flush();
	CHECK_FALSE(first.IsValid());
	CHECK_FALSE(second.IsValid());
	CHECK(f.scenes.Loaded().empty());
	CHECK_FALSE(f.scenes.Current().IsValid());
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK_FALSE(f.scenes.IsPaused());
	CHECK_FALSE(f.scenes.IsScriptSuspended());
	CHECK_FALSE(f.scenes.TakeStep());
	CHECK(stops == 0);
	CHECK_FALSE(f.resources.Has(note->Id()));
	CHECK(note->unloads == 1);
	CHECK(f.resources.Has(unowned->Id()));
	CHECK(unrelated.IsAlive());
	i32 roots = 0;
	f.world.EachIncludingDisabled<SceneRoot>([&](Entity, SceneRoot &) { ++roots; });
	CHECK(roots == 0);

	f.scenes.Open("tmp://first.scene");
	f.scenes.Flush();
	CHECK(f.scenes.FindByPath("tmp://first.scene").IsValid());
	const auto reloaded = f.resources.Get<Note>(note->Id());
	REQUIRE(reloaded != nullptr);
	CHECK(reloaded->text == "shared");
	f.scenes.Clear();
}

TEST_CASE("Create does not become current") {
	Fixture f;
	Scene scene = f.scenes.Create("Level");
	REQUIRE(scene.IsValid());
	CHECK_FALSE(f.scenes.Current().IsValid());
	CHECK(f.scenes.Loaded().size() == 1);
}

TEST_CASE("SetCurrent designates without loading or unloading") {
	Fixture f;
	Scene first = f.scenes.Create("First");
	Scene second = f.scenes.Create("Second");

	CHECK(f.scenes.SetCurrent(first));
	CHECK(f.scenes.Current().Root() == first.Root());
	CHECK(f.scenes.SetCurrent(second));
	CHECK(f.scenes.Current().Root() == second.Root());
	CHECK(f.scenes.Loaded().size() == 2);
}

TEST_CASE("SetCurrent of a default scene clears current") {
	Fixture f;
	f.scenes.SetCurrent(f.scenes.Create("Level"));
	REQUIRE(f.scenes.Current().IsValid());

	CHECK(f.scenes.SetCurrent(Scene()));
	CHECK_FALSE(f.scenes.Current().IsValid());
}

TEST_CASE("SetCurrent refuses a destroyed handle") {
	Fixture f;
	Scene scene = f.scenes.Create("Level");
	f.scenes.SetCurrent(scene);
	scene.Root().Destroy();

	Log::Clear();
	CHECK_FALSE(f.scenes.SetCurrent(scene));
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	// The stored current is dead; Current() reports none rather than a stale handle.
	CHECK_FALSE(f.scenes.Current().IsValid());
}

TEST_CASE("SetCurrent refuses a nested scene root") {
	Fixture f;
	Scene host = f.scenes.Create("Host");
	Scene nested = f.scenes.Create("Nested");
	nested.Root().SetParent(host.CreateEntity("Slot"));

	CHECK(f.scenes.SetCurrent(host));
	Log::Clear();
	CHECK_FALSE(f.scenes.SetCurrent(nested));
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
	CHECK(f.scenes.Current().Root() == host.Root());
}

TEST_CASE("SetCurrent refuses a snapshot copy") {
	Fixture f;
	Scene scene = f.scenes.Create("Level");
	REQUIRE(TakeSnapshot(scene, f.resources));

	Entity copy;
	f.world.EachIncludingDisabled<SceneRoot>([&](Entity entity, SceneRoot &) {
		if (entity.Has<SceneSnapshotCopy>()) {
			copy = entity;
		}
	});
	REQUIRE(copy.IsAlive());

	Log::Clear();
	CHECK_FALSE(f.scenes.SetCurrent(Scene(copy)));
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();

	DiscardSnapshot(scene, f.resources);
}

TEST_CASE("SetCurrent does not enable or disable anything") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Scene hud = f.scenes.Create("Hud");
	level.SetEnabled(false);

	CHECK(f.scenes.SetCurrent(level));
	CHECK_FALSE(level.IsEnabled());
	CHECK(hud.IsEnabled());

	CHECK(f.scenes.SetCurrent(hud));
	CHECK_FALSE(level.IsEnabled());
	CHECK(hud.IsEnabled());
}

TEST_CASE("Open of the same path creates another instance") {
	Fixture f;
	Scene seed = f.scenes.Create("Room");
	f.save(seed, "tmp://room.scene");
	UnloadScene(seed, f.resources);

	f.scenes.Open("tmp://room.scene");
	f.scenes.Open("tmp://room.scene");
	f.scenes.Flush();
	REQUIRE(f.scenes.Loaded().size() == 2);
	Scene a = f.scenes.Loaded()[0];
	Scene b = f.scenes.Loaded()[1];
	REQUIRE(a.IsValid());
	REQUIRE(b.IsValid());
	CHECK(a.Root() != b.Root());
	CHECK_FALSE(f.scenes.Current().IsValid());

	// FindByPath returns the oldest; both keep their source path.
	CHECK(f.scenes.FindByPath("tmp://room.scene").IsValid());
	CHECK(a.SourcePath() == "tmp://room.scene");
	CHECK(b.SourcePath() == "tmp://room.scene");
}

TEST_CASE("Find is by loaded root name") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	CHECK(f.scenes.Find("Level").Root() == level.Root());
	CHECK_FALSE(f.scenes.Find("Missing").IsValid());

	Scene nested = f.scenes.Create("Nested");
	nested.Root().SetParent(level.CreateEntity("Slot"));
	CHECK_FALSE(f.scenes.Find("Nested").IsValid());
}

TEST_CASE("A non-current scene's entities are still reachable") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Scene other = f.scenes.Create("Other");
	Entity player = other.CreateEntity("Player");
	f.scenes.SetCurrent(level);

	CHECK(f.world.Find("Other/Player") == player);
	CHECK(SceneOf(player).Root() == other.Root());
}

TEST_CASE("Close of a non-current scene leaves current alone") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Scene hud = f.scenes.Create("Hud");
	f.scenes.SetCurrent(level);

	f.scenes.Close(hud);
	f.scenes.Flush();
	CHECK_FALSE(hud.IsValid());
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(f.scenes.Loaded().size() == 1);
}

TEST_CASE("Close of current leaves no current") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Scene hud = f.scenes.Create("Hud");
	f.scenes.SetCurrent(level);

	f.scenes.Close(level);
	f.scenes.Flush();
	CHECK_FALSE(level.IsValid());
	CHECK_FALSE(f.scenes.Current().IsValid());
	REQUIRE(f.scenes.Loaded().size() == 1);
	CHECK(f.scenes.Loaded()[0].Root() == hud.Root());
}

TEST_CASE("Change replaces only current") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	level.CreateEntity("Old");
	f.save(level, "tmp://level.scene");
	UnloadScene(level, f.resources);

	Scene hud = f.scenes.Create("Hud");
	Entity hudMark = hud.CreateEntity("Score");
	f.scenes.Open("tmp://level.scene");
	f.scenes.Flush();
	Scene first = f.scenes.FindByPath("tmp://level.scene");
	REQUIRE(f.scenes.SetCurrent(first));

	Scene next = f.scenes.Create("Next");
	next.CreateEntity("New");
	f.save(next, "tmp://next.scene");
	UnloadScene(next, f.resources);

	f.scenes.Change("tmp://next.scene");
	f.scenes.Flush();
	Scene incoming = f.scenes.Current();
	REQUIRE(incoming.IsValid());
	CHECK(incoming.Name() == "Next");
	CHECK_FALSE(first.IsValid());
	CHECK(hud.IsValid());
	CHECK(hudMark.IsAlive());
	CHECK(f.world.Find("Next/New").IsAlive());
	CHECK_FALSE(f.world.Find("Level/Old").IsAlive());
	CHECK(f.scenes.Loaded().size() == 2);
}

TEST_CASE("Change with no current is Open plus SetCurrent") {
	Fixture f;
	Scene seed = f.scenes.Create("Level");
	seed.CreateEntity("Player");
	f.save(seed, "tmp://level.scene");
	UnloadScene(seed, f.resources);

	f.scenes.Change("tmp://level.scene");
	f.scenes.Flush();
	Scene loaded = f.scenes.Current();
	REQUIRE(loaded.IsValid());
	CHECK(loaded.Name() == "Level");
	CHECK(f.world.Find("Level/Player").IsAlive());
}

TEST_CASE("A failed Change leaves current untouched") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);

	Log::Clear();
	f.scenes.Change("tmp://missing.scene");
	f.scenes.Flush();
	Log::Clear();
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(level.IsValid());
}

TEST_CASE("Loaded still lists a disabled current") {
	Fixture f;
	Scene scene = f.scenes.Create("Level");
	Entity player = scene.CreateEntity("Player");
	player.Add(Transform2D{});
	f.scenes.SetCurrent(scene);
	scene.SetEnabled(false);

	Query<Transform2D> transforms = f.world.CreateQuery<Transform2D>();
	CHECK(transforms.Count() == 0);
	REQUIRE(f.scenes.Loaded().size() == 1);
	CHECK(f.scenes.Current().Root() == scene.Root());
	CHECK(f.scenes.Find("Level").Root() == scene.Root());
}

TEST_CASE("Open does not load until Flush") {
	Fixture f;
	Scene seed = f.scenes.Create("Room");
	f.save(seed, "tmp://room.scene");
	UnloadScene(seed, f.resources);

	f.scenes.Open("tmp://room.scene");
	CHECK(f.scenes.Loaded().empty());
	CHECK_FALSE(f.scenes.FindByPath("tmp://room.scene").IsValid());

	f.scenes.Flush();
	REQUIRE(f.scenes.Loaded().size() == 1);
	CHECK(f.scenes.FindByPath("tmp://room.scene").Name() == "Room");
}

TEST_CASE("Close does not unload until Flush") {
	Fixture f;
	Scene hud = f.scenes.Create("Hud");
	Entity mark = hud.CreateEntity("Score");

	f.scenes.Close(hud);
	CHECK(hud.IsValid());
	CHECK(mark.IsAlive());
	CHECK(f.scenes.Loaded().size() == 1);

	f.scenes.Flush();
	CHECK_FALSE(hud.IsValid());
	CHECK_FALSE(mark.IsAlive());
	CHECK(f.scenes.Loaded().empty());
}

TEST_CASE("Change does not swap until Flush") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	f.scenes.SetCurrent(level);

	Scene next = f.scenes.Create("Next");
	next.CreateEntity("New");
	f.save(next, "tmp://next.scene");
	UnloadScene(next, f.resources);

	f.scenes.Change("tmp://next.scene");
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(player.IsAlive());
	CHECK_FALSE(f.world.Find("Next/New").IsAlive());

	f.scenes.Flush();
	CHECK(f.scenes.Current().Name() == "Next");
	CHECK_FALSE(player.IsAlive());
	CHECK(f.world.Find("Next/New").IsAlive());
}

TEST_CASE("The last Change in the queue wins") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	f.scenes.SetCurrent(level);

	Scene next = f.scenes.Create("Next");
	next.CreateEntity("New");
	f.save(next, "tmp://next.scene");
	UnloadScene(next, f.resources);

	// If both ran, the valid Change would switch current and the missing one
	// would leave that new current in place. Dropping the earlier Change
	// means only the missing path runs, so current stays.
	f.scenes.Change("tmp://next.scene");
	f.scenes.Change("tmp://missing.scene");
	Log::Clear();
	f.scenes.Flush();
	Log::Clear();

	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(level.IsValid());
	CHECK_FALSE(f.world.Find("Next/New").IsAlive());
}

TEST_CASE("Open then Change keeps the opened scene") {
	Fixture f;
	Scene hudSeed = f.scenes.Create("Hud");
	hudSeed.CreateEntity("Score");
	f.save(hudSeed, "tmp://hud.scene");
	UnloadScene(hudSeed, f.resources);

	Scene levelSeed = f.scenes.Create("Level");
	levelSeed.CreateEntity("Player");
	f.save(levelSeed, "tmp://level.scene");
	UnloadScene(levelSeed, f.resources);

	Scene old = f.scenes.Create("Old");
	f.scenes.SetCurrent(old);

	f.scenes.Open("tmp://hud.scene");
	f.scenes.Change("tmp://level.scene");
	f.scenes.Flush();

	CHECK(f.scenes.FindByPath("tmp://hud.scene").IsValid());
	CHECK(f.world.Find("Hud/Score").IsAlive());
	CHECK(f.scenes.Current().Name() == "Level");
	CHECK(f.world.Find("Level/Player").IsAlive());
	CHECK_FALSE(old.IsValid());
	CHECK(f.scenes.Loaded().size() == 2);
}

TEST_CASE("Change from Query::Each does not abort and waits for Flush") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	player.Add(Transform2D{});
	f.scenes.SetCurrent(level);

	Scene next = f.scenes.Create("Next");
	next.CreateEntity("New");
	f.save(next, "tmp://next.scene");
	UnloadScene(next, f.resources);

	i32 visits = 0;
	Query<Transform2D> query = f.world.CreateQuery<Transform2D>();
	query.Each([&](Entity, Transform2D &) {
		++visits;
		f.scenes.Change("tmp://next.scene");
	});

	CHECK(visits == 1);
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(player.IsAlive());

	f.scenes.Flush();
	CHECK(f.scenes.Current().Name() == "Next");
	CHECK_FALSE(player.IsAlive());
}

TEST_CASE("A Change from a system waits until Flush") {
	Fixture f;
	Scene level = f.scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	player.Add(Transform2D{});
	f.scenes.SetCurrent(level);

	Scene next = f.scenes.Create("Next");
	next.CreateEntity("New");
	f.save(next, "tmp://next.scene");
	UnloadScene(next, f.resources);

	bool stillOldInside = false;
	f.world.RegisterSystem<Transform2D>("Switch", [&](Entity, Transform2D &) {
		f.scenes.Change("tmp://next.scene");
		stillOldInside = f.scenes.Current().Root() == level.Root() && player.IsAlive();
	});

	f.world.Progress(0.f);
	CHECK(stillOldInside);
	CHECK(f.scenes.Current().Root() == level.Root());
	CHECK(player.IsAlive());

	f.scenes.Flush();
	CHECK(f.scenes.Current().Name() == "Next");
	CHECK_FALSE(player.IsAlive());
}

TEST_CASE("Change loads incoming before closing outgoing") {
	Fixture f;
	f.resources.RegisterType<Note>("Note").Field("text", &Note::text);

	Scene old = f.scenes.Create("Old");
	Ref<Note> note = f.resources.Create<Note>();
	note->text = "shared";
	const ResourceID id = note->Id();
	REQUIRE(old.Own(f.resources, id));
	f.save(old, "tmp://old.scene");

	Scene incoming = f.scenes.Create("New");
	REQUIRE(incoming.Own(f.resources, id));
	f.save(incoming, "tmp://new.scene");
	UnloadScene(old, f.resources);
	UnloadScene(incoming, f.resources);

	f.scenes.Open("tmp://old.scene");
	f.scenes.Flush();
	Scene current = f.scenes.FindByPath("tmp://old.scene");
	REQUIRE(f.scenes.SetCurrent(current));
	Ref<Note> before = f.resources.Get<Note>(id);
	REQUIRE(before != nullptr);

	f.scenes.Change("tmp://new.scene");
	f.scenes.Flush();

	CHECK(f.resources.Has(id));
	CHECK(f.resources.Get<Note>(id) == before);
	CHECK(f.scenes.Current().Name() == "New");
	CHECK_FALSE(current.IsValid());
}
