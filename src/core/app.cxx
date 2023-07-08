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
#include "ui/ui_container.hxx"
#include "ui/ui_tree.hxx"

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

	mainShader.Load("res://shaders/main.vs", "res://shaders/main.fs");
	fullscreenShader.Load("res://shaders/fullscreen.vs", "res://shaders/fullscreen.fs");
	uiShader.Load("res://shaders/ui_panel.vs", "res://shaders/ui_panel.fs");

	err = m_testTexture.Load(TextureType::Texture2D, "res://image.png");
	if (err != OK) {
		std::cout << "Failed to load texture: " << err << std::endl;
	}

	m_batchRenderer.Init("res://shaders/batch2d.vs", "res://shaders/batch2d.fs");
	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 800.f, 0.f, 600.f));
	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));

	/*
	NewTree tree;
	tree.Root().SetOrientation(ContainerOrientation::Row);
	tree.Root().SetChildren({60.f, 40.f});

	tree.Root().Child(0)->SetOrientation(ContainerOrientation::Column);
	tree.Root().Child(0)->SetChildren({50.f, 50.f});
	*/

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

	for (float x = 0; x < 100'000; x += 1.f) {
		glm::vec4 points[4] = {
			{-0.5f, +0.5f, 0.f, 1.f},
			{-0.5f, -0.5f, 0.f, 1.f},
			{+0.5f, -0.5f, 0.f, 1.f},
			{+0.5f, +0.5f, 0.f, 1.f}};
		glm::vec2 uvs[4] = {
			{0.f, 1.f},
			{0.f, 0.f},
			{1.f, 0.f},
			{1.f, 1.f}};

		BatchVertex vertices[4];
		for (int i = 0; i < 4; i++) {
			points[i] = glm::mat4(1.f) * points[i];
			glm::mat4 tf = glm::translate(glm::mat4(1.f), {100.f, 100.f, 100.f});
			tf = glm::scale(tf, {2.f, 2.f, 2.f});
			tf = glm::rotate(tf, 1.7f, {0.f, 0.f, 1.f});
			tf = glm::rotate(tf, 1.7f, {0.f, 1.f, 0.f});
			tf = glm::rotate(tf, 1.7f, {1.f, 0.f, 0.f});

			vertices[i].x = (points[i].x * 50) + (x * 40);
			vertices[i].y = (points[i].y * 100) + 500;
			vertices[i].z = 0.f;
			vertices[i].r = 1.f;
			vertices[i].g = 1.f;
			vertices[i].b = 1.f;
			// vertices[i].r = (std::sin((float)(f + x + 3.1f)) * 0.5) + 0.5f;
			// vertices[i].g = (std::sin((float)(f + x + 7.1f)) * 0.5) + 0.5f;
			// vertices[i].b = (std::sin((float)(f + x + 8.1f)) * 0.5) + 0.5f;
			vertices[i].a = 1.f;
			vertices[i].u = uvs[i].x;
			vertices[i].v = uvs[i].y;
			vertices[i].t_id = static_cast<float>(m_testTexture.ID());
			// vertices[i].t_id = static_cast<float>(Renderer().BlankTexture().ID());
			vertices[i].d_id = x;
		}

		Renderer().PushQuad(vertices);
	}

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