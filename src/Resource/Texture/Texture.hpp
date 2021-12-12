/**
 * @file Texture.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#include "../Resource.hpp"


namespace Ease
{

class Texture : public Resource
{
   public:
      ~Texture();

   private:
      friend class cl_ResourceManager;
      
      uint32_t m_Width = 0;
      uint32_t m_Height = 0;
      uint32_t m_Channels = 0;

      u_char* m_Pixels = nullptr;

      std::string m_FilePath = "";
};

   
} // namespace Ease
