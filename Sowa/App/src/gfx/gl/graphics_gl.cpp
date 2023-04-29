#include "graphics_gl.hpp"

#include "gfx/gl/glfuncs.hpp"

namespace sowa {
namespace gfx {

GraphicsGL::GraphicsGL() {
}
GraphicsGL::~GraphicsGL() {
}

void GraphicsGL::SetDepthTest(bool v) {
	GL().setDepthTest(v);
}

IShader &GraphicsGL::Default2DShader() {
	return m_default2dshader;
}

} // namespace gfx
} // namespace sowa