#pragma once

#include <vector>

#include "core/types.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

class GLShaderStore;

class GLPipelineStore {
  public:
	PipelineHandle Create(const PipelineDesc &desc, GLShaderStore &shaders);
	void Destroy(PipelineHandle handle);
	void Bind(PipelineHandle handle) const;
	void DestroyAll();

	// Applies the pipeline's vertex layout to the currently bound
	// GL_ARRAY_BUFFER (call after binding the vertex buffer).
	void ApplyVertexLayout(PipelineHandle handle) const;

  private:
	struct Slot {
		u32 glVao = 0;
		u32 glProgram = 0;
		VertexLayout layout;
		bool alphaBlend = true;
		bool additiveBlend = false;
		u32 version = 1;
		bool occupied = false;
	};

	Slot *resolve(PipelineHandle handle);
	const Slot *resolve(PipelineHandle handle) const;

	std::vector<Slot> _slots;
};

} // namespace sowa
