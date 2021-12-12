#include "Texture.hpp"

#include "stb/stb_image.h"


namespace Ease
{
   
Texture::~Texture()
{
   if(m_Pixels != nullptr)
      stbi_image_free(m_Pixels);
}


} // namespace Ease
