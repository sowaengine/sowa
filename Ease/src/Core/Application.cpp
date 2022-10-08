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
#include "res/Roboto-Medium.ttf.h"
#include "res/imgui.ini.h"

#include "nmGfx/src/Core/Matrix.hpp"
#include "nmGfx/src/Core/Renderer.hpp"
#include "nmGfx/src/Core/Window.hpp"

#include "Servers/GuiServer/GuiServer.hpp"
#include "Servers/ScriptServer/ScriptServerAS.hpp"

namespace Ease {
Application::Application()
	: m_pCurrentScene(&m_GameScene) {
}

Application::~Application() {
}

void Application::Run(int argc, char const *argv[]) {
	SW_ENTRY()

	Ease::EngineContext *ctx = EngineContext::CreateContext();
	auto __ = Debug::ScopeTimer("Application");

	ctx->RegisterSingleton<Application>(Ease::Server::APPLICATION, *this);

	GuiServer *guiServer = GuiServer::CreateServer(this, *ctx);
	ctx->RegisterSingleton<GuiServer>(Ease::Server::GUISERVER, *guiServer);

	ScriptServerAS *scriptServerAS = ScriptServerAS::CreateServer(*ctx);
	ctx->RegisterSingleton<ScriptServerAS>(Ease::Server::SCRIPTSERVER_AS, *scriptServerAS);

	ProjectSettings &projectSettings = ProjectSettings::get_singleton();
	Ease::File::InsertFilepathEndpoint("abs", "./");
	bool project_loaded = false;
	std::string lastArg = argv[0];
	for (int i = 1; i < argc; i++) {
		if (lastArg == "--project") {
			projectSettings.LoadProject(argv[i]);
			project_loaded = true;
		}
		lastArg = argv[i];
	}
	if (!project_loaded)
		projectSettings.LoadProject("./");

	_renderer = std::make_unique<nmGfx::Renderer>();
	_renderer->Init(
		projectSettings._window.WindowWidth,
		projectSettings._window.WindowHeight,
		projectSettings._window.VideoWidth,
		projectSettings._window.VideoHeight,

		projectSettings._application.Name.c_str(),
		nmGfx::WindowFlags::NONE);
	_window._windowHandle = &_renderer->GetWindow();
	{
		std::vector<unsigned char> shader = File::GetFileContent("abs://templates/default2d.glsl");
		_renderer->GetData2D()._shader.LoadText(std::string(reinterpret_cast<char *>(shader.data()), shader.size()));
	}
	{
		std::vector<unsigned char> shader = File::GetFileContent("abs://templates/default3d.glsl");
		_renderer->GetData3D()._shader.LoadText(std::string(reinterpret_cast<char *>(shader.data()), shader.size()));
	}
	{
		std::vector<unsigned char> shader = File::GetFileContent("abs://templates/fullscreen.glsl");
		_renderer->GetDataFullscreen()._shader.LoadText(std::string(reinterpret_cast<char *>(shader.data()), shader.size()));
	}
	{
		std::vector<unsigned char> shader = File::GetFileContent("abs://templates/skybox.glsl");
		_renderer->GetData3D()._skyboxShader.LoadText(std::string(reinterpret_cast<char *>(shader.data()), shader.size()));
	}

	scriptServerAS->BeginContext();
	scriptServerAS->CreateModule("MyModule");
	auto script = File::GetFileContent("abs://test.as");
	scriptServerAS->LoadScript("MyModule", "test.as", script);
	// scriptServerAS->CallFunc("MyModule", "void init()");

	guiServer->InitGui(_renderer->GetWindow().GetGLFWwindow());

	scriptServerAS->InitModules();
	if (projectSettings._application.MainScene != "")
		m_pCurrentScene->LoadFromFile(projectSettings._application.MainScene.c_str());

#ifndef EASE_EDITOR
	StartGame();
#endif

	while (!_window.ShouldClose()) {
		_renderer->GetWindow().PollEvents();
		// Clear window

		Ease::Component::Transform2D &cam2dtc = GetCurrentScene()->CurrentCameraTransform2D();
		_renderer->Begin2D(
			nmGfx::CalculateModelMatrix(glm::vec3{cam2dtc.Position().x, cam2dtc.Position().y, 0.f}, cam2dtc.Rotation(), glm::vec3{cam2dtc.Scale().x, cam2dtc.Scale().y, 1.f}));
		Ease::Systems::System_SpriteRenderer2D(m_pCurrentScene);
		_renderer->End2D();

		if (m_AppRunning) {
			Ease::Systems::ProcessAll(m_pCurrentScene, SystemsFlags::Update_Logic);
		}
		scriptServerAS->UpdateModules();

		guiServer->BeginGui();

		scriptServerAS->GuiUpdateModules();
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

	Ease::ProjectSettings::get_singleton().debug_draw = false;
	Scene::CopyScene(m_GameScene, m_CopyScene);

	m_pCurrentScene->StartScene();
}

void Application::UpdateGame() {
}

void Application::StopGame() {
	if (!m_AppRunning)
		return;
	m_AppRunning = false;

	Ease::ProjectSettings::get_singleton().debug_draw = true;
	Scene::CopyScene(m_CopyScene, m_GameScene);

	m_pCurrentScene->StopScene();
}

void Application::ChangeScene(const char *path) {
	m_pCurrentScene->LoadFromFile(path);
}

uint32_t Application::Renderer_GetAlbedoFramebufferID() {
	return _renderer->GetData2D()._frameBuffer.GetAlbedoID();
}
} // namespace Ease
