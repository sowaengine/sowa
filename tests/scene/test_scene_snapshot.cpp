#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/log.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/scene_root.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/world.hpp"
#include "resource/resource.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene.hpp"
#include "scene/scene_serializer.hpp"
#include "scene/scene_snapshot.hpp"

using namespace sowa;

namespace {

struct Note : public Resource {
	std::string text;
	i32 count = 0;
	i32 reloads = 0;

	void Reload(const ResourceContext &) override { ++reloads; }
};

struct ResourceFixture {
	ResourceRegistry registry;

	ResourceFixture() {
		registry.RegisterType<Note>("Note")
			.Field("text", &Note::text)
			.Field("count", &Note::count);
	}

	Ref<Note> Make(Scene scene, const std::string &text, i32 count = 0) {
		Ref<Note> note = registry.Create<Note>();
		note->text = text;
		note->count = count;
		scene.Own(registry, note->Id());
		return note;
	}
};

} // namespace

TEST_CASE("TakeSnapshot is refused on an invalid scene or a second call") {
	World world;
	ResourceFixture fixture;

	Scene invalid;
	CHECK_FALSE(TakeSnapshot(invalid, fixture.registry));

	Scene scene = CreateScene(world, "Level");
	REQUIRE(TakeSnapshot(scene, fixture.registry));

	Log::Clear();
	CHECK_FALSE(TakeSnapshot(scene, fixture.registry));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
	Log::Clear();

	DiscardSnapshot(scene, fixture.registry);
}

TEST_CASE("Restore without a snapshot is refused") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	Log::Clear();
	CHECK_FALSE(RestoreSnapshot(scene, fixture.registry));
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
	Log::Clear();
	CHECK(scene.IsValid());
}

TEST_CASE("A snapshot copy is skipped by queries and systems") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	Entity player = scene.CreateEntity("Player");
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	player.Add(transform);

	Query<Transform2D> transforms = world.CreateQuery<Transform2D>();
	REQUIRE(transforms.Count() == 1);

	i32 ticks = 0;
	world.RegisterSystem<Transform2D>("Count", [&](Entity, Transform2D &) { ++ticks; });

	REQUIRE(TakeSnapshot(scene, fixture.registry));

	// If the disable pass only tagged the root, the copied child would still
	// match - disabling is not inherited, which is why the walk exists.
	CHECK(transforms.Count() == 1);
	CHECK(transforms.First() == player);

	ticks = 0;
	world.Progress(0.f);
	CHECK(ticks == 1);

	// The copy is tagged SceneSnapshotCopy; GetScenes matches disabled roots
	// (a paused tab is still loaded) and skips the tag, so a snapshot is not
	// a second open scene.
	CHECK(GetScenes(world).size() == 1);
	CHECK(GetScenes(world)[0].Root() == scene.Root());

	DiscardSnapshot(scene, fixture.registry);
}

TEST_CASE("A snapshot is unreachable by path") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Entity player = scene.CreateEntity("Player");

	REQUIRE(TakeSnapshot(scene, fixture.registry));

	// Duplicate would have named the copy Level2; the name is stripped so a
	// lookup can't stumble into the dormant tree.
	CHECK(world.Find("Level/Player") == player);
	CHECK_FALSE(world.Find("Level2").IsAlive());
	CHECK_FALSE(world.Find("Level2/Player").IsAlive());
	CHECK_FALSE(world.Find("Player").IsAlive());

	DiscardSnapshot(scene, fixture.registry);
}

TEST_CASE("Restore reverts entity mutations, creations and destructions") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	Entity player = scene.CreateEntity("Player");
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	transform.rotation = 45.f;
	player.Add(transform);
	Entity kept = scene.CreateEntity("Kept");

	REQUIRE(TakeSnapshot(scene, fixture.registry));

	player.Get<Transform2D>()->position = Vector2(99.f, 99.f);
	player.Get<Transform2D>()->rotation = 0.f;
	kept.Destroy();
	scene.CreateEntity("Spawned");

	REQUIRE(RestoreSnapshot(scene, fixture.registry));
	REQUIRE(scene.IsValid());
	CHECK(scene.Name() == "Level");

	Entity restored = world.Find("Level/Player");
	REQUIRE(restored.IsAlive());
	// Swap, not an in-place rewrite: the mutated entity is gone.
	CHECK(restored != player);
	CHECK(restored.Get<Transform2D>()->position.x == doctest::Approx(10.f));
	CHECK(restored.Get<Transform2D>()->position.y == doctest::Approx(20.f));
	CHECK(restored.Get<Transform2D>()->rotation == doctest::Approx(45.f));

	CHECK(world.Find("Level/Kept").IsAlive());
	CHECK_FALSE(world.Find("Level/Spawned").IsAlive());
	CHECK_FALSE(player.IsAlive());
}

TEST_CASE("Restore remaps entity references inside the scene") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	Entity camera = scene.CreateEntity("Camera");
	camera.Add(Camera2D{});
	REQUIRE(scene.SetActiveCamera(camera));

	REQUIRE(TakeSnapshot(scene, fixture.registry));

	scene.SetActiveCamera(Entity());
	camera.Destroy();
	CHECK_FALSE(scene.ActiveCamera().IsAlive());

	REQUIRE(RestoreSnapshot(scene, fixture.registry));

	Entity restored = scene.ActiveCamera();
	REQUIRE(restored.IsAlive());
	CHECK(restored.GetPath() == "Level/Camera");
	CHECK(restored.Has<Camera2D>());
	// Not the original handle - that entity was destroyed with the live tree.
	CHECK(restored != camera);
}

TEST_CASE("An autoload root is left alone") {
	World world;
	ResourceFixture fixture;
	Scene level = CreateScene(world, "Level");
	Scene autoload = CreateScene(world, "Autoload");
	Entity save = autoload.CreateEntity("Save");
	level.CreateEntity("Player");

	REQUIRE(TakeSnapshot(level, fixture.registry));
	level.CreateEntity("Temp");
	REQUIRE(RestoreSnapshot(level, fixture.registry));

	CHECK(autoload.IsValid());
	CHECK(save.IsAlive());
	CHECK(world.Find("Autoload/Save") == save);
	CHECK(GetScenes(world).size() == 2);
	CHECK_FALSE(world.Find("Level/Temp").IsAlive());
}

TEST_CASE("Discard drops the copy and leaves live mutations in place") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Entity player = scene.CreateEntity("Player");
	player.Add(Transform2D{});

	Entity root = scene.Root();
	REQUIRE(TakeSnapshot(scene, fixture.registry));
	player.Get<Transform2D>()->position = Vector2(5.f, 6.f);

	DiscardSnapshot(scene, fixture.registry);

	CHECK(scene.Root() == root);
	CHECK(player.IsAlive());
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(5.f));
	CHECK(world.CreateQuery<Transform2D>().Count() == 1);
	CHECK(GetScenes(world).size() == 1);

	// A discarded snapshot is gone, so a new play session can take another.
	CHECK(TakeSnapshot(scene, fixture.registry));
	DiscardSnapshot(scene, fixture.registry);
}

TEST_CASE("Mutated resource fields are restored, and Reload runs only then") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	const std::string original = "a string too long for any small-string optimization";
	Ref<Note> note = fixture.Make(scene, original, 7);
	const ResourceID id = note->Id();

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	CHECK(fixture.registry.OwnerCount(id) == 2);

	note->text = "changed";
	note->count = 99;
	REQUIRE(RestoreSnapshot(scene, fixture.registry));

	REQUIRE(fixture.registry.Get<Note>(id) == note);
	CHECK(note->text == original);
	CHECK(note->count == 7);
	CHECK(note->reloads == 1);
	CHECK(fixture.registry.OwnerCount(id) == 1);
	CHECK(scene.Owns(id));
}

TEST_CASE("Unchanged resource fields do not Reload") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Ref<Note> note = fixture.Make(scene, "same", 1);

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	REQUIRE(RestoreSnapshot(scene, fixture.registry));

	CHECK(note->text == "same");
	CHECK(note->reloads == 0);
}

TEST_CASE("A Disown during play cannot drop a snapshotted resource") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Ref<Note> note = fixture.Make(scene, "keep", 1);
	const ResourceID id = note->Id();

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	REQUIRE(scene.Disown(fixture.registry, id));
	// Without the extra retain, this Disown would have been the last claim
	// and the object would already be gone.
	REQUIRE(fixture.registry.Has(id));
	CHECK(fixture.registry.OwnerCount(id) == 1);

	REQUIRE(RestoreSnapshot(scene, fixture.registry));
	CHECK(scene.Owns(id));
	CHECK(fixture.registry.Get<Note>(id)->text == "keep");
	CHECK(fixture.registry.OwnerCount(id) == 1);
}

TEST_CASE("A resource owned only during play is released on restore") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Ref<Note> original = fixture.Make(scene, "base", 0);
	const ResourceID originalId = original->Id();

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	Ref<Note> spawned = fixture.Make(scene, "temp", 0);
	const ResourceID spawnedId = spawned->Id();
	REQUIRE(fixture.registry.Has(spawnedId));

	REQUIRE(RestoreSnapshot(scene, fixture.registry));

	CHECK(scene.Owns(originalId));
	CHECK_FALSE(scene.Owns(spawnedId));
	CHECK_FALSE(fixture.registry.Has(spawnedId));
	CHECK(fixture.registry.OwnerCount(originalId) == 1);
}

TEST_CASE("Discard releases the extra retain without restoring fields") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Ref<Note> note = fixture.Make(scene, "live", 0);
	const ResourceID id = note->Id();

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	CHECK(fixture.registry.OwnerCount(id) == 2);
	note->text = "mutated";

	DiscardSnapshot(scene, fixture.registry);

	CHECK(note->text == "mutated");
	CHECK(fixture.registry.OwnerCount(id) == 1);
	CHECK(scene.Owns(id));
}

TEST_CASE("A restored scene can be snapshotted again") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	scene.CreateEntity("Player").Add(Transform2D{});

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	REQUIRE(RestoreSnapshot(scene, fixture.registry));
	REQUIRE(TakeSnapshot(scene, fixture.registry));
	REQUIRE(RestoreSnapshot(scene, fixture.registry));
	CHECK(world.Find("Level/Player").IsAlive());
}

TEST_CASE("UnloadScene discards a snapshot so the copy does not leak") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(scene, "owned", 0)->Id();

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	CHECK(fixture.registry.OwnerCount(id) == 2);

	i32 copies = 0;
	world.EachIncludingDisabled<SceneRoot>([&](Entity entity, SceneRoot &) {
		if (entity.Has<SceneSnapshotCopy>())
			++copies;
	});
	REQUIRE(copies == 1);

	UnloadScene(scene, fixture.registry);

	copies = 0;
	world.EachIncludingDisabled<SceneRoot>([&](Entity entity, SceneRoot &) {
		if (entity.Has<SceneSnapshotCopy>())
			++copies;
	});
	CHECK(copies == 0);
	CHECK_FALSE(fixture.registry.Has(id));
	CHECK_FALSE(scene.IsValid());
}

TEST_CASE("Unload after restore drops the original ownership") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(scene, "owned", 0)->Id();

	REQUIRE(TakeSnapshot(scene, fixture.registry));
	REQUIRE(RestoreSnapshot(scene, fixture.registry));
	REQUIRE(fixture.registry.OwnerCount(id) == 1);

	UnloadScene(scene, fixture.registry);
	CHECK_FALSE(fixture.registry.Has(id));
	CHECK_FALSE(scene.IsValid());
}
