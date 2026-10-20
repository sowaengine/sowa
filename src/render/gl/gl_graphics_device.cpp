#include "render/gl/gl_graphics_device.hpp"

#include "render/gl/gl.hpp"

#include "core/log.hpp"
#include "core/window.hpp"

namespace sowa {

void GLGraphicsDevice::ApplyWindowHints() {
#if defined(__EMSCRIPTEN__)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// Render at device pixel ratio; the window size stays in CSS pixels.
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
	// Required on macOS for a real 3.2+/3.3 core context; without it GLFW
	// often hands back a legacy 2.1 context that rejects #version 330.
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
#endif
}

bool GLGraphicsDevice::Init(Window &window) {
	if (!window.IsCreated()) {
		Log::Error("GLGraphicsDevice::Init called with an uncreated Window");
		return false;
	}

	glfwMakeContextCurrent(window.Handle());

#if !defined(__EMSCRIPTEN__)
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Log::Error("GLGraphicsDevice: failed to load GL functions via glad");
		return false;
	}
#endif

	Log::Info("GLGraphicsDevice: GL {} ({})",
		reinterpret_cast<const char *>(glGetString(GL_VERSION)),
		reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

	const Vector2 size = window.GetSize();
	glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));

	_initialized = true;
	return true;
}

void GLGraphicsDevice::Shutdown() {
	if (!_initialized) {
		return;
	}
	_pipelines.DestroyAll();
	_buffers.DestroyAll();
	_renderTargets.DestroyAll(_textures);
	_textures.DestroyAll();
	_shaders.DestroyAll();
	_boundPipeline = {};
	_initialized = false;
}

void GLGraphicsDevice::Clear(const Color &color) {
	const GLfloat values[] = {color.r, color.g, color.b, color.a};
	glClearBufferfv(GL_COLOR, 0, values);
}

ShaderHandle GLGraphicsDevice::CreateShader(std::string_view vertexSource,
	std::string_view fragmentSource) {
	return _shaders.Create(vertexSource, fragmentSource);
}

void GLGraphicsDevice::DestroyShader(ShaderHandle handle) {
	_shaders.Destroy(handle);
}

void GLGraphicsDevice::BindShader(ShaderHandle handle) {
	_shaders.Bind(handle);
}

BufferHandle GLGraphicsDevice::CreateBuffer(BufferType type, BufferUsage usage, u32 size,
	const void *data) {
	return _buffers.Create(type, usage, size, data);
}

void GLGraphicsDevice::DestroyBuffer(BufferHandle handle) {
	_buffers.Destroy(handle);
}

void GLGraphicsDevice::UpdateBuffer(BufferHandle handle, const void *data, u32 size, u32 offset) {
	_buffers.Update(handle, data, size, offset);
}

PipelineHandle GLGraphicsDevice::CreatePipeline(const PipelineDesc &desc) {
	return _pipelines.Create(desc, _shaders);
}

void GLGraphicsDevice::DestroyPipeline(PipelineHandle handle) {
	if (_boundPipeline == handle) {
		_boundPipeline = {};
	}
	_pipelines.Destroy(handle);
}

void GLGraphicsDevice::BindPipeline(PipelineHandle handle) {
	_pipelines.Bind(handle);
	_boundPipeline = handle;
}

TextureHandle GLGraphicsDevice::CreateTexture(i32 width, i32 height, const void *rgbaPixels,
	TextureFilter filter) {
	return _textures.Create(width, height, rgbaPixels, filter);
}

void GLGraphicsDevice::DestroyTexture(TextureHandle handle) {
	_textures.Destroy(handle);
}

void GLGraphicsDevice::BindTexture(TextureHandle handle, u32 slot) {
	_textures.Bind(handle, slot);
}

i32 GLGraphicsDevice::GetTextureWidth(TextureHandle handle) const {
	return _textures.Width(handle);
}

i32 GLGraphicsDevice::GetTextureHeight(TextureHandle handle) const {
	return _textures.Height(handle);
}

u64 GLGraphicsDevice::GetNativeTextureId(TextureHandle handle) const {
	return _textures.GlId(handle);
}

RenderTargetHandle GLGraphicsDevice::CreateRenderTarget(const RenderTargetDesc &desc) {
	return _renderTargets.Create(desc, _textures);
}

void GLGraphicsDevice::DestroyRenderTarget(RenderTargetHandle handle) {
	_renderTargets.Destroy(handle, _textures);
}

void GLGraphicsDevice::BindRenderTarget(RenderTargetHandle handle) {
	_renderTargets.Bind(handle);
}

TextureHandle GLGraphicsDevice::GetRenderTargetColorTexture(RenderTargetHandle handle) const {
	return _renderTargets.ColorTexture(handle);
}

void GLGraphicsDevice::ClearRenderTargetUInt(RenderTargetHandle handle, u32 attachment, u32 value) {
	_renderTargets.ClearUInt(handle, attachment, value);
}

u32 GLGraphicsDevice::ReadRenderTargetUInt(RenderTargetHandle handle, u32 attachment, i32 x,
	i32 y) {
	return _renderTargets.ReadUInt(handle, attachment, x, y);
}

std::vector<u32> GLGraphicsDevice::ReadRenderTargetUIntRect(RenderTargetHandle handle,
	u32 attachment, i32 x, i32 y, i32 width, i32 height) {
	return _renderTargets.ReadUIntRect(handle, attachment, x, y, width, height);
}

void GLGraphicsDevice::SetViewport(i32 x, i32 y, i32 width, i32 height) {
	glViewport(x, y, width, height);
}

void GLGraphicsDevice::DrawIndexed(BufferHandle vertexBuffer, BufferHandle indexBuffer,
	u32 indexCount) {
	if (!_boundPipeline.IsValid()) {
		Log::Warn("GLGraphicsDevice::DrawIndexed: no pipeline bound");
		return;
	}

	const u32 vbo = _buffers.GlId(vertexBuffer);
	const u32 ibo = _buffers.GlId(indexBuffer);
	if (vbo == 0 || ibo == 0) {
		Log::Warn("GLGraphicsDevice::DrawIndexed: invalid buffer handle");
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	_pipelines.ApplyVertexLayout(_boundPipeline);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
}

namespace {

GLint CurrentUniformLocation(const char *name) {
	GLint program = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &program);
	if (program == 0) {
		return -1;
	}
	return glGetUniformLocation(program, name);
}

} // namespace

void GLGraphicsDevice::SetUniformFloat(const char *name, f32 value) {
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniform1f(loc, value);
}

void GLGraphicsDevice::SetUniformVec2(const char *name, const Vector2 &value) {
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniform2f(loc, value.x, value.y);
}

void GLGraphicsDevice::SetUniformVec3(const char *name, const Vector3 &value) {
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniform3f(loc, value.x, value.y, value.z);
}

void GLGraphicsDevice::SetUniformVec4(const char *name, f32 x, f32 y, f32 z, f32 w) {
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniform4f(loc, x, y, z, w);
}

void GLGraphicsDevice::SetUniformColor(const char *name, const Color &value) {
	SetUniformVec4(name, value.r, value.g, value.b, value.a);
}

void GLGraphicsDevice::SetUniformMat4(const char *name, const glm::mat4 &value) {
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void GLGraphicsDevice::SetUniformInt(const char *name, i32 value) {
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniform1i(loc, value);
}

void GLGraphicsDevice::SetUniformIntArray(const char *name, const i32 *values, u32 count) {
	if (!values || count == 0) {
		return;
	}
	const GLint loc = CurrentUniformLocation(name);
	if (loc < 0) {
		return;
	}
	glUniform1iv(loc, static_cast<GLsizei>(count), values);
}

void GLGraphicsDevice::SetUniformTexture(const char *name, TextureHandle texture, u32 slot) {
	BindTexture(texture, slot);
	SetUniformInt(name, static_cast<i32>(slot));
}

} // namespace sowa
