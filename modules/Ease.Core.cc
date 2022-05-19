#include <iostream>
#include "../include/Ease.hpp"
#include "../include/dylib.hpp"

class EaseCore : public Ease::BaseModule
{
   public:
      ~EaseCore()
      {
         std::cout << "EaseCore Destroyed" << std::endl;
      }

      void Start() override
      {
         std::cout << "EaseCore Started" << std::endl;
      }

      void Update() override
      {
         static int i = 0;
         i++;

         if(i % 60 == 0)
         {
            std::cout << "EaseCore Update" << std::endl;
         }
      }

};



DYLIB_API Ease::BaseModule* Create()
{
   EaseCore* lib = new EaseCore;
   lib->metadata.authorName = "Ease";
   lib->metadata.moduleName = "Core";
   lib->metadata.version = 1;
   return lib;
}

DYLIB_API void Destroy(Ease::BaseModule* lib)
{
   delete reinterpret_cast<EaseCore*>(lib);
}

