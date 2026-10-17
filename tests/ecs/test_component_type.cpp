#include <doctest/doctest.h>

#include "ecs/component_type.hpp"
#include "ecs/ecs_types.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/world.hpp"

using namespace sowa;

namespace {
struct Velocity {
	f32 dx = 0.f;
	f32 dy = 0.f;
};
} // namespace

TEST_CASE("World::RegisterComponent<T> rejects duplicate names") {
	World world;
	CHECK(world.RegisterComponent<Velocity>("Velocity").Result());

	EcsResult<ComponentType> duplicate = world.RegisterComponent<Velocity>("Velocity").Result();
	CHECK_FALSE(duplicate);
	CHECK(duplicate.error == EcsError::DuplicateName);
}

TEST_CASE("World::RegisterComponent(name, fields) rejects duplicate names") {
	World world;
	CHECK(world.RegisterComponent("Health", {{"current", FieldType::I32}}));

	EcsResult<ComponentType> duplicate = world.RegisterComponent("Health", {{"current", FieldType::I32}});
	CHECK_FALSE(duplicate);
	CHECK(duplicate.error == EcsError::DuplicateName);
}

TEST_CASE("Native and data-described registration share the same name namespace") {
	World world;
	CHECK(world.RegisterComponent<Velocity>("Thing").Result());

	EcsResult<ComponentType> duplicate = world.RegisterComponent("Thing", {{"x", FieldType::F32}});
	CHECK_FALSE(duplicate);
	CHECK(duplicate.error == EcsError::DuplicateName);
}
