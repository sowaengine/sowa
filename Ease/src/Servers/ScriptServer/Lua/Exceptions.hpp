#ifndef _E_LUA_EXCEPTIONS_HPP__
#define _E_LUA_EXCEPTIONS_HPP__

#include "fmt/core.h"
#include "sol/sol.hpp"
#include <exception>
#include <string>

namespace Ease::Lua {
class LuaException : public std::runtime_error {
  public:
	LuaException(const char *msg)
		: std::runtime_error(fmt::format("LuaException {}", msg)) {}
};

class InvalidModuleReturnException : public std::runtime_error {
  public:
	InvalidModuleReturnException(lua_State *L, sol::type returnType, const char *path)
		: std::runtime_error(fmt::format("InvalidModuleReturnException at Path: {}, Given Type: '{}', Expected 'table'", path, sol::type_name(L, returnType))) {}
};

class TypeMismatchException : public std::runtime_error {
  public:
	TypeMismatchException(const char *variableName, lua_State *L, sol::type expectedType, sol::type _type)
		: std::runtime_error(fmt::format("TypeMismatchException on '{}': Given type: '{}', Expected '{}'", variableName, sol::type_name(L, _type), sol::type_name(L, expectedType))) {}
};

} // namespace Ease::Lua

#endif // _E_LUA_EXCEPTIONS_HPP__