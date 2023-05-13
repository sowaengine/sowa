#ifndef SW_FONT_GL_HPP_
#define SW_FONT_GL_HPP_

#include <cstdint>
#include <map>

#include "glm/glm.hpp"

#include "../font.hpp"

namespace sowa {
namespace gfx {
class GLFont : public IFont {
  public:
	GLFont() = default;
	~GLFont();

    void LoadTTF(unsigned char* data, size_t size) override;

	uint32_t GetGlyphTextureID(int glyph) override;

	vec2f CalcTextSize(const std::string& text) override;

  private:
	friend class GraphicsGL;

	struct Character {
		uint32_t textureId = 0;
		glm::ivec2 size;
		glm::ivec2 bearing;
		uint32_t advance;
	};

	std::map<int, GLFont::Character> m_characters;
};
} // namespace gfx
} // namespace sowa

#endif // SW_FONT_GL_HPP_