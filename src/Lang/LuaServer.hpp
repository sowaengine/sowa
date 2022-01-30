#ifndef LUASERVER_H
#define LUASERVER_H
#pragma once

#include "sol/sol.hpp"
class Entity;

class LuaScriptComponent;

class LuaServer  
{
	private:
		LuaServer();

		sol::state m_State;

	public:
		static LuaServer& get_singleton();
		~LuaServer();

		operator lua_State* () { return m_State.lua_state(); }

		void BindLuaScriptComponent(LuaScriptComponent& component, Entity& entity);

		sol::state& GetState() { return m_State; }
};
#endif