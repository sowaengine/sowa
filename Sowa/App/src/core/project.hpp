#ifndef _E_PROJECT_HPP__
#define _E_PROJECT_HPP__
#pragma once

#include "core/engine_context.hpp"
#include "utils/math.hpp"
#include "serialize/serializer.hpp"

#include "yaml-cpp/yaml.h"


namespace Sowa {
class Project {
  public:
	static Project &Of(EngineContext *context);

	bool Load(const char *path);
	bool Save();

  private:
	EngineContext &_Context;

	std::filesystem::path _ProjectPath{""};

	YamlNode m_Doc{};

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
				Size windowsize{1280, 720};
				Size videosize{1920, 1080};
			} window;
		} settings;
	} proj;

  private:
	friend class Application;
	friend class Serializer<Project>;
	Project(EngineContext &ctx);
	~Project();

  public:
	Project(const Project &) = delete;
	Project(const Project &&) = delete;
	Project &operator=(const Project &) = delete;
	Project &operator=(const Project &&) = delete;
};

template <>
YamlNode Serializer<Project>::Save(const Project &in);
template <>
bool Serializer<Project>::Load(Project &out, const YAML::Node &doc);


} // namespace Sowa

#endif // _E_PROJECT_HPP__