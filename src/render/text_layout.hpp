#pragma once

#include <string_view>
#include <vector>

#include "core/reflection/field_type.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"

namespace sowa {

class Font;

enum class TextHorizontalAlignment {
	Left,
	Center,
	Right
};
SOWA_REFLECT_ENUM(TextHorizontalAlignment, {"Left", TextHorizontalAlignment::Left},
	{"Center", TextHorizontalAlignment::Center}, {"Right", TextHorizontalAlignment::Right});

enum class TextVerticalAlignment {
	Baseline,
	Top,
	Center,
	Bottom
};
SOWA_REFLECT_ENUM(TextVerticalAlignment, {"Baseline", TextVerticalAlignment::Baseline},
	{"Top", TextVerticalAlignment::Top}, {"Center", TextVerticalAlignment::Center},
	{"Bottom", TextVerticalAlignment::Bottom});

struct GlyphQuad {
	Rect rect;
	Rect uvRect;
};

struct TextLayoutOptions {
	// Zero uses the font's baked height. Positive sizes scale its bitmap.
	f32 fontSize = 0.f;
	// Zero disables wrapping. Positive widths wrap words, splitting long words.
	f32 wrapWidth = 0.f;
	f32 lineSpacing = 1.f;
	TextHorizontalAlignment horizontalAlignment = TextHorizontalAlignment::Left;
	TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Baseline;
	Vector2 offset{0.f, 0.f};
};

struct TextLayout {
	std::vector<GlyphQuad> quads;
	// Logical advance/line-metric bounds, not the glyphs' ink bounds.
	Rect bounds;
	u32 lineCount = 0;
};

// Each line aligns around offset.x; vertical alignment anchors the whole block
// at offset.y. Y-up, with the first line at the top. CRLF is one newline and
// tabs expand to four spaces. Uses the Font's currently baked glyph range.
TextLayout LayoutTextBlock(const Font &font, std::string_view text,
	const TextLayoutOptions &options = {});

} // namespace sowa
