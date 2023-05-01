#include "graphics_gl.hpp"

#include <vector>

#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"
#include "glad/glad.h"

namespace sowa {
namespace gfx {

GraphicsGL::GraphicsGL() {
	int maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	Debug::Log("Max texture size: {}", maxTextureSize);

	// 2d quad
	const std::vector<float> quad_data = {
		// vec2 position, vec2 uv
		-0.5f,
		0.5f,
		0.0f,
		1.0f,

		-0.5f,
		-0.5f,
		0.0f,
		0.0f,

		0.5f,
		-0.5f,
		1.0f,
		0.0f,

		0.5f,
		0.5f,
		1.0f,
		1.0f,
	};

	const std::vector<uint32_t> quad_indices = {
		0, 1, 2,
		0, 2, 3};

	m_default2dmesh.New();
	m_default2dmesh.ResetAttributes();
	m_default2dmesh.SetModelData(quad_data);
	m_default2dmesh.SetIndexData(quad_indices);
	m_default2dmesh.SetAttribute(0, GLAttributeType::Vec2);
	m_default2dmesh.SetAttribute(1, GLAttributeType::Vec2);
	m_default2dmesh.UploadAttributes();

	// Fullscreen quad
	const std::vector<float> fullscreen_quad_data = {
		// vec2 position, vec2 uv
		-1.0f,
		1.0f,
		0.0f,
		1.0f,

		-1.0f,
		-1.0f,
		0.0f,
		0.0f,

		1.0f,
		-1.0f,
		1.0f,
		0.0f,

		1.0f,
		1.0f,
		1.0f,
		1.0f,
	};

	const std::vector<uint32_t> fullscreen_quad_indices = {
		0, 1, 2,
		0, 2, 3};

	m_defaultFullscreenMesh.New();
	m_defaultFullscreenMesh.ResetAttributes();
	m_defaultFullscreenMesh.SetModelData(fullscreen_quad_data);
	m_defaultFullscreenMesh.SetIndexData(fullscreen_quad_indices);
	m_defaultFullscreenMesh.SetAttribute(0, GLAttributeType::Vec2);
	m_defaultFullscreenMesh.SetAttribute(1, GLAttributeType::Vec2);
	m_defaultFullscreenMesh.UploadAttributes();
}
GraphicsGL::~GraphicsGL() {
}

void GraphicsGL::SetDepthTest(bool v) {
	GL().setDepthTest(v);
}

IShader &GraphicsGL::Default2DShader() {
	return m_default2dshader;
}

IShader &GraphicsGL::DefaultFullscreenShader() {
	return m_defaultFullscreenShader;
}

void GraphicsGL::DrawQuad() {
	m_default2dmesh.Draw();
}
void GraphicsGL::DrawFullscreenQuad() {
	m_defaultFullscreenMesh.Draw();
}

void GraphicsGL::Clear() {
	GL().clearColor(0.02f, 0.04f, 0.07f, 0.f);
	GL().clearColorBit();
	GL().clearDepthBit();
}

} // namespace gfx
} // namespace sowa