#include <string>

#include <doctest/doctest.h>

#include "core/log.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/component_type.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "ecs/script_tags.hpp"
#include "ecs/world.hpp"

using namespace sowa;

TEST_CASE("ReplaceDataDescribedFields copies values by name, not offset") {
	World world;
	EcsResult<ComponentType> created = world.RegisterComponent("Player", {
																			 {"speed", FieldType::F32},
																			 {"lives", FieldType::I32},
																		 });
	REQUIRE(created);
	ComponentType type = created.value;

	Entity live = world.CreateEntity();
	live.Add(type);
	REQUIRE(live.GetField(type, "speed").Set(350.f));
	REQUIRE(live.GetField(type, "lives").Set(3));

	Entity frozen = world.CreateEntity();
	frozen.Add(type);
	REQUIRE(frozen.GetField(type, "speed").Set(200.f));
	frozen.SetEnabled(false);

	EcsResult<ComponentType> replaced = world.ReplaceDataDescribedFields(
		type, {{"jump", FieldType::F32}, {"speed", FieldType::F32}});
	REQUIRE(replaced);
	CHECK(replaced.error == EcsError::Ok);
	type = replaced.value;

	REQUIRE(type.FindField("jump") != nullptr);
	REQUIRE(type.FindField("speed") != nullptr);
	CHECK(type.FindField("lives") == nullptr);

	f32 speed = 0.f;
	f32 jump = -1.f;
	REQUIRE(live.GetField(type, "speed").Get(speed));
	REQUIRE(live.GetField(type, "jump").Get(jump));
	CHECK(speed == doctest::Approx(350.f));
	CHECK(jump == doctest::Approx(0.f));

	speed = 0.f;
	REQUIRE(frozen.GetField(type, "speed").Get(speed));
	CHECK(speed == doctest::Approx(200.f));
	CHECK(frozen.Has(type));
}

TEST_CASE("ReplaceDataDescribedFields refuses native components") {
	World world;
	struct Native {
		f32 x = 0.f;
	};
	ComponentType type = world.RegisterComponent<Native>("Native").Type();
	EcsResult<ComponentType> replaced =
		world.ReplaceDataDescribedFields(type, {{"x", FieldType::F32}});
	CHECK_FALSE(replaced);
	CHECK(replaced.error == EcsError::NotDataDescribed);
}

TEST_CASE("ScriptLive pairs survive a field migrate") {
	World world;
	EcsResult<ComponentType> created = world.RegisterComponent("Buff", {{"amount", FieldType::F32}});
	REQUIRE(created);
	ComponentType type = created.value;
	Entity entity = world.CreateEntity();
	entity.Add(type);
	entity.Raw().add<ScriptLive>(type.Raw());

	EcsResult<ComponentType> replaced =
		world.ReplaceDataDescribedFields(type, {{"amount", FieldType::F32},
												{"ticks", FieldType::F32}});
	REQUIRE(replaced);
	CHECK(entity.Raw().has<ScriptLive>(replaced.value.Raw()));
}

TEST_CASE("RegisterUntypedSystem walks matching entities") {
	World world;
	EcsResult<ComponentType> healthCreated = world.RegisterComponent("Health", {{"hp", FieldType::F32}});
	EcsResult<ComponentType> poisonCreated = world.RegisterComponent("Poison", {{"dps", FieldType::F32}});
	REQUIRE(healthCreated);
	REQUIRE(poisonCreated);
	ComponentType health = healthCreated.value;
	ComponentType poison = poisonCreated.value;

	Entity sick = world.CreateEntity();
	sick.Add(health);
	sick.Add(poison);
	REQUIRE(sick.GetField(health, "hp").Set(10.f));
	REQUIRE(sick.GetField(poison, "dps").Set(3.f));

	Entity well = world.CreateEntity();
	well.Add(health);
	REQUIRE(well.GetField(health, "hp").Set(10.f));

	i32 ran = 0;
	world.RegisterUntypedSystem("PoisonDamage", {health, poison}, {}, false, [&](Entity entity) {
		++ran;
		f32 hp = 0.f;
		f32 dps = 0.f;
		entity.GetField(health, "hp").Get(hp);
		entity.GetField(poison, "dps").Get(dps);
		entity.GetField(health, "hp").Set(hp - dps);
	});

	world.Progress(1.f);
	CHECK(ran == 1);
	f32 hp = 0.f;
	REQUIRE(sick.GetField(health, "hp").Get(hp));
	CHECK(hp == doctest::Approx(7.f));
	REQUIRE(well.GetField(health, "hp").Get(hp));
	CHECK(hp == doctest::Approx(10.f));
}

TEST_CASE("RegisterUntypedSystem with no terms runs once per frame") {
	World world;
	i32 ran = 0;
	world.RegisterUntypedSystem("Tick", {}, {}, false, [&](Entity entity) {
		++ran;
		CHECK_FALSE(entity.IsAlive());
	});
	world.Progress(1.f);
	CHECK(ran == 1);
	world.Progress(1.f);
	CHECK(ran == 2);
}

TEST_CASE("ComponentFieldsMatch ignores field order") {
	World world;
	EcsResult<ComponentType> created = world.RegisterComponent("Player", {
																			 {"speed", FieldType::F32},
																			 {"lives", FieldType::I32},
																		 });
	REQUIRE(created);
	ComponentType type = created.value;
	CHECK(world.ComponentFieldsMatch(type, {{"lives", FieldType::I32}, {"speed", FieldType::F32}}));
	CHECK_FALSE(world.ComponentFieldsMatch(type, {{"speed", FieldType::F32}}));
}
