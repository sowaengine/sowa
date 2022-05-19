#include "ResourceManager.h"

#include "Resource/Texture/Texture.h"
#include "raylib.h"
#include "dylib.hpp"

namespace Ease
{
   // Resource specific implementations


   /* ==============<Ease::Texture>============== */
   template<>
   std::shared_ptr<Ease::Texture> ResourceManager<Ease::Texture>::LoadResource(const char* path)
   {
      static ResourceID resCounter = 0;

      std::shared_ptr<Ease::Texture> tex = std::make_shared<Ease::Texture>();

      m_Resources[resCounter] = tex;
      tex->SetTexture(LoadTexture(path));
      tex->SetResourceID(resCounter);


      resCounter++;
      return tex;
   }

   template<>
   ResourceManager<Ease::Texture>& ResourceManager<Ease::Texture>::GetLoader()
   {
      static ResourceManager<Ease::Texture> loader;

      return loader;
   }
   /* ==============</Ease::Texture>============== */

   
   /* ==============<Ease::NativeModule>============== */
   template<>
   std::shared_ptr<Ease::NativeModule> ResourceManager<Ease::NativeModule>::LoadResource(const char* path)
   {
      static ResourceID resCounter = 0;

      std::string modulePath = path;
      modulePath += dylib::extension;

      dylib lib;
      lib.open(modulePath.c_str());
      
      auto createFunc = lib.get_function<BaseModule*()>("Create");
      auto deleteFunc = lib.get_function<void(BaseModule*)>("Destroy");

      BaseModule* myModule = createFunc();

      std::shared_ptr<Ease::NativeModule> nativeModule = std::make_shared<Ease::NativeModule>();
      nativeModule->SetModule(myModule);
      nativeModule->SetDeleteFunc(deleteFunc);

      m_Resources[resCounter] = nativeModule;
      nativeModule->SetResourceID(resCounter);

      resCounter++;
      return nativeModule;
   }

   template<>
   ResourceManager<Ease::NativeModule>& ResourceManager<Ease::NativeModule>::GetLoader()
   {
      static ResourceManager<Ease::NativeModule> moduleLoader;

      return moduleLoader;
   }
   /* ==============</Ease::NativeModule>============== */

   
} // namespace Ease