#include "NativeScriptResource.hpp"

#include "Utils/File.hpp"
#include <fstream>
#include <iostream>

#include <dlfcn.h>

namespace Ease
{

bool NativeScriptResource::loadScript(const std::string& path)
{
   m_ScriptPath = path;
   File::FileProperties resolvedPath = File::resolveFilepath(path);

   m_Obj = dlopen(m_ScriptPath.c_str(), RTLD_LAZY);
   if(!m_Obj) {
      throw std::runtime_error("Failed to load obj");
   }

   dlerror();
   return true;
}

   
} // namespace Ease
