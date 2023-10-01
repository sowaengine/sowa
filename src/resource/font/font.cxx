#include "font.hxx"

#include <iostream>

#include "ft2build.h"
#include FT_FREETYPE_H

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
	FT_Library freetype = GetFreeType();

	ErrorCode err = FileServer::get().ReadFileBytes(path, m_buffer);
	if (err != OK) {
		return err;
	}

	if (FT_New_Memory_Face(freetype, m_buffer.data(), m_buffer.size(), 0, reinterpret_cast<FT_Face *>(&m_face))) {
		return ERR_FAILED;
	}
	FT_Set_Pixel_Sizes(reinterpret_cast<FT_Face>(m_face), 0, 48);

	if (FT_Load_Char(reinterpret_cast<FT_Face>(m_face), 'X', FT_LOAD_RENDER)) {
		return ERR_FAILED;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int c = 0; c < 128; c++) {
		loadChar(c);
	}

	return OK;
}

uint32_t Font::GetGlyphTextureID(int codepoint) {
	return m_characters[codepoint].textureID;
}

glm::vec2 Font::CalcTextSize(const std::string &text) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide_string = converter.from_bytes(text);

	glm::vec2 size{0.f, 0.f};

	float scale = 1.f;
	std::wstring::const_iterator c;
	for (c = wide_string.begin(); c != wide_string.end(); c++) {
		Font::Character ch = m_characters[*c];
		size.x += (ch.advance >> 6) * scale;

		if (ch.size.y > size.y) {
			size.y = ch.size.y;
		}
	}

	return size;
}

void Font::loadChar(int codepoint) {
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