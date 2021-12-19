#include "Texture.hpp"

#include "stb/stb_image.h"


namespace Ease
{
   
Texture::~Texture()
{
   if(m_Pixels != nullptr)
      stbi_image_free(m_Pixels);
}

void Texture::Bind(int slot/* = 0*/)
{
   glActiveTexture(GL_TEXTURE0+slot);
   glBindTexture(GL_TEXTURE_2D, m_TextureID);
}


} // namespace Ease
