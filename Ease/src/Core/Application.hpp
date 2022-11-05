#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Core/EngineContext.hpp"
#include "Core/Window.hpp"
#include "ECS/Scene/Scene.hpp"
#include "Ease.hpp"
#include <filesystem>

namespace nmGfx {
class Renderer;
}

namespace Ease {
class Application {
  public:
	static Application &get_singleton() {
		static Application app;
		return app;
	}

	void Run(int argc, char const *argv[]);

	Window &GetWindow() { return _window; }

	void Log(const std::string &message); // Logs given string to console (And Editor Console on editor builds)

	void StartGame();
	void UpdateGame();
	void StopGame();
	bool IsRunning() { return m_AppRunning; }
	Reference<Ease::Scene> GetCurrentScene() { return _pCurrentScene; }
	Ease::Entity &SelectedEntity() { return _SelectedEntity; }

	void ChangeScene(const char *path);

	uint32_t Renderer_GetAlbedoFramebufferID();

  private:
	friend class Window;
	friend class Renderer;

	Ease::EngineContext *ctx{nullptr};

	Application();
	~Application();

	std::unique_ptr<nmGfx::Renderer> _renderer;
	Window _window;

	Reference<Ease::Scene> _GameScene;
	Reference<Ease::Scene> _CopyScene;
	Reference<Ease::Scene> _pCurrentScene;

	bool m_AppRunning = false;

	Ease::Entity _SelectedEntity;
};
} // namespace Ease

#endif