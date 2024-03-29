#include "font.hxx"

#include <iostream>

#include "ft2build.h"
#include FT_FREETYPE_H

#include "core/rendering/gl.hxx"

#include "servers/file_server.hxx"

static FT_Library GetFreeType() {
	static FT_Library lib = nullptr;
	if (nullptr == lib) {
		FT_Init_FreeType(&lib);
	}

	return lib;
}

Font::~Font() {
	FT_Done_Face(reinterpret_cast<FT_Face>(m_face));
}

ErrorCode Font::LoadTTF(const char *path) {
	Result<file_buffer *> res = FileServer::get().load_file(path);
	if (!res.ok()) {
		return res.error();
	}
	m_buffer = *res.get();

	return load_font();
}

ErrorCode Font::LoadTTF(const file_buffer &buffer) {
	m_buffer = buffer;
	return load_font();
}

uint32_t Font::GetGlyphTextureID(int codepoint) {
	return m_characters[codepoint].textureID;
}

glm::vec2 Font::CalcTextSize(const std::string &text) {
	glm::vec2 size{0.f, 0.f};

	float scale = 1.f;
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++) {
		Font::Character ch = m_characters[*c];
		size.x += (ch.advance >> 6) * scale;

		if (ch.size.y > size.y) {
			size.y = ch.size.y;
		}
	}

	return size;
}

ErrorCode Font::load_font() {
	FT_Library freetype = GetFreeType();

	if (FT_New_Memory_Face(freetype, m_buffer.data(), m_buffer.size(), 0, reinterpret_cast<FT_Face *>(&m_face))) {
		return ERR_FAILED;
	}
	FT_Set_Pixel_Sizes(reinterpret_cast<FT_Face>(m_face), 0, 48);

	if (FT_Load_Char(reinterpret_cast<FT_Face>(m_face), 'X', FT_LOAD_RENDER)) {
		return ERR_FAILED;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int c = 0; c < 128; c++) {
		load_char(c);
	}

	return OK;
}

void Font::load_char(int codepoint) {
	FT_Face face = reinterpret_cast<FT_Face>(m_face);
	FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);

	if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT)) {
		return;
	}
	if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
		return;
	}

	uint32_t texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	Font::Character character;
	character.textureID = texture;
	character.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
	character.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
	character.advance = static_cast<uint32_t>(face->glyph->advance.x);

	m_characters[codepoint] = character;
}