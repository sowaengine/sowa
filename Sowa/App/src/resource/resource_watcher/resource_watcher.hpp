#ifndef SW_RESOURCE_WATCHER_HPP_
#define SW_RESOURCE_WATCHER_HPP_

#include "stlpch.hpp"
#include "object_type.hpp"


namespace Sowa {
class ResourceWatcher {
  public:
	ResourceWatcher(const std::filesystem::path &rootPath);
	~ResourceWatcher();

	void Poll();

	void Register(const std::string& path, const std::weak_ptr<ObjectType>& o);

  private:
	std::filesystem::path m_RootPath;
	struct RegisterData {
		std::weak_ptr<ObjectType> o;
		time_t lastModify;

		RegisterData() : lastModify(0) {}
		RegisterData(std::weak_ptr<ObjectType> obj) : o(obj), lastModify(0) {}
	};
	std::unordered_map<std::string, RegisterData> m_RegisteredResources;
};
} // namespace Sowa

#endif // SW_RESOURCE_WATCHER_HPP_