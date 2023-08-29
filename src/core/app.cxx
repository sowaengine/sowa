#include "app.hxx"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/projection.hpp"

#include "core/graphics.hxx"
#include "core/time.hxx"

#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "data/lrtb_flags.hxx"
#include "data/toml_document.hxx"

#include "ui/new_container.hxx"
#include "ui/new_tree.hxx"
#include "ui/ui_canvas.hxx"

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

#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef SW_WEB
#include <emscripten.h>
#endif

static App *s_instance;

App::App() {
	s_instance = this;
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
	ModelBuilder::Quad2D(rectModel);
	ModelBuilder::Quad2D(fullscreenModel, 2.f);

	err = mainShader.Load("res://shaders/main.vs", "res://shaders/main.fs");
	if (err != OK) {
		std::cerr << "Failed to load main shader" << err << std::endl;
	}

	err = fullscreenShader.Load("res://shaders/fullscreen.vs", "res://shaders/fullscreen.fs");
	if (err != OK) {
		std::cerr << "Failed to load fullscreen shader" << std::endl;
	}

	err = uiShader.Load("res://shaders/ui_panel.vs", "res://shaders/ui_panel.fs");
	if (err != OK) {
		std::cerr << "Failed to load ui shader" << std::endl;
	}

	// err = m_testTexture.Load(TextureType::Texture2D, "res://image.png");
	err = m_testTexture.Load(TextureType::Vector2D, "res://tanks.svg");
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

	KeyCallback().append([this](InputEventKey event) {
		if (event.action == KEY_PRESSED && event.key == KEY_F5) {
			if (IsRunning())
				Stop();
			else
				Start();
		}
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
	{
		{
			NodeFactory factory;
			factory.constructor = []() -> Node * {
				return new Node;
			};

			factory.destructor = [](Node *node) {
				delete node;
			};

			db.BindNodeType<Node>("Node", 0, factory);

			NodeProperty nameProp;
			nameProp.get = [](Node *node) -> Property {
				return node->Name();
			};
			nameProp.set = [](Node *node, Property value) {
				if (std::string *p = std::any_cast<std::string>(&value)) {
					node->Name() = *p;
				}
			};

			db.BindProperty(db.GetNodeType("Node"), "name", nameProp);
		}

		{
			NodeFactory factory;
			factory.constructor = []() -> Node * {
				return new Sprite2D;
			};

			factory.destructor = [](Node *node) {
				delete reinterpret_cast<Sprite2D *>(node);
			};

			db.BindNodeType<Sprite2D>("Sprite2D", db.GetNodeType("Node"), factory);

			{
				NodeProperty positionProp;
				positionProp.get = [](Node *node) -> Property {
					return dynamic_cast<Sprite2D *>(node)->Position();
				};
				positionProp.set = [](Node *node, Property value) {
					if (glm::vec2 *p = std::any_cast<glm::vec2>(&value)) {
						dynamic_cast<Sprite2D *>(node)->Position() = *p;
					}
				};

				db.BindProperty(db.GetNodeType("Sprite2D"), "position", positionProp);
			}
			{
				NodeProperty textureProp;
				textureProp.get = [](Node *node) -> Property {
					return dynamic_cast<Sprite2D *>(node)->GetTexture();
				};
				textureProp.set = [](Node *node, Property value) {
					if (RID *p = std::any_cast<RID>(&value)) {
						dynamic_cast<Sprite2D *>(node)->GetTexture() = *p;
					}
				};

				db.BindProperty(db.GetNodeType("Sprite2D"), "texture", textureProp);
			}
		}
	}

	Behaviour rotationBehaviour = Behaviour::New(nullptr, [](Node *node) {
		Sprite2D *sprite = dynamic_cast<Sprite2D *>(node);
		if (nullptr == sprite) {
			std::cout << "Behaviour (Rotate Sprite) invalid typename: " << NodeDB::GetInstance().GetNodeTypeName(node->TypeHash()) << std::endl;
			return;
		}

		sprite->Rotation() += 0.5f;
	});

	BehaviourDB::GetInstance().RegisterBehaviour("Rotate Sprite", rotationBehaviour);
	BehaviourDB::GetInstance().RegisterBehaviour("8 Dir Movement", Behaviour::New(TopDownEightDirMovement::Start, TopDownEightDirMovement::Update));

	Main();

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

	// double x, y;
	// InputServer::GetInstance().GetMousePosition(x, y);
	// x *= (1920.f / (float)w);
	// y *= (1080.f / (float)h);
	// m_hoveredItem = m_layer2D.ReadAttachmentInt(1, x, y);

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, static_cast<float>(w), 0.f, static_cast<float>(h)));
	// m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 1920.f, 0.f, 1080.f));
	Renderer().Reset();

	SetRenderLayer(&m_layer2D);
	m_layer2D.Clear(0.5f, 0.7f, 0.1f, 0.f, true);

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 1920.f, 0.f, 1080.f, -128.f, 128.f));
	Renderer().Reset();
	glEnable(GL_DEPTH_TEST);

	if (m_pCurrentScene != nullptr) {
		m_pCurrentScene->UpdateScene();
	}

	Renderer().DrawText("Sowa Engine", &m_testFont, 10.f, 10.f, glm::mat4(1.f), 0.f, 1.f);
	Renderer().End();

	glDisable(GL_DEPTH_TEST);
	SetRenderLayer(nullptr);

	fullscreenShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layerUI.GetTargetTextureID(0));
	fullscreenModel.Draw();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layer2D.GetTargetTextureID(0));
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
	m_pCurrentScene->BeginScene();
}

void App::Start() {
	if (m_running)
		return;

	if (nullptr != m_pCurrentScene)
		Scene::copy(m_pCurrentScene, &m_backgroundScene);

	m_running = true;
}

void App::Stop() {
	if (!m_running)
		return;

	if (nullptr != m_pCurrentScene)
		Scene::copy(&m_backgroundScene, m_pCurrentScene);

	m_running = false;
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