#include "Core/Application.hpp"

#include "Debug.hpp"
#include "Sowa.hpp"

#include "stlpch.hpp"

#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Core/EngineContext.hpp"
#include "Core/ExportGenerator.hpp"
#include "Core/Renderer.hpp"
#include "Core/Window.hpp"

#include "ECS/Components/Components.hpp"
#include "ECS/Entity/Entity.hpp"
#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

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
	_GameScene = std::make_shared<Sowa::Scene>();
	_CopyScene = std::make_shared<Sowa::Scene>();

	_pCurrentScene = _GameScene;
}

Application::~Application() {
}

void Application::Run(int argc, char const *argv[]) {
	SW_ENTRY()

	_ExecutablePath = argv[0];

	ctx = EngineContext::CreateContext();
	auto __ = Debug::ScopeTimer("Application");

	ctx->RegisterSingleton<Application>(Sowa::Server::APPLICATION, *this);

	Sowa::File::InsertFilepathEndpoint("abs", "./");
	if (!Sowa::File::RegisterDataPath()) {
		Debug::Error("Engine data path not found. exiting");
		return;
	}

	ParseArgs(argc, argv);

	_renderer = std::make_unique<nmGfx::Renderer>();
	_renderer->Init(
		1920, 1080,
		1280, 720,
		// projectSettings->_window.WindowWidth,
		// projectSettings->_window.WindowHeight,
		// projectSettings->_window.VideoWidth,
		// projectSettings->_window.VideoHeight,
		"Sowa Engine",
		// projectSettings->_application.Name.c_str(),
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

		Component::Transform2D cam2dtc{};
		Component::Camera2D cam2d{};

		if (_AppRunning) {
			cam2dtc = GetCurrentScene()->CurrentCameraTransform2D();
			cam2d = GetCurrentScene()->CurrentCamera2D();
		}
#ifdef SW_EDITOR
		else {
			cam2dtc = _EditorCamera2D.transform;
			cam2d = _EditorCamera2D.camera;
		}
#endif

		if (_AppRunning) {
			Sowa::Systems::ProcessAll(_pCurrentScene.get(), SystemsFlags::Update_Logic, *ctx);
		}

		_renderer->Begin2D(
			nmGfx::CalculateModelMatrix(
				glm::vec3{cam2dtc.Position().x, -cam2dtc.Position().y, 0.f},
				cam2d.Rotatable() ? cam2dtc.Rotation() : 0.f,
				glm::vec3{cam2d.Zoom(), cam2d.Zoom(), 1.f}),
			{cam2d.Center().x, cam2d.Center().y},
			{0.2f, 0.2f, 0.2f, 1.f});

		Sowa::Systems::ProcessAll(_pCurrentScene.get(), SystemsFlags::Update_Draw, *ctx);

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
}

void Application::StartGame() {
	if (_AppRunning)
		return;
	_AppRunning = true;

	Scene::CopyScene(*_GameScene.get(), *_CopyScene.get());

	_pCurrentScene->StartScene();
	_SelectedEntity.SetRegistry(&_pCurrentScene->m_Registry);
}

void Application::UpdateGame() {
}

void Application::StopGame() {
	if (!_AppRunning)
		return;
	_AppRunning = false;

	Scene::CopyScene(*_CopyScene.get(), *_GameScene.get());

	_pCurrentScene->StopScene();
	_SelectedEntity.SetRegistry(&_pCurrentScene->m_Registry);
}

void Application::ChangeScene(const char *path) {
	_pCurrentScene->LoadFromFile(path);
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

void Application::SetEditorCameraPosition(const Vec2 &pos) {
	_EditorCamera2D.transform.Position() = pos;
}
void Application::SetEditorCameraZoom(float zoom) {
	_EditorCamera2D.camera.Zoom() = zoom;
}

void Application::ParseArgs(int argc, char const *argv[]) {
	std::vector<std::string> args(argc - 1);
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}

	bool projectLoaded = false;

	std::string lastArg = args[0];
	for (size_t i = 0; i < args.size(); i++) {
		std::string arg = args[i];

		if (lastArg == "--project") {

			projectLoaded = true;
		}
#ifdef SW_EDITOR
		else if (arg == "--no-editor") {
			argParse.editor = false;
		}
#endif

		lastArg = args[i];
	}

	// if (!projectLoaded)
	// 	ctx->GetSingleton<ProjectSettings>(Sowa::Server::PROJECTSETTINGS)->LoadProject("./");
}

} // namespace Sowa
