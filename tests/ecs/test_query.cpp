#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

struct Position {
	Vector2 value;
};

struct Velocity {
	Vector2 value;
};

struct Frozen {
	bool unused = false;
};

} // namespace

TEST_CASE("A query matches entities holding every requested component") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	Entity both = world.CreateEntity();
	both.Add(Position{{1.f, 2.f}});
	both.Add(Velocity{{3.f, 4.f}});

	Entity positionOnly = world.CreateEntity();
	positionOnly.Add(Position{{5.f, 6.f}});

	Entity neither = world.CreateEntity();

	Query<Position, Velocity> moving = world.CreateQuery<Position, Velocity>();
	REQUIRE(moving.IsValid());

	std::vector<Entity> matched;
	moving.Each([&](Entity entity, Position &, Velocity &) { matched.push_back(entity); });

	REQUIRE(matched.size() == 1);
	CHECK(matched[0] == both);
	CHECK(matched[0] != positionOnly);
	CHECK(matched[0] != neither);
}

TEST_CASE("Query callbacks write through to the real components") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	Entity entity = world.CreateEntity();
	entity.Add(Position{{0.f, 0.f}});
	entity.Add(Velocity{{10.f, -5.f}});

	Query<Position, Velocity> moving = world.CreateQuery<Position, Velocity>();
	moving.Each([](Entity, Position &position, Velocity &velocity) {
		position.value.x += velocity.value.x;
		position.value.y += velocity.value.y;
	});

	CHECK(entity.Get<Position>()->value.x == doctest::Approx(10.f));
	CHECK(entity.Get<Position>()->value.y == doctest::Approx(-5.f));
}

TEST_CASE("A query stays live as entities appear and disappear") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	// Built before any matching entity exists - a cached query has to pick
	// up tables created after it, not just those matching at build time.
	Query<Position> positions = world.CreateQuery<Position>();
	CHECK(positions.Count() == 0);
	CHECK_FALSE(positions.Any());
	CHECK_FALSE(positions.First().IsAlive());

	std::vector<Entity> entities;
	for (i32 i = 0; i < 8; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Position{{static_cast<f32>(i), 0.f}});
		entities.push_back(entity);
	}

	CHECK(positions.Count() == 8);
	CHECK(positions.Any());
	CHECK(positions.First().IsAlive());

	// Removing the component un-matches the entity without destroying it.
	entities[0].Remove<Position>();
	CHECK(positions.Count() == 7);

	entities[1].Destroy();
	CHECK(positions.Count() == 6);

	for (Entity &entity : entities)
		entity.Destroy();
	CHECK(positions.Count() == 0);
	CHECK_FALSE(positions.Any());
}

TEST_CASE("A query iterates the same result set repeatedly") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	for (i32 i = 0; i < 4; ++i)
		world.CreateEntity().Add(Position{{static_cast<f32>(i), 0.f}});

	Query<Position> positions = world.CreateQuery<Position>();

	for (i32 pass = 0; pass < 3; ++pass) {
		i32 seen = 0;
		positions.Each([&](Entity, Position &) { ++seen; });
		CHECK(seen == 4);
	}
}

TEST_CASE("Query copies share one underlying cached query") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{1.f, 1.f}});

	Query<Position> original = world.CreateQuery<Position>();
	Query<Position> copy = original;

	CHECK(copy.IsValid());
	CHECK(copy.Count() == original.Count());

	// The copy outliving the original must not leave a dangling query.
	{
		Query<Position> scoped = original;
		CHECK(scoped.Count() == 1);
	}
	CHECK(original.Count() == 1);
	CHECK(copy.Count() == 1);
}

TEST_CASE("Entities matching several components are matched by narrower queries too") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());
	REQUIRE(world.RegisterComponent<Frozen>("Frozen").Result());

	Entity entity = world.CreateEntity();
	entity.Add(Position{{1.f, 1.f}});
	entity.Add(Velocity{{1.f, 1.f}});
	entity.Add<Frozen>();

	CHECK(world.CreateQuery<Position>().Count() == 1);
	CHECK(world.CreateQuery<Position, Velocity>().Count() == 1);
	CHECK((world.CreateQuery<Position, Velocity, Frozen>().Count() == 1));
}

TEST_CASE("An invalid query is inert rather than crashing") {
	Query<Position> query;

	CHECK_FALSE(query.IsValid());
	CHECK(query.Count() == 0);
	CHECK_FALSE(query.Any());
	CHECK_FALSE(query.First().IsAlive());

	bool called = false;
	query.Each([&](Entity, Position &) { called = true; });
	CHECK_FALSE(called);
}

TEST_CASE("First returns a matching entity without walking the whole set") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity only = world.CreateEntity();
	only.Add(Position{{7.f, 8.f}});

	Query<Position> positions = world.CreateQuery<Position>();
	Entity found = positions.First();

	REQUIRE(found.IsAlive());
	CHECK(found == only);
	CHECK(found.Get<Position>()->value.x == doctest::Approx(7.f));
}

TEST_CASE("World::Each still works for one-off iteration") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	Entity entity = world.CreateEntity();
	entity.Add(Position{{0.f, 0.f}});
	entity.Add(Velocity{{2.f, 3.f}});

	i32 seen = 0;
	world.Each<Position, Velocity>([&](Entity, Position &position, Velocity &velocity) {
		position.value.x += velocity.value.x;
		++seen;
	});

	CHECK(seen == 1);
	CHECK(entity.Get<Position>()->value.x == doctest::Approx(2.f));
}

TEST_CASE("Collecting during iteration and mutating afterwards is safe") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	for (i32 i = 0; i < 16; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Position{{static_cast<f32>(i), 0.f}});
	}

	Query<Position> positions = world.CreateQuery<Position>();

	// The documented pattern: gather inside the loop, mutate outside it.
	std::vector<Entity> doomed;
	positions.Each([&](Entity entity, Position &position) {
		if (static_cast<i32>(position.value.x) % 2 == 0)
			doomed.push_back(entity);
	});

	REQUIRE(doomed.size() == 8);
	for (Entity &entity : doomed)
		entity.Destroy();

	CHECK(positions.Count() == 8);
	positions.Each([](Entity entity, Position &) { CHECK(entity.IsAlive()); });
}

// Runtime components use untyped query terms because they have no C++ struct.
// Raw flecs handles exercise the internal ECS path here; application code uses World.
TEST_CASE("A dynamic query over runtime-defined components is buildable today") {
	World world;

	// No C++ struct anywhere - exactly the shape of a script-defined component.
	EcsResult<ComponentType> health = world.RegisterComponent("Health", {{"current", FieldType::I32}});
	EcsResult<ComponentType> named = world.RegisterComponent("Named", {{"name", FieldType::String}});
	REQUIRE(health);
	REQUIRE(named);

	for (i32 i = 0; i < 5; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(health.value);
		REQUIRE(entity.GetField(health.value, "current").Set(i * 10));
		if (i % 2 == 0) {
			entity.Add(named.value);
			REQUIRE(entity.GetField(named.value, "name").Set("even-" + std::to_string(i)));
		}
	}

	// A script would name its components as strings, not hold handles.
	const ComponentType healthType = world.FindComponent("Health");
	const ComponentType namedType = world.FindComponent("Named");
	REQUIRE(healthType.IsValid());
	REQUIRE(namedType.IsValid());

	flecs::world raw = healthType.Raw().world();
	flecs::query<> query = raw.query_builder()
							   .with(healthType.Raw().id())
							   .with(namedType.Raw().id())
							   .cached()
							   .build();

	i32 matched = 0;
	i32 total = 0;
	query.each([&](flecs::entity handle) {
		Entity entity(handle);
		++matched;

		// Read and write fields by name through reflection.
		i32 current = 0;
		std::string name;
		REQUIRE(entity.GetField(healthType, "current").Get(current));
		REQUIRE(entity.GetField(namedType, "name").Get(name));
		total += current;
		CHECK(name.rfind("even-", 0) == 0);
		CHECK(entity.GetField(healthType, "current").Set(current + 1));
	});

	CHECK(matched == 3);		 // i = 0, 2, 4 carry both components
	CHECK(total == 0 + 20 + 40); // values read back correctly

	// Write-backs landed.
	i32 first = 0;
	REQUIRE(query.first().is_alive());
	REQUIRE(Entity(query.first()).GetField(healthType, "current").Get(first));
	CHECK(first % 10 == 1);
}

TEST_CASE("A disabled entity is skipped by queries") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity live = world.CreateEntity();
	live.Add(Position{{1.f, 2.f}});
	Entity dormant = world.CreateEntity();
	dormant.Add(Position{{3.f, 4.f}});

	Query<Position> positions = world.CreateQuery<Position>();
	CHECK(positions.Count() == 2);

	dormant.SetEnabled(false);
	CHECK_FALSE(dormant.IsEnabled());
	CHECK(live.IsEnabled());
	CHECK(positions.Count() == 1);
	CHECK(positions.First() == live);

	dormant.SetEnabled(true);
	CHECK(dormant.IsEnabled());
	CHECK(positions.Count() == 2);
}
