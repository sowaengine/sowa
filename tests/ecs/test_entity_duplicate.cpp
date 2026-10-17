#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/log.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "resource/resource_id.hpp"

using namespace sowa;

namespace {

// A string field is the interesting case: flecs has to use the registered C++
// copy hook, not a bit blit, or both entities end up sharing one buffer.
struct Label {
	std::string text;
	i32 count = 0;
};

// The reference case Duplicate exists to get right.
struct Link {
	Entity target;
};

struct Sprite {
	ResourceID texture;
};

void registerAll(World &world) {
	world.RegisterComponent<Label>("Label")
		.Field("text", &Label::text)
		.Field("count", &Label::count);
	world.RegisterComponent<Link>("Link").Field("target", &Link::target);
	world.RegisterComponent<Sprite>("Sprite").Field("texture", &Sprite::texture);
}

} // namespace

TEST_CASE("Duplicate copies component values") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.SetName("Original");
	Transform2D transform;
	transform.position = Vector2(10.f, 20.f);
	transform.rotation = 45.f;
	source.Add(transform);
	source.Add(Label{"hello", 7});

	Entity copy = source.Duplicate();

	REQUIRE(copy.IsAlive());
	CHECK(copy != source);
	CHECK(copy.Get<Transform2D>()->position.x == doctest::Approx(10.f));
	CHECK(copy.Get<Transform2D>()->rotation == doctest::Approx(45.f));
	CHECK(copy.Get<Label>()->text == "hello");
	CHECK(copy.Get<Label>()->count == 7);
}

TEST_CASE("A copied string field is its own buffer") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	// Long enough to be heap-allocated rather than living in the small-string
	// buffer, which is where a bit blit would go wrong.
	source.Add(Label{"a string too long for any small-string optimization", 1});

	Entity copy = source.Duplicate();
	source.Get<Label>()->text = "changed";

	CHECK(copy.Get<Label>()->text == "a string too long for any small-string optimization");
}

TEST_CASE("Duplicate copies the whole subtree, in order") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.SetName("Rig");
	Entity first = source.CreateChild("First");
	Entity second = source.CreateChild("Second");
	Entity nested = first.CreateChild("Nested");
	nested.Add(Label{"deep", 0});

	Entity copy = source.Duplicate();

	REQUIRE(copy.IsAlive());
	const std::vector<Entity> children = copy.GetChildren();
	REQUIRE(children.size() == 2);
	// Order is preserved because copies are appended in the source's order.
	CHECK(children[0].GetName() == "First");
	CHECK(children[1].GetName() == "Second");
	CHECK(children[0] != first);
	CHECK(children[1] != second);

	Entity copiedNested = copy.Find("First/Nested");
	REQUIRE(copiedNested.IsAlive());
	CHECK(copiedNested != nested);
	CHECK(copiedNested.Get<Label>()->text == "deep");

	// The original is untouched.
	CHECK(source.GetChildren().size() == 2);
	CHECK(nested.IsAlive());
}

TEST_CASE("A reference into the copied subtree is rewritten to the copy") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.SetName("Rig");
	Entity arm = source.CreateChild("Arm");
	source.Add(Link{arm});

	Entity copy = source.Duplicate();

	Entity copiedArm = copy.Find("Arm");
	REQUIRE(copiedArm.IsAlive());
	CHECK(copy.Get<Link>()->target == copiedArm);
	CHECK(copy.Get<Link>()->target != arm);
	// And the original still names its own.
	CHECK(source.Get<Link>()->target == arm);
}

TEST_CASE("A reference pointing outside the subtree is left alone") {
	World world;
	registerAll(world);

	Entity player = world.CreateEntity();
	player.SetName("Player");

	Entity bullet = world.CreateEntity();
	bullet.SetName("Bullet");
	bullet.Add(Link{player});

	Entity copy = bullet.Duplicate();

	// Nothing duplicated the player, so there is no copy of it to point at.
	CHECK(copy.Get<Link>()->target == player);
}

TEST_CASE("A reference is remapped even when it points at an entity copied later") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.SetName("Rig");
	Entity head = source.CreateChild("Head");
	Entity tail = source.CreateChild("Tail");
	// Forward reference: Head is cloned before Tail exists as a copy, so this
	// only resolves because the remap is a second pass.
	head.Add(Link{tail});

	Entity copy = source.Duplicate();

	Entity copiedTail = copy.Find("Tail");
	REQUIRE(copiedTail.IsAlive());
	CHECK(copy.Find("Head").Get<Link>()->target == copiedTail);
}

TEST_CASE("Sibling references between two copied children both remap") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	Entity left = source.CreateChild("Left");
	Entity right = source.CreateChild("Right");
	left.Add(Link{right});
	right.Add(Link{left});

	Entity copy = source.Duplicate();

	Entity copiedLeft = copy.Find("Left");
	Entity copiedRight = copy.Find("Right");
	CHECK(copiedLeft.Get<Link>()->target == copiedRight);
	CHECK(copiedRight.Get<Link>()->target == copiedLeft);
}

TEST_CASE("A ResourceID is copied verbatim, so both entities name one resource") {
	World world;
	registerAll(world);

	const ResourceID id{12345};
	Entity source = world.CreateEntity();
	source.Add(Sprite{id});

	Entity copy = source.Duplicate();

	// Copying an ID leaves resource ownership unchanged.
	CHECK(copy.Get<Sprite>()->texture == id);
}

TEST_CASE("Groups and visibility come across") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.AddGroup("hostile");
	source.AddGroup("spawned");
	source.SetVisible(false);

	Entity copy = source.Duplicate();

	CHECK(copy.IsInGroup("hostile"));
	CHECK(copy.IsInGroup("spawned"));
	CHECK_FALSE(copy.IsVisible());
	// Both copies belong to the same world-wide group.
	std::vector<Entity> hostile;
	world.EachInGroup("hostile", [&](Entity entity) { hostile.push_back(entity); });
	CHECK(hostile.size() == 2);
}

TEST_CASE("A null parent makes the copy a root entity") {
	World world;
	registerAll(world);

	Entity parent = world.CreateEntity();
	parent.SetName("Parent");
	Entity source = parent.CreateChild("Child");

	Entity copy = source.Duplicate();

	// flecs' clone carries the source's ChildOf across, so this only holds
	// because Duplicate detaches it.
	CHECK_FALSE(copy.HasParent());
	CHECK(copy.GetPath() == "Child");
	CHECK(parent.GetChildren().size() == 1);
}

TEST_CASE("Duplicating as a sibling uniquifies the name") {
	World world;
	registerAll(world);

	Entity parent = world.CreateEntity();
	parent.SetName("Parent");
	Entity source = parent.CreateChild("Sprite");

	Entity copy = source.Duplicate(source.GetParent());

	REQUIRE(copy.IsAlive());
	CHECK(copy.GetParent() == parent);
	// Parent before naming so uniqueness is checked in the destination scope.
	CHECK(copy.GetName() == "Sprite2");
	CHECK(world.Find("Parent/Sprite") == source);
	CHECK(world.Find("Parent/Sprite2") == copy);
}

TEST_CASE("Duplicate refuses to copy into its own subtree") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.SetName("Rig");
	Entity child = source.CreateChild("Arm");

	// Both of these would recurse without end: each level reads its children
	// fresh, and the copies would keep landing under one of them.
	CHECK_FALSE(source.Duplicate(source).IsAlive());
	CHECK_FALSE(source.Duplicate(child).IsAlive());
	CHECK(source.GetChildren().size() == 1);
}

TEST_CASE("Duplicating a dead entity gives a null handle") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.Destroy();

	CHECK_FALSE(source.Duplicate().IsAlive());
	CHECK_FALSE(Entity().Duplicate().IsAlive());
}

TEST_CASE("Duplicate refuses to run while flecs is deferring") {
	World world;
	registerAll(world);

	Entity source = world.CreateEntity();
	source.SetName("Rig");
	source.Add(Link{source.CreateChild("Arm")});
	source.Add(Label{"tick", 0});

	Entity result = source; // overwritten below; anything but null is a failure
	world.RegisterSystem<Label>("Duplicator", [&](Entity entity, Label &) {
		result = entity.Duplicate();
	});

	Log::Clear();
	world.Progress(0.f);

	// A clone made mid-iteration is queued, so its components don't exist yet
	// and the reference remap would silently rewrite nothing - handing back a
	// copy still pointing into the original. Refused loudly instead.
	CHECK_FALSE(result.IsAlive());
	REQUIRE_FALSE(Log::GetLines().empty());
	CHECK(Log::GetLines().back().severity == Log::Severity::Error);
	Log::Clear();
}
