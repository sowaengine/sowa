#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "ecs/system.hpp"
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

struct Doomed {
	bool unused = false;
};

} // namespace

TEST_CASE("A registered system runs on Progress over matching entities only") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	Entity moving = world.CreateEntity();
	moving.Add(Position{{0.f, 0.f}});
	moving.Add(Velocity{{10.f, 20.f}});

	Entity still = world.CreateEntity();
	still.Add(Position{{5.f, 5.f}});

	i32 ran = 0;
	System movement = world.RegisterSystem<Position, Velocity>(
		"Movement", [&](Entity, Position &position, Velocity &velocity) {
			++ran;
			position.value.x += velocity.value.x;
			position.value.y += velocity.value.y;
		});
	REQUIRE(movement.IsValid());

	world.Progress(1.f);

	CHECK(ran == 1);
	CHECK(moving.Get<Position>()->value.x == doctest::Approx(10.f));
	CHECK(still.Get<Position>()->value.x == doctest::Approx(5.f));

	world.Progress(1.f);
	CHECK(ran == 2);
	CHECK(moving.Get<Position>()->value.x == doctest::Approx(20.f));
}

TEST_CASE("Systems run in registration order") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	std::vector<std::string> order;
	world.RegisterSystem<Position>("First", [&](Entity, Position &) { order.push_back("First"); });
	world.RegisterSystem<Position>("Second", [&](Entity, Position &) { order.push_back("Second"); });
	world.RegisterSystem<Position>("Third", [&](Entity, Position &) { order.push_back("Third"); });

	world.Progress(1.f);

	REQUIRE(order.size() == 3);
	CHECK(order[0] == "First");
	CHECK(order[1] == "Second");
	CHECK(order[2] == "Third");
}

TEST_CASE("A disabled system is skipped and resumes when re-enabled") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	i32 ran = 0;
	System system = world.RegisterSystem<Position>("Counter", [&](Entity, Position &) { ++ran; });
	CHECK(system.IsEnabled());

	world.Progress(1.f);
	CHECK(ran == 1);

	// Disabling gameplay preserves the world.
	system.SetEnabled(false);
	CHECK_FALSE(system.IsEnabled());
	world.Progress(1.f);
	world.Progress(1.f);
	CHECK(ran == 1);

	system.SetEnabled(true);
	CHECK(system.IsEnabled());
	world.Progress(1.f);
	CHECK(ran == 2);
}

TEST_CASE("SetPlayOnlySystemsEnabled skips play-only systems and resumes them") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	i32 ran = 0;
	world.RegisterSystem<Position>("Counter", [&](Entity, Position &) { ++ran; });
	CHECK(world.PlayOnlySystemsEnabled());

	world.Progress(1.f);
	CHECK(ran == 1);

	world.SetPlayOnlySystemsEnabled(false);
	CHECK_FALSE(world.PlayOnlySystemsEnabled());
	world.Progress(1.f);
	world.Progress(1.f);
	CHECK(ran == 1);

	world.SetPlayOnlySystemsEnabled(true);
	world.Progress(1.f);
	CHECK(ran == 2);
}

TEST_CASE("CreateSystem systems are play-only unless Always") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());
	REQUIRE(world.RegisterComponent<Doomed>("Doomed").Result());
	Entity entity = world.CreateEntity();
	entity.Add(Position{{0.f, 0.f}});
	entity.Add(Velocity{{1.f, 0.f}});

	i32 ran = 0;
	world.CreateSystem<Position, Velocity>("Move")
		.Without<Doomed>()
		.Each([&](Entity, Position &, Velocity &) { ++ran; });

	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(1.f);
	CHECK(ran == 0);

	world.SetPlayOnlySystemsEnabled(true);
	world.Progress(1.f);
	CHECK(ran == 1);
}

TEST_CASE("Always systems keep running while play-only systems are off") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	i32 playRan = 0;
	i32 alwaysRan = 0;
	world.RegisterSystem<Position>("Spawner", [&](Entity, Position &) { ++playRan; });
	world.CreateSystem<Position>("Particles")
		.Always()
		.Each([&](Entity, Position &) { ++alwaysRan; });

	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(1.f);
	CHECK(playRan == 0);
	CHECK(alwaysRan == 1);

	world.SetPlayOnlySystemsEnabled(true);
	world.Progress(1.f);
	CHECK(playRan == 1);
	CHECK(alwaysRan == 2);
}

TEST_CASE("A play-only system registered while play-only systems are off starts disabled") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	world.SetPlayOnlySystemsEnabled(false);
	i32 ran = 0;
	world.RegisterSystem<Position>("Late", [&](Entity, Position &) { ++ran; });
	CHECK_FALSE(world.PlayOnlySystemsEnabled());
	world.Progress(1.f);
	CHECK(ran == 0);
}

TEST_CASE("A destroyed system stops running") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	i32 ran = 0;
	System system = world.RegisterSystem<Position>("Counter", [&](Entity, Position &) { ++ran; });

	world.Progress(1.f);
	CHECK(ran == 1);

	system.Destroy();
	CHECK_FALSE(system.IsValid());

	world.Progress(1.f);
	CHECK(ran == 1);

	// Destroying twice is a no-op, not a crash.
	system.Destroy();
	CHECK_FALSE(system.IsValid());
}

TEST_CASE("World::DeltaTime reports the frame being processed") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	world.CreateEntity().Add(Position{{0.f, 0.f}});

	f32 seen = 0.f;
	world.RegisterSystem<Position>("ReadDelta", [&](Entity, Position &) { seen = world.DeltaTime(); });

	world.Progress(0.25f);
	CHECK(seen == doctest::Approx(0.25f));

	world.Progress(0.5f);
	CHECK(seen == doctest::Approx(0.5f));
}

TEST_CASE("Systems can integrate over delta time") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	Entity entity = world.CreateEntity();
	entity.Add(Position{{0.f, 0.f}});
	entity.Add(Velocity{{100.f, 0.f}});

	world.RegisterSystem<Position, Velocity>(
		"Movement", [&](Entity, Position &position, Velocity &velocity) {
			position.value.x += velocity.value.x * world.DeltaTime();
		});

	world.Progress(0.5f);
	CHECK(entity.Get<Position>()->value.x == doctest::Approx(50.f));

	world.Progress(0.25f);
	CHECK(entity.Get<Position>()->value.x == doctest::Approx(75.f));
}

TEST_CASE("Destroying entities inside a system is deferred, not fatal") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Doomed>("Doomed").Result());

	std::vector<Entity> entities;
	for (i32 i = 0; i < 16; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Position{{static_cast<f32>(i), 0.f}});
		if (i % 2 == 0)
			entity.Add<Doomed>();
		entities.push_back(entity);
	}

	// The same thing aborts when done while iterating a Query by hand; inside
	// a system flecs defers it to the end of the run.
	i32 visited = 0;
	world.RegisterSystem<Position, Doomed>("Reaper", [&](Entity entity, Position &, Doomed &) {
		++visited;
		entity.Destroy();
	});

	world.Progress(1.f);

	CHECK(visited == 8);
	for (i32 i = 0; i < 16; ++i)
		CHECK(entities[i].IsAlive() == (i % 2 != 0));

	// Nothing left to reap on the next frame.
	visited = 0;
	world.Progress(1.f);
	CHECK(visited == 0);
}

TEST_CASE("Adding components inside a system is deferred, not fatal") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());

	Entity entity = world.CreateEntity();
	entity.Add(Position{{0.f, 0.f}});

	world.RegisterSystem<Position>("Upgrade", [&](Entity e, Position &) {
		if (!e.Has<Velocity>())
			e.Add(Velocity{{1.f, 1.f}});
	});

	world.Progress(1.f);
	CHECK(entity.Has<Velocity>());
	CHECK(entity.Get<Velocity>()->value.x == doctest::Approx(1.f));
}

TEST_CASE("An invalid system handle is inert") {
	System system;

	CHECK_FALSE(system.IsValid());
	CHECK_FALSE(system.IsEnabled());

	// Neither of these should do anything or crash.
	system.SetEnabled(true);
	system.SetEnabled(false);
	system.Destroy();

	CHECK_FALSE(system.IsValid());
	CHECK(system == System());
}

TEST_CASE("Without excludes entities holding the filtered component") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Velocity>("Velocity").Result());
	REQUIRE(world.RegisterComponent<Doomed>("Doomed").Result());

	Entity plain = world.CreateEntity();
	plain.Add(Position{{0.f, 0.f}});
	plain.Add(Velocity{{1.f, 0.f}});

	Entity excluded = world.CreateEntity();
	excluded.Add(Position{{0.f, 0.f}});
	excluded.Add(Velocity{{1.f, 0.f}});
	excluded.Add<Doomed>();

	std::vector<Entity> visited;
	world.CreateSystem<Position, Velocity>("Movement")
		.Without<Doomed>()
		.Each([&](Entity entity, Position &position, Velocity &velocity) {
			visited.push_back(entity);
			position.value.x += velocity.value.x;
		});

	world.Progress(1.f);

	REQUIRE(visited.size() == 1);
	CHECK(visited[0] == plain);
	CHECK(plain.Get<Position>()->value.x == doctest::Approx(1.f));
	// Never visited at all, so untouched.
	CHECK(excluded.Get<Position>()->value.x == doctest::Approx(0.f));
}

TEST_CASE("Exclusion follows the component as it moves between entities") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Doomed>("Doomed").Result());

	Entity a = world.CreateEntity();
	a.Add(Position{{0.f, 0.f}});
	Entity b = world.CreateEntity();
	b.Add(Position{{0.f, 0.f}});

	a.Add<Doomed>();

	world.CreateSystem<Position>("Tick").Without<Doomed>().Each(
		[](Entity, Position &position) { position.value.x += 1.f; });

	world.Progress(1.f);
	CHECK(a.Get<Position>()->value.x == doctest::Approx(0.f));
	CHECK(b.Get<Position>()->value.x == doctest::Approx(1.f));

	// Hand the marker over - the demo's Space key does exactly this.
	a.Remove<Doomed>();
	b.Add<Doomed>();

	world.Progress(1.f);
	CHECK(a.Get<Position>()->value.x == doctest::Approx(1.f));
	CHECK(b.Get<Position>()->value.x == doctest::Approx(1.f));
}

TEST_CASE("Without accepts a runtime-registered component type") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	// Hidden is built in; use a distinct name to avoid a duplicate registration.
	EcsResult<ComponentType> hidden = world.RegisterComponent("Frozen", {{"flag", FieldType::Bool}});
	REQUIRE(hidden);

	Entity visible = world.CreateEntity();
	visible.Add(Position{{0.f, 0.f}});
	Entity skipped = world.CreateEntity();
	skipped.Add(Position{{0.f, 0.f}});
	skipped.Add(hidden.value);

	i32 ran = 0;
	world.CreateSystem<Position>("Tick").Without(hidden.value).Each(
		[&](Entity, Position &) { ++ran; });

	world.Progress(1.f);
	CHECK(ran == 1);

	// An invalid ComponentType adds no term rather than breaking the query.
	i32 unfiltered = 0;
	world.CreateSystem<Position>("Unfiltered").Without(ComponentType()).Each(
		[&](Entity, Position &) { ++unfiltered; });

	world.Progress(1.f);
	CHECK(unfiltered == 2);
}
