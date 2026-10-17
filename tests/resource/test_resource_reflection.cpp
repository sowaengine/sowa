#include <memory>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/reflection/field_type.hpp"
#include "core/serialize/archive.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "resource/resource.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

enum class Filter { Nearest, Linear };

// Everything a real resource type needs: primitives, a math value type, an
// enum, a field that's shown but never saved, and an internal member that is
// simply never described.
struct Texture : public Resource {
	std::string path;
	Vector2 size{0.f, 0.f};
	Color tint;
	Filter filter = Filter::Nearest;
	bool flip = false;

	i32 gpuHandle = 0;   // exposed to the inspector, meaningless on disk
	i32 decodedBytes = 0; // never described at all
};

struct Clip : public Resource {
	std::string path;
	f32 volume = 1.f;
};

} // namespace

namespace sowa {
SOWA_REFLECT_ENUM(Filter, {"Nearest", Filter::Nearest}, {"Linear", Filter::Linear});
} // namespace sowa

namespace {

void describeTexture(ResourceRegistry &registry) {
	registry.RegisterType<Texture>("Texture")
		.Field("path", &Texture::path)
		.Field("size", &Texture::size)
		.Field("tint", &Texture::tint)
		.Field("filter", &Texture::filter)
		.Field("flip", &Texture::flip)
		.Field("gpuHandle", &Texture::gpuHandle, FieldFlags::Expose);
}

} // namespace

TEST_CASE("A described resource round-trips with no hand-written Serialize") {
	ResourceRegistry registry;
	describeTexture(registry);

	Ref<Texture> texture = registry.Create<Texture>();
	REQUIRE(texture != nullptr);
	texture->path = "res://icon.png";
	texture->size = Vector2(512.f, 256.f);
	texture->tint = Color(0.25f, 0.5f, 0.75f, 1.f);
	texture->filter = Filter::Linear;
	texture->flip = true;

	Archive saving = Archive::Save();
	texture->Serialize(saving);
	const std::string yaml = saving.ToString();

	Ref<Texture> restored = registry.Create<Texture>();
	REQUIRE(restored != nullptr);
	Archive loading = Archive::Load(yaml);
	REQUIRE(loading.IsValid());
	restored->Serialize(loading);

	CHECK(restored->path == "res://icon.png");
	CHECK(restored->size.x == doctest::Approx(512.f));
	CHECK(restored->size.y == doctest::Approx(256.f));
	CHECK(restored->tint.g == doctest::Approx(0.5f));
	CHECK(restored->filter == Filter::Linear);
	CHECK(restored->flip);
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("Enum fields are written by name, so renumbering survives") {
	ResourceRegistry registry;
	describeTexture(registry);

	Ref<Texture> texture = registry.Create<Texture>();
	texture->filter = Filter::Linear;

	Archive saving = Archive::Save();
	texture->Serialize(saving);
	CHECK(saving.ToString().find("Linear") != std::string::npos);
}

TEST_CASE("Fields() reports the description in registration order") {
	ResourceRegistry registry;
	describeTexture(registry);

	Ref<Texture> texture = registry.Create<Texture>();
	REQUIRE(texture != nullptr);

	const std::vector<FieldInfo> &fields = texture->Fields();
	REQUIRE(fields.size() == 6);
	CHECK(fields[0].name == "path");
	CHECK(fields[0].type == FieldType::String);
	CHECK(fields[1].type == FieldType::Vector2);
	CHECK(fields[2].type == FieldType::Color);
	CHECK(fields[3].type == FieldType::Enum);
	CHECK(fields[3].enumName == "Filter");
	CHECK(fields[4].name == "flip");

	// Offsets come from the pointer-to-member, so they match the real struct.
	Texture probe;
	const auto base = reinterpret_cast<const unsigned char *>(&probe);
	CHECK(fields[0].offset ==
		  static_cast<size_t>(reinterpret_cast<const unsigned char *>(&probe.path) - base));
	CHECK(fields[4].offset ==
		  static_cast<size_t>(reinterpret_cast<const unsigned char *>(&probe.flip) - base));
}

TEST_CASE("An Expose-only field is shown but never saved") {
	ResourceRegistry registry;
	describeTexture(registry);

	Ref<Texture> texture = registry.Create<Texture>();
	texture->gpuHandle = 42;

	Archive saving = Archive::Save();
	texture->Serialize(saving);
	CHECK(saving.ToString().find("gpuHandle") == std::string::npos);

	// Still reflected, so the inspector can display it.
	const FieldInfo *info = nullptr;
	for (const FieldInfo &field : texture->Fields()) {
		if (field.name == "gpuHandle")
			info = &field;
	}
	REQUIRE(info != nullptr);
	CHECK(info->IsExposed());
	CHECK_FALSE(info->IsSerialized());
}

TEST_CASE("An undescribed member is invisible to serialization") {
	ResourceRegistry registry;
	describeTexture(registry);

	Ref<Texture> texture = registry.Create<Texture>();
	texture->decodedBytes = 9999;

	Archive saving = Archive::Save();
	texture->Serialize(saving);
	CHECK(saving.ToString().find("decodedBytes") == std::string::npos);

	Ref<Texture> restored = registry.Create<Texture>();
	restored->decodedBytes = 7;
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);
	CHECK(restored->decodedBytes == 7);
}

TEST_CASE("A field absent from the document keeps its current value") {
	ResourceRegistry registry;
	describeTexture(registry);

	Ref<Texture> texture = registry.Create<Texture>();
	texture->path = "res://kept.png";
	texture->flip = true;

	Archive loading = Archive::Load("size: [4, 4]\n");
	REQUIRE(loading.IsValid());
	texture->Serialize(loading);

	CHECK(texture->size.x == doctest::Approx(4.f));
	CHECK(texture->path == "res://kept.png");
	CHECK(texture->flip);
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("Duplicate and empty field names are refused") {
	ResourceRegistry registry;
	registry.RegisterType<Clip>("Clip")
		.Field("path", &Clip::path)
		.Field("path", &Clip::volume) // would collide on one archive key
		.Field("", &Clip::volume);

	Ref<Clip> clip = registry.Create<Clip>();
	REQUIRE(clip != nullptr);
	REQUIRE(clip->Fields().size() == 1);
	CHECK(clip->Fields()[0].name == "path");
	CHECK(clip->Fields()[0].type == FieldType::String);
}

TEST_CASE("A failed RegisterType hands back an inert builder") {
	ResourceRegistry registry;
	registry.RegisterType<Clip>("Clip").Field("path", &Clip::path);

	// Same name again: the builder must be inert rather than describing
	// fields onto the type that already owns the name.
	ResourceTypeBuilder<Texture> second = registry.RegisterType<Texture>("Clip");
	CHECK_FALSE(second.IsValid());
	second.Field("path", &Texture::path);

	Ref<Clip> clip = registry.Create<Clip>();
	REQUIRE(clip != nullptr);
	CHECK(clip->Fields().size() == 1);
	CHECK(registry.Create("Texture") == nullptr);
}

TEST_CASE("FieldsOf answers without an instance, for the inspector") {
	ResourceRegistry registry;
	describeTexture(registry);

	CHECK(registry.FieldsOf("Texture").size() == 6);
	CHECK(registry.FieldsOf("Nonexistent").empty());
}

TEST_CASE("A resource built outside the registry has no fields and saves nothing") {
	Texture loose;
	loose.path = "res://loose.png";

	CHECK(loose.Fields().empty());

	Archive saving = Archive::Save();
	loose.Serialize(saving);
	CHECK(saving.ToString().find("loose.png") == std::string::npos);
}

// The field table is held by Ref rather than a raw pointer into the registry
// precisely so this case reads an empty table instead of freed memory.
TEST_CASE("A resource outliving its registry degrades to no fields") {
	Ref<Texture> texture;
	{
		ResourceRegistry registry;
		describeTexture(registry);
		texture = registry.Create<Texture>();
		REQUIRE(texture != nullptr);
		texture->path = "res://icon.png";
		REQUIRE(texture->Fields().size() == 6);
	}

	// Registry gone; the resource is still alive because we hold a Ref.
	CHECK(texture->path == "res://icon.png");
	CHECK(texture->Fields().size() == 6);

	Archive saving = Archive::Save();
	texture->Serialize(saving);
	CHECK(saving.ToString().find("res://icon.png") != std::string::npos);
}

namespace {

// Resource-to-resource references use the same ResourceID field type as sprite textures.
struct Material : public Resource {
	ResourceID texture;
	f32 roughness = 0.f;
};

} // namespace

TEST_CASE("A ResourceRef field round-trips the id verbatim") {
	ResourceRegistry registry;
	describeTexture(registry);
	registry.RegisterType<Material>("Material")
		.Field("texture", &Material::texture)
		.Field("roughness", &Material::roughness);

	Ref<Texture> texture = registry.Create<Texture>();
	Ref<Material> material = registry.Create<Material>();
	REQUIRE(material != nullptr);
	material->texture = texture->Id();
	material->roughness = 0.5f;

	REQUIRE(material->Fields()[0].type == FieldType::ResourceRef);

	Archive saving = Archive::Save();
	material->Serialize(saving);

	Ref<Material> restored = registry.Create<Material>();
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(loading.IsValid());
	restored->Serialize(loading);

	// The id has to survive exactly - 64 random bits, and anything that
	// rounded it would silently repoint the reference at nothing.
	CHECK(restored->texture == texture->Id());
	CHECK(restored->texture.value == texture->Id().value);
	CHECK(registry.Get<Texture>(restored->texture) == texture);
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("A ResourceRef survives an id with every high bit set") {
	ResourceRegistry registry;
	registry.RegisterType<Material>("Material").Field("texture", &Material::texture);

	Ref<Material> material = registry.Create<Material>();
	material->texture = ResourceID{0xFFFF'FFFF'FFFF'FFFFull};

	Archive saving = Archive::Save();
	material->Serialize(saving);

	Ref<Material> restored = registry.Create<Material>();
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);

	CHECK(restored->texture.value == 0xFFFF'FFFF'FFFF'FFFFull);
	CHECK_FALSE(loading.HasErrors());
}

namespace {

// A type whose on-disk form needs real logic rather than a field list.
struct Packed : public Resource {
	i32 width = 0;
	i32 height = 0;

	void Serialize(Archive &archive) override {
		std::string packed;
		if (archive.IsSaving()) {
			packed = std::to_string(width) + "x" + std::to_string(height);
			archive.Field("dimensions", packed);
			return;
		}
		if (archive.Field("dimensions", packed)) {
			const size_t split = packed.find('x');
			width = std::stoi(packed.substr(0, split));
			height = std::stoi(packed.substr(split + 1));
		}
	}
};

} // namespace

TEST_CASE("Overriding Serialize is still allowed for a custom on-disk form") {
	ResourceRegistry registry;
	registry.RegisterType<Packed>("Packed");

	Ref<Packed> packed = registry.Create<Packed>();
	REQUIRE(packed != nullptr);
	packed->width = 320;
	packed->height = 200;

	Archive saving = Archive::Save();
	packed->Serialize(saving);
	CHECK(saving.ToString().find("320x200") != std::string::npos);

	Ref<Packed> restored = registry.Create<Packed>();
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);
	CHECK(restored->width == 320);
	CHECK(restored->height == 200);
}

TEST_CASE("Serialize through the base pointer dispatches to the override") {
	ResourceRegistry registry;
	registry.RegisterType<Packed>("Packed");
	describeTexture(registry);

	registry.Create<Packed>()->width = 64;
	registry.Create<Texture>()->path = "res://a.png";

	// Scene serialization accesses resources through Resource and registered type names.
	std::vector<std::string> written;
	registry.Each([&](ResourceID, Resource &resource, std::string_view) {
		Archive saving = Archive::Save();
		resource.Serialize(saving);
		written.push_back(saving.ToString());
	});

	REQUIRE(written.size() == 2);
	CHECK(written[0].find("dimensions") != std::string::npos);
	CHECK(written[1].find("res://a.png") != std::string::npos);
}
