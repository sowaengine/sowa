#include <doctest/doctest.h>

#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/world.hpp"

using namespace sowa;

namespace {
struct Position {
	f32 x = 0.f;
	f32 y = 0.f;
};

struct Speed {
	f32 value = 0.f;
};
} // namespace

TEST_CASE("Entity Add/Has/Get/Remove for native components") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity e = world.CreateEntity();
	CHECK_FALSE(e.Has<Position>());
	CHECK(e.Get<Position>() == nullptr);

	e.Add(Position{3.f, 4.f});
	CHECK(e.Has<Position>());

	Position *p = e.Get<Position>();
	REQUIRE(p != nullptr);
	CHECK(p->x == doctest::Approx(3.f));
	CHECK(p->y == doctest::Approx(4.f));

	p->x = 10.f;
	CHECK(e.Get<Position>()->x == doctest::Approx(10.f));

	e.Remove<Position>();
	CHECK_FALSE(e.Has<Position>());
	CHECK(e.Get<Position>() == nullptr);
}

TEST_CASE("Entity::Add<T>() default-constructs and is idempotent") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity e = world.CreateEntity();
	e.Add<Position>();
	REQUIRE(e.Has<Position>());
	CHECK(e.Get<Position>()->x == doctest::Approx(0.f));

	e.Get<Position>()->x = 42.f;
	e.Add<Position>(); // already present - must not reset
	CHECK(e.Get<Position>()->x == doctest::Approx(42.f));
}

TEST_CASE("Entity component ops are safe on dead/null handles") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity dead;
	dead.Add(Position{1.f, 2.f});
	dead.Add<Position>();
	CHECK_FALSE(dead.Has<Position>());
	CHECK(dead.Get<Position>() == nullptr);
	dead.Remove<Position>();

	Entity e = world.CreateEntity();
	e.Add(Position{1.f, 2.f});
	e.Destroy();
	e.Add(Position{9.f, 9.f});
	CHECK_FALSE(e.Has<Position>());
	CHECK(e.Get<Position>() == nullptr);
	e.Remove<Position>();
}

TEST_CASE("Entity untyped Add/Has/Get/Remove by ComponentType") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent(
		"Health", {{"current", FieldType::I32}, {"max", FieldType::I32}});
	REQUIRE(registered);
	ComponentType health = registered.value;

	Entity e = world.CreateEntity();
	CHECK_FALSE(e.Has(health));
	CHECK(e.Get(health) == nullptr);

	e.Add(health);
	CHECK(e.Has(health));
	void *blob = e.Get(health);
	REQUIRE(blob != nullptr);

	// Layout is flecs-computed; I32 fields are at the start for this simple
	// struct. Write through the raw pointer and confirm Add is idempotent.
	auto *fields = static_cast<i32 *>(blob);
	fields[0] = 7;
	fields[1] = 10;

	e.Add(health); // already present - leave values alone
	CHECK(static_cast<i32 *>(e.Get(health))[0] == 7);
	CHECK(static_cast<i32 *>(e.Get(health))[1] == 10);

	e.Remove(health);
	CHECK_FALSE(e.Has(health));
	CHECK(e.Get(health) == nullptr);
}

TEST_CASE("Untyped ops are safe with invalid ComponentType or dead entity") {
	World world;
	ComponentType invalid;
	Entity e = world.CreateEntity();
	e.Add(invalid);
	CHECK_FALSE(e.Has(invalid));
	CHECK(e.Get(invalid) == nullptr);
	e.Remove(invalid);

	EcsResult<ComponentType> registered =
		world.RegisterComponent("Flag", {{"on", FieldType::Bool}});
	REQUIRE(registered);

	Entity dead;
	dead.Add(registered.value);
	CHECK_FALSE(dead.Has(registered.value));
	CHECK(dead.Get(registered.value) == nullptr);
}

TEST_CASE("World::FindComponent looks up by name") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	EcsResult<ComponentType> health = world.RegisterComponent(
		"Health", {{"current", FieldType::I32}});
	REQUIRE(health);

	ComponentType foundNative = world.FindComponent("Position");
	CHECK(foundNative.IsValid());

	ComponentType foundDynamic = world.FindComponent("Health");
	CHECK(foundDynamic.IsValid());
	CHECK(foundDynamic == health.value);

	CHECK_FALSE(world.FindComponent("Missing").IsValid());
}

TEST_CASE("World::Each iterates entities with the requested components") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());
	REQUIRE(world.RegisterComponent<Speed>("Speed").Result());

	Entity a = world.CreateEntity();
	a.Add(Position{1.f, 2.f});
	a.Add(Speed{1.f});

	Entity b = world.CreateEntity();
	b.Add(Position{5.f, 6.f});

	Entity c = world.CreateEntity();
	c.Add(Speed{2.f});

	int withPosition = 0;
	world.Each<Position>([&](Entity, Position &pos) {
		withPosition++;
		pos.x += 1.f;
	});
	CHECK(withPosition == 2);
	CHECK(a.Get<Position>()->x == doctest::Approx(2.f));
	CHECK(b.Get<Position>()->x == doctest::Approx(6.f));

	int withBoth = 0;
	world.Each<Position, Speed>([&](Entity e, Position &, Speed &) {
		withBoth++;
		CHECK(e == a);
	});
	CHECK(withBoth == 1);
}
