#include "NativeModule.h"

namespace Ease
{
   NativeModule::NativeModule()
      : m_pModule(nullptr)
   {

   }

   NativeModule::~NativeModule()
   {
      m_DeleteFunc(m_pModule);
      m_pModule = nullptr;
   }
   
   void NativeModule::CallStart()
   {
      m_pModule->Start();
   }
   
   void NativeModule::CallUpdate()
   {
      m_pModule->Update();
   }
} // namespace Ease