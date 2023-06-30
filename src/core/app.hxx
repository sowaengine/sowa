#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/error/error.hxx"
#include "core/graphics.hxx"
#include "data/project_settings.hxx"

#include <filesystem>

class App {
  public:
	~App();

	Error Init();
	Error Run();

	inline project_settings &ProjectSettings() { return m_projectSettings; }

  private:
	void mainLoop();
	static void mainLoopCaller(void *self);

  private:
	Model rectModel;
	Shader mainShader;
	project_settings m_projectSettings;

	friend class FileServer;
	std::filesystem::path m_appPath = "";
};

#endif // SW_APP_HXX