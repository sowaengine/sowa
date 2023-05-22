#ifndef SW_GRAPHICS_GL_HPP_
#define SW_GRAPHICS_GL_HPP_

#include <vector>

#include "./batch_renderer_gl.hpp"
#include "./buffer_gl.hpp"
#include "./mesh_gl.hpp"
#include "./shader_gl.hpp"
#include "./vertex_array_gl.hpp"

namespace sowa {
namespace gfx {

class GraphicsGL : public IGraphics {
  public:
	GraphicsGL();
	~GraphicsGL();

	void SetDepthTest(bool) override;

	IShader &Default2DShader() override;
	IShader &DefaultSolidColorShader() override;
	IShader &DefaultFullscreenShader() override;
	IShader &DefaultUITextShader() override;

	void DrawQuad() override;
	void DrawFullscreenQuad() override;
	void DrawText(const std::string &text, IFont *font, float x, float y, mat4 transform, float scale) override;
	void DrawTextBlank(const std::string &text, IFont *font) override;
	void DrawTextWithTransform(const std::string &text, IFont *font, mat4 modelTransform) override;
	void DrawTextUI(const std::string &text, IFont *font, DrawTextUIArgs args) override;

	void SetViewportStyle(SetViewportStyleArgs args, rect *r = nullptr) override;
	void Clear() override;

	inline IBatchRenderer &BatchRenderer2D() override { return m_batch2D; }
	inline IBatchRenderer &BatchRendererUI() override { return m_batchUI; }

  private:
	GLShader m_default2dshader;
	GLMesh m_default2dmesh;

	GLShader m_defaultSolidColorShader;

	GLShader m_defaultFullscreenShader;
	GLMesh m_defaultFullscreenMesh;

	GLShader m_defaultUITextShader;
	GLBuffer m_UITextBuffer;
	GLVertexArray m_UITextArray;

	BatchRendererGL m_batch2D;
	BatchRendererGL m_batchUI;
};

} // namespace gfx
} // namespace sowa

#endif // SW_GRAPHICS_GL_HPP_