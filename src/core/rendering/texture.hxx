#ifndef SW_TEXTURE_HXX
#define SW_TEXTURE_HXX
#pragma once

#include "core/error/error.hxx"
#include "gl.hxx"
#include <stdint.h>

namespace TextureType {
const int Texture2D = GL_TEXTURE_2D;
} // namespace TextureType
typedef int texture_type_t;

namespace TextureFormat {
const int RGBA = 0;
const int RGB = 0;
} // namespace TextureFormat

class Texture {
  public:
	Texture() = default;
	~Texture();

	void Bind(int slot = 0);
	void Unbind();

	Error Load(texture_type_t type, const char *path);
	Error Load2DUByteRGBA(unsigned char *data, int width, int height);

	void Delete();

	inline uint32_t ID() const { return m_id; }
	inline int Width() const { return m_width; }
	inline int Height() const { return m_height; }
	inline int Channels() const { return m_channels; }
	inline unsigned char *Pixels() const { return m_pixels; }

  private:
	uint32_t m_id = 0;
	int m_width = 0;
	int m_height = 0;
	int m_channels = 0;
	unsigned char *m_pixels = nullptr;
	bool m_shouldFree = false;

	texture_type_t m_type = TextureType::Texture2D;
};

#endif // SW_TEXTURE_HXX