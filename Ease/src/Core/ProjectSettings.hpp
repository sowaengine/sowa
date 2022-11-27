#ifndef _E_PROJECTSETTINGS_HPP__
#define _E_PROJECTSETTINGS_HPP__

#pragma once

#include "Core/EngineContext.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace Sowa {
class ProjectSettings {
  public:
	struct {
		std::string Name = "Sowa Engine";
		std::string Description = "";
		std::string MainScene = "";
		std::string Icon = "";
	} _application;
	struct {
		int WindowWidth = 1280;
		int WindowHeight = 720;

		int VideoWidth = 1920;
		int VideoHeight = 1280;

		bool Fullscreen = false;
	} _window;
	struct {
		std::vector<std::string> lua;
	} _modules;

	bool LoadProject(const char *path);
	bool SaveProject();

	std::filesystem::path projectpath{""};

#ifdef EASE_EDITOR
	bool debug_draw = true;
#else
	bool debug_draw = false;
#endif

	ProjectSettings(const ProjectSettings &) = delete;
	ProjectSettings(const ProjectSettings &&) = delete;
	ProjectSettings &operator=(const ProjectSettings &) = delete;
	ProjectSettings &operator=(const ProjectSettings &&) = delete;

  private:
	friend class Application;
	ProjectSettings(EngineContext &ctx);
	~ProjectSettings();

	static ProjectSettings *CreateServer(EngineContext &ctx) {
		return new ProjectSettings(ctx);
	}

	EngineContext &_Context;
};
} // namespace Sowa

#endif