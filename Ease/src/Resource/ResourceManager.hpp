#ifndef _E_RESOURCEMANAGER_HPP__
#define _E_RESOURCEMANAGER_HPP__

#pragma once

#include <map>
#include <memory>

#include "Debug.hpp"
#include "Ease.hpp"

#include "Resource/EditorTheme/EditorTheme.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/SpriteSheetAnimation/SpriteSheetAnimation.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Utils/Random.hpp"

namespace Ease {
template <typename T>
class ResourceManager {
  public:
	static_assert(std::is_base_of<BaseResource, T>::value, "T must inherit from BaseResource");

	ResourceManager() {}
	~ResourceManager() {}

	Reference<T> LoadResource(const char *path, ResourceID _id = 0) {
		Reference<T> resource = ResourceLoader::get_singleton().LoadResource<T>(path);
		if (resource == nullptr)
			return nullptr;

		ResourceID id = NewResourceID(_id);
		Debug::Log("Load resource at {} id {}", path, id);
		resource->SetResourceID(id);
		resource->SetFilepath(path);
		m_Resources[id] = resource;
		return resource;
	}
	bool SaveResource(const char *path, Reference<T> resource);
	bool DeleteResource(ResourceID id) {
		Reference<T> res = m_Resources[id];
		m_Resources[id] = nullptr;
		delete res;
		return true;
	}

	bool HasResource(ResourceID id) {
		for (const auto &[resID, res] : m_Resources) {
			if (resID == id)
				return true;
		}
		return false;
		// return m_Resources.count(id) == 1;
	}
	Reference<T> &GetResource(ResourceID id) {
		return m_Resources[id];
	}

	inline std::map<ResourceID, Reference<T>> &GetResources() { return m_Resources; }

  private:
	std::map<ResourceID, Reference<T>> m_Resources;

	// Generats new resource id (incrementing on each call). if baseID is non-zero, returns baseID
	ResourceID NewResourceID(ResourceID baseID) {
		ResourceID id = (baseID != 0 && !HasResource(baseID)) ? baseID : Random::GenerateID();
		if (HasResource(id))
			return NewResourceID(id);

		return id;
	}
};

// todo: ResourceSaver
template <>
bool ResourceManager<Ease::EditorTheme>::SaveResource(const char *path, std::shared_ptr<Ease::EditorTheme> resource);
} // namespace Ease

#endif