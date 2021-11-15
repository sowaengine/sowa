#include "Vector3.hpp"


namespace Ease::Lua
{

Vector3::Vector3()
{
   data_type = LUA_TYPE_VECTOR3;
}

//static
int Vector3::createWithFloats(lua_State* L, float x, float y, float z)
{
   lua_pushnumber(L, x);
   lua_pushnumber(L, y);
   lua_pushnumber(L, z);

   create(L);

   //lua_pop(L, 3);
   return 1;
}


// static
int Vector3::create(lua_State* L)
{
   lua_Number x, y, z;
   x = lua_tonumber(L, -3);
   y = lua_tonumber(L, -2);
   z = lua_tonumber(L, -1);

   auto* vec3 = (Lua::Vector3*)lua_newuserdata(L, sizeof(Lua::Vector3));
   vec3->vec = {x, y, z};

   luaL_getmetatable(L, "Vector3MetaTable");
   lua_setmetatable(L, -2);

   return 1;
}

// static
int Vector3::__index(lua_State* L)
{
   return 0;
}


// static
int Vector3::__add(lua_State* L)
{
   return 0;
}




   
} // namespace Ease::Lua
