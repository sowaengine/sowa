#ifndef SW_TEXTURE_GL_HPP_
#define SW_TEXTURE_GL_HPP_

#include <cstdint>

namespace sowa {
namespace gfx {

enum class GLTextureType {
	Texture2D = 0,
};

enum class GLTextureFormat {
	Rgba = 0,
	Rgb,
	RedInteger,
};

// Data stored in gpu
enum class GLTextureInternalFormat {
	Rgba = 0,
	Rgba16F,
	R32I,
};

enum class GLDataType {
	Float = 0,
	Int,
	UInt,
	UByte,
};

class GLTexture {
  public:
	GLTexture() = default;
	~GLTexture();

	void Bind(int slot = 0);
	void Unbind();

	bool Load2D(const unsigned char *data, size_t size, GLTextureInternalFormat internalFormat = GLTextureInternalFormat::Rgba);
	bool Load2DFromData(unsigned char *data, int width, int height, GLDataType dataType, GLTextureFormat format, GLTextureInternalFormat internalFormat = GLTextureInternalFormat::Rgba);
	void Delete();

	inline const uint32_t ID() const { return m_id; }
	
	inline const int Width() const { return m_width; }
	inline const int Height() const { return m_height; }
	inline const int Channels() const { return m_channels; }
	inline unsigned char* Pixels() const { return m_pixels; }

  private:
	uint32_t m_id = 0;
	int m_width = 0;
	int m_height = 0;
	int m_channels = 0;
	unsigned char *m_pixels = nullptr;

	GLTextureType m_type = GLTextureType::Texture2D;
};
} // namespace gfx
} // namespace sowa

#endif // SW_TEXTURE_2D_GL_HPP_