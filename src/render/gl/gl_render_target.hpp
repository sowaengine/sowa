#pragma once

#include <vector>

#include "core/types.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

class GLTextureStore;

class GLRenderTargetStore {
  public:
	RenderTargetHandle Create(const RenderTargetDesc &desc, GLTextureStore &textures);
	void Destroy(RenderTargetHandle handle, GLTextureStore &textures);
	void Bind(RenderTargetHandle handle) const;
	void DestroyAll(GLTextureStore &textures);

	TextureHandle ColorTexture(RenderTargetHandle handle) const;
	void ClearUInt(RenderTargetHandle handle, u32 attachment, u32 value) const;
	u32 ReadUInt(RenderTargetHandle handle, u32 attachment, i32 x, i32 y) const;
	std::vector<u32> ReadUIntRect(RenderTargetHandle handle, u32 attachment, i32 x, i32 y,
		i32 width, i32 height) const;
	i32 Width(RenderTargetHandle handle) const;
	i32 Height(RenderTargetHandle handle) const;

  private:
	struct Slot {
		u32 glFbo = 0;
		u32 glUIntTexture = 0;
		TextureHandle colorTexture;
		i32 width = 0;
		i32 height = 0;
		u32 version = 1;
		bool occupied = false;
	};

	Slot *resolve(RenderTargetHandle handle);
	const Slot *resolve(RenderTargetHandle handle) const;

	std::vector<Slot> _slots;
};

} // namespace sowa
