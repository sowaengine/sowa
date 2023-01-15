#ifndef _E_PROJECT_HPP__
#define _E_PROJECT_HPP__
#pragma once

#include "Core/EngineContext.hpp"
#include "Utils/Math.hpp"

namespace pugi {
class xml_document;
}

namespace Sowa {
class Project {
  public:
	static Project &Of(EngineContext *context);

	bool Load(const char *path);
	bool Save();

  private:
	EngineContext &_Context;

	std::filesystem::path _ProjectPath{""};

	std::unique_ptr<pugi::xml_document> _Doc{};

	struct {
		struct {
			struct {
				std::string name{"Sowa Engine"};
				std::string desc{""};
				std::string mainscene{""};
				std::string icon{""};
			} application;
			struct {
				bool fullscreen{false};
				Vec2 windowsize{1280.f, 720.f};
				Vec2 videosize{1920.f, 1080.f};
			} window;
		} settings;
	} proj;

  private:
	friend class Application;
	Project(EngineContext &ctx);
	~Project();

	bool v1LoadProject();

  public:
	Project(const Project &) = delete;
	Project(const Project &&) = delete;
	Project &operator=(const Project &) = delete;
	Project &operator=(const Project &&) = delete;
};
} // namespace Sowa

#endif // _E_PROJECT_HPP__