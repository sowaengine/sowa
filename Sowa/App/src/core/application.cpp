#include "core/application.hpp"

#include "debug.hpp"
#include "sowa.hpp"

#include "stlpch.hpp"

#include "resource/resource_loader.hpp"
#include "resource/resource_manager.hpp"
#include "resource/texture/texture.hpp"

#include "core/engine_context.hpp"
#include "core/export_generator.hpp"
#include "core/project.hpp"
#include "core/renderer.hpp"
#include "core/window.hpp"

#include "scene/2d/node2d.hpp"
#include "scene/2d/sprite2d.hpp"
#include "scene/node.hpp"
#include "scene/scene.hpp"
#include "scene/2d/text2d.hpp"

#include "core/input.hpp"
#include "utils/dialog.hpp"
#include "utils/file.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

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
	Project::Of(ctx).Save();
}

bool Application::Init(int argc, char const *argv[]) {
	SW_ENTRY()
	using namespace Debug;

	_ExecutablePath = argv[0];
	ctx = EngineContext::CreateContext();

	ctx->RegisterSingleton<Application>(Sowa::Server::APPLICATION, *this);

	Project *project = new Project(*ctx);
	ctx->RegisterSingleton<Project>(Sowa::Server::PROJECT, *project);

	ParseArgs(argc, argv);

	Sowa::File::InsertFilepathEndpoint("abs", "./");
	Sowa::File::InsertFilepathEndpoint("res", argParse.projectPath);
	if (!Sowa::File::RegisterDataPath()) {
		Debug::Error("Engine data path not found. exiting");
		return false;
	}

	auto &streams = Streams::GetInstance();
	streams.SetLevelText((uint32_t)LogLevel::Log, "LOG");
	streams.SetLevelText((uint32_t)LogLevel::Info, "INFO");
	streams.SetLevelText((uint32_t)LogLevel::Warn, "WARN");
	streams.SetLevelText((uint32_t)LogLevel::Error, "ERR");

	static std::ofstream tempStream;
	tempStream.open(fmt::format(argParse.logFile != "" ? argParse.logFile : "{}/sowa-{}.log", std::filesystem::temp_directory_path().string(), Sowa::Time::GetTime()), std::ios_base::app);

	streams.Add((uint32_t)LogLevel::Log, &std::cout);
	streams.Add((uint32_t)LogLevel::Info, &std::cout);
	streams.Add((uint32_t)LogLevel::Warn, &std::cout);
	streams.Add((uint32_t)LogLevel::Error, &std::cout);

	streams.Add((uint32_t)LogLevel::Info, reinterpret_cast<std::ostream *>(&tempStream));
	streams.Add((uint32_t)LogLevel::Warn, reinterpret_cast<std::ostream *>(&tempStream));
	streams.Add((uint32_t)LogLevel::Error, reinterpret_cast<std::ostream *>(&tempStream));

	project->Load(argParse.projectPath.c_str());

	_renderer = std::make_unique<nmGfx::Renderer>();
	unsigned int windowFlags = nmGfx::WindowFlags::NONE;
	windowFlags = !argParse.window ? windowFlags | nmGfx::WindowFlags::NO_WINDOW : windowFlags;
	_renderer->Init(
		project->proj.settings.window.windowsize.x,
		project->proj.settings.window.windowsize.y,
		project->proj.settings.window.videosize.x,
		project->proj.settings.window.videosize.y,
		project->proj.settings.application.name.c_str(),
		windowFlags);
	_window._windowHandle = &_renderer->GetWindow();
	_window.InitWindow(_renderer->GetWindow(), *ctx);

	auto icon = ResourceLoader::get_singleton().LoadResourceFromMemory<ImageTexture>(Res::App_include_res_textures_icon_png_data.data(), Res::App_include_res_textures_icon_png_data.size());
	_window.SetWindowIcon(icon);

	_renderer->SetBlending(true);
	_renderer->GetData2D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::App_include_res_shaders_default2d_glsl_data.data()), Res::App_include_res_shaders_default2d_glsl_data.size()));
	_renderer->GetData3D()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::App_include_res_shaders_default3d_glsl_data.data()), Res::App_include_res_shaders_default3d_glsl_data.size()));
	_renderer->GetDataFullscreen()._shader.LoadText(std::string(reinterpret_cast<char *>(Res::App_include_res_shaders_fullscreen_glsl_data.data()), Res::App_include_res_shaders_fullscreen_glsl_data.size()));
	_renderer->GetData3D()._skyboxShader.LoadText(std::string(reinterpret_cast<char *>(Res::App_include_res_shaders_skybox_glsl_data.data()), Res::App_include_res_shaders_skybox_glsl_data.size()));

	// if (projectSettings->_application.MainScene != "")
	// 	_pCurrentScene->LoadFromFile(projectSettings->_application.MainScene.c_str());

	RegisterNodeDestructor("Node", [](Node *node) {
		Allocator<Node>::Get().deallocate(reinterpret_cast<Node *>(node), 1);
	});
	RegisterNodeDestructor("Node2D", [](Node *node) {
		Allocator<Node2D>::Get().deallocate(reinterpret_cast<Node2D *>(node), 1);
	});
	RegisterNodeDestructor("Sprite2D", [](Node *node) {
		Allocator<Sprite2D>::Get().deallocate(reinterpret_cast<Sprite2D *>(node), 1);
	});
	RegisterNodeDestructor("Text2D", [](Node *node) {
		Allocator<Text2D>::Get().deallocate(reinterpret_cast<Text2D *>(node), 1);
	});

	Debug::Info("Sowa Engine v{}", SOWA_VERSION_STRING);

	Reference<Scene> scene = Scene::New();
	Node2D *node = scene->Create<Node2D>("New Node");

	Node2D *node1 = scene->Create<Node2D>("Node1");
	Node2D *node2 = scene->Create<Node2D>("Node2");
	Sprite2D *node3 = scene->Create<Sprite2D>("Node3");
	Text2D *node4 = scene->Create<Text2D>("Node4");

	node->AddChild(node1);
	node->AddChild(node2);
	node->AddChild(node3);
	node->AddChild(node4);
	node4->SetText("Sowa Engine");

	scene->SetRoot(node);
	SetCurrentScene(scene);

	return true;
}

bool Application::Process() {
	SW_ENTRY()

	_window.UpdateEvents();
	_renderer->GetWindow().PollEvents();
	if (_window.ShouldClose())
		return false;

	if (_Scene != nullptr) {
		_Scene->UpdateLogic();
	}

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
	if (_Scene != nullptr) {
		_Scene->UpdateDraw();
	}

	_renderer->End2D();

	_renderer->ClearLayers();
	_renderer->Draw2DLayer();

	_renderer->GetWindow().SwapBuffers();

	Step();
	return true;
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

Reference<Scene> Application::LoadScene(const char *path) {
	Debug::Error("Application::LoadScene() not implemented");
	return nullptr;
}

void Application::SetCurrentScene(Reference<Scene> scene) {
	_Scene = scene;
	if (_Scene != nullptr) {
		_Scene->Enter();
	}
}

void Application::RegisterNodeDestructor(const std::string &nodeType, std::function<void(Node *)> dtor) {
	_NodeTypeDestructors[nodeType] = dtor;
}
void Application::DestructNode(Node *node) {
	Debug::Log("Delete node '{}':'{}'", node->Name(), node->_NodeType);

	for (Node *child : node->GetChildren()) {
		node->RemoveNode(child);
	}
	if (_NodeTypeDestructors.find(node->_NodeType) != _NodeTypeDestructors.end()) {
		_NodeTypeDestructors[node->_NodeType](node);
	} else {
		Debug::Error("Tried to destruct unknown node type '{}'", node->_NodeType);
	}
}

void Application::Step() {
	_FrameCount++;

	if (_FrameCount % _SceneCollectInterval == 0) {
		if (_Scene != nullptr) {
			_Scene->CollectNodes();
		}
	}
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
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}

	if (args.size() > 0) {
		std::string lastArg = "";
		for (size_t i = 0; i < args.size(); i++) {
			std::string arg = args[i];

			if (lastArg == "--project") {
				argParse.projectPath = arg;
			} else if (lastArg == "--log-file") {
				argParse.logFile = arg;
			}
#ifdef SW_EDITOR
			else if (arg == "--no-editor") {
				argParse.editor = false;
			}
#endif
			else if (arg == "--no-window") {
				argParse.window = false;
			}

			lastArg = args[i];
		}
	}
}

} // namespace Sowa
