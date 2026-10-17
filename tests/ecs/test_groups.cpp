#include <algorithm>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/entity.hpp"
#include "ecs/world.hpp"

using namespace sowa;

namespace {

struct Health {
	i32 current = 0;
};

bool Contains(const std::vector<std::string> &values, const std::string &value) {
	return std::find(values.begin(), values.end(), value) != values.end();
}

std::vector<Entity> MembersOf(World &world, const std::string &group) {
	std::vector<Entity> members;
	world.EachInGroup(group, [&](Entity entity) { members.push_back(entity); });
	return members;
}

} // namespace

TEST_CASE("An entity can join and leave a group") {
	World world;
	Entity entity = world.CreateEntity();

	CHECK_FALSE(entity.IsInGroup("enemies"));

	entity.AddGroup("enemies");
	CHECK(entity.IsInGroup("enemies"));

	entity.RemoveGroup("enemies");
	CHECK_FALSE(entity.IsInGroup("enemies"));
}

TEST_CASE("Joining a group twice is idempotent") {
	World world;
	Entity entity = world.CreateEntity();

	entity.AddGroup("enemies");
	entity.AddGroup("enemies");

	CHECK(entity.IsInGroup("enemies"));
	REQUIRE(entity.GetGroups().size() == 1);
	CHECK(MembersOf(world, "enemies").size() == 1);
}

TEST_CASE("An entity can be in several groups at once") {
	World world;
	Entity entity = world.CreateEntity();

	entity.AddGroup("enemies");
	entity.AddGroup("pausable");
	entity.AddGroup("persists");

	const std::vector<std::string> groups = entity.GetGroups();
	REQUIRE(groups.size() == 3);
	CHECK(Contains(groups, "enemies"));
	CHECK(Contains(groups, "pausable"));
	CHECK(Contains(groups, "persists"));

	entity.RemoveGroup("pausable");
	const std::vector<std::string> remaining = entity.GetGroups();
	REQUIRE(remaining.size() == 2);
	CHECK_FALSE(Contains(remaining, "pausable"));
	CHECK(Contains(remaining, "enemies"));
}

TEST_CASE("EachInGroup finds every group member") {
	World world;

	std::vector<Entity> enemies;
	for (i32 i = 0; i < 5; ++i) {
		Entity entity = world.CreateEntity();
		entity.AddGroup("enemies");
		enemies.push_back(entity);
	}

	Entity friendly = world.CreateEntity();
	friendly.AddGroup("allies");

	const std::vector<Entity> members = MembersOf(world, "enemies");
	CHECK(members.size() == 5);
	for (const Entity &enemy : enemies)
		CHECK(std::find(members.begin(), members.end(), enemy) != members.end());
	CHECK(std::find(members.begin(), members.end(), friendly) == members.end());

	CHECK(MembersOf(world, "allies").size() == 1);
}

TEST_CASE("Leaving a group removes an entity from the query results") {
	World world;

	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();
	a.AddGroup("enemies");
	b.AddGroup("enemies");
	REQUIRE(MembersOf(world, "enemies").size() == 2);

	a.RemoveGroup("enemies");
	const std::vector<Entity> members = MembersOf(world, "enemies");
	REQUIRE(members.size() == 1);
	CHECK(members[0] == b);
}

TEST_CASE("Destroying an entity removes it from its groups") {
	World world;

	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();
	a.AddGroup("enemies");
	b.AddGroup("enemies");

	a.Destroy();

	const std::vector<Entity> members = MembersOf(world, "enemies");
	REQUIRE(members.size() == 1);
	CHECK(members[0] == b);
}

TEST_CASE("Querying an unknown group yields nothing rather than failing") {
	World world;
	Entity entity = world.CreateEntity();
	entity.AddGroup("enemies");

	CHECK(MembersOf(world, "nonexistent").empty());
	CHECK_FALSE(entity.IsInGroup("nonexistent"));
	// Removing from a group that was never created is a no-op.
	entity.RemoveGroup("nonexistent");
	CHECK(entity.IsInGroup("enemies"));
}

TEST_CASE("World::GetGroups lists every group that exists") {
	World world;

	CHECK(world.GetGroups().empty());

	Entity a = world.CreateEntity();
	a.AddGroup("enemies");
	a.AddGroup("pausable");
	Entity b = world.CreateEntity();
	b.AddGroup("allies");

	const std::vector<std::string> groups = world.GetGroups();
	REQUIRE(groups.size() == 3);
	CHECK(Contains(groups, "enemies"));
	CHECK(Contains(groups, "pausable"));
	CHECK(Contains(groups, "allies"));
}

TEST_CASE("Groups do not collide with entity, component or system names") {
	World world;
	REQUIRE(world.RegisterComponent<Health>("Health").Result());
	world.RegisterSystem<Health>("Regen", [](Entity, Health &) {});

	Entity entity = world.CreateEntity();
	entity.SetName("Player");

	// Same strings used as groups - they live in their own scope, so nothing
	// is shadowed in either direction.
	entity.AddGroup("Health");
	entity.AddGroup("Regen");
	entity.AddGroup("Player");

	CHECK(entity.IsInGroup("Health"));
	CHECK(entity.IsInGroup("Regen"));
	CHECK(entity.IsInGroup("Player"));

	// The entity kept its name, and the component is still resolvable.
	CHECK(entity.GetName() == "Player");
	CHECK(world.Find("Player") == entity);
	CHECK(world.FindComponent("Health").IsValid());

	// The group scope itself is not a scene entity.
	CHECK_FALSE(world.Find("Groups").IsAlive());
}

TEST_CASE("Groups are independent of the hierarchy") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();
	REQUIRE(child.SetParent(parent));

	child.AddGroup("enemies");

	// Membership does not propagate up or down the tree.
	CHECK(child.IsInGroup("enemies"));
	CHECK_FALSE(parent.IsInGroup("enemies"));

	const std::vector<Entity> members = MembersOf(world, "enemies");
	REQUIRE(members.size() == 1);
	CHECK(members[0] == child);

	// Reparenting does not disturb membership.
	child.ClearParent();
	CHECK(child.IsInGroup("enemies"));
}

TEST_CASE("Group names get the same separator sanitizing as entity names") {
	World world;
	Entity entity = world.CreateEntity();

	entity.AddGroup("a/b");
	CHECK(entity.IsInGroup("a/b")); // sanitized consistently on both sides
	CHECK(Contains(entity.GetGroups(), "a_b"));
}

TEST_CASE("Group operations are safe on dead and null handles") {
	World world;

	Entity dead = world.CreateEntity();
	dead.AddGroup("enemies");
	dead.Destroy();

	dead.AddGroup("enemies");
	dead.RemoveGroup("enemies");
	CHECK_FALSE(dead.IsInGroup("enemies"));
	CHECK(dead.GetGroups().empty());

	Entity null;
	null.AddGroup("enemies");
	null.RemoveGroup("enemies");
	CHECK_FALSE(null.IsInGroup("enemies"));
	CHECK(null.GetGroups().empty());

	// An empty group name is ignored rather than creating a nameless group.
	Entity entity = world.CreateEntity();
	entity.AddGroup("");
	CHECK(entity.GetGroups().empty());
	CHECK_FALSE(entity.IsInGroup(""));
}

TEST_CASE("Groups compose with component queries") {
	World world;
	REQUIRE(world.RegisterComponent<Health>("Health").Result());

	Entity hurtEnemy = world.CreateEntity();
	hurtEnemy.Add(Health{50});
	hurtEnemy.AddGroup("enemies");

	Entity healthlessEnemy = world.CreateEntity();
	healthlessEnemy.AddGroup("enemies");

	i32 healed = 0;
	world.EachInGroup("enemies", [&](Entity entity) {
		if (Health *health = entity.Get<Health>()) {
			health->current = 100;
			++healed;
		}
	});

	CHECK(healed == 1);
	CHECK(hurtEnemy.Get<Health>()->current == 100);
}
