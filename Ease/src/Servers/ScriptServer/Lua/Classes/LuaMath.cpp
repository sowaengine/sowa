#include "Servers/ScriptServer/LuaScriptServer.hpp"

namespace Ease {
void LuaScriptServer::RegisterMath() {
	_pState->new_usertype<Vec2>(
		"Vector2",
		sol::constructors<Vec2(), Vec2(float, float)>(),

		"x", &Vec2::x,
		"y", &Vec2::y,

		"length", &Vec2::Length,
		"clamp", &Vec2::Clamp,
		"angle", &Vec2::Angle);
}
} // namespace Ease
