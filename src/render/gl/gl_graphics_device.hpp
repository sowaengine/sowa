#pragma once

#include "render/gl/gl_buffer.hpp"
#include "render/gl/gl_pipeline.hpp"
#include "render/gl/gl_render_target.hpp"
#include "render/gl/gl_shader.hpp"
#include "render/gl/gl_texture.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

class GLGraphicsDevice : public GraphicsDevice {
  public:
	GraphicsAPI API() const override {
		return GraphicsAPI::OpenGL;
	}

	void ApplyWindowHints() override;
	bool Init(Window &window) override;
	void Shutdown() override;

	void Clear(const Color &color) override;

	ShaderHandle CreateShader(std::string_view vertexSource,
		std::string_view fragmentSource) override;
	void DestroyShader(ShaderHandle handle) override;
	void BindShader(ShaderHandle handle) override;

	BufferHandle CreateBuffer(BufferType type, BufferUsage usage, u32 size,
		const void *data) override;
	void DestroyBuffer(BufferHandle handle) override;
	void UpdateBuffer(BufferHandle handle, const void *data, u32 size, u32 offset) override;

	PipelineHandle CreatePipeline(const PipelineDesc &desc) override;
	void DestroyPipeline(PipelineHandle handle) override;
	void BindPipeline(PipelineHandle handle) override;

	TextureHandle CreateTexture(i32 width, i32 height, const void *rgbaPixels = nullptr,
		TextureFilter filter = TextureFilter::Linear) override;
	void DestroyTexture(TextureHandle handle) override;
	void BindTexture(TextureHandle handle, u32 slot) override;
	i32 GetTextureWidth(TextureHandle handle) const override;
	i32 GetTextureHeight(TextureHandle handle) const override;
	u64 GetNativeTextureId(TextureHandle handle) const override;

	RenderTargetHandle CreateRenderTarget(const RenderTargetDesc &desc) override;
	void DestroyRenderTarget(RenderTargetHandle handle) override;
	void BindRenderTarget(RenderTargetHandle handle) override;
	TextureHandle GetRenderTargetColorTexture(RenderTargetHandle handle) const override;
	void ClearRenderTargetUInt(RenderTargetHandle handle, u32 attachment, u32 value) override;
	u32 ReadRenderTargetUInt(RenderTargetHandle handle, u32 attachment, i32 x, i32 y) override;

	void SetViewport(i32 x, i32 y, i32 width, i32 height) override;

	void DrawIndexed(BufferHandle vertexBuffer, BufferHandle indexBuffer, u32 indexCount) override;

	void SetUniformFloat(const char *name, f32 value) override;
	void SetUniformVec2(const char *name, const Vector2 &value) override;
	void SetUniformVec3(const char *name, const Vector3 &value) override;
	void SetUniformVec4(const char *name, f32 x, f32 y, f32 z, f32 w) override;
	void SetUniformColor(const char *name, const Color &value) override;
	void SetUniformMat4(const char *name, const glm::mat4 &value) override;
	void SetUniformInt(const char *name, i32 value) override;
	void SetUniformIntArray(const char *name, const i32 *values, u32 count) override;
	void SetUniformTexture(const char *name, TextureHandle texture, u32 slot) override;

  private:
	GLShaderStore _shaders;
	GLBufferStore _buffers;
	GLPipelineStore _pipelines;
	GLTextureStore _textures;
	GLRenderTargetStore _renderTargets;
	PipelineHandle _boundPipeline;
	bool _initialized = false;
};

} // namespace sowa
