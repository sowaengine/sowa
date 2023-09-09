#include "app.hxx"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/projection.hpp"

#include "core/command/interfaces/command_palette.hxx"
#include "core/command/interfaces/scene_save_as.hxx"
#include "core/graphics.hxx"
#include "core/time.hxx"
#include "core/tweens.hxx"

#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"
#include "servers/script_server.hxx"

#include "data/toml_document.hxx"

#include "ui/new_container.hxx"
#include "ui/new_tree.hxx"
#include "ui/ui_canvas.hxx"

#include "scene/camera_2d.hxx"
#include "scene/node_2d.hxx"
#include "scene/node_db.hxx"
#include "scene/sprite_2d.hxx"
#include "scene/text.hxx"

#include "resource/resource.hxx"
#include "resource/resource_manager.hxx"

#include "behaviour/behaviour.hxx"
#include "behaviour/behaviour_db.hxx"

#include "behaviour/builtin/top_down_eight_dir.hxx"

#include "game/game.hxx"

#include "utils/utils.hxx"

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>

#ifdef SW_WEB
#include <emscripten.h>
#endif

static App *s_instance;

static float posX = 200.f;

App::App() {
	s_instance = this;
	Utils::Randomize();

	Tweens::GetInstance().RegisterTween(2.f, [](float f) {
		posX = 200.f + (f * 1000.f);
	});
}

App::~App() {
}

App &App::GetInstance() {
	return *s_instance;
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

	m_layer2D.SetTarget(0, RenderLayerTargetType::Vec4);
	m_layer2D.SetTarget(1, RenderLayerTargetType::Int);
	m_layer2D.Create(1920, 1080);

	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);

	m_layerUI.SetTarget(0, RenderLayerTargetType::Vec4);
	m_layerUI.SetTarget(1, RenderLayerTargetType::Int);
	m_layerUI.Create(w, h);

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
	ModelBuilder::Quad2D(fullscreenModel, 2.f);

	err = fullscreenShader.Load("res://shaders/fullscreen.vs", "res://shaders/fullscreen.fs");
	if (err != OK) {
		std::cerr << "Failed to load fullscreen shader" << std::endl;
	}

	err = m_testTexture.Load(TextureType::Texture2D, "res://image.png");
	if (err != OK) {
		std::cout << "Failed to load texture: " << err << std::endl;
	}

	// err = m_testFont.LoadTTF("res://Roboto-Medium.ttf");
	err = m_testFont.LoadTTF("res://NotoSansKR-Medium.otf");
	if (err != OK) {
		std::cout << "Failed to load font: " << err << std::endl;
	}

	err = m_batchRenderer.Init("res://shaders/batch2d.vs", "res://shaders/batch2d.fs");
	if (err != OK) {
		std::cout << "Failed to load renderer: " << err << std::endl;
	}

	Time::update();

	WindowResizeCallback().append([this](int width, int height) {
		this->m_layerUI.Resize(width, height);
	});

	ScrollCallback().append([this](InputEventScroll event) {
		if (!IsRunning()) {
			editor_scroll_event(event);
		}
	});

	MouseMoveCallback().append([this](InputEventMouseMove event) {
		if (!IsRunning()) {
			editor_mouse_move_event(event);
		}
	});

	KeyCallback().append([this](InputEventKey event) {
		if (!IsRunning()) {
			editor_key_event(event);
		}

		if (event.action == KEY_PRESSED && event.key == KEY_Z) {
			Tweens::GetInstance().RegisterTween(2.f, [](float f) {
				App::GetInstance().GetCurrentScene()->get_active_camera2d()->Zoom() = 1.f - (f * 0.5f);
			});
		}

		if (event.action == KEY_PRESSED && event.key == KEY_F5) {
			if (IsRunning())
				Stop();
			else
				Start();
		}

		command_interface_key_callback(event);
	});

	CharCallback().append([&](InputEventChar event) {
		command_interface_char_callback(event);
	});

	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	ResourceFactory imageTexture;
	imageTexture.typeName = "ImageTexture";
	imageTexture.type = ResourceType_ImageTexture;
	imageTexture.loadFunc = [](Resource *res, std::string path) {
		res->Data() = Texture();
		if (Texture *tex = res->As<Texture>(); nullptr != tex) {
			Error err = tex->Load(TextureType::Texture2D, path.c_str());
			if (err != OK) {
				std::cout << "Failed to load texture: " << err << std::endl;
			}
		}
	};
	ResourceManager::GetInstance().RegisterResource(".png", imageTexture);

	Resource *res = ResourceManager::GetInstance().Load("res://assets/shotThin.png");

	NodeDB &db = NodeDB::GetInstance();

#define REGISTER_NODE_TYPE(type, extends)                                \
	do {                                                                 \
		NodeFactory factory;                                             \
		factory.constructor = []() -> Node * {                           \
			return new type;                                             \
		};                                                               \
		factory.destructor = [](Node *node) {                            \
			delete reinterpret_cast<type *>(node);                       \
		};                                                               \
                                                                         \
		db.BindNodeType<type>(#type, db.GetNodeType(#extends), factory); \
	} while (0);

#define REGISTER_NODE_PROPERTY(type, propName, propAccessor, propType) \
	do {                                                               \
		NodeProperty prop;                                             \
		prop.get = [](Node *node) -> Property {                        \
			return dynamic_cast<type *>(node)->propAccessor;           \
		};                                                             \
		prop.get_ref = [](Node *node) -> Property {                    \
			return &(dynamic_cast<type *>(node)->propAccessor);        \
		};                                                             \
		prop.set = [](Node *node, Property value) {                    \
			if (propType *v = std::any_cast<propType>(&value)) {       \
				dynamic_cast<type *>(node)->propAccessor = *v;         \
			}                                                          \
		};                                                             \
		prop.typeName = #propType;                                     \
		db.BindProperty(db.GetNodeType(#type), propName, prop);        \
	} while (0);

	REGISTER_NODE_TYPE(Node, );
	REGISTER_NODE_PROPERTY(Node, "name", Name(), std::string);

	REGISTER_NODE_TYPE(Node2D, Node);
	REGISTER_NODE_PROPERTY(Node2D, "position", Position(), vec2);
	REGISTER_NODE_PROPERTY(Node2D, "scale", Scale(), vec2);
	REGISTER_NODE_PROPERTY(Node2D, "rotation", Rotation(), float);
	REGISTER_NODE_PROPERTY(Node2D, "z_index", ZIndex(), float);

	REGISTER_NODE_TYPE(Sprite2D, Node2D);
	REGISTER_NODE_PROPERTY(Sprite2D, "texture", GetTexture(), RID);

	REGISTER_NODE_TYPE(Camera2D, Node2D);
	REGISTER_NODE_PROPERTY(Camera2D, "zoom", Zoom(), vec2);

	BehaviourDB::GetInstance().RegisterBehaviour("8 Dir Movement", Behaviour::New(TopDownEightDirMovement::Start, TopDownEightDirMovement::Update));

	reload_scripts();
	Main();

	RegisterCommand("Start/Stop Game", [&]() {
		if (IsRunning())
			Stop();
		else
			Start();
	});
	RegisterCommand("Rebuild Scripts", [&]() {
		reload_scripts();

		if (IsRunning() && nullptr != GetCurrentScene()) {
			std::function<void(Node *)> func;
			func = [&func](Node *node) {
				node->ReloadBehaviours();

				for (Node *child : node->GetChildren()) {
					func(child);
				}
			};

			for (Node *node : GetCurrentScene()->Nodes()) {
				func(node);
			}
		}
	});
	RegisterCommand("Save Scene", [&]() {
		if (nullptr == GetCurrentScene())
			return;
		if (GetCurrentScene()->Path() == "") {
			SetCommandInterface(new SceneSaveAsInterface(""));
			return;
		}

		Error err = GetCurrentScene()->Save(GetCurrentScene()->Path().c_str());
		if (err != OK) {
			std::cout << "Failed to save scene " << err << std::endl;
		}
	});
	RegisterCommand("Save Scene As", [&]() {
		if (nullptr == GetCurrentScene())
			return;

		SetCommandInterface(new SceneSaveAsInterface(GetCurrentScene()->Path()));
	});
	RegisterCommand("Exit", [&]() {
		exit(0);
	});

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

void App::SetRenderLayer(RenderLayer *renderlayer) {
	if (nullptr == renderlayer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int w, h;
		RenderingServer::GetInstance().GetWindowSize(w, h);

		float windowRatio = (float)w / h;
		float videoRatio = (float)1920.f / 1080.f;

		if (windowRatio > videoRatio) {
			float width = h * videoRatio;
			float height = h;
			float gap = w - width;

			float x = gap / 2.f;
			glViewport(x, 0.f, width, height);

		} else {
			float width = w;
			float height = w / videoRatio;
			float gap = h - height;

			float y = gap / 2.f;
			glViewport(0, y, width, height);
		}

		return;
	}

	renderlayer->Bind();
}

void App::mainLoop() {
	InputServer::GetInstance().ProcessInput();
	glClearColor(0.3f, 0.3f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	Time::update();

	SetRenderLayer(&m_layerUI);
	m_layerUI.Clear(0.0f, 0.0f, 0.0f, 0.0f);

	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);
	glViewport(0, 0, w, h);

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, static_cast<float>(w), 0.f, static_cast<float>(h), -128.f, 128.f));
	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));
	Renderer().Reset();

	if (nullptr != m_commandInterface) {
		CommandInterface *interface = m_commandInterface;

		glm::vec3 normalColor(0.12f, 0.12f, 0.12f);
		glm::vec3 hoveredColor(0.2f, 0.2f, 0.2f);

		const float padding = 8.f;
		const float xPos = w / 3.f;
		const float width = w / 3.f;
		const float textScale = 0.4f;

		float cursorY = h;

		if (interface->text_input) {
			glm::vec2 textSize = m_testFont.CalcTextSize("I") * textScale;
			float height = textSize.y + (padding * 2);
			cursorY -= height;

			float textX = xPos + padding;
			float textY = cursorY + padding;

			const float outlineSize = 2.f;

			Renderer().PushQuad(xPos, cursorY, 0.f, width, height, 0.6, 0.6, 0.6, 1.f, 0.f, 0.f);
			Renderer().PushQuad(xPos + outlineSize, cursorY + outlineSize, 0.f, width - (outlineSize * 2), height - (outlineSize * 2), normalColor.x, normalColor.y, normalColor.z, 1.f, 0.f, 0.f);
			if (interface->text.size() > 0) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::string str = converter.to_bytes(interface->text);

				Renderer().DrawText(str, &m_testFont, textX, textY, glm::mat4(1.f), 0.f, textScale);
			} else if (interface->text_placeholder.size() > 0) {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				std::string str = converter.to_bytes(interface->text_placeholder);

				Renderer().DrawText(str, &m_testFont, textX, textY, glm::mat4(1.f), 0.f, textScale, 0.f, 0.f, 1.f, 1.f, 1.f, 0.6f);
			}

			static float f = 0.f;
			f += 0.5 * Time::Delta();
			float opacity = (std::sin(f) + 1.f) * 0.5f;

			const float cursorPadding = 5.f;
			const float cursorWidth = 2.f;

			std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
			std::string str = converter.to_bytes(this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor));
			float textCursorX = m_testFont.CalcTextSize(str).x * textScale;
			Renderer().PushQuad(xPos + textCursorX + textSize.x + 2.f, cursorY + cursorPadding, 0.f, cursorWidth, height - (cursorPadding * 2), 0.8f, 0.8f, 0.8f, opacity, 0.f, 0.f);
		}

		int index = -1;
		for (CommandOption &opt : interface->options) {
			index++;
			glm::vec2 size = m_testFont.CalcTextSize("I") * textScale;

			float height = size.y + (padding * 2);
			cursorY -= height;

			float textX = xPos + padding;
			float textY = cursorY + padding;

			glm::vec3 color = normalColor;
			if (index == interface->currentIndex)
				color = hoveredColor;

			Renderer().PushQuad(xPos, cursorY, 0.f, width, height, color.x, color.y, color.z, 1.f, 0.f, 0.f);
			Renderer().DrawText(opt.label, &m_testFont, textX, textY, glm::mat4(1.f), 0.f, textScale);
		}
	}

	Renderer().End();

	SetRenderLayer(&m_layer2D);
	m_layer2D.Clear(0.5f, 0.7f, 0.1f, 0.f, true);

	glm::mat4 view = glm::mat4(1.f);
	vec2 centerPoint(0.5f);
	vec2 position(0.f);
	vec2 zoom(1.f);
	float rotation = 0.f;
	if (IsRunning()) {
		if (GetCurrentScene()) {
			Camera2D *cam = dynamic_cast<Camera2D *>(GetCurrentScene()->get_active_camera2d());
			if (cam) {
				centerPoint = cam->CenterPoint();

				position = cam->GlobalPosition();
				zoom = cam->Zoom();
				if (cam->Rotatable()) {
					rotation = glm::radians(cam->GlobalRotation());
				}
			}
		}
	} else {
		position = m_editorCameraPos2d;
		zoom = m_editorCameraZoom2d;
	}
	view = glm::translate(view, {position.x, position.y, 0.f});
	view = glm::rotate(view, glm::radians(rotation), {0.f, 0.f, 1.f});
	view = glm::scale(view, {zoom.x, zoom.y, 1.f});
	view = glm::translate(view, {-(1920.f * 0.5f), -(1080.f * 0.5f), 0.f});

	view = glm::inverse(view);

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 1920.f, 0.f, 1080.f, -128.f, 128.f));
	m_batchRenderer.GetShader().UniformMat4("uView", view);
	Renderer().Reset();
	glEnable(GL_DEPTH_TEST);

	if (m_pCurrentScene != nullptr) {
		m_pCurrentScene->UpdateScene();
	}
	Tweens::GetInstance().Poll(Time::Delta());

	if (!IsRunning()) {
		Renderer().PushLine(vec2(0.f, 0.f), vec2(0.f, 1080.f * 1000), 5.f, 0.6f, 0.2f, 0.2f, 100.f, 0.f);
		Renderer().PushLine(vec2(0.f, 0.f), vec2(1920.f * 1000, 0.f), 5.f, 0.2f, 0.8f, 0.4f, 100.f, 0.f);

		Sprite2D *selectedNode = dynamic_cast<Sprite2D *>(GetCurrentScene()->get_node_by_id(m_selectedNode));
		if (nullptr != selectedNode) {
			int w, h;
			RenderingServer::GetInstance().GetWindowSize(w, h);

			Texture *texture = ResourceManager::GetInstance().GetAs<Texture>(selectedNode->GetTexture());
			float width = 128.f;
			float height = 128.f;
			if (texture) {
				width = texture->Width();
				height = texture->Height();
			}
			glm::mat4 model = glm::scale(selectedNode->CalculateTransform(), {width, height, 1.f});

			glm::vec4 points[4] = {
				{-0.5f, 0.5f, 0.f, 1.f},
				{-0.5f, -0.5f, 0.f, 1.f},
				{0.5f, -0.5f, 0.f, 1.f},
				{0.5f, 0.5f, 0.f, 1.f}};

			for (int i = 0; i < 4; i++) {
				points[i] = model * points[i];
			}

			float minX = points[0].x;
			float minY = points[0].y;
			float maxX = points[0].x;
			float maxY = points[0].y;
			for (size_t i = 0; i < 4; i++) {
				minX = std::min(minX, points[i].x);
				minY = std::min(minY, points[i].y);
				maxX = std::max(maxX, points[i].x);
				maxY = std::max(maxY, points[i].y);
			}
			float padding = 10.f * m_editorCameraZoom2d;
			minX -= padding;
			maxX += padding;
			minY -= padding;
			maxY += padding;

			float thickness = m_editorCameraZoom2d * 5;
			Renderer().PushLine(vec2(minX, minY), vec2(maxX, minY), thickness, 0.2f, 0.6f, 0.8f, 2.f, 100.f);
			Renderer().PushLine(vec2(maxX, minY), vec2(maxX, maxY), thickness, 0.2f, 0.6f, 0.8f, 2.f, 100.f);
			Renderer().PushLine(vec2(maxX, maxY), vec2(minX, maxY), thickness, 0.2f, 0.6f, 0.8f, 2.f, 100.f);
			Renderer().PushLine(vec2(minX, maxY), vec2(minX, minY), thickness, 0.2f, 0.6f, 0.8f, 2.f, 100.f);
		}
	}

	Renderer().DrawText("Sowa Engine", &m_testFont, posX, 10.f, glm::mat4(1.f), 0.f, 1.f);
	Renderer().End();

	if (!IsRunning()) {
		double x, y;
		Input::GetWindowMousePosition(x, y);
		int w, h;
		RenderingServer::GetInstance().GetWindowSize(w, h);

		x *= (1920.f / static_cast<float>(w));
		y *= (1080.f / static_cast<float>(h));

		int id = m_layer2D.ReadAttachmentInt(1, static_cast<int>(x), static_cast<int>(y));
		if (id == 0)
			m_hoveredNode = 0;
		if (id == 0 && Input::IsButtonJustPressed(MB_LEFT) && this->m_editorState == EditorState::None)
			m_selectedNode = 0;

		if (id != 0 && GetCurrentScene() && nullptr != GetCurrentScene()->get_node_by_id(id)) {
			m_hoveredNode = static_cast<size_t>(id);
			if (m_editorState == EditorState::None && Input::IsButtonJustPressed(MB_LEFT)) {
				m_selectedNode = m_hoveredNode;
			}
		}

		if (Input::IsButtonJustPressed(MB_LEFT) && this->m_editorState != EditorState::None) {
			m_editorState = EditorState::None;
			RenderingServer::GetInstance().SetCursorMode(CursorMode::Normal);
		}
	}

	glDisable(GL_DEPTH_TEST);
	SetRenderLayer(nullptr);

	fullscreenShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layer2D.GetTargetTextureID(0));
	fullscreenModel.Draw();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layerUI.GetTargetTextureID(0));
	fullscreenModel.Draw();

	RenderingServer::GetInstance().SwapBuffers();
	InputServer::GetInstance().PollEvents();
}

void App::mainLoopCaller(void *self) {
	static_cast<App *>(self)->mainLoop();
}

void App::SetCurrentScene(Scene *scene) {
	if (m_pCurrentScene != nullptr) {
		m_pCurrentScene->EndScene();
	}

	m_pCurrentScene = scene;
	if (m_running)
		m_pCurrentScene->BeginScene();
}

void App::Start() {
	if (m_running)
		return;

	if (nullptr != m_pCurrentScene)
		Scene::copy(m_pCurrentScene, &m_backgroundScene);

	m_running = true;

	GetCurrentScene()->BeginScene();
}

void App::Stop() {
	if (!m_running)
		return;

	if (nullptr != m_pCurrentScene)
		Scene::copy(&m_backgroundScene, m_pCurrentScene);

	m_running = false;
}

void App::RegisterCommand(std::string command, std::function<void()> action) {
	CommandOption opt;
	opt.label = command;
	opt.action = action;

	m_commands.push_back(opt);
}

void App::SetCommandInterface(CommandInterface *interface) {
	if (nullptr != m_commandInterface)
		delete m_commandInterface;

	m_commandInterface = interface;
}

void App::reload_scripts() {
	//
	ScriptServer::GetInstance().BeginBuild();

	auto files = FileServer::GetInstance().ReadDir("res://scripts/", true);
	for (FileEntry &file : files) {
		if (file.IsFile() && file.Path().extension() == ".as")
			ScriptServer::GetInstance().LoadScriptFile(file.Path().string().c_str());
	}

	ScriptServer::GetInstance().EndBuild();
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