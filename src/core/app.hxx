#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/error/error.hxx"
#include "core/graphics.hxx"
#include "data/project_settings.hxx"
#include "ui/ui_tree.hxx"

#include <filesystem>

#include "data/input_event/mouse_button.hxx"

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

	inline eventpp::CallbackList<void(InputEventMouseButton)> &MouseInputCallback() { return m_mouseInputCallback; }

  private:
	void mainLoop();
	static void mainLoopCaller(void *self);

  private:
	Model rectModel;
	Shader mainShader;

	Texture m_testTexture;

	Shader uiShader;

	Model fullscreenModel;
	Shader fullscreenShader;
	RenderLayer m_layer2D;
	RenderLayer m_layerUI;

	project_settings m_projectSettings;

	eventpp::CallbackList<void(InputEventMouseButton)> m_mouseInputCallback;

	UITree m_editorTree;
	int m_hoveringUINode = 0;

	friend class FileServer;
	std::filesystem::path m_appPath = "";
};

#endif // SW_APP_HXX