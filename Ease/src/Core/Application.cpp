#include "Core/Application.hpp"
#include "Debug.hpp"
#include "Ease.hpp"
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

#include "nmGfx/src/Core/Matrix.hpp"
#include "nmGfx/src/Core/Renderer.hpp"
#include "nmGfx/src/Core/Window.hpp"

#include "Servers/GuiServer/GuiServer.hpp"
#include "Servers/ScriptServer/LuaScriptServer.hpp"

#include "res/shaders/default2d.glsl.res.hpp"
#include "res/shaders/default3d.glsl.res.hpp"
#include "res/shaders/fullscreen.glsl.res.hpp"
#include "res/shaders/skybox.glsl.res.hpp"

namespace Ease {
Application::Application() {
	_GameScene = std::make_shared<Ease::Scene>();
	_CopyScene = std::make_shared<Ease::Scene>();

	_pCurrentScene = _GameScene;
}

Application::~Application() {
}

void Application::Run(int argc, char const *argv[]) {
	SW_ENTRY()

	ctx = EngineContext::CreateContext();
	auto __ = Debug::ScopeTimer("Application");

	ctx->RegisterSingleton<Application>(Ease::Server::APPLICATION, *this);

	GuiServer *guiServer = GuiServer::CreateServer(this, *ctx);
	ctx->RegisterSingleton<GuiServer>(Ease::Server::GUISERVER, *guiServer);

	LuaScriptServer *luaScriptServer = LuaScriptServer::CreateServer(*ctx);
	ctx->RegisterSingleton<LuaScriptServer>(Ease::Server::SCRIPTSERVER_LUA, *luaScriptServer);

	ProjectSettings *projectSettings = ProjectSettings::CreateServer(*ctx);
	ctx->RegisterSingleton<ProjectSettings>(Ease::Server::PROJECTSETTINGS, *projectSettings);

	Ease::File::InsertFilepathEndpoint("abs", "./");
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

	_renderer->GetData2D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_default2d_glsl_data.data()), Res::include_res_shaders_default2d_glsl_data.size()));
	_renderer->GetData3D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_default3d_glsl_data.data()), Res::include_res_shaders_default3d_glsl_data.size()));
	_renderer->GetDataFullscreen()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_fullscreen_glsl_data.data()), Res::include_res_shaders_fullscreen_glsl_data.size()));
	_renderer->GetData3D()._skyboxShader.LoadText(std::string(reinterpret_cast<char *>(Res::include_res_shaders_skybox_glsl_data.data()), Res::include_res_shaders_skybox_glsl_data.size()));

	guiServer->InitGui(_renderer->GetWindow().GetGLFWwindow());

	luaScriptServer->InitModules();
	if (projectSettings->_application.MainScene != "")
		_pCurrentScene->LoadFromFile(projectSettings->_application.MainScene.c_str());

#ifndef EASE_EDITOR
	StartGame();
#endif

	Ease::Entity entity = GetCurrentScene()->Create("Test");
	auto &tc = entity.AddComponent<Component::Transform2D>();
	tc.Scale() = {0.25f, 0.25f};
	auto &spr = entity.AddComponent<Component::Sprite2D>();
	spr.Texture() = ResourceLoader::get_singleton().LoadResource<Ease::ImageTexture>("abs://res/icon.png");

	while (!_window.ShouldClose()) {
		_renderer->GetWindow().PollEvents();
		if (IsRunning()) {
			static float f = 0;

			tc.m_Position.x = sin(f) * 200;
			tc.m_Position.y = cos(f) * 200;
			tc.Rotation() += 0.2f;

			f += 0.02f;
		}
		// Clear window

		Ease::Component::Transform2D &cam2dtc = GetCurrentScene()->CurrentCameraTransform2D();
		_renderer->Begin2D(
			nmGfx::CalculateModelMatrix(glm::vec3{cam2dtc.Position().x, cam2dtc.Position().y, 0.f}, cam2dtc.Rotation(), glm::vec3{cam2dtc.Scale().x, cam2dtc.Scale().y, 1.f}));
		Ease::Systems::System_Sprite2D(_pCurrentScene.get());
		_renderer->End2D();

		if (m_AppRunning) {
			Ease::Systems::ProcessAll(_pCurrentScene.get(), SystemsFlags::Update_Logic);
		}
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

void Application::Log(const std::string &message) {
	std::cout << "[APP] " << message << std::endl;

	// static std::shared_ptr<NativeModule> editor = GetModule("Ease.Editor");
	// if (editor != nullptr) {
	// 	editor->GetModule()->userValues["PrintMsg"].str_value = message;
	// 	editor->GetModule()->userFuncs["Print"]();
	// }
}

void Application::StartGame() {
	if (m_AppRunning)
		return;
	m_AppRunning = true;

	ctx->GetSingleton<ProjectSettings>(Ease::Server::PROJECTSETTINGS)->debug_draw = false;
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

	ctx->GetSingleton<ProjectSettings>(Ease::Server::PROJECTSETTINGS)->debug_draw = true;
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
} // namespace Ease
