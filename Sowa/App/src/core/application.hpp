#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include "stlpch.hpp"

#include "core/window.hpp"

#include "debug.hpp"
#include "scene/node.hpp"
#include "sowa.hpp"
#include "utils/event.hpp"
#include "utils/event/input_event.hpp"
#include "utils/math.hpp"

#include "resource/font/font.hpp"
#include "resource/resource_watcher/resource_watcher.hpp"

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

	bool Init(int argc, char const** argv);
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

	inline void SetAfterRenderCallback(std::function<void()> callback) { _AfterRenderCallback = callback; }

	inline Event<void(InputEvent)> &OnInput() { return _OnInput; }

	glm::mat4 GetCameraTransform();

  private:
	friend class Window;
	friend class Renderer;

	bool ParseArgs(int argc, char const** argv);
	Debug::ScopeTimer _AppTime = Debug::ScopeTimer("Application");

	Sowa::EngineContext *ctx{nullptr};

	Application();
	~Application();

	std::function<void()> _AfterRenderCallback{nullptr};

	std::unordered_map<std::string, std::function<void(Node *)>> _NodeTypeDestructors;
	Reference<Scene> _Scene{nullptr};

	Font _DefaultFont{};

	std::unique_ptr<nmGfx::Renderer> _renderer;
	Window _window;

	int _ResourcePollInterval{60};
	std::unique_ptr<ResourceWatcher> m_ResourceWatcher;

	void Step();

	Event<void(InputEvent)> _OnInput;

	bool _AppRunning = false;
	uint64_t _FrameCount{0};

	int _SceneCollectInterval{240};

	std::string _ExecutablePath{""};

	// Editor
	Vector2 _EditorCameraPos{0.f, 0.f};
	float _EditorCameraZoom{10.f};

	// Used for lerping
	Vector2 _CurrentEditorCameraPos{0.f, 0.f};
	float _CurrentEditorCameraZoom{10.f};

	struct {
		std::string projectPath{"./"};
		std::string logFile{""};
		bool window{true};
		bool autoStart{false};
	} argParse;
};
} // namespace Sowa

#endif