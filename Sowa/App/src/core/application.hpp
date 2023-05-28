#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include "stlpch.hpp"

#include "core/window.hpp"

#include "debug.hpp"
#include "math/math.hpp"
#include "scene/node.hpp"
#include "sowa.hpp"
#include "utils/event.hpp"
#include "utils/event/input_event.hpp"

#include "resource/font/font.hpp"
#include "resource/resource_watcher/resource_watcher.hpp"

#include "gfx/font.hpp"
#include "gfx/gl/framebuffer_gl.hpp"
#include "gfx/shader.hpp"
#include "gfx/window.hpp"
#include "gfx/window_manager.hpp"

class FT_LibraryRec_;

namespace nmGfx {
class Renderer;
}

namespace sowa {
class EngineContext;
class Scene;

class Application {
  public:
	static Application &get_singleton() {
		static Application app;
		return app;
	}

	bool Init(int argc, char const **argv);
	bool Process();

	gfx::Window &GetWindow() { return *m_window; }

	void StartGame();
	void UpdateGame();
	void StopGame();
	bool IsRunning() { return _AppRunning; }

	Reference<Scene> LoadScene(const char *path);

	uint32_t Renderer_GetAlbedoFramebufferID();

	void LaunchApp(const std::string &projectPath);

	inline const Reference<Scene> GetCurrentScene() { return _Scene; }
	void SetCurrentScene(Reference<Scene> scene);

	gfx::IFont *GetDefaultFont() { return m_defaultFont; }

	void RegisterNodeDestructor(const std::string &nodeType, std::function<void(Node *)> dtor);
	void DestructNode(Node *node);

	uint64_t GetFrameCount() { return _FrameCount; }

	// std::unique_ptr<nmGfx::Renderer> &RendererHandle() { return _renderer; }

	inline void SetAfterRenderCallback(std::function<void()> callback) { _AfterRenderCallback = callback; }

	inline Event<void(InputEvent)> &OnInput() { return _OnInput; }

	glm::mat4 GetCameraTransform();

	inline uint32_t PickedNode() { return m_picked_node; }
	inline uint32_t HoveringNode() { return m_hovering_node; }

	void BindProjectionUniform(gfx::IShader &shader, const std::string &uniformName);
	void BindViewUniform(gfx::IShader &shader, const std::string &uniformName);

	FT_LibraryRec_ *GetFTInstance();

  private:
	friend class Window;
	friend class Renderer;

	bool ParseArgs(int argc, char const **argv);
	Debug::ScopeTimer _AppTime = Debug::ScopeTimer("Application");

	sowa::EngineContext *ctx{nullptr};

	Application();
	~Application();

	std::function<void()> _AfterRenderCallback{nullptr};

	mat4 m_Latest2DViewMatrix = mat4(1.f);

	std::unordered_map<std::string, std::function<void(Node *)>> _NodeTypeDestructors;
	Reference<Scene> _Scene{nullptr};

	gfx::IFont *m_defaultFont = nullptr;

	// std::unique_ptr<nmGfx::Renderer> _renderer;
	gfx::Window *m_window;
	gfx::WindowManager m_windowManager;

	int _ResourcePollInterval{60};
	std::unique_ptr<ResourceWatcher> m_ResourceWatcher;

	void Step();

	Event<void(InputEvent)> _OnInput;

	bool _AppRunning = false;
	uint64_t _FrameCount{0};

	int _SceneCollectInterval{240};

	std::string _ExecutablePath{""};

	gfx::GLFramebuffer m_drawpass2d;
	gfx::GLFramebuffer m_finalDrawPass;

	FT_LibraryRec_ *m_ftInstance = nullptr;

	// Editor
	rect m_viewportRect = rect(0.f, 0.f, 0.f, 0.f);
	vec2 _EditorCameraPos{0.f, 0.f};
	float _EditorCameraZoom{10.f};
	uint32_t m_picked_node;
	uint32_t m_hovering_node;
	bool m_is_node_dragging{false};
	bool m_on_drag_mode{false}; // on press G

	// Used for lerping
	vec2 _CurrentEditorCameraPos{0.f, 0.f};
	float _CurrentEditorCameraZoom{10.f};

	struct {
		std::string projectPath{"./"};
		std::string logFile{""};
		bool window{true};
		bool autoStart{false};
	} argParse;
};
} // namespace sowa

#endif