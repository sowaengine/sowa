#pragma once
#include <vector>
#include "glad/glad.h"

#include "Ease.hpp"
#include "Renderer/Shader/Shader.hpp"

namespace Ease
{

struct TextureImportProperties
{
   uint32_t min_filter = GL_LINEAR;
   uint32_t mag_filter = GL_LINEAR;
   uint32_t wrap_s = GL_CLAMP;
   uint32_t wrap_t = GL_CLAMP;
};

class Texture
{
   
public:
   void setFilePath(const std::string& filepath) { m_FilePath = filepath; }

   void recreateTexture();

   void bind(unsigned int slot = 0) const;

   Texture() {}
   Texture(GLuint textureID, int width, int height, int channels)
   {
      m_TextureID = textureID;
      m_Width = width;
      m_Height = height;
      m_Channels = channels;
   }

   GLuint getTexID() const { return m_TextureID; }
   int getWidth() const { return m_Width; }
   int getHeight() const { return m_Height; }
   int getChannels() const { return m_Channels; }
   bool isValid() const { return m_Valid; }
private:
   friend class GLRenderer;

   std::string m_FilePath;
   GLuint m_TextureID;

   int m_Width;
   int m_Height;
   int m_Channels;

   bool m_Valid = false;
};
   
} // namespace Ease
