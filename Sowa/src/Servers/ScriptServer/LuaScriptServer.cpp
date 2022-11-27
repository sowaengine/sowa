#include "Servers/ScriptServer/LuaScriptServer.hpp"
#include "Debug.hpp"

#include "Core/ProjectSettings.hpp"
#include "Utils/Math.hpp"
#include "Utils/String.hpp"

#include "Lua/Exceptions.hpp"
#include "Lua/LuaModule.hpp"

#include "lua.hpp"
#include "sol/sol.hpp"

#include "Servers/GuiServer/GuiServer.hpp"
#include "Utils/Dialog.hpp"
#include <fstream>

namespace Sowa {
LuaScriptServer::LuaScriptServer(EngineContext &ctx) : _Context(ctx) {
	_pState = std::make_unique<sol::state>();
	_pState->open_libraries(sol::lib::base);

	RegisterECS();
	RegisterGuiServer();
	RegisterMath();
	RegisterResources();
	RegisterSingleton();

	// ProjectManager ( will be removed )
	static std::vector<std::string> projects;

	(*_pState)["reload_projects"] = []() {
		projects.clear();
		std::ifstream ifstream;
		ifstream.open(File::Path("editor://projects.txt").string(), std::ios_base::binary);
		if (ifstream.good()) {
			std::string line;
			while (std::getline(ifstream, line)) {
				projects.push_back(line);
				Debug::Log("Import Project {}", line);
			}
		}
	};

	(*_pState)["import_project"] = []() {
		std::string pathStr = Dialog::OpenFileDialog("Import Project", "", 1, {"project.ease"}, false, false, nullptr);
		if (pathStr != "") {
			std::filesystem::path path = pathStr;

			if (std::filesystem::is_regular_file(path)) {
				if (path.filename() == "project.ease") {
					std::ofstream ofstream;
					ofstream.open(File::Path("editor://projects.txt").string(), std::ios_base::app);
					if (ofstream.good()) {
						ofstream << path.string() << '\n';
					}
				}
			}
		}
	};

	(*_pState)["draw_projects"] = [this](GuiServer &self, double buttonWidth, double buttonHeight) {
		for (const std::string &project : projects) {
			if (self.Button(project, static_cast<int>(buttonWidth), static_cast<int>(buttonHeight))) {
				Application *app = this->_Context.GetSingleton<Application>(Server::APPLICATION);
				app->LaunchApp(project);
			}
		}
	};
}
LuaScriptServer::~LuaScriptServer() {
	_pState = nullptr;
}

void LuaScriptServer::InitModules() {
#ifdef SW_EDITOR
	LoadModule("abs://Editor/Sowa.Editor.lua");
#endif

	for (const std::string &path : _Context.GetSingleton<Sowa::ProjectSettings>(Sowa::Server::PROJECTSETTINGS)->_modules.lua) {
		LoadModule(path.c_str());
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

void LuaScriptServer::LoadModule(const char *path) {
	std::filesystem::path fullpath = Sowa::File::Path(path);

	std::vector<unsigned char> code = Sowa::File::GetFileContent(path);
	if (code.size() == 0) {
		Debug::Error("Module not found at {}", path);
		return;
	}
	std::string code_str{reinterpret_cast<char *>(code.data()), code.size()};
	auto res = _pState->safe_script(code_str, &sol::script_pass_on_error);

	try {
		LuaModule luamod;

		if (!res.valid()) {
			sol::error err = res;
			throw Lua::LuaException(err.what());
		}

		if (res.get_type() != sol::type::table) {
			throw Lua::InvalidModuleReturnException(_pState->lua_state(), res.get_type(), fullpath.string().c_str());
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

		Debug::Info("Load Lua module '{}' at '{}'", moduleName, fullpath.string().c_str());
	} catch (const std::exception &ex) {
		Debug::Error("[LuaException] {}", ex.what());
		return;
	}
}

} // namespace Sowa
