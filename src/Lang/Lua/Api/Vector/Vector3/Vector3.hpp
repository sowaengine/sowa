/**
 * @file Lua::Vector3.hpp
 * @author Lexographics
 * @brief glm::vec3 wrapper for lua api
 * @version 0.1
 * @date 2021-11-15
 */

#pragma once

#include "lua/lua.hpp"
#include "glm/glm.hpp"

#include "Lang/Lua/Api/BaseUserdata/BaseUserdata.hpp"

namespace Ease::Lua
{
   

struct Vector3 : public BaseUserdata
{
   /** @brief initializes BaseUserData::data_type */
   Vector3();

   glm::vec3 vec{0.f};

   /** @brief will create userdata vector3 for given L */
   static int create(lua_State* L);

   /**
    * @brief helper method that calls pushing number to stack and calling Vector3::create(L)
    */
   static int createWithFloats(lua_State* L, float x, float y, float z);
   

   /** @brief invoked metamethod when getting or setting members of vec3::vec (x, y, z) */
   static int __index(lua_State* L);


   /** @brief metamethod for adding two vec3's */
   static int __add(lua_State* L);
};


} // namespace Ease::Lua