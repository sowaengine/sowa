#include "resource_manager.hxx"
#include "utils/utils.hxx"
#include <filesystem>
#include <iostream>

ResourceManager &ResourceManager::GetInstance() {
	static ResourceManager *manager = new ResourceManager;
	return *manager;
}

void ResourceManager::RegisterResource(std::string extension, ResourceFactory factory) {
	m_resourceFactories[extension] = factory;
}

Resource *ResourceManager::Load(const std::string &path, RID id, ResourceType type) {
	ResourceFactory factory;
	if (id == 0) {
		id = Utils::RandRange(1, RAND_MAX);
	}

	if (type == ResourceType_None) {
		std::filesystem::path p = path;
		std::string extension = p.extension().string();
		if (m_resourceFactories.find(extension) == m_resourceFactories.end())
			return nullptr;

		factory = m_resourceFactories[extension];
	} else {
		for (auto &[ext, factory_] : m_resourceFactories) {
			if (factory_.type == type) {
				factory = factory_;
				break;
			}
		}
	}

	if (factory.loadFunc) {
		Resource *res = new Resource;

		factory.loadFunc(res, path);
		res->m_filepath = path;
		res->m_type = factory.type;
		res->m_resourceID = id;

		m_resources[id] = res;
		return res;
	}

	return nullptr;
}

Resource *ResourceManager::Get(RID id) {
	if (m_resources.find(id) == m_resources.end()) {
		return nullptr;
	}

	return m_resources[id];
}
