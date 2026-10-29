#pragma once

#include <utility>

#include <sol/sol.hpp>

namespace sowa {

// Expose callbacks as Lua functions instead of callable userdata.
template <typename Function> sol::object MakeLuaFunction(lua_State *state, Function &&function) {
	return sol::make_object(state, sol::as_function(std::forward<Function>(function)));
}

} // namespace sowa
