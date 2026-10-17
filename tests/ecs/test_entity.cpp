#include <unordered_set>

#include <doctest/doctest.h>

#include "ecs/entity.hpp"
#include "ecs/world.hpp"

using namespace sowa;

TEST_CASE("World::CreateEntity returns a live, unique entity") {
	World world;
	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();

	CHECK(a.IsAlive());
	CHECK(b.IsAlive());
	CHECK(a != b);
}

TEST_CASE("Entity::Destroy makes the entity not alive") {
	World world;
	Entity e = world.CreateEntity();
	CHECK(e.IsAlive());

	e.Destroy();
	CHECK_FALSE(e.IsAlive());

	e.Destroy(); // no-op, must not crash
	CHECK_FALSE(e.IsAlive());
}

TEST_CASE("Stale entity handles are detected via generation, not aliased to a new entity") {
	World world;
	Entity owner = world.CreateEntity();
	Entity stale = owner;
	owner.Destroy();

	Entity fresh = world.CreateEntity();
	REQUIRE(static_cast<u32>(stale.Raw().id()) == static_cast<u32>(fresh.Raw().id()));
	CHECK(stale.Raw().id() != fresh.Raw().id());
	CHECK_FALSE(stale.IsAlive());
	CHECK(fresh.IsAlive());
	stale.Destroy();
	CHECK(fresh.IsAlive());
}

TEST_CASE("Entity is usable as a hash-set/hash-map key") {
	World world;
	Entity a = world.CreateEntity();
	Entity b = world.CreateEntity();

	std::unordered_set<Entity> set;
	set.insert(a);
	set.insert(b);
	set.insert(a);

	CHECK(set.size() == 2);
	CHECK(set.count(a) == 1);
}
