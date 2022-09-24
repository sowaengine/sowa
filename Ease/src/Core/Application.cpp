#include "Core/Application.hpp"
//#include "Debug.hpp"
#include "Ease.hpp"
#include <iostream>

#include "Core/Renderer.hpp"

#include "Resource/NativeModule/NativeModule.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Resource/ResourceManager.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Core/EngineContext.hpp"
#include "Core/ExportGenerator.hpp"
#include "Core/ProjectSettings.hpp"
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

namespace Ease {
Application::Application()
	: m_pCurrentScene(&m_GameScene) {
}

Application::~Application() {
}

void Application::Run(int argc, char const *argv[]) {
	Ease::EngineContext *ctx = EngineContext::CreateContext();

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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// ImGui::GetIO().WantSaveIniSettings = false;
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(Res::roboto_medium_data.data(), Res::roboto_medium_data.size(), 14.f);
	// ImGui::LoadIniSettingsFromMemory(Res::imgui_default_layout.data(), Res::imgui_default_layout.size());
	ImGui_ImplGlfw_InitForOpenGL(_renderer->GetWindow().GetGLFWwindow(), true);
	ImGui_ImplOpenGL3_Init("#version 150");

	Reference<Ease::EditorTheme> theme = Ease::ResourceLoader::get_singleton().LoadResource<Ease::EditorTheme>("abs://res/theme.escfg");
	ImGui::GetStyle() = theme->GetStyle();

	for (const std::string &path : projectSettings._modules.modules) {
		LoadModule(path, 1);
	}
#ifdef EASE_EDITOR
	LoadModule("abs://Modules/Ease.Editor", 1);
#endif

	InitModules();
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
		UpdateModules();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		Modules_OnImGuiRender();
		ImGui::Render();

		_renderer->ClearLayers();
		_renderer->Draw2DLayer();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

// Initialize loaded native modules
void Application::InitModules() {
	for (auto &pair : m_Modules)
		pair.second->CallStart();
}

void Application::UpdateModules() {
	for (auto &pair : m_Modules)
		pair.second->CallUpdate();
}
void Application::Modules_OnImGuiRender() {
	for (auto &pair : m_Modules)
		pair.second->CallOnImGuiRender();
}

Application::ModuleLoadResult Application::LoadModule(const std::filesystem::path &modulePath, int minimumVersion) {
	static ResourceLoader &moduleLoader = Ease::ResourceLoader::get_singleton();

	Reference<NativeModule> myModule = moduleLoader.LoadResourceFromFile<NativeModule>(NativeModule::AddExtension(modulePath.string()).c_str());

	if (myModule == nullptr)
		return ModuleLoadResult::UNDEFINED_MODULE;
	// if(author != myModule->m_pModule->metadata.authorName || moduleName != myModule->m_pModule->metadata.moduleName)
	//    return ModuleLoadResult::UNDEFINED_MODULE;
	if (myModule->m_pModule->metadata.version < minimumVersion)
		return ModuleLoadResult::OUTDATED_VERSION;

	for (auto &[name, behaviour] : myModule->m_pModule->nativeBehaviours) {
		AddNativeBehaviour(modulePath.filename().concat(".").concat(name).string(), behaviour);
	}

	AddModule(modulePath.filename().string(), myModule);
	return ModuleLoadResult::SUCCESS;
}

void Application::AddModule(const std::string &name, std::shared_ptr<NativeModule> _module) {
	m_Modules[name] = _module;
}

void Application::AddNativeBehaviour(const std::string &name, NativeBehaviourFactory *behaviour) {
	m_NativeBehaviours[name] = behaviour;
}
std::shared_ptr<NativeModule> Application::GetModule(const std::string &name) {
	if (m_Modules.count(name) == 1)
		return m_Modules[name];
	else
		return nullptr;
}

Ease::NativeBehaviourFactory *Application::GetFactory(const std::string &name) {
	if (m_NativeBehaviours.count(name) == 1)
		return m_NativeBehaviours[name];
	else
		return nullptr;
}

uint32_t Application::Renderer_GetAlbedoFramebufferID() {
	return _renderer->GetData2D()._frameBuffer.GetAlbedoID();
}
} // namespace Ease
