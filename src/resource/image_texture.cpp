#include "resource/image_texture.hpp"

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace sowa {

void ImageTexture::Reload(const ResourceContext &context) {
	// Whatever was here goes first, so Reload is idempotent and a path change
	// can't leak the previous upload.
	Unload(context);

	if (path.empty()) {
		return;
	}

	if (context.fs == nullptr) {
		Log::Error("ImageTexture: can't load {} without a filesystem", path);
		return;
	}

	const auto file = context.fs->Load(path);
	if (!file) {
		Log::Error("ImageTexture: failed to read {} (error {})", path,
			static_cast<int>(file.error));
		return;
	}
	if (!file.value || file.value->Size() == 0) {
		Log::Error("ImageTexture: {} is empty", path);
		return;
	}

	// Flip rows for Y-up texture coordinates. Window icons and cursors can request top-row-first
	// pixels.
	stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(file.value->Data()),
		static_cast<int>(file.value->Size()), &width, &height, &channels, 4);
	if (pixels == nullptr) {
		Log::Error("ImageTexture: stb_image failed for {}: {}", path,
			stbi_failure_reason() != nullptr ? stbi_failure_reason() : "unknown");
		return;
	}

	// Without a graphics device, decoding still provides dimensions and retained pixels;
	// IsLoaded() remains false.
	TextureHandle handle;
	if (context.device != nullptr) {
		handle = context.device->CreateTexture(width, height, pixels, filter);
		if (!handle.IsValid()) {
			Log::Error("ImageTexture: GPU upload failed for {}", path);
			stbi_image_free(pixels);
			return;
		}
	}

	if (retainPixels) {
		const size_t byteCount = static_cast<size_t>(width) * height * 4;
		_pixels.assign(pixels, pixels + byteCount);
	}
	stbi_image_free(pixels);

	_handle = handle;
	_width = width;
	_height = height;
}

void ImageTexture::Unload(const ResourceContext &context) {
	// A null device means nothing was ever uploaded through one, so dropping
	// the handle is all that's left to do.
	if (_handle.IsValid() && context.device != nullptr) {
		context.device->DestroyTexture(_handle);
	}

	_handle = {};
	_width = 0;
	_height = 0;
	_pixels.clear();
	_pixels.shrink_to_fit();
}

void ImageTexture::Adopt(const ResourceContext &context, TextureHandle handle, i32 width,
	i32 height) {
	Unload(context);

	_handle = handle;
	_width = width;
	_height = height;
}

} // namespace sowa
