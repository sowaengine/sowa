#include "BaseUserdata.hpp"

#include <map>



namespace Ease::Lua
{

static const std::map<uint32_t, std::string> types = 
{
   { LUA_TYPE_VECTOR3,             LUA_TYPENAME_VECTOR3 },
   { LUA_TYPE_NODE,                LUA_TYPENAME_NODE },

   { LUA_TYPE_COMPONENT_BASE,      LUA_TYPENAME_COMPONENT_BASE },
   { LUA_TYPE_COMPONENT_TRANSFORM, LUA_TYPENAME_COMPONENT_TRANSFORM },
};

const std::string& BaseUserdata::getTypeName()
{
   for (const auto& type : types)
   {
      if(data_type == type.first)
         return std::string(type.second);
   }
   return std::string("None");
}


} // namespace Ease::Lua
