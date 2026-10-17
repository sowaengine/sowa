#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/serialize/archive.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/component_type.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/entity.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/world.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

struct Label {
	std::string text;
	Color tint;
	// Exposed but not serialized: derived state the inspector shows and the
	// file must not carry.
	f32 measuredWidth = 0.f;
	// Not described at all - invisible to reflection, so equally invisible
	// here.
	i32 internalCounter = 0;
};

struct Link {
	Entity target;
};

ComponentType registerLabel(World &world) {
	return world.RegisterComponent<Label>("Label")
		.Field("text", &Label::text)
		.Field("tint", &Label::tint)
		.Field("measuredWidth", &Label::measuredWidth, FieldFlags::Expose)
		.Type();
}

} // namespace

TEST_CASE("A native component round-trips with no per-component code") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");
	REQUIRE(type.IsValid());

	Entity source = world.CreateEntity();
	source.Add<Transform2D>(Transform2D{
		.position = Vector2(12.f, -34.f),
		.rotation = 90.f,
		.scale = Vector2(2.f, 3.f),
	});

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, source, type));
	const std::string text = saving.ToString();

	Entity target = world.CreateEntity();
	Archive loading = Archive::Load(text);
	REQUIRE(loading.IsValid());
	REQUIRE(SerializeComponent(loading, target, type));
	CHECK_FALSE(loading.HasErrors());

	const Transform2D *restored = target.Get<Transform2D>();
	REQUIRE(restored != nullptr);
	CHECK(restored->position.x == doctest::Approx(12.f));
	CHECK(restored->position.y == doctest::Approx(-34.f));
	CHECK(restored->rotation == doctest::Approx(90.f));
	CHECK(restored->scale.y == doctest::Approx(3.f));
}

TEST_CASE("Loading adds the component when the entity lacks it") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");

	Entity entity = world.CreateEntity();
	REQUIRE_FALSE(entity.Has<Transform2D>());

	Archive loading = Archive::Load("position: [5, 6]");
	REQUIRE(SerializeComponent(loading, entity, type));

	REQUIRE(entity.Has<Transform2D>());
	CHECK(entity.Get<Transform2D>()->position.x == doctest::Approx(5.f));
}

TEST_CASE("Saving a component the entity doesn't have fails rather than writing junk") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");

	Entity entity = world.CreateEntity();
	Archive saving = Archive::Save();
	CHECK_FALSE(SerializeComponent(saving, entity, type));
	CHECK(saving.ToString().find("position") == std::string::npos);
}

TEST_CASE("Undescribed and unserialized fields stay out of the document") {
	World world;
	const ComponentType type = registerLabel(world);

	Entity entity = world.CreateEntity();
	entity.Add<Label>(Label{"hi", Color(1.f, 0.f, 0.f, 1.f), 42.f, 7});

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, entity, type));
	const std::string text = saving.ToString();

	CHECK(text.find("text: hi") != std::string::npos);
	CHECK(text.find("tint:") != std::string::npos);
	CHECK(text.find("measuredWidth") == std::string::npos);
	CHECK(text.find("internalCounter") == std::string::npos);
}

TEST_CASE("Transform2D's derived world matrix is not serialized") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");

	Entity entity = world.CreateEntity();
	entity.Add<Transform2D>();

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, entity, type));
	CHECK(saving.ToString().find("worldMatrix") == std::string::npos);
}

TEST_CASE("Enum fields are written by name, so a scene survives renumbering") {
	World world;
	const ComponentType type = world.FindComponent("RenderOrder");
	REQUIRE(type.IsValid());

	Entity source = world.CreateEntity();
	source.Add<RenderOrder>(RenderOrder{
		.zIndex = 5,
		.zIndexMode = ZIndexMode::Absolute,
		.layer = 2,
		.layerMode = RenderLayerMode::Explicit,
	});

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, source, type));
	const std::string text = saving.ToString();

	CHECK(text.find("zIndexMode: Absolute") != std::string::npos);
	CHECK(text.find("layerMode: Explicit") != std::string::npos);
	// Derived render-order values are excluded from reflection.
	CHECK(text.find("effectiveZIndex") == std::string::npos);
	CHECK(text.find("effectiveLayer") == std::string::npos);

	Entity target = world.CreateEntity();
	Archive loading = Archive::Load(text);
	REQUIRE(SerializeComponent(loading, target, type));

	const RenderOrder *restored = target.Get<RenderOrder>();
	REQUIRE(restored != nullptr);
	CHECK(restored->zIndex == 5);
	CHECK(restored->zIndexMode == ZIndexMode::Absolute);
	CHECK(restored->layer == 2);
	CHECK(restored->layerMode == RenderLayerMode::Explicit);
}

TEST_CASE("An unknown enum name is reported and leaves the default in place") {
	World world;
	const ComponentType type = world.FindComponent("RenderOrder");

	Entity entity = world.CreateEntity();
	Archive loading = Archive::Load("zIndex: 1\nzIndexMode: Sideways\n");
	REQUIRE(SerializeComponent(loading, entity, type));

	CHECK(entity.Get<RenderOrder>()->zIndex == 1);
	CHECK(entity.Get<RenderOrder>()->zIndexMode == ZIndexMode::Relative);
	REQUIRE(loading.Errors().size() == 1);
	CHECK(loading.Errors()[0].find("Sideways") != std::string::npos);
}

// The point of driving this off reflection rather than per-type code is that a
// component nobody wrote C++ for behaves identically. A script-defined or
// editor-defined component is exactly this shape.
TEST_CASE("A component described at runtime serializes the same way") {
	World world;
	EcsResult<ComponentType> registered = world.RegisterComponent(
		"Health",
		{
			{"current", FieldType::I32},
			{"label", FieldType::String},
			{"tint", FieldType::Color},
			{"secret", FieldType::I32, FieldFlags::None},
			{"mode", FieldType::Enum, FieldFlags::Default, "Stance",
			 {{"Idle", 0}, {"Guard", 1}}},
		});
	REQUIRE(registered.error == EcsError::Ok);
	const ComponentType type = registered.value;

	Entity source = world.CreateEntity();
	source.Add(type);
	source.GetField(type, "current").Set<i32>(75);
	source.GetField(type, "label").Set<std::string>("player");
	source.GetField(type, "tint").Set<Color>(Color(0.f, 1.f, 0.f, 1.f));
	source.GetField(type, "secret").Set<i32>(9999);
	*static_cast<i32 *>(source.GetField(type, "mode").Data()) = 1;

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, source, type));
	const std::string text = saving.ToString();

	CHECK(text.find("current: 75") != std::string::npos);
	CHECK(text.find("mode: Guard") != std::string::npos);
	CHECK(text.find("secret") == std::string::npos);

	Entity target = world.CreateEntity();
	Archive loading = Archive::Load(text);
	REQUIRE(SerializeComponent(loading, target, type));
	CHECK_FALSE(loading.HasErrors());

	i32 current = 0;
	std::string label;
	Color tint;
	CHECK(target.GetField(type, "current").Get<i32>(current));
	CHECK(current == 75);
	CHECK(target.GetField(type, "label").Get<std::string>(label));
	CHECK(label == "player");
	CHECK(target.GetField(type, "tint").Get<Color>(tint));
	CHECK(tint.g == doctest::Approx(1.f));
	CHECK(*static_cast<const i32 *>(target.GetField(type, "mode").Data()) == 1);

	// FieldFlags::None means storage without policy: it exists in the layout
	// but is neither saved nor restored.
	i32 secret = -1;
	CHECK(target.GetField(type, "secret").Get<i32>(secret));
	CHECK(secret == 0);
}

TEST_CASE("Fields absent from the document keep their defaults") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");

	Entity entity = world.CreateEntity();
	entity.Add<Transform2D>(Transform2D{.position = Vector2(1.f, 1.f),
										.rotation = 45.f,
										.scale = Vector2(4.f, 4.f)});

	Archive loading = Archive::Load("rotation: 10");
	REQUIRE(SerializeComponent(loading, entity, type));

	const Transform2D *transform = entity.Get<Transform2D>();
	CHECK(transform->rotation == doctest::Approx(10.f));
	CHECK(transform->position.x == doctest::Approx(1.f));
	CHECK(transform->scale.x == doctest::Approx(4.f));
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("Entity references are written as paths but not resolved on load") {
	World world;
	const ComponentType type =
		world.RegisterComponent<Link>("Link").Field("target", &Link::target).Type();

	Entity referenced = world.CreateEntity();
	referenced.SetName("Player");

	Entity source = world.CreateEntity();
	source.Add<Link>(Link{referenced});

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, source, type));
	CHECK(saving.ToString().find("target: Player") != std::string::npos);

	Entity target = world.CreateEntity();
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(SerializeComponent(loading, target, type));

	// Nothing was silently dropped: the path survived in the file, and the
	// load side says out loud that resolving it is the scene loader's job.
	CHECK_FALSE(target.Get<Link>()->target.IsAlive());
	REQUIRE(loading.Errors().size() == 1);
	CHECK(loading.Errors()[0].find("target") != std::string::npos);
}

TEST_CASE("A null entity reference round-trips without a complaint") {
	World world;
	const ComponentType type =
		world.RegisterComponent<Link>("Link").Field("target", &Link::target).Type();

	Entity source = world.CreateEntity();
	source.Add<Link>();

	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, source, type));

	Entity target = world.CreateEntity();
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(SerializeComponent(loading, target, type));
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("Components nest under a caller-chosen key") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");

	Entity entity = world.CreateEntity();
	entity.Add<Transform2D>(Transform2D{.position = Vector2(3.f, 4.f)});

	Archive saving = Archive::Save();
	{
		Archive child = saving.Child("Transform2D");
		REQUIRE(SerializeComponent(child, entity, type));
	}
	const std::string text = saving.ToString();
	CHECK(text.find("Transform2D:") != std::string::npos);

	Entity restored = world.CreateEntity();
	Archive loading = Archive::Load(text);
	Archive child = loading.Child("Transform2D");
	REQUIRE(SerializeComponent(child, restored, type));
	CHECK(restored.Get<Transform2D>()->position.y == doctest::Approx(4.f));
}
