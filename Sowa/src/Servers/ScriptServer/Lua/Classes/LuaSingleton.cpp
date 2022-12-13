#include "Servers/ScriptServer/LuaScriptServer.hpp"

#include "Core/Application.hpp"

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

	_pState->create_named_table("Debug",
								"log",
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
}
} // namespace Sowa
