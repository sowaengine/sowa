#include "Node.hpp"

#include "Scene/SceneTree/SceneTree.hpp"
#include "Global.hpp"

#include "Lang/Lua/Api/Debug/Debug.hpp"

namespace Ease::Lua
{

Node::Node()
{
   data_type = LUA_TYPE_NODE;
}

//static
int Lua::Node::getNode(lua_State* L)
{
   const char* name = luaL_checkstring(L, -1);
   Ease::Node* node = Global::sceneTree.getRoot()->getChild(name);
   assert(node != nullptr && "Cant find node | Write better debugger");


   Lua::Node* lua_node = (Lua::Node*)lua_newuserdata(L, sizeof(Lua::Node));
   lua_node->nodeRef = node;
   
   luaL_getmetatable(L, "NodeMetaTable");
   lua_setmetatable(L, -2);

   return 1;
}



//static
int Lua::Node::__index(lua_State* L)
{
   assert(lua_isuserdata(L, 1));
   assert(lua_isstring(L, 2));


   Lua::Node* lua_node = (Lua::Node*)lua_touserdata(L, 1);
   std::string index = lua_tostring(L, 2);

   if(index == "name")
   {
      lua_pushstring(L, lua_node->nodeRef->getName().c_str());
      return 1;
   }
   else if(index == "Transform")
   {
      //lua_pushnumber(L, 1);
      lua_pushcfunction(L, Lua::Transform3D::lua_get);
      return 1;
   }

   lua_pushstring(L, "err");
   lua_error(L);
   
   return 0;
}

//static
int Lua::Node::__newindex(lua_State* L)
{
   assert(lua_isuserdata(L, 1));
   assert(lua_isstring(L, 2));

   Lua::Node* lua_node = (Lua::Node*)lua_touserdata(L, 1);
   std::string index = lua_tostring(L, 2);

   if(index == "name")
   {
      if(lua_isstring(L, 3))
      {
         lua_node->nodeRef->setName( std::string(lua_tostring(L, 3)) );
         return 0;
      }
      else
      {
         assert(false && "Make an error-debug utility");
      }
   }

   return 0;
}
   
} // namespace Ease::Lua
