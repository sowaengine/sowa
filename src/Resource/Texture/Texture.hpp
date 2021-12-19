/**
 * @file Texture.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once
#include "../Resource.hpp"
#include "glad/glad.h"

namespace Ease
{

class Texture : public Resource
{
   public:
      ~Texture();

      GLuint TextureID() { return m_TextureID; }
      uint32_t Width() { return m_Width; }
      uint32_t Height() { return m_Height; }

      void Bind(int slot = 0);


   private:
      friend class cl_ResourceManager;
      
      uint32_t m_Width = 0;
      uint32_t m_Height = 0;
      uint32_t m_Channels = 0;

      u_char* m_Pixels = nullptr;

      std::string m_FilePath = "";

      GLuint m_TextureID = 0;
};

   
} // namespace Ease
