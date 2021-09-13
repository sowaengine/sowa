#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "glad/glad.h"

namespace Ease
{
   class Texture
   {
      private:
         unsigned int m_UUID;
         GLuint m_TextureID;

         friend class ResourceManager;
      public:
         unsigned int getUUID() { return m_UUID; }
         GLuint getTextureID() { return m_TextureID; }

   };
}

#endif // __TEXTURE_HPP__