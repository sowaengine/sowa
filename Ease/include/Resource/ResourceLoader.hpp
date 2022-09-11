#ifndef _E_RESOURCELOADER_HPP__
#define _E_RESOURCELOADER_HPP__
#pragma once

#include "Ease.hpp"
#include "Utils/File.hpp"
#include <memory>

namespace Ease {
template <typename T>
struct ResourceLoaderImpl {
	Reference<T> Load(unsigned char *data, size_t size);
	Reference<T> LoadFromFile(const char *path); // Should only be available on necessary resource types.
	bool Unload(Reference<T> resource);
};

class ResourceLoader {
  public:
	static ResourceLoader &get_singleton() {
		static ResourceLoader app;
		return app;
	}

	template <typename T>
	Reference<T> LoadResource(const std::string &path) {
		std::vector<unsigned char> data = Ease::File::GetFileContent(path.c_str());
		if (data.size() > 0) {
			ResourceLoaderImpl<T> loader;
			return loader.Load(data.data(), data.size());
		}
		return nullptr;
	}

	/**
	 * @brief Should only used when necessary (e.g. loading nativemodules)
	 *
	 * @tparam T
	 * @param path
	 * @return Reference<T>
	 */
	template <typename T>
	Reference<T> LoadResourceFromFile(const std::string &path) {
		std::vector<unsigned char> data = Ease::File::GetFileContent(path.c_str());
		if (data.size() > 0) {
			ResourceLoaderImpl<T> loader;
			return loader.LoadFromFile(Ease::File::CreateTempFile(data.data(), data.size()).c_str());
		}
		return nullptr;
	}

	// Todo: void Poll() that hot reloads resources with Unload() -> Load()

  private:
	ResourceLoader() = default;
	~ResourceLoader() = default;
};
} // namespace Ease

#endif