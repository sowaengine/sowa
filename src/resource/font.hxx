#ifndef SW_FONT_HXX
#define SW_FONT_HXX
#pragma once

#include <map>
#include <string>

#include "core/error/error.hxx"
#include "glm/glm.hpp"

class Font {
  public:
	Font() = default;
	~Font();

	Error LoadTTF(const char *path);
	uint32_t GetGlyphTextureID(int codepoint);
	glm::vec2 CalcTextSize(const std::string &text);

  private:
	friend class BatchRenderer;

	struct Character {
		uint32_t textureID = 0;
		glm::ivec2 size;
		glm::ivec2 bearing;
		uint32_t advance;
	};

	std::map<int, Font::Character> m_characters;
};

#endif // SW_FONT_HXX