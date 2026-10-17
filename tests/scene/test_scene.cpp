#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/components/camera2d.hpp"
#include "ecs/components/scene_root.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/world.hpp"
#include "scene/scene.hpp"

using namespace sowa;

TEST_CASE("A scene is a root entity carrying SceneRoot") {
	World world;
	Scene scene = CreateScene(world, "Level");

	REQUIRE(scene.IsValid());
	CHECK(scene.Name() == "Level");
	CHECK(scene.Root().IsAlive());
	CHECK(scene.Root().Has<SceneRoot>());
	// A top-level entity, so its name is the first segment of every path in it.
	CHECK_FALSE(scene.Root().HasParent());
}

TEST_CASE("A default-constructed scene is invalid rather than dangerous") {
	Scene scene;

	CHECK_FALSE(scene.IsValid());
	CHECK_FALSE(scene);
	CHECK(scene.Name().empty());
	CHECK(scene.SourcePath().empty());
	CHECK_FALSE(scene.ActiveCamera().IsAlive());
	CHECK_FALSE(scene.SetActiveCamera(Entity()));
	CHECK_FALSE(scene.CreateEntity("nope").IsAlive());
	CHECK_FALSE(scene.Root().IsAlive());
	scene.SetEnabled(false);
	CHECK_FALSE(scene.IsEnabled());
}

TEST_CASE("Entities created through the scene land under its root") {
	World world;
	Scene scene = CreateScene(world, "Level");

	Entity player = scene.CreateEntity("Player");
	REQUIRE(player.IsAlive());
	CHECK(player.GetParent() == scene.Root());
	// Paths start with the scene name.
	CHECK(player.GetPath() == "Level/Player");
	CHECK(world.Find("Level/Player") == player);
}

TEST_CASE("Scene names uniquify like any other name") {
	World world;
	Scene first = CreateScene(world, "Level");
	Scene second = CreateScene(world, "Level");

	CHECK(first.Name() == "Level");
	// Not a silent shadow: both stay reachable by path.
	CHECK(second.Name() != "Level");
	CHECK(FindScene(world, first.Name()).Root() == first.Root());
	CHECK(FindScene(world, second.Name()).Root() == second.Root());
}

// The entity-level operation: destroying the root takes the subtree with it.
// Unloading a *scene* is UnloadScene, which also releases its resources.
TEST_CASE("Destroying a scene root takes its subtree and nothing else") {
	World world;
	Scene doomed = CreateScene(world, "Doomed");
	Scene survivor = CreateScene(world, "Survivor");

	Entity parent = doomed.CreateEntity("Parent");
	Entity child = parent.CreateChild("Child");
	Entity grandchild = child.CreateChild("Grandchild");
	Entity bystander = survivor.CreateEntity("Bystander");

	doomed.Root().Destroy();

	CHECK_FALSE(doomed.IsValid());
	CHECK_FALSE(parent.IsAlive());
	CHECK_FALSE(child.IsAlive());
	CHECK_FALSE(grandchild.IsAlive());

	CHECK(survivor.IsValid());
	CHECK(bystander.IsAlive());
}

TEST_CASE("A scene whose root loses SceneRoot stops being a scene") {
	World world;
	Scene scene = CreateScene(world, "Level");
	REQUIRE(scene.IsValid());

	scene.Root().Remove<SceneRoot>();

	// Reported rather than handing out a root that no longer means anything.
	CHECK_FALSE(scene.IsValid());
	CHECK(GetScenes(world).empty());
}

TEST_CASE("The source path round-trips on the root") {
	World world;
	Scene scene = CreateScene(world, "Level");

	CHECK(scene.SourcePath().empty());
	scene.SetSourcePath("res://levels/one.scene");
	CHECK(scene.SourcePath() == "res://levels/one.scene");

	// It's a component field, so it's visible to reflection like anything else.
	const SceneRoot *root = scene.Root().Get<SceneRoot>();
	REQUIRE(root != nullptr);
	CHECK(root->sourcePath == "res://levels/one.scene");
}

TEST_CASE("The active camera is per scene, not per world") {
	World world;
	Scene first = CreateScene(world, "First");
	Scene second = CreateScene(world, "Second");

	Entity firstCamera = first.CreateEntity("Camera");
	firstCamera.Add(Camera2D{});
	Entity secondCamera = second.CreateEntity("Camera");
	secondCamera.Add(Camera2D{});

	CHECK(first.SetActiveCamera(firstCamera));
	CHECK(second.SetActiveCamera(secondCamera));

	// Each scene retains its own active camera.
	CHECK(first.ActiveCamera() == firstCamera);
	CHECK(second.ActiveCamera() == secondCamera);
}

TEST_CASE("A camera from another scene is refused") {
	World world;
	Scene first = CreateScene(world, "First");
	Scene second = CreateScene(world, "Second");

	Entity stranger = second.CreateEntity("Camera");
	stranger.Add(Camera2D{});

	// A scene rendering through another scene's camera is a mistake in every
	// case, and one that shows up as an inexplicable viewport rather than an
	// error unless it's refused here.
	CHECK_FALSE(first.SetActiveCamera(stranger));
	CHECK_FALSE(first.ActiveCamera().IsAlive());
}

TEST_CASE("A camera nested deep inside the scene is accepted") {
	World world;
	Scene scene = CreateScene(world, "Level");

	Entity rig = scene.CreateEntity("Rig");
	Entity arm = rig.CreateChild("Arm");
	Entity camera = arm.CreateChild("Camera");
	camera.Add(Camera2D{});

	CHECK(scene.SetActiveCamera(camera));
	CHECK(scene.ActiveCamera() == camera);
}

TEST_CASE("Setting a null camera clears it") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity camera = scene.CreateEntity("Camera");
	camera.Add(Camera2D{});
	REQUIRE(scene.SetActiveCamera(camera));

	CHECK(scene.SetActiveCamera(Entity()));
	CHECK_FALSE(scene.ActiveCamera().IsAlive());
}

TEST_CASE("A destroyed camera reads back as none, not as a stale handle") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity camera = scene.CreateEntity("Camera");
	camera.Add(Camera2D{});
	REQUIRE(scene.SetActiveCamera(camera));

	camera.Destroy();

	// Not merely "not alive" - an actual null handle, so a caller comparing
	// against Entity() gets the same answer as one calling IsAlive(). A dead
	// handle keeps its id and world and would compare unequal to Entity().
	CHECK_FALSE(scene.ActiveCamera().IsAlive());
	CHECK(scene.ActiveCamera() == Entity());
}

TEST_CASE("SceneOf finds the scene an entity belongs to, at any depth") {
	World world;
	Scene scene = CreateScene(world, "Level");

	Entity parent = scene.CreateEntity("Parent");
	Entity child = parent.CreateChild("Child");
	Entity grandchild = child.CreateChild("Grandchild");

	CHECK(SceneOf(grandchild).Root() == scene.Root());
	CHECK(SceneOf(parent).Root() == scene.Root());
	// A root is in its own scene.
	CHECK(SceneOf(scene.Root()).Root() == scene.Root());

	// An entity created outside any scene belongs to none.
	CHECK_FALSE(SceneOf(world.CreateEntity()).IsValid());
	CHECK_FALSE(SceneOf(Entity()).IsValid());
}

// A nested scene root belongs to its host rather than appearing as a loaded top-level scene.
TEST_CASE("A nested scene root belongs to the scene containing it") {
	World world;
	Scene host = CreateScene(world, "Host");
	Scene nested = CreateScene(world, "Nested");
	nested.Root().SetParent(host.CreateEntity("Slot"));

	CHECK(SceneOf(nested.Root()).Root() == nested.Root());
	// ...but only top-level roots count as open scenes.
	const std::vector<Scene> scenes = GetScenes(world);
	REQUIRE(scenes.size() == 1);
	CHECK(scenes[0].Root() == host.Root());
}

TEST_CASE("GetScenes lists every open scene") {
	World world;
	CHECK(GetScenes(world).empty());

	Scene first = CreateScene(world, "First");
	Scene second = CreateScene(world, "Second");
	CHECK(GetScenes(world).size() == 2);

	first.Root().Destroy();
	REQUIRE(GetScenes(world).size() == 1);
	CHECK(GetScenes(world)[0].Root() == second.Root());
}

TEST_CASE("GetScenes includes a disabled scene") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity player = scene.CreateEntity("Player");
	player.Add(Transform2D{});

	Query<Transform2D> transforms = world.CreateQuery<Transform2D>();
	REQUIRE(transforms.Count() == 1);

	scene.SetEnabled(false);
	CHECK_FALSE(scene.IsEnabled());
	CHECK_FALSE(player.IsEnabled());
	// Systems skip it; the open list must not.
	CHECK(transforms.Count() == 0);
	REQUIRE(GetScenes(world).size() == 1);
	CHECK(GetScenes(world)[0].Root() == scene.Root());

	scene.SetEnabled(true);
	CHECK(player.IsEnabled());
	CHECK(transforms.Count() == 1);
}

TEST_CASE("Scene::SetEnabled walks the subtree, because disable is not inherited") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity parent = scene.CreateEntity("Parent");
	Entity child = parent.CreateChild("Child");
	parent.Add(Transform2D{});
	child.Add(Transform2D{});

	Query<Transform2D> transforms = world.CreateQuery<Transform2D>();
	REQUIRE(transforms.Count() == 2);

	// Disabling only the root would leave children matching queries.
	scene.Root().SetEnabled(false);
	CHECK(transforms.Count() == 2);

	scene.SetEnabled(false);
	CHECK(transforms.Count() == 0);
	CHECK_FALSE(parent.IsEnabled());
	CHECK_FALSE(child.IsEnabled());

	scene.SetEnabled(true);
	CHECK(transforms.Count() == 2);
}

TEST_CASE("FindScene only answers for actual scene roots") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity plain = world.CreateEntity();
	plain.SetName("NotAScene");

	CHECK(FindScene(world, "Level").Root() == scene.Root());
	CHECK_FALSE(FindScene(world, "NotAScene").IsValid());
	CHECK_FALSE(FindScene(world, "Nonexistent").IsValid());
}

TEST_CASE("SceneRoot is a built-in with its fields described") {
	World world;
	const ComponentType type = world.FindComponent("SceneRoot");

	REQUIRE(type.IsValid());
	CHECK(type.FindField("sourcePath") != nullptr);
	CHECK(type.FindField("activeCamera") != nullptr);
}

// A scene root transform moves its entire subtree, including prefab instances.
TEST_CASE("A scene root can carry any component, including a transform") {
	World world;
	Scene scene = CreateScene(world, "Level");

	Transform2D rootTransform;
	rootTransform.position = Vector2(100.f, 50.f);
	scene.Root().Add(rootTransform);

	Entity child = scene.CreateEntity("Child");
	Transform2D childTransform;
	childTransform.position = Vector2(10.f, 5.f);
	child.Add(childTransform);

	world.Progress(0.f);

	// The child inherits the root's transform, so moving the scene moves it.
	CHECK(child.Get<Transform2D>()->WorldPosition().x == doctest::Approx(110.f));
	CHECK(child.Get<Transform2D>()->WorldPosition().y == doctest::Approx(55.f));
}

// Transform inheritance skips ancestors without Transform2D.
TEST_CASE("A scene root without a transform is transparent") {
	World world;
	Scene scene = CreateScene(world, "Level");

	Entity child = scene.CreateEntity("Child");
	Transform2D childTransform;
	childTransform.position = Vector2(10.f, 5.f);
	child.Add(childTransform);

	world.Progress(0.f);

	CHECK(child.Get<Transform2D>()->WorldPosition().x == doctest::Approx(10.f));
	CHECK(child.Get<Transform2D>()->WorldPosition().y == doctest::Approx(5.f));
}
