#include "Resource/Texture/Texture.hpp"
#include "Resource/ResourceLoader.hpp"

namespace Ease
{
   template<>
   std::shared_ptr<Ease::Texture> ResourceLoaderImpl<Ease::Texture>::Load(unsigned char* data, size_t size)
   {
      std::shared_ptr<Ease::Texture> tex = std::make_shared<Ease::Texture>();
      if(!tex->_texture.Load2DFromMemory(data, size))
         return nullptr;
      return tex;
   }

   Texture::Texture()
   {

   }

   Texture::~Texture()
   {

   }
} // namespace Ease