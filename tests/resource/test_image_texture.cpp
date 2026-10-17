#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/reflection/field_type.hpp"
#include "core/serialize/archive.hpp"
#include "core/filesystem/file_system.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

// The registry's context carries a GraphicsDevice*, which can't be faked
// without a GL context - so these tests drive Adopt/Unload with a null device
// and count through the registry's observable behaviour instead.
struct Fixture {
	ResourceRegistry registry;

	Fixture() {
		registry.RegisterType<ImageTexture>("ImageTexture")
			.Field("path", &ImageTexture::path);
	}

	Ref<ImageTexture> Adopted(u32 handleId, i32 width = 32, i32 height = 16) {
		Ref<ImageTexture> texture = registry.Create<ImageTexture>();
		texture->Adopt(registry.Context(), TextureHandle{handleId, 1}, width, height);
		return texture;
	}
};

} // namespace

TEST_CASE("Adopt takes an already-uploaded texture") {
	Fixture fixture;
	Ref<ImageTexture> texture = fixture.Adopted(7, 64, 32);

	CHECK(texture->IsLoaded());
	CHECK(texture->Handle().id == 7u);
	CHECK(texture->Width() == 64);
	CHECK(texture->Height() == 32);
	CHECK(texture->Size().x == doctest::Approx(64.f));
	CHECK(texture->Size().y == doctest::Approx(32.f));
}

TEST_CASE("Reload makes the object match its fields, dropping what was there") {
	Fixture fixture;
	Ref<ImageTexture> texture = fixture.Adopted(7);
	REQUIRE(texture->IsLoaded());

	// Path is empty, so "match the fields" means hold nothing. An adopted
	// texture is not exempt - the fields are the source of truth.
	CHECK(fixture.registry.Reload(texture->Id()));
	CHECK_FALSE(texture->IsLoaded());
	CHECK(texture->Width() == 0);
}

TEST_CASE("Reload without a filesystem or device fails without loading") {
	Fixture fixture;
	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "res://icon.png";

	// The context is empty here, so this exercises the guard rather than the
	// decode. It must not crash and must not claim to have loaded.
	CHECK(fixture.registry.Reload(texture->Id()));
	CHECK_FALSE(texture->IsLoaded());
}

TEST_CASE("Reloading an unknown id reports the miss") {
	Fixture fixture;
	CHECK_FALSE(fixture.registry.Reload(ResourceID{4242}));
}

TEST_CASE("Removing a resource unloads it while the context is still alive") {
	Fixture fixture;
	Ref<ImageTexture> texture = fixture.Adopted(7);
	const ResourceID id = texture->Id();
	REQUIRE(texture->IsLoaded());

	CHECK(fixture.registry.Remove(id));

	// The Ref we still hold keeps the object alive, but its GPU-side state is
	// gone - released by the registry, which had the context, rather than by
	// ~ImageTexture, which would not have.
	CHECK_FALSE(texture->IsLoaded());
	CHECK(texture.use_count() == 1);
}

TEST_CASE("Clear unloads everything it drops") {
	Fixture fixture;
	Ref<ImageTexture> first = fixture.Adopted(1);
	Ref<ImageTexture> second = fixture.Adopted(2);

	fixture.registry.Clear();

	CHECK_FALSE(first->IsLoaded());
	CHECK_FALSE(second->IsLoaded());
}

// The registry unloads textures while the graphics device lives, even if a Ref outlives the
// registry.
TEST_CASE("A resource outliving its registry was already unloaded") {
	Ref<ImageTexture> texture;
	{
		Fixture fixture;
		texture = fixture.Adopted(7);
		REQUIRE(texture->IsLoaded());
	}

	CHECK(texture != nullptr);
	CHECK_FALSE(texture->IsLoaded());
}

TEST_CASE("path is the only described field") {
	Fixture fixture;
	Ref<ImageTexture> texture = fixture.Adopted(7);

	REQUIRE(texture->Fields().size() == 1);
	CHECK(texture->Fields()[0].name == "path");
	CHECK(texture->Fields()[0].type == FieldType::String);

	// The handle and dimensions are derived - same authored-vs-derived split
	// as Transform2D::worldMatrix, so none of it reaches the document.
	Archive saving = Archive::Save();
	texture->path = "res://icon.png";
	texture->Serialize(saving);
	const std::string yaml = saving.ToString();
	CHECK(yaml.find("res://icon.png") != std::string::npos);
	CHECK(yaml.find("handle") == std::string::npos);
	CHECK(yaml.find("width") == std::string::npos);
}

TEST_CASE("A scene file records the image by name, not its pixels") {
	Fixture fixture;
	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "res://icon.png";
	const ResourceID id = texture->Id();

	Archive saving = Archive::Save();
	texture->Serialize(saving);

	// Restore under the original ID to preserve references.
	fixture.registry.Remove(id);
	Ref<Resource> restored = fixture.registry.Create("ImageTexture", id);
	REQUIRE(restored != nullptr);
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);

	Ref<ImageTexture> typed = fixture.registry.Get<ImageTexture>(id);
	REQUIRE(typed != nullptr);
	CHECK(typed->path == "res://icon.png");
	CHECK(typed->Id() == id);
	// Deserialization sets fields; Reload is what acts on them.
	CHECK_FALSE(typed->IsLoaded());
}

// Image loading options.

namespace {

// A TGA is an 18-byte header plus raw pixels, so a test can build one with
// chosen colours and assert exactly which row comes back first. Same trick as
// the hand-built WAVs in the audio tests, and the only way to check the flip
// without a reference image to eyeball.
std::vector<u8> makeTga(i32 width, i32 height, const std::vector<u8> &rgbaTopFirst) {
	std::vector<u8> tga(18, 0);
	tga[2] = 2;  // uncompressed true-colour
	tga[12] = static_cast<u8>(width & 0xFF);
	tga[13] = static_cast<u8>((width >> 8) & 0xFF);
	tga[14] = static_cast<u8>(height & 0xFF);
	tga[15] = static_cast<u8>((height >> 8) & 0xFF);
	tga[16] = 32;    // bits per pixel
	tga[17] = 0x28;  // top-left origin + 8 alpha bits

	// TGA stores 32-bit pixels as BGRA.
	for (size_t i = 0; i < rgbaTopFirst.size(); i += 4) {
		tga.push_back(rgbaTopFirst[i + 2]);
		tga.push_back(rgbaTopFirst[i + 1]);
		tga.push_back(rgbaTopFirst[i + 0]);
		tga.push_back(rgbaTopFirst[i + 3]);
	}
	return tga;
}

// One pixel wide, two tall: red on top, blue underneath.
std::vector<u8> redOverBlue() {
	return {255, 0, 0, 255, 0, 0, 255, 255};
}

struct FileFixture {
	std::filesystem::path dir;
	FileSystem fs;
	StubGraphicsDevice device;
	ResourceRegistry registry;

	explicit FileFixture(bool withDevice = false) {
		dir = std::filesystem::temp_directory_path() / "sowa_image_texture_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);

		registry.SetContext(
			ResourceContext{withDevice ? &device : nullptr, &fs});
		registry.RegisterType<ImageTexture>("ImageTexture")
			.Field("path", &ImageTexture::path)
			.Field("flipVertically", &ImageTexture::flipVertically)
			.Field("retainPixels", &ImageTexture::retainPixels)
			.Field("filter", &ImageTexture::filter);
	}

	~FileFixture() { std::filesystem::remove_all(dir); }

	void Write(const std::string &name, const std::vector<u8> &bytes) {
		std::ofstream out(dir / name, std::ios::binary);
		out.write(reinterpret_cast<const char *>(bytes.data()),
				  static_cast<std::streamsize>(bytes.size()));
	}
};

} // namespace

TEST_CASE("flipVertically false keeps the image's own top row first") {
	FileFixture fixture;
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "tmp://pair.tga";
	texture->flipVertically = false;
	texture->retainPixels = true;
	fixture.registry.Reload(texture->Id());

	REQUIRE(texture->Pixels().size() == 8);
	// Red, the top row of the source.
	CHECK(texture->Pixels()[0] == 255);
	CHECK(texture->Pixels()[2] == 0);
}

TEST_CASE("flipVertically true puts the bottom row first, for Y-up") {
	FileFixture fixture;
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "tmp://pair.tga";
	texture->retainPixels = true;
	REQUIRE(texture->flipVertically); // the default
	fixture.registry.Reload(texture->Id());

	REQUIRE(texture->Pixels().size() == 8);
	// Blue, the bottom row of the source, now first.
	CHECK(texture->Pixels()[0] == 0);
	CHECK(texture->Pixels()[2] == 255);
}

TEST_CASE("retainPixels is what makes the CPU copy exist at all") {
	FileFixture fixture;
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> dropped = fixture.registry.Create<ImageTexture>();
	dropped->path = "tmp://pair.tga";
	fixture.registry.Reload(dropped->Id());
	REQUIRE_FALSE(dropped->retainPixels); // the default
	CHECK(dropped->Pixels().empty());
	CHECK(dropped->Width() == 1);
	CHECK(dropped->Height() == 2);

	Ref<ImageTexture> kept = fixture.registry.Create<ImageTexture>();
	kept->path = "tmp://pair.tga";
	kept->retainPixels = true;
	fixture.registry.Reload(kept->Id());
	CHECK(kept->Pixels().size() == 1 * 2 * 4);
}

TEST_CASE("Unloading releases the retained pixels too") {
	FileFixture fixture;
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "tmp://pair.tga";
	texture->retainPixels = true;
	fixture.registry.Reload(texture->Id());
	REQUIRE(!texture->Pixels().empty());

	fixture.registry.Remove(texture->Id());
	CHECK(texture->Pixels().empty());
}

TEST_CASE("Options take effect on the next Reload, not retroactively") {
	FileFixture fixture;
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "tmp://pair.tga";
	texture->retainPixels = true;
	fixture.registry.Reload(texture->Id());
	CHECK(texture->Pixels()[0] == 0); // flipped: blue first

	texture->flipVertically = false;
	// Not reloaded yet, so the old decode still stands.
	CHECK(texture->Pixels()[0] == 0);

	fixture.registry.Reload(texture->Id());
	CHECK(texture->Pixels()[0] == 255); // now red first
}

TEST_CASE("The filter reaches the device rather than stopping at the field") {
	FileFixture fixture(true);
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "tmp://pair.tga";
	REQUIRE(texture->filter == TextureFilter::Linear); // the default
	fixture.registry.Reload(texture->Id());

	CHECK(fixture.device.textureCreateCount == 1);
	CHECK(fixture.device.lastTextureFilter == TextureFilter::Linear);
	CHECK(fixture.device.lastTextureWidth == 1);
	CHECK(fixture.device.lastTextureHeight == 2);
	CHECK(texture->IsLoaded());

	texture->filter = TextureFilter::Nearest;
	fixture.registry.Reload(texture->Id());
	CHECK(fixture.device.lastTextureFilter == TextureFilter::Nearest);
}

TEST_CASE("A reload destroys the texture it replaces") {
	FileFixture fixture(true);
	fixture.Write("pair.tga", makeTga(1, 2, redOverBlue()));

	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "tmp://pair.tga";
	fixture.registry.Reload(texture->Id());
	REQUIRE(fixture.device.textureCreateCount == 1);
	CHECK(fixture.device.textureDestroyCount == 0);

	fixture.registry.Reload(texture->Id());
	CHECK(fixture.device.textureCreateCount == 2);
	// The first upload is released rather than leaked on the GPU.
	CHECK(fixture.device.textureDestroyCount == 1);

	fixture.registry.Remove(texture->Id());
	CHECK(fixture.device.textureDestroyCount == 2);
}

TEST_CASE("Every load option is described and survives a round trip") {
	FileFixture fixture;
	Ref<ImageTexture> texture = fixture.registry.Create<ImageTexture>();
	texture->path = "res://icon.png";
	texture->flipVertically = false;
	texture->retainPixels = true;
	texture->filter = TextureFilter::Nearest;

	REQUIRE(texture->Fields().size() == 4);

	Archive saving = Archive::Save();
	texture->Serialize(saving);
	const std::string yaml = saving.ToString();
	// The filter is written by name, so renumbering the enum can't silently
	// repoint an existing file at the wrong mode.
	CHECK(yaml.find("Nearest") != std::string::npos);

	Ref<ImageTexture> restored = fixture.registry.Create<ImageTexture>();
	Archive loading = Archive::Load(yaml);
	REQUIRE(loading.IsValid());
	restored->Serialize(loading);
	CHECK_FALSE(loading.HasErrors());

	CHECK(restored->path == "res://icon.png");
	CHECK_FALSE(restored->flipVertically);
	CHECK(restored->retainPixels);
	CHECK(restored->filter == TextureFilter::Nearest);
}
