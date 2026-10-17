#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

struct Health {
	i32 current = 0;
};

} // namespace

TEST_CASE("Entities can be named and report their name") {
	World world;
	Entity entity = world.CreateEntity();

	CHECK(entity.GetName().empty());
	CHECK(entity.SetName("Player") == "Player");
	CHECK(entity.GetName() == "Player");
}

TEST_CASE("Names are unique per parent scope, Godot-style") {
	World world;
	Entity parent = world.CreateEntity();
	parent.SetName("Parent");

	Entity first = world.CreateEntity();
	Entity second = world.CreateEntity();
	Entity third = world.CreateEntity();
	REQUIRE(first.SetParent(parent));
	REQUIRE(second.SetParent(parent));
	REQUIRE(third.SetParent(parent));

	CHECK(first.SetName("Sprite") == "Sprite");
	CHECK(second.SetName("Sprite") == "Sprite2");
	CHECK(third.SetName("Sprite") == "Sprite3");

	CHECK(parent.Find("Sprite") == first);
	CHECK(parent.Find("Sprite2") == second);
	CHECK(parent.Find("Sprite3") == third);
}

TEST_CASE("The same name is free again under a different parent") {
	World world;

	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();
	a.SetName("A");
	b.SetName("B");

	Entity childOfA = world.CreateEntity();
	Entity childOfB = world.CreateEntity();
	REQUIRE(childOfA.SetParent(a));
	REQUIRE(childOfB.SetParent(b));

	// Scoped like a filesystem: same leaf name, different directories.
	CHECK(childOfA.SetName("Sprite") == "Sprite");
	CHECK(childOfB.SetName("Sprite") == "Sprite");

	CHECK(a.Find("Sprite") == childOfA);
	CHECK(b.Find("Sprite") == childOfB);
}

TEST_CASE("Renaming an entity to its current name does not bump the counter") {
	World world;
	Entity entity = world.CreateEntity();

	CHECK(entity.SetName("Player") == "Player");
	CHECK(entity.SetName("Player") == "Player");
	CHECK(entity.SetName("Player") == "Player");
	CHECK(entity.GetName() == "Player");
}

TEST_CASE("An empty name clears the name") {
	World world;
	Entity entity = world.CreateEntity();

	entity.SetName("Temporary");
	REQUIRE(entity.GetName() == "Temporary");

	CHECK(entity.SetName("").empty());
	CHECK(entity.GetName().empty());
}

TEST_CASE("Path separators in a name are replaced") {
	World world;
	Entity entity = world.CreateEntity();

	// Left alone, these would make the entity unreachable by the path it
	// reports for itself.
	CHECK(entity.SetName("a/b") == "a_b");
	CHECK(entity.SetName("x::y") == "x__y");
}

TEST_CASE("GetPath reports a slash-separated path from the root") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity grandchild = world.CreateEntity();

	root.SetName("Player");
	REQUIRE(child.SetParent(root));
	child.SetName("Body");
	REQUIRE(grandchild.SetParent(child));
	grandchild.SetName("Sprite");

	CHECK(root.GetPath() == "Player");
	CHECK(child.GetPath() == "Player/Body");
	CHECK(grandchild.GetPath() == "Player/Body/Sprite");
}

TEST_CASE("Entity::Find resolves paths relative to itself") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity grandchild = world.CreateEntity();

	root.SetName("Player");
	REQUIRE(child.SetParent(root));
	child.SetName("Body");
	REQUIRE(grandchild.SetParent(child));
	grandchild.SetName("Sprite");

	CHECK(root.Find("Body") == child);
	CHECK(root.Find("Body/Sprite") == grandchild);
	CHECK(child.Find("Sprite") == grandchild);

	CHECK_FALSE(root.Find("Missing").IsAlive());
	CHECK_FALSE(root.Find("Body/Missing").IsAlive());
	CHECK_FALSE(root.Find("").IsAlive());
}

TEST_CASE("A relative lookup does not escape upward into parent scopes") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity sibling = world.CreateEntity();

	root.SetName("Player");
	REQUIRE(child.SetParent(root));
	child.SetName("Body");
	sibling.SetName("Enemy"); // a root-level entity, not under Player

	// Searching from child must not find something above it.
	CHECK_FALSE(child.Find("Enemy").IsAlive());
	CHECK_FALSE(child.Find("Player").IsAlive());
	// But the world-level lookup does.
	CHECK(world.Find("Enemy") == sibling);
}

TEST_CASE("World::Find resolves absolute paths from the root") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();

	root.SetName("Player");
	REQUIRE(child.SetParent(root));
	child.SetName("Body");

	CHECK(world.Find("Player") == root);
	CHECK(world.Find("Player/Body") == child);
	CHECK_FALSE(world.Find("Nope").IsAlive());
	CHECK_FALSE(world.Find("Player/Nope").IsAlive());
	CHECK_FALSE(world.Find("").IsAlive());
}

TEST_CASE("World::Find never returns a component or a system") {
	World world;
	REQUIRE(world.RegisterComponent<Health>("Health").Result());
	REQUIRE(world.RegisterComponent("Mana", {{"current", FieldType::I32}}));
	world.RegisterSystem<Health>("Regen", [](Entity, Health &) {});

	// All three are named entities at root scope, but none of them is a
	// scene entity, so a path lookup must not hand them back.
	CHECK_FALSE(world.Find("Health").IsAlive());
	CHECK_FALSE(world.Find("Mana").IsAlive());
	CHECK_FALSE(world.Find("Regen").IsAlive());

	// Components remain reachable through their own namespace.
	CHECK(world.FindComponent("Health").IsValid());
	CHECK(world.FindComponent("Mana").IsValid());

	// And a real entity of the same name is found normally.
	Entity pickup = world.CreateEntity();
	CHECK(pickup.SetName("Health") == "Health2"); // uniquified against the component
	CHECK(world.Find("Health2") == pickup);
}

TEST_CASE("Reparenting into a taken name re-uniquifies") {
	World world;

	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();
	a.SetName("A");
	b.SetName("B");

	Entity moved = world.CreateEntity();
	REQUIRE(moved.SetParent(a));
	CHECK(moved.SetName("Sprite") == "Sprite");

	Entity resident = world.CreateEntity();
	REQUIRE(resident.SetParent(b));
	CHECK(resident.SetName("Sprite") == "Sprite");

	// Moving into a scope where "Sprite" is taken must rename, or one of the
	// two silently becomes unreachable by path.
	REQUIRE(moved.SetParent(b));
	CHECK(moved.GetName() == "Sprite2");
	CHECK(moved.GetPath() == "B/Sprite2");

	CHECK(b.Find("Sprite") == resident);
	CHECK(b.Find("Sprite2") == moved);
	CHECK(resident.GetName() == "Sprite");
}

TEST_CASE("Clearing a parent re-uniquifies against the root scope") {
	World world;
	REQUIRE(world.RegisterComponent<Health>("Health").Result());

	Entity parent = world.CreateEntity();
	parent.SetName("Parent");

	Entity child = world.CreateEntity();
	REQUIRE(child.SetParent(parent));
	// Free under Parent, but taken at root by the Health component.
	CHECK(child.SetName("Health") == "Health");

	child.ClearParent();
	CHECK(child.GetName() == "Health2");
	CHECK(world.Find("Health2") == child);
}

TEST_CASE("Naming survives a reparent with no collision") {
	World world;

	Entity first = world.CreateEntity();
	Entity second = world.CreateEntity();
	first.SetName("First");
	second.SetName("Second");

	Entity child = world.CreateEntity();
	REQUIRE(child.SetParent(first));
	CHECK(child.SetName("Sprite") == "Sprite");

	// Moving to a parent with no clash keeps the name.
	REQUIRE(child.SetParent(second));
	CHECK(child.GetName() == "Sprite");
	CHECK(second.Find("Sprite") == child);
	CHECK_FALSE(first.Find("Sprite").IsAlive());
	CHECK(child.GetPath() == "Second/Sprite");
}

TEST_CASE("Naming is safe on dead and null handles") {
	World world;

	Entity dead = world.CreateEntity();
	dead.SetName("Doomed");
	dead.Destroy();

	CHECK(dead.SetName("Renamed").empty());
	CHECK(dead.GetName().empty());
	CHECK(dead.GetPath().empty());
	CHECK_FALSE(dead.Find("Anything").IsAlive());

	Entity null;
	CHECK(null.SetName("Nope").empty());
	CHECK(null.GetName().empty());
	CHECK(null.GetPath().empty());
	CHECK_FALSE(null.Find("Anything").IsAlive());
}

TEST_CASE("Destroying a named entity frees its name for reuse") {
	World world;
	Entity parent = world.CreateEntity();
	parent.SetName("Parent");

	Entity first = world.CreateEntity();
	REQUIRE(first.SetParent(parent));
	CHECK(first.SetName("Slot") == "Slot");

	first.Destroy();

	Entity replacement = world.CreateEntity();
	REQUIRE(replacement.SetParent(parent));
	CHECK(replacement.SetName("Slot") == "Slot");
	CHECK(parent.Find("Slot") == replacement);
}
