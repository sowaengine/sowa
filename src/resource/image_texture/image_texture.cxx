#include "image_texture.hxx"

#include "lunasvg.h"

#include "core/rendering/gl.hxx"
#include "servers/file_server.hxx"
#include "servers/rendering_server.hxx"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::~Texture() {
	Delete();
}

void Texture::Bind(int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(TextureType::GetType(m_type), m_id);
}
void Texture::Unbind() {
	glBindTexture(TextureType::GetType(m_type), 0);
}

Error Texture::Load(texture_type_t type, const char *path) {
	Delete();
	// m_filepath = path;

	file_buffer buffer;
	Error err = FileServer::get().ReadFileBytes(path, buffer);
	if (err != OK) {
		return err;
	}

	if (type == TextureType::Texture2D) {
		return loadTexture2D(buffer);
	}
	if (type == TextureType::Vector2D) {
		return loadVector2D(buffer);
	}
	return ERR_FAILED;
}

Error Texture::Load2DUByteRGBA(unsigned char *data, int width, int height) {
	Delete();
	// m_filepath = "";

	m_pixels = nullptr;
	m_shouldFree = false;
	m_type = TextureType::Texture2D;
	m_width = width;
	m_height = height;
	m_channels = 4;

	glGenTextures(1, &m_id);
	glBindTexture(TextureType::GetType(m_type), m_id);

	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(TextureType::GetType(m_type), 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(TextureType::GetType(m_type));

	return OK;
}

void Texture::Delete() {
	if (m_id != 0)
		if (RenderingServer::get().Active())
			glDeleteTextures(1, &m_id);

	if (m_pixels != nullptr && m_shouldFree)
		stbi_image_free(m_pixels);

	m_id = 0;
	m_pixels = nullptr;
}

Error Texture::loadTexture2D(file_buffer &buffer) {
	stbi_set_flip_vertically_on_load(true);
	m_pixels = stbi_load_from_memory(buffer.data(), buffer.size(), &m_width, &m_height, &m_channels, 4);
	m_shouldFree = true;
	m_type = TextureType::Texture2D;
	if (!m_pixels) {
		Delete();
		return ERR_INVALID_FILE;
	}

	glGenTextures(1, &m_id);
	glBindTexture(TextureType::GetType(m_type), m_id);

	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(TextureType::GetType(m_type), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(TextureType::GetType(m_type), 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
	glGenerateMipmap(TextureType::GetType(m_type));

	return OK;
}

Error Texture::loadVector2D(file_buffer &buffer) {
	using namespace lunasvg;

	std::unique_ptr<Document> document = Document::loadFromData((const char *)buffer.data(), buffer.size());
	Bitmap bitmap = document->renderToBitmap();
	bitmap.convertToRGBA();
	if (!bitmap.valid()) {
		return ERR_INVALID_FILE;
	}
	m_width = bitmap.width();
	m_height = bitmap.height();
	m_channels = 4;
	m_shouldFree = false;
	m_pixels = nullptr;
	m_type = TextureType::Vector2D;

	return Load2DUByteRGBA(bitmap.data(), bitmap.width(), bitmap.height());
}