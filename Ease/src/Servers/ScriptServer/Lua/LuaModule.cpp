#include "LuaModule.hpp"
#include "Debug.hpp"

#include "Exceptions.hpp"

namespace Ease {

LuaModule::LuaModule() {}
LuaModule::~LuaModule() {}

void LuaModule::CallStart() {
	if (_Start.valid()) {
		sol::protected_function_result res = _Start();
		if (!res.valid()) {
			sol::error err = res;
			Debug::Error("[LuaException] module.start {}", Lua::LuaException(err.what()).what());
		}
	}
}
void LuaModule::CallUpdate() {
	if (_Update.valid()) {
		sol::protected_function_result res = _Update();
		if (!res.valid()) {
			sol::error err = res;
			Debug::Error("[LuaException] module.update {}", Lua::LuaException(err.what()).what());
		}
	}
}
void LuaModule::CallGuiUpdate() {
	if (_GuiUpdate.valid()) {
		sol::protected_function_result res = _GuiUpdate();
		if (!res.valid()) {
			sol::error err = res;
			Debug::Error("[LuaException] module.gui_update {}", Lua::LuaException(err.what()).what());
		}
	}
}

void LuaModule::CallOnPrint(const std::string &message) {
	if (_OnPrint.valid()) {
		sol::protected_function_result res = _OnPrint(message);
		if (!res.valid()) {
			sol::error err = res;
			Debug::Error("[LuaException] module.on_print {}", Lua::LuaException(err.what()).what());
		}
	}
}
} // namespace Ease
