#include <cmath>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "render/text_draw.hpp"
#include "resource/font.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

// Font metrics and glyph placement can be tested without a graphics context.
struct Fixture {
	FileSystem fs;
	ResourceRegistry registry;
	Ref<Font> font;

	Fixture() {
		fs.MountFolder("res", SOWA_EXAMPLE_DIR);
		registry.SetContext(ResourceContext{nullptr, &fs});
		registry.RegisterType<Font>("Font");

		font = registry.Create<Font>();
		font->pixelSize = 32;
		registry.Reload(font->Id());
	}
};

std::vector<GlyphQuad> layout(const Font &font, std::string_view text,
							  Vector2 origin = Vector2(0.f, 0.f)) {
	std::vector<GlyphQuad> quads;
	LayoutText(font, text, origin, quads);
	return quads;
}

} // namespace

TEST_CASE("An empty string places nothing") {
	Fixture fixture;
	REQUIRE(fixture.font->HasGlyphs());

	std::vector<GlyphQuad> quads;
	CHECK(LayoutText(*fixture.font, "", Vector2(10.f, 20.f), quads) == doctest::Approx(0.f));
	CHECK(quads.empty());
}

TEST_CASE("Glyphs march left to right by their advances") {
	Fixture fixture;
	const std::vector<GlyphQuad> quads = layout(*fixture.font, "ABC");
	REQUIRE(quads.size() == 3);

	CHECK(quads[0].rect.x < quads[1].rect.x);
	CHECK(quads[1].rect.x < quads[2].rect.x);
}

TEST_CASE("The returned width is the sum of the advances") {
	Fixture fixture;

	std::vector<GlyphQuad> quads;
	const f32 width = LayoutText(*fixture.font, "Hello", Vector2(0.f, 0.f), quads);

	f32 expected = 0.f;
	for (const char letter : std::string("Hello"))
		expected += fixture.font->FindGlyph(static_cast<u32>(letter))->advance;

	CHECK(width == doctest::Approx(expected));
	// Width is the pen advance, not the ink extent, so it isn't the same as
	// the last quad's right edge - that's the point of tracking it separately.
	CHECK(width > 0.f);
}

TEST_CASE("Space advances the pen without placing a quad") {
	Fixture fixture;

	const std::vector<GlyphQuad> withSpace = layout(*fixture.font, "A B");
	CHECK(withSpace.size() == 2);

	std::vector<GlyphQuad> ignored;
	const f32 spaced = LayoutText(*fixture.font, "A B", Vector2(0.f, 0.f), ignored);
	ignored.clear();
	const f32 tight = LayoutText(*fixture.font, "AB", Vector2(0.f, 0.f), ignored);
	CHECK(spaced > tight);
}

TEST_CASE("Origin is the baseline, so capitals sit on it and descenders below") {
	Fixture fixture;
	const f32 baseline = 100.f;

	const std::vector<GlyphQuad> caps = layout(*fixture.font, "A", Vector2(0.f, baseline));
	REQUIRE(caps.size() == 1);
	// Bottom of 'A' lands on the baseline, within the packer's sub-pixel pad.
	CHECK(std::abs(caps[0].rect.y - baseline) < 1.f);

	const std::vector<GlyphQuad> tail = layout(*fixture.font, "y", Vector2(0.f, baseline));
	REQUIRE(tail.size() == 1);
	CHECK(tail[0].rect.y < baseline);
}

// Two runs at the same origin must share a baseline whether or not either has
// a descender - which is the whole reason origin isn't a corner.
TEST_CASE("A descender doesn't shift the line it's on") {
	Fixture fixture;

	const std::vector<GlyphQuad> plain = layout(*fixture.font, "no", Vector2(0.f, 50.f));
	const std::vector<GlyphQuad> hanging = layout(*fixture.font, "np", Vector2(0.f, 50.f));
	REQUIRE(plain.size() == 2);
	REQUIRE(hanging.size() == 2);

	// The shared leading 'n' is placed identically in both.
	CHECK(plain[0].rect.y == doctest::Approx(hanging[0].rect.y));
	CHECK(plain[0].rect.x == doctest::Approx(hanging[0].rect.x));
}

TEST_CASE("A newline returns to the origin's x and drops one line") {
	Fixture fixture;
	const Vector2 origin(30.f, 200.f);

	const std::vector<GlyphQuad> quads = layout(*fixture.font, "A\nA", origin);
	REQUIRE(quads.size() == 2);

	CHECK(quads[1].rect.x == doctest::Approx(quads[0].rect.x));
	CHECK(quads[1].rect.y ==
		  doctest::Approx(quads[0].rect.y - fixture.font->LineHeight()));
}

TEST_CASE("A multi-line width is the widest line, not the last") {
	Fixture fixture;

	std::vector<GlyphQuad> quads;
	const f32 width = LayoutText(*fixture.font, "WWWW\nl", Vector2(0.f, 0.f), quads);

	quads.clear();
	const f32 wideLine = LayoutText(*fixture.font, "WWWW", Vector2(0.f, 0.f), quads);
	CHECK(width == doctest::Approx(wideLine));
}

TEST_CASE("A codepoint the font never baked is skipped, advance included") {
	Fixture fixture;

	std::vector<GlyphQuad> quads;
	const f32 withUnknown = LayoutText(*fixture.font, "A\x01Z", Vector2(0.f, 0.f), quads);
	CHECK(quads.size() == 2);

	quads.clear();
	const f32 without = LayoutText(*fixture.font, "AZ", Vector2(0.f, 0.f), quads);
	// A gap, not a wrong-width line - the unknown byte contributes nothing.
	CHECK(withUnknown == doctest::Approx(without));
}

// Verifies non-ASCII input is skipped with the current ASCII-only glyph range.
TEST_CASE("A multi-byte UTF-8 sequence is dropped rather than mangled") {
	Fixture fixture;

	std::vector<GlyphQuad> quads;
	const std::string highByte("\xC3\xA9"); // UTF-8 e-acute
	CHECK(LayoutText(*fixture.font, highByte, Vector2(0.f, 0.f), quads) ==
		  doctest::Approx(0.f));
	CHECK(quads.empty());
}

TEST_CASE("Every placed quad samples a sub-rect of the atlas") {
	Fixture fixture;
	const std::vector<GlyphQuad> quads = layout(*fixture.font, "Sowa Engine");
	REQUIRE(!quads.empty());

	for (const GlyphQuad &quad : quads) {
		CHECK(quad.uvRect.w > 0.f);
		CHECK(quad.uvRect.h > 0.f);
		// A full-texture rect would mean the whole atlas drawn per glyph.
		CHECK(quad.uvRect.w < 1.f);
		CHECK(quad.uvRect.h < 1.f);
		CHECK(quad.uvRect.x + quad.uvRect.w <= doctest::Approx(1.f));
		CHECK(quad.uvRect.y + quad.uvRect.h <= doctest::Approx(1.f));
	}
}

TEST_CASE("Layout scales with the font's pixel size") {
	Fixture fixture;
	std::vector<GlyphQuad> quads;
	const f32 small = LayoutText(*fixture.font, "Sowa", Vector2(0.f, 0.f), quads);

	fixture.font->pixelSize = 64;
	fixture.registry.Reload(fixture.font->Id());

	quads.clear();
	const f32 large = LayoutText(*fixture.font, "Sowa", Vector2(0.f, 0.f), quads);
	CHECK(large > small * 1.8f);
}

TEST_CASE("A font that never loaded lays out nothing rather than crashing") {
	Fixture fixture;
	fixture.font->path = "res://nonexistent.ttf";
	fixture.registry.Reload(fixture.font->Id());
	REQUIRE_FALSE(fixture.font->HasGlyphs());

	std::vector<GlyphQuad> quads;
	CHECK(LayoutText(*fixture.font, "Sowa", Vector2(0.f, 0.f), quads) ==
		  doctest::Approx(0.f));
	CHECK(quads.empty());
}
