/**
 * @file Lua::Transform3D.hpp
 * @author Lexographics
 * @brief Transform3D Component wrapper for lua, any change to its values will affect NodeRef's Transform Component from __index metamethod
 * @version 0.1
 * @date 2021-11-15
 */

#pragma once

#include "glm/glm.hpp"
#include "lua/lua.hpp"
#include "../../Node.hpp"
#include "Scene/SceneTree/Node.hpp"

#include "Lang/Lua/Api/BaseUserdata/BaseUserdata.hpp"

namespace Ease::Lua
{

struct Transform3D : public BaseUserdata
{
   /** @brief initializes BaseUserData::data_type */
   Transform3D();



   glm::vec3 translation;
   glm::vec3 rotation;
   glm::vec3 scale;

   /**
    * @brief gets transform component of Lua::Node
    * 
    * @returns Transform3D Component as userdata on Metatable: "TransformComponentMetaTable"
    */
   static int lua_get(lua_State* L);

   /**
    * @brief invoked metamethod when getting translation etc. 
    */
   static int __index(lua_State* L);

   /**
    * @brief __newindex metamethod for setting properties of Transform3D userdata
    */
   static int __newindex(lua_State* L);


   // Private
   Ease::Node* nodeRef = nullptr;
};
   
} // namespace Ease::Lua
