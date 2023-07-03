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
	void Delete();

	inline const uint32_t ID() const { return m_id; }
	inline const int Width() const { return m_width; }
	inline const int Height() const { return m_height; }
	inline const int Channels() const { return m_channels; }
	inline const unsigned char *Pixels() const { return m_pixels; }

  private:
	uint32_t m_id = 0;
	int m_width = 0;
	int m_height = 0;
	int m_channels = 0;
	unsigned char *m_pixels = nullptr;

	texture_type_t m_type = TextureType::Texture2D;
};

#endif // SW_TEXTURE_HXX