#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"
#include "math/vector2.hpp"
#include "render/graphics_device.hpp"
#include "resource/resource.hpp"
#include "resource/resource_context.hpp"

namespace sowa {

// Image decoded from a file and optionally uploaded to the GPU. Reflection serializes
// the path and loading options; dimensions, pixels, and the GPU handle are derived.
class ImageTexture : public Resource {
  public:
	// Virtual filesystem path, such as "res://icon.png". Path and loading options take
	// effect on the next ResourceRegistry::Reload.
	std::string path;

	// Flip rows while decoding for Y-up rendering. Disable for top-row-first consumers
	// such as window icons and cursors.
	bool flipVertically = true;

	// Retain decoded RGBA for CPU access. Disable vertical flipping for top-row-first consumers.
	bool retainPixels = false;

	// How the texture is sampled when it isn't drawn one texel per pixel.
	// Linear by default (see TextureFilter); Nearest is what pixel art wants,
	// and is the reason this is per-texture rather than a global.
	TextureFilter filter = TextureFilter::Linear;

	void Reload(const ResourceContext &context) override;
	void Unload(const ResourceContext &context) override;

	// Owns an uploaded texture; Unload frees it. Leaves path unchanged, so Reload replaces
	// it using the current loading fields.
	void Adopt(const ResourceContext &context, TextureHandle handle, i32 width, i32 height);

	// Decoded RGBA, top row first if flipVertically was false. Empty unless
	// retainPixels was set at load time - the flag is what makes this cost
	// anything, so it's opt-in.
	const std::vector<u8> &Pixels() const {
		return _pixels;
	}

	TextureHandle Handle() const {
		return _handle;
	}

	i32 Width() const {
		return _width;
	}

	i32 Height() const {
		return _height;
	}

	Vector2 Size() const {
		return Vector2(static_cast<f32>(_width), static_cast<f32>(_height));
	}

	bool IsLoaded() const {
		return _handle.IsValid();
	}

  private:
	std::vector<u8> _pixels;
	TextureHandle _handle;
	i32 _width = 0;
	i32 _height = 0;
};

} // namespace sowa
