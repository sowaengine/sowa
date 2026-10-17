#pragma once

#include <string_view>
#include <vector>

#include "core/types.hpp"
#include "math/color.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "render/text_layout.hpp"
#include "resource/font.hpp"

namespace sowa {

class Renderer2D;

// Appends glyph placements from a baseline and returns widest-line advance. Newlines
// move down LineHeight; missing glyphs contribute no advance. LayoutTextBlock handles
// wrapping, alignment, and sizing. No kerning or shaping.
f32 LayoutText(const Font &font, std::string_view text, Vector2 origin,
	std::vector<GlyphQuad> &out);

// LayoutText plus submission. Does not bracket Begin/End - the caller owns the
// batch, so text can share one with whatever else it's drawn beside.
f32 DrawText(Renderer2D &renderer, const Font &font, std::string_view text, Vector2 origin,
	const Color &color = Color{}, f32 z = 0.f);

} // namespace sowa
