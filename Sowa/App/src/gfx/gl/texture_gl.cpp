#include "texture_gl.hpp"

#include "stb_image.h"
#include "glad/glad.h"

#include "gfx/gl/glfuncs.hpp"
#include "debug.hpp"

namespace sowa
{
namespace gfx
{
GLTexture::~GLTexture() {
    Delete();
}

void GLTexture::Bind(int slot /*= 0*/) {
    GL().activeTexture(slot);
    GL().bindTexture(m_type, m_id);
}
void GLTexture::Unbind() {
    GL().bindTexture(m_type, 0);
}

void GLTexture::SetFlip(bool v) {
    stbi_set_flip_vertically_on_load(v ? 0 : 1);
}

bool GLTexture::Load2D(const unsigned char* data, size_t size, GLTextureInternalFormat internalFormat) {
    Delete();
    m_type = GLTextureType::Texture2D;

    m_pixels = stbi_load_from_memory(data, size, &m_width, &m_height, &m_channels, 0);
    if(!m_pixels) {
        Delete();
        return false;
    }

    GL().genTextures(m_id);
    GL().bindTexture(m_type, m_id);
    GL().texParameteri(m_type, GLTextureWrapParam::WrapS, GLTextureWrap::Repeat);
    GL().texParameteri(m_type, GLTextureWrapParam::WrapT, GLTextureWrap::Repeat);
    GL().texParameteri(m_type, GLTextureFilterParam::MinFilter, GLTextureFilter::Nearest);
    GL().texParameteri(m_type, GLTextureFilterParam::MagFilter, GLTextureFilter::Nearest);

    GLTextureFormat format = GLTextureFormat::Rgba;
    if(m_channels == 4) {
        format = GLTextureFormat::Rgba;
    } else if(m_channels == 3) {
        format = GLTextureFormat::Rgb;
    } else if(m_channels == 1) {
        format = GLTextureFormat::RedInteger;
    }

    GL().texImage2D(m_type, 0, internalFormat, m_width, m_height, 0, GLDataType::UByte, m_pixels, format);
    GL().generateMipmap(m_type);

    return true;
}

bool GLTexture::Load2DFromData(unsigned char* data, int width, int height, GLDataType dataType, GLTextureFormat format, GLTextureInternalFormat internalFormat) {
    Delete();
    m_type = GLTextureType::Texture2D;

    m_pixels = data;
    m_width = width;
    m_height = height;
    m_channels = 0;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    // GL().generateMipmap(m_type);

    return true;
}

void GLTexture::Delete() {
    if(m_id != 0) {
        GL().deleteTexture(m_id);
    }
    if(m_pixels != nullptr) {
        stbi_image_free(m_pixels);
        m_pixels = nullptr;
    }

    m_id = 0;
}
} // namespace gfx
} // namespace sowa
