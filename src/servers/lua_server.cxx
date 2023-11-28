#include "lua_server.hxx"

#include "sol/sol.hpp"

#include "utils/utils.hxx"

LuaServer &LuaServer::get() {
	static LuaServer *server = new LuaServer;
	return *server;
}

void LuaServer::Init() {
	sol::state lua;
	lua.set_function("print", [](std::string e) { Utils::Log("{}", e); });

	auto res = lua.script("return {data= 5}");

	auto r = res.get<sol::table>();
	Utils::Log("Script returned data: {}", r.get<int>("data"));
}

LuaServer::LuaServer() {
}
LuaServer::~LuaServer() {
}