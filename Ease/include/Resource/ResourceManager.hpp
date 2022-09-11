#ifndef _E_RESOURCEMANAGER_HPP__
#define _E_RESOURCEMANAGER_HPP__

#pragma once

#include "Resource/AudioStream/AudioStream.hpp"
#include "Resource/EditorTheme/EditorTheme.hpp"
#include "Resource/NativeModule/NativeModule.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/SpriteSheetAnimation/SpriteSheetAnimation.hpp"
#include "Resource/Texture/Texture.hpp"
#include <map>
#include <memory>

typedef uint32_t ResourceID;

namespace Ease {
template <typename T>
class ResourceManager {
  public:
	ResourceManager() {}
	~ResourceManager() {}

	Reference<T> LoadResource(const char *path, ResourceID _id = 0) {
		Reference<T> resource = ResourceLoader::get_singleton().LoadResource<T>(path);

		ResourceID id = NewResourceID(_id);
		resource->SetResourceID(id);
		m_Resources[id] = resource;
		return resource;
	}
	Reference<T> LoadResourceFromFile(const char *path, ResourceID _id = 0) {
		Reference<T> resource = ResourceLoader::get_singleton().LoadResourceFromFile<T>(path);

		ResourceID id = NewResourceID(_id);
		resource->SetResourceID(id);
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
		static int c = 1;
		return baseID != 0 ? baseID : c++; // (funny)
	}
};

// todo: ResourceSaver
template <>
bool ResourceManager<Ease::EditorTheme>::SaveResource(const char *path, std::shared_ptr<Ease::EditorTheme> resource);
} // namespace Ease

#endif