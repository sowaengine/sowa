#include "app.hxx"

#include "core/graphics.hxx"
#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "data/toml_document.hxx"

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

	mainShader.SetVertexSource(GLSL(
		precision mediump float;

		layout(location = 0) in vec3 aPos;
		void main() {
			gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
		}));
	mainShader.SetFragmentSource(GLSL(
		precision mediump float;
		out vec4 FragColor;

		void main() {
			FragColor = vec4(1.0f, 0.6f, 0.2f, 1.0f);
			// gl_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
		}));
	mainShader.Build();

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

void App::mainLoop() {
	InputServer::GetInstance().ProcessInput();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	mainShader.Bind();
	rectModel.Draw();

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