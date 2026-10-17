#include <doctest/doctest.h>

#include <filesystem>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/serialize/archive.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/parallax2d.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/filesystem_browser.hpp"
#include "editor/hierarchy.hpp"
#include "editor/scene_commands.hpp"
#include "editor/scene_documents.hpp"
#include "math/vector2.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "scene/scene_serializer.hpp"

using namespace sowa;

namespace {

struct SceneInstanceFixture {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	std::filesystem::path dir;

	SceneInstanceFixture() {
		dir = std::filesystem::temp_directory_path() / "sowa_editor_scene_instance_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("res", dir);
		resources.SetContext(ResourceContext{nullptr, &fs, &resources});
		resources.RegisterType<PackedScene>("PackedScene").Field("path", &PackedScene::path);
	}

	~SceneInstanceFixture() {
		std::filesystem::remove_all(dir);
	}

	void SavePrefab(std::string_view path) {
		Scene prefab = CreateScene(world, "Enemy");
		Entity visual = prefab.CreateEntity("Visual");
		visual.Add(Transform2D{});
		REQUIRE(SaveSceneToFile(fs, path, prefab, resources) == FsError::Ok);
		UnloadScene(prefab, resources);
	}
};

} // namespace

TEST_CASE("DisplayName uses GetName, or Entity when unnamed") {
	World world;
	Entity named = world.CreateEntity();
	named.SetName("Player");
	CHECK(editor::DisplayName(named) == "Player");

	Entity unnamed = world.CreateEntity();
	CHECK(editor::DisplayName(unnamed) == "Entity");
	CHECK(editor::DisplayName(Entity()) == "");
}

TEST_CASE("IsSceneRoot is only a top-level SceneRoot") {
	World world;
	Scene scene = CreateScene(world, "Level");
	CHECK(editor::IsSceneRoot(scene.Root()));
	Entity child = scene.CreateEntity("Child");
	CHECK_FALSE(editor::IsSceneRoot(child));
}

TEST_CASE("CanReparent rejects self, descendants, scene roots, and no-ops") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity parent = scene.CreateEntity("Parent");
	Entity child = parent.CreateChild("Child");
	Entity other = scene.CreateEntity("Other");

	CHECK_FALSE(editor::CanReparent(child, child));
	CHECK_FALSE(editor::CanReparent(parent, child));
	CHECK_FALSE(editor::CanReparent(scene.Root(), other));
	CHECK_FALSE(editor::CanReparent(child, parent));
	CHECK(editor::CanReparent(child, other));
	CHECK_FALSE(editor::CanReparent(Entity(), other));
}

TEST_CASE("TryReparent moves the entity and keeps world transform") {
	World world;
	Scene scene = CreateScene(world, "Level");

	Entity oldParent = scene.CreateEntity("Old");
	Transform2D oldT;
	oldT.position = Vector2(100.f, 0.f);
	oldParent.Add(oldT);

	Entity child = oldParent.CreateChild("Child");
	Transform2D childT;
	childT.position = Vector2(10.f, 0.f);
	child.Add(childT);

	Entity newParent = scene.CreateEntity("New");
	Transform2D newT;
	newT.position = Vector2(0.f, 0.f);
	newParent.Add(newT);

	REQUIRE(editor::TryReparent(child, newParent));
	CHECK(child.GetParent() == newParent);
	const Transform2D *t = child.Get<Transform2D>();
	REQUIRE(t != nullptr);
	// World x was 110. New parent is at 0, so local becomes 110.
	CHECK(t->position.x == doctest::Approx(110.f));
	CHECK(t->position.y == doctest::Approx(0.f));
}

TEST_CASE("Instance-locked entities cannot be dragged, reparented, or deleted") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity instance = scene.CreateEntity("Inst");
	instance.Add(SceneInstance{});
	Entity locked = instance.CreateChild("Child");
	Entity sibling = scene.CreateEntity("Sibling");

	CHECK(IsInstanceRoot(instance));
	CHECK(IsInstanceLocked(locked));
	CHECK_FALSE(editor::CanDrag(locked));
	CHECK_FALSE(editor::CanReparent(locked, sibling));
	CHECK_FALSE(editor::CanDelete(locked));
	CHECK_FALSE(editor::CanCreateChild(instance));
	CHECK_FALSE(editor::CanCreateChild(locked));
	CHECK_FALSE(editor::CanReparent(sibling, instance));
	CHECK_FALSE(editor::CanReparent(sibling, locked));

	CHECK(editor::CanDrag(instance));
	CHECK(editor::CanDelete(instance));
	CHECK_FALSE(editor::TryReparent(locked, sibling));
	CHECK(locked.GetParent() == instance);
	CHECK_FALSE(editor::TryDelete(locked));
	CHECK(locked.IsAlive());
}

TEST_CASE("TryDelete destroys a child and refuses the scene root") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity child = scene.CreateEntity("Child");
	Entity grandchild = child.CreateChild("Grand");

	CHECK_FALSE(editor::CanDelete(scene.Root()));
	CHECK_FALSE(editor::TryDelete(scene.Root()));
	CHECK(scene.IsValid());

	REQUIRE(editor::TryDelete(child));
	CHECK_FALSE(child.IsAlive());
	CHECK_FALSE(grandchild.IsAlive());
}

TEST_CASE("SpawnChild applies each archetype's components") {
	World world;
	world.RegisterComponent<Sprite>("Sprite");
	Scene scene = CreateScene(world, "Level");
	Entity root = scene.Root();

	Entity empty = editor::SpawnChild(root, editor::EntityArchetype::Empty);
	REQUIRE(empty.IsAlive());
	CHECK(empty.GetParent() == root);
	CHECK(empty.GetName() == "Entity");
	CHECK_FALSE(empty.Has<Transform2D>());

	Entity node = editor::SpawnChild(root, editor::EntityArchetype::Node2D);
	REQUIRE(node.Has<Transform2D>());
	CHECK_FALSE(node.Has<Sprite>());
	CHECK_FALSE(node.Has<Camera2D>());
	CHECK(node.GetName() == "Node2D");

	Entity sprite = editor::SpawnChild(root, editor::EntityArchetype::Sprite);
	REQUIRE(sprite.Has<Transform2D>());
	CHECK(sprite.Has<Sprite>());
	CHECK(sprite.GetName() == "Sprite");

	Entity camera = editor::SpawnChild(root, editor::EntityArchetype::Camera2D);
	REQUIRE(camera.Has<Transform2D>());
	CHECK(camera.Has<Camera2D>());
	CHECK(camera.GetName() == "Camera2D");

	Entity parallax = editor::SpawnChild(root, editor::EntityArchetype::Parallax2D);
	REQUIRE(parallax.Has<Transform2D>());
	CHECK(parallax.Has<Parallax2D>());
	CHECK(parallax.GetName() == "Parallax2D");
}

TEST_CASE("SpawnChild refuses instance-locked parents") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity instance = scene.CreateEntity("Inst");
	instance.Add(SceneInstance{});
	CHECK_FALSE(editor::SpawnChild(instance, editor::EntityArchetype::Empty).IsAlive());
}

TEST_CASE("hierarchy scene instantiation creates one reusable owned PackedScene") {
	SceneInstanceFixture f;
	f.SavePrefab("res://enemy.scene");
	Scene host = CreateScene(f.world, "Level");
	Entity parent = host.CreateEntity("Enemies");

	const editor::SceneInstantiationResult first = editor::TryInstantiateScene(
		parent, "res:///enemy.scene", false, f.fs, f.world, f.resources);
	REQUIRE(first);
	CHECK(first.ownershipAdded);
	CHECK(first.path == "res://enemy.scene");
	CHECK(first.instance.GetParent() == parent);
	CHECK(first.instance.GetName() == "Enemy");
	CHECK(IsInstanceRoot(first.instance));
	Entity visual = first.instance.Find("Visual");
	REQUIRE(visual.IsAlive());
	CHECK(IsInstanceLocked(visual));

	const SceneInstance *firstMarker = first.instance.Get<SceneInstance>();
	REQUIRE(firstMarker != nullptr);
	const ResourceID packedId = firstMarker->packed;
	CHECK(host.Owns(packedId));
	CHECK(f.resources.OwnerCount(packedId) == 1);
	CHECK(f.resources.Count() == 1);

	const editor::SceneInstantiationResult second = editor::TryInstantiateScene(
		parent, "res://enemy.scene", false, f.fs, f.world, f.resources);
	REQUIRE(second);
	CHECK_FALSE(second.ownershipAdded);
	REQUIRE(second.instance.Get<SceneInstance>() != nullptr);
	CHECK(second.instance.Get<SceneInstance>()->packed == packedId);
	CHECK(second.instance.GetName() == "Enemy2");
	CHECK(f.resources.Count() == 1);
	CHECK(f.resources.OwnerCount(packedId) == 1);

	Archive saved = Archive::Save();
	REQUIRE(SaveScene(saved, host, f.resources));
	const std::string document = saved.ToString();
	CHECK(document.find("instance:") != std::string::npos);
	CHECK(document.find("name: Visual") == std::string::npos);

	UnloadScene(host, f.resources);
	CHECK(f.resources.Count() == 0);
	Archive loading = Archive::Load(document);
	Scene restored = LoadScene(loading, f.world, f.resources);
	REQUIRE(restored.IsValid());
	Entity restoredParent = restored.Root().Find("Enemies");
	REQUIRE(restoredParent.IsAlive());
	const std::vector<Entity> restoredInstances = restoredParent.GetChildren();
	REQUIRE(restoredInstances.size() == 2);
	CHECK(IsInstanceRoot(restoredInstances[0]));
	CHECK(IsInstanceRoot(restoredInstances[1]));
	CHECK(IsInstanceLocked(restoredInstances[0].Find("Visual")));
}

TEST_CASE("undoing scene instantiation releases host ownership without losing redo") {
	SceneInstanceFixture f;
	f.SavePrefab("res://enemy.scene");
	Scene host = CreateScene(f.world, "Level");
	host.SetSourcePath("res://level.scene");
	Entity parent = host.CreateEntity("Enemies");
	std::vector<editor::SceneDocumentSaveState> states;
	editor::UpdateSceneDocumentStates(states, {host});

	const editor::SceneInstantiationResult result = editor::TryInstantiateScene(
		parent, "res://enemy.scene", false, f.fs, f.world, f.resources);
	REQUIRE(result);
	REQUIRE(result.ownershipAdded);
	REQUIRE(editor::RecordInstantiatedScene(
		states, host, result.instance, f.resources, result.packed,
		result.ownershipAdded));
	CHECK(host.Owns(result.packed));
	CHECK(f.resources.OwnerCount(result.packed) == 2);

	REQUIRE(editor::UndoSceneDocument(states, host));
	CHECK_FALSE(host.Owns(result.packed));
	CHECK(f.resources.Count() == 1);
	Archive withoutInstance = Archive::Save();
	REQUIRE(SaveScene(withoutInstance, host, f.resources));
	CHECK(withoutInstance.ToString().find("resources:") == std::string::npos);

	REQUIRE(editor::RedoSceneDocument(states, host));
	CHECK(host.Owns(result.packed));
	CHECK(result.instance.IsDescendantOf(parent));
	states.clear();
	CHECK(f.resources.OwnerCount(result.packed) == 1);
}

TEST_CASE("hierarchy scene instantiation rejects unsafe targets without leaking") {
	SceneInstanceFixture f;
	f.SavePrefab("res://host.scene");
	Scene host = CreateScene(f.world, "Level");
	host.SetSourcePath("res://host.scene");
	Entity parent = host.CreateEntity("Parent");

	const editor::SceneInstantiationResult self = editor::TryInstantiateScene(
		parent, "res://host.scene", false, f.fs, f.world, f.resources);
	CHECK_FALSE(self);
	CHECK(self.error.find("itself") != std::string::npos);
	CHECK(f.resources.Count() == 0);

	const editor::SceneInstantiationResult playing = editor::TryInstantiateScene(
		parent, "res://host.scene", true, f.fs, f.world, f.resources);
	CHECK_FALSE(playing);
	CHECK(f.resources.Count() == 0);

	Entity instanceRoot = host.CreateEntity("Instance");
	instanceRoot.Add(SceneInstance{});
	const editor::SceneInstantiationResult locked = editor::TryInstantiateScene(
		instanceRoot, "res://host.scene", false, f.fs, f.world, f.resources);
	CHECK_FALSE(locked);
	CHECK(f.resources.Count() == 0);

	const editor::SceneInstantiationResult missing = editor::TryInstantiateScene(
		parent, "res://missing.scene", false, f.fs, f.world, f.resources);
	CHECK_FALSE(missing);
	CHECK(f.resources.Count() == 0);
}

TEST_CASE("Instance source action resolves the packed resource and opens or focuses its document") {
	SceneInstanceFixture f;
	f.SavePrefab("res://enemy.scene");
	SceneManager scenes(f.world, f.resources, f.fs);
	Scene host = scenes.Create("Host");
	REQUIRE(scenes.SetCurrent(host));
	const auto result = editor::TryInstantiateScene(host.Root(), "res://enemy.scene", false, f.fs,
		f.world, f.resources);
	REQUIRE(result);
	// Instance SceneRoot metadata can differ from its actual packed resource.
	Scene(result.instance).SetSourcePath("res://wrong.scene");
	const auto path = editor::InstanceSourcePath(result.instance, f.resources);
	CHECK(path == "res://enemy.scene");
	CHECK(editor::InstanceSourcePath(host.Root(), f.resources).empty());
	CHECK(editor::InstanceSourcePath(result.instance.Find("Visual"), f.resources).empty());
	REQUIRE(editor::TryOpenAsset(scenes, path, editor::AssetKind::Scene));
	const auto source = scenes.Current();
	CHECK(source.SourcePath() == path);
	CHECK_FALSE(host.IsEnabled());
	REQUIRE(editor::ActivateSceneDocument(scenes, host));
	REQUIRE(editor::TryOpenAsset(scenes, path, editor::AssetKind::Scene));
	CHECK(scenes.Current().Root() == source.Root());
	CHECK(scenes.Loaded().size() == 2);
	f.resources.Remove(result.packed);
	CHECK(editor::InstanceSourcePath(result.instance, f.resources).empty());
	CHECK(editor::InstanceSourcePath(Entity(), f.resources).empty());
}

TEST_CASE("PlaySelection clears on Play and finds the editor selection again after Stop") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);
	Scene level = scenes.Create("Level");
	Entity player = level.CreateEntity("Player");
	REQUIRE(scenes.SetCurrent(level));

	editor::PlaySelection selection;
	CHECK(selection.Update(false, player, world) == player);

	scenes.PlayCurrent();
	scenes.Flush();
	CHECK_FALSE(selection.Update(true, player, world).IsAlive());
	const Entity live = scenes.Current().Root().Find("Player");
	REQUIRE(live.IsAlive());
	CHECK(selection.Update(true, live, world) == live);

	scenes.Stop();
	scenes.Flush();
	// Stop rebuilds the document from its snapshot: the old handle is gone.
	REQUIRE_FALSE(player.IsAlive());
	const Entity restored = selection.Update(false, live, world);
	REQUIRE(restored.IsAlive());
	CHECK(restored == world.Find("Level/Player"));
	CHECK(selection.Update(false, restored, world) == restored);
}

TEST_CASE("PlaySelection restores nothing when nothing was selected before Play") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);
	Scene level = scenes.Create("Level");
	level.CreateEntity("Player");
	REQUIRE(scenes.SetCurrent(level));

	editor::PlaySelection selection;
	selection.Update(false, Entity(), world);
	scenes.PlayCurrent();
	scenes.Flush();
	CHECK_FALSE(selection.Update(true, Entity(), world).IsAlive());
	const Entity live = scenes.Current().Root().Find("Player");
	CHECK(selection.Update(true, live, world) == live);
	scenes.Stop();
	scenes.Flush();
	CHECK_FALSE(selection.Update(false, live, world).IsAlive());
}
