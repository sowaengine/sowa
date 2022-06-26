#include "NativeModule.hpp"

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
   void NativeModule::CallOnImGuiRender()
   {
      m_pModule->OnImGuiRender();
   }
} // namespace Ease