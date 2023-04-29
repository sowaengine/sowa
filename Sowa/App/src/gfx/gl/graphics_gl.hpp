#ifndef SW_GRAPHICS_GL_HPP_
#define SW_GRAPHICS_GL_HPP_

#include "../graphics.hpp"
#include "shader_gl.hpp"

namespace sowa {
namespace gfx {

class GraphicsGL : public IGraphics {
  public:
	GraphicsGL();
	~GraphicsGL();

	void SetDepthTest(bool) override;
	IShader &Default2DShader() override;

  private:
	GLShader m_default2dshader;
};

} // namespace gfx
} // namespace sowa

#endif // SW_GRAPHICS_GL_HPP_