/**
 * @file Lua::Debug.hpp
 * @author Lexographics
 * @brief Debugger functions for Lua
 * @version 0.1
 * @date 2021-11-15
 */

#pragma once
#include "lua/lua.hpp"


#define LUA_DUMP(L) __dumpstack__(L);
   

namespace Ease::Lua
{

/**
 * @note use LUA_DUMP (empty on debug builds)
 * @brief Dumps the current lua stack
 * 
 * @param L lua stack to dump
 */
void __dumpstack__(lua_State* L);
   
} // namespace Ease::Lua
