#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/component_type.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "core/reflection/field_ref.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/world.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

struct Transform {
	Vector2 position;
	f32 rotation = 0.f;
	// Derived state: exposed to the inspector, never written to disk.
	f32 cachedAngleRadians = 0.f;
	// Not described at all - invisible to reflection.
	i32 internalCounter = 0;
};

struct Label {
	std::string text;
	Color tint;
};

struct Follower {
	Entity target;
};

} // namespace

TEST_CASE("Native components describe their fields via pointer-to-member") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("position", &Transform::position)
							 .Field("rotation", &Transform::rotation)
							 .Field("cachedAngleRadians", &Transform::cachedAngleRadians, FieldFlags::Expose)
							 .Type();

	REQUIRE(type.IsValid());
	REQUIRE(type.Fields().size() == 3);

	CHECK(type.Fields()[0].name == "position");
	CHECK(type.Fields()[0].type == FieldType::Vector2);
	CHECK(type.Fields()[1].name == "rotation");
	CHECK(type.Fields()[1].type == FieldType::F32);
}

TEST_CASE("Native field offsets match the real C++ struct layout") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("position", &Transform::position)
							 .Field("rotation", &Transform::rotation)
							 .Type();

	const FieldInfo *position = type.FindField("position");
	const FieldInfo *rotation = type.FindField("rotation");
	REQUIRE(position != nullptr);
	REQUIRE(rotation != nullptr);

	CHECK(position->offset == offsetof(Transform, position));
	CHECK(rotation->offset == offsetof(Transform, rotation));
}

TEST_CASE("Undescribed native members are invisible to reflection") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("position", &Transform::position)
							 .Type();

	CHECK(type.FindField("internalCounter") == nullptr);
	CHECK(type.FindField("rotation") == nullptr);
}

TEST_CASE("A native component with no described fields reflects as empty") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform").Type();

	REQUIRE(type.IsValid());
	CHECK(type.Fields().empty());
	CHECK(type.FindField("position") == nullptr);
}

TEST_CASE("Serialize and Expose are independent per-field flags") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("position", &Transform::position)
							 .Field("cachedAngleRadians", &Transform::cachedAngleRadians, FieldFlags::Expose)
							 .Field("rotation", &Transform::rotation, FieldFlags::None)
							 .Type();

	const FieldInfo *position = type.FindField("position");
	const FieldInfo *cached = type.FindField("cachedAngleRadians");
	const FieldInfo *internal = type.FindField("rotation");
	REQUIRE(position != nullptr);
	REQUIRE(cached != nullptr);
	REQUIRE(internal != nullptr);

	CHECK(position->IsSerialized());
	CHECK(position->IsExposed());

	// Derived state: shown, not saved.
	CHECK_FALSE(cached->IsSerialized());
	CHECK(cached->IsExposed());

	// Internal: occupies the layout, but neither shown nor saved.
	CHECK_FALSE(internal->IsSerialized());
	CHECK_FALSE(internal->IsExposed());
}

TEST_CASE("Data-described components report field names, types and offsets") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent(
		"Health", {
					  {"current", FieldType::I32},
					  {"max", FieldType::I32},
					  {"regenPerSecond", FieldType::F32, FieldFlags::Expose},
				  });
	REQUIRE(registered);

	const ComponentType type = registered.value;
	REQUIRE(type.Fields().size() == 3);

	CHECK(type.Fields()[0].name == "current");
	CHECK(type.Fields()[0].type == FieldType::I32);
	CHECK(type.Fields()[0].offset == 0);

	// flecs packs the layout; whatever it picked, the fields must not
	// overlap and must be ordered.
	CHECK(type.Fields()[1].offset >= sizeof(i32));
	CHECK(type.Fields()[2].offset > type.Fields()[1].offset);

	CHECK_FALSE(type.Fields()[2].IsSerialized());
	CHECK(type.Fields()[2].IsExposed());
}

TEST_CASE("FindField returns nullptr for unknown fields and invalid types") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent("Health", {{"current", FieldType::I32}});
	REQUIRE(registered);

	CHECK(registered.value.FindField("current") != nullptr);
	CHECK(registered.value.FindField("missing") == nullptr);

	ComponentType invalid;
	CHECK(invalid.FindField("current") == nullptr);
	CHECK(invalid.Fields().empty());
}

TEST_CASE("GetField reads and writes a native component's fields") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("position", &Transform::position)
							 .Field("rotation", &Transform::rotation)
							 .Type();

	Entity entity = world.CreateEntity();
	entity.Add(Transform{{10.f, 20.f}, 45.f, 0.f, 0});

	FieldRef position = entity.GetField(type, "position");
	REQUIRE(position.IsValid());
	CHECK(position.Type() == FieldType::Vector2);

	Vector2 value;
	REQUIRE(position.Get(value));
	CHECK(value.x == doctest::Approx(10.f));
	CHECK(value.y == doctest::Approx(20.f));

	CHECK(position.Set(Vector2{-5.f, 7.5f}));
	CHECK(entity.Get<Transform>()->position.x == doctest::Approx(-5.f));
	CHECK(entity.Get<Transform>()->position.y == doctest::Approx(7.5f));

	// Writes land in the real struct member, so typed access sees them too.
	CHECK(entity.GetField(type, "rotation").Set(90.f));
	CHECK(entity.Get<Transform>()->rotation == doctest::Approx(90.f));
}

TEST_CASE("GetField reads and writes a data-described component's fields") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent(
		"Health", {
					  {"current", FieldType::I32},
					  {"max", FieldType::I32},
				  });
	REQUIRE(registered);
	const ComponentType type = registered.value;

	Entity entity = world.CreateEntity();
	entity.Add(type);

	// Default-initialized storage.
	i32 current = -1;
	REQUIRE(entity.GetField(type, "current").Get(current));
	CHECK(current == 0);

	CHECK(entity.GetField(type, "current").Set(30));
	CHECK(entity.GetField(type, "max").Set(100));

	REQUIRE(entity.GetField(type, "current").Get(current));
	CHECK(current == 30);

	i32 max = 0;
	REQUIRE(entity.GetField(type, "max").Get(max));
	CHECK(max == 100);
}

TEST_CASE("A field's type is checked before its memory is touched") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("rotation", &Transform::rotation)
							 .Type();

	Entity entity = world.CreateEntity();
	entity.Add<Transform>();

	FieldRef rotation = entity.GetField(type, "rotation");
	REQUIRE(rotation.IsValid());

	CHECK(rotation.As<f32>() != nullptr);
	CHECK(rotation.As<i32>() == nullptr);

	i32 wrongType = 7;
	CHECK_FALSE(rotation.Get(wrongType));
	CHECK(wrongType == 7);

	CHECK_FALSE(rotation.Set(i32{3}));
	CHECK(entity.Get<Transform>()->rotation == doctest::Approx(0.f));
}

TEST_CASE("GetField is safe on missing components, fields and dead entities") {
	World world;
	ComponentType type = world.RegisterComponent<Transform>("Transform")
							 .Field("rotation", &Transform::rotation)
							 .Type();

	Entity entity = world.CreateEntity();

	// Component not present on the entity.
	CHECK_FALSE(entity.GetField(type, "rotation").IsValid());

	entity.Add<Transform>();
	CHECK_FALSE(entity.GetField(type, "missing").IsValid());
	CHECK_FALSE(entity.GetField(ComponentType(), "rotation").IsValid());

	entity.Destroy();
	CHECK_FALSE(entity.GetField(type, "rotation").IsValid());

	Entity null;
	CHECK_FALSE(null.GetField(type, "rotation").IsValid());
}

TEST_CASE("An invalid FieldRef reads and writes nothing") {
	FieldRef ref;

	CHECK_FALSE(ref.IsValid());
	CHECK(ref.Data() == nullptr);
	CHECK(ref.Info() == nullptr);
	CHECK(ref.As<f32>() == nullptr);

	f32 value = 1.f;
	CHECK_FALSE(ref.Get(value));
	CHECK(value == doctest::Approx(1.f));
	CHECK_FALSE(ref.Set(2.f));
}

TEST_CASE("String fields are std::string on the native path") {
	World world;
	ComponentType type = world.RegisterComponent<Label>("Label")
							 .Field("text", &Label::text)
							 .Field("tint", &Label::tint)
							 .Type();

	REQUIRE(type.FindField("text") != nullptr);
	CHECK(type.FindField("text")->type == FieldType::String);
	CHECK(type.FindField("text")->offset == offsetof(Label, text));

	Entity entity = world.CreateEntity();
	entity.Add<Label>();

	CHECK(entity.GetField(type, "text").Set(std::string("hello")));
	CHECK(entity.Get<Label>()->text == "hello");

	std::string text;
	REQUIRE(entity.GetField(type, "text").Get(text));
	CHECK(text == "hello");
}

TEST_CASE("String fields are std::string on the data-described path too") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent("Named", {{"name", FieldType::String}});
	REQUIRE(registered);
	const ComponentType type = registered.value;

	Entity entity = world.CreateEntity();
	entity.Add(type);

	// Default-constructed, not a dangling char* - the whole reason String
	// maps to std::string on both paths.
	std::string name = "unset";
	REQUIRE(entity.GetField(type, "name").Get(name));
	CHECK(name.empty());

	// Long enough to force a heap allocation (past libstdc++/libc++'s small
	// string buffer), so this actually exercises ownership.
	const std::string longName = "player one, with a name far too long to fit in a small string buffer";
	CHECK(entity.GetField(type, "name").Set(longName));
	REQUIRE(entity.GetField(type, "name").Get(name));
	CHECK(name == longName);

	// Destroying the entity must run the std::string destructor rather than
	// leaking or freeing a raw pointer.
	entity.Destroy();
}

TEST_CASE("EntityRef fields round-trip a live entity handle") {
	World world;
	ComponentType type = world.RegisterComponent<Follower>("Follower")
							 .Field("target", &Follower::target)
							 .Type();

	REQUIRE(type.FindField("target") != nullptr);
	CHECK(type.FindField("target")->type == FieldType::EntityRef);

	Entity target = world.CreateEntity();
	Entity follower = world.CreateEntity();
	follower.Add<Follower>();

	CHECK(follower.GetField(type, "target").Set(target));

	Entity stored;
	REQUIRE(follower.GetField(type, "target").Get(stored));
	CHECK(stored == target);
	CHECK(stored.IsAlive());

	// A destroyed entity reference remains dead when its ID is recycled.
	target.Destroy();
	REQUIRE(follower.GetField(type, "target").Get(stored));
	CHECK_FALSE(stored.IsAlive());
}

namespace {

// String relocation requires ctor_move_dtor hooks; memcpy corrupts libstdc++ short strings.
const ecs_type_info_t *TypeInfoOf(ComponentType type) {
	return ecs_get_type_info(type.Raw().world(), type.Raw().id());
}

} // namespace

TEST_CASE("A native component holding a std::string gets full lifecycle hooks") {
	World world;
	ComponentType type = world.RegisterComponent<Label>("Label")
							 .Field("text", &Label::text)
							 .Type();

	const ecs_type_info_t *info = TypeInfoOf(type);
	REQUIRE(info != nullptr);

	CHECK(info->hooks.ctor != nullptr);
	CHECK(info->hooks.dtor != nullptr);
	CHECK(info->hooks.move != nullptr);
	CHECK(info->hooks.copy != nullptr);
	// The one that keeps table growth off the memcpy path.
	CHECK(info->hooks.ctor_move_dtor != nullptr);
}

TEST_CASE("A data-described component with a String field gets full lifecycle hooks") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent("Named", {{"name", FieldType::String}});
	REQUIRE(registered);

	const ecs_type_info_t *info = TypeInfoOf(registered.value);
	REQUIRE(info != nullptr);

	// Composed by flecs' RTT layer from the std::string member's own hooks.
	CHECK(info->hooks.ctor != nullptr);
	CHECK(info->hooks.dtor != nullptr);
	CHECK(info->hooks.move != nullptr);
	CHECK(info->hooks.copy != nullptr);
	CHECK(info->hooks.ctor_move_dtor != nullptr);
}

TEST_CASE("String fields survive table growth, migration and swap-removal") {
	World world;
	EcsResult<ComponentType> named = world.RegisterComponent("Named", {{"name", FieldType::String}});
	EcsResult<ComponentType> tag = world.RegisterComponent("Tag", {{"value", FieldType::I32}});
	REQUIRE(named);
	REQUIRE(tag);

	const ComponentType nameType = named.value;
	constexpr i32 count = 512;

	auto expected = [](i32 i) {
		// Long enough to heap-allocate, so relocation bugs surface as
		// use-after-free rather than as quietly-copied inline bytes.
		return "entity number " + std::to_string(i) + " with a deliberately long name to defeat SSO";
	};

	// Grows the table well past its initial capacity.
	std::vector<Entity> entities;
	for (i32 i = 0; i < count; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(nameType);
		REQUIRE(entity.GetField(nameType, "name").Set(expected(i)));
		entities.push_back(entity);
	}

	for (i32 i = 0; i < count; ++i) {
		std::string name;
		REQUIRE(entities[i].GetField(nameType, "name").Get(name));
		CHECK(name == expected(i));
	}

	// Migrates every other entity to a different archetype, which moves its
	// component data between tables.
	for (i32 i = 0; i < count; i += 2)
		entities[i].Add(tag.value);

	for (i32 i = 0; i < count; ++i) {
		std::string name;
		REQUIRE(entities[i].GetField(nameType, "name").Get(name));
		CHECK(name == expected(i));
	}

	// Destroying from the front repeatedly swap-removes the last row into the
	// hole, exercising the move path in the other direction.
	for (i32 i = 0; i < count; i += 3)
		entities[i].Destroy();

	for (i32 i = 0; i < count; ++i) {
		if (i % 3 == 0) {
			CHECK_FALSE(entities[i].IsAlive());
			continue;
		}
		std::string name;
		REQUIRE(entities[i].GetField(nameType, "name").Get(name));
		CHECK(name == expected(i));
	}
}

TEST_CASE("EntityRef fields work on the data-described path too") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent("Link", {{"to", FieldType::EntityRef}});
	REQUIRE(registered);
	const ComponentType type = registered.value;

	Entity target = world.CreateEntity();
	Entity source = world.CreateEntity();
	source.Add(type);

	// Default-initialized to a null handle, not to garbage.
	Entity stored = target;
	REQUIRE(source.GetField(type, "to").Get(stored));
	CHECK_FALSE(stored.IsAlive());

	CHECK(source.GetField(type, "to").Set(target));
	REQUIRE(source.GetField(type, "to").Get(stored));
	CHECK(stored == target);
	CHECK(stored.IsAlive());
}

TEST_CASE("Both registration paths agree on the layout of a shared field type") {
	World world;
	ComponentType native = world.RegisterComponent<Transform>("Transform")
							   .Field("position", &Transform::position)
							   .Type();
	EcsResult<ComponentType> described = world.RegisterComponent("Marker", {{"position", FieldType::Vector2}});
	REQUIRE(described);

	Entity a = world.CreateEntity();
	a.Add<Transform>();
	Entity b = world.CreateEntity();
	b.Add(described.value);

	CHECK(a.GetField(native, "position").Set(Vector2{3.f, 4.f}));
	CHECK(b.GetField(described.value, "position").Set(Vector2{3.f, 4.f}));

	Vector2 fromNative;
	Vector2 fromDescribed;
	REQUIRE(a.GetField(native, "position").Get(fromNative));
	REQUIRE(b.GetField(described.value, "position").Get(fromDescribed));
	CHECK(fromNative.x == doctest::Approx(fromDescribed.x));
	CHECK(fromNative.y == doctest::Approx(fromDescribed.y));
}

TEST_CASE("A failed registration yields an inert builder") {
	World world;
	REQUIRE(world.RegisterComponent<Transform>("Transform").Result());

	ComponentBuilder<Transform> duplicate = world.RegisterComponent<Transform>("Transform");
	CHECK_FALSE(duplicate);
	CHECK(duplicate.Error() == EcsError::DuplicateName);

	// Field() on an inert builder is a no-op, not a crash.
	duplicate.Field("position", &Transform::position);
	CHECK_FALSE(duplicate.Type().IsValid());
	CHECK(duplicate.Type().Fields().empty());
}
