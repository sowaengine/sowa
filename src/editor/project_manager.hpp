#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sowa::editor {

struct ManagedProject {
	std::string path;
	std::string name;
	std::string editorVersion;
	std::string lastOpened;
};

// Stores known projects and editor version metadata independently of the launcher UI.
class ProjectManager {
  public:
	explicit ProjectManager(std::filesystem::path registry);
	static std::filesystem::path DefaultRegistryPath();
	bool Load(std::string &error);
	bool Add(const std::filesystem::path &path, std::string &error);
	bool Create(const std::filesystem::path &directory, const std::string &name,
		std::string &error);
	bool Remove(std::string path, std::string &error);
	bool MarkOpened(std::string path, const std::string &version, std::string &error);

	const std::vector<ManagedProject> &Projects() const {
		return _projects;
	}

	static bool Inspect(const std::filesystem::path &path, ManagedProject &project,
		std::string &error);

  private:
	bool save(std::string &error);
	std::filesystem::path _registry;
	std::vector<ManagedProject> _projects;
};

// Desktop launcher. Opens editors in independent processes so the manager remains
// available and multiple projects can be open at once.
int RunProjectManager();
bool LaunchProjectManager(std::string &error);

} // namespace sowa::editor
