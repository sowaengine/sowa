#ifndef SW_APP_HXX
#define SW_APP_HXX
#pragma once

#include "core/command/command_interface.hxx"
#include "core/error/error.hxx"
#include "core/graphics.hxx"

#include "data/project_settings.hxx"

#include <deque>
#include <filesystem>

#include "data/input_event/input_event.hxx"
#include "gui/gui.hxx"
#include "resource/font/font.hxx"
#include "scene/scene.hxx"
#include "servers/rendering_server.hxx"
#include "ui/new_tree.hxx"

#include "eventpp/callbacklist.h"

struct NodeTypeName {
	std::string name;
	std::vector<NodeTypeName> children;
	bool addable_to_scene = false;

	NodeTypeName(const std::string &name, bool addable_to_scene)
		: name(name), addable_to_scene(addable_to_scene) {}
};

class App {
  public:
	App();
	~App();

	ErrorCode Init();
	ErrorCode Run();

	static App &get();

	inline project_settings &ProjectSettings() { return m_projectSettings; }
	inline std::vector<NodeTypeName> &NodeTypeNames() { return m_node_type_names; }

	void SetRenderLayer(RenderLayer *renderlayer);

	inline BatchRenderer &Renderer() { return m_batchRenderer; }

	inline eventpp::CallbackList<void(InputEventMouseButton)> &MouseInputCallback() { return m_mouseInputCallback; }
	inline eventpp::CallbackList<void(InputEventMouseMove)> &MouseMoveCallback() { return m_mouseMoveCallback; }
	inline eventpp::CallbackList<void(InputEventKey)> &KeyCallback() { return m_keyCallback; }
	inline eventpp::CallbackList<void(InputEventChar)> &CharCallback() { return m_charCallback; }
	inline eventpp::CallbackList<void(InputEventScroll)> &ScrollCallback() { return m_scrollCallback; }
	inline eventpp::CallbackList<void(InputEventClick)> &ClickCallback() { return m_clickCallback; }

	inline eventpp::CallbackList<void(int, int)> &WindowResizeCallback() { return m_windowResizeCallback; }

	void load_scene(const std::string &path);

	Font &TestFont() { return m_testFont; }

	inline bool IsRunning() const { return m_running; }
	rect ViewportRect() const { return m_viewportRect; }
	void Start();
	void Stop();

	void RegisterCommand(std::string command, std::function<void()> action);
	inline const std::vector<CommandOption> &Commands() { return m_commands; }
	void SetCommandInterface(CommandInterface *cmd_interface);

	void Log(const std::string &message);
	inline std::deque<std::string> &GetConsoleBuffer() { return m_consoleBuffer; }

	inline void SelectNode(size_t id) { m_selectedNode = id; }
	inline size_t GetSelectedNode() { return m_selectedNode; }

	// Register a routine that runs each {frame_interval} frame
	void register_routine(int frame_interval, std::function<void()> func);

	// Same as register_routine, but routines registered as tool will be executed
	// even when the game is not running (used in building editor tools)
	void register_tool_routine(int frame_interval, std::function<void()> func);

  private:
	void mainLoop();
	static void mainLoopCaller(void *self);

	void reload_scripts();

  private:
	// Editor
	Scene m_editor_scene;
	Scene *m_game_scene = nullptr; // stores scene pointer that SceneTree allocates on start
	enum class EditorState {
		None = 0,
		Dragging,
		Scaling
	};
	enum class EditorActionAxis {
		None = 0b11,
		X = 0b01,
		Y = 0b10
	};
	size_t m_selectedNode = 0;
	size_t m_hoveredNode = 0;
	size_t m_hovered_ui_item = 0;

	vec2 m_editorCameraPos2d = vec2(0.f, 0.f);
	float m_editorCameraZoom2d = 1.f;
	EditorState m_editorState = EditorState::None;
	EditorActionAxis m_actionAxis;
	vec2 m_actionDeltaVec2 = vec2(0.f);

	std::deque<std::string> m_consoleBuffer;
	size_t m_consoleBufferSize = 10;

	struct RoutineCounter {
		int interval = 0;
		int current_interval = 0;
		std::function<void()> func;
	};
	std::vector<RoutineCounter> m_routines;
	void process_routines();

	// tool routines have their own data so they can be removed on non editor builds
	std::vector<RoutineCounter> m_tool_routines;
	void process_tool_routines();

	void editor_mouse_move_event(InputEventMouseMove event);
	void editor_key_event(InputEventKey event);
	void editor_scroll_event(InputEventScroll event);
	//

	//
	void LoadProjectFromPath(const std::filesystem::path &path);
	void LoadProjectFromDialog();

	Font m_testFont;

	Model fullscreenModel;
	Shader fullscreenShader;
	RenderLayer m_layer2D;
	RenderLayer m_layerUI;

	project_settings m_projectSettings;
	std::vector<NodeTypeName> m_node_type_names;

	CommandInterface *m_commandInterface = nullptr;
	std::vector<CommandOption> m_commands;

	void command_interface_char_callback(InputEventChar event);
	void command_interface_key_callback(InputEventKey event);
	void load_resources();

	BatchRenderer m_batchRenderer;
	NewTree m_uiTree;

	eventpp::CallbackList<void(InputEventMouseButton)> m_mouseInputCallback;
	eventpp::CallbackList<void(InputEventMouseMove)> m_mouseMoveCallback;
	eventpp::CallbackList<void(InputEventKey)> m_keyCallback;
	eventpp::CallbackList<void(InputEventChar)> m_charCallback;
	eventpp::CallbackList<void(InputEventScroll)> m_scrollCallback;
	eventpp::CallbackList<void(InputEventClick)> m_clickCallback;

	eventpp::CallbackList<void(int, int)> m_windowResizeCallback;

	Gui m_gui;
	CursorStyle m_cursorStyle = CursorStyle::Normal;

	bool m_running = false;
	rect m_viewportRect{};

	friend class FileServer;
	std::filesystem::path m_appPath = "";

	// Used by App::load_scene();
	std::string m_scene_to_load = "";
};

#endif // SW_APP_HXX