#include "app.hxx"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/projection.hpp"

#include "core/command/interfaces/command_palette.hxx"
#include "core/command/interfaces/scene_save_as.hxx"
#include "core/graphics.hxx"
#include "core/time.hxx"

#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

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

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>

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

		if (event.action == KEY_PRESSED && event.key == KEY_P && event.modifiers.control && event.modifiers.shift && nullptr == this->m_commandInterface) {
			SetCommandInterface(new CommandPaletteInterface);
		}

		if (event.action == KEY_PRESSED && event.key == KEY_ESCAPE) {
			if (nullptr != this->m_commandInterface) {
				SetCommandInterface(nullptr);
			} else {
				exit(0);
			}
		}

		if (event.action == KEY_PRESSED && event.key == KEY_DOWN && nullptr != this->m_commandInterface) {
			if (this->m_commandInterface->options.size() > 0) {
				this->m_commandInterface->currentIndex += 1;

				this->m_commandInterface->currentIndex %= this->m_commandInterface->options.size();
			}
		}

		if (event.action == KEY_PRESSED && event.key == KEY_UP && nullptr != this->m_commandInterface) {
			if (this->m_commandInterface->options.size() > 0) {
				this->m_commandInterface->currentIndex -= 1;

				if (this->m_commandInterface->currentIndex == -1)
					this->m_commandInterface->currentIndex = this->m_commandInterface->options.size() - 1;
			}
		}

		if (event.action == KEY_PRESSED && event.key == KEY_ENTER && nullptr != this->m_commandInterface) {
			CommandInterface *oldInterface = this->m_commandInterface;

			if (this->m_commandInterface->action) {
				this->m_commandInterface->action();
			}

			if (this->m_commandInterface->options.size() > static_cast<size_t>(this->m_commandInterface->currentIndex)) {
				std::function<void()> func = this->m_commandInterface->options[this->m_commandInterface->currentIndex].action;
				if (func)
					func();
			}

			// if user did not set command interface to another, delete it.
			if (oldInterface == this->m_commandInterface) {
				SetCommandInterface(nullptr);
			}
		}

		if ((event.action == KEY_PRESSED || event.action == KEY_REPEAT) && event.key == KEY_BACKSPACE && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
			if (this->m_commandInterface->text.size() > 0 && this->m_commandInterface->text_cursor != 0) {
				if (event.modifiers.control) {
					std::size_t index = std::string::npos;
					if (this->m_commandInterface->text[this->m_commandInterface->text_cursor - 1] == ' ') {
						index = this->m_commandInterface->text.find_last_not_of(' ', this->m_commandInterface->text_cursor - 1);

					} else {
						index = this->m_commandInterface->text.find_last_of(' ', this->m_commandInterface->text_cursor - 1);
					}
					if (index != std::string::npos && index + 1 < this->m_commandInterface->text.size()) {
						index += 1;
					}

					std::string remaining = this->m_commandInterface->text.substr(this->m_commandInterface->text_cursor);

					if (index == std::string::npos) {
						// has no space
						this->m_commandInterface->text = remaining;
						this->m_commandInterface->text_cursor = 0;
					} else {
						this->m_commandInterface->text = this->m_commandInterface->text.substr(0, index) + remaining;
						this->m_commandInterface->text_cursor = index;
					}
				} else {
					if (this->m_commandInterface->text_cursor == this->m_commandInterface->text.size())
						this->m_commandInterface->text = this->m_commandInterface->text.substr(0, this->m_commandInterface->text.size() - 1);
					else
						this->m_commandInterface->text = this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor - 1) + this->m_commandInterface->text.substr(this->m_commandInterface->text_cursor);

					if (this->m_commandInterface->text_cursor > 0)
						this->m_commandInterface->text_cursor -= 1;
				}
			}
		}

		if ((event.action == KEY_PRESSED || event.action == KEY_REPEAT) && event.key == KEY_RIGHT && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
			if (this->m_commandInterface->text.size() > 0) {
				this->m_commandInterface->text_cursor += 1;

				if (this->m_commandInterface->text_cursor > this->m_commandInterface->text.size())
					this->m_commandInterface->text_cursor = this->m_commandInterface->text.size();
			}
		}

		if ((event.action == KEY_PRESSED || event.action == KEY_REPEAT) && event.key == KEY_LEFT && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
			if (this->m_commandInterface->text_cursor > 0)
				this->m_commandInterface->text_cursor -= 1;
		}

		if (event.action == KEY_PRESSED && event.key == KEY_HOME && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
			this->m_commandInterface->text_cursor = 0;
		}

		if (event.action == KEY_PRESSED && event.key == KEY_END && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
			this->m_commandInterface->text_cursor = this->m_commandInterface->text.size();
		}
	});

	CharCallback().append([&](InputEventChar event) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::wstring wstr = {static_cast<wchar_t>(event.codepoint)};
		std::string str = converter.to_bytes(wstr);

		if (nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
			if (this->m_commandInterface->text_cursor == this->m_commandInterface->text.size())
				this->m_commandInterface->text += str;
			else
				this->m_commandInterface->text = this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor) + str + this->m_commandInterface->text.substr(this->m_commandInterface->text_cursor);

			this->m_commandInterface->text_cursor += 1;
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
		prop.set = [](Node *node, Property value) {                    \
			if (propType *v = std::any_cast<propType>(&value)) {       \
				dynamic_cast<type *>(node)->propAccessor = *v;         \
			}                                                          \
		};                                                             \
		db.BindProperty(db.GetNodeType(#type), propName, prop);        \
	} while (0);

	REGISTER_NODE_TYPE(Node, );
	REGISTER_NODE_PROPERTY(Node, "name", Name(), std::string);

	REGISTER_NODE_TYPE(Node2D, Node);
	REGISTER_NODE_PROPERTY(Node2D, "position", Position(), glm::vec2);
	REGISTER_NODE_PROPERTY(Node2D, "scale", Scale(), glm::vec2);
	REGISTER_NODE_PROPERTY(Node2D, "rotation", Rotation(), float);
	REGISTER_NODE_PROPERTY(Node2D, "z_index", ZIndex(), float);

	REGISTER_NODE_TYPE(Sprite2D, Node2D);
	REGISTER_NODE_PROPERTY(Sprite2D, "texture", GetTexture(), RID);

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

	RegisterCommand("Start/Stop Game", [&]() {
		if (IsRunning())
			Stop();
		else
			Start();
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
			Renderer().DrawText(interface->text, &m_testFont, textX, textY, glm::mat4(1.f), 0.f, textScale);

			static float f = 0.f;
			f += 0.5 * Time::Delta();
			float opacity = (std::sin(f) + 1.f) * 0.5f;

			const float cursorPadding = 5.f;
			const float cursorWidth = 2.f;
			float textCursorX = m_testFont.CalcTextSize(this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor)).x * textScale;
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