#ifndef SW_GRAPHICS_GL_HPP_
#define SW_GRAPHICS_GL_HPP_

#include "../graphics.hpp"
#include "shader_gl.hpp"
#include "mesh_gl.hpp"

namespace sowa {
namespace gfx {

class GraphicsGL : public IGraphics {
  public:
	GraphicsGL();
	~GraphicsGL();

	void SetDepthTest(bool) override;
	IShader &Default2DShader() override;

	void DrawQuad() override;
	void Clear() override;

  private:
	GLShader m_default2dshader;
	GLMesh m_default2dmesh;
};

} // namespace gfx
} // namespace sowa

#endif // SW_GRAPHICS_GL_HPP_