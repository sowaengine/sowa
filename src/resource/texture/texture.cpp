#include "texture.hpp"

#include "util/utils.hpp"

namespace Ease
{
   Texture::Texture(const char* filepath)
   {
      m_UUID = Util::generateUUID();
   }
   Texture::Texture(uint32_t uuid, const char* filepath)
   {
      m_UUID = uuid;
   }

   Texture::~Texture()
   {

   }

}