#ifndef SW_FONT_HXX
#define SW_FONT_HXX
#pragma once

#include <map>
#include <string>

#include "core/error/error.hxx"
#include "data/file_buffer.hxx"
#include "glm/glm.hpp"

#include "resource/resource.hxx"

class Font : public Resource {
  public:
	Font() = default;
	~Font();

	ErrorCode LoadTTF(const char *path);
	uint32_t GetGlyphTextureID(int codepoint);
	glm::vec2 CalcTextSize(const std::string &text);

  private:
	friend class BatchRenderer;

	void loadChar(int codepoint);

	struct Character {
		uint32_t textureID = 0;
		glm::ivec2 size;
		glm::ivec2 bearing;
		uint32_t advance;
	};

	void *m_face = nullptr;
	file_buffer m_buffer;

	std::map<int, Font::Character> m_characters;
};

#endif // SW_FONT_HXX