#include "core/project_settings.hpp"
#include "ecs/world.hpp"
#include "editor/project_manager.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_serializer.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using namespace sowa;

namespace {
struct ManagerDirectory {
	std::filesystem::path root = std::filesystem::temp_directory_path() / "sowa_manager_tests";

	ManagerDirectory() {
		std::filesystem::remove_all(root);
		std::filesystem::create_directories(root);
	}

	~ManagerDirectory() {
		std::filesystem::remove_all(root);
	}
};
} // namespace

TEST_CASE("project manager creates a loadable project and persists canonical entries") {
	ManagerDirectory dir;
	editor::ProjectManager manager(dir.root / "config/projects.yaml");
	std::string error;
	REQUIRE(manager.Load(error));
	const auto path = dir.root / "My Game";
	REQUIRE(manager.Create(path, "My Game", error));
	REQUIRE(manager.Projects().size() == 1);
	FileSystem files;
	files.MountFolder("res", path.string());
	ProjectSettings settings;
	REQUIRE(settings.Load(files));
	CHECK(settings.application.name == "My Game");
	std::ifstream gitignore(path / ".gitignore");
	const std::string ignored((std::istreambuf_iterator<char>(gitignore)),
		std::istreambuf_iterator<char>());
	CHECK(ignored == "/export/\n/.sowa/\n");
	World world;
	ResourceRegistry resources;
	auto scene = LoadSceneFromFile(files, settings.application.mainScene, world, resources);
	REQUIRE(scene.IsValid());
	UnloadScene(scene, resources);
	REQUIRE(manager.Add(path / "project.sowa", error));
	CHECK(manager.Projects().size() == 1);
	REQUIRE(manager.MarkOpened(manager.Projects().front().path, "1.2.3", error));
	editor::ProjectManager reopened(dir.root / "config/projects.yaml");
	REQUIRE(reopened.Load(error));
	REQUIRE(reopened.Projects().size() == 1);
	CHECK(reopened.Projects().front().editorVersion == "1.2.3");
	CHECK_FALSE(reopened.Projects().front().lastOpened.empty());
	CHECK_FALSE(reopened.Create(path, "Overwrite", error));
	REQUIRE(reopened.Remove(reopened.Projects().front().path, error));
	CHECK(reopened.Projects().empty());
	CHECK(std::filesystem::exists(path / "project.sowa"));
}

TEST_CASE("project manager rejects missing and invalid projects and preserves corrupt registries") {
	ManagerDirectory dir;
	editor::ProjectManager manager(dir.root / "projects.yaml");
	std::string error;
	CHECK_FALSE(manager.Add(dir.root / "missing", error));
	CHECK_FALSE(error.empty());
	CHECK_FALSE(manager.Create(dir.root / "blank", "   ", error));
	std::ofstream(dir.root / "project.sowa") << "[invalid";
	CHECK_FALSE(manager.Add(dir.root, error));
	std::ofstream(dir.root / "projects.yaml") << "projects: invalid";
	CHECK_FALSE(manager.Load(error));
	CHECK_FALSE(error.empty());
	CHECK(manager.Projects().empty());
}
