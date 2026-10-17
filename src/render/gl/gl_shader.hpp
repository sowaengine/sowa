#pragma once

#include <string_view>
#include <vector>

#include "core/types.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

// Owns compiled GL programs behind versioned shader handles.
class GLShaderStore {
  public:
	ShaderHandle Create(std::string_view vertexSource, std::string_view fragmentSource);
	void Destroy(ShaderHandle handle);
	void Bind(ShaderHandle handle) const;
	void DestroyAll();

	// Returns the GL program name, or 0 if the handle is invalid/stale.
	u32 ProgramId(ShaderHandle handle) const;

  private:
	struct Slot {
		u32 glProgram = 0;
		u32 version = 1;
		bool occupied = false;
	};

	Slot *resolve(ShaderHandle handle);
	const Slot *resolve(ShaderHandle handle) const;

	std::vector<Slot> _slots;
};

} // namespace sowa
