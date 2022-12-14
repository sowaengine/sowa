#include "Servers/ScriptServer/LuaScriptServer.hpp"

#include "Core/Application.hpp"
#include "Core/Input.hpp"

#include "Utils/Math.hpp"
#include "Utils/String.hpp"

#include "fmt/args.h"
#include "fmt/core.h"

namespace Sowa {
void LuaScriptServer::RegisterSingleton() {
	_pState->new_usertype<Application>(
		"Application",

		"new", sol::no_constructor,
		"get", [this]() -> Sowa::Application & { return *this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION); },

		"get_current_scene", &Sowa::Application::GetCurrentScene,

		"start_game", &Application::StartGame,
		"stop_game", &Application::StopGame,

		"is_running", &Application::IsRunning,

		"set_editor_camera_position", &Application::SetEditorCameraPosition,
		"set_editor_camera_zoom", &Application::SetEditorCameraZoom,
		"get_editor_camera_position", &Application::GetEditorCameraPosition,
		"get_editor_camera_zoom", &Application::GetEditorCameraZoom,

		"selected_entity",
		sol::property(
			[](Sowa::Application &self) -> Sowa::Entity & { return self.SelectedEntity(); },
			[](Sowa::Application &self, Sowa::Entity &entity) -> void { self.SelectedEntity() = entity; })

	);

	_pState->new_usertype<Window>(
		"Window",

		"new", sol::no_constructor,
		"get", [this]() -> Sowa::Window & { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow(); },

		"get_window_size", [](Window &self) -> Vec2 { return Vec2((float)self.GetWindowWidth(), (float)self.GetWindowHeight()); });

	(*_pState)["Input"] = _pState->create_table_with(
		"is_key_just_pressed", sol::as_function([this](int key) -> bool { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow().IsKeyJustPressed(key); }),
		"is_key_just_released", sol::as_function([this](int key) -> bool { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow().IsKeyJustReleased(key); }),
		"is_key_down", sol::as_function([this](int key) -> bool { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow().IsKeyDown(key); }),
		"is_key_up", sol::as_function([this](int key) -> bool { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow().IsKeyUp(key); }),
		"get_scroll_delta_y", sol::as_function([this]() -> double { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow().GetScrollDeltaY(); }),
		"get_scroll_delta_x", sol::as_function([this]() -> double { return this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetWindow().GetScrollDeltaX(); })

	);

	(*_pState)["Debug"] = _pState->create_table_with("log",
													 [](sol::variadic_args args) {
														 if (args.size() == 0)
															 return;

														 std::string format{""};
														 format = args[0].as<std::string>();

														 fmt::dynamic_format_arg_store<fmt::format_context> store;

														 sol::variadic_args::iterator it = args.begin();
														 ++it; // skip format arg
														 for (; it != args.end(); ++it) {
															 sol::type type = (*it).get_type();

															 if (type == sol::type::boolean) {
																 bool value = (*it).as<bool>();
																 if (value)
																	 store.push_back("true");
																 else
																	 store.push_back("false");
															 } else if (type == sol::type::number) {
																 double value = (*it).as<double>();
																 store.push_back(value);
															 } else if (type == sol::type::string) {
																 std::string value = (*it).as<std::string>();
																 store.push_back(Format("\"{}\"", value));
															 } else if (type == sol::type::nil) {
																 store.push_back("nil");
															 } else if (type == sol::type::userdata) {
																 sol::userdata val = (*it).as<sol::userdata>();
																 store.push_back("userdata");
															 } else {
																 store.push_back(Format("<unknown_t:{}>", (int)type));
															 }
														 }

														 std::string msg = "FORMAT ERROR!";
														 try {
															 msg = fmt::vformat(format, store);
														 } catch (const std::exception &e) {
															 Debug::Error("String Format error: {}", e.what());
														 }

														 Debug::Log(msg);
													 });

	_pState->new_enum(
		"Key",

		"KEY_UNKNOWN", Key::KEY_UNKNOWN,
		"KEY_SPACE", Key::KEY_SPACE,
		"KEY_APOSTROPHE", Key::KEY_APOSTROPHE,
		"KEY_COMMA", Key::KEY_COMMA,
		"KEY_MINUS", Key::KEY_MINUS,
		"KEY_PERIOD", Key::KEY_PERIOD,
		"KEY_SLASH", Key::KEY_SLASH,
		"KEY_0", Key::KEY_0,
		"KEY_1", Key::KEY_1,
		"KEY_2", Key::KEY_2,
		"KEY_3", Key::KEY_3,
		"KEY_4", Key::KEY_4,
		"KEY_5", Key::KEY_5,
		"KEY_6", Key::KEY_6,
		"KEY_7", Key::KEY_7,
		"KEY_8", Key::KEY_8,
		"KEY_9", Key::KEY_9,
		"KEY_SEMICOLON", Key::KEY_SEMICOLON,
		"KEY_EQUAL", Key::KEY_EQUAL,
		"KEY_A", Key::KEY_A,
		"KEY_B", Key::KEY_B,
		"KEY_C", Key::KEY_C,
		"KEY_D", Key::KEY_D,
		"KEY_E", Key::KEY_E,
		"KEY_F", Key::KEY_F,
		"KEY_G", Key::KEY_G,
		"KEY_H", Key::KEY_H,
		"KEY_I", Key::KEY_I,
		"KEY_J", Key::KEY_J,
		"KEY_K", Key::KEY_K,
		"KEY_L", Key::KEY_L,
		"KEY_M", Key::KEY_M,
		"KEY_N", Key::KEY_N,
		"KEY_O", Key::KEY_O,
		"KEY_P", Key::KEY_P,
		"KEY_Q", Key::KEY_Q,
		"KEY_R", Key::KEY_R,
		"KEY_S", Key::KEY_S,
		"KEY_T", Key::KEY_T,
		"KEY_U", Key::KEY_U,
		"KEY_V", Key::KEY_V,
		"KEY_W", Key::KEY_W,
		"KEY_X", Key::KEY_X,
		"KEY_Y", Key::KEY_Y,
		"KEY_Z", Key::KEY_Z,
		"KEY_LEFT_BRACKET", Key::KEY_LEFT_BRACKET,
		"KEY_BACKSLASH", Key::KEY_BACKSLASH,
		"KEY_RIGHT_BRACKET", Key::KEY_RIGHT_BRACKET,
		"KEY_GRAVE_ACCENT", Key::KEY_GRAVE_ACCENT,
		"KEY_WORLD_1", Key::KEY_WORLD_1,
		"KEY_WORLD_2", Key::KEY_WORLD_2,
		"KEY_ESCAPE", Key::KEY_ESCAPE,
		"KEY_ENTER", Key::KEY_ENTER,
		"KEY_TAB", Key::KEY_TAB,
		"KEY_BACKSPACE", Key::KEY_BACKSPACE,
		"KEY_INSERT", Key::KEY_INSERT,
		"KEY_DELETE", Key::KEY_DELETE,
		"KEY_RIGHT", Key::KEY_RIGHT,
		"KEY_LEFT", Key::KEY_LEFT,
		"KEY_DOWN", Key::KEY_DOWN,
		"KEY_UP", Key::KEY_UP,
		"KEY_PAGE_UP", Key::KEY_PAGE_UP,
		"KEY_PAGE_DOWN", Key::KEY_PAGE_DOWN,
		"KEY_HOME", Key::KEY_HOME,
		"KEY_END", Key::KEY_END,
		"KEY_CAPS_LOCK", Key::KEY_CAPS_LOCK,
		"KEY_SCROLL_LOCK", Key::KEY_SCROLL_LOCK,
		"KEY_NUM_LOCK", Key::KEY_NUM_LOCK,
		"KEY_PRINT_SCREEN", Key::KEY_PRINT_SCREEN,
		"KEY_PAUSE", Key::KEY_PAUSE,
		"KEY_F1", Key::KEY_F1,
		"KEY_F2", Key::KEY_F2,
		"KEY_F3", Key::KEY_F3,
		"KEY_F4", Key::KEY_F4,
		"KEY_F5", Key::KEY_F5,
		"KEY_F6", Key::KEY_F6,
		"KEY_F7", Key::KEY_F7,
		"KEY_F8", Key::KEY_F8,
		"KEY_F9", Key::KEY_F9,
		"KEY_F10", Key::KEY_F10,
		"KEY_F11", Key::KEY_F11,
		"KEY_F12", Key::KEY_F12,
		"KEY_F13", Key::KEY_F13,
		"KEY_F14", Key::KEY_F14,
		"KEY_F15", Key::KEY_F15,
		"KEY_F16", Key::KEY_F16,
		"KEY_F17", Key::KEY_F17,
		"KEY_F18", Key::KEY_F18,
		"KEY_F19", Key::KEY_F19,
		"KEY_F20", Key::KEY_F20,
		"KEY_F21", Key::KEY_F21,
		"KEY_F22", Key::KEY_F22,
		"KEY_F23", Key::KEY_F23,
		"KEY_F24", Key::KEY_F24,
		"KEY_F25", Key::KEY_F25,
		"KEY_KP_0", Key::KEY_KP_0,
		"KEY_KP_1", Key::KEY_KP_1,
		"KEY_KP_2", Key::KEY_KP_2,
		"KEY_KP_3", Key::KEY_KP_3,
		"KEY_KP_4", Key::KEY_KP_4,
		"KEY_KP_5", Key::KEY_KP_5,
		"KEY_KP_6", Key::KEY_KP_6,
		"KEY_KP_7", Key::KEY_KP_7,
		"KEY_KP_8", Key::KEY_KP_8,
		"KEY_KP_9", Key::KEY_KP_9,
		"KEY_KP_DECIMAL", Key::KEY_KP_DECIMAL,
		"KEY_KP_DIVIDE", Key::KEY_KP_DIVIDE,
		"KEY_KP_MULTIPLY", Key::KEY_KP_MULTIPLY,
		"KEY_KP_SUBTRACT", Key::KEY_KP_SUBTRACT,
		"KEY_KP_ADD", Key::KEY_KP_ADD,
		"KEY_KP_ENTER", Key::KEY_KP_ENTER,
		"KEY_KP_EQUAL", Key::KEY_KP_EQUAL,
		"KEY_LEFT_SHIFT", Key::KEY_LEFT_SHIFT,
		"KEY_LEFT_CONTROL", Key::KEY_LEFT_CONTROL,
		"KEY_LEFT_ALT", Key::KEY_LEFT_ALT,
		"KEY_LEFT_SUPER", Key::KEY_LEFT_SUPER,
		"KEY_RIGHT_SHIFT", Key::KEY_RIGHT_SHIFT,
		"KEY_RIGHT_CONTROL", Key::KEY_RIGHT_CONTROL,
		"KEY_RIGHT_ALT", Key::KEY_RIGHT_ALT,
		"KEY_RIGHT_SUPER", Key::KEY_RIGHT_SUPER,
		"KEY_MENU", Key::KEY_MENU);
}
} // namespace Sowa
