#include "app.hxx"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/projection.hpp"

#include "core/graphics.hxx"
#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "data/toml_document.hxx"

#include "ui/new_container.hxx"
#include "ui/new_tree.hxx"

#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef SW_WEB
#include <emscripten.h>
#endif

static App *s_instance;

App::App() {
	s_instance = this;
}

App::~App() {
}

App &App::GetInstance() {
	return *s_instance;
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

	err = mainShader.Load("res://shaders/main.vs", "res://shaders/main.fs");
	if (err != OK) {
		std::cerr << "Failed to load main shader" << err << std::endl;
	}

	err = fullscreenShader.Load("res://shaders/fullscreen.vs", "res://shaders/fullscreen.fs");
	if (err != OK) {
		std::cerr << "Failed to load fullscreen shader" << std::endl;
	}

	err = uiShader.Load("res://shaders/ui_panel.vs", "res://shaders/ui_panel.fs");
	if (err != OK) {
		std::cerr << "Failed to load ui shader" << std::endl;
	}

	err = m_testTexture.Load(TextureType::Texture2D, "res://image.png");
	if (err != OK) {
		std::cout << "Failed to load texture: " << err << std::endl;
	}

	err = m_batchRenderer.Init("res://shaders/batch2d.vs", "res://shaders/batch2d.fs");
	if (err != OK) {
		std::cout << "Failed to load renderer: " << err << std::endl;
	}

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 800.f, 0.f, 600.f));
	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));

	m_uiTree.Root().SetOrientation(ContainerOrientation::Row);
	m_uiTree.Root().SetChildren({20.f, 55.f, 25.f});

	m_uiTree.Root().Child(1)->SetOrientation(ContainerOrientation::Column);
	m_uiTree.Root().Child(1)->SetChildren({30.f, 70.f});

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

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
	m_layer2D.Clear(0.5f, 0.5f, 0.5f, 1.0f);

	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);

	mainShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_testTexture.ID());

	uiShader.Bind();

	CursorMode cursorMode = CursorMode::Normal;

	double x, y;
	InputServer::GetInstance().GetMousePosition(x, y);
	x *= (1920.f / (float)w);
	y *= (1080.f / (float)h);
	m_hoveredItem = m_layer2D.ReadAttachmentInt(1, x, y);
	// std::cout << m_hoveredItem << std::endl;

	Renderer().Reset();

	static float f = 0.f;
	f += 0.1f;

	static int frame = 0;
	frame++;
	// std::cout << "Frame: " << frame << std::endl;
	static std::chrono::time_point t = std::chrono::system_clock::now();
	if (frame == 60) {
		std::chrono::time_point now = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> ms = now - t;

		std::cout << "took " << ms.count() << "ms" << std::endl;
	}

	/*
	for (float x = 0.f; x < 800; x += 32) {
		for (float y = 0.f; y < 600; y += 32) {

			float sinf = std::sin((x / 32.f + y / 32.f) * f * 0.02f) * 100;
			float cosf = std::cos((x / 32.f + y / 32.f) * f * 0.02f) * 100;
			Renderer().PushQuad(x + sinf, y + cosf, 0.f, 32.f, 32.f, fmod(x * 1.2f, 1.f), fmod(y * 0.2f, 1.f), fmod((x * 1.5f + y * 5.1f), 1.f), 1.f, 1.f, m_testTexture.ID());
		}
	}
	*/

	m_uiTree.Root().DrawLayout(0.f, 0.f, 800.f, 600.f);

	Renderer().End();

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