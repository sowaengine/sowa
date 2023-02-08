#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include "stlpch.hpp"

#include "core/window.hpp"

#include "debug.hpp"
#include "scene/node.hpp"
#include "sowa.hpp"
#include "utils/math.hpp"

#include "resource/font/font.hpp"

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

	bool Init(const std::vector<std::string>& args);
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

	Font& GetDefaultFont() { return _DefaultFont; }

	void RegisterNodeDestructor(const std::string &nodeType, std::function<void(Node *)> dtor);
	void DestructNode(Node *node);
	
	uint64_t GetFrameCount() { return _FrameCount; }

	std::unique_ptr<nmGfx::Renderer>& RendererHandle() { return _renderer; }
  private:
	friend class Window;
	friend class Renderer;

	void ParseArgs(const std::vector<std::string>& args);
	Debug::ScopeTimer _AppTime = Debug::ScopeTimer("Application");

	Sowa::EngineContext *ctx{nullptr};

	Application();
	~Application();

	std::unordered_map<std::string, std::function<void(Node *)>> _NodeTypeDestructors;
	Reference<Scene> _Scene{nullptr};

	Font _DefaultFont{};

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
		bool window{true};
	} argParse;
};
} // namespace Sowa

#endif