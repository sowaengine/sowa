#include "LuaBehaviour.hpp"
#include "Debug.hpp"

#include "ECS/Entity/Entity.hpp"
#include "Exceptions.hpp"

namespace Sowa {

LuaBehaviour::LuaBehaviour(const std::string &label, const std::string &id) {
	_Label = label;
	_Id = id;
}
LuaBehaviour::~LuaBehaviour() {}

void LuaBehaviour::CallStart(Sowa::Entity &entity) {
	if (_Start.valid()) {
		sol::protected_function_result res = _Start(entity);
		if (!res.valid()) {
			sol::error err = res;
			Debug::Error("[LuaException] module.start {}", Lua::LuaException(err.what()).what());
		}
	}
}
void LuaBehaviour::CallUpdate(Sowa::Entity &entity) {
	if (_Update.valid()) {
		sol::protected_function_result res = _Update(entity);
		if (!res.valid()) {
			sol::error err = res;
			Debug::Error("[LuaException] module.update {}", Lua::LuaException(err.what()).what());
		}
	}
}
} // namespace Sowa
