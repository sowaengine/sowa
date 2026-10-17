#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/components/render_order.hpp"
#include "ecs/entity.hpp"
#include "ecs/field_registry.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/world.hpp"

using namespace sowa;

namespace {

enum class Facing { North, East, South, West };

// Non-contiguous values exercise enum-name lookup.
enum class Quality { Low = 10, Medium = 20, High = 40 };

struct Movement {
	Facing facing = Facing::North;
	f32 speed = 0.f;
};

struct Settings {
	Quality quality = Quality::Medium;
};

} // namespace

namespace sowa {
SOWA_REFLECT_ENUM(Facing,
				  {"North", Facing::North},
				  {"East", Facing::East},
				  {"South", Facing::South},
				  {"West", Facing::West});

SOWA_REFLECT_ENUM(Quality,
				  {"Low", Quality::Low},
				  {"Medium", Quality::Medium},
				  {"High", Quality::High});
} // namespace sowa

TEST_CASE("A native enum member is described with its constants") {
	World world;
	REQUIRE(world.RegisterComponent<Movement>("Movement")
				.Field("facing", &Movement::facing)
				.Field("speed", &Movement::speed)
				.Result());

	const ComponentType type = world.FindComponent("Movement");
	const FieldInfo *facing = type.FindField("facing");
	REQUIRE(facing != nullptr);

	CHECK(facing->type == FieldType::Enum);
	CHECK(facing->enumName == "Facing");
	REQUIRE(facing->enumConstants.size() == 4);
	CHECK(facing->enumConstants[0].name == "North");
	CHECK(facing->enumConstants[0].value == 0);
	CHECK(facing->enumConstants[3].name == "West");
	CHECK(facing->enumConstants[3].value == 3);

	// The enum member doesn't disturb the offsets of what follows it.
	const FieldInfo *speed = type.FindField("speed");
	REQUIRE(speed != nullptr);
	CHECK(speed->offset == offsetof(Movement, speed));
	CHECK(facing->offset == offsetof(Movement, facing));
}

TEST_CASE("Enum constants keep their declared values, gaps and all") {
	World world;
	REQUIRE(world.RegisterComponent<Settings>("Settings")
				.Field("quality", &Settings::quality)
				.Result());

	const FieldInfo *quality = world.FindComponent("Settings").FindField("quality");
	REQUIRE(quality != nullptr);
	REQUIRE(quality->enumConstants.size() == 3);
	CHECK(quality->enumConstants[0].value == 10);
	CHECK(quality->enumConstants[1].value == 20);
	CHECK(quality->enumConstants[2].value == 40);
}

TEST_CASE("Name and value lookups round-trip, which is what serialization needs") {
	World world;
	REQUIRE(world.RegisterComponent<Settings>("Settings")
				.Field("quality", &Settings::quality)
				.Result());

	const FieldInfo *quality = world.FindComponent("Settings").FindField("quality");
	REQUIRE(quality != nullptr);

	// Saving writes the name so a scene file survives constants being
	// renumbered; loading maps it back.
	const std::string *name = quality->FindEnumName(20);
	REQUIRE(name != nullptr);
	CHECK(*name == "Medium");

	i32 value = 0;
	REQUIRE(quality->FindEnumValue("High", value));
	CHECK(value == 40);

	// A value with no constant, and a name that no longer exists, both have to
	// be reportable rather than silently mapping to something wrong.
	CHECK(quality->FindEnumName(999) == nullptr);
	i32 untouched = -7;
	CHECK_FALSE(quality->FindEnumValue("Ludicrous", untouched));
	CHECK(untouched == -7);
}

TEST_CASE("An enum field reads and writes through FieldRef") {
	World world;
	const ComponentType type = world.RegisterComponent<Movement>("Movement")
								   .Field("facing", &Movement::facing)
								   .Type();
	REQUIRE(type.IsValid());

	Entity entity = world.CreateEntity();
	entity.Add(Movement{Facing::South, 0.f});

	FieldRef ref = entity.GetField(type, "facing");
	REQUIRE(ref.IsValid());
	CHECK(ref.Type() == FieldType::Enum);

	Facing read = Facing::North;
	REQUIRE(ref.Get(read));
	CHECK(read == Facing::South);

	REQUIRE(ref.Set(Facing::West));
	CHECK(entity.Get<Movement>()->facing == Facing::West);
}

TEST_CASE("FieldRef will not hand out one enum type for another") {
	World world;
	const ComponentType type = world.RegisterComponent<Movement>("Movement")
								   .Field("facing", &Movement::facing)
								   .Type();
	Entity entity = world.CreateEntity();
	entity.Add(Movement{Facing::East, 0.f});

	FieldRef ref = entity.GetField(type, "facing");
	REQUIRE(ref.IsValid());

	// Every reflected enum stores as i32, so matching on FieldType alone would
	// let this through and reinterpret the bytes as the wrong enum.
	CHECK(ref.As<Quality>() == nullptr);
	Quality quality = Quality::Low;
	CHECK_FALSE(ref.Get(quality));
	CHECK(quality == Quality::Low);

	// A plain i32 is a different FieldType and must not match either, even
	// though the storage is identical.
	CHECK(ref.As<i32>() == nullptr);

	// The right type still works.
	CHECK(ref.As<Facing>() != nullptr);
}

TEST_CASE("A data-described component can declare an enum inline") {
	World world;

	// The script/editor path: no C++ enum exists, the constants are data.
	FieldDesc mode;
	mode.name = "mode";
	mode.type = FieldType::Enum;
	mode.enumName = "BlendMode";
	mode.enumConstants = {{"Normal", 0}, {"Additive", 1}, {"Multiply", 2}};

	REQUIRE(world.RegisterComponent("Material", {mode, {"opacity", FieldType::F32}}));

	const ComponentType type = world.FindComponent("Material");
	const FieldInfo *info = type.FindField("mode");
	REQUIRE(info != nullptr);
	CHECK(info->type == FieldType::Enum);
	CHECK(info->enumName == "BlendMode");
	REQUIRE(info->enumConstants.size() == 3);
	CHECK(info->enumConstants[1].name == "Additive");

	// And it is real storage, reachable and writable by offset like any field.
	Entity entity = world.CreateEntity();
	entity.Add(type);
	FieldRef ref = entity.GetField(type, "mode");
	REQUIRE(ref.IsValid());
	CHECK(*static_cast<i32 *>(ref.Data()) == 0);
	*static_cast<i32 *>(ref.Data()) = 2;
	CHECK(*static_cast<i32 *>(entity.GetField(type, "mode").Data()) == 2);
}

TEST_CASE("Native and data-described enum fields share one storage layout") {
	World world;
	REQUIRE(world.RegisterComponent<Movement>("Movement")
				.Field("facing", &Movement::facing)
				.Result());

	// Native and runtime enum fields both use i32 storage, keeping reflected offsets compatible.
	FieldDesc facing;
	facing.name = "facing";
	facing.type = FieldType::Enum;
	facing.enumName = "Facing";
	facing.enumConstants = {{"North", 0}, {"East", 1}, {"South", 2}, {"West", 3}};
	REQUIRE(world.RegisterComponent("ScriptedMover", {facing}));

	const ComponentType scripted = world.FindComponent("ScriptedMover");
	Entity entity = world.CreateEntity();
	entity.Add(scripted);

	FieldRef ref = entity.GetField(scripted, "facing");
	REQUIRE(ref.IsValid());
	// Reusing the already-registered "Facing" enum type, so the typed
	// accessor recognises it even though this component never saw the C++ enum.
	REQUIRE(ref.Set(Facing::South));
	CHECK(*static_cast<i32 *>(ref.Data()) == 2);
}

TEST_CASE("An enum with more constants than flecs' fixed desc cache") {
	World world;

	// ecs_enum_init uses a fixed 32-slot descriptor array. Runtime constant lists must
	// register child constants instead of copying into that array.
	constexpr i32 kCount = 40;
	FieldDesc big;
	big.name = "step";
	big.type = FieldType::Enum;
	big.enumName = "BigEnum";
	for (i32 i = 0; i < kCount; ++i)
		big.enumConstants.push_back(EnumConstant{"Step" + std::to_string(i), i});

	REQUIRE(world.RegisterComponent("BigComponent", {big}));

	const FieldInfo *info = world.FindComponent("BigComponent").FindField("step");
	REQUIRE(info != nullptr);
	CHECK(info->enumConstants.size() == kCount);

	const std::string *name = info->FindEnumName(39);
	REQUIRE(name != nullptr);
	CHECK(*name == "Step39");

	// The claim is that *flecs* holds all 40 too, not just our own copy of the
	// list - truncation at 32 is precisely what the incremental route avoids,
	// and checking only sowa's table would miss it entirely.
	flecs::world raw;
	const flecs::entity_t type = ResolveEnumTypeId(raw, "BigProbe", big.enumConstants);
	REQUIRE(type != 0);
	const EcsConstants *table =
		static_cast<const EcsConstants *>(ecs_get_id(raw.c_ptr(), type, ecs_id(EcsConstants)));
	REQUIRE(table != nullptr);
	CHECK(ecs_vec_count(&table->ordered_constants) == kCount);
}

TEST_CASE("The built-in ZIndexMode enum is reflected") {
	World world;

	const FieldInfo *mode = world.FindComponent("RenderOrder").FindField("zIndexMode");
	REQUIRE(mode != nullptr);
	CHECK(mode->type == FieldType::Enum);
	CHECK(mode->enumName == "ZIndexMode");
	REQUIRE(mode->enumConstants.size() == 2);
	CHECK(mode->enumConstants[0].name == "Relative");
	CHECK(mode->enumConstants[1].name == "Absolute");

	Entity entity = world.CreateEntity();
	entity.Add(RenderOrder{4, ZIndexMode::Absolute});

	FieldRef ref = entity.GetField(world.FindComponent("RenderOrder"), "zIndexMode");
	REQUIRE(ref.IsValid());
	ZIndexMode read = ZIndexMode::Relative;
	REQUIRE(ref.Get(read));
	CHECK(read == ZIndexMode::Absolute);
}

TEST_CASE("flecs itself understands the enum types we register by hand") {
	// Check flecs constant storage independently of the engine reflection table.
	flecs::world world;

	const flecs::entity_t type =
		ResolveEnumTypeId(world, "ProbeQuality", {{"Low", 10}, {"Medium", 20}, {"High", 40}});
	REQUIRE(type != 0);

	flecs::entity typeEntity(world, type);
	REQUIRE(typeEntity.has(ecs_id(EcsEnum)));
	CHECK(typeEntity.try_get<EcsEnum>()->underlying_type == ecs_id(ecs_i32_t));

	const EcsConstants *table =
		static_cast<const EcsConstants *>(ecs_get_id(world.c_ptr(), type, ecs_id(EcsConstants)));
	REQUIRE(table != nullptr);

	const i32 count = ecs_vec_count(&table->ordered_constants);
	REQUIRE(count == 3);
	const ecs_enum_constant_t *entries =
		static_cast<const ecs_enum_constant_t *>(ecs_vec_first(&table->ordered_constants));
	CHECK(std::string(entries[0].name) == "Low");
	CHECK(entries[0].value == 10);
	CHECK(std::string(entries[2].name) == "High");
	CHECK(entries[2].value == 40);
}

TEST_CASE("A zero-valued enum constant survives registration") {
	// Zero is the case ecs_enum_init cannot express through its desc struct -
	// it reads a zero as "unset" and falls back to auto-assignment - so going
	// through the (EcsConstant, i32) pair directly gets its own check.
	flecs::world world;

	const flecs::entity_t type =
		ResolveEnumTypeId(world, "ProbeZero", {{"None", 0}, {"Some", 7}});
	REQUIRE(type != 0);

	const EcsConstants *table =
		static_cast<const EcsConstants *>(ecs_get_id(world.c_ptr(), type, ecs_id(EcsConstants)));
	REQUIRE(table != nullptr);
	REQUIRE(ecs_vec_count(&table->ordered_constants) == 2);

	const ecs_enum_constant_t *entries =
		static_cast<const ecs_enum_constant_t *>(ecs_vec_first(&table->ordered_constants));
	CHECK(std::string(entries[0].name) == "None");
	CHECK(entries[0].value == 0);
	CHECK(std::string(entries[1].name) == "Some");
	CHECK(entries[1].value == 7);
}

TEST_CASE("Resolving the same enum name twice reuses one type entity") {
	flecs::world world;

	const flecs::entity_t first = ResolveEnumTypeId(world, "Shared", {{"A", 0}, {"B", 1}});
	const flecs::entity_t second = ResolveEnumTypeId(world, "Shared", {{"A", 0}, {"B", 1}});

	REQUIRE(first != 0);
	CHECK(first == second);

	// And the second call did not duplicate the constants onto the type.
	const EcsConstants *table =
		static_cast<const EcsConstants *>(ecs_get_id(world.c_ptr(), first, ecs_id(EcsConstants)));
	REQUIRE(table != nullptr);
	CHECK(ecs_vec_count(&table->ordered_constants) == 2);
}

TEST_CASE("An enum type entity is not returned by an entity path lookup") {
	World world;
	REQUIRE(world.RegisterComponent<Movement>("Movement")
				.Field("facing", &Movement::facing)
				.Result());

	// Enum type entities must be excluded from scene-path lookup.
	CHECK_FALSE(world.Find("Facing").IsAlive());
	CHECK_FALSE(world.Find("ZIndexMode").IsAlive());

	// A real entity may still take that name.
	Entity entity = world.CreateEntity();
	CHECK(entity.SetName("Facing") == "Facing2");
}
