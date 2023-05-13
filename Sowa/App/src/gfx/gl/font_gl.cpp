#include "glad/glad.h"

#include "font_gl.hpp"

#include "ft2build.h"
#include FT_FREETYPE_H

#include "core/application.hpp"
#include "gfx/gl/glfuncs.hpp"

namespace sowa {
namespace gfx {
GLFont::~GLFont() {
}
void GLFont::LoadTTF(unsigned char *data, size_t size) {
	FT_Library freetype = sowa::Application::get_singleton().GetFTInstance();

	FT_Face face;
	if (FT_New_Memory_Face(freetype, data, size, 0, &face)) {
		return;
	}
	FT_Set_Pixel_Sizes(face, 0, 48);

	if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) {
		return;
	}

	GL().setUnpackAlignment(1);
	for (int c = 0; c < 128; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			continue;
		}

		uint32_t texture = 0;
		GL().genTextures(texture);
		GL().bindTexture(GLTextureType::Texture2D, texture);
		GL().texImage2D(GLTextureType::Texture2D, 0, GLTextureInternalFormat::Red, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GLDataType::UByte, face->glyph->bitmap.buffer, GLTextureFormat::Red);
		GL().texParameteri(GLTextureType::Texture2D, GLTextureWrapParam::WrapS, GLTextureWrap::ClampToEdge);
		GL().texParameteri(GLTextureType::Texture2D, GLTextureWrapParam::WrapT, GLTextureWrap::ClampToEdge);
		GL().texParameteri(GLTextureType::Texture2D, GLTextureFilterParam::MinFilter, GLTextureFilter::Linear);
		GL().texParameteri(GLTextureType::Texture2D, GLTextureFilterParam::MagFilter, GLTextureFilter::Linear);
		GLFont::Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<uint32_t>(face->glyph->advance.x)
		};

		m_characters[c] = character;
	};

	FT_Done_Face(face);
}

uint32_t GLFont::GetGlyphTextureID(int glyph) {
	return m_characters[glyph].textureId;
}

vec2f GLFont::CalcTextSize(const std::string& text) {
	vec2f size{0.f, 0.f};

	float scale = 1.f;
	std::string::const_iterator c;
	for(c = text.begin(); c != text.end(); c++) {
		GLFont::Character ch = m_characters[*c];
		size.x += (ch.advance >> 6) * scale;

		if(ch.size.y > size.y) {
			size.y = ch.size.y;
		}
	}

	return size;
}

} // namespace gfx

} // namespace sowa