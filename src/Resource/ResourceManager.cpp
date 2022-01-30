#include "ResourceManager.hpp"  

#include <memory>
#include "Resource/TextureResource.hpp"
#include "Resource/LuaScript.hpp"
#include <random>
#include <chrono>

#include "Debug.hpp"

ResourceManager::ResourceManager()
{
   auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
   for(int i=0; i<ticks%256; i++)
      rand();
}
	
ResourceManager::~ResourceManager()
{
	
}

Resource* ResourceManager::GetResourceByID(uint32_t id) 
{
   for (Resource* resource : m_Resources)
   {
      if(resource->m_ResourceID == id)
         return resource;
   }
   LOG_ERROR("Resource With id %d doesn't exists!", id);
   return nullptr;
}

static uint32_t GenerateRandom()
{
   return rand();
}

std::shared_ptr<TextureResource> ResourceManager::NewTextureResource(uint32_t id /*= 0*/) 
{
   std::shared_ptr<TextureResource> tex = std::make_shared<TextureResource>();
   tex->m_ResourceID = id == 0 ? GenerateRandom() : id;
   tex->m_Name = "Texture";

   m_Resources.push_back(tex.get());
   return tex;
}

std::shared_ptr<LuaScript> ResourceManager::NewLuaScript(uint32_t id /*= 0*/)
{
   std::shared_ptr<LuaScript> script = std::make_shared<LuaScript>();
   script->m_ResourceID = id == 0 ? GenerateRandom() : id;
   script->m_Name = "LuaScript";

   m_Resources.push_back(script.get());
   return script;
}


ResourceManager& ResourceManager::get_singleton() 
{
   static ResourceManager resManager;
   return resManager;
}


void ResourceManager::RemoveResource(uint32_t id)
{
   size_t i=0;
   while(i < m_Resources.size())
   {
      if(m_Resources[i]->m_ResourceID == id)
         m_Resources.erase(m_Resources.begin()+i);
      else
         i++;
   }

   std::cout << "Has " << m_Resources.size() << " resources" << std::endl;
}