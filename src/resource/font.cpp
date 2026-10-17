#include "resource/font.hpp"

#include <cstring>
#include <vector>

#include "assets/embedded.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace sowa {

namespace {

constexpr i32 kMinAtlasSize = 256;
constexpr i32 kMaxAtlasSize = 4096;

// Oversampling improves small glyph edges at the cost of atlas area.
constexpr u32 kOversample = 2;

// Blank glyphs can have non-empty packed rectangles; inspect coverage before drawing.
bool HasInk(const std::vector<u8> &coverage, i32 atlasSize, const stbtt_packedchar &entry) {
	for (i32 y = entry.y0; y < entry.y1; ++y) {
		const u8 *row = &coverage[static_cast<size_t>(y) * atlasSize];
		for (i32 x = entry.x0; x < entry.x1; ++x) {
			if (row[x] != 0) {
				return true;
			}
		}
	}
	return false;
}

} // namespace

void Font::Reload(const ResourceContext &context) {
	Unload(context);

	if (pixelSize <= 0) {
		Log::Error("Font: {} has a pixel size of {}, which can't be rasterized", path, pixelSize);
		return;
	}

	Ref<FileData> file;
	auto bytes = assets::DefaultFont();
	if (!path.empty()) {
		if (context.fs == nullptr) {
			Log::Error("Font: can't load {} without a filesystem", path);
			return;
		}
		const auto loaded = context.fs->Load(path);
		if (!loaded) {
			Log::Error("Font: failed to read {} (error {})", path, static_cast<int>(loaded.error));
			return;
		}
		if (!loaded.value || loaded.value->Size() == 0) {
			Log::Error("Font: {} is empty", path);
			return;
		}
		file = loaded.value;
		bytes = {reinterpret_cast<const unsigned char *>(file->Data()), file->Size()};
	}

	const auto *fontData = bytes.data();

	stbtt_fontinfo info;
	const int offset = stbtt_GetFontOffsetForIndex(fontData, 0);
	if (offset < 0 || !stbtt_InitFont(&info, fontData, offset)) {
		Log::Error("Font: unsupported or invalid font {}", path);
		return;
	}

	const f32 scale = stbtt_ScaleForPixelHeight(&info, static_cast<f32>(pixelSize));

	const u32 glyphCount = kLastCodepoint - kFirstCodepoint + 1;
	std::vector<stbtt_packedchar> packed(glyphCount);
	std::vector<u8> coverage;

	// Grows until the glyphs fit. PackFontRanges returns 0 when it runs out of
	// room, which is the only way to know a size was too small.
	i32 atlasSize = kMinAtlasSize;
	bool fitted = false;
	for (; atlasSize <= kMaxAtlasSize; atlasSize *= 2) {
		coverage.assign(static_cast<size_t>(atlasSize) * atlasSize, 0);

		stbtt_pack_context packContext;
		if (stbtt_PackBegin(&packContext, coverage.data(), atlasSize, atlasSize, 0, 1, nullptr) ==
			0) {
			continue;
		}

		stbtt_PackSetOversampling(&packContext, kOversample, kOversample);

		stbtt_pack_range range{};
		range.font_size = static_cast<f32>(pixelSize);
		range.first_unicode_codepoint_in_range = static_cast<int>(kFirstCodepoint);
		range.num_chars = static_cast<int>(glyphCount);
		range.chardata_for_range = packed.data();

		const int ok = stbtt_PackFontRanges(&packContext, fontData, 0, &range, 1);
		stbtt_PackEnd(&packContext);

		if (ok != 0) {
			fitted = true;
			break;
		}
	}

	if (!fitted) {
		Log::Error("Font: {} at {}px doesn't fit in a {}x{} atlas", path, pixelSize, kMaxAtlasSize,
			kMaxAtlasSize);
		return;
	}

	// RGBA8 atlas: white pixels with alpha coverage, flipped for Y-up rendering.
	std::vector<u8> rgba(static_cast<size_t>(atlasSize) * atlasSize * 4);
	for (i32 y = 0; y < atlasSize; ++y) {
		const u8 *source = &coverage[static_cast<size_t>(atlasSize - 1 - y) * atlasSize];
		u8 *target = &rgba[static_cast<size_t>(y) * atlasSize * 4];
		for (i32 x = 0; x < atlasSize; ++x) {
			target[x * 4 + 0] = 255;
			target[x * 4 + 1] = 255;
			target[x * 4 + 2] = 255;
			target[x * 4 + 3] = source[x];
		}
	}

	// Upload only with a graphics device. Headless loads still provide glyph metrics and
	// atlas layout, but IsLoaded() remains false.
	TextureHandle atlas;
	if (context.device != nullptr) {
		atlas = context.device->CreateTexture(atlasSize, atlasSize, rgba.data());
		if (!atlas.IsValid()) {
			Log::Error("Font: GPU upload failed for {}", path);
			return;
		}
	}

	const f32 atlasSizeF = static_cast<f32>(atlasSize);
	for (u32 index = 0; index < glyphCount; ++index) {
		const stbtt_packedchar &entry = packed[index];

		Glyph glyph;
		glyph.advance = entry.xadvance;

		// Zero size identifies glyphs without ink.
		if (!HasInk(coverage, atlasSize, entry)) {
			_glyphs.emplace(kFirstCodepoint + index, glyph);
			continue;
		}

		// Use display-space offsets for glyph size; atlas rectangles include oversampling.
		glyph.size = Vector2(entry.xoff2 - entry.xoff, entry.yoff2 - entry.yoff);
		// stb works y-down, so yoff is negative for the part of a glyph above
		// the baseline. Negating puts the bearing in the engine's Y-up space.
		glyph.bearing = Vector2(entry.xoff, -entry.yoff);
		// v measured from the bottom, matching the flip applied above.
		glyph.uvRect = Rect(static_cast<f32>(entry.x0) / atlasSizeF,
			static_cast<f32>(atlasSize - entry.y1) / atlasSizeF,
			static_cast<f32>(entry.x1 - entry.x0) / atlasSizeF,
			static_cast<f32>(entry.y1 - entry.y0) / atlasSizeF);

		_glyphs.emplace(kFirstCodepoint + index, glyph);
	}

	int ascent = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

	_atlas = atlas;
	_atlasWidth = atlasSize;
	_atlasHeight = atlasSize;
	_ascent = static_cast<f32>(ascent) * scale;
	_descent = static_cast<f32>(descent) * scale;
	_lineHeight = static_cast<f32>(ascent - descent + lineGap) * scale;
}

void Font::Unload(const ResourceContext &context) {
	// A null device means nothing was ever uploaded through one, so dropping
	// the handle is all that's left to do.
	if (_atlas.IsValid() && context.device != nullptr) {
		context.device->DestroyTexture(_atlas);
	}

	_atlas = {};
	_atlasWidth = 0;
	_atlasHeight = 0;
	_ascent = 0.f;
	_descent = 0.f;
	_lineHeight = 0.f;
	_glyphs.clear();
}

const Glyph *Font::FindGlyph(u32 codepoint) const {
	const auto it = _glyphs.find(codepoint);
	return it != _glyphs.end() ? &it->second : nullptr;
}

} // namespace sowa
