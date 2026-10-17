#pragma once

#include <string_view>

#include "render/graphics_device.hpp"

namespace sowa {

class FileSystem;

// Draws a texture as a fullscreen NDC quad into the current viewport
// (typically the letterboxed present rect on the default framebuffer).
class FullscreenPresent {
  public:
	FullscreenPresent() = default;
	~FullscreenPresent();

	FullscreenPresent(const FullscreenPresent &) = delete;
	FullscreenPresent &operator=(const FullscreenPresent &) = delete;

	// Empty shaderPath selects the embedded shader.
	bool Init(GraphicsDevice &device, FileSystem &fs, std::string_view shaderPath = {});
	void Shutdown();

	bool IsInitialized() const {
		return _device != nullptr;
	}

	void Draw(TextureHandle texture);

  private:
	GraphicsDevice *_device = nullptr;
	ShaderHandle _shader;
	PipelineHandle _pipeline;
	BufferHandle _vertexBuffer;
	BufferHandle _indexBuffer;
};

} // namespace sowa
