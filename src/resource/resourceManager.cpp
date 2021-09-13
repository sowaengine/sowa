#include "resourceManager.hpp"

#include "glad/glad.h"


namespace Ease
{
   unsigned int ResourceManager::addTexture()
   {
      Texture& texture = m_Textures.emplace_back();

      return 0;
   }

   Texture* ResourceManager::getTexture(unsigned int uuid)
   {
      for(auto& texture : m_Textures)
      {
         if(texture.getUUID() == uuid)
            return &texture;
      }
      return nullptr;
   }

   void ResourceManager::freeTexture(unsigned int uuid) {
      freeTexture(getTexture(uuid));
   }

   void ResourceManager::freeTexture(Texture* texture){
      // gldeletetextures(texture->getEntityID());
   }

}