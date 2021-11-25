/**
 * @file BaseUserdata.hpp
 * @author Lexographics
 * @brief struct that every userdata in lua inherits from
 * @version 0.1
 * @date 2021-11-15
 */
#pragma once

#include <string>

#define LUA_TYPE_NONE 0x00
#define LUA_TYPENAME_NONE std::string("None")

#define LUA_TYPE_VECTOR3 0x01
#define LUA_TYPENAME_VECTOR3 std::string("Vector3")

#define LUA_TYPE_NODE 0x02
#define LUA_TYPENAME_NODE std::string("Node")



#define LUA_TYPE_COMPONENT_BASE 0xF00
#define LUA_TYPENAME_COMPONENT_BASE std::string("BaseComponent")

#define LUA_TYPE_COMPONENT_TRANSFORM 0xF01
#define LUA_TYPENAME_COMPONENT_TRANSFORM std::string("TransformComponent")



namespace Ease::Lua
{

struct BaseUserdata
{  
   uint32_t data_type = LUA_TYPE_NONE;

   std::string getTypeName();
};
   
} // namespace Ease::Lua
