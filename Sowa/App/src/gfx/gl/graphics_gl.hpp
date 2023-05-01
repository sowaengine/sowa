#ifndef SW_GRAPHICS_GL_HPP_
#define SW_GRAPHICS_GL_HPP_

#include "../graphics.hpp"
#include "mesh_gl.hpp"
#include "shader_gl.hpp"

namespace sowa {
namespace gfx {

class GraphicsGL : public IGraphics {
  public:
	GraphicsGL();
	~GraphicsGL();

	void SetDepthTest(bool) override;

	IShader &Default2DShader() override;
	IShader &DefaultFullscreenShader() override;

	void DrawQuad() override;
	void DrawFullscreenQuad() override;
	void Clear() override;

  private:
	GLShader m_default2dshader;
	GLShader m_defaultFullscreenShader;

	GLMesh m_default2dmesh;
	GLMesh m_defaultFullscreenMesh;
};

} // namespace gfx
} // namespace sowa

#endif // SW_GRAPHICS_GL_HPP_