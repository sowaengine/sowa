#include "luaScript.hpp"

#include "Utils/File.hpp"
#include <fstream>
#include <iostream>

#include <filesystem>

namespace Ease
{

const std::string& LuaScript::getScript()
{
   return m_Script;
}

bool LuaScript::loadScript(const std::string& path)
{
   m_ScriptPath = path;
   m_Script = "";
   File::FileProperties resolvedPath = File::resolveFilepath(path);

   std::ifstream ifstream(resolvedPath.fullPath);

   std::string line = "";
   while (getline(ifstream, line))
   {
      m_Script += line + "\n";
   }


   m_IsValid = true;
   return true;
}

   
} // namespace Ease
