#include "texture.hxx"

#include "gl.hxx"
#include "servers/file_server.hxx"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::~Texture() {
	Delete();
}

void Texture::Bind(int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(m_type, m_id);
}
void Texture::Unbind() {
	glBindTexture(m_type, 0);
}

Error Texture::Load(texture_type_t type, const char *path) {
	Delete();

	file_buffer buffer;
	Error err = FileServer::GetInstance().ReadFileBytes(path, buffer);
	if (err != OK) {
		return err;
	}

	stbi_set_flip_vertically_on_load(true);
	m_pixels = stbi_load_from_memory(buffer.data(), buffer.size(), &m_width, &m_height, &m_channels, 4);
	m_shouldFree = true;
	m_type = type;
	if (!m_pixels) {
		Delete();
		return ERR_INVALID_FILE;
	}

	glGenTextures(1, &m_id);
	glBindTexture(m_type, m_id);

	glTexParameteri(m_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(m_type, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
	glGenerateMipmap(m_type);

	return OK;
}

Error Texture::Load2DUByteRGBA(unsigned char *data, int width, int height) {
	Delete();

	m_pixels = nullptr;
	m_shouldFree = false;
	m_type = TextureType::Texture2D;
	m_width = width;
	m_height = height;
	m_channels = 4;

	glGenTextures(1, &m_id);
	glBindTexture(m_type, m_id);

	glTexParameteri(m_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(m_type, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(m_type);

	return OK;
}

void Texture::Delete() {
	if (m_id != 0)
		glDeleteTextures(1, &m_id);

	if (m_pixels != nullptr && m_shouldFree)
		stbi_image_free(m_pixels);

	m_id = 0;
	m_pixels = nullptr;
}
