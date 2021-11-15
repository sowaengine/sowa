#include "Transform3D.hpp"

#include "Lang/Lua/Api/Vector/Vector3/Vector3.hpp"
#include "Lang/Lua/Api/Debug/Debug.hpp"

namespace Ease::Lua
{

Transform3D::Transform3D()
{
   data_type = LUA_TYPE_COMPONENT_TRANSFORM;
}

//static
int Lua::Transform3D::lua_get(lua_State* L)
{
   int top = lua_gettop(L);
   Lua::Node* lua_node = (Lua::Node*)lua_touserdata(L, -top);
   
   auto& tc = lua_node->nodeRef->getComponent<Transform3DComponent>();
   
   auto* lua_tc = (Lua::Transform3D*)lua_newuserdata(L, sizeof(Lua::Transform3D));

   lua_tc->translation = tc.translation;
   lua_tc->rotation = tc.rotation;
   lua_tc->scale = tc.scale;

   lua_tc->nodeRef = lua_node->nodeRef;

   luaL_getmetatable(L, "TransformComponentMetaTable");
   lua_setmetatable(L, -2);
   
   return 1;
}

//static
int Lua::Transform3D::__index(lua_State* L)
{
   // todo: set -> set both this and nodeRef's transform component
   // todo: get -> return this->translation, rotation etc.
   LUA_DUMP(L)

   assert(lua_isuserdata(L, 1));
   assert(lua_isstring(L, 2));

   Lua::Node* lua_node = (Lua::Node*)lua_touserdata(L, 1);
   std::string index = lua_tostring(L, 2);

   if(index == "translation")
   {
      auto& tc = lua_node->nodeRef->getComponent<Transform3DComponent>();
      Lua::Vector3::createWithFloats(L, tc.translation.x, tc.translation.y, tc.translation.z);
      return 0;
   }

   return 0;
}
#include <string.h>
//static
int Lua::Transform3D::__newindex(lua_State* L)
{
   assert(lua_isuserdata(L, 1));
   assert(lua_isstring(L, 2));

   Lua::Transform3D* lua_tc = (Lua::Transform3D*)lua_touserdata(L, 1);
   std::string index = lua_tostring(L, 2);

   if(index == "translation")
   {
      if(lua_rawlen(L, 3) != sizeof(Vector3))
      {
         assert(false && "given translation is not a vector3 && Make a good debugger");
      }

      Vector3* vec = (Vector3*)lua_touserdata(L, 3);
      lua_tc->translation = vec->vec;
      
      auto& tc = lua_tc->nodeRef->getComponent<Transform3DComponent>();
      tc.translation = vec->vec;
      return 0;
   }
   else if(index == "rotation")
   {
      if(lua_rawlen(L, 3) != sizeof(Vector3))
      {
         assert(false && "given rotation is not a vector3 && Make a good debugger");
      }

      Vector3* vec = (Vector3*)lua_touserdata(L, 3);
      lua_tc->rotation = vec->vec;
      
      auto& tc = lua_tc->nodeRef->getComponent<Transform3DComponent>();
      tc.rotation = vec->vec;
      return 0;
   }

   return 0;
}
   
} // namespace Ease::Lua
