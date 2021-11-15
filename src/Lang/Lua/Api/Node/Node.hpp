/**
 * @file Lua::Node.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-11-15
 */
#pragma once

#include "glm/glm.hpp"
#include "lua/lua.hpp"

#include "Scene/SceneTree/Node.hpp"
#include "Scene/Components.hpp"

#include "Components/Components.hpp"
#include "Lang/Lua/Api/BaseUserdata/BaseUserdata.hpp"

namespace Ease::Lua
{

/**
 * @brief structure to get by calling get_node, get_root etc. in lua
 */
struct Node : public BaseUserdata
{

   /** @brief initializes BaseUserData::data_type */
   Node();

   
   /**
    * @brief Get the Node object with given name
    * 
    * @param 1 path to node (not finished yet, can only return child)
    * @returns lua userdata with type: Lua::Node with metatable: "NodeMetaTable"
    */
   static int getNode(lua_State* L);


   /**
    * @brief __index metamethod for indexing Components of the Node
    * 
    * 
    * @if get_node("name").Transform()
    *    @returns method to create indexed Component name
    *    @see Lua::Transform3D::create
    * @elseif get_node("name").name
    *    @returns string
    * @endif
    */
   static int __index(lua_State* L);


   /**
    * @brief __newindex metamethod for setting properties of Node userdata
    */
   static int __newindex(lua_State* L);

   // Private members that are only accessible natively
   Ease::Node* nodeRef = nullptr;



   /** 
    * one byte of padding to make sizeof unique
    * 
    * right now userdata's are identified by their name
    * please someone help
    * what can i do to identify void pointers?
    * 
    * i tried to make a base struct that holds uint32_t type
    * and i couldn't have it work. (i think this can work, i'll look at it later) @see BaseUserdata
    * i didn't go deep into this for now. because it's for debugging
    * but i must return to it before release
   */
   uint32_t __pad;
};


} // namespace Ease::Lua