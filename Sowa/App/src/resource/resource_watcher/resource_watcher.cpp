#include "resource_watcher.hpp"
#include "stlpch.hpp"

#include "serialize/serializer.hpp"

#include "sys/stat.h"
#include "utils/file.hpp"

namespace Sowa {
ResourceWatcher::ResourceWatcher(const std::filesystem::path &rootPath) : m_RootPath(rootPath) {
	std::cout << "ResourceWatcher initialized " << rootPath.string() << std::endl;
}

static time_t GetModifyTime(const char *path) {
	struct stat64 st;
	stat64(File::Path(path).string().c_str(), &st);
	return st.st_mtim.tv_sec;
}

void ResourceWatcher::Register(const std::string &path, const std::weak_ptr<ObjectType> &o) {
	m_RegisteredResources[path].o = o;
	m_RegisteredResources[path].lastModify = GetModifyTime(path.c_str());
}

ResourceWatcher::~ResourceWatcher() {
}

void ResourceWatcher::Poll() {
	for (auto &[path, o] : m_RegisteredResources) {
		auto obj = o.o.lock();
		if (obj == nullptr) {
			Debug::Log("Object {} was deleted", path);
		}

		time_t modifyTime = GetModifyTime(path.c_str());
		if (o.lastModify != modifyTime) {

			std::cout << File::GetFileContent(path.c_str()).size() << std::endl;

			Debug::Log("Reloading {}...", path);
			if (Serializer::get_singleton().Load(obj.get(), File::GetFileContent(path.c_str()))) {
				o.lastModify = modifyTime;
			} else {
				Debug::Error("Failed to load resource {}", path);
			}
		}
	}
}
} // namespace Sowa
