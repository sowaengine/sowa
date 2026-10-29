#include <filesystem>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "animation/animation.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/serialize/archive.hpp"
#include "ecs/component_type.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/scene_root.hpp"
#include "ecs/components/tilemap.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/visibility.hpp"
#include "ecs/world.hpp"
#include "resource/font.hpp"
#include "resource/resource.hpp"
#include "resource/resource_registry.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include "scene/scene.hpp"
#include "scene/scene_serializer.hpp"
#include "ui/theme.hpp"

using namespace sowa;

namespace {

struct ResourceLink : Resource {
	std::string label;
	ResourceID next;
};

// Saves scene and loads it back into a *different* world, so nothing can
// pass by accidentally still pointing at the original entities.
ResourceRegistry &scratchRegistry() {
	static ResourceRegistry registry;
	return registry;
}

Scene RoundTrip(Scene scene, World &target, bool *hadErrors = nullptr) {
	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, scratchRegistry()));

	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(loading.IsValid());
	Scene loaded = LoadScene(loading, target, scratchRegistry());
	if (hadErrors != nullptr)
		*hadErrors = loading.HasErrors();
	return loaded;
}

} // namespace

TEST_CASE("A scene round-trips its root, name and metadata") {
	World source;
	Scene scene = CreateScene(source, "Level");
	scene.SetSourcePath("res://levels/one.scene");

	World target;
	Scene loaded = RoundTrip(scene, target);

	REQUIRE(loaded.IsValid());
	CHECK(loaded.Name() == "Level");
	CHECK(loaded.SourcePath() == "res://levels/one.scene");
	// The root is an ordinary entity carrying SceneRoot, so it comes back the
	// same way everything else does.
	CHECK(loaded.Root().Has<SceneRoot>());
}

TEST_CASE("Component values survive the round trip") {
	World source;
	Scene scene = CreateScene(source, "Level");
	Entity player = scene.CreateEntity("Player");

	Transform2D transform;
	transform.position = Vector2(10.f, -20.f);
	transform.rotation = 45.f;
	transform.scale = Vector2(2.f, 3.f);
	player.Add(transform);
	player.Add(RenderOrder{7, ZIndexMode::Absolute, 2, RenderLayerMode::Explicit});

	World target;
	Scene loaded = RoundTrip(scene, target);
	REQUIRE(loaded.IsValid());

	Entity restored = target.Find("Level/Player");
	REQUIRE(restored.IsAlive());

	const Transform2D *t = restored.Get<Transform2D>();
	REQUIRE(t != nullptr);
	CHECK(t->position.x == doctest::Approx(10.f));
	CHECK(t->position.y == doctest::Approx(-20.f));
	CHECK(t->rotation == doctest::Approx(45.f));
	CHECK(t->scale.y == doctest::Approx(3.f));

	const RenderOrder *order = restored.Get<RenderOrder>();
	REQUIRE(order != nullptr);
	CHECK(order->zIndex == 7);
	// Enums come back by name, so renumbering wouldn't silently repoint them.
	CHECK(order->zIndexMode == ZIndexMode::Absolute);
	CHECK(order->layerMode == RenderLayerMode::Explicit);
}

TEST_CASE("The hierarchy survives to arbitrary depth") {
	World source;
	Scene scene = CreateScene(source, "Level");
	Entity a = scene.CreateEntity("A");
	Entity b = a.CreateChild("B");
	Entity c = b.CreateChild("C");
	(void)c;

	World target;
	Scene loaded = RoundTrip(scene, target);
	REQUIRE(loaded.IsValid());

	Entity restored = target.Find("Level/A/B/C");
	REQUIRE(restored.IsAlive());
	CHECK(restored.GetParent().GetName() == "B");
	CHECK(restored.GetParent().GetParent().GetName() == "A");
	CHECK(restored.GetParent().GetParent().GetParent() == loaded.Root());
}

// Document sequence order must preserve child insertion order.
TEST_CASE("Child order is preserved, not just membership") {
	World source;
	Scene scene = CreateScene(source, "Level");
	for (const char *name : {"First", "Second", "Third", "Fourth"})
		scene.CreateEntity(name);

	World target;
	Scene loaded = RoundTrip(scene, target);
	REQUIRE(loaded.IsValid());

	const std::vector<Entity> children = loaded.Root().GetChildren();
	REQUIRE(children.size() == 4);
	CHECK(children[0].GetName() == "First");
	CHECK(children[1].GetName() == "Second");
	CHECK(children[2].GetName() == "Third");
	CHECK(children[3].GetName() == "Fourth");
}

TEST_CASE("Saving twice produces byte-identical documents") {
	World source;
	Scene scene = CreateScene(source, "Level");
	Entity a = scene.CreateEntity("A");
	a.Add(Transform2D{});
	scene.CreateEntity("B").Add(RenderOrder{3});

	Archive first = Archive::Save();
	SaveScene(first, scene, scratchRegistry());
	Archive second = Archive::Save();
	SaveScene(second, scene, scratchRegistry());

	// Repeated saves must preserve ordering.
	CHECK(first.ToString() == second.ToString());
}

// Sort component names so file order does not depend on registration order.
TEST_CASE("Components are written in name order, not archetype order") {
	World world;
	Scene scene = CreateScene(world, "Level");
	Entity entity = scene.CreateEntity("Thing");
	// Added in an order that is neither alphabetical nor registration order.
	entity.Add(RenderOrder{1});
	entity.Add(Camera2D{});
	entity.Add(Transform2D{});

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, scratchRegistry()));
	const std::string yaml = saving.ToString();

	const size_t camera = yaml.find("Camera2D:");
	const size_t order = yaml.find("RenderOrder:");
	const size_t transform = yaml.find("Transform2D:");
	REQUIRE(camera != std::string::npos);
	REQUIRE(order != std::string::npos);
	REQUIRE(transform != std::string::npos);
	CHECK(camera < order);
	CHECK(order < transform);
}

TEST_CASE("Groups round-trip") {
	World source;
	Scene scene = CreateScene(source, "Level");
	Entity enemy = scene.CreateEntity("Enemy");
	enemy.AddGroup("hostile");
	enemy.AddGroup("spawned");

	World target;
	Scene loaded = RoundTrip(scene, target);
	REQUIRE(loaded.IsValid());

	Entity restored = target.Find("Level/Enemy");
	REQUIRE(restored.IsAlive());
	CHECK(restored.IsInGroup("hostile"));
	CHECK(restored.IsInGroup("spawned"));
	CHECK_FALSE(restored.IsInGroup("friendly"));
}

// Hidden has no fields, so only its presence carries meaning - which is why it
// had to become a registered component to be writable at all.
TEST_CASE("A hidden entity comes back hidden") {
	World source;
	Scene scene = CreateScene(source, "Level");
	Entity visible = scene.CreateEntity("Visible");
	Entity hidden = scene.CreateEntity("Hidden");
	hidden.SetVisible(false);
	(void)visible;

	World target;
	Scene loaded = RoundTrip(scene, target);
	REQUIRE(loaded.IsValid());

	CHECK(target.Find("Level/Visible").IsVisible());
	CHECK_FALSE(target.Find("Level/Hidden").IsVisible());
}

// Resolve forward references only after loading every entity.
TEST_CASE("An entity reference resolves against the loaded scene") {
	World source;
	Scene scene = CreateScene(source, "Level");
	Entity camera = scene.CreateEntity("Camera");
	camera.Add(Camera2D{});
	REQUIRE(scene.SetActiveCamera(camera));

	World target;
	bool hadErrors = true;
	Scene loaded = RoundTrip(scene, target, &hadErrors);
	REQUIRE(loaded.IsValid());

	Entity restoredCamera = target.Find("Level/Camera");
	REQUIRE(restoredCamera.IsAlive());
	CHECK(loaded.ActiveCamera() == restoredCamera);
	// And it isn't reported as a problem, which is what the default handling
	// would have done.
	CHECK_FALSE(hadErrors);
}

TEST_CASE("A second load's entity refs bind inside that scene, not the first") {
	World world;
	Scene original = CreateScene(world, "Level");
	Entity camera = original.CreateEntity("Camera");
	camera.Add(Camera2D{});
	REQUIRE(original.SetActiveCamera(camera));

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, original, scratchRegistry()));

	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(loading.IsValid());
	Scene copy = LoadScene(loading, world, scratchRegistry());
	REQUIRE(copy.IsValid());
	REQUIRE(copy.Name() != original.Name());

	Entity copyCamera = copy.Root().Find("Camera");
	REQUIRE(copyCamera.IsAlive());
	CHECK(copy.ActiveCamera() == copyCamera);
	CHECK(copy.ActiveCamera() != camera);
	CHECK(original.ActiveCamera() == camera);
}

TEST_CASE("A forward reference resolves too") {
	World source;
	Scene scene = CreateScene(source, "Level");
	// The camera is created *after* the root that names it, so on load the
	// reference is read before its target exists.
	Entity rig = scene.CreateEntity("Rig");
	Entity camera = rig.CreateChild("Camera");
	camera.Add(Camera2D{});
	REQUIRE(scene.SetActiveCamera(camera));

	World target;
	Scene loaded = RoundTrip(scene, target);
	REQUIRE(loaded.IsValid());
	CHECK(loaded.ActiveCamera() == target.Find("Level/Rig/Camera"));
}

TEST_CASE("A reference to a missing entity is reported, not silently dropped") {
	World world;
	Archive loading = Archive::Load(
		"root:\n"
		"  name: Level\n"
		"  components:\n"
		"    SceneRoot:\n"
		"      sourcePath: \"\"\n"
		"      activeCamera: Level/Nowhere\n");
	REQUIRE(loading.IsValid());

	Scene loaded = LoadScene(loading, world, scratchRegistry());
	REQUIRE(loaded.IsValid());
	CHECK_FALSE(loaded.ActiveCamera().IsAlive());
	CHECK(loading.HasErrors());
}

TEST_CASE("An unknown component name is reported and the rest still loads") {
	World world;
	Archive loading = Archive::Load(
		"root:\n"
		"  name: Level\n"
		"  components:\n"
		"    SceneRoot: {sourcePath: \"\"}\n"
		"  children:\n"
		"    - name: Player\n"
		"      components:\n"
		"        Transform2D: {position: [5, 6]}\n"
		"        NotAComponent: {value: 1}\n");
	REQUIRE(loading.IsValid());

	Scene loaded = LoadScene(loading, world, scratchRegistry());
	REQUIRE(loaded.IsValid());

	Entity player = world.Find("Level/Player");
	REQUIRE(player.IsAlive());
	// The known component still landed - a damaged file loads as far as it can.
	REQUIRE(player.Get<Transform2D>() != nullptr);
	CHECK(player.Get<Transform2D>()->position.x == doctest::Approx(5.f));
	CHECK(loading.HasErrors());
}

TEST_CASE("A document with no root yields no scene") {
	World world;
	Archive loading = Archive::Load("something: else\n");
	REQUIRE(loading.IsValid());

	CHECK_FALSE(LoadScene(loading, world, scratchRegistry()).IsValid());
}

TEST_CASE("A root without SceneRoot loads as an entity that isn't a scene") {
	World world;
	Archive loading = Archive::Load("root:\n  name: Loose\n");
	REQUIRE(loading.IsValid());

	// Reported by IsValid rather than by fabricating the component, so a file
	// that isn't a scene can't quietly become one.
	CHECK_FALSE(LoadScene(loading, world, scratchRegistry()).IsValid());
	CHECK(world.Find("Loose").IsAlive());
}

TEST_CASE("Saving an invalid scene fails rather than writing an empty document") {
	Archive archive = Archive::Save();
	CHECK_FALSE(SaveScene(archive, Scene(), scratchRegistry()));
}

TEST_CASE("Two loads of one document coexist, with names uniquified") {
	World world;
	Scene original = CreateScene(world, "Level");
	original.CreateEntity("Player").Add(Transform2D{});

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, original, scratchRegistry()));

	Archive first = Archive::Load(saving.ToString());
	Archive second = Archive::Load(saving.ToString());
	Scene a = LoadScene(first, world, scratchRegistry());
	Scene b = LoadScene(second, world, scratchRegistry());

	REQUIRE(a.IsValid());
	REQUIRE(b.IsValid());
	// Unique scene names keep both roots reachable by path.
	CHECK(a.Name() != b.Name());
	CHECK(a.Root() != b.Root());
	CHECK(GetScenes(world).size() == 3);
}

TEST_CASE("A scene round-trips through a file") {
	const auto dir = std::filesystem::temp_directory_path() / "sowa_scene_file_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);

	FileSystem fs;
	fs.MountFolder("tmp", dir);

	World source;
	Scene scene = CreateScene(source, "Level");
	Entity player = scene.CreateEntity("Player");
	Transform2D transform;
	transform.position = Vector2(3.f, 4.f);
	player.Add(transform);

	CHECK(SaveSceneToFile(fs, "tmp://level.scene", scene, scratchRegistry()) == FsError::Ok);

	World target;
	Scene loaded = LoadSceneFromFile(fs, "tmp://level.scene", target, scratchRegistry());
	REQUIRE(loaded.IsValid());
	// Loading stamps where it came from, so saving back needs no argument.
	CHECK(loaded.SourcePath() == "tmp://level.scene");
	CHECK(target.Find("Level/Player").Get<Transform2D>()->position.x == doctest::Approx(3.f));

	std::filesystem::remove_all(dir);
}

TEST_CASE("Loading a missing or malformed file yields no scene") {
	const auto dir = std::filesystem::temp_directory_path() / "sowa_scene_file_bad";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);

	FileSystem fs;
	fs.MountFolder("tmp", dir);

	World world;
	CHECK_FALSE(LoadSceneFromFile(fs, "tmp://nonexistent.scene", world, scratchRegistry()).IsValid());

	FileData junk;
	const std::string text = "root:\n  - this is: not a mapping\n   bad indent\n";
	junk.bytes.assign(reinterpret_cast<const std::byte *>(text.data()),
					  reinterpret_cast<const std::byte *>(text.data() + text.size()));
	REQUIRE(fs.Save("tmp://broken.scene", junk) == FsError::Ok);
	CHECK_FALSE(LoadSceneFromFile(fs, "tmp://broken.scene", world, scratchRegistry()).IsValid());

	std::filesystem::remove_all(dir);
}



// Scene-owned resources.

namespace {

struct Note : public Resource {
	std::string text;
};

// A registry with a type the tests can create freely, plus a world, since
// resource round-trips need both sides.
struct ResourceFixture {
	ResourceRegistry registry;

	ResourceFixture() {
		registry.RegisterType<Note>("Note").Field("text", &Note::text);
	}

	ResourceID Make(Scene scene, const std::string &text) {
		Ref<Note> note = registry.Create<Note>();
		note->text = text;
		scene.Own(registry, note->Id());
		return note->Id();
	}
};

} // namespace

TEST_CASE("A scene tracks what it owns, in order, without duplicates") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	const ResourceID first = fixture.Make(scene, "one");
	const ResourceID second = fixture.Make(scene, "two");

	CHECK(scene.Owns(first));
	CHECK(scene.Owns(second));
	REQUIRE(scene.OwnedResources().size() == 2);
	CHECK(scene.OwnedResources()[0] == first);
	CHECK(scene.OwnedResources()[1] == second);

	// Owning twice would release twice on unload.
	CHECK_FALSE(scene.Own(fixture.registry, first));
	CHECK(scene.OwnedResources().size() == 2);

	CHECK_FALSE(scene.Own(fixture.registry, ResourceID{}));
	CHECK(scene.OwnedResources().size() == 2);
}

// Disowning drops a claim rather than merely forgetting one. Handing a
// resource to something that outlives the scene therefore means that something
// retains it *first* - which is ordinary refcount discipline, and was only
// avoidable while ownership was an uncounted list.
TEST_CASE("Disowning releases, so a hand-off retains first") {
	World world;
	ResourceFixture fixture;

	SUBCASE("the last owner disowning lets the resource go") {
		Scene scene = CreateScene(world, "Level");
		const ResourceID id = fixture.Make(scene, "dropped");
		REQUIRE(fixture.registry.OwnerCount(id) == 1);

		CHECK(scene.Disown(fixture.registry, id));
		CHECK_FALSE(scene.Owns(id));
		CHECK_FALSE(fixture.registry.Has(id));
		// Nothing to disown twice.
		CHECK_FALSE(scene.Disown(fixture.registry, id));
	}

	SUBCASE("a prior retain is what makes it survive") {
		Scene scene = CreateScene(world, "Level");
		const ResourceID id = fixture.Make(scene, "handed on");

		REQUIRE(fixture.registry.Retain(id)); // the new holder claims it
		CHECK(fixture.registry.OwnerCount(id) == 2);
		CHECK(scene.Disown(fixture.registry, id));

		CHECK(fixture.registry.Has(id));
		CHECK(fixture.registry.OwnerCount(id) == 1);
	}
}

TEST_CASE("Unloading a scene releases its resources and its entities") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	Entity entity = scene.CreateEntity("Thing");
	const ResourceID id = fixture.Make(scene, "doomed");
	REQUIRE(fixture.registry.Has(id));

	UnloadScene(scene, fixture.registry);

	CHECK_FALSE(fixture.registry.Has(id));
	CHECK_FALSE(entity.IsAlive());
	CHECK_FALSE(scene.IsValid());
	// The handle is consumed, not merely stale - which is what taking it by
	// reference is announcing. IsValid() would report false either way, since
	// the root is dead; this is the stronger promise.
	CHECK(scene.Root() == Entity());
}

// Registry removal must preserve resources still held by callers through Ref.
TEST_CASE("Unloading can't pull a resource out from under a live holder") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	Ref<Note> held = fixture.registry.Create<Note>();
	held->text = "in use";
	scene.Own(fixture.registry, held->Id());
	const ResourceID id = held->Id();

	UnloadScene(scene, fixture.registry);

	CHECK_FALSE(fixture.registry.Has(id));
	// The holder's object is intact; only the registry's claim went away.
	CHECK(held->text == "in use");
	CHECK(held.use_count() == 1);
}

// Disown is how a resource outlives its scene - the reason there is no second
// teardown function that skips the release.
TEST_CASE("A disowned resource survives the unload") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	const ResourceID kept = fixture.Make(scene, "kept");
	const ResourceID released = fixture.Make(scene, "released");

	// Retained by whatever is taking it over, then disowned by the scene -
	// dropping the scene's claim without dropping the resource.
	REQUIRE(fixture.registry.Retain(kept));
	REQUIRE(scene.Disown(fixture.registry, kept));
	UnloadScene(scene, fixture.registry);

	CHECK(fixture.registry.Has(kept));
	CHECK(fixture.registry.OwnerCount(kept) == 1);
	CHECK_FALSE(fixture.registry.Has(released));
}

// Root().Destroy() is the entity operation and knows nothing about scene
// resources, which is exactly why it reads as an entity operation rather than
// as a scene one that quietly does less than its name suggests.
TEST_CASE("Destroying the root entity directly does not release resources") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(scene, "orphaned");

	scene.Root().Destroy();

	CHECK(fixture.registry.Has(id));
}

TEST_CASE("Owned resources round-trip with the scene") {
	World source;
	ResourceFixture fixture;
	Scene scene = CreateScene(source, "Level");
	const ResourceID id = fixture.Make(scene, "hello");

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, fixture.registry));

	// Wiped, so nothing can pass by the original still being present.
	UnloadScene(scene, fixture.registry);
	REQUIRE(fixture.registry.Count() == 0);

	World target;
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(loading.IsValid());
	Scene loaded = LoadScene(loading, target, fixture.registry);
	REQUIRE(loaded.IsValid());
	CHECK_FALSE(loading.HasErrors());

	// Restored under the same id, which is the property that makes component
	// references resolve with no fixup.
	CHECK(loaded.Owns(id));
	Ref<Note> restored = fixture.registry.Get<Note>(id);
	REQUIRE(restored != nullptr);
	CHECK(restored->text == "hello");
	CHECK(restored->Id() == id);
}

// Restored IDs require no pointer fixups.
TEST_CASE("A component's resource reference resolves after a round trip") {
	World source;
	ResourceFixture fixture;
	Scene scene = CreateScene(source, "Level");
	const ResourceID id = fixture.Make(scene, "texture-ish");

	// A component field holding a ResourceID, standing in for Sprite::texture.
	EcsResult<ComponentType> holder = source.RegisterComponent(
		"Holder", {{"resource", FieldType::ResourceRef}});
	REQUIRE(holder);
	Entity entity = scene.CreateEntity("Thing");
	entity.Add(holder.value);
	REQUIRE(entity.GetField(holder.value, "resource").Set(id));

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, fixture.registry));
	UnloadScene(scene, fixture.registry);

	World target;
	target.RegisterComponent("Holder", {{"resource", FieldType::ResourceRef}});
	Archive loading = Archive::Load(saving.ToString());
	Scene loaded = LoadScene(loading, target, fixture.registry);
	REQUIRE(loaded.IsValid());

	Entity restored = target.Find("Level/Thing");
	REQUIRE(restored.IsAlive());
	ResourceID restoredId;
	REQUIRE(restored.GetField(target.FindComponent("Holder"), "resource").Get(restoredId));
	CHECK(restoredId == id);
	// And it points at a resource that actually exists again.
	REQUIRE(fixture.registry.Get<Note>(restoredId) != nullptr);
	CHECK(fixture.registry.Get<Note>(restoredId)->text == "texture-ish");
}

TEST_CASE("A scene with no resources writes no resources block") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, fixture.registry));
	CHECK(saving.ToString().find("resources") == std::string::npos);
}

TEST_CASE("A resource record of an unknown type is reported") {
	World world;
	ResourceRegistry empty;
	Archive loading = Archive::Load(
		"resources:\n"
		"  - id: 42\n"
		"    type: NotARegisteredType\n"
		"    data: {}\n"
		"root:\n"
		"  name: Level\n"
		"  components:\n"
		"    SceneRoot: {sourcePath: \"\"}\n");
	REQUIRE(loading.IsValid());

	Scene loaded = LoadScene(loading, world, empty);
	// The scene still loads; only the resource is missing.
	REQUIRE(loaded.IsValid());
	CHECK(loaded.OwnedResources().empty());
	CHECK(loading.HasErrors());
}

TEST_CASE("Owning a resource that has since been removed is reported on save") {
	World world;
	ResourceFixture fixture;
	Scene scene = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(scene, "vanishing");

	// Removed without disowning, so the scene's list outlives the resource.
	REQUIRE(fixture.registry.Remove(id));

	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, scene, fixture.registry));
	CHECK(saving.HasErrors());
}

// Scenes sharing a resource ID must share ownership so unloading either keeps the
// other scene's references valid.
TEST_CASE("Two scenes loading the same resource share it") {
	World world;
	ResourceFixture fixture;

	Scene original = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(original, "shared");
	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, original, fixture.registry));
	UnloadScene(original, fixture.registry);

	Archive firstDoc = Archive::Load(saving.ToString());
	Archive secondDoc = Archive::Load(saving.ToString());
	Scene a = LoadScene(firstDoc, world, fixture.registry);
	Scene b = LoadScene(secondDoc, world, fixture.registry);
	REQUIRE(a.IsValid());
	REQUIRE(b.IsValid());

	// One resource, owned by both - not two copies, which would need two ids
	// and would then not match the ids inside the components referencing it.
	CHECK(fixture.registry.Count() == 1);
	CHECK(a.Owns(id));
	CHECK(b.Owns(id));
	CHECK_FALSE(firstDoc.HasErrors());
	CHECK_FALSE(secondDoc.HasErrors());
}

TEST_CASE("A shared resource survives until the last owner unloads") {
	World world;
	ResourceFixture fixture;

	Scene original = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(original, "shared");
	Archive saving = Archive::Save();
	REQUIRE(SaveScene(saving, original, fixture.registry));
	UnloadScene(original, fixture.registry);

	Archive firstDoc = Archive::Load(saving.ToString());
	Archive secondDoc = Archive::Load(saving.ToString());
	Scene a = LoadScene(firstDoc, world, fixture.registry);
	Scene b = LoadScene(secondDoc, world, fixture.registry);

	UnloadScene(a, fixture.registry);
	// b still owns it, so it stays - otherwise b's components would reference
	// an id that no longer resolves.
	CHECK(fixture.registry.Has(id));
	REQUIRE(fixture.registry.Get<Note>(id) != nullptr);
	CHECK(fixture.registry.Get<Note>(id)->text == "shared");

	UnloadScene(b, fixture.registry);
	CHECK_FALSE(fixture.registry.Has(id));
}

// Resource owner counts work for both scenes and dependent resources.
TEST_CASE("Any second owner keeps a resource alive") {
	World world;
	ResourceFixture fixture;

	Scene host = CreateScene(world, "Host");
	Scene nested = CreateScene(world, "Nested");
	nested.Root().SetParent(host.CreateEntity("Slot"));

	Ref<Note> note = fixture.registry.Create<Note>();
	const ResourceID id = note->Id();
	CHECK(fixture.registry.OwnerCount(id) == 0);

	Scene outer = CreateScene(world, "Outer");
	CHECK(outer.Own(fixture.registry, id));
	CHECK(nested.Own(fixture.registry, id));
	CHECK(fixture.registry.OwnerCount(id) == 2);

	UnloadScene(outer, fixture.registry);
	CHECK(fixture.registry.Has(id));
	CHECK(fixture.registry.OwnerCount(id) == 1);
}

TEST_CASE("A record whose id belongs to a different type is refused") {
	World world;
	ResourceFixture fixture;
	Ref<Note> existing = fixture.registry.Create<Note>();

	Archive loading = Archive::Load(
		"resources:\n"
		"  - id: " + std::to_string(existing->Id().value) + "\n"
		"    type: NotANote\n"
		"    data: {}\n"
		"root:\n"
		"  name: Level\n"
		"  components:\n"
		"    SceneRoot: {sourcePath: \"\"}\n");
	REQUIRE(loading.IsValid());

	Scene loaded = LoadScene(loading, world, fixture.registry);
	REQUIRE(loaded.IsValid());
	// Sharing is only right when it's the same resource; a type mismatch means
	// the id collided rather than matched.
	CHECK_FALSE(loaded.Owns(existing->Id()));
	CHECK(loading.HasErrors());
}

// Load the incoming scene before unloading the outgoing scene to avoid rebuilding
// shared resources between them.
TEST_CASE("Loading before unloading keeps a shared resource alive across a switch") {
	World world;
	ResourceFixture fixture;
	Scene seed = CreateScene(world, "Level");
	const ResourceID id = fixture.Make(seed, "tex");
	Archive doc = Archive::Save();
	REQUIRE(SaveScene(doc, seed, fixture.registry));
	const std::string text = doc.ToString();
	UnloadScene(seed, fixture.registry);

	SUBCASE("load then unload reuses the same object") {
		Archive first = Archive::Load(text);
		Archive second = Archive::Load(text);
		Scene outgoing = LoadScene(first, world, fixture.registry);
		Ref<Note> before = fixture.registry.Get<Note>(id);
		REQUIRE(before != nullptr);

		Scene incoming = LoadScene(second, world, fixture.registry);
		UnloadScene(outgoing, fixture.registry);

		CHECK(fixture.registry.Has(id));
		// Same object, so no decode and no GPU upload happened in between.
		CHECK(fixture.registry.Get<Note>(id) == before);
		UnloadScene(incoming, fixture.registry);
	}

	SUBCASE("unload then load rebuilds it") {
		Archive first = Archive::Load(text);
		Archive second = Archive::Load(text);
		Scene outgoing = LoadScene(first, world, fixture.registry);
		Ref<Note> before = fixture.registry.Get<Note>(id);
		REQUIRE(before != nullptr);

		UnloadScene(outgoing, fixture.registry);
		CHECK_FALSE(fixture.registry.Has(id));

		Scene incoming = LoadScene(second, world, fixture.registry);
		CHECK(fixture.registry.Has(id));
		// A different object: torn down and built again from the file.
		CHECK(fixture.registry.Get<Note>(id) != before);
		UnloadScene(incoming, fixture.registry);
	}
}

// Resources live until their last owner releases them. PackedScene owns dependencies
// between bursts of instances so destroying the last instance does not trigger a reload.
TEST_CASE("A shared resource lives until its last owner releases it") {
	World world;
	ResourceFixture fixture;

	Scene seed = CreateScene(world, "Prefab");
	const ResourceID tex = fixture.Make(seed, "texture");
	Archive doc = Archive::Save();
	REQUIRE(SaveScene(doc, seed, fixture.registry));
	const std::string prefab = doc.ToString();
	UnloadScene(seed, fixture.registry);

	auto spawn = [&]() {
		Archive d = Archive::Load(prefab);
		return LoadScene(d, world, fixture.registry);
	};

	std::vector<Scene> copies;
	for (int i = 0; i < 8; ++i)
		copies.push_back(spawn());

	// One resource, not eight: every load after the first shares.
	CHECK(fixture.registry.Count() == 1);
	Ref<Note> object = fixture.registry.Get<Note>(tex);
	REQUIRE(object != nullptr);

	for (int i = 0; i < 7; ++i)
		UnloadScene(copies[static_cast<size_t>(i)], fixture.registry);
	CHECK(fixture.registry.Has(tex));
	// Never rebuilt while an owner remained.
	CHECK(fixture.registry.Get<Note>(tex) == object);

	UnloadScene(copies[7], fixture.registry);
	CHECK_FALSE(fixture.registry.Has(tex));

	// Loading again rebuilds the released resource.
	Scene again = spawn();
	CHECK(fixture.registry.Get<Note>(tex) != object);
	UnloadScene(again, fixture.registry);
}

TEST_CASE("resource ID collisions remap references inside other resources") {
	World world;
	ResourceRegistry resources;
	resources.RegisterType<Font>("Font").Field("pixelSize", &Font::pixelSize);
	RegisterUITheme(resources);
	auto load = [&](const char *name, int pixelSize) {
		const auto data = std::string("resources:\n") +
			"  - id: 200\n    type: UITheme\n    data:\n      font: 100\n"
			"      rules: [{role: Button, font: 100, texture: 100}]\n"
			"  - id: 100\n    type: Font\n    data: {pixelSize: " +
			std::to_string(pixelSize) + "}\nroot:\n  name: " + name +
			"\n  components:\n    SceneRoot: {}\n"
			"    Tilemap:\n      sheets: [{id: 1, resource: 100}]\n"
			"      tiles: [{id: 1, resource: 100}]\n  children: []\n";
		auto archive = Archive::Load(data);
		auto scene = LoadScene(archive, world, resources);
		REQUIRE(scene.IsValid());
		return scene;
	};
	Scene first = load("First", 32);
	Scene second = load("Second", 64);
	Ref<UITheme> theme;
	for (auto id : second.OwnedResources()) {
		if (auto candidate = resources.Get<UITheme>(id)) {
			theme = candidate;
		}
	}
	REQUIRE(theme);
	auto font = resources.Get<Font>(theme->font);
	REQUIRE(font);
	CHECK(font->pixelSize == 64);
	REQUIRE(theme->rules.size() == 1);
	CHECK(theme->rules[0].appearance.font == font->Id());
	CHECK(theme->rules[0].appearance.texture == font->Id());
	const auto *tiles = second.Root().Get<Tilemap>();
	REQUIRE(tiles);
	REQUIRE(tiles->Sheets().size() == 1);
	REQUIRE(tiles->Tiles().size() == 1);
	CHECK(tiles->Sheets()[0].resource == font->Id());
	CHECK(tiles->Tiles()[0].resource == font->Id());
	CHECK(resources.Get<Font>(ResourceID{100})->pixelSize == 32);
	UnloadScene(second, resources);
	UnloadScene(first, resources);
	CHECK(resources.Count() == 0);
}

TEST_CASE("Resource remapping propagates through forward references and cycles") {
	World world;
	ResourceRegistry resources;
	resources.RegisterType<ResourceLink>("Link")
		.Field("label", &ResourceLink::label)
		.ResourceField("next", &ResourceLink::next, "Link");
	auto load = [&](const char *lastLabel) {
		auto archive = Archive::Load(std::string(R"(
resources:
  - {id: 100, type: Link, data: {label: A, next: 200}}
  - {id: 200, type: Link, data: {label: B, next: 300}}
  - {id: 300, type: Link, data: {label: )") +
			lastLabel + R"(, next: 100}}
root: {name: Graph, components: {SceneRoot: {}}}
)");
		auto scene = LoadScene(archive, world, resources);
		REQUIRE(scene.IsValid());
		REQUIRE_FALSE(archive.HasErrors());
		return scene;
	};
	auto first = load("C");
	auto identical = load("C");
	CHECK(resources.Count() == 3);
	CHECK(identical.OwnedResources() == first.OwnedResources());
	auto changed = load("Changed");
	REQUIRE(changed.OwnedResources().size() == 3);
	CHECK(resources.Count() == 6);
	const auto &ids = changed.OwnedResources();
	CHECK(ids[0] != ResourceID{100});
	CHECK(resources.Get<ResourceLink>(ids[0])->next == ids[1]);
	CHECK(resources.Get<ResourceLink>(ids[1])->next == ids[2]);
	CHECK(resources.Get<ResourceLink>(ids[2])->next == ids[0]);
	CHECK(resources.Get<ResourceLink>(ids[2])->label == "Changed");
	CHECK(resources.Get<ResourceLink>(ResourceID{300})->label == "C");
	UnloadScene(changed, resources);
	CHECK(resources.Count() == 3);
	UnloadScene(identical, resources);
	CHECK(resources.Count() == 3);
	UnloadScene(first, resources);
	CHECK(resources.Count() == 0);
}

TEST_CASE("Reloading owned resource records updates objects in place") {
	ResourceRegistry resources;
	resources.RegisterType<ResourceLink>("Link")
		.Field("label", &ResourceLink::label)
		.ResourceField("next", &ResourceLink::next, "Link");
	auto first = Archive::Load(R"(resources:
  - {id: 100, type: Link, data: {label: Before, next: 200}}
  - {id: 200, type: Link, data: {label: Other, next: 100}}
)");
	ResourceRemap remap;
	const auto owned = LoadResourceRecords(first, resources, remap, false);
	auto original = resources.Get<ResourceLink>(ResourceID{100});
	REQUIRE(original);
	auto updated = Archive::Load(R"(resources:
  - {id: 100, type: Link, data: {label: After, next: 200}}
  - {id: 200, type: Link, data: {label: Other, next: 100}}
)");
	CHECK(LoadResourceRecords(updated, resources, remap, false, owned) == owned);
	CHECK(resources.Count() == 2);
	CHECK(resources.Get<ResourceLink>(ResourceID{100}) == original);
	CHECK(original->label == "After");
	CHECK(original->next == ResourceID{200});
	CHECK(remap.empty());
}

TEST_CASE("Custom animation serializers remap resource references without changing numeric data") {
	auto mapper = +[](u64 id, void *) -> u64 { return id == 100 ? 200 : id; };
	auto spriteData = Archive::Load(R"(animations:
  - {name: walk, texture: 100, columns: 100, frames: [100]}
)")
						  .WithResourceMapper(mapper, nullptr);
	SpriteSheetAnimation sprites;
	sprites.Serialize(spriteData);
	REQUIRE(sprites.Find("walk"));
	CHECK(sprites.Find("walk")->texture == ResourceID{200});
	CHECK(sprites.Find("walk")->columns == 100);
	CHECK(sprites.Find("walk")->frames == std::vector<i32>{100});
	auto animationData = Archive::Load(R"(clips:
  - name: change
    duration: 100
    tracks:
      - type: ResourceRef
        keys: [{time: 100, value: 100}]
      - type: U32
        keys: [{time: 100, value: 100}]
)")
							 .WithResourceMapper(mapper, nullptr);
	AnimationLibrary animations;
	animations.Serialize(animationData);
	REQUIRE_FALSE(animationData.HasErrors());
	REQUIRE(animations.clips.size() == 1);
	const auto &clip = animations.clips[0];
	REQUIRE(clip.tracks.size() == 2);
	CHECK(clip.duration == 100);
	REQUIRE(clip.tracks[0].Keys().size() == 1);
	REQUIRE(clip.tracks[1].Keys().size() == 1);
	CHECK(std::get<ResourceID>(clip.tracks[0].Keys()[0].value) == ResourceID{200});
	CHECK(std::get<u32>(clip.tracks[1].Keys()[0].value) == 100);
	CHECK(clip.tracks[0].Keys()[0].time == 100);
}
