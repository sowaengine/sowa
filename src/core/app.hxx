#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/error/error.hxx"
#include "core/graphics.hxx"
#include "data/project_settings.hxx"

class App {
  public:
	~App();

	Error Init();
	Error Run();

  private:
	void mainLoop();
	static void mainLoopCaller(void *self);

  private:
	Model rectModel;
	Shader mainShader;
	project_settings m_projectSettings;
};

#endif // SW_APP_HXX