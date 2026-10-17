#include "render/text_draw.hpp"

#include "render/renderer2d.hpp"

namespace sowa {

f32 LayoutText(const Font &font, std::string_view text, Vector2 origin,
	std::vector<GlyphQuad> &out) {
	Vector2 pen = origin;
	f32 widest = 0.f;

	for (const char raw : text) {
		if (raw == '\n') {
			widest = pen.x - origin.x > widest ? pen.x - origin.x : widest;
			pen.x = origin.x;
			pen.y -= font.LineHeight();
			continue;
		}

		// Avoid sign extension when converting bytes to codepoints.
		const u32 codepoint = static_cast<u8>(raw);

		const Glyph *glyph = font.FindGlyph(codepoint);
		if (glyph == nullptr) {
			continue;
		}

		// A blank glyph still advances the pen; it just has nothing to draw.
		// Font normalises those to zero size precisely so this is one check.
		if (glyph->size.x > 0.f && glyph->size.y > 0.f) {
			// bearing.y is the top of the glyph above the baseline, so the
			// quad's bottom edge sits size.y below that - which for a
			// descender lands under the baseline, as it should.
			const f32 left = pen.x + glyph->bearing.x;
			const f32 bottom = pen.y + glyph->bearing.y - glyph->size.y;
			out.push_back(
				GlyphQuad{Rect(left, bottom, glyph->size.x, glyph->size.y), glyph->uvRect});
		}

		pen.x += glyph->advance;
	}

	return pen.x - origin.x > widest ? pen.x - origin.x : widest;
}

f32 DrawText(Renderer2D &renderer, const Font &font, std::string_view text, Vector2 origin,
	const Color &color, f32 z) {
	if (!font.IsLoaded()) {
		return 0.f;
	}

	std::vector<GlyphQuad> quads;
	const f32 width = LayoutText(font, text, origin, quads);

	for (const GlyphQuad &quad : quads) {
		renderer.DrawQuad(quad.rect.x, quad.rect.y, quad.rect.w, quad.rect.h, quad.uvRect,
			font.Atlas(), color, z);
	}

	return width;
}

} // namespace sowa
