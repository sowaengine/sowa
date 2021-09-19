#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include "glad/glad.h"
#include <string>


namespace Ease
{
   enum TextureProperties
   {
      NONE,
      REPEAT,
      CLAMP_TO_EDGE,
      LINEAR,
      CLAMP,
   };

   class Texture
   {
      private:
         unsigned int m_UUID;

         int m_Width, m_Height, m_Bits;
         GLuint m_TextureID;
         std::string m_Filepath;

         /** Texture Properties **/
         TextureProperties min_filter = TextureProperties::LINEAR;
         TextureProperties mag_filter = TextureProperties::LINEAR;
         TextureProperties wrap_s = TextureProperties::CLAMP;
         TextureProperties wrap_t = TextureProperties::CLAMP;

         friend class ResourceManager;
         friend class sceneSerializer;
      public:
         unsigned int getUUID() { return m_UUID; }
         GLuint getTextureID() { return m_TextureID; }

         Texture(const std::string& filepath);
         Texture(uint32_t uuid, const std::string& filepath);
         ~Texture();


         void loadTexture();
         void deleteTexture();

         void bind(unsigned int slot = 0);
         void unbind();

   };
}

#endif // __TEXTURE_HPP__