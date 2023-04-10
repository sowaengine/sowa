#include "core/application.hpp"

#include "debug.hpp"
#include "sowa.hpp"

#include "stlpch.hpp"

#include "resource/resource_loader.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_watcher/resource_watcher.hpp"
#include "resource/texture/image_texture.hpp"
#include "resource/texture/ninepatch_texture.hpp"

#include "core/engine_context.hpp"
#include "core/export_generator.hpp"
#include "core/project.hpp"
#include "core/renderer.hpp"
#include "core/window.hpp"

#include "scene/2d/ninepatchrect.hpp"
#include "scene/2d/node2d.hpp"
#include "scene/2d/sprite2d.hpp"
#include "scene/2d/text2d.hpp"
#include "scene/node.hpp"
#include "scene/scene.hpp"

#include "core/input.hpp"
#include "utils/algorithm.hpp"
#include "utils/dialog.hpp"
#include "utils/file.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"

#include "nmGfx/src/Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"
#include "nmGfx/src/Core/nm_Window.hpp"

#include "argparser/arg_parser.hpp"

#include "res/Roboto-Medium.ttf.res.hpp"
#include "res/shaders/default2d.glsl.res.hpp"
#include "res/shaders/default3d.glsl.res.hpp"
#include "res/shaders/fullscreen.glsl.res.hpp"
#include "res/shaders/skybox.glsl.res.hpp"

#include "res/textures/icon_512x.png.res.hpp"

#ifdef SW_WINDOWS
#include <windows.h>
#endif

namespace Sowa {
static void InitStreams(const std::string logFile);

Application::Application(){
	SW_ENTRY()

}

Application::~Application() {
	SW_ENTRY()
}

static Reference<NinePatchTexture> s_NinePatch;

static float mapLog(float x) {
	return log10(x);
}
static float lerp(float from, float to, float t) {
	return from + ((to - from) * (t * t));
} 

bool Application::Init(int argc, char const **argv) {
	SW_ENTRY()
	using namespace Debug;

	if (argc == 0) {
		Debug::Log("Application::Init expects at least one argument (argv[0]");
		return false;
	}

	_ExecutablePath = argv[0];
	ctx = EngineContext::CreateContext();

	ctx->RegisterSingleton<Application>(Sowa::Server::APPLICATION, *this);

	Project *project = new Project(*ctx);
	ctx->RegisterSingleton<Project>(Sowa::Server::PROJECT, *project);

	if (!ParseArgs(argc, argv)) {
		return false;
	}

	Serializer::get_singleton().RegisterSerializer(Project::Typename(), SerializeImpl(Project::SaveImpl, Project::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(Size::Typename(), SerializeImpl(Size::SaveImpl, Size::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(ImageTexture::Typename(), SerializeImpl(ImageTexture::SaveImpl, ImageTexture::LoadImpl));

	Serializer::get_singleton().RegisterSerializer(Vector2::Typename(), SerializeImpl(Vector2::SaveImpl, Vector2::LoadImpl));

	Serializer::get_singleton().RegisterSerializer(Scene::Typename(), SerializeImpl(Scene::SaveImpl, Scene::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(Node::Typename(), SerializeImpl(Node::SaveImpl, Node::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(Node2D::Typename(), SerializeImpl(Node2D::SaveImpl, Node2D::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(Sprite2D::Typename(), SerializeImpl(Sprite2D::SaveImpl, Sprite2D::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(Text2D::Typename(), SerializeImpl(Text2D::SaveImpl, Text2D::LoadImpl));

	InitStreams(argParse.logFile);

	std::filesystem::path projectPath = argParse.projectPath;
	if (!project->Load(projectPath.string().c_str())) {
		projectPath = Dialog::OpenFileDialog("Open project file", "", 1, {"project.sowa"}, false, false);
		if (projectPath.string() == "") {
			return false;
		}

		if (!project->Load(projectPath.string().c_str())) {
			Debug::Error("Invalid project file");
			return false;
		}
	}
	if (projectPath.filename() == "project.sowa") {
		projectPath = projectPath.parent_path();
	}
	m_ResourceWatcher = std::make_unique<ResourceWatcher>(projectPath.string());

	Sowa::File::InsertFilepathEndpoint("abs", "./");
	Sowa::File::InsertFilepathEndpoint("res", projectPath);
	if (!Sowa::File::RegisterDataPath()) {
		Debug::Error("Engine data path not found. exiting");
		return false;
	}

	_renderer = std::make_unique<nmGfx::Renderer>();
	unsigned int windowFlags = nmGfx::WindowFlags::NONE;
	windowFlags = !argParse.window ? windowFlags | nmGfx::WindowFlags::NO_WINDOW : windowFlags;
	_renderer->Init(
		NotZero(project->proj.settings.window.windowsize.w, 1280),
		NotZero(project->proj.settings.window.windowsize.h, 720),
		NotZero(project->proj.settings.window.videosize.w, 1920),
		NotZero(project->proj.settings.window.videosize.h, 1080),
		project->proj.settings.application.name.c_str(),
		windowFlags);
	_window._windowHandle = &_renderer->GetWindow();
	_window.InitWindow(_renderer->GetWindow(), *ctx);

	auto icon = ResourceLoader::get_singleton().LoadResourceFromMemory<ImageTexture>(FILE_BUFFER(Res::App_include_res_textures_icon_512x_png_data));
	_window.SetWindowIcon(icon);

	_renderer->SetBlending(true);
	_renderer->GetData2D()._shader.LoadText(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_default2d_glsl_data)));
	_renderer->GetData3D()._shader.LoadText(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_default3d_glsl_data)));
	_renderer->GetDataFullscreen()._shader.LoadText(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_fullscreen_glsl_data)));
	_renderer->GetData3D()._skyboxShader.LoadText(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_skybox_glsl_data)));

	if (!Renderer::get_singleton().LoadFont(_DefaultFont, FILE_BUFFER(Res::App_include_res_Roboto_Medium_ttf_data))) {
		Debug::Error("Failed to load default font");
		return false;
	}

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
	RegisterNodeDestructor("NinePatchRect", [](Node *node) {
		Allocator<NinePatchRect>::Get().deallocate(reinterpret_cast<NinePatchRect *>(node), 1);
	});

	Debug::Info("Sowa Engine v{}", SOWA_VERSION);

	Reference<Scene> scene = Scene::New();
	Node2D *node = scene->Create<Node2D>("New Node");

	Node2D *node1 = scene->Create<Node2D>("Node1");
	Node2D *node2 = scene->Create<Node2D>("Node2");
	Sprite2D *node3 = scene->Create<Sprite2D>("Node3");
	Text2D *node4 = scene->Create<Text2D>("Node4");

	Reference<ImageTexture> anotherTexture = std::make_shared<ImageTexture>();
	Serializer::get_singleton().Load(anotherTexture.get(), File::GetFileContent("res://Saul_Goodman.png"));
	m_ResourceWatcher->Register("res://Saul_Goodman.png", anotherTexture);

	s_NinePatch = ResourceLoader::get_singleton().LoadResource<NinePatchTexture>("res://image.png");

	NinePatchRect *button = scene->Create<NinePatchRect>("Button");
	button->Texture() = s_NinePatch;
	button->Position().x = 400;
	button->Scale() = {.25f, .25f};

	// node3->Scale() = {0.25f, 0.25f};
	node3->Texture() = anotherTexture;
	node3->Position() = {-200, -200};
	node4->Position() = {200.f, 0.f};

	node->AddChild(node1);
	node->AddChild(node2);
	node->AddChild(node3);
	node->AddChild(node4);
	node->AddChild(button);
	node4->SetText("Sowa Engine | Lexographics");

	scene->SetRoot(node);
	SetCurrentScene(scene);

	OnInput() += [this](InputEvent e) {
		if (e.Type() == InputEventType::MouseMove) {
			if (!IsRunning() && GetWindow().IsButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
				_EditorCameraPos.x += e.mouseMove.deltaX * mapLog(_EditorCameraZoom);
				_EditorCameraPos.y -= e.mouseMove.deltaY * mapLog(_EditorCameraZoom);
			}
			// Debug::Log("Mouse Pos: ({},{}), delta: ({},{})", e.mouseMove.posX, e.mouseMove.posY, e.mouseMove.deltaX, e.mouseMove.posY);
		} else if (e.Type() == InputEventType::Key) {
			if (e.key.action == GLFW_PRESS && e.key.key == GLFW_KEY_F5) {
				if (IsRunning())
					StopGame();
				else
					StartGame();
			}
			// Debug::Log("Key Event: key: {}, scancode: {}", e.key.key, e.key.scanCode);
			if (e.key.key == GLFW_KEY_ESCAPE) {
				exit(0);
			}
		} else if (e.Type() == InputEventType::Scroll) {
			if (!IsRunning()) {
				float oldZoom = _EditorCameraZoom;

				_EditorCameraZoom -= e.scroll.scrollY * _EditorCameraZoom * 0.4;
				_EditorCameraZoom = MAX(_EditorCameraZoom, 1.1);

				Vector2 midPoint;
				midPoint.x = GetWindow().GetVideoWidth() / 2.f;
				midPoint.y = GetWindow().GetVideoHeight() / 2.f;

				_EditorCameraPos.x += (GetWindow().GetGameMousePosition().x - midPoint.x) * (mapLog(oldZoom) - mapLog(_EditorCameraZoom));
				_EditorCameraPos.y -= (GetWindow().GetGameMousePosition().y - midPoint.y) * (mapLog(oldZoom) - mapLog(_EditorCameraZoom));
			}

			// Debug::Log("Scroll Event: x: {}, y: {}", e.scroll.scrollX, e.scroll.scrollY);
		} else if (e.Type() == InputEventType::MouseButton) {
			// Debug::Log("Mouse Button Event: button: {}, action: {}, mods: {}", e.mouseButton.button, e.mouseButton.action, e.mouseButton.modifiers);
		} else if (e.Type() == InputEventType::Character) {
			// Debug::Log("Character Event: codePoint: {}", (char)e.character.codePoint);
		}
	};
	if (argParse.autoStart) {
		StartGame();
	}
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
		GetCameraTransform(),
		{0.5f, 0.5f},
		{0.2f, 0.2f, 0.2f, 1.f});

	if (!_AppRunning) {
		// Draw center cursor
		float centerX, centerY;
		float cursorSize = 5.f;
		float cursorThickness = 4.f;
		centerX = _CurrentEditorCameraPos.x;
		centerY = _CurrentEditorCameraPos.y;
		Renderer::get_singleton().DrawLine({centerX - cursorSize, centerY}, {centerX + cursorSize, centerY}, cursorThickness * mapLog(_CurrentEditorCameraZoom), {1.f, 1.f, 0.f});
		Renderer::get_singleton().DrawLine({centerX, centerY - cursorSize}, {centerX, centerY + cursorSize}, cursorThickness * mapLog(_CurrentEditorCameraZoom), {1.f, 1.f, 0.f});

		Renderer::get_singleton().DrawLine({0.f, 0.f}, {1920.f * 100, 0.f}, 2.f * mapLog(_CurrentEditorCameraZoom), {1.f, 0.f, 0.f, .6f});
		Renderer::get_singleton().DrawLine({0.f, 0.f}, {0.f, 1080.f * 100}, 2.f * mapLog(_CurrentEditorCameraZoom), {0.f, 1.f, 0.f, .6f});

		float viewportThickness = 3.f;
		Renderer::get_singleton().DrawLine({0, 0}, {_window.GetVideoWidth(), 0}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});
		Renderer::get_singleton().DrawLine({_window.GetVideoWidth(), 0}, {_window.GetVideoWidth(), _window.GetVideoHeight()}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});
		Renderer::get_singleton().DrawLine({_window.GetVideoWidth(), _window.GetVideoHeight()}, {0, _window.GetVideoHeight()}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});
		Renderer::get_singleton().DrawLine({0, _window.GetVideoHeight()}, {0, 0}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});

	} else {
		static float f = 0.f;
		f += 0.02f;
		((Sprite2D *)_Scene->GetRoot()->GetChild("Node3"))->Position() = {std::sin(f) * 200, std::cos(f) * 200};
		((Sprite2D *)_Scene->GetRoot()->GetChild("Node3"))->Rotation() += 0.4f;
	}

	if (_Scene != nullptr) {
		_Scene->UpdateDraw();
	}

	if (!_AppRunning) {
		float spacing = 512.f;
		int lineCount = 256;
		for (int i = -lineCount; i <= lineCount; i++) {
			Renderer::get_singleton().DrawLine({i * spacing, -10000}, {i * spacing, 10000}, 2 * mapLog(_EditorCameraZoom), {.7f, .7f, .7f, .3f});
		}
		for (int i = -lineCount; i <= lineCount; i++) {
			Renderer::get_singleton().DrawLine({10000, i * spacing}, {-10000, i * spacing}, 2 * mapLog(_EditorCameraZoom), {.7f, .7f, .7f, .3f});
		}
	}

	_renderer->End2D();

	_renderer->ClearLayers();
	_renderer->Draw2DLayer();

	if (_AfterRenderCallback != nullptr) {
		_AfterRenderCallback();
	}

	_renderer->GetWindow().SwapBuffers();

	Step();
	return true;
}

glm::mat4 Application::GetCameraTransform() {
	SW_ENTRY()
	_CurrentEditorCameraZoom = lerp(_CurrentEditorCameraZoom, _EditorCameraZoom, 0.5f);
	_CurrentEditorCameraPos.x = lerp(_CurrentEditorCameraPos.x, _EditorCameraPos.x, 0.5f);
	_CurrentEditorCameraPos.y = lerp(_CurrentEditorCameraPos.y, _EditorCameraPos.y, 0.5f);
	if (IsRunning())
		return nmGfx::CalculateModelMatrix(
			{.0f, .0f}, .0f, {1.f, 1.f});
	else
		return nmGfx::CalculateModelMatrix(
			glm::vec3{_CurrentEditorCameraPos.x, _CurrentEditorCameraPos.y, 0.f},
			0.f,
			glm::vec3{mapLog(_CurrentEditorCameraZoom), mapLog(_CurrentEditorCameraZoom), 1.f});
}
void Application::StartGame() {
	SW_ENTRY()
	if (_AppRunning)
		return;
	_AppRunning = true;
}

void Application::UpdateGame() {
	SW_ENTRY()
}

void Application::StopGame() {
	SW_ENTRY()
	if (!_AppRunning)
		return;
	_AppRunning = false;
}

Reference<Scene> Application::LoadScene(const char *path) {
	SW_ENTRY()
	Debug::Error("Application::LoadScene() not implemented");
	return nullptr;
}

void Application::SetCurrentScene(Reference<Scene> scene) {
	SW_ENTRY()
	_Scene = scene;
	if (_Scene != nullptr) {
		_Scene->Enter();
	}
}

void Application::RegisterNodeDestructor(const std::string &nodeType, std::function<void(Node *)> dtor) {
	SW_ENTRY()
	_NodeTypeDestructors[nodeType] = dtor;
}
void Application::DestructNode(Node *node) {
	SW_ENTRY()
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
	SW_ENTRY()
	_FrameCount++;

	if (_FrameCount % _SceneCollectInterval == 0) {
		if (_Scene != nullptr) {
			_Scene->CollectNodes();
		}
	}
	if (_FrameCount % _ResourcePollInterval == 0) {
		m_ResourceWatcher->Poll();
	}
}

uint32_t Application::Renderer_GetAlbedoFramebufferID() {
	SW_ENTRY()
	return _renderer->GetData2D()._frameBuffer.GetAlbedoID();
}

bool Application::ParseArgs(int argc, char const **argv) {
	SW_ENTRY()
	auto args = ArgParser(argc - 1, argv + 1);

	auto openOpt = args.GetOption("project");
	if (openOpt.value != "") {
		argParse.projectPath = openOpt.value;
	}
	auto logFileOpt = args.GetOption("log-file");
	if (logFileOpt.value != "") {
		argParse.logFile = logFileOpt.value;
	}

	if (args.HasFlag("--version") || args.HasFlag("v")) {
		std::cout << "Sowa Engine " SOWA_VERSION_TAG << std::endl;
		return false;
	}
	if (args.HasFlag("no-window")) {
		argParse.window = false;
	}
	if (args.HasFlag("run")) {
		argParse.autoStart = true;
	}
	return true;
}

void InitStreams(const std::string logFile) {
	using namespace Debug;

	auto &streams = Streams::GetInstance();
	streams.SetLevelText((uint32_t)LogLevel::Log, "LOG");
	streams.SetLevelText((uint32_t)LogLevel::Info, "INFO");
	streams.SetLevelText((uint32_t)LogLevel::Warn, "WARN");
	streams.SetLevelText((uint32_t)LogLevel::Error, "ERR");

	static std::ofstream tempStream;
	tempStream.open(fmt::format(logFile != "" ? logFile : "{}/sowa-{}.log", std::filesystem::temp_directory_path().string(), Sowa::Time::GetTime()), std::ios_base::app);

	streams.Add((uint32_t)LogLevel::Log, &std::cout);
	streams.Add((uint32_t)LogLevel::Info, &std::cout);
	streams.Add((uint32_t)LogLevel::Warn, &std::cout);
	streams.Add((uint32_t)LogLevel::Error, &std::cout);

	streams.Add((uint32_t)LogLevel::Info, reinterpret_cast<std::ostream *>(&tempStream));
	streams.Add((uint32_t)LogLevel::Warn, reinterpret_cast<std::ostream *>(&tempStream));
	streams.Add((uint32_t)LogLevel::Error, reinterpret_cast<std::ostream *>(&tempStream));
}

} // namespace Sowa
