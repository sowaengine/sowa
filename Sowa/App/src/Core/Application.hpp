#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include "stlpch.hpp"

#include "Core/Window.hpp"

#include "ECS/Components/Camera2D/Camera2D.hpp"
#include "ECS/Components/Transform2D/Transform2D.hpp"
#include "ECS/Scene/Scene.hpp"

#include "Sowa.hpp"
#include "Utils/Math.hpp"

namespace nmGfx {
class Renderer;
}

namespace Sowa {
class EngineContext;

class Application {
  public:
	static Application &get_singleton() {
		static Application app;
		return app;
	}

	void Run(int argc, char const *argv[]);
	void ParseArgs(int argc, char const *argv[]);

	Window &GetWindow() { return _window; }

	void StartGame();
	void UpdateGame();
	void StopGame();
	bool IsRunning() { return _AppRunning; }
	Reference<Sowa::Scene> GetCurrentScene() { return _pCurrentScene; }
	Sowa::Entity &SelectedEntity() { return _SelectedEntity; }

	void ChangeScene(const char *path);

	uint32_t Renderer_GetAlbedoFramebufferID();

	void LaunchApp(const std::string &projectPath);

	void SetEditorCameraPosition(const Vec2 &pos);
	void SetEditorCameraZoom(float zoom);
	const Vec2 &GetEditorCameraPosition() { return _EditorCamera2D.transform.Position(); }
	float GetEditorCameraZoom() { return _EditorCamera2D.camera.Zoom(); }

  private:
	friend class Window;
	friend class Renderer;
	friend class GuiServer;

	Sowa::EngineContext *ctx{nullptr};

	Application();
	~Application();

	std::unique_ptr<nmGfx::Renderer> _renderer;
	Window _window;

	Reference<Sowa::Scene> _GameScene;
	Reference<Sowa::Scene> _CopyScene;
	Reference<Sowa::Scene> _pCurrentScene;

	bool _AppRunning = false;

	Sowa::Entity _SelectedEntity;
	
	std::string _ExecutablePath{""};

	struct {
		Component::Transform2D transform;
		Component::Camera2D camera;
	} _EditorCamera2D;

	struct {
#ifdef SW_EDITOR
		bool editor = true;
#endif
	} argParse;
};
} // namespace Sowa

#endif