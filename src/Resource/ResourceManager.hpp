#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H
#pragma once

#include <memory>
#include "Resource/TextureResource.hpp"
#include "Resource/LuaScript.hpp"
#include <vector>
#include <unordered_map>

class ResourceManager  
{
	private:
		std::vector<Resource*> m_Resources;
		
		
	public:
		static ResourceManager& get_singleton();
		
		void RemoveResource(uint32_t id);


		ResourceManager();
		~ResourceManager();

		const std::vector<Resource*> GetResources() { return m_Resources; }
		Resource* GetResourceByID(uint32_t id);

		// Giving id of 0 or none results in new id generation
		std::shared_ptr<TextureResource> NewTextureResource(uint32_t id = 0);
		// Giving id of 0 or none results in new id generation
		std::shared_ptr<LuaScript> NewLuaScript(uint32_t id = 0);


		

};
#endif