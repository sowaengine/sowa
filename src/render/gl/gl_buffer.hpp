#pragma once

#include <vector>

#include "core/types.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

class GLBufferStore {
  public:
	BufferHandle Create(BufferType type, BufferUsage usage, u32 size, const void *data);
	void Destroy(BufferHandle handle);
	void Update(BufferHandle handle, const void *data, u32 size, u32 offset);
	void DestroyAll();

	// Returns the GL buffer name, or 0 if the handle is invalid/stale.
	u32 GlId(BufferHandle handle) const;
	BufferType Type(BufferHandle handle) const;

  private:
	struct Slot {
		u32 glBuffer = 0;
		BufferType type = BufferType::Vertex;
		u32 size = 0;
		u32 version = 1;
		bool occupied = false;
	};

	Slot *resolve(BufferHandle handle);
	const Slot *resolve(BufferHandle handle) const;

	std::vector<Slot> _slots;
};

} // namespace sowa
