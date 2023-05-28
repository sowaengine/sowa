#ifndef SW_FRAMEBUFFER_GL_HPP_
#define SW_FRAMEBUFFER_GL_HPP_

#include "gfx/graphics.hpp"
#include "texture_gl.hpp"
#include <cstdint>
#include <map>

namespace sowa {
namespace gfx {

enum class GLFramebufferTargetType {
	Vec4,
	Int,
	// Depth24_Stencil8,
};

struct GLFramebufferTarget {
	GLFramebufferTargetType type;
	GLTexture texture;
};

class GLFramebuffer {
  public:
	GLFramebuffer() = default;
	~GLFramebuffer();

	void Create(int width, int height);
	void Delete();

	void SetTarget(uint32_t slot, GLFramebufferTargetType type);

	void Bind();
	void Unbind();

	int GetTargetTextureID(int slot);

	int ReadAttachmentInt(int slot, int x, int y);
	void ReadAttachmentColor(int slot, int x, int y, unsigned char color[4]);

	ViewportDrawMode &DrawMode();

  private:
	// slot, target
	std::map<uint32_t, GLFramebufferTarget> m_targets;
	int m_width = 0;
	int m_height = 0;

	uint32_t m_id = 0;
	uint32_t m_rbo = 0;

	ViewportDrawMode m_drawMode = ViewportDrawMode_KeepRatio;
};
} // namespace gfx
} // namespace sowa

#endif // SW_FRAMEBUFFER_GL_HPP_