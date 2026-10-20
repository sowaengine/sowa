#pragma once

#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace sowa {

class Window;

enum class GraphicsAPI {
	OpenGL,
};

enum class BufferType {
	Vertex,
	Index,
};

enum class BufferUsage {
	Static,
	Dynamic,
};

enum class VertexAttrType {
	Float,
	Vec2,
	Vec3,
	Vec4,
	UInt32,
};

struct VertexAttribute {
	u32 location = 0;
	VertexAttrType type = VertexAttrType::Float;
};

struct VertexLayout {
	std::vector<VertexAttribute> attributes;
	// If 0, stride is computed from the attributes (tightly packed).
	u32 stride = 0;
};

inline u32 VertexAttrTypeSize(VertexAttrType type) {
	switch (type) {
	case VertexAttrType::Float:
		return sizeof(f32);
	case VertexAttrType::Vec2:
		return sizeof(f32) * 2;
	case VertexAttrType::Vec3:
		return sizeof(f32) * 3;
	case VertexAttrType::Vec4:
		return sizeof(f32) * 4;
	case VertexAttrType::UInt32:
		return sizeof(u32);
	}
	return 0;
}

inline u32 VertexAttrTypeComponents(VertexAttrType type) {
	switch (type) {
	case VertexAttrType::Float:
		return 1;
	case VertexAttrType::Vec2:
		return 2;
	case VertexAttrType::Vec3:
		return 3;
	case VertexAttrType::Vec4:
		return 4;
	case VertexAttrType::UInt32:
		return 1;
	}
	return 0;
}

inline u32 VertexLayoutStride(const VertexLayout &layout) {
	if (layout.stride != 0) {
		return layout.stride;
	}
	u32 stride = 0;
	for (const VertexAttribute &attr : layout.attributes) {
		stride += VertexAttrTypeSize(attr.type);
	}
	return stride;
}

// Opaque GPU-resource handles. id 0 is always invalid; version is bumped
// when a slot is freed so stale handles stop matching after reuse.
struct ShaderHandle {
	u32 id = 0;
	u32 version = 0;

	bool IsValid() const {
		return id != 0;
	}

	bool operator==(const ShaderHandle &) const = default;
	bool operator!=(const ShaderHandle &) const = default;
};

struct BufferHandle {
	u32 id = 0;
	u32 version = 0;

	bool IsValid() const {
		return id != 0;
	}

	bool operator==(const BufferHandle &) const = default;
	bool operator!=(const BufferHandle &) const = default;
};

struct PipelineHandle {
	u32 id = 0;
	u32 version = 0;

	bool IsValid() const {
		return id != 0;
	}

	bool operator==(const PipelineHandle &) const = default;
	bool operator!=(const PipelineHandle &) const = default;
};

struct TextureHandle {
	u32 id = 0;
	u32 version = 0;

	bool IsValid() const {
		return id != 0;
	}

	bool operator==(const TextureHandle &) const = default;
	bool operator!=(const TextureHandle &) const = default;
};

struct RenderTargetHandle {
	u32 id = 0;
	u32 version = 0;

	bool IsValid() const {
		return id != 0;
	}

	bool operator==(const RenderTargetHandle &) const = default;
	bool operator!=(const RenderTargetHandle &) const = default;
};

struct PipelineDesc {
	ShaderHandle shader;
	VertexLayout layout;
	bool alphaBlend = true;
	bool additiveBlend = false;
};

// Texture sampling: Linear interpolates; Nearest preserves hard texel edges.
enum class TextureFilter {
	Nearest,
	Linear,
};

// Reflection traits live beside their type to keep core independent of rendering.
// ImageTexture uses these to expose its filter field.
SOWA_REFLECT_ENUM(TextureFilter, {"Nearest", TextureFilter::Nearest},
	{"Linear", TextureFilter::Linear});

struct RenderTargetDesc {
	i32 width = 1;
	i32 height = 1;
	// Applies to the colour attachment, which is what the present pass samples
	// when it scales the viewport into the window.
	TextureFilter filter = TextureFilter::Linear;
	// Adds an R32UI color attachment at location 1. Used for general entity
	// hover/picking while location 0 remains the presented RGBA image.
	bool unsignedIntegerAttachment = false;
};

// Rendering backend interface. Device-owned opaque handles identify resources;
// pipelines combine shaders, vertex layouts, and blend state.
class GraphicsDevice {
  public:
	virtual ~GraphicsDevice() = default;

	virtual GraphicsAPI API() const = 0;

	// Must be called after Window::EnsureGlfwInitialized() and before
	// Window::Create(). Sets backend-specific GLFW context hints (e.g.
	// OpenGL 3.3 core).
	virtual void ApplyWindowHints() = 0;

	virtual bool Init(Window &window) = 0;
	virtual void Shutdown() = 0;

	virtual void Clear(const Color &color) = 0;

	virtual ShaderHandle CreateShader(std::string_view vertexSource,
		std::string_view fragmentSource) = 0;
	virtual void DestroyShader(ShaderHandle handle) = 0;
	virtual void BindShader(ShaderHandle handle) = 0;

	virtual BufferHandle CreateBuffer(BufferType type, BufferUsage usage, u32 size,
		const void *data = nullptr) = 0;
	virtual void DestroyBuffer(BufferHandle handle) = 0;
	virtual void UpdateBuffer(BufferHandle handle, const void *data, u32 size, u32 offset = 0) = 0;

	virtual PipelineHandle CreatePipeline(const PipelineDesc &desc) = 0;
	virtual void DestroyPipeline(PipelineHandle handle) = 0;
	virtual void BindPipeline(PipelineHandle handle) = 0;

	// Creates an RGBA8 2D texture. rgbaPixels may be null to allocate an
	// empty texture (e.g. for render-target color attachments).
	virtual TextureHandle CreateTexture(i32 width, i32 height, const void *rgbaPixels = nullptr,
		TextureFilter filter = TextureFilter::Linear) = 0;
	virtual void DestroyTexture(TextureHandle handle) = 0;
	virtual void BindTexture(TextureHandle handle, u32 slot = 0) = 0;
	virtual i32 GetTextureWidth(TextureHandle handle) const = 0;
	virtual i32 GetTextureHeight(TextureHandle handle) const = 0;
	// Backend-native texture identifier (OpenGL: GLuint). Used by ImGui::Image.
	virtual u64 GetNativeTextureId(TextureHandle handle) const = 0;

	// Offscreen framebuffer with an RGBA8 color attachment and, when requested,
	// an R32UI attachment. Bind with an invalid handle to return to the default
	// (window) framebuffer.
	virtual RenderTargetHandle CreateRenderTarget(const RenderTargetDesc &desc) = 0;
	virtual void DestroyRenderTarget(RenderTargetHandle handle) = 0;
	virtual void BindRenderTarget(RenderTargetHandle handle) = 0;
	virtual TextureHandle GetRenderTargetColorTexture(RenderTargetHandle handle) const = 0;
	virtual void ClearRenderTargetUInt(RenderTargetHandle handle, u32 attachment, u32 value) = 0;
	virtual u32 ReadRenderTargetUInt(RenderTargetHandle handle, u32 attachment, i32 x, i32 y) = 0;

	virtual std::vector<u32> ReadRenderTargetUIntRect(RenderTargetHandle, u32, i32, i32, i32,
		i32) = 0;

	virtual void SetViewport(i32 x, i32 y, i32 width, i32 height) = 0;

	// Draws with the currently bound pipeline. Index type is u32.
	virtual void DrawIndexed(BufferHandle vertexBuffer, BufferHandle indexBuffer,
		u32 indexCount) = 0;

	// Uniform setters apply to the currently bound program (via BindPipeline
	// / BindShader).
	virtual void SetUniformFloat(const char *name, f32 value) = 0;
	virtual void SetUniformVec2(const char *name, const Vector2 &value) = 0;
	virtual void SetUniformVec3(const char *name, const Vector3 &value) = 0;
	virtual void SetUniformVec4(const char *name, f32 x, f32 y, f32 z, f32 w) = 0;
	virtual void SetUniformColor(const char *name, const Color &value) = 0;
	virtual void SetUniformMat4(const char *name, const glm::mat4 &value) = 0;
	virtual void SetUniformInt(const char *name, i32 value) = 0;
	virtual void SetUniformIntArray(const char *name, const i32 *values, u32 count) = 0;
	// Binds texture to slot and sets the sampler uniform to that slot.
	virtual void SetUniformTexture(const char *name, TextureHandle texture, u32 slot = 0) = 0;
};

Scope<GraphicsDevice> CreateGraphicsDevice(GraphicsAPI api);

} // namespace sowa
