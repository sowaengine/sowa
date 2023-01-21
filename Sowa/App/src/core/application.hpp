#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include "stlpch.hpp"

#include "core/window.hpp"

#include "debug.hpp"
#include "scene/node.hpp"
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

	Reference<Scene> LoadScene(const char *path);

	uint32_t Renderer_GetAlbedoFramebufferID();

	void LaunchApp(const std::string &projectPath);

	inline const Reference<Scene> GetCurrentScene() { return _Scene; }
	void SetCurrentScene(Reference<Scene> scene);

	void RegisterNodeDestructor(const std::string &nodeType, std::function<void(Node *)> dtor);
	void DestructNode(Node *node);

  private:
	friend class Window;
	friend class Renderer;

	void ParseArgs(int argc, char const *argv[]);
	Debug::ScopeTimer _AppTime = Debug::ScopeTimer("Application");

	Sowa::EngineContext *ctx{nullptr};

	Application();
	~Application();

	Reference<Scene> _Scene{nullptr};
	std::unordered_map<std::string, std::function<void(Node *)>> _NodeTypeDestructors;

	std::unique_ptr<nmGfx::Renderer> _renderer;
	Window _window;

	void Step();

	bool _AppRunning = false;
	uint64_t _FrameCount{0};

	int _SceneCollectInterval{240};

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