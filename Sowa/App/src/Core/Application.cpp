#include "Core/Application.hpp"

#include "Debug.hpp"
#include "Sowa.hpp"

#include "stlpch.hpp"

#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Core/EngineContext.hpp"
#include "Core/ExportGenerator.hpp"
#include "Core/Project.hpp"
#include "Core/Renderer.hpp"
#include "Core/Window.hpp"

#include "Core/Input.hpp"
#include "Utils/Dialog.hpp"
#include "Utils/File.hpp"
#include "Utils/String.hpp"

#include "nmGfx/src/Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"
#include "nmGfx/src/Core/nm_Window.hpp"

#include "res/shaders/default2d.glsl.res.hpp"
#include "res/shaders/default3d.glsl.res.hpp"
#include "res/shaders/fullscreen.glsl.res.hpp"
#include "res/shaders/skybox.glsl.res.hpp"

#include "res/textures/icon.png.res.hpp"

#ifdef SW_WINDOWS
#include <windows.h>
#endif

namespace Sowa {
Application::Application() {
}

Application::~Application() {
}

void Application::Run(int argc, char const *argv[]) {
	SW_ENTRY()

	_ExecutablePath = argv[0];

	ctx = EngineContext::CreateContext();
	auto __ = Debug::ScopeTimer("Application");

	ctx->RegisterSingleton<Application>(Sowa::Server::APPLICATION, *this);

	Project *project = new Project(*ctx);
	ctx->RegisterSingleton<Project>(Sowa::Server::PROJECT, *project);

	Sowa::File::InsertFilepathEndpoint("abs", "./");
	if (!Sowa::File::RegisterDataPath()) {
		Debug::Error("Engine data path not found. exiting");
		return;
	}

	ParseArgs(argc, argv);

	_renderer = std::make_unique<nmGfx::Renderer>();
	_renderer->Init(
		project->proj.settings.window.windowsize.x,
		project->proj.settings.window.windowsize.y,
		project->proj.settings.window.videosize.x,
		project->proj.settings.window.videosize.y,
		project->proj.settings.application.name.c_str(),
		nmGfx::WindowFlags::NONE);
	_window._windowHandle = &_renderer->GetWindow();
	_window.InitWindow(_renderer->GetWindow(), *ctx);

	auto icon = ResourceLoader::get_singleton().LoadResourceFromMemory<ImageTexture>(Res::include_res_textures_icon_png_data.data(), Res::include_res_textures_icon_png_data.size());
	_window.SetWindowIcon(icon);

	_renderer->GetData2D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_default2d_glsl_data.data()), Res::include_res_shaders_default2d_glsl_data.size()));
	_renderer->GetData3D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_default3d_glsl_data.data()), Res::include_res_shaders_default3d_glsl_data.size()));
	_renderer->GetDataFullscreen()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_fullscreen_glsl_data.data()), Res::include_res_shaders_fullscreen_glsl_data.size()));
	_renderer->GetData3D()._skyboxShader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_skybox_glsl_data.data()), Res::include_res_shaders_skybox_glsl_data.size()));

	// if (projectSettings->_application.MainScene != "")
	// 	_pCurrentScene->LoadFromFile(projectSettings->_application.MainScene.c_str());

#ifndef SW_EDITOR
	StartGame();
#endif

	while (!_window.ShouldClose()) {
		_window.UpdateEvents();
		_renderer->GetWindow().PollEvents();

#ifdef SW_EDITOR
		else {
			cam2dtc = _EditorCamera2D.transform;
			cam2d = _EditorCamera2D.camera;
		}
#endif

		_renderer->Begin2D(
			nmGfx::CalculateModelMatrix(
				glm::vec3{0.f, 0.f, 0.f},
				0.f,
				glm::vec3{1.f, 1.f, 1.f}),
			{0.5f, 0.5f},
			{0.2f, 0.2f, 0.2f, 1.f});

#ifdef SW_EDITOR
		if (argParse.editor) {
			if (!_AppRunning) {
				Renderer::get_singleton().DrawLine({0.f, 0.f}, {1920.f * 100, 0.f}, 5.f, {1.f, 0.f, 0.f});
				Renderer::get_singleton().DrawLine({0.f, 0.f}, {0.f, -1080.f * 100}, 5.f, {0.f, 1.f, 0.f});
			}
		}
#endif

		_renderer->End2D();

		_renderer->ClearLayers();
		_renderer->Draw2DLayer();

		_renderer->GetWindow().SwapBuffers();
	}

	Project::Of(ctx).Save();
}

void Application::StartGame() {
	if (_AppRunning)
		return;
	_AppRunning = true;
}

void Application::UpdateGame() {
}

void Application::StopGame() {
	if (!_AppRunning)
		return;
	_AppRunning = false;
}

void Application::ChangeScene(const char *path) {
	Debug::Error("Application::ChangeScene() not implemented");
}

uint32_t Application::Renderer_GetAlbedoFramebufferID() {
	return _renderer->GetData2D()._frameBuffer.GetAlbedoID();
}

#if defined(SW_LINUX)
void Application::LaunchApp(const std::string &projectPath) {
	_AppRunning = false;

	std::filesystem::path project = projectPath;
	project = project.parent_path();

	if (execl(_ExecutablePath.c_str(), "--project", project.string().c_str(), "--module", "abs://Editor/Sowa.Editor.lua", (char *)0) == -1) {
		Debug::Error("Failed to launch project");
	}
}
#elif defined(SW_WINDOWS)
void Application::LaunchApp(const std::string &projectPath) {
	std::filesystem::path project = projectPath;
	project = project.parent_path();

	std::string appPath = _ExecutablePath;
	Debug::Log("{} --project {}", appPath, project.string());

	std::string params = Format("--project {} --module abs://Editor/Sowa.Editor.lua", project.string());

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess((LPSTR)appPath.c_str(), (LPSTR)params.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		Debug::Error("Failed to launch project");
	}

	exit(EXIT_SUCCESS);
}
#else
#error "Sowa::Application::LaunchApp() is not implemented in current platform"
#endif

void Application::ParseArgs(int argc, char const *argv[]) {
	std::vector<std::string> args(argc - 1);
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}
	if (args.size() == 0)
		return;

	bool projectLoaded = false;

	std::string lastArg = args[0];
	for (size_t i = 0; i < args.size(); i++) {
		std::string arg = args[i];

		if (lastArg == "--project") {
			Project::Of(ctx).Load(arg.c_str());
			projectLoaded = true;
		}
#ifdef SW_EDITOR
		else if (arg == "--no-editor") {
			argParse.editor = false;
		}
#endif

		lastArg = args[i];
	}

	if (!projectLoaded)
		Project::Of(ctx).Load("./");
}

} // namespace Sowa
