#include "Resource/NativeModule/NativeModule.hpp"
#include "Resource/ResourceLoader.hpp"
#include "dylib.hpp"

namespace Ease
{
   template<>
   Reference<NativeModule> ResourceLoaderImpl<NativeModule>::LoadFromFile(const char* path)
   {
      dylib lib;
      lib.open(path);
      
      auto createFunc = lib.get_function<BaseModule*()>("Create");
      auto deleteFunc = lib.get_function<void(BaseModule*)>("Destroy");

      BaseModule* myModule = createFunc();

      Reference<Ease::NativeModule> nativeModule = std::make_shared<Ease::NativeModule>();
      nativeModule->SetModule(myModule);
      nativeModule->SetDeleteFunc(deleteFunc);

      return nativeModule;
   }

   // static
   std::string NativeModule::AddExtension(const std::string& base)
   {
      return base + dylib::extension;
   }

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