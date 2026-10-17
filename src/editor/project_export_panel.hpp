#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "build/export_targets.hpp"

namespace sowa {
class FileSystem;

namespace build {
class BuildScript;
}
} // namespace sowa

namespace sowa::editor {

class ProjectExportPanel {
  public:
	void Open();
	void Reset();
	// prepare commits editor changes and saves modified scenes before building.
	void Draw(FileSystem &files, bool playing, bool settingsUnsaved,
		const std::function<bool()> &prepare);

  private:
	void reload(FileSystem &files);
	bool saveTargets();
	void drawTarget(build::ExportTarget &target);
	void runExport(const std::function<bool()> &prepare, std::vector<std::string> targets,
		bool all);

	bool _openRequested = false;
	std::filesystem::path _projectRoot;
	build::ExportTargets _targets;
	std::shared_ptr<build::BuildScript> _script;
	std::string _loadError;
	int _selected = -1;
	bool _dirty = false;
	std::string _status;
	bool _succeeded = false;
};

} // namespace sowa::editor
