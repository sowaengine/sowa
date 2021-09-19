#ifndef __RESOURCE_MANAGER_HPP__
#define __RESOURCE_MANAGER_HPP__

#include <vector>
#include "resource/texture/texture.hpp"

namespace Ease
{
   class ResourceManager
   {
      private:
         std::vector<Texture> m_Textures;
      public:
         unsigned int addTexture(const char* filepath);
         unsigned int addTexture(uint32_t uuid, const char* filepath);
         Texture* getTexture(unsigned int uuid);
         void freeTexture(unsigned int uuid);
         void freeTexture(Texture* texture);

         void loadTexture(unsigned int uuid);
   };
}

#endif // __RESOURCE_MANAGER_HPP__