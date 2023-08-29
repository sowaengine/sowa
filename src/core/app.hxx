#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/error/error.hxx"
#include "core/graphics.hxx"
#include "data/project_settings.hxx"

#include <filesystem>

#include "data/input_event/input_event.hxx"
#include "data/lrtb_flags.hxx"
#include "resource/font/font.hxx"
#include "ui/new_tree.hxx"

#include "scene/scene.hxx"

#include "eventpp/callbacklist.h"

class App {
  public:
	App();
	~App();

	Error Init();
	Error Run();

	static App &GetInstance();

	inline project_settings &ProjectSettings() { return m_projectSettings; }

	void SetRenderLayer(RenderLayer *renderlayer);

	inline BatchRenderer &Renderer() { return m_batchRenderer; }

	inline eventpp::CallbackList<void(InputEventMouseButton)> &MouseInputCallback() { return m_mouseInputCallback; }
	inline eventpp::CallbackList<void(InputEventMouseMove)> &MouseMoveCallback() { return m_mouseMoveCallback; }
	inline eventpp::CallbackList<void(InputEventKey)> &KeyCallback() { return m_keyCallback; }

	inline eventpp::CallbackList<void(int, int)> &WindowResizeCallback() { return m_windowResizeCallback; }

	void SetCurrentScene(Scene *scene);
	inline Scene *GetCurrentScene() { return m_pCurrentScene; }

	Font &TestFont() { return m_testFont; }

	inline bool IsRunning() const { return m_running; }
	void Start();
	void Stop();

  private:
	void mainLoop();
	static void mainLoopCaller(void *self);

  private:
	Model rectModel;
	Shader mainShader;

	Scene *m_pCurrentScene = nullptr;
	Scene m_backgroundScene;

	Texture m_testTexture;
	Font m_testFont;

	Shader uiShader;

	Model fullscreenModel;
	Shader fullscreenShader;
	RenderLayer m_layer2D;
	RenderLayer m_layerUI;

	project_settings m_projectSettings;

	BatchRenderer m_batchRenderer;
	NewTree m_uiTree;

	eventpp::CallbackList<void(InputEventMouseButton)> m_mouseInputCallback;
	eventpp::CallbackList<void(InputEventMouseMove)> m_mouseMoveCallback;
	eventpp::CallbackList<void(InputEventKey)> m_keyCallback;

	eventpp::CallbackList<void(int, int)> m_windowResizeCallback;

	bool m_running = false;

	friend class FileServer;
	std::filesystem::path m_appPath = "";
};

#endif // SW_APP_HXX