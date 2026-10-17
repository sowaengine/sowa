#include "editor/project_manager.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <yaml-cpp/yaml.h>

#include "core/project_settings.hpp"

namespace sowa::editor {
namespace fs = std::filesystem;

ProjectManager::ProjectManager(fs::path registry) : _registry(std::move(registry)) {
}

fs::path ProjectManager::DefaultRegistryPath() {
#ifdef _WIN32
	if (const char *base = std::getenv("APPDATA")) {
		return fs::path(base) / "Sowa" / "projects.yaml";
	}
#else
	if (const char *base = std::getenv("XDG_CONFIG_HOME")) {
		return fs::path(base) / "sowa" / "projects.yaml";
	}
	if (const char *base = std::getenv("HOME")) {
#ifdef __APPLE__
		return fs::path(base) / "Library/Application Support/Sowa/projects.yaml";
#else
		return fs::path(base) / ".config/sowa/projects.yaml";
#endif
	}
#endif
	return fs::current_path() / "sowa-projects.yaml";
}

bool ProjectManager::Load(std::string &error) {
	error.clear();
	try {
		std::vector<ManagedProject> projects;
		if (fs::exists(_registry)) {
			auto doc = YAML::LoadFile(_registry.string());
			if (!doc.IsMap() || !doc["projects"].IsSequence()) {
				error = "Invalid project registry: " + _registry.string();
				return false;
			}
			for (auto record : doc["projects"]) {
				ManagedProject project;
				project.path = record["path"].as<std::string>();
				project.name = record["name"].as<std::string>("Untitled");
				project.editorVersion = record["editorVersion"].as<std::string>("");
				project.lastOpened = record["lastOpened"].as<std::string>("");
				if (std::none_of(projects.begin(), projects.end(),
						[&](const auto &p) { return p.path == project.path; })) {
					projects.push_back(std::move(project));
				}
			}
		}
		_projects = std::move(projects);
		return true;
	} catch (const std::exception &ex) {
		error = ex.what();
		return false;
	}
}

bool ProjectManager::save(std::string &error) {
	try {
		YAML::Node doc;
		doc["version"] = 1;
		doc["projects"] = YAML::Node(YAML::NodeType::Sequence);
		for (const auto &project : _projects) {
			YAML::Node record;
			record["path"] = project.path;
			record["name"] = project.name;
			record["editorVersion"] = project.editorVersion;
			record["lastOpened"] = project.lastOpened;
			doc["projects"].push_back(record);
		}
		if (!_registry.parent_path().empty()) {
			fs::create_directories(_registry.parent_path());
		}
		auto temporary = _registry;
		temporary += ".tmp";
		std::ofstream out(temporary, std::ios::trunc);
		out << doc;
		out.close();
		if (!out) {
			error = "Cannot write project registry: " + temporary.string();
			return false;
		}
		fs::rename(temporary, _registry);
		error.clear();
		return true;
	} catch (const std::exception &ex) {
		error = ex.what();
		return false;
	}
}

bool ProjectManager::Inspect(const fs::path &path, ManagedProject &project, std::string &error) {
	error.clear();
	try {
		fs::path directory = path;
		if (directory.filename() == "project.sowa") {
			directory = directory.parent_path();
		}
		if (directory.empty() || !fs::is_directory(directory) ||
			!fs::is_regular_file(directory / "project.sowa")) {
			error = "Choose a project folder containing project.sowa.";
			return false;
		}
		FileSystem files;
		files.MountFolder("res", directory.string());
		ProjectSettings settings;
		if (!settings.Load(files)) {
			error = "Cannot read project.sowa. Check that it contains valid project settings.";
			return false;
		}
		project.path = fs::canonical(directory).string();
		project.name = settings.application.name.empty() ? directory.filename().string()
														 : settings.application.name;
		return true;
	} catch (const std::exception &ex) {
		error = ex.what();
		return false;
	}
}

bool ProjectManager::Add(const fs::path &path, std::string &error) {
	if (!Load(error)) {
		return false;
	}
	ManagedProject project;
	if (!Inspect(path, project, error)) {
		return false;
	}
	auto before = _projects;
	auto found = std::find_if(_projects.begin(), _projects.end(),
		[&](const auto &p) { return p.path == project.path; });
	if (found == _projects.end()) {
		_projects.insert(_projects.begin(), std::move(project));
	} else {
		found->name = project.name;
	}
	if (save(error)) {
		return true;
	}
	_projects = std::move(before);
	return false;
}

bool ProjectManager::Create(const fs::path &directory, const std::string &name,
	std::string &error) {
	error.clear();
	if (name.empty() || name.find_first_not_of(" \t\r\n") == std::string::npos) {
		error = "Enter a project name.";
		return false;
	}
	try {
		if (directory.empty() || fs::exists(directory)) {
			error = "Choose a new folder. Existing folders are never overwritten.";
			return false;
		}
		fs::create_directories(directory);
		fs::create_directory(directory / "scenes");
		fs::create_directory(directory / "scripts");
		fs::create_directory(directory / "assets");
		FileSystem files;
		files.MountFolder("res", directory.string());
		ProjectSettings settings;
		settings.application.name = name;
		settings.application.mainScene = "res://scenes/main.scene";
		std::ofstream scene(directory / "scenes/main.scene");
		scene << "version: 1\nroot:\n  name: Main\n  components:\n    SceneRoot: {}\n";
		scene.close();
		// Exports and their manifests (see build/) are generated output.
		std::ofstream gitignore(directory / ".gitignore");
		gitignore << "/export/\n/.sowa/\n";
		gitignore.close();
		if (!scene || !gitignore || !settings.Save(files)) {
			error = "Could not write the new project. Check folder permissions.";
			return false;
		}
		if (!Add(directory, error)) {
			error =
				"Project created at " + directory.string() + ", but registration failed: " + error;
			return false;
		}
		return true;
	} catch (const std::exception &ex) {
		error = ex.what();
		return false;
	}
}

bool ProjectManager::Remove(std::string path, std::string &error) {
	if (!Load(error)) {
		return false;
	}
	auto before = _projects;
	std::erase_if(_projects, [&](const auto &project) { return project.path == path; });
	if (save(error)) {
		return true;
	}
	_projects = std::move(before);
	return false;
}

bool ProjectManager::MarkOpened(std::string path, const std::string &version, std::string &error) {
	if (!Load(error)) {
		return false;
	}
	auto before = _projects;
	auto found = std::find_if(_projects.begin(), _projects.end(),
		[&](const auto &p) { return p.path == path; });
	if (found == _projects.end()) {
		error = "Project is not registered.";
		return false;
	}
	ManagedProject refreshed = *found;
	if (!Inspect(path, refreshed, error)) {
		return false;
	}
	refreshed.editorVersion = version;
	const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::ostringstream time;
	time << std::put_time(std::gmtime(&now), "%Y-%m-%d %H:%M UTC");
	refreshed.lastOpened = time.str();
	_projects.erase(found);
	_projects.insert(_projects.begin(), std::move(refreshed));
	if (save(error)) {
		return true;
	}
	_projects = std::move(before);
	return false;
}

} // namespace sowa::editor
