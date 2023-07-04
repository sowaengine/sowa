#include "app.hxx"

#include "core/graphics.hxx"
#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "data/toml_document.hxx"

#include "ui/ui_container.hxx"
#include "ui/ui_tree.hxx"

#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef SW_WEB
#include <emscripten.h>
#endif

App::~App() {
}

#ifdef SW_WEB
EM_JS(void, sync_fs_from_db, (), {
	Module.timer = false;
	FS.syncfs(
		true, function(err) {
			if (err) {
				console.error("An error occured while syncing fs", err);
			}
			Module.timer = true;
		});
});

EM_JS(bool, check_timer, (), {
	return Module.timer;
});
#endif

Error App::Init() {
#ifndef SW_WEB
	m_appPath = "./project";
#else
	m_appPath = "/app";
#endif
	FileServer::Create(this);

	// Create working dir
#ifdef SW_WEB
	EM_ASM({
		let text = Module.UTF8ToString($0, $1);

		FS.mkdir(text);
		FS.mount(IDBFS, {}, text);
	},
		   m_appPath.string().c_str(), m_appPath.string().size());
	sync_fs_from_db();

	while (!check_timer()) {
		emscripten_sleep(1);
	}
#endif

	Error err = m_projectSettings.Load("res://project.sowa");
	if (err != OK) {
		//
	}

	RenderingServer::GetInstance().CreateWindow(m_projectSettings.window_width, m_projectSettings.window_height, m_projectSettings.app_name);

	m_layer2D.SetTarget(0, RenderLayerTargetType::Vec4);
	m_layer2D.SetTarget(1, RenderLayerTargetType::Int);
	m_layer2D.Create(1920, 1080);

	// m_layerUI.SetTarget(0, RenderLayerTargetType::Vec4);
	// m_layerUI.SetTarget(1, RenderLayerTargetType::Int);
	// m_layerUI.Create(1920, 1080);

#ifdef SW_WEB
	// Update page title
	if (m_projectSettings.app_name != "") {
		EM_ASM({
			document.title = Module.UTF8ToString($0, $1);
			console.log(document.title);
		},
			   m_projectSettings.app_name.c_str(), m_projectSettings.app_name.size());
	}
#endif

	// Initialize rendering
	ModelBuilder::Quad2D(rectModel);
	ModelBuilder::Quad2D(fullscreenModel, 2.f);

	mainShader.Load("res://shaders/main.vs", "res://shaders/main.fs");
	fullscreenShader.Load("res://shaders/fullscreen.vs", "res://shaders/fullscreen.fs");
	uiShader.Load("res://shaders/ui_panel.vs", "res://shaders/ui_panel.fs");

	err = m_testTexture.Load(TextureType::Texture2D, "res://image.png");
	if (err != OK) {
		std::cout << "Failed to load texture: " << err << std::endl;
	}

	m_editorTree.Root().width = "1920px";
	m_editorTree.Root().height = "1080px";
	m_editorTree.Root().anchor = Anchor::Center;
	m_editorTree.Root().layoutModel = LayoutModel::Flex;

	UIContainer &cont = m_editorTree.Root().Children().emplace_back();
	cont.wrap = Wrap::Wrap;
	cont.flexDirection = FlexDirection::Row;
	cont.justifyContent = JustifyContent::Middle;
	cont.layoutModel = LayoutModel::Flex;
	cont.anchor = Anchor::Left;
	cont.width = "27%";
	cont.height = "100%";
	cont.backgroundColor = Color::FromRGB(200, 100, 20);
	cont.padding = Padding::All(5.f);
	cont.id = 1;
	cont.active = true;
	// cont.visible = false;

	UIContainer &inner = m_editorTree.Root().Children().emplace_back();
	inner.wrap = Wrap::Wrap;
	inner.flexDirection = FlexDirection::Row;
	inner.justifyContent = JustifyContent::Middle;
	inner.layoutModel = LayoutModel::Flex;
	// inner.anchor = Anchor::Left;
	inner.width = "73%";
	inner.height = "100%";
	inner.backgroundColor = Color::FromRGB(200, 100, 20);
	inner.padding = Padding::All(5.f);

	m_editorTree.Calculate();

	return OK;
}

Error App::Run() {
#ifdef SW_WEB
	emscripten_set_main_loop_arg(mainLoopCaller, this, 0, 1);
#else
	while (!RenderingServer::GetInstance().WindowShouldClose()) {
		mainLoop();
	}
#endif
	return OK;
}

void App::SetRenderLayer(RenderLayer *renderlayer) {
	if (nullptr == renderlayer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int width, height;
		RenderingServer::GetInstance().GetWindowSize(width, height);
		glViewport(0, 0, width, height);

		return;
	}

	renderlayer->Bind();
}

void App::mainLoop() {
	InputServer::GetInstance().ProcessInput();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SetRenderLayer(&m_layer2D);
	m_layer2D.Clear(0.2f, 0.5f, 0.7f, 1.0f);

	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);
	m_editorTree.Root().width.Number() = w;
	m_editorTree.Root().height.Number() = h;
	m_editorTree.Calculate();

	mainShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_testTexture.ID());

	uiShader.Bind();

	CursorMode cursorMode = CursorMode::Normal;

	m_editorTree.DrawLayout();

	double x, y;
	InputServer::GetInstance().GetMousePosition(x, y);
	x *= (1920.f / (float)w);
	y *= (1080.f / (float)h);
	int id = m_layer2D.ReadAttachmentInt(1, x, y);
	// std::cout << id << std::endl;
	if (id == 1) {
		cursorMode = CursorMode::Pointer;
	}

	SetRenderLayer(nullptr);

	RenderingServer::GetInstance().SetCursorMode(cursorMode);

	fullscreenShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layer2D.GetTargetTextureID(0));
	fullscreenModel.Draw();

	RenderingServer::GetInstance().SwapBuffers();
	InputServer::GetInstance().PollEvents();
}

void App::mainLoopCaller(void *self) {
	static_cast<App *>(self)->mainLoop();
}

extern "C" void Unload() {
#ifdef SW_WEB
	EM_ASM(
		FS.syncfs(
			false, function(err) {
				if (err) {
					alert("An error occured while syncing fs", err);
				} else {
					console.log("Successfully synced");
				}
			}););
#endif
}