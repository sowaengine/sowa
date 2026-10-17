#pragma once

#include <string>
#include <unordered_map>

#include "core/types.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "render/graphics_device.hpp"
#include "resource/resource.hpp"
#include "resource/resource_context.hpp"

namespace sowa {

// Rasterized glyph in the font atlas.
struct Glyph {
	// Where the glyph sits in the atlas, in 0..1 texture coordinates.
	Rect uvRect;

	// Size of the glyph's bitmap in pixels.
	Vector2 size{0.f, 0.f};

	// Offset from the cursor to the glyph's top-left corner in Y-up pixels.
	Vector2 bearing{0.f, 0.f};

	// Horizontal cursor advance, including side bearings.
	f32 advance = 0.f;
};

// Rasterized glyph atlas with metrics for shared text layout.
class Font : public Resource {
  public:
	// Empty selects embedded Roboto Regular; otherwise a .ttf/.otf/.ttc virtual path.
	std::string path;

	// Baked glyph height. Use a separate Font resource for each atlas size.
	i32 pixelSize = 32;

	void Reload(const ResourceContext &context) override;
	void Unload(const ResourceContext &context) override;

	// Returns null outside the baked codepoint range.
	const Glyph *FindGlyph(u32 codepoint) const;

	// Ascent is positive and descent is negative in Y-up coordinates.
	// LineHeight includes the font line gap.
	f32 Ascent() const {
		return _ascent;
	}

	f32 Descent() const {
		return _descent;
	}

	f32 LineHeight() const {
		return _lineHeight;
	}

	TextureHandle Atlas() const {
		return _atlas;
	}

	i32 AtlasWidth() const {
		return _atlasWidth;
	}

	i32 AtlasHeight() const {
		return _atlasHeight;
	}

	// Whether the atlas reached the GPU. Reload provides glyph and line metrics
	// even without a graphics device.
	bool IsLoaded() const {
		return _atlas.IsValid();
	}

	// Whether Reload produced glyphs, regardless of the upload.
	bool HasGlyphs() const {
		return !_glyphs.empty();
	}

	// Printable ASCII range baked into the atlas; FindGlyph returns null outside it.
	static constexpr u32 kFirstCodepoint = 32; // space
	static constexpr u32 kLastCodepoint = 126; // '~'

  private:
	std::unordered_map<u32, Glyph> _glyphs;

	TextureHandle _atlas;
	i32 _atlasWidth = 0;
	i32 _atlasHeight = 0;

	f32 _ascent = 0.f;
	f32 _descent = 0.f;
	f32 _lineHeight = 0.f;
};

} // namespace sowa
