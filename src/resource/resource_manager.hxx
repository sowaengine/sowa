#ifndef SW_RESOURCE_MANAGER
#define SW_RESOURCE_MANAGER
#pragma once

#include <functional>
#include <unordered_map>

#include "resource.hxx"
#include "resource_type.hxx"

struct ResourceFactory {
	std::function<void(Resource *, std::string)> loadFunc;
	ResourceType type;
	std::string typeName;
};

class ResourceManager {
  public:
	static ResourceManager &GetInstance();

	void RegisterResource(std::string extension, ResourceFactory factory);
	Resource *Load(const std::string &path, RID id = 0, ResourceType type = ResourceType_None);

	Resource *Get(RID id);

	Resource *New(RID id = 0) {
		Resource *res = new Resource;
		if (id == 0) {
			id = 123;
		}

		res->m_resourceID = id;
		m_resources[id] = res;
		return res;
	}

	template <typename T>
	T *GetAs(RID id) {
		Resource *res = Get(id);
		if (nullptr == res)
			return nullptr;

		return res->As<T>();
	}

	inline std::unordered_map<RID, Resource *> &Resources() {
		return m_resources;
	}

  private:
	std::unordered_map<RID, Resource *> m_resources;

	std::unordered_map<std::string, ResourceFactory> m_resourceFactories;
	// "ImageTexture" -> ResourceType_ImageTexture
	std::unordered_map<std::string, ResourceType> m_resouceTypes;
};

#endif // SW_RESOURCE_MANAGER