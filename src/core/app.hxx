#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/command/command_interface.hxx"
#include "core/error/error.hxx"
#include "core/graphics.hxx"

#include "data/project_settings.hxx"

#include <filesystem>

#include "data/input_event/input_event.hxx"
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
	inline eventpp::CallbackList<void(InputEventChar)> &CharCallback() { return m_charCallback; }

	inline eventpp::CallbackList<void(int, int)> &WindowResizeCallback() { return m_windowResizeCallback; }

	void SetCurrentScene(Scene *scene);
	inline Scene *GetCurrentScene() { return m_pCurrentScene; }

	Font &TestFont() { return m_testFont; }

	inline bool IsRunning() const { return m_running; }
	void Start();
	void Stop();

	void RegisterCommand(std::string command, std::function<void()> action);
	inline const std::vector<CommandOption> &Commands() { return m_commands; }
	void SetCommandInterface(CommandInterface *interface);

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

	CommandInterface *m_commandInterface = nullptr;
	std::vector<CommandOption> m_commands;

	BatchRenderer m_batchRenderer;
	NewTree m_uiTree;

	eventpp::CallbackList<void(InputEventMouseButton)> m_mouseInputCallback;
	eventpp::CallbackList<void(InputEventMouseMove)> m_mouseMoveCallback;
	eventpp::CallbackList<void(InputEventKey)> m_keyCallback;
	eventpp::CallbackList<void(InputEventChar)> m_charCallback;

	eventpp::CallbackList<void(int, int)> m_windowResizeCallback;

	bool m_running = false;

	friend class FileServer;
	std::filesystem::path m_appPath = "";
};

#endif // SW_APP_HXX