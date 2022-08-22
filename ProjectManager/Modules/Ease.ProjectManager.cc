#include <iostream>
#include "Ease.hpp"
#include "Core/Application.hpp"

class EaseProjectManager;
EaseProjectManager* g_Module = nullptr;


class EaseProjectManager : public Ease::BaseModule
{
   public:

      void Start() override
      {
         std::cout << "Start" << std::endl;
      }
      void Update() override
      {
         std::cout << "Update" << std::endl;
      }
};

class MyComponent : public Ease::NativeBehaviour
{

};


MODULE_BEGIN(EaseProjectManager, "Ease", "ProjectManager", 1)
   BIND_NATIVEBEHAVIOUR(MyComponent);
   g_Module = lib;
MODULE_END(EaseProjectManager)

/*
EASE_API Ease::BaseModule* Create()
{
   EaseProjectManager* lib = new EaseProjectManager;
   lib->metadata.authorName = "Ease";
   lib->metadata.moduleName = "ProjectManager";
   lib->metadata.version = 1;

   // EASEMODULE_BIND_NATIVEBEHAVIOUR(lib, MyComponent);

   g_Module = lib;
   return lib;
}

EASE_API void Destroy(Ease::BaseModule* lib)
{
   delete reinterpret_cast<EaseProjectManager*>(lib);
}*/