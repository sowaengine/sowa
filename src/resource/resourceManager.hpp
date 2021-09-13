#ifndef __RESOURCE_MANAGER_HPP__
#define __RESOURCE_MANAGER_HPP__

#include <vector>
#include "resource/texture/texture.hpp"

namespace Ease
{
   class ResourceManager
   {
      std::vector<Texture> m_Textures;
      unsigned int addTexture();
      Texture* getTexture(unsigned int uuid);
      void freeTexture(unsigned int uuid);
      void freeTexture(Texture* texture);
   };
}

#endif // __RESOURCE_MANAGER_HPP__