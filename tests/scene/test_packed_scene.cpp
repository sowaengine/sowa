#include <filesystem>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/serialize/archive.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "resource/resource.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene.hpp"
#include "scene/scene_serializer.hpp"

using namespace sowa;

namespace {

struct Note : public Resource {
	std::string text;
};

struct Aim {
	Entity target;
};

struct Gun {
	i32 unused = 0;
};

struct Fixture {
	World world;
	ResourceRegistry registry;
	FileSystem fs;
	std::filesystem::path dir;

	Fixture() {
		dir = std::filesystem::temp_directory_path() / "sowa_packed_scene_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);
		registry.SetContext(ResourceContext{nullptr, &fs});
		registry.RegisterType<Note>("Note").Field("text", &Note::text);
		registry.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);
		world.RegisterComponent<Aim>("Aim").Field("target", &Aim::target);
		world.RegisterComponent<Gun>("Gun").Field("unused", &Gun::unused);
	}

	~Fixture() { std::filesystem::remove_all(dir); }

	ResourceID makeNote(Scene scene, const std::string &text) {
		Ref<Note> note = registry.Create<Note>();
		note->text = text;
		scene.Own(registry, note->Id());
		return note->Id();
	}

	void save(Scene scene, std::string_view path) {
		REQUIRE(SaveSceneToFile(fs, path, scene, registry) == FsError::Ok);
	}

	Ref<PackedScene> loadPacked(std::string_view path) {
		Ref<PackedScene> packed = registry.Create<PackedScene>();
		packed->path = std::string(path);
		registry.Reload(packed->Id());
		return packed;
	}
};

} // namespace

TEST_CASE("Instantiate builds a nested tree that is not an open scene") {
	Fixture f;
	Scene prefab = CreateScene(f.world, "Bullet");
	Entity sprite = prefab.CreateEntity("Sprite");
	sprite.Add(Transform2D{});
	f.save(prefab, "tmp://bullet.scene");
	UnloadScene(prefab, f.registry);

	Scene host = CreateScene(f.world, "Level");
	Ref<PackedScene> packed = f.loadPacked("tmp://bullet.scene");
	REQUIRE(host.Own(f.registry, packed->Id()));

	Entity instance = packed->Instantiate(f.world, f.registry, host.Root());
	REQUIRE(instance.IsAlive());
	CHECK(instance.GetParent() == host.Root());
	CHECK(instance.Has<SceneRoot>());
	CHECK(IsInstanceRoot(instance));
	CHECK_FALSE(IsInstanceLocked(instance));

	Entity child = instance.Find("Sprite");
	REQUIRE(child.IsAlive());
	CHECK(IsInstanceLocked(child));
	CHECK_FALSE(IsInstanceRoot(child));

	REQUIRE(GetScenes(f.world).size() == 1);
	CHECK(GetScenes(f.world)[0].Root() == host.Root());
	CHECK_FALSE(Scene(instance).Owns(packed->Id()));
}

TEST_CASE("Two instances share a resource the PackedScene retains") {
	Fixture f;
	Scene prefab = CreateScene(f.world, "Bullet");
	const ResourceID noteId = f.makeNote(prefab, "shared");
	f.save(prefab, "tmp://bullet.scene");
	UnloadScene(prefab, f.registry);
	CHECK_FALSE(f.registry.Has(noteId));

	Ref<PackedScene> packed = f.loadPacked("tmp://bullet.scene");
	REQUIRE(f.registry.Has(noteId));
	CHECK(f.registry.OwnerCount(noteId) == 1);

	Scene host = CreateScene(f.world, "Level");
	REQUIRE(host.Own(f.registry, packed->Id()));

	Entity a = packed->Instantiate(f.world, f.registry, host.Root());
	Entity b = packed->Instantiate(f.world, f.registry, host.Root());
	REQUIRE(a.IsAlive());
	REQUIRE(b.IsAlive());
	CHECK(f.registry.OwnerCount(noteId) == 1);
	CHECK_FALSE(Scene(a).Owns(noteId));
	CHECK_FALSE(Scene(b).Owns(noteId));

	a.Destroy();
	b.Destroy();
	CHECK(f.registry.Has(noteId));

	UnloadScene(host, f.registry);
	CHECK_FALSE(f.registry.Has(packed->Id()));
	CHECK_FALSE(f.registry.Has(noteId));
}

TEST_CASE("Instantiate remaps entity refs inside the template") {
	Fixture f;
	Scene prefab = CreateScene(f.world, "Bullet");
	Entity muzzle = prefab.CreateEntity("Muzzle");
	prefab.Root().Add(Aim{muzzle});
	f.save(prefab, "tmp://bullet.scene");
	UnloadScene(prefab, f.registry);

	Ref<PackedScene> packed = f.loadPacked("tmp://bullet.scene");
	Entity instance = packed->Instantiate(f.world, f.registry);
	REQUIRE(instance.IsAlive());
	CHECK(GetScenes(f.world).empty());
	CHECK_FALSE(FindScene(f.world, instance.GetName()).IsValid());

	Entity copiedMuzzle = instance.Find("Muzzle");
	REQUIRE(copiedMuzzle.IsAlive());
	REQUIRE(instance.Get<Aim>() != nullptr);
	CHECK(instance.Get<Aim>()->target == copiedMuzzle);
	CHECK(instance.Get<Aim>()->target != muzzle);

	Entity other = packed->Instantiate(f.world, f.registry);
	REQUIRE(other.IsAlive());
	REQUIRE(other.Get<Aim>() != nullptr);
	CHECK(other.Get<Aim>()->target == other.Find("Muzzle"));
	CHECK(other.Get<Aim>()->target != copiedMuzzle);
}

TEST_CASE("Instantiate result can be edited immediately") {
	Fixture f;
	Scene prefab = CreateScene(f.world, "Bullet");
	Transform2D transform;
	transform.rotation = 0.f;
	prefab.Root().Add(transform);
	f.save(prefab, "tmp://bullet.scene");
	UnloadScene(prefab, f.registry);

	Ref<PackedScene> packed = f.loadPacked("tmp://bullet.scene");
	Entity bullet = packed->Instantiate(f.world, f.registry);
	REQUIRE(bullet.Get<Transform2D>() != nullptr);
	bullet.Get<Transform2D>()->rotation = 90.f;
	CHECK(bullet.Get<Transform2D>()->rotation == doctest::Approx(90.f));
}

TEST_CASE("A host scene round-trips an instance without baking its children") {
	Fixture f;
	Scene prefab = CreateScene(f.world, "Bullet");
	prefab.CreateEntity("Sprite");
	f.save(prefab, "tmp://bullet.scene");
	UnloadScene(prefab, f.registry);

	Ref<PackedScene> packed = f.loadPacked("tmp://bullet.scene");
	Scene host = CreateScene(f.world, "Level");
	REQUIRE(host.Own(f.registry, packed->Id()));
	Entity instance = packed->Instantiate(f.world, f.registry, host.Root());
	instance.SetName("Shot");
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	instance.Add(transform);

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, host, f.registry));
	const std::string yaml = saving.ToString();
	CHECK(yaml.find("instance:") != std::string::npos);
	// The prefab child is live in the tree but must not be written into the host.
	CHECK(yaml.find("Sprite") == std::string::npos);

	UnloadScene(host, f.registry);
	CHECK_FALSE(instance.IsAlive());

	Archive loading = Archive::Load(yaml);
	REQUIRE(loading.IsValid());
	Scene loaded = LoadScene(loading, f.world, f.registry);
	REQUIRE(loaded.IsValid());

	Entity restored = f.world.Find("Level/Shot");
	REQUIRE(restored.IsAlive());
	CHECK(IsInstanceRoot(restored));
	CHECK(restored.Find("Sprite").IsAlive());
	CHECK(IsInstanceLocked(restored.Find("Sprite")));
	REQUIRE(restored.Get<Transform2D>() != nullptr);
	CHECK(restored.Get<Transform2D>()->position.x == doctest::Approx(10.f));
	CHECK(GetScenes(f.world).size() == 1);
}

TEST_CASE("Inside a system the tree exists now and parenting waits") {
	Fixture f;
	Scene prefab = CreateScene(f.world, "Bullet");
	Transform2D transform;
	prefab.Root().Add(transform);
	f.save(prefab, "tmp://bullet.scene");
	UnloadScene(prefab, f.registry);

	Ref<PackedScene> packed = f.loadPacked("tmp://bullet.scene");
	std::vector<Entity> guns;
	for (int i = 0; i < 3; ++i) {
		Entity gun = f.world.CreateEntity();
		gun.Add(Gun{});
		guns.push_back(gun);
	}

	i32 ran = 0;
	bool parentEmptyDuringSystem = false;
	bool rotationWritable = false;
	f.world.RegisterSystem<Gun>("Fire", [&](Entity gun, Gun &) {
		++ran;
		Entity bullet = packed->Instantiate(f.world, f.registry, gun);
		REQUIRE(bullet.IsAlive());
		REQUIRE(bullet.Get<Transform2D>() != nullptr);
		bullet.Get<Transform2D>()->rotation = 90.f;
		rotationWritable = bullet.Get<Transform2D>()->rotation == 90.f;
		if (!bullet.GetParent().IsAlive())
			parentEmptyDuringSystem = true;
	});

	f.world.Progress(0.f);

	CHECK(ran == 3);
	CHECK(rotationWritable);
	CHECK(parentEmptyDuringSystem);
	for (Entity gun : guns) {
		REQUIRE(gun.GetChildren().size() == 1);
		CHECK(gun.GetChildren()[0].Get<Transform2D>()->rotation == doctest::Approx(90.f));
	}
}

TEST_CASE("Instantiate without a loaded document is refused") {
	Fixture f;
	Ref<PackedScene> packed = f.registry.Create<PackedScene>();
	Log::Clear();
	CHECK_FALSE(packed->Instantiate(f.world, f.registry).IsAlive());
	CHECK_FALSE(Log::GetLines().empty());
	Log::Clear();
}
