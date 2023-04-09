#ifndef _E_PROJECT_HPP__
#define _E_PROJECT_HPP__
#pragma once

#include "core/engine_context.hpp"
#include "serialize/serializer.hpp"
#include "utils/math.hpp"

#include "object_type.hpp"
#include "yaml-cpp/yaml.h"

namespace Sowa {
class Project : public ObjectType {
  public:
	static Project &Of(EngineContext *context);

	bool Load(const char *path);
	bool Save();

	static std::string Typename() { return "Sowa::Project"; }

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
	EngineContext &_Context;

	std::filesystem::path _ProjectPath{""};

	FileBuffer m_Doc{};

	static FileBuffer SaveImpl(ObjectType *);
	static bool LoadImpl(ObjectType* out, const FileBuffer& doc);

  private:
	friend class Application;
	Project(EngineContext &ctx);
	~Project();

  public:
	Project(const Project &) = delete;
	Project(const Project &&) = delete;
	Project &operator=(const Project &) = delete;
	Project &operator=(const Project &&) = delete;
};

} // namespace Sowa

#endif // _E_PROJECT_HPP__