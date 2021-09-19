#include "texture.hpp"

#include "util/utils.hpp"
#include "stb/stb_image.h"

#include <unordered_map>
#include <iostream>


namespace Ease
{
   Texture::Texture(const std::string& filepath)
   {
      m_UUID = Util::generateUUID();
      m_Filepath = filepath;
      m_Width = 0; m_Height = 0; m_Bits = 0;
   }
   Texture::Texture(uint32_t uuid, const std::string& filepath)
   {
      m_UUID = uuid;
      m_Filepath = filepath;
      m_Width = 0; m_Height = 0; m_Bits = 0;
   }

   Texture::~Texture()
   {
      deleteTexture();
   }

   const std::unordered_map<TextureProperties, int> texPropToGLtable = {
      {TextureProperties::REPEAT       , GL_REPEAT},
      {TextureProperties::CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE},
      {TextureProperties::LINEAR       , GL_LINEAR},
      {TextureProperties::CLAMP        , GL_CLAMP},
   };
   int convertTexPropToGLid(TextureProperties prop, int fallback) {
      auto it = texPropToGLtable.find(prop);
      if(it != texPropToGLtable.end()) {
         return it->second;
      }
      std::cout << "Returning fallback" << std::endl;
      return fallback;
   }
   

   void Texture::loadTexture()
   {
      if(m_Filepath == "") return;

      stbi_set_flip_vertically_on_load(true);
      auto buffer = stbi_load(m_Filepath.c_str(), &m_Width, &m_Height, &m_Bits, 4/*RGBA*/);

      glGenTextures(1, &m_TextureID);
      glBindBuffer(GL_TEXTURE_2D, m_TextureID);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, convertTexPropToGLid(min_filter, GL_LINEAR));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, convertTexPropToGLid(mag_filter, GL_LINEAR));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, convertTexPropToGLid(wrap_s, GL_CLAMP));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, convertTexPropToGLid(wrap_t, GL_CLAMP));
      
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
      glBindTexture(GL_TEXTURE_2D, 0);

      if(buffer)
         stbi_image_free(buffer);
   }

   void Texture::deleteTexture() {
      glDeleteTextures(1, &m_TextureID);
   }


   void Texture::bind(unsigned int slot)
   {
      glActiveTexture(GL_TEXTURE0+slot);
      glBindTexture(GL_TEXTURE_2D, m_TextureID);
   }

   void Texture::unbind() {
      glBindTexture(GL_TEXTURE_2D, 0);
   }

}