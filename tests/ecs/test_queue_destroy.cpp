#include <vector>

#include <doctest/doctest.h>

#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/world.hpp"

using namespace sowa;

namespace {

struct Marker {
	i32 value = 0;
};

struct Doomed {
	bool unused = false;
};

} // namespace

TEST_CASE("QueueDestroy does not destroy immediately") {
	World world;
	Entity entity = world.CreateEntity();

	entity.QueueDestroy();

	CHECK(entity.IsAlive());
	world.Progress(0.f);
	CHECK_FALSE(entity.IsAlive());
}

TEST_CASE("QueueDestroy ignores stale and null handles while processing valid requests") {
	World world;
	REQUIRE(world.RegisterComponent<Marker>("Marker").Result());
	Entity owner = world.CreateEntity();
	Entity stale = owner;
	owner.Destroy();
	Entity survivor = world.CreateEntity();
	REQUIRE(static_cast<u32>(stale.Raw().id()) == static_cast<u32>(survivor.Raw().id()));
	survivor.Add(Marker{17});
	Entity doomed = world.CreateEntity();

	stale.QueueDestroy();
	world.QueueDestroy(stale);
	Entity().QueueDestroy();
	world.QueueDestroy(Entity());
	doomed.QueueDestroy();
	world.Progress(0.f);

	CHECK_FALSE(doomed.IsAlive());
	REQUIRE(survivor.IsAlive());
	REQUIRE(survivor.Get<Marker>() != nullptr);
	CHECK(survivor.Get<Marker>()->value == 17);
}

TEST_CASE("Queueing the same entity twice still destroys it once") {
	World world;
	Entity entity = world.CreateEntity();

	entity.QueueDestroy();
	entity.QueueDestroy();
	world.QueueDestroy(entity);

	world.Progress(0.f);
	CHECK_FALSE(entity.IsAlive());

	// Recycled id must not be aliased to the queued handle.
	Entity fresh = world.CreateEntity();
	CHECK(fresh.IsAlive());
	CHECK_FALSE(entity.IsAlive());
}

TEST_CASE("QueueDestroy on a parent takes the subtree") {
	World world;
	Entity parent = world.CreateEntity();
	Entity child = parent.CreateChild("Child");
	Entity grandchild = child.CreateChild("Grandchild");

	parent.QueueDestroy();
	CHECK(child.IsAlive());
	CHECK(grandchild.IsAlive());

	world.Progress(0.f);
	CHECK_FALSE(parent.IsAlive());
	CHECK_FALSE(child.IsAlive());
	CHECK_FALSE(grandchild.IsAlive());
}

TEST_CASE("QueueDestroy is safe inside Query::Each") {
	World world;
	REQUIRE(world.RegisterComponent<Marker>("Marker").Result());

	std::vector<Entity> entities;
	for (i32 i = 0; i < 8; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Marker{i});
		entities.push_back(entity);
	}

	Query<Marker> markers = world.CreateQuery<Marker>();
	i32 visited = 0;
	markers.Each([&](Entity entity, Marker &marker) {
		++visited;
		if (marker.value % 2 == 0) {
			entity.QueueDestroy();
		}
	});

	// Queued destruction leaves entities alive until Progress rather than mutating during Each.
	CHECK(visited == 8);
	for (i32 i = 0; i < 8; ++i) {
		CHECK(entities[static_cast<size_t>(i)].IsAlive());
	}
	CHECK(markers.Count() == 8);

	world.Progress(0.f);

	for (i32 i = 0; i < 8; ++i) {
		CHECK(entities[static_cast<size_t>(i)].IsAlive() == (i % 2 != 0));
	}
	CHECK(markers.Count() == 4);
}

TEST_CASE("A queued entity still runs later systems this frame") {
	World world;
	REQUIRE(world.RegisterComponent<Marker>("Marker").Result());
	REQUIRE(world.RegisterComponent<Doomed>("Doomed").Result());

	Entity entity = world.CreateEntity();
	entity.Add(Marker{0});
	entity.Add<Doomed>();

	i32 seenAfterQueue = 0;
	world.RegisterSystem<Marker, Doomed>("Reaper",
		[&](Entity e, Marker &, Doomed &) { e.QueueDestroy(); });
	world.RegisterSystem<Marker>("Witness", [&](Entity, Marker &) { ++seenAfterQueue; });

	world.Progress(0.f);

	// QueueDestroy waits until Progress finishes, unlike Destroy() which
	// flecs flushes at the end of the system that called it.
	CHECK(seenAfterQueue == 1);
	CHECK_FALSE(entity.IsAlive());
}

TEST_CASE("Immediate Destroy is still immediate outside iteration") {
	World world;
	Entity entity = world.CreateEntity();
	entity.Destroy();
	CHECK_FALSE(entity.IsAlive());
}
