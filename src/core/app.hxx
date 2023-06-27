#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/error/error.hxx"

#include "core/graphics.hxx"

class App {
  public:
	Error Init();
	Error Run();

  private:
	void mainLoop();

  private:
	Model rectModel;
	Shader mainShader;
};

#endif // SW_APP_HXX