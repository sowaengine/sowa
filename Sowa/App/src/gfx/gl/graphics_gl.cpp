#include "graphics_gl.hpp"

#include <vector>

#include "debug.hpp"
#include "gfx/gl/glfuncs.hpp"
#include "glad/glad.h"

namespace sowa {
namespace gfx {

GraphicsGL::GraphicsGL() {

	std::vector<float> quad_data(16);
	// vec2 position, vec2 uv
	quad_data[0] = -0.5f;
	quad_data[1] = 0.5f;
	quad_data[2] = 0.0f;
	quad_data[3] = 1.0f;

	quad_data[4] = -0.5f;
	quad_data[5] = -0.5f;
	quad_data[6] = 0.0f;
	quad_data[7] = 0.0f;

	quad_data[8] = 0.5f;
	quad_data[9] = -0.5f;
	quad_data[10] = 1.0f;
	quad_data[11] = 0.0f;

	quad_data[12] = 0.5f;
	quad_data[13] = 0.5f;
	quad_data[14] = 1.0f;
	quad_data[15] = 1.0f;

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
}
GraphicsGL::~GraphicsGL() {
}

void GraphicsGL::SetDepthTest(bool v) {
	GL().setDepthTest(v);
}

IShader &GraphicsGL::Default2DShader() {
	return m_default2dshader;
}

void GraphicsGL::DrawQuad() {
	GL().viewport(0, 0, 1280, 720);
	m_default2dmesh.Draw();
}

void GraphicsGL::Clear() {
	GL().bindFramebuffer(0);
	GL().clearColor(0.02f, 0.04f, 0.07f, 1.f);
	GL().setDepthTest(false);
	GL().clearColorBit();
}

} // namespace gfx
} // namespace sowa