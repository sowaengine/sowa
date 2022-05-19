#include <iostream>
#include "../include/Ease.hpp"
#include "../include/dylib.hpp"

class EaseEditor : public Ease::BaseModule
{
   public:
      ~EaseEditor()
      {
         std::cout << "Editor Destroyed" << std::endl;
      }

      void Start() override
      {
         std::cout << "Editor Started" << std::endl;
      }

      void Update() override
      {
         static int i = 0;
         i++;

         if(i % 60 == 0)
         {
            std::cout << "Editor Update" << std::endl;
         }
      }

};



DYLIB_API Ease::BaseModule* Create()
{
   EaseEditor* lib = new EaseEditor;
   lib->metadata.authorName = "Ease";
   lib->metadata.moduleName = "Editor";
   lib->metadata.version = 1;
   return lib;
}

DYLIB_API void Destroy(Ease::BaseModule* lib)
{
   delete reinterpret_cast<EaseEditor*>(lib);
}

