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
#include "core/script_server.hpp"
#include "core/window.hpp"

#include "scene/2d/button.hpp"
#include "scene/2d/nine_slice_panel.hpp"
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

#include "gfx/gl/glfuncs.hpp"
#include "gfx/gl/graphics_gl.hpp"
#include "gfx/gl/texture_gl.hpp"
#include "gfx/graphics.hpp"
#include "gfx/window.hpp"
#include "gfx/window_manager.hpp"

#include "nmGfx/src/Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"
#include "nmGfx/src/Core/nm_Window.hpp"

#include "argparser/arg_parser.hpp"

#include "res/Roboto-Medium.ttf.res.hpp"
#include "res/shaders/default2d.frag.res.hpp"
#include "res/shaders/default2d.vert.res.hpp"
#include "res/shaders/fullscreen.frag.res.hpp"
#include "res/shaders/fullscreen.vert.res.hpp"
#include "res/shaders/solid_color.frag.res.hpp"
#include "res/shaders/solid_color.vert.res.hpp"

#include "res/textures/icon_512x.png.res.hpp"

#ifdef SW_WINDOWS
#include <windows.h>
#endif

namespace sowa {
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
static float lerpAngle(float from, float to, float t) {
	float diff = fmod(to - from, 3.141592653589 * 2);
	float dist = fmod(2.0 * diff, 3.141592653589 * 2) - diff;
	return from + dist * t;
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

	ctx->RegisterSingleton<Application>(sowa::Server::APPLICATION, *this);

	Project *project = new Project(*ctx);
	ctx->RegisterSingleton<Project>(sowa::Server::PROJECT, *project);

	sowa::script_server *scriptServer = new sowa::script_server(*ctx);
	ctx->RegisterSingleton<sowa::script_server>(sowa::Server::SCRIPT_SERVER, *scriptServer);

	if (!ParseArgs(argc, argv)) {
		return false;
	}

	Serializer::get_singleton().RegisterSerializer(Project::Typename(), SerializeImpl(Project::SaveImpl, Project::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(size::Typename(), SerializeImpl(size::SaveImpl, size::LoadImpl));
	Serializer::get_singleton().RegisterSerializer(ImageTexture::Typename(), SerializeImpl(ImageTexture::SaveImpl, ImageTexture::LoadImpl));

	Serializer::get_singleton().RegisterSerializer(vec2::Typename(), SerializeImpl(vec2::SaveImpl, vec2::LoadImpl));

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

	sowa::File::InsertFilepathEndpoint("abs", "./");
	sowa::File::InsertFilepathEndpoint("res", projectPath);
	if (!sowa::File::RegisterDataPath()) {
		Debug::Error("Engine data path not found. exiting");
		return false;
	}
	// initialize servers before renderer
	scriptServer->init();

	m_windowManager.m_ctx = ctx;
	m_window = &m_windowManager.CreateWindow();
	{
		gfx::WindowFlags windowFlags = gfx::WindowFlags_::None;
		windowFlags = !argParse.window ? windowFlags | gfx::WindowFlags_::Hidden : windowFlags;

		sowa::gfx::InitWindowArgs args;
		args.windowWidth = NotZero(project->proj.settings.window.windowsize.w, 1280);
		args.windowHeight = NotZero(project->proj.settings.window.windowsize.h, 720);
		args.videoWidth = NotZero(project->proj.settings.window.videosize.w, 1920);
		args.videoHeight = NotZero(project->proj.settings.window.videosize.h, 1080);
		args.title = project->proj.settings.application.name;
		args.flags = windowFlags;

		m_window->InitWindow(args);
	}

	gfx::IGraphics *graphics = new gfx::GraphicsGL();
	gfx::IGraphics::SetInstance(graphics);

	const sowa::vec2 videoSize = m_window->GetVideoSize();
	m_drawpass2d.SetTarget(0, gfx::GLFramebufferTargetType::Vec4);
	m_drawpass2d.Create(videoSize.x, videoSize.y);

	gfx::GLTexture icon;
	icon.Load2D(FILE_BUFFER(Res::App_include_res_textures_icon_512x_png_data));
	m_window->SetWindowIcon(icon);

	Graphics().SetDepthTest(true);
	Graphics().Default2DShader().New(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_default2d_vert_data)), std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_default2d_frag_data)));
	Graphics().DefaultSolidColorShader().New(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_solid_color_vert_data)), std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_solid_color_frag_data)));
	Graphics().DefaultFullscreenShader().New(std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_fullscreen_vert_data)), std::string(FILE_BUFFER_CHAR(Res::App_include_res_shaders_fullscreen_frag_data)));

	// if (!Renderer::get_singleton().LoadFont(_DefaultFont, FILE_BUFFER(Res::App_include_res_Roboto_Medium_ttf_data))) {
	// 	Debug::Error("Failed to load default font");
	// 	return false;
	// }

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
	RegisterNodeDestructor("NineSlicePanel", [](Node *node) {
		Allocator<NineSlicePanel>::Get().deallocate(reinterpret_cast<NineSlicePanel *>(node), 1);
	});

	Debug::Info("Sowa Engine v{}", SOWA_VERSION);

	Reference<Scene> scene = Scene::New();
	Node2D *node = scene->Create<Node2D>("New Node");

	Node2D *node1 = scene->Create<Node2D>("Node1");
	Node2D *node2 = scene->Create<Node2D>("Node2");
	Sprite2D *node3 = scene->Create<Sprite2D>("Node3");
	Text2D *node4 = scene->Create<Text2D>("Node4");

	// Reference<ImageTexture> anotherTexture = std::make_shared<ImageTexture>();
	// Serializer::get_singleton().Load(anotherTexture.get(), File::GetFileContent("res://kenney.png"));
	// m_ResourceWatcher->Register("res://kenney.png", anotherTexture);

	// s_NinePatch = ResourceLoader::get_singleton().LoadResource<NinePatchTexture>("res://uv.jpg");

	// Button *button = scene->Create<Button>("Button", 12);
	// button->Texture() = s_NinePatch;
	// button->Position() = {600, 200};
	// button->Scale() = {.25f, .25f};
	// button->Rotation() = 9.f;
	// button->Size() = {3000.f, 2000.f};
	// button->Text() = "Click me";

	Reference<ImageTexture> texture = std::make_shared<ImageTexture>();
	Serializer::get_singleton().Load(texture.get(), File::GetFileContent("res://kenney_space-shooter-redux/PNG/playerShip2_red.png"));

	node3->Scale() = {1.f, 1.f};
	node3->Rotation() = 45.f;
	node3->Texture() = texture;
	node3->Position() = {600, 300};
	node4->Position() = {200.f, -200.f};

	node->AddChild(node1);
	node->AddChild(node2);
	node->AddChild(node3);
	node->AddChild(node4);
	// node->AddChild(button);
	node4->SetText("Sowa Engine | Lexographics");

	scene->SetRoot(node);
	SetCurrentScene(scene);

	OnInput() += [this](InputEvent e) {
		if (e.Type() == InputEventType::MouseMove) {
			float ratioX = (float)GetWindow().GetVideoSize().x / GetWindow().GetWindowSize().x;
			float ratioY = (float)GetWindow().GetVideoSize().x / GetWindow().GetWindowSize().y;

			if (!IsRunning() && GetWindow().IsButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
				_EditorCameraPos.x += e.mouseMove.deltaX * mapLog(_EditorCameraZoom) * ratioX;
				_EditorCameraPos.y -= e.mouseMove.deltaY * mapLog(_EditorCameraZoom) * ratioY;

				if (m_is_node_dragging && PickedNode() != 0) {
					Node *node = GetCurrentScene()->GetNodeByID(PickedNode());
					if (node != nullptr) {
						Node2D *node2d = dynamic_cast<Node2D *>(node);
						if (node2d != nullptr) {
							node2d->Position().x += e.mouseMove.deltaX * mapLog(_EditorCameraZoom) * ratioX;
							node2d->Position().y -= e.mouseMove.deltaY * mapLog(_EditorCameraZoom) * ratioY;
						}
					}
				}
			}
			if (
				(m_is_node_dragging && !IsRunning() && GetWindow().IsButtonDown(GLFW_MOUSE_BUTTON_LEFT) && PickedNode() != 0 && GetCurrentScene() != nullptr) ||
				m_on_drag_mode && !IsRunning() && PickedNode() != 0 && !GetWindow().IsButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
				Node *node = GetCurrentScene()->GetNodeByID(PickedNode());
				if (node != nullptr) {
					Node2D *node2d = dynamic_cast<Node2D *>(node);
					if (node2d != nullptr) {
						node2d->Position().x -= e.mouseMove.deltaX * mapLog(_EditorCameraZoom) * ratioX;
						node2d->Position().y += e.mouseMove.deltaY * mapLog(_EditorCameraZoom) * ratioY;
					}
				}
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

				vec2 midPoint;
				midPoint.x = GetWindow().GetVideoSize().x / 2.f;
				midPoint.y = GetWindow().GetVideoSize().y / 2.f;

				_EditorCameraPos.x += (GetWindow().GetVideoMousePosition().x - midPoint.x) * (mapLog(oldZoom) - mapLog(_EditorCameraZoom));
				_EditorCameraPos.y -= (GetWindow().GetVideoMousePosition().y - midPoint.y) * (mapLog(oldZoom) - mapLog(_EditorCameraZoom));
			}

			// Debug::Log("Scroll Event: x: {}, y: {}", e.scroll.scrollX, e.scroll.scrollY);
		} else if (e.Type() == InputEventType::MouseButton) {
			// Debug::Log("Mouse Button Event: button: {}, action: {}, mods: {}", e.mouseButton.button, e.mouseButton.action, e.mouseButton.modifiers);
			if (e.mouseButton.button == 0 && e.mouseButton.action == 0) {
				m_is_node_dragging = false;
			}
		} else if (e.Type() == InputEventType::MouseClick) {
			// Debug::Log("Mouse Click Event: button: {}, single: {}, mods: {}", e.mouseClick.button, e.mouseClick.single, e.mouseClick.modifiers);
			// if modifiers & ctrl, multi select
			if (!m_on_drag_mode && e.mouseClick.button == 0 && e.mouseClick.single) {
				this->m_picked_node = this->m_hovering_node;
			}

			if (m_on_drag_mode && e.mouseClick.button == 0 && e.mouseClick.single) {
				// set window mode to visible
				m_on_drag_mode = false;
			}

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

	m_window->PollEvents();
	if (m_window->ShouldClose())
		return false;

	if (_Scene != nullptr) {
		_Scene->UpdateLogic();
	}

	if (HoveringNode() != 0 && HoveringNode() == PickedNode() && GetWindow().IsButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		m_is_node_dragging = true;
	}
	if (PickedNode() != 0 && GetWindow().IsKeyJustPressed(GLFW_KEY_G)) {
		// set window mode to hidden
		m_on_drag_mode = true;
	}

	Graphics().SetDepthTest(true);

	m_drawpass2d.Bind();
	Graphics().Clear();
	// _renderer->Begin2D(
	// 	GetCameraTransform(),
	// 	{0.5f, 0.5f},
	// 	{0.2f, 0.2f, 0.2f, 1.f});

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
		Renderer::get_singleton().DrawLine({0, 0}, {m_window->GetVideoSize().x, 0}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});
		Renderer::get_singleton().DrawLine({m_window->GetVideoSize().x, 0}, {m_window->GetVideoSize().x, m_window->GetVideoSize().y}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});
		Renderer::get_singleton().DrawLine({m_window->GetVideoSize().x, m_window->GetVideoSize().y}, {0, m_window->GetVideoSize().y}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});
		Renderer::get_singleton().DrawLine({0, m_window->GetVideoSize().y}, {0, 0}, viewportThickness * mapLog(_CurrentEditorCameraZoom), {.0f, .2f, .7f});

	} else {
		static float f = 0.f;
		f += 0.02f;
		((Sprite2D *)_Scene->GetRoot()->GetChild("Node3"))->Position() = {std::sin(f) * 200, std::cos(f) * 200};
		((Sprite2D *)_Scene->GetRoot()->GetChild("Node3"))->Rotation() += 0.4f;
	}

	static float g;
	g += 0.02f;
	// ((NineSlicePanel *)_Scene->GetRoot()->GetChild("Button"))->Size().x = 3000 + (std::sin(g) * 100);
	// ((NineSlicePanel *)_Scene->GetRoot()->GetChild("Button"))->Size().y = 2000 + (std::cos(g) * 100);

	// Draw node selection
	if (!IsRunning() && PickedNode() != 0 && GetCurrentScene() != nullptr) {
		Node *node = GetCurrentScene()->GetNodeByID(PickedNode());
		if (node != nullptr) {
			Node2D *node2d = dynamic_cast<Node2D *>(node);
			if (node2d != nullptr) {
				float selectionThickness = 5.f;
				Renderer::get_singleton().DrawLine({node2d->Position().x - 30, node2d->Position().y}, {node2d->Position().x + 30, node2d->Position().y}, selectionThickness * mapLog(_CurrentEditorCameraZoom), {.1f, .4f, .6f, 3.f});
				Renderer::get_singleton().DrawLine({node2d->Position().x, node2d->Position().y - 30}, {node2d->Position().x, node2d->Position().y + 30}, selectionThickness * mapLog(_CurrentEditorCameraZoom), {.1f, .4f, .6f, 3.f});
			}
		}
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

	vec2 mousePos = GetWindow().GetVideoMousePosition();
	// mousePos.x += (_EditorCameraPos.x * mapLog(_EditorCameraZoom)) - (GetWindow().GetVideoSize().x / 2);
	// mousePos.y -= (_EditorCameraPos.y * mapLog(_EditorCameraZoom)) - (GetWindow().GetVideoSize().y / 2);
	// int id = _renderer->Get2DPickID(mousePos.x, GetWindow().GetVideoSize().y - mousePos.y);
	// if (GetCurrentScene() != nullptr) {
	// 	if (GetCurrentScene()->GetNodeByID(id) != nullptr) {
	// 		m_hovering_node = static_cast<uint32_t>(id);
	// 	} else {
	// look for editor clickables
	// 		m_hovering_node = 0;
	// 	}
	// }

	const sowa::vec2 windowSize = m_window->GetWindowSize();
	const sowa::vec2 videoSize = m_window->GetVideoSize();

	static gfx::GLTexture tex;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		auto data = File::GetFileContent("res://kenney_space-shooter-redux/PNG/playerShip2_blue.png");
		tex.Load2D(data.data(), data.size());
	}

	static vec2 position{1920.f / 2, 1080.f / 2};
	const float speed = 200 * (1.0f / 60);

	vec2 pos = m_window->GetVideoMousePosition();
	pos.y = m_window->GetVideoSize().y - pos.y;

	float targetRot = atan2(pos.y - position.y, pos.x - position.x) - (3.141592653589 / 2);
	static float rot = targetRot;
	rot = lerpAngle(rot, targetRot, 0.25f);

	sowa::mat4 modelMatrix = CalculateModelMatrix({position.x, position.y, -10}, {0.f, 0.f, glm::degrees(rot)}, {112.f * 1.5f, 75.f * 1.5f, 1.f}, {0.f, 0.f, 0.f}, mat4(1.f));

	BindProjectionUniform(Graphics().Default2DShader(), "uProj");
	BindProjectionUniform(Graphics().DefaultSolidColorShader(), "uProj");

	Graphics().Default2DShader().Bind();
	Graphics().Default2DShader().UniformTexture("uTexture", tex.ID(), 0);
	Graphics().Default2DShader().UniformMat4("uModel", modelMatrix);
	Graphics().DrawQuad();

	vec2 velocity{0.f, 0.f};
	if (m_window->IsKeyDown(GLFW_KEY_W)) {
		velocity.y += 1;
	}
	if (m_window->IsKeyDown(GLFW_KEY_S)) {
		velocity.y -= 1;
	}
	if (m_window->IsKeyDown(GLFW_KEY_D)) {
		velocity.x += 1;
	}
	if (m_window->IsKeyDown(GLFW_KEY_A)) {
		velocity.x -= 1;
	}
	if (velocity.length() > 0.1) {
		velocity = velocity.clamp();
		velocity.x *= speed;
		velocity.y *= speed;
	} else {
		velocity = {0.f, 0.f};
	}

	position.x += velocity.x;
	position.y += velocity.y;

	m_drawpass2d.Unbind();
	Graphics().Clear();

	Graphics().DefaultFullscreenShader().Bind();
	Graphics().DefaultFullscreenShader().UniformTexture("gAlbedo", m_drawpass2d.GetTargetTextureID(0), 0);

	static gfx::ViewportDrawMode mode = gfx::ViewportDrawMode_KeepRatio;
	if (m_window->IsKeyDown(GLFW_KEY_1)) {
		mode = gfx::ViewportDrawMode_KeepRatio;
	}
	if (m_window->IsKeyDown(GLFW_KEY_2)) {
		mode = gfx::ViewportDrawMode_KeepWidth;
	}
	if (m_window->IsKeyDown(GLFW_KEY_3)) {
		mode = gfx::ViewportDrawMode_KeepHeight;
	}
	if (m_window->IsKeyDown(GLFW_KEY_4)) {
		mode = gfx::ViewportDrawMode_Stretch;
	}
	if (m_window->IsKeyDown(GLFW_KEY_5)) {
		mode = gfx::ViewportDrawMode_Contain;
	}

	if (m_window->IsKeyDown(GLFW_KEY_F1)) {
		gfx::GL().setPolygonMode(gfx::GLPolygonMode::Fill);
	}
	if (m_window->IsKeyDown(GLFW_KEY_F2)) {
		gfx::GL().setPolygonMode(gfx::GLPolygonMode::Line);
	}

	{
		gfx::SetViewportStyleArgs args;
		args.mode = mode;
		args.windowWidth = windowSize.x;
		args.windowHeight = windowSize.y;
		args.videoWidth = videoSize.x;
		args.videoHeight = videoSize.y;
		Graphics().SetViewportStyle(args);
		Graphics().DrawFullscreenQuad();
	}

	// if (_AfterRenderCallback != nullptr) {
	// 	_AfterRenderCallback();
	// }

	m_window->SwapBuffers();

	Step();
	return true;
}

void Application::BindProjectionUniform(gfx::IShader &shader, const std::string &uniformName) {
	const sowa::vec2 &videoSize = m_window->GetVideoSize();
	sowa::mat4 projectionMatrix;
	{
		CalculateOrthoArgs args;
		args.width = videoSize.x;
		args.height = videoSize.y;
		args.centerX = 0.0f;
		args.centerY = 0.0f;
		args.near = 0.f;
		args.far = 1000.f;
		projectionMatrix = CalculateOrtho(args);
	}

	shader.UniformMat4(uniformName, projectionMatrix);
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
	// return _renderer->GetData2D()._frameBuffer.GetAlbedoID();
	return 0;
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
	tempStream.open(fmt::format(logFile != "" ? logFile : "{}/sowa-{}.log", std::filesystem::temp_directory_path().string(), sowa::Time::GetTime()), std::ios_base::app);

	streams.Add((uint32_t)LogLevel::Log, &std::cout);
	streams.Add((uint32_t)LogLevel::Info, &std::cout);
	streams.Add((uint32_t)LogLevel::Warn, &std::cout);
	streams.Add((uint32_t)LogLevel::Error, &std::cout);

	streams.Add((uint32_t)LogLevel::Info, reinterpret_cast<std::ostream *>(&tempStream));
	streams.Add((uint32_t)LogLevel::Warn, reinterpret_cast<std::ostream *>(&tempStream));
	streams.Add((uint32_t)LogLevel::Error, reinterpret_cast<std::ostream *>(&tempStream));
}

} // namespace sowa
