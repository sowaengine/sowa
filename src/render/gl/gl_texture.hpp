#pragma once

#include <vector>

#include "core/types.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

class GLTextureStore {
  public:
	TextureHandle Create(i32 width, i32 height, const void *rgbaPixels,
		TextureFilter filter = TextureFilter::Linear);
	void Destroy(TextureHandle handle);
	void Bind(TextureHandle handle, u32 slot) const;
	void DestroyAll();

	u32 GlId(TextureHandle handle) const;
	i32 Width(TextureHandle handle) const;
	i32 Height(TextureHandle handle) const;

  private:
	struct Slot {
		u32 glTexture = 0;
		i32 width = 0;
		i32 height = 0;
		u32 version = 1;
		bool occupied = false;
	};

	Slot *resolve(TextureHandle handle);
	const Slot *resolve(TextureHandle handle) const;

	std::vector<Slot> _slots;
};

} // namespace sowa
