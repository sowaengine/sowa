#ifndef SW_RESOURCE_WATCHER_HPP_
#define SW_RESOURCE_WATCHER_HPP_

#include "stlpch.hpp"
#include "object_type.hpp"


namespace sowa {
class ResourceWatcher {
  public:
	ResourceWatcher(const std::filesystem::path &rootPath);
	~ResourceWatcher();

	void Poll();

	void Register(const std::string& path, const std::weak_ptr<object_type>& o);

  private:
	std::filesystem::path m_RootPath;
	struct RegisterData {
		std::weak_ptr<object_type> o;
		time_t lastModify;

		RegisterData() : lastModify(0) {}
		RegisterData(std::weak_ptr<object_type> obj) : o(obj), lastModify(0) {}
	};
	std::unordered_map<std::string, RegisterData> m_RegisteredResources;
};
} // namespace sowa

#endif // SW_RESOURCE_WATCHER_HPP_