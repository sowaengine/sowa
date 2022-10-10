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
	Ease::Scene *GetCurrentScene() { return m_pCurrentScene; }
	Ease::Entity GetPickedEntity() { return m_MousePickedEntity; }

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

	Scene m_GameScene;
	Scene m_CopyScene;
	Scene *m_pCurrentScene;

	bool m_AppRunning = false;

	Ease::Entity m_MousePickedEntity;
};
} // namespace Ease

#endif