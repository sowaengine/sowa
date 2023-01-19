#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include "stlpch.hpp"

#include "core/window.hpp"

#include "debug.hpp"
#include "sowa.hpp"
#include "utils/math.hpp"

namespace nmGfx {
class Renderer;
}

namespace Sowa {
class EngineContext;
class Scene;

class Application {
  public:
	static Application &get_singleton() {
		static Application app;
		return app;
	}

	bool Init(int argc, char const *argv[]);
	bool Process();

	Window &GetWindow() { return _window; }

	void StartGame();
	void UpdateGame();
	void StopGame();
	bool IsRunning() { return _AppRunning; }

	void ChangeScene(const char *path);

	uint32_t Renderer_GetAlbedoFramebufferID();

	void LaunchApp(const std::string &projectPath);

	inline Reference<Scene> CurrentScene() { return _Scene; }

  private:
	friend class Window;
	friend class Renderer;

	void ParseArgs(int argc, char const *argv[]);
	Debug::ScopeTimer _AppTime = Debug::ScopeTimer("Application");

	Sowa::EngineContext *ctx{nullptr};

	Application();
	~Application();

	Reference<Scene> _Scene{nullptr};

	std::unique_ptr<nmGfx::Renderer> _renderer;
	Window _window;

	bool _AppRunning = false;

	std::string _ExecutablePath{""};

	struct {
#ifdef SW_EDITOR
		bool editor = true;
#endif
		std::string projectPath{"./"};
		std::string logFile{""};
	} argParse;
};
} // namespace Sowa

#endif