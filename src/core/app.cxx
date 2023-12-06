#include "app.hxx"

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>

#include "behaviour/behaviour.hxx"
#include "behaviour/behaviour_db.hxx"
#include "behaviour/builtin/top_down_eight_dir.hxx"

#include "core/command/interfaces/command_palette.hxx"
#include "core/command/interfaces/scene_save_as.hxx"
#include "core/graphics.hxx"
#include "core/store.hxx"
#include "core/time.hxx"
#include "core/tweens.hxx"

#include "scene/scene_tree.hxx"

#include "data/toml_document.hxx"
#include "game/game.hxx"
#include "scene/nodes.hxx"

#include "resource/resource.hxx"
#include "resource/resource_manager.hxx"
#include "resource/sprite_sheet_animation/sprite_sheet_animation.hxx"

#include "servers/audio_server.hxx"
#include "servers/file_server.hxx"
#include "servers/gui_server.hxx"
#include "servers/input_server.hxx"
#include "servers/lua_server.hxx"
#include "servers/physics_server_2d.hxx"
#include "servers/rendering_server.hxx"
#include "servers/script_server.hxx"

#include "ui/new_container.hxx"
#include "ui/new_tree.hxx"
#include "ui/ui_canvas.hxx"

#include "utils/utils.hxx"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/projection.hpp"

// #define SW_TEMPLATE

#include "editor/editor_config.hxx"

#ifdef SW_WEB
#include <emscripten.h>
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// #include "mz.h"
// #include "mz_strm.h"
// #include "mz_strm_mem.h"
// #include "mz_zip.h"
// #include "mz_zip_rw.h"
// #include "unzip.h"
// #include "zip.h"

static App *s_instance;

App::App() {
	s_instance = this;
	Utils::Randomize();
}

App::~App() {
}

App &App::get() {
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

ErrorCode App::Init() {

#ifndef SW_WEB
	// m_appPath = "./project";
#else
	// #error project picker should be added // FIXME
	m_appPath = "/app";
#endif

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

	ErrorCode err;

	EditorConfig::get().Load();
	EditorConfig::get().Save();

	if (EditorConfig::get().auto_open_last_project && EditorConfig::get().last_project != "") {
		LoadProjectFromPath(EditorConfig::get().last_project);
	} else {
		LoadProjectFromDialog();
	}

	RenderingServer::get().create_window(m_projectSettings.window_width, m_projectSettings.window_height, m_projectSettings.app_name);

	m_layer2D.SetTarget(0, RenderLayerTargetType::Vec4);
	m_layer2D.SetTarget(1, RenderLayerTargetType::Int);
	m_layer2D.Create(1920, 1080);

	vec2 windowSize = RenderingServer::get().GetWindowSize();

	AudioServer::get();
	PhysicsServer2D::get().init();

	m_layerUI.SetTarget(0, RenderLayerTargetType::Vec4);
	m_layerUI.SetTarget(1, RenderLayerTargetType::Int);
	m_layerUI.Create(windowSize.x, windowSize.y);

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

	load_resources();

	Time::update();

	WindowResizeCallback().append([this](int width, int height) {
		this->m_layerUI.Resize(width, height);
	});

#ifndef SW_TEMPLATE
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
			// Tweens::get().RegisterTween(2.f, [](float f) {
			//    App::get().SceneTree::get().get_scene()->get_active_camera2d()->Zoom() = 1.f - (f * 0.5f);
			// });
		}

		if (event.action == KEY_PRESSED && event.modifiers.shift && event.key == KEY_R) {
			if (IsRunning())
				Stop();
			else
				Start();
		}

		if (event.action == KEY_PRESSED && event.key == KEY_R && !this->IsRunning() && event.modifiers.alt) {
			SceneTree::get().get_scene()->load(SceneTree::get().get_scene()->Path().c_str());
			this->reload_scripts();
		}

		if (event.action == KEY_PRESSED && event.key == KEY_S && event.modifiers.control) {
			ErrorCode err = SceneTree::get().get_scene()->save(SceneTree::get().get_scene()->Path().c_str());
			if (err != OK) {
				std::cout << "Failed to save scene " << err << std::endl;
			}
		}

		command_interface_key_callback(event);
	});

	CharCallback().append([&](InputEventChar event) {
		command_interface_char_callback(event);
	});
#endif

	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	ResourceFactory imageTexture;
	imageTexture.typeName = "ImageTexture";
	imageTexture.type = ResourceType_ImageTexture;
	imageTexture.loadFunc = [](Resource *res, std::string path) {
		res->Data() = Texture();
		if (Texture *tex = res->As<Texture>(); nullptr != tex) {
			ErrorCode err = tex->Load(TextureType::Texture2D, path.c_str());
			if (err != OK) {
				std::cout << "Failed to load texture: " << err << "(" << path << ")" << std::endl;
			}
		}
	};
	ResourceManager::get().RegisterResource(".png", imageTexture);

	ResourceFactory audioStream;
	audioStream.typeName = "AudioStream";
	audioStream.type = ResourceType_AudioStream;
	audioStream.loadFunc = [](Resource *res, std::string path) {
		res->Data() = AudioStream();
		if (AudioStream *stream = res->As<AudioStream>(); nullptr != stream) {
			ErrorCode err = stream->Load(path.c_str());
			if (err != OK) {
				std::cout << "Failed to load audio stream: " << err << "(" << path << ")" << std::endl;
			}
		}
	};
	ResourceManager::get().RegisterResource(".ogg", audioStream);
	ResourceManager::get().RegisterResource(".wav", audioStream);

	ResourceFactory spriteSheetAnimation;
	spriteSheetAnimation.typeName = "SpriteSheetAnimation";
	spriteSheetAnimation.type = ResourceType_SpriteSheetAnimation;
	spriteSheetAnimation.loadFunc = [](Resource *res, std::string path) {
		res->Data() = SpriteSheetAnimation();
		if (SpriteSheetAnimation *anim = res->As<SpriteSheetAnimation>(); nullptr != anim) {
			ErrorCode err = anim->Load(path.c_str());
			if (err != OK) {
				std::cout << "Failed to load sprite sheet animation: " << err << std::endl;
			}
		}
	};
	ResourceManager::get().RegisterResource(".anim", spriteSheetAnimation);

	NodeDB &db = NodeDB::get();

#define REGISTER_NODE_TYPE(type, extends)                                    \
	do {                                                                     \
		NodeFactory factory;                                                 \
		factory.constructor = []() -> Node * {                               \
			return new type;                                                 \
		};                                                                   \
		factory.destructor = [](Node *node) {                                \
			delete reinterpret_cast<type *>(node);                           \
		};                                                                   \
                                                                             \
		db.bind_node_type<type>(#type, db.get_node_type(#extends), factory); \
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
		db.bind_property(db.get_node_type(#type), propName, prop);     \
	} while (0);

	REGISTER_NODE_TYPE(Node, );
	REGISTER_NODE_PROPERTY(Node, "name", name(), std::string);
	REGISTER_NODE_PROPERTY(Node, "visible", visible(), bool);
	NodeTypeName &nt_node = NodeTypeNames().emplace_back("Node", true);

	REGISTER_NODE_TYPE(AudioStreamPlayer, Node);
	REGISTER_NODE_PROPERTY(AudioStreamPlayer, "stream", stream(), RID);
	REGISTER_NODE_PROPERTY(AudioStreamPlayer, "autoplay", autoplay(), bool);
	nt_node.children.emplace_back("AudioStreamPlayer", true);

	REGISTER_NODE_TYPE(Node2D, Node);
	REGISTER_NODE_PROPERTY(Node2D, "position", position(), vec2);
	REGISTER_NODE_PROPERTY(Node2D, "scale", scale(), vec2);
	REGISTER_NODE_PROPERTY(Node2D, "rotation", rotation(), float);
	REGISTER_NODE_PROPERTY(Node2D, "z_index", z_index(), float);
	NodeTypeName &nt_node2d = NodeTypeNames().emplace_back("Node2D", true);

	REGISTER_NODE_TYPE(PhysicsBody2D, Node2D);
	REGISTER_NODE_PROPERTY(PhysicsBody2D, "body_type", body_type(), PhysicsBodyType);
	nt_node2d.children.emplace_back("PhysicsBody2D", true);

	REGISTER_NODE_TYPE(RectCollider2D, Node2D);
	REGISTER_NODE_PROPERTY(RectCollider2D, "width", width(), float);
	REGISTER_NODE_PROPERTY(RectCollider2D, "height", height(), float);
	nt_node2d.children.emplace_back("RectCollider2D", true);

	REGISTER_NODE_TYPE(CircleCollider2D, Node2D);
	REGISTER_NODE_PROPERTY(CircleCollider2D, "radius", radius(), float);
	nt_node2d.children.emplace_back("CircleCollider2D", true);

	REGISTER_NODE_TYPE(Sprite2D, Node2D);
	REGISTER_NODE_PROPERTY(Sprite2D, "texture", texture(), RID);
	nt_node2d.children.emplace_back("Sprite2D", true);

	REGISTER_NODE_TYPE(Text2D, Node2D);
	REGISTER_NODE_PROPERTY(Text2D, "color_red", color_red(), float);
	REGISTER_NODE_PROPERTY(Text2D, "color_green", color_green(), float);
	REGISTER_NODE_PROPERTY(Text2D, "color_blue", color_blue(), float);
	REGISTER_NODE_PROPERTY(Text2D, "color_alpha", color_alpha(), float);
	REGISTER_NODE_PROPERTY(Text2D, "text", text(), std::string);
	nt_node2d.children.emplace_back("Text2D", true);

	REGISTER_NODE_TYPE(AnimatedSprite2D, Node2D);
	REGISTER_NODE_PROPERTY(AnimatedSprite2D, "spritesheet", spritesheet(), RID);
	REGISTER_NODE_PROPERTY(AnimatedSprite2D, "speed", speed(), float);
	REGISTER_NODE_PROPERTY(AnimatedSprite2D, "playing", playing(), bool);
	nt_node2d.children.emplace_back("AnimatedSprite2D", true);

	REGISTER_NODE_TYPE(Camera2D, Node2D);
	REGISTER_NODE_PROPERTY(Camera2D, "zoom", zoom(), vec2);
	nt_node2d.children.emplace_back("Camera2D", true);

	BehaviourDB::get().RegisterBehaviour("8 Dir Movement", Behaviour::New(TopDownEightDirMovement::Start, TopDownEightDirMovement::Update));

	LuaServer::get().Init();
	reload_scripts();
	// Main();

	RegisterCommand("Start/Stop Game", [&]() {
		if (IsRunning())
			Stop();
		else
			Start();
	});
	RegisterCommand("Rebuild Scripts", [&]() {
		reload_scripts();

		if (IsRunning()) {
			std::function<void(Node *)> func;
			func = [&func](Node *node) {
				node->reload_behaviours();

				for (Node *child : node->get_children()) {
					func(child);
				}
			};

			func(SceneTree::get().get_scene()->Root());
		}
	});
	RegisterCommand("Save Scene", [&]() {
		if (SceneTree::get().get_scene()->Path() == "") {
			SetCommandInterface(new SceneSaveAsInterface(""));
			return;
		}

		ErrorCode err = SceneTree::get().get_scene()->save(SceneTree::get().get_scene()->Path().c_str());
		if (err != OK) {
			std::cout << "Failed to save scene " << err << std::endl;
		}
	});
	RegisterCommand("Save Scene As", [&]() {
		SetCommandInterface(new SceneSaveAsInterface(SceneTree::get().get_scene()->Path()));
	});
	RegisterCommand("Exit", [&]() {
		exit(0);
	});

#ifndef SW_TEMPLATE
	GuiServer::get().Initialize();
#endif

	if (m_projectSettings.config_main_scene != "") {
		ErrorCode err = SceneTree::get().get_scene()->load(m_projectSettings.config_main_scene.c_str());
		if (err != OK) {
			std::cout << "Failed to load scene" << std::endl;
		}
	}

#ifdef SW_TEMPLATE
	Start();
#endif

	m_game_scene = SceneTree::get().m_scene;
#ifndef SW_TEMPLATE
	Scene::copy(m_game_scene, &m_editor_scene);
	SceneTree::get().m_scene = &m_editor_scene;
#endif

	return OK;
}

ErrorCode App::Run() {
#ifdef SW_WEB
	emscripten_set_main_loop_arg(mainLoopCaller, this, 0, 1);
#else
	while (!RenderingServer::get().WindowShouldClose()) {
		mainLoop();
	}
#endif
	return OK;
}

void App::SetRenderLayer(RenderLayer *renderlayer) {
	if (nullptr == renderlayer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		vec2 windowSize = RenderingServer::get().GetWindowSize();

		float windowRatio = (float)windowSize.x / windowSize.y;
		float videoRatio = (float)1920.f / 1080.f;

		if (windowRatio > videoRatio) {
			float width = windowSize.y * videoRatio;
			float height = windowSize.y;
			float gap = windowSize.x - width;

			float x = gap / 2.f;
			glViewport(x, 0.f, width, height);
			m_viewportRect = rect(x, 0.f, width, height);

		} else {
			float width = windowSize.x;
			float height = windowSize.x / videoRatio;
			float gap = windowSize.y - height;

			float y = gap / 2.f;
			glViewport(0.f, y, width, height);
			m_viewportRect = rect(0.f, y, width, height);
		}

		return;
	}

	renderlayer->Bind();
}

void App::mainLoop() {
	if (m_scene_to_load != "") {
		SceneTree::get().get_scene()->_end_scene();
		SceneTree::get().get_scene()->load(m_scene_to_load.c_str());
		SceneTree::get().get_scene()->_begin_scene();

		m_scene_to_load = "";
	}

	InputServer::get().ProcessInput();
	glClearColor(0.28f, 0.28f, 0.28f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	Time::update();

	SetRenderLayer(&m_layerUI);
	m_layerUI.Clear(0.0f, 0.0f, 0.0f, 0.0f);

	vec2 windowSize = RenderingServer::get().GetWindowSize();

	glViewport(0, 0, windowSize.x, windowSize.y);

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, static_cast<float>(windowSize.x), 0.f, static_cast<float>(windowSize.y), -128.f, 128.f));
	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));
	Renderer().Reset();

#ifndef SW_TEMPLATE
	GuiServer::get().Begin();
	GuiServer::get().Update();
	GuiServer::get().End();
#endif

	if (nullptr != m_commandInterface) {
		CommandInterface *cmd_interface = m_commandInterface;

		glm::vec3 normalColor(0.12f, 0.12f, 0.12f);
		glm::vec3 hoveredColor(0.2f, 0.2f, 0.2f);

		const float padding = 8.f;
		const float xPos = windowSize.x / 3.f;
		const float width = windowSize.x / 3.f;
		const float textScale = 0.4f;

		float cursorY = windowSize.y;

		if (cmd_interface->text_input) {
			glm::vec2 textSize = m_testFont.CalcTextSize("I") * textScale;
			float height = textSize.y + (padding * 2);
			cursorY -= height;

			float textX = xPos + padding;
			float textY = cursorY + padding;

			const float outlineSize = 2.f;

			Renderer().PushQuad(xPos, cursorY, 0.f, width, height, 0.6, 0.6, 0.6, 1.f, 0.f, 0.f);
			Renderer().PushQuad(xPos + outlineSize, cursorY + outlineSize, 0.f, width - (outlineSize * 2), height - (outlineSize * 2), normalColor.x, normalColor.y, normalColor.z, 1.f, 0.f, 0.f);
			if (cmd_interface->text.size() > 0) {
				std::string str = cmd_interface->text;

				Renderer().draw_text(str, &m_testFont, textX, textY, 0.f, glm::mat4(1.f), 0.f, textScale);
			} else if (cmd_interface->text_placeholder.size() > 0) {
				std::string str = cmd_interface->text_placeholder;

				Renderer().draw_text(str, &m_testFont, textX, textY, 0.f, glm::mat4(1.f), 0.f, textScale, 0.f, 0.f, 1.f, 1.f, 1.f, 0.6f);
			}

			static float f = 0.f;
			f += 0.5 * Time::Delta();
			float opacity = (std::sin(f) + 1.f) * 0.5f;

			const float cursorPadding = 5.f;
			const float cursorWidth = 2.f;

			std::string str = this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor);
			float textCursorX = m_testFont.CalcTextSize(str).x * textScale;
			Renderer().PushQuad(xPos + textCursorX + textSize.x + 2.f, cursorY + cursorPadding, 0.f, cursorWidth, height - (cursorPadding * 2), 0.8f, 0.8f, 0.8f, opacity, 0.f, 0.f);
		}

		int index = -1;
		for (CommandOption &opt : cmd_interface->options) {
			index++;
			glm::vec2 size = m_testFont.CalcTextSize("I") * textScale;

			float height = size.y + (padding * 2);
			cursorY -= height;

			float textX = xPos + padding;
			float textY = cursorY + padding;

			glm::vec3 color = normalColor;
			if (index == cmd_interface->currentIndex)
				color = hoveredColor;

			Renderer().PushQuad(xPos, cursorY, 0.f, width, height, color.x, color.y, color.z, 1.f, 0.f, 0.f);
			Renderer().draw_text(opt.label, &m_testFont, textX, textY, 0.f, glm::mat4(1.f), 0.f, textScale);
		}
	}

	/*
	m_gui.PutWindowSize(w, h);
	m_gui.state_lmouse_just_pressed = Input::IsButtonJustPressed(MB_LEFT);
	m_gui.state_lmouse_down = Input::IsButtonDown(MB_LEFT);
	m_gui.state_mouse_pos = Input::GetMousePosition();
	m_gui.state_mouse_motion = Input::GetMouseMotion();

	m_gui.BeginWindow("Console");
	for (auto &line : m_consoleBuffer) {
		m_gui.Text(line);
	}
	m_gui.EndWindow();

	m_gui.Update();
	GuiCursorState state = m_gui.GetCursorState();
	if (state == GuiCursorState::Resize) {
		m_cursorStyle = CursorStyle::Resize;
	} else if (state == GuiCursorState::Resize_X) {
		m_cursorStyle = CursorStyle::ResizeX;
	} else if (state == GuiCursorState::Resize_Y) {
		m_cursorStyle = CursorStyle::ResizeY;
	}
	*/

	Renderer().End();

	SetRenderLayer(&m_layer2D);
	m_layer2D.Clear(0.5f, 0.7f, 0.1f, 0.f, true);

	glm::mat4 view = glm::mat4(1.f);
	vec2 centerPoint(0.f);
	if (!IsRunning()) {
		centerPoint = vec2(0.5f);
	}

	vec2 position(0.f);
	vec2 zoom(1.f);
	float rotation = 0.f;
	if (IsRunning()) {
		Camera2D *cam = dynamic_cast<Camera2D *>(SceneTree::get().get_scene()->get_active_camera2d());
		if (cam) {
			centerPoint = cam->center_point();

			position = cam->global_position();
			zoom = cam->zoom();
			if (cam->rotatable()) {
				rotation = glm::radians(cam->global_rotation());
			}
		}
	} else {
		position = m_editorCameraPos2d;
		zoom = m_editorCameraZoom2d;
	}
	view = glm::translate(view, {position.x, position.y, 0.f});
	view = glm::rotate(view, glm::radians(rotation), {0.f, 0.f, 1.f});
	view = glm::scale(view, {zoom.x, zoom.y, 1.f});
	view = glm::translate(view, {-(1920.f * centerPoint.x), -(1080.f * centerPoint.y), 0.f});

	view = glm::inverse(view);

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 1920.f, 0.f, 1080.f, -128.f, 128.f));
	m_batchRenderer.GetShader().UniformMat4("uView", view);
	Renderer().Reset();
	glEnable(GL_DEPTH_TEST);

	SceneTree::get()._update_globals();
	SceneTree::get().get_scene()->_update_scene();

	if (IsRunning()) {
		PhysicsServer2D::get().step();
	}
	PhysicsServer2D::get().debug_draw();

	Tweens::get().Poll(Time::Delta());

	if (!IsRunning()) {
		rect cam = rect(m_editorCameraPos2d.x, m_editorCameraPos2d.y, 1920.f * m_editorCameraZoom2d, 1080.f * m_editorCameraZoom2d);

		vec2 begin = vec2(cam.x - (cam.w * 0.5f), cam.y - (cam.h * 0.5f));
		vec2 end = vec2(cam.x + (cam.w * 0.5f), cam.y + (cam.h * 0.5f));

		float grid_spacing = 500.f;
		float thickness = 3.f * m_editorCameraZoom2d;
		for (float f = begin.x - fmodf(begin.x, grid_spacing); f < end.x; f += grid_spacing) {
			Renderer().PushLine(vec2(f, cam.y - cam.h), vec2(f, cam.y + cam.h), thickness, 1.f, 1.f, 1.f, .3f, 100.f);
		}
		for (float f = begin.y - fmodf(begin.y, grid_spacing); f < end.y; f += grid_spacing) {
			Renderer().PushLine(vec2(cam.x - cam.w, f), vec2(cam.x + cam.w, f), thickness, 1.f, 1.f, 1.f, .3f, 100.f);
		}

		thickness = 7.f * m_editorCameraZoom2d;

		Renderer().PushLine(vec2(0.f, 0.f), vec2(0.f, 1080.f * 1000), thickness, 0.6f, 0.2f, 0.2, 1.f, 101.f);
		Renderer().PushLine(vec2(0.f, 0.f), vec2(1920.f * 1000, 0.f), thickness, 0.2f, 0.8f, 0.4f, 1.f, 101.f);

		Renderer().PushLine(vec2(0.f, 0.f), vec2(0.f, 1080.f), thickness, 0.1f, 0.3f, 0.6, 1.f, 100.f);
		Renderer().PushLine(vec2(0.f, 1080.f), vec2(1920.f, 1080.f), thickness, 0.1f, 0.3f, 0.6, 1.f, 100.f);
		Renderer().PushLine(vec2(1920.f, 1080.f), vec2(1920.f, 0.f), thickness, 0.1f, 0.3f, 0.6, 1.f, 100.f);
		Renderer().PushLine(vec2(1920.f, 0.f), vec2(0.f, 0.f), thickness, 0.1f, 0.3f, 0.6, 1.f, 100.f);

		Node *selectedNode = SceneTree::get().get_scene()->get_node_by_id(m_selectedNode);
		Node2D *selectedNode2D = dynamic_cast<Node2D *>(selectedNode);
		if (nullptr != selectedNode2D) {
			vec2 pos = selectedNode2D->global_position();

			float length = 20.f * m_editorCameraZoom2d;
			float thickness = m_editorCameraZoom2d * 5;
			Renderer().PushLine(vec2(pos.x - length, pos.y),
								vec2(pos.x + length, pos.y), thickness, 0.83f, 0.62f,
								0.3f, 2.f, 100.f);
			Renderer().PushLine(vec2(pos.x, pos.y - length),
								vec2(pos.x, pos.y + length), thickness, 0.83f, 0.62f,
								0.3f, 2.f, 100.f);
		}

		Sprite2D *selectedSprite2D = dynamic_cast<Sprite2D *>(selectedNode);
		if (nullptr != selectedSprite2D) {
			Texture *texture =
				ResourceManager::get().GetAs<Texture>(selectedSprite2D->texture());
			float width = 128.f;
			float height = 128.f;
			if (texture) {
				width = texture->Width();
				height = texture->Height();
			}
			glm::mat4 model =
				glm::scale(selectedSprite2D->calculate_transform(), {width, height, 1.f});

			glm::vec4 points[4] = {{-0.5f, 0.5f, 0.f, 1.f},
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
			Renderer().PushLine(vec2(minX, minY), vec2(maxX, minY), thickness, 0.2f,
								0.6f, 0.8f, 2.f, 100.f);
			Renderer().PushLine(vec2(maxX, minY), vec2(maxX, maxY), thickness, 0.2f,
								0.6f, 0.8f, 2.f, 100.f);
			Renderer().PushLine(vec2(maxX, maxY), vec2(minX, maxY), thickness, 0.2f,
								0.6f, 0.8f, 2.f, 100.f);
			Renderer().PushLine(vec2(minX, maxY), vec2(minX, minY), thickness, 0.2f,
								0.6f, 0.8f, 2.f, 100.f);
		}
	}

#if false
	static std::vector<float> values;
	int easingCount = (int)Utils::Easing::EASING_COUNT;
	if (values.size() == 0) {
		for (int i = 0; i < easingCount; i++) {
			values.push_back(0.f);
		}
	}

	if (Input::IsKeyJustPressed(KEY_Z)) {
		values.clear();

		for (int i = 0; i < easingCount; i++) {
			values.push_back(0.f);
			Tweens::get().RegisterTween(
				2.f, [i](float f) {
					values[i] = f;
				},
				(Utils::Easing)i);
		}
	}
	for (size_t i = 0; i < values.size(); i++) {
		static std::vector<std::string> names = {
			"Linear",
			"Sine in", "Sine out", "Sine in/out",
			"Cubic in", "Cubic out", "Cubic in/out",
			"Quint in", "Quint out", "Quint in/out",
			"Circ in", "Circ out", "Circ in/out",
			"Elastic in", "Elastic out", "Elastic in/out",
			"Quad in", "Quad out", "Quad in/out",
			"Quart in", "Quart out", "Quart in/out",
			"Expo in", "Expo out", "Expo in/out",
			"Back in", "Back out", "Back in/out",
			"Bounce in", "Bounce out", "Bounce in/out"};

		Renderer().draw_text(names.at(i), &m_testFont, -100.f, -280 - (i * 60.f), 0.f, glm::mat4(1.f), 0.f, 1.f);

		Renderer().PushQuad(200.f + (values[i] * 1000.f), -300 - (i * 60.f), 0.f, 64.f, 64.f, 1.f, 1.f, 1.f, 1.f, 0.f, static_cast<float>(m_testTexture.ID()));
	}
#endif

	Renderer().End();

	if (!IsRunning()) {
		vec2 windowSize = RenderingServer::get().GetWindowSize();
		// vec2 cursorPos = ViewportRect().map_point(vec2(x, y), rect(0.f, 0.f, 1920.f, 1080.f));
		vec2 cursorPos = Input::GetWindowMousePosition();
		cursorPos.x *= 1920.f / (float)windowSize.x;
		cursorPos.y *= 1080.f / (float)windowSize.y;

		int id = m_layer2D.ReadAttachmentInt(1, static_cast<int>(cursorPos.x),
											 static_cast<int>(cursorPos.y));
		if (id == 0)
			m_hoveredNode = 0;
		if (id == 0 && Input::IsButtonJustClicked(MB_LEFT) &&
			this->m_editorState == EditorState::None && Input::IsCursorInside())
			m_selectedNode = 0;

		if (id != 0 && nullptr != SceneTree::get().get_scene()->get_node_by_id(id)) {
			m_hoveredNode = static_cast<size_t>(id);
			if (m_editorState == EditorState::None &&
				Input::IsButtonJustClicked(MB_LEFT) && Input::IsCursorInside()) {
				if (m_selectedNode == m_hoveredNode)
					m_selectedNode = 0;
				else
					m_selectedNode = m_hoveredNode;
			}
		}

		if (Input::IsButtonJustClicked(MB_LEFT) &&
			this->m_editorState != EditorState::None) {
			m_editorState = EditorState::None;
			RenderingServer::get().SetCursorMode(CursorMode::Normal);
		}
	}

	process_tool_routines();
	if (IsRunning())
		process_routines();

	glDisable(GL_DEPTH_TEST);
	SetRenderLayer(nullptr);

	fullscreenShader.Bind();

#ifdef SW_TEMPLATE
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layer2D.GetTargetTextureID(0));
	fullscreenModel.Draw();
#endif

	glViewport(0, 0, windowSize.x, windowSize.y);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layerUI.GetTargetTextureID(0));
	fullscreenModel.Draw();

	RenderingServer::get().SetCursorStyle(m_cursorStyle);
	m_cursorStyle = CursorStyle::Normal;

	RenderingServer::get().SwapBuffers();
	InputServer::get().PollEvents();
}

void App::mainLoopCaller(void *self) {
	static_cast<App *>(self)->mainLoop();
}

/*
void App::SetCurrentScene(Scene *scene) {
	if (IsRunning()) {
		SceneTree::get().get_scene()->_end_scene();
	}

	SceneTree::get().m_scene = scene;
	if (IsRunning())
		SceneTree::get().get_scene()->_begin_scene();
}
*/

void App::load_scene(const std::string &path) {
	m_scene_to_load = path;
}

void App::Start() {
	// if (nullptr == GetCurrentScene()) {
	// 	Utils::Error("Failed to start scene: no scene is active");
	// 	return;
	// }

	if (m_running)
		return;
	m_running = true;

	SceneTree::get().m_scene = m_game_scene;
	Scene::copy(&m_editor_scene, SceneTree::get().m_scene);
	SceneTree::get()._begin_globals();
	SceneTree::get().get_scene()->_begin_scene();
}

void App::Stop() {
#ifdef SW_TEMPLATE
	return;
#endif

	if (!m_running)
		return;

	Store<std::string, std::string>::get().clear();
	Tweens::get().clear();

	SceneTree::get().get_scene()->_end_scene();
	SceneTree::get()._end_globals();
	SceneTree::get().m_scene = &m_editor_scene;

	m_running = false;
}

void App::RegisterCommand(std::string command, std::function<void()> action) {
	CommandOption opt;
	opt.label = command;
	opt.action = action;

	m_commands.push_back(opt);
}

void App::SetCommandInterface(CommandInterface *cmd_interface) {
	if (nullptr != m_commandInterface)
		delete m_commandInterface;

	m_commandInterface = cmd_interface;
}

void App::Log(const std::string &message) {
	m_consoleBuffer.emplace_back(message);
	if (m_consoleBuffer.size() > m_consoleBufferSize) {
		m_consoleBuffer.pop_front();
	}
}

void App::reload_scripts() {
	//
	/*
	ScriptServer::get().BeginBuild();

	auto files = FileServer::get().ReadDir("res://scripts/", true);
	for (FileEntry &file : files) {
		if (file.IsFile() && file.Path().extension() == ".as")
			ScriptServer::get().LoadScriptFile(file.Path().string().c_str());
	}

	ScriptServer::get().EndBuild();
	*/
}

void App::LoadProjectFromPath(const std::filesystem::path &path) {
	m_appPath = path.parent_path();
	ErrorCode err = m_projectSettings.Load(path.c_str());
	if (err != OK) {
		Utils::Error("Failed to open project at {}", path.string());
		exit(1);
	}

	EditorConfig::get().last_project = path.string();
	EditorConfig::get().Save();
}

void App::LoadProjectFromDialog() {
	std::filesystem::path path = Utils::OpenFileDialog("Load Project File", {"project.sowa"}, "project.sowa File", false);
	if (path.empty()) {
		Utils::Error("Invalid project file");
		exit(1);
	}

	LoadProjectFromPath(path);
}

void App::register_routine(int frame_interval, std::function<void()> func) {
	RoutineCounter counter;
	counter.current_interval = 0;
	counter.interval = frame_interval;
	counter.func = func;

	m_routines.push_back(counter);
}

void App::register_tool_routine(int frame_interval, std::function<void()> func) {
	RoutineCounter counter;
	counter.current_interval = 0;
	counter.interval = frame_interval;
	counter.func = func;

	m_tool_routines.push_back(counter);
}

void App::process_routines() {
	for (RoutineCounter &routine : m_routines) {
		routine.current_interval++;

		if (routine.current_interval >= routine.interval) {
			routine.current_interval = 0;

			if (routine.func) {
				routine.func();
			}
		}
	}
}

void App::process_tool_routines() {
	for (RoutineCounter &routine : m_tool_routines) {
		routine.current_interval++;

		if (routine.current_interval >= routine.interval) {
			routine.current_interval = 0;

			if (routine.func) {
				routine.func();
			}
		}
	}
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