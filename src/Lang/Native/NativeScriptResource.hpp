#pragma once

#include <string>

namespace Ease
{

class NativeScriptResource
{
public:
   bool loadScript(const std::string& path);

   void* getObj() { return m_Obj; }

   bool isValid() { return m_Obj != nullptr; }
private:
   std::string m_ScriptPath;

   void* m_Obj = nullptr;


   
};
   
} // namespace Ease
