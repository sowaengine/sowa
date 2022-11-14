#include "Servers/ScriptServer/LuaScriptServer.hpp"
#include "Debug.hpp"

#include "Core/ProjectSettings.hpp"
#include "Utils/Math.hpp"
#include "Utils/String.hpp"

#include "Lua/Exceptions.hpp"
#include "Lua/LuaModule.hpp"

#include "lua.hpp"
#include "sol/sol.hpp"

namespace Ease {
LuaScriptServer::LuaScriptServer(EngineContext &ctx) : _Context(ctx) {
	_pState = std::make_unique<sol::state>();
	_pState->open_libraries(sol::lib::base);

	RegisterECS();
	RegisterGuiServer();
	RegisterMath();
	RegisterResources();
	RegisterSingleton();
}
LuaScriptServer::~LuaScriptServer() {
	_pState = nullptr;
}

void LuaScriptServer::InitModules() {
	for (const std::string &path : _Context.GetSingleton<Ease::ProjectSettings>(Ease::Server::PROJECTSETTINGS)->_modules.lua) {
		std::filesystem::path fullpath = Ease::File::Path(path);

		std::vector<unsigned char> code = Ease::File::GetFileContent(path.c_str());
		std::string code_str{reinterpret_cast<char *>(code.data()), code.size()};
		auto res = _pState->safe_script(code_str, &sol::script_pass_on_error);

		try {
			LuaModule luamod;

			if (!res.valid()) {
				sol::error err = res;
				throw Lua::LuaException(err.what());
			}

			if (res.get_type() != sol::type::table) {
				throw Lua::InvalidModuleReturnException(_pState->lua_state(), res.get_type(), fullpath.c_str());
			}

			sol::table mod = res.get<sol::table>();
			auto author = mod["author"];
			if (author.valid()) {
				if (author.get_type() != sol::type::string) {
					throw Lua::TypeMismatchException("module.author", _pState->lua_state(), sol::type::string, author.get_type());
				}

				luamod._Author = author.get<sol::string_view>();
			}

			auto name = mod["name"];
			if (name.valid()) {
				if (name.get_type() != sol::type::string) {
					throw Lua::TypeMismatchException("module.name", _pState->lua_state(), sol::type::string, name.get_type());
				}

				luamod._Name = name.get<sol::string_view>();
			}

			auto version = mod["version"];
			if (version.valid()) {
				if (version.get_type() != sol::type::number) {
					throw Lua::TypeMismatchException("module.version", _pState->lua_state(), sol::type::number, version.get_type());
				}

				luamod._Version = version.get<int>();
			}

			auto start = mod["start"];
			if (start.valid()) {
				if (start.get_type() != sol::type::function) {
					throw Lua::TypeMismatchException("module.start", _pState->lua_state(), sol::type::function, start.get_type());
				}

				luamod._Start = start.get<sol::function>();
			}

			auto update = mod["update"];
			if (update.valid()) {
				if (update.get_type() != sol::type::function) {
					throw Lua::TypeMismatchException("module.update", _pState->lua_state(), sol::type::function, update.get_type());
				}

				luamod._Update = update.get<sol::function>();
			}

			auto gui_update = mod["gui_update"];
			if (gui_update.valid()) {
				if (gui_update.get_type() != sol::type::function) {
					throw Lua::TypeMismatchException("module.gui_update", _pState->lua_state(), sol::type::function, gui_update.get_type());
				}

				luamod._GuiUpdate = gui_update.get<sol::function>();
			}

			auto on_print = mod["on_print"];
			if (on_print.valid()) {
				if (on_print.get_type() != sol::type::function) {
					throw Lua::TypeMismatchException("module.on_print", _pState->lua_state(), sol::type::function, on_print.get_type());
				}

				luamod._OnPrint = on_print.get<sol::function>();
				Debug::OnPrint() += [luamod](std::string message) mutable { luamod.CallOnPrint(message); };
			}

			std::string moduleName = Format("{}.{}", luamod._Author, luamod._Name);
			_Modules[moduleName] = luamod;

			Debug::Info("Load Lua module '{}' at '{}'", moduleName, fullpath.c_str());
		} catch (const std::exception &ex) {
			Debug::Error("[LuaException] {}", ex.what());
			continue;
		}
	}

	for (auto &[name, mod] : _Modules) {
		mod.CallStart();
	}
}
void LuaScriptServer::UpdateModules() {
	for (auto &[name, mod] : _Modules) {
		mod.CallUpdate();
	}
}
void LuaScriptServer::GuiUpdateModules() {
	for (auto &[name, mod] : _Modules) {
		mod.CallGuiUpdate();
	}
}
} // namespace Ease
