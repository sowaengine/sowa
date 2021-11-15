#pragma once

#include <string>

namespace Ease
{

class LuaScript
{
public:
   const std::string& getScript();

   // Returns like res://path/to/script.lua
   const std::string& getFilename() { return m_ScriptPath; }

   bool loadScript(const std::string& path);

   bool isValid() { return m_IsValid; }
private:
   std::string m_ScriptPath;
   std::string m_Script;

   bool m_IsValid = false;
};
   
} // namespace Ease
