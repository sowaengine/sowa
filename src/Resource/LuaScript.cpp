#include "LuaScript.hpp"  
	
LuaScript::LuaScript()
   : m_Filepath(prop_filepath.dataStr)
{
   prop_filepath.name = "Script";
   prop_filepath.type = PropertyType::SCRIPT;
   
   properties.push_back(&prop_filepath);
}

LuaScript::LuaScript(const std::string& filepath) 
   : m_Filepath(prop_filepath.dataStr)
{
   prop_filepath.dataStr = filepath;

   prop_filepath.name = "Script";
   prop_filepath.type = PropertyType::SCRIPT;

   properties.push_back(&prop_filepath);
}
	
LuaScript::~LuaScript()
{
}

void LuaScript::SetFilepath(const std::string& path) 
{
   m_Filepath = path;
}

const std::string& LuaScript::GetFilepath() 
{
   return m_Filepath;
}


void LuaScript::PropertiesUpdated()
{
   SetFilepath(prop_filepath.dataStr);
}
