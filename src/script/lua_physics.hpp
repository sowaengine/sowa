#pragma once

#include "script/lua_types.hpp"
#include <sol/sol.hpp>

namespace sowa {
class PhysicsWorld;
void BindPhysics(sol::state &lua, PhysicsWorld *physics);
sol::object RaycastMethod(ComponentView &view, const std::string &name, sol::this_state state);
void ValidatePhysicsField(const ComponentView &view, const std::string &name, sol::object value);
} // namespace sowa
