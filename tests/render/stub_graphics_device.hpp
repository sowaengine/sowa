#pragma once

#include <string>

#include "core/types.hpp"
#include "math/color.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

// A GraphicsDevice that implements the interface and records what it was
// asked to do, for tests that need to observe calls reaching the RHI without
// a GL context. Shared rather than redeclared per test file - there are two
// callers now, and a second copy would drift.
class StubGraphicsDevice : public GraphicsDevice {
  public:
	GraphicsAPI API() const override { return GraphicsAPI::OpenGL; }
	void ApplyWindowHints() override {}
	bool Init(Window &) override { return true; }
	void Shutdown() override {}
	void Clear(const Color &) override {}

	ShaderHandle CreateShader(std::string_view vertexSource,
							  std::string_view fragmentSource) override {
		createCalled = true;
		lastVertex = std::string(vertexSource);
		lastFragment = std::string(fragmentSource);
		return ShaderHandle{1, 1};
	}
	void DestroyShader(ShaderHandle) override {}
	void BindShader(ShaderHandle) override {}

	BufferHandle CreateBuffer(BufferType, BufferUsage, u32,
							  const void *) override {
		return {};
	}
	void DestroyBuffer(BufferHandle) override {}
	void UpdateBuffer(BufferHandle, const void *, u32, u32) override {}

	PipelineHandle CreatePipeline(const PipelineDesc &) override { return {}; }
	void DestroyPipeline(PipelineHandle) override {}
	void BindPipeline(PipelineHandle) override {}

	TextureHandle CreateTexture(i32 width, i32 height, const void *pixels,
								TextureFilter filter) override {
		++textureCreateCount;
		lastTextureWidth = width;
		lastTextureHeight = height;
		lastTextureFilter = filter;
		lastTexturePixelsWereNull = pixels == nullptr;
		return TextureHandle{static_cast<u32>(textureCreateCount), 1};
	}
	void DestroyTexture(TextureHandle handle) override {
		++textureDestroyCount;
		lastDestroyed = handle;
	}
	void BindTexture(TextureHandle, u32) override {}
	i32 GetTextureWidth(TextureHandle) const override { return 0; }
	i32 GetTextureHeight(TextureHandle) const override { return 0; }
	u64 GetNativeTextureId(TextureHandle) const override { return 0; }

	RenderTargetHandle CreateRenderTarget(const RenderTargetDesc &desc) override {
		lastRenderTargetDesc = desc;
		return {};
	}
	void DestroyRenderTarget(RenderTargetHandle) override {}
	void BindRenderTarget(RenderTargetHandle) override {}
	TextureHandle
	GetRenderTargetColorTexture(RenderTargetHandle) const override {
		return {};
	}
	void ClearRenderTargetUInt(RenderTargetHandle, u32 attachment,
							   u32 value) override {
		lastClearedUIntAttachment = attachment;
		lastClearedUIntValue = value;
	}
	u32 ReadRenderTargetUInt(RenderTargetHandle, u32 attachment, i32 x,
							 i32 y) override {
		lastReadUIntAttachment = attachment;
		lastReadX = x;
		lastReadY = y;
		return readUIntResult;
	}

	void SetViewport(i32, i32, i32, i32) override {}

	void DrawIndexed(BufferHandle, BufferHandle, u32) override {}
	void SetUniformFloat(const char *, f32) override {}
	void SetUniformVec2(const char *, const Vector2 &) override {}
	void SetUniformVec3(const char *, const Vector3 &) override {}
	void SetUniformVec4(const char *, f32, f32, f32, f32) override {}
	void SetUniformColor(const char *, const Color &) override {}
	void SetUniformMat4(const char *, const glm::mat4 &) override {}
	void SetUniformInt(const char *, i32) override {}
	void SetUniformIntArray(const char *, const i32 *, u32) override {}
	void SetUniformTexture(const char *, TextureHandle, u32) override {}

	bool createCalled = false;
	std::string lastVertex;
	std::string lastFragment;

	i32 textureCreateCount = 0;
	i32 textureDestroyCount = 0;
	i32 lastTextureWidth = 0;
	i32 lastTextureHeight = 0;
	TextureFilter lastTextureFilter = TextureFilter::Nearest;
	bool lastTexturePixelsWereNull = true;
	TextureHandle lastDestroyed;
	RenderTargetDesc lastRenderTargetDesc;
	u32 lastClearedUIntAttachment = 0;
	u32 lastClearedUIntValue = 0;
	u32 lastReadUIntAttachment = 0;
	i32 lastReadX = 0;
	i32 lastReadY = 0;
	u32 readUIntResult = 0;
};

} // namespace sowa
