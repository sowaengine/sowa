#include <doctest/doctest.h>

#include "ecs/entity_pick_map.hpp"
#include "ecs/world.hpp"

using namespace sowa;

TEST_CASE("EntityPickMap reserves zero and reuses an entity's frame token") {
	World world;
	Entity first = world.CreateEntity();
	Entity second = world.CreateEntity();
	EntityPickMap picks;
	picks.BeginFrame();

	CHECK(picks.TokenFor(Entity()) == 0);
	const u32 firstToken = picks.TokenFor(first);
	CHECK(firstToken == 1);
	CHECK(picks.TokenFor(first) == firstToken);
	const u32 secondToken = picks.TokenFor(second);
	CHECK(secondToken == 2);
	CHECK(picks.Resolve(firstToken) == first);
	CHECK(picks.Resolve(secondToken) == second);
	CHECK_FALSE(picks.Resolve(0).IsAlive());
	CHECK_FALSE(picks.Resolve(99).IsAlive());
}

TEST_CASE("EntityPickMap never resolves a destroyed entity") {
	World world;
	Entity entity = world.CreateEntity();
	EntityPickMap picks;
	picks.BeginFrame();
	const u32 token = picks.TokenFor(entity);
	entity.Destroy();

	CHECK_FALSE(picks.Resolve(token).IsAlive());
}

TEST_CASE("EntityPickMap tokens are rebuilt each render frame") {
	World world;
	Entity first = world.CreateEntity();
	Entity second = world.CreateEntity();
	EntityPickMap picks;
	picks.BeginFrame();
	CHECK(picks.TokenFor(first) == 1);
	CHECK(picks.TokenFor(second) == 2);

	picks.BeginFrame();
	CHECK(picks.TokenFor(second) == 1);
	CHECK(picks.Resolve(1) == second);
}
