#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/reflection/field_type.hpp"
#include "core/serialize/archive.hpp"
#include "resource/font.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

// Font loading skips GPU upload without a graphics device.
struct Fixture {
	FileSystem fs;
	ResourceRegistry registry;

	Fixture() {
		fs.MountFolder("res", SOWA_EXAMPLE_DIR);
		fs.MountFolder("fonts", std::filesystem::path(SOWA_ASSETS_DIR) / "fonts");
		registry.SetContext(ResourceContext{nullptr, &fs});
		registry.RegisterType<Font>("Font")
			.Field("path", &Font::path)
			.Field("pixelSize", &Font::pixelSize);
	}

	Ref<Font> Load(const char *file = "", i32 pixelSize = 32) {
		Ref<Font> font = registry.Create<Font>();
		font->path = file;
		font->pixelSize = pixelSize;
		registry.Reload(font->Id());
		return font;
	}
};

} // namespace

TEST_CASE("A real font rasterizes every glyph in the baked range") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();
	REQUIRE(font != nullptr);
	REQUIRE(font->HasGlyphs());

	for (u32 codepoint = Font::kFirstCodepoint; codepoint <= Font::kLastCodepoint; ++codepoint)
		CHECK(font->FindGlyph(codepoint) != nullptr);

	// Nothing outside it, so a caller decides what a missing glyph looks like.
	CHECK(font->FindGlyph(Font::kFirstCodepoint - 1) == nullptr);
	CHECK(font->FindGlyph(Font::kLastCodepoint + 1) == nullptr);
	CHECK(font->FindGlyph(0x00E9) == nullptr); // e-acute
}

TEST_CASE("Without a device the atlas is laid out but not uploaded") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	CHECK(font->HasGlyphs());
	CHECK_FALSE(font->IsLoaded());
	CHECK_FALSE(font->Atlas().IsValid());

	// The layout still happened, which is what the uv rects are relative to.
	CHECK(font->AtlasWidth() > 0);
	CHECK(font->AtlasHeight() == font->AtlasWidth());
}

TEST_CASE("Advances reflect the actual glyph widths") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	const Glyph *narrow = font->FindGlyph('i');
	const Glyph *wide = font->FindGlyph('m');
	REQUIRE(narrow != nullptr);
	REQUIRE(wide != nullptr);

	CHECK(narrow->advance > 0.f);
	CHECK(narrow->advance < wide->advance);
}

TEST_CASE("Space has an advance but no bitmap") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	const Glyph *space = font->FindGlyph(' ');
	REQUIRE(space != nullptr);
	CHECK(space->advance > 0.f);
	CHECK(space->size.x == doctest::Approx(0.f));
	CHECK(space->size.y == doctest::Approx(0.f));
}

// The y-flip is the easiest thing here to get backwards, and the symptom would
// be upside-down text that no other assertion notices.
TEST_CASE("Bearings are in Y-up space, so caps sit above the baseline") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	const Glyph *cap = font->FindGlyph('A');
	REQUIRE(cap != nullptr);
	// Top of 'A' is above the baseline.
	CHECK(cap->bearing.y > 0.f);
	// ...and it doesn't descend below it: bearing.y is the top, so the bottom
	// is bearing.y - size.y, which for a capital should be about zero.
	// Within a pixel: oversampling pads the packed box very slightly.
	CHECK(std::abs(cap->bearing.y - cap->size.y) < 1.f);

	// A descender does go below.
	const Glyph *descender = font->FindGlyph('y');
	REQUIRE(descender != nullptr);
	CHECK(descender->bearing.y - descender->size.y < 0.f);
}

TEST_CASE("Vertical metrics follow the Y-up sign convention") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	CHECK(font->Ascent() > 0.f);
	CHECK(font->Descent() < 0.f);
	// Baseline to baseline covers at least the full glyph height.
	CHECK(font->LineHeight() >= font->Ascent() - font->Descent());
}

TEST_CASE("Metrics scale with the baked pixel size") {
	Fixture fixture;
	Ref<Font> small = fixture.Load("", 16);
	Ref<Font> large = fixture.Load("", 64);

	CHECK(large->Ascent() > small->Ascent() * 3.f);
	CHECK(large->FindGlyph('m')->advance > small->FindGlyph('m')->advance * 3.f);
}

TEST_CASE("Glyph uv rects stay inside the atlas and don't overlap") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	struct Box {
		f32 x0, y0, x1, y1;
	};
	std::vector<Box> boxes;

	for (u32 codepoint = Font::kFirstCodepoint; codepoint <= Font::kLastCodepoint; ++codepoint) {
		const Glyph *glyph = font->FindGlyph(codepoint);
		REQUIRE(glyph != nullptr);

		const Rect &uv = glyph->uvRect;
		CHECK(uv.x >= 0.f);
		CHECK(uv.y >= 0.f);
		CHECK(uv.x + uv.w <= doctest::Approx(1.f));
		CHECK(uv.y + uv.h <= doctest::Approx(1.f));

		if (uv.w > 0.f && uv.h > 0.f)
			boxes.push_back(Box{uv.x, uv.y, uv.x + uv.w, uv.y + uv.h});
	}

	// A packer handing two glyphs the same region would show one letter's ink
	// inside another, which no per-glyph assertion above would notice.
	for (size_t i = 0; i < boxes.size(); ++i) {
		for (size_t j = i + 1; j < boxes.size(); ++j) {
			const bool disjoint = boxes[i].x1 <= boxes[j].x0 || boxes[j].x1 <= boxes[i].x0 ||
								  boxes[i].y1 <= boxes[j].y0 || boxes[j].y1 <= boxes[i].y0;
			CHECK(disjoint);
		}
	}
}

// Oversampling changes atlas size but must preserve glyph aspect ratio.
TEST_CASE("uv rects have the same aspect ratio as the glyphs they hold") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();

	for (const char letter : std::string("WlgQ.")) {
		const Glyph *glyph = font->FindGlyph(static_cast<u32>(letter));
		REQUIRE(glyph != nullptr);
		REQUIRE(glyph->size.x > 0.f);

		const f32 uvAspect = (glyph->uvRect.w * static_cast<f32>(font->AtlasWidth())) /
							 (glyph->uvRect.h * static_cast<f32>(font->AtlasHeight()));
		CHECK(uvAspect == doctest::Approx(glyph->size.x / glyph->size.y).epsilon(0.05));
	}
}

TEST_CASE("A missing file leaves the font empty instead of half-built") {
	Fixture fixture;
	Ref<Font> font = fixture.Load("res://nonexistent.ttf");

	CHECK_FALSE(font->HasGlyphs());
	CHECK_FALSE(font->IsLoaded());
	CHECK(font->Ascent() == doctest::Approx(0.f));
	CHECK(font->FindGlyph('A') == nullptr);
}

TEST_CASE("A file that isn't a font is rejected") {
	Fixture fixture;
	Ref<Font> font = fixture.Load("res://icon.png");

	CHECK_FALSE(font->HasGlyphs());
	CHECK(font->FindGlyph('A') == nullptr);
}

TEST_CASE("A nonsense pixel size is refused rather than rasterized") {
	Fixture fixture;
	Ref<Font> font = fixture.Load("", 0);
	CHECK_FALSE(font->HasGlyphs());

	Ref<Font> negative = fixture.Load("", -8);
	CHECK_FALSE(negative->HasGlyphs());
}

TEST_CASE("Reload drops the previous rasterization") {
	Fixture fixture;
	Ref<Font> font = fixture.Load();
	REQUIRE(font->HasGlyphs());
	const f32 originalAscent = font->Ascent();

	font->path = "res://nonexistent.ttf";
	fixture.registry.Reload(font->Id());

	// Matching the fields means holding nothing, not keeping stale glyphs.
	CHECK_FALSE(font->HasGlyphs());
	CHECK(font->Ascent() == doctest::Approx(0.f));
	CHECK(originalAscent > 0.f);
}

TEST_CASE("Only path and pixelSize are described") {
	Fixture fixture;
	Ref<Font> font = fixture.Load("fonts://Roboto-Regular.ttf");

	REQUIRE(font->Fields().size() == 2);
	CHECK(font->Fields()[0].name == "path");
	CHECK(font->Fields()[1].name == "pixelSize");

	Archive saving = Archive::Save();
	font->Serialize(saving);
	const std::string yaml = saving.ToString();
	CHECK(yaml.find("Roboto-Regular.ttf") != std::string::npos);
	CHECK(yaml.find("32") != std::string::npos);
	// The atlas and the glyph table are derived; none of it belongs on disk.
	CHECK(yaml.find("atlas") == std::string::npos);
	CHECK(yaml.find("advance") == std::string::npos);
}

TEST_CASE("A font round-trips through an archive") {
	Fixture fixture;
	const char *path = "";
	SUBCASE("Embedded default font") {}
	SUBCASE("Custom font file") {
		path = "fonts://Roboto-Regular.ttf";
	}
	Ref<Font> font = fixture.Load(path, 48);
	const ResourceID id = font->Id();

	Archive saving = Archive::Save();
	font->Serialize(saving);

	fixture.registry.Remove(id);
	Ref<Resource> restored = fixture.registry.Create("Font", id);
	REQUIRE(restored != nullptr);
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);

	Ref<Font> typed = fixture.registry.Get<Font>(id);
	REQUIRE(typed != nullptr);
	CHECK(typed->path == path);
	CHECK(typed->pixelSize == 48);
	// Deserialization sets fields; Reload is what acts on them.
	CHECK_FALSE(typed->HasGlyphs());

	fixture.registry.Reload(id);
	CHECK(typed->HasGlyphs());
}
