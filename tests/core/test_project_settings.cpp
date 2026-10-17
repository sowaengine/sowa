#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string_view>

#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/log.hpp"
#include "core/project_settings.hpp"

using namespace sowa;

namespace {

std::filesystem::path makeTempDir() {
	const auto dir =
		std::filesystem::temp_directory_path() / "sowa_project_settings_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	return dir;
}

void writeFile(const std::filesystem::path &path, std::string_view contents) {
	std::ofstream out(path, std::ios::binary);
	out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

} // namespace

TEST_CASE("ProjectSettings::Load reads nested project.sowa") {
	const auto root = makeTempDir();
	writeFile(root / "project.sowa", R"(
Application:
  Name: "Test Project"
  Author: "Sowa Engine"
  Version: "2.4.1"
  Icon: "res://icon.png"
  MainScene: "res://menu.scene"
  Scripts:
    - "res://scripts/main.lua"
Rendering:
  Window:
    Width: 800
    Height: 600
  Viewport:
    Width: 1920
    Height: 1080
    ScaleMode: KeepWidth
Physics:
  TicksPerSecond: 30
GlobalStore:
  lives: "3"
  player_name: Sowa
)");

	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings settings;
	REQUIRE(settings.Load(fs));
	CHECK(settings.application.name == "Test Project");
	CHECK(settings.application.author == "Sowa Engine");
	CHECK(settings.application.version == "2.4.1");
	CHECK(settings.application.icon == "res://icon.png");
	CHECK(settings.application.mainScene == "res://menu.scene");
	REQUIRE(settings.application.scripts.size() == 1);
	CHECK(settings.application.scripts[0] == "res://scripts/main.lua");
	CHECK(settings.rendering.window.width == 800);
	CHECK(settings.rendering.window.height == 600);
	CHECK(settings.rendering.viewport.width == 1920);
	CHECK(settings.rendering.viewport.height == 1080);
	CHECK(settings.rendering.viewport.scaleMode == ViewportScaleMode::KeepWidth);
	CHECK(settings.physics.ticksPerSecond == 30);
	CHECK(settings.globalStore.Get("lives") == "3");
	CHECK(settings.globalStore.Get("player_name") == "Sowa");

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings::Load keeps defaults for missing keys") {
	const auto root = makeTempDir();
	writeFile(root / "project.sowa", R"(
Application:
  Name: "Partial"
)");

	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings settings;
	settings.globalStore.Set("default_key", "kept");
	REQUIRE(settings.Load(fs));
	CHECK(settings.application.name == "Partial");
	CHECK(settings.application.author.empty());
	CHECK(settings.application.version == "0.1.0");
	CHECK(settings.application.icon.empty());
	CHECK(settings.application.mainScene.empty());
	CHECK(settings.application.scripts.empty());
	CHECK(settings.application.autoloads.empty());
	CHECK(settings.rendering.window.width == 1280);
	CHECK(settings.rendering.window.height == 720);
	CHECK(settings.rendering.viewport.width == 1280);
	CHECK(settings.rendering.viewport.height == 720);
	CHECK(settings.rendering.viewport.scaleMode ==
		  ViewportScaleMode::KeepAspect);
	CHECK(settings.physics.ticksPerSecond == 60);
	CHECK(settings.globalStore.Get("default_key") == "kept");

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings::Load fails when project.sowa is missing") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings settings;
	CHECK_FALSE(settings.Load(fs));

	std::filesystem::remove_all(root);
}

// The same Serialize schema must round-trip settings through saving and loading.
TEST_CASE("ProjectSettings round-trips through Save and Load") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings written;
	written.application.name = "Round Trip";
	written.application.author = "Nobody";
	written.application.version = "1.0.0";
	written.application.icon = "res://icon.png";
	written.application.mainScene = "res://start.scene";
	written.application.scripts = {"res://scripts/main.lua"};
	written.application.autoloads = {"res://scenes/hud.scene"};
	written.rendering.window.width = 1600;
	written.rendering.window.height = 900;
	written.rendering.viewport.width = 640;
	written.rendering.viewport.height = 360;
	written.rendering.viewport.scaleMode = ViewportScaleMode::NoResize;
	written.physics.ticksPerSecond = 120;
	written.globalStore.Set("lives", "3");
	written.globalStore.Set("player_name", "Sowa");

	REQUIRE(written.Save(fs));

	ProjectSettings read;
	REQUIRE(read.Load(fs));
	CHECK(read.application.name == "Round Trip");
	CHECK(read.application.author == "Nobody");
	CHECK(read.application.version == "1.0.0");
	CHECK(read.application.icon == "res://icon.png");
	CHECK(read.application.mainScene == "res://start.scene");
	CHECK(read.application.scripts ==
		  std::vector<std::string>{"res://scripts/main.lua"});
	CHECK(read.application.autoloads ==
		  std::vector<std::string>{"res://scenes/hud.scene"});
	CHECK(read.rendering.window.width == 1600);
	CHECK(read.rendering.window.height == 900);
	CHECK(read.rendering.viewport.width == 640);
	CHECK(read.rendering.viewport.height == 360);
	CHECK(read.rendering.viewport.scaleMode == ViewportScaleMode::NoResize);
	CHECK(read.physics.ticksPerSecond == 120);
	CHECK(read.globalStore.Get("lives") == "3");
	CHECK(read.globalStore.Get("player_name") == "Sowa");

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings::Load survives an unknown ScaleMode") {
	const auto root = makeTempDir();
	writeFile(root / "project.sowa", R"(
Application:
  Name: "Still Loads"
Rendering:
  Viewport:
    ScaleMode: Sideways
)");

	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings settings;
	// One bad field is a warning, not a failed project load.
	REQUIRE(settings.Load(fs));
	CHECK(settings.application.name == "Still Loads");
	CHECK(settings.rendering.viewport.scaleMode == ViewportScaleMode::KeepAspect);

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings::Load fails on invalid YAML") {
	const auto root = makeTempDir();
	writeFile(root / "project.sowa", "Application: [unterminated");

	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings settings;
	CHECK_FALSE(settings.Load(fs));

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings round-trips input actions in authored order") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);

	ProjectSettings written;
	written.input.actions = {
		{"jump", {{Key::Space}, {Key::W}, {Key::MouseLeft}}},
		{"quick_save", {{Key::S, InputModifierControl | InputModifierShift}}},
		{"unbound", {}},
	};
	REQUIRE(written.Save(fs));

	ProjectSettings read;
	REQUIRE(read.Load(fs));
	CHECK(read.input.actions == written.input.actions);

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings::Load reads input actions and drops invalid bindings") {
	const auto root = makeTempDir();
	writeFile(root / "project.sowa", R"(
Input:
  Actions:
    move_left: [A, Left]
    save: [Ctrl+S, Ctrl+Nope]
)");

	FileSystem fs;
	fs.MountFolder("res", root);

	Log::Clear();
	ProjectSettings settings;
	settings.input.actions = {{"stale", {{Key::Q}}}};
	REQUIRE(settings.Load(fs));
	REQUIRE(settings.input.actions.size() == 2);
	CHECK(settings.input.actions[0] ==
		  InputAction{"move_left", {{Key::A}, {Key::Left}}});
	CHECK(settings.input.actions[1] == InputAction{"save", {{Key::S, InputModifierControl}}});

	bool warned = false;
	for (const auto &line : Log::GetLines()) {
		warned = warned || (line.severity == Log::Severity::Warn &&
							   line.text.find("Ctrl+Nope") != std::string::npos);
	}
	CHECK(warned);

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings input section: absent keeps actions, empty clears them") {
	const auto root = makeTempDir();
	writeFile(root / "project.sowa", "Application:\n  Name: \"No Input\"\n");

	FileSystem fs;
	fs.MountFolder("res", root);

	// Like every other key, an absent section leaves the value alone.
	ProjectSettings settings;
	settings.input.actions = {{"kept", {{Key::K}}}};
	REQUIRE(settings.Load(fs));
	CHECK(settings.input.actions.size() == 1);

	// Saving no actions still writes the section, so loading it clears.
	ProjectSettings empty;
	REQUIRE(empty.Save(fs));
	REQUIRE(settings.Load(fs));
	CHECK(settings.input.actions.empty());

	std::filesystem::remove_all(root);
}

TEST_CASE("ProjectSettings keeps the previous physics rate for invalid values") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);
	for (const std::string value : {"0", "-30", "fast", "1.5"}) {
		CAPTURE(value);
		writeFile(root / "project.sowa", "Physics:\n  TicksPerSecond: " + value + "\n");
		ProjectSettings settings;
		settings.physics.ticksPerSecond = 30;
		REQUIRE(settings.Load(fs));
		CHECK(settings.physics.ticksPerSecond == 30);
	}
	std::filesystem::remove_all(root);
}
