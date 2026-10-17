#pragma once
#include "script/lua_types.hpp"
#include <sol/sol.hpp>

namespace sowa {
sol::object TilemapMethod(ComponentView &view, const std::string &name, sol::this_state state);
}
