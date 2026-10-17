#pragma once

#include <string>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/serialize/archive.hpp"
#include "core/store.hpp"
#include "core/types.hpp"
#include "render/viewport_scale.hpp"

namespace sowa {

// Top level project config loaded from res://project.sowa (YAML)
class ProjectSettings : public Serializable {
  public:
	struct {
		std::string name;
		std::string author;
		// Shown to players and stamped into exports; build.lua reads it
		// as ctx.project.application.version.
		std::string version = "0.1.0";
		// res:// path of a PNG used as the window icon (Linux/Windows)
		// and, on web exports, the page favicon. Empty means none.
		std::string icon;
		// Startup scene for PlayProject and exported games. Empty means unconfigured;
		// PlayProject refuses to start without it.
		std::string mainScene;
		// Lua modules run once at Init (and again on ReloadModules). They
		// register component types and systems before any scene that names
		// those types is loaded.
		std::vector<std::string> scripts;
		// Scene files instanced at Play, surviving Change, destroyed on Stop.
		// Not loaded in the editor while stopped.
		std::vector<std::string> autoloads;
	} application;

	struct {
		struct {
			i32 width = 1280;
			i32 height = 720;
		} window;

		struct {
			i32 width = 1280;
			i32 height = 720;
			ViewportScaleMode scaleMode = ViewportScaleMode::KeepAspect;
		} viewport;
	} rendering;

	struct {
		// Named input actions, in authored order. Loaded into Input at
		// startup and whenever saved settings are applied.
		std::vector<InputAction> actions;
	} input;

	// Initial project values copied to the live global store at startup.
	StringStore globalStore;

	// Shared schema for saving and loading project settings.
	void Serialize(Archive &archive) override;

	// Returns false for a missing, unreadable, or invalid document.
	// Missing or malformed fields keep their defaults; malformed fields log warnings.
	bool Load(FileSystem &fs);

	// Writes res://project.sowa.
	bool Save(FileSystem &fs);

  private:
	void serializeInput(Archive &archive, const std::vector<std::string> &rootKeys);
};

} // namespace sowa
