#include "ResourceManager.hpp"
#include <memory>
#include "../Texture/Texture.hpp"
#include "Debug.hpp"

#include "stb/stb_image.h"

#include "Resource/Project/Project.hpp"

#include "Global.hpp"
namespace Ease
{

//static
void cl_ResourceManager::Init()
{
   static cl_ResourceManager resManager;
   Global::ResourceManager = &resManager;
}

std::shared_ptr<Texture> cl_ResourceManager::LoadTexture(const std::string& path)
{
   std::string RelativePath = Global::Project->GetAbsolutePath(path);


   std::shared_ptr<Texture> Tex = std::make_shared<Texture>();
   Tex->m_FilePath = RelativePath;
   

   int w, h, channels;
   Tex->m_Pixels = stbi_load(RelativePath.c_str(), &w, &h, &channels, 0);
	if(!Tex->m_Pixels) {
		LOG_ERROR("Failed To Load Texture: %s", RelativePath.c_str());
      return nullptr;
	}
   Tex->m_ID = GenerateID();
   Tex->m_Width = w;
   Tex->m_Height = h;
	Tex->m_Channels = channels;

   glGenTextures(1, &Tex->m_TextureID);
   glBindTexture(GL_TEXTURE_2D, Tex->m_TextureID);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, Tex->m_Pixels);
   glGenerateMipmap(GL_TEXTURE_2D);

   //stbi_image_free(Tex->m_Pixels);
   LOG("Loaded texture: %d", Tex->TextureID())
   return Tex;
}

void cl_ResourceManager::DestroyResource(ResourceID id)
{
   LOG("Destroyed texture: %d", id)
}

   
} // namespace Ease
