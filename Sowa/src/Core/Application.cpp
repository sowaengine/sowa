#include "Core/Application.hpp"

#include "Debug.hpp"
#include "Sowa.hpp"

#include <filesystem>
#include <iostream>

#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Core/EngineContext.hpp"
#include "Core/ExportGenerator.hpp"
#include "Core/ProjectSettings.hpp"
#include "Core/Renderer.hpp"
#include "Core/Window.hpp"

#include "ECS/Components/Components.hpp"
#include "ECS/Entity/Entity.hpp"
#include "ECS/Scene/Scene.hpp"
#include "ECS/Systems/Systems.hpp"

#include "imgui-docking/backends/imgui_impl_glfw.h"
#include "imgui-docking/backends/imgui_impl_opengl3.h"
#include "imgui-docking/imgui.h"

#include "Core/Input.hpp"
#include "Utils/Dialog.hpp"
#include "Utils/File.hpp"
#include "Utils/String.hpp"

#include "nmGfx/src/Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"
#include "nmGfx/src/Core/nm_Window.hpp"

#include "Servers/GuiServer/GuiServer.hpp"
#include "Servers/ScriptServer/LuaScriptServer.hpp"

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

	GuiServer *guiServer = GuiServer::CreateServer(this, *ctx);
	ctx->RegisterSingleton<GuiServer>(Sowa::Server::GUISERVER, *guiServer);

	LuaScriptServer *luaScriptServer = LuaScriptServer::CreateServer(*ctx);
	ctx->RegisterSingleton<LuaScriptServer>(Sowa::Server::SCRIPTSERVER_LUA, *luaScriptServer);

	ProjectSettings *projectSettings = ProjectSettings::CreateServer(*ctx);
	ctx->RegisterSingleton<ProjectSettings>(Sowa::Server::PROJECTSETTINGS, *projectSettings);

	Sowa::File::InsertFilepathEndpoint("abs", "./");
	if (!Sowa::File::RegisterDataPath()) {
		Debug::Error("Engine data path not found. exiting");
		return;
	}
	bool project_loaded = false;
	std::string lastArg = argv[0];
	for (int i = 1; i < argc; i++) {
		if (lastArg == "--project") {
			projectSettings->LoadProject(argv[i]);
			project_loaded = true;
		}
		lastArg = argv[i];
	}
	if (!project_loaded)
		projectSettings->LoadProject("./");

	_renderer = std::make_unique<nmGfx::Renderer>();
	_renderer->Init(
		projectSettings->_window.WindowWidth,
		projectSettings->_window.WindowHeight,
		projectSettings->_window.VideoWidth,
		projectSettings->_window.VideoHeight,

		projectSettings->_application.Name.c_str(),
		nmGfx::WindowFlags::NONE);
	_window._windowHandle = &_renderer->GetWindow();

	{
		Reference<ImageTexture> icon = nullptr;

		if (projectSettings->_application.Icon != "") {
			icon = ResourceLoader::get_singleton().LoadResource<ImageTexture>(projectSettings->_application.Icon);

			if (!_window.SetWindowIcon(icon))
				icon = nullptr;
		}

		if (icon == nullptr) {
			icon = ResourceLoader::get_singleton().LoadResourceFromMemory<ImageTexture>(Res::include_res_textures_icon_png_data.data(), Res::include_res_textures_icon_png_data.size());
			_window.SetWindowIcon(icon);
		}
	}

	_renderer->GetData2D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_default2d_glsl_data.data()), Res::include_res_shaders_default2d_glsl_data.size()));
	_renderer->GetData3D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_default3d_glsl_data.data()), Res::include_res_shaders_default3d_glsl_data.size()));
	_renderer->GetDataFullscreen()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_fullscreen_glsl_data.data()), Res::include_res_shaders_fullscreen_glsl_data.size()));
	_renderer->GetData3D()._skyboxShader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_skybox_glsl_data.data()), Res::include_res_shaders_skybox_glsl_data.size()));

	guiServer->InitGui(_renderer->GetWindow().GetGLFWwindow());

	if (projectSettings->_application.MainScene != "")
		_pCurrentScene->LoadFromFile(projectSettings->_application.MainScene.c_str());
	luaScriptServer->InitModules();

#ifndef SW_EDITOR
	StartGame();
#endif

	while (!_window.ShouldClose()) {
		_renderer->GetWindow().PollEvents();

		Component::Transform2D cam2dtc{};
		Component::Camera2D cam2d{};

		if (m_AppRunning) {
			cam2dtc = GetCurrentScene()->CurrentCameraTransform2D();
			cam2d = GetCurrentScene()->CurrentCamera2D();
		}

		if (m_AppRunning) {
			Sowa::Systems::ProcessAll(_pCurrentScene.get(), SystemsFlags::Update_Logic);
		}

		_renderer->Begin2D(
			nmGfx::CalculateModelMatrix(
				glm::vec3{cam2dtc.Position().x, cam2dtc.Position().y, 0.f},
				cam2d.Rotatable() ? cam2dtc.Rotation() : 0.f,
				glm::vec3{cam2d.Zoom(), cam2d.Zoom(), 1.f}),
			{cam2d.Center().x, cam2d.Center().y});

		Sowa::Systems::System_Sprite2D(_pCurrentScene.get());

		_renderer->End2D();

		luaScriptServer->UpdateModules();

		guiServer->BeginGui();

		luaScriptServer->GuiUpdateModules();
		ImGui::Render();

		_renderer->ClearLayers();
		_renderer->Draw2DLayer();

		guiServer->EndGui();

		_renderer->GetWindow().SwapBuffers();
	}
}

void Application::StartGame() {
	if (m_AppRunning)
		return;
	m_AppRunning = true;

	ctx->GetSingleton<ProjectSettings>(Sowa::Server::PROJECTSETTINGS)->debug_draw = false;
	Scene::CopyScene(*_GameScene.get(), *_CopyScene.get());

	_pCurrentScene->StartScene();
	_SelectedEntity.SetRegistry(&_pCurrentScene->m_Registry);
}

void Application::UpdateGame() {
}

void Application::StopGame() {
	if (!m_AppRunning)
		return;
	m_AppRunning = false;

	ctx->GetSingleton<ProjectSettings>(Sowa::Server::PROJECTSETTINGS)->debug_draw = true;
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
	m_AppRunning = false;

	std::filesystem::path project = projectPath;
	project = project.parent_path();

	execl(File::Path("abs://Editor/sowa.editor").string().c_str(), "--project", project.string().c_str(), (char *)0);
}
#elif defined(SW_WINDOWS)
void Application::LaunchApp(const std::string &projectPath) {
	m_AppRunning = false;

	std::filesystem::path project = projectPath;
	project = project.parent_path();

	std::string appPath = File::Path("abs://Editor/sowa.editor.exe").string();
	Debug::Log("{} --project {}", appPath, project.string());
	execl(appPath.c_str(), "--project", project.string().c_str(), (char *)0);
}
#else
#error "Sowa::Application::LaunchApp() is not implemented in current platform"
#endif
} // namespace Sowa
